Add-Type -AssemblyName System.Drawing
$dir = "d:\code\NewPilot\packaging\resources\Images"
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }

function Make-Logo($name, $w, $h) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    
    # Solid white background
    $g.Clear([System.Drawing.Color]::FromArgb(255, 255, 255, 255))
    $brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 0, 0, 0))
    
    # Calculate starting font size conservatively so "NP" fits inside
    $fontSize = [math]::Max(7, [math]::Min($w, $h) / 3.4)
    
    # Dynamically measure and scale down font size if needed
    while ($fontSize -gt 4) {
        $font = New-Object System.Drawing.Font("Segoe UI", $fontSize, [System.Drawing.FontStyle]::Bold)
        $sz = $g.MeasureString("NP", $font)
        if ($sz.Width -le ($w * 0.75) -and $sz.Height -le ($h * 0.75)) {
            break
        }
        $fontSize = $fontSize - 0.5
    }
    
    $font = New-Object System.Drawing.Font("Segoe UI", $fontSize, [System.Drawing.FontStyle]::Bold)
    $sf = New-Object System.Drawing.StringFormat
    $sf.Alignment = [System.Drawing.StringAlignment]::Center
    $sf.LineAlignment = [System.Drawing.StringAlignment]::Center
    $rect = New-Object System.Drawing.RectangleF(0, 0, $w, $h)
    
    $g.DrawString("NP", $font, $brush, $rect, $sf)
    $outPath = Join-Path $dir $name
    $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $g.Dispose()
    $bmp.Dispose()
}

Make-Logo "Square150x150Logo.png" 150 150
Make-Logo "Square44x44Logo.png" 44 44
Make-Logo "Square71x71Logo.png" 71 71
Make-Logo "Square310x310Logo.png" 310 310
Make-Logo "Wide310x150Logo.png" 310 150
Make-Logo "StoreLogo.png" 50 50
Make-Logo "StoreLogo300.png" 300 300
Write-Host "NP logo assets centered and scaled successfully."
