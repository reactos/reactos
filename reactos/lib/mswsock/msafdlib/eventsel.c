/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS Winsock 2 SPI
 * FILE:        lib/mswsock/lib/init.c
 * PURPOSE:     DLL Initialization
 */

/* INCLUDES ******************************************************************/
#include "msafd.h"

/* DATA **********************************************************************/

typedef struct _SOCK_EVENT_MAPPING
{
    ULONG AfdBit;
    ULONG WinsockBit;
} SOCK_EVENT_MAPPING, *PSOCK_EVENT_MAPPING;

SOCK_EVENT_MAPPING PollEventMapping[] =
 {
    {AFD_EVENT_RECEIVE_BIT, FD_READ_BIT},
    {AFD_EVENT_SEND_BIT, FD_WRITE_BIT},
    {AFD_EVENT_OOB_RECEIVE_BIT, FD_OOB_BIT},
    {AFD_EVENT_ACCEPT_BIT, FD_ACCEPT_BIT},
    {AFD_EVENT_QOS_BIT, FD_QOS_BIT},
    {AFD_EVENT_GROUP_QOS_BIT, FD_GROUP_QOS_BIT},
    {AFD_EVENT_ROUTING_INTERFACE_CHANGE_BIT, FD_ROUTING_INTERFACE_CHANGE_BIT},
    {AFD_EVENT_ADDRESS_LIST_CHANGE_BIT, FD_ADDRESS_LIST_CHANGE_BIT}
};

/* FUNCTIONS *****************************************************************/

INT
WSPAPI
SockEventSelectHelper(IN PSOCKET_INFORMATION Socket,
                      IN WSAEVENT EventObject,
                      IN LONG Events)
{
    IO_STATUS_BLOCK    IoStatusBlock;
    AFD_EVENT_SELECT_INFO PollInfo;
    NTSTATUS Status;
    PWINSOCK_TEB_DATA ThreadData = NtCurrentTeb()->WinSockData;

    /* Acquire the lock */
    EnterCriticalSection(&Socket->Lock);

    /* Set Structure Info */
    PollInfo.EventObject = EventObject;
    PollInfo.Events = 0;

    /* Set receive event */
    if (Events & FD_READ) PollInfo.Events |= AFD_EVENT_RECEIVE;
    
    /* Set write event */
    if (Events & FD_WRITE) PollInfo.Events |= AFD_EVENT_SEND;
    
    /* Set out-of-band (OOB) receive event */
    if (Events & FD_OOB) PollInfo.Events |= AFD_EVENT_OOB_RECEIVE;
    
    /* Set accept event */
    if (Events & FD_ACCEPT) PollInfo.Events |= AFD_EVENT_ACCEPT;
    
    /* Send Quality-of-Service (QOS) event */
    if (Events & FD_QOS) PollInfo.Events |= AFD_EVENT_QOS;
    
    /* Send Group Quality-of-Service (QOS) event */
    if (Events & FD_GROUP_QOS) PollInfo.Events |= AFD_EVENT_GROUP_QOS;

    /* Send connect event. Note, this also includes connect failures */
    if (Events & FD_CONNECT) PollInfo.Events |= AFD_EVENT_CONNECT |
                                                AFD_EVENT_CONNECT_FAIL;

    /* Send close event. Note, this includes both aborts and disconnects */
    if (Events & FD_CLOSE) PollInfo.Events |= AFD_EVENT_DISCONNECT |
                                              AFD_EVENT_ABORT;

    /* Send PnP events related to live network hardware changes */
    if (Events & FD_ROUTING_INTERFACE_CHANGE)
    {
        PollInfo.Events |= AFD_EVENT_ROUTING_INTERFACE_CHANGE;
    }
    if (Events & FD_ADDRESS_LIST_CHANGE)
    {
        PollInfo.Events |= AFD_EVENT_ADDRESS_LIST_CHANGE;
    }

    /* Send IOCTL */
    Status = NtDeviceIoControlFile(Socket->WshContext.Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_EVENT_SELECT,
                                   &PollInfo,
                                   sizeof(PollInfo),
                                   NULL,
                                   0);
    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        LeaveCriticalSection(&Socket->Lock);
        return NtStatusToSocketError(Status);
    }

    /* Set Socket Data*/
    Socket->EventObject = EventObject;
    Socket->NetworkEvents = Events;

    /* Release lock and return success */
    LeaveCriticalSection(&Socket->Lock);
    return NO_ERROR;
}

