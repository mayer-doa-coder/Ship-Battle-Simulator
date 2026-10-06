param(
    [string]$PresentationPath = "$(Split-Path -Parent $PSScriptRoot)\Blue Playful Ships Masters Of The Seas Presentation.pptx"
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$media = Join-Path $root 'presentation_media'
$backup = Join-Path $root 'Blue Playful Ships Masters Of The Seas Presentation.backup.pptx'

function Rgb([int]$r, [int]$g, [int]$b) {
    return [int]($r + 256 * $g + 65536 * $b)
}

$C = @{
    Navy       = Rgb 24 40 72
    Deep       = Rgb 11 23 43
    Blue       = Rgb 62 88 147
    MidBlue    = Rgb 88 127 190
    Sky        = Rgb 123 148 204
    Pale       = Rgb 246 245 255
    White      = Rgb 255 255 255
    Ink        = Rgb 35 49 78
    Muted      = Rgb 88 101 128
    Gold       = Rgb 255 185 64
    Coral      = Rgb 219 59 45
    Teal       = Rgb 37 154 170
    Green      = Rgb 69 154 94
    DarkGold   = Rgb 112 74 28
}

$msoTrue = -1
$msoFalse = 0
$msoTextOrientationHorizontal = 1
$msoShapeRectangle = 1
$msoShapeRoundedRectangle = 5
$msoShapeOval = 9
$ppLayoutBlank = 12
$ppAlignLeft = 1
$ppAlignCenter = 2
$ppAlignRight = 3
$ppAnchorTop = 1
$ppAnchorMiddle = 3

function Add-Rect($slide, [float]$x, [float]$y, [float]$w, [float]$h, [int]$fill,
                  [float]$transparency = 0, [int]$line = -1, [float]$lineWeight = 1,
                  [switch]$Rounded) {
    $kind = if ($Rounded) { $msoShapeRoundedRectangle } else { $msoShapeRectangle }
    $shape = $slide.Shapes.AddShape($kind, $x, $y, $w, $h)
    $shape.Fill.Visible = $msoTrue
    $shape.Fill.Solid()
    $shape.Fill.ForeColor.RGB = $fill
    $shape.Fill.Transparency = [math]::Max(0.0, [math]::Min(1.0, $transparency / 100.0))
    if ($line -lt 0) {
        $shape.Line.Visible = $msoFalse
    } else {
        $shape.Line.Visible = $msoTrue
        $shape.Line.ForeColor.RGB = $line
        $shape.Line.Weight = $lineWeight
    }
    return $shape
}

function Add-Circle($slide, [float]$x, [float]$y, [float]$d, [int]$fill, [float]$transparency = 0) {
    $shape = $slide.Shapes.AddShape($msoShapeOval, $x, $y, $d, $d)
    $shape.Fill.Solid()
    $shape.Fill.ForeColor.RGB = $fill
    $shape.Fill.Transparency = [math]::Max(0.0, [math]::Min(1.0, $transparency / 100.0))
    $shape.Line.Visible = $msoFalse
    return $shape
}

function Add-Text($slide, [string]$text, [float]$x, [float]$y, [float]$w, [float]$h,
                  [float]$size, [int]$color, [switch]$Bold,
                  [ValidateSet('Left','Center','Right')] [string]$Align = 'Left',
                  [string]$Font = 'Aptos', [switch]$Middle) {
    $shape = $slide.Shapes.AddTextbox($msoTextOrientationHorizontal, $x, $y, $w, $h)
    $shape.Line.Visible = $msoFalse
    $shape.Fill.Visible = $msoFalse
    $shape.TextFrame2.WordWrap = $msoTrue
    $shape.TextFrame2.AutoSize = 0
    $shape.TextFrame2.MarginLeft = 0
    $shape.TextFrame2.MarginRight = 0
    $shape.TextFrame2.MarginTop = 0
    $shape.TextFrame2.MarginBottom = 0
    $shape.TextFrame2.VerticalAnchor = if ($Middle) { $ppAnchorMiddle } else { $ppAnchorTop }
    $range = $shape.TextFrame2.TextRange
    $range.Text = $text
    $range.Font.Name = $Font
    $range.Font.Size = $size
    $range.Font.Bold = if ($Bold) { $msoTrue } else { $msoFalse }
    $range.Font.Fill.Visible = $msoTrue
    $range.Font.Fill.ForeColor.RGB = $color
    $range.ParagraphFormat.Alignment = switch ($Align) {
        'Center' { $ppAlignCenter }
        'Right'  { $ppAlignRight }
        default  { $ppAlignLeft }
    }
    return $shape
}

function Add-Picture($slide, [string]$path, [float]$x, [float]$y, [float]$w, [float]$h,
                     [int]$lineColor = -1, [float]$lineWeight = 1.5) {
    if (-not (Test-Path -LiteralPath $path)) { throw "Missing presentation image: $path" }
    $shape = $slide.Shapes.AddPicture($path, $msoFalse, $msoTrue, $x, $y, $w, $h)
    if ($lineColor -ge 0) {
        $shape.Line.Visible = $msoTrue
        $shape.Line.ForeColor.RGB = $lineColor
        $shape.Line.Weight = $lineWeight
    } else {
        $shape.Line.Visible = $msoFalse
    }
    return $shape
}

function Add-Tag($slide, [string]$text, [float]$x, [float]$y, [float]$w,
                 [int]$fill = $C.Gold, [int]$textColor = $C.Navy) {
    Add-Rect $slide $x $y $w 32 $fill 0 -1 0 -Rounded | Out-Null
    Add-Text $slide $text $x ($y + 1) $w 30 15 $textColor -Bold -Align Center -Middle | Out-Null
}

function Add-LightHeader($slide, [int]$number, [string]$eyebrow, [string]$title) {
    Add-Rect $slide 0 0 1440 810 $C.Pale | Out-Null
    Add-Text $slide $eyebrow 54 32 700 26 16 $C.MidBlue -Bold | Out-Null
    Add-Text $slide $title 54 58 1250 70 44 $C.Blue -Bold | Out-Null
    Add-Text $slide ("{0:D2}" -f $number) 1330 43 58 38 17 $C.Blue -Bold -Align Right | Out-Null
    Add-Rect $slide 54 122 92 6 $C.Gold | Out-Null
    $star = Join-Path $media 'template_star.svg'
    if (Test-Path $star) { Add-Picture $slide $star 1370 31 28 28 | Out-Null }
}

function Add-Footer($slide, [string]$text = 'BROADSIDE  /  OPENGL GRAPHICS SHOWCASE') {
    Add-Text $slide $text 54 777 1040 20 11 $C.Muted -Bold | Out-Null
}

function Add-Metric($slide, [string]$value, [string]$label, [float]$x, [float]$y,
                    [int]$accent = $C.Gold, [int]$text = $C.Ink) {
    Add-Circle $slide $x ($y + 8) 12 $accent | Out-Null
    Add-Text $slide $value ($x + 24) $y 160 44 30 $text -Bold | Out-Null
    Add-Text $slide $label ($x + 24) ($y + 40) 210 38 14 $C.Muted -Bold | Out-Null
}

if (-not (Test-Path -LiteralPath $PresentationPath)) { throw "Presentation not found: $PresentationPath" }
if (-not (Test-Path -LiteralPath $backup)) {
    Copy-Item -LiteralPath $PresentationPath -Destination $backup
}

$img = @{
    Overview   = Join-Path $media 'overview.png'
    Ship       = Join-Path $media 'ship_side.png'
    Storm      = Join-Path $media 'storm.png'
    Night      = Join-Path $media 'night.png'
    Combat     = Join-Path $media 'combat.png'
    Map        = Join-Path $media 'world_map.png'
    RayOff     = Join-Path $media 'ray_off.png'
    RayOn      = Join-Path $media 'ray_on.png'
    Underwater = Join-Path $media 'underwater.png'
    Gallery    = Join-Path $media 'gallery.png'
    Port       = Join-Path $media 'port_nassau.png'
    World      = Join-Path $root 'docs\stage_d_evidence\preset_f1.png'
}

$app = New-Object -ComObject PowerPoint.Application
try {
    $presentation = $app.Presentations.Open((Resolve-Path $PresentationPath).Path, $msoFalse, $msoFalse, $msoFalse)
    for ($i = $presentation.Slides.Count; $i -ge 1; --$i) {
        $presentation.Slides.Item($i).Delete()
    }

    # 1 — Title
    $s = $presentation.Slides.Add(1, $ppLayoutBlank)
    Add-Picture $s $img.Ship 0 0 1440 810 | Out-Null
    Add-Rect $s 0 0 760 810 $C.Deep 18 | Out-Null
    Add-Rect $s 0 0 1440 810 $C.Deep 78 | Out-Null
    Add-Tag $s 'GRAPHICS PROJECT SHOWCASE' 70 82 310 | Out-Null
    Add-Text $s 'BROADSIDE' 68 165 740 110 76 $C.White -Bold | Out-Null
    Add-Text $s 'SHIP BATTLE SIMULATOR' 72 278 610 48 27 $C.Gold -Bold | Out-Null
    Add-Text $s 'A procedural pirate world built with C++17, OpenGL 3.3 Core and GLSL.' 72 350 620 112 28 $C.White | Out-Null
    Add-Rect $s 72 488 156 6 $C.Coral | Out-Null
    Add-Text $s 'Tawhidul Hasan  •  Roll 2107004' 72 520 620 40 20 $C.White -Bold | Out-Null
    Add-Text $s 'Lighting  •  Shading  •  Animation  •  Interaction  •  Hybrid Ray Tracing' 72 585 620 72 17 $C.Sky | Out-Null
    $star = Join-Path $media 'template_star.svg'
    if (Test-Path $star) { Add-Picture $s $star 1328 68 46 46 | Out-Null }
    Add-Text $s '01' 1328 748 58 28 14 $C.White -Bold -Align Right | Out-Null

    # 2 — Introduction
    $s = $presentation.Slides.Add(2, $ppLayoutBlank)
    Add-LightHeader $s 2 'INTRODUCTION' 'A Playable Computer Graphics Showcase'
    Add-Picture $s $img.Overview 570 150 814 458 $C.White 3 | Out-Null
    Add-Rect $s 570 608 814 78 $C.Navy 0 -1 0 | Out-Null
    Add-Text $s 'Sail, fight, trade and explore in one continuously simulated 3D world.' 598 628 758 34 19 $C.White -Bold -Align Center | Out-Null
    Add-Metric $s 'C++17' 'APPLICATION' 62 176 $C.Gold | Out-Null
    Add-Metric $s '3.3' 'OPENGL CORE' 62 278 $C.Coral | Out-Null
    Add-Metric $s '100%' 'PROCEDURAL GEOMETRY' 62 380 $C.Teal | Out-Null
    Add-Rect $s 58 506 450 142 $C.White 0 (Rgb 222 226 241) 1.5 -Rounded | Out-Null
    Add-Text $s 'Design goal' 82 530 180 24 15 $C.MidBlue -Bold | Out-Null
    Add-Text $s 'Make the required graphics theory visible through a real interactive pirate-ship experience.' 82 566 392 64 21 $C.Ink -Bold | Out-Null
    Add-Footer $s

    # 3 — Pipeline and procedural modelling
    $s = $presentation.Slides.Add(3, $ppLayoutBlank)
    Add-LightHeader $s 3 'CORE GRAPHICS' 'Rendering Pipeline & Procedural Modelling'
    Add-Picture $s $img.Ship 54 158 794 447 $C.White 3 | Out-Null
    Add-Tag $s 'NO DOWNLOADED 3D MODELS' 82 181 260 $C.Coral $C.White | Out-Null
    Add-Rect $s 882 154 502 452 $C.White 0 (Rgb 220 225 239) 1.5 -Rounded | Out-Null
    Add-Text $s 'Hierarchical construction' 914 184 430 38 23 $C.Blue -Bold | Out-Null
    Add-Text $s 'Hull, decks, masts, sails, cannons, crew and props are assembled from reusable indexed meshes.' 914 232 430 88 19 $C.Ink | Out-Null
    Add-Text $s 'Model → View → Projection' 914 348 430 30 17 $C.MidBlue -Bold | Out-Null
    Add-Text $s 'pclip = P · V · M · pobject' 914 394 430 55 29 $C.Navy -Bold -Font 'Cambria Math' -Align Center | Out-Null
    Add-Text $s 'Separate rigid frames keep child parts synchronized while avoiding non-uniform scale distortion.' 914 482 430 84 18 $C.Muted | Out-Null
    $steps = @('INPUT','UPDATE Δt','RENDER','SWAP')
    for ($i=0; $i -lt $steps.Count; ++$i) {
        $x = 92 + $i * 310
        Add-Rect $s $x 662 238 54 $(if($i % 2 -eq 0){$C.Blue}else{$C.Navy}) 0 -1 0 -Rounded | Out-Null
        Add-Text $s $steps[$i] $x 673 238 30 17 $C.White -Bold -Align Center | Out-Null
        if ($i -lt 3) { Add-Text $s '→' ($x + 250) 665 48 42 25 $C.Gold -Bold -Align Center | Out-Null }
    }
    Add-Footer $s

    # 4 — Lighting and shading
    $s = $presentation.Slides.Add(4, $ppLayoutBlank)
    Add-LightHeader $s 4 'MANDATORY GRAPHICS CONCEPTS' 'Lighting, Gouraud & Phong Shading'
    Add-Picture $s $img.Gallery 48 150 744 419 $C.White 3 | Out-Null
    Add-Tag $s 'MATERIAL & HIGHLIGHT GALLERY' 76 173 290 $C.Gold $C.Navy | Out-Null
    Add-Rect $s 825 150 558 420 $C.Navy 0 -1 0 -Rounded | Out-Null
    Add-Text $s 'Phong illumination' 860 180 490 34 22 $C.Gold -Bold | Out-Null
    Add-Text $s 'I = Ia + Σ (Id + Is) + Ie' 860 228 490 50 29 $C.White -Bold -Font 'Cambria Math' -Align Center | Out-Null
    Add-Text $s 'Ambient' 862 308 126 24 16 $C.Sky -Bold | Out-Null
    Add-Text $s 'Ia = Iamb · ka' 1000 306 330 28 18 $C.White -Font 'Cambria Math' | Out-Null
    Add-Text $s 'Diffuse' 862 360 126 24 16 $C.Sky -Bold | Out-Null
    Add-Text $s 'Id = IL · kd · max(N·L, 0)' 1000 358 350 28 18 $C.White -Font 'Cambria Math' | Out-Null
    Add-Text $s 'Specular' 862 412 126 24 16 $C.Sky -Bold | Out-Null
    Add-Text $s 'Is = IL · ks · max(R·V, 0)^ns' 1000 410 350 28 18 $C.White -Font 'Cambria Math' | Out-Null
    Add-Text $s 'Gouraud: light per vertex → interpolate colour' 860 474 460 24 16 $C.White | Out-Null
    Add-Text $s 'Phong: interpolate normal → light per fragment' 860 514 460 24 16 $C.White | Out-Null
    Add-Rect $s 48 615 1335 86 $C.White 0 (Rgb 220 225 239) 1.5 -Rounded | Out-Null
    Add-Text $s 'LIVE DEMO' 78 640 150 24 15 $C.Coral -Bold | Out-Null
    Add-Text $s '1  Flat     2  Gouraud     3  Phong     K  lighting terms     B  Blinn/Phong     L  light masks' 222 632 1110 38 21 $C.Ink -Bold -Align Center | Out-Null
    Add-Footer $s

    # 5 — Environment
    $s = $presentation.Slides.Add(5, $ppLayoutBlank)
    Add-LightHeader $s 5 'DYNAMIC ENVIRONMENT' 'Ocean, Weather, Time & Underwater World'
    Add-Picture $s $img.Storm 48 150 812 457 $C.White 3 | Out-Null
    Add-Tag $s 'STORM' 72 174 112 $C.Coral $C.White | Out-Null
    Add-Picture $s $img.Night 900 150 440 248 $C.White 3 | Out-Null
    Add-Tag $s 'NIGHT' 920 170 102 $C.Gold $C.Navy | Out-Null
    Add-Picture $s $img.Underwater 900 430 440 248 $C.White 3 | Out-Null
    Add-Tag $s 'UNDERWATER' 920 450 150 $C.Teal $C.White | Out-Null
    Add-Rect $s 48 625 812 66 $C.Navy 0 -1 0 -Rounded | Out-Null
    Add-Text $s '4 travelling sine waves • rain + wind • lightning • haze • day / sunset / night' 76 644 758 30 18 $C.White -Bold -Align Center | Out-Null
    Add-Text $s 'The same wave table drives CPU ship motion and GLSL sea displacement.' 78 714 760 32 17 $C.Ink -Bold | Out-Null
    Add-Text $s 'Depth visibility, seabed caustics, bubbles and fish create a separate underwater rendering state.' 900 710 440 50 16 $C.Muted | Out-Null
    Add-Footer $s

    # 6 — Ship and crew systems
    $s = $presentation.Slides.Add(6, $ppLayoutBlank)
    Add-Picture $s $img.Ship 0 0 1440 810 | Out-Null
    Add-Rect $s 0 0 1440 112 $C.Deep 12 | Out-Null
    Add-Text $s 'SHIP, CREW & INTERACTIVE SYSTEMS' 54 28 1050 54 38 $C.White -Bold | Out-Null
    Add-Text $s '06' 1332 38 58 28 16 $C.White -Bold -Align Right | Out-Null
    Add-Rect $s 910 142 464 560 $C.Deep 12 -1 0 -Rounded | Out-Null
    Add-Tag $s 'STATE-BASED ANIMATION' 944 174 252 $C.Gold $C.Navy | Out-Null
    $features = @(
        @('SAILS','Full / half / furled; smooth deployment changes speed.'),
        @('ANCHOR','Animated chain and anchor decelerate and hold the ship.'),
        @('STEERING','Wheel, helmsman and rudder follow the steering input.'),
        @('CREW','Roles cover helm, cannon, rigging, cargo and repairs.')
    )
    for($i=0;$i -lt $features.Count;$i++){
        $yy=242+$i*108
        Add-Circle $s 946 ($yy+5) 34 $(if($i%2 -eq 0){$C.Coral}else{$C.Teal}) | Out-Null
        Add-Text $s ($i+1).ToString() 946 ($yy+9) 34 22 14 $C.White -Bold -Align Center | Out-Null
        Add-Text $s $features[$i][0] 997 $yy 330 26 17 $C.Gold -Bold | Out-Null
        Add-Text $s $features[$i][1] 997 ($yy+31) 330 54 16 $C.White | Out-Null
    }
    Add-Text $s 'Every moving part is attached to the ship hierarchy, so waves and rotation preserve synchronization.' 64 702 790 56 20 $C.White -Bold | Out-Null

    # 7 — Combat
    $s = $presentation.Slides.Add(7, $ppLayoutBlank)
    Add-Picture $s $img.Combat 0 0 1440 810 | Out-Null
    Add-Rect $s 0 0 1440 810 $C.Deep 74 | Out-Null
    Add-Rect $s 0 0 1440 112 $C.Deep 8 | Out-Null
    Add-Text $s 'COMBAT, DAMAGE & PARTICLE EFFECTS' 54 28 1120 54 38 $C.White -Bold | Out-Null
    Add-Text $s '07' 1332 38 58 28 16 $C.White -Bold -Align Right | Out-Null
    Add-Tag $s 'MUZZLE → BALLISTIC FLIGHT → IMPACT' 64 150 390 $C.Coral $C.White | Out-Null
    Add-Rect $s 64 205 560 402 $C.Deep 18 -1 0 -Rounded | Out-Null
    Add-Text $s 'Broadside combat' 98 240 470 34 24 $C.Gold -Bold | Out-Null
    Add-Text $s "• Round, chain and grape ammunition`n• Recoil, muzzle flash and drifting smoke`n• Water splashes, sparks and debris`n• Hull holes, sail tears and broken rails`n• Breakable masts, spreading fire and sinking" 100 298 470 224 20 $C.White | Out-Null
    Add-Rect $s 98 540 454 46 $C.Gold 0 -1 0 -Rounded | Out-Null
    Add-Text $s '3,200-particle bounded pool' 98 551 454 25 18 $C.Navy -Bold -Align Center | Out-Null
    Add-Text $s 'Reusable effect systems keep combat visual while preventing unbounded growth.' 760 664 590 68 20 $C.White -Bold -Align Right | Out-Null

    # 8 — Living world and AI
    $s = $presentation.Slides.Add(8, $ppLayoutBlank)
    Add-LightHeader $s 8 'WORLD DEPTH' 'Living Ocean, Wildlife & Obstacle-Aware AI'
    Add-Picture $s $img.World 54 150 820 461 $C.White 3 | Out-Null
    Add-Tag $s 'ROUTED TRAFFIC + DOLPHINS + LANDMARKS' 78 174 390 $C.Gold $C.Navy | Out-Null
    Add-Rect $s 910 150 474 461 $C.White 0 (Rgb 220 225 239) 1.5 -Rounded | Out-Null
    Add-Metric $s '8' 'SEA REGIONS' 944 178 $C.Coral | Out-Null
    Add-Metric $s '48' 'DISCOVERABLE SITES' 944 274 $C.Teal | Out-Null
    Add-Metric $s '32' 'ROUTED TRAFFIC SHIPS' 944 370 $C.Gold | Out-Null
    Add-Text $s 'Enemy navigation plans a hull-wide route around land and remembers its avoidance side, preventing repeated harbor oscillation.' 944 486 400 95 18 $C.Ink -Bold | Out-Null
    Add-Rect $s 54 641 1330 80 $C.Navy 0 -1 0 -Rounded | Out-Null
    Add-Text $s 'Birds • fish schools • smooth dolphin jumps • factions • dynamic encounters • persistent discoveries' 78 664 1280 34 20 $C.White -Bold -Align Center | Out-Null
    Add-Footer $s

    # 9 — Ports and campaign
    $s = $presentation.Slides.Add(9, $ppLayoutBlank)
    Add-LightHeader $s 9 'GAMEPLAY SYSTEMS' 'Ports, Cargo, Trading & Connected Campaign'
    Add-Picture $s $img.Port 54 150 786 442 $C.White 3 | Out-Null
    Add-Tag $s 'NASSAU — FREE CAMERA CAPTURE' 78 174 312 $C.Gold $C.Navy | Out-Null
    Add-Rect $s 876 150 508 442 $C.Navy 0 -1 0 -Rounded | Out-Null
    Add-Text $s 'Living port workflow' 910 184 430 32 23 $C.Gold -Bold | Out-Null
    Add-Text $s 'Approach → moor → dock → timed loading / unloading → depart' 910 234 430 76 21 $C.White -Bold -Align Center | Out-Null
    Add-Text $s 'Cargo does not teleport: NPC and crew states carry visible crates through a 3.2 s transfer cycle.' 910 338 430 92 18 $C.White | Out-Null
    Add-Text $s 'Prices respond to stock, demand, production, damage and relationship.' 910 465 430 70 18 $C.Sky | Out-Null
    Add-Metric $s '13' 'PORTS' 70 628 $C.Coral | Out-Null
    Add-Metric $s '14' 'GOODS' 352 628 $C.Teal | Out-Null
    Add-Metric $s '24' 'CARGO CAPACITY' 634 628 $C.Gold | Out-Null
    Add-Metric $s '10' 'CONNECTED MISSIONS' 944 628 $C.Coral | Out-Null
    Add-Footer $s

    # 10 — Navigation
    $s = $presentation.Slides.Add(10, $ppLayoutBlank)
    Add-LightHeader $s 10 'EXPLORATION' 'Compass, World Map & Treasure Discovery'
    Add-Picture $s $img.Map 42 145 1010 568 $C.White 3 | Out-Null
    Add-Rect $s 1084 145 306 568 $C.Navy 0 -1 0 -Rounded | Out-Null
    Add-Text $s 'MAP LANGUAGE' 1112 180 250 26 16 $C.Gold -Bold | Out-Null
    Add-Text $s "YOU   player`nP     port`nI     island`nS     discovered site`n?     unknown`nO     objective`nT     treasure`nE     enemy" 1114 230 250 258 17 $C.White -Font 'Consolas' | Out-Null
    Add-Text $s 'Treasure clues begin approximate, then become exact as the player approaches and discovers evidence.' 1112 514 246 116 18 $C.Sky -Bold | Out-Null
    Add-Tag $s 'F10 MAP  •  ARROWS PAN  •  +/− ZOOM' 1088 650 296 $C.Coral $C.White | Out-Null
    Add-Footer $s

    # 11 — Ray tracing
    $s = $presentation.Slides.Add(11, $ppLayoutBlank)
    Add-LightHeader $s 11 'EXTRA GRAPHICS FEATURE' 'Hybrid Ray-Traced Ocean Shadows'
    Add-Picture $s $img.RayOff 46 156 646 363 $C.White 3 | Out-Null
    Add-Picture $s $img.RayOn 748 156 646 363 $C.White 3 | Out-Null
    Add-Tag $s 'RT OFF' 70 178 110 $C.Coral $C.White | Out-Null
    Add-Tag $s 'RT ON' 772 178 110 $C.Green $C.White | Out-Null
    foreach($x in @(300,1002)) {
        $focus = $s.Shapes.AddShape($msoShapeOval, $x, 330, 180, 86)
        $focus.Fill.Visible = $msoFalse
        $focus.Line.Visible = $msoTrue
        $focus.Line.ForeColor.RGB = $C.Gold
        $focus.Line.Weight = 3
    }
    Add-Rect $s 46 548 1348 158 $C.Navy 0 -1 0 -Rounded | Out-Null
    Add-Text $s 'Ray: r(t) = O + tD' 78 578 330 34 22 $C.Gold -Bold -Font 'Cambria Math' | Out-Null
    Add-Text $s 'Sphere hit: Δ = b² − c ≥ 0' 78 626 370 34 22 $C.White -Font 'Cambria Math' | Out-Null
    Add-Text $s 'One sun-visibility ray per above-water ocean fragment; tested against at most 16 nearby analytic sphere proxies.' 474 572 860 50 19 $C.White -Bold | Out-Null
    Add-Text $s 'Toggle with 0. This is a limited hybrid shadow feature—not a full-scene or recursive ray tracer.' 474 634 860 42 18 $C.Sky | Out-Null
    Add-Footer $s

    # 12 — Implementation and results
    $s = $presentation.Slides.Add(12, $ppLayoutBlank)
    Add-Picture $s $img.Overview 0 0 1440 810 | Out-Null
    Add-Rect $s 0 0 1440 810 $C.Deep 25 | Out-Null
    Add-Rect $s 0 0 1440 112 $C.Deep 8 | Out-Null
    Add-Text $s 'IMPLEMENTATION, OPTIMIZATION & VERIFIED RESULTS' 54 28 1160 54 36 $C.White -Bold | Out-Null
    Add-Text $s '12' 1332 38 58 28 16 $C.White -Bold -Align Right | Out-Null
    $cards = @(
        @('STACK',"C++17 • OpenGL 3.3 Core`nGLFW • GLAD • GLM • GLSL",$C.Gold),
        @('MOTION',"Delta-time updates`nShared CPU / GPU wave model",$C.Coral),
        @('PERFORMANCE',"LOD + range culling`nBounded pools + static batching",$C.Teal),
        @('REGRESSION',"31 / 31 campaign checks`n10 / 10 camera checks",$C.Green)
    )
    for($i=0;$i -lt 4;$i++){
        $x=64+($i%2)*660; $y=158+[math]::Floor($i/2)*210
        Add-Rect $s $x $y 612 174 $C.Deep 12 $cards[$i][2] 2 -Rounded | Out-Null
        Add-Text $s $cards[$i][0] ($x+30) ($y+26) 190 26 16 $cards[$i][2] -Bold | Out-Null
        Add-Text $s $cards[$i][1] ($x+30) ($y+66) 550 76 22 $C.White -Bold | Out-Null
    }
    Add-Rect $s 64 612 1272 100 $C.White 4 -1 0 -Rounded | Out-Null
    Add-Text $s 'LIVE CONTROLS' 92 635 180 25 15 $C.Coral -Bold | Out-Null
    Add-Text $s 'WASD sail  •  mouse + Space aim/fire  •  Ctrl broadside  •  Z sails  •  J anchor  •  F10 map  •  0 ray trace  •  1/2/3 shading' 280 626 1022 50 17 $C.Navy -Bold -Align Center | Out-Null
    Add-Text $s 'All reported features are present in the supplied source and executable checks.' 92 690 1210 24 15 $C.Muted -Bold -Align Center | Out-Null

    # 13 — Thank you
    $s = $presentation.Slides.Add(13, $ppLayoutBlank)
    Add-Picture $s $img.Night 0 0 1440 810 | Out-Null
    Add-Rect $s 0 0 1440 810 $C.Deep 48 | Out-Null
    Add-Circle $s 642 118 156 $C.Gold 8 | Out-Null
    Add-Picture $s $star 694 169 52 52 | Out-Null
    Add-Text $s 'THANK YOU' 240 320 960 90 68 $C.White -Bold -Align Center | Out-Null
    Add-Text $s 'Questions?' 240 420 960 50 30 $C.Gold -Bold -Align Center | Out-Null
    Add-Text $s 'BROADSIDE  •  C++ / OPENGL 3.3 CORE / GLSL' 240 536 960 32 18 $C.Sky -Bold -Align Center | Out-Null
    Add-Text $s 'Tawhidul Hasan  •  Roll 2107004' 240 594 960 32 18 $C.White -Bold -Align Center | Out-Null
    Add-Text $s '13' 1332 748 58 28 14 $C.White -Bold -Align Right | Out-Null

    try {
        $presentation.BuiltInDocumentProperties.Item('Title').Value = 'Broadside — Ship Battle Simulator'
        $presentation.BuiltInDocumentProperties.Item('Subject').Value = 'Computer Graphics Project Showcase'
        $presentation.BuiltInDocumentProperties.Item('Author').Value = 'Tawhidul Hasan'
    } catch {
        Write-Warning 'PowerPoint did not expose built-in metadata; slide content is unaffected.'
    }
    $presentation.Save()
    $presentation.Close()
}
finally {
    $app.Quit()
    [System.Runtime.InteropServices.Marshal]::ReleaseComObject($app) | Out-Null
}

Write-Output "Updated: $PresentationPath"
Write-Output "Backup:  $backup"
