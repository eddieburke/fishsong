"""Font neural network trainer with serif/non-serif tagging and vector glyph inputs.

This module provides a self-contained workflow that:
- Builds datasets on the fly from system fonts (Windows Fonts or HTML5/OS fonts).
- Encodes vector outlines + kerning metadata + 256x256 SDFs.
- Trains a multi-task model for fast convergence and serif classification.
- Generates missing glyphs and exports outputs as SDF images or numpy arrays.
- Exposes minimal FastAPI endpoints for a future Web UI.
"""
from __future__ import annotations

import argparse
import json
import math
import os
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, List, Tuple

import numpy as np
import torch
from torch import nn
from torch.utils.data import DataLoader, Dataset

try:
    from fontTools.ttLib import TTFont
except ImportError:  # pragma: no cover
    TTFont = None

try:
    from PIL import Image, ImageDraw, ImageFont
except ImportError:  # pragma: no cover
    Image = None
    ImageDraw = None
    ImageFont = None

try:  # optional for distance transform
    from scipy.ndimage import distance_transform_edt
except ImportError:  # pragma: no cover
    distance_transform_edt = None

try:  # optional web endpoint
    from fastapi import FastAPI
    from fastapi.responses import JSONResponse
except ImportError:  # pragma: no cover
    FastAPI = None
    JSONResponse = None

try:
    import requests
except ImportError:  # pragma: no cover
    requests = None


@dataclass
class GlyphSample:
    glyph_id: int
    outline: torch.Tensor
    kerning_context: torch.Tensor
    sdf: torch.Tensor
    serif_label: int
    font_id: int
    outline_mask: torch.Tensor | None = None


@dataclass
class TrainingConfig:
    batch_size: int = 32
    epochs: int = 15
    lr: float = 3e-4
    weight_decay: float = 1e-2
    serif_loss_weight: float = 0.5
    reconstruction_weight: float = 1.0
    consistency_weight: float = 0.2
    consistency_steps: int = 1


class GlyphVectorDataset(Dataset):
    """Dataset that stores glyph vector outlines, kerning metadata, and SDFs."""

    def __init__(self, npz_path: Path) -> None:
        data = np.load(npz_path, allow_pickle=True)
        self.outlines = data["outlines"]
        self.kerning = data["kerning"]
        self.sdfs = data["sdfs"]
        self.serif_labels = data["serif_labels"]
        self.font_ids = data["font_ids"]
        self.glyph_ids = data["glyph_ids"]
        self.outline_masks = data.get("outline_masks")

    def __len__(self) -> int:
        return len(self.outlines)

    def __getitem__(self, idx: int) -> GlyphSample:
        outline_mask = None
        if self.outline_masks is not None:
            outline_mask = torch.tensor(self.outline_masks[idx], dtype=torch.float32)
        return GlyphSample(
            glyph_id=int(self.glyph_ids[idx]),
            outline=torch.tensor(self.outlines[idx], dtype=torch.float32),
            kerning_context=torch.tensor(self.kerning[idx], dtype=torch.float32),
            sdf=torch.tensor(self.sdfs[idx], dtype=torch.float32),
            serif_label=int(self.serif_labels[idx]),
            font_id=int(self.font_ids[idx]),
            outline_mask=outline_mask,
        )


class OutlineEncoder(nn.Module):
    """Encodes vector outlines into a glyph embedding."""

    def __init__(self, input_dim: int, hidden_dim: int) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(input_dim, hidden_dim),
            nn.GELU(),
            nn.LayerNorm(hidden_dim),
            nn.Linear(hidden_dim, hidden_dim),
            nn.GELU(),
        )

    def forward(self, outline: torch.Tensor) -> torch.Tensor:
        return self.net(outline)


class KerningEncoder(nn.Module):
    """Encodes kerning context and font metadata."""

    def __init__(self, kerning_dim: int, hidden_dim: int) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(kerning_dim, hidden_dim),
            nn.GELU(),
            nn.LayerNorm(hidden_dim),
        )

    def forward(self, kerning_context: torch.Tensor) -> torch.Tensor:
        return self.net(kerning_context)


