/*
 * PROJECT:     ReactOS Setup Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Bootloader support functions
 * COPYRIGHT:   Copyright 2017-2024 Hermès Bélusca-Maïto <hermes.belusca@sfr.fr>
 */

/* INCLUDES ******************************************************************/

#include "precomp.h"

#include "bldrsup.h"
#include "filesup.h"
//#include "partlist.h"
#include "bootcode.h"
#include "bootsect.h"

#include "setuplib.h" // HAXX for IsUnattendedSetup and USETUP_DATA!!

#include "bootsup.h"

#define NDEBUG
#include <debug.h>

/* FREELDR/NTLDR BOOT LISTS FUNCTIONS ****************************************/

static VOID
TrimTrailingPathSeparators_UStr(
    _Inout_ PUNICODE_STRING UnicodeString)
{
    while (UnicodeString->Length >= sizeof(WCHAR) &&
           UnicodeString->Buffer[UnicodeString->Length / sizeof(WCHAR) - 1] == OBJ_NAME_PATH_SEPARATOR)
    {
        UnicodeString->Length -= sizeof(WCHAR);
    }
}


typedef struct _SETUP_BOOT_ENTRY
{
    ULONG_PTR BootEntryKey;
    PCWSTR FriendlyName;    // LoadIdentifier
    PCWSTR BootFilePath;    // EfiOsLoaderFilePath
    PCWSTR OsLoadPath;      // OsLoaderFilePath // OsFilePath
    PCWSTR OsLoadOptions;
} SETUP_BOOT_ENTRY, *PSETUP_BOOT_ENTRY;

// TODO:
// Global (in USETUP_DATA) list of the new boot entries (boot set list)
// to create in the system -- be it in freeldr.ini, boot.ini or elsewhere.
//
// For the time being, this is done with CreateFreeLoaderIniForReactOS()
// and the variables below to tell whether an extra boot set for the backed up
// boot sector needs to be created.
static BOOLEAN CreateBootEntryForBootSector = FALSE;
static SETUP_BOOT_ENTRY BootSector_Entry = {0};
static PWSTR BootSector_BootPath = NULL;
static PCWSTR BootSector_BootSector = NULL;

static BOOLEAN
AddBootEntriesFromINFFile(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PVOID BootStoreHandle,
    _In_ PCWSTR ArcPath)
{
    HINF InfFile;
    INFCONTEXT Context;
    UINT ErrorLine;
    INT IntValue;
    PCWSTR Value;
    BOOLEAN Success = FALSE;
    WCHAR FileNameBuffer[MAX_PATH];

    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;
    BOOT_STORE_OPTIONS BootOptions;

    PCWSTR CurrentBootEntry;
    ULONG BootTimeout;


    /* Get the SetupData/BootItems value */
    if (!SpInfFindFirstLine(pSetupData->SetupInf, L"SetupData", L"BootItems", &Context) ||
        !INF_GetData(&Context, NULL, &Value))
    {
        DPRINT1("Unable to find SetupData/BootItems file\n");
        return FALSE;
    }

    /* Build the path */
    CombinePaths(FileNameBuffer, ARRAYSIZE(FileNameBuffer), 2,
                 pSetupData->SourcePath.Buffer, Value);

    INF_FreeData(Value);

    /* Open the BootItems INF file */
    InfFile = SpInfOpenInfFile(FileNameBuffer,
                               NULL,
                               INF_STYLE_WIN4, // INF_STYLE_OLDNT,
                               pSetupData->LanguageId,
                               &ErrorLine);
    if (InfFile == INVALID_HANDLE_VALUE)
        return FALSE;


    /* Setup the common BootEntry data fields */
    BootEntry->Version = FreeLdr;
    BootEntry->BootFilePath = NULL;
    BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
    *(ULONGLONG*)Options->Signature = NTOS_OPTIONS_SIGNATURE;

    /*
     * Search for the "BootEntries" section, then enumerate
     * each boot entry and add them to the boot store.
     */
    Success = SpInfFindFirstLine(InfFile, L"BootEntries", NULL, &Context);
    if (!Success)
    {
        DPRINT1("Unable to find section '%S' in BOOTITEMS.INF\n", L"BootEntries");
        goto Quit;
    }
    for (; Success; Success = SpInfFindNextLine(&Context, &Context))
    {
        PCWSTR BootEntryKey;
        PCWSTR FriendlyName;
        PCWSTR BootFilePath;
        PCWSTR OsLoadPath;
        PCWSTR OsLoadOptions;

        /* Get BootEntryKey */
        if (!INF_GetDataField(&Context, 0, &BootEntryKey))
        {
            DPRINT1("BootEntryKey not found\n");
            continue; // Ignore this entry
        }

        /* Get FriendlyName */
        if (!INF_GetDataField(&Context, 1, &FriendlyName))
        {
            DPRINT1("FriendlyName not found\n");
            INF_FreeData(BootEntryKey);
            continue; // Ignore this entry
        }

        /* Get optional BootFilePath */
        BootFilePath = NULL;
        if (INF_GetDataField(&Context, 2, &BootFilePath) && !*BootFilePath)
        {
            INF_FreeData(BootFilePath);
            BootFilePath = NULL;
        }

        /* Get optional OsLoadPath */
        OsLoadPath = NULL;
        if (INF_GetDataField(&Context, 3, &OsLoadPath) && !*OsLoadPath)
        {
            INF_FreeData(OsLoadPath);
            OsLoadPath = NULL;
        }

        /* Get optional OsLoadOptions */
        OsLoadOptions = NULL;
        if (INF_GetDataField(&Context, 4, &OsLoadOptions) && !*OsLoadOptions)
        {
            INF_FreeData(OsLoadOptions);
            OsLoadOptions = NULL;
        }

        /* Add the boot entry */
        BootEntry->BootEntryKey = MAKESTRKEY(BootEntryKey);
        BootEntry->FriendlyName = FriendlyName;
        // BootEntry->BootFilePath = BootFilePath;
        Options->OsLoadPath = (OsLoadPath ? OsLoadPath : ArcPath);
        Options->OsLoadOptions = OsLoadOptions;
        AddBootStoreEntry(BootStoreHandle, BootEntry, NULL);

        INF_FreeData(BootEntryKey);
        INF_FreeData(FriendlyName);
        INF_FreeData(BootFilePath);
        INF_FreeData(OsLoadPath);
        INF_FreeData(OsLoadOptions);
    }

    /*
     * Set the general boot options
     */
    if (IsUnattendedSetup)
    {
        /* Get the BootLoaderOptions/CurrentBootEntryUnattend value */
        if (SpInfFindFirstLine(InfFile, L"BootLoaderOptions", L"CurrentBootEntryUnattend", &Context) &&
            INF_GetData(&Context, NULL, &Value))
        {
            CurrentBootEntry = Value; // string
            INF_FreeData(Value);
        }

        BootTimeout = 0; // Timeout 0 for unattended
    }
    else
    {
        /* Get the BootLoaderOptions/CurrentBootEntry value */
        if (SpInfFindFirstLine(InfFile, L"BootLoaderOptions", L"CurrentBootEntry", &Context) &&
            INF_GetData(&Context, NULL, &Value))
        {
            CurrentBootEntry = Value; // string
            INF_FreeData(Value);
        }

        /* Get the BootLoaderOptions/BootTimeout value */
        if (SpInfFindFirstLine(InfFile, L"BootLoaderOptions", L"BootTimeout", &Context) &&
            SpInfGetIntField(&Context, 1, &IntValue))
        {
            BootTimeout = IntValue;
        }
    }

    BootOptions.NextBootEntryKey = MAKESTRKEY(CurrentBootEntry);
    BootOptions.Timeout = BootTimeout;

    SetBootStoreOptions(BootStoreHandle, &BootOptions,
                        BOOT_OPTIONS_TIMEOUT | BOOT_OPTIONS_NEXT_BOOTENTRY_KEY);
    Success = TRUE;

Quit:
    /* Close the INF */
    SpInfCloseInfFile(InfFile);
    return Success;
}

