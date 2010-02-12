/* Direct Play Lobby 2 & 3 Implementation
 *
 * Copyright 1998,1999,2000 - Peter Hunnisett
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/debug.h"

#include "dplayx_global.h"
#include "dplayx_messages.h"
#include "dplayx_queue.h"
#include "dplobby.h"
#include "dpinit.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/*****************************************************************************
 * Predeclare the interface implementation structures
 */
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobbyAImpl;
typedef struct IDirectPlayLobbyImpl  IDirectPlayLobbyWImpl;
typedef struct IDirectPlayLobby2Impl IDirectPlayLobby2AImpl;
typedef struct IDirectPlayLobby2Impl IDirectPlayLobby2WImpl;
typedef struct IDirectPlayLobby3Impl IDirectPlayLobby3AImpl;
typedef struct IDirectPlayLobby3Impl IDirectPlayLobby3WImpl;

/* Forward declarations for this module helper methods */
HRESULT DPL_CreateCompoundAddress ( LPCDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount,
                                    LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );

HRESULT DPL_CreateAddress( REFGUID guidSP, REFGUID guidDataType, LPCVOID lpData, DWORD dwDataSize,
                           LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );



extern HRESULT DPL_EnumAddress( LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, LPCVOID lpAddress,
                                DWORD dwAddressSize, LPVOID lpContext );

static HRESULT WINAPI DPL_ConnectEx( IDirectPlayLobbyAImpl* This,
                                     DWORD dwFlags, REFIID riid,
                                     LPVOID* lplpDP, IUnknown* pUnk );

BOOL DPL_CreateAndSetLobbyHandles( DWORD dwDestProcessId, HANDLE hDestProcess,
                                   LPHANDLE lphStart, LPHANDLE lphDeath,
                                   LPHANDLE lphRead );


/*****************************************************************************
 * IDirectPlayLobby {1,2,3} implementation structure
 *
 * The philosophy behind this extra pointer dereference is that I wanted to
 * have the same structure for all types of objects without having to do
 * a lot of casting. I also only wanted to implement an interface in the
 * object it was "released" with IUnknown interface being implemented in the 1 version.
 * Of course, with these new interfaces comes the data required to keep the state required
 * by these interfaces. So, basically, the pointers contain the data associated with
 * a release. If you use the data associated with release 3 in a release 2 object, you'll
 * get a run time trap, as that won't have any data.
 *
 */
struct DPLMSG
{
  DPQ_ENTRY( DPLMSG ) msgs;  /* Link to next queued message */
};
typedef struct DPLMSG* LPDPLMSG;

typedef struct tagDirectPlayLobbyIUnknownData
{
  LONG              ulObjRef;
  CRITICAL_SECTION  DPL_lock;
} DirectPlayLobbyIUnknownData;

typedef struct tagDirectPlayLobbyData
{
  HKEY  hkCallbackKeyHack;
  DWORD dwMsgThread;
  DPQ_HEAD( DPLMSG ) msgs;  /* List of messages received */
} DirectPlayLobbyData;

typedef struct tagDirectPlayLobby2Data
{
  BOOL dummy;
} DirectPlayLobby2Data;

typedef struct tagDirectPlayLobby3Data
{
  BOOL dummy;
} DirectPlayLobby3Data;

#define DPL_IMPL_FIELDS \
 LONG ulInterfaceRef; \
 DirectPlayLobbyIUnknownData*  unk; \
 DirectPlayLobbyData*          dpl; \
 DirectPlayLobby2Data*         dpl2; \
 DirectPlayLobby3Data*         dpl3;

struct IDirectPlayLobbyImpl
{
    const IDirectPlayLobbyVtbl *lpVtbl;
    DPL_IMPL_FIELDS
};

struct IDirectPlayLobby2Impl
{
    const IDirectPlayLobby2Vtbl *lpVtbl;
    DPL_IMPL_FIELDS
};

struct IDirectPlayLobby3Impl
{
    const IDirectPlayLobby3Vtbl *lpVtbl;
    DPL_IMPL_FIELDS
};

/* Forward declarations of virtual tables */
static const IDirectPlayLobbyVtbl  directPlayLobbyWVT;
static const IDirectPlayLobby2Vtbl directPlayLobby2WVT;
static const IDirectPlayLobby3Vtbl directPlayLobby3WVT;

static const IDirectPlayLobbyVtbl  directPlayLobbyAVT;
static const IDirectPlayLobby2Vtbl directPlayLobby2AVT;
static const IDirectPlayLobby3Vtbl directPlayLobby3AVT;

static BOOL DPL_CreateIUnknown( LPVOID lpDPL )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)lpDPL;

  This->unk = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->unk) ) );
  if ( This->unk == NULL )
  {
    return FALSE;
  }

  InitializeCriticalSection( &This->unk->DPL_lock );

  return TRUE;
}

static BOOL DPL_DestroyIUnknown( LPVOID lpDPL )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)lpDPL;

  DeleteCriticalSection( &This->unk->DPL_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}

static BOOL DPL_CreateLobby1( LPVOID lpDPL )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)lpDPL;

  This->dpl = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dpl) ) );
  if ( This->dpl == NULL )
  {
    return FALSE;
  }

  DPQ_INIT( This->dpl->msgs );

  return TRUE;
}

static BOOL DPL_DestroyLobby1( LPVOID lpDPL )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)lpDPL;

  if( This->dpl->dwMsgThread )
  {
    FIXME( "Should kill the msg thread\n" );
  }

  DPQ_DELETEQ( This->dpl->msgs, msgs, LPDPLMSG, cbDeleteElemFromHeap );

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dpl );

  return TRUE;
}

