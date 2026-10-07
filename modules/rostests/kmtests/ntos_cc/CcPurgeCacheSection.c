/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel-Mode Test Suite for purging a data section without a cache map
 * COPYRIGHT:   Copyright 2026 Tomas Srnka <tomas.srnka@e2b.dev>
 */

#include <kmt_test.h>

#define TEST_FILE_SIZE (3 * 1024 * 1024)
#define HIGH_OFFSET (2 * 1024 * 1024)
#define VIEW_SIZE (64 * 1024)

static
NTSTATUS
MapAndTouch(
    _In_ HANDLE SectionHandle,
    _In_ LONGLONG Offset,
    _In_ BOOLEAN Write,
    _Out_ PVOID *BaseAddress)
{
    NTSTATUS Status;
    LARGE_INTEGER SectionOffset;
    SIZE_T ViewSize = VIEW_SIZE;
    volatile UCHAR *Byte;

    *BaseAddress = NULL;
    SectionOffset.QuadPart = Offset;
    Status = ZwMapViewOfSection(SectionHandle,
                                NtCurrentProcess(),
                                BaseAddress,
                                0,
                                0,
                                &SectionOffset,
                                &ViewSize,
                                ViewUnmap,
                                0,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
        return Status;

    Byte = *BaseAddress;
    _SEH2_TRY
    {
        if (Write)
            *Byte = 'X';
        else
            (VOID)*Byte;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

START_TEST(CcPurgeCacheSection)
{
    NTSTATUS Status;
    HANDLE FileHandle, SectionHandle = NULL;
    UNICODE_STRING FileName = RTL_CONSTANT_STRING(L"\\SystemRoot\\kmtest-CcPurgeCacheSection.bin");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatus;
    FILE_END_OF_FILE_INFORMATION EndOfFileInfo;
    FILE_DISPOSITION_INFORMATION DispositionInfo;
    PFILE_OBJECT FileObject = NULL;
    PVOID HighView, LowView;
    BOOLEAN Purged;

    InitializeObjectAttributes(&ObjectAttributes,
                               &FileName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwCreateFile(&FileHandle,
                          GENERIC_READ | GENERIC_WRITE | DELETE | SYNCHRONIZE,
                          &ObjectAttributes,
                          &IoStatus,
                          NULL,
                          FILE_ATTRIBUTE_NORMAL,
                          0,
                          FILE_SUPERSEDE,
                          FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                          NULL,
                          0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    if (skip(NT_SUCCESS(Status), "No test file\n"))
        return;

    EndOfFileInfo.EndOfFile.QuadPart = TEST_FILE_SIZE;
    Status = ZwSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &EndOfFileInfo,
                                  sizeof(EndOfFileInfo),
                                  FileEndOfFileInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);

    InitializeObjectAttributes(&ObjectAttributes, NULL, OBJ_KERNEL_HANDLE, NULL, NULL);
    Status = ZwCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             &ObjectAttributes,
                             NULL,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             FileHandle);
    ok_eq_hex(Status, STATUS_SUCCESS);

    Status = ObReferenceObjectByHandle(FileHandle,
                                       0,
                                       *IoFileObjectType,
                                       KernelMode,
                                       (PVOID*)&FileObject,
                                       NULL);
    ok_eq_hex(Status, STATUS_SUCCESS);

    if (!skip(FileObject != NULL && SectionHandle != NULL, "No section\n") &&
        !skip(FileObject->SectionObjectPointer->SharedCacheMap == NULL, "The file is cached\n"))
    {
        /* Make the high page first, so that it is not in the page table created last */
        Status = MapAndTouch(SectionHandle, HIGH_OFFSET, TRUE, &HighView);
        ok_eq_hex(Status, STATUS_SUCCESS);
        Status = MapAndTouch(SectionHandle, 0, TRUE, &LowView);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (HighView) ZwUnmapViewOfSection(NtCurrentProcess(), HighView);
        if (LowView) ZwUnmapViewOfSection(NtCurrentProcess(), LowView);

        /* A purge of the whole section must see the mapped page at the high
         * offset, above the page table that was created last */
        Status = MapAndTouch(SectionHandle, HIGH_OFFSET, FALSE, &HighView);
        ok_eq_hex(Status, STATUS_SUCCESS);
        FsRtlAcquireFileExclusive(FileObject);
        Purged = CcPurgeCacheSection(FileObject->SectionObjectPointer, NULL, 0, FALSE);
        FsRtlReleaseFile(FileObject);
        ok_bool_false(Purged, "CcPurgeCacheSection with a mapped page returned");
        if (HighView) ZwUnmapViewOfSection(NtCurrentProcess(), HighView);

        FsRtlAcquireFileExclusive(FileObject);
        Purged = CcPurgeCacheSection(FileObject->SectionObjectPointer, NULL, 0, FALSE);
        FsRtlReleaseFile(FileObject);
        ok_bool_true(Purged, "CcPurgeCacheSection without mapped pages returned");
    }

    if (FileObject)
        ObDereferenceObject(FileObject);
    if (SectionHandle)
        ZwClose(SectionHandle);

    DispositionInfo.DeleteFile = TRUE;
    Status = ZwSetInformationFile(FileHandle,
                                  &IoStatus,
                                  &DispositionInfo,
                                  sizeof(DispositionInfo),
                                  FileDispositionInformation);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ZwClose(FileHandle);
}