static NTSTATUS
CreateFreeLoaderIniForReactOS(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PCWSTR IniPath,
    _In_ PCWSTR ArcPath)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) +
                      max(sizeof(NTOS_OPTIONS), sizeof(BOOTSECTOR_OPTIONS))];
    PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;

    /* Initialize the INI file and create the common FreeLdr sections */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr,
                           BS_CreateAlways /* BS_OpenAlways */, BS_ReadWriteAccess);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Create the "TitleText" and "MinimalUI" values in the "Display" section */
    SetCustomBootStoreOption(BootStoreHandle, L"Display", L"TitleText", L"ReactOS Boot Manager");
    SetCustomBootStoreOption(BootStoreHandle, L"Display", L"MinimalUI", L"Yes");

    /* Add the ReactOS entries */
    if (!AddBootEntriesFromINFFile(pSetupData, BootStoreHandle, ArcPath))
    {
        INFCONTEXT Context;
        PCWSTR Value, FriendlyName;
        PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

        /* We failed adding the custom boot entries: add a default one based
         * on the "LoadIdentifier" and "OsLoadOptions" from txtsetup.sif */
        DPRINT1("Could not add custom boot entries, use fallback\n");

        /* Get the SetupData/LoadIdentifier value */
        if (!SpInfFindFirstLine(pSetupData->SetupInf, L"SetupData", L"LoadIdentifier", &Context) ||
            !INF_GetData(&Context, NULL, &Value) || !*Value)
        {
            /* Use default "ReactOS" identifier */
            Value = NULL;
            FriendlyName = L"ReactOS";
        }
        else
        {
            FriendlyName = Value;
        }

        /* Setup the common BootEntry data fields */
        BootEntry->Version = FreeLdr;
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
        *(ULONGLONG*)Options->Signature = NTOS_OPTIONS_SIGNATURE;

        /* Add the boot entry */
        BootEntry->BootEntryKey = MAKESTRKEY(L"ReactOS"); // Hardcoded value
        BootEntry->FriendlyName = FriendlyName;
        // BootEntry->BootFilePath = BootFilePath;
        Options->OsLoadPath = ArcPath;
        Options->OsLoadOptions = L"/FASTDETECT";
        AddBootStoreEntry(BootStoreHandle, BootEntry, NULL);

        INF_FreeData(Value);
    }

    if (CreateBootEntryForBootSector)
    {
        PBOOTSECTOR_OPTIONS Options = (PBOOTSECTOR_OPTIONS)&BootEntry->OsOptions;

    WCHAR BootPathBuffer[MAX_PATH] = L"";

    /* Since the BootPath given here is in NT format
     * (not ARC), we need to hack-generate a mapping */
    ULONG DiskNumber = 0, PartitionNumber = 0;
    PCWSTR PathComponent = NULL;

    /* From the NT path, compute the disk, partition and path components */
    // NOTE: this function doesn't support stuff like \Device\FloppyX ...
    if (NtPathToDiskPartComponents(BootSector_BootPath, &DiskNumber, &PartitionNumber, &PathComponent))
    {
        DPRINT1("BootPath = '%S' points to disk #%d, partition #%d, path '%S'\n",
                BootSector_BootPath, DiskNumber, PartitionNumber, PathComponent);

        /* HACK-build a possible ARC path:
         * Hard disk path: multi(0)disk(0)rdisk(x)partition(y)[\path] */
        RtlStringCchPrintfW(BootPathBuffer, _countof(BootPathBuffer),
                            L"multi(0)disk(0)rdisk(%lu)partition(%lu)",
                            DiskNumber, PartitionNumber);
        if (PathComponent && *PathComponent &&
            (PathComponent[0] != L'\\' || PathComponent[1]))
        {
            RtlStringCchCatW(BootPathBuffer, _countof(BootPathBuffer),
                             PathComponent);
        }
    }
    else
    {
        PCWSTR Path = BootSector_BootPath;

        if ((_wcsnicmp(Path, L"\\Device\\Floppy", 14) == 0) &&
            (Path += 14) && iswdigit(*Path))
        {
            DiskNumber = wcstoul(Path, (PWSTR*)&PathComponent, 10);
            if (PathComponent && *PathComponent && *PathComponent != L'\\')
                PathComponent = NULL;

            /* HACK-build a possible ARC path:
             * Floppy disk path: multi(0)disk(0)fdisk(x)[\path] */
            RtlStringCchPrintfW(BootPathBuffer, _countof(BootPathBuffer),
                                L"multi(0)disk(0)fdisk(%lu)", DiskNumber);
            if (PathComponent && *PathComponent &&
                (PathComponent[0] != L'\\' || PathComponent[1]))
            {
                RtlStringCchCatW(BootPathBuffer, _countof(BootPathBuffer),
                                 PathComponent);
            }
        }
        else
        {
            /* HACK: Just keep the unresolved NT path and hope for the best... */

            /* Remove any trailing backslash if needed */
            UNICODE_STRING RootPartition;
            RtlInitUnicodeString(&RootPartition, BootSector_BootPath);
            TrimTrailingPathSeparators_UStr(&RootPartition);

            /* RootPartition is BootPath without counting any trailing
             * path separator. Because of this, we need to copy the string
             * in the buffer, instead of just using a pointer to it. */
            RtlStringCchPrintfW(BootPathBuffer, _countof(BootPathBuffer),
                                L"%wZ", &RootPartition);

            DPRINT1("Unhandled NT path '%S'\n", BootSector_BootPath);
        }
    }

        /* Setup the common BootEntry data fields */
        BootEntry->Version = FreeLdr;
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptionsLength = sizeof(BOOTSECTOR_OPTIONS);
        *(ULONGLONG*)Options->Signature = BOOTSECTOR_OPTIONS_SIGNATURE;

        Options->BootPath = BootPathBuffer;
        Options->FileName = BootSector_BootSector;

        BootEntry->BootEntryKey = BootSector_Entry.BootEntryKey;
        BootEntry->FriendlyName = BootSector_Entry.FriendlyName;
        AddBootStoreEntry(BootStoreHandle, BootEntry, NULL);

        /* We are done */
        CreateBootEntryForBootSector = FALSE;
        BootSector_BootPath = NULL;
        RtlZeroMemory(&BootSector_Entry, sizeof(BootSector_Entry));
    }

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS;
}


//
// I think this function can be generalizable as:
// "find the corresponding 'ReactOS' boot entry in this loader config file
// (here abstraction comes there), and if none, add a new one".
//

typedef struct _ENUM_REACTOS_ENTRIES_DATA
{
    ULONG i;
    BOOLEAN UseExistingEntry;
    PCWSTR ArcPath;
    WCHAR SectionName[80];
    WCHAR OsName[80];
} ENUM_REACTOS_ENTRIES_DATA, *PENUM_REACTOS_ENTRIES_DATA;