static BOOL DPL_CreateLobby2( LPVOID lpDPL )
{
  IDirectPlayLobby2AImpl *This = (IDirectPlayLobby2AImpl *)lpDPL;

  This->dpl2 = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dpl2) ) );
  if ( This->dpl2 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

static BOOL DPL_DestroyLobby2( LPVOID lpDPL )
{
  IDirectPlayLobby2AImpl *This = (IDirectPlayLobby2AImpl *)lpDPL;

  HeapFree( GetProcessHeap(), 0, This->dpl2 );

  return TRUE;
}

static BOOL DPL_CreateLobby3( LPVOID lpDPL )
{
  IDirectPlayLobby3AImpl *This = (IDirectPlayLobby3AImpl *)lpDPL;

  This->dpl3 = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dpl3) ) );
  if ( This->dpl3 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

static BOOL DPL_DestroyLobby3( LPVOID lpDPL )
{
  IDirectPlayLobby3AImpl *This = (IDirectPlayLobby3AImpl *)lpDPL;

  HeapFree( GetProcessHeap(), 0, This->dpl3 );

  return TRUE;
}


/* The COM interface for upversioning an interface
 * We've been given a GUID (riid) and we need to replace the present
 * interface with that of the requested interface.
 *
 * Snip from some Microsoft document:
 * There are four requirements for implementations of QueryInterface (In these
 * cases, "must succeed" means "must succeed barring catastrophic failure."):
 *
 *  * The set of interfaces accessible on an object through
 *    IUnknown::QueryInterface must be static, not dynamic. This means that
 *    if a call to QueryInterface for a pointer to a specified interface
 *    succeeds the first time, it must succeed again, and if it fails the
 *    first time, it must fail on all subsequent queries.
 *  * It must be symmetric ~W if a client holds a pointer to an interface on
 *    an object, and queries for that interface, the call must succeed.
 *  * It must be reflexive ~W if a client holding a pointer to one interface
 *    queries successfully for another, a query through the obtained pointer
 *    for the first interface must succeed.
 *  * It must be transitive ~W if a client holding a pointer to one interface
 *    queries successfully for a second, and through that pointer queries
 *    successfully for a third interface, a query for the first interface
 *    through the pointer for the third interface must succeed.
 */
extern
HRESULT DPL_CreateInterface
         ( REFIID riid, LPVOID* ppvObj )
{
  TRACE( " for %s\n", debugstr_guid( riid ) );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( IDirectPlayLobbyWImpl ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  if( IsEqualGUID( &IID_IDirectPlayLobby, riid ) )
  {
    IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobbyWVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) )
  {
    IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobbyAVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
    IDirectPlayLobby2WImpl *This = (IDirectPlayLobby2WImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby2WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
    IDirectPlayLobby2AImpl *This = (IDirectPlayLobby2AImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby2AVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3, riid ) )
  {
    IDirectPlayLobby3WImpl *This = (IDirectPlayLobby3WImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby3WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3A, riid ) )
  {
    IDirectPlayLobby3AImpl *This = (IDirectPlayLobby3AImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby3AVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  /* Initialize it */
  if ( DPL_CreateIUnknown( *ppvObj ) &&
       DPL_CreateLobby1( *ppvObj ) &&
       DPL_CreateLobby2( *ppvObj ) &&
       DPL_CreateLobby3( *ppvObj )
     )
  {
    IDirectPlayLobby_AddRef( (LPDIRECTPLAYLOBBY)*ppvObj );
    return S_OK;
  }

  /* Initialize failed, destroy it */
  DPL_DestroyLobby3( *ppvObj );
  DPL_DestroyLobby2( *ppvObj );
  DPL_DestroyLobby1( *ppvObj );
  DPL_DestroyIUnknown( *ppvObj );
  HeapFree( GetProcessHeap(), 0, *ppvObj );

  *ppvObj = NULL;
  return DPERR_NOMEMORY;
}

static HRESULT WINAPI DPL_QueryInterface
( LPDIRECTPLAYLOBBYA iface,
  REFIID riid,
  LPVOID* ppvObj )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;
  TRACE("(%p)->(%s,%p)\n", This, debugstr_guid( riid ), ppvObj );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( *This ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  CopyMemory( *ppvObj, This, sizeof( *This )  );
  (*(IDirectPlayLobbyAImpl**)ppvObj)->ulInterfaceRef = 0;

  if( IsEqualGUID( &IID_IDirectPlayLobby, riid ) )
  {
    IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobbyWVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobbyA, riid ) )
  {
    IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobbyAVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2, riid ) )
  {
    IDirectPlayLobby2WImpl *This = (IDirectPlayLobby2WImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby2WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby2A, riid ) )
  {
    IDirectPlayLobby2AImpl *This = (IDirectPlayLobby2AImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby2AVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3, riid ) )
  {
    IDirectPlayLobby3WImpl *This = (IDirectPlayLobby3WImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby3WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlayLobby3A, riid ) )
  {
    IDirectPlayLobby3AImpl *This = (IDirectPlayLobby3AImpl *)*ppvObj;
    This->lpVtbl = &directPlayLobby3AVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  IDirectPlayLobby_AddRef( (LPDIRECTPLAYLOBBY)*ppvObj );

  return S_OK;
}

/*
 * Simple procedure. Just increment the reference count to this
 * structure and return the new reference count.
 */
static ULONG WINAPI DPL_AddRef
( LPDIRECTPLAYLOBBY iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;

  ulObjRefCount       = InterlockedIncrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedIncrement( &This->ulInterfaceRef );

  TRACE( "ref count incremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  return ulObjRefCount;
}

/*
 * Simple COM procedure. Decrease the reference count to this object.
 * If the object no longer has any reference counts, free up the associated
 * memory.
 */
static ULONG WINAPI DPL_Release
( LPDIRECTPLAYLOBBYA iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;

  ulObjRefCount       = InterlockedDecrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedDecrement( &This->ulInterfaceRef );

  TRACE( "ref count decremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  /* Deallocate if this is the last reference to the object */
  if( ulObjRefCount == 0 )
  {
     DPL_DestroyLobby3( This );
     DPL_DestroyLobby2( This );
     DPL_DestroyLobby1( This );
     DPL_DestroyIUnknown( This );
  }

  if( ulInterfaceRefCount == 0 )
  {
    HeapFree( GetProcessHeap(), 0, This );
  }

  return ulInterfaceRefCount;
}


/********************************************************************
 *
 * Connects an application to the session specified by the DPLCONNECTION
 * structure currently stored with the DirectPlayLobby object.
 *
 * Returns an IDirectPlay interface.
 *
 */
static HRESULT WINAPI DPL_ConnectEx
( IDirectPlayLobbyAImpl* This,
  DWORD     dwFlags,
  REFIID    riid,
  LPVOID*   lplpDP,
  IUnknown* pUnk)
{
  HRESULT         hr;
  DWORD           dwOpenFlags = 0;
  DWORD           dwConnSize = 0;
  LPDPLCONNECTION lpConn;

  FIXME("(%p)->(0x%08lx,%p,%p): semi stub\n", This, dwFlags, lplpDP, pUnk );

  if( pUnk )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Backwards compatibility */
  if( dwFlags == 0 )
  {
    dwFlags = DPCONNECT_RETURNSTATUS;
  }

  /* Create the DirectPlay interface */
  if( ( hr = DP_CreateInterface( riid, lplpDP ) ) != DP_OK )
  {
     ERR( "error creating interface for %s:%s.\n",
          debugstr_guid( riid ), DPLAYX_HresultToString( hr ) );
     return hr;
  }

  /* FIXME: Is it safe/correct to use appID of 0? */
  hr = IDirectPlayLobby_GetConnectionSettings( (LPDIRECTPLAYLOBBY)This,
                                               0, NULL, &dwConnSize );
  if( hr != DPERR_BUFFERTOOSMALL )
  {
    return hr;
  }

  lpConn = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwConnSize );

  if( lpConn == NULL )
  {
    return DPERR_NOMEMORY;
  }

  /* FIXME: Is it safe/correct to use appID of 0? */
  hr = IDirectPlayLobby_GetConnectionSettings( (LPDIRECTPLAYLOBBY)This,
                                               0, lpConn, &dwConnSize );
  if( FAILED( hr ) )
  {
    HeapFree( GetProcessHeap(), 0, lpConn );
    return hr;
  }

#if 0
  /* - Need to call IDirectPlay::EnumConnections with the service provider to get that good information
   * - Need to call CreateAddress to create the lpConnection param for IDirectPlay::InitializeConnection
   * - Call IDirectPlay::InitializeConnection
   */

  /* Now initialize the Service Provider */
  hr = IDirectPlayX_InitializeConnection( (*(LPDIRECTPLAY2*)lplpDP),
#endif


  /* Setup flags to pass into DirectPlay::Open */
  if( dwFlags & DPCONNECT_RETURNSTATUS )
  {
    dwOpenFlags |= DPOPEN_RETURNSTATUS;
  }
  dwOpenFlags |= lpConn->dwFlags;

  hr = IDirectPlayX_Open( (*(LPDIRECTPLAY2*)lplpDP), lpConn->lpSessionDesc,
                          dwOpenFlags );

  HeapFree( GetProcessHeap(), 0, lpConn );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_Connect
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  LPDIRECTPLAY2A* lplpDP,
  IUnknown* pUnk)
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;
  return DPL_ConnectEx( This, dwFlags, &IID_IDirectPlay2A,
                        (LPVOID)lplpDP, pUnk );
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_Connect
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDIRECTPLAY2* lplpDP,
  IUnknown* pUnk)
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface; /* Yes cast to A */
  return DPL_ConnectEx( This, dwFlags, &IID_IDirectPlay2,
                        (LPVOID)lplpDP, pUnk );
}

/********************************************************************
 *
 * Creates a DirectPlay Address, given a service provider-specific network
 * address.
 * Returns an address contains the globally unique identifier
 * (GUID) of the service provider and data that the service provider can
 * interpret as a network address.
 *
 * NOTE: It appears that this method is supposed to be really really stupid
 *       with no error checking on the contents.
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_CreateAddress
( LPDIRECTPLAYLOBBYA iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateAddress( guidSP, guidDataType, lpData, dwDataSize,
                            lpAddress, lpdwAddressSize, TRUE );
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_CreateAddress
( LPDIRECTPLAYLOBBY iface,
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateAddress( guidSP, guidDataType, lpData, dwDataSize,
                            lpAddress, lpdwAddressSize, FALSE );
}

HRESULT DPL_CreateAddress(
  REFGUID guidSP,
  REFGUID guidDataType,
  LPCVOID lpData,
  DWORD dwDataSize,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize,
  BOOL bAnsiInterface )
{
  const DWORD dwNumAddElements = 2; /* Service Provide & address data type */
  DPCOMPOUNDADDRESSELEMENT addressElements[ 2 /* dwNumAddElements */ ];

  TRACE( "(%p)->(%p,%p,0x%08lx,%p,%p,%d)\n", guidSP, guidDataType, lpData, dwDataSize,
                                             lpAddress, lpdwAddressSize, bAnsiInterface );

  addressElements[ 0 ].guidDataType = DPAID_ServiceProvider;
  addressElements[ 0 ].dwDataSize = sizeof( GUID );
  addressElements[ 0 ].lpData = (LPVOID)guidSP;

  addressElements[ 1 ].guidDataType = *guidDataType;
  addressElements[ 1 ].dwDataSize = dwDataSize;
  addressElements[ 1 ].lpData = (LPVOID)lpData;

  /* Call CreateCompoundAddress to cut down on code.
     NOTE: We can do this because we don't support DPL 1 interfaces! */
  return DPL_CreateCompoundAddress( addressElements, dwNumAddElements,
                                    lpAddress, lpdwAddressSize, bAnsiInterface );
}



/********************************************************************
 *
 * Parses out chunks from the DirectPlay Address buffer by calling the
 * given callback function, with lpContext, for each of the chunks.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumAddress
( LPDIRECTPLAYLOBBYA iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;

  TRACE("(%p)->(%p,%p,0x%08lx,%p)\n", This, lpEnumAddressCallback, lpAddress,
                                      dwAddressSize, lpContext );

  return DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddress
( LPDIRECTPLAYLOBBY iface,
  LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
  LPCVOID lpAddress,
  DWORD dwAddressSize,
  LPVOID lpContext )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;

  TRACE("(%p)->(%p,%p,0x%08lx,%p)\n", This, lpEnumAddressCallback, lpAddress,
                                      dwAddressSize, lpContext );

  return DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );
}

extern HRESULT DPL_EnumAddress( LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, LPCVOID lpAddress,
                                DWORD dwAddressSize, LPVOID lpContext )
{
  DWORD dwTotalSizeEnumerated = 0;

  /* FIXME: First chunk is always the total size chunk - Should we report it? */

  while ( dwTotalSizeEnumerated < dwAddressSize )
  {
    const DPADDRESS* lpElements = (const DPADDRESS*)lpAddress;
    DWORD dwSizeThisEnumeration;

    /* Invoke the enum method. If false is returned, stop enumeration */
    if ( !lpEnumAddressCallback( &lpElements->guidDataType,
                                 lpElements->dwDataSize,
                                 (BYTE*)lpElements + sizeof( DPADDRESS ),
                                 lpContext ) )
    {
      break;
    }

    dwSizeThisEnumeration  = sizeof( DPADDRESS ) + lpElements->dwDataSize;
    lpAddress = (const BYTE*) lpAddress + dwSizeThisEnumeration;
    dwTotalSizeEnumerated += dwSizeThisEnumeration;
  }

  return DP_OK;
}

/********************************************************************
 *
 * Enumerates all the address types that a given service provider needs to
 * build the DirectPlay Address.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBYA iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;

  HKEY   hkResult;
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
  DWORD  dwIndex, sizeOfSubKeyName=50;
  char   subKeyName[51];
  FILETIME filetime;

  TRACE(" (%p)->(%p,%p,%p,0x%08lx)\n", This, lpEnumAddressTypeCallback, guidSP, lpContext, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !lpEnumAddressTypeCallback || !*lpEnumAddressTypeCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  if( guidSP == NULL )
  {
    return DPERR_INVALIDOBJECT;
  }

    /* Need to loop over the service providers in the registry */
    if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                         0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
    {
      /* Hmmm. Does this mean that there are no service providers? */
      ERR(": no service providers?\n");
      return DP_OK;
    }

    /* Traverse all the service providers we have available */
    for( dwIndex=0;
         RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName,
                        NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
         ++dwIndex, sizeOfSubKeyName=50 )
    {

      HKEY     hkServiceProvider, hkServiceProviderAt;
      GUID     serviceProviderGUID;
      DWORD    returnTypeGUID, sizeOfReturnBuffer = 50;
      char     atSubKey[51];
      char     returnBuffer[51];
      WCHAR    buff[51];
      DWORD    dwAtIndex;
      LPCSTR   atKey = "Address Types";
      LPCSTR   guidDataSubKey   = "Guid";
      FILETIME filetime;


      TRACE(" this time through: %s\n", subKeyName );

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                         &hkServiceProvider ) != ERROR_SUCCESS )
      {
         ERR(": what the heck is going on?\n" );
         continue;
      }

      if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                            NULL, &returnTypeGUID, (LPBYTE)returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
      {
        ERR(": missing GUID registry data members\n" );
        continue;
      }

      /* FIXME: Check return types to ensure we're interpreting data right */
      MultiByteToWideChar( CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff)/sizeof(WCHAR) );
      CLSIDFromString( buff, &serviceProviderGUID );
      /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

      /* Determine if this is the Service Provider that the user asked for */
      if( !IsEqualGUID( &serviceProviderGUID, guidSP ) )
      {
        continue;
      }

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkServiceProvider, atKey, 0, KEY_READ,
                         &hkServiceProviderAt ) != ERROR_SUCCESS )
      {
        TRACE(": No Address Types registry data sub key/members\n" );
        break;
      }

      /* Traverse all the address type we have available */
      for( dwAtIndex=0;
           RegEnumKeyExA( hkServiceProviderAt, dwAtIndex, atSubKey, &sizeOfSubKeyName,
                          NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
           ++dwAtIndex, sizeOfSubKeyName=50 )
      {
        TRACE( "Found Address Type GUID %s\n", atSubKey );

        /* FIXME: Check return types to ensure we're interpreting data right */
        MultiByteToWideChar( CP_ACP, 0, atSubKey, -1, buff, sizeof(buff)/sizeof(WCHAR) );
        CLSIDFromString( buff, &serviceProviderGUID );
        /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

        /* The enumeration will return FALSE if we are not to continue */
        if( !lpEnumAddressTypeCallback( &serviceProviderGUID, lpContext, 0 ) )
        {
           WARN("lpEnumCallback returning FALSE\n" );
           break; /* FIXME: This most likely has to break from the procedure...*/
        }

      }

      /* We only enumerate address types for 1 GUID. We've found it, so quit looking */
      break;
    }

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumAddressTypes
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMADDRESSTYPESCALLBACK lpEnumAddressTypeCallback,
  REFGUID guidSP,
  LPVOID lpContext,
  DWORD dwFlags )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Enumerates what applications are registered with DirectPlay by
 * invoking the callback function with lpContext.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyWImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBY iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback,
  LPVOID lpContext,
  DWORD dwFlags )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;

  FIXME("(%p)->(%p,%p,0x%08lx):stub\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_EnumLocalApplications
( LPDIRECTPLAYLOBBYA iface,
  LPDPLENUMLOCALAPPLICATIONSCALLBACK lpEnumLocalAppCallback,
  LPVOID lpContext,
  DWORD dwFlags )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;

  HKEY hkResult;
  LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Applications";
  LPCSTR guidDataSubKey  = "Guid";
  DWORD dwIndex, sizeOfSubKeyName=50;
  char subKeyName[51];
  FILETIME filetime;

  TRACE("(%p)->(%p,%p,0x%08lx)\n", This, lpEnumLocalAppCallback, lpContext, dwFlags );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( !lpEnumLocalAppCallback || !*lpEnumLocalAppCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Need to loop over the service providers in the registry */
  if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                     0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
  {
    /* Hmmm. Does this mean that there are no service providers? */
    ERR(": no service providers?\n");
    return DP_OK;
  }

  /* Traverse all registered applications */
  for( dwIndex=0;
       RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName, NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
       ++dwIndex, sizeOfSubKeyName=50 )
  {

    HKEY       hkServiceProvider;
    GUID       serviceProviderGUID;
    DWORD      returnTypeGUID, sizeOfReturnBuffer = 50;
    char       returnBuffer[51];
    WCHAR      buff[51];
    DPLAPPINFO dplAppInfo;

    TRACE(" this time through: %s\n", subKeyName );

    /* Get a handle for this particular service provider */
    if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                       &hkServiceProvider ) != ERROR_SUCCESS )
    {
       ERR(": what the heck is going on?\n" );
       continue;
    }

    if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                          NULL, &returnTypeGUID, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
      ERR(": missing GUID registry data members\n" );
      continue;
    }

    /* FIXME: Check return types to ensure we're interpreting data right */
    MultiByteToWideChar( CP_ACP, 0, returnBuffer, -1, buff, sizeof(buff)/sizeof(WCHAR) );
    CLSIDFromString( buff, &serviceProviderGUID );
    /* FIXME: Have I got a memory leak on the serviceProviderGUID? */

    dplAppInfo.dwSize               = sizeof( dplAppInfo );
    dplAppInfo.guidApplication      = serviceProviderGUID;
    dplAppInfo.u.lpszAppNameA = subKeyName;

    EnterCriticalSection( &This->unk->DPL_lock );

    memcpy( &This->dpl->hkCallbackKeyHack, &hkServiceProvider, sizeof( hkServiceProvider ) );

    if( !lpEnumLocalAppCallback( &dplAppInfo, lpContext, dwFlags ) )
    {
       LeaveCriticalSection( &This->unk->DPL_lock );
       break;
    }

    LeaveCriticalSection( &This->unk->DPL_lock );
  }

  return DP_OK;
}

