[CmdletBinding()]
param(
    [Parameter(Mandatory = $false)]
    [string]$Python = "python",

    [Parameter(Mandatory = $false)]
    [string]$Dataset = "data/glyph_vectors.npz",

    [Parameter(Mandatory = $false)]
    [string]$Output = "outputs",

    [Parameter(Mandatory = $false)]
    [int]$Epochs = 15,

    [Parameter(Mandatory = $false)]
    [int]$BatchSize = 32,

    [Parameter(Mandatory = $false)]
    [double]$LearningRate = 0.0003,

    [Parameter(Mandatory = $false)]
    [double]$WeightDecay = 0.01,

    [Parameter(Mandatory = $false)]
    [string]$Glyphs = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",

    [Parameter(Mandatory = $false)]
    [switch]$Serve,

    [Parameter(Mandatory = $false)]
    [string]$SeedChar = "A",

    [Parameter(Mandatory = $false)]
    [string]$TargetChars = "ABCDEFGHIJKLMNOPQRSTUVWXYZ",

    [Parameter(Mandatory = $false)]
    [string]$SeedImage = "",

    [Parameter(Mandatory = $false)]
    [switch]$NoAutoDataset,

    [Parameter(Mandatory = $false)]
    [switch]$UseGui
)

function Invoke-FontTrainer {
    param(
        [string]$PythonExe,
        [string]$DatasetPath,
        [string]$OutputPath,
        [int]$EpochCount,
        [int]$Batch,
        [double]$LR,
        [double]$WD,
        [string]$GlyphSet,
        [string]$SeedCharValue,
        [string]$TargetCharsValue,
        [string]$SeedImagePath,
        [switch]$ServeMode,
        [switch]$DisableAutoDataset
    )

    $argsList = @(
        "font_trainer.py",
        "--dataset", $DatasetPath,
        "--output", $OutputPath,
        "--epochs", $EpochCount,
        "--batch-size", $Batch,
        "--learning-rate", $LR,
        "--weight-decay", $WD,
        "--glyphs", $GlyphSet,
        "--seed-char", $SeedCharValue,
        "--target-chars", $TargetCharsValue
    )
    if ($SeedImagePath -ne "") {
        $argsList += "--seed-image"
        $argsList += $SeedImagePath
    }

    if ($ServeMode) {
        $argsList += "--serve"
    }
    if ($DisableAutoDataset) {
        $argsList += "--no-auto-dataset"
    }

    Write-Host "Running: $PythonExe $($argsList -join ' ')" -ForegroundColor Cyan
    & $PythonExe @argsList
}

if ($UseGui) {
    try {
        Add-Type -AssemblyName System.Windows.Forms
    } catch {
        Write-Warning "System.Windows.Forms not available; falling back to CLI parameters."
        $UseGui = $false
    }
}

if ($UseGui) {
    $form = New-Object System.Windows.Forms.Form
    $form.Text = "Font Trainer"
    $form.Width = 460
    $form.Height = 420

    $labels = @(
        "Dataset", "Output", "Epochs", "Batch Size", "Learning Rate", "Weight Decay", "Glyphs",
        "Seed Char", "Target Chars", "Seed Image"
    )
    $defaults = @(
        $Dataset, $Output, $Epochs, $BatchSize, $LearningRate, $WeightDecay, $Glyphs,
        $SeedChar, $TargetChars, $SeedImage
    )

    $textBoxes = @()
    for ($i = 0; $i -lt $labels.Count; $i++) {
        $label = New-Object System.Windows.Forms.Label
        $label.Text = $labels[$i]
        $label.Left = 20
        $label.Top = 20 + ($i * 45)
        $label.Width = 120
        $form.Controls.Add($label)

        $textBox = New-Object System.Windows.Forms.TextBox
        $textBox.Left = 150
        $textBox.Top = 18 + ($i * 45)
        $textBox.Width = 260
        $textBox.Text = $defaults[$i]
        $form.Controls.Add($textBox)
        $textBoxes += $textBox
    }

    $serveCheckbox = New-Object System.Windows.Forms.CheckBox
    $serveCheckbox.Left = 150
    $serveCheckbox.Top = 18 + ($labels.Count * 45)
    $serveCheckbox.Text = "Start Web UI endpoint"
    $serveCheckbox.Width = 240
    $form.Controls.Add($serveCheckbox)

    $autoCheckbox = New-Object System.Windows.Forms.CheckBox
    $autoCheckbox.Left = 150
    $autoCheckbox.Top = 18 + (($labels.Count + 1) * 45)
    $autoCheckbox.Text = "Disable auto dataset creation"
    $autoCheckbox.Width = 240
    $form.Controls.Add($autoCheckbox)

    $button = New-Object System.Windows.Forms.Button
    $button.Text = "Run"
    $button.Left = 150
    $button.Top = 18 + (($labels.Count + 2) * 45)
    $button.Width = 120
    $button.Add_Click({
        $Dataset = $textBoxes[0].Text
        $Output = $textBoxes[1].Text
        $Epochs = [int]$textBoxes[2].Text
        $BatchSize = [int]$textBoxes[3].Text
        $LearningRate = [double]$textBoxes[4].Text
        $WeightDecay = [double]$textBoxes[5].Text
        $Glyphs = $textBoxes[6].Text
        $SeedChar = $textBoxes[7].Text
        $TargetChars = $textBoxes[8].Text
        $SeedImage = $textBoxes[9].Text
        $Serve = $serveCheckbox.Checked
        $NoAutoDataset = $autoCheckbox.Checked
        $form.Close()
    })
    $form.Controls.Add($button)

    $form.ShowDialog() | Out-Null
}

Invoke-FontTrainer \
    -PythonExe $Python \
    -DatasetPath $Dataset \
    -OutputPath $Output \
    -EpochCount $Epochs \
    -Batch $BatchSize \
    -LR $LearningRate \
    -WD $WeightDecay \
    -GlyphSet $Glyphs \
    -SeedCharValue $SeedChar \
    -TargetCharsValue $TargetChars \
    -SeedImagePath $SeedImage \
    -ServeMode:$Serve \
    -DisableAutoDataset:$NoAutoDataset
