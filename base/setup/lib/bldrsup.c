/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/bldrsup.c
 * PURPOSE:         NT 5.x family (MS Windows <= 2003, and ReactOS)
 *                  boot loaders management.
 * PROGRAMMER:      Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

// TODO: Add support for NT 6.x family! (detection + BCD manipulation).

/* INCLUDES *****************************************************************/

#include "precomp.h"

#include "bldrsup.h"
#include "filesup.h"
#include "inicache.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

typedef struct _NTOS_BOOT_LOADER_FILES
{
    NTOS_BOOT_LOADER_TYPE Type;
    PCWSTR LoaderExecutable;
    PCWSTR LoaderConfigurationFile;
    // EnumerateInstallations;
} NTOS_BOOT_LOADER_FILES, *PNTOS_BOOT_LOADER_FILES;

// Question 1: What if config file is optional?
// Question 2: What if many config files are possible?
NTOS_BOOT_LOADER_FILES NtosBootLoaders[] =
{
    {FreeLdr, L"freeldr.sys", L"freeldr.ini"},
    {NtLdr  , L"ntldr"      , L"boot.ini"},     // FIXME: What about osloader.exe, etc...?
//  {NtLdr  , L"setupldr"   , L"txtsetup.sif"}, // FIXME
//  {BootMgr, L"bootmgr"    , L"BCD"}
};


/* FUNCTIONS ****************************************************************/

//
// We need, for each type of bootloader (FreeLdr, NtLdr, Bootmgr):
// 1. A function that detects its presence and its version;
// 2. A function that opens/closes its corresponding configuration file;
// 3. A function that adds a new boot entry. Note that for the first two BLDRs
//    this is a .INI file, while in the latter case this is a registry hive...
//

NTSTATUS
FindNTOSBootLoader( // By handle
    IN HANDLE PartitionHandle, // OPTIONAL
    IN NTOS_BOOT_LOADER_TYPE Type,
    OUT PULONG Version OPTIONAL)