/********************************************************************
 *
 * Retrieves the DPLCONNECTION structure that contains all the information
 * needed to start and connect an application. This was generated using
 * either the RunApplication or SetConnectionSettings methods.
 *
 * NOTES: If lpData is NULL then just return lpdwDataSize. This allows
 *        the data structure to be allocated by our caller which can then
 *        call this procedure/method again with a valid data pointer.
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_GetConnectionSettingsA( dwAppID,
                                      lpData,
                                      lpdwDataSize
                                    );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_GetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,%p,%p)\n", This, dwAppID, lpData, lpdwDataSize );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_GetConnectionSettingsW( dwAppID,
                                      lpData,
                                      lpdwDataSize
                                    );

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

/********************************************************************
 *
 * Retrieves the message sent between a lobby client and a DirectPlay
 * application. All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;
  FIXME(":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_ReceiveLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDWORD lpdwMessageFlags,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  FIXME(":stub %p %08lx %08lx %p %p %p\n", This, dwFlags, dwAppID, lpdwMessageFlags, lpData,
         lpdwDataSize );
  return DPERR_OUTOFMEMORY;
}

typedef struct tagRunApplicationEnumStruct
{
  IDirectPlayLobbyAImpl* This;

  GUID  appGUID;
  LPSTR lpszPath;
  LPSTR lpszFileName;
  LPSTR lpszCommandLine;
  LPSTR lpszCurrentDirectory;
} RunApplicationEnumStruct, *lpRunApplicationEnumStruct;

/* To be called by RunApplication to find how to invoke the function */
static BOOL CALLBACK RunApplicationA_EnumLocalApplications
( LPCDPLAPPINFO   lpAppInfo,
  LPVOID          lpContext,
  DWORD           dwFlags )
{
  lpRunApplicationEnumStruct lpData = (lpRunApplicationEnumStruct)lpContext;

  if( IsEqualGUID( &lpAppInfo->guidApplication, &lpData->appGUID ) )
  {
    char  returnBuffer[200];
    DWORD returnType, sizeOfReturnBuffer;
    LPCSTR clSubKey   = "CommandLine";
    LPCSTR cdSubKey   = "CurrentDirectory";
    LPCSTR fileSubKey = "File";
    LPCSTR pathSubKey = "Path";

    /* FIXME: Lazy man hack - dplay struct has the present reg key saved */

    sizeOfReturnBuffer = 200;

    /* Get all the appropriate data from the registry */
    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, clSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing CommandLine registry data member\n" );
    }
    else
    {
        if ((lpData->lpszCommandLine = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszCommandLine, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, cdSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing CurrentDirectory registry data member\n" );
    }
    else
    {
        if ((lpData->lpszCurrentDirectory = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszCurrentDirectory, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, fileSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing File registry data member\n" );
    }
    else
    {
        if ((lpData->lpszFileName = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszFileName, returnBuffer );
    }

    sizeOfReturnBuffer = 200;

    if( RegQueryValueExA( lpData->This->dpl->hkCallbackKeyHack, pathSubKey,
                          NULL, &returnType, (LPBYTE)returnBuffer,
                          &sizeOfReturnBuffer ) != ERROR_SUCCESS )
    {
       ERR( ": missing Path registry data member\n" );
    }
    else
    {
        if ((lpData->lpszPath = HeapAlloc( GetProcessHeap(), 0, strlen(returnBuffer)+1 )))
            strcpy( lpData->lpszPath, returnBuffer );
    }

    return FALSE; /* No need to keep going as we found what we wanted */
  }

  return TRUE; /* Keep enumerating, haven't found the application yet */
}

BOOL DPL_CreateAndSetLobbyHandles( DWORD dwDestProcessId, HANDLE hDestProcess,
                                   LPHANDLE lphStart, LPHANDLE lphDeath,
                                   LPHANDLE lphRead )
{
  /* These are the handles for the created process */
  HANDLE hAppStart = 0, hAppDeath = 0, hAppRead  = 0;
  SECURITY_ATTRIBUTES s_attrib;

  s_attrib.nLength              = sizeof( s_attrib );
  s_attrib.lpSecurityDescriptor = NULL;
  s_attrib.bInheritHandle       = TRUE;

  *lphStart = CreateEventW( &s_attrib, TRUE, FALSE, NULL );
  *lphDeath = CreateEventW( &s_attrib, TRUE, FALSE, NULL );
  *lphRead  = CreateEventW( &s_attrib, TRUE, FALSE, NULL );

  if( ( !DuplicateHandle( GetCurrentProcess(), *lphStart,
                          hDestProcess, &hAppStart,
                          0, FALSE, DUPLICATE_SAME_ACCESS ) ) ||
      ( !DuplicateHandle( GetCurrentProcess(), *lphDeath,
                          hDestProcess, &hAppDeath,
                          0, FALSE, DUPLICATE_SAME_ACCESS ) ) ||
      ( !DuplicateHandle( GetCurrentProcess(), *lphRead,
                          hDestProcess, &hAppRead,
                          0, FALSE, DUPLICATE_SAME_ACCESS ) )
    )
  {
    if (*lphStart) { CloseHandle(*lphStart); *lphStart = 0; }
    if (*lphDeath) { CloseHandle(*lphDeath); *lphDeath = 0; }
    if (*lphRead)  { CloseHandle(*lphRead);  *lphRead  = 0; }
    /* FIXME: Handle leak... */
    ERR( "Unable to dup handles\n" );
    return FALSE;
  }

  if( !DPLAYX_SetLobbyHandles( dwDestProcessId,
                               hAppStart, hAppDeath, hAppRead ) )
  {
    /* FIXME: Handle leak... */
    return FALSE;
  }

  return TRUE;
}


/********************************************************************
 *
 * Starts an application and passes to it all the information to
 * connect to a session.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_RunApplication
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;
  HRESULT hr;
  RunApplicationEnumStruct enumData;
  char temp[200];
  STARTUPINFOA startupInfo;
  PROCESS_INFORMATION newProcessInfo;
  LPSTR appName;
  DWORD dwSuspendCount;
  HANDLE hStart, hDeath, hSettingRead;

  TRACE( "(%p)->(0x%08lx,%p,%p,%p)\n",
         This, dwFlags, lpdwAppID, lpConn, hReceiveEvent );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( DPLAYX_AnyLobbiesWaitingForConnSettings() )
  {
    FIXME( "Waiting lobby not being handled correctly\n" );
  }

  EnterCriticalSection( &This->unk->DPL_lock );

  ZeroMemory( &enumData, sizeof( enumData ) );
  enumData.This    = This;
  enumData.appGUID = lpConn->lpSessionDesc->guidApplication;

  /* Our callback function will fill up the enumData structure with all the information
     required to start a new process */
  IDirectPlayLobby_EnumLocalApplications( iface, RunApplicationA_EnumLocalApplications,
                                          (LPVOID)(&enumData), 0 );

  /* First the application name */
  strcpy( temp, enumData.lpszPath );
  strcat( temp, "\\" );
  strcat( temp, enumData.lpszFileName );
  HeapFree( GetProcessHeap(), 0, enumData.lpszPath );
  HeapFree( GetProcessHeap(), 0, enumData.lpszFileName );
  if ((appName = HeapAlloc( GetProcessHeap(), 0, strlen(temp)+1 ))) strcpy( appName, temp );

  /* Now the command line */
  strcat( temp, " " );
  strcat( temp, enumData.lpszCommandLine );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
  if ((enumData.lpszCommandLine = HeapAlloc( GetProcessHeap(), 0, strlen(temp)+1 )))
      strcpy( enumData.lpszCommandLine, temp );

  ZeroMemory( &startupInfo, sizeof( startupInfo ) );
  startupInfo.cb = sizeof( startupInfo );
  /* FIXME: Should any fields be filled in? */

  ZeroMemory( &newProcessInfo, sizeof( newProcessInfo ) );

  if( !CreateProcessA( appName,
                       enumData.lpszCommandLine,
                       NULL,
                       NULL,
                       FALSE,
                       CREATE_DEFAULT_ERROR_MODE | CREATE_NEW_CONSOLE | CREATE_SUSPENDED, /* Creation Flags */
                       NULL,
                       enumData.lpszCurrentDirectory,
                       &startupInfo,
                       &newProcessInfo
                     )
    )
  {
    ERR( "Failed to create process for app %s\n", appName );

    HeapFree( GetProcessHeap(), 0, appName );
    HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
    HeapFree( GetProcessHeap(), 0, enumData.lpszCurrentDirectory );

    LeaveCriticalSection( &This->unk->DPL_lock );
    return DPERR_CANTCREATEPROCESS;
  }

  HeapFree( GetProcessHeap(), 0, appName );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCommandLine );
  HeapFree( GetProcessHeap(), 0, enumData.lpszCurrentDirectory );

  /* Reserve this global application id! */
  if( !DPLAYX_CreateLobbyApplication( newProcessInfo.dwProcessId ) )
  {
    ERR( "Unable to create global application data for 0x%08lx\n",
           newProcessInfo.dwProcessId );
  }

  hr = IDirectPlayLobby_SetConnectionSettings( iface, 0, newProcessInfo.dwProcessId, lpConn );

  if( hr != DP_OK )
  {
    ERR( "SetConnectionSettings failure %s\n", DPLAYX_HresultToString( hr ) );
    LeaveCriticalSection( &This->unk->DPL_lock );
    return hr;
  }

  /* Setup the handles for application notification */
  DPL_CreateAndSetLobbyHandles( newProcessInfo.dwProcessId,
                                newProcessInfo.hProcess,
                                &hStart, &hDeath, &hSettingRead );

  /* Setup the message thread ID */
  This->dpl->dwMsgThread =
    CreateLobbyMessageReceptionThread( hReceiveEvent, hStart, hDeath, hSettingRead );

  DPLAYX_SetLobbyMsgThreadId( newProcessInfo.dwProcessId, This->dpl->dwMsgThread );

  LeaveCriticalSection( &This->unk->DPL_lock );

  /* Everything seems to have been set correctly, update the dwAppID */
  *lpdwAppID = newProcessInfo.dwProcessId;

  /* Unsuspend the process - should return the prev suspension count */
  if( ( dwSuspendCount = ResumeThread( newProcessInfo.hThread ) ) != 1 )
  {
    ERR( "ResumeThread failed with 0x%08lx\n", dwSuspendCount );
  }

  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_RunApplication
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  LPDWORD lpdwAppID,
  LPDPLCONNECTION lpConn,
  HANDLE hReceiveEvent )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  FIXME( "(%p)->(0x%08lx,%p,%p,%p):stub\n", This, dwFlags, lpdwAppID, lpConn, (void *)hReceiveEvent );
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Sends a message between the application and the lobby client.
 * All messages are queued until received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_SendLobbyMessage
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPVOID lpData,
  DWORD dwDataSize )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

/********************************************************************
 *
 * Modifies the DPLCONNECTION structure to contain all information
 * needed to start and connect an application.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyWImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  IDirectPlayLobbyWImpl *This = (IDirectPlayLobbyWImpl *)iface;
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,0x%08lx,%p)\n", This, dwFlags, dwAppID, lpConn );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_SetConnectionSettingsW( dwFlags, dwAppID, lpConn );

  /* FIXME: Don't think that this is supposed to fail, but the docuementation
            is somewhat sketchy. I'll try creating a lobby application
            for this... */
  if( hr == DPERR_NOTLOBBIED )
  {
    FIXME( "Unlobbied app setting connections. Is this correct behavior?\n" );
    if( dwAppID == 0 )
    {
       dwAppID = GetCurrentProcessId();
    }
    DPLAYX_CreateLobbyApplication( dwAppID );
    hr = DPLAYX_SetConnectionSettingsW( dwFlags, dwAppID, lpConn );
  }

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

static HRESULT WINAPI IDirectPlayLobbyAImpl_SetConnectionSettings
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  LPDPLCONNECTION lpConn )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface;
  HRESULT hr;

  TRACE("(%p)->(0x%08lx,0x%08lx,%p)\n", This, dwFlags, dwAppID, lpConn );

  EnterCriticalSection( &This->unk->DPL_lock );

  hr = DPLAYX_SetConnectionSettingsA( dwFlags, dwAppID, lpConn );

  /* FIXME: Don't think that this is supposed to fail, but the docuementation
            is somewhat sketchy. I'll try creating a lobby application
            for this... */
  if( hr == DPERR_NOTLOBBIED )
  {
    FIXME( "Unlobbied app setting connections. Is this correct behavior?\n" );
    dwAppID = GetCurrentProcessId();
    DPLAYX_CreateLobbyApplication( dwAppID );
    hr = DPLAYX_SetConnectionSettingsA( dwFlags, dwAppID, lpConn );
  }

  LeaveCriticalSection( &This->unk->DPL_lock );

  return hr;
}

