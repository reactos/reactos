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
    TestSharedCacheMap();
}
