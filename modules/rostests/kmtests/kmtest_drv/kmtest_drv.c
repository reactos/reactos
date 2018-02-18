/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite driver
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 *              Copyright 2013 Nikolay Borisov <nib9@aber.ac.uk>
 */

#include <ntddk.h>
#include <ntifs.h>
#include <ndk/ketypes.h>
#include <ntstrsafe.h>
#include <limits.h>
#include <pseh/pseh2.h>

#define NDEBUG
#include <debug.h>

#include <kmt_public.h>
#define KMT_DEFINE_TEST_FUNCTIONS
#include <kmt_test.h>

/* Usermode callback definitions */
typedef struct _KMT_USER_WORK_ENTRY
{
    LIST_ENTRY ListEntry;
    KEVENT WorkDoneEvent;
    KMT_CALLBACK_REQUEST_PACKET Request;
    PKMT_RESPONSE Response;
} KMT_USER_WORK_ENTRY, *PKMT_USER_WORK_ENTRY;

typedef struct _KMT_USER_WORK_LIST
{
    LIST_ENTRY ListHead;
    FAST_MUTEX Lock;
    KEVENT NewWorkEvent;
} KMT_USER_WORK_LIST, *PKMT_USER_WORK_LIST;

/* Prototypes */
DRIVER_INITIALIZE DriverEntry;
static DRIVER_UNLOAD DriverUnload;
__drv_dispatchType(IRP_MJ_CREATE)
static DRIVER_DISPATCH DriverCreate;
__drv_dispatchType(IRP_MJ_CLEANUP)
static DRIVER_DISPATCH DriverCleanup;
__drv_dispatchType(IRP_MJ_CLOSE)
static DRIVER_DISPATCH DriverClose;
__drv_dispatchType(IRP_MJ_DEVICE_CONTROL)
static DRIVER_DISPATCH DriverIoControl;
static VOID KmtCleanUsermodeCallbacks(VOID);

/* Globals */
static PDEVICE_OBJECT MainDeviceObject;
PDRIVER_OBJECT KmtDriverObject = NULL;
static KMT_USER_WORK_LIST WorkList;
static ULONG RequestId = 0;

/* Entry */
/**
 * @name DriverEntry
 *
 * Driver Entry point.
 *
 * @param DriverObject
 *        Driver Object
 * @param RegistryPath
 *        Driver Registry Path
 *
 * @return Status
 */
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status = STATUS_SUCCESS;
    UNICODE_STRING DeviceName;
    PKMT_DEVICE_EXTENSION DeviceExtension;
    PKPRCB Prcb;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(RegistryPath);

    DPRINT("DriverEntry\n");

    Prcb = KeGetCurrentPrcb();
    KmtIsCheckedBuild = (Prcb->BuildType & PRCB_BUILD_DEBUG) != 0;
    KmtIsMultiProcessorBuild = (Prcb->BuildType & PRCB_BUILD_UNIPROCESSOR) == 0;
    KmtDriverObject = DriverObject;

    RtlInitUnicodeString(&DeviceName, KMTEST_DEVICE_DRIVER_PATH);
    Status = IoCreateDevice(DriverObject, sizeof(KMT_DEVICE_EXTENSION),
                            &DeviceName,
                            FILE_DEVICE_UNKNOWN,
                            FILE_DEVICE_SECURE_OPEN | FILE_READ_ONLY_DEVICE,
                            FALSE, &MainDeviceObject);

    if (!NT_SUCCESS(Status))
        goto cleanup;

    DPRINT("DriverEntry. Created DeviceObject %p. DeviceExtension %p\n",
             MainDeviceObject, MainDeviceObject->DeviceExtension);
    DeviceExtension = MainDeviceObject->DeviceExtension;
    DeviceExtension->ResultBuffer = NULL;
    DeviceExtension->Mdl = NULL;

    DriverObject->DriverUnload = DriverUnload;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = DriverCreate;
    DriverObject->MajorFunction[IRP_MJ_CLEANUP] = DriverCleanup;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = DriverClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DriverIoControl;

    ExInitializeFastMutex(&WorkList.Lock);
    KeInitializeEvent(&WorkList.NewWorkEvent, NotificationEvent, FALSE);
    InitializeListHead(&WorkList.ListHead);

