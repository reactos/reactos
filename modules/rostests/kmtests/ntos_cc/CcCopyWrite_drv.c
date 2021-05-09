/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for CcCopyWrite function
 * PROGRAMMER:      Pierre Schweitzer <pierre@reactos.org>
 */

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

typedef struct _TEST_FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    FAST_MUTEX HeaderMutex;
    ULONG WriteLength;
} TEST_FCB, *PTEST_FCB;

static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;
static KMT_IRP_HANDLER TestIrpHandler;
static FAST_IO_DISPATCH TestFastIoDispatch;
static BOOLEAN InBehaviourTest;

BOOLEAN ReadCalled;
LARGE_INTEGER ReadOffset;
ULONG ReadLength;

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
    _Out_ PVOID Buffer,
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
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    *DeviceName = L"CcCopyWrite";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_CLEANUP, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_WRITE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_FLUSH_BUFFERS, NULL, TestIrpHandler);

    TestFastIoDispatch.FastIoRead = FastIoRead;
    TestFastIoDispatch.FastIoWrite = FastIoWrite;
    DriverObject->FastIoDispatch = &TestFastIoDispatch;


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
void
reset_read(void)
{
    ReadCalled = FALSE;
    ReadOffset.QuadPart = MAXLONGLONG;
    ReadLength = MAXULONG;
}

#define ok_read_called(_Offset, _Length) do {                           \
    ok(ReadCalled, "CcCopyWrite should have triggerred a read.\n");     \
    ok_eq_longlong(ReadOffset.QuadPart, _Offset);                       \
    ok_eq_ulong(ReadLength, _Length);                                   \
}while(0)

#define ok_read_not_called() ok(!ReadCalled, "CcCopyWrite shouldn't have triggered a read.\n")

