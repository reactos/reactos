/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     ARM64 specific definitions
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

#define TAG_HW_RESOURCE_LIST    'lRwH'
#define TAG_HW_DISK_CONTEXT     'cDwH'

/* UEFI disks have no BIOS drive numbers */
UCHAR DriveMapGetBiosDriveNumber(PCSTR DeviceName);

VOID StallExecutionProcessor(ULONG Microseconds);

extern UCHAR FrldrBootDrive;
extern ULONG FrldrBootPartition;
