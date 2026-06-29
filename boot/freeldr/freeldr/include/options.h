/*
 * PROJECT:     FreeLoader
 * LICENSE:     Dual-licensed:
 *              GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     FreeLoader Setup and Configuration F2 menu.
 * COPYRIGHT:   Copyright 2022-2026 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#pragma once

#define HAS_OPTION_MENU_EDIT_CMDLINE
#define HAS_OPTION_MENU_CUSTOM_BOOT

VOID
FreeLdrSetupMenu(
    _In_opt_ OperatingSystemItem* OperatingSystem);

VOID
DisplayBootTimeOptions(
    _In_ OperatingSystemItem* OperatingSystem);

VOID OptionMenuReboot(VOID);
