/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/iomgr/error.c
 * PURPOSE:         I/O Error Functions and Error Log Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <subsys/iolog/iolog.h>

#define NDEBUG
#include <debug.h>

/* TYPES *********************************************************************/

typedef struct _IOP_ERROR_LOG_WORKER_DPC
{
    KDPC Dpc;
    KTIMER Timer;
} IOP_ERROR_LOG_WORKER_DPC, *PIOP_ERROR_LOG_WORKER_DPC;

/* GLOBALS *******************************************************************/

#define IOP_MAXIMUM_LOG_SIZE    (100 * PAGE_SIZE)
LONG IopTotalLogSize;
LIST_ENTRY IopErrorLogListHead;
KSPIN_LOCK IopLogListLock;

BOOLEAN IopLogWorkerRunning;
BOOLEAN IopLogPortConnected;
HANDLE IopLogPort;
WORK_QUEUE_ITEM IopErrorLogWorkItem;

PDEVICE_OBJECT IopErrorLogObject;

/* PRIVATE FUNCTIONS *********************************************************/

VOID
NTAPI
IopLogDpcRoutine(IN PKDPC Dpc,
                 IN PVOID DeferredContext,
                 IN PVOID SystemArgument1,
                 IN PVOID SystemArgument2)
{
    /* If we have a DPC, free it */
    if (Dpc) ExFreePool(Dpc);

    /* Initialize and queue the work item */
    ExInitializeWorkItem(&IopErrorLogWorkItem, IopLogWorker, NULL);
    ExQueueWorkItem(&IopErrorLogWorkItem, DelayedWorkQueue);
}

PLIST_ENTRY
NTAPI
IopGetErrorLogEntry(VOID)
{
    KIRQL OldIrql;
    PLIST_ENTRY ListEntry;

    /* Acquire the lock and check if the list is empty */
    KeAcquireSpinLock(&IopLogListLock, &OldIrql);
    if (IsListEmpty(&IopErrorLogListHead))
    {
        /* List is empty, disable the worker and return NULL */
        IopLogWorkerRunning = FALSE;
        ListEntry = NULL;
    }
    else
    {
        /* Otherwise, remove an entry */
        ListEntry = RemoveHeadList(&IopErrorLogListHead);
    }

    /* Release the lock and return the entry */
    KeReleaseSpinLock(&IopLogListLock, OldIrql);
    return ListEntry;
}

VOID
NTAPI
IopRestartLogWorker(VOID)
{
    PIOP_ERROR_LOG_WORKER_DPC WorkerDpc;
    LARGE_INTEGER Timeout;

    /* Allocate a DPC Context */
    WorkerDpc = ExAllocatePool(NonPagedPool, sizeof(IOP_ERROR_LOG_WORKER_DPC));
    if (!WorkerDpc)
    {
        /* Fail */
        IopLogWorkerRunning = FALSE;
        return;
    }

    /* Initialize DPC and Timer */
    KeInitializeDpc(&WorkerDpc->Dpc, IopLogDpcRoutine, WorkerDpc);
    KeInitializeTimer(&WorkerDpc->Timer);

    /* Restart after 30 seconds */
    Timeout.QuadPart = (LONGLONG)-300000000;
    KeSetTimer(&WorkerDpc->Timer, Timeout, &WorkerDpc->Dpc);
}

BOOLEAN
NTAPI
IopConnectLogPort(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING PortName = RTL_CONSTANT_STRING(ELF_PORT_NAME);
    SECURITY_QUALITY_OF_SERVICE SecurityQos;

    /* Make sure we're not already connected */
    if (IopLogPortConnected) return TRUE;

    /* Setup the QoS structure */
    SecurityQos.Length = sizeof(SecurityQos);
    SecurityQos.ImpersonationLevel = SecurityIdentification;
    SecurityQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    SecurityQos.EffectiveOnly = TRUE;

    /* Connect the port */
    Status = ZwConnectPort(&IopLogPort,
                           &PortName,
                           &SecurityQos,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL);
    if (NT_SUCCESS(Status))
    {
        /* Remember we're connected */
        IopLogPortConnected = TRUE;
        return TRUE;
    }

    /* We failed, try again */
    IopRestartLogWorker();
    return FALSE;
}

