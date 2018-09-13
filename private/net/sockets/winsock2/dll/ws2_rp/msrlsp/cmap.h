//---------------------------------------------------------------
//  Copyright (c)1997 Microsoft Corporation, All Rights Reserved.
//
//  cmap.h
//
//  Maps between socket handles and RSOCKET objects.
//
//---------------------------------------------------------------

#ifndef __CMAP_HXX__
#define __CMAP_HXX__

#define  DEFAULT_MAP_SIZE        10

//---------------------------------------------------------------
//  CSocketMap
//---------------------------------------------------------------
typedef struct _SOCKET_MAP_ENTRY
{
    SOCKET   hSocket;      // Key: Socket Handle.
    RSOCKET *pRSocket;     // Value: Pointer to the LSP Socket Object.
} SOCKET_MAP_ENTRY;

class CSocketMap
{

public:

    // Construction
	CSocketMap();
	BOOL Initialize(  DWORD dwMapSize = DEFAULT_MAP_SIZE );

	// Given a socket handle, find the associated DSOCKET.
	RSOCKET *Lookup( SOCKET hSocket );

	// Add a new (hSocket,pRSocket) pair.
	BOOL     Add( SOCKET hSocket, RSOCKET *pRSocket );

    // Lookup and remove from the mapping.
    RSOCKET *Remove( SOCKET hSocket );

	~CSocketMap();

private:
    DWORD             m_dwMapSize;
    SOCKET_MAP_ENTRY *m_pMap;
    RTL_CRITICAL_SECTION  m_cs;
};

#endif
