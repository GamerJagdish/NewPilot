Add-Type -AssemblyName System.Drawing
$dir = "d:\code\NewPilot\packaging\resources"
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }
$icoPath = Join-Path $dir "app.ico"

function Render-RoundedBitmap([int]$size) {
    $bmp = New-Object System.Drawing.Bitmap($size, $size, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    
    # Transparent canvas
    $g.Clear([System.Drawing.Color]::Transparent)
    
    # 4% padding margin so rounded corners anti-alias smoothly
    $pad = [math]::Max(1.0, [float]$size * 0.04)
    $rectW = [float]$size - (2.0 * $pad)
    $rectH = [float]$size - (2.0 * $pad)
    $radius = [float]$rectW * 0.22 # Modern Windows 11 squircle radius
    
    $badgePath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = $radius * 2.0
    
    $badgePath.AddArc($pad, $pad, $diameter, $diameter, 180, 90)
    $badgePath.AddArc($pad + $rectW - $diameter, $pad, $diameter, $diameter, 270, 90)
    $badgePath.AddArc($pad + $rectW - $diameter, $pad + $rectH - $diameter, $diameter, $diameter, 0, 90)
    $badgePath.AddArc($pad, $pad + $rectH - $diameter, $diameter, $diameter, 90, 90)
    $badgePath.CloseFigure()
    
    # Fill rounded badge with pure white
    $bgBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 255, 255))
    $g.FillPath($bgBrush, $badgePath)
    
    # Soft subtle border for contrast on light backgrounds
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(28, 0, 0, 0), [math]::Max(1.0, [float]$size * 0.015))
    $g.DrawPath($pen, $badgePath)
    $pen.Dispose()
    $badgePath.Dispose()
    
    # Vector glyph outline for mathematically perfect centering
    $fontFamily = New-Object System.Drawing.FontFamily("Segoe UI")
    $emSize = [float]$size * 0.48
    $textPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $sf = [System.Drawing.StringFormat]::GenericTypographic
    $textPath.AddString("NP", $fontFamily, [int][System.Drawing.FontStyle]::Bold, $emSize, (New-Object System.Drawing.PointF(0, 0)), $sf)
    
    $bounds = $textPath.GetBounds()
    $targetX = ([float]$size - $bounds.Width) / 2.0 - $bounds.X
    $targetY = ([float]$size - $bounds.Height) / 2.0 - $bounds.Y
    
    $matrix = New-Object System.Drawing.Drawing2D.Matrix
    $matrix.Translate($targetX, $targetY)
    $textPath.Transform($matrix)
    $matrix.Dispose()
    
    # PURE BLACK brush (0, 0, 0)
    $blackBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 0, 0, 0))
    $g.FillPath($blackBrush, $textPath)
    
    $textPath.Dispose()
    $fontFamily.Dispose()
    $blackBrush.Dispose()
    $bgBrush.Dispose()
    $g.Dispose()
    
    return $bmp
}

function Build-Ico([System.Drawing.Bitmap[]]$bitmaps, [string]$icoFile) {
    if (Test-Path $icoFile) { Remove-Item $icoFile -Force }
    $fs = [System.IO.File]::Create($icoFile)
    $bw = New-Object System.IO.BinaryWriter($fs)
    
    # ICONDIR
    $bw.Write([UInt16]0) # Reserved
    $bw.Write([UInt16]1) # Type: 1 = ICO
    $bw.Write([UInt16]$bitmaps.Count) # Count
    
    $pngBytesList = New-Object System.Collections.Generic.List[byte[]]
    foreach ($bmp in $bitmaps) {
        $ms = New-Object System.IO.MemoryStream
        $bmp.Save($ms, [System.Drawing.Imaging.ImageFormat]::Png)
        $pngBytesList.Add($ms.ToArray())
        $ms.Dispose()
    }
    
    # Calculate offset: 6 bytes header + 16 bytes per directory entry
    $offset = 6 + (16 * $bitmaps.Count)
    
    for ($i = 0; $i -lt $bitmaps.Count; $i++) {
        $bmp = $bitmaps[$i]
        $bytes = $pngBytesList[$i]
        
        $w = if ($bmp.Width -ge 256) { 0 } else { [byte]$bmp.Width }
        $h = if ($bmp.Height -ge 256) { 0 } else { [byte]$bmp.Height }
        
        $bw.Write([byte]$w)
        $bw.Write([byte]$h)
        $bw.Write([byte]0) # Color count
        $bw.Write([byte]0) # Reserved
        $bw.Write([UInt16]1) # Planes
        $bw.Write([UInt16]32) # Bit count
        $bw.Write([UInt32]$bytes.Length) # Image size
        $bw.Write([UInt32]$offset) # Offset
        
        $offset += $bytes.Length
    }
    
    for ($i = 0; $i -lt $bitmaps.Count; $i++) {
        $bw.Write($pngBytesList[$i])
    }
    
    $bw.Close()
    $fs.Close()
}

$sizes = @(16, 20, 24, 32, 40, 48, 64, 128, 256)
$bitmaps = @()
foreach ($s in $sizes) {
    $bitmaps += Render-RoundedBitmap $s
}

Build-Ico $bitmaps $icoPath

foreach ($b in $bitmaps) { $b.Dispose() }
Write-Host "Generated modern multi-resolution rounded app.ico with pure black text and vector centering at $icoPath"