class SDFEncoder(nn.Module):
    """Encodes 256x256 SDF images into embeddings."""

    def __init__(self, hidden_dim: int) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Conv2d(1, 16, kernel_size=5, stride=2, padding=2),
            nn.GELU(),
            nn.Conv2d(16, 32, kernel_size=5, stride=2, padding=2),
            nn.GELU(),
            nn.Conv2d(32, 64, kernel_size=5, stride=2, padding=2),
            nn.GELU(),
            nn.Flatten(),
            nn.Linear(64 * 32 * 32, hidden_dim),
            nn.GELU(),
        )

    def forward(self, sdf: torch.Tensor) -> torch.Tensor:
        return self.net(sdf)


class FontStyleHead(nn.Module):
    """Predicts serif vs non-serif label."""

    def __init__(self, hidden_dim: int) -> None:
        super().__init__()
        self.net = nn.Sequential(
            nn.Linear(hidden_dim, hidden_dim // 2),
            nn.GELU(),
            nn.Linear(hidden_dim // 2, 2),
        )

    def forward(self, embedding: torch.Tensor) -> torch.Tensor:
        return self.net(embedding)


class SDFDecoder(nn.Module):
    """Decodes font embedding + glyph id into a 256x256 SDF."""

    def __init__(self, hidden_dim: int, glyph_vocab: int) -> None:
        super().__init__()
        self.glyph_embed = nn.Embedding(glyph_vocab, hidden_dim)
        self.net = nn.Sequential(
            nn.Linear(hidden_dim * 2, hidden_dim * 4),
            nn.GELU(),
            nn.Linear(hidden_dim * 4, 64 * 32 * 32),
            nn.GELU(),
        )
        self.deconv = nn.Sequential(
            nn.ConvTranspose2d(64, 32, kernel_size=4, stride=2, padding=1),
            nn.GELU(),
            nn.ConvTranspose2d(32, 16, kernel_size=4, stride=2, padding=1),
            nn.GELU(),
            nn.ConvTranspose2d(16, 1, kernel_size=4, stride=2, padding=1),
        )

    def forward(self, font_embedding: torch.Tensor, glyph_id: torch.Tensor) -> torch.Tensor:
        glyph_embedding = self.glyph_embed(glyph_id)
        merged = torch.cat([font_embedding, glyph_embedding], dim=-1)
        features = self.net(merged).view(-1, 64, 32, 32)
        return self.deconv(features)


class FontTrainer(nn.Module):
    """Multi-task network combining outline + kerning + SDF + style tagging."""

    def __init__(self, outline_dim: int, kerning_dim: int, hidden_dim: int, glyph_vocab: int) -> None:
        super().__init__()
        self.outline_encoder = OutlineEncoder(outline_dim, hidden_dim)
        self.kerning_encoder = KerningEncoder(kerning_dim, hidden_dim)
        self.sdf_encoder = SDFEncoder(hidden_dim)
        self.fusion = nn.Sequential(
            nn.Linear(hidden_dim * 3, hidden_dim),
            nn.GELU(),
            nn.LayerNorm(hidden_dim),
        )
        self.style_head = FontStyleHead(hidden_dim)
        self.decoder = SDFDecoder(hidden_dim, glyph_vocab)

    def forward(
        self,
        outline: torch.Tensor,
        kerning_context: torch.Tensor,
        sdf: torch.Tensor,
        glyph_id: torch.Tensor,
    ) -> Tuple[torch.Tensor, torch.Tensor, torch.Tensor]:
        outline_emb = self.outline_encoder(outline)
        kerning_emb = self.kerning_encoder(kerning_context)
        sdf_emb = self.sdf_encoder(sdf)
        merged = self.fusion(torch.cat([outline_emb, kerning_emb, sdf_emb], dim=-1))
        style_logits = self.style_head(merged)
        reconstructed = self.decoder(merged, glyph_id)
        return merged, style_logits, reconstructed


class TrainState:
    """Mutable training state for logging and web endpoints."""

    def __init__(self) -> None:
        self.epoch = 0
        self.loss = 0.0
        self.serif_loss = 0.0
        self.recon_loss = 0.0
        self.consistency_loss = 0.0

    def to_dict(self) -> dict:
        return {
            "epoch": self.epoch,
            "loss": self.loss,
            "serif_loss": self.serif_loss,
            "recon_loss": self.recon_loss,
            "consistency_loss": self.consistency_loss,
        }


def build_dataloader(dataset: Dataset, batch_size: int) -> DataLoader:
    return DataLoader(dataset, batch_size=batch_size, shuffle=True, drop_last=True)


def training_step(
    model: FontTrainer,
    batch: List[GlyphSample],
    optimizer: torch.optim.Optimizer,
    config: TrainingConfig,
) -> Tuple[float, float, float, float]:
    outlines = torch.stack([sample.outline for sample in batch])
    kernings = torch.stack([sample.kerning_context for sample in batch])
    sdfs = torch.stack([sample.sdf for sample in batch])
    serif_labels = torch.tensor([sample.serif_label for sample in batch], dtype=torch.long)
    glyph_ids = torch.tensor([sample.glyph_id for sample in batch], dtype=torch.long)
    _, style_logits, reconstructed = model(outlines, kernings, sdfs, glyph_ids)

    serif_loss = nn.CrossEntropyLoss()(style_logits, serif_labels)
    reconstruction_loss = nn.MSELoss()(reconstructed, sdfs)

    consistency_loss = torch.tensor(0.0)
    if config.consistency_weight > 0.0 and config.consistency_steps > 0:
        for _ in range(config.consistency_steps):
            _, style_logits_aug, reconstructed_aug = model(outlines, kernings, sdfs, glyph_ids)
            serif_consistency = nn.MSELoss()(style_logits, style_logits_aug.detach())
            reconstruction_consistency = nn.MSELoss()(reconstructed, reconstructed_aug.detach())
            consistency_loss = consistency_loss + serif_consistency + reconstruction_consistency
        consistency_loss = consistency_loss / config.consistency_steps

    loss = (
        config.serif_loss_weight * serif_loss
        + config.reconstruction_weight * reconstruction_loss
        + config.consistency_weight * consistency_loss
    )

    optimizer.zero_grad()
    loss.backward()
    optimizer.step()

    return loss.item(), serif_loss.item(), reconstruction_loss.item(), consistency_loss.item()


def train_font_model(
    dataset_path: Path,
    outline_dim: int,
    kerning_dim: int,
    glyph_vocab: int,
    config: TrainingConfig | None = None,
    state: TrainState | None = None,
) -> FontTrainer:
    config = config or TrainingConfig()
    dataset = GlyphVectorDataset(dataset_path)
    model = FontTrainer(outline_dim, kerning_dim, hidden_dim=256, glyph_vocab=glyph_vocab)

    optimizer = torch.optim.AdamW(model.parameters(), lr=config.lr, weight_decay=config.weight_decay)
    loader = build_dataloader(dataset, config.batch_size)

    model.train()
    for epoch in range(config.epochs):
        running_loss = 0.0
        last_serif = 0.0
        last_recon = 0.0
        last_consistency = 0.0
        for batch in loader:
            loss, serif_loss, recon_loss, consistency_loss = training_step(model, batch, optimizer, config)
            running_loss += loss
            last_serif = serif_loss
            last_recon = recon_loss
            last_consistency = consistency_loss
        avg_loss = running_loss / len(loader)
        if state is not None:
            state.epoch = epoch + 1
            state.loss = avg_loss
            state.serif_loss = last_serif
            state.recon_loss = last_recon
            state.consistency_loss = last_consistency
        print(
            "epoch="
            f"{epoch + 1} loss={avg_loss:.4f} serif={last_serif:.4f} recon={last_recon:.4f} "
            f"consistency={last_consistency:.4f}"
        )

    return model


def generate_font_from_few_shots(
    model: FontTrainer,
    seed_glyphs: Iterable[Tuple[int, np.ndarray, np.ndarray, np.ndarray]],
    target_glyph_ids: Iterable[int],
) -> List[np.ndarray]:
    """Generate missing glyphs using a mean font embedding from few shots.

    seed_glyphs: iterable of (glyph_id, outline_vec, kerning_vec, sdf)
    target_glyph_ids: glyph ids to generate.
    """
    model.eval()
    embeddings = []
    with torch.no_grad():
        for glyph_id, outline_vec, kerning_vec, sdf in seed_glyphs:
            outline = torch.tensor(outline_vec, dtype=torch.float32).unsqueeze(0)
            kerning = torch.tensor(kerning_vec, dtype=torch.float32).unsqueeze(0)
            sdf_tensor = torch.tensor(sdf, dtype=torch.float32).unsqueeze(0)
            glyph_tensor = torch.tensor([glyph_id], dtype=torch.long)
            embedding, _, _ = model(outline, kerning, sdf_tensor, glyph_tensor)
            embeddings.append(embedding.squeeze(0))
        font_embedding = torch.stack(embeddings).mean(dim=0, keepdim=True)

        generated = []
        for glyph_id in target_glyph_ids:
            glyph_tensor = torch.tensor([glyph_id], dtype=torch.long)
            sdf_out = model.decoder(font_embedding, glyph_tensor)
            generated.append(sdf_out.squeeze(0).cpu().numpy())

    return generated


def generate_outputs(
    model: FontTrainer,
    seed_glyphs: Iterable[Tuple[int, np.ndarray, np.ndarray, np.ndarray]],
    target_glyph_ids: Iterable[int],
    output_dir: Path,
) -> List[Path]:
    """Generate SDF outputs and store them as .npy + .png files."""
    output_dir.mkdir(parents=True, exist_ok=True)
    generated = generate_font_from_few_shots(model, seed_glyphs, target_glyph_ids)
    paths: List[Path] = []
    for glyph_id, sdf in zip(target_glyph_ids, generated):
        sdf_path = output_dir / f"glyph_{glyph_id}.npy"
        np.save(sdf_path, sdf)
        paths.append(sdf_path)
        save_sdf_image(sdf, output_dir / f"glyph_{glyph_id}.png")
    return paths


def generate_single_glyph(
    model: FontTrainer,
    seed_glyphs: Iterable[Tuple[int, np.ndarray, np.ndarray, np.ndarray]],
    target_glyph_id: int,
) -> np.ndarray:
    """Generate a single glyph SDF given seed glyphs for style."""
    generated = generate_font_from_few_shots(model, seed_glyphs, [target_glyph_id])
    return generated[0]


def image_to_sdf(image_path: Path, canvas: int = 256) -> np.ndarray:
    if Image is None:
        raise RuntimeError("Pillow is required to load images.")
    image = Image.open(image_path).convert("L").resize((canvas, canvas))
    bitmap = np.array(image) > 0
    sdf = compute_sdf(bitmap)
    return sdf[None, ...]


def save_sdf_image(sdf: np.ndarray, output_path: Path) -> None:
    if Image is None:
        return
    normalized = (sdf - sdf.min()) / (sdf.max() - sdf.min() + 1e-6)
    image = Image.fromarray((normalized.squeeze() * 255).astype(np.uint8), mode="L")
    image.save(output_path)


def list_system_fonts() -> List[Path]:
    fonts: List[Path] = []
    if os.name == "nt":
        windows_fonts = Path(os.environ.get("WINDIR", "C:\\Windows")) / "Fonts"
        if windows_fonts.exists():
            fonts.extend(windows_fonts.glob("*.ttf"))
            fonts.extend(windows_fonts.glob("*.otf"))
    else:
        for path in ("/usr/share/fonts", "/usr/local/share/fonts", str(Path.home() / ".fonts")):
            root = Path(path)
            if root.exists():
                fonts.extend(root.rglob("*.ttf"))
                fonts.extend(root.rglob("*.otf"))
    return fonts


def download_google_fonts(output_dir: Path) -> List[Path]:
    if requests is None:
        return []
    output_dir.mkdir(parents=True, exist_ok=True)
    font_urls = {
        "Roboto-Regular.ttf": "https://fonts.gstatic.com/s/roboto/v30/KFOmCnqEu92Fr1Mu4mxP.ttf",
        "Merriweather-Regular.ttf": "https://fonts.gstatic.com/s/merriweather/v30/u-440qyriQwlOrhSvowK_l5-eRZRe.ttf",
    }
    downloaded: List[Path] = []
    for name, url in font_urls.items():
        target = output_dir / name
        if target.exists():
            downloaded.append(target)
            continue
        try:
            response = requests.get(url, timeout=20)
            response.raise_for_status()
            target.write_bytes(response.content)
            downloaded.append(target)
        except Exception:
            continue
    return downloaded


def serif_label_from_font(font: TTFont) -> int:
    if "OS/2" in font:
        class_value = font["OS/2"].sFamilyClass
        serif_class = class_value >> 8
        if serif_class in {1, 2, 3, 4, 5, 7}:
            return 1
    name = ""
    if "name" in font:
        try:
            name = font["name"].getDebugName(1) or ""
        except Exception:
            name = ""
    lower = name.lower()
    if "serif" in lower and "sans" not in lower:
        return 1
    return 0


def chamfer_distance(mask: np.ndarray) -> np.ndarray:
    height, width = mask.shape
    max_distance = height + width
    distances = np.where(mask, 0.0, max_distance).astype(np.float32)

    for y in range(height):
        for x in range(width):
            value = distances[y, x]
            if y > 0:
                value = min(value, distances[y - 1, x] + 1)
            if x > 0:
                value = min(value, distances[y, x - 1] + 1)
            if y > 0 and x > 0:
                value = min(value, distances[y - 1, x - 1] + math.sqrt(2))
            distances[y, x] = value

    for y in range(height - 1, -1, -1):
        for x in range(width - 1, -1, -1):
            value = distances[y, x]
            if y + 1 < height:
                value = min(value, distances[y + 1, x] + 1)
            if x + 1 < width:
                value = min(value, distances[y, x + 1] + 1)
            if y + 1 < height and x + 1 < width:
                value = min(value, distances[y + 1, x + 1] + math.sqrt(2))
            distances[y, x] = value

    return distances


def compute_sdf(bitmap: np.ndarray) -> np.ndarray:
    mask = bitmap.astype(bool)
    if distance_transform_edt is not None:
        outside = distance_transform_edt(~mask)
        inside = distance_transform_edt(mask)
    else:
        outside = chamfer_distance(~mask)
        inside = chamfer_distance(mask)
    sdf = inside - outside
    sdf = np.clip(sdf / (np.max(np.abs(sdf)) + 1e-6), -1.0, 1.0)
    return sdf.astype(np.float32)


def render_glyph_sdf(font_path: Path, char: str, size: int = 220, canvas: int = 256) -> np.ndarray:
    if ImageFont is None:
        return np.zeros((1, canvas, canvas), dtype=np.float32)
    font = ImageFont.truetype(str(font_path), size=size)
    image = Image.new("L", (canvas, canvas), color=0)
    draw = ImageDraw.Draw(image)
    bbox = draw.textbbox((0, 0), char, font=font)
    offset_x = (canvas - (bbox[2] - bbox[0])) // 2 - bbox[0]
    offset_y = (canvas - (bbox[3] - bbox[1])) // 2 - bbox[1]
    draw.text((offset_x, offset_y), char, fill=255, font=font)
    bitmap = np.array(image) > 0
    sdf = compute_sdf(bitmap)
    return sdf[None, ...]


def extract_glyph_outline_vector(font: TTFont, glyph_name: str, max_points: int) -> Tuple[np.ndarray, np.ndarray]:
    if "glyf" not in font:
        return np.zeros((max_points * 2,), dtype=np.float32), np.zeros((max_points * 2,), dtype=np.float32)
    glyf = font["glyf"]
    if glyph_name not in glyf:
        return np.zeros((max_points * 2,), dtype=np.float32), np.zeros((max_points * 2,), dtype=np.float32)
    glyph = glyf[glyph_name]
    if glyph.isComposite():
        coordinates = np.zeros((0, 2))
    else:
        coordinates = glyph.getCoordinates(glyf)[0] if glyph.numberOfContours else np.zeros((0, 2))
    coords = coordinates.flatten().astype(np.float32)
    outline = np.zeros((max_points * 2,), dtype=np.float32)
    mask = np.zeros((max_points * 2,), dtype=np.float32)
    count = min(len(coords), max_points * 2)
    outline[:count] = coords[:count]
    mask[:count] = 1.0
    return outline, mask


def extract_kerning_vector(font: TTFont, glyph_name: str) -> np.ndarray:
    metrics = []
    if "kern" in font:
        for table in font["kern"].kernTables:
            for (left, right), value in table.kernTable.items():
                if left == glyph_name or right == glyph_name:
                    metrics.append(value)
    if metrics:
        values = np.array(metrics, dtype=np.float32)
        stats = np.array([values.mean(), values.std(), values.min(), values.max(), len(values)], dtype=np.float32)
    else:
        stats = np.zeros((5,), dtype=np.float32)

    meta = np.zeros((5,), dtype=np.float32)
    if "OS/2" in font:
        os2 = font["OS/2"]
        meta = np.array(
            [
                os2.sTypoAscender,
                os2.sTypoDescender,
                os2.usWeightClass,
                os2.usWidthClass,
                os2.sFamilyClass,
            ],
            dtype=np.float32,
        )
    return np.concatenate([stats, meta], axis=0)


def create_dataset_from_fonts(
    output_path: Path,
    fonts: List[Path],
    glyphs: str,
    outline_dim: int = 256,
) -> Tuple[int, int]:
    if TTFont is None:
        raise RuntimeError("fontTools is required to build datasets.")
    kerning_dim = 10
    glyph_vocab = len(glyphs)

    outlines: List[np.ndarray] = []
    masks: List[np.ndarray] = []
    kernings: List[np.ndarray] = []
    sdfs: List[np.ndarray] = []
    serif_labels: List[int] = []
    font_ids: List[int] = []
    glyph_ids: List[int] = []

    for font_index, font_path in enumerate(fonts):
        try:
            font = TTFont(str(font_path))
        except Exception:
            continue
        cmap = font.getBestCmap() or {}
        serif = serif_label_from_font(font)

        for glyph_index, char in enumerate(glyphs):
            code = ord(char)
            if code not in cmap:
                continue
            glyph_name = cmap[code]
            outline, mask = extract_glyph_outline_vector(font, glyph_name, max_points=outline_dim // 2)
            outlines.append(outline)
            masks.append(mask)
            kernings.append(extract_kerning_vector(font, glyph_name))
            sdfs.append(render_glyph_sdf(font_path, char))
            serif_labels.append(serif)
            font_ids.append(font_index)
            glyph_ids.append(glyph_index)

    if not outlines:
        raise RuntimeError("No glyphs were extracted from the available fonts.")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    np.savez(
        output_path,
        outlines=np.stack(outlines, axis=0),
        outline_masks=np.stack(masks, axis=0),
        kerning=np.stack(kernings, axis=0),
        sdfs=np.stack(sdfs, axis=0),
        serif_labels=np.array(serif_labels, dtype=np.int64),
        font_ids=np.array(font_ids, dtype=np.int64),
        glyph_ids=np.array(glyph_ids, dtype=np.int64),
    )

    return outline_dim, kerning_dim


def create_app(state: TrainState) -> FastAPI:
    if FastAPI is None:
        raise RuntimeError("FastAPI is not available.")
    app = FastAPI()

    @app.get("/status")
    def status() -> JSONResponse:
        return JSONResponse(state.to_dict())

    @app.get("/health")
    def health() -> JSONResponse:
        return JSONResponse({"ok": True})

    return app


def main() -> None:
    parser = argparse.ArgumentParser(description="Vector + SDF font trainer")
    parser.add_argument("--dataset", type=Path, default=Path("data/glyph_vectors.npz"))
    parser.add_argument("--no-auto-dataset", action="store_true")
    parser.add_argument("--epochs", type=int, default=15)
    parser.add_argument("--batch-size", type=int, default=32)
    parser.add_argument("--learning-rate", type=float, default=3e-4)
    parser.add_argument("--weight-decay", type=float, default=1e-2)
    parser.add_argument("--glyphs", type=str, default="ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789")
    parser.add_argument("--output", type=Path, default=Path("outputs"))
    parser.add_argument("--serve", action="store_true")
    parser.add_argument("--seed-char", type=str, default="A")
    parser.add_argument("--target-chars", type=str, default="ABCDEFGHIJKLMNOPQRSTUVWXYZ")
    parser.add_argument("--seed-image", type=Path, default=None)
    args = parser.parse_args()

    auto_dataset = not args.no_auto_dataset
    if auto_dataset and not args.dataset.exists():
        fonts = list_system_fonts()
        if not fonts:
            fonts = download_google_fonts(Path("data/html5_fonts"))
        if not fonts:
            raise RuntimeError("No system or HTML5 fonts found to build dataset.")
        create_dataset_from_fonts(args.dataset, fonts=fonts, glyphs=args.glyphs)

    config = TrainingConfig(
        batch_size=args.batch_size,
        epochs=args.epochs,
        lr=args.learning_rate,
        weight_decay=args.weight_decay,
    )
    state = TrainState()
    dataset = GlyphVectorDataset(args.dataset)
    outline_dim = dataset.outlines.shape[1]
    kerning_dim = dataset.kerning.shape[1]
    glyph_vocab = int(dataset.glyph_ids.max()) + 1

    model = train_font_model(
        dataset_path=args.dataset,
        outline_dim=outline_dim,
        kerning_dim=kerning_dim,
        glyph_vocab=glyph_vocab,
        config=config,
        state=state,
    )

    if args.seed_image is not None:
        sdf = image_to_sdf(args.seed_image)
        outline_seed = np.zeros((outline_dim,), dtype=np.float32)
        kerning_seed = np.zeros((kerning_dim,), dtype=np.float32)
        seed_id = args.glyphs.find(args.seed_char) if args.seed_char in args.glyphs else 0
        targets = [args.glyphs.find(ch) for ch in args.target_chars if ch in args.glyphs]
        if not targets:
            targets = [seed_id]
        outputs = generate_outputs(
            model,
            seed_glyphs=[(seed_id, outline_seed, kerning_seed, sdf)],
            target_glyph_ids=targets,
            output_dir=args.output,
        )
        print(f"Generated {len(outputs)} glyph(s) from seed image into {args.output}.")
        return

    if args.seed_char in args.glyphs:
        seed_id = args.glyphs.find(args.seed_char)
        matches = np.where(dataset.glyph_ids == seed_id)[0]
        if matches.size > 0:
            seed_outline = dataset.outlines[matches[0]]
            seed_kerning = dataset.kerning[matches[0]]
            seed_sdf = dataset.sdfs[matches[0]]
            targets = [args.glyphs.find(ch) for ch in args.target_chars if ch in args.glyphs]
            if targets:
                outputs = generate_outputs(
                    model,
                    seed_glyphs=[(seed_id, seed_outline, seed_kerning, seed_sdf)],
                    target_glyph_ids=targets,
                    output_dir=args.output,
                )
                print(f"Generated {len(outputs)} glyph(s) from seed '{args.seed_char}' into {args.output}.")
                return

    if args.serve:
        app = create_app(state)
        import uvicorn

        uvicorn.run(app, host="0.0.0.0", port=8000)
    else:
        print(json.dumps(state.to_dict(), indent=2))
        print(f"Model trained. Outputs can be generated to {args.output}.")


if __name__ == "__main__":
    main()
