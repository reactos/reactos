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

#define PtrToPfn(p) \
    ((((ULONGLONG)(p)) >> PAGE_SHIFT) & 0xFFFFFFFFFULL)

#define VAtoPXI(va) ((((ULONG64)(va)) >> PXI_SHIFT) & 0x1FF)
#define VAtoPPI(va) ((((ULONG64)(va)) >> PPI_SHIFT) & 0x1FF)
#define VAtoPDI(va) ((((ULONG64)(va)) >> PDI_SHIFT) & 0x1FF)
#define VAtoPTI(va) ((((ULONG64)(va)) >> PTI_SHIFT) & 0x1FF)