/********************************************************************
 *
 * Registers an event that will be set when a lobby message is received.
 *
 */
static HRESULT WINAPI IDirectPlayLobbyAImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBYA iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}

static HRESULT WINAPI IDirectPlayLobbyWImpl_SetLobbyMessageEvent
( LPDIRECTPLAYLOBBY iface,
  DWORD dwFlags,
  DWORD dwAppID,
  HANDLE hReceiveEvent )
{
  FIXME(":stub\n");
  return DPERR_OUTOFMEMORY;
}


/* DPL 2 methods */
static HRESULT WINAPI IDirectPlayLobby2WImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2 iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateCompoundAddress( lpElements, dwElementCount, lpAddress, lpdwAddressSize, FALSE );
}

static HRESULT WINAPI IDirectPlayLobby2AImpl_CreateCompoundAddress
( LPDIRECTPLAYLOBBY2A iface,
  LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize )
{
  return DPL_CreateCompoundAddress( lpElements, dwElementCount, lpAddress, lpdwAddressSize, TRUE );
}

HRESULT DPL_CreateCompoundAddress
( LPCDPCOMPOUNDADDRESSELEMENT lpElements,
  DWORD dwElementCount,
  LPVOID lpAddress,
  LPDWORD lpdwAddressSize,
  BOOL bAnsiInterface )
{
  DWORD dwSizeRequired = 0;
  DWORD dwElements;
  LPCDPCOMPOUNDADDRESSELEMENT lpOrigElements = lpElements;

  TRACE("(%p,0x%08lx,%p,%p)\n", lpElements, dwElementCount, lpAddress, lpdwAddressSize );

  /* Parameter check */
  if( ( lpElements == NULL ) ||
      ( dwElementCount == 0 )   /* FIXME: Not sure if this is a failure case */
    )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* Add the total size chunk */
  dwSizeRequired += sizeof( DPADDRESS ) + sizeof( DWORD );

  /* Calculate the size of the buffer required */
  for ( dwElements = dwElementCount; dwElements > 0; --dwElements, ++lpElements )
  {
    if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ServiceProvider ) ) ||
         ( IsEqualGUID( &lpElements->guidDataType, &DPAID_LobbyProvider ) )
       )
    {
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( GUID );
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Phone ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Modem ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INet ) )
            )
    {
      if( !bAnsiInterface )
      {
        ERR( "Ansi GUIDs used for unicode interface\n" );
        return DPERR_INVALIDFLAGS;
      }

      dwSizeRequired += sizeof( DPADDRESS ) + lpElements->dwDataSize;
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_PhoneW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ModemW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetW ) )
            )
    {
      if( bAnsiInterface )
      {
        ERR( "Unicode GUIDs used for ansi interface\n" );
        return DPERR_INVALIDFLAGS;
      }

      FIXME( "Right size for unicode interface?\n" );
      dwSizeRequired += sizeof( DPADDRESS ) + lpElements->dwDataSize * sizeof( WCHAR );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetPort ) )
    {
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( WORD );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ComPort ) )
    {
      FIXME( "Right size for unicode interface?\n" );
      dwSizeRequired += sizeof( DPADDRESS ) + sizeof( DPCOMPORTADDRESS ); /* FIXME: Right size? */
    }
    else
    {
      ERR( "Unknown GUID %s\n", debugstr_guid(&lpElements->guidDataType) );
      return DPERR_INVALIDFLAGS;
    }
  }

  /* The user wants to know how big a buffer to allocate for us */
  if( ( lpAddress == NULL ) ||
      ( *lpdwAddressSize < dwSizeRequired )
    )
  {
    *lpdwAddressSize = dwSizeRequired;
    return DPERR_BUFFERTOOSMALL;
  }

  /* Add the total size chunk */
  {
    LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

    CopyMemory( &lpdpAddress->guidDataType, &DPAID_TotalSize, sizeof( GUID ) );
    lpdpAddress->dwDataSize = sizeof( DWORD );
    lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

    *(LPDWORD)lpAddress = dwSizeRequired;
    lpAddress = (char *) lpAddress + sizeof( DWORD );
  }

  /* Calculate the size of the buffer required */
  for( dwElements = dwElementCount, lpElements = lpOrigElements;
       dwElements > 0;
       --dwElements, ++lpElements )
  {
    if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ServiceProvider ) ) ||
         ( IsEqualGUID( &lpElements->guidDataType, &DPAID_LobbyProvider ) )
       )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      CopyMemory( &lpdpAddress->guidDataType, &lpElements->guidDataType,
                  sizeof( GUID ) );
      lpdpAddress->dwDataSize = sizeof( GUID );
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      CopyMemory( lpAddress, lpElements->lpData, sizeof( GUID ) );
      lpAddress = (char *) lpAddress + sizeof( GUID );
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Phone ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_Modem ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INet ) )
            )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      CopyMemory( &lpdpAddress->guidDataType, &lpElements->guidDataType,
                  sizeof( GUID ) );
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      lstrcpynA( (LPSTR)lpAddress,
                 (LPCSTR)lpElements->lpData,
                 lpElements->dwDataSize );
      lpAddress = (char *) lpAddress + lpElements->dwDataSize;
    }
    else if ( ( IsEqualGUID( &lpElements->guidDataType, &DPAID_PhoneW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ModemW ) ) ||
              ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetW ) )
            )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      CopyMemory( &lpdpAddress->guidDataType, &lpElements->guidDataType,
                  sizeof( GUID ) );
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      lstrcpynW( (LPWSTR)lpAddress,
                 (LPCWSTR)lpElements->lpData,
                 lpElements->dwDataSize );
      lpAddress = (char *) lpAddress + lpElements->dwDataSize * sizeof( WCHAR );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_INetPort ) )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      CopyMemory( &lpdpAddress->guidDataType, &lpElements->guidDataType,
                  sizeof( GUID ) );
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      *((LPWORD)lpAddress) = *((LPWORD)lpElements->lpData);
      lpAddress = (char *) lpAddress + sizeof( WORD );
    }
    else if ( IsEqualGUID( &lpElements->guidDataType, &DPAID_ComPort ) )
    {
      LPDPADDRESS lpdpAddress = (LPDPADDRESS)lpAddress;

      CopyMemory( &lpdpAddress->guidDataType, &lpElements->guidDataType,
                  sizeof( GUID ) );
      lpdpAddress->dwDataSize = lpElements->dwDataSize;
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );

      CopyMemory( lpAddress, lpElements->lpData, sizeof( DPADDRESS ) );
      lpAddress = (char *) lpAddress + sizeof( DPADDRESS );
    }
  }

  return DP_OK;
}

