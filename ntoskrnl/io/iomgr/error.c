/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/io/error.c
 * PURPOSE:         I/O Error Functions and Error Log Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */
/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* TYPES *********************************************************************/

typedef struct _IOP_ERROR_LOG_WORKER_DPC
{
    KDPC Dpc;
    KTIMER Timer;
} IOP_ERROR_LOG_WORKER_DPC, *PIOP_ERROR_LOG_WORKER_DPC;

/* GLOBALS *******************************************************************/

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
    UNICODE_STRING PortName = RTL_CONSTANT_STRING(L"\\ErrorLogPort");
    NTSTATUS Status;

    /* Make sure we're not already connected */
    if (IopLogPortConnected) return TRUE;

    /* Connect the port */
    Status = ZwConnectPort(&IopLogPort,
                           &PortName,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL,
                           NULL);
    if (NT_SUCCESS(Status))
    {
        /* Remmeber we're connected */
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
    PELF_API_MSG Message;
    PIO_ERROR_LOG_MESSAGE ErrorMessage;
    PLIST_ENTRY ListEntry;
    PERROR_LOG_ENTRY LogEntry;
    PIO_ERROR_LOG_PACKET Packet;
    PCHAR StringBuffer;
    ULONG RemainingLength;
    PDRIVER_OBJECT DriverObject;
    ULONG DriverNameLength = 0, DeviceNameLength;
    UNICODE_STRING DriverNameString;
    NTSTATUS Status;
    UCHAR Buffer[256];
    POBJECT_NAME_INFORMATION ObjectNameInfo = (POBJECT_NAME_INFORMATION)&Buffer;
    POBJECT_NAME_INFORMATION PoolObjectNameInfo = NULL;
    ULONG ReturnedLength, MessageLength;
    PWCHAR p;
    ULONG ExtraStringLength;
    PAGED_CODE();

    /* Connect to the port */
    if (!IopConnectLogPort()) return;

    /* Allocate the message */
    Message = ExAllocatePool(PagedPool, IO_ERROR_LOG_MESSAGE_LENGTH);
    if (!Message)
    {
        /* Couldn't allocate, try again */
        IopRestartLogWorker();
        return;
    }

    /* Copy the message */
    RtlZeroMemory(Message, sizeof(ELF_API_MSG));

    /* Get the actual I/O Structure */
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
        StringBuffer = (PVOID)ALIGN_UP(StringBuffer, WCHAR);

        /* Set the offset for the driver's name to the current buffer */
        ErrorMessage->DriverNameOffset = (ULONG)(StringBuffer -
                                                 (ULONG_PTR)ErrorMessage);

        /* Check how much space we have left for the device string */
        RemainingLength = (ULONG)((ULONG_PTR)Message +
                                  IO_ERROR_LOG_MESSAGE_LENGTH -
                                  (ULONG_PTR)StringBuffer);

        /* Now check if there is a driver object */
        DriverObject = LogEntry->DriverObject;
        if (DriverObject)
        {
            /* Check if the driver has a name */
            if (DriverObject->DriverName.Buffer)
            {
                /* Use its name */
                DriverNameString.Buffer = DriverObject->DriverName.Buffer;
                DriverNameLength = DriverObject->DriverName.Length;
            }
            else
                DriverNameString.Buffer = NULL;

            /* Check if there isn't a valid name*/
            if (!DriverNameLength)
            {
                /* Query the name directly */
                Status = ObQueryNameString(DriverObject,
                                           ObjectNameInfo,
                                           sizeof(Buffer),
                                           &ReturnedLength);
                if (!(NT_SUCCESS(Status)) || !(ObjectNameInfo->Name.Length))
                {
                    /* We don't have a name */
                    DriverNameLength = 0;
                }
            }
        }
        else
        {
            /* Use default name */
            DriverNameString.Buffer = L"Application Popup";
            DriverNameLength = wcslen(DriverNameString.Buffer) * sizeof(WCHAR);
        }

        /* Check if we have a driver name by here */
        if (DriverNameLength)
        {
            /* Skip to the end of the driver's name */
            p = &DriverNameString.Buffer[DriverNameLength / sizeof(WCHAR)];

            /* Now we'll walk backwards and assume the minimum size */
            DriverNameLength = sizeof(WCHAR);
            p--;
            while ((*p != L'\\') && (p != DriverNameString.Buffer))
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
             * Now make sure that the driver name fits in our buffer, minus 3
             * NULL chars, and copy the name in our string buffer
             */
            DriverNameLength = min(DriverNameLength,
                                   RemainingLength - 3 * sizeof(UNICODE_NULL));
            RtlCopyMemory(StringBuffer, p, DriverNameLength);
        }

        /* Null-terminate the driver name */
        *((PWSTR)(StringBuffer + DriverNameLength)) = L'\0';
        DriverNameLength += sizeof(WCHAR);

        /* Go to the next string buffer position */
        StringBuffer += DriverNameLength;
        RemainingLength -= DriverNameLength;

        /* Update the string offset and check if we have a device object */
        ErrorMessage->EntryData.StringOffset = (USHORT)
                                               ((ULONG_PTR)StringBuffer -
                                               (ULONG_PTR)ErrorMessage);
        if (LogEntry->DeviceObject)
        {
            /* We do, query its name */
            Status = ObQueryNameString(LogEntry->DeviceObject,
                                       ObjectNameInfo,
                                       sizeof(OBJECT_NAME_INFORMATION) +
                                       100 -
                                       DriverNameLength,
                                       &ReturnedLength);
            if ((!NT_SUCCESS(Status)) || !(ObjectNameInfo->Name.Length))
            {
                /* Setup an empty name */
                ObjectNameInfo->Name.Length = 0;
                ObjectNameInfo->Name.Buffer = L"";

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
                            ObjectNameInfo->Name.Length = 100 -
                                                          DriverNameLength;
                        }
                    }
                }
            }
        }
        else
        {
            /* No device object, setup an empty name */
            ObjectNameInfo->Name.Length = 0;
            ObjectNameInfo->Name.Buffer = L"";
        }

        /*
         * Now make sure that the device name fits in our buffer, minus 2
         * NULL chars, and copy the name in our string buffer
         */
        DeviceNameLength = min(ObjectNameInfo->Name.Length,
                               RemainingLength - 2 * sizeof(UNICODE_NULL));
        RtlCopyMemory(StringBuffer,
                      ObjectNameInfo->Name.Buffer,
                      DeviceNameLength);

        /* Null-terminate the device name */
        *((PWSTR)(StringBuffer + DeviceNameLength)) = L'\0';
        DeviceNameLength += sizeof(WCHAR);

        /* Free the buffer if we had one */
        if (PoolObjectNameInfo) ExFreePool(PoolObjectNameInfo);

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
            *((PWSTR)(StringBuffer + ExtraStringLength)) = L'\0';
        }

        /* Set the driver name length */
        ErrorMessage->DriverNameLength = (USHORT)DriverNameLength;

        /* Update the message length to include the device and driver names */
        MessageLength += DeviceNameLength + DriverNameLength;
        ErrorMessage->Size = (USHORT)MessageLength;

        /* Now update it again, internally, for the size of the actual LPC */
        MessageLength += (FIELD_OFFSET(ELF_API_MSG, IoErrorMessage) -
                          FIELD_OFFSET(ELF_API_MSG, Unknown[0]));

        /* Set the total and data lengths */
        Message->h.u1.s1.TotalLength = (USHORT)(sizeof(PORT_MESSAGE) +
                                                MessageLength);
        Message->h.u1.s1.DataLength = (USHORT)(MessageLength);

        /* Send the message */
        Status = NtRequestPort(IopLogPort, (PPORT_MESSAGE)Message);
        if (!NT_SUCCESS(Status))
        {
            /* Requeue log message and restart the worker */
            ExInterlockedInsertTailList(&IopErrorLogListHead,
                                        &LogEntry->ListEntry,
                                        &IopLogListLock);
            IopLogWorkerRunning = FALSE;
            IopRestartLogWorker();
            break;
        }

        /* Derefernece the device object */
        if (LogEntry->DeviceObject) ObDereferenceObject(LogEntry->DeviceObject);
        if (DriverObject) ObDereferenceObject(LogEntry->DriverObject);

        /* Update size */
        InterlockedExchangeAdd(&IopTotalLogSize,
                               -(LogEntry->Size - sizeof(ERROR_LOG_ENTRY)));
    }

    /* Free the LPC Message */
    ExFreePool(Message);
}

