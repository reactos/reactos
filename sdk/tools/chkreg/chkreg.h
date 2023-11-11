/*
 * PROJECT:     ReactOS Tools
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Check Registry Utility main header
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

#pragma once

/* INCLUDES *****************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <typedefs.h>

#define CMLIB_HOST
#include <cmlib.h>

/* GLOBALS *******************************************************************/

extern BOOLEAN FixBrokenHive;

/* DEFINES *******************************************************************/

// Definitions copied from <ntstatus.h>
// We only want to include host headers, so we define them manually
#define STATUS_SUCCESS                   ((NTSTATUS)0x00000000)
#define STATUS_UNSUCCESSFUL              ((NTSTATUS)0xC0000001)
#define STATUS_NO_MEMORY                 ((NTSTATUS)0xC0000017)
#define STATUS_REGISTRY_CORRUPT          ((NTSTATUS)0xC000014C)

/* FUNCTION PROTOTYPES *******************************************************/

/* hivchk.c */
BOOLEAN
ChkRegAnalyzeHive(
    IN PCSTR HiveName);

/* hivio.c */
BOOLEAN
NTAPI
CmpFileRead(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    OUT PVOID Buffer,
    IN SIZE_T BufferLength);

BOOLEAN
NTAPI
CmpFileWrite(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN PULONG FileOffset,
    IN PVOID Buffer,
    IN SIZE_T BufferLength);

BOOLEAN
NTAPI
CmpFileSetSize(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    IN ULONG FileSize,
    IN ULONG OldFileSize);

BOOLEAN
NTAPI
CmpFileFlush(
    IN PHHIVE RegistryHive,
    IN ULONG FileType,
    PLARGE_INTEGER FileOffset,
    ULONG Length);

BOOLEAN
ChkRegOpenHive(
    IN PCSTR HiveName,
    IN BOOLEAN WriteToHive,
    OUT PVOID *HiveData);

/* hivmem.c */
PVOID
NTAPI
CmpAllocate(
    IN SIZE_T Size,
    IN BOOLEAN Paged,
    IN ULONG Tag);

VOID
NTAPI
CmpFree(
    IN PVOID Ptr,
    IN ULONG Quota);

/* hivinit.c */
NTSTATUS
ChkRegInitializeHive(
    IN OUT PHHIVE RegistryHive,
    IN PVOID HiveData);

/* EOF */
