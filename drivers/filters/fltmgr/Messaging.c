/*
* PROJECT:         Filesystem Filter Manager
* LICENSE:         GPL - See COPYING in the top level directory
* FILE:            drivers/filters/fltmgr/Messaging.c
* PURPOSE:         Contains the routines to handle the comms layer
* PROGRAMMERS:     Ged Murphy (gedmurphy@reactos.org)
*/

/* INCLUDES ******************************************************************/

#include "fltmgr.h"
#include "fltmgrint.h"
#include <fltmgr_shared.h>

#define NDEBUG
#include <debug.h>


/* DATA *********************************************************************/

UNICODE_STRING CommsDeviceName = RTL_CONSTANT_STRING(L"\\FileSystem\\Filters\\FltMgrMsg");
PDEVICE_OBJECT CommsDeviceObject;

POBJECT_TYPE ServerPortObjectType;
POBJECT_TYPE ClientPortObjectType;

static
BOOLEAN
FltpDisconnectPort(
    _In_ PFLT_PORT_OBJECT PortObject
);

static
NTSTATUS
CreateClientPort(
    _In_ PFILE_OBJECT FileObject,
    _Inout_ PIRP Irp
);

static
NTSTATUS
CloseClientPort(
    _In_ PFILE_OBJECT FileObject,
    _Inout_ PIRP Irp
);

static
NTSTATUS
InitializeMessageWaiterQueue(
    _Inout_ PFLT_MESSAGE_WAITER_QUEUE MsgWaiterQueue
);

static
PPORT_CCB
CreatePortCCB(
    _In_ PFLT_PORT_OBJECT PortObject
);



/* EXPORTED FUNCTIONS ******************************************************/