// OUT PHANDLE ConfigFileHande OPTIONAL ????
{
    // UINT i;

    if (Type >= BldrTypeMax)
        return STATUS_INVALID_PARAMETER;

    // FIXME: Unused for now, but should be used later!!
    *Version = 0;
    // TODO: Check for BLDR version ONLY if Version != NULL

    /* Check whether the loader executable exists */
    if (!DoesFileExist(PartitionHandle, NtosBootLoaders[Type].LoaderExecutable))
    {
        /* The loader does not exist, continue with another one */
        // DPRINT1("Loader executable '%S' does not exist, continue with another one...\n", NtosBootLoaders[Type].LoaderExecutable);
        DPRINT1("Loader executable '%S' does not exist\n", NtosBootLoaders[Type].LoaderExecutable);
        return STATUS_NOT_FOUND;
    }

    /* Check whether the loader configuration file exists */
    if (!DoesFileExist(PartitionHandle, NtosBootLoaders[Type].LoaderConfigurationFile))
    {
        /* The loader does not exist, continue with another one */
        // FIXME: Consider it might be optional??
        // DPRINT1("Loader configuration file '%S' does not exist, continue with another one...\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        DPRINT1("Loader configuration file '%S' does not exist\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        return STATUS_NOT_FOUND;
    }

#if 0
    /* Check whether the loader configuration file exists */
    Status = OpenAndMapFile(PartitionHandle, NtosBootLoaders[Type].LoaderConfigurationFile,
                            &FileHandle, &SectionHandle, &ViewBase, &FileSize, FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* The loader does not exist, continue with another one */
        // FIXME: Consider it might be optional??
        DPRINT1("Loader configuration file '%S' does not exist, continue with another one...\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        return STATUS_NOT_FOUND;
    }
#endif

    return STATUS_SUCCESS;
}


static NTSTATUS
FreeLdrEnumerateBootEntries(
    IN PCHAR FileBuffer,
    IN ULONG FileLength,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL)
{
    NTSTATUS Status;
    PINICACHE IniCache;
    PINICACHEITERATOR Iterator;
    PINICACHESECTION IniSection, OsIniSection;
    PWCHAR SectionName, KeyData;
/**/NTOS_BOOT_ENTRY xxBootEntry;/**/
    PNTOS_BOOT_ENTRY BootEntry = &xxBootEntry;
    UNICODE_STRING InstallName;

    /* Open an *existing* FreeLdr.ini configuration file */
    Status = IniCacheLoadFromMemory(&IniCache, FileBuffer, FileLength, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get the "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"Operating Systems");
    if (IniSection == NULL)
    {
        IniCacheDestroy(IniCache);
        return STATUS_UNSUCCESSFUL;
    }

    /* Enumerate all the valid installations */
    Iterator = IniCacheFindFirstValue(IniSection, &SectionName, &KeyData);
    if (!Iterator) goto Quit;
    do
    {
        // FIXME: Poor-man quotes removal (improvement over bootsup.c:UpdateFreeLoaderIni).
        if (KeyData[0] == L'"')
        {
            /* Quoted name, copy up to the closing quote */
            PWCHAR Begin = &KeyData[1];
            PWCHAR End   = wcschr(Begin, L'"');
            if (!End)
                End = Begin + wcslen(Begin);
            RtlInitEmptyUnicodeString(&InstallName, Begin, (ULONG_PTR)End - (ULONG_PTR)Begin);
            InstallName.Length = InstallName.MaximumLength;
        }
        else
        {
            /* Non-quoted name, copy everything */
            RtlInitUnicodeString(&InstallName, KeyData);
        }

        DPRINT1("Boot entry '%wZ' in OS section '%S'\n", &InstallName, SectionName);

        /* Search for an existing ReactOS entry */
        OsIniSection = IniCacheGetSection(IniCache, SectionName);
        if (!OsIniSection)
            continue;

        /* Check for supported boot type "Windows2003" */
        Status = IniCacheGetKey(OsIniSection, L"BootType", &KeyData);
        if (NT_SUCCESS(Status))
        {
            // TODO: What to do with "Windows" ; "WindowsNT40" ; "ReactOSSetup" ?
            if ((KeyData == NULL) ||
                ( (_wcsicmp(KeyData, L"Windows2003") != 0) &&
                  (_wcsicmp(KeyData, L"\"Windows2003\"") != 0) ))
            {
                /* This is not a ReactOS entry */
                continue;
            }
        }
        else
        {
            /* Certainly not a ReactOS installation */
            continue;
        }

        /* BootType is Windows2003. Now check its SystemPath. */
        Status = IniCacheGetKey(OsIniSection, L"SystemPath", &KeyData);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("    A Win2k3 install '%wZ' without an ARC path?!\n", &InstallName);
            continue;
        }

        DPRINT1("    Found a candidate Win2k3 install '%wZ' with ARC path '%S'\n", &InstallName, KeyData);
        // KeyData == SystemRoot;

        BootEntry->FriendlyName = &InstallName;
        BootEntry->OsLoadPath = KeyData;
        /* Unused stuff (for now...) */
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptions = NULL;
        BootEntry->OsLoadOptions = NULL;

        Status = EnumBootEntriesRoutine(FreeLdr, BootEntry, Parameter);
        // TODO: Stop enumeration if !NT_SUCCESS(Status);
    }
    while (IniCacheFindNextValue(Iterator, &SectionName, &KeyData));

    IniCacheFindClose(Iterator);

Quit:
    IniCacheDestroy(IniCache);
    return STATUS_SUCCESS;
}

static NTSTATUS
NtLdrEnumerateBootEntries(
    IN PCHAR FileBuffer,
    IN ULONG FileLength,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL)
{
    NTSTATUS Status;
    PINICACHE IniCache;
    PINICACHEITERATOR Iterator;
    PINICACHESECTION IniSection;
    PWCHAR SectionName, KeyData;
/**/NTOS_BOOT_ENTRY xxBootEntry;/**/
    PNTOS_BOOT_ENTRY BootEntry = &xxBootEntry;
    UNICODE_STRING InstallName;

    /* Open an *existing* boot.ini configuration file */
    Status = IniCacheLoadFromMemory(&IniCache, FileBuffer, FileLength, FALSE);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Get the "Operating Systems" section */
    IniSection = IniCacheGetSection(IniCache, L"operating systems");
    if (IniSection == NULL)
    {
        IniCacheDestroy(IniCache);
        return STATUS_UNSUCCESSFUL;
    }

    /* Enumerate all the valid installations */
    Iterator = IniCacheFindFirstValue(IniSection, &SectionName, &KeyData);
    if (!Iterator) goto Quit;
    do
    {
        // FIXME: Poor-man quotes removal (improvement over bootsup.c:UpdateFreeLoaderIni).
        if (KeyData[0] == L'"')
        {
            /* Quoted name, copy up to the closing quote */
            PWCHAR Begin = &KeyData[1];
            PWCHAR End   = wcschr(Begin, L'"');
            if (!End)
                End = Begin + wcslen(Begin);
            RtlInitEmptyUnicodeString(&InstallName, Begin, (ULONG_PTR)End - (ULONG_PTR)Begin);
            InstallName.Length = InstallName.MaximumLength;
        }
        else
        {
            /* Non-quoted name, copy everything */
            RtlInitUnicodeString(&InstallName, KeyData);
        }

        DPRINT1("Boot entry '%wZ' in OS section '%S'\n", &InstallName, SectionName);

        DPRINT1("    Found a Win2k3 install '%wZ' with ARC path '%S'\n", &InstallName, SectionName);
        // SectionName == SystemRoot;

        BootEntry->FriendlyName = &InstallName;
        BootEntry->OsLoadPath = SectionName;
        /* Unused stuff (for now...) */
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptions = NULL;
        BootEntry->OsLoadOptions = NULL;

        Status = EnumBootEntriesRoutine(NtLdr, BootEntry, Parameter);
        // TODO: Stop enumeration if !NT_SUCCESS(Status);
    }
    while (IniCacheFindNextValue(Iterator, &SectionName, &KeyData));

    IniCacheFindClose(Iterator);

Quit:
    IniCacheDestroy(IniCache);
    return STATUS_SUCCESS;
}


// This function may be viewed as being similar to ntos:NtEnumerateBootEntries().
NTSTATUS
EnumerateNTOSBootEntries(
    IN HANDLE PartitionHandle, // OPTIONAL
    IN NTOS_BOOT_LOADER_TYPE Type,
//  IN ULONG Flags, // Determine which data to retrieve
    IN PENUM_BOOT_ENTRIES_ROUTINE EnumBootEntriesRoutine,
    IN PVOID Parameter OPTIONAL)
{
    NTSTATUS Status;
    HANDLE FileHandle;
    HANDLE SectionHandle;
    // SIZE_T ViewSize;
    ULONG FileSize;
    PVOID ViewBase;

    /*
     * NOTE: Currently we open & map the loader configuration file without
     * further tests. It's OK as long as we only deal with FreeLdr's freeldr.ini
     * and NTLDR's boot.ini files. But as soon as we'll implement support for
     * BOOTMGR detection, the "configuration file" will be the BCD registry
     * hive and then, we'll have instead to mount the hive & open it.
     */

    /* Check whether the loader configuration file exists */
    Status = OpenAndMapFile(PartitionHandle, NtosBootLoaders[Type].LoaderConfigurationFile,
                            &FileHandle, &SectionHandle, &ViewBase, &FileSize, FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* The loader does not exist, continue with another one */
        // FIXME: Consider it might be optional??
        // DPRINT1("Loader configuration file '%S' does not exist, continue with another one...\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        DPRINT1("Loader configuration file '%S' does not exist\n", NtosBootLoaders[Type].LoaderConfigurationFile);
        return Status;
    }

    /* The loader configuration file exists, interpret it to find valid installations */
    switch (NtosBootLoaders[Type].Type)
    {
    case FreeLdr:
        Status = FreeLdrEnumerateBootEntries(ViewBase, FileSize, /* Flags, */
                                             EnumBootEntriesRoutine, Parameter);
        break;

    case NtLdr:
        Status = NtLdrEnumerateBootEntries(ViewBase, FileSize, /* Flags, */
                                           EnumBootEntriesRoutine, Parameter);
        break;

    default:
        DPRINT1("Loader type %d is currently unsupported!\n", NtosBootLoaders[Type].Type);
        Status = STATUS_SUCCESS;
    }

    /* Finally, unmap and close the file */
    UnMapFile(SectionHandle, ViewBase);
    NtClose(FileHandle);

    return Status;
}

/* EOF */