/* DPL 3 methods */

static HRESULT WINAPI IDirectPlayLobby3WImpl_ConnectEx
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, REFIID riid,
  LPVOID* lplpDP, IUnknown* pUnk )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface ;
  return DPL_ConnectEx( This, dwFlags, riid, lplpDP, pUnk );
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_ConnectEx
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, REFIID riid,
  LPVOID* lplpDP, IUnknown* pUnk )
{
  IDirectPlayLobbyAImpl *This = (IDirectPlayLobbyAImpl *)iface ;
  return DPL_ConnectEx( This, dwFlags, riid, lplpDP, pUnk );
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_RegisterApplication
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, LPDPAPPLICATIONDESC lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_RegisterApplication
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, LPDPAPPLICATIONDESC lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_UnregisterApplication
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags, REFGUID lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_UnregisterApplication
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags, REFGUID lpAppDesc )
{
  FIXME(":stub\n");
  return DP_OK;
}

static HRESULT WINAPI IDirectPlayLobby3WImpl_WaitForConnectionSettings
( LPDIRECTPLAYLOBBY3 iface, DWORD dwFlags )
{
  HRESULT hr         = DP_OK;
  BOOL    bStartWait = (dwFlags & DPLWAIT_CANCEL) ? FALSE : TRUE;

  TRACE( "(%p)->(0x%08lx)\n", iface, dwFlags );

  if( DPLAYX_WaitForConnectionSettings( bStartWait ) )
  {
    /* FIXME: What is the correct error return code? */
    hr = DPERR_NOTLOBBIED;
  }

  return hr;
}

