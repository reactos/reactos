/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Setup Library
 * FILE:            base/setup/lib/setuplib.c
 * PURPOSE:         Setup Library - Main initialization helpers
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *****************************************************************/

#include "precomp.h"
#include "filesup.h"
#include "infsupp.h"
#include "inicache.h"

#include "setuplib.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

VOID
CheckUnattendedSetup(
    IN OUT PUSETUP_DATA pSetupData)
{
    INFCONTEXT Context;
    HINF UnattendInf;
    UINT ErrorLine;
    INT IntValue;
    PCWSTR Value;
    WCHAR UnattendInfPath[MAX_PATH];

    CombinePaths(UnattendInfPath, ARRAYSIZE(UnattendInfPath), 2,
                 pSetupData->SourcePath.Buffer, L"unattend.inf");

    DPRINT("UnattendInf path: '%S'\n", UnattendInfPath);

    if (DoesFileExist(NULL, UnattendInfPath) == FALSE)
    {
        DPRINT("Does not exist: %S\n", UnattendInfPath);
        return;
    }

    /* Load 'unattend.inf' from installation media */
    UnattendInf = SpInfOpenInfFile(UnattendInfPath,
                                   NULL,
                                   INF_STYLE_OLDNT,
                                   pSetupData->LanguageId,
                                   &ErrorLine);
    if (UnattendInf == INVALID_HANDLE_VALUE)
    {
        DPRINT("SpInfOpenInfFile() failed\n");
        return;
    }

    /* Open 'Unattend' section */
    if (!SpInfFindFirstLine(UnattendInf, L"Unattend", L"Signature", &Context))
    {
        DPRINT("SpInfFindFirstLine() failed for section 'Unattend'\n");
        goto Quit;
    }

    /* Get pointer 'Signature' key */
    if (!INF_GetData(&Context, NULL, &Value))
    {
        DPRINT("INF_GetData() failed for key 'Signature'\n");
        goto Quit;
    }

    /* Check 'Signature' string */
    if (_wcsicmp(Value, L"$ReactOS$") != 0)
    {
        DPRINT("Signature not $ReactOS$\n");
        INF_FreeData(Value);
        goto Quit;
    }

    INF_FreeData(Value);

    /* Check if Unattend setup is enabled */
    if (!SpInfFindFirstLine(UnattendInf, L"Unattend", L"UnattendSetupEnabled", &Context))
    {
        DPRINT("Can't find key 'UnattendSetupEnabled'\n");
        goto Quit;
    }

    if (!INF_GetData(&Context, NULL, &Value))
    {
        DPRINT("Can't read key 'UnattendSetupEnabled'\n");
        goto Quit;
    }

    if (_wcsicmp(Value, L"yes") != 0)
    {
        DPRINT("Unattend setup is disabled by 'UnattendSetupEnabled' key!\n");
        INF_FreeData(Value);
        goto Quit;
    }

    INF_FreeData(Value);

    /* Search for 'DestinationDiskNumber' in the 'Unattend' section */
    if (!SpInfFindFirstLine(UnattendInf, L"Unattend", L"DestinationDiskNumber", &Context))
    {
        DPRINT("SpInfFindFirstLine() failed for key 'DestinationDiskNumber'\n");
        goto Quit;
    }

    if (!SpInfGetIntField(&Context, 1, &IntValue))
    {
        DPRINT("SpInfGetIntField() failed for key 'DestinationDiskNumber'\n");
        goto Quit;
    }

    pSetupData->DestinationDiskNumber = (LONG)IntValue;

    /* Search for 'DestinationPartitionNumber' in the 'Unattend' section */
    if (!SpInfFindFirstLine(UnattendInf, L"Unattend", L"DestinationPartitionNumber", &Context))
    {
        DPRINT("SpInfFindFirstLine() failed for key 'DestinationPartitionNumber'\n");
        goto Quit;
    }

    if (!SpInfGetIntField(&Context, 1, &IntValue))
    {
        DPRINT("SpInfGetIntField() failed for key 'DestinationPartitionNumber'\n");
        goto Quit;
    }

    pSetupData->DestinationPartitionNumber = (LONG)IntValue;

    /* Search for 'InstallationDirectory' in the 'Unattend' section (optional) */
    if (SpInfFindFirstLine(UnattendInf, L"Unattend", L"InstallationDirectory", &Context))
    {
        /* Get pointer 'InstallationDirectory' key */
        if (!INF_GetData(&Context, NULL, &Value))
        {
            DPRINT("INF_GetData() failed for key 'InstallationDirectory'\n");
            goto Quit;
        }

        RtlStringCchCopyW(pSetupData->InstallationDirectory,
                          ARRAYSIZE(pSetupData->InstallationDirectory),
                          Value);

        INF_FreeData(Value);
    }

    IsUnattendedSetup = TRUE;
    DPRINT("Running unattended setup\n");

    /* Search for 'MBRInstallType' in the 'Unattend' section */
    pSetupData->MBRInstallType = -1;
    if (SpInfFindFirstLine(UnattendInf, L"Unattend", L"MBRInstallType", &Context))
    {
        if (SpInfGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->MBRInstallType = IntValue;
        }
    }

    /* Search for 'FormatPartition' in the 'Unattend' section */
    pSetupData->FormatPartition = 0;
    if (SpInfFindFirstLine(UnattendInf, L"Unattend", L"FormatPartition", &Context))
    {
        if (SpInfGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->FormatPartition = IntValue;
        }
    }

    pSetupData->AutoPartition = 0;
    if (SpInfFindFirstLine(UnattendInf, L"Unattend", L"AutoPartition", &Context))
    {
        if (SpInfGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->AutoPartition = IntValue;
        }
    }

    /* Search for LocaleID in the 'Unattend' section */
    if (SpInfFindFirstLine(UnattendInf, L"Unattend", L"LocaleID", &Context))
    {
        if (INF_GetData(&Context, NULL, &Value))
        {
            LONG Id = wcstol(Value, NULL, 16);
            RtlStringCchPrintfW(pSetupData->LocaleID,
                                ARRAYSIZE(pSetupData->LocaleID),
                                L"%08lx", Id);
            INF_FreeData(Value);
       }
    }

    /* Search for FsType in the 'Unattend' section */
    pSetupData->FsType = 0;
    if (SpInfFindFirstLine(UnattendInf, L"Unattend", L"FsType", &Context))
    {
        if (SpInfGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->FsType = IntValue;
        }
    }

Quit:
    SpInfCloseInfFile(UnattendInf);
}