INT 
WSPAPI 
WSPEventSelect(SOCKET Handle, 
               WSAEVENT hEventObject, 
               LONG lNetworkEvents, 
               LPINT lpErrno)
{
    PSOCKET_INFORMATION Socket;
    PWINSOCK_TEB_DATA ThreadData;
    INT ErrorCode;
    BOOLEAN BlockMode;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Set Socket to Non-Blocking */
    BlockMode = TRUE;
    ErrorCode = SockSetInformation(Socket,
                                   AFD_INFO_BLOCKING_MODE,
                                   &BlockMode,
                                   NULL,
                                   NULL);
    if (ErrorCode != NO_ERROR) goto error;

    /* AFD was notified, set it locally as well */
    Socket->SharedData.NonBlocking = TRUE;

    /* Check if there is an async select in progress */
    if (Socket->EventObject)
    {
        /* Lock the socket */
        EnterCriticalSection(&Socket->Lock);

        /* Erase all data */
        Socket->SharedData.hWnd = NULL;
        Socket->SharedData.wMsg = 0;
        Socket->SharedData.AsyncEvents = 0;

        /* Unbalance the sequence number so the request will fail */
        Socket->SharedData.SequenceNumber++;

        /* Give socket access back */
        LeaveCriticalSection(&Socket->Lock);
    }

    /* Make sure the flags are valid */
    if ((lNetworkEvents & ~FD_ALL_EVENTS))
    {
        /* More then the possible combination, fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Call the helper */
    ErrorCode = SockEventSelectHelper(Socket, hEventObject, lNetworkEvents);

error:
    /* Dereference the socket, if we have one here */
    if (Socket) SockDereferenceSocket(Socket);

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}

INT
WSPAPI
WSPEnumNetworkEvents(IN SOCKET Handle, 
                     IN WSAEVENT hEventObject, 
                     OUT LPWSANETWORKEVENTS lpNetworkEvents, 
                     OUT LPINT lpErrno)
{
    AFD_ENUM_NETWORK_EVENTS_INFO EventInfo;
    IO_STATUS_BLOCK IoStatusBlock;
    PSOCKET_INFORMATION Socket;
    PWINSOCK_TEB_DATA ThreadData;
    INT ErrorCode;
    NTSTATUS Status, EventStatus;
    PSOCK_EVENT_MAPPING EventMapping;
    ULONG i;

    /* Enter prolog */
    ErrorCode = SockEnterApiFast(&ThreadData);
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Get the socket structure */
    Socket = SockFindAndReferenceSocket(Handle, TRUE);
    if (!Socket)
    {
        /* Fail */
        ErrorCode = WSAENOTSOCK;
        goto error;
    }

    /* Make sure we got a pointer */
    if (!lpNetworkEvents)
    {
        /* Fail */
        ErrorCode = WSAEINVAL;
        goto error;
    }

    /* Lock the socket */
    EnterCriticalSection(&Socket->Lock);

    /* Send IOCTL */
    Status = NtDeviceIoControlFile((HANDLE)Handle,
                                   ThreadData->EventHandle,
                                   NULL,
                                   NULL,
                                   &IoStatusBlock,
                                   IOCTL_AFD_ENUM_NETWORK_EVENTS,
                                   hEventObject,
                                   0,
                                   &EventInfo,
                                   sizeof(EventInfo));
    
    /* Check if we need to wait */
    if (Status == STATUS_PENDING)
    {
        /* Wait for completion */
        SockWaitForSingleObject(ThreadData->EventHandle,
                                Socket->Handle,
                                NO_BLOCKING_HOOK,
                                NO_TIMEOUT);

        /* Get new status */
        Status = IoStatusBlock.Status;
    }

    /* Check for error */
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        ErrorCode = NtStatusToSocketError(Status);
        goto error;
    }

    /* Set Events to wait for */
    lpNetworkEvents->lNetworkEvents = 0;

    /* Set our Event Mapping structure */
    EventMapping = PollEventMapping;

    /* Loop it */
    for (i = 0; i < (sizeof(PollEventMapping) / 2 * sizeof(ULONG)); i++)
    {
        /* First check if we have a match for this bit */
        if (EventInfo.PollEvents & (1 << EventMapping->AfdBit))
        {
            /* Match found, write the equivalent bit */
            lpNetworkEvents->lNetworkEvents |= (1 << EventMapping->WinsockBit);

            /* Now get the status */
            EventStatus = EventInfo.EventStatus[EventMapping->AfdBit];

            /* Check if it failed */
            if (!NT_SUCCESS(Status))
            {
                /* Write the Winsock status code directly */
                lpNetworkEvents->iErrorCode[EventMapping->WinsockBit] = NtStatusToSocketError(EventStatus);
            }
            else
            {
                /* Write success */
                lpNetworkEvents->iErrorCode[EventMapping->WinsockBit] = NO_ERROR;
            }

            /* Move to the next mapping array */
            EventMapping++;
        }

        /* Handle the special cases with two flags. Start with connect */
        if (EventInfo.PollEvents & AFD_EVENT_CONNECT)
        {
            /* Set the equivalent bit */
            lpNetworkEvents->lNetworkEvents |= FD_CONNECT;

            /* Now get the status */
            EventStatus = EventInfo.EventStatus[AFD_EVENT_CONNECT_BIT];

            /* Check if it failed */
            if (!NT_SUCCESS(Status))
            {
                /* Write the Winsock status code directly */
                lpNetworkEvents->iErrorCode[FD_CONNECT_BIT] = NtStatusToSocketError(EventStatus);
            }
            else
            {
                /* Write success */
                lpNetworkEvents->iErrorCode[FD_CONNECT_BIT] = NO_ERROR;
            }
        }
        else if (EventInfo.PollEvents & AFD_EVENT_CONNECT_FAIL)
        {
            /* Do the same thing, but for the failure */
            lpNetworkEvents->lNetworkEvents |= FD_CONNECT;

            /* Now get the status */
            EventStatus = EventInfo.EventStatus[AFD_EVENT_CONNECT_FAIL_BIT];

            /* Check if it failed */
            if (!NT_SUCCESS(Status))
            {
                /* Write the Winsock status code directly */
                lpNetworkEvents->iErrorCode[FD_CONNECT_BIT] = NtStatusToSocketError(EventStatus);
            }
            else
            {
                /* Write success */
                lpNetworkEvents->iErrorCode[FD_CONNECT_BIT] = NO_ERROR;
            }
        }

        /* Now handle Abort/Disconnect */
        if (EventInfo.PollEvents & AFD_EVENT_ABORT)
        {
            /* Set the equivalent bit */
            lpNetworkEvents->lNetworkEvents |= FD_CLOSE;

            /* Now get the status */
            EventStatus = EventInfo.EventStatus[AFD_EVENT_ABORT_BIT];

            /* Check if it failed */
            if (!NT_SUCCESS(Status))
            {
                /* Write the Winsock status code directly */
                lpNetworkEvents->iErrorCode[FD_CLOSE_BIT] = NtStatusToSocketError(EventStatus);
            }
            else
            {
                /* Write success */
                lpNetworkEvents->iErrorCode[FD_CLOSE_BIT] = NO_ERROR;
            }
        }
        else if (EventInfo.PollEvents & AFD_EVENT_DISCONNECT)
        {
            /* Do the same thing, but for the failure */
            lpNetworkEvents->lNetworkEvents |= FD_CLOSE;

            /* Now get the status */
            EventStatus = EventInfo.EventStatus[AFD_EVENT_DISCONNECT_BIT];

            /* Check if it failed */
            if (!NT_SUCCESS(Status))
            {
                /* Write the Winsock status code directly */
                lpNetworkEvents->iErrorCode[FD_CLOSE_BIT] = NtStatusToSocketError(EventStatus);
            }
            else
            {
                /* Write success */
                lpNetworkEvents->iErrorCode[FD_CLOSE_BIT] = NO_ERROR;
            }
        }
    }

error:
    /* Dereference the socket, if we have one here */
    if (Socket) SockDereferenceSocket(Socket);

    /* Check for error */
    if (ErrorCode != NO_ERROR)
    {
        /* Fail */
        *lpErrno = ErrorCode;
        return SOCKET_ERROR;
    }

    /* Return success */
    return NO_ERROR;
}