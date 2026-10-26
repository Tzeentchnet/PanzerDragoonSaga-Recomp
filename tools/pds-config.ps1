[CmdletBinding()]
param(
    [switch]$ValidateOnly,
    [string]$ConfigPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$script:RepositoryRoot = Split-Path -Parent $PSScriptRoot
if ([string]::IsNullOrWhiteSpace($ConfigPath)) {
    $localAppData = [Environment]::GetFolderPath(
        [Environment+SpecialFolder]::LocalApplicationData
    )
    if ([string]::IsNullOrWhiteSpace($localAppData)) {
        throw 'Windows did not provide a LocalApplicationData directory.'
    }
    $ConfigPath = Join-Path (
        Join-Path $localAppData 'PanzerDragoonSaga-Recomp'
    ) 'settings.ini'
}
$script:ConfigPath = [IO.Path]::GetFullPath($ConfigPath)

$script:KeyboardActions = [ordered]@{
    'a' = 'Saturn A'
    'b' = 'Saturn B'
    'c' = 'Saturn C'
    'start' = 'Start'
    'up' = 'Up'
    'down' = 'Down'
    'left' = 'Left'
    'right' = 'Right'
    'x' = 'Saturn X'
    'y' = 'Saturn Y'
    'l' = 'Saturn L'
    'r' = 'Saturn R'
}

$script:GamepadInputs = [ordered]@{
    'south' = 'South face button (A / Cross)'
    'east' = 'East face button (B / Circle)'
    'west' = 'West face button (X / Square)'
    'north' = 'North face button (Y / Triangle)'
    'back' = 'Back / Select'
    'start' = 'Start / Options'
    'left_stick' = 'Left stick click'
    'right_stick' = 'Right stick click'
    'left_shoulder' = 'Left shoulder'
    'right_shoulder' = 'Right shoulder'
    'dpad_up' = 'D-pad Up'
    'dpad_down' = 'D-pad Down'
    'dpad_left' = 'D-pad Left'
    'dpad_right' = 'D-pad Right'
    'left_trigger' = 'Left trigger'
    'right_trigger' = 'Right trigger'
}

$script:KeyboardKeys = @(
    'None',
    'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
    'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    'Return', 'Escape', 'Backspace', 'Tab', 'Space',
    '-', '=', '[', ']', '\', '#', ';', "'", '`', ',', '.', '/',
    'CapsLock',
    'F1', 'F2', 'F3', 'F4', 'F5', 'F6',
    'F7', 'F8', 'F9', 'F10', 'F11', 'F12',
    'PrintScreen', 'ScrollLock', 'Pause',
    'Insert', 'Home', 'PageUp', 'Delete', 'End', 'PageDown',
    'Right', 'Left', 'Down', 'Up',
    'Numlock',
    'Keypad /', 'Keypad *', 'Keypad -', 'Keypad +', 'Keypad Enter',
    'Keypad 1', 'Keypad 2', 'Keypad 3', 'Keypad 4', 'Keypad 5',
    'Keypad 6', 'Keypad 7', 'Keypad 8', 'Keypad 9', 'Keypad 0', 'Keypad .',
    'Application',
    'Left Ctrl', 'Left Shift', 'Left Alt', 'Left GUI',
    'Right Ctrl', 'Right Shift', 'Right Alt', 'Right GUI'
)

$script:GamepadActions = @(
    'None', 'A', 'B', 'C', 'Start',
    'Up', 'Down', 'Left', 'Right',
    'X', 'Y', 'L', 'R'
)

function New-DefaultSettings {
    $settings = [ordered]@{
        'config.version' = '1'
        'launcher.build' = 'Release'
        'graphics.renderer' = 'auto'
        'graphics.window_width' = '1280'
        'graphics.window_height' = '960'
        'graphics.fullscreen' = 'false'
        'graphics.vsync' = 'false'
        'audio_pacing.volume_percent' = '100'
        'audio_pacing.frame_limit' = '30'
        'input.keyboard.a' = 'C'
        'input.keyboard.b' = 'X'
        'input.keyboard.c' = 'Z'
        'input.keyboard.start' = 'Return'
        'input.keyboard.up' = 'Up'
        'input.keyboard.down' = 'Down'
        'input.keyboard.left' = 'Left'
        'input.keyboard.right' = 'Right'
        'input.keyboard.x' = 'A'
        'input.keyboard.y' = 'S'
        'input.keyboard.l' = 'Q'
        'input.keyboard.r' = 'W'
        'input.gamepad.south' = 'a'
        'input.gamepad.east' = 'b'
        'input.gamepad.west' = 'x'
        'input.gamepad.north' = 'c'
        'input.gamepad.back' = 'none'
        'input.gamepad.start' = 'start'
        'input.gamepad.left_stick' = 'none'
        'input.gamepad.right_stick' = 'none'
        'input.gamepad.left_shoulder' = 'l'
        'input.gamepad.right_shoulder' = 'r'
        'input.gamepad.dpad_up' = 'up'
        'input.gamepad.dpad_down' = 'down'
        'input.gamepad.dpad_left' = 'left'
        'input.gamepad.dpad_right' = 'right'
        'input.gamepad.left_trigger' = 'l'
        'input.gamepad.right_trigger' = 'r'
        'input.gamepad.left_stick_directions' = 'true'
        'experimental.gpu_vdp1' = 'true'
        'experimental.combined_dual_plane' = 'true'
        'experimental.smooth_gouraud' = 'true'
        'experimental.direct_ray_rendering' = 'true'
    }
    return $settings
}

function Get-CanonicalChoice {
    param(
        [string]$Name,
        [string]$Value,
        [string[]]$Allowed
    )

    foreach ($candidate in $Allowed) {
        if ($candidate -ieq $Value) {
            return $candidate
        }
    }
    throw "$Name must be one of: $($Allowed -join ', ')."
}

function Get-CanonicalBoolean {
    param(
        [string]$Name,
        [string]$Value
    )

    if ($Value -ieq 'true') {
        return 'true'
    }
    if ($Value -ieq 'false') {
        return 'false'
    }
    throw "$Name must be true or false."
}

function Get-ValidatedInteger {
    param(
        [string]$Name,
        [string]$Value,
        [int]$Minimum,
        [int]$Maximum,
        [switch]$AllowZero
    )

    $number = 0
    if (-not [int]::TryParse($Value, [ref]$number)) {
        throw "$Name must be an integer."
    }
    if ($AllowZero -and $number -eq 0) {
        return '0'
    }
    if ($number -lt $Minimum -or $number -gt $Maximum) {
        throw "$Name must be between $Minimum and $Maximum."
    }
    return $number.ToString([Globalization.CultureInfo]::InvariantCulture)
}

function Test-AndNormalizeSettings {
    param([Collections.Specialized.OrderedDictionary]$Settings)

    if ($Settings['config.version'] -ne '1') {
        throw 'Only configuration version 1 is supported.'
    }

    $Settings['launcher.build'] = Get-CanonicalChoice `
        'Build' $Settings['launcher.build'] @('Debug', 'Release', 'Shipping')
    $Settings['graphics.renderer'] = (
        Get-CanonicalChoice 'Renderer' $Settings['graphics.renderer'] @(
            'auto', 'd3d11', 'd3d12', 'vulkan', 'software'
        )
    ).ToLowerInvariant()
    $Settings['graphics.window_width'] = Get-ValidatedInteger `
        'Window width' $Settings['graphics.window_width'] 640 7680
    $Settings['graphics.window_height'] = Get-ValidatedInteger `
        'Window height' $Settings['graphics.window_height'] 480 4320
    $Settings['graphics.fullscreen'] = Get-CanonicalBoolean `
        'Fullscreen' $Settings['graphics.fullscreen']
    $Settings['graphics.vsync'] = Get-CanonicalBoolean `
        'VSync' $Settings['graphics.vsync']
    $Settings['audio_pacing.volume_percent'] = Get-ValidatedInteger `
        'Volume' $Settings['audio_pacing.volume_percent'] 0 100
    $Settings['audio_pacing.frame_limit'] = Get-ValidatedInteger `
        'Frame limit' $Settings['audio_pacing.frame_limit'] 1 240 -AllowZero

    $usedKeys = @{}
    foreach ($action in $script:KeyboardActions.Keys) {
        $identity = "input.keyboard.$action"
        $canonical = Get-CanonicalChoice `
            "Keyboard binding $action" $Settings[$identity] $script:KeyboardKeys
        $Settings[$identity] = $canonical
        if ($canonical -ine 'None') {
            $normalized = $canonical.ToLowerInvariant()
            if ($usedKeys.ContainsKey($normalized)) {
                throw "Keyboard key '$canonical' is assigned more than once."
            }
            $usedKeys[$normalized] = $true
        }
    }

    foreach ($inputName in $script:GamepadInputs.Keys) {
        $identity = "input.gamepad.$inputName"
        $Settings[$identity] = (
            Get-CanonicalChoice `
                "Gamepad binding $inputName" `
                $Settings[$identity] `
                $script:GamepadActions
        ).ToLowerInvariant()
    }
    $Settings['input.gamepad.left_stick_directions'] = Get-CanonicalBoolean `
        'Left-stick directions' `
        $Settings['input.gamepad.left_stick_directions']

    foreach ($name in @(
        'gpu_vdp1',
        'combined_dual_plane',
        'smooth_gouraud',
        'direct_ray_rendering'
    )) {
        $identity = "experimental.$name"
        $Settings[$identity] = Get-CanonicalBoolean $name $Settings[$identity]
    }
}

function ConvertFrom-IniLines {
    param([string[]]$Lines)

    $settings = New-DefaultSettings
    $seen = @{}
    $section = ''
    $lineNumber = 0

    foreach ($rawLine in $Lines) {
        $lineNumber++
        $line = $rawLine.Trim()
        if (
            $line.Length -eq 0 -or
            $line.StartsWith(';') -or
            $line.StartsWith('#')
        ) {
            continue
        }
        if ($line.StartsWith('[')) {
            if ($line -notmatch '^\[([^\]]+)\]$') {
                throw "Configuration line $lineNumber has an invalid section header."
            }
            $section = $Matches[1].Trim().ToLowerInvariant()
            continue
        }
        if ($section.Length -eq 0) {
            throw "Configuration line $lineNumber appears before a section."
        }

        $separator = $line.IndexOf('=')
        if ($separator -lt 1 -or $separator -eq ($line.Length - 1)) {
            throw "Configuration line $lineNumber must use key=value."
        }

        $key = $line.Substring(0, $separator).Trim().ToLowerInvariant()
        $value = $line.Substring($separator + 1).Trim()
        $identity = "$section.$key"
        if (-not $settings.Contains($identity)) {
            throw "Configuration line $lineNumber contains unknown setting '$identity'."
        }
        if ($seen.ContainsKey($identity)) {
            throw "Configuration line $lineNumber repeats setting '$identity'."
        }
        $seen[$identity] = $true
        $settings[$identity] = $value
    }

    if (-not $seen.ContainsKey('config.version')) {
        throw 'The configuration is missing [config] version.'
    }
    Test-AndNormalizeSettings $settings
    return $settings
}

function ConvertTo-IniText {
    param([Collections.Specialized.OrderedDictionary]$Settings)

    $lines = [Collections.Generic.List[string]]::new()
    [void]$lines.Add('[config]')
    [void]$lines.Add("version=$($Settings['config.version'])")
    [void]$lines.Add('')
    [void]$lines.Add('[launcher]')
    [void]$lines.Add("build=$($Settings['launcher.build'])")
    [void]$lines.Add('')
    [void]$lines.Add('[graphics]')
    foreach ($name in @(
        'renderer',
        'window_width',
        'window_height',
        'fullscreen',
        'vsync'
    )) {
        [void]$lines.Add("$name=$($Settings["graphics.$name"])")
    }
    [void]$lines.Add('')
    [void]$lines.Add('[audio_pacing]')
    [void]$lines.Add(
        "volume_percent=$($Settings['audio_pacing.volume_percent'])"
    )
    [void]$lines.Add(
        "frame_limit=$($Settings['audio_pacing.frame_limit'])"
    )
    [void]$lines.Add('')
    [void]$lines.Add('[input.keyboard]')
    foreach ($name in $script:KeyboardActions.Keys) {
        [void]$lines.Add("$name=$($Settings["input.keyboard.$name"])")
    }
    [void]$lines.Add('')
    [void]$lines.Add('[input.gamepad]')
    foreach ($name in $script:GamepadInputs.Keys) {
        [void]$lines.Add("$name=$($Settings["input.gamepad.$name"])")
    }
    [void]$lines.Add(
        'left_stick_directions=' +
        $Settings['input.gamepad.left_stick_directions']
    )
    [void]$lines.Add('')
    [void]$lines.Add('[experimental]')
    foreach ($name in @(
        'gpu_vdp1',
        'combined_dual_plane',
        'smooth_gouraud',
        'direct_ray_rendering'
    )) {
        [void]$lines.Add("$name=$($Settings["experimental.$name"])")
    }
    [void]$lines.Add('')

    return ($lines -join [Environment]::NewLine)
}

function Read-SettingsFile {
    param([string]$Path)
    return ConvertFrom-IniLines ([IO.File]::ReadAllLines($Path))
}

function Write-SettingsFile {
    param(
        [string]$Path,
        [Collections.Specialized.OrderedDictionary]$Settings
    )

    Test-AndNormalizeSettings $Settings
    $directory = [IO.Path]::GetDirectoryName($Path)
    if (-not [string]::IsNullOrWhiteSpace($directory)) {
        [void][IO.Directory]::CreateDirectory($directory)
    }
    $encoding = [Text.UTF8Encoding]::new($false)
    [IO.File]::WriteAllText(
        $Path,
        (ConvertTo-IniText $Settings),
        $encoding
    )
}

if ($ValidateOnly) {
    $defaults = New-DefaultSettings
    Test-AndNormalizeSettings $defaults
    $roundTrip = ConvertFrom-IniLines (
        (ConvertTo-IniText $defaults) -split '\r?\n'
    )
    foreach ($key in $defaults.Keys) {
        if ([string]$defaults[$key] -cne [string]$roundTrip[$key]) {
            throw "Configuration self-test failed for '$key'."
        }
    }
    if (Test-Path -LiteralPath $script:ConfigPath -PathType Leaf) {
        [void](Read-SettingsFile $script:ConfigPath)
        Write-Host "Configuration is valid: $($script:ConfigPath)"
    }
    else {
        Write-Host 'Configuration schema and default round trip are valid.'
    }
    return
}

Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()

function New-ComboBox {
    param([string[]]$Items)

    $combo = [Windows.Forms.ComboBox]::new()
    $combo.DropDownStyle = [Windows.Forms.ComboBoxStyle]::DropDownList
    $combo.Width = 240
    [void]$combo.Items.AddRange($Items)
    return $combo
}

function Add-LabeledControl {
    param(
        [Windows.Forms.TableLayoutPanel]$Panel,
        [int]$Row,
        [string]$Text,
        [Windows.Forms.Control]$Control
    )

    $label = [Windows.Forms.Label]::new()
    $label.Text = $Text
    $label.AutoSize = $true
    $label.Anchor = [Windows.Forms.AnchorStyles]::Left
    $Panel.Controls.Add($label, 0, $Row)
    $Control.Anchor = [Windows.Forms.AnchorStyles]::Left
    $Panel.Controls.Add($Control, 1, $Row)
}

function Set-ComboValue {
    param(
        [Windows.Forms.ComboBox]$Combo,
        [string]$Value
    )

    $index = $Combo.FindStringExact($Value)
    if ($index -lt 0) {
        throw "The saved value '$Value' is not available in the launcher."
    }
    $Combo.SelectedIndex = $index
}

function ConvertTo-GamepadDisplay {
    param([string]$Value)

    foreach ($candidate in $script:GamepadActions) {
        if ($candidate -ieq $Value) {
            return $candidate
        }
    }
    throw "Unknown gamepad action '$Value'."
}

$form = [Windows.Forms.Form]::new()
$form.Text = 'Panzer Dragoon Saga - Configuration'
$form.StartPosition = [Windows.Forms.FormStartPosition]::CenterScreen
$form.ClientSize = [Drawing.Size]::new(900, 700)
$form.MinimumSize = [Drawing.Size]::new(800, 620)

$tabs = [Windows.Forms.TabControl]::new()
$tabs.Location = [Drawing.Point]::new(12, 12)
$tabs.Size = [Drawing.Size]::new(876, 600)
$tabs.Anchor = (
    [Windows.Forms.AnchorStyles]::Top -bor
    [Windows.Forms.AnchorStyles]::Bottom -bor
    [Windows.Forms.AnchorStyles]::Left -bor
    [Windows.Forms.AnchorStyles]::Right
)
$form.Controls.Add($tabs)

$generalTab = [Windows.Forms.TabPage]::new('General')
$keyboardTab = [Windows.Forms.TabPage]::new('Keyboard')
$gamepadTab = [Windows.Forms.TabPage]::new('Gamepad')
$experimentalTab = [Windows.Forms.TabPage]::new('Experimental')
[void]$tabs.TabPages.AddRange(@(
    $generalTab,
    $keyboardTab,
    $gamepadTab,
    $experimentalTab
))

$generalLayout = [Windows.Forms.TableLayoutPanel]::new()
$generalLayout.Dock = [Windows.Forms.DockStyle]::Fill
$generalLayout.Padding = [Windows.Forms.Padding]::new(16)
$generalLayout.ColumnCount = 2
$generalLayout.RowCount = 9
[void]$generalLayout.ColumnStyles.Add(
    [Windows.Forms.ColumnStyle]::new(
        [Windows.Forms.SizeType]::Absolute,
        220
    )
)
[void]$generalLayout.ColumnStyles.Add(
    [Windows.Forms.ColumnStyle]::new(
        [Windows.Forms.SizeType]::Percent,
        100
    )
)
$generalTab.Controls.Add($generalLayout)

$buildCombo = New-ComboBox @('Debug', 'Release', 'Shipping')
$rendererCombo = New-ComboBox @(
    'auto', 'd3d12', 'vulkan', 'd3d11', 'software'
)
$widthControl = [Windows.Forms.NumericUpDown]::new()
$widthControl.Minimum = 640
$widthControl.Maximum = 7680
$widthControl.Increment = 16
$widthControl.Width = 120
$heightControl = [Windows.Forms.NumericUpDown]::new()
$heightControl.Minimum = 480
$heightControl.Maximum = 4320
$heightControl.Increment = 16
$heightControl.Width = 120
$fullscreenCheck = [Windows.Forms.CheckBox]::new()
$fullscreenCheck.Text = 'Use desktop fullscreen'
$fullscreenCheck.AutoSize = $true
$vsyncCheck = [Windows.Forms.CheckBox]::new()
$vsyncCheck.Text = 'Synchronize presentation to the display'
$vsyncCheck.AutoSize = $true
$volumeControl = [Windows.Forms.NumericUpDown]::new()
$volumeControl.Minimum = 0
$volumeControl.Maximum = 100
$volumeControl.Width = 120
$frameLimitControl = [Windows.Forms.NumericUpDown]::new()
$frameLimitControl.Minimum = 0
$frameLimitControl.Maximum = 240
$frameLimitControl.Width = 120

Add-LabeledControl $generalLayout 0 'Build to launch' $buildCombo
Add-LabeledControl $generalLayout 1 'Renderer' $rendererCombo
Add-LabeledControl $generalLayout 2 'Window width' $widthControl
Add-LabeledControl $generalLayout 3 'Window height' $heightControl
Add-LabeledControl $generalLayout 4 'Fullscreen' $fullscreenCheck
Add-LabeledControl $generalLayout 5 'VSync' $vsyncCheck
Add-LabeledControl $generalLayout 6 'Volume' $volumeControl
Add-LabeledControl `
    $generalLayout 7 'Frame limit (0 = unlimited)' $frameLimitControl

$generalNote = [Windows.Forms.Label]::new()
$generalNote.Text = (
    'Auto requests Direct3D 12 and allows fallback. Software uses ' +
    'Direct3D 11 with the Microsoft Basic Render Driver (WARP).'
)
$generalNote.AutoSize = $true
$generalNote.MaximumSize = [Drawing.Size]::new(760, 0)
$generalLayout.Controls.Add($generalNote, 0, 8)
$generalLayout.SetColumnSpan($generalNote, 2)

$keyboardTab.AutoScroll = $true
$keyboardLayout = [Windows.Forms.TableLayoutPanel]::new()
$keyboardLayout.Dock = [Windows.Forms.DockStyle]::Top
$keyboardLayout.Padding = [Windows.Forms.Padding]::new(16)
$keyboardLayout.AutoSize = $true
$keyboardLayout.ColumnCount = 2
[void]$keyboardLayout.ColumnStyles.Add(
    [Windows.Forms.ColumnStyle]::new(
        [Windows.Forms.SizeType]::Absolute,
        220
    )
)
[void]$keyboardLayout.ColumnStyles.Add(
    [Windows.Forms.ColumnStyle]::new(
        [Windows.Forms.SizeType]::Absolute,
        280
    )
)
$keyboardTab.Controls.Add($keyboardLayout)
$script:KeyboardCombos = @{}
$row = 0
foreach ($entry in $script:KeyboardActions.GetEnumerator()) {
    $combo = New-ComboBox $script:KeyboardKeys
    $script:KeyboardCombos[$entry.Key] = $combo
    Add-LabeledControl $keyboardLayout $row $entry.Value $combo
    $row++
}

$gamepadTab.AutoScroll = $true
$gamepadLayout = [Windows.Forms.TableLayoutPanel]::new()
$gamepadLayout.Dock = [Windows.Forms.DockStyle]::Top
$gamepadLayout.Padding = [Windows.Forms.Padding]::new(16)
$gamepadLayout.AutoSize = $true
$gamepadLayout.ColumnCount = 2
[void]$gamepadLayout.ColumnStyles.Add(
    [Windows.Forms.ColumnStyle]::new(
        [Windows.Forms.SizeType]::Absolute,
        280
    )
)
[void]$gamepadLayout.ColumnStyles.Add(
    [Windows.Forms.ColumnStyle]::new(
        [Windows.Forms.SizeType]::Absolute,
        280
    )
)
$gamepadTab.Controls.Add($gamepadLayout)
$script:GamepadCombos = @{}
$row = 0
foreach ($entry in $script:GamepadInputs.GetEnumerator()) {
    $combo = New-ComboBox $script:GamepadActions
    $script:GamepadCombos[$entry.Key] = $combo
    Add-LabeledControl $gamepadLayout $row $entry.Value $combo
    $row++
}
$leftStickCheck = [Windows.Forms.CheckBox]::new()
$leftStickCheck.Text = 'Use the left analog stick for directional input'
$leftStickCheck.AutoSize = $true
Add-LabeledControl $gamepadLayout $row 'Analog navigation' $leftStickCheck

$experimentalLayout = [Windows.Forms.FlowLayoutPanel]::new()
$experimentalLayout.Dock = [Windows.Forms.DockStyle]::Fill
$experimentalLayout.FlowDirection = [Windows.Forms.FlowDirection]::TopDown
$experimentalLayout.WrapContents = $false
$experimentalLayout.Padding = [Windows.Forms.Padding]::new(16)
$experimentalTab.Controls.Add($experimentalLayout)

$experimentalWarning = [Windows.Forms.Label]::new()
$experimentalWarning.Text = (
    'These settings expose incomplete or diagnostic rendering paths. ' +
    'Use the defaults unless you are testing renderer behavior.'
)
$experimentalWarning.AutoSize = $true
$experimentalWarning.MaximumSize = [Drawing.Size]::new(760, 0)
$experimentalLayout.Controls.Add($experimentalWarning)

$gpuVdp1Check = [Windows.Forms.CheckBox]::new()
$gpuVdp1Check.Text = 'GPU VDP1 rendering'
$gpuVdp1Check.AutoSize = $true
$combinedDualPlaneCheck = [Windows.Forms.CheckBox]::new()
$combinedDualPlaneCheck.Text = 'Combined dual-plane RBG0 rendering'
$combinedDualPlaneCheck.AutoSize = $true
$smoothGouraudCheck = [Windows.Forms.CheckBox]::new()
$smoothGouraudCheck.Text = 'Smooth Gouraud falloff'
$smoothGouraudCheck.AutoSize = $true
$directRayCheck = [Windows.Forms.CheckBox]::new()
$directRayCheck.Text = 'Direct 3D ray rendering'
$directRayCheck.AutoSize = $true
[void]$experimentalLayout.Controls.AddRange(@(
    $gpuVdp1Check,
    $combinedDualPlaneCheck,
    $smoothGouraudCheck,
    $directRayCheck
))

$buttonPanel = [Windows.Forms.FlowLayoutPanel]::new()
$buttonPanel.FlowDirection = [Windows.Forms.FlowDirection]::RightToLeft
$buttonPanel.Location = [Drawing.Point]::new(12, 620)
$buttonPanel.Size = [Drawing.Size]::new(876, 36)
$buttonPanel.Anchor = (
    [Windows.Forms.AnchorStyles]::Bottom -bor
    [Windows.Forms.AnchorStyles]::Left -bor
    [Windows.Forms.AnchorStyles]::Right
)
$form.Controls.Add($buttonPanel)

$closeButton = [Windows.Forms.Button]::new()
$closeButton.Text = 'Close'
$closeButton.AutoSize = $true
$saveButton = [Windows.Forms.Button]::new()
$saveButton.Text = 'Save'
$saveButton.AutoSize = $true
$launchButton = [Windows.Forms.Button]::new()
$launchButton.Text = 'Save && Launch'
$launchButton.AutoSize = $true
$buildLaunchButton = [Windows.Forms.Button]::new()
$buildLaunchButton.Text = 'Build && Launch'
$buildLaunchButton.AutoSize = $true
$resetButton = [Windows.Forms.Button]::new()
$resetButton.Text = 'Reset Defaults'
$resetButton.AutoSize = $true
[void]$buttonPanel.Controls.AddRange(@(
    $closeButton,
    $launchButton,
    $buildLaunchButton,
    $saveButton,
    $resetButton
))

$statusLabel = [Windows.Forms.Label]::new()
$statusLabel.Location = [Drawing.Point]::new(12, 665)
$statusLabel.Size = [Drawing.Size]::new(876, 24)
$statusLabel.Anchor = (
    [Windows.Forms.AnchorStyles]::Bottom -bor
    [Windows.Forms.AnchorStyles]::Left -bor
    [Windows.Forms.AnchorStyles]::Right
)
$statusLabel.Text = "Settings file: $($script:ConfigPath)"
$form.Controls.Add($statusLabel)

function Set-ControlsFromSettings {
    param([Collections.Specialized.OrderedDictionary]$Settings)

    Set-ComboValue $buildCombo $Settings['launcher.build']
    Set-ComboValue $rendererCombo $Settings['graphics.renderer']
    $widthControl.Value = [int]$Settings['graphics.window_width']
    $heightControl.Value = [int]$Settings['graphics.window_height']
    $fullscreenCheck.Checked = (
        $Settings['graphics.fullscreen'] -eq 'true'
    )
    $vsyncCheck.Checked = $Settings['graphics.vsync'] -eq 'true'
    $volumeControl.Value = [int]$Settings['audio_pacing.volume_percent']
    $frameLimitControl.Value = [int]$Settings['audio_pacing.frame_limit']

    foreach ($action in $script:KeyboardActions.Keys) {
        Set-ComboValue `
            $script:KeyboardCombos[$action] `
            $Settings["input.keyboard.$action"]
    }
    foreach ($inputName in $script:GamepadInputs.Keys) {
        Set-ComboValue `
            $script:GamepadCombos[$inputName] `
            (ConvertTo-GamepadDisplay $Settings["input.gamepad.$inputName"])
    }
    $leftStickCheck.Checked = (
        $Settings['input.gamepad.left_stick_directions'] -eq 'true'
    )
    $gpuVdp1Check.Checked = $Settings['experimental.gpu_vdp1'] -eq 'true'
    $combinedDualPlaneCheck.Checked = (
        $Settings['experimental.combined_dual_plane'] -eq 'true'
    )
    $smoothGouraudCheck.Checked = (
        $Settings['experimental.smooth_gouraud'] -eq 'true'
    )
    $directRayCheck.Checked = (
        $Settings['experimental.direct_ray_rendering'] -eq 'true'
    )
}

function Get-SettingsFromControls {
    $settings = New-DefaultSettings
    $settings['launcher.build'] = [string]$buildCombo.SelectedItem
    $settings['graphics.renderer'] = [string]$rendererCombo.SelectedItem
    $settings['graphics.window_width'] = [string][int]$widthControl.Value
    $settings['graphics.window_height'] = [string][int]$heightControl.Value
    $settings['graphics.fullscreen'] = (
        $fullscreenCheck.Checked.ToString().ToLowerInvariant()
    )
    $settings['graphics.vsync'] = (
        $vsyncCheck.Checked.ToString().ToLowerInvariant()
    )
    $settings['audio_pacing.volume_percent'] = (
        [string][int]$volumeControl.Value
    )
    $settings['audio_pacing.frame_limit'] = (
        [string][int]$frameLimitControl.Value
    )

    foreach ($action in $script:KeyboardActions.Keys) {
        $settings["input.keyboard.$action"] = [string](
            $script:KeyboardCombos[$action].SelectedItem
        )
    }
    foreach ($inputName in $script:GamepadInputs.Keys) {
        $settings["input.gamepad.$inputName"] = (
            [string]$script:GamepadCombos[$inputName].SelectedItem
        ).ToLowerInvariant()
    }
    $settings['input.gamepad.left_stick_directions'] = (
        $leftStickCheck.Checked.ToString().ToLowerInvariant()
    )
    $settings['experimental.gpu_vdp1'] = (
        $gpuVdp1Check.Checked.ToString().ToLowerInvariant()
    )
    $settings['experimental.combined_dual_plane'] = (
        $combinedDualPlaneCheck.Checked.ToString().ToLowerInvariant()
    )
    $settings['experimental.smooth_gouraud'] = (
        $smoothGouraudCheck.Checked.ToString().ToLowerInvariant()
    )
    $settings['experimental.direct_ray_rendering'] = (
        $directRayCheck.Checked.ToString().ToLowerInvariant()
    )

    Test-AndNormalizeSettings $settings
    return $settings
}

function Show-ErrorMessage {
    param(
        [string]$Title,
        [string]$Message
    )

    [void][Windows.Forms.MessageBox]::Show(
        $form,
        $Message,
        $Title,
        [Windows.Forms.MessageBoxButtons]::OK,
        [Windows.Forms.MessageBoxIcon]::Error
    )
}

function Save-CurrentSettings {
    try {
        $settings = Get-SettingsFromControls
        Write-SettingsFile $script:ConfigPath $settings
        $statusLabel.Text = "Saved: $($script:ConfigPath)"
        return $settings
    }
    catch {
        Show-ErrorMessage 'Could not save settings' $_.Exception.Message
        return $null
    }
}

function Start-ConfiguredGame {
    param([Collections.Specialized.OrderedDictionary]$Settings)

    $build = $Settings['launcher.build']
    $executable = Join-Path $script:RepositoryRoot (
        "out\build\windows-msvc-x64\PDS\$build\PDS.exe"
    )
    if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
        throw (
            "The $build executable has not been built.`n`n" +
            "Expected: $executable`n`n" +
            'Use Build && Launch or run windows-build.bat first.'
        )
    }

    $dataDirectory = Join-Path $script:RepositoryRoot 'data'
    $commonData = Join-Path $dataDirectory 'COMMON.DAT'
    if (-not (Test-Path -LiteralPath $commonData -PathType Leaf)) {
        throw (
            "Game data was not found.`n`n" +
            "Expected: $commonData`n`n" +
            'Extract the CD1 contents into the repository data directory.'
        )
    }

    [void](Start-Process `
        -FilePath $executable `
        -ArgumentList @('--config', "`"$($script:ConfigPath)`"") `
        -WorkingDirectory $dataDirectory)
    $statusLabel.Text = "Launched $build."
}

function Invoke-SelectedBuild {
    param([Collections.Specialized.OrderedDictionary]$Settings)

    $buildScript = Join-Path $script:RepositoryRoot 'windows-build.bat'
    if (-not (Test-Path -LiteralPath $buildScript -PathType Leaf)) {
        throw "Build wrapper not found: $buildScript"
    }

    $form.UseWaitCursor = $true
    $statusLabel.Text = "Building $($Settings['launcher.build'])..."
    [Windows.Forms.Application]::DoEvents()
    Push-Location $script:RepositoryRoot
    try {
        & $buildScript $Settings['launcher.build']
        if ($LASTEXITCODE -ne 0) {
            throw (
                "The $($Settings['launcher.build']) build failed with " +
                "exit code $LASTEXITCODE."
            )
        }
    }
    finally {
        Pop-Location
        $form.UseWaitCursor = $false
    }
    $statusLabel.Text = "Built $($Settings['launcher.build'])."
}

$settings = New-DefaultSettings
if (Test-Path -LiteralPath $script:ConfigPath -PathType Leaf) {
    try {
        $settings = Read-SettingsFile $script:ConfigPath
    }
    catch {
        Show-ErrorMessage `
            'Could not load settings' `
            ($_.Exception.Message + "`n`nDefaults will be shown instead.")
        $settings = New-DefaultSettings
    }
}
Set-ControlsFromSettings $settings

$closeButton.Add_Click({ $form.Close() })
$resetButton.Add_Click({
    Set-ControlsFromSettings (New-DefaultSettings)
    $statusLabel.Text = 'Defaults restored in the editor. Click Save to persist.'
})
$saveButton.Add_Click({ [void](Save-CurrentSettings) })
$launchButton.Add_Click({
    $current = Save-CurrentSettings
    if ($null -eq $current) {
        return
    }
    try {
        Start-ConfiguredGame $current
    }
    catch {
        Show-ErrorMessage 'Could not launch the game' $_.Exception.Message
    }
})
$buildLaunchButton.Add_Click({
    $current = Save-CurrentSettings
    if ($null -eq $current) {
        return
    }
    try {
        Invoke-SelectedBuild $current
        Start-ConfiguredGame $current
    }
    catch {
        Show-ErrorMessage 'Build or launch failed' $_.Exception.Message
    }
})

[void]$form.ShowDialog()
