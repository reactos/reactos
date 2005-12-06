/* This contains the implementation of the Lobby Service
 * Providers interface required to communicate with Direct Play
 *
 * Copyright 2001 Peter Hunnisett
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

#include "winerror.h"
#include "wine/debug.h"

#include "lobbysp.h"
#include "dplay_global.h"
#include "dpinit.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* Prototypes */
static BOOL DPLSP_CreateIUnknown( LPVOID lpSP );
static BOOL DPLSP_DestroyIUnknown( LPVOID lpSP );
static BOOL DPLSP_CreateDPLobbySP( LPVOID lpSP, IDirectPlay2Impl* dp );
static BOOL DPLSP_DestroyDPLobbySP( LPVOID lpSP );


/* Predefine the interface */
typedef struct IDPLobbySPImpl IDPLobbySPImpl;

typedef struct tagDPLobbySPIUnknownData
{
  LONG              ulObjRef;
  CRITICAL_SECTION  DPLSP_lock;
} DPLobbySPIUnknownData;

typedef struct tagDPLobbySPData
{
  IDirectPlay2Impl* dplay;
} DPLobbySPData;

#define DPLSP_IMPL_FIELDS \
   LONG ulInterfaceRef; \
   DPLobbySPIUnknownData* unk; \
   DPLobbySPData* sp;

struct IDPLobbySPImpl
{
  const IDPLobbySPVtbl *lpVtbl;
  DPLSP_IMPL_FIELDS
};

/* Forward declaration of virtual tables */
static const IDPLobbySPVtbl dpLobbySPVT;

HRESULT DPLSP_CreateInterface( REFIID riid, LPVOID* ppvObj, IDirectPlay2Impl* dp )
{
  TRACE( " for %s\n", debugstr_guid( riid ) );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( IDPLobbySPImpl ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  if( IsEqualGUID( &IID_IDPLobbySP, riid ) )
  {
    IDPLobbySPImpl *This = (IDPLobbySPImpl *)*ppvObj;
    This->lpVtbl = &dpLobbySPVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  /* Initialize it */
  if( DPLSP_CreateIUnknown( *ppvObj ) &&
      DPLSP_CreateDPLobbySP( *ppvObj, dp )
    )
  {
    IDPLobbySP_AddRef( (LPDPLOBBYSP)*ppvObj );
    return S_OK;
  }

  /* Initialize failed, destroy it */
  DPLSP_DestroyDPLobbySP( *ppvObj );
  DPLSP_DestroyIUnknown( *ppvObj );

  HeapFree( GetProcessHeap(), 0, *ppvObj );
  *ppvObj = NULL;

  return DPERR_NOMEMORY;
}

static BOOL DPLSP_CreateIUnknown( LPVOID lpSP )
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)lpSP;

  This->unk = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->unk) ) );

  if ( This->unk == NULL )
  {
    return FALSE;
  }

  InitializeCriticalSection( &This->unk->DPLSP_lock );

  return TRUE;
}

static BOOL DPLSP_DestroyIUnknown( LPVOID lpSP )
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)lpSP;

  DeleteCriticalSection( &This->unk->DPLSP_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}

static BOOL DPLSP_CreateDPLobbySP( LPVOID lpSP, IDirectPlay2Impl* dp )
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)lpSP;

  This->sp = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->sp) ) );

  if ( This->sp == NULL )
  {
    return FALSE;
  }

  This->sp->dplay = dp;

  /* Normally we should be keeping a reference, but since only the dplay
   * interface that created us can destroy us, we do not keep a reference
   * to it (ie we'd be stuck with always having one reference to the dplay
   * object, and hence us, around).
   * NOTE: The dp object does reference count us.
   *
   * FIXME: This is a kludge to get around a problem where a queryinterface
   *        is used to get a new interface and then is closed. We will then
   *        reference garbage. However, with this we will never deallocate
   *        the interface we store. The correct fix is to require all
   *        DP internal interfaces to use the This->dp2 interface which
   *        should be changed to This->dp
   */
  IDirectPlayX_AddRef( (LPDIRECTPLAY2)dp );


  return TRUE;
}

static BOOL DPLSP_DestroyDPLobbySP( LPVOID lpSP )
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)lpSP;

  HeapFree( GetProcessHeap(), 0, This->sp );

  return TRUE;
}