// PENUM_BOOT_ENTRIES_ROUTINE
static NTSTATUS
NTAPI
EnumerateReactOSEntries(
    _In_ BOOT_STORE_TYPE Type,
    _In_ PBOOT_STORE_ENTRY BootEntry,
    _In_opt_ PVOID Parameter)
{
    PENUM_REACTOS_ENTRIES_DATA Data = (PENUM_REACTOS_ENTRIES_DATA)Parameter;
    PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

    /* We have a boot entry */

    /* Check for supported boot type "Windows2003" */
    if ((BootEntry->OsOptionsLength < sizeof(NTOS_OPTIONS)) ||
        (*(ULONGLONG*)BootEntry->OsOptions /* Signature */ != NTOS_OPTIONS_SIGNATURE))
    {
        /* This is not a ReactOS entry */
        DPRINT("    An installation '%S' of unsupported type %lu\n",
               BootEntry->FriendlyName, BootEntry->OsOptionsLength);
        /* Continue the enumeration */
        goto SkipThisEntry;
    }

    /* BootType is Windows2003, now check OsLoadPath */
    if (!Options->OsLoadPath || !*Options->OsLoadPath)
    {
        /* Certainly not a ReactOS installation */
        DPRINT1("    A Win2k3 install '%S' without an ARC path?!\n", BootEntry->FriendlyName);
        /* Continue the enumeration */
        goto SkipThisEntry;
    }

    if (_wcsicmp(Options->OsLoadPath, Data->ArcPath) != 0)
    {
        /* This is a ReactOS entry, but the SystemRoot does not match
         * the one we are looking for: continue the enumeration */
        goto SkipThisEntry;
    }

    DPRINT("    Found a candidate Win2k3 install '%S' with ARC path '%S'\n",
           BootEntry->FriendlyName, Options->OsLoadPath);

    DPRINT("EnumerateReactOSEntries: OsLoadPath: '%S'\n", Options->OsLoadPath);

    Data->UseExistingEntry = TRUE;
    RtlStringCchCopyW(Data->OsName, ARRAYSIZE(Data->OsName), BootEntry->FriendlyName);

    /* We have found our entry, stop the enumeration now! */
    return STATUS_NO_MORE_ENTRIES;

SkipThisEntry:
    Data->UseExistingEntry = FALSE;
    if (Type == FreeLdr && wcscmp(Data->SectionName, (PWSTR)BootEntry->BootEntryKey)== 0)
    {
        RtlStringCchPrintfW(Data->SectionName, ARRAYSIZE(Data->SectionName),
                            L"ReactOS_%lu", Data->i);
        RtlStringCchPrintfW(Data->OsName, ARRAYSIZE(Data->OsName),
                            L"ReactOS %lu", Data->i);
        Data->i++;
    }
    return STATUS_SUCCESS;
}

static
NTSTATUS
UpdateFreeLoaderIni(
    IN PCWSTR IniPath,
    IN PCWSTR ArcPath)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    ENUM_REACTOS_ENTRIES_DATA Data;

    /* Open the INI file */
    Status = OpenBootStore(&BootStoreHandle, IniPath, FreeLdr,
                           BS_OpenExisting /* BS_OpenAlways */, BS_ReadWriteAccess);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Find an existing usable or an unused section name */
    Data.UseExistingEntry = TRUE;
    Data.i = 1;
    Data.ArcPath = ArcPath;
    RtlStringCchCopyW(Data.SectionName, ARRAYSIZE(Data.SectionName), L"ReactOS");
    RtlStringCchCopyW(Data.OsName, ARRAYSIZE(Data.OsName), L"ReactOS");

    //
    // FIXME: We temporarily use EnumerateBootStoreEntries, until
    // both QueryBootStoreEntry and ModifyBootStoreEntry get implemented.
    //
    Status = EnumerateBootStoreEntries(BootStoreHandle, EnumerateReactOSEntries, &Data);

    /* Create a new "ReactOS" entry if there is none already existing that suits us */
    if (!Data.UseExistingEntry)
    {
        UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
        PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
        PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

        // RtlStringCchPrintfW(Data.SectionName, ARRAYSIZE(Data.SectionName), L"ReactOS_%lu", Data.i);
        // RtlStringCchPrintfW(Data.OsName, ARRAYSIZE(Data.OsName), L"ReactOS %lu", Data.i);

        BootEntry->Version = FreeLdr;
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
        *(ULONGLONG*)Options->Signature = NTOS_OPTIONS_SIGNATURE;

        BootEntry->BootEntryKey = MAKESTRKEY(Data.SectionName);
        BootEntry->FriendlyName = Data.OsName;
        Options->OsLoadPath = ArcPath;
        Options->OsLoadOptions = L"/FASTDETECT";
        AddBootStoreEntry(BootStoreHandle, BootEntry, NULL);
    }

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS;
}

static
NTSTATUS
UpdateBootIni(
    IN PCWSTR IniPath,
    IN PCWSTR EntryName,    // ~= ArcPath
    IN PCWSTR EntryValue)
{
    NTSTATUS Status;
    PVOID BootStoreHandle;
    ENUM_REACTOS_ENTRIES_DATA Data;

    /* Open the INI file */
    Status = OpenBootStore(&BootStoreHandle, IniPath, NtLdr,
                           BS_OpenExisting /* BS_OpenAlways */, BS_ReadWriteAccess);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Find an existing usable or an unused section name */
    Data.UseExistingEntry = TRUE;
    // Data.i = 1;
    Data.ArcPath = EntryName;
    // RtlStringCchCopyW(Data.SectionName, ARRAYSIZE(Data.SectionName), L"ReactOS");
    RtlStringCchCopyW(Data.OsName, ARRAYSIZE(Data.OsName), L"ReactOS");

    //
    // FIXME: We temporarily use EnumerateBootStoreEntries, until
    // both QueryBootStoreEntry and ModifyBootStoreEntry get implemented.
    //
    Status = EnumerateBootStoreEntries(BootStoreHandle, EnumerateReactOSEntries, &Data);

    /* If either the key was not found, or contains something else, add a new one */
    if (!Data.UseExistingEntry /* ||
        ( (Status == STATUS_NO_MORE_ENTRIES) && wcscmp(Data.OsName, EntryValue) ) */)
    {
        // NOTE: Technically it should be "BootSector"...
        UCHAR xxBootEntry[FIELD_OFFSET(BOOT_STORE_ENTRY, OsOptions) + sizeof(NTOS_OPTIONS)];
        PBOOT_STORE_ENTRY BootEntry = (PBOOT_STORE_ENTRY)&xxBootEntry;
        PNTOS_OPTIONS Options = (PNTOS_OPTIONS)&BootEntry->OsOptions;

        BootEntry->Version = NtLdr;
        BootEntry->BootFilePath = NULL;
        BootEntry->OsOptionsLength = sizeof(NTOS_OPTIONS);
        *(ULONGLONG*)Options->Signature = NTOS_OPTIONS_SIGNATURE;

        BootEntry->BootEntryKey = MAKESTRKEY(0 /*Data.SectionName*/);
        // BootEntry->FriendlyName = Data.OsName;
        BootEntry->FriendlyName = EntryValue;
        Options->OsLoadPath = EntryName;
        Options->OsLoadOptions = NULL; // L"";
        AddBootStoreEntry(BootStoreHandle, BootEntry, NULL);
    }

    /* Close the INI file */
    CloseBootStore(BootStoreHandle);
    return STATUS_SUCCESS; // Status;
}


NTSTATUS
CopyBootManager(
    _In_ PCWSTR SourceRootPath,
    _In_ PCWSTR SystemRootPath/*,
    _In_ PCWSTR DestFileName*/)
{
    NTSTATUS Status;
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    // TODO: Retrieve freeldr.sys paths from TXTSETUP.SIF file

    /* Copy FreeLoader to the system partition, always overwriting the older version */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2, SourceRootPath, L"\\loader\\freeldr.sys");
    CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath, L"freeldr.sys");

    DPRINT("Copy: %S ==> %S\n", SrcPath, DstPath);
    Status = SetupCopyFile(SrcPath, DstPath, FALSE);
    if (!NT_SUCCESS(Status))
        DPRINT1("SetupCopyFile() failed: Status %lx\n", Status);
    return Status;
}

