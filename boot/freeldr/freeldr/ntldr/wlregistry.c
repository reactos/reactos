/*
 * PROJECT:         EFI Windows Loader
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            boot/freeldr/freeldr/windows/wlregistry.c
 * PURPOSE:         Registry support functions
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ***************************************************************/

#include <freeldr.h>
#include "winldr.h"
#include "registry.h"
#include <internal/cmboot.h>

#include <debug.h>
DBG_DEFAULT_CHANNEL(WINDOWS);

// The only global var here, used to mark mem pages as NLS in WinLdrSetupMemoryLayout()
ULONG TotalNLSSize = 0;

static BOOLEAN
WinLdrGetNLSNames(
    _In_ HKEY ControlSet,
    _Inout_ PUNICODE_STRING AnsiFileName,
    _Inout_ PUNICODE_STRING OemFileName,
    _Inout_ PUNICODE_STRING LangFileName, // CaseTable
    _Inout_ PUNICODE_STRING OemHalFileName);

static BOOLEAN
WinLdrScanRegistry(
    IN OUT PLIST_ENTRY BootDriverListHead);


/* FUNCTIONS **************************************************************/

static BOOLEAN
WinLdrLoadSystemHive(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCSTR DirectoryPath,
    IN PCSTR HiveName)
{
    ULONG FileId;
    CHAR FullHiveName[MAX_PATH];
    ARC_STATUS Status;
    FILEINFORMATION FileInfo;
    ULONG HiveFileSize;
    PVOID HiveDataPhysical;
    PVOID HiveDataVirtual;
    ULONG BytesRead;

    /* Concatenate path and filename to get the full name */
    RtlStringCbCopyA(FullHiveName, sizeof(FullHiveName), DirectoryPath);
    RtlStringCbCatA(FullHiveName, sizeof(FullHiveName), HiveName);

    NtLdrOutputLoadMsg(FullHiveName, NULL);
    Status = ArcOpen(FullHiveName, OpenReadOnly, &FileId);
    if (Status != ESUCCESS)
    {
        WARN("Error while opening '%s', Status: %u\n", FullHiveName, Status);
        return FALSE;
    }

    /* Get the file length */
    Status = ArcGetFileInformation(FileId, &FileInfo);
    if (Status != ESUCCESS)
    {
        WARN("Hive file has 0 size!\n");
        ArcClose(FileId);
        return FALSE;
    }
    HiveFileSize = FileInfo.EndingAddress.LowPart;

    /* Round up the size to page boundary and alloc memory */
    HiveDataPhysical = MmAllocateMemoryWithType(
        MM_SIZE_TO_PAGES(HiveFileSize + MM_PAGE_SIZE - 1) << MM_PAGE_SHIFT,
        LoaderRegistryData);

    if (HiveDataPhysical == NULL)
    {
        WARN("Could not alloc memory for hive!\n");
        ArcClose(FileId);
        return FALSE;
    }

    /* Convert address to virtual */
    HiveDataVirtual = PaToVa(HiveDataPhysical);

    /* Fill LoaderBlock's entries */
    LoaderBlock->RegistryLength = HiveFileSize;
    LoaderBlock->RegistryBase = HiveDataVirtual;

    /* Finally read from file to the memory */
    Status = ArcRead(FileId, HiveDataPhysical, HiveFileSize, &BytesRead);
    if (Status != ESUCCESS)
    {
        WARN("Error while reading '%s', Status: %u\n", FullHiveName, Status);
        ArcClose(FileId);
        return FALSE;
    }

    // FIXME: HACK: Get the boot filesystem driver name now...
    BootFileSystem = FsGetServiceName(FileId);

    ArcClose(FileId);
    return TRUE;
}

