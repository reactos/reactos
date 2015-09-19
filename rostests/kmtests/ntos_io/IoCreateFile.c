/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Io Regressions KM-Test (IoCreateFile)
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

static UNICODE_STRING SystemRoot = RTL_CONSTANT_STRING(L"\\SystemRoot\\");
static UNICODE_STRING Regedit = RTL_CONSTANT_STRING(L"regedit.exe");
static UNICODE_STRING Foobar = RTL_CONSTANT_STRING(L"foobar.exe");
static UNICODE_STRING SystemRootRegedit = RTL_CONSTANT_STRING(L"\\SystemRoot\\regedit.exe");
static UNICODE_STRING SystemRootFoobar = RTL_CONSTANT_STRING(L"\\SystemRoot\\foobar.exe");
static UNICODE_STRING SystemRootFoobarFoobar = RTL_CONSTANT_STRING(L"\\SystemRoot\\foobar\\foobar.exe");
static UNICODE_STRING FoobarFoobar = RTL_CONSTANT_STRING(L"foobar\\foobar.exe");

static
VOID
NTAPI
KernelModeTest(IN PVOID Context)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParentHandle, SystemRootHandle, TargetHandle;
    PFILE_OBJECT ParentFileObject, TargetFileObject, SystemRootFileObject;

    UNREFERENCED_PARAMETER(Context);

    /* Kernelmode mandatory for IoCreateFile */
    ok(ExGetPreviousMode() == KernelMode, "UserMode returned!\n");

    /* First of all, open \\SystemRoot
     * We're interested in 3 pieces of information about it:
     * -> Its target (it's a symlink): \Windows or \ReactOS
     * -> Its associated File Object
     * -> Its associated FCB
     */
    TargetFileObject = NULL;
    IoStatusBlock.Status = 0xFFFFFFFF;
    TargetHandle = INVALID_HANDLE_VALUE;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenFile(&TargetHandle,
                        GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        Status = ObReferenceObjectByHandle(TargetHandle,
                                           FILE_READ_DATA,
                                           *IoFileObjectType,
                                           KernelMode,
                                           (PVOID *)&TargetFileObject,
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
    }

    ok(TargetFileObject != NULL, "Not target to continue!\n");
    if (TargetFileObject == NULL)
    {
        if (TargetHandle != INVALID_HANDLE_VALUE)
        {
            ObCloseHandle(TargetHandle, KernelMode);
        }
        return;
    }

    /* Open target directory of \SystemRoot\Regedit.exe
     * This must lead to \SystemRoot opening
     */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRootRegedit,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = IoCreateFile(&ParentHandle,
                          GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_OPEN_TARGET_DIRECTORY);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        Status = ObReferenceObjectByHandle(ParentHandle,
                                           FILE_READ_DATA,
                                           *IoFileObjectType,
                                           KernelMode,
                                           (PVOID *)&ParentFileObject,
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (Status == STATUS_SUCCESS)
        {
            /* At that point, file object must point to \SystemRoot
             * But must not be the same FO than target (diverted file object)
             * This means FCB & FileName are equal
             * But CCB & FO are different
             * CCB must be != NULL, otherwise it means open failed
             */
            ok(ParentFileObject != TargetFileObject, "Diverted file object must be different\n");
            ok_eq_pointer(ParentFileObject->RelatedFileObject, NULL);
            ok_eq_pointer(ParentFileObject->FsContext, TargetFileObject->FsContext);
            ok(ParentFileObject->FsContext2 != 0x0, "Parent must be open!\n");
            ok(ParentFileObject->FsContext2 != TargetFileObject->FsContext2, "Parent open must have its own context!\n");
            ok_eq_long(RtlCompareUnicodeString(&ParentFileObject->FileName, &TargetFileObject->FileName, FALSE), 0);
            ObDereferenceObject(ParentFileObject);
        }
        /* Because target exists FSD must signal it */
        ok_eq_long(IoStatusBlock.Information, FILE_EXISTS);
        ObCloseHandle(ParentHandle, KernelMode);
    }

    /* Do the same with relative open */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenFile(&SystemRootHandle,
                        GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        IoStatusBlock.Status = 0xFFFFFFFF;
        IoStatusBlock.Information = 0xFFFFFFFF;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &Regedit,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   SystemRootHandle,
                                   NULL);
        Status = IoCreateFile(&ParentHandle,
                              GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN,
                              FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0,
                              CreateFileTypeNone,
                              NULL,
                              IO_OPEN_TARGET_DIRECTORY);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
        if (Status == STATUS_SUCCESS)
        {
            Status = ObReferenceObjectByHandle(ParentHandle,
                                               FILE_READ_DATA,
                                               *IoFileObjectType,
                                               KernelMode,
                                               (PVOID *)&ParentFileObject,
                                               NULL);
            ok_eq_hex(Status, STATUS_SUCCESS);
            if (Status == STATUS_SUCCESS)
            {
                ok(ParentFileObject != TargetFileObject, "Diverted file object must be different\n");
                ok_eq_pointer(ParentFileObject->FsContext, TargetFileObject->FsContext);
                ok(ParentFileObject->FsContext2 != 0x0, "Parent must be open!\n");
                ok(ParentFileObject->FsContext2 != TargetFileObject->FsContext2, "Parent open must have its own context!\n");
                ok_eq_long(RtlCompareUnicodeString(&ParentFileObject->FileName, &TargetFileObject->FileName, FALSE), 0);
                Status = ObReferenceObjectByHandle(SystemRootHandle,
                                                   FILE_READ_DATA,
                                                   *IoFileObjectType,
                                                   KernelMode,
                                                   (PVOID *)&SystemRootFileObject,
                                                   NULL);
                ok_eq_hex(Status, STATUS_SUCCESS);
                if (Status == STATUS_SUCCESS)
                {
                    ok_eq_pointer(ParentFileObject->RelatedFileObject, SystemRootFileObject);
                    ok(ParentFileObject->RelatedFileObject != TargetFileObject, "File objects must be different\n");
                    ok(SystemRootFileObject != TargetFileObject, "File objects must be different\n");
                    ObDereferenceObject(SystemRootFileObject);
                }
                ObDereferenceObject(ParentFileObject);
            }
            ok_eq_long(IoStatusBlock.Information, FILE_EXISTS);
            ObCloseHandle(ParentHandle, KernelMode);
        }
        ObCloseHandle(SystemRootHandle, KernelMode);
    }

    /* *** */

    /* Now redo the same scheme, but using a target that doesn't exist
     * The difference will be in IoStatusBlock.Information, the FSD will
     * inform that the target doesn't exist.
     * Clear for rename :-)
     */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRootFoobar,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = IoCreateFile(&ParentHandle,
                          GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_OPEN_TARGET_DIRECTORY);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        Status = ObReferenceObjectByHandle(ParentHandle,
                                           FILE_READ_DATA,
                                           *IoFileObjectType,
                                           KernelMode,
                                           (PVOID *)&ParentFileObject,
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (Status == STATUS_SUCCESS)
        {
            ok(ParentFileObject != TargetFileObject, "Diverted file object must be different\n");
            ok_eq_pointer(ParentFileObject->RelatedFileObject, NULL);
            ok_eq_pointer(ParentFileObject->FsContext, TargetFileObject->FsContext);
            ok(ParentFileObject->FsContext2 != 0x0, "Parent must be open!\n");
            ok(ParentFileObject->FsContext2 != TargetFileObject->FsContext2, "Parent open must have its own context!\n");
            ok_eq_long(RtlCompareUnicodeString(&ParentFileObject->FileName, &TargetFileObject->FileName, FALSE), 0);
            ObDereferenceObject(ParentFileObject);
        }
        ok_eq_long(IoStatusBlock.Information, FILE_DOES_NOT_EXIST);
        ObCloseHandle(ParentHandle, KernelMode);
    }

    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenFile(&SystemRootHandle,
                        GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        IoStatusBlock.Status = 0xFFFFFFFF;
        IoStatusBlock.Information = 0xFFFFFFFF;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &Foobar,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   SystemRootHandle,
                                   NULL);
        Status = IoCreateFile(&ParentHandle,
                              GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN,
                              FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0,
                              CreateFileTypeNone,
                              NULL,
                              IO_OPEN_TARGET_DIRECTORY);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
        if (Status == STATUS_SUCCESS)
        {
            Status = ObReferenceObjectByHandle(ParentHandle,
                                               FILE_READ_DATA,
                                               *IoFileObjectType,
                                               KernelMode,
                                               (PVOID *)&ParentFileObject,
                                               NULL);
            ok_eq_hex(Status, STATUS_SUCCESS);
            if (Status == STATUS_SUCCESS)
            {
                ok(ParentFileObject != TargetFileObject, "Diverted file object must be different\n");
                ok_eq_pointer(ParentFileObject->FsContext, TargetFileObject->FsContext);
                ok(ParentFileObject->FsContext2 != 0x0, "Parent must be open!\n");
                ok(ParentFileObject->FsContext2 != TargetFileObject->FsContext2, "Parent open must have its own context!\n");
                ok_eq_long(RtlCompareUnicodeString(&ParentFileObject->FileName, &TargetFileObject->FileName, FALSE), 0);
                Status = ObReferenceObjectByHandle(SystemRootHandle,
                                                   FILE_READ_DATA,
                                                   *IoFileObjectType,
                                                   KernelMode,
                                                   (PVOID *)&SystemRootFileObject,
                                                   NULL);
                ok_eq_hex(Status, STATUS_SUCCESS);
                if (Status == STATUS_SUCCESS)
                {
                    ok_eq_pointer(ParentFileObject->RelatedFileObject, SystemRootFileObject);
                    ok(ParentFileObject->RelatedFileObject != TargetFileObject, "File objects must be different\n");
                    ok(SystemRootFileObject != TargetFileObject, "File objects must be different\n");
                    ObDereferenceObject(SystemRootFileObject);
                }
                ObDereferenceObject(ParentFileObject);
            }
            ok_eq_long(IoStatusBlock.Information, FILE_DOES_NOT_EXIST);
            ObCloseHandle(ParentHandle, KernelMode);
        }
        ObCloseHandle(SystemRootHandle, KernelMode);
    }

    ObDereferenceObject(TargetFileObject);
    ObCloseHandle(TargetHandle, KernelMode);

    /* *** */

    /* Direct target open of something that doesn't exist */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRootFoobarFoobar,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = IoCreateFile(&ParentHandle,
                          GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_OPEN_TARGET_DIRECTORY);
    ok_eq_hex(Status, STATUS_OBJECT_PATH_NOT_FOUND);
    ok_eq_hex(IoStatusBlock.Status, 0xFFFFFFFF);
    if (Status == STATUS_SUCCESS)
    {
        ObCloseHandle(ParentHandle, KernelMode);
    }

    /* Relative target open of something that doesn't exist */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenFile(&SystemRootHandle,
                        GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        IoStatusBlock.Status = 0xFFFFFFFF;
        IoStatusBlock.Information = 0xFFFFFFFF;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &FoobarFoobar,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   SystemRootHandle,
                                   NULL);
        Status = IoCreateFile(&ParentHandle,
                              GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN,
                              FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0,
                              CreateFileTypeNone,
                              NULL,
                              IO_OPEN_TARGET_DIRECTORY);
        ok_eq_hex(Status, STATUS_OBJECT_PATH_NOT_FOUND);
        ok_eq_hex(IoStatusBlock.Status, 0xFFFFFFFF);
        if (Status == STATUS_SUCCESS)
        {
            ObCloseHandle(ParentHandle, KernelMode);
        }
        ObCloseHandle(SystemRootHandle, KernelMode);
    }
}