//
// TODO: Make this function more generic, that saves a global list
// of boot entries into either the Firmware or the current software
// (e.g. FreeLoader) Boot Manager.
// (--> Have a parameter that specifies the target boot store?)
//
NTSTATUS
SaveBootEntries(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PCWSTR SystemRootPath,
    _In_ PCWSTR DestinationArcPath)
{
    NTSTATUS Status;

    /* Create or update 'freeldr.ini' */
    if (!DoesFileExist_2(SystemRootPath, L"freeldr.ini"))
    {
        /* Create new 'freeldr.ini' */
        DPRINT1("Create new 'freeldr.ini'\n");
        Status = CreateFreeLoaderIniForReactOS(pSetupData, SystemRootPath, DestinationArcPath);
        if (!NT_SUCCESS(Status))
            DPRINT1("CreateFreeLoaderIniForReactOS() failed: Status %lx\n", Status);
    }
    else
    {
        /* Update existing 'freeldr.ini' */
        DPRINT1("Update existing 'freeldr.ini'\n");
        Status = UpdateFreeLoaderIni(SystemRootPath, DestinationArcPath);
        if (!NT_SUCCESS(Status))
            DPRINT1("UpdateFreeLoaderIni() failed: Status %lx\n", Status);
    }

    return Status;
}


/* BIOS-BASED PC FUNCTIONS ***************************************************/

static
BOOLEAN
IsBootSectorValid(
    _In_ PBOOTCODE BootCode)
{
    /*
     * We first demand that the bootsector has a valid signature at its end.
     * We then check the first 3 bytes (as a ULONG) of the bootsector for a
     * potential "valid" instruction (the BIOS starts execution of the bootsector
     * at its beginning). Currently this criterium is that this ULONG must be
     * non-zero. If both these tests pass, then the bootsector is valid; otherwise
     * it is invalid and certainly needs to be overwritten.
     */

    BOOLEAN IsValid = FALSE;

    /* Require a minimum length of SECTORSIZE */
    if (BootCode->Length < SECTORSIZE)
        return FALSE;

    /* Check for the existence of the bootsector signature */
    IsValid = (*(PUSHORT)((PUCHAR)BootCode->BootCode + 0x1FE) == 0xAA55);
    if (IsValid)
    {
        /* Check for the first instruction encoded on three bytes */
        IsValid = (((*(PULONG)BootCode->BootCode) & 0x00FFFFFF) != 0x00000000);
    }

    return IsValid;
}

static
NTSTATUS
SaveBootSector(
    _In_ PBOOTCODE BootCode,
    _In_ PCWSTR DstPath)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle;
    // LARGE_INTEGER FileOffset;

    /* Write the bootsector to DstPath */
    RtlInitUnicodeString(&Name, DstPath);
    InitializeObjectAttributes(&ObjectAttributes,
                               &Name,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtCreateFile(&FileHandle,
                          GENERIC_WRITE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE,
                          FILE_SYNCHRONOUS_IO_NONALERT | FILE_SEQUENTIAL_ONLY,
                          NULL,
                          0);
    if (!NT_SUCCESS(Status))
        return Status;

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         BootCode->BootCode,
                         BootCode->Length,
                         NULL,
                         NULL);
    NtClose(FileHandle);
    return Status;
}


static const struct
{
    PCSTR BootCodeName;
    PFS_INSTALL_BOOTCODE InstallBootCode;
    ULONG BackupSize;
    PCWSTR BootCodeFilePath; // TODO: Hardcoded paths. Retrieve from INF file?
} BootCodes[] =
{
    {"MBR"  , InstallMbrBootCode  , sizeof(PARTITION_SECTOR), L"\\loader\\dosmbr.bin"},
    {"FAT"  , InstallFatBootCode  , FAT_BOOTSECTOR_SIZE     , L"\\loader\\fat.bin"   }, // FAT12/16
    {"FAT32", InstallFat32BootCode, FAT32_BOOTSECTOR_SIZE   , L"\\loader\\fat32.bin" },
    {"NTFS" , InstallNtfsBootCode , NTFS_BOOTSECTOR_SIZE    , L"\\loader\\ntfs.bin"  },
    {"BTRFS", InstallBtrfsBootCode, BTRFS_BOOTSECTOR_SIZE   , L"\\loader\\btrfs.bin" },
};

typedef enum
{
    BT_MBR = 0,
    BT_FAT = 1,
    BT_FAT32,
    BT_NTFS,
    BT_BTRFS,
} BOOTCODE_ID;

/**
 * @brief
 * Installs a new MBR boot code to the first sector of a disk.
 * Only for BIOS-based PCs.
 *
 * @param[in]   SystemRootPath
 * System partition path.
 *
 * @param[in]   SourceRootPath
 * Installation source, where to copy the MBR boot sector file from.
 *
 * @param[in]   SystemDiskPath
 * The L"\\Device\\Harddisk%d\\Partition0" of the hard disk
 * containing the system partition.
 *
 * @param[in]   BackupOldMbr
 * TRUE if the caller wants to backup the old MBR in an "mbr.old"
 * file at the root of the system partition; FALSE if not.
 **/
static
NTSTATUS
InstallMBRToDisk(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCWSTR SystemDiskPath,
    _In_ BOOLEAN BackupOldMbr)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    BOOTCODE BootCode = {0};
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

#if 0
    /*
     * The SystemDiskPath parameter has been built with the following
     * instruction by the caller; I'm not yet sure whether I actually
     * want this function to build the path instead, hence I keep
     * this code here but disabled for now...
     */
    WCHAR SystemDiskPath[MAX_PATH];
    RtlStringCchPrintfW(SystemDiskPath, ARRAYSIZE(SystemDiskPath),
                        L"\\Device\\Harddisk%d\\Partition0",
                        DiskNumber);
#endif

    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2,
                 SourceRootPath->Buffer, BootCodes[BT_MBR].BootCodeFilePath);

    /* Allocate and read the new bootsector from SrcPath */
    RtlInitUnicodeString(&Name, SrcPath);
    Status = ReadBootCodeFromFile(&BootCode, &Name, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Optionally save the original MBR if it is valid */
    if (BackupOldMbr) do
    {
        /*
         * Verify whether the new MBR is identical to the old one,
         * in which case we will not back it up.
         */
        BOOTCODE OrgBootCode = {0};

        /* Remove any trailing backslash if needed */
        RtlInitUnicodeString(&Name, SystemDiskPath);
        TrimTrailingPathSeparators_UStr(&Name);

        /* Allocate and read the original disk MBR */
        Status = ReadBootCodeFromFile(&OrgBootCode, &Name,
                                      BootCodes[BT_MBR].BackupSize);
        if (!NT_SUCCESS(Status))
        {
            /* If we failed, there is no valid MBR */
            break;
        }
        if (!IsBootSectorValid(&OrgBootCode))
            goto Skip;

        /* Compare the MBR bootcodes, and save the
         * original one only if they are different */
        {
        BOOTCODE_REGION ExcludeRegion[] =
            {
                {FIELD_OFFSET(PARTITION_SECTOR, Signature),
                sizeof(PARTITION_SECTOR) -
                    FIELD_OFFSET(PARTITION_SECTOR, Signature)},
                {0, 0}
            };
        if (CompareBootCodes(&OrgBootCode, &BootCode, ExcludeRegion))
            goto Skip; // They are identical, return.
        }

        /* Backup the old MBR */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2,
                     SystemRootPath->Buffer, L"mbr.old");

        DPRINT1("Save MBR: %S ==> %S\n", SystemDiskPath, DstPath);
        Status = SaveBootSector(&OrgBootCode, DstPath);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SaveBootSector() failed: Status %lx\n", Status);
            // Don't care if we succeeded saving or not the old MBR, just go ahead.
        }

Skip:
        /* Free the bootsector and return */
        FreeBootCode(&OrgBootCode);
    } while (0);

    /* Remove any trailing backslash if needed */
    RtlInitUnicodeString(&Name, SystemDiskPath);
    TrimTrailingPathSeparators_UStr(&Name);

    /* Install the new MBR */
    DPRINT1("Install %s bootcode: %S ==> %S\n",
            BootCodes[BT_MBR].BootCodeName, SrcPath, SystemDiskPath);
    Status = InstallBootCodeToDisk(&BootCode, &Name,
                                   BootCodes[BT_MBR].InstallBootCode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("InstallBootCodeToDisk(%s) failed: Status %lx\n",
                BootCodes[BT_MBR].BootCodeName, Status);
    }

    /* Free the bootsector and return */
    FreeBootCode(&BootCode);
    return Status;
}