VOID
InstallSetupInfFile(
    IN OUT PUSETUP_DATA pSetupData)
{
    NTSTATUS Status;
    PINICACHE IniCache;

#if 0 // HACK FIXME!
    PINICACHE UnattendCache;
    PINICACHEITERATOR Iterator;
#else
    // WCHAR CrLf[] = {L'\r', L'\n'};
    CHAR CrLf[] = {'\r', '\n'};
    HANDLE FileHandle, UnattendFileHandle, SectionHandle;
    FILE_STANDARD_INFORMATION FileInfo;
    ULONG FileSize;
    PVOID ViewBase;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
#endif

    PINI_SECTION IniSection;
    WCHAR PathBuffer[MAX_PATH];
    WCHAR UnattendInfPath[MAX_PATH];

    /* Create a $winnt$.inf file with default entries */
    IniCache = IniCacheCreate();
    if (!IniCache)
        return;

    IniSection = IniAddSection(IniCache, L"SetupParams");
    if (IniSection)
    {
        /* Key "skipmissingfiles" */
        // RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                            // L"\"%s\"", L"WinNt5.2");
        // IniAddKey(IniSection, L"Version", PathBuffer);
    }

    IniSection = IniAddSection(IniCache, L"Data");
    if (IniSection)
    {
        RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                            L"\"%s\"", IsUnattendedSetup ? L"yes" : L"no");
        IniAddKey(IniSection, L"UnattendedInstall", PathBuffer);

        // "floppylessbootpath" (yes/no)

        RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                            L"\"%s\"", L"winnt");
        IniAddKey(IniSection, L"ProductType", PathBuffer);

        RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                            L"\"%s\\\"", pSetupData->SourceRootPath.Buffer);
        IniAddKey(IniSection, L"SourcePath", PathBuffer);

        // "floppyless" ("0")
    }

#if 0

    /* TODO: Append the standard unattend.inf file */
    CombinePaths(UnattendInfPath, ARRAYSIZE(UnattendInfPath), 2,
                 pSetupData->SourcePath.Buffer, L"unattend.inf");
    if (DoesFileExist(NULL, UnattendInfPath) == FALSE)
    {
        DPRINT("Does not exist: %S\n", UnattendInfPath);
        goto Quit;
    }

    Status = IniCacheLoad(&UnattendCache, UnattendInfPath, FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot load %S as an INI file!\n", UnattendInfPath);
        goto Quit;
    }

    IniCacheDestroy(UnattendCache);

Quit:
    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                 pSetupData->DestinationPath.Buffer, L"System32\\$winnt$.inf");
    IniCacheSave(IniCache, PathBuffer);
    IniCacheDestroy(IniCache);

#else

    CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                 pSetupData->DestinationPath.Buffer, L"System32\\$winnt$.inf");
    IniCacheSave(IniCache, PathBuffer);
    IniCacheDestroy(IniCache);

    /* TODO: Append the standard unattend.inf file */
    CombinePaths(UnattendInfPath, ARRAYSIZE(UnattendInfPath), 2,
                 pSetupData->SourcePath.Buffer, L"unattend.inf");
    if (DoesFileExist(NULL, UnattendInfPath) == FALSE)
    {
        DPRINT("Does not exist: %S\n", UnattendInfPath);
        return;
    }

    RtlInitUnicodeString(&FileName, PathBuffer);
    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
                               NULL,
                               NULL);
    Status = NtOpenFile(&FileHandle,
                        FILE_APPEND_DATA | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT | FILE_NON_DIRECTORY_FILE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot load %S as an INI file!\n", PathBuffer);
        return;
    }

    /* Query the file size */
    Status = NtQueryInformationFile(FileHandle,
                                    &IoStatusBlock,
                                    &FileInfo,
                                    sizeof(FileInfo),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryInformationFile() failed (Status %lx)\n", Status);
        FileInfo.EndOfFile.QuadPart = 0ULL;
    }

    Status = OpenAndMapFile(NULL,
                            UnattendInfPath,
                            &UnattendFileHandle,
                            &FileSize,
                            &SectionHandle,
                            &ViewBase,
                            FALSE);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Cannot load %S !\n", UnattendInfPath);
        NtClose(FileHandle);
        return;
    }

    /* Write to the INI file */

    /* "\r\n" */
    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         (PVOID)CrLf,
                         sizeof(CrLf),
                         &FileInfo.EndOfFile,
                         NULL);

    Status = NtWriteFile(FileHandle,
                         NULL,
                         NULL,
                         NULL,
                         &IoStatusBlock,
                         ViewBase,
                         FileSize,
                         NULL,
                         NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("NtWriteFile() failed (Status %lx)\n", Status);
    }

    /* Finally, unmap and close the file */
    UnMapAndCloseFile(UnattendFileHandle, SectionHandle, ViewBase);

    NtClose(FileHandle);
