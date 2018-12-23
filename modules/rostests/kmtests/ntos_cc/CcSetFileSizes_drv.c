/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for CcSetFileSizes function
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

#define IOCTL_START_TEST  1
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
static BOOLEAN TestUnpin = FALSE;
static BOOLEAN TestSizing = FALSE;
static BOOLEAN TestDirtying = FALSE;
static BOOLEAN TestUncaching = FALSE;
static BOOLEAN TestWritten = FALSE;

NTSTATUS
TestEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PCUNICODE_STRING RegistryPath,
    _Out_ PCWSTR *DeviceName,
    _Inout_ INT *Flags)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    *DeviceName = L"CcSetFileSizes";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_WRITE, NULL, TestIrpHandler);
    KmtRegisterMessageHandler(0, NULL, TestMessageHandler);

    return Status;
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

static CC_FILE_SIZES NewFileSizes = {
    RTL_CONSTANT_LARGE_INTEGER((LONGLONG)VACB_MAPPING_GRANULARITY), // .AllocationSize
    RTL_CONSTANT_LARGE_INTEGER((LONGLONG)VACB_MAPPING_GRANULARITY), // .FileSize
    RTL_CONSTANT_LARGE_INTEGER((LONGLONG)VACB_MAPPING_GRANULARITY)  // .ValidDataLength
};