/**
 * @brief
 * Attempts to recognize well-known DOS, OS/2 or Windows 9x boot loaders
 * (for FAT12/16/32 volumes only).
 *
 * @param[in]   SystemRootPath
 * Drive where to check for a boot loader.
 *
 * @param[out]  Section, Description, BootSector, BootSectorSize
 * Suggested boot sector parameters for creating a boot replacement entry.
 *
 * @return  TRUE in case a boot loader was recognized; FALSE if not.
 **/
static
BOOLEAN
RecognizeDOSLoader(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PBOOTCODE BootCode,
    _Out_ PCWSTR* Section,
    _Out_ PCWSTR* Description,
    _Out_ PCWSTR* BootSector,
    _Out_ PULONG BootSectorSize)
{
    typedef struct _BOOTCODE_BYTES
    {
        PUCHAR Buffer;
        SIZE_T Length;
    } BOOTCODE_BYTES, *PBOOTCODE_BYTES;
#define BYTES(str)  {(PUCHAR)(str), sizeof(str)-sizeof((str)[0])}

    typedef struct _LOADER_DESC
    {
        PCSTR DbgDescription;   //< String for the DPRINT1("Found %s\n", ...); message.
        PBOOTCODE_BYTES BootCodeBytes; //< Optional NULL-terminated list of pairs of bytes to find in the boot sector.
        PCWSTR* BootFiles;      //< Optional NULL-terminated list of boot files to check for.
        PCWSTR Section;         //< Suggested FREELDR.INI boot entry section name.
        PCWSTR Description;     //< Suggested FREELDR.INI boot entry description.
        PCWSTR BootSectorFileName;  //< Suggested backup boot sector file name.
    } LOADER_DESC, *PLOADER_DESC;

/* COMPAQ MS-DOS 1.x (1.11, 1.12, based on MS-DOS 1.25) boot loader */
    static BOOTCODE_BYTES BytesCompaqDos[] = {{NULL, 0}};
    static PCWSTR         FilesCompaqDos[] = {L"IOSYS.COM", L"MSDOS.COM", NULL};
    static const LOADER_DESC DescCompaqDos =
    {
        "COMPAQ MS-DOS 1.x (1.11, 1.12) / MS-DOS 1.25",
        BytesCompaqDos,
        FilesCompaqDos,
        L"CPQDOS",
        L"COMPAQ MS-DOS 1.x / MS-DOS 1.25",
        L"BOOTSECT.DOS"
    };
/* Microsoft DOS or Windows 9x boot loader */
    static BOOTCODE_BYTES BytesMsDos[] = {BYTES("IO      SYS"), {NULL, 0}}; // and "MSDOS   SYS" ?
    static PCWSTR         FilesMsDos[] = {L"IO.SYS", L"MSDOS.SYS", NULL};
    static const LOADER_DESC DescMsDos =
    {
        "Microsoft DOS or Windows 9x",
        BytesMsDos,
        FilesMsDos,
        L"MSDOS",
        L"MS-DOS/Windows 9x",
        L"BOOTSECT.DOS"
    };
/* Windows 9x boot loader */
    static BOOTCODE_BYTES BytesWin9x[] = {BYTES("WINBOOT SYS"), {NULL, 0}};
    static PCWSTR         FilesWin9x[] = {L"WINBOOT.SYS", NULL};
    static const LOADER_DESC DescWin9x =
    {
        "Microsoft Windows 9x",
        BytesWin9x,
        FilesWin9x,
        L"WIN9X",
        L"Microsoft Windows 9x",
        L"BOOTSECT.W9X"
    };
/* IBM PC-DOS or DR-DOS 5.x boot loader */
    static BOOTCODE_BYTES BytesPcDos[] =
        {BYTES("IBMBIO  COM"), {NULL, 0}}; // and "IBMDOS  COM" ?
    static PCWSTR         FilesPcDos[] =
        {L"IBMIO.COM" /* Some people refer to this file instead of IBMBIO.COM */,
         L"IBMBIO.COM", L"IBMDOS.COM", NULL};
    static const LOADER_DESC DescPcDos =
    {
        "IBM PC-DOS or DR-DOS 5.x/6.x or IBM OS/2 1.0",
        BytesPcDos,
        FilesPcDos,
        L"IBMDOS",
        L"IBM PC-DOS or DR-DOS 5.x/6.x or IBM OS/2 1.0",
        L"BOOTSECT.DOS"
    };
/* DR-DOS 3.x boot loader */
    static BOOTCODE_BYTES BytesDrDos[] = {BYTES("DRBIOS  SYS"), {NULL, 0}};
    static PCWSTR         FilesDrDos[] = {L"DRBIOS.SYS", L"DRBDOS.SYS", NULL};
    static const LOADER_DESC DescDrDos =
    {
        "DR-DOS 3.x",
        BytesDrDos,
        FilesDrDos,
        L"DRDOS",
        L"DR-DOS 3.x",
        L"BOOTSECT.DOS"
    };
/* Enhanced DR-DOS 7.x boot loader */
    static BOOTCODE_BYTES BytesEnhDos[] = {BYTES("DRBIO   SYS"), {NULL, 0}};
    static PCWSTR         FilesEnhDos[] = {L"DRBIO.SYS", L"DRDOS.SYS", NULL};
    static const LOADER_DESC DescEnhDos =
    {
        "Enhanced DR-DOS 7.x",
        BytesEnhDos,
        FilesEnhDos,
        L"EDRDOS",
        L"Enhanced DR-DOS 7.x",
        L"BOOTSECT.EDR"
    };
/* Real-Time RxDOS boot loader */
    static BOOTCODE_BYTES BytesRxDos[] = {BYTES("RXDOSBIOSYSRXDOS   SYS"), {NULL, 0}};
    static PCWSTR         FilesRxDos[] = {L"RXDOSBIO.SYS", L"RXDOS.SYS", NULL};
    static const LOADER_DESC DescRxDos =
    {
        "Real-Time RxDOS 6/7",
        BytesRxDos,
        FilesRxDos,
        L"RXDOS",
        L"RxDOS 6/7",
        L"BOOTSECT.RXD"
    };
/* Dell Real-Mode Kernel (DRMK) OS */ // v4 or v8.00 ?
    static BOOTCODE_BYTES BytesDrmk[] = {BYTES("DELLBIO BIN"), {NULL, 0}};
    static PCWSTR         FilesDrmk[] = {L"DELLBIO.BIN", L"DELLRMK.BIN", NULL};
    static const LOADER_DESC DescDrmk =
    {
        "Dell Real-Mode Kernel OS",
        BytesDrmk,
        FilesDrmk,
        L"DRMK",
        L"Dell Real-Mode Kernel OS",
        L"BOOTSECT.DRK"
    };
/* MS OS/2 1.x */
    static BOOTCODE_BYTES BytesMsOs2[] = {{NULL, 0}};
    static PCWSTR         FilesMsOs2[] = {L"OS2BOOT.COM", L"OS2BIO.COM", L"OS2DOS.COM", NULL};
    static const LOADER_DESC DescMsOs2 =
    {
        "MS OS/2 1.x",
        BytesMsOs2,
        FilesMsOs2,
        L"MSOS2",
        L"MS OS/2 1.x",
        L"BOOTSECT.OS2"
    };
/* MS or IBM OS/2 */
    static BOOTCODE_BYTES BytesIbmOs2[] = {{NULL, 0}};
    static PCWSTR         FilesIbmOs2[] = {L"OS2BOOT", L"OS2LDR", L"OS2KRNL", NULL};
    static const LOADER_DESC DescIbmOs2 =
    {
        "MS/IBM OS/2",
        BytesIbmOs2,
        FilesIbmOs2,
        L"IBMOS2",
        L"MS/IBM OS/2",
        L"BOOTSECT.OS2"
    };
/* FreeDOS boot loader */
    static BOOTCODE_BYTES BytesFrDos[] = {BYTES("KERNEL  SYS"), {NULL, 0}};
    static PCWSTR         FilesFrDos[] = {L"KERNEL.SYS", NULL};
    static const LOADER_DESC DescFrDos =
    {
        "FreeDOS boot loader",
        BytesFrDos,
        FilesFrDos,
        L"FDOS",
        L"FreeDOS",
        L"BOOTSECT.DOS"
    };
/* FreeDOS MetaKern boot manager, by Eric Auer */
    static BOOTCODE_BYTES BytesMetaK[] = {BYTES("METAKERNSYS"), {NULL, 0}};
    static PCWSTR         FilesMetaK[] = {L"METAKERN.SYS", /*L"KERNEL.SYS",*/ NULL};
    static const LOADER_DESC DescMetaK =
    {
        "FreeDOS MetaKern boot manager",
        BytesMetaK,
        FilesMetaK,
        L"METAKERN",
        L"FreeDOS MetaKern",
        L"BOOTSECT.FMK"
    };

    static const LOADER_DESC* DOSLoaders[] =
    {
        &DescCompaqDos, &DescMsDos, &DescWin9x, &DescPcDos, &DescDrDos,
        &DescEnhDos, &DescRxDos, &DescDrmk, &DescMsOs2, &DescIbmOs2,
        &DescFrDos, &DescMetaK
    };

    /* Search for the boot loaders */
    ULONG i;
    BOOLEAN Found = FALSE;

    for (i = 0; i < RTL_NUMBER_OF(DOSLoaders); ++i)
    {
        BOOLEAN FoundBytes = FALSE, FoundFiles = FALSE;

        /* Find any of the byte arrays */
        // TODO: Any? or all? How to differentiate in
        // the list above between the two possible cases?
        if (DOSLoaders[i]->BootCodeBytes)
        {
            PBOOTCODE_BYTES Bytes = DOSLoaders[i]->BootCodeBytes;
            for (; !FoundBytes && (Bytes->Buffer && Bytes->Length); ++Bytes)
            {
                FoundBytes = !!FindInBootCode(BootCode, Bytes->Buffer, Bytes->Length);
            }
        }

        /* Find any of the specified files */
        // TODO: Any? or all? How to differentiate in
        // the list above between the two possible cases?
        if (DOSLoaders[i]->BootFiles)
        {
            PCWSTR* File = DOSLoaders[i]->BootFiles;
            for (; !FoundFiles && *File; ++File)
            {
                FoundFiles = DoesFileExist_2(SystemRootPath->Buffer, *File);
            }
        }

        Found = FoundBytes && FoundFiles;
        if (Found)
            break;
    }

    if (Found)
    {
        DPRINT1("Found %s\n", DOSLoaders[i]->DbgDescription);
        *Section     = DOSLoaders[i]->Section;
        *Description = DOSLoaders[i]->Description;
        *BootSector  = DOSLoaders[i]->BootSectorFileName;
        // *BootSectorSize = SECTORSIZE;
    }
    // else, unrecognized boot loader.

    return Found;
}