BOOLEAN
WinLdrInitSystemHive(
    IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
    IN PCSTR SystemRoot,
    IN BOOLEAN Setup)
{
    CHAR SearchPath[1024];
    PCSTR HiveName;
    BOOLEAN Success;

    if (Setup)
    {
        RtlStringCbCopyA(SearchPath, sizeof(SearchPath), SystemRoot);
        HiveName = "SETUPREG.HIV";
    }
    else
    {
        // There is a simple logic here: try to load usual hive (system), if it
        // fails, then give system.alt a try, and finally try a system.sav

        // FIXME: For now we only try system
        RtlStringCbCopyA(SearchPath, sizeof(SearchPath), SystemRoot);
        RtlStringCbCatA(SearchPath, sizeof(SearchPath), "system32\\config\\");
        HiveName = "SYSTEM";
    }

    TRACE("WinLdrInitSystemHive: loading hive %s%s\n", SearchPath, HiveName);
    Success = WinLdrLoadSystemHive(LoaderBlock, SearchPath, HiveName);
    if (!Success)
    {
        UiMessageBox("Could not load %s hive!", HiveName);
        return FALSE;
    }

    /* Import what was loaded */
    Success = RegImportBinaryHive(VaToPa(LoaderBlock->RegistryBase), LoaderBlock->RegistryLength);
    if (!Success)
    {
        UiMessageBox("Importing binary hive failed!");
        return FALSE;
    }

    /* Initialize the 'CurrentControlSet' link */
    if (!RegInitCurrentControlSet(FALSE))
    {
        UiMessageBox("Initializing CurrentControlSet link failed!");
        return FALSE;
    }

    return TRUE;
}

BOOLEAN WinLdrScanSystemHive(IN OUT PLOADER_PARAMETER_BLOCK LoaderBlock,
                             IN PCSTR SystemRoot)
{
    BOOLEAN Success;
    DECLARE_UNICODE_STRING_SIZE(AnsiFileName, MAX_PATH);
    DECLARE_UNICODE_STRING_SIZE(OemFileName, MAX_PATH);
    DECLARE_UNICODE_STRING_SIZE(LangFileName, MAX_PATH); // CaseTable
    DECLARE_UNICODE_STRING_SIZE(OemHalFileName, MAX_PATH);
    CHAR SearchPath[1024];

    /* Scan registry and prepare boot drivers list */
    Success = WinLdrScanRegistry(&LoaderBlock->BootDriverListHead);
    if (!Success)
    {
        UiMessageBox("Failed to load boot drivers!");
        return FALSE;
    }

    /* Get names of NLS files */
    Success = WinLdrGetNLSNames(CurrentControlSetKey,
                                &AnsiFileName,
                                &OemFileName,
                                &LangFileName,
                                &OemHalFileName);
    if (!Success)
    {
        UiMessageBox("Getting NLS names from registry failed!");
        return FALSE;
    }

    TRACE("NLS data: '%wZ' '%wZ' '%wZ' '%wZ'\n",
          &AnsiFileName, &OemFileName, &LangFileName, &OemHalFileName);

    /* Load NLS data */
    RtlStringCbCopyA(SearchPath, sizeof(SearchPath), SystemRoot);
    RtlStringCbCatA(SearchPath, sizeof(SearchPath), "system32\\");
    Success = WinLdrLoadNLSData(LoaderBlock,
                                SearchPath,
                                &AnsiFileName,
                                &OemFileName,
                                &LangFileName,
                                &OemHalFileName);
    TRACE("NLS data loading %s\n", Success ? "successful" : "failed");

    return TRUE;
}


/* PRIVATE FUNCTIONS ******************************************************/

