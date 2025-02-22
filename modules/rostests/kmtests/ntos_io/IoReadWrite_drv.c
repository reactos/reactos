/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test driver for Read/Write operations
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "IoReadWrite.h"

#define NDEBUG
#include <debug.h>

typedef struct _TEST_FCB
{
    FSRTL_ADVANCED_FCB_HEADER Header;
    SECTION_OBJECT_POINTERS SectionObjectPointers;
    FAST_MUTEX HeaderMutex;
    BOOLEAN Cached;
} TEST_FCB, *PTEST_FCB;

static KMT_IRP_HANDLER TestIrpHandler;
static FAST_IO_READ TestFastIoRead;
static FAST_IO_WRITE TestFastIoWrite;

static FAST_IO_DISPATCH TestFastIoDispatch;
static ULONG TestLastFastReadKey;
static ULONG TestLastFastWriteKey;
static PFILE_OBJECT TestFileObject;
static PDEVICE_OBJECT TestDeviceObject;

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

    *DeviceName = L"IoReadWrite";
    *Flags = TESTENTRY_NO_EXCLUSIVE_DEVICE |
             TESTENTRY_BUFFERED_IO_DEVICE |
             TESTENTRY_NO_READONLY_DEVICE;

    TestFastIoDispatch.FastIoRead = TestFastIoRead;
    TestFastIoDispatch.FastIoWrite = TestFastIoWrite;
    DriverObject->FastIoDispatch = &TestFastIoDispatch;

    KmtRegisterIrpHandler(IRP_MJ_CREATE, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_CLEANUP, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_READ, NULL, TestIrpHandler);
    KmtRegisterIrpHandler(IRP_MJ_WRITE, NULL, TestIrpHandler);

    return Status;
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
TestAcquireForLazyWrite(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    ok_eq_pointer(Context, NULL);
    ok(0, "Unexpected call to AcquireForLazyWrite\n");
    return TRUE;
}

static
VOID
NTAPI
TestReleaseFromLazyWrite(
    _In_ PVOID Context)
{
    ok_eq_pointer(Context, NULL);
    ok(0, "Unexpected call to ReleaseFromLazyWrite\n");
}

static
BOOLEAN
NTAPI
TestAcquireForReadAhead(
    _In_ PVOID Context,
    _In_ BOOLEAN Wait)
{
    ok_eq_pointer(Context, NULL);
    ok(0, "Unexpected call to AcquireForReadAhead\n");
    return TRUE;
}

static
VOID
NTAPI
TestReleaseFromReadAhead(
    _In_ PVOID Context)
{
    ok_eq_pointer(Context, NULL);
    ok(0, "Unexpected call to ReleaseFromReadAhead\n");
}

static
NTSTATUS
TestCommonRead(
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ LONGLONG FileOffset,
    _In_ ULONG LockKey,
    _Out_ PIO_STATUS_BLOCK IoStatus)
{
    if (FileOffset >= TEST_FILE_SIZE)
    {
        trace("FileOffset %I64d > file size\n", FileOffset);
        IoStatus->Status = STATUS_END_OF_FILE;
        IoStatus->Information = 0;
    }
    else if (Length == 0 || Buffer == NULL)
    {
        IoStatus->Status = STATUS_BUFFER_OVERFLOW;
        IoStatus->Information = TEST_FILE_SIZE - FileOffset;
    }
    else
    {
        Length = min(Length, TEST_FILE_SIZE - FileOffset);
        RtlFillMemory(Buffer, Length, KEY_GET_DATA(LockKey));
        IoStatus->Status = TestGetReturnStatus(LockKey);
        IoStatus->Information = Length;
    }
    if (LockKey & KEY_RETURN_PENDING)
        return STATUS_PENDING;
    return IoStatus->Status;
}

