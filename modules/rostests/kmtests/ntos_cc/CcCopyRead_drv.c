/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for CcCopyRead function
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
    BOOLEAN BigFile;
} TEST_FCB, *PTEST_FCB;

static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;
static KMT_IRP_HANDLER TestIrpHandler;
static FAST_IO_DISPATCH TestFastIoDispatch;
static BOOLEAN InBehaviourTest;

BOOLEAN ReadCalledNonCached;
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

    *DeviceName = L"CcCopyRead";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_CLEANUP, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);

    TestFastIoDispatch.FastIoRead = FastIoRead;
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
    ReadCalledNonCached = FALSE;
    ReadOffset.QuadPart = MAXLONGLONG;
    ReadLength = MAXULONG;
}

#define ok_read_called(_Offset, _Length) do {                                               \
    ok(ReadCalledNonCached, "CcCopyRead should have triggerred a  non-cached read.\n");     \
    ok_eq_longlong(ReadOffset.QuadPart, _Offset);                                           \
    ok_eq_ulong(ReadLength, _Length);                                                       \
}while(0)

#define ok_read_not_called() ok(!ReadCalledNonCached, "CcCopyRead shouldn't have triggered a read.\n")

static
VOID
Test_CcCopyRead(PFILE_OBJECT FileObject)
{

    BOOLEAN Ret;
    LARGE_INTEGER Offset;
    CHAR Buffer[10];
    IO_STATUS_BLOCK IoStatus;

    memset(Buffer, 0xAC, 10);

    /* Test bogus file object & file offset */
    Ret = 'x';
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, NULL, 0, FALSE, NULL, &IoStatus);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_char(Ret, 'x');

    Ret = 'x';
    Offset.QuadPart = 0;
    KmtStartSeh()
        Ret = CcCopyRead(NULL, &Offset, 10, FALSE, Buffer, &IoStatus);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_char(Ret, 'x');

    /* What happens on invalid buffer */
    Ret = 'x';
    Offset.QuadPart = 0;
    memset(&IoStatus, 0xAB, sizeof(IoStatus));
    reset_read();
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, &Offset, 0, TRUE, NULL, &IoStatus);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_true(Ret, "CcCopyRead(0, NULL) should succeed\n");
    /* When there is nothing to write, there is no reason to read */
    ok_read_not_called();
    ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
    ok_eq_ulongptr(IoStatus.Information, 0);

    Ret = 'x';
    Offset.QuadPart = 0;
    memset(&IoStatus, 0xAB, sizeof(IoStatus));
    reset_read();
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, &Offset, 10, TRUE, NULL, &IoStatus);
    KmtEndSeh(STATUS_INVALID_USER_BUFFER);
    ok_eq_char(Ret, 'x');
    /* This raises an exception, but it actually triggered a read */
    ok_read_called(0, PAGE_SIZE);
    ok_eq_hex(IoStatus.Status, 0xABABABAB);
    ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0xABABABABABABABAB);

    /* So this one succeeds, as the page is now resident */
    Ret = 'x';
    Offset.QuadPart = 0;
    memset(&IoStatus, 0xAB, sizeof(IoStatus));
    reset_read();
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, &Offset, 10, FALSE, Buffer, &IoStatus);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_true(Ret, "CcCopyRead should succeed\n");
    /* But there was no read triggered, as the page is already resident. */
    ok_read_not_called();
    ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
    ok_eq_ulongptr(IoStatus.Information, 10);

    /* But this one doesn't */
    Ret = 'x';
    Offset.QuadPart = PAGE_SIZE;
    memset(&IoStatus, 0xAB, sizeof(IoStatus));
    reset_read();
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, &Offset, 10, FALSE, Buffer, &IoStatus);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_false(Ret, "CcCopyRead should fail\n");
    /* But it triggered a read anyway. */
    ok_read_called(PAGE_SIZE, PAGE_SIZE);
    ok_eq_hex(IoStatus.Status, 0xABABABAB);
    ok_eq_ulongptr(IoStatus.Information, (ULONG_PTR)0xABABABABABABABAB);

    /* Of course, waiting for it succeeds and triggers the read */
    Ret = 'x';
    Offset.QuadPart = PAGE_SIZE * 2;
    memset(&IoStatus, 0xAB, sizeof(IoStatus));
    reset_read();
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, &Offset, 10, TRUE, Buffer, &IoStatus);
    KmtEndSeh(STATUS_SUCCESS);
    ok_bool_true(Ret, "CcCopyRead should succeed\n");
    ok_read_called(PAGE_SIZE * 2, PAGE_SIZE);
    ok_eq_hex(IoStatus.Status, STATUS_SUCCESS);
    ok_eq_ulongptr(IoStatus.Information, 10);

    /* Try the same without a status block */
    Ret = 'x';
    Offset.QuadPart = PAGE_SIZE * 2;
    KmtStartSeh()
        Ret = CcCopyRead(FileObject, &Offset, 10, TRUE, Buffer, NULL);
    KmtEndSeh(STATUS_ACCESS_VIOLATION);
    ok_eq_char(Ret, 'x');
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
           IoStack->MajorFunction == IRP_MJ_READ);

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
        Fcb->BigFile = FALSE;
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
            Fcb->Header.AllocationSize.QuadPart = 1004;
            Fcb->Header.FileSize.QuadPart = 1004;
            Fcb->Header.ValidDataLength.QuadPart = 1004;
        }
        else if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
                 IoStack->FileObject->FileName.Buffer[1] == 'R')
        {
            Fcb->Header.AllocationSize.QuadPart = 62;
            Fcb->Header.FileSize.QuadPart = 62;
            Fcb->Header.ValidDataLength.QuadPart = 62;
        }
        else if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
                 IoStack->FileObject->FileName.Buffer[1] == 'F')
        {
            Fcb->Header.AllocationSize.QuadPart = 4294967296;
            Fcb->Header.FileSize.QuadPart = 4294967296;
            Fcb->Header.ValidDataLength.QuadPart = 4294967296;
            Fcb->BigFile = TRUE;
        }
        else
        {
            Fcb->Header.AllocationSize.QuadPart = 512;
            Fcb->Header.FileSize.QuadPart = 512;
            Fcb->Header.ValidDataLength.QuadPart = 512;
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
        BOOLEAN Ret;
        ULONG Length;
        PVOID Buffer;
        LARGE_INTEGER Offset;
        static const UNICODE_STRING BehaviourTestFileName = RTL_CONSTANT_STRING(L"\\BehaviourTestFile");

        Offset = IoStack->Parameters.Read.ByteOffset;
        Length = IoStack->Parameters.Read.Length;
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);

        /* Check special file name */
        InBehaviourTest = RtlCompareUnicodeString(&IoStack->FileObject->FileName, &BehaviourTestFileName, TRUE) == 0;

        if (!FlagOn(Irp->Flags, IRP_NOCACHE))
        {
            ok_irql(PASSIVE_LEVEL);

            if (InBehaviourTest)
            {
                Test_CcCopyRead(IoStack->FileObject);
                Status = Irp->IoStatus.Status = STATUS_SUCCESS;
            }
            else
            {
                /* We don't want to test alignement for big files (not the purpose of the test) */
                if (!Fcb->BigFile)
                {
                    ok(Offset.QuadPart % PAGE_SIZE != 0, "Offset is aligned: %I64i\n", Offset.QuadPart);
                    ok(Length % PAGE_SIZE != 0, "Length is aligned: %I64i\n", Length);
                }

                Buffer = Irp->AssociatedIrp.SystemBuffer;
                ok(Buffer != NULL, "Null pointer!\n");

                _SEH2_TRY
                {
                    Ret = CcCopyRead(IoStack->FileObject, &Offset, Length, TRUE, Buffer,
                                    &Irp->IoStatus);
                    ok_bool_true(Ret, "CcCopyRead");
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Irp->IoStatus.Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                Status = Irp->IoStatus.Status;

                if (NT_SUCCESS(Status))
                {
                    if (Offset.QuadPart <= 1000LL && Offset.QuadPart + Length > 1000LL)
                    {
                        ok_eq_hex(*(PUSHORT)((ULONG_PTR)Buffer + (ULONG_PTR)(1000LL - Offset.QuadPart)), 0xFFFF);
                    }
                    else
                    {
                        ok_eq_hex(*(PUSHORT)Buffer, 0xBABA);
                    }
                }
            }
        }
        else
        {
            PMDL Mdl;

            ReadCalledNonCached = TRUE;
            ReadOffset = Offset;
            ReadLength = Length;

            ok_irql(APC_LEVEL);
            ok((Offset.QuadPart % PAGE_SIZE == 0 || Offset.QuadPart == 0), "Offset is not aligned: %I64i\n", Offset.QuadPart);
            ok(Length % PAGE_SIZE == 0, "Length is not aligned: %I64i\n", Length);

            ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
            Buffer = MapAndLockUserBuffer(Irp, Length);
            ok(Buffer != NULL, "Null pointer!\n");
            RtlFillMemory(Buffer, Length, 0xBA);

            Status = STATUS_SUCCESS;
            if (Offset.QuadPart <= 1000LL && Offset.QuadPart + Length > 1000LL)
            {
                *(PUSHORT)((ULONG_PTR)Buffer + (ULONG_PTR)(1000LL - Offset.QuadPart)) = 0xFFFF;
            }

            Mdl = Irp->MdlAddress;
            ok(Mdl != NULL, "Null pointer for MDL!\n");
            ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
            ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
            ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) != 0, "Non paging IO\n");
            ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");
        }

        if (NT_SUCCESS(Status))
        {
            Irp->IoStatus.Information = Length;
            IoStack->FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;
        }

        InBehaviourTest = FALSE;
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
