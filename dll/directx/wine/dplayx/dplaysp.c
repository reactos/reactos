/* This contains the implementation of the interface Service
 * Providers require to communicate with Direct Play
 *
 * Copyright 2000 Peter Hunnisett
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

#include <string.h>
#include "winerror.h"
#include "wine/debug.h"

#include "wine/dplaysp.h"
#include "dplay_global.h"
#include "name_server.h"
#include "dplayx_messages.h"

#include "dplayx_global.h" /* FIXME: For global hack */

/* FIXME: Need to add interface locking inside procedures */

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

typedef struct IDirectPlaySPImpl
{
  IDirectPlaySP IDirectPlaySP_iface;
  LONG ref;
  void *remote_data;
  DWORD remote_data_size;
  void *local_data;
  DWORD local_data_size;
  IDirectPlayImpl *dplay; /* FIXME: This should perhaps be iface not impl */
} IDirectPlaySPImpl;

/* This structure is passed to the DP object for safe keeping */
typedef struct tagDP_SPPLAYERDATA
{
  LPVOID lpPlayerLocalData;
  DWORD  dwPlayerLocalDataSize;

  LPVOID lpPlayerRemoteData;
  DWORD  dwPlayerRemoteDataSize;
} DP_SPPLAYERDATA, *LPDP_SPPLAYERDATA;


static inline IDirectPlaySPImpl *impl_from_IDirectPlaySP( IDirectPlaySP *iface )
{
  return CONTAINING_RECORD( iface, IDirectPlaySPImpl, IDirectPlaySP_iface );
}

static HRESULT WINAPI IDirectPlaySPImpl_QueryInterface( IDirectPlaySP *iface, REFIID riid,
        void **ppv )
{
  TRACE("(%p)->(%s,%p)\n", iface, debugstr_guid( riid ), ppv );

  if ( IsEqualGUID( &IID_IUnknown, riid ) || IsEqualGUID( &IID_IDirectPlaySP, riid ) )
  {
    *ppv = iface;
    IDirectPlaySP_AddRef( iface );
    return S_OK;
  }

  FIXME( "Unsupported interface %s\n", debugstr_guid( riid ) );
  *ppv = NULL;
  return E_NOINTERFACE;
}

static ULONG WINAPI IDirectPlaySPImpl_AddRef( IDirectPlaySP *iface )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  ULONG ref = InterlockedIncrement( &This->ref );

  TRACE( "(%p) ref=%ld\n", This, ref );

  return ref;
}

static ULONG WINAPI IDirectPlaySPImpl_Release( IDirectPlaySP *iface )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  ULONG ref = InterlockedDecrement( &This->ref );

  TRACE( "(%p) ref=%ld\n", This, ref );

  if( !ref )
  {
    free( This->remote_data );
    free( This->local_data );
    free( This );
  }

  return ref;
}

