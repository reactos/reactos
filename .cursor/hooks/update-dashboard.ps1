# PROJECT:     VerOS NT
# LICENSE:     GNU General Public License, version 2 or later (GPL-2.0-or-later)
# FILE:        .cursor/hooks/update-dashboard.ps1
# PURPOSE:     Refresh the dashboard after a source change
# COPYRIGHT:   Copyright 2026 Ryan Wilson <ryan@verosnt.com>
# PROGRAMMERS: Ryan Wilson <ryan@verosnt.com>

$ErrorActionPreference = "Continue"
try { $null = [Console]::In.ReadToEnd() } catch { }
$hook = Join-Path $PSScriptRoot "..\..\tools\dashboard\Invoke-DashboardHook.ps1"
& powershell.exe -NoProfile -ExecutionPolicy Bypass -File $hook -FromWorkingTree
Write-Output "{}"
exit 0
