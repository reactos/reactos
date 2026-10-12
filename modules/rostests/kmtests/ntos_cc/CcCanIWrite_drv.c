/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test driver for CcCanIWrite per-file dirty page accounting
 * COPYRIGHT:   Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include <kmt_test.h>

#define TEST_FILE_PAGES     16
#define DIRTY_THRESHOLD     8
#define STRESS_FILE_PAGES   (2 * VACB_MAPPING_GRANULARITY / PAGE_SIZE)
#define STRESS_THREADS      4
#define STRESS_ITERATIONS   2000

typedef struct _TEST_FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    FAST_MUTEX HeaderMutex;
} TEST_FCB, *PTEST_FCB;

typedef struct _STRESS_CONTEXT
{
    PFILE_OBJECT FileObject;
    ULONG Id;
    ULONG Seed;
    ULONG Failures;
    ULONG Exceptions;
} STRESS_CONTEXT, *PSTRESS_CONTEXT;

static const UNICODE_STRING TestFileName = RTL_CONSTANT_STRING(L"\\TestFile");
static const UNICODE_STRING StressFileName = RTL_CONSTANT_STRING(L"\\StressFile");

static KMT_IRP_HANDLER TestIrpHandler;
static FAST_IO_DISPATCH TestFastIoDispatch;
static BOOLEAN LazyWriteAllowed = TRUE;

static
BOOLEAN
NTAPI
FastIoRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    IoStatus->Status = STATUS_NOT_SUPPORTED;
    return FALSE;
}

static
BOOLEAN
NTAPI
FastIoWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    IoStatus->Status = STATUS_NOT_SUPPORTED;
    return FALSE;
}

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    *DeviceName = L"CcCanIWrite";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_CLEANUP, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_WRITE, NULL, TestIrpHandler);

    TestFastIoDispatch.FastIoRead = FastIoRead;
    TestFastIoDispatch.FastIoWrite = FastIoWrite;
    DriverObject->FastIoDispatch = &TestFastIoDispatch;

    return STATUS_SUCCESS;
}

VOID
TestUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
}

static
BOOLEAN
NTAPI
AcquireForLazyWrite(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    return LazyWriteAllowed;
}

static
VOID
NTAPI
ReleaseFromLazyWrite(
    _In_ PVOID Context)
{
    return;
}

static
BOOLEAN
NTAPI
AcquireForReadAhead(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    return TRUE;
}

static
VOID
NTAPI
ReleaseFromReadAhead(
    _In_ PVOID Context)
{
    return;
}

static CACHE_MANAGER_CALLBACKS Callbacks = {
    AcquireForLazyWrite,
    ReleaseFromLazyWrite,
    AcquireForReadAhead,
    ReleaseFromReadAhead,
};

static
PVOID
MapAndLockUserBuffer(
    _Inout_ PIRP Irp,
    _In_ ULONG BufferLength)
{
    PMDL Mdl;

    if (Irp->MdlAddress == NULL)
    {
        Mdl = IoAllocateMdl(Irp->UserBuffer, BufferLength, FALSE, FALSE, Irp);
        if (Mdl == NULL)
        {
            return NULL;
        }

        _SEH2_TRY
        {
            MmProbeAndLockPages(Mdl, Irp->RequestorMode, IoWriteAccess);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            IoFreeMdl(Mdl);
            Irp->MdlAddress = NULL;
            _SEH2_YIELD(return NULL);
        }
        _SEH2_END;
    }

    return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
}

static
VOID
DirtyRange(
    _In_ PFILE_OBJECT FileObject,
    _In_ LONGLONG Offset,
    _In_ ULONG Length)
{
    BOOLEAN Ret;
    LARGE_INTEGER FileOffset;
    CHAR Bytes[2] = { (CHAR)0xAC, (CHAR)0xAC };

    ASSERT(Length <= sizeof(Bytes));

    FileOffset.QuadPart = Offset;
    Ret = CcCopyWrite(FileObject, &FileOffset, Length, TRUE, Bytes);
    ok_bool_true(Ret, "CcCopyWrite");
}

#define ok_dirty_pages(_Count) do {                                                                 \
    Ret = CcCanIWrite(FileObject, (DIRTY_THRESHOLD - (_Count)) * PAGE_SIZE, FALSE, FALSE);          \
    ok_bool_true(Ret, "Write fitting in the remaining threshold");                                  \
    Ret = CcCanIWrite(FileObject, (DIRTY_THRESHOLD - (_Count) + 1) * PAGE_SIZE, FALSE, FALSE);      \
    ok_bool_false(Ret, "Write exceeding the threshold");                                            \
} while (0)