#endif
}

NTSTATUS
GetSourcePaths(
    OUT PUNICODE_STRING SourcePath,
    OUT PUNICODE_STRING SourceRootPath,
    OUT PUNICODE_STRING SourceRootDir)
{
    NTSTATUS Status;
    HANDLE LinkHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UCHAR ImageFileBuffer[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
    PUNICODE_STRING InstallSourcePath = (PUNICODE_STRING)&ImageFileBuffer;
    WCHAR SystemRootBuffer[MAX_PATH] = L"";
    UNICODE_STRING SystemRootPath = RTL_CONSTANT_STRING(L"\\SystemRoot");
    ULONG BufferSize;
    PWCHAR Ptr;

    // FIXME: commented out to allow installation from USB
#if 0
    /* Determine the installation source path via the full path of the installer */
    RtlInitEmptyUnicodeString(InstallSourcePath,
                              (PWSTR)((ULONG_PTR)ImageFileBuffer + sizeof(UNICODE_STRING)),
                              sizeof(ImageFileBuffer) - sizeof(UNICODE_STRING)
            /* Reserve space for a NULL terminator */ - sizeof(UNICODE_NULL));
    BufferSize = sizeof(ImageFileBuffer);
    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessImageFileName,
                                       InstallSourcePath,
                                       BufferSize,
                                       NULL);
    // STATUS_INFO_LENGTH_MISMATCH or STATUS_BUFFER_TOO_SMALL ?
    if (!NT_SUCCESS(Status))
        return Status;

    /* Manually NULL-terminate */
    InstallSourcePath->Buffer[InstallSourcePath->Length / sizeof(WCHAR)] = UNICODE_NULL;

    /* Strip the trailing file name */
    Ptr = wcsrchr(InstallSourcePath->Buffer, OBJ_NAME_PATH_SEPARATOR);
    if (Ptr)
        *Ptr = UNICODE_NULL;
    InstallSourcePath->Length = wcslen(InstallSourcePath->Buffer) * sizeof(WCHAR);
#endif

    /*
     * Now resolve the full path to \SystemRoot. In case it prefixes
     * the installation source path determined from the full path of
     * the installer, we use instead the resolved \SystemRoot as the
     * installation source path.
     * Otherwise, we use instead the path from the full installer path.
     */

    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRootPath,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenSymbolicLinkObject(&LinkHandle,
                                      SYMBOLIC_LINK_QUERY,
                                      &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /*
         * We failed at opening the \SystemRoot link (usually due to wrong
         * access rights). Do not consider this as a fatal error, but use
         * instead the image file path as the installation source path.
         */
        DPRINT1("NtOpenSymbolicLinkObject(%wZ) failed with Status 0x%08lx\n",
                &SystemRootPath, Status);
        goto InitPaths;
    }

    RtlInitEmptyUnicodeString(&SystemRootPath,
                              SystemRootBuffer,
                              sizeof(SystemRootBuffer));

    /* Resolve the link and close its handle */
    Status = NtQuerySymbolicLinkObject(LinkHandle,
                                       &SystemRootPath,
                                       &BufferSize);
    NtClose(LinkHandle);

    if (!NT_SUCCESS(Status))
        return Status; // Unexpected error

    /* Check whether the resolved \SystemRoot is a prefix of the image file path */
    // FIXME: commented out to allow installation from USB
    // if (RtlPrefixUnicodeString(&SystemRootPath, InstallSourcePath, TRUE))
    {
        /* Yes it is, so we use instead SystemRoot as the installation source path */
        InstallSourcePath = &SystemRootPath;
    }


InitPaths:
    /*
     * Retrieve the different source path components
     */
    RtlCreateUnicodeString(SourcePath, InstallSourcePath->Buffer);

    /* Strip trailing directory */
    Ptr = wcsrchr(InstallSourcePath->Buffer, OBJ_NAME_PATH_SEPARATOR);
    if (Ptr)
    {
        RtlCreateUnicodeString(SourceRootDir, Ptr);
        *Ptr = UNICODE_NULL;
    }
    else
    {
        RtlCreateUnicodeString(SourceRootDir, L"");
    }

    RtlCreateUnicodeString(SourceRootPath, InstallSourcePath->Buffer);

    return STATUS_SUCCESS;
}

