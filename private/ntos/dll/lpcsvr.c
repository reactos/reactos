//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       lpcsvr.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-12-97   RichardW   Created
//
//----------------------------------------------------------------------------

#include <ntos.h>
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include "lpcsvr.h"

#define RtlpLpcLockServer( s ) RtlEnterCriticalSection( &s->Lock );
#define RtlpLpcUnlockServer( s ) RtlLeaveCriticalSection( &s->Lock );

#define RtlpLpcContextFromClient( p ) ( CONTAINING_RECORD( p, LPCSVR_CONTEXT, PrivateContext ) )

//+---------------------------------------------------------------------------
//
//  Function:   RtlpLpcDerefContext
//
//  Synopsis:   Deref the context.  If this context is being cleaned up after
//              the server has been deleted, then the message is freed directly,
//              rather than being released to the general queue.
//
//  Arguments:  [Context] --
//              [Message] --
//
//  History:    2-06-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------
VOID
RtlpLpcDerefContext(
    PLPCSVR_CONTEXT Context,
    PLPCSVR_MESSAGE Message
    )
{
    PLPCSVR_SERVER Server ;

    Server = Context->Server ;

    if ( InterlockedDecrement( &Context->RefCount ) < 0 )
    {
        //
        // All gone, time to clean up:
        //

        RtlpLpcLockServer( Server );

        if ( Context->List.Flink )
        {
            RemoveEntryList( &Context->List );

            Server->ContextCount -- ;

        }
        else
        {
            if ( Message )
            {
                RtlFreeHeap( RtlProcessHeap(),
                             0,
                             Message );
            }
        }

        RtlpLpcUnlockServer( Server );

        if ( Context->CommPort )
        {
            NtClose( Context->CommPort );
        }

        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Context );
    }
    else
    {
        RtlpLpcLockServer( Server );

        Server->MessagePoolSize++ ;

        if ( Server->MessagePoolSize < Server->MessagePoolLimit )
        {
            Message->Header.Next = Server->MessagePool ;

            Server->MessagePool = Message ;
        }
        else
        {
            Server->MessagePoolSize-- ;

            RtlFreeHeap( RtlProcessHeap(),
                         0,
                         Message );

        }

        RtlpLpcUnlockServer( Server );
    }

}

//+---------------------------------------------------------------------------
//
//  Function:   RtlpLpcWorkerThread
//
//  Synopsis:   General worker thread
//
//  Arguments:  [Parameter] --
//
//  History:    2-06-98   RichardW   Created
//
//  Notes:
//
//----------------------------------------------------------------------------


VOID
RtlpLpcWorkerThread(
    PVOID Parameter
    )
{
    PLPCSVR_MESSAGE Message ;
    PLPCSVR_CONTEXT Context ;
    NTSTATUS Status ;
    BOOLEAN Accept ;

    Message = (PLPCSVR_MESSAGE) Parameter ;

    Context = Message->Header.Context ;

    switch ( Message->Message.u2.s2.Type & 0xF )
    {
        case LPC_REQUEST:
        case LPC_DATAGRAM:
            DbgPrint("Calling Server's Request function\n");
            Status = Context->Server->Init.RequestFn(
                                &Context->PrivateContext,
                                &Message->Message,
                                &Message->Message
                                );

            if ( NT_SUCCESS( Status ) )
            {
                Status = NtReplyPort( Context->CommPort,
                                      &Message->Message );

                if ( !NT_SUCCESS( Status ) )
                {
                    //
                    // See what happened.  The client may have gone away already.
                    //

                    break;

                }
            }
            break;

        case LPC_CONNECTION_REQUEST:
            DbgPrint("Calling Server's Connect function\n");
            Status = Context->Server->Init.ConnectFn(
                                &Context->PrivateContext,
                                &Message->Message,
                                &Accept
                                );

            //
            // If the comm port is still null, then do the accept.  Otherwise, the
            // server called RtlAcceptConnectPort() explicitly, to set up a view.
            //

            if ( NT_SUCCESS( Status ) )
            {
                if ( Context->CommPort == NULL )
                {
                    Status = NtAcceptConnectPort(
                                    &Context->CommPort,
                                    Context,
                                    &Message->Message,
                                    Accept,
                                    NULL,
                                    NULL );

                    if ( !Accept )
                    {
                        //
                        // Yank the context out of the list, since it is worthless
                        //

                        Context->RefCount = 0 ;

                    }
                    else
                    {
                        Status = NtCompleteConnectPort( Context->CommPort );
                    }
                }

            }
            else
            {
                Status = NtAcceptConnectPort(
                            &Context->CommPort,
                            NULL,
                            &Message->Message,
                            FALSE,
                            NULL,
                            NULL );

                Context->RefCount = 0 ;

            }

            break;

        case LPC_CLIENT_DIED:
            DbgPrint( "Calling Server's Rundown function\n" );
            Status = Context->Server->Init.RundownFn(
                                    &Context->PrivateContext,
                                    &Message->Message
                                    );

            InterlockedDecrement( &Context->RefCount );

            break;

        default:
            //
            // An unexpected message came through.  Normal LPC servers
            // don't handle the other types of messages.  Drop it.
            //

            break;
    }

    RtlpLpcDerefContext( Context, Message );

    return ;


}

