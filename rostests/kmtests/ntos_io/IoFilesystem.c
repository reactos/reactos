/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite File System test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

/* FIXME: Test this stuff on non-FAT volumes */

static
NTSTATUS
QueryFileInfo(
    _In_ HANDLE FileHandle,
    _Out_ PVOID *Info,
    _Inout_ PSIZE_T Length,
    _In_ FILE_INFORMATION_CLASS FileInformationClass)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;
    PVOID Buffer;

    *Info = NULL;
    if (*Length)
    {
        Buffer = KmtAllocateGuarded(*Length);
        if (skip(Buffer != NULL, "Failed to allocate %Iu bytes\n", *Length))
            return STATUS_INSUFFICIENT_RESOURCES;
    }
    else
    {
        Buffer = NULL;
    }
    RtlFillMemory(Buffer, *Length, 0xDD);
    RtlFillMemory(&IoStatus, sizeof(IoStatus), 0x55);
    _SEH2_TRY
    {
        Status = ZwQueryInformationFile(FileHandle,
                                        &IoStatus,
                                        Buffer,
                                        *Length,
                                        FileInformationClass);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
        ok(0, "Exception %lx querying class %d with length %Iu\n",
           Status, FileInformationClass, *Length);
    }
    _SEH2_END;
    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(FileHandle, FALSE, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = IoStatus.Status;
    }
    *Length = IoStatus.Information;
    *Info = Buffer;
    return Status;
}

static
VOID
TestAllInformation(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING FileName = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\ntoskrnl.exe");
    UNICODE_STRING Ntoskrnl = RTL_CONSTANT_STRING(L"ntoskrnl.exe");
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE FileHandle;
    IO_STATUS_BLOCK IoStatus;
    PFILE_ALL_INFORMATION FileAllInfo;
    SIZE_T Length;
    ULONG NameLength;
    PWCHAR Name = NULL;
    UNICODE_STRING NamePart;

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&FileHandle,
                        SYNCHRONIZE | FILE_READ_ATTRIBUTES,
                        &ObjectAttributes,
                        &IoStatus,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_NON_DIRECTORY_FILE);
    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(FileHandle, FALSE, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = IoStatus.Status;
    }
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No file handle, %lx\n", Status))
        return;

    /* NtQueryInformationFile doesn't do length checks for kernel callers in a free build */
    if (KmtIsCheckedBuild)
    {
    /* Zero length */
    Length = 0;
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok_eq_size(Length, (ULONG_PTR)0x5555555555555555);
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* One less than the minimum */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) - 1;
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_INFO_LENGTH_MISMATCH);
    ok_eq_size(Length, (ULONG_PTR)0x5555555555555555);
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);
    }

    /* The minimum allowed */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName);
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName));
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* Plenty of space -- determine NameLength and copy the name */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + MAX_PATH * sizeof(WCHAR);
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!skip(NT_SUCCESS(Status) && FileAllInfo != NULL, "No info\n"))
    {
        NameLength = FileAllInfo->NameInformation.FileNameLength;
        ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength);
        Name = ExAllocatePoolWithTag(PagedPool, NameLength + sizeof(UNICODE_NULL), 'sFmK');
        if (!skip(Name != NULL, "Could not allocate %lu bytes\n", NameLength + (ULONG)sizeof(UNICODE_NULL)))
        {
            RtlCopyMemory(Name,
                          FileAllInfo->NameInformation.FileName,
                          NameLength);
            Name[NameLength / sizeof(WCHAR)] = UNICODE_NULL;
            ok(Name[0] == L'\\', "Name is %ls, expected first char to be \\\n", Name);
            ok(NameLength >= Ntoskrnl.Length + sizeof(WCHAR), "NameLength %lu too short\n", NameLength);
            if (NameLength >= Ntoskrnl.Length)
            {
                NamePart.Buffer = Name + (NameLength - Ntoskrnl.Length) / sizeof(WCHAR);
                NamePart.Length = Ntoskrnl.Length;
                NamePart.MaximumLength = NamePart.Length;
                ok(RtlEqualUnicodeString(&NamePart, &Ntoskrnl, TRUE),
                   "Name ends in '%wZ', expected %wZ\n", &NamePart, &Ntoskrnl);
            }
        }
        ok(FileAllInfo->NameInformation.FileName[NameLength / sizeof(WCHAR)] == 0xdddd,
           "Char past FileName is %x\n",
           FileAllInfo->NameInformation.FileName[NameLength / sizeof(WCHAR)]);
    }
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* One char less than needed */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength - sizeof(WCHAR);
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength - sizeof(WCHAR));
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* One byte less than needed */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength - 1;
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength - 1);
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* Exactly the required size */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength;
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength);
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* One byte more than needed */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength + 1;
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength);
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    /* One char more than needed */
    Length = FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength + sizeof(WCHAR);
    Status = QueryFileInfo(FileHandle, (PVOID*)&FileAllInfo, &Length, FileAllInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size(Length, FIELD_OFFSET(FILE_ALL_INFORMATION, NameInformation.FileName) + NameLength);
    if (FileAllInfo)
        KmtFreeGuarded(FileAllInfo);

    ExFreePoolWithTag(Name, 'sFmK');

    Status = ObCloseHandle(FileHandle, KernelMode);
    ok_eq_hex(Status, STATUS_SUCCESS);
}