_Must_inspect_result_
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FLTAPI
FltCreateCommunicationPort(_In_ PFLT_FILTER Filter,
                           _Outptr_ PFLT_PORT *ServerPort,
                           _In_ POBJECT_ATTRIBUTES ObjectAttributes,
                           _In_opt_ PVOID ServerPortCookie,
                           _In_ PFLT_CONNECT_NOTIFY ConnectNotifyCallback,
                           _In_ PFLT_DISCONNECT_NOTIFY DisconnectNotifyCallback,
                           _In_opt_ PFLT_MESSAGE_NOTIFY MessageNotifyCallback,
                           _In_ LONG MaxConnections)
{
    PFLT_SERVER_PORT_OBJECT PortObject;
    NTSTATUS Status;

    /* The caller must allow at least one connection */
    if (MaxConnections == 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* The request must be for a kernel handle */
    if (!(ObjectAttributes->Attributes & OBJ_KERNEL_HANDLE))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /*
     * Get rundown protection on the target to stop the owner
     * from unloading whilst this port object is open. It gets
     * removed in the FltpServerPortClose callback
     */
    Status = FltObjectReference(Filter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Create the server port object for this filter */
    Status = ObCreateObject(KernelMode,
                            ServerPortObjectType,
                            ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(FLT_SERVER_PORT_OBJECT),
                            0,
                            0,
                            (PVOID *)&PortObject);
    if (NT_SUCCESS(Status))
    {
        /* Zero out the struct */
        RtlZeroMemory(PortObject, sizeof(FLT_SERVER_PORT_OBJECT));

        /* Increment the ref count on the target filter */
        FltpObjectPointerReference((PFLT_OBJECT)Filter);

        /* Setup the filter port object */
        PortObject->Filter = Filter;
        PortObject->ConnectNotify = ConnectNotifyCallback;
        PortObject->DisconnectNotify = DisconnectNotifyCallback;
        PortObject->MessageNotify = MessageNotifyCallback;
        PortObject->Cookie = ServerPortCookie;
        PortObject->MaxConnections = MaxConnections;

        /* Insert the object */
        Status = ObInsertObject(PortObject,
                                NULL,
                                STANDARD_RIGHTS_ALL | FILE_READ_DATA,
                                0,
                                NULL,
                                (PHANDLE)ServerPort);
        if (NT_SUCCESS(Status))
        {
            /* Lock the connection list */
            ExAcquireFastMutex(&Filter->ConnectionList.mLock);

            /* Add the new port object to the connection list and increment the count */
            InsertTailList(&Filter->ConnectionList.mList, &PortObject->FilterLink);
            Filter->ConnectionList.mCount++;

            /* Unlock the connection list*/
            ExReleaseFastMutex(&Filter->ConnectionList.mLock);
        }
    }

    if (!NT_SUCCESS(Status))
    {
        /* Allow the filter to be cleaned up */
        FltObjectDereference(Filter);
    }

    return Status;
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FLTAPI
FltCloseCommunicationPort(_In_ PFLT_PORT ServerPort)
{
    /* Just close the handle to initiate the cleanup callbacks */
    ZwClose(ServerPort);
}

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FLTAPI
FltCloseClientPort(_In_ PFLT_FILTER Filter,
                   _Inout_ PFLT_PORT *ClientPort)
{
    PFLT_PORT Port;

    /* Protect against the handle being used whilst we're closing it */
    FltAcquirePushLockShared(&Filter->PortLock);

    /* Store the port handle while we have the lock held */
    Port = *ClientPort;

    if (*ClientPort)
    {
        /* Set the hadle to null */
        *ClientPort = NULL;
    }

    /* Unlock the port */
    FltReleasePushLock(&Filter->PortLock);

    if (Port)
    {
        /* Close the safe handle */
        ZwClose(Port);
    }
}

_Must_inspect_result_
_IRQL_requires_max_(APC_LEVEL)
NTSTATUS
FLTAPI
FltSendMessage(_In_ PFLT_FILTER Filter,
               _In_ PFLT_PORT *ClientPort,
               _In_reads_bytes_(SenderBufferLength) PVOID SenderBuffer,
               _In_ ULONG SenderBufferLength,
               _Out_writes_bytes_opt_(*ReplyLength) PVOID ReplyBuffer,
               _Inout_opt_ PULONG ReplyLength,
               _In_opt_ PLARGE_INTEGER Timeout)
{
    UNREFERENCED_PARAMETER(Filter);
    UNREFERENCED_PARAMETER(ClientPort);
    UNREFERENCED_PARAMETER(SenderBuffer);
    UNREFERENCED_PARAMETER(SenderBufferLength);
    UNREFERENCED_PARAMETER(ReplyBuffer);
    UNREFERENCED_PARAMETER(ReplyLength);
    UNREFERENCED_PARAMETER(Timeout);
    return STATUS_NOT_IMPLEMENTED;
}

/* INTERNAL FUNCTIONS ******************************************************/


NTSTATUS
FltpMsgCreate(_In_ PDEVICE_OBJECT DeviceObject,
              _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;

    /* Get the stack location */
    StackPtr = IoGetCurrentIrpStackLocation(Irp);

    FLT_ASSERT(StackPtr->MajorFunction == IRP_MJ_CREATE);

    /* Check if this is a caller wanting to connect */
    if (StackPtr->MajorFunction == IRP_MJ_CREATE)
    {
        /* Create the client port for this connection and exit */
        Status = CreateClientPort(StackPtr->FileObject, Irp);
    }
    else
    {
        Status = STATUS_INVALID_PARAMETER;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, 0);
    }

    return Status;
}

NTSTATUS
FltpMsgDispatch(_In_ PDEVICE_OBJECT DeviceObject,
                _Inout_ PIRP Irp)
{
    PIO_STACK_LOCATION StackPtr;
    NTSTATUS Status;

    /* Get the stack location */
    StackPtr = IoGetCurrentIrpStackLocation(Irp);

    /* Check if this is a caller wanting to connect */
    if (StackPtr->MajorFunction == IRP_MJ_CLOSE)
    {
        /* Create the client port for this connection and exit */
        Status = CloseClientPort(StackPtr->FileObject, Irp);
    }
    else
    {
        // We don't support anything else yet
        Status = STATUS_NOT_IMPLEMENTED;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        Irp->IoStatus.Information = 0;
        IoCompleteRequest(Irp, 0);
    }

    return Status;
}

VOID
NTAPI
FltpServerPortClose(_In_opt_ PEPROCESS Process,
                    _In_ PVOID Object,
                    _In_ ACCESS_MASK GrantedAccess,
                    _In_ ULONG ProcessHandleCount,
                    _In_ ULONG SystemHandleCount)
{
    PFLT_SERVER_PORT_OBJECT PortObject;
    PFAST_MUTEX Lock;

    /* Cast the object to a server port object */
    PortObject = (PFLT_SERVER_PORT_OBJECT)Object;

    /* Lock the connection list */
    Lock = &PortObject->Filter->ConnectionList.mLock;
    ExAcquireFastMutex(Lock);

    /* Remove the server port object from the list */
    RemoveEntryList(&PortObject->FilterLink);

    /* Unlock the connection list */
    ExReleaseFastMutex(Lock);

    /* Remove the rundown protection we added to stop the owner from tearing us down */
    FltObjectDereference(PortObject->Filter);
}

VOID
NTAPI
FltpServerPortDelete(PVOID Object)
{
    /* Decrement the filter count we added in the create routine */
    PFLT_SERVER_PORT_OBJECT PortObject = (PFLT_SERVER_PORT_OBJECT)Object;
    FltpObjectPointerDereference((PFLT_OBJECT)PortObject->Filter);

}

VOID
NTAPI
FltpClientPortClose(_In_opt_ PEPROCESS Process,
                    _In_ PVOID Object,
                    _In_ ACCESS_MASK GrantedAccess,
                    _In_ ULONG ProcessHandleCount,
                    _In_ ULONG SystemHandleCount)
{
    PFLT_PORT_OBJECT PortObject = (PFLT_PORT_OBJECT)Object;

    if (FltpDisconnectPort(PortObject))
    {
        InterlockedDecrement(&PortObject->ServerPort->NumberOfConnections);
    }
}

BOOLEAN
FltpDisconnectPort(_In_ PFLT_PORT_OBJECT PortObject)
{
    BOOLEAN Disconnected = FALSE;

    /* Lock the port object while we disconnect it */
    ExAcquireFastMutex(&PortObject->Lock);

    /* Make sure we have a valid connection */
    if (PortObject->Disconnected == FALSE)
    {
        /* Let any waiters know we're dusconnecing */
        KeSetEvent(&PortObject->DisconnectEvent, 0, 0);

        // cleanup everything in the message queue (PortObject->MsgQ.Csq)

        /* Set the disconnected state to true */
        PortObject->Disconnected = TRUE;
        Disconnected = TRUE;
    }

    /* Unlock and exit*/
    ExReleaseFastMutex(&PortObject->Lock);
    return Disconnected;
}

VOID
NTAPI
FltpClientPortDelete(PVOID Object)
{
    PFLT_PORT_OBJECT PortObject = (PFLT_PORT_OBJECT)Object;
    ObDereferenceObject(PortObject->ServerPort);
}


NTSTATUS
FltpSetupCommunicationObjects(_In_ PDRIVER_OBJECT DriverObject)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING SymLinkName;
    UNICODE_STRING Name;
    NTSTATUS Status;

    GENERIC_MAPPING Mapping =
    {
        STANDARD_RIGHTS_READ,
        STANDARD_RIGHTS_WRITE,
        STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
        FLT_PORT_ALL_ACCESS
    };

    /* Create the server comms object type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(OBJECT_TYPE_INITIALIZER));
    RtlInitUnicodeString(&Name, L"FilterConnectionPort");
    ObjectTypeInitializer.Length = sizeof(OBJECT_TYPE_INITIALIZER);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = Mapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(FLT_SERVER_PORT_OBJECT);
    ObjectTypeInitializer.ValidAccessMask = GENERIC_ALL;
    ObjectTypeInitializer.CloseProcedure = FltpServerPortClose;
    ObjectTypeInitializer.DeleteProcedure = FltpServerPortDelete;
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ServerPortObjectType);
    if (!NT_SUCCESS(Status)) return Status;

    /* Create the client comms object type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(OBJECT_TYPE_INITIALIZER));
    RtlInitUnicodeString(&Name, L"FilterCommunicationPort");
    ObjectTypeInitializer.Length = sizeof(OBJECT_TYPE_INITIALIZER);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.GenericMapping = Mapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(FLT_PORT_OBJECT);
    ObjectTypeInitializer.ValidAccessMask = GENERIC_ALL;
    ObjectTypeInitializer.CloseProcedure = FltpClientPortClose;
    ObjectTypeInitializer.DeleteProcedure = FltpClientPortDelete;
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ClientPortObjectType);
    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    /* Create the device object */
    Status = IoCreateDevice(DriverObject,
                            0,
                            &CommsDeviceName,
                            FILE_DEVICE_WPD,
                            0,
                            0,
                            &CommsDeviceObject);
    if (NT_SUCCESS(Status))
    {
        /* Setup a symbolic link for the device */
        RtlInitUnicodeString(&SymLinkName, L"\\DosDevices\\FltMgrMsg");
        Status = IoCreateSymbolicLink(&SymLinkName, &CommsDeviceName);
    }

Quit:
    if (!NT_SUCCESS(Status))
    {
        /* Something went wrong, undo */
        if (CommsDeviceObject)
        {
            IoDeleteDevice(CommsDeviceObject);
            CommsDeviceObject = NULL;
        }
        if (ClientPortObjectType)
        {
            ObMakeTemporaryObject(ClientPortObjectType);
            ObDereferenceObject(ClientPortObjectType);
            ClientPortObjectType = NULL;
        }

        if (ServerPortObjectType)
        {
            ObMakeTemporaryObject(ServerPortObjectType);
            ObDereferenceObject(ServerPortObjectType);
            ServerPortObjectType = NULL;
        }
    }

    return Status;
}

/* CSQ IRP CALLBACKS *******************************************************/


NTSTATUS
NTAPI
FltpAddMessageWaiter(_In_ PIO_CSQ Csq,
                     _In_ PIRP Irp,
                     _In_ PVOID InsertContext)
{
    PFLT_MESSAGE_WAITER_QUEUE MessageWaiterQueue;

    /* Get the start of the waiter queue struct */
    MessageWaiterQueue = CONTAINING_RECORD(Csq,
                                           FLT_MESSAGE_WAITER_QUEUE,
                                           Csq);

    /* Insert the IRP at the end of the queue */
    InsertTailList(&MessageWaiterQueue->WaiterQ.mList,
                   &Irp->Tail.Overlay.ListEntry);

    /* return success */
    return STATUS_SUCCESS;
}

VOID
NTAPI
FltpRemoveMessageWaiter(_In_ PIO_CSQ Csq,
                        _In_ PIRP Irp)
{
    /* Remove the IRP from the queue */
    RemoveEntryList(&Irp->Tail.Overlay.ListEntry);
}

PIRP
NTAPI
FltpGetNextMessageWaiter(_In_ PIO_CSQ Csq,
                         _In_ PIRP Irp,
                         _In_ PVOID PeekContext)
{
    PFLT_MESSAGE_WAITER_QUEUE MessageWaiterQueue;
    PIRP NextIrp = NULL;
    PLIST_ENTRY NextEntry;
    PIO_STACK_LOCATION IrpStack;

    /* Get the start of the waiter queue struct */
    MessageWaiterQueue = CONTAINING_RECORD(Csq,
                                           FLT_MESSAGE_WAITER_QUEUE,
                                           Csq);

    /* Is the IRP valid? */
    if (Irp == NULL)
    {
        /* Start peeking from the listhead */
        NextEntry = MessageWaiterQueue->WaiterQ.mList.Flink;
    }
    else
    {
        /* Start peeking from that IRP onwards */
        NextEntry = Irp->Tail.Overlay.ListEntry.Flink;
    }

    /* Loop through the queue */
    while (NextEntry != &MessageWaiterQueue->WaiterQ.mList)
    {
        /* Store the next IRP in the list */
        NextIrp = CONTAINING_RECORD(NextEntry, IRP, Tail.Overlay.ListEntry);

        /* Did we supply a PeekContext on insert? */
        if (!PeekContext)
        {
            /* We already have the next IRP */
            break;
        }
        else
        {
            /* Get the stack of the next IRP */
            IrpStack = IoGetCurrentIrpStackLocation(NextIrp);

            /* Does the PeekContext match the object? */
            if (IrpStack->FileObject == (PFILE_OBJECT)PeekContext)
            {
                /* We have a match */
                break;
            }

            /* Move to the next IRP */
            NextIrp = NULL;
            NextEntry = NextEntry->Flink;
        }
    }

    return NextIrp;
}

_Acquires_lock_(((PFLT_MESSAGE_WAITER_QUEUE)CONTAINING_RECORD(Csq, FLT_MESSAGE_WAITER_QUEUE, Csq))->WaiterQ.mLock)
_IRQL_saves_global_(Irql, ((PFLT_MESSAGE_WAITER_QUEUE)CONTAINING_RECORD(Csq, DEVICE_EXTENSION, IrpQueue))->WaiterQ.mLock)
_IRQL_raises_(DISPATCH_LEVEL)
VOID
NTAPI
FltpAcquireMessageWaiterLock(_In_ PIO_CSQ Csq,
                             _Out_ PKIRQL Irql)
{
    PFLT_MESSAGE_WAITER_QUEUE MessageWaiterQueue;

    UNREFERENCED_PARAMETER(Irql);

    /* Get the start of the waiter queue struct */
    MessageWaiterQueue = CONTAINING_RECORD(Csq,
                                           FLT_MESSAGE_WAITER_QUEUE,
                                           Csq);

    /* Acquire the IRP queue lock */
    ExAcquireFastMutex(&MessageWaiterQueue->WaiterQ.mLock);
}

_Releases_lock_(((PFLT_MESSAGE_WAITER_QUEUE)CONTAINING_RECORD(Csq, DEVICE_EXTENSION, IrpQueue))->WaiterQ.mLock)
_IRQL_restores_global_(Irql, ((PFLT_MESSAGE_WAITER_QUEUE)CONTAINING_RECORD(Csq, DEVICE_EXTENSION, IrpQueue))->WaiterQ.mLock)
_IRQL_requires_(DISPATCH_LEVEL)
VOID
NTAPI
FltpReleaseMessageWaiterLock(_In_ PIO_CSQ Csq,
                             _In_ KIRQL Irql)
{
    PFLT_MESSAGE_WAITER_QUEUE MessageWaiterQueue;

    UNREFERENCED_PARAMETER(Irql);

    /* Get the start of the waiter queue struct */
    MessageWaiterQueue = CONTAINING_RECORD(Csq,
                                           FLT_MESSAGE_WAITER_QUEUE,
                                           Csq);

    /* Release the IRP queue lock */
    ExReleaseFastMutex(&MessageWaiterQueue->WaiterQ.mLock);
}

VOID
NTAPI
FltpCancelMessageWaiter(_In_ PIO_CSQ Csq,
                        _In_ PIRP Irp)
{
    /* Cancel the IRP */
    Irp->IoStatus.Status = STATUS_CANCELLED;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
}


/* PRIVATE FUNCTIONS ******************************************************/

static
NTSTATUS
CreateClientPort(_In_ PFILE_OBJECT FileObject,
                   _Inout_ PIRP Irp)
{
    PFLT_SERVER_PORT_OBJECT ServerPortObject = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    PFILTER_PORT_DATA FilterPortData;
    PFLT_PORT_OBJECT ClientPortObject = NULL;
    PFLT_PORT PortHandle = NULL;
    PPORT_CCB PortCCB = NULL;
    //ULONG BufferLength;
    LONG NumConns;
    NTSTATUS Status;

    /* We received the buffer via FilterConnectCommunicationPort, cast it back to its original form */
    FilterPortData = Irp->AssociatedIrp.SystemBuffer;

    /* Get a reference to the server port the filter created */
    Status = ObReferenceObjectByName(&FilterPortData->PortName,
                                     0,
                                     0,
                                     FLT_PORT_ALL_ACCESS,
                                     ServerPortObjectType,
                                     ExGetPreviousMode(),
                                     0,
                                     (PVOID *)&ServerPortObject);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Increment the number of connections on the server port */
    NumConns = InterlockedIncrement(&ServerPortObject->NumberOfConnections);
    if (NumConns > ServerPortObject->MaxConnections)
    {
        Status = STATUS_CONNECTION_COUNT_LIMIT;
        goto Quit;
    }

    /* Initialize a basic kernel handle request */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);

    /* Now create the new client port object */
    Status = ObCreateObject(KernelMode,
                            ClientPortObjectType,
                            &ObjectAttributes,
                            KernelMode,
                            NULL,
                            sizeof(FLT_PORT_OBJECT),
                            0,
                            0,
                            (PVOID *)&ClientPortObject);
    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    /* Clear out the buffer */
    RtlZeroMemory(ClientPortObject, sizeof(FLT_PORT_OBJECT));

    /* Initialize the locks */
    ExInitializeRundownProtection(&ClientPortObject->MsgNotifRundownRef);
    ExInitializeFastMutex(&ClientPortObject->Lock);

    /* Set the server port object this belongs to */
    ClientPortObject->ServerPort = ServerPortObject;

    /* Setup the message queue */
    Status = InitializeMessageWaiterQueue(&ClientPortObject->MsgQ);
    if (!NT_SUCCESS(Status))
    {
        goto Quit;
    }

    /* Create the CCB which we'll attach to the file object */
    PortCCB = CreatePortCCB(ClientPortObject);
    if (PortCCB == NULL)
    {
        Status = STATUS_INSUFFICIENT_RESOURCES;
        goto Quit;
    }

    /* Now insert the new client port into the object manager */
    Status = ObInsertObject(ClientPortObject, NULL, FLT_PORT_ALL_ACCESS, 1, NULL, (PHANDLE)&PortHandle);
    if (!NT_SUCCESS(Status))
    {
        /* ObInsertObject() failed and already dereferenced ClientPortObject */
        ClientPortObject = NULL;
        goto Quit;
    }

    /* Add a reference to the filter to keep it alive while we do some work with it */
    Status = FltObjectReference(ServerPortObject->Filter);
    if (NT_SUCCESS(Status))
    {
        /* Invoke the callback to let the filter know we have a connection */
        Status = ServerPortObject->ConnectNotify(PortHandle,
                                                 ServerPortObject->Cookie,
                                                 NULL, //ConnectionContext
                                                 0, //SizeOfContext
                                                 &ClientPortObject->Cookie);
        if (NT_SUCCESS(Status))
        {
            /* Add the client port CCB to the file object */
            FileObject->FsContext2 = PortCCB;

            /* Lock the port list on the filter and add this new port object to the list */
            ExAcquireFastMutex(&ServerPortObject->Filter->PortList.mLock);
            InsertTailList(&ServerPortObject->Filter->PortList.mList, &ClientPortObject->FilterLink);
            ExReleaseFastMutex(&ServerPortObject->Filter->PortList.mLock);
        }

        /* We're done with the filter object, decremement the count */
        FltObjectDereference(ServerPortObject->Filter);
    }


Quit:
    if (!NT_SUCCESS(Status))
    {
        if (ClientPortObject)
        {
            ObDereferenceObject(ClientPortObject);
        }

        if (PortHandle)
        {
            ZwClose(PortHandle);
        }
        else if (ServerPortObject)
        {
            InterlockedDecrement(&ServerPortObject->NumberOfConnections);
            ObDereferenceObject(ServerPortObject);
        }

        if (PortCCB)
        {
            ExFreePoolWithTag(PortCCB, FM_TAG_CCB);
        }
    }

    return Status;
}