static HRESULT WINAPI IDirectPlaySPImpl_AddMRUEntry( IDirectPlaySP *iface, LPCWSTR lpSection,
        LPCWSTR lpKey, const void *lpData, DWORD dwDataSize, DWORD dwMaxEntries )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  /* Should be able to call the comctl32 undocumented MRU routines.
     I suspect that the interface works appropriately */
  FIXME( "(%p)->(%p,%p%p,0x%08lx,0x%08lx): stub\n",
         This, lpSection, lpKey, lpData, dwDataSize, dwMaxEntries );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_CreateAddress( IDirectPlaySP *iface, REFGUID guidSP,
        REFGUID guidDataType, const void *lpData, DWORD dwDataSize, void *lpAddress,
        DWORD *lpdwAddressSize )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  FIXME( "(%p)->(%s,%s,%p,0x%08lx,%p,%p): stub\n",
         This, debugstr_guid(guidSP), debugstr_guid(guidDataType),
         lpData, dwDataSize, lpAddress, lpdwAddressSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_EnumAddress( IDirectPlaySP *iface,
        LPDPENUMADDRESSCALLBACK lpEnumAddressCallback, const void *lpAddress, DWORD dwAddressSize,
        void *lpContext )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  TRACE( "(%p)->(%p,%p,0x%08lx,%p)\n",
         This, lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );

  DPL_EnumAddress( lpEnumAddressCallback, lpAddress, dwAddressSize, lpContext );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_EnumMRUEntries( IDirectPlaySP *iface, LPCWSTR lpSection,
        LPCWSTR lpKey, LPENUMMRUCALLBACK lpEnumMRUCallback, void *lpContext )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  /* Should be able to call the comctl32 undocumented MRU routines.
     I suspect that the interface works appropriately */
  FIXME( "(%p)->(%p,%p,%p,%p): stub\n",
         This, lpSection, lpKey, lpEnumMRUCallback, lpContext );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetPlayerFlags( IDirectPlaySP *iface, DPID idPlayer,
        DWORD *lpdwPlayerFlags )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  FIXME( "(%p)->(0x%08lx,%p): stub\n",
         This, idPlayer, lpdwPlayerFlags );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetSPPlayerData( IDirectPlaySP *iface, DPID idPlayer,
        void **lplpData, DWORD *lpdwDataSize, DWORD dwFlags )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  HRESULT hr;
  LPDP_SPPLAYERDATA lpPlayerData;

  TRACE( "(%p)->(0x%08lx,%p,%p,0x%08lx)\n",
         This, idPlayer, lplpData, lpdwDataSize, dwFlags );

  hr = DP_GetSPPlayerData( This->dplay, idPlayer, (void**)&lpPlayerData );

  if( FAILED(hr) )
  {
    TRACE( "Couldn't get player data: %s\n", DPLAYX_HresultToString(hr) );
    return DPERR_INVALIDPLAYER;
  }

  /* What to do in the case where there is nothing set yet? */
  if( dwFlags == DPSET_LOCAL )
  {
    *lplpData     = lpPlayerData->lpPlayerLocalData;
    *lpdwDataSize = lpPlayerData->dwPlayerLocalDataSize;
  }
  else if( dwFlags == DPSET_REMOTE )
  {
    *lplpData     = lpPlayerData->lpPlayerRemoteData;
    *lpdwDataSize = lpPlayerData->dwPlayerRemoteDataSize;
  }

  if( *lplpData == NULL )
  {
    hr = DPERR_GENERIC;
  }

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_HandleMessage( IDirectPlaySP *iface, void *lpMessageBody,
        DWORD dwMessageBodySize, void *lpMessageHeader )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  LPDPMSG_SENDENVELOPE lpMsg = lpMessageBody;
  HRESULT hr = DPERR_GENERIC;
  WORD wCommandId;
  WORD wVersion;
  DPSP_REPLYDATA data;

  TRACE( "(%p)->(%p,0x%08lx,%p)\n",
         This, lpMessageBody, dwMessageBodySize, lpMessageHeader );

  if ( dwMessageBodySize < sizeof( DPMSG_SENDENVELOPE ) )
    return DPERR_GENERIC;

  if ( lpMsg->dwMagic != DPMSGMAGIC_DPLAYMSG )
  {
    LPDPMSG_SYSMSGENVELOPE gameMsg = lpMessageBody;
    return DP_HandleGameMessage( This->dplay, lpMessageBody, dwMessageBodySize,
                                 gameMsg->dwPlayerFrom, gameMsg->dwPlayerTo );
  }

  wCommandId = lpMsg->wCommandId;
  wVersion   = lpMsg->wVersion;

  TRACE( "Incoming message has envelope of 0x%08lx, %u, %u\n",
         lpMsg->dwMagic, wCommandId, wVersion );

  if( lpMsg->dwMagic != DPMSGMAGIC_DPLAYMSG )
  {
    ERR( "Unknown magic 0x%08lx!\n", lpMsg->dwMagic );
    return DPERR_GENERIC;
  }

#if 0
  {
    const LPDWORD lpcHeader = lpMessageHeader;

    TRACE( "lpMessageHeader = [0x%08lx] [0x%08lx] [0x%08lx] [0x%08lx] [0x%08lx]\n",
           lpcHeader[0], lpcHeader[1], lpcHeader[2], lpcHeader[3], lpcHeader[4] );
   }
#endif

  /* Pass everything else to Direct Play */
  data.lpMessage     = NULL;
  data.dwMessageSize = 0;

  /* Pass this message to the dplay interface to handle */
  hr = DP_HandleMessage( This->dplay, lpMessageBody, dwMessageBodySize, lpMessageHeader,
                         wCommandId, wVersion, &data.lpMessage, &data.dwMessageSize );

  if( FAILED(hr) )
  {
    ERR( "Command processing failed %s\n", DPLAYX_HresultToString(hr) );
  }

  /* Do we want a reply? */
  if( data.lpMessage != NULL )
  {
    data.lpSPMessageHeader = lpMessageHeader;
    data.idNameServer      = 0;
    data.lpISP             = iface;

    hr = This->dplay->dp2->spData.lpCB->Reply( &data );

    if( FAILED(hr) )
    {
      ERR( "Reply failed %s\n", DPLAYX_HresultToString(hr) );
    }
  }

  return hr;

#if 0
  HRESULT hr = DP_OK;
  HANDLE  hReceiveEvent = 0;
  /* FIXME: Acquire some sort of interface lock */
  /* FIXME: Need some sort of context for this callback. Need to determine
   *        how this is actually done with the SP
   */
  /* FIXME: Who needs to delete the message when done? */
  switch( lpMsg->dwType )
  {
    case DPSYS_CREATEPLAYERORGROUP:
    {
      LPDPMSG_CREATEPLAYERORGROUP msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_CreatePlayer( This, lpMessageHeader, msg->dpId,
                                 &msg->dpnName, 0, msg->lpData,
                                 msg->dwDataSize, msg->dwFlags, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        /* Group in group situation? */
        if( msg->dpIdParent == DPID_NOPARENT_GROUP )
        {
          hr = DP_IF_CreateGroup( This, lpMessageHeader, msg->dpId,
                                  &msg->dpnName, 0, msg->lpData,
                                  msg->dwDataSize, msg->dwFlags, ... );
        }
        else /* Group in Group */
        {
          hr = DP_IF_CreateGroupInGroup( This, lpMessageHeader, msg->dpIdParent,
                                         &msg->dpnName, 0, msg->lpData,
                                         msg->dwDataSize, msg->dwFlags, ... );
        }
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for DPSYS_CREATEPLAYERORGROUP\n" );
        return;
      }

      break;
    }

    case DPSYS_DESTROYPLAYERORGROUP:
    {
      LPDPMSG_DESTROYPLAYERORGROUP msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_DestroyPlayer( This, msg->dpId, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_DestroyGroup( This, msg->dpId, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for DPSYS_DESTROYPLAYERORGROUP\n" );
        return;
      }

      break;
    }

    case DPSYS_ADDPLAYERTOGROUP:
    {
      LPDPMSG_ADDPLAYERTOGROUP msg = lpMsg;

      hr = DP_IF_AddPlayerToGroup( This, msg->dpIdGroup, msg->dpIdPlayer, ... );
      break;
    }

    case DPSYS_DELETEPLAYERFROMGROUP:
    {
      LPDPMSG_DELETEPLAYERFROMGROUP msg = lpMsg;

      hr = DP_IF_DeletePlayerFromGroup( This, msg->dpIdGroup, msg->dpIdPlayer,
                                        ... );

      break;
    }

    case DPSYS_SESSIONLOST:
    {
      LPDPMSG_SESSIONLOST msg = lpMsg;

      FIXME( "DPSYS_SESSIONLOST not handled\n" );

      break;
    }

    case DPSYS_HOST:
    {
      LPDPMSG_HOST msg = lpMsg;

      FIXME( "DPSYS_HOST not handled\n" );

      break;
    }

    case DPSYS_SETPLAYERORGROUPDATA:
    {
      LPDPMSG_SETPLAYERORGROUPDATA msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_SetPlayerData( This, msg->dpId, msg->lpData, msg->dwDataSize,                                  DPSET_REMOTE, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_SetGroupData( This, msg->dpId, msg->lpData, msg->dwDataSize,
                                 DPSET_REMOTE, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for LPDPMSG_SETPLAYERORGROUPDATA\n" );
        return;
      }

      break;
    }

    case DPSYS_SETPLAYERORGROUPNAME:
    {
      LPDPMSG_SETPLAYERORGROUPNAME msg = lpMsg;

      if( msg->dwPlayerType == DPPLAYERTYPE_PLAYER )
      {
        hr = DP_IF_SetPlayerName( This, msg->dpId, msg->dpnName, ... );
      }
      else if( msg->dwPlayerType == DPPLAYERTYPE_GROUP )
      {
        hr = DP_IF_SetGroupName( This, msg->dpId, msg->dpnName, ... );
      }
      else /* Hmmm? */
      {
        ERR( "Corrupt msg->dwPlayerType for LPDPMSG_SETPLAYERORGROUPDATA\n" );
        return;
      }

      break;
    }

    case DPSYS_SETSESSIONDESC;
    {
      LPDPMSG_SETSESSIONDESC msg = lpMsg;

      hr = DP_IF_SetSessionDesc( This, &msg->dpDesc );

      break;
    }

    case DPSYS_ADDGROUPTOGROUP:
    {
      LPDPMSG_ADDGROUPTOGROUP msg = lpMsg;

      hr = DP_IF_AddGroupToGroup( This, msg->dpIdParentGroup, msg->dpIdGroup,
                                  ... );

      break;
    }

    case DPSYS_DELETEGROUPFROMGROUP:
    {
      LPDPMSG_DELETEGROUPFROMGROUP msg = lpMsg;

      hr = DP_IF_DeleteGroupFromGroup( This, msg->dpIdParentGroup,
                                       msg->dpIdGroup, ... );

      break;
    }

    case DPSYS_SECUREMESSAGE:
    {
      LPDPMSG_SECUREMESSAGE msg = lpMsg;

      FIXME( "DPSYS_SECUREMESSAGE not implemented\n" );

      break;
    }

    case DPSYS_STARTSESSION:
    {
      LPDPMSG_STARTSESSION msg = lpMsg;

      FIXME( "DPSYS_STARTSESSION not implemented\n" );

      break;
    }

    case DPSYS_CHAT:
    {
      LPDPMSG_CHAT msg = lpMsg;

      FIXME( "DPSYS_CHAT not implemented\n" );

      break;
    }

    case DPSYS_SETGROUPOWNER:
    {
      LPDPMSG_SETGROUPOWNER msg = lpMsg;

      FIXME( "DPSYS_SETGROUPOWNER not implemented\n" );

      break;
    }

    case DPSYS_SENDCOMPLETE:
    {
      LPDPMSG_SENDCOMPLETE msg = lpMsg;

      FIXME( "DPSYS_SENDCOMPLETE not implemented\n" );

      break;
    }

    default:
    {
      /* NOTE: This should be a user defined type. There is nothing that we
       *       need to do with it except queue it.
       */
      TRACE( "Received user message type(?) 0x%08lx through SP.\n",
              lpMsg->dwType );
      break;
    }
  }

  FIXME( "Queue message in the receive queue. Need some context data!\n" );

  if( FAILED(hr) )
  {
    ERR( "Unable to perform action for msg type 0x%08lx\n", lpMsg->dwType );
  }
  /* If a receive event was registered for this player, invoke it */
  if( hReceiveEvent )
  {
    SetEvent( hReceiveEvent );
  }
#endif
}

static HRESULT WINAPI IDirectPlaySPImpl_SetSPPlayerData( IDirectPlaySP *iface, DPID idPlayer,
        void *lpData, DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  HRESULT           hr;
  LPDP_SPPLAYERDATA lpPlayerEntry;
  LPVOID            lpPlayerData;

  TRACE( "(%p)->(0x%08lx,%p,0x%08lx,0x%08lx)\n", This, idPlayer, lpData, dwDataSize, dwFlags );

  hr = DP_GetSPPlayerData( This->dplay, idPlayer, (void**)&lpPlayerEntry );
  if( FAILED(hr) )
  {
    /* Player must not exist */
    return DPERR_INVALIDPLAYER;
  }

  lpPlayerData = malloc( dwDataSize );
  CopyMemory( lpPlayerData, lpData, dwDataSize );

  if( dwFlags == DPSET_LOCAL )
  {
    lpPlayerEntry->lpPlayerLocalData = lpPlayerData;
    lpPlayerEntry->dwPlayerLocalDataSize = dwDataSize;
  }
  else if( dwFlags == DPSET_REMOTE )
  {
    lpPlayerEntry->lpPlayerRemoteData = lpPlayerData;
    lpPlayerEntry->dwPlayerRemoteDataSize = dwDataSize;
  }

  hr = DP_SetSPPlayerData( This->dplay, idPlayer, lpPlayerEntry );

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_CreateCompoundAddress( IDirectPlaySP *iface,
        const DPCOMPOUNDADDRESSELEMENT *lpElements, DWORD dwElementCount, void *lpAddress,
        DWORD *lpdwAddressSize )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  FIXME( "(%p)->(%p,0x%08lx,%p,%p): stub\n",
         This, lpElements, dwElementCount, lpAddress, lpdwAddressSize );

  return DP_OK;
}

static HRESULT WINAPI IDirectPlaySPImpl_GetSPData( IDirectPlaySP *iface, void **lplpData,
        DWORD *lpdwDataSize, DWORD dwFlags )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  HRESULT hr = DP_OK;

  TRACE( "(%p)->(%p,%p,0x%08lx)\n", This, lplpData, lpdwDataSize, dwFlags );

#if 0
  /* This is what the documentation says... */
  if( dwFlags != DPSET_REMOTE )
  {
    return DPERR_INVALIDPARAMS;
  }
#else
  /* ... but most service providers call this with 1 */
  /* Guess that this is using a DPSET_LOCAL or DPSET_REMOTE type of
   * thing?
   */
  if( dwFlags != DPSET_REMOTE )
  {
    TRACE( "Undocumented dwFlags 0x%08lx used\n", dwFlags );
  }
#endif

  /* FIXME: What to do in the case where this isn't initialized yet? */

  /* Yes, we're supposed to return a pointer to the memory we have stored! */
  if( dwFlags == DPSET_REMOTE )
  {
    *lpdwDataSize = This->remote_data_size;
    *lplpData = This->remote_data;

    if( !This->remote_data )
      hr = DPERR_GENERIC;
  }
  else if( dwFlags == DPSET_LOCAL )
  {
    *lpdwDataSize = This->local_data_size;
    *lplpData = This->local_data;

    if( !This->local_data )
      hr = DPERR_GENERIC;
  }

  return hr;
}

static HRESULT WINAPI IDirectPlaySPImpl_SetSPData( IDirectPlaySP *iface, void *lpData,
        DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );
  LPVOID lpSpData;

  TRACE( "(%p)->(%p,0x%08lx,0x%08lx)\n", This, lpData, dwDataSize, dwFlags );

#if 0
  /* This is what the documentation says... */
  if( dwFlags != DPSET_REMOTE )
  {
    return DPERR_INVALIDPARAMS;
  }
#else
  /* ... but most service providers call this with 1 */
  /* Guess that this is using a DPSET_LOCAL or DPSET_REMOTE type of
   * thing?
   */
  if( dwFlags != DPSET_REMOTE )
  {
    TRACE( "Undocumented dwFlags 0x%08lx used\n", dwFlags );
  }
#endif

  lpSpData = malloc( dwDataSize );
  CopyMemory( lpSpData, lpData, dwDataSize );

  /* If we have data already allocated, free it and replace it */
  if( dwFlags == DPSET_REMOTE )
  {
    free( This->remote_data );
    This->remote_data_size = dwDataSize;
    This->remote_data = lpSpData;
  }
  else if ( dwFlags == DPSET_LOCAL )
  {
    free( This->local_data );
    This->local_data = lpSpData;
    This->local_data_size = dwDataSize;
  }

  return DP_OK;
}