static
VOID
Substitute(
    _Out_writes_bytes_(BufferSize) PWCHAR Buffer,
    _In_ ULONG BufferSize,
    _In_ PCWSTR Template,
    _In_ PCWSTR SystemDriveName,
    _In_ PCWSTR SystemRootName)
{
    UNICODE_STRING SystemDriveTemplate = RTL_CONSTANT_STRING(L"C:");
    UNICODE_STRING SystemRootTemplate = RTL_CONSTANT_STRING(L"ReactOS");
    ULONG SystemDriveLength;
    ULONG SystemRootLength;
    PWCHAR Dest = Buffer;
    UNICODE_STRING String;

    SystemDriveLength = wcslen(SystemDriveName) * sizeof(WCHAR);
    SystemRootLength = wcslen(SystemRootName) * sizeof(WCHAR);

    RtlInitUnicodeString(&String, Template);
    ASSERT(String.Length % sizeof(WCHAR) == 0);
    while (String.Length)
    {
        if (RtlPrefixUnicodeString(&SystemDriveTemplate, &String, TRUE))
        {
            ASSERT((Dest - Buffer) * sizeof(WCHAR) + SystemDriveLength < BufferSize);
            RtlCopyMemory(Dest,
                          SystemDriveName,
                          SystemDriveLength);
            Dest += SystemDriveLength / sizeof(WCHAR);

            String.Buffer += SystemDriveTemplate.Length / sizeof(WCHAR);
            String.Length -= SystemDriveTemplate.Length;
            String.MaximumLength -= SystemDriveTemplate.Length;
            continue;
        }

        if (RtlPrefixUnicodeString(&SystemRootTemplate, &String, TRUE))
        {
            ASSERT((Dest - Buffer) * sizeof(WCHAR) + SystemRootLength < BufferSize);
            RtlCopyMemory(Dest,
                          SystemRootName,
                          SystemRootLength);
            Dest += SystemRootLength / sizeof(WCHAR);

            String.Buffer += SystemRootTemplate.Length / sizeof(WCHAR);
            String.Length -= SystemRootTemplate.Length;
            String.MaximumLength -= SystemRootTemplate.Length;
            continue;
        }

        ASSERT(Dest - Buffer < BufferSize / sizeof(WCHAR));
        *Dest++ = String.Buffer[0];

        String.Buffer++;
        String.Length -= sizeof(WCHAR);
        String.MaximumLength -= sizeof(WCHAR);
    }
    ASSERT(Dest - Buffer < BufferSize / sizeof(WCHAR));
    *Dest = UNICODE_NULL;
}