ERROR_NUMBER
LoadSetupInf(
    IN OUT PUSETUP_DATA pSetupData)
{
    INFCONTEXT Context;
    UINT ErrorLine;
    INT IntValue;
    PCWSTR Value;
    WCHAR FileNameBuffer[MAX_PATH];

    CombinePaths(FileNameBuffer, ARRAYSIZE(FileNameBuffer), 2,
                 pSetupData->SourcePath.Buffer, L"txtsetup.sif");

    DPRINT("SetupInf path: '%S'\n", FileNameBuffer);

    pSetupData->SetupInf =
        SpInfOpenInfFile(FileNameBuffer,
                         NULL,
                         INF_STYLE_WIN4,
                         pSetupData->LanguageId,
                         &ErrorLine);
    if (pSetupData->SetupInf == INVALID_HANDLE_VALUE)
        return ERROR_LOAD_TXTSETUPSIF;

    /* Open 'Version' section */
    if (!SpInfFindFirstLine(pSetupData->SetupInf, L"Version", L"Signature", &Context))
        return ERROR_CORRUPT_TXTSETUPSIF;

    /* Get pointer 'Signature' key */
    if (!INF_GetData(&Context, NULL, &Value))
        return ERROR_CORRUPT_TXTSETUPSIF;

    /* Check 'Signature' string */
    if (_wcsicmp(Value, L"$ReactOS$") != 0 &&
        _wcsicmp(Value, L"$Windows NT$") != 0)
    {
        INF_FreeData(Value);
        return ERROR_SIGNATURE_TXTSETUPSIF;
    }

    INF_FreeData(Value);

    /* Open 'DiskSpaceRequirements' section */
    if (!SpInfFindFirstLine(pSetupData->SetupInf, L"DiskSpaceRequirements", L"FreeSysPartDiskSpace", &Context))
        return ERROR_CORRUPT_TXTSETUPSIF;

    pSetupData->RequiredPartitionDiskSpace = ~0;

    /* Get the 'FreeSysPartDiskSpace' value */
    if (!SpInfGetIntField(&Context, 1, &IntValue))
        return ERROR_CORRUPT_TXTSETUPSIF;

    pSetupData->RequiredPartitionDiskSpace = (ULONG)IntValue;

    //
    // Support "SetupSourceDevice" and "SetupSourcePath" in txtsetup.sif
    // See CORE-9023
    // Support for that should also be added in setupldr.
    //

    /* Update the Setup Source paths */
    if (SpInfFindFirstLine(pSetupData->SetupInf, L"SetupData", L"SetupSourceDevice", &Context))
    {
        /*
         * Get optional pointer 'SetupSourceDevice' key, its presence
         * will dictate whether we also need 'SetupSourcePath'.
         */
        if (INF_GetData(&Context, NULL, &Value))
        {
            /* Free the old source root path string and create the new one */
            RtlFreeUnicodeString(&pSetupData->SourceRootPath);
            RtlCreateUnicodeString(&pSetupData->SourceRootPath, Value);
            INF_FreeData(Value);

            if (!SpInfFindFirstLine(pSetupData->SetupInf, L"SetupData", L"SetupSourcePath", &Context))
            {
                /* The 'SetupSourcePath' value is mandatory! */
                return ERROR_CORRUPT_TXTSETUPSIF;
            }

            /* Get pointer 'SetupSourcePath' key */
            if (!INF_GetData(&Context, NULL, &Value))
            {
                /* The 'SetupSourcePath' value is mandatory! */
                return ERROR_CORRUPT_TXTSETUPSIF;
            }

            /* Free the old source path string and create the new one */
            RtlFreeUnicodeString(&pSetupData->SourceRootDir);
            RtlCreateUnicodeString(&pSetupData->SourceRootDir, Value);
            INF_FreeData(Value);
        }
    }

    /* Search for 'DefaultPath' in the 'SetupData' section */
    pSetupData->InstallationDirectory[0] = 0;
    if (SpInfFindFirstLine(pSetupData->SetupInf, L"SetupData", L"DefaultPath", &Context))
    {
        /* Get pointer 'DefaultPath' key */
        if (!INF_GetData(&Context, NULL, &Value))
            return ERROR_CORRUPT_TXTSETUPSIF;

        RtlStringCchCopyW(pSetupData->InstallationDirectory,
                          ARRAYSIZE(pSetupData->InstallationDirectory),
                          Value);

        INF_FreeData(Value);
    }

    return ERROR_SUCCESS;
}

/**
 * @brief   Find or set the active system partition.
 **/