static
BOOLEAN
RecognizeNTLoader(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PBOOTCODE BootCode,
    _Out_ BOOT_STORE_TYPE* Type/*,
    _Out_ PCWSTR* Section,
    _Out_ PCWSTR* Description,
    _Out_ PCWSTR* BootSector,
    _Out_ PULONG BootSectorSize*/)
{
    // *Section        = NULL;
    // *Description    = NULL;
    // *BootSector     = NULL;
    // // *BootSectorSize = 0;

    // FIXME: Check for Vista+ bootloader!

    // // For FAT12/16/32 sectors:
    //    !!FindInBootCode(BootCode, "NTLDR      ", 8+3);
    // // For NTFS sectors: ??

    /*** Status = FindBootStore(PartitionHandle, NtLdr, &Version); ***/
    /*** Status = FindBootStore(PartitionHandle, BootMgr, &Version); ***/
    if (DoesFileExist_2(SystemRootPath->Buffer, L"NTLDR") ||
        DoesFileExist_2(SystemRootPath->Buffer, L"BOOT.INI"))
    {
        /* Search root directory for 'NTLDR' and 'BOOT.INI' */
        DPRINT1("Found Microsoft Windows NT/2000/XP boot loader\n");

        // TODO: Return actual NT loader type: FreeLdr, BootMgr
        *Type = NtLdr;
        return TRUE;
    }
    return FALSE;
}

static
BOOLEAN
RecognizeLinuxLoader(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PBOOTCODE BootCode,
    _Out_ PCWSTR* Section,
    _Out_ PCWSTR* Description,
    _Out_ PCWSTR* BootSector,
    _Out_ PULONG BootSectorSize)
{
    /* Certainly SysLinux, GRUB, LILO... or an unknown boot loader */
    DPRINT1("*nix boot loader found\n");
    *Section     = L"Linux";
    *Description = L"Linux";
    *BootSector  = L"BOOTSECT.LIN";
    // *BootSectorSize = BTRFS_BOOTSECTOR_SIZE; // FIXME: Hardcoded size
    return TRUE;
}


/**
 * @brief
 * Installs a new VBR boot code to the first sector of a partition.
 * Only for BIOS-based PCs.
 *
 * @param[in]   SystemRootPath
 * System partition path.
 *
 * @param[in]   SourceRootPath
 * The installation source, where to copy the FreeLdr boot sector file from.
 *
 * @param[in]   FileSystemName
 * The file system for the corresponding VBR boot code to install.
 *
 * @param[in]   RecognizeAndBackupOldVbr
 * TRUE if the caller wants to recognize any existing known boot loader,
 * backup the old VBR in an "vbr.old" file at the root of the system partition
 * and create an additional boot entry for it. FALSE if not.
 **/