static
BOOLEAN
NTAPI
TestFastIoRead(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _Out_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PTEST_FCB Fcb;
    NTSTATUS Status;

    //trace("FastIoRead: %p %lx %I64d+%lu -> %p\n", FileObject, LockKey, FileOffset->QuadPart, Length, Buffer);
    ok_eq_pointer(FileObject, TestFileObject);
    ok_bool_true(Wait, "Wait is");
    ok_eq_pointer(DeviceObject, TestDeviceObject);
    Fcb = FileObject->FsContext;
    ok_bool_true(Fcb->Cached, "Cached is");

    TestLastFastReadKey = LockKey;
    ok((ULONG_PTR)Buffer < MM_USER_PROBE_ADDRESS, "Buffer is %p\n", Buffer);
    ok((ULONG_PTR)FileOffset > MM_USER_PROBE_ADDRESS, "FileOffset is %p\n", FileOffset);
    ok((ULONG_PTR)IoStatus > MM_USER_PROBE_ADDRESS, "IoStatus is %p\n", IoStatus);
    _SEH2_TRY
    {
        Status = TestCommonRead(Buffer, Length, FileOffset->QuadPart, LockKey, IoStatus);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        IoStatus->Status = _SEH2_GetExceptionCode();
        return FALSE;
    }
    _SEH2_END;

    if (Status == STATUS_PENDING)
        return FALSE;

    if (LockKey & KEY_USE_FASTIO)
        return TRUE;
    else
        return FALSE;
}

static
NTSTATUS
TestCommonWrite(
    _In_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ LONGLONG FileOffset,
    _In_ ULONG LockKey,
    _Out_ PIO_STATUS_BLOCK IoStatus)
{
    ULONG i;
    PUCHAR BufferBytes = Buffer;

    for (i = 0; i < Length; i++)
        ok(BufferBytes[i] == KEY_GET_DATA(LockKey), "Buffer[%lu] = 0x%x, expected 0x%x\n", i, BufferBytes[i], KEY_GET_DATA(LockKey));
    IoStatus->Status = TestGetReturnStatus(LockKey);
    IoStatus->Information = Length;

    if (LockKey & KEY_RETURN_PENDING)
        return STATUS_PENDING;
    return IoStatus->Status;
}

