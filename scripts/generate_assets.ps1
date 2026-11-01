Add-Type -AssemblyName System.Drawing
$dir = "d:\code\NewPilot\packaging\resources\Images"
if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force -Path $dir | Out-Null }

function Make-Logo($name, $w, $h) {
    $bmp = New-Object System.Drawing.Bitmap($w, $h, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
    $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
    $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
    
    # 100% Transparent canvas
    $g.Clear([System.Drawing.Color]::Transparent)
    
    # Calculate padding and outer dimensions matching aspect ratio (w x h)
    $minSide = [math]::Min($w, $h)
    $pad = [math]::Max(1.0, [float]$minSide * 0.04)
    $rectW = [float]$w - (2.0 * $pad)
    $rectH = [float]$h - (2.0 * $pad)
    $radius = [float]$minSide * 0.22 # Modern Windows 11 rounded corner radius
    
    $badgePath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $diameter = $radius * 2.0
    
    # 4 rounded corners of the (w x h) shape
    $badgePath.AddArc($pad, $pad, $diameter, $diameter, 180, 90)
    $badgePath.AddArc($pad + $rectW - $diameter, $pad, $diameter, $diameter, 270, 90)
    $badgePath.AddArc($pad + $rectW - $diameter, $pad + $rectH - $diameter, $diameter, $diameter, 0, 90)
    $badgePath.AddArc($pad, $pad + $rectH - $diameter, $diameter, $diameter, 90, 90)
    $badgePath.CloseFigure()
    
    # Pure White background
    $bgBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 255, 255))
    $g.FillPath($bgBrush, $badgePath)
    
    # Subtle border for light backgrounds
    $pen = New-Object System.Drawing.Pen([System.Drawing.Color]::FromArgb(28, 0, 0, 0), [math]::Max(1.0, [float]$minSide * 0.015))
    $g.DrawPath($pen, $badgePath)
    $pen.Dispose()
    $badgePath.Dispose()
    
    # Vector glyph outline for mathematically perfect centering
    $fontFamily = New-Object System.Drawing.FontFamily("Segoe UI")
    $emSize = [float]$minSide * 0.48
    $textPath = New-Object System.Drawing.Drawing2D.GraphicsPath
    $sf = [System.Drawing.StringFormat]::GenericTypographic
    $textPath.AddString("NP", $fontFamily, [int][System.Drawing.FontStyle]::Bold, $emSize, (New-Object System.Drawing.PointF(0, 0)), $sf)
    
    $bounds = $textPath.GetBounds()
    $targetX = ([float]$w - $bounds.Width) / 2.0 - $bounds.X
    $targetY = ([float]$h - $bounds.Height) / 2.0 - $bounds.Y
    
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
    
    $outPath = Join-Path $dir $name
    $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
    $bmp.Dispose()
}

Make-Logo "Square150x150Logo.png" 150 150
Make-Logo "Square44x44Logo.png" 44 44
Make-Logo "Square71x71Logo.png" 71 71
Make-Logo "Square310x310Logo.png" 310 310
Make-Logo "Wide310x150Logo.png" 310 150
Make-Logo "StoreLogo.png" 50 50
Make-Logo "StoreLogo300.png" 300 300
Write-Host "Modern rounded NP logo assets generated successfully with pure black text and exact vector centering."