static
HRESULT WINAPI DPLSP_QueryInterface
( LPDPLOBBYSP iface,
  REFIID riid,
  LPVOID* ppvObj
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  TRACE("(%p)->(%s,%p)\n", This, debugstr_guid( riid ), ppvObj );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( *This ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  CopyMemory( *ppvObj, This, sizeof( *This )  );
  (*(IDPLobbySPImpl**)ppvObj)->ulInterfaceRef = 0;

  if( IsEqualGUID( &IID_IDPLobbySP, riid ) )
  {
    IDPLobbySPImpl *This = (IDPLobbySPImpl *)*ppvObj;
    This->lpVtbl = &dpLobbySPVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  IDPLobbySP_AddRef( (LPDPLOBBYSP)*ppvObj );

  return S_OK;
}

static
ULONG WINAPI DPLSP_AddRef
( LPDPLOBBYSP iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;

  ulObjRefCount       = InterlockedIncrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedIncrement( &This->ulInterfaceRef );

  TRACE( "ref count incremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  return ulObjRefCount;
}

static
ULONG WINAPI DPLSP_Release
( LPDPLOBBYSP iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;

  ulObjRefCount       = InterlockedDecrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedDecrement( &This->ulInterfaceRef );

  TRACE( "ref count decremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  /* Deallocate if this is the last reference to the object */
  if( ulObjRefCount == 0 )
  {
     DPLSP_DestroyDPLobbySP( This );
     DPLSP_DestroyIUnknown( This );
  }

  if( ulInterfaceRefCount == 0 )
  {
    HeapFree( GetProcessHeap(), 0, This );
  }

  return ulInterfaceRefCount;
}

static
HRESULT WINAPI IDPLobbySPImpl_AddGroupToGroup
( LPDPLOBBYSP iface,
  LPSPDATA_ADDREMOTEGROUPTOGROUP argtg
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, argtg );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_AddPlayerToGroup
( LPDPLOBBYSP iface,
  LPSPDATA_ADDREMOTEPLAYERTOGROUP arptg
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, arptg );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_CreateGroup
( LPDPLOBBYSP iface,
  LPSPDATA_CREATEREMOTEGROUP crg
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, crg );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_CreateGroupInGroup
( LPDPLOBBYSP iface,
  LPSPDATA_CREATEREMOTEGROUPINGROUP crgig
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, crgig );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_DeleteGroupFromGroup
( LPDPLOBBYSP iface,
  LPSPDATA_DELETEREMOTEGROUPFROMGROUP drgfg
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, drgfg );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_DeletePlayerFromGroup
( LPDPLOBBYSP iface,
  LPSPDATA_DELETEREMOTEPLAYERFROMGROUP drpfg
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, drpfg );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_DestroyGroup
( LPDPLOBBYSP iface,
  LPSPDATA_DESTROYREMOTEGROUP drg
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, drg );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_EnumSessionsResponse
( LPDPLOBBYSP iface,
  LPSPDATA_ENUMSESSIONSRESPONSE er
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, er );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_GetSPDataPointer
( LPDPLOBBYSP iface,
  LPVOID* lplpData
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, lplpData );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_HandleMessage
( LPDPLOBBYSP iface,
  LPSPDATA_HANDLEMESSAGE hm
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, hm );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_SendChatMessage
( LPDPLOBBYSP iface,
  LPSPDATA_CHATMESSAGE cm
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, cm );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_SetGroupName
( LPDPLOBBYSP iface,
  LPSPDATA_SETREMOTEGROUPNAME srgn
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, srgn );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_SetPlayerName
( LPDPLOBBYSP iface,
  LPSPDATA_SETREMOTEPLAYERNAME srpn
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, srpn );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_SetSessionDesc
( LPDPLOBBYSP iface,
  LPSPDATA_SETSESSIONDESC ssd
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, ssd );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_SetSPDataPointer
( LPDPLOBBYSP iface,
  LPVOID lpData
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, lpData );
  return DP_OK;
}

static
HRESULT WINAPI IDPLobbySPImpl_StartSession
( LPDPLOBBYSP iface,
  LPSPDATA_STARTSESSIONCOMMAND ssc
)
{
  IDPLobbySPImpl *This = (IDPLobbySPImpl *)iface;
  FIXME( "(%p)->(%p):stub\n", This, ssc );
  return DP_OK;
}


static const IDPLobbySPVtbl dpLobbySPVT =
{

  DPLSP_QueryInterface,
  DPLSP_AddRef,
  DPLSP_Release,

  IDPLobbySPImpl_AddGroupToGroup,
  IDPLobbySPImpl_AddPlayerToGroup,
  IDPLobbySPImpl_CreateGroup,
  IDPLobbySPImpl_CreateGroupInGroup,
  IDPLobbySPImpl_DeleteGroupFromGroup,
  IDPLobbySPImpl_DeletePlayerFromGroup,
  IDPLobbySPImpl_DestroyGroup,
  IDPLobbySPImpl_EnumSessionsResponse,
  IDPLobbySPImpl_GetSPDataPointer,
  IDPLobbySPImpl_HandleMessage,
  IDPLobbySPImpl_SendChatMessage,
  IDPLobbySPImpl_SetGroupName,
  IDPLobbySPImpl_SetPlayerName,
  IDPLobbySPImpl_SetSessionDesc,
  IDPLobbySPImpl_SetSPDataPointer,
  IDPLobbySPImpl_StartSession

};
