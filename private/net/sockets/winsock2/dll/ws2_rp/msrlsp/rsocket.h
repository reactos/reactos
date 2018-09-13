//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//  Copyright c 1996 Intel Corporation
//
//  rsocket.h
//
//  Define our LSP socket class. The RP_SOCKET class defines the handle for
//  a restricted process's TCP/IP socket.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version. Modeled after the dsocket.h
//                         LSP example code.
//
//---------------------------------------------------------------------------

#ifndef _RSOCKET_
#define _RSOCKET_

#include <windows.h>
#include "llist.h"
#include "fwdref.h"

class RSOCKET
{
  public:

    static
    INT
    DSocketClassInitialize();

    static
    INT
    DSocketClassCleanup();


    RSOCKET();

    INT
    Initialize( IN PRPROVIDER          Provider,
                IN SOCKET              ProviderSocket,
                IN DWORD               dwCatalogEntryId,
                IN ULONG_PTR            dwContext,
                IN IRestrictedProcess *pIRestrictedProcess );

    ~RSOCKET();

    SOCKET     GetSocketHandle();

    void       SetSocketHandle( SOCKET SocketHandle );

    PRPROVIDER GetDProvider();

    DWORD      GetCatalogEntryId();

    ULONG_PTR   GetContext();

    IRestrictedProcess* GetIRestrictedProcess();

    void       SetIRestrictedProcess(IRestrictedProcess *pIRestrictedProcess);

    INT        RegisterAsyncOperation( HWND     hWindow,
                                       UINT     uiMessage,
                                       LONG     lEvents );

    VOID       SignalAsyncEvent();

    LONG       GetAsyncEventMask();

    HANDLE     GetAsyncEventHandle();

    static BOOL     InitListDSocket();

    static RSOCKET *FindListDSocket( SOCKET s );

    static BOOL     AddListDSocket( RSOCKET *pRSocket );

    static BOOL     RemoveListDSocket( RSOCKET *pRSocket );

    static RSOCKET *RemoveHeadListDSocket();

    // Provides the linkage for a list of the open RSOCKET objects:
    LIST_ENTRY  m_SocketListLinkage;

  private:

    // Reference  to  the  RPROVIDER object representing the next service
    // provider in the chain:
    PRPROVIDER  m_Provider;

    // The socket handle for m_Provider:
    SOCKET  m_SocketHandle;

    // The catalog entry id of the provider that this socket is attached to:
    DWORD   m_dwCatalogEntryId;

    // The socket handle returned from WPUCreateSocketHandle (this is the
    // socket handle that the outside world sees):
    SOCKET  m_dwContext;

    // The event mask for the events the client has registered interest in.
    LONG    m_lAsyncEvents;       // BUGBUG: Should this be DWORD?

    // The window handle to receive net event messages:
    HWND    m_hAsyncWindow;

    // The socket handle for this socket. (the same as m_dwContext):
    SOCKET  m_AsyncSocket;

    // The message to send to the client to signal net envents.
    UINT    m_uiAsyncMessage;

    // The handle to the WIN32 event that is used with WSPEventSelect() to
    // register interest in events contained in m_lAsyncEvents:
    HANDLE  m_hNetEvent;

    // COM Interface to the restricted sockets helper process object.
    IRestrictedProcess *m_pIRestrictedProcess;

};


//---------------------------------------------------------------------------
//  RSOCKET::GetSocketHandle()
//
//  Return the "actual" socket handle associated with this RSOCKET instance.
//---------------------------------------------------------------------------
inline SOCKET RSOCKET::GetSocketHandle()
    {
    return m_SocketHandle;
    }

//---------------------------------------------------------------------------
//  RSOCKET::SetSocketHandle()
//
//  SEt the "actual" socket handle associated with this RSOCKET instance.
//---------------------------------------------------------------------------
inline void RSOCKET::SetSocketHandle( SOCKET SocketHandle )
    {
    m_SocketHandle = SocketHandle;
    }

//---------------------------------------------------------------------------
//  RSOCKET::GetDProvider()
//
//  Return a pointer to the RPROVIDER associated with this RSOCKET instance.
//---------------------------------------------------------------------------
inline
PRPROVIDER RSOCKET::GetDProvider()
{
    return(m_Provider);
}


//---------------------------------------------------------------------------
//  RSOCKET::GetCatalogEntryId()
//
//  Return the WinSock2 catalog entry ID associated with this RSOCKET instance.
//---------------------------------------------------------------------------
inline
DWORD RSOCKET::GetCatalogEntryId()
{
    return(m_dwCatalogEntryId);
}


//---------------------------------------------------------------------------
//  RSOCKET::GetContext()
//
//  Return the socket handle for this socket that was returned from
//  WPUCreateSocketHandle().
//---------------------------------------------------------------------------
inline
ULONG_PTR RSOCKET::GetContext()
{
    return(m_dwContext);
}

//---------------------------------------------------------------------------
//  RSOCKET::GetAsyncEventMask()
//
//  Return the event mask for this socket.
//---------------------------------------------------------------------------
inline
LONG RSOCKET::GetAsyncEventMask()
{
    return(m_lAsyncEvents);
}

//---------------------------------------------------------------------------
//  RSOCKET::GetAsyncEventHandle()
//
//  Return the handle of the event associated with this socket.
//---------------------------------------------------------------------------
inline
HANDLE RSOCKET::GetAsyncEventHandle()
{
    return(m_hNetEvent);
}

//---------------------------------------------------------------------------
//  RSOCKET::GetIRestrictedProcess()
//
//  Return the IRestrictedProcess COM object.
//---------------------------------------------------------------------------
inline
IRestrictedProcess *RSOCKET::GetIRestrictedProcess()
{
    return m_pIRestrictedProcess;
}

//---------------------------------------------------------------------------
//  RSOCKET::SetIRestrictedProcess()
//
//  Set the IRestrictedProcess COM object.
//---------------------------------------------------------------------------
inline
void RSOCKET::SetIRestrictedProcess( IRestrictedProcess *pIRestrictedProcess )
{
    m_pIRestrictedProcess = pIRestrictedProcess;
}

#endif // _RSOCKET_
