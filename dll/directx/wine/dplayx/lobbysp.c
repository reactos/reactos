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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "winerror.h"
#include "wine/debug.h"

#include "lobbysp.h"
#include "dplay_global.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

typedef struct IDPLobbySPImpl
{
  IDPLobbySP IDPLobbySP_iface;
  LONG ref;
  IDirectPlayImpl *dplay;
} IDPLobbySPImpl;

static inline IDPLobbySPImpl *impl_from_IDPLobbySP(IDPLobbySP *iface)
{
    return CONTAINING_RECORD(iface, IDPLobbySPImpl, IDPLobbySP_iface);
}

static HRESULT WINAPI IDPLobbySPImpl_QueryInterface( IDPLobbySP *iface, REFIID riid,
        void **ppv )
{
  TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid( riid ), ppv );

  if ( IsEqualGUID( &IID_IUnknown, riid ) || IsEqualGUID( &IID_IDPLobbySP, riid ) )
  {
    *ppv = iface;
    IDPLobbySP_AddRef(iface);
    return S_OK;
  }

  FIXME("Unsupported interface %s\n", debugstr_guid(riid));
  *ppv = NULL;
  return E_NOINTERFACE;
}

static ULONG WINAPI IDPLobbySPImpl_AddRef( IDPLobbySP *iface )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  ULONG ref = InterlockedIncrement( &This->ref );

  TRACE( "(%p) ref=%ld\n", This, ref );

  return ref;
}

static ULONG WINAPI IDPLobbySPImpl_Release( IDPLobbySP *iface )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  ULONG ref = InterlockedDecrement( &This->ref );

  TRACE( "(%p) ref=%ld\n", This, ref );

  if( !ref )
    free( This );

  return ref;
}

static HRESULT WINAPI IDPLobbySPImpl_AddGroupToGroup( IDPLobbySP *iface,
        SPDATA_ADDREMOTEGROUPTOGROUP *argtg )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, argtg );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_AddPlayerToGroup( IDPLobbySP *iface,
        SPDATA_ADDREMOTEPLAYERTOGROUP *arptg )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, arptg );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_CreateGroup( IDPLobbySP *iface,
        SPDATA_CREATEREMOTEGROUP *crg )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, crg );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_CreateGroupInGroup( IDPLobbySP *iface,
        SPDATA_CREATEREMOTEGROUPINGROUP *crgig )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, crgig );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_DeleteGroupFromGroup( IDPLobbySP *iface,
        SPDATA_DELETEREMOTEGROUPFROMGROUP *drgfg )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, drgfg );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_DeletePlayerFromGroup( IDPLobbySP *iface,
        SPDATA_DELETEREMOTEPLAYERFROMGROUP *drpfg )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, drpfg );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_DestroyGroup( IDPLobbySP *iface,
        SPDATA_DESTROYREMOTEGROUP *drg )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, drg );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_EnumSessionsResponse( IDPLobbySP *iface,
        SPDATA_ENUMSESSIONSRESPONSE *er )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, er );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_GetSPDataPointer( IDPLobbySP *iface, LPVOID* lplpData )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, lplpData );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_HandleMessage( IDPLobbySP *iface, SPDATA_HANDLEMESSAGE *hm )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, hm );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_SendChatMessage( IDPLobbySP *iface,
        SPDATA_CHATMESSAGE *cm )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, cm );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_SetGroupName( IDPLobbySP *iface,
        SPDATA_SETREMOTEGROUPNAME *srgn )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, srgn );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_SetPlayerName( IDPLobbySP *iface,
        SPDATA_SETREMOTEPLAYERNAME *srpn )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, srpn );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_SetSessionDesc( IDPLobbySP *iface,
        SPDATA_SETSESSIONDESC *ssd )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, ssd );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_SetSPDataPointer( IDPLobbySP *iface, void *lpData )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, lpData );
  return DP_OK;
}

static HRESULT WINAPI IDPLobbySPImpl_StartSession( IDPLobbySP *iface,
        SPDATA_STARTSESSIONCOMMAND *ssc )
{
  IDPLobbySPImpl *This = impl_from_IDPLobbySP( iface );
  FIXME( "(%p)->(%p):stub\n", This, ssc );
  return DP_OK;
}


static const IDPLobbySPVtbl dpLobbySPVT =
{
  IDPLobbySPImpl_QueryInterface,
  IDPLobbySPImpl_AddRef,
  IDPLobbySPImpl_Release,
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

HRESULT dplobbysp_create( REFIID riid, void **ppv, IDirectPlayImpl *dp )
{
  IDPLobbySPImpl *obj;
  HRESULT hr;

  TRACE( "(%s, %p)\n", debugstr_guid( riid ), ppv );

  *ppv = NULL;
  obj = malloc( sizeof( *obj ) );
  if ( !obj )
    return DPERR_OUTOFMEMORY;

  obj->IDPLobbySP_iface.lpVtbl = &dpLobbySPVT;
  obj->ref = 1;
  obj->dplay = dp;

  hr = IDPLobbySP_QueryInterface( &obj->IDPLobbySP_iface, riid, ppv );
  IDPLobbySP_Release( &obj->IDPLobbySP_iface );

  return hr;
}