VOID
NTAPI
IopLogWorker(IN PVOID Parameter)
{
#define IO_ERROR_OBJECT_NAMES_LENGTH    100

    NTSTATUS Status;
    PELF_API_MSG Message;
    PIO_ERROR_LOG_MESSAGE ErrorMessage;
    PLIST_ENTRY ListEntry;
    PERROR_LOG_ENTRY LogEntry;
    PIO_ERROR_LOG_PACKET Packet;
    PCHAR StringBuffer;
    ULONG RemainingLength;
    PDRIVER_OBJECT DriverObject;
    PWCHAR NameString;
    ULONG DriverNameLength, DeviceNameLength;
    UCHAR Buffer[sizeof(OBJECT_NAME_INFORMATION) + IO_ERROR_OBJECT_NAMES_LENGTH];
    POBJECT_NAME_INFORMATION ObjectNameInfo = (POBJECT_NAME_INFORMATION)&Buffer;
    POBJECT_NAME_INFORMATION PoolObjectNameInfo;
    ULONG ReturnedLength, MessageLength;
    ULONG ExtraStringLength;
    PWCHAR p;

    PAGED_CODE();

    UNREFERENCED_PARAMETER(Parameter);

    /* Connect to the port */
    if (!IopConnectLogPort()) return;

    /* Allocate the message */
    Message = ExAllocatePoolWithTag(PagedPool, IO_ERROR_LOG_MESSAGE_LENGTH, TAG_IO);
    if (!Message)
    {
        /* Couldn't allocate, try again */
        IopRestartLogWorker();
        return;
    }

    /* Zero out the message and get the actual I/O structure */
    RtlZeroMemory(Message, sizeof(*Message));
    ErrorMessage = &Message->IoErrorMessage;

    /* Start loop */
    while (TRUE)
    {
        /* Get an entry */
        ListEntry = IopGetErrorLogEntry();
        if (!ListEntry) break;
        LogEntry = CONTAINING_RECORD(ListEntry, ERROR_LOG_ENTRY, ListEntry);

        /* Get pointer to the log packet */
        Packet = (PIO_ERROR_LOG_PACKET)((ULONG_PTR)LogEntry +
                                        sizeof(ERROR_LOG_ENTRY));

        /* Calculate the total length of the message only */
        MessageLength = sizeof(IO_ERROR_LOG_MESSAGE) -
                        sizeof(ERROR_LOG_ENTRY) -
                        sizeof(IO_ERROR_LOG_PACKET) +
                        LogEntry->Size;

        /* Copy the packet */
        RtlCopyMemory(&ErrorMessage->EntryData,
                      Packet,
                      LogEntry->Size - sizeof(ERROR_LOG_ENTRY));

        /* Set the timestamp and time */
        ErrorMessage->TimeStamp = LogEntry->TimeStamp;
        ErrorMessage->Type = IO_TYPE_ERROR_MESSAGE;

        /* Check if this message has any strings */
        if (Packet->NumberOfStrings)
        {
            /* String buffer is after the current strings */
            StringBuffer = (PCHAR)&ErrorMessage->EntryData +
                            Packet->StringOffset;
        }
        else
        {
            /* Otherwise, string buffer is at the end */
            StringBuffer = (PCHAR)ErrorMessage + MessageLength;
        }

        /* Align the buffer */
        StringBuffer = ALIGN_UP_POINTER(StringBuffer, WCHAR);

        /* Set the offset for the driver's name to the current buffer */
        ErrorMessage->DriverNameOffset = (ULONG)(StringBuffer -
                                                (PCHAR)ErrorMessage);

        /* Check how much space we have left for the device string */
        RemainingLength = (ULONG)((ULONG_PTR)Message +
                                  IO_ERROR_LOG_MESSAGE_LENGTH -
                                  (ULONG_PTR)StringBuffer);

        NameString = NULL;
        DriverNameLength = 0; DeviceNameLength = 0;
        PoolObjectNameInfo = NULL;
        ObjectNameInfo = (POBJECT_NAME_INFORMATION)&Buffer;

        /* Now check if there is a driver object */
        DriverObject = LogEntry->DriverObject;
        if (DriverObject)
        {
            /* Check if the driver has a name, and use it if so */
            if (DriverObject->DriverName.Buffer)
            {
                NameString = DriverObject->DriverName.Buffer;
                DriverNameLength = DriverObject->DriverName.Length;
            }
            else
            {
                NameString = NULL;
                DriverNameLength = 0;
            }

            /* Check if there isn't a valid name */
            if (!DriverNameLength)
            {
                /* Query the name directly */
                Status = ObQueryNameString(DriverObject,
                                           ObjectNameInfo,
                                           sizeof(Buffer),
                                           &ReturnedLength);
                if (!NT_SUCCESS(Status) || (ObjectNameInfo->Name.Length == 0))
                {
                    /* We don't have a name */
                    DriverNameLength = 0;
                }
                else
                {
                    NameString = ObjectNameInfo->Name.Buffer;
                    DriverNameLength = ObjectNameInfo->Name.Length;
                }
            }
        }
        else
        {
            /* Use default name */
            NameString = L"Application Popup";
            DriverNameLength = (ULONG)wcslen(NameString) * sizeof(WCHAR);
        }

        /* Check if we have a driver name */
        if (DriverNameLength)
        {
            /* Skip to the end of the driver's name */
            p = &NameString[DriverNameLength / sizeof(WCHAR)];

            /* Now we'll walk backwards and assume the minimum size */
            DriverNameLength = sizeof(WCHAR);
            p--;
            while ((*p != L'\\') && (p != NameString))
            {
                /* No backslash found, keep going */
                p--;
                DriverNameLength += sizeof(WCHAR);
            }

            /* Now we probably hit the backslash itself, skip past it */
            if (*p == L'\\')
            {
                p++;
                DriverNameLength -= sizeof(WCHAR);
            }

            /*
             * Now make sure that the driver name fits in the buffer, minus
             * 3 NULL chars (driver name, device name, and remaining strings),
             * and copy the driver name in the string buffer.
             */
            DriverNameLength = min(DriverNameLength,
                                   RemainingLength - 3 * sizeof(UNICODE_NULL));
            RtlCopyMemory(StringBuffer, p, DriverNameLength);
        }

        /* Null-terminate the driver name */
        *((PWSTR)(StringBuffer + DriverNameLength)) = UNICODE_NULL;
        DriverNameLength += sizeof(WCHAR);

        /* Go to the next string buffer position */
        StringBuffer += DriverNameLength;
        RemainingLength -= DriverNameLength;

        /* Update the string offset */
        ErrorMessage->EntryData.StringOffset =
            (USHORT)((ULONG_PTR)StringBuffer - (ULONG_PTR)ErrorMessage);

        /* Check if we have a device object */
        if (LogEntry->DeviceObject)
        {
            /* We do, query its name */
            Status = ObQueryNameString(LogEntry->DeviceObject,
                                       ObjectNameInfo,
                                       sizeof(Buffer) - DriverNameLength,
                                       &ReturnedLength);
            if (!NT_SUCCESS(Status) || (ObjectNameInfo->Name.Length == 0))
            {
                /* Setup an empty name */
                RtlInitEmptyUnicodeString(&ObjectNameInfo->Name, L"", 0);

                /* Check if we failed because our buffer wasn't large enough */
                if (Status == STATUS_INFO_LENGTH_MISMATCH)
                {
                    /* Then we'll allocate one... we really want this name! */
                    PoolObjectNameInfo = ExAllocatePoolWithTag(PagedPool,
                                                               ReturnedLength,
                                                               TAG_IO);
                    if (PoolObjectNameInfo)
                    {
                        /* Query it again */
                        ObjectNameInfo = PoolObjectNameInfo;
                        Status = ObQueryNameString(LogEntry->DeviceObject,
                                                   ObjectNameInfo,
                                                   ReturnedLength,
                                                   &ReturnedLength);
                        if (NT_SUCCESS(Status))
                        {
                            /* Success, update the information */
                            ObjectNameInfo->Name.Length =
                                IO_ERROR_OBJECT_NAMES_LENGTH - (USHORT)DriverNameLength;
                        }
                    }
                }
            }

            NameString = ObjectNameInfo->Name.Buffer;
            DeviceNameLength = ObjectNameInfo->Name.Length;
        }
        else
        {
            /* No device object, setup an empty name */
            NameString = L"";
            DeviceNameLength = 0;
        }

        /*
         * Now make sure that the device name fits in the buffer, minus
         * 2 NULL chars (device name, and remaining strings), and copy
         * the device name in the string buffer.
         */
        DeviceNameLength = min(DeviceNameLength,
                               RemainingLength - 2 * sizeof(UNICODE_NULL));
        RtlCopyMemory(StringBuffer, NameString, DeviceNameLength);

        /* Null-terminate the device name */
        *((PWSTR)(StringBuffer + DeviceNameLength)) = UNICODE_NULL;
        DeviceNameLength += sizeof(WCHAR);

        /* Free the buffer if we had one */
        if (PoolObjectNameInfo)
        {
            ExFreePoolWithTag(PoolObjectNameInfo, TAG_IO);
            PoolObjectNameInfo = NULL;
        }

        /* Go to the next string buffer position */
        ErrorMessage->EntryData.NumberOfStrings++;
        StringBuffer += DeviceNameLength;
        RemainingLength -= DeviceNameLength;

        /* Check if we have any extra strings */
        if (Packet->NumberOfStrings)
        {
            /* Find out the size of the extra strings */
            ExtraStringLength = LogEntry->Size -
                                sizeof(ERROR_LOG_ENTRY) -
                                Packet->StringOffset;

            /* Round up the length */
            ExtraStringLength = ROUND_UP(ExtraStringLength, sizeof(WCHAR));

            /* Make sure that the extra strings fit in our buffer */
            if (ExtraStringLength > (RemainingLength - sizeof(UNICODE_NULL)))
            {
                /* They wouldn't, so set normalize the length */
                MessageLength -= ExtraStringLength - RemainingLength;
                ExtraStringLength = RemainingLength - sizeof(UNICODE_NULL);
            }

            /* Now copy the extra strings */
            RtlCopyMemory(StringBuffer,
                          (PCHAR)Packet + Packet->StringOffset,
                          ExtraStringLength);

            /* Null-terminate them */
            *((PWSTR)(StringBuffer + ExtraStringLength)) = UNICODE_NULL;
        }

        /* Set the driver name length */
        ErrorMessage->DriverNameLength = (USHORT)DriverNameLength;

        /* Update the message length to include the driver and device names */
        MessageLength += DriverNameLength + DeviceNameLength;
        ErrorMessage->Size = (USHORT)MessageLength;

        /* Now update it again for the size of the actual LPC */
        MessageLength += (FIELD_OFFSET(ELF_API_MSG, IoErrorMessage) -
                          FIELD_OFFSET(ELF_API_MSG, Unknown[0]));

        /* Set the total and data lengths */
        Message->Header.u1.s1.TotalLength =
            (USHORT)(sizeof(PORT_MESSAGE) + MessageLength);
        Message->Header.u1.s1.DataLength = (USHORT)MessageLength;

        /* Send the message */
        Status = ZwRequestPort(IopLogPort, &Message->Header);
        if (!NT_SUCCESS(Status))
        {
            /*
             * An error happened while sending the message on the port.
             * Close the port, requeue the log message on top of the list
             * and restart the worker.
             */
            ZwClose(IopLogPort);
            IopLogPortConnected = FALSE;

            ExInterlockedInsertHeadList(&IopErrorLogListHead,
                                        &LogEntry->ListEntry,
                                        &IopLogListLock);

            IopRestartLogWorker();
            break;
        }

        /* NOTE: The following is basically 'IoFreeErrorLogEntry(Packet)' */

        /* Dereference both objects */
        if (LogEntry->DeviceObject) ObDereferenceObject(LogEntry->DeviceObject);
        if (LogEntry->DriverObject) ObDereferenceObject(LogEntry->DriverObject);

        /* Decrease the total allocation size and free the entry */
        InterlockedExchangeAdd(&IopTotalLogSize, -(LONG)LogEntry->Size);
        ExFreePoolWithTag(LogEntry, TAG_ERROR_LOG);
    }

    /* Free the LPC Message */
    ExFreePoolWithTag(Message, TAG_IO);
}

