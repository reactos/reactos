//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//  Copyright (c) 1996 Intel Corporation
//
//  rsocket.cpp
//
//  Implementation of the RSOCKET class used by the restricted processes for
//  a TCP/IP sockets.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version. Modeled after dsocket.cpp
//                         example code.
//
//---------------------------------------------------------------------------

#include "precomp.h"
#include "globals.h"

// Variables to keep track of the sockets we have open
LIST_ENTRY            g_SocketList;
RTL_CRITICAL_SECTION  g_csSocketList;

//---------------------------------------------------------------------------
//  RSOCKET::RSOCKET()
//
//  RSOCKET object constructor. Note: RSOCKET::Initialize() does all of the
//  real initialization.
//---------------------------------------------------------------------------
RSOCKET::RSOCKET()
    {
    // Set our data members to known values
    m_Provider          = NULL;
    m_SocketHandle      = (SOCKET)SOCKET_ERROR;
    m_dwCatalogEntryId  = 0;
    m_dwContext         = INVALID_SOCKET;
    m_lAsyncEvents      = 0;
    m_hAsyncWindow      = NULL;
    m_AsyncSocket       = INVALID_SOCKET;
    m_uiAsyncMessage    = 0;
    m_hNetEvent         = 0;
    m_pIRestrictedProcess = 0;
    }

//---------------------------------------------------------------------------
//  RSOCKET::Initialize()
//
//  Completes the initialization of the RSOCKET object. This must be the
//  first member function called for the RSOCKET object. This procedure
//  should be called only once for the object.
//
//  Provider - Supplies  a  reference  to  the RPROVIDER object associated with
//             this RSOCKET instance.
//
//  ProviderSocket - The socket handle returned from the lower level provider.
//
//  CatalogEntryId - The CatalogEntryId for the provider referenced by
//                   m_Provider.
//
//  Context        - The socket handle returned from WPUCreateSocketHandle().
//
//  The  function returns NO_ERROR if successful. Otherwise it
//  returns an appropriate Winsock error code if the initialization
//  cannot be completed.
//---------------------------------------------------------------------------
INT RSOCKET::Initialize( IN PRPROVIDER          Provider,
                         IN SOCKET              ProviderSocket,
                         IN DWORD               dwCatalogEntryId,
                         IN ULONG_PTR            dwContext,
                         IN IRestrictedProcess *pIRestrictedProcess )
{
    // Store the provider and process object.
    m_Provider = Provider;
    m_SocketHandle = ProviderSocket;
    m_dwCatalogEntryId = dwCatalogEntryId;
    m_dwContext = dwContext;
    m_pIRestrictedProcess = pIRestrictedProcess;

    DEBUGF( DBG_TRACE,
            ("Initializing socket %X\n",this));

    return NO_ERROR;
}


//---------------------------------------------------------------------------
//  RSOCKET::~RSOCKET()
//
//  RSOCKET destructor. Cleanup before the RSOCKET instance is deleted.
//---------------------------------------------------------------------------
RSOCKET::~RSOCKET()
    {
    gWorkerThread->UnregisterSocket( this );

    if (m_pIRestrictedProcess)
       {
       m_pIRestrictedProcess->Release();   // Shouldn't need a loop...
       m_pIRestrictedProcess = 0;
       }

    DEBUGF( DBG_TRACE, ("Destroying socket %X\n",this));
    }


//---------------------------------------------------------------------------
//  RSOCKET::RegisterAsyncOperation()
//
//  Registers an event mask that will signal the specified window with the
//  specified message when one of the events occurs.
//
//  Returns NO_ERROR on success, or a Winsock error code.
//---------------------------------------------------------------------------
INT RSOCKET::RegisterAsyncOperation( HWND     Window,
                                     UINT     Message,
                                     LONG     lEvents )
    {
    INT ReturnCode;

    ReturnCode = WSAENOBUFS;

    if (lEvents)
        {
        //Create a WIN32 event to hand to the worker thread.
        if (NULL == m_hNetEvent)
            {
            m_hNetEvent =  CreateEvent( NULL,
                                        TRUE,
                                        FALSE,
                                        NULL);
            }

        // Write down the user request
        if (m_hNetEvent)
            {
            m_hAsyncWindow  = Window;
            m_AsyncSocket   = m_dwContext;
            m_uiAsyncMessage= Message;
            m_lAsyncEvents  = lEvents;

            // Register this socket with the worker thread.
            ReturnCode = gWorkerThread->RegisterSocket( this );
            }
        }
    else
        {
        DEBUGF( DBG_TRACE,
                ("Unegistering socket %X\n",this));
        m_hAsyncWindow  = NULL;
        m_AsyncSocket   = INVALID_SOCKET;
        m_uiAsyncMessage= 0;
        m_lAsyncEvents  = 0;

        ReturnCode = gWorkerThread->UnregisterSocket( this );
        }

    return ReturnCode;
    }