VOID
RtlpLpcServerCallback(
    PVOID Parameter,
    BOOLEAN TimedOut
    )
{
    PLPCSVR_SERVER Server ;
    NTSTATUS Status ;
    PLPCSVR_MESSAGE Message ;
    PLPCSVR_CONTEXT Context ;
    PLARGE_INTEGER RealTimeout ;
    LPCSVR_FILTER_RESULT FilterResult ;

    Server = (PLPCSVR_SERVER) Parameter ;

    if ( Server->WaitHandle )
    {
        Server->WaitHandle = NULL ;
    }

    while ( 1 )
    {
        DbgPrint("Entering LPC server\n" );

        RtlpLpcLockServer( Server );

        if ( Server->Flags & LPCSVR_SHUTDOWN_PENDING )
        {
            break;
        }

        if ( Server->MessagePool )
        {
            Message = Server->MessagePool ;
            Server->MessagePool = Message->Header.Next ;
        }
        else
        {
            Message = RtlAllocateHeap( RtlProcessHeap(),
                                       0,
                                       Server->MessageSize );

        }

        RtlpLpcUnlockServer( Server );

        if ( !Message )
        {
            LARGE_INTEGER SleepInterval ;

            SleepInterval.QuadPart = 125 * 10000 ;

            NtDelayExecution( FALSE, &SleepInterval );
            continue;
        }


        if ( Server->Timeout.QuadPart )
        {
            RealTimeout = &Server->Timeout ;
        }
        else
        {
            RealTimeout = NULL ;
        }

        Status = NtReplyWaitReceivePortEx(
                        Server->Port,
                        &Context,
                        NULL,
                        &Message->Message,
                        RealTimeout );

        DbgPrint("Server: NtReplyWaitReceivePort completed with %x\n", Status );

        if ( NT_SUCCESS( Status ) )
        {
            //
            // If we timed out, nobody was waiting for us:
            //

            if ( Status == STATUS_TIMEOUT )
            {
                //
                // Set up a general wait that will call back to this function
                // when ready.
                //

                RtlpLpcLockServer( Server );

                if ( ( Server->Flags & LPCSVR_SHUTDOWN_PENDING ) == 0 )
                {

                    Status = RtlRegisterWait( &Server->WaitHandle,
                                              Server->Port,
                                              RtlpLpcServerCallback,
                                              Server,
                                              0xFFFFFFFF,
                                              WT_EXECUTEONLYONCE );
                }

                RtlpLpcUnlockServer( Server );

                break;

            }

            if ( Status == STATUS_SUCCESS )
            {
                if ( Context )
                {
                    InterlockedIncrement( &Context->RefCount );
                }
                else
                {
                    //
                    // New connection.  Create a new context record
                    //

                    Context = RtlAllocateHeap( RtlProcessHeap(),
                                               0,
                                               sizeof( LPCSVR_CONTEXT ) +
                                                    Server->Init.ContextSize );

                    if ( !Context )
                    {
                        HANDLE Bogus ;

                        Status = NtAcceptConnectPort(
                                    &Bogus,
                                    NULL,
                                    &Message->Message,
                                    FALSE,
                                    NULL,
                                    NULL );

                        RtlpLpcLockServer( Server );

                        Message->Header.Next = Server->MessagePool ;
                        Server->MessagePool = Message ;

                        RtlpLpcUnlockServer( Server );

                        continue;
                    }

                    Context->Server = Server ;
                    Context->RefCount = 1 ;
                    Context->CommPort = NULL ;

                    RtlpLpcLockServer( Server );

                    InsertTailList( &Server->ContextList, &Context->List );
                    Server->ContextCount++ ;

                    RtlpLpcUnlockServer( Server );
                }


                Message->Header.Context = Context ;

                FilterResult = LpcFilterAsync ;

                if ( Server->Init.FilterFn )
                {
                    FilterResult = Server->Init.FilterFn( Context, &Message->Message );

                    if (FilterResult == LpcFilterDrop )
                    {
                        RtlpLpcDerefContext( Context, Message );

                        continue;

                    }
                }

                if ( (Server->Flags & LPCSVR_SYNCHRONOUS) ||
                     (FilterResult == LpcFilterSync) )
                {
                    RtlpLpcWorkerThread( Message );
                }
                else
                {
                    RtlQueueWorkItem( RtlpLpcWorkerThread,
                                      Message,
                                      0 );

                }
            }
        }
        else
        {
            //
            // Error?  Better shut down...
            //

            break;
        }

    }

}