static
NTSTATUS
InstallVBRToPartition(
    _In_ PCUNICODE_STRING SystemRootPath,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCWSTR FileSystemName,
    _In_ BOOLEAN RecognizeAndBackupOldVbr)
{
    NTSTATUS Status;
    BOOTCODE_ID BtId;
    BOOLEAN ReplaceBootCode;
    UNICODE_STRING Name;
    BOOTCODE BootCode = {0};
    WCHAR SrcPath[MAX_PATH];
    WCHAR DstPath[MAX_PATH];

    /* Remove any trailing backslash if needed */
    UNICODE_STRING RootPartition = *SystemRootPath;
    TrimTrailingPathSeparators_UStr(&RootPartition);

    DPRINT("System path: '%wZ'\n", SystemRootPath);

    /* Map the known filesystem to a table lookup ID */
    ASSERT(FileSystemName);
    if (wcsicmp(FileSystemName, L"FAT") == 0)
        BtId = BT_FAT;
    else if (wcsicmp(FileSystemName, L"FAT32") == 0)
        BtId = BT_FAT32;
    else if (wcsicmp(FileSystemName, L"NTFS" ) == 0)
        BtId = BT_NTFS;
    else if (wcsicmp(FileSystemName, L"BTRFS") == 0)
        BtId = BT_BTRFS;
    /*
    else if (wcsicmp(FileSystemName, L"EXT2") == 0 ||
             wcsicmp(FileSystemName, L"EXT3") == 0 ||
             wcsicmp(FileSystemName, L"EXT4") == 0)
    */
    else
    {
        /* Unknown file system */
        DPRINT1("Unknown or unsupported file system '%S'\n", FileSystemName);
        return STATUS_NOT_SUPPORTED;
    }


    /* Allocate and read the new bootsector from SrcPath */
    CombinePaths(SrcPath, ARRAYSIZE(SrcPath), 2,
                 SourceRootPath->Buffer, BootCodes[BtId].BootCodeFilePath);
    RtlInitUnicodeString(&Name, SrcPath);
    Status = ReadBootCodeFromFile(&BootCode, &Name, 0);
    if (!NT_SUCCESS(Status))
        return Status;

    //
    // Newly-formatted partition --> install VBR and freeldr only.
    //
    // Otherwise:
    //
    // - If no valid boot sector --> install VBR and freeldr only.
    //
    // - If valid boot sector, try to recognize loader:
    //
    //   * NT boot loader: try adding ourselves to the boot.ini or BCD.
    //     NOTE: We don't currently support BCD, so we'll throw error.
    //
    //   * DOS, Linux boot loader or unknown: backup old VBR,
    //     install new VBR and freeldr, and add extra boot entry
    //     to boot old VBR.
    //

    /* Suppose non-NT bootloaders: install our own bootloader */
    ReplaceBootCode = TRUE;

    /*
     * If requested, check for the existence of known bootloaders, depending
     * on the target filesystem, and determine whether to replace the boot code
     * or use existing one instead (e.g. NTLDR).
     */
    if (RecognizeAndBackupOldVbr) do
    {
        PCWSTR Section;
        PCWSTR Description;
        PCWSTR BackupSector;
        ULONG  BackupSize = BootCodes[BtId].BackupSize; // Default boot sector backup size.
        BOOTCODE OrgBootCode = {0};
        BOOLEAN bRec = FALSE;

        /* Allocate and read only the first sector of the partition's
         * current original bootsector for performing recognition */
        Status = ReadBootCodeFromFile(&OrgBootCode, &RootPartition, SECTORSIZE);
        if (!NT_SUCCESS(Status))
        {
            /* If we failed, don't do any VBR recognition */
            break;
        }
        if (!IsBootSectorValid(&OrgBootCode))
            goto Skip;

        /* Compare the original boot code with the new one we install.
         * If they are the same, don't backup the original boot code. */
        {
        BOOTCODE_REGION ExcludeRegionFAT[] =
            {
                {FIELD_OFFSET(FAT_BOOTSECTOR, OemName),
                 FIELD_OFFSET(FAT_BOOTSECTOR, BootCodeAndData) -
                 FIELD_OFFSET(FAT_BOOTSECTOR, OemName)},
                {0, 0}
            };
        BOOTCODE_REGION ExcludeRegionFAT32[] =
            {
                {FIELD_OFFSET(FAT32_BOOTSECTOR, OemName),
                 FIELD_OFFSET(FAT32_BOOTSECTOR, BootCodeAndData) -
                 FIELD_OFFSET(FAT32_BOOTSECTOR, OemName)},
                {FIELD_OFFSET(FAT32_BOOTSECTOR, BackupBootSector),
                 RTL_FIELD_SIZE(FAT32_BOOTSECTOR, BackupBootSector)},
                {0, 0}
            };
        BOOTCODE_REGION ExcludeRegionBTRFS[] =
            {
                {FIELD_OFFSET(BTRFS_BOOTSECTOR, PartitionStartLBA),
                 RTL_FIELD_SIZE(BTRFS_BOOTSECTOR, PartitionStartLBA)},
                {0, 0}
            };
        BOOTCODE_REGION ExcludeRegionNTFS[] =
            {
                {FIELD_OFFSET(NTFS_BOOTSECTOR, OEMID),
                 FIELD_OFFSET(NTFS_BOOTSECTOR, BootStrap) -
                 FIELD_OFFSET(NTFS_BOOTSECTOR, OEMID)},
                {0, 0}
            };

        PBOOTCODE_REGION pExcludeRegion = NULL;
        switch (BtId)
        {
        case BT_FAT:
            pExcludeRegion = ExcludeRegionFAT;
            break;
        case BT_FAT32:
            pExcludeRegion = ExcludeRegionFAT32;
            break;
        case BT_NTFS:
            pExcludeRegion = ExcludeRegionNTFS;
            break;
        case BT_BTRFS:
            pExcludeRegion = ExcludeRegionBTRFS;
            break;
        default:
            ASSERT(FALSE);
        }

        if (CompareBootCodes(&OrgBootCode, &BootCode, pExcludeRegion))
            goto Skip; // They are identical, return.
        }

        switch (BtId)
        {
        case BT_FAT: case BT_FAT32:
        {
            /* Check for NT and other bootloaders */
            BOOT_STORE_TYPE Type;

            bRec = RecognizeNTLoader(SystemRootPath,
                                     &OrgBootCode,
                                     &Type/*,
                                     &Section,
                                     &Description,
                                     &BackupSector,
                                     &BackupSize*/);
            if (bRec)
            {
                /* NT bootloader: Add ourselves to existing bootloader */
                ReplaceBootCode = FALSE;
                goto Skip;
            }

            bRec = RecognizeDOSLoader(SystemRootPath,
                                      &OrgBootCode,
                                      &Section,
                                      &Description,
                                      &BackupSector,
                                      &BackupSize);
#if 0
            if (!bRec)
            {
                bRec = RecognizeLinuxLoader(SystemRootPath,
                                            &OrgBootCode,
                                            &Section,
                                            &Description,
                                            &BackupSector,
                                            &BackupSize);
            }
#endif
            break;
        }

        case BT_NTFS:
        {
            /* Check for NT and other bootloaders */
            BOOT_STORE_TYPE Type;

            bRec = RecognizeNTLoader(SystemRootPath,
                                     &OrgBootCode,
                                     &Type/*,
                                     &Section,
                                     &Description,
                                     &BackupSector,
                                     &BackupSize*/);
            if (bRec)
            {
                /* NT bootloader: Add ourselves to existing bootloader */
                ReplaceBootCode = FALSE;
                goto Skip;
            }

#if 0
            if (!bRec)
            {
                bRec = RecognizeLinuxLoader(SystemRootPath,
                                            &OrgBootCode,
                                            &Section,
                                            &Description,
                                            &BackupSector,
                                            &BackupSize);
            }
#endif
            break;
        }

        case BT_BTRFS:
        {
            /* Check for other bootloaders */

            bRec = RecognizeLinuxLoader(SystemRootPath,
                                        &OrgBootCode,
                                        &Section,
                                        &Description,
                                        &BackupSector,
                                        &BackupSize);
            break;
        }

        default:
            ASSERT(FALSE);
        }

        if (!bRec)
        {
            /* No or unknown boot loader */
            DPRINT1("No or unknown boot loader found\n");
            Section      = L"Unknown";
            Description  = L"Unknown Operating System";
            BackupSector = L"BOOTSECT.OLD";
            // BackupSize = SECTORSIZE;
        }

        /* The original VBR can be replaced by now */
        ASSERT(ReplaceBootCode);

        /* Backup the old VBR */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, BackupSector);

        DPRINT1("Save bootsector: %S ==> %S\n", SystemRootPath->Buffer, DstPath);
        Status = SaveBootSector(&OrgBootCode, DstPath/*, BackupSize*/);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("SaveBootSector() failed: Status %lx\n", Status);
            FreeBootCode(&OrgBootCode);
            goto Quit;
        }

        /* Add a boot entry to the old VBR */
        CreateBootEntryForBootSector = TRUE;
        BootSector_Entry.BootEntryKey = MAKESTRKEY(Section);
        BootSector_Entry.FriendlyName = Description;
        BootSector_BootPath = RootPartition.Buffer; // SystemRootPath->Buffer;
        BootSector_BootSector = BackupSector;
        // TODO: Just call a function that adds a single boot entry.
        // For the time being, this is handled by the
        // CreateFreeLoaderIniForReactOS() call done later.