//---------------------------------------------------------------------------
//  RSOCKET::SignalAsyncEvent()
//
//  This is the notification function called by the worker thread to
//  signal network events using a Windows Message.
//---------------------------------------------------------------------------
VOID RSOCKET::SignalAsyncEvent()
    {
    WSANETWORKEVENTS  Events;
    INT               ErrorCode;

    ErrorCode = NO_ERROR;
    assert(this != NULL);

    // Find out what happend.
    m_Provider->WSPEnumNetworkEvents( m_SocketHandle,
                                      m_hNetEvent,
                                      &Events,
                                      &ErrorCode );

    if (NO_ERROR == ErrorCode)
        {
        //
        // Signal all the valid events
        //

        if (Events.lNetworkEvents & FD_READ & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_READ,Events.iErrorCode[FD_READ_BIT] ));
            }

        if (Events.lNetworkEvents & FD_WRITE & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_WRITE,Events.iErrorCode[FD_WRITE_BIT] ));
            }

        if (Events.lNetworkEvents & FD_OOB & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_OOB,Events.iErrorCode[FD_OOB_BIT] ));
            }

        if (Events.lNetworkEvents & FD_ACCEPT & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_ACCEPT,Events.iErrorCode[FD_ACCEPT_BIT] ));
            }

        if (Events.lNetworkEvents & FD_CONNECT & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_CONNECT,Events.iErrorCode[FD_CONNECT_BIT] ));
            }

        if (Events.lNetworkEvents & FD_CLOSE & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_CLOSE,Events.iErrorCode[FD_CLOSE_BIT] ));
            }

        if (Events.lNetworkEvents & FD_QOS & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_QOS,Events.iErrorCode[FD_QOS_BIT] ));
            }

        if (Events.lNetworkEvents & FD_GROUP_QOS & m_lAsyncEvents)
            {
            PostMessage( m_hAsyncWindow,
                         m_uiAsyncMessage,
                         m_dwContext,
                         MAKELONG( FD_GROUP_QOS,Events.iErrorCode[FD_GROUP_QOS_BIT] ));
            }
        }
    }

//---------------------------------------------------------------------------
//  RSOCKET::AddListDSocket()
//
//---------------------------------------------------------------------------
BOOL RSOCKET::InitListDSocket()
    {
    NTSTATUS    NtStatus;

    InitializeListHead(&g_SocketList);

    NtStatus = RtlInitializeCriticalSection(&g_csSocketList);

    return TRUE;
    }

//---------------------------------------------------------------------------
//  RSOCKET::FindListDSocket()
//
//  Search the list of currently open sockets to find the RSOCKET object
//  that is associated with the specified socket handle.
//---------------------------------------------------------------------------
RSOCKET *RSOCKET::FindListDSocket( SOCKET s )
    {
    NTSTATUS    NtStatus;
    RSOCKET    *pRSocket;
    LIST_ENTRY *pListEntry;

    NtStatus = RtlEnterCriticalSection(&g_csSocketList);

    pListEntry = g_SocketList.Flink;

    while ( pListEntry != &g_SocketList)
        {
        pRSocket = CONTAINING_RECORD( pListEntry,
                                      RSOCKET,
                                      m_SocketListLinkage );

        if (pRSocket->m_dwContext == s)
            {
            NtStatus = RtlLeaveCriticalSection(&g_csSocketList);
            return pRSocket;
            }

        pListEntry = pListEntry->Flink;
        }

    NtStatus = RtlLeaveCriticalSection(&g_csSocketList);

    return 0;
    }

//---------------------------------------------------------------------------
//  RSOCKET::AddListDSocket()
//
//---------------------------------------------------------------------------
BOOL RSOCKET::AddListDSocket( RSOCKET *pRSocket )
    {
    NTSTATUS  NtStatus;

    NtStatus = RtlEnterCriticalSection(&g_csSocketList);

    InsertHeadList( &g_SocketList,
                    &pRSocket->m_SocketListLinkage );

    NtStatus = RtlLeaveCriticalSection(&g_csSocketList);

    return TRUE;
    }

//---------------------------------------------------------------------------
//  RSOCKET::RemoveListDSocket()
//
//---------------------------------------------------------------------------
BOOL RSOCKET::RemoveListDSocket( RSOCKET *pRSocket )
    {
    NTSTATUS  NtStatus;

    NtStatus = RtlEnterCriticalSection(&g_csSocketList);

    RemoveEntryList( &pRSocket->m_SocketListLinkage );

    NtStatus = RtlLeaveCriticalSection(&g_csSocketList);

    return TRUE;
    }

//---------------------------------------------------------------------------
//  RSOCKET::RemoveHeadListDSocket()
//
//---------------------------------------------------------------------------
RSOCKET *RSOCKET::RemoveHeadListDSocket()
    {
    NTSTATUS    NtStatus;
    RSOCKET    *pRSocket;
    LIST_ENTRY *pListEntry;

    NtStatus = RtlEnterCriticalSection(&g_csSocketList);

    if (IsListEmpty(&g_SocketList))
       {
       pRSocket = 0;
       }
    else
       {
       pListEntry = RemoveHeadList(&g_SocketList);
       pRSocket = CONTAINING_RECORD( pListEntry,
                                     RSOCKET,
                                     m_SocketListLinkage );
       }

    NtStatus = RtlLeaveCriticalSection(&g_csSocketList);

    return pRSocket;
    }

