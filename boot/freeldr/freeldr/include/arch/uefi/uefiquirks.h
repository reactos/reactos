/*
 * PROJECT:     FreeLoader UEFI Support
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 *              or MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     UEFI "quirks" header
 * COPYRIGHT:   Copyright 2026 Leandro Friedrich <leandro@works-on-my.computer>
 */


/*
 * Some NVIDIA Tegra-based platforms (e.g. Surface RT) expose an unusable first GOP handle, resulting in no display output.
 * Using another restores display output.
 */
#define UEFI_QUIRK_SKIP_FIRST_GOP  0x00000001

ULONG
UefiGetMachineQuirks(VOID);