// Queries registry for those three file names
static BOOLEAN
WinLdrGetNLSNames(
    _In_ HKEY ControlSet,
    _Inout_ PUNICODE_STRING AnsiFileName,
    _Inout_ PUNICODE_STRING OemFileName,
    _Inout_ PUNICODE_STRING LangFileName, // CaseTable
    _Inout_ PUNICODE_STRING OemHalFileName)
{
    LONG rc;
    HKEY hKey;
    ULONG BufferSize;
    WCHAR szIdBuffer[80];

    /* Open the CodePage key */
    rc = RegOpenKey(ControlSet, L"Control\\NLS\\CodePage", &hKey);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Couldn't open CodePage registry key\n");
        return FALSE;
    }

    /* Get ANSI codepage file */
    BufferSize = sizeof(szIdBuffer);
    rc = RegQueryValue(hKey, L"ACP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Couldn't get ACP NLS setting\n");
        goto Quit;
    }

    BufferSize = AnsiFileName->MaximumLength;
    rc = RegQueryValue(hKey, szIdBuffer, NULL,
                       (PUCHAR)AnsiFileName->Buffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("ACP NLS Setting exists, but isn't readable\n");
        //goto Quit;
        AnsiFileName->Length = 0;
        RtlAppendUnicodeToString(AnsiFileName, L"c_1252.nls"); // HACK: ReactOS bug CORE-6105
    }
    else
    {
        AnsiFileName->Length = (USHORT)BufferSize - sizeof(UNICODE_NULL);
    }

    /* Get OEM codepage file */
    BufferSize = sizeof(szIdBuffer);
    rc = RegQueryValue(hKey, L"OEMCP", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Couldn't get OEMCP NLS setting\n");
        goto Quit;
    }

    BufferSize = OemFileName->MaximumLength;
    rc = RegQueryValue(hKey, szIdBuffer, NULL,
                       (PUCHAR)OemFileName->Buffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("OEMCP NLS setting exists, but isn't readable\n");
        //goto Quit;
        OemFileName->Length = 0;
        RtlAppendUnicodeToString(OemFileName, L"c_437.nls"); // HACK: ReactOS bug CORE-6105
    }
    else
    {
        OemFileName->Length = (USHORT)BufferSize - sizeof(UNICODE_NULL);
    }

    /* Get OEM HAL font file */
    BufferSize = OemHalFileName->MaximumLength;
    rc = RegQueryValue(hKey, L"OEMHAL", NULL,
                       (PUCHAR)OemHalFileName->Buffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Couldn't get OEMHAL NLS setting\n");
        //goto Quit;
        RtlInitEmptyUnicodeString(OemHalFileName, NULL, 0);
    }
    else
    {
        OemHalFileName->Length = (USHORT)BufferSize - sizeof(UNICODE_NULL);
    }

    RegCloseKey(hKey);

    /* Open the Language key */
    rc = RegOpenKey(ControlSet, L"Control\\NLS\\Language", &hKey);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Couldn't open Language registry key\n");
        return FALSE;
    }

    /* Get the Unicode case table file */
    BufferSize = sizeof(szIdBuffer);
    rc = RegQueryValue(hKey, L"Default", NULL, (PUCHAR)szIdBuffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Couldn't get Language Default setting\n");
        goto Quit;
    }

    BufferSize = LangFileName->MaximumLength;
    rc = RegQueryValue(hKey, szIdBuffer, NULL,
                       (PUCHAR)LangFileName->Buffer, &BufferSize);
    if (rc != ERROR_SUCCESS)
    {
        //TRACE("Language Default setting exists, but isn't readable\n");
        //goto Quit;
        LangFileName->Length = 0;
        RtlAppendUnicodeToString(LangFileName, L"l_intl.nls");
    }
    else
    {
        LangFileName->Length = (USHORT)BufferSize - sizeof(UNICODE_NULL);
    }

Quit:
    RegCloseKey(hKey);
    return (rc == ERROR_SUCCESS);
}