Skip:
        /* Free the bootsector and return */
        FreeBootCode(&OrgBootCode);
    } while (0);


    /* Install the new bootloader, either on the disk or into a file */
    if (ReplaceBootCode)
    {
        /* Install the new bootsector on the disk */
        DPRINT1("Install %s bootcode: %S ==> %S\n",
                BootCodes[BtId].BootCodeName, SrcPath, SystemRootPath->Buffer);
        Status = InstallBootCodeToDisk(&BootCode, &RootPartition,
                                       BootCodes[BtId].InstallBootCode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootCodeToDisk(%s) failed: Status %lx\n",
                    BootCodes[BtId].BootCodeName, Status);
        }
    }
    else
    {
        // TODO: ASSERT that it's either Ntldr or BootMgr

        /* Add ourselves to existing bootloader */
        // FIXME: Add support for BCD (BootMgr)
        //
        // TODO: Just call a function that adds a single boot entry,
        // into the Ntldr or the BootMgr boot store.
        //
        /* Update 'boot.ini' */
        /* Windows' NTLDR loads an external bootsector file when the specified drive
           letter is C:, otherwise it will interpret it as a boot DOS path specifier */
        DPRINT1("Update 'boot.ini'\n");
        Status = UpdateBootIni(SystemRootPath->Buffer,
                               L"C:\\bootsect.ros",
                               L"ReactOS");
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("UpdateBootIni() failed: Status %lx\n", Status);
            goto Quit;
        }

        /* Install the new bootcode into a file */
        CombinePaths(DstPath, ARRAYSIZE(DstPath), 2, SystemRootPath->Buffer, L"bootsect.ros");RtlInitUnicodeString(&Name, DstPath);

        DPRINT1("Install %s bootcode: %S ==> %S\n",
                BootCodes[BtId].BootCodeName, SrcPath, DstPath);
        Status = InstallBootCodeToFile(&BootCode, &Name, // DstPath,
                                       &RootPartition,
                                       BootCodes[BtId].InstallBootCode);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallBootCodeToFile(%s) failed: Status %lx\n",
                    BootCodes[BtId].BootCodeName, Status);
        }
    }

Quit:
    /* Free the bootsector and return */
    FreeBootCode(&BootCode);
    return Status;
}


/* GENERIC FUNCTIONS *********************************************************/

/**
 * @brief
 * Installs FreeLoader on the system and configure the boot entries.
 *
 * @todo
 * Split this function into just the InstallBootManager, and a separate one
 * for just the boot entries.
 *
 * @param[in]   SourceRootPath
 * The installation source, where to copy the FreeLdr boot manager from.
 *
 * @param[in]   SystemRootPath , SystemPartition
 * The system partition path, where the FreeLdr boot manager and its
 * settings are saved to.
 *
 * @param[in]   DestinationArcPath
 * The ReactOS installation path in ARC format.
 *
 * @param[in]   Options
 * For BIOS-based PCs:
 * LOBYTE:
 *      0: Install only on VBR;
 *      1: Install on both VBR and MBR.
 *      2: Install on removable floppy (FAT12-formatted)
 * HIBYTE:
 *      TRUE: Recognize and backup the original VBR.
 *      FALSE: Unconditionally erase the original VBR.
 **/
NTSTATUS
InstallBootManagerAndBootEntries(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING SystemRootPath,
    /**/_In_opt_ PPARTENTRY SystemPartition,/**/ // FIXME: Redundant param.
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ ULONG_PTR Options)
{
    NTSTATUS Status;
    UCHAR InstallType = (Options & 0x0F);

__debugbreak();

    // TODO:
    // For BIOS-based PC:
    // 1. Install MBR (optional)
    // 2. Install VBR
    // For other platforms: do something else.

    // FIXME: We currently only support BIOS-based PCs
    if (InstallType <= 1)
    {
        /* Step 1: Write the VBR */
        BOOLEAN RecognizeAndBackupOldVbr = !!(Options & 0xF0);
        Status = InstallVBRToPartition(SystemRootPath,
                                       SourceRootPath,
                                       SystemPartition->FileSystem,
                                       RecognizeAndBackupOldVbr);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallVBRToPartition() failed: Status 0x%lx\n", Status);
            // STATUS_BAD_MASTER_BOOT_RECORD
            return ERROR_WRITE_BOOT;
            // return Status;
        }

        /* Step 2: Write the MBR if the disk containing
         * the system partition is not a super-floppy */
        if ((InstallType == 1) && !IsSuperFloppy(SystemPartition->DiskEntry))
        {
            WCHAR SystemDiskPath[MAX_PATH];
            RtlStringCchPrintfW(SystemDiskPath, ARRAYSIZE(SystemDiskPath),
                                L"\\Device\\Harddisk%d\\Partition0",
                                SystemPartition->DiskEntry->DiskNumber);
            Status = InstallMBRToDisk(SystemRootPath,
                                      SourceRootPath,
                                      SystemDiskPath,
                                      !SystemPartition->DiskEntry->NewDisk);
            if (!NT_SUCCESS(Status))
            {
                DPRINT1("InstallMBRToDisk() failed: Status 0x%lx\n", Status);
                return ERROR_INSTALL_BOOTCODE;
                // return Status;
            }
        }
    }
    else if (InstallType == 2)
    {
        /* Install the bootsector */
        Status = InstallVBRToPartition(SystemRootPath,
                                       SourceRootPath,
                                       (PCWSTR)SystemPartition, // FIXME: Temp HACK!!
                                       FALSE);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("InstallVBRToPartition() failed: Status 0x%lx\n", Status);
            return Status;
        }
    }
    else
    {
        // TODO: Other platforms
        return STATUS_NOT_SUPPORTED;
    }

    /* Copy FreeLoader to the system partition, always overwriting the older version */
    Status = CopyBootManager(SourceRootPath->Buffer, SystemRootPath->Buffer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CopyBootManager() failed: Status %lx\n", Status);
        return Status;
    }

    /* Save the boot entries */
    Status = SaveBootEntries(pSetupData, SystemRootPath->Buffer, DestinationArcPath->Buffer);
    if (!NT_SUCCESS(Status))
        DPRINT1("SaveBootEntries() failed: Status %lx\n", Status);
    return Status;
}

NTSTATUS
InstallBootcodeToRemovable(
    _In_ PUSETUP_DATA pSetupData,
    _In_ PCUNICODE_STRING RemovableRootPath, // == SystemRootPath
    _In_ PCUNICODE_STRING SourceRootPath,
    _In_ PCUNICODE_STRING DestinationArcPath,
    _In_ PCWSTR FileSystemName)
{
    static const UNICODE_STRING DeviceFloppy = RTL_CONSTANT_STRING(L"\\Device\\Floppy");
    NTSTATUS Status;
    BOOLEAN IsFloppy;

    /* Verify that the removable disk is accessible */
    if (DoesDirExist(NULL, RemovableRootPath->Buffer) == FALSE)
        return STATUS_DEVICE_NOT_READY;

    /* Check whether this is floppy or something else */
    // FIXME: This is all hardcoded! TODO: Determine dynamically
    IsFloppy = RtlPrefixUnicodeString(&DeviceFloppy, RemovableRootPath, TRUE);
    if (IsFloppy) FileSystemName = L"FAT";

    /* Format the removable disk */
    // FormatPartition(...)
    Status = FormatFileSystem(RemovableRootPath->Buffer,
                              FileSystemName,
                              (IsFloppy ? FMIFS_FLOPPY : FMIFS_REMOVABLE),
                              NULL,
                              TRUE,
                              0,
                              NULL);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_NOT_SUPPORTED)
            DPRINT1("%s FS non existent on this system?!\n", FileSystemName);
        else
            DPRINT1("VfatFormat() failed: Status %lx\n", Status);
        return Status;
    }

    /* Copy FreeLoader to the removable disk and save the boot entries */
    Status = InstallBootManagerAndBootEntries(pSetupData,
                                              SourceRootPath,
                                              RemovableRootPath,
                                              (PPARTENTRY)FileSystemName, // FIXME: Temp HACK!!
                                              DestinationArcPath,
                                              2 /* Install on removable media */);
    if (!NT_SUCCESS(Status))
        DPRINT1("InstallBootManagerAndBootEntries() failed: Status %lx\n", Status);
    return Status;
}

/* EOF */
