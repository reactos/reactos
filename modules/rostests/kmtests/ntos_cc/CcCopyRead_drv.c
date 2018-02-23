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
} TEST_FCB, *PTEST_FCB;

static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;
static KMT_IRP_HANDLER TestIrpHandler;
static FAST_IO_DISPATCH TestFastIoDispatch;

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
    ASSERT(Irp->MdlAddress); // Replace dead code from r72165.

    return MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
}


static
NTSTATUS
TestIrpHandler(
    _In_ PDEVICE_OBJECT DeviceObject,
    _In_ PIRP Irp,
    _In_ PIO_STACK_LOCATION IoStack)
{
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

        Offset = IoStack->Parameters.Read.ByteOffset;
        Length = IoStack->Parameters.Read.Length;
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);

        if (!FlagOn(Irp->Flags, IRP_NOCACHE))
        {
            ok_irql(PASSIVE_LEVEL);
            ok(Offset.QuadPart % PAGE_SIZE != 0, "Offset is aligned: %I64i\n", Offset.QuadPart);
            ok(Length % PAGE_SIZE != 0, "Length is aligned: %I64i\n", Length);

            Buffer = Irp->AssociatedIrp.SystemBuffer;
            ok(Buffer != NULL, "No SystemBuffer was allocated!\n");

            // Even if there is no buffer, call CcCopyRead and let it fail.
            _SEH2_TRY
            {
                Ret = CcCopyRead(IoStack->FileObject, &Offset, Length, TRUE, Buffer,
                                 &Irp->IoStatus);
                ok_bool_true(Ret, "CcCopyRead returned");
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
        else
        {
            PMDL Mdl;

            ok_irql(APC_LEVEL);
            ok(Offset.QuadPart % PAGE_SIZE == 0, "Offset is not aligned: %I64i\n", Offset.QuadPart); // Revert the useless change from r72182.
            ok(Length % PAGE_SIZE == 0, "Length is not aligned: %I64i\n", Length);

            ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
            Buffer = MapAndLockUserBuffer(Irp, Length);
            if (skip(Buffer != NULL, "MapAndLockUserBuffer returned NULL!\n"))
            {
                // Set status at both places.
                Status = Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
            }
            else
            {

            RtlFillMemory(Buffer, Length, 0xBA);

            Status = STATUS_SUCCESS;
            if (Offset.QuadPart <= 1000LL && Offset.QuadPart + Length > 1000LL)
            {
                *(PUSHORT)((ULONG_PTR)Buffer + (ULONG_PTR)(1000LL - Offset.QuadPart)) = 0xFFFF;
            }

            } // else, Buffer != NULL

            Mdl = Irp->MdlAddress;
            ok(Mdl != NULL, "Null pointer for MDL!\n");
            ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
            ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
            ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) != 0, "Paging IO not for reading\n"); // r75845 copypasta.
            ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");
        }

        if (NT_SUCCESS(Status))
        {
            Irp->IoStatus.Information = Length;
            IoStack->FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_CLEANUP)
    {
        ok_irql(PASSIVE_LEVEL);
        KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
        CcUninitializeCacheMap(IoStack->FileObject, NULL, &CacheUninitEvent);
        KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        Fcb = IoStack->FileObject->FsContext;
        ExFreePoolWithTag(Fcb, 'FwrI');
        IoStack->FileObject->FsContext = NULL;
        Status = STATUS_SUCCESS;
    }

    // CcCopyRead never sets STATUS_PENDING, especially with wait=TRUE.
    ASSERT(Status != STATUS_PENDING);

// Remove wrong code added as is in r71445.
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    trace("TestIrpHandler returns: Status = 0x%08x", Status);
    return Status;
}