BOOLEAN
WinLdrLoadNLSData(
    _Inout_ PLOADER_PARAMETER_BLOCK LoaderBlock,
    _In_ PCSTR DirectoryPath,
    _In_ PCUNICODE_STRING AnsiFileName,
    _In_ PCUNICODE_STRING OemFileName,
    _In_ PCUNICODE_STRING LangFileName, // CaseTable
    _In_ PCUNICODE_STRING OemHalFileName)
{
    ARC_STATUS Status;
    FILEINFORMATION FileInfo;
    ULONG AnsiFileId = -1, OemFileId = -1, LangFileId = -1;
    ULONG AnsiFileSize, OemFileSize, LangFileSize;
    ULONG TotalSize;
    ULONG BytesRead;
    PVOID NlsDataBase, NlsVirtual;
    BOOLEAN AnsiEqualsOem = FALSE;
    CHAR FileName[MAX_PATH];

    /* There may be a case, where OEM and ANSI pages coincide */
    if (RtlCompareUnicodeString(AnsiFileName, OemFileName, TRUE) == 0)
        AnsiEqualsOem = TRUE;

    /* Open file with ANSI and store its size */
    RtlStringCbPrintfA(FileName, sizeof(FileName), "%s%wZ",
                       DirectoryPath, AnsiFileName);
    Status = ArcOpen(FileName, OpenReadOnly, &AnsiFileId);
    if (Status != ESUCCESS)
    {
        WARN("Error while opening '%s', Status: %u\n", FileName, Status);
        goto Quit;
    }

    Status = ArcGetFileInformation(AnsiFileId, &FileInfo);
    if (Status != ESUCCESS)
        goto Quit;
    AnsiFileSize = FileInfo.EndingAddress.LowPart;
    TRACE("AnsiFileSize: %d\n", AnsiFileSize);

    /* Open OEM file and store its length */
    if (AnsiEqualsOem)
    {
        OemFileSize = 0;
    }
    else
    {
        RtlStringCbPrintfA(FileName, sizeof(FileName), "%s%wZ",
                           DirectoryPath, OemFileName);
        Status = ArcOpen(FileName, OpenReadOnly, &OemFileId);
        if (Status != ESUCCESS)
        {
            WARN("Error while opening '%s', Status: %u\n", FileName, Status);
            goto Quit;
        }

        Status = ArcGetFileInformation(OemFileId, &FileInfo);
        if (Status != ESUCCESS)
            goto Quit;
        OemFileSize = FileInfo.EndingAddress.LowPart;
    }
    TRACE("OemFileSize: %d\n", OemFileSize);

    /* Finally open the language codepage file and store its length */
    RtlStringCbPrintfA(FileName, sizeof(FileName), "%s%wZ",
                       DirectoryPath, LangFileName);
    Status = ArcOpen(FileName, OpenReadOnly, &LangFileId);
    if (Status != ESUCCESS)
    {
        WARN("Error while opening '%s', Status: %u\n", FileName, Status);
        goto Quit;
    }

    Status = ArcGetFileInformation(LangFileId, &FileInfo);
    if (Status != ESUCCESS)
        goto Quit;
    LangFileSize = FileInfo.EndingAddress.LowPart;
    TRACE("LangFileSize: %d\n", LangFileSize);

    //
    // TODO: The OEMHAL file.
    //

    /* Sum up all three length, having in mind that every one of them
       must start at a page boundary => thus round up each file to a page */
    TotalSize = MM_SIZE_TO_PAGES(AnsiFileSize) +
                MM_SIZE_TO_PAGES(OemFileSize)  +
                MM_SIZE_TO_PAGES(LangFileSize);

    /* Store it for later marking the pages as NlsData type */
    TotalNLSSize = TotalSize;

    NlsDataBase = MmAllocateMemoryWithType(TotalSize*MM_PAGE_SIZE, LoaderNlsData);
    if (NlsDataBase == NULL)
        goto Quit;

    NlsVirtual = PaToVa(NlsDataBase);
    LoaderBlock->NlsData->AnsiCodePageData = NlsVirtual;

    LoaderBlock->NlsData->OemCodePageData =
        (PVOID)((ULONG_PTR)NlsVirtual +
        (MM_SIZE_TO_PAGES(AnsiFileSize) << MM_PAGE_SHIFT));

    LoaderBlock->NlsData->UnicodeCodePageData =
        (PVOID)((ULONG_PTR)NlsVirtual +
        (MM_SIZE_TO_PAGES(AnsiFileSize) << MM_PAGE_SHIFT) +
        (MM_SIZE_TO_PAGES(OemFileSize)  << MM_PAGE_SHIFT));

    /* ANSI and OEM data are the same - just set pointers to the same area */
    if (AnsiEqualsOem)
        LoaderBlock->NlsData->OemCodePageData = LoaderBlock->NlsData->AnsiCodePageData;

    /* Now actually read the data into memory, starting with the ANSI file */
    RtlStringCbPrintfA(FileName, sizeof(FileName), "%s%wZ",
                       DirectoryPath, AnsiFileName);
    NtLdrOutputLoadMsg(FileName, NULL);
    Status = ArcRead(AnsiFileId,
                     VaToPa(LoaderBlock->NlsData->AnsiCodePageData),
                     AnsiFileSize, &BytesRead);
    if (Status != ESUCCESS)
    {
        WARN("Error while reading '%s', Status: %u\n", FileName, Status);
        goto Quit;
    }

    /* OEM now, if it isn't the same as the ANSI one */
    if (!AnsiEqualsOem)
    {
        RtlStringCbPrintfA(FileName, sizeof(FileName), "%s%wZ",
                           DirectoryPath, OemFileName);
        NtLdrOutputLoadMsg(FileName, NULL);
        Status = ArcRead(OemFileId,
                         VaToPa(LoaderBlock->NlsData->OemCodePageData),
                         OemFileSize, &BytesRead);
        if (Status != ESUCCESS)
        {
            WARN("Error while reading '%s', Status: %u\n", FileName, Status);
            goto Quit;
        }
    }

    /* Finally the language file */
    RtlStringCbPrintfA(FileName, sizeof(FileName), "%s%wZ",
                       DirectoryPath, LangFileName);
    NtLdrOutputLoadMsg(FileName, NULL);
    Status = ArcRead(LangFileId,
                     VaToPa(LoaderBlock->NlsData->UnicodeCodePageData),
                     LangFileSize, &BytesRead);
    if (Status != ESUCCESS)
    {
        WARN("Error while reading '%s', Status: %u\n", FileName, Status);
        goto Quit;
    }

    //
    // THIS IS a HACK and should be replaced by actually loading the OEMHAL file!
    //
    LoaderBlock->OemFontFile = VaToPa(LoaderBlock->NlsData->UnicodeCodePageData);

    /* Convert NlsTables address to VA */
    LoaderBlock->NlsData = PaToVa(LoaderBlock->NlsData);

Quit:
    if (LangFileId != -1)
        ArcClose(LangFileId);
    if (OemFileId != -1)
        ArcClose(OemFileId);
    if (AnsiFileId != -1)
        ArcClose(AnsiFileId);

    if (Status != ESUCCESS)
        UiMessageBox("Error reading NLS file %s", FileName);

    return (Status == ESUCCESS);
}