static
VOID
NTAPI
IopFreeApc(IN PKAPC Apc,
           IN OUT PKNORMAL_ROUTINE* NormalRoutine,
           IN OUT PVOID* NormalContext,
           IN OUT PVOID* SystemArgument1,
           IN OUT PVOID* SystemArgument2)
{
    PAGED_CODE();

    /* Free the APC */
    ExFreePoolWithTag(Apc, TAG_APC);
}

static
VOID
NTAPI
IopRaiseHardError(IN PVOID NormalContext,
                  IN PVOID SystemArgument1,
                  IN PVOID SystemArgument2)
{
    PIRP Irp = NormalContext;
    //PVPB Vpb = SystemArgument1;
    //PDEVICE_OBJECT DeviceObject = SystemArgument2;

    UNIMPLEMENTED;

    /* FIXME: UNIMPLEMENTED */
    Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_DISK_INCREMENT);
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
PVOID
NTAPI
IoAllocateErrorLogEntry(IN PVOID IoObject,
                        IN UCHAR EntrySize)
{
    PERROR_LOG_ENTRY LogEntry;
    ULONG LogEntrySize;
    PDEVICE_OBJECT DeviceObject;
    PDRIVER_OBJECT DriverObject;

    /* Make sure we have an object */
    if (!IoObject) return NULL;

    /* Check if this is a device object or driver object */
    DeviceObject = (PDEVICE_OBJECT)IoObject;
    if (DeviceObject->Type == IO_TYPE_DEVICE)
    {
        /* It's a device, get the driver */
        // DeviceObject = (PDEVICE_OBJECT)IoObject;
        DriverObject = DeviceObject->DriverObject;
    }
    else if (DeviceObject->Type == IO_TYPE_DRIVER)
    {
        /* It's a driver, so we don't have a device */
        DeviceObject = NULL;
        DriverObject = (PDRIVER_OBJECT)IoObject;
    }
    else
    {
        /* Fail */
        return NULL;
    }

    /* Check whether the size is too small or too large */
    if ((EntrySize < sizeof(IO_ERROR_LOG_PACKET)) ||
        (EntrySize > ERROR_LOG_MAXIMUM_SIZE))
    {
        /* Fail */
        return NULL;
    }

    /* Round up the size and calculate the total size */
    EntrySize = ROUND_UP(EntrySize, sizeof(PVOID));
    LogEntrySize = sizeof(ERROR_LOG_ENTRY) + EntrySize;

    /* Check if we're past our buffer */
    // TODO: Improve (what happens in case of concurrent calls?)
    if (IopTotalLogSize + LogEntrySize > IOP_MAXIMUM_LOG_SIZE) return NULL;

    /* Allocate the entry */
    LogEntry = ExAllocatePoolWithTag(NonPagedPool,
                                     LogEntrySize,
                                     TAG_ERROR_LOG);
    if (!LogEntry) return NULL;

    /* Reference the Objects */
    if (DeviceObject) ObReferenceObject(DeviceObject);
    if (DriverObject) ObReferenceObject(DriverObject);

    /* Update log size */
    InterlockedExchangeAdd(&IopTotalLogSize, LogEntrySize);

    /* Clear the entry and set it up */
    RtlZeroMemory(LogEntry, LogEntrySize);
    LogEntry->Type = IO_TYPE_ERROR_LOG;
    LogEntry->Size = LogEntrySize;
    LogEntry->DeviceObject = DeviceObject;
    LogEntry->DriverObject = DriverObject;

    /* Return the entry data */
    return (PVOID)((ULONG_PTR)LogEntry + sizeof(ERROR_LOG_ENTRY));
}

