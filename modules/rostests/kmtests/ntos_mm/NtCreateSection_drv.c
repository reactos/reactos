/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for NtCreateSection function
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

static UNICODE_STRING InitOnCreate = RTL_CONSTANT_STRING(L"\\InitOnCreate");
static UNICODE_STRING InitOnRW = RTL_CONSTANT_STRING(L"\\InitOnRW");
static UNICODE_STRING InvalidInit = RTL_CONSTANT_STRING(L"\\InvalidInit");

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

static
BOOLEAN
NTAPI
FastIoQueryStandardInfo(
    _In_ PFILE_OBJECT FileObject,
    _In_ BOOLEAN Wait,
    _Out_ PFILE_STANDARD_INFORMATION Buffer,
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

    *DeviceName = L"NtCreateSection";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    KmtRegisterIrpHandler(IRP_MJ_CLEANUP, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_WRITE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_QUERY_INFORMATION, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_SET_INFORMATION, NULL, TestIrpHandler);

    TestFastIoDispatch.FastIoRead = FastIoRead;
    TestFastIoDispatch.FastIoWrite = FastIoWrite;
    TestFastIoDispatch.FastIoQueryStandardInfo = FastIoQueryStandardInfo;
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
           IoStack->MajorFunction == IRP_MJ_READ ||
           IoStack->MajorFunction == IRP_MJ_WRITE ||
           IoStack->MajorFunction == IRP_MJ_QUERY_INFORMATION ||
           IoStack->MajorFunction == IRP_MJ_SET_INFORMATION);

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        ULONG RequestedDisposition = ((IoStack->Parameters.Create.Options >> 24) & 0xff);
        ok(RequestedDisposition == FILE_CREATE || RequestedDisposition == FILE_OPEN, "Invalid disposition: %lu\n", RequestedDisposition);

        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR))
        {
            TestDeviceObject = DeviceObject;
            TestFileObject = IoStack->FileObject;
        }
        Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Fcb), 'FwrI');
        RtlZeroMemory(Fcb, sizeof(*Fcb));
        ExInitializeFastMutex(&Fcb->HeaderMutex);
        FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);

        /* Consider file/dir doesn't exist */
        if (RequestedDisposition == FILE_CREATE)
        {
            Fcb->Header.AllocationSize.QuadPart = 0;
            Fcb->Header.FileSize.QuadPart = 0;
            Fcb->Header.ValidDataLength.QuadPart = 0;
        }
        else
        {
            Fcb->Header.AllocationSize.QuadPart = 512;
            Fcb->Header.FileSize.QuadPart = 512;
            Fcb->Header.ValidDataLength.QuadPart = 512;
        }
        Fcb->Header.IsFastIoPossible = FastIoIsNotPossible;

        DPRINT1("File: %wZ\n", &IoStack->FileObject->FileName);

        IoStack->FileObject->FsContext = Fcb;
        if (RtlCompareUnicodeString(&IoStack->FileObject->FileName, &InvalidInit, FALSE) != 0)
        {
            IoStack->FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
        }

        if (IoStack->FileObject->FileName.Length == 0 ||
            RtlCompareUnicodeString(&IoStack->FileObject->FileName, &InitOnCreate, FALSE) == 0)
        {
            DPRINT1("Init\n");

            CcInitializeCacheMap(IoStack->FileObject, 
                                 (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                 FALSE, &Callbacks, NULL);
        }

        Irp->IoStatus.Information = (RequestedDisposition == FILE_CREATE) ? FILE_CREATED : FILE_OPENED;
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

        if (Offset.QuadPart + Length > Fcb->Header.FileSize.QuadPart)
        {
            Status = STATUS_END_OF_FILE;
        }
        else if (Length == 0)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            if (!FlagOn(Irp->Flags, IRP_NOCACHE))
            {
                Buffer = Irp->AssociatedIrp.SystemBuffer;
                ok(Buffer != NULL, "Null pointer!\n");

                _SEH2_TRY
                {
                    if (IoStack->FileObject->PrivateCacheMap == NULL)
                    {
                        DPRINT1("Init\n");
                        ok_eq_ulong(RtlCompareUnicodeString(&IoStack->FileObject->FileName, &InitOnRW, FALSE), 0);
                        CcInitializeCacheMap(IoStack->FileObject, 
                                             (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                             FALSE, &Callbacks, Fcb);
                    }

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
            }
            else
            {
                ok(Irp->AssociatedIrp.SystemBuffer == NULL, "A SystemBuffer was allocated!\n");
                Buffer = MapAndLockUserBuffer(Irp, Length);
                ok(Buffer != NULL, "Null pointer!\n");
                RtlFillMemory(Buffer, Length, 0xBA);

                Status = STATUS_SUCCESS;
            }
        }

        if (NT_SUCCESS(Status))
        {
            Irp->IoStatus.Information = Length;
            IoStack->FileObject->CurrentByteOffset.QuadPart = Offset.QuadPart + Length;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        BOOLEAN Ret;
        ULONG Length;
        PVOID Buffer;
        LARGE_INTEGER Offset;

        Offset = IoStack->Parameters.Write.ByteOffset;
        Length = IoStack->Parameters.Write.Length;
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);

        if (Length == 0)
        {
            Status = STATUS_SUCCESS;
        }
        else
        {
            if (!FlagOn(Irp->Flags, IRP_NOCACHE))
            {
                Buffer = Irp->AssociatedIrp.SystemBuffer;
                ok(Buffer != NULL, "Null pointer!\n");

                _SEH2_TRY
                {
                    if (IoStack->FileObject->PrivateCacheMap == NULL)
                    {
                        ok_eq_ulong(RtlCompareUnicodeString(&IoStack->FileObject->FileName, &InitOnRW, FALSE), 0);
                        CcInitializeCacheMap(IoStack->FileObject,
                                             (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                             FALSE, &Callbacks, Fcb);
                    }

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
            else
            {
                PMDL Mdl;

                Mdl = Irp->MdlAddress;
                ok(Mdl != NULL, "Null pointer for MDL!\n");
                ok((Mdl->MdlFlags & MDL_PAGES_LOCKED) != 0, "MDL not locked\n");
                ok((Mdl->MdlFlags & MDL_SOURCE_IS_NONPAGED_POOL) == 0, "MDL from non paged\n");
                ok((Mdl->MdlFlags & MDL_IO_PAGE_READ) == 0, "Paging IO for reading\n");
                ok((Irp->Flags & IRP_PAGING_IO) != 0, "Non paging IO\n");

                Status = STATUS_SUCCESS;
            }

            if (NT_SUCCESS(Status))
            {
                if (Length + Offset.QuadPart > Fcb->Header.FileSize.QuadPart)
                {
                    Fcb->Header.AllocationSize.QuadPart = Length + Offset.QuadPart;
                    Fcb->Header.FileSize.QuadPart = Length + Offset.QuadPart;
                    Fcb->Header.ValidDataLength.QuadPart = Length + Offset.QuadPart;

                    if (CcIsFileCached(IoStack->FileObject))
                    {
                        CcSetFileSizes(IoStack->FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                    }
                }
            }
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_CLEANUP)
    {
        Fcb = IoStack->FileObject->FsContext;
        ok(Fcb != NULL, "Null pointer!\n");
        if (IoStack->FileObject->SectionObjectPointer != NULL)
        {
            LARGE_INTEGER Zero = RTL_CONSTANT_LARGE_INTEGER(0LL);

            if (CcIsFileCached(IoStack->FileObject))
            {
                CcFlushCache(&Fcb->SectionObjectPointers, NULL, 0, NULL);
                CcPurgeCacheSection(&Fcb->SectionObjectPointers, NULL, 0, FALSE);
            }

            KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
            CcUninitializeCacheMap(IoStack->FileObject, &Zero, &CacheUninitEvent);
            KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        }
        ExFreePoolWithTag(Fcb, 'FwrI');
        IoStack->FileObject->FsContext = NULL;
        Status = STATUS_SUCCESS;
    }
    else if (IoStack->MajorFunction == IRP_MJ_QUERY_INFORMATION)
    {
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);
        ok_eq_ulong(IoStack->Parameters.QueryFile.FileInformationClass, FileStandardInformation);

        if (IoStack->Parameters.QueryFile.FileInformationClass == FileStandardInformation)
        {
            PFILE_STANDARD_INFORMATION StandardInfo = Irp->AssociatedIrp.SystemBuffer;
            ULONG BufferLength = IoStack->Parameters.QueryFile.Length;

            if (BufferLength < sizeof(FILE_STANDARD_INFORMATION))
            {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                ok(StandardInfo != NULL, "Null pointer!\n");
                ok(Fcb != NULL, "Null pointer!\n");

                StandardInfo->AllocationSize = Fcb->Header.AllocationSize;
                StandardInfo->EndOfFile = Fcb->Header.FileSize;
                StandardInfo->Directory = FALSE;
                StandardInfo->NumberOfLinks = 1;
                StandardInfo->DeletePending = FALSE;

                Irp->IoStatus.Information = sizeof(FILE_STANDARD_INFORMATION);
                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            Status = STATUS_NOT_IMPLEMENTED;
        }
    }
    else if (IoStack->MajorFunction == IRP_MJ_SET_INFORMATION)
    {
        Fcb = IoStack->FileObject->FsContext;

        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);
        ok_eq_ulong(IoStack->Parameters.SetFile.FileInformationClass, FileEndOfFileInformation);

        if (IoStack->Parameters.SetFile.FileInformationClass == FileEndOfFileInformation)
        {
            PFILE_END_OF_FILE_INFORMATION EOFInfo = Irp->AssociatedIrp.SystemBuffer;
            ULONG BufferLength = IoStack->Parameters.SetFile.Length;

            if (BufferLength < sizeof(FILE_END_OF_FILE_INFORMATION))
            {
                Status = STATUS_BUFFER_OVERFLOW;
            }
            else
            {
                ULONG TestSize = 0;

                ok(EOFInfo != NULL, "Null pointer!\n");
                ok(Fcb != NULL, "Null pointer!\n");
                ok_bool_false(IoStack->Parameters.SetFile.AdvanceOnly, "AdvanceOnly set!\n");
                ok(EOFInfo->EndOfFile.QuadPart > Fcb->Header.AllocationSize.QuadPart, "New size smaller\n");

                if (Fcb->Header.AllocationSize.QuadPart != 0)
                {
                    TestSize = 512;
                }

                Fcb->Header.AllocationSize.QuadPart = EOFInfo->EndOfFile.QuadPart;
                ok_eq_ulong(Fcb->Header.FileSize.QuadPart, TestSize);
                ok_eq_ulong(Fcb->Header.ValidDataLength.QuadPart, TestSize);

                if (CcIsFileCached(IoStack->FileObject))
                {
                    CcSetFileSizes(IoStack->FileObject, (PCC_FILE_SIZES)(&(Fcb->Header.AllocationSize)));
                }

                ok_eq_ulong(Fcb->Header.FileSize.QuadPart, TestSize);
                ok_eq_ulong(Fcb->Header.ValidDataLength.QuadPart, TestSize);

                Status = STATUS_SUCCESS;
            }
        }
        else
        {
            Status = STATUS_NOT_IMPLEMENTED;
        }
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