static
VOID
Test_CcCanIWrite(
    _In_ PFILE_OBJECT FileObject)
{
    BOOLEAN Ret;
    ULONG i;
    IO_STATUS_BLOCK IoStatus;

    LazyWriteAllowed = FALSE;
    CcSetDirtyPageThreshold(FileObject, DIRTY_THRESHOLD);

    /* Nothing is dirty, the per-file limit is not checked */
    Ret = CcCanIWrite(FileObject, DIRTY_THRESHOLD * PAGE_SIZE, FALSE, FALSE);
    ok_bool_true(Ret, "Write with no dirty page");

    /* One byte dirties one page, not one view */
    DirtyRange(FileObject, 0, 1);
    ok_dirty_pages(1);

    /* Dirtying the same page again changes nothing */
    DirtyRange(FileObject, 0, 1);
    ok_dirty_pages(1);

    /* A write straddling a page boundary dirties both pages */
    DirtyRange(FileObject, 2 * PAGE_SIZE - 1, 2);
    ok_dirty_pages(3);

    for (i = 3; i < DIRTY_THRESHOLD - 1; i++)
    {
        DirtyRange(FileObject, i * PAGE_SIZE, 1);
        ok_dirty_pages(i + 1);
    }

    /* At the threshold, any write is deferred */
    DirtyRange(FileObject, (DIRTY_THRESHOLD - 1) * PAGE_SIZE, 1);
    Ret = CcCanIWrite(FileObject, 1, FALSE, FALSE);
    ok_bool_false(Ret, "Write at the threshold");

    /* Flushing clears the count */
    CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &IoStatus);
    ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
    Ret = CcCanIWrite(FileObject, DIRTY_THRESHOLD * PAGE_SIZE, FALSE, FALSE);
    ok_bool_true(Ret, "Write after flush");

    /* A page dirtied before the flush counts again */
    DirtyRange(FileObject, 0, 1);
    ok_dirty_pages(1);

    LazyWriteAllowed = TRUE;
}

static
ULONG
StressRandom(
    _Inout_ PULONG Seed)
{
    *Seed = *Seed * 1103515245UL + 12345UL;
    return (*Seed >> 16) & 0x7FFF;
}

static
VOID
NTAPI
StressWorker(
    _In_ PVOID Context)
{
    PSTRESS_CONTEXT Ctx = Context;
    ULONG i;

    for (i = 0; i < STRESS_ITERATIONS; i++)
    {
        ULONG Op = StressRandom(&Ctx->Seed) % 10;
        ULONG Page = StressRandom(&Ctx->Seed) % STRESS_FILE_PAGES;
        LARGE_INTEGER Offset;
        IO_STATUS_BLOCK IoStatus;
        CHAR Byte = (CHAR)Ctx->Id;

        Offset.QuadPart = (LONGLONG)Page * PAGE_SIZE;
        IoStatus.Status = STATUS_SUCCESS;

        _SEH2_TRY
        {
            if (Op < 6)
            {
                if (!CcCopyWrite(Ctx->FileObject, &Offset, sizeof(Byte), TRUE, &Byte))
                    Ctx->Failures++;
            }
            else if (Op < 8)
            {
                CcFlushCache(Ctx->FileObject->SectionObjectPointer, NULL, 0, &IoStatus);
                if (!NT_SUCCESS(IoStatus.Status))
                    Ctx->Failures++;
            }
            else if (Op == 8)
            {
                CcFlushCache(Ctx->FileObject->SectionObjectPointer, &Offset, PAGE_SIZE, &IoStatus);
                if (!NT_SUCCESS(IoStatus.Status))
                    Ctx->Failures++;
            }
            else
            {
                CcCanIWrite(Ctx->FileObject, (Page % DIRTY_THRESHOLD + 1) * PAGE_SIZE, FALSE, FALSE);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            Ctx->Exceptions++;
        }
        _SEH2_END;
    }

    PsTerminateSystemThread(STATUS_SUCCESS);
}

static
VOID
Test_CcDirtyStress(
    _In_ PFILE_OBJECT FileObject)
{
    STRESS_CONTEXT Contexts[STRESS_THREADS];
    HANDLE Threads[STRESS_THREADS];
    BOOLEAN Ret;
    ULONG i;
    NTSTATUS Status;
    IO_STATUS_BLOCK IoStatus;

    LazyWriteAllowed = FALSE;
    CcSetDirtyPageThreshold(FileObject, DIRTY_THRESHOLD);

    for (i = 0; i < STRESS_THREADS; i++)
    {
        Contexts[i].FileObject = FileObject;
        Contexts[i].Id = i + 1;
        Contexts[i].Seed = 0x1234 * (i + 1);
        Contexts[i].Failures = 0;
        Contexts[i].Exceptions = 0;

        Status = PsCreateSystemThread(&Threads[i],
                                      THREAD_ALL_ACCESS,
                                      NULL,
                                      NULL,
                                      NULL,
                                      StressWorker,
                                      &Contexts[i]);
        ok_eq_hex(Status, STATUS_SUCCESS);
        if (!NT_SUCCESS(Status))
            Threads[i] = NULL;
    }

    for (i = 0; i < STRESS_THREADS; i++)
    {
        if (Threads[i] == NULL)
            continue;

        Status = ZwWaitForSingleObject(Threads[i], FALSE, NULL);
        ok_eq_hex(Status, STATUS_SUCCESS);
        ZwClose(Threads[i]);

        ok_eq_ulong(Contexts[i].Failures, 0);
        ok_eq_ulong(Contexts[i].Exceptions, 0);
    }

    /* Nothing may be left counted once everything is flushed */
    CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &IoStatus);
    ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
    Ret = CcCanIWrite(FileObject, DIRTY_THRESHOLD * PAGE_SIZE, FALSE, FALSE);
    ok_bool_true(Ret, "Write after stress and flush");

    /* Every page must count exactly once, so no stale bit may remain */
    for (i = 0; i < STRESS_FILE_PAGES; i++)
    {
        DirtyRange(FileObject, (LONGLONG)i * PAGE_SIZE, 1);
        ok_dirty_pages(i % (DIRTY_THRESHOLD - 1) + 1);

        if (i % (DIRTY_THRESHOLD - 1) == DIRTY_THRESHOLD - 2)
        {
            CcFlushCache(FileObject->SectionObjectPointer, NULL, 0, &IoStatus);
            ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
        }
    }

    LazyWriteAllowed = TRUE;
}