static void WINAPI IDirectPlaySPImpl_SendComplete( IDirectPlaySP *iface, void *unknownA,
        DWORD unknownB )
{
  IDirectPlaySPImpl *This = impl_from_IDirectPlaySP( iface );

  FIXME( "(%p)->(%p,0x%08lx): stub\n",
         This, unknownA, unknownB );
}

static const IDirectPlaySPVtbl directPlaySPVT =
{
  IDirectPlaySPImpl_QueryInterface,
  IDirectPlaySPImpl_AddRef,
  IDirectPlaySPImpl_Release,
  IDirectPlaySPImpl_AddMRUEntry,
  IDirectPlaySPImpl_CreateAddress,
  IDirectPlaySPImpl_EnumAddress,
  IDirectPlaySPImpl_EnumMRUEntries,
  IDirectPlaySPImpl_GetPlayerFlags,
  IDirectPlaySPImpl_GetSPPlayerData,
  IDirectPlaySPImpl_HandleMessage,
  IDirectPlaySPImpl_SetSPPlayerData,
  IDirectPlaySPImpl_CreateCompoundAddress,
  IDirectPlaySPImpl_GetSPData,
  IDirectPlaySPImpl_SetSPData,
  IDirectPlaySPImpl_SendComplete
};

HRESULT dplaysp_create( REFIID riid, void **ppv, IDirectPlayImpl *dp )
{
  IDirectPlaySPImpl *obj;
  HRESULT hr;

  TRACE( "(%s, %p)\n", debugstr_guid( riid ), ppv );

  *ppv = NULL;
  obj = calloc( 1, sizeof( *obj ) );
  if ( !obj )
    return DPERR_OUTOFMEMORY;

  obj->IDirectPlaySP_iface.lpVtbl = &directPlaySPVT;
  obj->ref = 1;
  obj->dplay = dp;

  hr = IDirectPlaySP_QueryInterface( &obj->IDirectPlaySP_iface, riid, ppv );
  IDirectPlaySP_Release( &obj->IDirectPlaySP_iface );

  return hr;
}

/* DP external interfaces to call into DPSP interface */

/* Allocate the structure */
LPVOID DPSP_CreateSPPlayerData(void)
{
  TRACE( "Creating SPPlayer data struct\n" );
  return calloc( 1, sizeof( DP_SPPLAYERDATA ) );
}