BOOLEAN
InitSystemPartition(
    /**/_In_ PPARTLIST PartitionList,       /* HACK HACK! */
    /**/_In_ PPARTENTRY InstallPartition,   /* HACK HACK! */
    /**/_Out_ PPARTENTRY* pSystemPartition, /* HACK HACK! */
    _In_opt_ PFSVOL_CALLBACK FsVolCallback,
    _In_opt_ PVOID Context)
{
    FSVOL_OP Result;
    PPARTENTRY SystemPartition;
    PPARTENTRY OldActivePart;

    /*
     * If we install on a fixed disk, try to find a supported system
     * partition on the system. Otherwise if we install on a removable disk
     * use the install partition as the system partition.
     */
    if (InstallPartition->DiskEntry->MediaType == FixedMedia)
    {
        SystemPartition = FindSupportedSystemPartition(PartitionList,
                                                       FALSE,
                                                       InstallPartition->DiskEntry,
                                                       InstallPartition);
        /* Use the original system partition as the old active partition hint */
        OldActivePart = PartitionList->SystemPartition;

        if ( SystemPartition && PartitionList->SystemPartition &&
            (SystemPartition != PartitionList->SystemPartition) )
        {
            DPRINT1("We are using a different system partition!!\n");

            Result = FsVolCallback(Context,
                                   ChangeSystemPartition,
                                   (ULONG_PTR)SystemPartition,
                                   0);
            if (Result != FSVOL_DOIT)
                return FALSE;
        }
    }
    else // if (InstallPartition->DiskEntry->MediaType == RemovableMedia)
    {
        SystemPartition = InstallPartition;
        /* Don't specify any old active partition hint */
        OldActivePart = NULL;
    }

    if (!SystemPartition)
    {
        FsVolCallback(Context,
                      FSVOLNOTIFY_PARTITIONERROR,
                      ERROR_SYSTEM_PARTITION_NOT_FOUND,
                      0);
        return FALSE;
    }

    *pSystemPartition = SystemPartition;

    /*
     * If the system partition can be created in some
     * non-partitioned space, create it now.
     */
    if (!SystemPartition->IsPartitioned)
    {
        /* Automatically create the partition; it will be
         * formatted later with default parameters */
        // FIXME: Don't use the whole empty space, but a minimal size
        // specified from the TXTSETUP.SIF or unattended setup.
        CreatePartition(PartitionList,
                        SystemPartition,
                        0ULL,
                        0);
        ASSERT(SystemPartition->IsPartitioned);
    }

    /* Set it as such */
    if (!SetActivePartition(PartitionList, SystemPartition, OldActivePart))
    {
        DPRINT1("SetActivePartition(0x%p) failed?!\n", SystemPartition);
        ASSERT(FALSE);
    }

    /*
     * In all cases, whether or not we are going to perform a formatting,
     * we must perform a filesystem check of the system partition.
     */
    SystemPartition->Volume.NeedsCheck = TRUE;

    return TRUE;
}

NTSTATUS
InitDestinationPaths(
    IN OUT PUSETUP_DATA pSetupData,
    IN PCWSTR InstallationDir,
    IN PPARTENTRY PartEntry)    // FIXME: HACK!
{
    NTSTATUS Status;
    PDISKENTRY DiskEntry = PartEntry->DiskEntry;
    WCHAR PathBuffer[MAX_PATH];

    ASSERT(PartEntry->IsPartitioned && PartEntry->PartitionNumber != 0);

    /* Create 'pSetupData->DestinationRootPath' string */
    RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
    Status = RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                     L"\\Device\\Harddisk%lu\\Partition%lu\\",
                     DiskEntry->DiskNumber,
                     PartEntry->PartitionNumber);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlStringCchPrintfW() failed with status 0x%08lx\n", Status);
        return Status;
    }

    Status = RtlCreateUnicodeString(&pSetupData->DestinationRootPath, PathBuffer) ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlCreateUnicodeString() failed with status 0x%08lx\n", Status);
        return Status;
    }

    DPRINT("DestinationRootPath: %wZ\n", &pSetupData->DestinationRootPath);

    // FIXME! Which variable to choose?
    if (!InstallationDir)
        InstallationDir = pSetupData->InstallationDirectory;

/** Equivalent of 'NTOS_INSTALLATION::SystemArcPath' **/
    /* Create 'pSetupData->DestinationArcPath' */
    RtlFreeUnicodeString(&pSetupData->DestinationArcPath);

    if (DiskEntry->MediaType == FixedMedia)
    {
        if (DiskEntry->BiosFound)
        {
#if 1
            Status = RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                             L"multi(0)disk(0)rdisk(%lu)partition(%lu)\\",
                             DiskEntry->HwFixedDiskNumber,
                             PartEntry->OnDiskPartitionNumber);
#else
            Status = RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                             L"multi(%lu)disk(%lu)rdisk(%lu)partition(%lu)\\",
                             DiskEntry->HwAdapterNumber,
                             DiskEntry->HwControllerNumber,
                             DiskEntry->HwFixedDiskNumber,
                             PartEntry->OnDiskPartitionNumber);
#endif
            DPRINT1("Fixed disk found by BIOS, using MULTI ARC path '%S'\n", PathBuffer);
        }
        else
        {
            Status = RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                             L"scsi(%u)disk(%u)rdisk(%u)partition(%lu)\\",
                             DiskEntry->Port,
                             DiskEntry->Bus,
                             DiskEntry->Id,
                             PartEntry->OnDiskPartitionNumber);
            DPRINT1("Fixed disk not found by BIOS, using SCSI ARC path '%S'\n", PathBuffer);
        }
    }
    else // if (DiskEntry->MediaType == RemovableMedia)
    {
#if 1
        Status = RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                         L"multi(0)disk(0)rdisk(%lu)partition(%lu)\\",
                         0, 1);
        DPRINT1("Removable disk, using MULTI ARC path '%S'\n", PathBuffer);
#else
        Status = RtlStringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                         L"signature(%08x)disk(%u)rdisk(%u)partition(%lu)\\",
                         DiskEntry->LayoutBuffer->Signature,
                         DiskEntry->Bus,
                         DiskEntry->Id,
                         PartEntry->OnDiskPartitionNumber);
        DPRINT1("Removable disk, using SIGNATURE ARC path '%S'\n", PathBuffer);