static BOOLEAN
WinLdrScanRegistry(
    IN OUT PLIST_ENTRY BootDriverListHead)
{
    BOOLEAN Success;

    /* Find all boot drivers */
    Success = CmpFindDrivers(SystemHive,
                             HKEY_TO_HCI(CurrentControlSetKey),
                             BootLoad,
                             BootFileSystem,
                             BootDriverListHead);
    if (!Success)
        goto Quit;

    /* Sort by group/tag */
    Success = CmpSortDriverList(SystemHive,
                                HKEY_TO_HCI(CurrentControlSetKey),
                                BootDriverListHead);
    if (!Success)
        goto Quit;

    /* Remove circular dependencies (cycles) and sort */
    Success = CmpResolveDriverDependencies(BootDriverListHead);
    if (!Success)
        goto Quit;

Quit:
    /* In case of failure, free the boot driver list */
    if (!Success)
        CmpFreeDriverList(SystemHive, BootDriverListHead);

    return Success;
}

/**
 * @brief
 * Inserts the specified driver entry into the driver list, or updates
 * an existing entry with new ImagePath, ErrorControl, Group and Tag values.
 *
 * @param[in,out]   DriverListHead
 * The driver list where to insert the driver entry.
 *
 * @param[in]   InsertAtHead
 * Whether to insert the driver at the head (TRUE) or at the tail (FALSE)
 * of the driver list.
 *
 * @param[in]   DriverName
 * The driver's name.
 *
 * @param[in]   ImagePath
 * Optional path the the driver's image. If none is specified,
 * a default path is constructed out of the driver's name.
 *
 * @param[in]   GroupName
 * Optional driver group name.
 *
 * @param[in]   ErrorControl
 * @param[in]   Tag
 * The ErrorControl and group Tag values for the driver.
 *
 * @return
 * TRUE if the driver has been inserted into the list or updated, FALSE if not.
 **/