static
VOID
TestRelativeNames(VOID)
{
    NTSTATUS Status;
    struct
    {
        PCWSTR ParentPathTemplate;
        PCWSTR RelativePathTemplate;
        BOOLEAN IsDirectory;
        NTSTATUS Status;
        BOOLEAN IsDrive;
    } Tests[] =
    {
        { NULL,                         L"C:\\",                            TRUE,   STATUS_SUCCESS, TRUE },
        { NULL,                         L"C:\\\\",                          TRUE,   STATUS_SUCCESS, TRUE },
        { NULL,                         L"C:\\\\\\",                        TRUE,   STATUS_OBJECT_NAME_INVALID, TRUE },
        { NULL,                         L"C:\\ReactOS",                     TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\ReactOS\\",                   TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\ReactOS\\\\",                 TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\ReactOS\\\\\\",               TRUE,   STATUS_OBJECT_NAME_INVALID },
        { NULL,                         L"C:\\\\ReactOS",                   TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\\\ReactOS\\",                 TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\ReactOS\\explorer.exe",       FALSE,  STATUS_SUCCESS },
        { NULL,                         L"C:\\ReactOS\\\\explorer.exe",     FALSE,  STATUS_OBJECT_NAME_INVALID },
        { NULL,                         L"C:\\ReactOS\\explorer.exe\\",     FALSE,  STATUS_OBJECT_NAME_INVALID },
        { NULL,                         L"C:\\ReactOS\\explorer.exe\\\\",   FALSE,  STATUS_OBJECT_NAME_INVALID },
        /* This will never return STATUS_NOT_A_DIRECTORY. IsDirectory=TRUE is a little hacky but achieves that without special handling */
        { NULL,                         L"C:\\ReactOS\\explorer.exe\\\\\\", TRUE,   STATUS_OBJECT_NAME_INVALID },
        { L"C:\\",                      L"",                                TRUE,   STATUS_SUCCESS },
        { L"C:\\",                      L"\\",                              TRUE,   STATUS_OBJECT_NAME_INVALID },
        { L"C:\\",                      L"ReactOS",                         TRUE,   STATUS_SUCCESS },
        { L"C:\\",                      L"\\ReactOS",                       TRUE,   STATUS_OBJECT_NAME_INVALID },
        { L"C:\\",                      L"ReactOS\\",                       TRUE,   STATUS_SUCCESS },
        { L"C:\\",                      L"\\ReactOS\\",                     TRUE,   STATUS_OBJECT_NAME_INVALID },
        { L"C:\\ReactOS",               L"",                                TRUE,   STATUS_SUCCESS },
        { L"C:\\ReactOS",               L"explorer.exe",                    FALSE,  STATUS_SUCCESS },
        { L"C:\\ReactOS\\explorer.exe", L"",                                FALSE,  STATUS_SUCCESS },
        { L"C:\\ReactOS\\explorer.exe", L"file",                            FALSE,  STATUS_OBJECT_PATH_NOT_FOUND },
        /* Let's try some nonexistent things */
        { NULL,                         L"C:\\ReactOS\\IDoNotExist",        FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file",  FALSE,  STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file?", FALSE,  STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file\\\\",TRUE,STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file\\\\\\",TRUE,STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\AmIInvalid?",        FALSE,  STATUS_OBJECT_NAME_INVALID },
        { NULL,                         L"C:\\ReactOS\\.",                  TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\..",                 TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\...",                TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\.\\system32",        TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\..\\ReactOS",        TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        { L"C:\\",                      L".",                               TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\",                      L"..",                              TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\",                      L"...",                             TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\",                      L".\\ReactOS",                      TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        { L"C:\\",                      L"..\\ReactOS",                     TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        { L"C:\\ReactOS",               L".",                               TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\ReactOS",               L"..",                              TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\ReactOS",               L"...",                             TRUE,   STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\ReactOS",               L".\\system32",                     TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        { L"C:\\ReactOS",               L"..\\ReactOS",                     TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        /* Volume open */
        { NULL,                         L"C:",                              FALSE,  STATUS_SUCCESS, TRUE },
        { L"C:",                        L"",                                FALSE,  STATUS_SUCCESS, TRUE },
        { L"C:",                        L"\\",                              TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
        { L"C:",                        L"file",                            TRUE,   STATUS_OBJECT_PATH_NOT_FOUND },
    };
    ULONG i;
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING ParentPath;
    UNICODE_STRING RelativePath;
    HANDLE ParentHandle;
    HANDLE FileHandle;
    UNICODE_STRING SystemRoot = RTL_CONSTANT_STRING(L"\\SystemRoot");
    HANDLE SymbolicLinkHandle = NULL;
    WCHAR LinkNameBuffer[128];
    UNICODE_STRING SymbolicLinkName;
    PWSTR SystemDriveName;
    PWSTR SystemRootName;
    PWCHAR Buffer = NULL;
    BOOLEAN TrailingBackslash;
    LARGE_INTEGER AllocationSize;
    FILE_DISPOSITION_INFORMATION DispositionInfo;

    /* Query \SystemRoot */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenSymbolicLinkObject(&SymbolicLinkHandle,
                                      GENERIC_READ,
                                      &ObjectAttributes);
    if (skip(NT_SUCCESS(Status), "Failed to open SystemRoot, %lx\n", Status))
        return;

    RtlInitEmptyUnicodeString(&SymbolicLinkName,
                              LinkNameBuffer,
                              sizeof(LinkNameBuffer));
    Status = ZwQuerySymbolicLinkObject(SymbolicLinkHandle,
                                       &SymbolicLinkName,
                                       NULL);
    ObCloseHandle(SymbolicLinkHandle, KernelMode);
    if (skip(NT_SUCCESS(Status), "Failed to query SystemRoot, %lx\n", Status))
        return;

    /* Split SymbolicLinkName into drive and path */
    SystemDriveName = SymbolicLinkName.Buffer;
    SystemRootName = SymbolicLinkName.Buffer + SymbolicLinkName.Length / sizeof(WCHAR);
    *SystemRootName-- = UNICODE_NULL;
    while (*SystemRootName != L'\\')
    {
        ASSERT(SystemRootName > SymbolicLinkName.Buffer);
        SystemRootName--;
    }
    *SystemRootName++ = UNICODE_NULL;
    trace("System Drive: '%ls'\n", SystemDriveName);
    trace("System Root: '%ls'\n", SystemRootName);

    /* Allocate path buffer */
    Buffer = ExAllocatePoolWithTag(PagedPool, MAXUSHORT, 'sFmK');
    if (skip(Buffer != NULL, "No buffer\n"))
        return;

    /* Finally run some tests! */
    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        /* Open parent directory first */
        ParentHandle = NULL;
        if (Tests[i].ParentPathTemplate)
        {
            Substitute(Buffer,
                       MAXUSHORT,
                       Tests[i].ParentPathTemplate,
                       SystemDriveName,
                       SystemRootName);
            RtlInitUnicodeString(&ParentPath, Buffer);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &ParentPath,
                                       OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
            Status = ZwOpenFile(&ParentHandle,
                                GENERIC_READ,
                                &ObjectAttributes,
                                &IoStatus,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                0);
            ok(Status == STATUS_SUCCESS,
               "[%lu] Status = %lx, expected STATUS_SUCCESS\n", i, Status);
            if (skip(NT_SUCCESS(Status), "No parent handle %lu\n", i))
                continue;
        }

        /* Now open the relative file: */
        Substitute(Buffer,
                   MAXUSHORT,
                   Tests[i].RelativePathTemplate,
                   SystemDriveName,
                   SystemRootName);
        RtlInitUnicodeString(&RelativePath, Buffer);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RelativePath,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   ParentHandle,
                                   NULL);
        TrailingBackslash = FALSE;
        if (wcslen(Buffer) && Buffer[wcslen(Buffer) - 1] == L'\\')
            TrailingBackslash = TRUE;

        /* (1) No flags */
        Status = ZwOpenFile(&FileHandle,
                            GENERIC_READ,
                            &ObjectAttributes,
                            &IoStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            0);
        ok(Status == Tests[i].Status,
           "[%lu] Status = %lx, expected %lx\n", i, Status, Tests[i].Status);
        if (NT_SUCCESS(Status))
            ObCloseHandle(FileHandle, KernelMode);

        /* (2) Directory File */
        Status = ZwOpenFile(&FileHandle,
                            GENERIC_READ,
                            &ObjectAttributes,
                            &IoStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE);
        if (Tests[i].IsDirectory || (!TrailingBackslash && !NT_SUCCESS(Tests[i].Status)))
            ok(Status == Tests[i].Status,
               "[%lu] Status = %lx, expected %lx\n", i, Status, Tests[i].Status);
        else
            ok(Status == STATUS_NOT_A_DIRECTORY,
               "[%lu] Status = %lx, expected STATUS_NOT_A_DIRECTORY\n", i, Status);
        if (NT_SUCCESS(Status))
            ObCloseHandle(FileHandle, KernelMode);

        /* (3) Non-Directory File */
        Status = ZwOpenFile(&FileHandle,
                            GENERIC_READ,
                            &ObjectAttributes,
                            &IoStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_NON_DIRECTORY_FILE);
        if (Tests[i].IsDirectory && NT_SUCCESS(Tests[i].Status))
            ok(Status == STATUS_FILE_IS_A_DIRECTORY,
               "[%lu] Status = %lx, expected STATUS_FILE_IS_A_DIRECTORY\n", i, Status);
        else
            ok(Status == Tests[i].Status,
               "[%lu] Status = %lx, expected %lx\n", i, Status, Tests[i].Status);
        if (NT_SUCCESS(Status))
            ObCloseHandle(FileHandle, KernelMode);

        /* (4) Directory + Non-Directory */
        Status = ZwOpenFile(&FileHandle,
                            GENERIC_READ,
                            &ObjectAttributes,
                            &IoStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            FILE_DIRECTORY_FILE | FILE_NON_DIRECTORY_FILE);
        if (Tests[i].Status == STATUS_OBJECT_NAME_INVALID && Tests[i].IsDrive)
            ok(Status == STATUS_OBJECT_NAME_INVALID,
               "[%lu] Status = %lx, expected STATUS_OBJECT_NAME_INVALID\n", i, Status);
        else
            ok(Status == STATUS_INVALID_PARAMETER,
               "[%lu] Status = %lx, expected STATUS_INVALID_PARAMETER\n", i, Status);
        if (NT_SUCCESS(Status))
            ObCloseHandle(FileHandle, KernelMode);

        /* (5) Try to create it */
        AllocationSize.QuadPart = 0;
        Status = ZwCreateFile(&FileHandle,
                              GENERIC_READ | DELETE,
                              &ObjectAttributes,
                              &IoStatus,
                              &AllocationSize,
                              FILE_ATTRIBUTE_NORMAL,
                              FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                              FILE_CREATE,
                              0,
                              NULL,
                              0);
        if (Tests[i].Status == STATUS_OBJECT_NAME_NOT_FOUND)
            ok(Status == STATUS_SUCCESS,
               "[%lu] Status = %lx, expected STATUS_SUCCESS\n", i, Status);
        else if (Tests[i].Status == STATUS_OBJECT_NAME_INVALID && Tests[i].IsDrive)
            ok(Status == STATUS_OBJECT_NAME_INVALID,
               "[%lu] Status = %lx, expected STATUS_OBJECT_NAME_INVALID\n", i, Status);else if (Tests[i].IsDrive)
            ok(Status == STATUS_ACCESS_DENIED,
               "[%lu] Status = %lx, expected STATUS_ACCESS_DENIED\n", i, Status);
        else if (Tests[i].Status == STATUS_SUCCESS)
            ok(Status == STATUS_OBJECT_NAME_COLLISION,
               "[%lu] Status = %lx, expected STATUS_OBJECT_NAME_COLLISION\n", i, Status);
        else
            ok(Status == Tests[i].Status,
               "[%lu] Status = %lx, expected %lx; %ls -- %ls\n", i, Status, Tests[i].Status, Tests[i].ParentPathTemplate, Tests[i].RelativePathTemplate);
        if (NT_SUCCESS(Status))
        {
            if (IoStatus.Information == FILE_CREATED)
            {
                DispositionInfo.DeleteFile = TRUE;
                Status = ZwSetInformationFile(FileHandle,
                                              &IoStatus,
                                              &DispositionInfo,
                                              sizeof(DispositionInfo),
                                              FileDispositionInformation);
                ok(Status == STATUS_SUCCESS,
                   "[%lu] Status = %lx, expected STATUS_SUCCESS\n", i, Status);
            }
            ObCloseHandle(FileHandle, KernelMode);
        }

        /* And close */
        ObCloseHandle(ParentHandle, KernelMode);
    }

    ExFreePoolWithTag(Buffer, 'sFmK');
}

static
VOID
TestSharedCacheMap(VOID)
{
    NTSTATUS Status;
    struct
    {
        PCWSTR ParentPath;
        PCWSTR RelativePath;
    } Tests[] =
    {
        { 0, L"\\SystemRoot\\system32\\drivers\\etc\\hosts" },
        { L"\\SystemRoot", L"system32\\drivers\\etc\\hosts" },
        { L"\\SystemRoot\\system32", L"drivers\\etc\\hosts" },
        { L"\\SystemRoot\\system32\\drivers", L"etc\\hosts" },
        { L"\\SystemRoot\\system32\\drivers\\etc", L"hosts" },
    };
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    UNICODE_STRING ParentPath;
    UNICODE_STRING RelativePath;
    HANDLE ParentHandle[RTL_NUMBER_OF(Tests)] = { NULL };
    HANDLE FileHandle[RTL_NUMBER_OF(Tests)] = { NULL };
    PFILE_OBJECT FileObject[RTL_NUMBER_OF(Tests)] = { NULL };
    PFILE_OBJECT SystemRootObject = NULL;
    UCHAR Buffer[32];
    HANDLE EventHandle;
    LARGE_INTEGER FileOffset;
    ULONG i;

    /* We need an event for ZwReadFile */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateEvent(&EventHandle,
                           SYNCHRONIZE,
                           &ObjectAttributes,
                           NotificationEvent,
                           FALSE);
    if (skip(NT_SUCCESS(Status), "No event\n"))
        goto Cleanup;

    /* Open all test files and get their FILE_OBJECT pointers */
    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        if (Tests[i].ParentPath)
        {
            RtlInitUnicodeString(&ParentPath, Tests[i].ParentPath);
            InitializeObjectAttributes(&ObjectAttributes,
                                       &ParentPath,
                                       OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                       NULL,
                                       NULL);
            Status = ZwOpenFile(&ParentHandle[i],
                                GENERIC_READ,
                                &ObjectAttributes,
                                &IoStatus,
                                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                                0);
            ok_eq_hex(Status, STATUS_SUCCESS);
            if (skip(NT_SUCCESS(Status), "No parent handle %lu\n", i))
                goto Cleanup;
        }

        RtlInitUnicodeString(&RelativePath, Tests[i].RelativePath);
        InitializeObjectAttributes(&ObjectAttributes,
                                   &RelativePath,
                                   OBJ_KERNEL_HANDLE | OBJ_CASE_INSENSITIVE,
                                   ParentHandle[i],
                                   NULL);
        Status = ZwOpenFile(&FileHandle[i],
                            FILE_ALL_ACCESS,
                            &ObjectAttributes,
                            &IoStatus,
                            FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                            0);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (skip(NT_SUCCESS(Status), "No file handle %lu\n", i))
            goto Cleanup;

        Status = ObReferenceObjectByHandle(FileHandle[i],
                                           FILE_ALL_ACCESS,
                                           *IoFileObjectType,
                                           KernelMode,
                                           (PVOID*)&FileObject[i],
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (skip(NT_SUCCESS(Status), "No file object %lu\n", i))
            goto Cleanup;
    }

    /* Also get a file object for the SystemRoot directory */
    Status = ObReferenceObjectByHandle(ParentHandle[1],
                                       GENERIC_READ,
                                       *IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&SystemRootObject,
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No SystemRoot object\n"))
        goto Cleanup;

    /* Before read, caching is not initialized */
    ok_eq_pointer(SystemRootObject->SectionObjectPointer, NULL);
    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        ok(FileObject[i]->SectionObjectPointer != NULL, "FileObject[%lu]->SectionObjectPointer = NULL\n", i);
        ok(FileObject[i]->SectionObjectPointer == FileObject[0]->SectionObjectPointer,
           "FileObject[%lu]->SectionObjectPointer = %p, expected %p\n",
           i, FileObject[i]->SectionObjectPointer, FileObject[0]->SectionObjectPointer);
    }
    if (!skip(FileObject[0]->SectionObjectPointer != NULL, "No section object pointers\n"))
        ok_eq_pointer(FileObject[0]->SectionObjectPointer->SharedCacheMap, NULL);

    /* Perform a read on one handle to initialize caching */
    FileOffset.QuadPart = 0;
    Status = ZwReadFile(FileHandle[0],
                        EventHandle,
                        NULL,
                        NULL,
                        &IoStatus,
                        Buffer,
                        sizeof(Buffer),
                        &FileOffset,
                        NULL);
    if (Status == STATUS_PENDING)
    {
        Status = ZwWaitForSingleObject(EventHandle, FALSE, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = IoStatus.Status;
    }
    ok_eq_hex(Status, STATUS_SUCCESS);

    /* Now we see a SharedCacheMap for the file */
    ok_eq_pointer(SystemRootObject->SectionObjectPointer, NULL);
    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        ok(FileObject[i]->SectionObjectPointer != NULL, "FileObject[%lu]->SectionObjectPointer = NULL\n", i);
        ok(FileObject[i]->SectionObjectPointer == FileObject[0]->SectionObjectPointer,
           "FileObject[%lu]->SectionObjectPointer = %p, expected %p\n",
           i, FileObject[i]->SectionObjectPointer, FileObject[0]->SectionObjectPointer);
    }
    if (!skip(FileObject[0]->SectionObjectPointer != NULL, "No section object pointers\n"))
        ok(FileObject[0]->SectionObjectPointer->SharedCacheMap != NULL, "SharedCacheMap is NULL\n");

Cleanup:
    if (SystemRootObject)
        ObDereferenceObject(SystemRootObject);
    if (EventHandle)
        ObCloseHandle(EventHandle, KernelMode);
    for (i = 0; i < RTL_NUMBER_OF(Tests); i++)
    {
        if (FileObject[i])
            ObDereferenceObject(FileObject[i]);
        if (FileHandle[i])
            ObCloseHandle(FileHandle[i], KernelMode);
        if (ParentHandle[i])
            ObCloseHandle(ParentHandle[i], KernelMode);
    }
}

START_TEST(IoFilesystem)
{
    TestAllInformation();
    TestRelativeNames();
    TestSharedCacheMap();
}