#endif
    }

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlStringCchPrintfW() failed with status 0x%08lx\n", Status);
        RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
        return Status;
    }

    Status = ConcatPaths(PathBuffer, ARRAYSIZE(PathBuffer), 1, InstallationDir);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ConcatPaths() failed with status 0x%08lx\n", Status);
        RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
        return Status;
    }

    Status = RtlCreateUnicodeString(&pSetupData->DestinationArcPath, PathBuffer) ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlCreateUnicodeString() failed with status 0x%08lx\n", Status);
        RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
        return Status;
    }

/** Equivalent of 'NTOS_INSTALLATION::SystemNtPath' **/
    /* Create 'pSetupData->DestinationPath' string */
    RtlFreeUnicodeString(&pSetupData->DestinationPath);
    Status = CombinePaths(PathBuffer, ARRAYSIZE(PathBuffer), 2,
                          pSetupData->DestinationRootPath.Buffer, InstallationDir);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("CombinePaths() failed with status 0x%08lx\n", Status);
        RtlFreeUnicodeString(&pSetupData->DestinationArcPath);
        RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
        return Status;
    }

    Status = RtlCreateUnicodeString(&pSetupData->DestinationPath, PathBuffer) ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlCreateUnicodeString() failed with status 0x%08lx\n", Status);
        RtlFreeUnicodeString(&pSetupData->DestinationArcPath);
        RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
        return Status;
    }

/** Equivalent of 'NTOS_INSTALLATION::PathComponent' **/
    // FIXME: This is only temporary!! Must be removed later!
    Status = RtlCreateUnicodeString(&pSetupData->InstallPath, InstallationDir) ? STATUS_SUCCESS : STATUS_NO_MEMORY;

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RtlCreateUnicodeString() failed with status 0x%08lx\n", Status);
        RtlFreeUnicodeString(&pSetupData->DestinationPath);
        RtlFreeUnicodeString(&pSetupData->DestinationArcPath);
        RtlFreeUnicodeString(&pSetupData->DestinationRootPath);
        return Status;
    }

    return STATUS_SUCCESS;
}

// NTSTATUS
ERROR_NUMBER
InitializeSetup(
    IN OUT PUSETUP_DATA pSetupData,
    IN ULONG InitPhase)
{
    if (InitPhase == 0)
    {
        RtlZeroMemory(pSetupData, sizeof(*pSetupData));

        /* Initialize error handling */
        pSetupData->LastErrorNumber = ERROR_SUCCESS;
        pSetupData->ErrorRoutine = NULL;

        /* Initialize global unicode strings */
        RtlInitUnicodeString(&pSetupData->SourcePath, NULL);
        RtlInitUnicodeString(&pSetupData->SourceRootPath, NULL);
        RtlInitUnicodeString(&pSetupData->SourceRootDir, NULL);
        RtlInitUnicodeString(&pSetupData->DestinationArcPath, NULL);
        RtlInitUnicodeString(&pSetupData->DestinationPath, NULL);
        RtlInitUnicodeString(&pSetupData->DestinationRootPath, NULL);
        RtlInitUnicodeString(&pSetupData->SystemRootPath, NULL);

        // FIXME: This is only temporary!! Must be removed later!
        /***/RtlInitUnicodeString(&pSetupData->InstallPath, NULL);/***/

        //
        // TODO: Load and start SetupDD, and ask it for the information
        //

        return ERROR_SUCCESS;
    }
    else
    if (InitPhase == 1)
    {
        ERROR_NUMBER Error;
        NTSTATUS Status;

        /* Get the source path and source root path */
        //
        // NOTE: Sometimes the source path may not be in SystemRoot !!
        // (and this is the case when using the 1st-stage GUI setup!)
        //
        Status = GetSourcePaths(&pSetupData->SourcePath,
                                &pSetupData->SourceRootPath,
                                &pSetupData->SourceRootDir);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("GetSourcePaths() failed (Status 0x%08lx)\n", Status);
            return ERROR_NO_SOURCE_DRIVE;
        }
        /*
         * Example of output:
         *   SourcePath: '\Device\CdRom0\I386'
         *   SourceRootPath: '\Device\CdRom0'
         *   SourceRootDir: '\I386'
         */
        DPRINT1("SourcePath (1): '%wZ'\n", &pSetupData->SourcePath);
        DPRINT1("SourceRootPath (1): '%wZ'\n", &pSetupData->SourceRootPath);
        DPRINT1("SourceRootDir (1): '%wZ'\n", &pSetupData->SourceRootDir);

        /* Load 'txtsetup.sif' from the installation media */
        Error = LoadSetupInf(pSetupData);
        if (Error != ERROR_SUCCESS)
        {
            DPRINT1("LoadSetupInf() failed (Error 0x%lx)\n", Error);
            return Error;
        }
        DPRINT1("SourcePath (2): '%wZ'\n", &pSetupData->SourcePath);
        DPRINT1("SourceRootPath (2): '%wZ'\n", &pSetupData->SourceRootPath);
        DPRINT1("SourceRootDir (2): '%wZ'\n", &pSetupData->SourceRootDir);

        return ERROR_SUCCESS;
    }

    return ERROR_SUCCESS;
}