BOOLEAN
WinLdrAddDriverToList(
    _Inout_ PLIST_ENTRY DriverListHead,
    _In_ BOOLEAN InsertAtHead,
    _In_ PCWSTR DriverName,
    _In_opt_ PCWSTR ImagePath,
    _In_opt_ PCWSTR GroupName,
    _In_ ULONG ErrorControl,
    _In_ ULONG Tag)
{
    PBOOT_DRIVER_NODE DriverNode;
    PBOOT_DRIVER_LIST_ENTRY DriverEntry;
    BOOLEAN AlreadyInserted;
    USHORT PathLength;
    UNICODE_STRING DriverNameU;
    UNICODE_STRING RegistryPath;
    UNICODE_STRING FilePath = {0};
    UNICODE_STRING RegistryString = {0};
    UNICODE_STRING GroupString = {0};

    /* Check whether the driver is already in the list */
    RtlInitUnicodeString(&DriverNameU, DriverName);
    AlreadyInserted = CmpIsDriverInList(DriverListHead,
                                        &DriverNameU,
                                        &DriverNode);
    if (AlreadyInserted)
    {
        /* If so, we have obtained its node */
        ASSERT(DriverNode);
        DriverEntry = &DriverNode->ListEntry;
    }
    else
    {
        /* Allocate a driver node and initialize it */
        DriverNode = CmpAllocate(sizeof(BOOT_DRIVER_NODE), FALSE, TAG_CM);
        if (!DriverNode)
            return FALSE;

        RtlZeroMemory(DriverNode, sizeof(BOOT_DRIVER_NODE));
        DriverEntry = &DriverNode->ListEntry;

        /* Driver Name */
        RtlInitEmptyUnicodeString(&DriverNode->Name,
                                  CmpAllocate(DriverNameU.Length, FALSE, TAG_CM),
                                  DriverNameU.Length);
        if (!DriverNode->Name.Buffer)
            goto Failure;

        if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&DriverNode->Name, &DriverNameU)))
            goto Failure;
    }

    /* Check whether we have a valid ImagePath. If not, we need
     * to build it like "System32\\Drivers\\blah.sys" */
    if (ImagePath && *ImagePath)
    {
        /* Just copy ImagePath to the corresponding field in the structure */
        PathLength = (USHORT)(wcslen(ImagePath)) * sizeof(WCHAR);
        RtlInitEmptyUnicodeString(&FilePath,
                                  CmpAllocate(PathLength, FALSE, TAG_WLDR_NAME),
                                  PathLength);
        if (!FilePath.Buffer)
            goto Failure;

        if (!NT_SUCCESS(RtlAppendUnicodeToString(&FilePath, ImagePath)))
            goto Failure;
    }
    else
    {
        /* We have to construct ImagePath ourselves */
        PathLength = DriverNode->Name.Length + sizeof(L"system32\\drivers\\.sys");
        RtlInitEmptyUnicodeString(&FilePath,
                                  CmpAllocate(PathLength, FALSE, TAG_WLDR_NAME),
                                  PathLength);
        if (!FilePath.Buffer)
            goto Failure;

        if (!NT_SUCCESS(RtlAppendUnicodeToString(&FilePath, L"system32\\drivers\\"))  ||
            !NT_SUCCESS(RtlAppendUnicodeStringToString(&FilePath, &DriverNode->Name)) ||
            !NT_SUCCESS(RtlAppendUnicodeToString(&FilePath, L".sys")))
        {
            goto Failure;
        }
    }

    /* Registry path */
    RtlInitUnicodeString(&RegistryPath,
                         L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\");
    PathLength = RegistryPath.Length + DriverNode->Name.Length;
    RtlInitEmptyUnicodeString(&RegistryString,
                              CmpAllocate(PathLength, FALSE, TAG_WLDR_NAME),
                              PathLength);
    if (!RegistryString.Buffer)
        goto Failure;

    if (!NT_SUCCESS(RtlAppendUnicodeStringToString(&RegistryString, &RegistryPath)) ||
        !NT_SUCCESS(RtlAppendUnicodeStringToString(&RegistryString, &DriverNode->Name)))
    {
        goto Failure;
    }

    /* Group */
    if (GroupName && *GroupName)
    {
        /*
         * NOTE: Here we can use our own allocator as we alone maintain the
         * group string. This is different from the other allocated strings,
         * where we instead need to use the same (hive) allocator as the
         * one used by CmpAddDriverToList(), for interoperability purposes.
         */
        RtlCreateUnicodeString(&GroupString, GroupName);
        if (!GroupString.Buffer)
            goto Failure;
    }
    else
    {
        RtlInitEmptyUnicodeString(&GroupString, NULL, 0);
    }

    /* Set or replace the driver node's file path */
    if (DriverEntry->FilePath.Buffer)
    {
        CmpFree(DriverEntry->FilePath.Buffer,
                DriverEntry->FilePath.MaximumLength);
    }
    DriverEntry->FilePath = FilePath;
    FilePath.Buffer = NULL;

    /* Set or replace the driver node's registry path */
    if (DriverEntry->RegistryPath.Buffer)
    {
        CmpFree(DriverEntry->RegistryPath.Buffer,
                DriverEntry->RegistryPath.MaximumLength);
    }
    DriverEntry->RegistryPath = RegistryString;
    RegistryString.Buffer = NULL;

    /* Set or replace the driver node's group */
    if (DriverNode->Group.Buffer)
    {
        /*
         * If the buffer is inside the registry hive's memory, this means that
         * it has been set by CmpAddDriverToList() to point to some data within
         * the hive; thus we should not free the buffer but just replace it.
         * Otherwise, this is a buffer previously allocated by ourselves, that
         * we can free.
         *
         * NOTE: This function does not have an explicit LoaderBlock input
         * parameter pointer, since it does not need it, except for this
         * very place. So instead, use the global WinLdrSystemBlock pointer.
         */
        PLOADER_PARAMETER_BLOCK LoaderBlock =
            (WinLdrSystemBlock ? &WinLdrSystemBlock->LoaderBlock : NULL);

        if (!LoaderBlock || !LoaderBlock->RegistryBase || !LoaderBlock->RegistryLength ||
            ((ULONG_PTR)DriverNode->Group.Buffer <
                (ULONG_PTR)VaToPa(LoaderBlock->RegistryBase)) ||
            ((ULONG_PTR)DriverNode->Group.Buffer >=
                (ULONG_PTR)VaToPa(LoaderBlock->RegistryBase) + LoaderBlock->RegistryLength))
        {
            RtlFreeUnicodeString(&DriverNode->Group);
        }
    }
    DriverNode->Group = GroupString;
    GroupString.Buffer = NULL;

    /* ErrorControl and Tag */
    DriverNode->ErrorControl = ErrorControl;
    DriverNode->Tag = Tag;

    /* Insert the entry into the list if it does not exist there already */
    if (!AlreadyInserted)
    {
        if (InsertAtHead)
            InsertHeadList(DriverListHead, &DriverEntry->Link);
        else
            InsertTailList(DriverListHead, &DriverEntry->Link);
    }

    return TRUE;

Failure:
    if (GroupString.Buffer)
        RtlFreeUnicodeString(&GroupString);
    if (RegistryString.Buffer)
        CmpFree(RegistryString.Buffer, RegistryString.MaximumLength);
    if (FilePath.Buffer)
        CmpFree(FilePath.Buffer, FilePath.MaximumLength);

    /* If it does not exist in the list already, free the allocated
     * driver node, otherwise keep the original one in place. */
    if (!AlreadyInserted)
    {
        if (DriverEntry->RegistryPath.Buffer)
        {
            CmpFree(DriverEntry->RegistryPath.Buffer,
                    DriverEntry->RegistryPath.MaximumLength);
        }
        if (DriverEntry->FilePath.Buffer)
        {
            CmpFree(DriverEntry->FilePath.Buffer,
                    DriverEntry->FilePath.MaximumLength);
        }
        if (DriverNode->Name.Buffer)
        {
            CmpFree(DriverNode->Name.Buffer,
                    DriverNode->Name.MaximumLength);
        }
        CmpFree(DriverNode, sizeof(BOOT_DRIVER_NODE));
    }

    return FALSE;
}
