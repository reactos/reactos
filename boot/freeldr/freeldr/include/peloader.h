/*
 * PROJECT:     FreeLoader
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides routines for loading PE files.
 *              (Deprecated remark) To be merged with arch/i386/loader.c in future.
 *
 * COPYRIGHT:   Copyright 1998-2003 Brian Palmer <brianp@sginet.com>
 *              Copyright 2006-2019 Aleksey Bragin <aleksey@reactos.org>
 *
 * NOTES:       The source code in this file is based on the work of respective
 *              authors of PE loading code in ReactOS and Brian Palmer and
 *              Alex Ionescu's arch/i386/loader.c, and my research project
 *              (creating a native EFI loader for Windows).
 *
 *              This article was very handy during development:
 *              http://msdn.microsoft.com/msdnmag/issues/02/03/PE2/
 */

#pragma once

/* Optional user-provided callback used by the PE loader
 * when it loads DLLs imported by a main image. */
typedef VOID
(NTAPI *PELDR_IMPORTDLL_LOAD_CALLBACK)(
    _In_ PCSTR FileName);

extern PELDR_IMPORTDLL_LOAD_CALLBACK PeLdrImportDllLoadCallback;

BOOLEAN
PeLdrLoadImage(
    IN PCHAR FileName,
    IN TYPE_OF_MEMORY MemoryType,
    OUT PVOID *ImageBasePA);

BOOLEAN
PeLdrAllocateDataTableEntry(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCCH BaseDllName,
    IN PCCH FullDllName,
    IN PVOID BasePA,
    OUT PLDR_DATA_TABLE_ENTRY *NewEntry);

VOID
PeLdrFreeDataTableEntry(
    // _In_ PLIST_ENTRY ModuleListHead,
    _In_ PLDR_DATA_TABLE_ENTRY Entry);

BOOLEAN
PeLdrScanImportDescriptorTable(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCCH DirectoryPath,
    IN PLDR_DATA_TABLE_ENTRY ScanDTE);

BOOLEAN
PeLdrCheckForLoadedDll(
    IN OUT PLIST_ENTRY ModuleListHead,
    IN PCH DllName,
    OUT PLDR_DATA_TABLE_ENTRY *LoadedEntry);
