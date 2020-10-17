/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     BootCode support functions.
 * COPYRIGHT:   Copyright 2020 Hermes Belusca-Maito
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

NTSTATUS
ReadBootCodeByHandle(
    IN OUT PBOOTCODE BootCodeInfo,
    IN HANDLE FileHandle,
    IN ULONG Length OPTIONAL);

NTSTATUS
ReadBootCodeFromFile(
    IN OUT PBOOTCODE BootCodeInfo,
    IN PUNICODE_STRING FilePath,
    IN ULONG Length OPTIONAL);

VOID
FreeBootCode(
    IN OUT PBOOTCODE BootCodeInfo);

/* EOF */