static
VOID
Test_CcCopyWrite(PFILE_OBJECT FileObject)
{

    BOOLEAN Ret;
    LARGE_INTEGER Offset;
    CHAR Buffer[10];

    memset(Buffer, 0xAC, 10);

    /* Test bogus file object & file offset */
    Ret = 'x';
    KmtStartSeh()
        Ret = CcCopyWrite(FileObject, NULL, 0, FALSE, NULL);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_char(Ret, 'x');

    Ret = 'x';
    Offset.QuadPart = 0;
    KmtStartSeh()
        Ret = CcCopyWrite(NULL, &Offset, 10, FALSE, Buffer);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_char(Ret, 'x');

    /* What happens on invalid buffer */
    Ret = 'x';
    Offset.QuadPart = 0;
    reset_read();
    KmtStartSeh()
        Ret = CcCopyWrite(FileObject, &Offset, 0, TRUE, NULL);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_true(Ret, "CcCopyWrite(0, NULL) should succeed\n");
    /* When there is nothing to write, there is no reason to read */
    ok_read_not_called();

    Ret = 'x';
    Offset.QuadPart = 0;
    reset_read();
    KmtStartSeh()
        Ret = CcCopyWrite(FileObject, &Offset, 10, TRUE, NULL);
    KmtEndSeh(STATUS_INVALID_USER_BUFFER);
    ok_eq_char(Ret, 'x');
    /* This raises an exception, but it actually triggered a read */
    ok_read_called(0, PAGE_SIZE);

    /* So this one succeeds, as the page is now resident */
    Ret = 'x';
    Offset.QuadPart = 0;
    reset_read();
    KmtStartSeh()
        Ret = CcCopyWrite(FileObject, &Offset, 10, FALSE, Buffer);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_true(Ret, "CcCopyWrite should succeed\n");
    /* But there was no read triggered, as the page is already resident. */
    ok_read_not_called();

    /* But this one doesn't */
    Ret = 'x';
    Offset.QuadPart = PAGE_SIZE;
    reset_read();
    KmtStartSeh()
        Ret = CcCopyWrite(FileObject, &Offset, 10, FALSE, Buffer);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_false(Ret, "CcCopyWrite should fail\n");
    /* But it triggered a read anyway. */
    ok_read_called(PAGE_SIZE, PAGE_SIZE);

    /* Of course, waiting for it succeeds and triggers the read */
    Ret = 'x';
    Offset.QuadPart = PAGE_SIZE * 2;
    reset_read();
    KmtStartSeh()
        Ret = CcCopyWrite(FileObject, &Offset, 10, TRUE, Buffer);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_true(Ret, "CcCopyWrite should succeed\n");
    ok_read_called(PAGE_SIZE * 2, PAGE_SIZE);
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

    DPRINT("IRP %x/%x\n", IoStack->MajorFunction, IoStack->MinorFunction);
    ASSERT(IoStack->MajorFunction == IRP_MJ_CLEANUP ||
           IoStack->MajorFunction == IRP_MJ_CREATE ||
           IoStack->MajorFunction == IRP_MJ_READ ||
           IoStack->MajorFunction == IRP_MJ_WRITE ||
           IoStack->MajorFunction == IRP_MJ_FLUSH_BUFFERS);

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        ok_irql(PASSIVE_LEVEL);

        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR))
        {
            TestDeviceObject = DeviceObject;
            TestFileObject = IoStack->FileObject;
        }
        Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Fcb), 'FwrI');
        RtlZeroMemory(Fcb, sizeof(*Fcb));
        ExInitializeFastMutex(&Fcb->HeaderMutex);
        FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);
        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
            IoStack->FileObject->FileName.Buffer[1] == 'B')
        {
            Fcb->Header.AllocationSize.QuadPart = 1000000;
            Fcb->Header.FileSize.QuadPart = 1000000;
            Fcb->Header.ValidDataLength.QuadPart = 1000000;
        }
        else if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
                 IoStack->FileObject->FileName.Buffer[1] == 'S')
        {
            Fcb->Header.AllocationSize.QuadPart = 512;
            Fcb->Header.FileSize.QuadPart = 512;
            Fcb->Header.ValidDataLength.QuadPart = 512;
        }
        else if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
                 IoStack->FileObject->FileName.Buffer[1] == 'V')
        {
            Fcb->Header.AllocationSize.QuadPart = 62;
            Fcb->Header.FileSize.QuadPart = 62;
            Fcb->Header.ValidDataLength.QuadPart = 62;
        }
        else
        {
            Fcb->Header.AllocationSize.QuadPart = 1004;
            Fcb->Header.FileSize.QuadPart = 1004;
            Fcb->Header.ValidDataLength.QuadPart = 1004;
        }
        Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;
        IoStack->FileObject->FsContext = Fcb;
        IoStack->FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;

        CcInitializeCacheMap(IoStack->FileObject,
                             (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                             FALSE, &Callbacks, NULL);

        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
    }
    else if (IoStack->MajorFunction == IRP_MJ_READ)
    {
        PMDL Mdl;
        ULONG Length;
        PVOID Buffer;
        LARGE_INTEGER Offset;

        Offset = IoStack->Parameters.Read.ByteOffset;
        Length = IoStack->Parameters.Read.Length;
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);

        ok(BooleanFlagOn(Irp->Flags, IRP_NOCACHE), "IRP not coming from Cc!\n");
        ok_irql(APC_LEVEL);
        ok((Offset.QuadPart == 0 || Offset.QuadPart == 4096 || Offset.QuadPart == 8192), "Unexpected offset: %I64i\n", Offset.QuadPart);
        ok(Length % PAGE_SIZE == 0, "Length is not aligned: %I64i\n", Length);

        ReadCalled = TRUE;
        ReadOffset = Offset;
        ReadLength = Length;

        ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
        Buffer = MapAndLockUserBuffer(Irp, Length);
        ok(Buffer != NULL, "Null pointer!\n");
        RtlFillMemory(Buffer, Length, 0xBA);

        Status = STATUS_SUCCESS;

        Mdl = Irp->MdlAddress;
        ok(Mdl != NULL, "Null pointer for MDL!\n");
        ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
        ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
        ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) != 0, "Non paging IO\n");
        ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");

        Irp->IoStatus.Information = Length;
        IoStack->FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;
    }
    else if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        PMDL Mdl;
        ULONG Length;
        PVOID Buffer;
        LARGE_INTEGER Offset;
        static const UNICODE_STRING BehaviourTestFileName = RTL_CONSTANT_STRING(L"\\BehaviourTestFile");

        Offset = IoStack->Parameters.Read.ByteOffset;
        Length = IoStack->Parameters.Read.Length;
        Fcb = IoStack->FileObject->FsContext;

        ok_bool_false(InBehaviourTest, "Shouldn't trigger write from CcCopyWrite\n");
        /* Check special file name */
        InBehaviourTest = RtlCompareUnicodeString(&IoStack->FileObject->FileName, &BehaviourTestFileName, TRUE) == 0;

        if (!FlagOn(Irp->Flags, IRP_NOCACHE))
        {
            BOOLEAN Ret;

            ok_irql(PASSIVE_LEVEL);

            if (InBehaviourTest)
            {
                Test_CcCopyWrite(IoStack->FileObject);
                Status = Irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else
            {
                Buffer = Irp->AssociatedIrp.SystemBuffer;
                ok(Buffer != NULL, "Null pointer!\n");

                Fcb->WriteLength = Length;

                _SEH2_TRY
                {
                    Ret = CcCopyWrite(IoStack->FileObject, &Offset, Length, TRUE, Buffer);
                    ok_bool_true(Ret, "CcCopyWrite");
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Irp->IoStatus.Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                Status = Irp->IoStatus.Status;
            }
        }
        else
        {
            ok_irql(PASSIVE_LEVEL);

            if (InBehaviourTest)
            {
                /* We wrote only to the 1st and 3rd page */
                ok((Offset.QuadPart == 0) || (Offset.QuadPart == PAGE_SIZE * 2), "Expected a non-cached write on 1st or 3rd page\n");
                /* We wrote that much with CcCopyWrite */
                ok_eq_ulong(Length, PAGE_SIZE);
            }
            else
            {
                ok((Offset.QuadPart == 0 || Offset.QuadPart == 4096), "Unexpected offset: %I64i\n", Offset.QuadPart);
                ok_eq_ulong(Length, ROUND_TO_PAGES(Fcb->WriteLength));
            }

            ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
            Buffer = MapAndLockUserBuffer(Irp, Length);
            ok(Buffer != NULL, "Null pointer!\n");

            Status = STATUS_SUCCESS;

            Mdl = Irp->MdlAddress;
            ok(Mdl != NULL, "Null pointer for MDL!\n");
            ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
            ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
            ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0, "MDL for read paging IO\n");
            ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");
        }

        InBehaviourTest = FALSE;
    }
    else if (IoStack->MajorFunction == IRP_MJ_FLUSH_BUFFERS)
    {
        IO_STATUS_BLOCK IoStatus;

        Fcb = IoStack->FileObject->FsContext;
        CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, &IoStatus);

        Status = STATUS_SUCCESS;
    }
    else if (IoStack->MajorFunction == IRP_MJ_CLEANUP)
    {
        ok_irql(PASSIVE_LEVEL);
        KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
        CcUninitializeCacheMap(IoStack->FileObject, &Zero, &CacheUninitEvent);
        KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        Fcb = IoStack->FileObject->FsContext;
        ExFreePoolWithTag(Fcb, 'FwrI');
        IoStack->FileObject->FsContext = NULL;
        Status = STATUS_SUCCESS;
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

    return Status;
}