VOID
NTAPI
IopFreeApc(IN PKAPC Apc,
           IN PKNORMAL_ROUTINE *NormalRoutine,
           IN PVOID *NormalContext,
           IN PVOID *SystemArgument1,
           IN PVOID *SystemArgument2)
{
    /* Free the APC */
    ExFreePool(Apc);
}

VOID
NTAPI
IopRaiseHardError(IN PKAPC Apc,
                  IN PKNORMAL_ROUTINE *NormalRoutine,
                  IN PVOID *NormalContext,
                  IN PVOID *SystemArgument1,
                  IN PVOID *SystemArgument2)
{
    PIRP Irp = (PIRP)NormalContext;
    //PVPB Vpb = (PVPB)SystemArgument1;
    //PDEVICE_OBJECT DeviceObject = (PDEVICE_OBJECT)SystemArgument2;

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
    PDRIVER_OBJECT DriverObject;
    PDEVICE_OBJECT DeviceObject;

    /* Make sure we have an object */
    if (!IoObject) return NULL;

    /* Check if we're past our buffer */
    if (IopTotalLogSize > PAGE_SIZE) return NULL;

    /* Calculate the total size and allocate it */
    LogEntrySize = sizeof(ERROR_LOG_ENTRY) + EntrySize;
    LogEntry = ExAllocatePoolWithTag(NonPagedPool,
                                     LogEntrySize,
                                     TAG_ERROR_LOG);
    if (!LogEntry) return NULL;

    /* Check if this is a device object or driver object */
    if (((PDEVICE_OBJECT)IoObject)->Type == IO_TYPE_DEVICE)
    {
        /* It's a device, get the driver */
        DeviceObject = (PDEVICE_OBJECT)IoObject;
        DriverObject = DeviceObject->DriverObject;
    }
    else if (((PDEVICE_OBJECT)IoObject)->Type == IO_TYPE_DRIVER)
    {
        /* It's a driver, so we don' thave a device */
        DeviceObject = NULL;
        DriverObject = IoObject;
    }
    else
    {
        /* Fail */
        return NULL;
    }

    /* Reference the Objects */
    if (DeviceObject) ObReferenceObject(DeviceObject);
    if (DriverObject) ObReferenceObject(DriverObject);

    /* Update log size */
    InterlockedExchangeAdd(&IopTotalLogSize, EntrySize);

    /* Clear the entry and set it up */
    RtlZeroMemory(LogEntry, EntrySize);
    LogEntry->Type = IO_TYPE_ERROR_LOG;
    LogEntry->Size = EntrySize;
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

    /* Make sure there's an entry */
    if (!ElEntry) return;

    /* Get the actual header */
    LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry - sizeof(ERROR_LOG_ENTRY));

    /* Dereference both objects */
    if (LogEntry->DeviceObject) ObDereferenceObject(LogEntry->DeviceObject);
    if (LogEntry->DriverObject) ObDereferenceObject(LogEntry->DriverObject);

    /* Decrease total allocation size and free the entry */
    InterlockedExchangeAdd(&IopTotalLogSize,
                           -(LogEntry->Size - sizeof(ERROR_LOG_ENTRY)));
    ExFreePool(LogEntry);
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

    /* Get the main header */
    LogEntry = (PERROR_LOG_ENTRY)((ULONG_PTR)ElEntry -
                                  sizeof(ERROR_LOG_ENTRY));

    /* Get time stamp */
    KeQuerySystemTime(&LogEntry->TimeStamp);

    /* Acquire the lock and insert this write in the list */
    KeAcquireSpinLock(&IopLogListLock, &Irql);
    InsertHeadList(&IopErrorLogListHead, &LogEntry->ListEntry);

    /* Check if the worker is runnign */
    if (!IopLogWorkerRunning)
    {
#if 0
        /* It's not, initialize it and queue it */
        ExInitializeWorkItem(&IopErrorLogWorkItem,
                             IopLogWorker,
                             &IopErrorLogWorkItem);
        ExQueueWorkItem(&IopErrorLogWorkItem, DelayedWorkQueue);
        IopLogWorkerRunning = TRUE;
#endif
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
    PETHREAD Thread = (PETHREAD)&Irp->Tail.Overlay.Thread;
    PKAPC ErrorApc;

    /* Don't do anything if hard errors are disabled on the thread */
    if (Thread->HardErrorsAreDisabled)
    {
        /* Complete the request */
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, IO_DISK_INCREMENT);
        return;
    }

    /* Setup an APC */
    ErrorApc = ExAllocatePoolWithTag(NonPagedPool,
                                     sizeof(KAPC),
                                     TAG_APC);
    KeInitializeApc(ErrorApc,
                    &Thread->Tcb,
                    Irp->ApcEnvironment,
                    NULL,
                    (PKRUNDOWN_ROUTINE)IopFreeApc,
                    (PKNORMAL_ROUTINE)IopRaiseHardError,
                    KernelMode,
                    Irp);

    /* Queue an APC to deal with the error (see osr documentation) */
    KeInsertQueueApc(ErrorApc, Vpb, RealDeviceObject, 0);
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
    UNIMPLEMENTED;
    return(FALSE);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(IN BOOLEAN HardErrorEnabled)
{
    PETHREAD Thread = PsGetCurrentThread();
    BOOLEAN Old;

    /* Get the current value */
    Old = !Thread->HardErrorsAreDisabled;

    /* Set the new one and return the old */
    Thread->HardErrorsAreDisabled = !HardErrorEnabled;
    return Old;
}

/* EOF */