static HRESULT WINAPI IDirectPlayLobby3AImpl_WaitForConnectionSettings
( LPDIRECTPLAYLOBBY3A iface, DWORD dwFlags )
{
  HRESULT hr         = DP_OK;
  BOOL    bStartWait = (dwFlags & DPLWAIT_CANCEL) ? FALSE : TRUE;

  TRACE( "(%p)->(0x%08lx)\n", iface, dwFlags );

  if( DPLAYX_WaitForConnectionSettings( bStartWait ) )
  {
    /* FIXME: What is the correct error return code? */
    hr = DPERR_NOTLOBBIED;
  }

  return hr;
}


/* Virtual Table definitions for DPL{1,2,3}{A,W} */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobbyAVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 1 (ascii) Virtual Table for methods */
/* All lobby 1 methods are exactly the same except QueryInterface */
static const IDirectPlayLobbyVtbl directPlayLobbyAVT =
{

  XCAST(QueryInterface)DPL_QueryInterface,
  XCAST(AddRef)DPL_AddRef,
  XCAST(Release)DPL_Release,

  IDirectPlayLobbyAImpl_Connect,
  IDirectPlayLobbyAImpl_CreateAddress,
  IDirectPlayLobbyAImpl_EnumAddress,
  IDirectPlayLobbyAImpl_EnumAddressTypes,
  IDirectPlayLobbyAImpl_EnumLocalApplications,
  IDirectPlayLobbyAImpl_GetConnectionSettings,
  IDirectPlayLobbyAImpl_ReceiveLobbyMessage,
  IDirectPlayLobbyAImpl_RunApplication,
  IDirectPlayLobbyAImpl_SendLobbyMessage,
  IDirectPlayLobbyAImpl_SetConnectionSettings,
  IDirectPlayLobbyAImpl_SetLobbyMessageEvent
};
#undef XCAST