/*
 * @implemented
 */
VOID
NTAPI
IoFreeErrorLogEntry(IN PVOID ElEntry)
{
    PERROR_LOG_ENTRY LogEntry;

    /* Make sure there is an entry */
    if (!ElEntry) return;

    /* Get the actual header */
    LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry - sizeof(ERROR_LOG_ENTRY));

    /* Dereference both objects */
    if (LogEntry->DeviceObject) ObDereferenceObject(LogEntry->DeviceObject);
    if (LogEntry->DriverObject) ObDereferenceObject(LogEntry->DriverObject);

    /* Decrease the total allocation size and free the entry */
    InterlockedExchangeAdd(&IopTotalLogSize, -(LONG)LogEntry->Size);
    ExFreePoolWithTag(LogEntry, TAG_ERROR_LOG);
}

/*
 * @implemented
 */
VOID
NTAPI
IoWriteErrorLogEntry(IN PVOID ElEntry)
{
    PERROR_LOG_ENTRY LogEntry;
    KIRQL Irql;

    /* Make sure there is an entry */
    if (!ElEntry) return;

    /* Get the actual header */
    LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry - sizeof(ERROR_LOG_ENTRY));

    /* Get time stamp */
    KeQuerySystemTime(&LogEntry->TimeStamp);

    /* Acquire the lock and insert this write in the list */
    KeAcquireSpinLock(&IopLogListLock, &Irql);
    InsertHeadList(&IopErrorLogListHead, &LogEntry->ListEntry);

    /* Check if the worker is running */
    if (!IopLogWorkerRunning)
    {
        /* It's not, initialize it and queue it */
        IopLogWorkerRunning = TRUE;
        ExInitializeWorkItem(&IopErrorLogWorkItem, IopLogWorker, NULL);
        ExQueueWorkItem(&IopErrorLogWorkItem, DelayedWorkQueue);
    }

    /* Release the lock and return */
    KeReleaseSpinLock(&IopLogListLock, Irql);
}

