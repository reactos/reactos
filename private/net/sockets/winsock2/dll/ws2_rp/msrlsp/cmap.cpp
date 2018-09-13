//------------------------------------------------------------------------
//  Copyright (c)1997 Microsoft Corporation, All Rights Reserved.
//
//  cmap.cpp
//
//  Maintain a mapping between handles (SOCKET) and socket objects
//  (RSOCKET).
//
//  In a restricted process, we can't use WPUCreateSocketHandle(), so
//  we need to create our own handles for use by Winsock2. Because of
//  this we need to maintain our own mapping of handle->socket objects,
//  and we can't use WPUQuerySocketHandleContext().
//
//  Note: that this is only used in a restricted process, which can't
//  be a server (listen() not allowed, etc.) so a hash is overkill.
//
//  Revision History:
//
//  edwardr    12-18-97     Initial version.
//
//------------------------------------------------------------------------

#include "precomp.h"

//------------------------------------------------------------------------
//  Constructor
//
//------------------------------------------------------------------------
CSocketMap::CSocketMap()
    {
    m_dwMapSize = 0;
    m_pMap = 0;
    }

//------------------------------------------------------------------------
//  CSocketMap::Initialize()
//
//  Initialize must be call before using a CSocketMap. Return TRUE on 
//  success, FALSE on failure.
//------------------------------------------------------------------------
BOOL CSocketMap::Initialize( DWORD dwNewMapSize )
    {
    if (!dwNewMapSize)
       {
       return FALSE;
       }

    // If the map size has already been set, then initialize has
    // already been called successfully.
    if (!m_dwMapSize)
       {
       m_pMap = new SOCKET_MAP_ENTRY [dwNewMapSize];
       if (!m_pMap)
          {
          return FALSE;
          }

       NTSTATUS  Status = RtlInitializeCriticalSection(&m_cs);
       if (!NT_SUCCESS(Status))
           {
           m_dwMapSize = 0;
           delete m_pMap;
           m_pMap = 0;
           return FALSE;
           }

       m_dwMapSize = dwNewMapSize;

       for (DWORD i=0; i<m_dwMapSize; i++)
           {
           m_pMap[i].hSocket = INVALID_SOCKET;
           m_pMap[i].pRSocket = 0;
           }
       }

    return TRUE;
    }

//------------------------------------------------------------------------
//  Destructor
//
//------------------------------------------------------------------------
CSocketMap::~CSocketMap()
    {
    NTSTATUS  Status;

    if (m_pMap)
        {
        Status = RtlDeleteCriticalSection(&m_cs);

        delete m_pMap;
        }

    }

//------------------------------------------------------------------------
//  CSocketMap::Lookup()
//
//  Given a socket handle, find the associated LSP socket object.
//------------------------------------------------------------------------
RSOCKET *CSocketMap::Lookup( SOCKET hSocket )
    {
    SOCKET_MAP_ENTRY *pMap = m_pMap;

    if (!pMap)
        {
        return 0;
        }

    NTSTATUS  Status = RtlEnterCriticalSection(&m_cs);

    for (DWORD i=0; i<m_dwMapSize; i++)
        {
        if (pMap->hSocket == hSocket)
            {
            Status = RtlLeaveCriticalSection(&m_cs);
            return pMap->pRSocket;
            }

        pMap++;
        }

    Status = RtlLeaveCriticalSection(&m_cs);

    return 0;
    }

//------------------------------------------------------------------------
//  CSocketMap::Add()
//
//  Add a mapping between the specified socket handle and LSP socket 
//  object.
//
//  Return 
//    TRUE   :Success. 
//    FALSE  :Failure. Invalid (key,value) pair or out of memory.
//------------------------------------------------------------------------
BOOL CSocketMap::Add( SOCKET   hSocket, 
                      RSOCKET *pRSocket )
    {
    DWORD  i;

    // Only add entries that look valid...
    if ((hSocket == INVALID_SOCKET) || (pRSocket == 0))
        {
        return FALSE;
        }

    NTSTATUS  Status = RtlEnterCriticalSection(&m_cs);

    SOCKET_MAP_ENTRY *pMap = m_pMap;

    // Find a unused entry in the table and insert.
    for (i=0; i<m_dwMapSize; i++)
        {
        if (pMap->hSocket == INVALID_SOCKET)
           {
           pMap->hSocket = hSocket;
           pMap->pRSocket = pRSocket;
           Status = RtlLeaveCriticalSection(&m_cs);
           return TRUE;
           }

        pMap++;
        }

    // If we get here, then the current table is full, grow
    // the table...
    DWORD  dwNewMapSize = (3*m_dwMapSize)/2;  // 0.5 times bigger.

    SOCKET_MAP_ENTRY *pNewMap = new SOCKET_MAP_ENTRY [dwNewMapSize];

    if (!pNewMap)
       {
       Status = RtlLeaveCriticalSection(&m_cs);
       return FALSE;     // Out of memory...
       }

    for (i=0; i<(int)m_dwMapSize; i++)
       {
       pNewMap[i].hSocket = m_pMap[i].hSocket;
       pNewMap[i].pRSocket = m_pMap[i].pRSocket;
       }

    pNewMap[m_dwMapSize].hSocket = hSocket;
    pNewMap[m_dwMapSize++].pRSocket = pRSocket;

    for (i=m_dwMapSize; i<dwNewMapSize; i++)
       {
       pNewMap[i].hSocket = INVALID_SOCKET;
       pNewMap[i].pRSocket = 0;
       }

    m_dwMapSize = dwNewMapSize;
    delete m_pMap;
    m_pMap = pNewMap;

    Status = RtlLeaveCriticalSection(&m_cs);

    return TRUE;
    }

//------------------------------------------------------------------------
//  CSocketMap::Remove()
//
//  Find the entry for the specified socket handle, return the associated
//  LSP socket object and then remove the entry from the mapping.
//------------------------------------------------------------------------
RSOCKET *CSocketMap::Remove( SOCKET hSocket )
    {
    RSOCKET          *pRSocket;
    SOCKET_MAP_ENTRY *pMap = m_pMap;
    NTSTATUS          Status = RtlEnterCriticalSection(&m_cs);

    for (DWORD i=0; i<m_dwMapSize; i++)
        {
        if (pMap->hSocket == hSocket)
            {
            pRSocket = pMap->pRSocket;
            pMap->hSocket = INVALID_SOCKET;
            pMap->pRSocket = 0;
            Status = RtlLeaveCriticalSection(&m_cs);
            return pRSocket;
            }

        pMap++;
        }

    Status = RtlLeaveCriticalSection(&m_cs);

    return 0;
    }