cleanup:
    if (MainDeviceObject && !NT_SUCCESS(Status))
    {
        IoDeleteDevice(MainDeviceObject);
        MainDeviceObject = NULL;
    }

    return Status;
}

/* Dispatch functions */
/**
 * @name DriverUnload
 *
 * Driver cleanup funtion.
 *
 * @param DriverObject
 *        Driver Object
 */
static
VOID
NTAPI
DriverUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    PAGED_CODE();

    UNREFERENCED_PARAMETER(DriverObject);

    DPRINT("DriverUnload\n");

    KmtCleanUsermodeCallbacks();

    if (MainDeviceObject)
    {
#if DBG
        PKMT_DEVICE_EXTENSION DeviceExtension = MainDeviceObject->DeviceExtension;
        ASSERT(!DeviceExtension->Mdl);
        ASSERT(!DeviceExtension->ResultBuffer);
#endif
        ASSERT(!ResultBuffer);
        IoDeleteDevice(MainDeviceObject);
    }
}

/**
 * @name DriverCreate
 *
 * Driver Dispatch function for IRP_MJ_CREATE
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static
NTSTATUS
NTAPI
DriverCreate(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverCreate. DeviceObject=%p, RequestorMode=%d, FileObject=%p, FsContext=%p, FsContext2=%p\n",
             DeviceObject, Irp->RequestorMode, IoStackLocation->FileObject,
             IoStackLocation->FileObject->FsContext, IoStackLocation->FileObject->FsContext2);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverCleanup
 *
 * Driver Dispatch function for IRP_MJ_CLEANUP
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static
NTSTATUS
NTAPI
DriverCleanup(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    PKMT_DEVICE_EXTENSION DeviceExtension;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverCleanup. DeviceObject=%p, RequestorMode=%d, FileObject=%p, FsContext=%p, FsContext2=%p\n",
             DeviceObject, Irp->RequestorMode, IoStackLocation->FileObject,
             IoStackLocation->FileObject->FsContext, IoStackLocation->FileObject->FsContext2);

    ASSERT(IoStackLocation->FileObject->FsContext2 == NULL);
    DeviceExtension = DeviceObject->DeviceExtension;
    if (DeviceExtension->Mdl && IoStackLocation->FileObject->FsContext == DeviceExtension->Mdl)
    {
        MmUnlockPages(DeviceExtension->Mdl);
        IoFreeMdl(DeviceExtension->Mdl);
        DeviceExtension->Mdl = NULL;
        ResultBuffer = DeviceExtension->ResultBuffer = NULL;
    }
    else
    {
        ASSERT(IoStackLocation->FileObject->FsContext == NULL);
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverClose
 *
 * Driver Dispatch function for IRP_MJ_CLOSE
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static
NTSTATUS
NTAPI
DriverClose(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    DPRINT("DriverClose. DeviceObject=%p, RequestorMode=%d\n",
             DeviceObject, Irp->RequestorMode);

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name DriverIoControl
 *
 * Driver Dispatch function for IRP_MJ_DEVICE_CONTROL
 *
 * @param DeviceObject
 *        Device Object
 * @param Irp
 *        I/O request packet
 *
 * @return Status
 */