static
NTSTATUS
TestIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    LARGE_INTEGER Zero = RTL_CONSTANT_LARGE_INTEGER(0LL);
    NTSTATUS Status;
    PTEST_FCB Fcb;
    CACHE_UNINITIALIZE_EVENT CacheUninitEvent;

    PAGED_CODE();

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        Fcb = ExAllocatePoolZero(NonPagedPool, sizeof(*Fcb), 'FciW');
        if (Fcb == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            LONGLONG FileSize = TEST_FILE_PAGES * PAGE_SIZE;

            if (RtlCompareUnicodeString(&IoStack->FileObject->FileName, &StressFileName, TRUE) == 0)
            {
                FileSize = STRESS_FILE_PAGES * PAGE_SIZE;
            }

            ExInitializeFastMutex(&Fcb->HeaderMutex);
            FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);
            Fcb->Header.AllocationSize.QuadPart = FileSize;
            Fcb->Header.FileSize.QuadPart = FileSize;
            Fcb->Header.ValidDataLength.QuadPart = FileSize;
            Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
            IoStack->FileObject->FsContext = Fcb;
            IoStack->FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

            CcInitializeCacheMap(IoStack->FileObject,
                                 (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                 FALSE, &Callbacks, NULL);

            Irp->IoStatus.Information = FILE_OPENED;
            Status = STATUS_SUCCESS;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_READ)
    {
        ULONG Length = IoStack->Parameters.Read.Length;
        PVOID Buffer = MapAndLockUserBuffer(Irp, Length);

        if (Buffer == NULL)
        {
            Status = STATUS_INSUFFICIENT_RESOURCES;
        }
        else
        {
            RtlFillMemory(Buffer, Length, 0xBA);
            Irp->IoStatus.Information = Length;
            IoStack->FileObject->CurrentByteOffset.QuadPart =
                IoStack->Parameters.Read.ByteOffset.QuadPart + Length;
            Status = STATUS_SUCCESS;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        if (!FlagOn(Irp->Flags, IRP_NOCACHE))
        {
            if (RtlCompareUnicodeString(&IoStack->FileObject->FileName, &TestFileName, TRUE) == 0)
            {
                Test_CcCanIWrite(IoStack->FileObject);
            }
            else if (RtlCompareUnicodeString(&IoStack->FileObject->FileName, &StressFileName, TRUE) == 0)
            {
                Test_CcDirtyStress(IoStack->FileObject);
            }
        }

        Irp->IoStatus.Information = IoStack->Parameters.Write.Length;
        Status = STATUS_SUCCESS;
    }
    else if (IoStack->MajorFunction == IRP_MJ_CLEANUP)
    {
        KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
        CcUninitializeCacheMap(IoStack->FileObject, &Zero, &CacheUninitEvent);
        KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        Fcb = IoStack->FileObject->FsContext;
        ExFreePoolWithTag(Fcb, 'FciW');
        IoStack->FileObject->FsContext = NULL;
        Status = STATUS_SUCCESS;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}