static
NTSTATUS
CloseClientPort(_In_ PFILE_OBJECT FileObject,
                _Inout_ PIRP Irp)
{
    PFLT_CCB Ccb;

    Ccb = (PFLT_CCB)FileObject->FsContext2;

    /* Remove the reference on the filter we added when we opened the port */
    ObDereferenceObject(Ccb->Data.Port.Port);

    // FIXME: Free the CCB

    return STATUS_SUCCESS;
}

static
NTSTATUS
InitializeMessageWaiterQueue(_Inout_ PFLT_MESSAGE_WAITER_QUEUE MsgWaiterQueue)
{
    NTSTATUS Status;

    /* Setup the IRP queue */
    Status = IoCsqInitializeEx(&MsgWaiterQueue->Csq,
                               FltpAddMessageWaiter,
                               FltpRemoveMessageWaiter,
                               FltpGetNextMessageWaiter,
                               FltpAcquireMessageWaiterLock,
                               FltpReleaseMessageWaiterLock,
                               FltpCancelMessageWaiter);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Initialize the waiter queue */
    ExInitializeFastMutex(&MsgWaiterQueue->WaiterQ.mLock);
    InitializeListHead(&MsgWaiterQueue->WaiterQ.mList);
    MsgWaiterQueue->WaiterQ.mCount = 0;

    /* We don't have a minimum waiter length */
    MsgWaiterQueue->MinimumWaiterLength = (ULONG)-1;

    /* Init the semaphore and event used for counting and signaling available IRPs */
    KeInitializeSemaphore(&MsgWaiterQueue->Semaphore, 0, MAXLONG);
    KeInitializeEvent(&MsgWaiterQueue->Event, NotificationEvent, FALSE);

    return STATUS_SUCCESS;
}

static
PPORT_CCB
CreatePortCCB(_In_ PFLT_PORT_OBJECT PortObject)
{
    PPORT_CCB PortCCB;

    /* Allocate a CCB struct to hold the client port object info */
    PortCCB = ExAllocatePoolWithTag(NonPagedPool, sizeof(PORT_CCB), FM_TAG_CCB);
    if (PortCCB)
    {
        /* Initialize the structure */
        PortCCB->Port = PortObject;
        PortCCB->ReplyWaiterList.mCount = 0;
        ExInitializeFastMutex(&PortCCB->ReplyWaiterList.mLock);
        KeInitializeEvent(&PortCCB->ReplyWaiterList.mLock.Event, SynchronizationEvent, 0);
    }

    return PortCCB;
}