static
PVOID
MapAndLockUserBuffer(
    _In_ _Out_ PIRP Irp,
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
PerformTest(
    ULONG TestId,
    PDEVICE_OBJECT DeviceObject)
{
    PVOID Bcb;
    BOOLEAN Ret;
    PULONG Buffer;
    PTEST_FCB Fcb;
    LARGE_INTEGER Offset;
    IO_STATUS_BLOCK IoStatus;

    ok_eq_pointer(TestFileObject, NULL);
    ok_eq_pointer(TestDeviceObject, NULL);
    ok_eq_ulong(TestTestId, -1);

    TestWritten = FALSE;
    TestDeviceObject = DeviceObject;
    TestTestId = TestId;
    TestFileObject = IoCreateStreamFileObject(NULL, DeviceObject);
    if (!skip(TestFileObject != NULL, "Failed to allocate FO\n"))
    {
        Fcb = ExAllocatePool(NonPagedPool, sizeof(TEST_FCB));
        if (!skip(Fcb != NULL, "ExAllocatePool failed\n"))
        {
            RtlZeroMemory(Fcb, sizeof(TEST_FCB));
            ExInitializeFastMutex(&Fcb->HeaderMutex);
            FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);

            TestFileObject->FsContext = Fcb;
            TestFileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
            Fcb->Header.AllocationSize.QuadPart = VACB_MAPPING_GRANULARITY;
            Fcb->Header.FileSize.QuadPart = VACB_MAPPING_GRANULARITY - PAGE_SIZE;
            Fcb->Header.ValidDataLength.QuadPart = VACB_MAPPING_GRANULARITY - PAGE_SIZE;

            if ((TestId > 1 && TestId < 4) || TestId == 5)
            {
                Fcb->Header.AllocationSize.QuadPart = VACB_MAPPING_GRANULARITY - PAGE_SIZE;
            }

            KmtStartSeh();
            CcInitializeCacheMap(TestFileObject, (PCC_FILE_SIZES)&Fcb->Header.AllocationSize, TRUE, &Callbacks, NULL);
            KmtEndSeh(STATUS_SUCCESS);

            if (!skip(CcIsFileCached(TestFileObject) == TRUE, "CcInitializeCacheMap failed\n"))
            {
                trace("Starting test: %d\n", TestId);

                if (TestId == 0 || TestId == 2)
                {
                    Offset.QuadPart = 0;
                    KmtStartSeh();
                    Ret = CcMapData(TestFileObject, &Offset, VACB_MAPPING_GRANULARITY - PAGE_SIZE, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_SUCCESS);

                    if (!skip(Ret == TRUE, "CcMapData failed\n"))
                    {
                        ok_eq_ulong(Buffer[(VACB_MAPPING_GRANULARITY - PAGE_SIZE - sizeof(ULONG)) / sizeof(ULONG)], 0xBABABABA);
                        CcUnpinData(Bcb);
                    }

                    KmtStartSeh();
                    CcSetFileSizes(TestFileObject, &NewFileSizes);
                    KmtEndSeh(STATUS_SUCCESS);

                    Fcb->Header.AllocationSize.QuadPart = VACB_MAPPING_GRANULARITY;
                    Fcb->Header.FileSize.QuadPart = VACB_MAPPING_GRANULARITY;

                    Offset.QuadPart = 0;
                    KmtStartSeh();
                    Ret = CcMapData(TestFileObject, &Offset, VACB_MAPPING_GRANULARITY, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_SUCCESS);

                    if (!skip(Ret == TRUE, "CcMapData failed\n"))
                    {
                        ok_eq_ulong(Buffer[(VACB_MAPPING_GRANULARITY  - sizeof(ULONG)) / sizeof(ULONG)], 0xBABABABA);

                        CcUnpinData(Bcb);
                    }
                }
                else if (TestId == 1 || TestId == 3)
                {
                    Buffer = ExAllocatePool(NonPagedPool, PAGE_SIZE);
                    if (!skip(Buffer != NULL, "ExAllocatePool failed\n"))
                    {
                        Ret = FALSE;
                        Offset.QuadPart = VACB_MAPPING_GRANULARITY - 2 * PAGE_SIZE;

                        KmtStartSeh();
                        Ret = CcCopyRead(TestFileObject, &Offset, PAGE_SIZE, TRUE, Buffer, &IoStatus);
                        KmtEndSeh(STATUS_SUCCESS);

                        ok_eq_ulong(Buffer[(PAGE_SIZE - sizeof(ULONG)) / sizeof(ULONG)], 0xBABABABA);

                        KmtStartSeh();
                        CcSetFileSizes(TestFileObject, &NewFileSizes);
                        KmtEndSeh(STATUS_SUCCESS);

                        Fcb->Header.AllocationSize.QuadPart = VACB_MAPPING_GRANULARITY;
                        Fcb->Header.FileSize.QuadPart = VACB_MAPPING_GRANULARITY;
                        RtlZeroMemory(Buffer, PAGE_SIZE);

                        Offset.QuadPart = VACB_MAPPING_GRANULARITY - PAGE_SIZE;

                        KmtStartSeh();
                        Ret = CcCopyRead(TestFileObject, &Offset, PAGE_SIZE, TRUE, Buffer, &IoStatus);
                        KmtEndSeh(STATUS_SUCCESS);

                        ok_eq_ulong(Buffer[(PAGE_SIZE - sizeof(ULONG)) / sizeof(ULONG)], 0xBABABABA);

                        ExFreePool(Buffer);
                    }
                }
                else if (TestId == 4 || TestId == 5)
                {
                    /* Kill lazy writer */
                    CcSetAdditionalCacheAttributes(TestFileObject, FALSE, TRUE);

                    Offset.QuadPart = 0;
                    KmtStartSeh();
                    Ret = CcPinRead(TestFileObject, &Offset, VACB_MAPPING_GRANULARITY - PAGE_SIZE, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
                    KmtEndSeh(STATUS_SUCCESS);

                    if (!skip(Ret == TRUE, "CcPinRead failed\n"))
                    {
                        LARGE_INTEGER Flushed;

                        ok_eq_ulong(Buffer[(VACB_MAPPING_GRANULARITY - PAGE_SIZE - sizeof(ULONG)) / sizeof(ULONG)], 0xBABABABA);
                        Buffer[(VACB_MAPPING_GRANULARITY - PAGE_SIZE - sizeof(ULONG)) / sizeof(ULONG)] = 0xDADADADA;

                        TestDirtying = TRUE;
                        CcSetDirtyPinnedData(Bcb, NULL);
                        TestDirtying = FALSE;

                        ok_bool_false(TestWritten, "Dirty VACB has been unexpectedly written!\n");

                        TestSizing = TRUE;
                        KmtStartSeh();
                        CcSetFileSizes(TestFileObject, &NewFileSizes);
                        KmtEndSeh(STATUS_SUCCESS);
                        TestSizing = FALSE;

                        ok_bool_false(TestWritten, "Dirty VACB has been unexpectedly written!\n");

                        Fcb->Header.AllocationSize.QuadPart = VACB_MAPPING_GRANULARITY;
                        Fcb->Header.FileSize.QuadPart = VACB_MAPPING_GRANULARITY;

                        Flushed = CcGetFlushedValidData(TestFileObject->SectionObjectPointer, FALSE);
                        ok(Flushed.QuadPart == 0, "Flushed: %I64d\n", Flushed.QuadPart);

                        TestUnpin = TRUE;
                        CcUnpinData(Bcb);
                        TestUnpin = FALSE;

                        ok_bool_false(TestWritten, "Dirty VACB has been unexpectedly written!\n");

                        Offset.QuadPart = 0;
                        KmtStartSeh();
                        Ret = CcMapData(TestFileObject, &Offset, VACB_MAPPING_GRANULARITY, MAP_WAIT, &Bcb, (PVOID *)&Buffer);
                        KmtEndSeh(STATUS_SUCCESS);

                        if (!skip(Ret == TRUE, "CcMapData failed\n"))
                        {
                            ok_eq_ulong(Buffer[(VACB_MAPPING_GRANULARITY - PAGE_SIZE - sizeof(ULONG)) / sizeof(ULONG)], 0xDADADADA);
                            ok_eq_ulong(Buffer[(VACB_MAPPING_GRANULARITY  - sizeof(ULONG)) / sizeof(ULONG)], 0xBABABABA);

                            CcUnpinData(Bcb);

                            ok_bool_false(TestWritten, "Dirty VACB has been unexpectedly written!\n");
                        }
                    }
                }
            }
        }
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

    if (!skip(TestFileObject != NULL, "No test FO\n"))
    {
        if (CcIsFileCached(TestFileObject))
        {
            TestUncaching = TRUE;
            KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
            CcUninitializeCacheMap(TestFileObject, &Zero, &CacheUninitEvent);
            KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
            TestUncaching = FALSE;
        }

        if (TestFileObject->FsContext != NULL)
        {
            ExFreePool(TestFileObject->FsContext);
            TestFileObject->FsContext = NULL;
            TestFileObject->SectionObjectPointer = NULL;
        }

        ObDereferenceObject(TestFileObject);
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

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStack->MajorFunction, IoStack->MinorFunction);
    ASSERT(IoStack->MajorFunction == IRP_MJ_READ ||
           IoStack->MajorFunction == IRP_MJ_WRITE);

    FsRtlEnterFileSystem();

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_READ)
    {
        PMDL Mdl;
        ULONG Length;
        PTEST_FCB Fcb;
        LARGE_INTEGER Offset;
        PVOID Buffer, OrigBuffer;

        Offset = IoStack->Parameters.Read.ByteOffset;
        Length = IoStack->Parameters.Read.Length;
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);
        ok(Fcb != NULL, "Null FCB\n");

        ok(FlagOn(Irp->Flags, IRP_NOCACHE), "Not coming from Cc\n");

        ok_irql(APC_LEVEL);
        ok((Offset.QuadPart % PAGE_SIZE == 0 || Offset.QuadPart == 0), "Offset is not aligned: %I64i\n", Offset.QuadPart);
        ok(Length % PAGE_SIZE == 0, "Length is not aligned: %I64i\n", Length);

        ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
        OrigBuffer = Buffer = MapAndLockUserBuffer(Irp, Length);
        ok(Buffer != NULL, "Null pointer!\n");

        if (Offset.QuadPart < Fcb->Header.FileSize.QuadPart)
        {
            RtlFillMemory(Buffer, min(Length, Fcb->Header.FileSize.QuadPart - Offset.QuadPart), 0xBA);
            Buffer = (PVOID)((ULONG_PTR)Buffer + (ULONG_PTR)min(Length, Fcb->Header.FileSize.QuadPart - Offset.QuadPart));

            if (Length > (Fcb->Header.FileSize.QuadPart - Offset.QuadPart))
            {
                RtlFillMemory(Buffer, Length - Fcb->Header.FileSize.QuadPart, 0xBD);
            }
        }
        else
        {
            RtlFillMemory(Buffer, Length, 0xBD);
        }

        if ((TestTestId == 4 || TestTestId == 5) && TestWritten &&
            Offset.QuadPart <= VACB_MAPPING_GRANULARITY - PAGE_SIZE - sizeof(ULONG) &&
            Offset.QuadPart + Length >= VACB_MAPPING_GRANULARITY - PAGE_SIZE)
        {
            Buffer = (PVOID)((ULONG_PTR)OrigBuffer + (VACB_MAPPING_GRANULARITY - PAGE_SIZE - sizeof(ULONG)));
            RtlFillMemory(Buffer, sizeof(ULONG), 0xDA);
        }

        Status = STATUS_SUCCESS;

        Mdl = Irp->MdlAddress;
        ok(Mdl != NULL, "Null pointer for MDL!\n");
        ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
        ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
        ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) != 0, "Non paging IO\n");
        ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");

        Irp->IoStatus.Information = Length;
    }
    else if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        PMDL Mdl;
        ULONG Length;
        PVOID Buffer;
        LARGE_INTEGER Offset;

        Offset = IoStack->Parameters.Write.ByteOffset;
        Length = IoStack->Parameters.Write.Length;

        ok((TestTestId == 4 || TestTestId == 5), "Unexpected test id: %d\n", TestTestId);
        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);

        ok_bool_false(TestUnpin, "Write triggered while unpinning!\n");
        ok_bool_false(TestSizing, "Write triggered while sizing!\n");
        ok_bool_false(TestDirtying, "Write triggered while dirtying!\n");
        ok_bool_true(TestUncaching, "Write not triggered while uncaching!\n");

        ok(FlagOn(Irp->Flags, IRP_NOCACHE), "Not coming from Cc\n");

        ok_irql(PASSIVE_LEVEL);
        ok((Offset.QuadPart % PAGE_SIZE == 0 || Offset.QuadPart == 0), "Offset is not aligned: %I64i\n", Offset.QuadPart);
        ok(Length % PAGE_SIZE == 0, "Length is not aligned: %I64i\n", Length);

        Buffer = MapAndLockUserBuffer(Irp, Length);
        ok(Buffer != NULL, "Null pointer!\n");

        Mdl = Irp->MdlAddress;
        ok(Mdl != NULL, "Null pointer for MDL!\n");
        ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
        ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
        ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");

        TestWritten = TRUE;
        Status = STATUS_SUCCESS;
        Irp->IoStatus.Information = Length;
    }

    if (Status == STATUS_PENDING)
    {
        IoMarkIrpPending(Irp);
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
        Status = STATUS_PENDING;
    }
    else
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    FsRtlExitFileSystem();

    return Status;
}
