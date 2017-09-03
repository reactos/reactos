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
    PWCHAR Value;
    WCHAR UnattendInfPath[MAX_PATH];

    CombinePaths(UnattendInfPath, ARRAYSIZE(UnattendInfPath), 2,
                 pSetupData->SourcePath.Buffer, L"unattend.inf");

    DPRINT1("UnattendInf path: '%S'\n", UnattendInfPath);

    if (DoesFileExist(NULL, UnattendInfPath) == FALSE)
    {
        DPRINT1("Does not exist: %S\n", UnattendInfPath);
        return;
    }

    /* Load 'unattend.inf' from installation media */
    UnattendInf = SetupOpenInfFileExW(UnattendInfPath,
                                      NULL,
                                      INF_STYLE_OLDNT,
                                      pSetupData->LanguageId,
                                      &ErrorLine);

    if (UnattendInf == INVALID_HANDLE_VALUE)
    {
        DPRINT1("SetupOpenInfFileExW() failed\n");
        return;
    }

    /* Open 'Unattend' section */
    if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"Signature", &Context))
    {
        DPRINT("SetupFindFirstLineW() failed for section 'Unattend'\n");
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
    if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"UnattendSetupEnabled", &Context))
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
    if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"DestinationDiskNumber", &Context))
    {
        DPRINT("SetupFindFirstLine() failed for key 'DestinationDiskNumber'\n");
        goto Quit;
    }

    if (!SetupGetIntField(&Context, 1, &IntValue))
    {
        DPRINT("SetupGetIntField() failed for key 'DestinationDiskNumber'\n");
        goto Quit;
    }

    pSetupData->DestinationDiskNumber = (LONG)IntValue;

    /* Search for 'DestinationPartitionNumber' in the 'Unattend' section */
    if (!SetupFindFirstLineW(UnattendInf, L"Unattend", L"DestinationPartitionNumber", &Context))
    {
        DPRINT("SetupFindFirstLine() failed for key 'DestinationPartitionNumber'\n");
        goto Quit;
    }

    if (!SetupGetIntField(&Context, 1, &IntValue))
    {
        DPRINT("SetupGetIntField() failed for key 'DestinationPartitionNumber'\n");
        goto Quit;
    }

    pSetupData->DestinationPartitionNumber = (LONG)IntValue;

    /* Search for 'InstallationDirectory' in the 'Unattend' section (optional) */
    if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"InstallationDirectory", &Context))
    {
        /* Get pointer 'InstallationDirectory' key */
        if (!INF_GetData(&Context, NULL, &Value))
        {
            DPRINT("INF_GetData() failed for key 'InstallationDirectory'\n");
            goto Quit;
        }
        wcscpy(pSetupData->InstallationDirectory, Value);
        INF_FreeData(Value);
    }

    IsUnattendedSetup = TRUE;
    DPRINT("Running unattended setup\n");

    /* Search for 'MBRInstallType' in the 'Unattend' section */
    pSetupData->MBRInstallType = -1;
    if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"MBRInstallType", &Context))
    {
        if (SetupGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->MBRInstallType = IntValue;
        }
    }

    /* Search for 'FormatPartition' in the 'Unattend' section */
    pSetupData->FormatPartition = 0;
    if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"FormatPartition", &Context))
    {
        if (SetupGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->FormatPartition = IntValue;
        }
    }

    pSetupData->AutoPartition = 0;
    if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"AutoPartition", &Context))
    {
        if (SetupGetIntField(&Context, 1, &IntValue))
        {
            pSetupData->AutoPartition = IntValue;
        }
    }

    /* Search for LocaleID in the 'Unattend' section */
    if (SetupFindFirstLineW(UnattendInf, L"Unattend", L"LocaleID", &Context))
    {
        if (INF_GetData(&Context, NULL, &Value))
        {
            LONG Id = wcstol(Value, NULL, 16);
            swprintf(pSetupData->LocaleID, L"%08lx", Id);
            INF_FreeData(Value);
       }
    }

Quit:
    SetupCloseInfFile(UnattendInf);
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
    // PCWSTR CrLf = L"\r\n";
    PCSTR CrLf = "\r\n";
    HANDLE FileHandle, UnattendFileHandle, SectionHandle;
    FILE_STANDARD_INFORMATION FileInfo;
    ULONG FileSize;
    PVOID ViewBase;
    UNICODE_STRING FileName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