VOID
FinishSetup(
    IN OUT PUSETUP_DATA pSetupData)
{
    /* Destroy the computer settings list */
    if (pSetupData->ComputerList != NULL)
    {
        DestroyGenericList(pSetupData->ComputerList, TRUE);
        pSetupData->ComputerList = NULL;
    }

    /* Destroy the display settings list */
    if (pSetupData->DisplayList != NULL)
    {
        DestroyGenericList(pSetupData->DisplayList, TRUE);
        pSetupData->DisplayList = NULL;
    }

    /* Destroy the keyboard settings list */
    if (pSetupData->KeyboardList != NULL)
    {
        DestroyGenericList(pSetupData->KeyboardList, TRUE);
        pSetupData->KeyboardList = NULL;
    }

    /* Destroy the keyboard layout list */
    if (pSetupData->LayoutList != NULL)
    {
        DestroyGenericList(pSetupData->LayoutList, TRUE);
        pSetupData->LayoutList = NULL;
    }

    /* Destroy the languages list */
    if (pSetupData->LanguageList != NULL)
    {
        DestroyGenericList(pSetupData->LanguageList, FALSE);
        pSetupData->LanguageList = NULL;
    }

    /* Close the Setup INF */
    SpInfCloseInfFile(pSetupData->SetupInf);
}

/*
 * SIDEEFFECTS
 *  Calls RegInitializeRegistry
 *  Calls ImportRegistryFile
 *  Calls SetDefaultPagefile
 *  Calls SetMountedDeviceValues
 */
ERROR_NUMBER
UpdateRegistry(
    IN OUT PUSETUP_DATA pSetupData,
    /**/IN BOOLEAN RepairUpdateFlag,     /* HACK HACK! */
    /**/IN PPARTLIST PartitionList,      /* HACK HACK! */
    /**/IN WCHAR DestinationDriveLetter, /* HACK HACK! */
    /**/IN PCWSTR SelectedLanguageId,    /* HACK HACK! */
    IN PREGISTRY_STATUS_ROUTINE StatusRoutine OPTIONAL,
    IN PFONTSUBSTSETTINGS SubstSettings OPTIONAL)
{
    ERROR_NUMBER ErrorNumber;
    NTSTATUS Status;
    INFCONTEXT InfContext;
    PCWSTR Action;
    PCWSTR File;
    PCWSTR Section;
    BOOLEAN Success;
    BOOLEAN ShouldRepairRegistry = FALSE;
    BOOLEAN Delete;

    if (RepairUpdateFlag)
    {
        DPRINT1("TODO: Updating / repairing the registry is not completely implemented yet!\n");

        /* Verify the registry hives and check whether we need to update or repair any of them */
        Status = VerifyRegistryHives(&pSetupData->DestinationPath, &ShouldRepairRegistry);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("VerifyRegistryHives failed, Status 0x%08lx\n", Status);
            ShouldRepairRegistry = FALSE;
        }
        if (!ShouldRepairRegistry)
            DPRINT1("No need to repair the registry\n");
    }

