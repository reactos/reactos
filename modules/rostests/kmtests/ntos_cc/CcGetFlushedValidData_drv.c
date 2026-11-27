/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         MIT (https://www.spdx.org/licenses/MIT)
 * PURPOSE:         Test driver for CcGetFlushedValidData function
 * COPYRIGHT:       Copyright 2026 Alex Mendoza <05alex.mendozaa@gmail.com>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define IOCTL_START_TEST 1
#define IOCTL_FINISH_TEST 2

typedef struct _TEST_FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    FAST_MUTEX HeaderMutex;
} TEST_FCB, *PTEST_FCB;

static ULONG TestTestId = -1;
static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;
static KMT_IRP_HANDLER TestIrpHandler;
static KMT_MESSAGE_HANDLER TestMessageHandler;

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    *DeviceName = L"CcGetFlushedValidData";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_WRITE, NULL, TestIrpHandler);
    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    return STATUS_SUCCESS;
}

VOID
TestUnload(
    _In_ PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();
}

BOOLEAN
NTAPI
AcquireForLazyWrite(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    return TRUE;
}

VOID
NTAPI
ReleaseFromLazyWrite(
    _In_ PVOID Context)
{
    return;
}

BOOLEAN
NTAPI
AcquireForReadAhead(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    return TRUE;
}

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

/* Dirty the first PAGE_SIZE bytes at the given offset (via pin/write) */
static
VOID
DirtyPage(
    _In_ PLARGE_INTEGER Offset)
{
    PVOID Bcb;
    PULONG Buffer;
    BOOLEAN Ret;

    KmtStartSeh();
    Ret = CcPinRead(TestFileObject, Offset, PAGE_SIZE, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
    KmtEndSeh(STATUS_SUCCESS);

    if (skip(Ret == TRUE, "CcPinRead failed\n"))
    {
        return;
    }

    CcSetDirtyPinnedData(Bcb, NULL);
    CcUnpinData(Bcb);
}

static
VOID
PerformTest(
    ULONG TestId,
    PDEVICE_OBJECT DeviceObject)
{
    PTEST_FCB Fcb;
    LARGE_INTEGER Flushed;
    LARGE_INTEGER Offset;
    IO_STATUS_BLOCK IoStatus;
    SECTION_OBJECT_POINTERS EmptySop = { NULL, NULL, NULL };

    ok_eq_pointer(TestFileObject, NULL);
    ok_eq_pointer(TestDeviceObject, NULL);
    ok_eq_ulong(TestTestId, -1);

    TestDeviceObject = DeviceObject;
    TestTestId = TestId;

    /* A file that isn't cached should return MAXLONGLONG */
    if (TestId == 0)
    {
        Flushed = CcGetFlushedValidData(&EmptySop, FALSE);
        ok(Flushed.QuadPart == MAXLONGLONG, "Flushed: %I64d\n", Flushed.QuadPart);
        return;
    }

    TestFileObject = IoCreateStreamFileObject(NULL, DeviceObject);
    if (skip(TestFileObject != NULL, "Failed to allocate FO\n"))
    {
        return;
    }

    Fcb = ExAllocatePool(NonPagedPool, sizeof(TEST_FCB));
    if (skip(Fcb != NULL, "ExAllocatePool failed\n"))
    {
        return;
    }

    RtlZeroMemory(Fcb, sizeof(TEST_FCB));
    ExInitializeFastMutex(&Fcb->HeaderMutex);
    FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);

    TestFileObject->FsContext = Fcb;
    TestFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
    Fcb->Header.AllocationSize.QuadPart = 2 * VACB_MAPPING_GRANULARITY;
    Fcb->Header.FileSize.QuadPart = 2 * VACB_MAPPING_GRANULARITY;
    Fcb->Header.ValidDataLength.QuadPart = 2 * VACB_MAPPING_GRANULARITY;

    KmtStartSeh();
    CcInitializeCacheMap(TestFileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize, TRUE, &Callbacks, NULL);
    KmtEndSeh(STATUS_SUCCESS);

    if (skip(CcIsFileCached(TestFileObject) == TRUE, "CcInitializeCacheMap failed\n"))
    {
        return;
    }

    trace("Starting test: %d\n", TestId);

    /* Test 1: nothing is dirty yet, everything is "flushed" up to ValidDataLength */
    if (TestId == 1)
    {
        Flushed = CcGetFlushedValidData(TestFileObject->SectionObjectPointer, FALSE);
        ok(Flushed.QuadPart == 2 * VACB_MAPPING_GRANULARITY, "Flushed: %I64d\n", Flushed.QuadPart);
    }

    /* Test 2: dirty the first page, nothing is flushed anymore */
    else if (TestId == 2)
    {
        Offset.QuadPart = 0;
        DirtyPage(&Offset);

        Flushed = CcGetFlushedValidData(TestFileObject->SectionObjectPointer, FALSE);
        ok(Flushed.QuadPart == 0, "Flushed: %I64d\n", Flushed.QuadPart);
    }

    /* Test 3: dirty a page past the first VACB, everything before it is still flushed */
    else if (TestId == 3)
    {
        Offset.QuadPart = VACB_MAPPING_GRANULARITY;
        DirtyPage(&Offset);

        Flushed = CcGetFlushedValidData(TestFileObject->SectionObjectPointer, FALSE);
        ok(Flushed.QuadPart == VACB_MAPPING_GRANULARITY, "Flushed: %I64d\n", Flushed.QuadPart);
    }

    /* Test 4: dirty then flush, we're back to fully flushed */
    else if (TestId == 4)
    {
        Offset.QuadPart = 0;
        DirtyPage(&Offset);

        KmtStartSeh();
        CcFlushCache(TestFileObject->SectionObjectPointer, NULL, 0, &IoStatus);
        KmtEndSeh(STATUS_SUCCESS);

        Flushed = CcGetFlushedValidData(TestFileObject->SectionObjectPointer, FALSE);
        ok(Flushed.QuadPart == 2 * VACB_MAPPING_GRANULARITY, "Flushed: %I64d\n", Flushed.QuadPart);
    }
}