NTSTATUS
RtlCreateLpcServer(
    POBJECT_ATTRIBUTES PortName,
    PLPCSVR_INITIALIZE Init,
    PLARGE_INTEGER IdleTimeout,
    ULONG MessageSize,
    ULONG Options,
    PVOID * LpcServer
    )
{
    PLPCSVR_SERVER Server ;
    NTSTATUS Status ;
    HANDLE Thread ;
    CLIENT_ID Id ;

    *LpcServer = NULL ;

    Server = RtlAllocateHeap( RtlProcessHeap(),
                              0,
                              sizeof( LPCSVR_SERVER ) );

    if ( !Server )
    {
        return STATUS_INSUFFICIENT_RESOURCES ;
    }

    RtlInitializeCriticalSectionAndSpinCount(
            &Server->Lock,
            1000 );

    InitializeListHead( &Server->ContextList );
    Server->ContextCount = 0 ;

    Server->Init = *Init ;
    if ( !IdleTimeout )
    {
        Server->Timeout.QuadPart = 0 ;
    }
    else
    {
        Server->Timeout = *IdleTimeout ;
    }

    Server->MessageSize = MessageSize + sizeof( LPCSVR_MESSAGE ) -
                            sizeof( PORT_MESSAGE );

    Server->MessagePool = 0 ;
    Server->MessagePoolSize = 0 ;
    Server->MessagePoolLimit = 4 ;

    Server->Flags = Options ;

    //
    // Create the LPC port:
    //

    Status = NtCreateWaitablePort(
                            &Server->Port,
                            PortName,
                            MessageSize,
                            MessageSize,
                            MessageSize * 4
                            );

    if ( !NT_SUCCESS( Status ) )
    {
        RtlDeleteCriticalSection( &Server->Lock );
        RtlFreeHeap( RtlProcessHeap(), 0, Server );
        return Status ;
    }

    *LpcServer = Server ;

    //
    // Now, post the handle over to a wait queue
    //
    Status = RtlRegisterWait(
                        &Server->WaitHandle,
                        Server->Port,
                        RtlpLpcServerCallback,
                        Server,
                        0xFFFFFFFF,
                        WT_EXECUTEONLYONCE
                        );

    return Status ;
}


