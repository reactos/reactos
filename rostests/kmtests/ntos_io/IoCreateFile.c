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
TestSymlinks(VOID)
{
    HANDLE ReparseHandle;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatusBlock;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PREPARSE_DATA_BUFFER Reparse;
    FILE_DISPOSITION_INFORMATION ToDelete;
    PFILE_OBJECT FileObject;
    UNICODE_STRING SysDir, Foobar, Regedit;
    ULONG Size;

    /* Get Windows/ReactOS directory */
    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRoot,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&ReparseHandle,
                        FILE_READ_DATA,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                        FILE_DIRECTORY_FILE);
    if (skip(NT_SUCCESS(Status), "Opening \\SystemRoot failed: %lx\n", Status))
    {
        return;
    }

    Status = ObReferenceObjectByHandle(ReparseHandle,
                                       FILE_READ_DATA,
                                       *IoFileObjectType,
                                       UserMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    if (skip(NT_SUCCESS(Status), "Querying name failed: %lx\n", Status))
    {
        ZwClose(ReparseHandle);
        return;
    }

    SysDir.Buffer = ExAllocatePool(NonPagedPool, FileObject->FileName.Length + sizeof(L"\\??\\C:"));
    if (skip(SysDir.Buffer != NULL, "Allocating memory failed\n"))
    {
        ObDereferenceObject(FileObject);
        ZwClose(ReparseHandle);
        return;
    }

    SysDir.Length = sizeof(L"\\??\\C:") - sizeof(UNICODE_NULL);
    SysDir.MaximumLength = FileObject->FileName.Length + sizeof(L"\\??\\C:");
    RtlCopyMemory(SysDir.Buffer, L"\\??\\C:", sizeof(L"\\??\\C:") - sizeof(UNICODE_NULL));
    RtlAppendUnicodeStringToString(&SysDir, &FileObject->FileName);

    Foobar.Buffer = ExAllocatePool(NonPagedPool, FileObject->FileName.Length + sizeof(L"\\foobar.exe"));
    if (skip(Foobar.Buffer != NULL, "Allocating memory failed\n"))
    {
        ExFreePool(SysDir.Buffer);
        ObDereferenceObject(FileObject);
        ZwClose(ReparseHandle);
        return;
    }

    Foobar.Length = 0;
    Foobar.MaximumLength = FileObject->FileName.Length + sizeof(L"\\foobar.exe");
    RtlCopyUnicodeString(&Foobar, &FileObject->FileName);
    RtlCopyMemory(&Foobar.Buffer[Foobar.Length / sizeof(WCHAR)], L"\\foobar.exe", sizeof(L"\\foobar.exe") - sizeof(UNICODE_NULL));
    Foobar.Length += (sizeof(L"\\foobar.exe") - sizeof(UNICODE_NULL));

    Regedit.Buffer = ExAllocatePool(NonPagedPool, FileObject->FileName.Length + sizeof(L"\\regedit.exe"));
    if (skip(Regedit.Buffer != NULL, "Allocating memory failed\n"))
    {
        ExFreePool(Foobar.Buffer);
        ExFreePool(SysDir.Buffer);
        ObDereferenceObject(FileObject);
        ZwClose(ReparseHandle);
        return;
    }

    Regedit.Length = 0;
    Regedit.MaximumLength = FileObject->FileName.Length + sizeof(L"\\regedit.exe");
    RtlCopyUnicodeString(&Regedit, &FileObject->FileName);
    RtlCopyMemory(&Regedit.Buffer[Regedit.Length / sizeof(WCHAR)], L"\\regedit.exe", sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL));
    Regedit.Length += (sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL));

    ObDereferenceObject(FileObject);
    ZwClose(ReparseHandle);

    ToDelete.DeleteFile = TRUE;
    Size = FIELD_OFFSET(REPARSE_DATA_BUFFER, SymbolicLinkReparseBuffer.PathBuffer) + SysDir.Length * 2 + sizeof(L"\\regedit.exe") * 2 - sizeof(L"\\??\\") - sizeof(UNICODE_NULL);

    InitializeObjectAttributes(&ObjectAttributes,
                               &SystemRootFoobar,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);
    Status = ZwCreateFile(&ReparseHandle,
                          GENERIC_READ | GENERIC_WRITE | DELETE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_SUPERSEDE,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "Creating file failed: %lx\n", Status))
    {
        ExFreePool(Regedit.Buffer);
        ExFreePool(Foobar.Buffer);
        ExFreePool(SysDir.Buffer);
        return;
    }

    Reparse = ExAllocatePool(NonPagedPool, Size);
    RtlZeroMemory(Reparse, Size);
    Reparse->ReparseTag = IO_REPARSE_TAG_SYMLINK;
    Reparse->ReparseDataLength = 12 + SysDir.Length * 2 + sizeof(L"\\regedit.exe") * 2 - sizeof(L"\\??\\") - sizeof(UNICODE_NULL);
    Reparse->SymbolicLinkReparseBuffer.SubstituteNameLength = SysDir.Length + sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL);
    Reparse->SymbolicLinkReparseBuffer.PrintNameLength = SysDir.Length + sizeof(L"\\regedit.exe") - sizeof(L"\\??\\");
    Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset = Reparse->SymbolicLinkReparseBuffer.PrintNameLength;
    RtlCopyMemory(Reparse->SymbolicLinkReparseBuffer.PathBuffer,
                  (WCHAR *)((ULONG_PTR)SysDir.Buffer + sizeof(L"\\??\\") - sizeof(UNICODE_NULL)),
                  SysDir.Length - sizeof(L"\\??\\") + sizeof(UNICODE_NULL));
    RtlCopyMemory((WCHAR *)((ULONG_PTR)Reparse->SymbolicLinkReparseBuffer.PathBuffer + SysDir.Length - sizeof(L"\\??\\") + sizeof(UNICODE_NULL)),
                  L"\\regedit.exe", sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL));
    RtlCopyMemory((WCHAR *)((ULONG_PTR)Reparse->SymbolicLinkReparseBuffer.PathBuffer + Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset),
                  SysDir.Buffer, SysDir.Length);
    RtlCopyMemory((WCHAR *)((ULONG_PTR)Reparse->SymbolicLinkReparseBuffer.PathBuffer + Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset + SysDir.Length),
                  L"\\regedit.exe", sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL));

    Status = ZwFsControlFile(ReparseHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_SET_REPARSE_POINT,
                             Reparse,
                             Size,
                             NULL,
                             0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (!NT_SUCCESS(Status))
    {
        ZwClose(ReparseHandle);

        Status = ZwCreateFile(&ReparseHandle,
                              FILE_WRITE_ATTRIBUTES | DELETE | SYNCHRONIZE,
                              &ObjectAttributes,
                              &IoStatusBlock,
                              NULL,
                              FILE_ATTRIBUTE_NORMAL,
                              0,
                              FILE_SUPERSEDE,
                              FILE_NON_DIRECTORY_FILE  | FILE_OPEN_REPARSE_POINT | FILE_SYNCHRONOUS_IO_NONALERT | FILE_OPEN_FOR_BACKUP_INTENT,
                              NULL,
                              0);
        if (skip(NT_SUCCESS(Status), "Creating symlink failed: %lx\n", Status))
        {
            Status = ZwOpenFile(&ReparseHandle,
                                DELETE,
                                &ObjectAttributes,
                                &IoStatusBlock,
                                FILE_SHARE_DELETE,
                                FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE);
            ok_eq_hex(Status, STATUS_SUCCESS);
            ZwClose(ReparseHandle);
            ExFreePool(Regedit.Buffer);
            ExFreePool(Foobar.Buffer);
            ExFreePool(SysDir.Buffer);
            ExFreePool(Reparse);
            return;
        }

        Status = ZwFsControlFile(ReparseHandle,
                                 NULL,
                                 NULL,
                                 NULL,
                                 &IoStatusBlock,
                                 FSCTL_SET_REPARSE_POINT,
                                 Reparse,
                                 Size,
                                 NULL,
                                 0);
    }

    if (skip(NT_SUCCESS(Status), "Creating symlink failed: %lx\n", Status))
    {
        ZwSetInformationFile(ReparseHandle,
                             &IoStatusBlock,
                             &ToDelete,
                             sizeof(ToDelete),
                             FileDispositionInformation);
        ZwClose(ReparseHandle);
        ExFreePool(Regedit.Buffer);
        ExFreePool(Foobar.Buffer);
        ExFreePool(SysDir.Buffer);
        ExFreePool(Reparse);
        return;
    }

    ZwClose(ReparseHandle);

    Status = ZwCreateFile(&ReparseHandle,
                          GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0);
    ok(Status == STATUS_SUCCESS || /* Windows Vista+ */
       Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED, /* Windows 2003 (SP1, SP2) */
        "ZwCreateFile returned unexpected status: %lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        Status = ObReferenceObjectByHandle(ReparseHandle,
                                           FILE_READ_DATA,
                                           *IoFileObjectType,
                                           UserMode,
                                           (PVOID *)&FileObject,
                                           NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (NT_SUCCESS(Status))
        {
            ok(RtlCompareUnicodeString(&Regedit, &FileObject->FileName, TRUE) == 0,
               "Expected: %wZ. Opened: %wZ\n", &Regedit, &FileObject->FileName);
            ObDereferenceObject(FileObject);
        }

        ZwClose(ReparseHandle);
    }

    ExFreePool(Regedit.Buffer);

    Status = IoCreateFile(&ReparseHandle,
                          GENERIC_READ,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE,
                          NULL,
                          0,
                          CreateFileTypeNone,
                          NULL,
                          IO_NO_PARAMETER_CHECKING | IO_STOP_ON_SYMLINK);
    ok(Status == STATUS_STOPPED_ON_SYMLINK || /* Windows Vista+ */
       Status == STATUS_IO_REPARSE_TAG_NOT_HANDLED, /* Windows 2003 (SP1, SP2) */
        "ZwCreateFile returned unexpected status: %lx\n", Status);
    if (NT_SUCCESS(Status))
    {
        ZwClose(ReparseHandle);
    }

    Status = ZwCreateFile(&ReparseHandle,
                          GENERIC_READ | GENERIC_WRITE | DELETE,
                          &ObjectAttributes,
                          &IoStatusBlock,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                          FILE_OPEN,
                          FILE_NON_DIRECTORY_FILE | FILE_OPEN_REPARSE_POINT | FILE_OPEN_FOR_BACKUP_INTENT,
                          NULL,
                          0);
    if (skip(NT_SUCCESS(Status), "Creating opening reparse point: %lx\n", Status))
    {
        Status = ZwOpenFile(&ReparseHandle,
                            DELETE,
                            &ObjectAttributes,
                            &IoStatusBlock,
                            FILE_SHARE_DELETE,
                            FILE_NON_DIRECTORY_FILE | FILE_DELETE_ON_CLOSE);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ZwClose(ReparseHandle);
        ExFreePool(Foobar.Buffer);
        ExFreePool(SysDir.Buffer);
        ExFreePool(Reparse);
        return;
    }

    Status = ObReferenceObjectByHandle(ReparseHandle,
                                       FILE_READ_DATA,
                                       *IoFileObjectType,
                                       UserMode,
                                       (PVOID *)&FileObject,
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (NT_SUCCESS(Status))
    {
        ok(RtlCompareUnicodeString(&Foobar, &FileObject->FileName, TRUE) == 0,
           "Expected: %wZ. Opened: %wZ\n", &Foobar, &FileObject->FileName);
        ObDereferenceObject(FileObject);
    }

    ExFreePool(Foobar.Buffer);

    RtlZeroMemory(Reparse, Size);
    Status = ZwFsControlFile(ReparseHandle,
                             NULL,
                             NULL,
                             NULL,
                             &IoStatusBlock,
                             FSCTL_GET_REPARSE_POINT,
                             NULL,
                             0,
                             Reparse,
                             Size);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(IoStatusBlock.Information, Size);
    if (NT_SUCCESS(Status))
    {
        PWSTR Buffer;
        UNICODE_STRING ReparsePath, FullPath;

        ok_eq_hex(Reparse->ReparseTag, IO_REPARSE_TAG_SYMLINK);
        ok_eq_hex(Reparse->ReparseDataLength, 12 + SysDir.Length * 2 + sizeof(L"\\regedit.exe") * 2 - sizeof(L"\\??\\") - sizeof(UNICODE_NULL));
        ok_eq_hex(Reparse->SymbolicLinkReparseBuffer.Flags, 0);

        FullPath.Length = 0;
        FullPath.MaximumLength = SysDir.Length + sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL);
        Buffer = FullPath.Buffer = ExAllocatePool(NonPagedPool, FullPath.MaximumLength);
        if (!skip(Buffer != NULL, "Memory allocation failed!\n"))
        {
            RtlCopyUnicodeString(&FullPath, &SysDir);
            RtlCopyMemory(&FullPath.Buffer[FullPath.Length / sizeof(WCHAR)], L"\\regedit.exe", sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL));
            FullPath.Length += (sizeof(L"\\regedit.exe") - sizeof(UNICODE_NULL));
            ReparsePath.Buffer = (PWSTR)((ULONG_PTR)Reparse->SymbolicLinkReparseBuffer.PathBuffer + Reparse->SymbolicLinkReparseBuffer.SubstituteNameOffset);
            ReparsePath.Length = ReparsePath.MaximumLength = Reparse->SymbolicLinkReparseBuffer.SubstituteNameLength;
            ok(RtlCompareUnicodeString(&ReparsePath, &FullPath, TRUE) == 0, "Expected: %wZ. Got: %wZ\n", &ReparsePath, &FullPath);

            FullPath.Length -= (sizeof(L"\\??\\") - sizeof(UNICODE_NULL));
            FullPath.MaximumLength -= (sizeof(L"\\??\\") - sizeof(UNICODE_NULL));
            FullPath.Buffer = (PWSTR)((ULONG_PTR)Buffer + sizeof(L"\\??\\") - sizeof(UNICODE_NULL));
            ReparsePath.Buffer = (PWSTR)((ULONG_PTR)Reparse->SymbolicLinkReparseBuffer.PathBuffer + Reparse->SymbolicLinkReparseBuffer.PrintNameOffset);
            ReparsePath.Length = ReparsePath.MaximumLength = Reparse->SymbolicLinkReparseBuffer.PrintNameLength;
            ok(RtlCompareUnicodeString(&ReparsePath, &FullPath, TRUE) == 0, "Expected: %wZ. Got: %wZ\n", &ReparsePath, &FullPath);

            ExFreePool(Buffer);
        }
    }

    ExFreePool(SysDir.Buffer);
    ExFreePool(Reparse);

    ZwSetInformationFile(ReparseHandle,
                         &IoStatusBlock,
                         &ToDelete,
                         sizeof(ToDelete),
                         FileDispositionInformation);
    ZwClose(ReparseHandle);
}

//static
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

    TestSymlinks();

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