static
BOOLEAN
NTAPI
TestFastIoWrite(
    _In_ PFILE_OBJECT FileObject,
    _In_ PLARGE_INTEGER FileOffset,
    _In_ ULONG Length,
    _In_ BOOLEAN Wait,
    _In_ ULONG LockKey,
    _In_ PVOID Buffer,
    _Out_ PIO_STATUS_BLOCK IoStatus,
    _In_ PDEVICE_OBJECT DeviceObject)
{
    PTEST_FCB Fcb;
    NTSTATUS Status;

    //trace("FastIoWrite: %p %lx %p -> %I64d+%lu\n", FileObject, LockKey, Buffer, FileOffset->QuadPart, Length);
    ok_eq_pointer(FileObject, TestFileObject);
    ok_bool_true(Wait, "Wait is");
    ok_eq_pointer(DeviceObject, TestDeviceObject);
    Fcb = FileObject->FsContext;
    ok_bool_true(Fcb->Cached, "Cached is");

    TestLastFastWriteKey = LockKey;
    ok((ULONG_PTR)Buffer < MM_USER_PROBE_ADDRESS, "Buffer is %p\n", Buffer);
    ok((ULONG_PTR)FileOffset > MM_USER_PROBE_ADDRESS, "FileOffset is %p\n", FileOffset);
    ok((ULONG_PTR)IoStatus > MM_USER_PROBE_ADDRESS, "IoStatus is %p\n", IoStatus);
    _SEH2_TRY
    {
        Status = TestCommonWrite(Buffer, Length, FileOffset->QuadPart, LockKey, IoStatus);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        IoStatus->Status = _SEH2_GetExceptionCode();
        return FALSE;
    }
    _SEH2_END;

    if (Status == STATUS_PENDING)
        return FALSE;

    if (LockKey & KEY_USE_FASTIO)
        return TRUE;
    else
        return FALSE;
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
    CACHE_MANAGER_CALLBACKS Callbacks;
    CACHE_UNINITIALIZE_EVENT CacheUninitEvent;

    PAGED_CODE();

    DPRINT("IRP %x/%x\n", IoStack->MajorFunction, IoStack->MinorFunction);
    ASSERT(IoStack->MajorFunction == IRP_MJ_CREATE ||
           IoStack->MajorFunction == IRP_MJ_CLEANUP ||
           IoStack->MajorFunction == IRP_MJ_READ ||
           IoStack->MajorFunction == IRP_MJ_WRITE);

    Status = STATUS_NOT_SUPPORTED;
    Irp->IoStatus.Information = 0;

    if (IoStack->MajorFunction == IRP_MJ_CREATE)
    {
        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR))
        {
            TestDeviceObject = DeviceObject;
            TestFileObject = IoStack->FileObject;
        }
        Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(*Fcb), 'FwrI');
        RtlZeroMemory(Fcb, sizeof(*Fcb));
        ExInitializeFastMutex(&Fcb->HeaderMutex);
        FsRtlSetupAdvancedHeader(&Fcb->Header, &Fcb->HeaderMutex);
        Fcb->Header.AllocationSize.QuadPart = TEST_FILE_SIZE;
        Fcb->Header.FileSize.QuadPart = TEST_FILE_SIZE;
        Fcb->Header.ValidDataLength.QuadPart = TEST_FILE_SIZE;
        IoStack->FileObject->FsContext = Fcb;
        IoStack->FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
        if (IoStack->FileObject->FileName.Length >= 2 * sizeof(WCHAR) &&
            IoStack->FileObject->FileName.Buffer[1] != 'N')
        {
            Fcb->Cached = TRUE;
            Callbacks.AcquireForLazyWrite = TestAcquireForLazyWrite;
            Callbacks.ReleaseFromLazyWrite = TestReleaseFromLazyWrite;
            Callbacks.AcquireForReadAhead = TestAcquireForReadAhead;
            Callbacks.ReleaseFromReadAhead = TestReleaseFromReadAhead;
            CcInitializeCacheMap(IoStack->FileObject,
                                 (PCC_FILE_SIZES)&Fcb->Header.AllocationSize,
                                 FALSE,
                                 &Callbacks,
                                 NULL);
        }
        Irp->IoStatus.Information = FILE_OPENED;
        Status = STATUS_SUCCESS;
    }
    else if (IoStack->MajorFunction == IRP_MJ_CLEANUP)
    {
        KeInitializeEvent(&CacheUninitEvent.Event, NotificationEvent, FALSE);
        CcUninitializeCacheMap(IoStack->FileObject, NULL, &CacheUninitEvent);
        KeWaitForSingleObject(&CacheUninitEvent.Event, Executive, KernelMode, FALSE, NULL);
        Fcb = IoStack->FileObject->FsContext;
        ExFreePoolWithTag(Fcb, 'FwrI');
        IoStack->FileObject->FsContext = NULL;
        Status = STATUS_SUCCESS;
    }
    else if (IoStack->MajorFunction == IRP_MJ_READ)
    {
        //trace("IRP_MJ_READ: %p %lx %I64d+%lu -> %p\n", IoStack->FileObject, IoStack->Parameters.Read.Key, IoStack->Parameters.Read.ByteOffset.QuadPart, IoStack->Parameters.Read.Length, Irp->AssociatedIrp.SystemBuffer);
        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);
        Fcb = IoStack->FileObject->FsContext;
        if (Fcb->Cached)
            ok_eq_hex(IoStack->Parameters.Read.Key, TestLastFastReadKey);
        ok(Irp->AssociatedIrp.SystemBuffer == NULL ||
           (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer > MM_USER_PROBE_ADDRESS,
           "Buffer is %p\n",
           Irp->AssociatedIrp.SystemBuffer);
        Status = TestCommonRead(Irp->AssociatedIrp.SystemBuffer,
                                IoStack->Parameters.Read.Length,
                                IoStack->Parameters.Read.ByteOffset.QuadPart,
                                IoStack->Parameters.Read.Key,
                                &Irp->IoStatus);
    }
    else if (IoStack->MajorFunction == IRP_MJ_WRITE)
    {
        //trace("IRP_MJ_WRITE: %p %lx %I64d+%lu -> %p\n", IoStack->FileObject, IoStack->Parameters.Write.Key, IoStack->Parameters.Write.ByteOffset.QuadPart, IoStack->Parameters.Write.Length, Irp->AssociatedIrp.SystemBuffer);
        ok_eq_pointer(DeviceObject, TestDeviceObject);
        ok_eq_pointer(IoStack->FileObject, TestFileObject);
        Fcb = IoStack->FileObject->FsContext;
        if (Fcb->Cached)
            ok_eq_hex(IoStack->Parameters.Write.Key, TestLastFastWriteKey);
        ok(Irp->AssociatedIrp.SystemBuffer == NULL ||
           (ULONG_PTR)Irp->AssociatedIrp.SystemBuffer > MM_USER_PROBE_ADDRESS,
           "Buffer is %p\n",
           Irp->AssociatedIrp.SystemBuffer);
        Status = TestCommonWrite(Irp->AssociatedIrp.SystemBuffer,
                                 IoStack->Parameters.Write.Length,
                                 IoStack->Parameters.Write.ByteOffset.QuadPart,
                                 IoStack->Parameters.Write.Key,
                                 &Irp->IoStatus);
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
