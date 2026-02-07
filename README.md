# Fishsong

This repository includes a self-contained font neural network trainer that learns serif vs non-serif tags
from vector glyph outlines, kerning metadata, and 256x256 SDFs. It can build datasets automatically from
system fonts or bundled HTML5 font downloads, train quickly, and generate missing glyph outputs with an
optional web endpoint.

## Highlights

- **Direct vector input**: uses outline point vectors with padding masks.
- **SDF rendering**: 256x256 SDFs are generated per glyph and used in training/decoding.
- **Kerning + metadata**: kerning statistics and font metrics are encoded as features.
- **Fast convergence**: multi-task loss with consistency checks.
- **Auto dataset**: scans system fonts and falls back to HTML5 fonts if needed.
- **Web UI endpoints**: optional FastAPI `/status` + `/health` endpoints for dashboards.

## Quick start

1. Ensure Python dependencies (fontTools, Pillow, torch). Optional: scipy, fastapi, uvicorn, requests.
2. Run the trainer (dataset is created automatically if missing):
   ```bash
   python font_trainer.py --epochs 10
   ```
3. Optional: start the web endpoint:
   ```bash
   python font_trainer.py --serve
   ```

## PowerShell 7 script (CLI/GUI)

Use the script for a CLI or GUI-driven setup with learning rate, batch size, glyph set, and more:

```powershell
pwsh ./font_trainer.ps1 -Epochs 20 -BatchSize 16 -LearningRate 0.0002 -Serve
```

```powershell
pwsh ./font_trainer.ps1 -UseGui
```

Disable auto dataset creation if you want to supply your own dataset file:

```powershell
pwsh ./font_trainer.ps1 -NoAutoDataset -Dataset "path/to/glyph_vectors.npz"
```

## Output generation

The trainer can generate missing glyph SDFs and export `.npy` and `.png` outputs via the helper utilities
in `font_trainer.py`. You can wire these into a Web UI or downstream font pipeline.

### Generate a specific glyph or alphabet from a seed

- Generate a specific glyph (e.g., `A`) using the dataset seed:
  ```bash
  python font_trainer.py --seed-char A --target-chars A --output outputs
  ```
- Generate the full alphabet using a seed image of your own `A`:
  ```bash
  python font_trainer.py --seed-image path/to/A.png --seed-char A --target-chars ABCDEFGHIJKLMNOPQRSTUVWXYZ --output outputs
  ```