NTSTATUS
RtlShutdownLpcServer(
    PVOID LpcServer
    )
{
    PLPCSVR_SERVER Server ;
    OBJECT_ATTRIBUTES ObjA ;
    PLIST_ENTRY Scan ;
    PLPCSVR_CONTEXT Context ;
    PLPCSVR_MESSAGE Message ;
    NTSTATUS Status ;

    Server = (PLPCSVR_SERVER) LpcServer ;

    RtlpLpcLockServer( Server );

    if ( Server->Flags & LPCSVR_SHUTDOWN_PENDING )
    {
        RtlpLpcUnlockServer( Server );

        return STATUS_PENDING ;
    }

    if ( Server->WaitHandle )
    {
        RtlDeregisterWait( Server->WaitHandle );

        Server->WaitHandle = NULL ;
    }

    if ( Server->Timeout.QuadPart == 0 )
    {
        RtlpLpcUnlockServer( Server );

        return STATUS_NOT_IMPLEMENTED ;
    }

    //
    // If there are receives still pending, we have to sync
    // with those threads.  To do so, we will tag the shutdown
    // flag, and then wait the timeout amount.
    //

    if ( Server->ReceiveThreads != 0 )
    {

        InitializeObjectAttributes( &ObjA,
                                    NULL,
                                    0,
                                    0,
                                    0 );

        Status = NtCreateEvent( &Server->ShutdownEvent,
                                EVENT_ALL_ACCESS,
                                &ObjA,
                                NotificationEvent,
                                FALSE );

        if ( !NT_SUCCESS( Status ) )
        {
            RtlpLpcUnlockServer( Server );

            return Status ;

        }

        Server->Flags |= LPCSVR_SHUTDOWN_PENDING ;

        RtlpLpcUnlockServer( Server );

        Status = NtWaitForSingleObject(
                            Server->ShutdownEvent,
                            FALSE,
                            &Server->Timeout );

        if ( Status == STATUS_TIMEOUT )
        {
            //
            // Hmm, the LPC server thread is hung somewhere,
            // press on
            //
        }

        RtlpLpcLockServer( Server );

        NtClose( Server->ShutdownEvent );

        Server->ShutdownEvent = NULL ;

    }
    else
    {
        Server->Flags |= LPCSVR_SHUTDOWN_PENDING ;
    }

    //
    // The server object is locked, and there are no receives
    // pending.  Or, the receives appear hung.  Skim through the
    // context list, calling the server code.  The disconnect
    // message is NULL, indicating that this is a server initiated
    // shutdown.
    //


    while ( ! IsListEmpty( &Server->ContextList ) )
    {
        Scan = RemoveHeadList( &Server->ContextList );

        Context = CONTAINING_RECORD( Scan, LPCSVR_CONTEXT, List );

        Status = Server->Init.RundownFn(
                                Context->PrivateContext,
                                NULL );

        Context->List.Flink = NULL ;

        RtlpLpcDerefContext( Context, NULL );

    }

    //
    // All contexts have been deleted:  clean up the messages
    //

    while ( Server->MessagePool )
    {
        Message = Server->MessagePool ;

        Server->MessagePool = Message ;

        RtlFreeHeap( RtlProcessHeap(),
                     0,
                     Message );
    }


    //
    // Clean up server objects
    //

    return(STATUS_SUCCESS);

}

NTSTATUS
RtlImpersonateLpcClient(
    PVOID Context,
    PPORT_MESSAGE Message
    )
{
    PLPCSVR_CONTEXT LpcContext ;

    LpcContext = RtlpLpcContextFromClient( Context );

    return NtImpersonateClientOfPort(
                    LpcContext->CommPort,
                    Message );

}

NTSTATUS
RtlCallbackLpcClient(
    PVOID Context,
    PPORT_MESSAGE Request,
    PPORT_MESSAGE Callback
    )
{
    NTSTATUS Status ;
    PLPCSVR_CONTEXT LpcContext ;

    if ( Request != Callback )
    {
        Callback->ClientId = Request->ClientId ;
        Callback->MessageId = Request->MessageId ;
    }

    LpcContext = RtlpLpcContextFromClient( Context );

    Status = NtRequestWaitReplyPort(
                    LpcContext->CommPort,
                    Callback,
                    Callback
                    );

    return Status ;

}
