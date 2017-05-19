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

    /* Create our new server port object */
    Status = ObCreateObject(0,
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

    return STATUS_NOT_IMPLEMENTED;
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
    ObfDereferenceObject(PortObject->ServerPort);
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
            ObfDereferenceObject(ClientPortObjectType);
            ClientPortObjectType = NULL;
        }

        if (ServerPortObjectType)
        {
            ObMakeTemporaryObject(ServerPortObjectType);
            ObfDereferenceObject(ServerPortObjectType);
            ServerPortObjectType = NULL;
        }
    }

    return Status;
}

/* PRIVATE FUNCTIONS ******************************************************/