/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobbyWVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 1 (unicode) Virtual Table for methods */
static const IDirectPlayLobbyVtbl directPlayLobbyWVT =
{

  XCAST(QueryInterface)DPL_QueryInterface,
  XCAST(AddRef)DPL_AddRef,
  XCAST(Release)DPL_Release,

  IDirectPlayLobbyWImpl_Connect,
  IDirectPlayLobbyWImpl_CreateAddress,
  IDirectPlayLobbyWImpl_EnumAddress,
  IDirectPlayLobbyWImpl_EnumAddressTypes,
  IDirectPlayLobbyWImpl_EnumLocalApplications,
  IDirectPlayLobbyWImpl_GetConnectionSettings,
  IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  IDirectPlayLobbyWImpl_RunApplication,
  IDirectPlayLobbyWImpl_SendLobbyMessage,
  IDirectPlayLobbyWImpl_SetConnectionSettings,
  IDirectPlayLobbyWImpl_SetLobbyMessageEvent
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby2AVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 2 (ascii) Virtual Table for methods */
static const IDirectPlayLobby2Vtbl directPlayLobby2AVT =
{

  XCAST(QueryInterface)DPL_QueryInterface,
  XCAST(AddRef)DPL_AddRef,
  XCAST(Release)DPL_Release,

  XCAST(Connect)IDirectPlayLobbyAImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyAImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyAImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyAImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyAImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyAImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyAImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyAImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyAImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyAImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyAImpl_SetLobbyMessageEvent,

  IDirectPlayLobby2AImpl_CreateCompoundAddress
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby2AVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

/* Direct Play Lobby 2 (unicode) Virtual Table for methods */
static const IDirectPlayLobby2Vtbl directPlayLobby2WVT =
{

  XCAST(QueryInterface)DPL_QueryInterface,
  XCAST(AddRef)DPL_AddRef,
  XCAST(Release)DPL_Release,

  XCAST(Connect)IDirectPlayLobbyWImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyWImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyWImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyWImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyWImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyWImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyWImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyWImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyWImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyWImpl_SetLobbyMessageEvent,

  IDirectPlayLobby2WImpl_CreateCompoundAddress
};
#undef XCAST

/* Direct Play Lobby 3 (ascii) Virtual Table for methods */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby3AVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirectPlayLobby3Vtbl directPlayLobby3AVT =
{
  XCAST(QueryInterface)DPL_QueryInterface,
  XCAST(AddRef)DPL_AddRef,
  XCAST(Release)DPL_Release,

  XCAST(Connect)IDirectPlayLobbyAImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyAImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyAImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyAImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyAImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyAImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyAImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyAImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyAImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyAImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyAImpl_SetLobbyMessageEvent,

  XCAST(CreateCompoundAddress)IDirectPlayLobby2AImpl_CreateCompoundAddress,

  IDirectPlayLobby3AImpl_ConnectEx,
  IDirectPlayLobby3AImpl_RegisterApplication,
  IDirectPlayLobby3AImpl_UnregisterApplication,
  IDirectPlayLobby3AImpl_WaitForConnectionSettings
};
#undef XCAST

/* Direct Play Lobby 3 (unicode) Virtual Table for methods */

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlayLobby3WVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirectPlayLobby3Vtbl directPlayLobby3WVT =
{
  XCAST(QueryInterface)DPL_QueryInterface,
  XCAST(AddRef)DPL_AddRef,
  XCAST(Release)DPL_Release,

  XCAST(Connect)IDirectPlayLobbyWImpl_Connect,
  XCAST(CreateAddress)IDirectPlayLobbyWImpl_CreateAddress,
  XCAST(EnumAddress)IDirectPlayLobbyWImpl_EnumAddress,
  XCAST(EnumAddressTypes)IDirectPlayLobbyWImpl_EnumAddressTypes,
  XCAST(EnumLocalApplications)IDirectPlayLobbyWImpl_EnumLocalApplications,
  XCAST(GetConnectionSettings)IDirectPlayLobbyWImpl_GetConnectionSettings,
  XCAST(ReceiveLobbyMessage)IDirectPlayLobbyWImpl_ReceiveLobbyMessage,
  XCAST(RunApplication)IDirectPlayLobbyWImpl_RunApplication,
  XCAST(SendLobbyMessage)IDirectPlayLobbyWImpl_SendLobbyMessage,
  XCAST(SetConnectionSettings)IDirectPlayLobbyWImpl_SetConnectionSettings,
  XCAST(SetLobbyMessageEvent)IDirectPlayLobbyWImpl_SetLobbyMessageEvent,

  XCAST(CreateCompoundAddress)IDirectPlayLobby2WImpl_CreateCompoundAddress,

  IDirectPlayLobby3WImpl_ConnectEx,
  IDirectPlayLobby3WImpl_RegisterApplication,
  IDirectPlayLobby3WImpl_UnregisterApplication,
  IDirectPlayLobby3WImpl_WaitForConnectionSettings
};
#undef XCAST


/*********************************************************
 *
 * Direct Play Lobby Interface Implementation
 *
 *********************************************************/

/***************************************************************************
 *  DirectPlayLobbyCreateA   (DPLAYX.4)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateA( LPGUID lpGUIDDSP,
                                       LPDIRECTPLAYLOBBYA *lplpDPL,
                                       IUnknown *lpUnk,
                                       LPVOID lpData,
                                       DWORD dwDataSize )
{
  TRACE("lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     return DPERR_INVALIDPARAMS;
  }

  if( lpUnk )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return CLASS_E_NOAGGREGATION;
  }

  return DPL_CreateInterface( &IID_IDirectPlayLobbyA, (void**)lplpDPL );
}

/***************************************************************************
 *  DirectPlayLobbyCreateW   (DPLAYX.5)
 *
 */
HRESULT WINAPI DirectPlayLobbyCreateW( LPGUID lpGUIDDSP,
                                       LPDIRECTPLAYLOBBY *lplpDPL,
                                       IUnknown *lpUnk,
                                       LPVOID lpData,
                                       DWORD dwDataSize )
{
  TRACE("lpGUIDDSP=%p lplpDPL=%p lpUnk=%p lpData=%p dwDataSize=%08lx\n",
        lpGUIDDSP,lplpDPL,lpUnk,lpData,dwDataSize);

  /* Parameter Check: lpGUIDSP, lpUnk & lpData must be NULL. dwDataSize must
   * equal 0. These fields are mostly for future expansion.
   */
  if ( lpGUIDDSP || lpData || dwDataSize )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return DPERR_INVALIDPARAMS;
  }

  if( lpUnk )
  {
     *lplpDPL = NULL;
     ERR("Bad parameters!\n" );
     return CLASS_E_NOAGGREGATION;
  }

  return DPL_CreateInterface( &IID_IDirectPlayLobby, (void**)lplpDPL );
}
