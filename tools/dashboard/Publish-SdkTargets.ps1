# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        tools/dashboard/Publish-SdkTargets.ps1
# PURPOSE:     Publish each module's effective SDK header target
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

param(
    [string]$SourceRoot = "",
    [string]$WebsitePath = "",
    [switch]$SelfTest
)

$ErrorActionPreference = "Stop"

function ConvertTo-PlainJson {
    param($Value, [int]$Indent = 0)
    $pad = "  " * $Indent
    $next = "  " * ($Indent + 1)
    if ($null -eq $Value) { return "null" }
    if ($Value -is [bool]) {
        if ($Value) { return "true" }
        return "false"
    }
    if ($Value -is [string]) {
        $text = $Value.Replace("\", "\\").Replace('"', '\"').Replace("`r", "\r").Replace("`n", "\n")
        return '"' + $text + '"'
    }
    $isWhole = $Value -is [int] -or $Value -is [long] -or $Value -is [decimal]
    $isFloat = $Value -is [double] -or $Value -is [float] -or $Value -is [single]
    if ($isWhole -or $isFloat) {
        $number = [double]$Value
        $rounded = [math]::Round($number, 1)
        if ([math]::Abs($rounded - [math]::Truncate($rounded)) -lt 0.001) {
            return ([int64][math]::Truncate($rounded)).ToString([Globalization.CultureInfo]::InvariantCulture)
        }
        return $rounded.ToString("0.0", [Globalization.CultureInfo]::InvariantCulture)
    }
    if ($Value -is [System.Collections.IList]) {
        if ($Value.Count -eq 0) { return "[]" }
        $lines = @()
        foreach ($item in $Value) {
            $lines += ($next + (ConvertTo-PlainJson -Value $item -Indent ($Indent + 1)))
        }
        return "[" + [Environment]::NewLine + ($lines -join ("," + [Environment]::NewLine)) + [Environment]::NewLine + $pad + "]"
    }
    $map = $Value
    if ($Value -is [pscustomobject]) {
        $map = [ordered]@{}
        foreach ($prop in $Value.PSObject.Properties) { $map[$prop.Name] = $prop.Value }
    }
    if ($map -is [System.Collections.IDictionary]) {
        if ($map.Count -eq 0) { return "{}" }
        $lines = @()
        foreach ($key in @($map.Keys)) {
            $encoded = ConvertTo-PlainJson -Value $map[$key] -Indent ($Indent + 1)
            $lines += ($next + '"' + $key + '": ' + $encoded)
        }
        return "{" + [Environment]::NewLine + ($lines -join ("," + [Environment]::NewLine)) + [Environment]::NewLine + $pad + "}"
    }
    throw "Unsupported JSON value: $($Value.GetType().FullName)"
}

function Write-JsonFile {
    param([string]$Path, $Value)
    $directory = Split-Path -Parent $Path
    if ($directory -and -not (Test-Path -LiteralPath $directory)) {
        New-Item -ItemType Directory -Force -Path $directory | Out-Null
    }
    $json = ConvertTo-PlainJson $Value
    $utf8 = New-Object System.Text.UTF8Encoding $false
    [System.IO.File]::WriteAllText($Path, $json + [Environment]::NewLine, $utf8)
}

function Copy-Defs {
    param($Defs)
    return @{ Winnt = $Defs.Winnt; Ntddi = $Defs.Ntddi }
}

function Format-WinntHex {
    param($Value)
    if ($null -eq $Value) { return $null }
    $number = [int64]$Value
    if ($number -le 0xFFFF) { return "0x{0:X4}" -f $number }
    return "0x{0:X8}" -f $number
}

function Format-NtddiHex {
    param($Value)
    if ($null -eq $Value) { return $null }
    return "0x{0:X8}" -f ([int64]$Value)
}

function Get-CMakeBody {
    param([string]$Text)
    $builder = New-Object System.Text.StringBuilder
    $quote = $false
    for ($i = 0; $i -lt $Text.Length; $i++) {
        $char = $Text[$i]
        if ($char -eq '"' -and -not $quote) { $quote = $true; [void]$builder.Append($char); continue }
        if ($char -eq '"' -and $quote) { $quote = $false; [void]$builder.Append($char); continue }
        if (-not $quote -and $char -eq '#') {
            while ($i -lt $Text.Length -and $Text[$i] -ne "`n") { $i++ }
            if ($i -lt $Text.Length) { [void]$builder.Append("`n") }
            continue
        }
        [void]$builder.Append($char)
    }
    return $builder.ToString()
}

function Get-CMakeCommands {
    param([string]$Text)
    $commands = @()
    $length = $Text.Length
    $i = 0
    while ($i -lt $length) {
        while ($i -lt $length -and [char]::IsWhiteSpace($Text[$i])) { $i++ }
        if ($i -ge $length) { break }
        $start = $i
        while ($i -lt $length -and ($Text[$i] -match '[A-Za-z0-9_]')) { $i++ }
        if ($i -eq $start) { $i++; continue }
        $name = $Text.Substring($start, $i - $start)
        while ($i -lt $length -and [char]::IsWhiteSpace($Text[$i])) { $i++ }
        if ($i -ge $length -or $Text[$i] -ne '(') { continue }
        $depth = 0
        $argStart = $i + 1
        while ($i -lt $length) {
            $char = $Text[$i]
            if ($char -eq '(') { $depth++ }
            elseif ($char -eq ')') {
                $depth--
                if ($depth -eq 0) { break }
            }
            $i++
        }
        $args = ""
        if ($i -lt $length -and $depth -eq 0) {
            $args = $Text.Substring($argStart, $i - $argStart)
            $i++
        }
        $commands += [pscustomobject]@{ Name = $name.ToLowerInvariant(); Args = $args }
    }
    return $commands
}

function Get-CMakeTokens {
    param([string]$Text)
    $tokens = @()
    $length = $Text.Length
    $i = 0
    while ($i -lt $length) {
        while ($i -lt $length -and [char]::IsWhiteSpace($Text[$i])) { $i++ }
        if ($i -ge $length) { break }
        $char = $Text[$i]
        if ($char -eq '"') {
            $i++
            $start = $i
            while ($i -lt $length -and $Text[$i] -ne '"') { $i++ }
            $tokens += $Text.Substring($start, $i - $start)
            if ($i -lt $length) { $i++ }
            continue
        }
        if ($char -eq '(' -or $char -eq ')') {
            $tokens += [string]$char
            $i++
            continue
        }
        $start = $i
        while ($i -lt $length -and -not [char]::IsWhiteSpace($Text[$i]) -and $Text[$i] -ne '(' -and $Text[$i] -ne ')') { $i++ }
        $tokens += $Text.Substring($start, $i - $start)
    }
    return $tokens
}

function ConvertTo-VersionNumber {
    param([string]$Token)
    if ($Token -notmatch '^(?i)(?:-D|/D)(?:_WIN32_WINNT|NTDDI_VERSION)=(?<hex>0x[0-9a-f]+)$' -and
        $Token -notmatch '^(?i)(?:_WIN32_WINNT|NTDDI_VERSION)=(?<hex>0x[0-9a-f]+)$') {
        return $null
    }
    $hex = $Matches['hex']
    $kind = "ntddi"
    if ($Token -match '(?i)_WIN32_WINNT') { $kind = "winnt" }
    return [pscustomobject]@{ Kind = $kind; Value = [int64]$hex }
}

function Set-DefValue {
    param($Defs, [string]$Kind, $Value)
    if ($Kind -eq "winnt") { $Defs.Winnt = $Value }
    else { $Defs.Ntddi = $Value }
}

function Remove-DefValue {
    param($Defs, [string]$Kind, $Value)
    if ($Kind -eq "winnt") {
        if ($null -ne $Defs.Winnt -and [int64]$Defs.Winnt -eq [int64]$Value) { $Defs.Winnt = $null }
    }
    elseif ($null -ne $Defs.Ntddi -and [int64]$Defs.Ntddi -eq [int64]$Value) {
        $Defs.Ntddi = $null
    }
}

function Apply-DefinitionTokens {
    param($Defs, [string[]]$Tokens, [bool]$Remove)
    foreach ($token in $Tokens) {
        $parsed = ConvertTo-VersionNumber $token
        if (-not $parsed) { continue }
        if ($Remove) { Remove-DefValue $Defs $parsed.Kind $parsed.Value }
        else { Set-DefValue $Defs $parsed.Kind $parsed.Value }
    }
}

function Resolve-CMakeInput {
    param([string]$FromFile, [string]$Raw, [string]$Root, [bool]$Directory)
    $text = $Raw.Trim().Trim('"')
    $text = $text -replace '\$\{REACTOS_SOURCE_DIR\}/?', ''
    $text = $text -replace '\$\{CMAKE_CURRENT_SOURCE_DIR\}/?', ''
    $text = $text -replace '\$\{CMAKE_CURRENT_LIST_DIR\}/?', ''
    if ($text -match '\$\{') { return $null }
    $text = $text -replace '\\', '/'
    $bases = @(
        (Split-Path -Parent $FromFile),
        $Root,
        (Join-Path $Root "sdk\cmake")
    )
    foreach ($base in $bases) {
        $candidate = Join-Path $base ($text -replace '/', '\')
        if ($Directory) {
            $list = Join-Path $candidate "CMakeLists.txt"
            if (Test-Path -LiteralPath $list -PathType Leaf) { return $list }
        }
        elseif (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
        elseif (Test-Path -LiteralPath $candidate -PathType Container) {
            $list = Join-Path $candidate "CMakeLists.txt"
            if (Test-Path -LiteralPath $list -PathType Leaf) { return $list }
        }
    }
    return $null
}

function Test-BoolToken {
    param([string]$Token, $Vars)
    switch ($Token.ToUpperInvariant()) {
        "TRUE" { return $true }
        "ON" { return $true }
        "YES" { return $true }
        "FALSE" { return $false }
        "OFF" { return $false }
        "NO" { return $false }
    }
    if ($Token -eq "1") { return $true }
    if ($Token -eq "0") { return $false }
    if ($Vars.ContainsKey($Token)) { return [bool]$Vars[$Token] }
    return $false
}

function Expand-CMakePath {
    param([string]$Token, [string]$FromFile, [string]$Root)
    $text = $Token
    $dir = (Split-Path -Parent $FromFile) -replace '\\', '/'
    $rootText = $Root -replace '\\', '/'
    $text = $text.Replace('${CMAKE_CURRENT_SOURCE_DIR}', $dir)
    $text = $text.Replace('${CMAKE_CURRENT_LIST_DIR}', $dir)
    $text = $text.Replace('${REACTOS_SOURCE_DIR}', $rootText)
    return $text
}

function Invoke-CMakeCondition {
    param([string[]]$Tokens, [int]$Index, $Vars, [string]$FromFile, [string]$Root)
    $state = @{ Index = $Index; Unknown = $false }
    function Peek { if ($state.Index -ge $Tokens.Count) { return $null }; return $Tokens[$state.Index] }
    function Take { $token = Peek; if ($null -ne $token) { $state.Index++ }; return $token }

    function Eval-Primary {
        $token = Take
        if ($null -eq $token) { $state.Unknown = $true; return $false }
        if ($token -eq "(") {
            $value = Eval-Or
            if ((Peek) -eq ")") { Take | Out-Null }
            return $value
        }
        if ($token -match '^(?i)NOT$') { return -not (Eval-Primary) }
        if ($token -match '^(?i)DEFINED$') {
            $name = Take
            return $Vars.ContainsKey($name)
        }
        if ($token -match '^(?i)EXISTS$') {
            $path = Expand-CMakePath -Token (Take) -FromFile $FromFile -Root $Root
            if ([string]::IsNullOrWhiteSpace($path)) { return $false }
            return (Test-Path -LiteralPath ($path -replace '/', '\'))
        }
        $next = Peek
        if ($next -and $next -match '^(?i)STREQUAL$') {
            Take | Out-Null
            $right = Take
            $left = $token
            if ($Vars.ContainsKey($left)) { $left = [string]$Vars[$left] }
            return [string]$left -eq [string]$right
        }
        if ($next -and $next -match '^(?i)MATCHES$') {
            Take | Out-Null
            $pattern = Take
            $left = $token
            if ($Vars.ContainsKey($left)) { $left = [string]$Vars[$left] }
            return [string]$left -match $pattern
        }
        if ($next -and $next -match '^(?i)VERSION_') { $state.Unknown = $true; return $true }
        return (Test-BoolToken $token $Vars)
    }
    function Eval-And {
        $value = Eval-Primary
        while ((Peek) -and (Peek) -match '^(?i)AND$') {
            Take | Out-Null
            $right = Eval-Primary
            $value = $value -and $right
        }
        return $value
    }
    function Eval-Or {
        $value = Eval-And
        while ((Peek) -and (Peek) -match '^(?i)OR$') {
            Take | Out-Null
            $right = Eval-And
            $value = $value -or $right
        }
        return $value
    }
    $value = Eval-Or
    return [pscustomobject]@{ Value = [bool]$value; Unknown = [bool]$state.Unknown }
}

function Get-TargetName {
    param([string]$ArgumentText)
    $tokens = @(Get-CMakeTokens $ArgumentText)
    if ($tokens.Count -eq 0) { return $null }
    $name = $tokens[0].Trim('"')
    if (-not $name -or $name.StartsWith('$')) { return $null }
    $skip = @("INTERFACE", "ALIAS", "IMPORTED")
    if ($tokens.Count -gt 1 -and $skip -contains $tokens[1].ToUpperInvariant()) { return $null }
    return $name
}

function Get-ModuleLayer {
    param([string]$RelativePath, [string]$Name)
    $path = $RelativePath.Replace('\', '/').ToLowerInvariant()
    $module = $Name.ToLowerInvariant()
    if ($module -in @("ntoskrnl", "ntkrnlmp", "hal", "halmp", "win32k", "win32ksys") -or
        $path.StartsWith("ntoskrnl/") -or $path.StartsWith("hal/") -or $path.StartsWith("boot/")) {
        return "Core Kernel"
    }
    $graphics = @(
        "win32ss/gdi/", "win32ss/reactx/", "win32ss/drivers/displays/",
        "dll/directx/", "drivers/video/", "drivers/wdm/audio/",
        "dll/win32/gdiplus/", "dll/win32/msimg32/", "dll/win32/avifil32/",
        "dll/win32/mmdevapi/", "dll/win32/wdmaud.drv/"
    )
    foreach ($prefix in $graphics) {
        if ($path.StartsWith($prefix)) { return "Graphics/Media" }
    }
    $shell = @(
        "base/shell/", "dll/shellext/", "base/ctf/",
        "dll/win32/shell32/", "dll/win32/ole32/", "dll/win32/oleaut32/",
        "dll/win32/combase/", "dll/win32/shlwapi/", "dll/win32/shcore/",
        "dll/win32/actxprxy/", "dll/win32/urlmon/", "dll/win32/mshtml/",
        "dll/win32/comctl32/", "dll/win32/comdlg32/", "dll/win32/browseui/",
        "dll/win32/oleacc/"
    )
    foreach ($prefix in $shell) {
        if ($path.StartsWith($prefix)) { return "Shell/COM" }
    }
    $device = @(
        "drivers/bus/", "drivers/usb/", "drivers/network/", "drivers/input/",
        "drivers/serial/", "drivers/storage/", "drivers/hid/", "drivers/bluetooth/",
        "drivers/pcmcia/", "dll/win32/setupapi/", "dll/win32/cfgmgr32/",
        "dll/win32/devmgr/", "dll/win32/newdev/", "dll/win32/netcfgx/",
        "dll/win32/iphlpapi/", "dll/win32/ws2_32/", "dll/win32/mswsock/",
        "dll/win32/dnsapi/", "dll/win32/dhcpcsvc/", "dll/win32/rasapi32/",
        "dll/win32/wininet/", "dll/win32/winhttp/"
    )
    foreach ($prefix in $device) {
        if ($path.StartsWith($prefix)) { return "Device/PnP & Comms" }
    }
    if ($path.StartsWith("drivers/") -or $path.StartsWith("base/services/") -or
        $path.StartsWith("sdk/lib/drivers/") -or $path.StartsWith("win32ss/drivers/") -or
        $path.StartsWith("win32ss/printing/")) {
        return "Drivers/Services"
    }
    return "Core User-Mode"
}

function Read-SdkMacros {
    param([string]$HeaderPath)
    $rows = @()
    foreach ($line in (Get-Content -LiteralPath $HeaderPath)) {
        if ($line -notmatch '^\s*#define\s+(?<name>(?:NTDDI_|_WIN32_WINNT_)[A-Za-z0-9_]+)\s+(?<value>\S+)(?:\s+//\s*(?<comment>.*))?$') {
            continue
        }
        $comment = ""
        if ($Matches['comment']) { $comment = $Matches['comment'].Trim() }
        $rows += [pscustomobject]@{ Name = $Matches['name']; Raw = $Matches['value'].Trim(); Comment = $comment }
    }
    $resolved = @{}
    $pending = New-Object System.Collections.Generic.List[object]
    foreach ($row in $rows) { $pending.Add($row) }
    for ($pass = 0; $pass -lt 8 -and $pending.Count -gt 0; $pass++) {
        $next = New-Object System.Collections.Generic.List[object]
        foreach ($row in $pending) {
            if ($row.Raw -match '^(?i)0x[0-9a-f]+$') {
                $resolved[$row.Name] = [int64]$row.Raw
            }
            elseif ($resolved.ContainsKey($row.Raw)) {
                $resolved[$row.Name] = $resolved[$row.Raw]
            }
            else { $next.Add($row) }
        }
        $pending = $next
    }
    $byName = @{}
    foreach ($row in $rows) { $byName[$row.Name] = $row }
    return [pscustomobject]@{ Resolved = $resolved; ByName = $byName }
}

function Get-MacroLabel {
    param($Catalog, [int64]$Value, [string]$Kind)
    $candidates = @()
    foreach ($name in @($Catalog.Resolved.Keys)) {
        if ([int64]$Catalog.Resolved[$name] -ne $Value) { continue }
        if ($Kind -eq "winnt" -and -not $name.StartsWith("_WIN32_WINNT_")) { continue }
        if ($Kind -eq "ntddi" -and -not $name.StartsWith("NTDDI_")) { continue }
        $row = $Catalog.ByName[$name]
        $hex = $row.Raw -match '^(?i)0x[0-9a-f]+$'
        $candidates += [pscustomobject]@{ Name = $name; Comment = $row.Comment; Hex = [bool]$hex }
    }
    $commentedHex = @($candidates | Where-Object { $_.Hex -and $_.Comment })
    if ($commentedHex.Count -gt 0) { return $commentedHex[0].Comment }
    $commented = @($candidates | Where-Object { $_.Comment })
    if ($commented.Count -gt 0) { return $commented[0].Comment }
    foreach ($candidate in $candidates) {
        if ($candidate.Name -match '^NTDDI_(.+)SP(\d+)$') {
            $baseName = "NTDDI_" + $Matches[1]
            if ($Catalog.ByName.ContainsKey($baseName) -and $Catalog.ByName[$baseName].Comment) {
                return ($Catalog.ByName[$baseName].Comment + " SP" + $Matches[2])
            }
        }
    }
    if ($Kind -eq "winnt") {
        $osValue = ([int64]$Value) -shl 16
        $fromNtddi = Get-MacroLabel -Catalog $Catalog -Value $osValue -Kind "ntddi"
        if ($fromNtddi -and $fromNtddi -ne "Unlisted") {
            if ($Value -eq 0x0502) { return "Windows Server 2003 / XP x64" }
            return $fromNtddi
        }
    }
    if ($candidates.Count -gt 0) { return $candidates[0].Name }
    return "Unlisted"
}

function Complete-TargetVersion {
    param($Winnt, $Ntddi)
    $winntFrom = "cmake"
    $ntddiFrom = "cmake"
    if ($null -eq $Winnt) {
        $Winnt = [int64]0x0600
        $winntFrom = "header"
    }
    if ($null -eq $Ntddi) {
        $ntddiFrom = "header"
        if ([int64]$Winnt -eq 0x0502) { $Ntddi = [int64]0x05020100 }
        else { $Ntddi = ([int64]$Winnt -band 0xFFFF) -shl 16 }
    }
    return [pscustomobject]@{
        Winnt     = [int64]$Winnt
        Ntddi     = [int64]$Ntddi
        WinntFrom = $winntFrom
        NtddiFrom = $ntddiFrom
    }
}

function Invoke-SdkFile {
    param([string]$Path, $Defs, [bool]$Included, $IfStack)
    if ($null -eq $script:SdkIncludeStack) {
        $script:SdkIncludeStack = New-Object System.Collections.Generic.Stack[bool]
    }
    $script:SdkIncludeStack.Push([bool]$Included)
    try {
    if ([string]::IsNullOrWhiteSpace($Path)) { return }
    if (-not (Test-Path -LiteralPath $Path)) { return }
    $full = [System.IO.Path]::GetFullPath($Path)
    if ($script:SdkSeenFiles.ContainsKey($full) -and -not $script:SdkIncludeStack.Peek()) { return }
    if (-not $script:SdkIncludeStack.Peek()) { $script:SdkSeenFiles[$full] = $true }
    $rootFull = [System.IO.Path]::GetFullPath($script:SdkRoot)
    $relative = $full.Substring($rootFull.Length).TrimStart('\', '/') -replace '\\', '/'
    $created = @()
    $subdirs = @()
    $blockDepth = 0
    $raw = ""
    if (Test-Path -LiteralPath $full) { $raw = [System.IO.File]::ReadAllText($full) }
    $commands = @(Get-CMakeCommands (Get-CMakeBody $raw))
    if ($relative -eq 'CMakeLists.txt') {
        $subs = @($commands | Where-Object { $_.Name -eq 'add_subdirectory' })
        $ifs = @($commands | Where-Object { $_.Name -eq 'if' })
    }
    foreach ($command in $commands) {
        $taken = ($IfStack.Count -eq 0) -or $IfStack[$IfStack.Count - 1].Taken
        switch ($command.Name) {
            "function" { $blockDepth++; continue }
            "macro" { $blockDepth++; continue }
            "endfunction" { if ($blockDepth -gt 0) { $blockDepth-- }; continue }
            "endmacro" { if ($blockDepth -gt 0) { $blockDepth-- }; continue }
        }
        if ($blockDepth -gt 0) { continue }
        switch ($command.Name) {
            "if" {
                $parent = ($IfStack.Count -eq 0) -or $IfStack[$IfStack.Count - 1].Taken
                $result = Invoke-CMakeCondition -Tokens @(Get-CMakeTokens $command.Args) -Index 0 -Vars $script:SdkVars -FromFile $full -Root $script:SdkRoot
                $branch = $result.Value
                if ($result.Unknown) { $branch = $true }
                $IfStack.Add(@{ Taken = ($parent -and $branch); Seen = ([bool]$branch -or $result.Unknown) }) | Out-Null
                continue
            }
            "elseif" {
                if ($IfStack.Count -eq 0) { continue }
                $frame = $IfStack[$IfStack.Count - 1]
                $parent = $true
                if ($IfStack.Count -gt 1) { $parent = $IfStack[$IfStack.Count - 2].Taken }
                $result = Invoke-CMakeCondition -Tokens @(Get-CMakeTokens $command.Args) -Index 0 -Vars $script:SdkVars -FromFile $full -Root $script:SdkRoot
                if ($frame.Seen -or $result.Unknown) { $frame.Taken = $false; $frame.Seen = $true }
                else {
                    $frame.Taken = $parent -and $result.Value
                    if ($result.Value) { $frame.Seen = $true }
                }
                continue
            }
            "else" {
                if ($IfStack.Count -eq 0) { continue }
                $frame = $IfStack[$IfStack.Count - 1]
                $parent = $true
                if ($IfStack.Count -gt 1) { $parent = $IfStack[$IfStack.Count - 2].Taken }
                $frame.Taken = $parent -and -not $frame.Seen
                $frame.Seen = $true
                continue
            }
            "endif" {
                if ($IfStack.Count -gt 0) { $IfStack.RemoveAt($IfStack.Count - 1) }
                continue
            }
        }
        if (-not $taken) { continue }
        switch ($command.Name) {
            "option" {
                $tokens = @(Get-CMakeTokens $command.Args)
                if ($tokens.Count -ge 3 -and -not $script:SdkVars.ContainsKey($tokens[0])) {
                    $script:SdkVars[$tokens[0]] = Test-BoolToken $tokens[$tokens.Count - 1] $script:SdkVars
                }
            }
            "set" {
                $tokens = @(Get-CMakeTokens $command.Args)
                if ($tokens.Count -eq 2 -and $tokens[1] -match '^(?i)TRUE|FALSE|ON|OFF|0|1$') {
                    $script:SdkVars[$tokens[0]] = Test-BoolToken $tokens[1] $script:SdkVars
                }
            }
            "add_definitions" { Apply-DefinitionTokens $Defs @(Get-CMakeTokens $command.Args) $false }
            "add_compile_definitions" { Apply-DefinitionTokens $Defs @(Get-CMakeTokens $command.Args) $false }
            "remove_definitions" { Apply-DefinitionTokens $Defs @(Get-CMakeTokens $command.Args) $true }
            "remove_compile_definitions" { Apply-DefinitionTokens $Defs @(Get-CMakeTokens $command.Args) $true }
            "include" {
                $tokens = @(Get-CMakeTokens $command.Args)
                if ($tokens.Count -eq 0) { continue }
                $includePath = Resolve-CMakeInput -FromFile $full -Raw $tokens[0] -Root $script:SdkRoot -Directory $false
                if (-not $includePath) { continue }
                $nested = Invoke-SdkFile -Path $includePath -Defs $Defs -Included $true -IfStack $IfStack
                if ($nested) {
                    $created += @($nested.Created)
                    $subdirs += @($nested.Subdirs)
                }
            }
            "add_subdirectory" {
                $tokens = @(Get-CMakeTokens $command.Args)
                if ($tokens.Count -eq 0) { continue }
                $child = Resolve-CMakeInput -FromFile $full -Raw $tokens[0] -Root $script:SdkRoot -Directory $true
                if (-not $child) { continue }
                $subdirs += [pscustomobject]@{ Path = $child; Defs = (Copy-Defs $Defs) }
            }
            "add_library" {
                $name = Get-TargetName $command.Args
                if ($name) { $created += $name }
            }
            "add_executable" {
                $name = Get-TargetName $command.Args
                if ($name) { $created += $name }
            }
            "add_hal" {
                $name = Get-TargetName $command.Args
                if ($name) { $created += $name }
            }
            "target_compile_definitions" {
                $tokens = @(Get-CMakeTokens $command.Args)
                if ($tokens.Count -eq 0) { continue }
                $name = $tokens[0].Trim('"')
                if ($name.StartsWith('$')) { continue }
                $overlay = @{ WinntSet = $false; Winnt = $null; NtddiSet = $false; Ntddi = $null }
                if ($script:Overlays.ContainsKey($name)) { $overlay = $script:Overlays[$name] }
                foreach ($token in $tokens) {
                    $parsed = ConvertTo-VersionNumber $token
                    if (-not $parsed) { continue }
                    if ($parsed.Kind -eq "winnt") { $overlay.WinntSet = $true; $overlay.Winnt = $parsed.Value }
                    else { $overlay.NtddiSet = $true; $overlay.Ntddi = $parsed.Value }
                }
                $script:Overlays[$name] = $overlay
            }
        }
    }
    if (-not $script:SdkIncludeStack.Peek()) {
        foreach ($name in @($created | Select-Object -Unique)) {
            $winnt = $Defs.Winnt
            $ntddi = $Defs.Ntddi
            if ($script:Overlays.ContainsKey($name)) {
                $overlay = $script:Overlays[$name]
                if ($overlay.WinntSet) { $winnt = $overlay.Winnt }
                if ($overlay.NtddiSet) { $ntddi = $overlay.Ntddi }
            }
            $script:SdkAuditTargets.Add([pscustomobject]@{
                Name  = $name
                Path  = $relative
                Winnt = $winnt
                Ntddi = $ntddi
            }) | Out-Null
        }
        foreach ($subdir in @($subdirs)) {
            Invoke-SdkFile -Path $subdir.Path -Defs (Copy-Defs $subdir.Defs) -Included $false -IfStack (New-Object System.Collections.Generic.List[object]) | Out-Null
        }
    }
    return [pscustomobject]@{ Created = @($created); Subdirs = @($subdirs) }
    } finally {
        [void]$script:SdkIncludeStack.Pop()
    }
}

function Get-SdkAudit {
    param([string]$Root)
    $header = Join-Path $Root "sdk\include\psdk\sdkddkver.h"
    if (-not (Test-Path -LiteralPath $header)) { throw "sdkddkver.h was not found at $header" }
    $catalog = Read-SdkMacros $header
    $rootList = Join-Path $Root "CMakeLists.txt"
    if (-not (Test-Path -LiteralPath $rootList)) { throw "CMakeLists.txt was not found at $rootList" }

    $vars = @{
        ARCH                 = "amd64"
        MSVC                 = $false
        CMAKE_CROSSCOMPILING = $true
        CMAKE_C_COMPILER_ID  = "GNU"
        MSVC_IDE             = $false
        MSVC_VERSION         = 0
        CMAKE_VERSION        = "3.17.0"
    }
    $script:SdkAuditTargets = New-Object System.Collections.Generic.List[object]
    $script:SdkSeenFiles = @{}

    $script:SdkRoot = $Root
    $script:SdkVars = $vars
    $script:Overlays = @{}
    Invoke-SdkFile -Path $rootList -Defs @{ Winnt = $null; Ntddi = $null } -Included $false -IfStack (New-Object System.Collections.Generic.List[object]) | Out-Null

    $unique = @{}
    foreach ($target in $script:SdkAuditTargets) { $unique[$target.Name] = $target }
    $modules = @()
    foreach ($name in ($unique.Keys | Sort-Object)) {
        $target = $unique[$name]
        $done = Complete-TargetVersion -Winnt $target.Winnt -Ntddi $target.Ntddi
        $winntLabel = Get-MacroLabel -Catalog $catalog -Value $done.Winnt -Kind "winnt"
        $ntddiLabel = Get-MacroLabel -Catalog $catalog -Value $done.Ntddi -Kind "ntddi"
        $label = $ntddiLabel
        if ($label -eq "Unlisted") { $label = $winntLabel }
        $modules += [pscustomobject]@{
            Name         = $target.Name
            Path         = $target.Path
            Layer        = (Get-ModuleLayer $target.Path $target.Name)
            Win32Winnt   = (Format-WinntHex $done.Winnt)
            NtddiVersion = (Format-NtddiHex $done.Ntddi)
            WinntFrom    = $done.WinntFrom
            NtddiFrom    = $done.NtddiFrom
            Label        = $label
            WinntValue   = $done.Winnt
            NtddiValue   = $done.Ntddi
        }
    }
    return $modules
}

function New-SdkSections {
    param($Modules)
    $order = @(
        "Core Kernel",
        "Core User-Mode",
        "Device/PnP & Comms",
        "Shell/COM",
        "Graphics/Media",
        "Drivers/Services"
    )
    $groups = @()
    foreach ($layer in $order) {
        $rows = @($Modules | Where-Object { $_.Layer -eq $layer } | Sort-Object Name)
        if ($rows.Count -eq 0) { continue }
        $encoded = @()
        foreach ($row in $rows) {
            $encoded += [ordered]@{
                name         = $row.Name
                path         = $row.Path
                win32Winnt   = $row.Win32Winnt
                ntddiVersion = $row.NtddiVersion
                winntFrom    = $row.WinntFrom
                ntddiFrom    = $row.NtddiFrom
                label        = $row.Label
            }
        }
        $groups += [ordered]@{ layer = $layer; modules = $encoded }
    }
    $counts = @{}
    foreach ($row in @($Modules)) {
        $key = $row.Win32Winnt + "|" + $row.NtddiVersion + "|" + $row.Label
        if (-not $counts.ContainsKey($key)) {
            $counts[$key] = [pscustomobject]@{
                Win32Winnt   = $row.Win32Winnt
                NtddiVersion = $row.NtddiVersion
                Label        = $row.Label
                Count        = 0
                WinntValue   = $row.WinntValue
                NtddiValue   = $row.NtddiValue
            }
        }
        $counts[$key].Count++
    }
    $summary = @()
    foreach ($item in ($counts.Values | Sort-Object WinntValue, NtddiValue, Label)) {
        $summary += [ordered]@{
            win32Winnt   = $item.Win32Winnt
            ntddiVersion = $item.NtddiVersion
            label        = $item.Label
            count        = $item.Count
        }
    }
    return [pscustomobject]@{ Groups = $groups; Summary = $summary }
}

function Test-KernelPin {
    param($Modules)
    $expected = @{ ntoskrnl = "0x0502"; hal = "0x0502"; win32k = "0x0502" }
    $found = @{}
    foreach ($row in @($Modules)) {
        if ($expected.ContainsKey($row.Name)) { $found[$row.Name] = $row.Win32Winnt }
    }
    $problems = @()
    foreach ($name in @($expected.Keys | Sort-Object)) {
        if (-not $found.ContainsKey($name)) { $problems += "$name was not in the amd64 build" }
        elseif ($found[$name] -ne $expected[$name]) { $problems += "$name is $($found[$name])" }
    }
    if ($problems.Count -gt 0) {
        throw ("Core kernel SDK pin is 0x0502. " + ($problems -join "; "))
    }
}

function Get-SdkLevelPercent {
    param($Modules)
    if ($null -eq $Modules) { return $null }
    $seen = @{}
    $steps = New-Object System.Collections.Generic.List[int64]
    $values = @($Modules | ForEach-Object { [int64]$_.WinntValue })
    foreach ($value in ($values + [int64]0x0502 + [int64]0x0A00)) {
        if ($value -lt 0x0502 -or $value -gt 0x0A00) { continue }
        if ($seen.ContainsKey([string]$value)) { continue }
        $seen[[string]$value] = $true
        $steps.Add([int64]$value)
    }
    $sorted = @($steps | Sort-Object)
    $count = @($Modules).Count
    if ($sorted.Count -lt 2 -or $count -eq 0) { return $null }
    $span = $sorted.Count - 1
    $sum = 0.0
    foreach ($row in @($Modules)) {
        $value = [int64]$row.WinntValue
        if ($value  -ge  $sorted[$sorted.Count - 1]) { $sum += 100; continue }
        if ($value  -le  $sorted[0]) { continue }
        for ($i = 0; $i -lt $span; $i++) {
            $lo = [int64]$sorted[$i]
            $hi = [int64]$sorted[$i + 1]
            if ($value -lt $lo -or $value -gt $hi) { continue }
            $t = 0.0
            if ($hi -ne $lo) { $t = [double]($value - $lo) / [double]($hi - $lo) }
            $sum += 100.0 * ($i + $t) / $span
            break
        }
    }
    return [math]::Round($sum / $count, 1)
}

function Get-GraphicsPercent {
    param($Rows)
    $scores = @()
    foreach ($row in @($Rows)) {
        if ($null -eq $row) { continue }
        $text = ([string]$row.value).Trim().ToLowerInvariant()
        if ([string]$row.tone -eq "score" -or $text.EndsWith("%")) {
            $clean = $text.TrimEnd("%")
            $number = 0.0
            if ([double]::TryParse($clean, [Globalization.NumberStyles]::Float, [Globalization.CultureInfo]::InvariantCulture, [ref]$number)) {
                $scores += $number
            }
            continue
        }
        if ($text -eq "untested" -or $text -eq "unknown" -or $text -eq "") { continue }
        if ($text -eq "working" -or $text -eq "ready" -or $text -eq "ok") { $scores += 100.0 }
        elseif ($text -eq "partial") { $scores += 50.0 }
        elseif ($text -eq "missing" -or $text -eq "failing") { $scores += 0.0 }
    }
    if ($scores.Count -eq 0) { return $null }
    return [math]::Round(($scores | Measure-Object -Average).Average, 1)
}

function Get-SyscallPercent {
    param($Calls)
    $rows = @($Calls)
    if ($rows.Count -eq 0) { return $null }
    $matched = @($rows | Where-Object { $null -ne $_.compatibility -and [double]$_.compatibility -eq 100 }).Count
    return [math]::Round(100.0 * $matched / $rows.Count, 1)
}

function Get-TrendStamp {
    param([string]$At)
    $when = [datetime]::MinValue
    $styles = [Globalization.DateTimeStyles]::RoundtripKind
    $culture = [Globalization.CultureInfo]::InvariantCulture
    if (-not [datetime]::TryParse($At, $culture, $styles, [ref]$when)) {
        throw "Unparsed dashboard time: [$At]"
    }
    return $when.ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
}

function Get-TrendDay {
    param([string]$At)
    $when = [datetime]::MinValue
    $styles = [Globalization.DateTimeStyles]::RoundtripKind
    $culture = [Globalization.CultureInfo]::InvariantCulture
    if (-not [datetime]::TryParse($At, $culture, $styles, [ref]$when)) {
        throw "Unparsed dashboard time: [$At]"
    }
    return $when.ToUniversalTime().ToString("yyyy-MM-dd")
}

function Set-TrendField {
    param($Row, [string]$Name, $Value)
    if ($null -eq $Value) { return }
    $Row[$Name] = [math]::Round([double]$Value, 1)
}

function Get-CompatibilityTrend {
    param($Site, [string]$HistoryPath, $SdkLevel)
    $map = @{}
    if ($Site.PSObject.Properties.Name -contains "compatibilityTrend") {
        foreach ($point in @($Site.compatibilityTrend)) {
            if (-not $point.at) { continue }
            $day = Get-TrendDay ([string]$point.at)
            $map[$day] = [ordered]@{
                at        = ((Get-TrendStamp ([string]$point.at)))
                kernelAbi = $point.kernelAbi
                syscall   = $point.syscall
                graphics  = $point.graphics
                sdkLevel  = $point.sdkLevel
            }
        }
    }
    if ($HistoryPath -and (Test-Path -LiteralPath $HistoryPath)) {
        $history = Get-Content -LiteralPath $HistoryPath -Raw | ConvertFrom-Json
        foreach ($snap in $history) {
            if (-not $snap.capturedAt) { continue }
            $day = Get-TrendDay ([string]$snap.capturedAt)
            if (-not $map.ContainsKey($day)) {
                $map[$day] = [ordered]@{ at = ((Get-TrendStamp ([string]$snap.capturedAt))); kernelAbi = $null; syscall = $null; graphics = $null; sdkLevel = $null }
            }
            $syscall = $null
            if ($null -ne $snap.syscallOverall) { $syscall = $snap.syscallOverall }
            else { $syscall = Get-SyscallPercent $snap.syscalls }
            Set-TrendField $map[$day] "syscall" $syscall
            Set-TrendField $map[$day] "kernelAbi" $snap.kernelAbi
            Set-TrendField $map[$day] "graphics" (Get-GraphicsPercent $snap.graphics)
            Set-TrendField $map[$day] "sdkLevel" $snap.sdkLevel
        }
    }
    if ($Site.capturedAt) {
        $day = Get-TrendDay ([string]$Site.capturedAt)
        if (-not $map.ContainsKey($day)) {
            $map[$day] = [ordered]@{ at = ((Get-TrendStamp ([string]$Site.capturedAt))); kernelAbi = $null; syscall = $null; graphics = $null; sdkLevel = $null }
        } else {
            $map[$day].at = ((Get-TrendStamp ([string]$Site.capturedAt)))
        }
        $syscall = $null
        if ($null -ne $Site.syscallOverall) { $syscall = $Site.syscallOverall }
        else { $syscall = Get-SyscallPercent $Site.syscalls }
        Set-TrendField $map[$day] "kernelAbi" $Site.kernelAbi
        Set-TrendField $map[$day] "syscall" $syscall
        Set-TrendField $map[$day] "graphics" (Get-GraphicsPercent $Site.graphics)
        Set-TrendField $map[$day] "sdkLevel" $SdkLevel
    }
    $rows = @()
    foreach ($day in ($map.Keys | Sort-Object)) { $rows += $map[$day] }
    return $rows
}

function Merge-SdkSnapshot {
    param([string]$Path, $Groups, $Summary, [string]$Note, $Modules)
    if (-not (Test-Path -LiteralPath $Path)) { throw "Website snapshot was not found at $Path" }
    $level = Get-SdkLevelPercent $Modules
    $captured = (Get-Date).ToUniversalTime().ToString("yyyy-MM-ddTHH:mm:ssZ")
    . (Join-Path $PSScriptRoot "DashboardStore.ps1")
    Write-DashboardSection -DashboardRoot (Split-Path -Parent $Path) -Name "sdk-targets" -CapturedAt $captured -Values ([ordered]@{
        sdkTargets        = @($Groups)
        sdkTargetsSummary = @($Summary)
        sdkTargetsNote    = $Note
        sdkLevel          = $level
    })
}

$SdkNote = "Core kernel components (ntoskrnl, hal, and win32k) remain intentionally pinned to 0x0502 for stability while user-mode modules advance incrementally to 0x0A00 (NTDDI_WIN10_CO, Windows 10 version 21H2, build 19044) per Milestone 14."

function Invoke-SdkSelfTest {
    $root = Join-Path ([System.IO.Path]::GetTempPath()) ("sdk-targets-" + [guid]::NewGuid().ToString("N"))
    New-Item -ItemType Directory -Force -Path $root | Out-Null
    try {
        $headerDir = Join-Path $root "sdk\include\psdk"
        New-Item -ItemType Directory -Force -Path $headerDir | Out-Null
        $header = @"
#define _WIN32_WINNT_WS03                   0x0502
#define _WIN32_WINNT_VISTA                  0x0600
#define _WIN32_WINNT_WIN7                   0x0601
#define _WIN32_WINNT_WIN8                   0x0602
#define _WIN32_WINNT_WIN10                  0x0A00
#define NTDDI_WS03                          0x05020000      // Windows Server 2003
#define NTDDI_WS03SP1                       0x05020100
#define NTDDI_WS03SP4                       0x05020400
#define NTDDI_VISTA                         0x06000000      // Windows Vista
#define NTDDI_WIN7                          0x06010000      // Windows 7
#define NTDDI_WIN8                          0x06020000      // Windows 8
#define NTDDI_WIN10                         0x0A000000      // Windows 10
#define NTDDI_WIN10_CO                      0x0A00000B      // Windows 10 22H2 / NTDDI_WIN10_CO 19045
"@
        Set-Content -LiteralPath (Join-Path $headerDir "sdkddkver.h") -Value $header -Encoding ASCII
        $rootCmake = @'
if(NOT CMAKE_CROSSCOMPILING)
    add_subdirectory(hostonly)
else()
    add_definitions(-D_WIN32_WINNT=0x502)
    add_subdirectory(ntoskrnl)
    add_subdirectory(hal)
    add_subdirectory(win32ss)
    add_subdirectory(dll)
    add_subdirectory(drivers)
    if(ARCH STREQUAL "i386")
        add_subdirectory(skipped)
    endif()
endif()
'@
        Set-Content -LiteralPath (Join-Path $root "CMakeLists.txt") -Value $rootCmake -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "hostonly") | Out-Null
        Set-Content (Join-Path $root "hostonly\CMakeLists.txt") "add_library(hostonly STATIC empty.c)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "skipped") | Out-Null
        Set-Content (Join-Path $root "skipped\CMakeLists.txt") "add_definitions(-D_WIN32_WINNT=0x601)`nadd_library(skipped STATIC empty.c)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "ntoskrnl") | Out-Null
        Set-Content (Join-Path $root "ntoskrnl\CMakeLists.txt") "add_definitions(-DNTDDI_VERSION=0x05020400)`nadd_executable(ntoskrnl ntos.c)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "hal") | Out-Null
        Set-Content (Join-Path $root "hal\CMakeLists.txt") "function(add_hal _halname)`nadd_library(`${_halname} MODULE hal.c)`nendfunction()`nadd_hal(hal SOURCES hal.c)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "win32ss") | Out-Null
        Set-Content (Join-Path $root "win32ss\CMakeLists.txt") "add_library(win32k MODULE win32k.c)`n" -Encoding ASCII
        $dll = Join-Path $root "dll\win32\example"
        New-Item -ItemType Directory -Force -Path $dll | Out-Null
        Set-Content (Join-Path $root "dll\CMakeLists.txt") "add_subdirectory(win32)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "dll\win32") | Out-Null
        Set-Content (Join-Path $root "dll\win32\CMakeLists.txt") "add_subdirectory(example)`n" -Encoding ASCII
        Set-Content (Join-Path $dll "CMakeLists.txt") "remove_definitions(-D_WIN32_WINNT=0x502)`nadd_definitions(-D_WIN32_WINNT=0x600)`nadd_library(example SHARED example.c)`n" -Encoding ASCII
        $disk = Join-Path $root "drivers\storage\class\disk"
        New-Item -ItemType Directory -Force -Path $disk | Out-Null
        Set-Content (Join-Path $root "drivers\CMakeLists.txt") "add_subdirectory(storage)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "drivers\storage") | Out-Null
        Set-Content (Join-Path $root "drivers\storage\CMakeLists.txt") "add_subdirectory(class)`n" -Encoding ASCII
        New-Item -ItemType Directory -Force -Path (Join-Path $root "drivers\storage\class") | Out-Null
        Set-Content (Join-Path $root "drivers\storage\class\CMakeLists.txt") "add_subdirectory(disk)`n" -Encoding ASCII
        Set-Content (Join-Path $disk "CMakeLists.txt") "remove_definitions(-D_WIN32_WINNT=0x502)`ntarget_compile_definitions(disk PUBLIC _WIN32_WINNT=0x602 NTDDI_VERSION=0x06020000)`nadd_library(disk MODULE disk.c)`n" -Encoding ASCII
        $modules = @(Get-SdkAudit $root)
        $names = @($modules | ForEach-Object { $_.Name })
        if ($names -contains "hostonly" -or $names -contains "skipped") { throw "inactive branches were audited" }
        $nt = @($modules | Where-Object { $_.Name -eq "ntoskrnl" })[0]
        if ($nt.Win32Winnt -ne "0x0502" -or $nt.NtddiVersion -ne "0x05020400") { throw "ntoskrnl was $($nt.Win32Winnt) $($nt.NtddiVersion)" }
        $halRow = @($modules | Where-Object { $_.Name -eq "hal" })[0]
        if ($halRow.Win32Winnt -ne "0x0502" -or $halRow.NtddiVersion -ne "0x05020100" -or $halRow.NtddiFrom -ne "header") { throw "hal was $($halRow.Win32Winnt) $($halRow.NtddiVersion) $($halRow.NtddiFrom)" }
        $example = @($modules | Where-Object { $_.Name -eq "example" })[0]
        if ($example.Win32Winnt -ne "0x0600" -or $example.Label -notmatch "Vista") { throw "example was $($example.Win32Winnt) $($example.Label)" }
        $diskRow = @($modules | Where-Object { $_.Name -eq "disk" })[0]
        if ($diskRow.Win32Winnt -ne "0x0602" -or $diskRow.NtddiVersion -ne "0x06020000" -or $diskRow.Layer -ne "Device/PnP & Comms") {
            throw "disk was $($diskRow.Win32Winnt) $($diskRow.NtddiVersion) $($diskRow.Layer)"
        }
        Test-KernelPin $modules
        $sections = New-SdkSections $modules
        if (@($sections.Summary).Count -lt 3) { throw "summary collapsed distinct levels" }
        $site = Join-Path $root "latest.json"
        Write-JsonFile $site ([ordered]@{ sample = $false; overall = 80.2; syscalls = @(); notes = "kept" })
        Merge-SdkSnapshot -Path $site -Groups $sections.Groups -Summary $sections.Summary -Note $SdkNote
        $merged = Get-Content -LiteralPath $site -Raw | ConvertFrom-Json
        if ($merged.overall -ne 80.2 -or $merged.notes -ne "kept") { throw "existing snapshot fields changed" }
        if (@($merged.sdkTargetsSummary).Count -lt 3) { throw "summary was not merged" }
        Write-Host "SDK target fixture passed."
    }
    finally {
        if (Test-Path -LiteralPath $root) { Remove-Item -LiteralPath $root -Recurse -Force }
    }
}

if (-not $SourceRoot) {
    $SourceRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
}

if ($SelfTest) {
    Invoke-SdkSelfTest
    exit 0
}

if (-not $WebsitePath) { $WebsitePath = Join-Path $SourceRoot "veros\dashboard\latest.json" }
$modules = @(Get-SdkAudit $SourceRoot)
if ($modules.Count -eq 0) { throw "The amd64 build produced no module targets" }
Test-KernelPin $modules
$sections = New-SdkSections $modules
Merge-SdkSnapshot -Path $WebsitePath -Groups $sections.Groups -Summary $sections.Summary -Note $SdkNote -Modules $modules
Write-Host ("Updated $WebsitePath sdk targets: " + @($modules).Count + " modules, " + @($sections.Summary).Count + " levels")
exit 0