static
VOID
CleanupTest(
    ULONG TestId,
    PDEVICE_OBJECT DeviceObject)
{
    LARGE_INTEGER Zero = RTL_CONSTANT_LARGE_INTEGER(0LL);
    CACHE_UNINITIALIZE_EVENT CacheUninitEvent;

    ok_eq_pointer(TestDeviceObject, DeviceObject);
    ok_eq_ulong(TestTestId, TestId);

    if (!skip(TestFileObject != NULL || TestId == 0, "No test FO\n"))
    {
        if (TestFileObject != NULL)
        {
            if (CcIsFileCached(TestFileObject))
            {
                KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
                CcUninitializeCacheMap(TestFileObject, &Zero, &CacheUninitEvent);
                KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
            }

            if (TestFileObject->FsContext != NULL)
            {
                ExFreePool(TestFileObject->FsContext);
                TestFileObject->FsContext = NULL;
                TestFileObject->SectionObjectPointer = NULL;
            }

            ObDereferenceObject(TestFileObject);
        }
    }

    TestFileObject = NULL;
    TestDeviceObject = NULL;
    TestTestId = -1;
}

static
NTSTATUS
TestMessageHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ ULONG ControlCode,
    _In_opt_ PVOID Buffer,
    _In_ SIZE_T InLength,
    _Inout_ PSIZE_T OutLength)
{
    NTSTATUS Status = STATUS_SUCCESS;

    FsRtlEnterFileSystem();

    switch (ControlCode)
    {
        case IOCTL_START_TEST:
            ok_eq_ulong((ULONG)InLength, sizeof(ULONG));
            PerformTest(*(PULONG)Buffer, DeviceObject);
            break;

        case IOCTL_FINISH_TEST:
            ok_eq_ulong((ULONG)InLength, sizeof(ULONG));
            CleanupTest(*(PULONG)Buffer, DeviceObject);
            break;

        default:
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    FsRtlExitFileSystem();

    return Status;
}

static
NTSTATUS
TestIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
    NTSTATUS Status;
    ULONG Length;
    PVOID Buffer;

    PAGED_CODE();

    FsRtlEnterFileSystem();

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_READ)
    {
        Length = IoStack->Parameters.Read.Length;

        Buffer = MapAndLockUserBuffer(Irp, Length);
        ok(Buffer != NULL, "Null pointer!\n");
        if (Buffer != NULL)
        {
            /* Content doesn't matter for these tests */
            RtlFillMemory(Buffer, Length, 0xBA);
            Status = STATUS_SUCCESS;
            Irp->IoStatus.Information = Length;
        }
        else
        {
            Status = STATUS_UNSUCCESSFUL;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        Length = IoStack->Parameters.Write.Length;

        Buffer = MapAndLockUserBuffer(Irp, Length);
        ok(Buffer != NULL, "Null pointer!\n");

        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = Length;
    }

    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    FsRtlExitFileSystem();

    return Status;
}