static
NTSTATUS
NTAPI
DriverIoControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    NTSTATUS Status = STATUS_SUCCESS;
    PIO_STACK_LOCATION IoStackLocation;
    SIZE_T Length = 0;

    PAGED_CODE();

    IoStackLocation = IoGetCurrentIrpStackLocation(Irp);

    DPRINT("DriverIoControl. Code=0x%08X, DeviceObject=%p, FileObject=%p, FsContext=%p, FsContext2=%p\n",
             IoStackLocation->Parameters.DeviceIoControl.IoControlCode,
             DeviceObject, IoStackLocation->FileObject,
             IoStackLocation->FileObject->FsContext, IoStackLocation->FileObject->FsContext2);

    switch (IoStackLocation->Parameters.DeviceIoControl.IoControlCode)
    {
        case IOCTL_KMTEST_GET_TESTS:
        {
            PCKMT_TEST TestEntry;
            LPSTR OutputBuffer = Irp->AssociatedIrp.SystemBuffer;
            size_t Remaining = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

            DPRINT("DriverIoControl. IOCTL_KMTEST_GET_TESTS, outlen=%lu\n",
                     IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            for (TestEntry = TestList; TestEntry->TestName; ++TestEntry)
            {
                RtlStringCbCopyExA(OutputBuffer, Remaining, TestEntry->TestName, &OutputBuffer, &Remaining, 0);
                if (Remaining)
                {
                    *OutputBuffer++ = '\0';
                    --Remaining;
                }
            }
            if (Remaining)
            {
                *OutputBuffer++ = '\0';
                --Remaining;
            }
            Length = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength - Remaining;
            break;
        }
        case IOCTL_KMTEST_RUN_TEST:
        {
            ANSI_STRING TestName;
            PCKMT_TEST TestEntry;

            DPRINT("DriverIoControl. IOCTL_KMTEST_RUN_TEST, inlen=%lu, outlen=%lu\n",
                     IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                     IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);
            TestName.Length = TestName.MaximumLength = (USHORT)min(IoStackLocation->Parameters.DeviceIoControl.InputBufferLength, USHRT_MAX);
            TestName.Buffer = Irp->AssociatedIrp.SystemBuffer;
            DPRINT("DriverIoControl. Run test: %Z\n", &TestName);

            for (TestEntry = TestList; TestEntry->TestName; ++TestEntry)
            {
                ANSI_STRING EntryName;
                if (TestEntry->TestName[0] == '-')
                    RtlInitAnsiString(&EntryName, TestEntry->TestName + 1);
                else
                    RtlInitAnsiString(&EntryName, TestEntry->TestName);

                if (!RtlCompareString(&TestName, &EntryName, FALSE))
                {
                    DPRINT1("DriverIoControl. Starting test %Z\n", &EntryName);
                    TestEntry->TestFunction();
                    DPRINT1("DriverIoControl. Finished test %Z\n", &EntryName);
                    break;
                }
            }

            if (!TestEntry->TestName)
                Status = STATUS_OBJECT_NAME_INVALID;

            break;
        }
        case IOCTL_KMTEST_SET_RESULTBUFFER:
        {
            PKMT_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;

            DPRINT("DriverIoControl. IOCTL_KMTEST_SET_RESULTBUFFER, buffer=%p, inlen=%lu, outlen=%lu\n",
                    IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer,
                    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            if (DeviceExtension->Mdl)
            {
                if (IoStackLocation->FileObject->FsContext != DeviceExtension->Mdl)
                {
                    Status = STATUS_ACCESS_DENIED;
                    break;
                }
                MmUnlockPages(DeviceExtension->Mdl);
                IoFreeMdl(DeviceExtension->Mdl);
                IoStackLocation->FileObject->FsContext = NULL;
                ResultBuffer = DeviceExtension->ResultBuffer = NULL;
            }

            DeviceExtension->Mdl = IoAllocateMdl(IoStackLocation->Parameters.DeviceIoControl.Type3InputBuffer,
                                                    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                                                    FALSE, FALSE, NULL);
            if (!DeviceExtension->Mdl)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            _SEH2_TRY
            {
                MmProbeAndLockPages(DeviceExtension->Mdl, UserMode, IoModifyAccess);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
                IoFreeMdl(DeviceExtension->Mdl);
                DeviceExtension->Mdl = NULL;
                break;
            } _SEH2_END;

            ResultBuffer = DeviceExtension->ResultBuffer = MmGetSystemAddressForMdlSafe(DeviceExtension->Mdl, NormalPagePriority);
            IoStackLocation->FileObject->FsContext = DeviceExtension->Mdl;

            DPRINT("DriverIoControl. ResultBuffer: %ld %ld %ld %ld\n",
                    ResultBuffer->Successes, ResultBuffer->Failures,
                    ResultBuffer->LogBufferLength, ResultBuffer->LogBufferMaxLength);
            break;
        }
        case IOCTL_KMTEST_USERMODE_AWAIT_REQ:
        {
            PLIST_ENTRY Entry;
            PKMT_USER_WORK_ENTRY WorkItem;

            DPRINT("DriverIoControl. IOCTL_KMTEST_USERMODE_AWAIT_REQ, len=%lu\n",
                    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            /* TODO: prevent multiple concurrent invocations */
            Status = KeWaitForSingleObject(&WorkList.NewWorkEvent, UserRequest, UserMode, FALSE, NULL);
            if (Status == STATUS_USER_APC || Status == STATUS_KERNEL_APC)
                break;

            if (IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength < sizeof(KMT_CALLBACK_REQUEST_PACKET))
            {
                Status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            ASSERT(!IsListEmpty(&WorkList.ListHead));

            Entry = WorkList.ListHead.Flink;
            WorkItem = CONTAINING_RECORD(Entry, KMT_USER_WORK_ENTRY, ListEntry);

            Length = sizeof(WorkItem->Request);
            RtlCopyMemory(Irp->AssociatedIrp.SystemBuffer, &WorkItem->Request, Length);
            Status = STATUS_SUCCESS;

            KeClearEvent(&WorkList.NewWorkEvent);
            break;

        }
        case IOCTL_KMTEST_USERMODE_SEND_RESPONSE:
        {
            PLIST_ENTRY Entry;
            PKMT_USER_WORK_ENTRY WorkEntry;
            PVOID Response;
            ULONG ResponseSize = IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength;

            DPRINT("DriverIoControl. IOCTL_KMTEST_USERMODE_SEND_RESPONSE, inlen=%lu, outlen=%lu\n",
                    IoStackLocation->Parameters.DeviceIoControl.InputBufferLength,
                    IoStackLocation->Parameters.DeviceIoControl.OutputBufferLength);

            if (IoStackLocation->Parameters.DeviceIoControl.InputBufferLength != sizeof(ULONG) || ResponseSize != sizeof(KMT_RESPONSE))
            {
                Status = STATUS_INVALID_BUFFER_SIZE;
                break;
            }

            /* FIXME: don't misuse the output buffer as an input! */
            Response = MmGetSystemAddressForMdlSafe(Irp->MdlAddress, NormalPagePriority);
            if (Response == NULL)
            {
                Status = STATUS_INSUFFICIENT_RESOURCES;
                break;
            }

            ExAcquireFastMutex(&WorkList.Lock);

            Status = STATUS_OBJECTID_NOT_FOUND;

            Entry = WorkList.ListHead.Flink;
            while (Entry != &WorkList.ListHead)
            {
                WorkEntry = CONTAINING_RECORD(Entry, KMT_USER_WORK_ENTRY, ListEntry);
                if (WorkEntry->Request.RequestId == *(PULONG)Irp->AssociatedIrp.SystemBuffer)
                {
                    WorkEntry->Response = ExAllocatePoolWithTag(PagedPool, sizeof(KMT_RESPONSE), 'pseR');
                    if (WorkEntry->Response == NULL)
                    {
                        Status = STATUS_INSUFFICIENT_RESOURCES;
                        break;
                    }

                    RtlCopyMemory(WorkEntry->Response, Response, ResponseSize);
                    KeSetEvent(&WorkEntry->WorkDoneEvent, IO_NO_INCREMENT, FALSE);
                    Status = STATUS_SUCCESS;
                    break;
                }

                Entry = Entry->Flink;
            }

            ExReleaseFastMutex(&WorkList.Lock);

            break;
        }
        default:
            DPRINT1("DriverIoControl. Invalid IoCtl code 0x%08X\n",
                     IoStackLocation->Parameters.DeviceIoControl.IoControlCode);
            Status = STATUS_INVALID_DEVICE_REQUEST;
            break;
    }

    Irp->IoStatus.Status = Status;
    Irp->IoStatus.Information = Length;

    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    return Status;
}

/**
 * @name KmtUserModeCallback
 *
 * Enqueue a request to the usermode callback queue and blocks until the work
 * is finished.
 *
 * @param Operation
 *        TODO
 * @param Parameters
 *        TODO
 *        TODO: why is this PVOID?
 *
 * @return Response from user mode
 */
PKMT_RESPONSE
KmtUserModeCallback(
    IN KMT_CALLBACK_INFORMATION_CLASS Operation,
    IN PVOID Parameters)
{
    PKMT_RESPONSE Result;
    NTSTATUS Status;
    PKMT_USER_WORK_ENTRY WorkEntry;
    LARGE_INTEGER Timeout;

    PAGED_CODE();

    WorkEntry = ExAllocatePoolWithTag(PagedPool, sizeof(KMT_USER_WORK_ENTRY), 'ekrW');
    if (WorkEntry == NULL)
        return NULL;

    KeInitializeEvent(&WorkEntry->WorkDoneEvent, NotificationEvent, FALSE);
    WorkEntry->Request.RequestId = RequestId++;
    WorkEntry->Request.OperationClass = Operation;
    WorkEntry->Request.Parameters = Parameters;
    WorkEntry->Response = NULL;

    ExAcquireFastMutex(&WorkList.Lock);
    InsertTailList(&WorkList.ListHead, &WorkEntry->ListEntry);
    ExReleaseFastMutex(&WorkList.Lock);

    KeSetEvent(&WorkList.NewWorkEvent, IO_NO_INCREMENT, FALSE);

    Timeout.QuadPart = -10 * 1000 * 1000 * 10; //wait for 10 seconds
    Status = KeWaitForSingleObject(&WorkEntry->WorkDoneEvent, Executive, UserMode, FALSE, &Timeout);

    if (Status == STATUS_USER_APC || Status == STATUS_KERNEL_APC || Status == STATUS_TIMEOUT)
    {
        DPRINT1("Unexpected callback abortion! Reason: %lx\n", Status);
    }

    ExAcquireFastMutex(&WorkList.Lock);
    RemoveEntryList(&WorkEntry->ListEntry);
    ExReleaseFastMutex(&WorkList.Lock);

    Result = WorkEntry->Response;

    ExFreePoolWithTag(WorkEntry, 'ekrW');

    return Result;
}

/**
 * @name KmtFreeCallbackResponse
 *
 * TODO
 *
 * @param Response
 *        TODO
 */
VOID
KmtFreeCallbackResponse(
    PKMT_RESPONSE Response)
{
    PAGED_CODE();

    ExFreePoolWithTag(Response, 'pseR');
}

/**
 * @name KmtCleanUsermodeCallbacks
 *
 * TODO
 */
static
VOID
KmtCleanUsermodeCallbacks(VOID)
{
    PLIST_ENTRY Entry;

    PAGED_CODE();

    ExAcquireFastMutex(&WorkList.Lock);

    Entry = WorkList.ListHead.Flink;
    while (Entry != &WorkList.ListHead)
    {
        PKMT_USER_WORK_ENTRY WorkEntry = CONTAINING_RECORD(Entry, KMT_USER_WORK_ENTRY, ListEntry);
        if (WorkEntry->Response != NULL)
        {
            KmtFreeCallbackResponse(WorkEntry->Response);
        }

        Entry = Entry->Flink;

        ExFreePoolWithTag(WorkEntry, 'ekrW');
    }

    ExReleaseFastMutex(&WorkList.Lock);
}