static
VOID
NTAPI
UserModeTest(VOID)
{
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ParentHandle, SystemRootHandle;

    ok(ExGetPreviousMode() == UserMode, "KernelMode returned!\n");

    /* Attempt direct target open */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRootRegedit,
                               OBJ_CASE_INSENSITIVE,
                               NULL, NULL);
    Status = IoCreateFile(&ParentHandle,
                          GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          0,
                          FILE_SHARE_READ | FILE_SHARE_WRITE,
                          FILE_OPEN,
                          FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_OPEN_TARGET_DIRECTORY);
    ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
    ok_eq_hex(IoStatusBlock.Status, 0xFFFFFFFF);
    if (Status == STATUS_SUCCESS)
    {
        ObCloseHandle(ParentHandle, UserMode);
    }

    /* Attempt relative target open */
    IoStatusBlock.Status = 0xFFFFFFFF;
    IoStatusBlock.Information = 0xFFFFFFFF;
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL, NULL);
    Status = ZwOpenFile(&SystemRootHandle,
                        GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        IoStatusBlock.Status = 0xFFFFFFFF;
        IoStatusBlock.Information = 0xFFFFFFFF;
        InitializeObjectAttributes(&ObjectAttributes,
                                   &Regedit,
                                   OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                   SystemRootHandle,
                                   NULL);
        Status = IoCreateFile(&ParentHandle,
                              GENERIC_WRITE | GENERIC_READ | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              0,
                              FILE_SHARE_READ | FILE_SHARE_WRITE,
                              FILE_OPEN,
                              FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                              NULL,
                              0,
                              CreateFileTypeNone,
                              NULL,
                              IO_OPEN_TARGET_DIRECTORY);
        ok_eq_hex(Status, STATUS_ACCESS_VIOLATION);
        ok_eq_hex(IoStatusBlock.Status, 0xFFFFFFFF);
        if (Status == STATUS_SUCCESS)
        {
            ObCloseHandle(ParentHandle, KernelMode);
        }
        ObCloseHandle(SystemRootHandle, KernelMode);
    }
}

START_TEST(IoCreateFile)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE ThreadHandle;
    PVOID ThreadObject = NULL;

    /* Justify the next comment/statement */
    UserModeTest();

    /* We've to be in kernel mode, so spawn a thread */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = PsCreateSystemThread(&ThreadHandle,
                                  SYNCHRONIZE,
                                  &ObjectAttributes,
                                  NULL,
                                  NULL,
                                  KernelModeTest,
                                  NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (Status == STATUS_SUCCESS)
    {
        /* Then, just wait on our thread to finish */
        Status = ObReferenceObjectByHandle(ThreadHandle,
                                           SYNCHRONIZE,
                                           *PsThreadType,
                                           KernelMode,
                                           &ThreadObject,
                                           NULL);
        ObCloseHandle(ThreadHandle, KernelMode);

        Status = KeWaitForSingleObject(ThreadObject,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ObDereferenceObject(ThreadObject);
    }
}
