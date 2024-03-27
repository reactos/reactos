/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Generic BootCode support functions.
 * COPYRIGHT:   Copyright 2020-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

#pragma once

#ifdef SECTORSIZE
#undef SECTORSIZE
#endif
#define SECTORSIZE 512

typedef struct _BOOTCODE
{
    PVOID BootCode;
    ULONG Length;
} BOOTCODE, *PBOOTCODE;

/* Can be used as an element in a list which ends with a "NULL" region {0,0} */
typedef struct _BOOTCODE_REGION
{
    ULONG Offset;
    ULONG Length;
} BOOTCODE_REGION, *PBOOTCODE_REGION;


NTSTATUS
ReadBootCodeByHandle(
    _Inout_ PBOOTCODE BootCode,
    _In_ HANDLE FileHandle,
    _In_opt_ ULONG Length);

NTSTATUS
ReadBootCodeFromFile(
    _Inout_ PBOOTCODE BootCode,
    _In_ PUNICODE_STRING FilePath,
    _In_opt_ ULONG Length);

VOID
FreeBootCode(
    _Inout_ PBOOTCODE BootCode);

BOOLEAN
CompareBootCodes(
    _In_ PBOOTCODE BootCode1,
    _In_ PBOOTCODE BootCode2,
    _In_ PBOOTCODE_REGION ExcludeRegions);

PUCHAR
FindInBootCode(
    _In_ PBOOTCODE BootCode,
    _In_ PUCHAR Buffer,
    _In_ SIZE_T Length);

/* EOF */