#endif

    PINICACHESECTION IniSection;
    WCHAR PathBuffer[MAX_PATH];
    WCHAR UnattendInfPath[MAX_PATH];

    /* Create a $winnt$.inf file with default entries */
    IniCache = IniCacheCreate();
    if (!IniCache)
        return;

    IniSection = IniCacheAppendSection(IniCache, L"SetupParams");
    if (IniSection)
    {
        /* Key "skipmissingfiles" */
        // StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                         // L"\"%s\"", L"WinNt5.2");
        // IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          // L"Version", PathBuffer);
    }

    IniSection = IniCacheAppendSection(IniCache, L"Data");
    if (IniSection)
    {
        StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                         L"\"%s\"", IsUnattendedSetup ? L"yes" : L"no");
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"UnattendedInstall", PathBuffer);

        // "floppylessbootpath" (yes/no)

        StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                         L"\"%s\"", L"winnt");
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"ProductType", PathBuffer);

        StringCchPrintfW(PathBuffer, ARRAYSIZE(PathBuffer),
                         L"\"%s\\\"", pSetupData->SourceRootPath.Buffer);
        IniCacheInsertKey(IniSection, NULL, INSERT_LAST,
                          L"SourcePath", PathBuffer);

        // "floppyless" ("0")
    }

#if 0

    /* TODO: Append the standard unattend.inf file */
    CombinePaths(UnattendInfPath, ARRAYSIZE(UnattendInfPath), 2,
                 pSetupData->SourcePath.Buffer, L"unattend.inf");
    if (DoesFileExist(NULL, UnattendInfPath) == FALSE)
    {
        DPRINT1("Does not exist: %S\n", UnattendInfPath);
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
        DPRINT1("Does not exist: %S\n", UnattendInfPath);
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
                            &SectionHandle,
                            &ViewBase,
                            &FileSize,
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
                         2 * sizeof(CHAR), // 2 * sizeof(WCHAR),
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
    HANDLE Handle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UCHAR ImageFileBuffer[sizeof(UNICODE_STRING) + MAX_PATH * sizeof(WCHAR)];
    PUNICODE_STRING InstallSourcePath = (PUNICODE_STRING)&ImageFileBuffer;
    WCHAR SystemRootBuffer[MAX_PATH] = L"";
    UNICODE_STRING SystemRootPath = RTL_CONSTANT_STRING(L"\\SystemRoot");
    ULONG BufferSize;
    PWCHAR Ptr;

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

    Status = NtOpenSymbolicLinkObject(&Handle,
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

    Status = NtQuerySymbolicLinkObject(Handle,
                                       &SystemRootPath,
                                       &BufferSize);
    NtClose(Handle);

    if (!NT_SUCCESS(Status))
        return Status; // Unexpected error

    /* Check whether the resolved \SystemRoot is a prefix of the image file path */
    if (RtlPrefixUnicodeString(&SystemRootPath, InstallSourcePath, TRUE))
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
    OUT HINF* SetupInf,
    IN OUT PUSETUP_DATA pSetupData)
{
    INFCONTEXT Context;
    UINT ErrorLine;
    INT IntValue;
    PWCHAR Value;
    WCHAR FileNameBuffer[MAX_PATH];

    CombinePaths(FileNameBuffer, ARRAYSIZE(FileNameBuffer), 2,
                 pSetupData->SourcePath.Buffer, L"txtsetup.sif");

    DPRINT1("SetupInf path: '%S'\n", FileNameBuffer);

    *SetupInf = SetupOpenInfFileExW(FileNameBuffer,
                                   NULL,
                                   INF_STYLE_WIN4 | INF_STYLE_OLDNT,
                                   pSetupData->LanguageId,
                                   &ErrorLine);

    if (*SetupInf == INVALID_HANDLE_VALUE)
        return ERROR_LOAD_TXTSETUPSIF;

    /* Open 'Version' section */
    if (!SetupFindFirstLineW(*SetupInf, L"Version", L"Signature", &Context))
        return ERROR_CORRUPT_TXTSETUPSIF;

    /* Get pointer 'Signature' key */
    if (!INF_GetData(&Context, NULL, &Value))
        return ERROR_CORRUPT_TXTSETUPSIF;

    /* Check 'Signature' string */
    if (_wcsicmp(Value, L"$ReactOS$") != 0)
    {
        INF_FreeData(Value);
        return ERROR_SIGNATURE_TXTSETUPSIF;
    }

    INF_FreeData(Value);

    /* Open 'DiskSpaceRequirements' section */
    if (!SetupFindFirstLineW(*SetupInf, L"DiskSpaceRequirements", L"FreeSysPartDiskSpace", &Context))
        return ERROR_CORRUPT_TXTSETUPSIF;

    pSetupData->RequiredPartitionDiskSpace = ~0;

    /* Get the 'FreeSysPartDiskSpace' value */
    if (!SetupGetIntField(&Context, 1, &IntValue))
        return ERROR_CORRUPT_TXTSETUPSIF;

    pSetupData->RequiredPartitionDiskSpace = (ULONG)IntValue;

    //
    // TODO: Support "SetupSourceDevice" and "SetupSourcePath" in txtsetup.sif
    // See CORE-9023
    //

    /* Search for 'DefaultPath' in the 'SetupData' section */
    if (SetupFindFirstLineW(*SetupInf, L"SetupData", L"DefaultPath", &Context))
    {
        /* Get pointer 'DefaultPath' key */
        if (!INF_GetData(&Context, NULL, &Value))
            return ERROR_CORRUPT_TXTSETUPSIF;

        wcscpy(pSetupData->InstallationDirectory, Value);
        INF_FreeData(Value);
    }

    return ERROR_SUCCESS;
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
    IN HINF SetupInf,
    IN OUT PUSETUP_DATA pSetupData,
    /**/IN BOOLEAN RepairUpdateFlag,     /* HACK HACK! */
    /**/IN PPARTLIST PartitionList,      /* HACK HACK! */
    /**/IN WCHAR DestinationDriveLetter, /* HACK HACK! */
    /**/IN PWCHAR SelectedLanguageId,    /* HACK HACK! */
    IN PGENERIC_LIST DisplayList,
    IN PGENERIC_LIST LayoutList,
    IN PGENERIC_LIST LanguageList,
    IN PREGISTRY_STATUS_ROUTINE StatusRoutine OPTIONAL)
{
    ERROR_NUMBER ErrorNumber;
    NTSTATUS Status;
    INFCONTEXT InfContext;
    PWSTR Action;
    PWSTR File;
    PWSTR Section;
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

        Success = SetupFindFirstLineW(SetupInf, L"HiveInfs.Fresh", NULL, &InfContext);       // Windows-compatible
        if (!Success)
            Success = SetupFindFirstLineW(SetupInf, L"HiveInfs.Install", NULL, &InfContext); // ReactOS-specific

        if (!Success)
        {
            DPRINT1("SetupFindFirstLine() failed\n");
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

        Success = SetupFindFirstLineW(SetupInf, L"HiveInfs.Upgrade", NULL, &InfContext);
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
    } while (SetupFindNextLine(&InfContext, &InfContext));

    if (!RepairUpdateFlag || ShouldRepairRegistry)
    {
        /* See the explanation for this test above */

        /* Update display registry settings */
        if (StatusRoutine) StatusRoutine(DisplaySettingsUpdate);
        if (!ProcessDisplayRegistry(SetupInf, DisplayList))
        {
            ErrorNumber = ERROR_UPDATE_DISPLAY_SETTINGS;
            goto Cleanup;
        }

        /* Set the locale */
        if (StatusRoutine) StatusRoutine(LocaleSettingsUpdate);
        if (!ProcessLocaleRegistry(LanguageList))
        {
            ErrorNumber = ERROR_UPDATE_LOCALESETTINGS;
            goto Cleanup;
        }

        /* Add keyboard layouts */
        if (StatusRoutine) StatusRoutine(KeybLayouts);
        if (!AddKeyboardLayouts(SelectedLanguageId))
        {
            ErrorNumber = ERROR_ADDING_KBLAYOUTS;
            goto Cleanup;
        }

        /* Set GeoID */
        if (!SetGeoID(MUIGetGeoID(SelectedLanguageId)))
        {
            ErrorNumber = ERROR_UPDATE_GEOID;
            goto Cleanup;
        }

        if (!IsUnattendedSetup)
        {
            /* Update keyboard layout settings */
            if (StatusRoutine) StatusRoutine(KeybSettingsUpdate);
            if (!ProcessKeyboardLayoutRegistry(LayoutList, SelectedLanguageId))
            {
                ErrorNumber = ERROR_UPDATE_KBSETTINGS;
                goto Cleanup;
            }
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
