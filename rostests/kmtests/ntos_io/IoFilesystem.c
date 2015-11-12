/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite File System test
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

/* FIXME: Test this stuff on non-FAT volumes */

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
    } Tests[] =
    {
        { NULL,                         L"C:\\",                            TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\\\",                          TRUE,   STATUS_SUCCESS },
        { NULL,                         L"C:\\\\\\",                        TRUE,   STATUS_OBJECT_NAME_INVALID },
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
        /* Let's try some nonexistent things */
        { NULL,                         L"C:\\ReactOS\\IDoNotExist",        FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file",  FALSE,  STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file?", FALSE,  STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file\\\\",TRUE,STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\IDoNotExist\\file\\\\\\",TRUE,STATUS_OBJECT_PATH_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\AmIInvalid?",        FALSE,  STATUS_OBJECT_NAME_INVALID },
        { NULL,                         L"C:\\ReactOS\\.",                  FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\..",                 FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { NULL,                         L"C:\\ReactOS\\...",                FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\",                      L".",                               FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\",                      L"..",                              FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\",                      L"...",                             FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\ReactOS",               L".",                               FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\ReactOS",               L"..",                              FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
        { L"C:\\ReactOS",               L"...",                             FALSE,  STATUS_OBJECT_NAME_NOT_FOUND },
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
    TestRelativeNames();
    TestSharedCacheMap();
}