/*
 * @implemented
 */
VOID
NTAPI
IoRaiseHardError(IN PIRP Irp,
                 IN PVPB Vpb,
                 IN PDEVICE_OBJECT RealDeviceObject)
{
    PETHREAD Thread = Irp->Tail.Overlay.Thread;
    PKAPC ErrorApc;

    /* Don't do anything if hard errors are disabled on the thread */
    if (Thread->HardErrorsAreDisabled)
    {
        /* Complete the request */
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        return;
    }

    // TODO: In case we were called in the context of a paging I/O or for
    // a synchronous operation, that happens with APCs disabled, queue the
    // hard-error call for later processing (see also IofCompleteRequest).

    /* Setup an APC and queue it to deal with the error (see OSR documentation) */
    ErrorApc = ExAllocatePoolWithTag(NonPagedPool, sizeof(*ErrorApc), TAG_APC);
    if (!ErrorApc)
    {
        /* Fail */
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        return;
    }

    KeInitializeApc(ErrorApc,
                    &Thread->Tcb,
                    Irp->ApcEnvironment,
                    IopFreeApc,
                    NULL,
                    IopRaiseHardError,
                    KernelMode,
                    Irp);

    KeInsertQueueApc(ErrorApc, Vpb, RealDeviceObject, IO_NO_INCREMENT);
}

/*
 * @unimplemented
 */
BOOLEAN
NTAPI
IoRaiseInformationalHardError(IN NTSTATUS ErrorStatus,
                              IN PUNICODE_STRING String,
                              IN PKTHREAD Thread)
{
    DPRINT1("IoRaiseInformationalHardError: %lx, '%wZ'\n", ErrorStatus, String);
#if DBG
    ASSERT(ErrorStatus != STATUS_FILE_CORRUPT_ERROR); /* CORE-17587 */
#endif
    return FALSE;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(IN BOOLEAN HardErrorEnabled)
{
    PETHREAD Thread = PsGetCurrentThread();
    BOOLEAN OldMode;

    /* Get the current value */
    OldMode = !Thread->HardErrorsAreDisabled;

    /* Set the new one and return the old */
    Thread->HardErrorsAreDisabled = !HardErrorEnabled;
    return OldMode;
}