DoUpdate:
    ErrorNumber = ERROR_SUCCESS;

    /* Update the registry */
    if (StatusRoutine) StatusRoutine(RegHiveUpdate);

    /* Initialize the registry and setup the registry hives */
    Status = RegInitializeRegistry(&pSetupData->DestinationPath);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("RegInitializeRegistry() failed\n");
        /********** HACK!!!!!!!!!!! **********/
        if (Status == STATUS_NOT_IMPLEMENTED)
        {
            /* The hack was called, return its corresponding error */
            return ERROR_INITIALIZE_REGISTRY;
        }
        else
        /*************************************/
        {
            /* Something else failed */
            return ERROR_CREATE_HIVE;
        }
    }

    if (!RepairUpdateFlag || ShouldRepairRegistry)
    {
        /*
         * We fully setup the hives, in case we are doing a fresh installation
         * (RepairUpdateFlag == FALSE), or in case we are doing an update
         * (RepairUpdateFlag == TRUE) BUT we have some registry hives to
         * "repair" (aka. recreate: ShouldRepairRegistry == TRUE).
         */

        Success = SpInfFindFirstLine(pSetupData->SetupInf, L"HiveInfs.Fresh", NULL, &InfContext);       // Windows-compatible
        if (!Success)
            Success = SpInfFindFirstLine(pSetupData->SetupInf, L"HiveInfs.Install", NULL, &InfContext); // ReactOS-specific

        if (!Success)
        {
            DPRINT1("SpInfFindFirstLine() failed\n");
            ErrorNumber = ERROR_FIND_REGISTRY;
            goto Cleanup;
        }
    }
    else // if (RepairUpdateFlag && !ShouldRepairRegistry)
    {
        /*
         * In case we are doing an update (RepairUpdateFlag == TRUE) and
         * NO registry hives need a repair (ShouldRepairRegistry == FALSE),
         * we only update the hives.
         */

        Success = SpInfFindFirstLine(pSetupData->SetupInf, L"HiveInfs.Upgrade", NULL, &InfContext);
        if (!Success)
        {
            /* Nothing to do for update! */
            DPRINT1("No update needed for the registry!\n");
            goto Cleanup;
        }
    }

    do
    {
        INF_GetDataField(&InfContext, 0, &Action);
        INF_GetDataField(&InfContext, 1, &File);
        INF_GetDataField(&InfContext, 2, &Section);

        DPRINT("Action: %S  File: %S  Section %S\n", Action, File, Section);

        if (Action == NULL)
        {
            INF_FreeData(Action);
            INF_FreeData(File);
            INF_FreeData(Section);
            break; // Hackfix
        }

        if (!_wcsicmp(Action, L"AddReg"))
            Delete = FALSE;
        else if (!_wcsicmp(Action, L"DelReg"))
            Delete = TRUE;
        else
        {
            DPRINT1("Unrecognized registry INF action '%S'\n", Action);
            INF_FreeData(Action);
            INF_FreeData(File);
            INF_FreeData(Section);
            continue;
        }

        INF_FreeData(Action);

        if (StatusRoutine) StatusRoutine(ImportRegHive, File);

        if (!ImportRegistryFile(pSetupData->SourcePath.Buffer,
                                File, Section,
                                pSetupData->LanguageId, Delete))
        {
            DPRINT1("Importing %S failed\n", File);
            INF_FreeData(File);
            INF_FreeData(Section);
            ErrorNumber = ERROR_IMPORT_HIVE;
            goto Cleanup;
        }
    } while (SpInfFindNextLine(&InfContext, &InfContext));

    if (!RepairUpdateFlag || ShouldRepairRegistry)
    {
        /* See the explanation for this test above */

        PGENERIC_LIST_ENTRY Entry;
        PCWSTR LanguageId; // LocaleID;

        Entry = GetCurrentListEntry(pSetupData->DisplayList);
        ASSERT(Entry);
        pSetupData->DisplayType = ((PGENENTRY)GetListEntryData(Entry))->Id;
        ASSERT(pSetupData->DisplayType);

        /* Update display registry settings */
        if (StatusRoutine) StatusRoutine(DisplaySettingsUpdate);
        if (!ProcessDisplayRegistry(pSetupData->SetupInf, pSetupData->DisplayType))
        {
            ErrorNumber = ERROR_UPDATE_DISPLAY_SETTINGS;
            goto Cleanup;
        }

        Entry = GetCurrentListEntry(pSetupData->LanguageList);
        ASSERT(Entry);
        LanguageId = ((PGENENTRY)GetListEntryData(Entry))->Id;
        ASSERT(LanguageId);

        /* Set the locale */
        if (StatusRoutine) StatusRoutine(LocaleSettingsUpdate);
        if (!ProcessLocaleRegistry(/*pSetupData->*/LanguageId))
        {
            ErrorNumber = ERROR_UPDATE_LOCALESETTINGS;
            goto Cleanup;
        }

        /* Add the keyboard layouts for the given language (without user override) */
        if (StatusRoutine) StatusRoutine(KeybLayouts);
        if (!AddKeyboardLayouts(SelectedLanguageId))
        {
            ErrorNumber = ERROR_ADDING_KBLAYOUTS;
            goto Cleanup;
        }

        if (!IsUnattendedSetup)
        {
            Entry = GetCurrentListEntry(pSetupData->LayoutList);
            ASSERT(Entry);
            pSetupData->LayoutId = ((PGENENTRY)GetListEntryData(Entry))->Id;
            ASSERT(pSetupData->LayoutId);

            /* Update keyboard layout settings with user-overridden values */
            // FIXME: Wouldn't it be better to do it all at once
            // with the AddKeyboardLayouts() step?
            if (StatusRoutine) StatusRoutine(KeybSettingsUpdate);
            if (!ProcessKeyboardLayoutRegistry(pSetupData->LayoutId, SelectedLanguageId))
            {
                ErrorNumber = ERROR_UPDATE_KBSETTINGS;
                goto Cleanup;
            }
        }

        /* Set GeoID */
        if (!SetGeoID(MUIGetGeoID(SelectedLanguageId)))
        {
            ErrorNumber = ERROR_UPDATE_GEOID;
            goto Cleanup;
        }

        /* Add codepage information to registry */
        if (StatusRoutine) StatusRoutine(CodePageInfoUpdate);
        if (!AddCodePage(SelectedLanguageId))
        {
            ErrorNumber = ERROR_ADDING_CODEPAGE;
            goto Cleanup;
        }

        /* Set the default pagefile entry */
        SetDefaultPagefile(DestinationDriveLetter);

        /* Update the mounted devices list */
        // FIXME: This should technically be done by mountmgr (if AutoMount is enabled)!
        SetMountedDeviceValues(PartitionList);
    }

#ifdef __REACTOS__
    if (SubstSettings)
    {
        /* HACK */
        DoRegistryFontFixup(SubstSettings, wcstoul(SelectedLanguageId, NULL, 16));
    }
#endif

Cleanup:
    //
    // TODO: Unload all the registry stuff, perform cleanup,
    // and copy the created hive files into .sav files.
    //
    RegCleanupRegistry(&pSetupData->DestinationPath);

    /*
     * Check whether we were in update/repair mode but we were actually
     * repairing the registry hives. If so, we have finished repairing them,
     * and we now reset the flag and run the proper registry update.
     * Otherwise we have finished the registry update!
     */
    if (RepairUpdateFlag && ShouldRepairRegistry)
    {
        ShouldRepairRegistry = FALSE;
        goto DoUpdate;
    }

    return ErrorNumber;
}

/* EOF */
