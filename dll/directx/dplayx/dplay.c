/* Direct Play 2,3,4 Implementation
 *
 * Copyright 1998,1999,2000,2001 - Peter Hunnisett
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

#include "config.h"
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "winnt.h"
#include "winreg.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "dpinit.h"
#include "dplayx_global.h"
#include "name_server.h"
#include "dplayx_queue.h"
#include "dplaysp.h"
#include "dplay_global.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* FIXME: Should this be externed? */
extern HRESULT DPL_CreateCompoundAddress
( LPCDPCOMPOUNDADDRESSELEMENT lpElements, DWORD dwElementCount,
  LPVOID lpAddress, LPDWORD lpdwAddressSize, BOOL bAnsiInterface );


/* Local function prototypes */
static lpPlayerList DP_FindPlayer( IDirectPlay2AImpl* This, DPID dpid );
static lpPlayerData DP_CreatePlayer( IDirectPlay2Impl* iface, LPDPID lpid,
                                     LPDPNAME lpName, DWORD dwFlags,
                                     HANDLE hEvent, BOOL bAnsi );
static BOOL DP_CopyDPNAMEStruct( LPDPNAME lpDst, LPDPNAME lpSrc, BOOL bAnsi );
static void DP_SetPlayerData( lpPlayerData lpPData, DWORD dwFlags,
                              LPVOID lpData, DWORD dwDataSize );

static lpGroupData DP_CreateGroup( IDirectPlay2AImpl* iface, LPDPID lpid,
                                   LPDPNAME lpName, DWORD dwFlags,
                                   DPID idParent, BOOL bAnsi );
static void DP_SetGroupData( lpGroupData lpGData, DWORD dwFlags,
                             LPVOID lpData, DWORD dwDataSize );
static void DP_DeleteDPNameStruct( LPDPNAME lpDPName );
static void DP_DeletePlayer( IDirectPlay2Impl* This, DPID dpid );
static BOOL CALLBACK cbDeletePlayerFromAllGroups( DPID dpId,
                                                  DWORD dwPlayerType,
                                                  LPCDPNAME lpName,
                                                  DWORD dwFlags,
                                                  LPVOID lpContext );
static lpGroupData DP_FindAnyGroup( IDirectPlay2AImpl* This, DPID dpid );
static BOOL CALLBACK cbRemoveGroupOrPlayer( DPID dpId, DWORD dwPlayerType,
                                            LPCDPNAME lpName, DWORD dwFlags,
                                            LPVOID lpContext );
static void DP_DeleteGroup( IDirectPlay2Impl* This, DPID dpid );

/* Forward declarations of virtual tables */
static const IDirectPlay2Vtbl directPlay2AVT;
static const IDirectPlay3Vtbl directPlay3AVT;
static const IDirectPlay4Vtbl directPlay4AVT;

static const IDirectPlay2Vtbl directPlay2WVT;
static const IDirectPlay3Vtbl directPlay3WVT;
static const IDirectPlay4Vtbl directPlay4WVT;

/* Helper methods for player/group interfaces */
static HRESULT WINAPI DP_IF_DeletePlayerFromGroup
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idGroup,
            DPID idPlayer, BOOL bAnsi );
static HRESULT WINAPI DP_IF_CreatePlayer
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, LPDPID lpidPlayer,
            LPDPNAME lpPlayerName, HANDLE hEvent, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_DestroyGroup
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idGroup, BOOL bAnsi );
static HRESULT WINAPI DP_IF_DestroyPlayer
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idPlayer, BOOL bAnsi );
static HRESULT WINAPI DP_IF_EnumGroupPlayers
          ( IDirectPlay2Impl* This, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_EnumGroups
          ( IDirectPlay2Impl* This, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_EnumPlayers
          ( IDirectPlay2Impl* This, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetGroupData
          ( IDirectPlay2Impl* This, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetGroupName
          ( IDirectPlay2Impl* This, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetPlayerData
          ( IDirectPlay2Impl* This, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetPlayerName
          ( IDirectPlay2Impl* This, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize, BOOL bAnsi );
static HRESULT WINAPI DP_IF_SetGroupName
          ( IDirectPlay2Impl* This, DPID idGroup, LPDPNAME lpGroupName,
            DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_SetPlayerData
          ( IDirectPlay2Impl* This, DPID idPlayer, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_SetPlayerName
          ( IDirectPlay2Impl* This, DPID idPlayer, LPDPNAME lpPlayerName,
            DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_AddGroupToGroup
          ( IDirectPlay3Impl* This, DPID idParentGroup, DPID idGroup );
static HRESULT WINAPI DP_IF_CreateGroup
          ( IDirectPlay2AImpl* This, LPVOID lpMsgHdr, LPDPID lpidGroup,
            LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize,
            DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_CreateGroupInGroup
          ( IDirectPlay3Impl* This, LPVOID lpMsgHdr, DPID idParentGroup,
            LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_AddPlayerToGroup
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idGroup,
            DPID idPlayer, BOOL bAnsi );
static HRESULT WINAPI DP_IF_DeleteGroupFromGroup
          ( IDirectPlay3Impl* This, DPID idParentGroup, DPID idGroup );
static HRESULT WINAPI DP_SetSessionDesc
          ( IDirectPlay2Impl* This, LPCDPSESSIONDESC2 lpSessDesc,
            DWORD dwFlags, BOOL bInitial, BOOL bAnsi  );
static HRESULT WINAPI DP_SecureOpen
          ( IDirectPlay2Impl* This, LPCDPSESSIONDESC2 lpsd, DWORD dwFlags,
            LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials,
            BOOL bAnsi );
static HRESULT WINAPI DP_SendEx
          ( IDirectPlay2Impl* This, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout,
            LPVOID lpContext, LPDWORD lpdwMsgID, BOOL bAnsi );
static HRESULT WINAPI DP_IF_Receive
          ( IDirectPlay2Impl* This, LPDPID lpidFrom, LPDPID lpidTo,
            DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetMessageQueue
          ( IDirectPlay4Impl* This, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes, BOOL bAnsi );
static HRESULT WINAPI DP_SP_SendEx
          ( IDirectPlay2Impl* This, DWORD dwFlags,
            LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout,
            LPVOID lpContext, LPDWORD lpdwMsgID );
static HRESULT WINAPI DP_IF_SetGroupData
          ( IDirectPlay2Impl* This, DPID idGroup, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetPlayerCaps
          ( IDirectPlay2Impl* This, DPID idPlayer, LPDPCAPS lpDPCaps,
            DWORD dwFlags );
static HRESULT WINAPI DP_IF_Close( IDirectPlay2Impl* This, BOOL bAnsi );
static HRESULT WINAPI DP_IF_CancelMessage
          ( IDirectPlay4Impl* This, DWORD dwMsgID, DWORD dwFlags,
            DWORD dwMinPriority, DWORD dwMaxPriority, BOOL bAnsi );
static HRESULT WINAPI DP_IF_EnumGroupsInGroup
          ( IDirectPlay3AImpl* This, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetGroupParent
          ( IDirectPlay3AImpl* This, DPID idGroup, LPDPID lpidGroup,
            BOOL bAnsi );
static HRESULT WINAPI DP_IF_GetCaps
          ( IDirectPlay2Impl* This, LPDPCAPS lpDPCaps, DWORD dwFlags );
static HRESULT WINAPI DP_IF_EnumSessions
          ( IDirectPlay2Impl* This, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout,
            LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi );
static HRESULT WINAPI DP_IF_InitializeConnection
          ( IDirectPlay3Impl* This, LPVOID lpConnection, DWORD dwFlags, BOOL bAnsi );
static BOOL CALLBACK cbDPCreateEnumConnections( LPCGUID lpguidSP,
    LPVOID lpConnection, DWORD dwConnectionSize, LPCDPNAME lpName,
    DWORD dwFlags, LPVOID lpContext );
static BOOL WINAPI DP_BuildSPCompoundAddr( LPGUID lpcSpGuid, LPVOID* lplpAddrBuf,
                                           LPDWORD lpdwBufSize );



static inline DPID DP_NextObjectId(void);
static DPID DP_GetRemoteNextObjectId(void);


static void DP_CopySessionDesc( LPDPSESSIONDESC2 destSessionDesc,
                                LPCDPSESSIONDESC2 srcSessDesc, BOOL bAnsi );


static HMODULE DP_LoadSP( LPCGUID lpcGuid, LPSPINITDATA lpSpData, LPBOOL lpbIsDpSp );
static HRESULT DP_InitializeDPSP( IDirectPlay3Impl* This, HMODULE hServiceProvider );
static HRESULT DP_InitializeDPLSP( IDirectPlay3Impl* This, HMODULE hServiceProvider );






#define DPID_NOPARENT_GROUP 0 /* Magic number to indicate no parent of group */
#define DPID_SYSTEM_GROUP DPID_NOPARENT_GROUP /* If system group is supported
                                                 we don't have to change much */
#define DPID_NAME_SERVER 0x19a9d65b  /* Don't ask me why */

/* Strip out dwFlag values which cannot be sent in the CREATEGROUP msg */
#define DPMSG_CREATEGROUP_DWFLAGS(x) ( (x) & DPGROUP_HIDDEN )

/* Strip out all dwFlags values for CREATEPLAYER msg */
#define DPMSG_CREATEPLAYER_DWFLAGS(x) 0

static LONG kludgePlayerGroupId = 1000;

/* ------------------------------------------------------------------ */


static BOOL DP_CreateIUnknown( LPVOID lpDP )
{
  IDirectPlay2AImpl *This = (IDirectPlay2AImpl *)lpDP;

  This->unk = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->unk) ) );
  if ( This->unk == NULL )
  {
    return FALSE;
  }

  InitializeCriticalSection( &This->unk->DP_lock );

  return TRUE;
}

static BOOL DP_DestroyIUnknown( LPVOID lpDP )
{
  IDirectPlay2AImpl *This = (IDirectPlay2AImpl *)lpDP;

  DeleteCriticalSection( &This->unk->DP_lock );
  HeapFree( GetProcessHeap(), 0, This->unk );

  return TRUE;
}

static BOOL DP_CreateDirectPlay2( LPVOID lpDP )
{
  IDirectPlay2AImpl *This = (IDirectPlay2AImpl *)lpDP;

  This->dp2 = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dp2) ) );
  if ( This->dp2 == NULL )
  {
    return FALSE;
  }

  This->dp2->bConnectionOpen = FALSE;

  This->dp2->hEnumSessionThread = INVALID_HANDLE_VALUE;

  This->dp2->bHostInterface = FALSE;

  DPQ_INIT(This->dp2->receiveMsgs);
  DPQ_INIT(This->dp2->sendMsgs);
  DPQ_INIT(This->dp2->replysExpected);

  if( !NS_InitializeSessionCache( &This->dp2->lpNameServerData ) )
  {
    /* FIXME: Memory leak */
    return FALSE;
  }

  /* Provide an initial session desc with nothing in it */
  This->dp2->lpSessionDesc = HeapAlloc( GetProcessHeap(),
                                        HEAP_ZERO_MEMORY,
                                        sizeof( *This->dp2->lpSessionDesc ) );
  if( This->dp2->lpSessionDesc == NULL )
  {
    /* FIXME: Memory leak */
    return FALSE;
  }
  This->dp2->lpSessionDesc->dwSize = sizeof( *This->dp2->lpSessionDesc );

  /* We are emulating a dp 6 implementation */
  This->dp2->spData.dwSPVersion = DPSP_MAJORVERSION;

  This->dp2->spData.lpCB = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                      sizeof( *This->dp2->spData.lpCB ) );
  This->dp2->spData.lpCB->dwSize = sizeof( *This->dp2->spData.lpCB );
  This->dp2->spData.lpCB->dwVersion = DPSP_MAJORVERSION;

  /* This is the pointer to the service provider */
  if( FAILED( DPSP_CreateInterface( &IID_IDirectPlaySP,
                                    (LPVOID*)&This->dp2->spData.lpISP, This ) )
    )
  {
    /* FIXME: Memory leak */
    return FALSE;
  }

  /* Setup lobby provider information */
  This->dp2->dplspData.dwSPVersion = DPSP_MAJORVERSION;
  This->dp2->dplspData.lpCB = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                         sizeof( *This->dp2->dplspData.lpCB ) );
  This->dp2->dplspData.lpCB->dwSize = sizeof(  *This->dp2->dplspData.lpCB );

  if( FAILED( DPLSP_CreateInterface( &IID_IDPLobbySP,
                                     (LPVOID*)&This->dp2->dplspData.lpISP, This ) )
    )
  {
    /* FIXME: Memory leak */
    return FALSE;
  }

  return TRUE;
}

/* Definition of the global function in dplayx_queue.h. #
 * FIXME: Would it be better to have a dplayx_queue.c for this function? */
DPQ_DECL_DELETECB( cbDeleteElemFromHeap, LPVOID )
{
  HeapFree( GetProcessHeap(), 0, elem );
}

/* Function to delete the list of groups with this interface. Needs to
 * delete the group and player lists associated with this group as well
 * as the group data associated with this group. It should not delete
 * player data as that is shared with the top player list and will be
 * deleted with that.
 */
DPQ_DECL_DELETECB( cbDeleteGroupsElem, lpGroupList );
DPQ_DECL_DELETECB( cbDeleteGroupsElem, lpGroupList )
{
  DPQ_DELETEQ( elem->lpGData->groups, groups,
               lpGroupList, cbDeleteElemFromHeap );
  DPQ_DELETEQ( elem->lpGData->players, players,
               lpPlayerList, cbDeleteElemFromHeap );
  HeapFree( GetProcessHeap(), 0, elem->lpGData );
  HeapFree( GetProcessHeap(), 0, elem );
}

/* Function to delete the list of players with this interface. Needs to
 * delete the player data for all players as well.
 */
DPQ_DECL_DELETECB( cbDeletePlayerElem, lpPlayerList );
DPQ_DECL_DELETECB( cbDeletePlayerElem, lpPlayerList )
{
  HeapFree( GetProcessHeap(), 0, elem->lpPData );
  HeapFree( GetProcessHeap(), 0, elem );
}

static BOOL DP_DestroyDirectPlay2( LPVOID lpDP )
{
  IDirectPlay2AImpl *This = (IDirectPlay2AImpl *)lpDP;

  if( This->dp2->hEnumSessionThread != INVALID_HANDLE_VALUE )
  {
    TerminateThread( This->dp2->hEnumSessionThread, 0 );
    CloseHandle( This->dp2->hEnumSessionThread );
  }

  /* Finish with the SP - have it shutdown */
  if( This->dp2->spData.lpCB->ShutdownEx )
  {
    DPSP_SHUTDOWNDATA data;

    TRACE( "Calling SP ShutdownEx\n" );

    data.lpISP = This->dp2->spData.lpISP;

    (*This->dp2->spData.lpCB->ShutdownEx)( &data );
  }
  else if (This->dp2->spData.lpCB->Shutdown ) /* obsolete interface */
  {
    TRACE( "Calling obsolete SP Shutdown\n" );
    (*This->dp2->spData.lpCB->Shutdown)();
  }

  /* Unload the SP (if it exists) */
  if( This->dp2->hServiceProvider != 0 )
  {
    FreeLibrary( This->dp2->hServiceProvider );
  }

  /* Unload the Lobby Provider (if it exists) */
  if( This->dp2->hDPLobbyProvider != 0 )
  {
    FreeLibrary( This->dp2->hDPLobbyProvider );
  }

#if 0
  DPQ_DELETEQ( This->dp2->players, players, lpPlayerList, cbDeletePlayerElem );
  DPQ_DELETEQ( This->dp2->groups, groups, lpGroupList, cbDeleteGroupsElem );
#endif

  /* FIXME: Need to delete receive and send msgs queue contents */

  NS_DeleteSessionCache( This->dp2->lpNameServerData );

  HeapFree( GetProcessHeap(), 0, This->dp2->lpSessionDesc );

  IDirectPlaySP_Release( This->dp2->spData.lpISP );

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dp2 );

  return TRUE;
}

static BOOL DP_CreateDirectPlay3( LPVOID lpDP )
{
  IDirectPlay3AImpl *This = (IDirectPlay3AImpl *)lpDP;

  This->dp3 = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dp3) ) );
  if ( This->dp3 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

static BOOL DP_DestroyDirectPlay3( LPVOID lpDP )
{
  IDirectPlay3AImpl *This = (IDirectPlay3AImpl *)lpDP;

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dp3 );

  return TRUE;
}

static BOOL DP_CreateDirectPlay4( LPVOID lpDP )
{
  IDirectPlay4AImpl *This = (IDirectPlay4AImpl *)lpDP;

  This->dp4 = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(This->dp4) ) );
  if ( This->dp4 == NULL )
  {
    return FALSE;
  }

  return TRUE;
}

static BOOL DP_DestroyDirectPlay4( LPVOID lpDP )
{
  IDirectPlay3AImpl *This = (IDirectPlay3AImpl *)lpDP;

  /* Delete the contents */
  HeapFree( GetProcessHeap(), 0, This->dp4 );

  return TRUE;
}


/* Create a new interface */
extern
HRESULT DP_CreateInterface
         ( REFIID riid, LPVOID* ppvObj )
{
  TRACE( " for %s\n", debugstr_guid( riid ) );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( IDirectPlay2Impl ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  if( IsEqualGUID( &IID_IDirectPlay2, riid ) )
  {
    IDirectPlay2Impl *This = (IDirectPlay2Impl *)*ppvObj;
    This->lpVtbl = &directPlay2WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay2A, riid ) )
  {
    IDirectPlay2AImpl *This = (IDirectPlay2AImpl *)*ppvObj;
    This->lpVtbl = &directPlay2AVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3, riid ) )
  {
    IDirectPlay3Impl *This = (IDirectPlay3Impl *)*ppvObj;
    This->lpVtbl = &directPlay3WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3A, riid ) )
  {
    IDirectPlay3AImpl *This = (IDirectPlay3AImpl *)*ppvObj;
    This->lpVtbl = &directPlay3AVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay4, riid ) )
  {
    IDirectPlay4Impl *This = (IDirectPlay4Impl *)*ppvObj;
    This->lpVtbl = &directPlay4WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay4A, riid ) )
  {
    IDirectPlay4AImpl *This = (IDirectPlay4AImpl *)*ppvObj;
    This->lpVtbl = &directPlay4AVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  /* Initialize it */
  if ( DP_CreateIUnknown( *ppvObj ) &&
       DP_CreateDirectPlay2( *ppvObj ) &&
       DP_CreateDirectPlay3( *ppvObj ) &&
       DP_CreateDirectPlay4( *ppvObj )
     )
  {
    IDirectPlayX_AddRef( (LPDIRECTPLAY2A)*ppvObj );

    return S_OK;
  }

  /* Initialize failed, destroy it */
  DP_DestroyDirectPlay4( *ppvObj );
  DP_DestroyDirectPlay3( *ppvObj );
  DP_DestroyDirectPlay2( *ppvObj );
  DP_DestroyIUnknown( *ppvObj );

  HeapFree( GetProcessHeap(), 0, *ppvObj );

  *ppvObj = NULL;
  return DPERR_NOMEMORY;
}


/* Direct Play methods */

/* Shared between all dplay types */
static HRESULT WINAPI DP_QueryInterface
         ( LPDIRECTPLAY2 iface, REFIID riid, LPVOID* ppvObj )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  TRACE("(%p)->(%s,%p)\n", This, debugstr_guid( riid ), ppvObj );

  *ppvObj = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                       sizeof( *This ) );

  if( *ppvObj == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  CopyMemory( *ppvObj, This, sizeof( *This )  );
  (*(IDirectPlay2Impl**)ppvObj)->ulInterfaceRef = 0;

  if( IsEqualGUID( &IID_IDirectPlay2, riid ) )
  {
    IDirectPlay2Impl *This = (IDirectPlay2Impl *)*ppvObj;
    This->lpVtbl = &directPlay2WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay2A, riid ) )
  {
    IDirectPlay2AImpl *This = (IDirectPlay2AImpl *)*ppvObj;
    This->lpVtbl = &directPlay2AVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3, riid ) )
  {
    IDirectPlay3Impl *This = (IDirectPlay3Impl *)*ppvObj;
    This->lpVtbl = &directPlay3WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay3A, riid ) )
  {
    IDirectPlay3AImpl *This = (IDirectPlay3AImpl *)*ppvObj;
    This->lpVtbl = &directPlay3AVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay4, riid ) )
  {
    IDirectPlay4Impl *This = (IDirectPlay4Impl *)*ppvObj;
    This->lpVtbl = &directPlay4WVT;
  }
  else if( IsEqualGUID( &IID_IDirectPlay4A, riid ) )
  {
    IDirectPlay4AImpl *This = (IDirectPlay4AImpl *)*ppvObj;
    This->lpVtbl = &directPlay4AVT;
  }
  else
  {
    /* Unsupported interface */
    HeapFree( GetProcessHeap(), 0, *ppvObj );
    *ppvObj = NULL;

    return E_NOINTERFACE;
  }

  IDirectPlayX_AddRef( (LPDIRECTPLAY2)*ppvObj );

  return S_OK;
}

/* Shared between all dplay types */
static ULONG WINAPI DP_AddRef
         ( LPDIRECTPLAY3 iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;

  ulObjRefCount       = InterlockedIncrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedIncrement( &This->ulInterfaceRef );

  TRACE( "ref count incremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  return ulObjRefCount;
}

static ULONG WINAPI DP_Release
( LPDIRECTPLAY3 iface )
{
  ULONG ulInterfaceRefCount, ulObjRefCount;

  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;

  ulObjRefCount       = InterlockedDecrement( &This->unk->ulObjRef );
  ulInterfaceRefCount = InterlockedDecrement( &This->ulInterfaceRef );

  TRACE( "ref count decremented to %lu:%lu for %p\n",
         ulInterfaceRefCount, ulObjRefCount, This );

  /* Deallocate if this is the last reference to the object */
  if( ulObjRefCount == 0 )
  {
     /* If we're destroying the object, this must be the last ref
        of the last interface */
     DP_DestroyDirectPlay4( This );
     DP_DestroyDirectPlay3( This );
     DP_DestroyDirectPlay2( This );
     DP_DestroyIUnknown( This );
  }

  /* Deallocate the interface */
  if( ulInterfaceRefCount == 0 )
  {
    HeapFree( GetProcessHeap(), 0, This );
  }

  return ulObjRefCount;
}

static inline DPID DP_NextObjectId(void)
{
  return (DPID)InterlockedIncrement( &kludgePlayerGroupId );
}

/* *lplpReply will be non NULL iff there is something to reply */
HRESULT DP_HandleMessage( IDirectPlay2Impl* This, LPCVOID lpcMessageBody,
                          DWORD  dwMessageBodySize, LPCVOID lpcMessageHeader,
                          WORD wCommandId, WORD wVersion,
                          LPVOID* lplpReply, LPDWORD lpdwMsgSize )
{
  TRACE( "(%p)->(%p,0x%08lx,%p,%u,%u)\n",
         This, lpcMessageBody, dwMessageBodySize, lpcMessageHeader, wCommandId,
         wVersion );

  switch( wCommandId )
  {
    /* Name server needs to handle this request */
    case DPMSGCMD_ENUMSESSIONSREQUEST:
    {
      /* Reply expected */
      NS_ReplyToEnumSessionsRequest( lpcMessageBody, lplpReply, lpdwMsgSize, This );

      break;
    }

    /* Name server needs to handle this request */
    case DPMSGCMD_ENUMSESSIONSREPLY:
    {
      /* No reply expected */
      NS_AddRemoteComputerAsNameServer( lpcMessageHeader,
                                        This->dp2->spData.dwSPHeaderSize,
                                        (LPDPMSG_ENUMSESSIONSREPLY)lpcMessageBody,
                                        This->dp2->lpNameServerData );
      break;
    }

    case DPMSGCMD_REQUESTNEWPLAYERID:
    {
      LPCDPMSG_REQUESTNEWPLAYERID lpcMsg =
        (LPCDPMSG_REQUESTNEWPLAYERID)lpcMessageBody;

      LPDPMSG_NEWPLAYERIDREPLY lpReply;

      *lpdwMsgSize = This->dp2->spData.dwSPHeaderSize + sizeof( *lpReply );

      *lplpReply = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, *lpdwMsgSize );

      FIXME( "Ignoring dwFlags 0x%08lx in request msg\n",
             lpcMsg->dwFlags );

      /* Setup the reply */
      lpReply = (LPDPMSG_NEWPLAYERIDREPLY)( (BYTE*)(*lplpReply) +
                                            This->dp2->spData.dwSPHeaderSize );

      lpReply->envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
      lpReply->envelope.wCommandId = DPMSGCMD_NEWPLAYERIDREPLY;
      lpReply->envelope.wVersion   = DPMSGVER_DP6;

      lpReply->dpidNewPlayerId = DP_NextObjectId();

      TRACE( "Allocating new playerid 0x%08lx from remote request\n",
             lpReply->dpidNewPlayerId );

      break;
    }

    case DPMSGCMD_GETNAMETABLEREPLY:
    case DPMSGCMD_NEWPLAYERIDREPLY:
    {

#if 0
      if( wCommandId == DPMSGCMD_NEWPLAYERIDREPLY )
        DebugBreak();
#endif
      DP_MSG_ReplyReceived( This, wCommandId, lpcMessageBody, dwMessageBodySize );

      break;
    }

#if 1
    case DPMSGCMD_JUSTENVELOPE:
    {
      TRACE( "GOT THE SELF MESSAGE: %p -> 0x%08lx\n", lpcMessageHeader, ((LPDWORD)lpcMessageHeader)[1] );
      NS_SetLocalAddr( This->dp2->lpNameServerData, lpcMessageHeader, 20 );
      DP_MSG_ReplyReceived( This, wCommandId, lpcMessageBody, dwMessageBodySize );
    }
#endif

    case DPMSGCMD_FORWARDADDPLAYER:
    {
#if 0
      DebugBreak();
#endif
#if 1
    TRACE( "Sending message to self to get my addr\n" );
    DP_MSG_ToSelf( This, 1 ); /* This is a hack right now */
#endif
      break;
    }

    case DPMSGCMD_FORWARDADDPLAYERNACK:
    {
      DP_MSG_ErrorReceived( This, wCommandId, lpcMessageBody, dwMessageBodySize );
      break;
    }

    default:
    {
      FIXME( "Unknown wCommandId %u. Ignoring message\n", wCommandId );
      DebugBreak();
      break;
    }
  }

  /* FIXME: There is code in dplaysp.c to handle dplay commands. Move to here. */

  return DP_OK;
}


static HRESULT WINAPI DP_IF_AddPlayerToGroup
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idGroup,
            DPID idPlayer, BOOL bAnsi )
{
  lpGroupData  lpGData;
  lpPlayerList lpPList;
  lpPlayerList lpNewPList;

  TRACE( "(%p)->(%p,0x%08lx,0x%08lx,%u)\n",
         This, lpMsgHdr, idGroup, idPlayer, bAnsi );

  /* Find the group */
  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  /* Find the player */
  if( ( lpPList = DP_FindPlayer( This, idPlayer ) ) == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  /* Create a player list (ie "shortcut" ) */
  lpNewPList = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpNewPList ) );
  if( lpNewPList == NULL )
  {
    return DPERR_CANTADDPLAYER;
  }

  /* Add the shortcut */
  lpPList->lpPData->uRef++;
  lpNewPList->lpPData = lpPList->lpPData;

  /* Add the player to the list of players for this group */
  DPQ_INSERT(lpGData->players,lpNewPList,players);

  /* Let the SP know that we've added a player to the group */
  if( This->dp2->spData.lpCB->AddPlayerToGroup )
  {
    DPSP_ADDPLAYERTOGROUPDATA data;

    TRACE( "Calling SP AddPlayerToGroup\n" );

    data.idPlayer = idPlayer;
    data.idGroup  = idGroup;
    data.lpISP    = This->dp2->spData.lpISP;

    (*This->dp2->spData.lpCB->AddPlayerToGroup)( &data );
  }

  /* Inform all other peers of the addition of player to the group. If there are
   * no peers keep this event quiet.
   * Also, if this event was the result of another machine sending it to us,
   * don't bother rebroadcasting it.
   */
  if( ( lpMsgHdr == NULL ) &&
      This->dp2->lpSessionDesc &&
      ( This->dp2->lpSessionDesc->dwFlags & DPSESSION_MULTICASTSERVER ) )
  {
    DPMSG_ADDPLAYERTOGROUP msg;
    msg.dwType = DPSYS_ADDPLAYERTOGROUP;

    msg.dpIdGroup  = idGroup;
    msg.dpIdPlayer = idPlayer;

    /* FIXME: Correct to just use send effectively? */
    /* FIXME: Should size include data w/ message or just message "header" */
    /* FIXME: Check return code */
    DP_SendEx( This, DPID_SERVERPLAYER, DPID_ALLPLAYERS, 0, &msg, sizeof( msg ),               0, 0, NULL, NULL, bAnsi );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_AddPlayerToGroup
          ( LPDIRECTPLAY2A iface, DPID idGroup, DPID idPlayer )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_AddPlayerToGroup( This, NULL, idGroup, idPlayer, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_AddPlayerToGroup
          ( LPDIRECTPLAY2 iface, DPID idGroup, DPID idPlayer )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_AddPlayerToGroup( This, NULL, idGroup, idPlayer, FALSE );
}

static HRESULT WINAPI DP_IF_Close( IDirectPlay2Impl* This, BOOL bAnsi )
{
  HRESULT hr = DP_OK;

  TRACE("(%p)->(%u)\n", This, bAnsi );

  /* FIXME: Need to find a new host I assume (how?) */
  /* FIXME: Need to destroy all local groups */
  /* FIXME: Need to migrate all remotely visible players to the new host */

  /* Invoke the SP callback to inform of session close */
  if( This->dp2->spData.lpCB->CloseEx )
  {
    DPSP_CLOSEDATA data;

    TRACE( "Calling SP CloseEx\n" );

    data.lpISP = This->dp2->spData.lpISP;

    hr = (*This->dp2->spData.lpCB->CloseEx)( &data );

  }
  else if ( This->dp2->spData.lpCB->Close ) /* Try obsolete version */
  {
    TRACE( "Calling SP Close (obsolete interface)\n" );

    hr = (*This->dp2->spData.lpCB->Close)();
  }

  return hr;
}

static HRESULT WINAPI DirectPlay2AImpl_Close
          ( LPDIRECTPLAY2A iface )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_Close( This, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_Close
          ( LPDIRECTPLAY2 iface )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_Close( This, FALSE );
}

static
lpGroupData DP_CreateGroup( IDirectPlay2AImpl* This, LPDPID lpid,
                            LPDPNAME lpName, DWORD dwFlags,
                            DPID idParent, BOOL bAnsi )
{
  lpGroupData lpGData;

  /* Allocate the new space and add to end of high level group list */
  lpGData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpGData ) );

  if( lpGData == NULL )
  {
    return NULL;
  }

  DPQ_INIT(lpGData->groups);
  DPQ_INIT(lpGData->players);

  /* Set the desired player ID - no sanity checking to see if it exists */
  lpGData->dpid = *lpid;

  DP_CopyDPNAMEStruct( &lpGData->name, lpName, bAnsi );

  /* FIXME: Should we check that the parent exists? */
  lpGData->parent  = idParent;

  /* FIXME: Should we validate the dwFlags? */
  lpGData->dwFlags = dwFlags;

  TRACE( "Created group id 0x%08lx\n", *lpid );

  return lpGData;
}

/* This method assumes that all links to it are already deleted */
static void
DP_DeleteGroup( IDirectPlay2Impl* This, DPID dpid )
{
  lpGroupList lpGList;

  TRACE( "(%p)->(0x%08lx)\n", This, dpid );

  DPQ_REMOVE_ENTRY( This->dp2->lpSysGroup->groups, groups, lpGData->dpid, ==, dpid, lpGList );

  if( lpGList == NULL )
  {
    ERR( "DPID 0x%08lx not found\n", dpid );
    return;
  }

  if( --(lpGList->lpGData->uRef) )
  {
    FIXME( "Why is this not the last reference to group?\n" );
    DebugBreak();
  }

  /* Delete player */
  DP_DeleteDPNameStruct( &lpGList->lpGData->name );
  HeapFree( GetProcessHeap(), 0, lpGList->lpGData );

  /* Remove and Delete Player List object */
  HeapFree( GetProcessHeap(), 0, lpGList );

}

static lpGroupData DP_FindAnyGroup( IDirectPlay2AImpl* This, DPID dpid )
{
  lpGroupList lpGroups;

  TRACE( "(%p)->(0x%08lx)\n", This, dpid );

  if( dpid == DPID_SYSTEM_GROUP )
  {
    return This->dp2->lpSysGroup;
  }
  else
  {
    DPQ_FIND_ENTRY( This->dp2->lpSysGroup->groups, groups, lpGData->dpid, ==, dpid, lpGroups );
  }

  if( lpGroups == NULL )
  {
    return NULL;
  }

  return lpGroups->lpGData;
}

static HRESULT WINAPI DP_IF_CreateGroup
          ( IDirectPlay2AImpl* This, LPVOID lpMsgHdr, LPDPID lpidGroup,
            LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize,
            DWORD dwFlags, BOOL bAnsi )
{
  lpGroupData lpGData;

  TRACE( "(%p)->(%p,%p,%p,%p,0x%08lx,0x%08lx,%u)\n",
         This, lpMsgHdr, lpidGroup, lpGroupName, lpData, dwDataSize,
         dwFlags, bAnsi );

  /* If the name is not specified, we must provide one */
  if( DPID_UNKNOWN == *lpidGroup )
  {
    /* If we are the name server, we decide on the group ids. If not, we
     * must ask for one before attempting a creation.
     */
    if( This->dp2->bHostInterface )
    {
      *lpidGroup = DP_NextObjectId();
    }
    else
    {
      *lpidGroup = DP_GetRemoteNextObjectId();
    }
  }

  lpGData = DP_CreateGroup( This, lpidGroup, lpGroupName, dwFlags,
                            DPID_NOPARENT_GROUP, bAnsi );

  if( lpGData == NULL )
  {
    return DPERR_CANTADDPLAYER; /* yes player not group */
  }

  if( DPID_SYSTEM_GROUP == *lpidGroup )
  {
    This->dp2->lpSysGroup = lpGData;
    TRACE( "Inserting system group\n" );
  }
  else
  {
    /* Insert into the system group */
    lpGroupList lpGroup = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpGroup ) );
    lpGroup->lpGData = lpGData;

    DPQ_INSERT( This->dp2->lpSysGroup->groups, lpGroup, groups );
  }

  /* Something is now referencing this data */
  lpGData->uRef++;

  /* Set all the important stuff for the group */
  DP_SetGroupData( lpGData, DPSET_REMOTE, lpData, dwDataSize );

  /* FIXME: We should only create the system group if GetCaps returns
   *        DPCAPS_GROUPOPTIMIZED.
   */

  /* Let the SP know that we've created this group */
  if( This->dp2->spData.lpCB->CreateGroup )
  {
    DPSP_CREATEGROUPDATA data;
    DWORD dwCreateFlags = 0;

    TRACE( "Calling SP CreateGroup\n" );

    if( *lpidGroup == DPID_NOPARENT_GROUP )
      dwCreateFlags |= DPLAYI_GROUP_SYSGROUP;

    if( lpMsgHdr == NULL )
      dwCreateFlags |= DPLAYI_PLAYER_PLAYERLOCAL;

    if( dwFlags & DPGROUP_HIDDEN )
      dwCreateFlags |= DPLAYI_GROUP_HIDDEN;

    data.idGroup           = *lpidGroup;
    data.dwFlags           = dwCreateFlags;
    data.lpSPMessageHeader = lpMsgHdr;
    data.lpISP             = This->dp2->spData.lpISP;

    (*This->dp2->spData.lpCB->CreateGroup)( &data );
  }

  /* Inform all other peers of the creation of a new group. If there are
   * no peers keep this event quiet.
   * Also if this message was sent to us, don't rebroadcast.
   */
  if( ( lpMsgHdr == NULL ) &&
      This->dp2->lpSessionDesc &&
      ( This->dp2->lpSessionDesc->dwFlags & DPSESSION_MULTICASTSERVER ) )
  {
    DPMSG_CREATEPLAYERORGROUP msg;
    msg.dwType = DPSYS_CREATEPLAYERORGROUP;

    msg.dwPlayerType     = DPPLAYERTYPE_GROUP;
    msg.dpId             = *lpidGroup;
    msg.dwCurrentPlayers = 0; /* FIXME: Incorrect? */
    msg.lpData           = lpData;
    msg.dwDataSize       = dwDataSize;
    msg.dpnName          = *lpGroupName;
    msg.dpIdParent       = DPID_NOPARENT_GROUP;
    msg.dwFlags          = DPMSG_CREATEGROUP_DWFLAGS( dwFlags );

    /* FIXME: Correct to just use send effectively? */
    /* FIXME: Should size include data w/ message or just message "header" */
    /* FIXME: Check return code */
    DP_SendEx( This, DPID_SERVERPLAYER, DPID_ALLPLAYERS, 0, &msg, sizeof( msg ),
               0, 0, NULL, NULL, bAnsi );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_CreateGroup
          ( LPDIRECTPLAY2A iface, LPDPID lpidGroup, LPDPNAME lpGroupName,
            LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  *lpidGroup = DPID_UNKNOWN;

  return DP_IF_CreateGroup( (IDirectPlay2AImpl*)iface, NULL, lpidGroup,
                            lpGroupName, lpData, dwDataSize, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_CreateGroup
          ( LPDIRECTPLAY2 iface, LPDPID lpidGroup, LPDPNAME lpGroupName,
            LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  *lpidGroup = DPID_UNKNOWN;

  return DP_IF_CreateGroup( (IDirectPlay2AImpl*)iface, NULL, lpidGroup,
                            lpGroupName, lpData, dwDataSize, dwFlags, FALSE );
}


static void
DP_SetGroupData( lpGroupData lpGData, DWORD dwFlags,
                 LPVOID lpData, DWORD dwDataSize )
{
  /* Clear out the data with this player */
  if( dwFlags & DPSET_LOCAL )
  {
    if ( lpGData->dwLocalDataSize != 0 )
    {
      HeapFree( GetProcessHeap(), 0, lpGData->lpLocalData );
      lpGData->lpLocalData = NULL;
      lpGData->dwLocalDataSize = 0;
    }
  }
  else
  {
    if( lpGData->dwRemoteDataSize != 0 )
    {
      HeapFree( GetProcessHeap(), 0, lpGData->lpRemoteData );
      lpGData->lpRemoteData = NULL;
      lpGData->dwRemoteDataSize = 0;
    }
  }

  /* Reallocate for new data */
  if( lpData != NULL )
  {
    LPVOID lpNewData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                  sizeof( dwDataSize ) );
    CopyMemory( lpNewData, lpData, dwDataSize );

    if( dwFlags & DPSET_LOCAL )
    {
      lpGData->lpLocalData     = lpData;
      lpGData->dwLocalDataSize = dwDataSize;
    }
    else
    {
      lpGData->lpRemoteData     = lpNewData;
      lpGData->dwRemoteDataSize = dwDataSize;
    }
  }

}

/* This function will just create the storage for the new player.  */
static
lpPlayerData DP_CreatePlayer( IDirectPlay2Impl* This, LPDPID lpid,
                              LPDPNAME lpName, DWORD dwFlags,
                              HANDLE hEvent, BOOL bAnsi )
{
  lpPlayerData lpPData;

  TRACE( "(%p)->(%p,%p,%u)\n", This, lpid, lpName, bAnsi );

  /* Allocate the storage for the player and associate it with list element */
  lpPData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpPData ) );
  if( lpPData == NULL )
  {
    return NULL;
  }

  /* Set the desired player ID */
  lpPData->dpid = *lpid;

  DP_CopyDPNAMEStruct( &lpPData->name, lpName, bAnsi );

  lpPData->dwFlags = dwFlags;

  /* If we were given an event handle, duplicate it */
  if( hEvent != 0 )
  {
    if( !DuplicateHandle( GetCurrentProcess(), hEvent,
                          GetCurrentProcess(), &lpPData->hEvent,
                          0, FALSE, DUPLICATE_SAME_ACCESS )
      )
    {
      /* FIXME: Memory leak */
      ERR( "Can't duplicate player msg handle %p\n", hEvent );
    }
  }

  /* Initialize the SP data section */
  lpPData->lpSPPlayerData = DPSP_CreateSPPlayerData();

  TRACE( "Created player id 0x%08lx\n", *lpid );

  return lpPData;
}

/* Delete the contents of the DPNAME struct */
static void
DP_DeleteDPNameStruct( LPDPNAME lpDPName )
{
  HeapFree( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDPName->lpszShortNameA );
  HeapFree( GetProcessHeap(), HEAP_ZERO_MEMORY, lpDPName->lpszLongNameA );
}

/* This method assumes that all links to it are already deleted */
static void
DP_DeletePlayer( IDirectPlay2Impl* This, DPID dpid )
{
  lpPlayerList lpPList;

  TRACE( "(%p)->(0x%08lx)\n", This, dpid );

  DPQ_REMOVE_ENTRY( This->dp2->lpSysGroup->players, players, lpPData->dpid, ==, dpid, lpPList );

  if( lpPList == NULL )
  {
    ERR( "DPID 0x%08lx not found\n", dpid );
    return;
  }

  /* Verify that this is the last reference to the data */
  if( --(lpPList->lpPData->uRef) )
  {
    FIXME( "Why is this not the last reference to player?\n" );
    DebugBreak();
  }

  /* Delete player */
  DP_DeleteDPNameStruct( &lpPList->lpPData->name );

  CloseHandle( lpPList->lpPData->hEvent );
  HeapFree( GetProcessHeap(), 0, lpPList->lpPData );

  /* Delete Player List object */
  HeapFree( GetProcessHeap(), 0, lpPList );
}

static lpPlayerList DP_FindPlayer( IDirectPlay2AImpl* This, DPID dpid )
{
  lpPlayerList lpPlayers;

  TRACE( "(%p)->(0x%08lx)\n", This, dpid );

  DPQ_FIND_ENTRY( This->dp2->lpSysGroup->players, players, lpPData->dpid, ==, dpid, lpPlayers );

  return lpPlayers;
}

/* Basic area for Dst must already be allocated */
static BOOL DP_CopyDPNAMEStruct( LPDPNAME lpDst, LPDPNAME lpSrc, BOOL bAnsi )
{
  if( lpSrc == NULL )
  {
    ZeroMemory( lpDst, sizeof( *lpDst ) );
    lpDst->dwSize = sizeof( *lpDst );
    return TRUE;
  }

  if( lpSrc->dwSize != sizeof( *lpSrc) )
  {
    return FALSE;
  }

  /* Delete any existing pointers */
  HeapFree( GetProcessHeap(), 0, lpDst->lpszShortNameA );
  HeapFree( GetProcessHeap(), 0, lpDst->lpszLongNameA );

  /* Copy as required */
  CopyMemory( lpDst, lpSrc, lpSrc->dwSize );

  if( bAnsi )
  {
    if( lpSrc->lpszShortNameA )
    {
        lpDst->lpszShortNameA = HeapAlloc( GetProcessHeap(), 0,
                                             strlen(lpSrc->lpszShortNameA)+1 );
        strcpy( lpDst->lpszShortNameA, lpSrc->lpszShortNameA );
    }
    if( lpSrc->lpszLongNameA )
    {
        lpDst->lpszLongNameA = HeapAlloc( GetProcessHeap(), 0,
                                              strlen(lpSrc->lpszLongNameA)+1 );
        strcpy( lpDst->lpszLongNameA, lpSrc->lpszLongNameA );
    }
  }
  else
  {
    if( lpSrc->lpszShortNameA )
    {
        lpDst->lpszShortName = HeapAlloc( GetProcessHeap(), 0,
                                              (strlenW(lpSrc->lpszShortName)+1)*sizeof(WCHAR) );
        strcpyW( lpDst->lpszShortName, lpSrc->lpszShortName );
    }
    if( lpSrc->lpszLongNameA )
    {
        lpDst->lpszLongName = HeapAlloc( GetProcessHeap(), 0,
                                             (strlenW(lpSrc->lpszLongName)+1)*sizeof(WCHAR) );
        strcpyW( lpDst->lpszLongName, lpSrc->lpszLongName );
    }
  }

  return TRUE;
}

static void
DP_SetPlayerData( lpPlayerData lpPData, DWORD dwFlags,
                  LPVOID lpData, DWORD dwDataSize )
{
  /* Clear out the data with this player */
  if( dwFlags & DPSET_LOCAL )
  {
    if ( lpPData->dwLocalDataSize != 0 )
    {
      HeapFree( GetProcessHeap(), 0, lpPData->lpLocalData );
      lpPData->lpLocalData = NULL;
      lpPData->dwLocalDataSize = 0;
    }
  }
  else
  {
    if( lpPData->dwRemoteDataSize != 0 )
    {
      HeapFree( GetProcessHeap(), 0, lpPData->lpRemoteData );
      lpPData->lpRemoteData = NULL;
      lpPData->dwRemoteDataSize = 0;
    }
  }

  /* Reallocate for new data */
  if( lpData != NULL )
  {
    LPVOID lpNewData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                  sizeof( dwDataSize ) );
    CopyMemory( lpNewData, lpData, dwDataSize );

    if( dwFlags & DPSET_LOCAL )
    {
      lpPData->lpLocalData     = lpData;
      lpPData->dwLocalDataSize = dwDataSize;
    }
    else
    {
      lpPData->lpRemoteData     = lpNewData;
      lpPData->dwRemoteDataSize = dwDataSize;
    }
  }

}

static HRESULT WINAPI DP_IF_CreatePlayer
( IDirectPlay2Impl* This,
  LPVOID lpMsgHdr, /* NULL for local creation, non NULL for remote creation */
  LPDPID lpidPlayer,
  LPDPNAME lpPlayerName,
  HANDLE hEvent,
  LPVOID lpData,
  DWORD dwDataSize,
  DWORD dwFlags,
  BOOL bAnsi )
{
  HRESULT hr = DP_OK;
  lpPlayerData lpPData;
  lpPlayerList lpPList;
  DWORD dwCreateFlags = 0;

  TRACE( "(%p)->(%p,%p,%p,%p,0x%08lx,0x%08lx,%u)\n",
         This, lpidPlayer, lpPlayerName, hEvent, lpData,
         dwDataSize, dwFlags, bAnsi );

  if( dwFlags == 0 )
  {
    dwFlags = DPPLAYER_SPECTATOR;
  }

  if( lpidPlayer == NULL )
  {
    return DPERR_INVALIDPARAMS;
  }


  /* Determine the creation flags for the player. These will be passed
   * to the name server if requesting a player id and to the SP when
   * informing it of the player creation
   */
  {
    if( dwFlags & DPPLAYER_SERVERPLAYER )
    {
      if( *lpidPlayer == DPID_SERVERPLAYER )
      {
        /* Server player for the host interface */
        dwCreateFlags |= DPLAYI_PLAYER_APPSERVER;
      }
      else if( *lpidPlayer == DPID_NAME_SERVER )
      {
        /* Name server - master of everything */
        dwCreateFlags |= (DPLAYI_PLAYER_NAMESRVR|DPLAYI_PLAYER_SYSPLAYER);
      }
      else
      {
        /* Server player for a non host interface */
        dwCreateFlags |= DPLAYI_PLAYER_SYSPLAYER;
      }
    }

    if( lpMsgHdr == NULL )
      dwCreateFlags |= DPLAYI_PLAYER_PLAYERLOCAL;
  }

  /* Verify we know how to handle all the flags */
  if( !( ( dwFlags & DPPLAYER_SERVERPLAYER ) ||
         ( dwFlags & DPPLAYER_SPECTATOR )
       )
    )
  {
    /* Assume non fatal failure */
    ERR( "unknown dwFlags = 0x%08lx\n", dwFlags );
  }

  /* If the name is not specified, we must provide one */
  if( *lpidPlayer == DPID_UNKNOWN )
  {
    /* If we are the session master, we dish out the group/player ids */
    if( This->dp2->bHostInterface )
    {
      *lpidPlayer = DP_NextObjectId();
    }
    else
    {
      hr = DP_MSG_SendRequestPlayerId( This, dwCreateFlags, lpidPlayer );

      if( FAILED(hr) )
      {
        ERR( "Request for ID failed: %s\n", DPLAYX_HresultToString( hr ) );
        return hr;
      }
    }
  }
  else
  {
    /* FIXME: Would be nice to perhaps verify that we don't already have
     *        this player.
     */
  }

  /* FIXME: Should we be storing these dwFlags or the creation ones? */
  lpPData = DP_CreatePlayer( This, lpidPlayer, lpPlayerName, dwFlags,
                             hEvent, bAnsi );

  if( lpPData == NULL )
  {
    return DPERR_CANTADDPLAYER;
  }

  /* Create the list object and link it in */
  lpPList = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpPList ) );
  if( lpPList == NULL )
  {
    FIXME( "Memory leak\n" );
    return DPERR_CANTADDPLAYER;
  }

  lpPData->uRef = 1;
  lpPList->lpPData = lpPData;

  /* Add the player to the system group */
  DPQ_INSERT( This->dp2->lpSysGroup->players, lpPList, players );

  /* Update the information and send it to all players in the session */
  DP_SetPlayerData( lpPData, DPSET_REMOTE, lpData, dwDataSize );

  /* Let the SP know that we've created this player */
  if( This->dp2->spData.lpCB->CreatePlayer )
  {
    DPSP_CREATEPLAYERDATA data;

    data.idPlayer          = *lpidPlayer;
    data.dwFlags           = dwCreateFlags;
    data.lpSPMessageHeader = lpMsgHdr;
    data.lpISP             = This->dp2->spData.lpISP;

    TRACE( "Calling SP CreatePlayer 0x%08lx: dwFlags: 0x%08lx lpMsgHdr: %p\n",
           *lpidPlayer, data.dwFlags, data.lpSPMessageHeader );

    hr = (*This->dp2->spData.lpCB->CreatePlayer)( &data );
  }

  if( FAILED(hr) )
  {
    ERR( "Failed to create player with sp: %s\n", DPLAYX_HresultToString(hr) );
    return hr;
  }

  /* Now let the SP know that this player is a member of the system group */
  if( This->dp2->spData.lpCB->AddPlayerToGroup )
  {
    DPSP_ADDPLAYERTOGROUPDATA data;

    data.idPlayer = *lpidPlayer;
    data.idGroup  = DPID_SYSTEM_GROUP;
    data.lpISP    = This->dp2->spData.lpISP;

    TRACE( "Calling SP AddPlayerToGroup (sys group)\n" );

    hr = (*This->dp2->spData.lpCB->AddPlayerToGroup)( &data );
  }

  if( FAILED(hr) )
  {
    ERR( "Failed to add player to sys group with sp: %s\n",
         DPLAYX_HresultToString(hr) );
    return hr;
  }

#if 1
  if( This->dp2->bHostInterface == FALSE )
  {
    /* Let the name server know about the creation of this player */
    /* FIXME: Is this only to be done for the creation of a server player or
     *        is this used for regular players? If only for server players, move
     *        this call to DP_SecureOpen(...);
     */
#if 0
    TRACE( "Sending message to self to get my addr\n" );
    DP_MSG_ToSelf( This, *lpidPlayer ); /* This is a hack right now */
#endif

    hr = DP_MSG_ForwardPlayerCreation( This, *lpidPlayer);
  }
#else
  /* Inform all other peers of the creation of a new player. If there are
   * no peers keep this quiet.
   * Also, if this was a remote event, no need to rebroadcast it.
   */
  if( ( lpMsgHdr == NULL ) &&
      This->dp2->lpSessionDesc &&
      ( This->dp2->lpSessionDesc->dwFlags & DPSESSION_MULTICASTSERVER ) )
  {
    DPMSG_CREATEPLAYERORGROUP msg;
    msg.dwType = DPSYS_CREATEPLAYERORGROUP;

    msg.dwPlayerType     = DPPLAYERTYPE_PLAYER;
    msg.dpId             = *lpidPlayer;
    msg.dwCurrentPlayers = 0; /* FIXME: Incorrect */
    msg.lpData           = lpData;
    msg.dwDataSize       = dwDataSize;
    msg.dpnName          = *lpPlayerName;
    msg.dpIdParent       = DPID_NOPARENT_GROUP;
    msg.dwFlags          = DPMSG_CREATEPLAYER_DWFLAGS( dwFlags );

    /* FIXME: Correct to just use send effectively? */
    /* FIXME: Should size include data w/ message or just message "header" */
    /* FIXME: Check return code */
    hr = DP_SendEx( This, DPID_SERVERPLAYER, DPID_ALLPLAYERS, 0, &msg,
                    sizeof( msg ), 0, 0, NULL, NULL, bAnsi );
  }
#endif

  return hr;
}

static HRESULT WINAPI DirectPlay2AImpl_CreatePlayer
          ( LPDIRECTPLAY2A iface, LPDPID lpidPlayer, LPDPNAME lpPlayerName,
            HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;

  if( dwFlags & DPPLAYER_SERVERPLAYER )
  {
    *lpidPlayer = DPID_SERVERPLAYER;
  }
  else
  {
    *lpidPlayer = DPID_UNKNOWN;
  }

  return DP_IF_CreatePlayer( This, NULL, lpidPlayer, lpPlayerName, hEvent,
                           lpData, dwDataSize, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_CreatePlayer
          ( LPDIRECTPLAY2 iface, LPDPID lpidPlayer, LPDPNAME lpPlayerName,
            HANDLE hEvent, LPVOID lpData, DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;

  if( dwFlags & DPPLAYER_SERVERPLAYER )
  {
    *lpidPlayer = DPID_SERVERPLAYER;
  }
  else
  {
    *lpidPlayer = DPID_UNKNOWN;
  }

  return DP_IF_CreatePlayer( This, NULL, lpidPlayer, lpPlayerName, hEvent,
                           lpData, dwDataSize, dwFlags, FALSE );
}

static DPID DP_GetRemoteNextObjectId(void)
{
  FIXME( ":stub\n" );

  /* Hack solution */
  return DP_NextObjectId();
}

static HRESULT WINAPI DP_IF_DeletePlayerFromGroup
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idGroup,
            DPID idPlayer, BOOL bAnsi )
{
  HRESULT hr = DP_OK;

  lpGroupData  lpGData;
  lpPlayerList lpPList;

  TRACE( "(%p)->(%p,0x%08lx,0x%08lx,%u)\n",
         This, lpMsgHdr, idGroup, idPlayer, bAnsi );

  /* Find the group */
  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  /* Find the player */
  if( ( lpPList = DP_FindPlayer( This, idPlayer ) ) == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  /* Remove the player shortcut from the group */
  DPQ_REMOVE_ENTRY( lpGData->players, players, lpPData->dpid, ==, idPlayer, lpPList );

  if( lpPList == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  /* One less reference */
  lpPList->lpPData->uRef--;

  /* Delete the Player List element */
  HeapFree( GetProcessHeap(), 0, lpPList );

  /* Inform the SP if they care */
  if( This->dp2->spData.lpCB->RemovePlayerFromGroup )
  {
    DPSP_REMOVEPLAYERFROMGROUPDATA data;

    TRACE( "Calling SP RemovePlayerFromGroup\n" );

    data.idPlayer = idPlayer;
    data.idGroup  = idGroup;
    data.lpISP    = This->dp2->spData.lpISP;

    hr = (*This->dp2->spData.lpCB->RemovePlayerFromGroup)( &data );
  }

  /* Need to send a DELETEPLAYERFROMGROUP message */
  FIXME( "Need to send a message\n" );

  return hr;
}

static HRESULT WINAPI DirectPlay2AImpl_DeletePlayerFromGroup
          ( LPDIRECTPLAY2A iface, DPID idGroup, DPID idPlayer )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_DeletePlayerFromGroup( This, NULL, idGroup, idPlayer, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_DeletePlayerFromGroup
          ( LPDIRECTPLAY2 iface, DPID idGroup, DPID idPlayer )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_DeletePlayerFromGroup( This, NULL, idGroup, idPlayer, FALSE );
}

typedef struct _DPRGOPContext
{
  IDirectPlay3Impl* This;
  BOOL              bAnsi;
  DPID              idGroup;
} DPRGOPContext, *lpDPRGOPContext;

static BOOL CALLBACK
cbRemoveGroupOrPlayer(
    DPID            dpId,
    DWORD           dwPlayerType,
    LPCDPNAME       lpName,
    DWORD           dwFlags,
    LPVOID          lpContext )
{
  lpDPRGOPContext lpCtxt = (lpDPRGOPContext)lpContext;

  TRACE( "Removing element:0x%08lx (type:0x%08lx) from element:0x%08lx\n",
           dpId, dwPlayerType, lpCtxt->idGroup );

  if( dwPlayerType == DPPLAYERTYPE_GROUP )
  {
    if( FAILED( DP_IF_DeleteGroupFromGroup( lpCtxt->This, lpCtxt->idGroup,
                                            dpId )
              )
      )
    {
      ERR( "Unable to delete group 0x%08lx from group 0x%08lx\n",
             dpId, lpCtxt->idGroup );
    }
  }
  else
  {
    if( FAILED( DP_IF_DeletePlayerFromGroup( (IDirectPlay2Impl*)lpCtxt->This,
                                             NULL, lpCtxt->idGroup,
                                             dpId, lpCtxt->bAnsi )
              )
      )
    {
      ERR( "Unable to delete player 0x%08lx from grp 0x%08lx\n",
             dpId, lpCtxt->idGroup );
    }
  }

  return TRUE; /* Continue enumeration */
}

static HRESULT WINAPI DP_IF_DestroyGroup
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idGroup, BOOL bAnsi )
{
  lpGroupData lpGData;
  DPRGOPContext context;

  FIXME( "(%p)->(%p,0x%08lx,%u): semi stub\n",
         This, lpMsgHdr, idGroup, bAnsi );

  /* Find the group */
  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDPLAYER; /* yes player */
  }

  context.This    = (IDirectPlay3Impl*)This;
  context.bAnsi   = bAnsi;
  context.idGroup = idGroup;

  /* Remove all players that this group has */
  DP_IF_EnumGroupPlayers( This, idGroup, NULL,
                          cbRemoveGroupOrPlayer, (LPVOID)&context, 0, bAnsi );

  /* Remove all links to groups that this group has since this is dp3 */
  DP_IF_EnumGroupsInGroup( (IDirectPlay3Impl*)This, idGroup, NULL,
                           cbRemoveGroupOrPlayer, (LPVOID)&context, 0, bAnsi );

  /* Remove this group from the parent group - if it has one */
  if( ( idGroup != DPID_SYSTEM_GROUP ) &&
      ( lpGData->parent != DPID_SYSTEM_GROUP )
    )
  {
    DP_IF_DeleteGroupFromGroup( (IDirectPlay3Impl*)This, lpGData->parent,
                                idGroup );
  }

  /* Now delete this group data and list from the system group */
  DP_DeleteGroup( This, idGroup );

  /* Let the SP know that we've destroyed this group */
  if( This->dp2->spData.lpCB->DeleteGroup )
  {
    DPSP_DELETEGROUPDATA data;

    FIXME( "data.dwFlags is incorrect\n" );

    data.idGroup = idGroup;
    data.dwFlags = 0;
    data.lpISP   = This->dp2->spData.lpISP;

    (*This->dp2->spData.lpCB->DeleteGroup)( &data );
  }

  FIXME( "Send out a DESTORYPLAYERORGROUP message\n" );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_DestroyGroup
          ( LPDIRECTPLAY2A iface, DPID idGroup )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_DestroyGroup( This, NULL, idGroup, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_DestroyGroup
          ( LPDIRECTPLAY2 iface, DPID idGroup )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_DestroyGroup( This, NULL, idGroup, FALSE );
}

typedef struct _DPFAGContext
{
  IDirectPlay2Impl* This;
  DPID              idPlayer;
  BOOL              bAnsi;
} DPFAGContext, *lpDPFAGContext;

static HRESULT WINAPI DP_IF_DestroyPlayer
          ( IDirectPlay2Impl* This, LPVOID lpMsgHdr, DPID idPlayer, BOOL bAnsi )
{
  DPFAGContext cbContext;

  FIXME( "(%p)->(%p,0x%08lx,%u): semi stub\n",
         This, lpMsgHdr, idPlayer, bAnsi );

  if( This->dp2->connectionInitialized == NO_PROVIDER )
  {
    return DPERR_UNINITIALIZED;
  }

  if( DP_FindPlayer( This, idPlayer ) == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  /* FIXME: If the player is remote, we must be the host to delete this */

  cbContext.This     = This;
  cbContext.idPlayer = idPlayer;
  cbContext.bAnsi    = bAnsi;

  /* Find each group and call DeletePlayerFromGroup if the player is a
     member of the group */
  DP_IF_EnumGroups( This, NULL, cbDeletePlayerFromAllGroups,
                    (LPVOID)&cbContext, DPENUMGROUPS_ALL, bAnsi );

  /* Now delete player and player list from the sys group */
  DP_DeletePlayer( This, idPlayer );

  /* Let the SP know that we've destroyed this group */
  if( This->dp2->spData.lpCB->DeletePlayer )
  {
    DPSP_DELETEPLAYERDATA data;

    FIXME( "data.dwFlags is incorrect\n" );

    data.idPlayer = idPlayer;
    data.dwFlags = 0;
    data.lpISP   = This->dp2->spData.lpISP;

    (*This->dp2->spData.lpCB->DeletePlayer)( &data );
  }

  FIXME( "Send a DELETEPLAYERORGROUP msg\n" );

  return DP_OK;
}

static BOOL CALLBACK
cbDeletePlayerFromAllGroups(
    DPID            dpId,
    DWORD           dwPlayerType,
    LPCDPNAME       lpName,
    DWORD           dwFlags,
    LPVOID          lpContext )
{
  lpDPFAGContext lpCtxt = (lpDPFAGContext)lpContext;

  if( dwPlayerType == DPPLAYERTYPE_GROUP )
  {
    DP_IF_DeletePlayerFromGroup( lpCtxt->This, NULL, dpId, lpCtxt->idPlayer,
                                 lpCtxt->bAnsi );

    /* Enumerate all groups in this group since this will normally only
     * be called for top level groups
     */
    DP_IF_EnumGroupsInGroup( (IDirectPlay3Impl*)lpCtxt->This,
                             dpId, NULL,
                             cbDeletePlayerFromAllGroups,
                             (LPVOID)lpContext, DPENUMGROUPS_ALL,
                             lpCtxt->bAnsi );

  }
  else
  {
    ERR( "Group callback has dwPlayerType = 0x%08lx\n", dwPlayerType );
  }

  return TRUE;
}

static HRESULT WINAPI DirectPlay2AImpl_DestroyPlayer
          ( LPDIRECTPLAY2A iface, DPID idPlayer )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_DestroyPlayer( This, NULL, idPlayer, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_DestroyPlayer
          ( LPDIRECTPLAY2 iface, DPID idPlayer )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_DestroyPlayer( This, NULL, idPlayer, FALSE );
}

static HRESULT WINAPI DP_IF_EnumGroupPlayers
          ( IDirectPlay2Impl* This, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi )
{
  lpGroupData  lpGData;
  lpPlayerList lpPList;

  FIXME("(%p)->(0x%08lx,%p,%p,%p,0x%08lx,%u): semi stub\n",
          This, idGroup, lpguidInstance, lpEnumPlayersCallback2,
          lpContext, dwFlags, bAnsi );

  if( This->dp2->connectionInitialized == NO_PROVIDER )
  {
    return DPERR_UNINITIALIZED;
  }

  /* Find the group */
  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  if( DPQ_IS_EMPTY( lpGData->players ) )
  {
    return DP_OK;
  }

  lpPList = DPQ_FIRST( lpGData->players );

  /* Walk the players in this group */
  for( ;; )
  {
    /* We do not enum the name server or app server as they are of no
     * concequence to the end user.
     */
    if( ( lpPList->lpPData->dpid != DPID_NAME_SERVER ) &&
        ( lpPList->lpPData->dpid != DPID_SERVERPLAYER )
      )
    {

      /* FIXME: Need to add stuff for dwFlags checking */

      if( !lpEnumPlayersCallback2( lpPList->lpPData->dpid, DPPLAYERTYPE_PLAYER,
                                   &lpPList->lpPData->name,
                                   lpPList->lpPData->dwFlags,
                                   lpContext )
        )
      {
        /* User requested break */
        return DP_OK;
      }
    }

    if( DPQ_IS_ENDOFLIST( lpPList->players ) )
    {
      break;
    }

    lpPList = DPQ_NEXT( lpPList->players );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_EnumGroupPlayers
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumGroupPlayers( This, idGroup, lpguidInstance,
                               lpEnumPlayersCallback2, lpContext,
                               dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_EnumGroupPlayers
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumGroupPlayers( This, idGroup, lpguidInstance,
                               lpEnumPlayersCallback2, lpContext,
                               dwFlags, FALSE );
}

/* NOTE: This only enumerates top level groups (created with CreateGroup) */
static HRESULT WINAPI DP_IF_EnumGroups
          ( IDirectPlay2Impl* This, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi )
{
  return DP_IF_EnumGroupsInGroup( (IDirectPlay3Impl*)This,
                                  DPID_SYSTEM_GROUP, lpguidInstance,
                                  lpEnumPlayersCallback2, lpContext,
                                  dwFlags, bAnsi );
}

static HRESULT WINAPI DirectPlay2AImpl_EnumGroups
          ( LPDIRECTPLAY2A iface, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumGroups( This, lpguidInstance, lpEnumPlayersCallback2,
                         lpContext, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_EnumGroups
          ( LPDIRECTPLAY2 iface, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumGroups( This, lpguidInstance, lpEnumPlayersCallback2,
                         lpContext, dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_EnumPlayers
          ( IDirectPlay2Impl* This, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi )
{
  return DP_IF_EnumGroupPlayers( This, DPID_SYSTEM_GROUP, lpguidInstance,
                                 lpEnumPlayersCallback2, lpContext,
                                 dwFlags, bAnsi );
}

static HRESULT WINAPI DirectPlay2AImpl_EnumPlayers
          ( LPDIRECTPLAY2A iface, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumPlayers( This, lpguidInstance, lpEnumPlayersCallback2,
                          lpContext, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_EnumPlayers
          ( LPDIRECTPLAY2 iface, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumPlayers( This, lpguidInstance, lpEnumPlayersCallback2,
                          lpContext, dwFlags, FALSE );
}

/* This function should call the registered callback function that the user
   passed into EnumSessions for each entry available.
 */
static void DP_InvokeEnumSessionCallbacks
       ( LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
         LPVOID lpNSInfo,
         DWORD dwTimeout,
         LPVOID lpContext )
{
  LPDPSESSIONDESC2 lpSessionDesc;

  FIXME( ": not checking for conditions\n" );

  /* Not sure if this should be pruning but it's convenient */
  NS_PruneSessionCache( lpNSInfo );

  NS_ResetSessionEnumeration( lpNSInfo );

  /* Enumerate all sessions */
  /* FIXME: Need to indicate ANSI */
  while( (lpSessionDesc = NS_WalkSessions( lpNSInfo ) ) != NULL )
  {
    TRACE( "EnumSessionsCallback2 invoked\n" );
    if( !lpEnumSessionsCallback2( lpSessionDesc, &dwTimeout, 0, lpContext ) )
    {
      return;
    }
  }

  /* Invoke one last time to indicate that there is no more to come */
  lpEnumSessionsCallback2( NULL, &dwTimeout, DPESC_TIMEDOUT, lpContext );
}

static DWORD CALLBACK DP_EnumSessionsSendAsyncRequestThread( LPVOID lpContext )
{
  EnumSessionAsyncCallbackData* data = (EnumSessionAsyncCallbackData*)lpContext;
  HANDLE hSuicideRequest = data->hSuicideRequest;
  DWORD dwTimeout = data->dwTimeout;

  TRACE( "Thread started with timeout = 0x%08lx\n", dwTimeout );

  for( ;; )
  {
    HRESULT hr;

    /* Sleep up to dwTimeout waiting for request to terminate thread */
    if( WaitForSingleObject( hSuicideRequest, dwTimeout ) == WAIT_OBJECT_0 )
    {
      TRACE( "Thread terminating on terminate request\n" );
      break;
    }

    /* Now resend the enum request */
    hr = NS_SendSessionRequestBroadcast( &data->requestGuid,
                                         data->dwEnumSessionFlags,
                                         data->lpSpData );

    if( FAILED(hr) )
    {
      ERR( "Enum broadcase request failed: %s\n", DPLAYX_HresultToString(hr) );
      /* FIXME: Should we kill this thread? How to inform the main thread? */
    }

  }

  TRACE( "Thread terminating\n" );

  /* Clean up the thread data */
  CloseHandle( hSuicideRequest );
  HeapFree( GetProcessHeap(), 0, lpContext );

  /* FIXME: Need to have some notification to main app thread that this is
   *        dead. It would serve two purposes. 1) allow sync on termination
   *        so that we don't actually send something to ourselves when we
   *        become name server (race condition) and 2) so that if we die
   *        abnormally something else will be able to tell.
   */

  return 1;
}

static void DP_KillEnumSessionThread( IDirectPlay2Impl* This )
{
  /* Does a thread exist? If so we were doing an async enum session */
  if( This->dp2->hEnumSessionThread != INVALID_HANDLE_VALUE )
  {
    TRACE( "Killing EnumSession thread %p\n",
           This->dp2->hEnumSessionThread );

    /* Request that the thread kill itself nicely */
    SetEvent( This->dp2->hKillEnumSessionThreadEvent );
    CloseHandle( This->dp2->hKillEnumSessionThreadEvent );

    /* We no longer need to know about the thread */
    CloseHandle( This->dp2->hEnumSessionThread );

    This->dp2->hEnumSessionThread = INVALID_HANDLE_VALUE;
  }
}

static HRESULT WINAPI DP_IF_EnumSessions
          ( IDirectPlay2Impl* This, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout,
            LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi )
{
  HRESULT hr = DP_OK;

  TRACE( "(%p)->(%p,0x%08lx,%p,%p,0x%08lx,%u)\n",
         This, lpsd, dwTimeout, lpEnumSessionsCallback2, lpContext, dwFlags,
         bAnsi );

  /* Can't enumerate if the interface is already open */
  if( This->dp2->bConnectionOpen )
  {
    return DPERR_GENERIC;
  }

#if 1
  /* The loading of a lobby provider _seems_ to require a backdoor loading
   * of the service provider to also associate with this DP object. This is
   * because the app doesn't seem to have to call EnumConnections and
   * InitializeConnection for the SP before calling this method. As such
   * we'll do their dirty work for them with a quick hack so as to always
   * load the TCP/IP service provider.
   *
   * The correct solution would seem to involve creating a dialog box which
   * contains the possible SPs. These dialog boxes most likely follow SDK
   * examples.
   */
   if( This->dp2->bDPLSPInitialized && !This->dp2->bSPInitialized )
   {
     LPVOID lpConnection;
     DWORD  dwSize;

     WARN( "Hack providing TCP/IP SP for lobby provider activated\n" );

     if( !DP_BuildSPCompoundAddr( (LPGUID)&DPSPGUID_TCPIP, &lpConnection, &dwSize ) )
     {
       ERR( "Can't build compound addr\n" );
       return DPERR_GENERIC;
     }

     hr = DP_IF_InitializeConnection( (IDirectPlay3Impl*)This, lpConnection,
                                      0, bAnsi );
     if( FAILED(hr) )
     {
       return hr;
     }

     /* Free up the address buffer */
     HeapFree( GetProcessHeap(), 0, lpConnection );

     /* The SP is now initialized */
     This->dp2->bSPInitialized = TRUE;
   }
#endif


  /* Use the service provider default? */
  if( dwTimeout == 0 )
  {
    DPCAPS spCaps;
    spCaps.dwSize = sizeof( spCaps );

    DP_IF_GetCaps( This, &spCaps, 0 );
    dwTimeout = spCaps.dwTimeout;

    /* The service provider doesn't provide one either! */
    if( dwTimeout == 0 )
    {
      /* Provide the TCP/IP default */
      dwTimeout = DPMSG_WAIT_5_SECS;
    }
  }

  if( dwFlags & DPENUMSESSIONS_STOPASYNC )
  {
    DP_KillEnumSessionThread( This );
    return hr;
  }

  /* FIXME: Interface locking sucks in this method */
  if( ( dwFlags & DPENUMSESSIONS_ASYNC ) )
  {
    /* Enumerate everything presently in the local session cache */
    DP_InvokeEnumSessionCallbacks( lpEnumSessionsCallback2,
                                   This->dp2->lpNameServerData, dwTimeout,
                                   lpContext );


    /* See if we've already created a thread to service this interface */
    if( This->dp2->hEnumSessionThread == INVALID_HANDLE_VALUE )
    {
      DWORD dwThreadId;

      /* Send the first enum request inline since the user may cancel a dialog
       * if one is presented. Also, may also have a connecting return code.
       */
      hr = NS_SendSessionRequestBroadcast( &lpsd->guidApplication,
                                           dwFlags, &This->dp2->spData );

      if( !FAILED(hr) )
      {
        EnumSessionAsyncCallbackData* lpData
          = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpData ) );
        /* FIXME: need to kill the thread on object deletion */
        lpData->lpSpData  = &This->dp2->spData;

        CopyMemory( &lpData->requestGuid, &lpsd->guidApplication, sizeof(GUID) );
        lpData->dwEnumSessionFlags = dwFlags;
        lpData->dwTimeout = dwTimeout;

        This->dp2->hKillEnumSessionThreadEvent =
          CreateEventW( NULL, TRUE, FALSE, NULL );

        if( !DuplicateHandle( GetCurrentProcess(),
                              This->dp2->hKillEnumSessionThreadEvent,
                              GetCurrentProcess(),
                              &lpData->hSuicideRequest,
                              0, FALSE, DUPLICATE_SAME_ACCESS )
          )
        {
          ERR( "Can't duplicate thread killing handle\n" );
        }

        TRACE( ": creating EnumSessionsRequest thread\n" );

        This->dp2->hEnumSessionThread = CreateThread( NULL,
                                                      0,
                                                      DP_EnumSessionsSendAsyncRequestThread,
                                                      lpData,
                                                      0,
                                                      &dwThreadId );
      }
    }
  }
  else
  {
    /* Invalidate the session cache for the interface */
    NS_InvalidateSessionCache( This->dp2->lpNameServerData );

    /* Send the broadcast for session enumeration */
    hr = NS_SendSessionRequestBroadcast( &lpsd->guidApplication,
                                         dwFlags,
                                         &This->dp2->spData );


    SleepEx( dwTimeout, FALSE );

    DP_InvokeEnumSessionCallbacks( lpEnumSessionsCallback2,
                                   This->dp2->lpNameServerData, dwTimeout,
                                   lpContext );
  }

  return hr;
}

static HRESULT WINAPI DirectPlay2AImpl_EnumSessions
          ( LPDIRECTPLAY2A iface, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout,
            LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumSessions( This, lpsd, dwTimeout, lpEnumSessionsCallback2,
                             lpContext, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_EnumSessions
          ( LPDIRECTPLAY2 iface, LPDPSESSIONDESC2 lpsd, DWORD dwTimeout,
            LPDPENUMSESSIONSCALLBACK2 lpEnumSessionsCallback2,
            LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_EnumSessions( This, lpsd, dwTimeout, lpEnumSessionsCallback2,
                             lpContext, dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_GetPlayerCaps
          ( IDirectPlay2Impl* This, DPID idPlayer, LPDPCAPS lpDPCaps,
            DWORD dwFlags )
{
  DPSP_GETCAPSDATA data;

  TRACE("(%p)->(0x%08lx,%p,0x%08lx)\n", This, idPlayer, lpDPCaps, dwFlags);

  /* Query the service provider */
  data.idPlayer = idPlayer;
  data.dwFlags  = dwFlags;
  data.lpCaps   = lpDPCaps;
  data.lpISP    = This->dp2->spData.lpISP;

  return (*This->dp2->spData.lpCB->GetCaps)( &data );
}

static HRESULT WINAPI DP_IF_GetCaps
          ( IDirectPlay2Impl* This, LPDPCAPS lpDPCaps, DWORD dwFlags )
{
  return DP_IF_GetPlayerCaps( This, DPID_ALLPLAYERS, lpDPCaps, dwFlags );
}

static HRESULT WINAPI DirectPlay2AImpl_GetCaps
          ( LPDIRECTPLAY2A iface, LPDPCAPS lpDPCaps, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetCaps( This, lpDPCaps, dwFlags );
}

static HRESULT WINAPI DirectPlay2WImpl_GetCaps
          ( LPDIRECTPLAY2 iface, LPDPCAPS lpDPCaps, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetCaps( This, lpDPCaps, dwFlags );
}

static HRESULT WINAPI DP_IF_GetGroupData
          ( IDirectPlay2Impl* This, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags, BOOL bAnsi )
{
  lpGroupData lpGData;
  DWORD dwRequiredBufferSize;
  LPVOID lpCopyDataFrom;

  TRACE( "(%p)->(0x%08lx,%p,%p,0x%08lx,%u)\n",
         This, idGroup, lpData, lpdwDataSize, dwFlags, bAnsi );

  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  /* How much buffer is required? */
  if( dwFlags & DPSET_LOCAL )
  {
    dwRequiredBufferSize = lpGData->dwLocalDataSize;
    lpCopyDataFrom       = lpGData->lpLocalData;
  }
  else
  {
    dwRequiredBufferSize = lpGData->dwRemoteDataSize;
    lpCopyDataFrom       = lpGData->lpRemoteData;
  }

  /* Is the user requesting to know how big a buffer is required? */
  if( ( lpData == NULL ) ||
      ( *lpdwDataSize < dwRequiredBufferSize )
    )
  {
    *lpdwDataSize = dwRequiredBufferSize;
    return DPERR_BUFFERTOOSMALL;
  }

  CopyMemory( lpData, lpCopyDataFrom, dwRequiredBufferSize );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetGroupData
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetGroupData( This, idGroup, lpData, lpdwDataSize,
                           dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_GetGroupData
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetGroupData( This, idGroup, lpData, lpdwDataSize,
                           dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_GetGroupName
          ( IDirectPlay2Impl* This, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize, BOOL bAnsi )
{
  lpGroupData lpGData;
  LPDPNAME    lpName = (LPDPNAME)lpData;
  DWORD       dwRequiredDataSize;

  FIXME("(%p)->(0x%08lx,%p,%p,%u) ANSI ignored\n",
          This, idGroup, lpData, lpdwDataSize, bAnsi );

  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  dwRequiredDataSize = lpGData->name.dwSize;

  if( lpGData->name.lpszShortNameA )
  {
    dwRequiredDataSize += strlen( lpGData->name.lpszShortNameA ) + 1;
  }

  if( lpGData->name.lpszLongNameA )
  {
    dwRequiredDataSize += strlen( lpGData->name.lpszLongNameA ) + 1;
  }

  if( ( lpData == NULL ) ||
      ( *lpdwDataSize < dwRequiredDataSize )
    )
  {
    *lpdwDataSize = dwRequiredDataSize;
    return DPERR_BUFFERTOOSMALL;
  }

  /* Copy the structure */
  CopyMemory( lpName, &lpGData->name, lpGData->name.dwSize );

  if( lpGData->name.lpszShortNameA )
  {
    strcpy( ((char*)lpName)+lpGData->name.dwSize,
            lpGData->name.lpszShortNameA );
  }
  else
  {
    lpName->lpszShortNameA = NULL;
  }

  if( lpGData->name.lpszShortNameA )
  {
    strcpy( ((char*)lpName)+lpGData->name.dwSize,
            lpGData->name.lpszLongNameA );
  }
  else
  {
    lpName->lpszLongNameA = NULL;
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetGroupName
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetGroupName( This, idGroup, lpData, lpdwDataSize, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_GetGroupName
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData,
            LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetGroupName( This, idGroup, lpData, lpdwDataSize, FALSE );
}

static HRESULT WINAPI DP_IF_GetMessageCount
          ( IDirectPlay2Impl* This, DPID idPlayer,
            LPDWORD lpdwCount, BOOL bAnsi )
{
  FIXME("(%p)->(0x%08lx,%p,%u): stub\n", This, idPlayer, lpdwCount, bAnsi );
  return DP_IF_GetMessageQueue( (IDirectPlay4Impl*)This, 0, idPlayer,
                                DPMESSAGEQUEUE_RECEIVE, lpdwCount, NULL,
                                bAnsi );
}

static HRESULT WINAPI DirectPlay2AImpl_GetMessageCount
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPDWORD lpdwCount )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetMessageCount( This, idPlayer, lpdwCount, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_GetMessageCount
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPDWORD lpdwCount )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetMessageCount( This, idPlayer, lpdwCount, FALSE );
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerAddress
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idPlayer, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerAddress
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p,%p): stub\n", This, idPlayer, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerCaps
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPDPCAPS lpPlayerCaps,
            DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetPlayerCaps( This, idPlayer, lpPlayerCaps, dwFlags );
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerCaps
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPDPCAPS lpPlayerCaps,
            DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetPlayerCaps( This, idPlayer, lpPlayerCaps, dwFlags );
}

static HRESULT WINAPI DP_IF_GetPlayerData
          ( IDirectPlay2Impl* This, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags, BOOL bAnsi )
{
  lpPlayerList lpPList;
  DWORD dwRequiredBufferSize;
  LPVOID lpCopyDataFrom;

  TRACE( "(%p)->(0x%08lx,%p,%p,0x%08lx,%u)\n",
         This, idPlayer, lpData, lpdwDataSize, dwFlags, bAnsi );

  if( This->dp2->connectionInitialized == NO_PROVIDER )
  {
    return DPERR_UNINITIALIZED;
  }

  if( ( lpPList = DP_FindPlayer( This, idPlayer ) ) == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  /* How much buffer is required? */
  if( dwFlags & DPSET_LOCAL )
  {
    dwRequiredBufferSize = lpPList->lpPData->dwLocalDataSize;
    lpCopyDataFrom       = lpPList->lpPData->lpLocalData;
  }
  else
  {
    dwRequiredBufferSize = lpPList->lpPData->dwRemoteDataSize;
    lpCopyDataFrom       = lpPList->lpPData->lpRemoteData;
  }

  /* Is the user requesting to know how big a buffer is required? */
  if( ( lpData == NULL ) ||
      ( *lpdwDataSize < dwRequiredBufferSize )
    )
  {
    *lpdwDataSize = dwRequiredBufferSize;
    return DPERR_BUFFERTOOSMALL;
  }

  CopyMemory( lpData, lpCopyDataFrom, dwRequiredBufferSize );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerData
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetPlayerData( This, idPlayer, lpData, lpdwDataSize,
                            dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerData
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetPlayerData( This, idPlayer, lpData, lpdwDataSize,
                            dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_GetPlayerName
          ( IDirectPlay2Impl* This, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize, BOOL bAnsi )
{
  lpPlayerList lpPList;
  LPDPNAME    lpName = (LPDPNAME)lpData;
  DWORD       dwRequiredDataSize;

  FIXME( "(%p)->(0x%08lx,%p,%p,%u): ANSI\n",
         This, idPlayer, lpData, lpdwDataSize, bAnsi );

  if( ( lpPList = DP_FindPlayer( This, idPlayer ) ) == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  dwRequiredDataSize = lpPList->lpPData->name.dwSize;

  if( lpPList->lpPData->name.lpszShortNameA )
  {
    dwRequiredDataSize += strlen( lpPList->lpPData->name.lpszShortNameA ) + 1;
  }

  if( lpPList->lpPData->name.lpszLongNameA )
  {
    dwRequiredDataSize += strlen( lpPList->lpPData->name.lpszLongNameA ) + 1;
  }

  if( ( lpData == NULL ) ||
      ( *lpdwDataSize < dwRequiredDataSize )
    )
  {
    *lpdwDataSize = dwRequiredDataSize;
    return DPERR_BUFFERTOOSMALL;
  }

  /* Copy the structure */
  CopyMemory( lpName, &lpPList->lpPData->name, lpPList->lpPData->name.dwSize );

  if( lpPList->lpPData->name.lpszShortNameA )
  {
    strcpy( ((char*)lpName)+lpPList->lpPData->name.dwSize,
            lpPList->lpPData->name.lpszShortNameA );
  }
  else
  {
    lpName->lpszShortNameA = NULL;
  }

  if( lpPList->lpPData->name.lpszShortNameA )
  {
    strcpy( ((char*)lpName)+lpPList->lpPData->name.dwSize,
            lpPList->lpPData->name.lpszLongNameA );
  }
  else
  {
    lpName->lpszLongNameA = NULL;
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetPlayerName
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetPlayerName( This, idPlayer, lpData, lpdwDataSize, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_GetPlayerName
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData,
            LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_GetPlayerName( This, idPlayer, lpData, lpdwDataSize, FALSE );
}

static HRESULT WINAPI DP_GetSessionDesc
          ( IDirectPlay2Impl* This, LPVOID lpData, LPDWORD lpdwDataSize,
            BOOL bAnsi )
{
  DWORD dwRequiredSize;

  TRACE( "(%p)->(%p,%p,%u)\n", This, lpData, lpdwDataSize, bAnsi );

  if( This->dp2->connectionInitialized == NO_PROVIDER )
  {
    return DPERR_UNINITIALIZED;
  }

  if( ( lpData == NULL ) && ( lpdwDataSize == NULL ) )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* FIXME: Get from This->dp2->lpSessionDesc */
  dwRequiredSize = DP_CalcSessionDescSize( This->dp2->lpSessionDesc, bAnsi );

  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < dwRequiredSize )
     )
  {
    *lpdwDataSize = dwRequiredSize;
    return DPERR_BUFFERTOOSMALL;
  }

  DP_CopySessionDesc( lpData, This->dp2->lpSessionDesc, bAnsi );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_GetSessionDesc
          ( LPDIRECTPLAY2A iface, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_GetSessionDesc( This, lpData, lpdwDataSize, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_GetSessionDesc
          ( LPDIRECTPLAY2 iface, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_GetSessionDesc( This, lpData, lpdwDataSize, TRUE );
}

/* Intended only for COM compatibility. Always returns an error. */
static HRESULT WINAPI DirectPlay2AImpl_Initialize
          ( LPDIRECTPLAY2A iface, LPGUID lpGUID )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  TRACE("(%p)->(%p): stub\n", This, lpGUID );
  return DPERR_ALREADYINITIALIZED;
}

/* Intended only for COM compatibility. Always returns an error. */
static HRESULT WINAPI DirectPlay2WImpl_Initialize
          ( LPDIRECTPLAY2 iface, LPGUID lpGUID )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  TRACE("(%p)->(%p): stub\n", This, lpGUID );
  return DPERR_ALREADYINITIALIZED;
}


static HRESULT WINAPI DP_SecureOpen
          ( IDirectPlay2Impl* This, LPCDPSESSIONDESC2 lpsd, DWORD dwFlags,
            LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials,
            BOOL bAnsi )
{
  HRESULT hr = DP_OK;

  FIXME( "(%p)->(%p,0x%08lx,%p,%p): partial stub\n",
         This, lpsd, dwFlags, lpSecurity, lpCredentials );

  if( This->dp2->bConnectionOpen )
  {
    TRACE( ": rejecting already open connection.\n" );
    return DPERR_ALREADYINITIALIZED;
  }

  /* If we're enumerating, kill the thread */
  DP_KillEnumSessionThread( This );

  if( dwFlags & DPOPEN_CREATE )
  {
    /* Rightoo - this computer is the host and the local computer needs to be
       the name server so that others can join this session */
    NS_SetLocalComputerAsNameServer( lpsd, This->dp2->lpNameServerData );

    This->dp2->bHostInterface = TRUE;

    hr = DP_SetSessionDesc( This, lpsd, 0, TRUE, bAnsi );
    if( FAILED( hr ) )
    {
      ERR( "Unable to set session desc: %s\n", DPLAYX_HresultToString( hr ) );
      return hr;
    }
  }

  /* Invoke the conditional callback for the service provider */
  if( This->dp2->spData.lpCB->Open )
  {
    DPSP_OPENDATA data;

    FIXME( "Not all data fields are correct. Need new parameter\n" );

    data.bCreate           = (dwFlags & DPOPEN_CREATE ) ? TRUE : FALSE;
    data.lpSPMessageHeader = (dwFlags & DPOPEN_CREATE ) ? NULL
                                                        : NS_GetNSAddr( This->dp2->lpNameServerData );
    data.lpISP             = This->dp2->spData.lpISP;
    data.bReturnStatus     = (dwFlags & DPOPEN_RETURNSTATUS) ? TRUE : FALSE;
    data.dwOpenFlags       = dwFlags;
    data.dwSessionFlags    = This->dp2->lpSessionDesc->dwFlags;

    hr = (*This->dp2->spData.lpCB->Open)(&data);
    if( FAILED( hr ) )
    {
      ERR( "Unable to open session: %s\n", DPLAYX_HresultToString( hr ) );
      return hr;
    }
  }

  {
    /* Create the system group of which everything is a part of */
    DPID systemGroup = DPID_SYSTEM_GROUP;

    hr = DP_IF_CreateGroup( This, NULL, &systemGroup, NULL,
                            NULL, 0, 0, TRUE );

  }

  if( dwFlags & DPOPEN_JOIN )
  {
    DPID dpidServerId = DPID_UNKNOWN;

    /* Create the server player for this interface. This way we can receive
     * messages for this session.
     */
    /* FIXME: I suppose that we should be setting an event for a receive
     *        type of thing. That way the messaging thread could know to wake
     *        up. DPlay would then trigger the hEvent for the player the
     *        message is directed to.
     */
    hr = DP_IF_CreatePlayer( This, NULL, &dpidServerId, NULL, 0, NULL,
                             0,
                             DPPLAYER_SERVERPLAYER | DPPLAYER_LOCAL , bAnsi );

  }
  else if( dwFlags & DPOPEN_CREATE )
  {
    DPID dpidNameServerId = DPID_NAME_SERVER;

    hr = DP_IF_CreatePlayer( This, NULL, &dpidNameServerId, NULL, 0, NULL,
                             0, DPPLAYER_SERVERPLAYER, bAnsi );
  }

  if( FAILED(hr) )
  {
    ERR( "Couldn't create name server/system player: %s\n",
         DPLAYX_HresultToString(hr) );
  }

  return hr;
}

static HRESULT WINAPI DirectPlay2AImpl_Open
          ( LPDIRECTPLAY2A iface, LPDPSESSIONDESC2 lpsd, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  TRACE("(%p)->(%p,0x%08lx)\n", This, lpsd, dwFlags );
  return DP_SecureOpen( This, lpsd, dwFlags, NULL, NULL, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_Open
          ( LPDIRECTPLAY2 iface, LPDPSESSIONDESC2 lpsd, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  TRACE("(%p)->(%p,0x%08lx)\n", This, lpsd, dwFlags );
  return DP_SecureOpen( This, lpsd, dwFlags, NULL, NULL, FALSE );
}

static HRESULT WINAPI DP_IF_Receive
          ( IDirectPlay2Impl* This, LPDPID lpidFrom, LPDPID lpidTo,
            DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize, BOOL bAnsi )
{
  LPDPMSG lpMsg = NULL;

  FIXME( "(%p)->(%p,%p,0x%08lx,%p,%p,%u): stub\n",
         This, lpidFrom, lpidTo, dwFlags, lpData, lpdwDataSize, bAnsi );

  if( This->dp2->connectionInitialized == NO_PROVIDER )
  {
    return DPERR_UNINITIALIZED;
  }

  if( dwFlags == 0 )
  {
    dwFlags = DPRECEIVE_ALL;
  }

  /* If the lpData is NULL, we must be peeking the message */
  if(  ( lpData == NULL ) &&
      !( dwFlags & DPRECEIVE_PEEK )
    )
  {
    return DPERR_INVALIDPARAMS;
  }

  if( dwFlags & DPRECEIVE_ALL )
  {
    lpMsg = This->dp2->receiveMsgs.lpQHFirst;

    if( !( dwFlags & DPRECEIVE_PEEK ) )
    {
      FIXME( "Remove from queue\n" );
    }
  }
  else if( ( dwFlags & DPRECEIVE_TOPLAYER ) ||
           ( dwFlags & DPRECEIVE_FROMPLAYER )
         )
  {
    FIXME( "Find matching message 0x%08lx\n", dwFlags );
  }
  else
  {
    ERR( "Hmmm..dwFlags 0x%08lx\n", dwFlags );
  }

  if( lpMsg == NULL )
  {
    return DPERR_NOMESSAGES;
  }

  /* Copy into the provided buffer */
  CopyMemory( lpData, lpMsg->msg, *lpdwDataSize );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_Receive
          ( LPDIRECTPLAY2A iface, LPDPID lpidFrom, LPDPID lpidTo,
            DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_Receive( This, lpidFrom, lpidTo, dwFlags,
                        lpData, lpdwDataSize, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_Receive
          ( LPDIRECTPLAY2 iface, LPDPID lpidFrom, LPDPID lpidTo,
            DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_Receive( This, lpidFrom, lpidTo, dwFlags,
                        lpData, lpdwDataSize, FALSE );
}

static HRESULT WINAPI DirectPlay2AImpl_Send
          ( LPDIRECTPLAY2A iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_SendEx( This, idFrom, idTo, dwFlags, lpData, dwDataSize,
                    0, 0, NULL, NULL, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_Send
          ( LPDIRECTPLAY2 iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPVOID lpData, DWORD dwDataSize )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_SendEx( This, idFrom, idTo, dwFlags, lpData, dwDataSize,
                    0, 0, NULL, NULL, FALSE );
}

static HRESULT WINAPI DP_IF_SetGroupData
          ( IDirectPlay2Impl* This, DPID idGroup, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi )
{
  lpGroupData lpGData;

  TRACE( "(%p)->(0x%08lx,%p,0x%08lx,0x%08lx,%u)\n",
         This, idGroup, lpData, dwDataSize, dwFlags, bAnsi );

  /* Parameter check */
  if( ( lpData == NULL ) &&
      ( dwDataSize != 0 )
    )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* Find the pointer to the data for this player */
  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDOBJECT;
  }

  if( !(dwFlags & DPSET_LOCAL) )
  {
    FIXME( "Was this group created by this interface?\n" );
    /* FIXME: If this is a remote update need to allow it but not
     *        send a message.
     */
  }

  DP_SetGroupData( lpGData, dwFlags, lpData, dwDataSize );

  /* FIXME: Only send a message if this group is local to the session otherwise
   * it will have been rejected above
   */
  if( !(dwFlags & DPSET_LOCAL) )
  {
    FIXME( "Send msg?\n" );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetGroupData
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetGroupData( This, idGroup, lpData, dwDataSize, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_SetGroupData
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetGroupData( This, idGroup, lpData, dwDataSize, dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_SetGroupName
          ( IDirectPlay2Impl* This, DPID idGroup, LPDPNAME lpGroupName,
            DWORD dwFlags, BOOL bAnsi )
{
  lpGroupData lpGData;

  TRACE( "(%p)->(0x%08lx,%p,0x%08lx,%u)\n", This, idGroup,
         lpGroupName, dwFlags, bAnsi );

  if( ( lpGData = DP_FindAnyGroup( This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  DP_CopyDPNAMEStruct( &lpGData->name, lpGroupName, bAnsi );

  /* Should send a DPMSG_SETPLAYERORGROUPNAME message */
  FIXME( "Message not sent and dwFlags ignored\n" );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetGroupName
          ( LPDIRECTPLAY2A iface, DPID idGroup, LPDPNAME lpGroupName,
            DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetGroupName( This, idGroup, lpGroupName, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_SetGroupName
          ( LPDIRECTPLAY2 iface, DPID idGroup, LPDPNAME lpGroupName,
            DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetGroupName( This, idGroup, lpGroupName, dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_SetPlayerData
          ( IDirectPlay2Impl* This, DPID idPlayer, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi )
{
  lpPlayerList lpPList;

  TRACE( "(%p)->(0x%08lx,%p,0x%08lx,0x%08lx,%u)\n",
         This, idPlayer, lpData, dwDataSize, dwFlags, bAnsi );

  /* Parameter check */
  if( ( lpData == NULL ) &&
      ( dwDataSize != 0 )
    )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* Find the pointer to the data for this player */
  if( ( lpPList = DP_FindPlayer( This, idPlayer ) ) == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  if( !(dwFlags & DPSET_LOCAL) )
  {
    FIXME( "Was this group created by this interface?\n" );
    /* FIXME: If this is a remote update need to allow it but not
     *        send a message.
     */
  }

  DP_SetPlayerData( lpPList->lpPData, dwFlags, lpData, dwDataSize );

  if( !(dwFlags & DPSET_LOCAL) )
  {
    FIXME( "Send msg?\n" );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetPlayerData
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetPlayerData( This, idPlayer, lpData, dwDataSize,
                              dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_SetPlayerData
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetPlayerData( This, idPlayer, lpData, dwDataSize,
                              dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_SetPlayerName
          ( IDirectPlay2Impl* This, DPID idPlayer, LPDPNAME lpPlayerName,
            DWORD dwFlags, BOOL bAnsi )
{
  lpPlayerList lpPList;

  TRACE( "(%p)->(0x%08lx,%p,0x%08lx,%u)\n",
         This, idPlayer, lpPlayerName, dwFlags, bAnsi );

  if( ( lpPList = DP_FindPlayer( This, idPlayer ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  DP_CopyDPNAMEStruct( &lpPList->lpPData->name, lpPlayerName, bAnsi );

  /* Should send a DPMSG_SETPLAYERORGROUPNAME message */
  FIXME( "Message not sent and dwFlags ignored\n" );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetPlayerName
          ( LPDIRECTPLAY2A iface, DPID idPlayer, LPDPNAME lpPlayerName,
            DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetPlayerName( This, idPlayer, lpPlayerName, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_SetPlayerName
          ( LPDIRECTPLAY2 iface, DPID idPlayer, LPDPNAME lpPlayerName,
            DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_IF_SetPlayerName( This, idPlayer, lpPlayerName, dwFlags, FALSE );
}

static HRESULT WINAPI DP_SetSessionDesc
          ( IDirectPlay2Impl* This, LPCDPSESSIONDESC2 lpSessDesc,
            DWORD dwFlags, BOOL bInitial, BOOL bAnsi  )
{
  DWORD            dwRequiredSize;
  LPDPSESSIONDESC2 lpTempSessDesc;

  TRACE( "(%p)->(%p,0x%08lx,%u,%u)\n",
         This, lpSessDesc, dwFlags, bInitial, bAnsi );

  if( This->dp2->connectionInitialized == NO_PROVIDER )
  {
    return DPERR_UNINITIALIZED;
  }

  if( dwFlags )
  {
    return DPERR_INVALIDPARAMS;
  }

  /* Only the host is allowed to update the session desc */
  if( !This->dp2->bHostInterface )
  {
    return DPERR_ACCESSDENIED;
  }

  /* FIXME: Copy into This->dp2->lpSessionDesc */
  dwRequiredSize = DP_CalcSessionDescSize( lpSessDesc, bAnsi );
  lpTempSessDesc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwRequiredSize );

  if( lpTempSessDesc == NULL )
  {
    return DPERR_OUTOFMEMORY;
  }

  /* Free the old */
  HeapFree( GetProcessHeap(), 0, This->dp2->lpSessionDesc );

  This->dp2->lpSessionDesc = lpTempSessDesc;

  /* Set the new */
  DP_CopySessionDesc( This->dp2->lpSessionDesc, lpSessDesc, bAnsi );

  /* If this is an external invocation of the interface, we should be
   * letting everyone know that things have changed. Otherwise this is
   * just an initialization and it doesn't need to be propagated.
   */
  if( !bInitial )
  {
    FIXME( "Need to send a DPMSG_SETSESSIONDESC msg to everyone\n" );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay2AImpl_SetSessionDesc
          ( LPDIRECTPLAY2A iface, LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_SetSessionDesc( This, lpSessDesc, dwFlags, FALSE, TRUE );
}

static HRESULT WINAPI DirectPlay2WImpl_SetSessionDesc
          ( LPDIRECTPLAY2 iface, LPDPSESSIONDESC2 lpSessDesc, DWORD dwFlags )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface;
  return DP_SetSessionDesc( This, lpSessDesc, dwFlags, FALSE, TRUE );
}

/* FIXME: See about merging some of this stuff with dplayx_global.c stuff */
DWORD DP_CalcSessionDescSize( LPCDPSESSIONDESC2 lpSessDesc, BOOL bAnsi )
{
  DWORD dwSize = 0;

  if( lpSessDesc == NULL )
  {
    /* Hmmm..don't need any size? */
    ERR( "NULL lpSessDesc\n" );
    return dwSize;
  }

  dwSize += sizeof( *lpSessDesc );

  if( bAnsi )
  {
    if( lpSessDesc->lpszSessionNameA )
    {
      dwSize += lstrlenA( lpSessDesc->lpszSessionNameA ) + 1;
    }

    if( lpSessDesc->lpszPasswordA )
    {
      dwSize += lstrlenA( lpSessDesc->lpszPasswordA ) + 1;
    }
  }
  else /* UNICODE */
  {
    if( lpSessDesc->lpszSessionName )
    {
      dwSize += sizeof( WCHAR ) *
        ( lstrlenW( lpSessDesc->lpszSessionName ) + 1 );
    }

    if( lpSessDesc->lpszPassword )
    {
      dwSize += sizeof( WCHAR ) *
        ( lstrlenW( lpSessDesc->lpszPassword ) + 1 );
    }
  }

  return dwSize;
}

/* Assumes that contugous buffers are already allocated. */
static void DP_CopySessionDesc( LPDPSESSIONDESC2 lpSessionDest,
                                LPCDPSESSIONDESC2 lpSessionSrc, BOOL bAnsi )
{
  BYTE* lpStartOfFreeSpace;

  if( lpSessionDest == NULL )
  {
    ERR( "NULL lpSessionDest\n" );
    return;
  }

  CopyMemory( lpSessionDest, lpSessionSrc, sizeof( *lpSessionSrc ) );

  lpStartOfFreeSpace = ((BYTE*)lpSessionDest) + sizeof( *lpSessionSrc );

  if( bAnsi )
  {
    if( lpSessionSrc->lpszSessionNameA )
    {
      lstrcpyA( (LPSTR)lpStartOfFreeSpace,
                lpSessionDest->lpszSessionNameA );
      lpSessionDest->lpszSessionNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=
        lstrlenA( (LPSTR)lpSessionDest->lpszSessionNameA ) + 1;
    }

    if( lpSessionSrc->lpszPasswordA )
    {
      lstrcpyA( (LPSTR)lpStartOfFreeSpace,
                lpSessionDest->lpszPasswordA );
      lpSessionDest->lpszPasswordA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=
        lstrlenA( (LPSTR)lpSessionDest->lpszPasswordA ) + 1;
    }
  }
  else /* UNICODE */
  {
    if( lpSessionSrc->lpszSessionName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace,
                lpSessionDest->lpszSessionName );
      lpSessionDest->lpszSessionName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace += sizeof(WCHAR) *
        ( lstrlenW( (LPWSTR)lpSessionDest->lpszSessionName ) + 1 );
    }

    if( lpSessionSrc->lpszPassword )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace,
                lpSessionDest->lpszPassword );
      lpSessionDest->lpszPassword = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace += sizeof(WCHAR) *
        ( lstrlenW( (LPWSTR)lpSessionDest->lpszPassword ) + 1 );
    }
  }
}


static HRESULT WINAPI DP_IF_AddGroupToGroup
          ( IDirectPlay3Impl* This, DPID idParentGroup, DPID idGroup )
{
  lpGroupData lpGParentData;
  lpGroupData lpGData;
  lpGroupList lpNewGList;

  TRACE( "(%p)->(0x%08lx,0x%08lx)\n", This, idParentGroup, idGroup );

  if( ( lpGParentData = DP_FindAnyGroup( (IDirectPlay2AImpl*)This, idParentGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  if( ( lpGData = DP_FindAnyGroup( (IDirectPlay2AImpl*)This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  /* Create a player list (ie "shortcut" ) */
  lpNewGList = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpNewGList ) );
  if( lpNewGList == NULL )
  {
    return DPERR_CANTADDPLAYER;
  }

  /* Add the shortcut */
  lpGData->uRef++;
  lpNewGList->lpGData = lpGData;

  /* Add the player to the list of players for this group */
  DPQ_INSERT( lpGData->groups, lpNewGList, groups );

  /* Send a ADDGROUPTOGROUP message */
  FIXME( "Not sending message\n" );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_AddGroupToGroup
          ( LPDIRECTPLAY3A iface, DPID idParentGroup, DPID idGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_AddGroupToGroup( This, idParentGroup, idGroup );
}

static HRESULT WINAPI DirectPlay3WImpl_AddGroupToGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, DPID idGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_AddGroupToGroup( This, idParentGroup, idGroup );
}

static HRESULT WINAPI DP_IF_CreateGroupInGroup
          ( IDirectPlay3Impl* This, LPVOID lpMsgHdr, DPID idParentGroup,
            LPDPID lpidGroup, LPDPNAME lpGroupName, LPVOID lpData,
            DWORD dwDataSize, DWORD dwFlags, BOOL bAnsi )
{
  lpGroupData lpGParentData;
  lpGroupList lpGList;
  lpGroupData lpGData;

  TRACE( "(%p)->(0x%08lx,%p,%p,%p,0x%08lx,0x%08lx,%u)\n",
         This, idParentGroup, lpidGroup, lpGroupName, lpData,
         dwDataSize, dwFlags, bAnsi );

  /* Verify that the specified parent is valid */
  if( ( lpGParentData = DP_FindAnyGroup( (IDirectPlay2AImpl*)This,
                                         idParentGroup ) ) == NULL
    )
  {
    return DPERR_INVALIDGROUP;
  }

  lpGData = DP_CreateGroup( (IDirectPlay2AImpl*)This, lpidGroup, lpGroupName,
                            dwFlags, idParentGroup, bAnsi );

  if( lpGData == NULL )
  {
    return DPERR_CANTADDPLAYER; /* yes player not group */
  }

  /* Something else is referencing this data */
  lpGData->uRef++;

  DP_SetGroupData( lpGData, DPSET_REMOTE, lpData, dwDataSize );

  /* The list has now been inserted into the interface group list. We now
     need to put a "shortcut" to this group in the parent group */
  lpGList = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpGList ) );
  if( lpGList == NULL )
  {
    FIXME( "Memory leak\n" );
    return DPERR_CANTADDPLAYER; /* yes player not group */
  }

  lpGList->lpGData = lpGData;

  DPQ_INSERT( lpGParentData->groups, lpGList, groups );

  /* Let the SP know that we've created this group */
  if( This->dp2->spData.lpCB->CreateGroup )
  {
    DPSP_CREATEGROUPDATA data;

    TRACE( "Calling SP CreateGroup\n" );

    data.idGroup           = *lpidGroup;
    data.dwFlags           = dwFlags;
    data.lpSPMessageHeader = lpMsgHdr;
    data.lpISP             = This->dp2->spData.lpISP;

    (*This->dp2->spData.lpCB->CreateGroup)( &data );
  }

  /* Inform all other peers of the creation of a new group. If there are
   * no peers keep this quiet.
   */
  if( This->dp2->lpSessionDesc &&
      ( This->dp2->lpSessionDesc->dwFlags & DPSESSION_MULTICASTSERVER ) )
  {
    DPMSG_CREATEPLAYERORGROUP msg;

    msg.dwType = DPSYS_CREATEPLAYERORGROUP;
    msg.dwPlayerType = DPPLAYERTYPE_GROUP;
    msg.dpId = *lpidGroup;
    msg.dwCurrentPlayers = idParentGroup; /* FIXME: Incorrect? */
    msg.lpData = lpData;
    msg.dwDataSize = dwDataSize;
    msg.dpnName = *lpGroupName;

    /* FIXME: Correct to just use send effectively? */
    /* FIXME: Should size include data w/ message or just message "header" */
    /* FIXME: Check return code */
    DP_SendEx( (IDirectPlay2Impl*)This,
               DPID_SERVERPLAYER, DPID_ALLPLAYERS, 0, &msg, sizeof( msg ),
               0, 0, NULL, NULL, bAnsi );
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_CreateGroupInGroup
          ( LPDIRECTPLAY3A iface, DPID idParentGroup, LPDPID lpidGroup,
            LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize,
            DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;

  *lpidGroup = DPID_UNKNOWN;

  return DP_IF_CreateGroupInGroup( This, NULL, idParentGroup, lpidGroup,
                                   lpGroupName, lpData, dwDataSize, dwFlags,
                                   TRUE );
}

static HRESULT WINAPI DirectPlay3WImpl_CreateGroupInGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, LPDPID lpidGroup,
            LPDPNAME lpGroupName, LPVOID lpData, DWORD dwDataSize,
            DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;

  *lpidGroup = DPID_UNKNOWN;

  return DP_IF_CreateGroupInGroup( This, NULL, idParentGroup, lpidGroup,
                                   lpGroupName, lpData, dwDataSize,
                                   dwFlags, FALSE );
}

static HRESULT WINAPI DP_IF_DeleteGroupFromGroup
          ( IDirectPlay3Impl* This, DPID idParentGroup, DPID idGroup )
{
  lpGroupList lpGList;
  lpGroupData lpGParentData;

  TRACE("(%p)->(0x%08lx,0x%08lx)\n", This, idParentGroup, idGroup );

  /* Is the parent group valid? */
  if( ( lpGParentData = DP_FindAnyGroup( (IDirectPlay2AImpl*)This, idParentGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  /* Remove the group from the parent group queue */
  DPQ_REMOVE_ENTRY( lpGParentData->groups, groups, lpGData->dpid, ==, idGroup, lpGList );

  if( lpGList == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  /* Decrement the ref count */
  lpGList->lpGData->uRef--;

  /* Free up the list item */
  HeapFree( GetProcessHeap(), 0, lpGList );

  /* Should send a DELETEGROUPFROMGROUP message */
  FIXME( "message not sent\n" );

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_DeleteGroupFromGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, DPID idGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_DeleteGroupFromGroup( This, idParentGroup, idGroup );
}

static HRESULT WINAPI DirectPlay3WImpl_DeleteGroupFromGroup
          ( LPDIRECTPLAY3 iface, DPID idParentGroup, DPID idGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_DeleteGroupFromGroup( This, idParentGroup, idGroup );
}

static
BOOL WINAPI DP_BuildSPCompoundAddr( LPGUID lpcSpGuid, LPVOID* lplpAddrBuf,
                                    LPDWORD lpdwBufSize )
{
  DPCOMPOUNDADDRESSELEMENT dpCompoundAddress;
  HRESULT                  hr;

  dpCompoundAddress.dwDataSize = sizeof( GUID );
  memcpy( &dpCompoundAddress.guidDataType, &DPAID_ServiceProvider,
          sizeof( GUID ) ) ;
  dpCompoundAddress.lpData = lpcSpGuid;

  *lplpAddrBuf = NULL;
  *lpdwBufSize = 0;

  hr = DPL_CreateCompoundAddress( &dpCompoundAddress, 1, *lplpAddrBuf,
                                  lpdwBufSize, TRUE );

  if( hr != DPERR_BUFFERTOOSMALL )
  {
    ERR( "can't get buffer size: %s\n", DPLAYX_HresultToString( hr ) );
    return FALSE;
  }

  /* Now allocate the buffer */
  *lplpAddrBuf = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                            *lpdwBufSize );

  hr = DPL_CreateCompoundAddress( &dpCompoundAddress, 1, *lplpAddrBuf,
                                  lpdwBufSize, TRUE );
  if( FAILED(hr) )
  {
    ERR( "can't create address: %s\n", DPLAYX_HresultToString( hr ) );
    return FALSE;
  }

  return TRUE;
}

static HRESULT WINAPI DirectPlay3AImpl_EnumConnections
          ( LPDIRECTPLAY3A iface, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  TRACE("(%p)->(%p,%p,%p,0x%08lx)\n", This, lpguidApplication, lpEnumCallback, lpContext, dwFlags );

  /* A default dwFlags (0) is backwards compatible -- DPCONNECTION_DIRECTPLAY */
  if( dwFlags == 0 )
  {
    dwFlags = DPCONNECTION_DIRECTPLAY;
  }

  if( ! ( ( dwFlags & DPCONNECTION_DIRECTPLAY ) ||
          ( dwFlags & DPCONNECTION_DIRECTPLAYLOBBY ) )
    )
  {
    return DPERR_INVALIDFLAGS;
  }

  if( !lpEnumCallback || !*lpEnumCallback )
  {
     return DPERR_INVALIDPARAMS;
  }

  /* Enumerate DirectPlay service providers */
  if( dwFlags & DPCONNECTION_DIRECTPLAY )
  {
    HKEY hkResult;
    LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
    LPCSTR guidDataSubKey  = "Guid";
    char subKeyName[51];
    DWORD dwIndex, sizeOfSubKeyName=50;
    FILETIME filetime;

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
         ++dwIndex, sizeOfSubKeyName=51 )
    {

      HKEY     hkServiceProvider;
      GUID     serviceProviderGUID;
      DWORD    returnTypeGUID, sizeOfReturnBuffer = 50;
      char     returnBuffer[51];
      WCHAR    buff[51];
      DPNAME   dpName;
      BOOL     bBuildPass;

      LPVOID                   lpAddressBuffer = NULL;
      DWORD                    dwAddressBufferSize = 0;

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

      /* Fill in the DPNAME struct for the service provider */
      dpName.dwSize             = sizeof( dpName );
      dpName.dwFlags            = 0;
      dpName.lpszShortNameA = subKeyName;
      dpName.lpszLongNameA  = NULL;

      /* Create the compound address for the service provider.
       * NOTE: This is a gruesome architectural scar right now.  DP
       * uses DPL and DPL uses DP.  Nasty stuff. This may be why the
       * native dll just gets around this little bit by allocating an
       * 80 byte buffer which isn't even filled with a valid compound
       * address. Oh well. Creating a proper compound address is the
       * way to go anyways despite this method taking slightly more
       * heap space and realtime :) */

      bBuildPass = DP_BuildSPCompoundAddr( &serviceProviderGUID,
                                           &lpAddressBuffer,
                                           &dwAddressBufferSize );
      if( !bBuildPass )
      {
        ERR( "Can't build compound addr\n" );
        return DPERR_GENERIC;
      }

      /* The enumeration will return FALSE if we are not to continue */
      if( !lpEnumCallback( &serviceProviderGUID, lpAddressBuffer, dwAddressBufferSize,
                           &dpName, DPCONNECTION_DIRECTPLAY, lpContext ) )
      {
         return DP_OK;
      }
    }
  }

  /* Enumerate DirectPlayLobby service providers */
  if( dwFlags & DPCONNECTION_DIRECTPLAYLOBBY )
  {
    HKEY hkResult;
    LPCSTR searchSubKey    = "SOFTWARE\\Microsoft\\DirectPlay\\Lobby Providers";
    LPCSTR guidDataSubKey  = "Guid";
    char subKeyName[51];
    DWORD dwIndex, sizeOfSubKeyName=50;
    FILETIME filetime;

    /* Need to loop over the service providers in the registry */
    if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                         0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
    {
      /* Hmmm. Does this mean that there are no service providers? */
      ERR(": no service providers?\n");
      return DP_OK;
    }


    /* Traverse all the lobby providers we have available */
    for( dwIndex=0;
         RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName,
                        NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
         ++dwIndex, sizeOfSubKeyName=51 )
    {

      HKEY     hkServiceProvider;
      GUID     serviceProviderGUID;
      DWORD    returnTypeGUID, sizeOfReturnBuffer = 50;
      char     returnBuffer[51];
      WCHAR    buff[51];
      DPNAME   dpName;
      HRESULT  hr;

      DPCOMPOUNDADDRESSELEMENT dpCompoundAddress;
      LPVOID                   lpAddressBuffer = NULL;
      DWORD                    dwAddressBufferSize = 0;

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

      /* Fill in the DPNAME struct for the service provider */
      dpName.dwSize             = sizeof( dpName );
      dpName.dwFlags            = 0;
      dpName.lpszShortNameA = subKeyName;
      dpName.lpszLongNameA  = NULL;

      /* Create the compound address for the service provider.
         NOTE: This is a gruesome architectural scar right now. DP uses DPL and DPL uses DP
               nast stuff. This may be why the native dll just gets around this little bit by
               allocating an 80 byte buffer which isn't even a filled with a valid compound
               address. Oh well. Creating a proper compound address is the way to go anyways
               despite this method taking slightly more heap space and realtime :) */

      dpCompoundAddress.guidDataType = DPAID_LobbyProvider;
      dpCompoundAddress.dwDataSize   = sizeof( GUID );
      dpCompoundAddress.lpData       = &serviceProviderGUID;

      if( ( hr = DPL_CreateCompoundAddress( &dpCompoundAddress, 1, lpAddressBuffer,
                                     &dwAddressBufferSize, TRUE ) ) != DPERR_BUFFERTOOSMALL )
      {
        ERR( "can't get buffer size: %s\n", DPLAYX_HresultToString( hr ) );
        return hr;
      }

      /* Now allocate the buffer */
      lpAddressBuffer = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwAddressBufferSize );

      if( ( hr = DPL_CreateCompoundAddress( &dpCompoundAddress, 1, lpAddressBuffer,
                                     &dwAddressBufferSize, TRUE ) ) != DP_OK )
      {
        ERR( "can't create address: %s\n", DPLAYX_HresultToString( hr ) );
        return hr;
      }

      /* The enumeration will return FALSE if we are not to continue */
      if( !lpEnumCallback( &serviceProviderGUID, lpAddressBuffer, dwAddressBufferSize,
                           &dpName, DPCONNECTION_DIRECTPLAYLOBBY, lpContext ) )
      {
         return DP_OK;
      }
    }
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_EnumConnections
          ( LPDIRECTPLAY3 iface, LPCGUID lpguidApplication, LPDPENUMCONNECTIONSCALLBACK lpEnumCallback, LPVOID lpContext, DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(%p,%p,%p,0x%08lx): stub\n", This, lpguidApplication, lpEnumCallback, lpContext, dwFlags );
  return DP_OK;
}

static HRESULT WINAPI DP_IF_EnumGroupsInGroup
          ( IDirectPlay3AImpl* This, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2,
            LPVOID lpContext, DWORD dwFlags, BOOL bAnsi )
{
  lpGroupList lpGList;
  lpGroupData lpGData;

  FIXME( "(%p)->(0x%08lx,%p,%p,%p,0x%08lx,%u): semi stub\n",
         This, idGroup, lpguidInstance, lpEnumPlayersCallback2,
         lpContext, dwFlags, bAnsi );

  if( ( lpGData = DP_FindAnyGroup( (IDirectPlay2AImpl*)This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  if( DPQ_IS_EMPTY( lpGData->groups ) )
  {
    return DP_OK;
  }

  lpGList = DPQ_FIRST( lpGData->groups );

  for( ;; )
  {
    /* FIXME: Should check dwFlags for match here */

    if( !(*lpEnumPlayersCallback2)( lpGList->lpGData->dpid, DPPLAYERTYPE_GROUP,
                                    &lpGList->lpGData->name, dwFlags,
                                    lpContext ) )
    {
      return DP_OK; /* User requested break */
    }

    if( DPQ_IS_ENDOFLIST( lpGList->groups ) )
    {
      break;
    }

    lpGList = DPQ_NEXT( lpGList->groups );

  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_EnumGroupsInGroup
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext,
            DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_EnumGroupsInGroup( This, idGroup, lpguidInstance,
                                  lpEnumPlayersCallback2, lpContext, dwFlags,
                                  TRUE );
}

static HRESULT WINAPI DirectPlay3WImpl_EnumGroupsInGroup
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPGUID lpguidInstance,
            LPDPENUMPLAYERSCALLBACK2 lpEnumPlayersCallback2, LPVOID lpContext,
            DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_EnumGroupsInGroup( This, idGroup, lpguidInstance,
                                  lpEnumPlayersCallback2, lpContext, dwFlags,
                                  FALSE );
}

static HRESULT WINAPI DirectPlay3AImpl_GetGroupConnectionSettings
          ( LPDIRECTPLAY3A iface, DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, dwFlags, idGroup, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetGroupConnectionSettings
          ( LPDIRECTPLAY3 iface, DWORD dwFlags, DPID idGroup, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, dwFlags, idGroup, lpData, lpdwDataSize );
  return DP_OK;
}

static BOOL CALLBACK DP_GetSpLpGuidFromCompoundAddress(
    REFGUID         guidDataType,
    DWORD           dwDataSize,
    LPCVOID         lpData,
    LPVOID          lpContext )
{
  /* Looking for the GUID of the provider to load */
  if( ( IsEqualGUID( guidDataType, &DPAID_ServiceProvider ) ) ||
      ( IsEqualGUID( guidDataType, &DPAID_LobbyProvider ) )
    )
  {
    TRACE( "Found SP/LP (%s) %s (data size = 0x%08lx)\n",
           debugstr_guid( guidDataType ), debugstr_guid( lpData ), dwDataSize );

    if( dwDataSize != sizeof( GUID ) )
    {
      ERR( "Invalid sp/lp guid size 0x%08lx\n", dwDataSize );
    }

    memcpy( lpContext, lpData, dwDataSize );

    /* There shouldn't be more than 1 GUID/compound address */
    return FALSE;
  }

  /* Still waiting for what we want */
  return TRUE;
}


/* Find and perform a LoadLibrary on the requested SP or LP GUID */
static HMODULE DP_LoadSP( LPCGUID lpcGuid, LPSPINITDATA lpSpData, LPBOOL lpbIsDpSp )
{
  UINT i;
  LPCSTR spSubKey         = "SOFTWARE\\Microsoft\\DirectPlay\\Service Providers";
  LPCSTR lpSubKey         = "SOFTWARE\\Microsoft\\DirectPlay\\Lobby Providers";
  LPCSTR guidDataSubKey   = "Guid";
  LPCSTR majVerDataSubKey = "dwReserved1";
  LPCSTR minVerDataSubKey = "dwReserved2";
  LPCSTR pathSubKey       = "Path";

  TRACE( " request to load %s\n", debugstr_guid( lpcGuid ) );

  /* FIXME: Cloned code with a quick hack. */
  for( i=0; i<2; i++ )
  {
    HKEY hkResult;
    LPCSTR searchSubKey;
    char subKeyName[51];
    DWORD dwIndex, sizeOfSubKeyName=50;
    FILETIME filetime;

    (i == 0) ? (searchSubKey = spSubKey ) : (searchSubKey = lpSubKey );
    *lpbIsDpSp = (i == 0) ? TRUE : FALSE;


    /* Need to loop over the service providers in the registry */
    if( RegOpenKeyExA( HKEY_LOCAL_MACHINE, searchSubKey,
                         0, KEY_READ, &hkResult ) != ERROR_SUCCESS )
    {
      /* Hmmm. Does this mean that there are no service providers? */
      ERR(": no service providers?\n");
      return 0;
    }

    /* Traverse all the service providers we have available */
    for( dwIndex=0;
         RegEnumKeyExA( hkResult, dwIndex, subKeyName, &sizeOfSubKeyName,
                        NULL, NULL, NULL, &filetime ) != ERROR_NO_MORE_ITEMS;
         ++dwIndex, sizeOfSubKeyName=51 )
    {

      HKEY     hkServiceProvider;
      GUID     serviceProviderGUID;
      DWORD    returnType, sizeOfReturnBuffer = 255;
      char     returnBuffer[256];
      WCHAR    buff[51];
      DWORD    dwTemp, len;

      TRACE(" this time through: %s\n", subKeyName );

      /* Get a handle for this particular service provider */
      if( RegOpenKeyExA( hkResult, subKeyName, 0, KEY_READ,
                         &hkServiceProvider ) != ERROR_SUCCESS )
      {
         ERR(": what the heck is going on?\n" );
         continue;
      }

      if( RegQueryValueExA( hkServiceProvider, guidDataSubKey,
                            NULL, &returnType, (LPBYTE)returnBuffer,
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
      if( !IsEqualGUID( &serviceProviderGUID, lpcGuid ) )
      {
        continue;
      }

      if( i == 0 ) /* DP SP */
      {
        len = MultiByteToWideChar( CP_ACP, 0, subKeyName, -1, NULL, 0 );
        lpSpData->lpszName = HeapAlloc( GetProcessHeap(), 0, len*sizeof(WCHAR) );
        MultiByteToWideChar( CP_ACP, 0, subKeyName, -1, lpSpData->lpszName, len );
      }

      sizeOfReturnBuffer = 255;

      /* Get dwReserved1 */
      if( RegQueryValueExA( hkServiceProvider, majVerDataSubKey,
                            NULL, &returnType, (LPBYTE)returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
      {
         ERR(": missing dwReserved1 registry data members\n") ;
         continue;
      }

      if( i == 0 )
          memcpy( &lpSpData->dwReserved1, returnBuffer, sizeof(lpSpData->dwReserved1) );

      sizeOfReturnBuffer = 255;

      /* Get dwReserved2 */
      if( RegQueryValueExA( hkServiceProvider, minVerDataSubKey,
                            NULL, &returnType, (LPBYTE)returnBuffer,
                            &sizeOfReturnBuffer ) != ERROR_SUCCESS )
      {
         ERR(": missing dwReserved1 registry data members\n") ;
         continue;
      }

      if( i == 0 )
          memcpy( &lpSpData->dwReserved2, returnBuffer, sizeof(lpSpData->dwReserved2) );

      sizeOfReturnBuffer = 255;

      /* Get the path for this service provider */
      if( ( dwTemp = RegQueryValueExA( hkServiceProvider, pathSubKey,
                            NULL, NULL, (LPBYTE)returnBuffer,
                            &sizeOfReturnBuffer ) ) != ERROR_SUCCESS )
      {
        ERR(": missing PATH registry data members: 0x%08lx\n", dwTemp );
        continue;
      }

      TRACE( "Loading %s\n", returnBuffer );
      return LoadLibraryA( returnBuffer );
    }
  }

  return 0;
}

static
HRESULT DP_InitializeDPSP( IDirectPlay3Impl* This, HMODULE hServiceProvider )
{
  HRESULT hr;
  LPDPSP_SPINIT SPInit;

  /* Initialize the service provider by calling SPInit */
  SPInit = (LPDPSP_SPINIT)GetProcAddress( hServiceProvider, "SPInit" );

  if( SPInit == NULL )
  {
    ERR( "Service provider doesn't provide SPInit interface?\n" );
    FreeLibrary( hServiceProvider );
    return DPERR_UNAVAILABLE;
  }

  TRACE( "Calling SPInit (DP SP entry point)\n" );

  hr = (*SPInit)( &This->dp2->spData );

  if( FAILED(hr) )
  {
    ERR( "DP SP Initialization failed: %s\n", DPLAYX_HresultToString(hr) );
    FreeLibrary( hServiceProvider );
    return hr;
  }

  /* FIXME: Need to verify the sanity of the returned callback table
   *        using IsBadCodePtr */
  This->dp2->bSPInitialized = TRUE;

  /* This interface is now initialized as a DP object */
  This->dp2->connectionInitialized = DP_SERVICE_PROVIDER;

  /* Store the handle of the module so that we can unload it later */
  This->dp2->hServiceProvider = hServiceProvider;

  return hr;
}

static
HRESULT DP_InitializeDPLSP( IDirectPlay3Impl* This, HMODULE hLobbyProvider )
{
  HRESULT hr;
  LPSP_INIT DPLSPInit;

  /* Initialize the service provider by calling SPInit */
  DPLSPInit = (LPSP_INIT)GetProcAddress( hLobbyProvider, "DPLSPInit" );

  if( DPLSPInit == NULL )
  {
    ERR( "Service provider doesn't provide DPLSPInit interface?\n" );
    FreeLibrary( hLobbyProvider );
    return DPERR_UNAVAILABLE;
  }

  TRACE( "Calling DPLSPInit (DPL SP entry point)\n" );

  hr = (*DPLSPInit)( &This->dp2->dplspData );

  if( FAILED(hr) )
  {
    ERR( "DPL SP Initialization failed: %s\n", DPLAYX_HresultToString(hr) );
    FreeLibrary( hLobbyProvider );
    return hr;
  }

  /* FIXME: Need to verify the sanity of the returned callback table
   *        using IsBadCodePtr */

  This->dp2->bDPLSPInitialized = TRUE;

  /* This interface is now initialized as a lobby object */
  This->dp2->connectionInitialized = DP_LOBBY_PROVIDER;

  /* Store the handle of the module so that we can unload it later */
  This->dp2->hDPLobbyProvider = hLobbyProvider;

  return hr;
}

static HRESULT WINAPI DP_IF_InitializeConnection
          ( IDirectPlay3Impl* This, LPVOID lpConnection, DWORD dwFlags, BOOL bAnsi )
{
  HMODULE hServiceProvider;
  HRESULT hr;
  GUID guidSP;
  const DWORD dwAddrSize = 80; /* FIXME: Need to calculate it correctly */
  BOOL bIsDpSp; /* TRUE if Direct Play SP, FALSE if Direct Play Lobby SP */

  TRACE("(%p)->(%p,0x%08lx,%u)\n", This, lpConnection, dwFlags, bAnsi );

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDFLAGS;
  }

  /* Find out what the requested SP is and how large this buffer is */
  hr = DPL_EnumAddress( DP_GetSpLpGuidFromCompoundAddress, lpConnection,
                        dwAddrSize, &guidSP );

  if( FAILED(hr) )
  {
    ERR( "Invalid compound address?\n" );
    return DPERR_UNAVAILABLE;
  }

  /* Load the service provider */
  hServiceProvider = DP_LoadSP( &guidSP, &This->dp2->spData, &bIsDpSp );

  if( hServiceProvider == 0 )
  {
    ERR( "Unable to load service provider\n" );
    return DPERR_UNAVAILABLE;
  }

  if( bIsDpSp )
  {
     /* Fill in what we can of the Service Provider required information.
      * The rest was be done in DP_LoadSP
      */
     This->dp2->spData.lpAddress = lpConnection;
     This->dp2->spData.dwAddressSize = dwAddrSize;
     This->dp2->spData.lpGuid = &guidSP;

     hr = DP_InitializeDPSP( This, hServiceProvider );
  }
  else
  {
     This->dp2->dplspData.lpAddress = lpConnection;

     hr = DP_InitializeDPLSP( This, hServiceProvider );
  }

  if( FAILED(hr) )
  {
    return hr;
  }

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_InitializeConnection
          ( LPDIRECTPLAY3A iface, LPVOID lpConnection, DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;

  /* This may not be externally invoked once either an SP or LP is initialized */
  if( This->dp2->connectionInitialized != NO_PROVIDER )
  {
    return DPERR_ALREADYINITIALIZED;
  }

  return DP_IF_InitializeConnection( This, lpConnection, dwFlags, TRUE );
}

static HRESULT WINAPI DirectPlay3WImpl_InitializeConnection
          ( LPDIRECTPLAY3 iface, LPVOID lpConnection, DWORD dwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;

  /* This may not be externally invoked once either an SP or LP is initialized */
  if( This->dp2->connectionInitialized != NO_PROVIDER )
  {
    return DPERR_ALREADYINITIALIZED;
  }

  return DP_IF_InitializeConnection( This, lpConnection, dwFlags, FALSE );
}

static HRESULT WINAPI DirectPlay3AImpl_SecureOpen
          ( LPDIRECTPLAY3A iface, LPCDPSESSIONDESC2 lpsd, DWORD dwFlags,
            LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface; /* Yes a dp 2 interface */
  return DP_SecureOpen( This, lpsd, dwFlags, lpSecurity, lpCredentials, TRUE );
}

static HRESULT WINAPI DirectPlay3WImpl_SecureOpen
          ( LPDIRECTPLAY3 iface, LPCDPSESSIONDESC2 lpsd, DWORD dwFlags,
            LPCDPSECURITYDESC lpSecurity, LPCDPCREDENTIALS lpCredentials )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface; /* Yes a dp 2 interface */
  return DP_SecureOpen( This, lpsd, dwFlags, lpSecurity, lpCredentials, FALSE );
}

static HRESULT WINAPI DirectPlay3AImpl_SendChatMessage
          ( LPDIRECTPLAY3A iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub\n", This, idFrom, idTo, dwFlags, lpChatMessage );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_SendChatMessage
          ( LPDIRECTPLAY3 iface, DPID idFrom, DPID idTo, DWORD dwFlags, LPDPCHAT lpChatMessage )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,0x%08lx,%p): stub\n", This, idFrom, idTo, dwFlags, lpChatMessage );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_SetGroupConnectionSettings
          ( LPDIRECTPLAY3A iface, DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,%p): stub\n", This, dwFlags, idGroup, lpConnection );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_SetGroupConnectionSettings
          ( LPDIRECTPLAY3 iface, DWORD dwFlags, DPID idGroup, LPDPLCONNECTION lpConnection )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,%p): stub\n", This, dwFlags, idGroup, lpConnection );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_StartSession
          ( LPDIRECTPLAY3A iface, DWORD dwFlags, DPID idGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, dwFlags, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_StartSession
          ( LPDIRECTPLAY3 iface, DWORD dwFlags, DPID idGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, dwFlags, idGroup );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetGroupFlags
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPDWORD lpdwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetGroupFlags
          ( LPDIRECTPLAY3 iface, DPID idGroup, LPDWORD lpdwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DP_IF_GetGroupParent
          ( IDirectPlay3AImpl* This, DPID idGroup, LPDPID lpidGroup,
            BOOL bAnsi )
{
  lpGroupData lpGData;

  TRACE("(%p)->(0x%08lx,%p,%u)\n", This, idGroup, lpidGroup, bAnsi );

  if( ( lpGData = DP_FindAnyGroup( (IDirectPlay2AImpl*)This, idGroup ) ) == NULL )
  {
    return DPERR_INVALIDGROUP;
  }

  *lpidGroup = lpGData->dpid;

  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetGroupParent
          ( LPDIRECTPLAY3A iface, DPID idGroup, LPDPID lpidGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_GetGroupParent( This, idGroup, lpidGroup, TRUE );
}
static HRESULT WINAPI DirectPlay3WImpl_GetGroupParent
          ( LPDIRECTPLAY3 iface, DPID idGroup, LPDPID lpidGroup )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  return DP_IF_GetGroupParent( This, idGroup, lpidGroup, FALSE );
}

static HRESULT WINAPI DirectPlay3AImpl_GetPlayerAccount
          ( LPDIRECTPLAY3A iface, DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, idPlayer, dwFlags, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetPlayerAccount
          ( LPDIRECTPLAY3 iface, DPID idPlayer, DWORD dwFlags, LPVOID lpData, LPDWORD lpdwDataSize )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx,%p,%p): stub\n", This, idPlayer, dwFlags, lpData, lpdwDataSize );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3AImpl_GetPlayerFlags
          ( LPDIRECTPLAY3A iface, DPID idPlayer, LPDWORD lpdwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idPlayer, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay3WImpl_GetPlayerFlags
          ( LPDIRECTPLAY3 iface, DPID idPlayer, LPDWORD lpdwFlags )
{
  IDirectPlay3Impl *This = (IDirectPlay3Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idPlayer, lpdwFlags );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_GetGroupOwner
          ( LPDIRECTPLAY4A iface, DPID idGroup, LPDPID lpidGroupOwner )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpidGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_GetGroupOwner
          ( LPDIRECTPLAY4 iface, DPID idGroup, LPDPID lpidGroupOwner )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;
  FIXME("(%p)->(0x%08lx,%p): stub\n", This, idGroup, lpidGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4AImpl_SetGroupOwner
          ( LPDIRECTPLAY4A iface, DPID idGroup , DPID idGroupOwner )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DirectPlay4WImpl_SetGroupOwner
          ( LPDIRECTPLAY4 iface, DPID idGroup , DPID idGroupOwner )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;
  FIXME("(%p)->(0x%08lx,0x%08lx): stub\n", This, idGroup, idGroupOwner );
  return DP_OK;
}

static HRESULT WINAPI DP_SendEx
          ( IDirectPlay2Impl* This, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout,
            LPVOID lpContext, LPDWORD lpdwMsgID, BOOL bAnsi )
{
  lpPlayerList lpPList;
  lpGroupData  lpGData;
  BOOL         bValidDestination = FALSE;

  FIXME( "(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,0x%08lx,0x%08lx,0x%08lx,%p,%p,%u)"
         ": stub\n",
         This, idFrom, idTo, dwFlags, lpData, dwDataSize, dwPriority,
         dwTimeout, lpContext, lpdwMsgID, bAnsi );

  /* FIXME: Add parameter checking */
  /* FIXME: First call to this needs to aquire a message id which will be
   *        used for multiple sends
   */

  /* NOTE: Can't send messages to yourself - this will be trapped in receive */

  /* Verify that the message is being sent from a valid local player. The
   * from player may be anonymous DPID_UNKNOWN
   */
  if( idFrom != DPID_UNKNOWN )
  {
    if( ( lpPList = DP_FindPlayer( This, idFrom ) ) == NULL )
    {
      WARN( "INFO: Invalid from player 0x%08lx\n", idFrom );
      return DPERR_INVALIDPLAYER;
    }
  }

  /* Verify that the message is being sent to a valid player, group or to
   * everyone. If it's valid, send it to those players.
   */
  if( idTo == DPID_ALLPLAYERS )
  {
    bValidDestination = TRUE;

    /* See if SP has the ability to multicast. If so, use it */
    if( This->dp2->spData.lpCB->SendToGroupEx )
    {
      FIXME( "Use group sendex to group 0\n" );
    }
    else if( This->dp2->spData.lpCB->SendToGroup ) /* obsolete interface */
    {
      FIXME( "Use obsolete group send to group 0\n" );
    }
    else /* No multicast, multiplicate */
    {
      /* Send to all players we know about */
      FIXME( "Send to all players using EnumPlayersInGroup\n" );
    }
  }

  if( ( !bValidDestination ) &&
      ( DP_FindPlayer( This, idTo ) != NULL )
    )
  {
    bValidDestination = TRUE;

    /* Have the service provider send this message */
    /* FIXME: Could optimize for local interface sends */
    return DP_SP_SendEx( This, dwFlags, lpData, dwDataSize, dwPriority,
                         dwTimeout, lpContext, lpdwMsgID );
  }

  if( ( !bValidDestination ) &&
      ( ( lpGData = DP_FindAnyGroup( This, idTo ) ) != NULL )
    )
  {
    bValidDestination = TRUE;

    /* See if SP has the ability to multicast. If so, use it */
    if( This->dp2->spData.lpCB->SendToGroupEx )
    {
      FIXME( "Use group sendex\n" );
    }
    else if( This->dp2->spData.lpCB->SendToGroup ) /* obsolete interface */
    {
      FIXME( "Use obsolete group send to group\n" );
    }
    else /* No multicast, multiplicate */
    {
      FIXME( "Send to all players using EnumPlayersInGroup\n" );
    }

#if 0
    if( bExpectReply )
    {
      DWORD dwWaitReturn;

      This->dp2->hReplyEvent = CreateEventW( NULL, FALSE, FALSE, NULL );

      dwWaitReturn = WaitForSingleObject( hReplyEvent, dwTimeout );
      if( dwWaitReturn != WAIT_OBJECT_0 )
      {
        ERR( "Wait failed 0x%08lx\n", dwWaitReturn );
      }
    }
#endif
  }

  if( !bValidDestination )
  {
    return DPERR_INVALIDPLAYER;
  }
  else
  {
    /* FIXME: Should return what the send returned */
    return DP_OK;
  }
}


static HRESULT WINAPI DirectPlay4AImpl_SendEx
          ( LPDIRECTPLAY4A iface, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout,
            LPVOID lpContext, LPDWORD lpdwMsgID )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface; /* yes downcast to 2 */
  return DP_SendEx( This, idFrom, idTo, dwFlags, lpData, dwDataSize,
                    dwPriority, dwTimeout, lpContext, lpdwMsgID, TRUE );
}

static HRESULT WINAPI DirectPlay4WImpl_SendEx
          ( LPDIRECTPLAY4 iface, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout,
            LPVOID lpContext, LPDWORD lpdwMsgID )
{
  IDirectPlay2Impl *This = (IDirectPlay2Impl *)iface; /* yes downcast to 2 */
  return DP_SendEx( This, idFrom, idTo, dwFlags, lpData, dwDataSize,
                    dwPriority, dwTimeout, lpContext, lpdwMsgID, FALSE );
}

static HRESULT WINAPI DP_SP_SendEx
          ( IDirectPlay2Impl* This, DWORD dwFlags,
            LPVOID lpData, DWORD dwDataSize, DWORD dwPriority, DWORD dwTimeout,
            LPVOID lpContext, LPDWORD lpdwMsgID )
{
  LPDPMSG lpMElem;

  FIXME( ": stub\n" );

  /* FIXME: This queuing should only be for async messages */

  lpMElem = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpMElem ) );
  lpMElem->msg = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwDataSize );

  CopyMemory( lpMElem->msg, lpData, dwDataSize );

  /* FIXME: Need to queue based on priority */
  DPQ_INSERT( This->dp2->sendMsgs, lpMElem, msgs );

  return DP_OK;
}

static HRESULT WINAPI DP_IF_GetMessageQueue
          ( IDirectPlay4Impl* This, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes, BOOL bAnsi )
{
  HRESULT hr = DP_OK;

  FIXME( "(%p)->(0x%08lx,0x%08lx,0x%08lx,%p,%p,%u): semi stub\n",
         This, idFrom, idTo, dwFlags, lpdwNumMsgs, lpdwNumBytes, bAnsi );

  /* FIXME: Do we need to do idFrom and idTo sanity checking here? */
  /* FIXME: What about sends which are not immediate? */

  if( This->dp2->spData.lpCB->GetMessageQueue )
  {
    DPSP_GETMESSAGEQUEUEDATA data;

    FIXME( "Calling SP GetMessageQueue - is it right?\n" );

    /* FIXME: None of this is documented :( */

    data.lpISP        = This->dp2->spData.lpISP;
    data.dwFlags      = dwFlags;
    data.idFrom       = idFrom;
    data.idTo         = idTo;
    data.lpdwNumMsgs  = lpdwNumMsgs;
    data.lpdwNumBytes = lpdwNumBytes;

    hr = (*This->dp2->spData.lpCB->GetMessageQueue)( &data );
  }
  else
  {
    FIXME( "No SP for GetMessageQueue - fake some data\n" );
  }

  return hr;
}

static HRESULT WINAPI DirectPlay4AImpl_GetMessageQueue
          ( LPDIRECTPLAY4A iface, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;
  return DP_IF_GetMessageQueue( This, idFrom, idTo, dwFlags, lpdwNumMsgs,
                                lpdwNumBytes, TRUE );
}

static HRESULT WINAPI DirectPlay4WImpl_GetMessageQueue
          ( LPDIRECTPLAY4 iface, DPID idFrom, DPID idTo, DWORD dwFlags,
            LPDWORD lpdwNumMsgs, LPDWORD lpdwNumBytes )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;
  return DP_IF_GetMessageQueue( This, idFrom, idTo, dwFlags, lpdwNumMsgs,
                                lpdwNumBytes, FALSE );
}

static HRESULT WINAPI DP_IF_CancelMessage
          ( IDirectPlay4Impl* This, DWORD dwMsgID, DWORD dwFlags,
            DWORD dwMinPriority, DWORD dwMaxPriority, BOOL bAnsi )
{
  HRESULT hr = DP_OK;

  FIXME( "(%p)->(0x%08lx,0x%08lx,%u): semi stub\n",
         This, dwMsgID, dwFlags, bAnsi );

  if( This->dp2->spData.lpCB->Cancel )
  {
    DPSP_CANCELDATA data;

    TRACE( "Calling SP Cancel\n" );

    /* FIXME: Undocumented callback */

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = dwFlags;
    data.lprglpvSPMsgID = NULL;
    data.cSPMsgID       = dwMsgID;
    data.dwMinPriority  = dwMinPriority;
    data.dwMaxPriority  = dwMaxPriority;

    hr = (*This->dp2->spData.lpCB->Cancel)( &data );
  }
  else
  {
    FIXME( "SP doesn't implement Cancel\n" );
  }

  return hr;
}

static HRESULT WINAPI DirectPlay4AImpl_CancelMessage
          ( LPDIRECTPLAY4A iface, DWORD dwMsgID, DWORD dwFlags )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDFLAGS;
  }

  if( dwMsgID == 0 )
  {
    dwFlags |= DPCANCELSEND_ALL;
  }

  return DP_IF_CancelMessage( This, dwMsgID, dwFlags, 0, 0, TRUE );
}

static HRESULT WINAPI DirectPlay4WImpl_CancelMessage
          ( LPDIRECTPLAY4 iface, DWORD dwMsgID, DWORD dwFlags )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDFLAGS;
  }

  if( dwMsgID == 0 )
  {
    dwFlags |= DPCANCELSEND_ALL;
  }

  return DP_IF_CancelMessage( This, dwMsgID, dwFlags, 0, 0, FALSE );
}

static HRESULT WINAPI DirectPlay4AImpl_CancelPriority
          ( LPDIRECTPLAY4A iface, DWORD dwMinPriority, DWORD dwMaxPriority,
            DWORD dwFlags )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDFLAGS;
  }

  return DP_IF_CancelMessage( This, 0, DPCANCELSEND_PRIORITY, dwMinPriority,
                              dwMaxPriority, TRUE );
}

static HRESULT WINAPI DirectPlay4WImpl_CancelPriority
          ( LPDIRECTPLAY4 iface, DWORD dwMinPriority, DWORD dwMaxPriority,
            DWORD dwFlags )
{
  IDirectPlay4Impl *This = (IDirectPlay4Impl *)iface;

  if( dwFlags != 0 )
  {
    return DPERR_INVALIDFLAGS;
  }

  return DP_IF_CancelMessage( This, 0, DPCANCELSEND_PRIORITY, dwMinPriority,
                              dwMaxPriority, FALSE );
}

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay2WVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirectPlay2Vtbl directPlay2WVT =
{
  XCAST(QueryInterface)DP_QueryInterface,
  XCAST(AddRef)DP_AddRef,
  XCAST(Release)DP_Release,

  DirectPlay2WImpl_AddPlayerToGroup,
  DirectPlay2WImpl_Close,
  DirectPlay2WImpl_CreateGroup,
  DirectPlay2WImpl_CreatePlayer,
  DirectPlay2WImpl_DeletePlayerFromGroup,
  DirectPlay2WImpl_DestroyGroup,
  DirectPlay2WImpl_DestroyPlayer,
  DirectPlay2WImpl_EnumGroupPlayers,
  DirectPlay2WImpl_EnumGroups,
  DirectPlay2WImpl_EnumPlayers,
  DirectPlay2WImpl_EnumSessions,
  DirectPlay2WImpl_GetCaps,
  DirectPlay2WImpl_GetGroupData,
  DirectPlay2WImpl_GetGroupName,
  DirectPlay2WImpl_GetMessageCount,
  DirectPlay2WImpl_GetPlayerAddress,
  DirectPlay2WImpl_GetPlayerCaps,
  DirectPlay2WImpl_GetPlayerData,
  DirectPlay2WImpl_GetPlayerName,
  DirectPlay2WImpl_GetSessionDesc,
  DirectPlay2WImpl_Initialize,
  DirectPlay2WImpl_Open,
  DirectPlay2WImpl_Receive,
  DirectPlay2WImpl_Send,
  DirectPlay2WImpl_SetGroupData,
  DirectPlay2WImpl_SetGroupName,
  DirectPlay2WImpl_SetPlayerData,
  DirectPlay2WImpl_SetPlayerName,
  DirectPlay2WImpl_SetSessionDesc
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay2AVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirectPlay2Vtbl directPlay2AVT =
{
  XCAST(QueryInterface)DP_QueryInterface,
  XCAST(AddRef)DP_AddRef,
  XCAST(Release)DP_Release,

  DirectPlay2AImpl_AddPlayerToGroup,
  DirectPlay2AImpl_Close,
  DirectPlay2AImpl_CreateGroup,
  DirectPlay2AImpl_CreatePlayer,
  DirectPlay2AImpl_DeletePlayerFromGroup,
  DirectPlay2AImpl_DestroyGroup,
  DirectPlay2AImpl_DestroyPlayer,
  DirectPlay2AImpl_EnumGroupPlayers,
  DirectPlay2AImpl_EnumGroups,
  DirectPlay2AImpl_EnumPlayers,
  DirectPlay2AImpl_EnumSessions,
  DirectPlay2AImpl_GetCaps,
  DirectPlay2AImpl_GetGroupData,
  DirectPlay2AImpl_GetGroupName,
  DirectPlay2AImpl_GetMessageCount,
  DirectPlay2AImpl_GetPlayerAddress,
  DirectPlay2AImpl_GetPlayerCaps,
  DirectPlay2AImpl_GetPlayerData,
  DirectPlay2AImpl_GetPlayerName,
  DirectPlay2AImpl_GetSessionDesc,
  DirectPlay2AImpl_Initialize,
  DirectPlay2AImpl_Open,
  DirectPlay2AImpl_Receive,
  DirectPlay2AImpl_Send,
  DirectPlay2AImpl_SetGroupData,
  DirectPlay2AImpl_SetGroupName,
  DirectPlay2AImpl_SetPlayerData,
  DirectPlay2AImpl_SetPlayerName,
  DirectPlay2AImpl_SetSessionDesc
};
#undef XCAST


/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay3AVT.fun))
#else
# define XCAST(fun)     (void*)
#endif

static const IDirectPlay3Vtbl directPlay3AVT =
{
  XCAST(QueryInterface)DP_QueryInterface,
  XCAST(AddRef)DP_AddRef,
  XCAST(Release)DP_Release,

  XCAST(AddPlayerToGroup)DirectPlay2AImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2AImpl_Close,
  XCAST(CreateGroup)DirectPlay2AImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2AImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2AImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2AImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2AImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2AImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2AImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2AImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2AImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2AImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2AImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2AImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2AImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2AImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2AImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2AImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2AImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2AImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2AImpl_Initialize,
  XCAST(Open)DirectPlay2AImpl_Open,
  XCAST(Receive)DirectPlay2AImpl_Receive,
  XCAST(Send)DirectPlay2AImpl_Send,
  XCAST(SetGroupData)DirectPlay2AImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2AImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2AImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2AImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2AImpl_SetSessionDesc,

  DirectPlay3AImpl_AddGroupToGroup,
  DirectPlay3AImpl_CreateGroupInGroup,
  DirectPlay3AImpl_DeleteGroupFromGroup,
  DirectPlay3AImpl_EnumConnections,
  DirectPlay3AImpl_EnumGroupsInGroup,
  DirectPlay3AImpl_GetGroupConnectionSettings,
  DirectPlay3AImpl_InitializeConnection,
  DirectPlay3AImpl_SecureOpen,
  DirectPlay3AImpl_SendChatMessage,
  DirectPlay3AImpl_SetGroupConnectionSettings,
  DirectPlay3AImpl_StartSession,
  DirectPlay3AImpl_GetGroupFlags,
  DirectPlay3AImpl_GetGroupParent,
  DirectPlay3AImpl_GetPlayerAccount,
  DirectPlay3AImpl_GetPlayerFlags
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay3WVT.fun))
#else
# define XCAST(fun)     (void*)
#endif
static const IDirectPlay3Vtbl directPlay3WVT =
{
  XCAST(QueryInterface)DP_QueryInterface,
  XCAST(AddRef)DP_AddRef,
  XCAST(Release)DP_Release,

  XCAST(AddPlayerToGroup)DirectPlay2WImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2WImpl_Close,
  XCAST(CreateGroup)DirectPlay2WImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2WImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2WImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2WImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2WImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2WImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2WImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2WImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2WImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2WImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2WImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2WImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2WImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2WImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2WImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2WImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2WImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2WImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2WImpl_Initialize,
  XCAST(Open)DirectPlay2WImpl_Open,
  XCAST(Receive)DirectPlay2WImpl_Receive,
  XCAST(Send)DirectPlay2WImpl_Send,
  XCAST(SetGroupData)DirectPlay2WImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2WImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2WImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2WImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2WImpl_SetSessionDesc,

  DirectPlay3WImpl_AddGroupToGroup,
  DirectPlay3WImpl_CreateGroupInGroup,
  DirectPlay3WImpl_DeleteGroupFromGroup,
  DirectPlay3WImpl_EnumConnections,
  DirectPlay3WImpl_EnumGroupsInGroup,
  DirectPlay3WImpl_GetGroupConnectionSettings,
  DirectPlay3WImpl_InitializeConnection,
  DirectPlay3WImpl_SecureOpen,
  DirectPlay3WImpl_SendChatMessage,
  DirectPlay3WImpl_SetGroupConnectionSettings,
  DirectPlay3WImpl_StartSession,
  DirectPlay3WImpl_GetGroupFlags,
  DirectPlay3WImpl_GetGroupParent,
  DirectPlay3WImpl_GetPlayerAccount,
  DirectPlay3WImpl_GetPlayerFlags
};
#undef XCAST

/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay4WVT.fun))
#else
# define XCAST(fun)     (void*)
#endif
static const IDirectPlay4Vtbl directPlay4WVT =
{
  XCAST(QueryInterface)DP_QueryInterface,
  XCAST(AddRef)DP_AddRef,
  XCAST(Release)DP_Release,

  XCAST(AddPlayerToGroup)DirectPlay2WImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2WImpl_Close,
  XCAST(CreateGroup)DirectPlay2WImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2WImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2WImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2WImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2WImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2WImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2WImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2WImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2WImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2WImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2WImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2WImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2WImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2WImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2WImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2WImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2WImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2WImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2WImpl_Initialize,
  XCAST(Open)DirectPlay2WImpl_Open,
  XCAST(Receive)DirectPlay2WImpl_Receive,
  XCAST(Send)DirectPlay2WImpl_Send,
  XCAST(SetGroupData)DirectPlay2WImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2WImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2WImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2WImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2WImpl_SetSessionDesc,

  XCAST(AddGroupToGroup)DirectPlay3WImpl_AddGroupToGroup,
  XCAST(CreateGroupInGroup)DirectPlay3WImpl_CreateGroupInGroup,
  XCAST(DeleteGroupFromGroup)DirectPlay3WImpl_DeleteGroupFromGroup,
  XCAST(EnumConnections)DirectPlay3WImpl_EnumConnections,
  XCAST(EnumGroupsInGroup)DirectPlay3WImpl_EnumGroupsInGroup,
  XCAST(GetGroupConnectionSettings)DirectPlay3WImpl_GetGroupConnectionSettings,
  XCAST(InitializeConnection)DirectPlay3WImpl_InitializeConnection,
  XCAST(SecureOpen)DirectPlay3WImpl_SecureOpen,
  XCAST(SendChatMessage)DirectPlay3WImpl_SendChatMessage,
  XCAST(SetGroupConnectionSettings)DirectPlay3WImpl_SetGroupConnectionSettings,
  XCAST(StartSession)DirectPlay3WImpl_StartSession,
  XCAST(GetGroupFlags)DirectPlay3WImpl_GetGroupFlags,
  XCAST(GetGroupParent)DirectPlay3WImpl_GetGroupParent,
  XCAST(GetPlayerAccount)DirectPlay3WImpl_GetPlayerAccount,
  XCAST(GetPlayerFlags)DirectPlay3WImpl_GetPlayerFlags,

  DirectPlay4WImpl_GetGroupOwner,
  DirectPlay4WImpl_SetGroupOwner,
  DirectPlay4WImpl_SendEx,
  DirectPlay4WImpl_GetMessageQueue,
  DirectPlay4WImpl_CancelMessage,
  DirectPlay4WImpl_CancelPriority
};
#undef XCAST


/* Note: Hack so we can reuse the old functions without compiler warnings */
#if !defined(__STRICT_ANSI__) && defined(__GNUC__)
# define XCAST(fun)     (typeof(directPlay4AVT.fun))
#else
# define XCAST(fun)     (void*)
#endif
static const IDirectPlay4Vtbl directPlay4AVT =
{
  XCAST(QueryInterface)DP_QueryInterface,
  XCAST(AddRef)DP_AddRef,
  XCAST(Release)DP_Release,

  XCAST(AddPlayerToGroup)DirectPlay2AImpl_AddPlayerToGroup,
  XCAST(Close)DirectPlay2AImpl_Close,
  XCAST(CreateGroup)DirectPlay2AImpl_CreateGroup,
  XCAST(CreatePlayer)DirectPlay2AImpl_CreatePlayer,
  XCAST(DeletePlayerFromGroup)DirectPlay2AImpl_DeletePlayerFromGroup,
  XCAST(DestroyGroup)DirectPlay2AImpl_DestroyGroup,
  XCAST(DestroyPlayer)DirectPlay2AImpl_DestroyPlayer,
  XCAST(EnumGroupPlayers)DirectPlay2AImpl_EnumGroupPlayers,
  XCAST(EnumGroups)DirectPlay2AImpl_EnumGroups,
  XCAST(EnumPlayers)DirectPlay2AImpl_EnumPlayers,
  XCAST(EnumSessions)DirectPlay2AImpl_EnumSessions,
  XCAST(GetCaps)DirectPlay2AImpl_GetCaps,
  XCAST(GetGroupData)DirectPlay2AImpl_GetGroupData,
  XCAST(GetGroupName)DirectPlay2AImpl_GetGroupName,
  XCAST(GetMessageCount)DirectPlay2AImpl_GetMessageCount,
  XCAST(GetPlayerAddress)DirectPlay2AImpl_GetPlayerAddress,
  XCAST(GetPlayerCaps)DirectPlay2AImpl_GetPlayerCaps,
  XCAST(GetPlayerData)DirectPlay2AImpl_GetPlayerData,
  XCAST(GetPlayerName)DirectPlay2AImpl_GetPlayerName,
  XCAST(GetSessionDesc)DirectPlay2AImpl_GetSessionDesc,
  XCAST(Initialize)DirectPlay2AImpl_Initialize,
  XCAST(Open)DirectPlay2AImpl_Open,
  XCAST(Receive)DirectPlay2AImpl_Receive,
  XCAST(Send)DirectPlay2AImpl_Send,
  XCAST(SetGroupData)DirectPlay2AImpl_SetGroupData,
  XCAST(SetGroupName)DirectPlay2AImpl_SetGroupName,
  XCAST(SetPlayerData)DirectPlay2AImpl_SetPlayerData,
  XCAST(SetPlayerName)DirectPlay2AImpl_SetPlayerName,
  XCAST(SetSessionDesc)DirectPlay2AImpl_SetSessionDesc,

  XCAST(AddGroupToGroup)DirectPlay3AImpl_AddGroupToGroup,
  XCAST(CreateGroupInGroup)DirectPlay3AImpl_CreateGroupInGroup,
  XCAST(DeleteGroupFromGroup)DirectPlay3AImpl_DeleteGroupFromGroup,
  XCAST(EnumConnections)DirectPlay3AImpl_EnumConnections,
  XCAST(EnumGroupsInGroup)DirectPlay3AImpl_EnumGroupsInGroup,
  XCAST(GetGroupConnectionSettings)DirectPlay3AImpl_GetGroupConnectionSettings,
  XCAST(InitializeConnection)DirectPlay3AImpl_InitializeConnection,
  XCAST(SecureOpen)DirectPlay3AImpl_SecureOpen,
  XCAST(SendChatMessage)DirectPlay3AImpl_SendChatMessage,
  XCAST(SetGroupConnectionSettings)DirectPlay3AImpl_SetGroupConnectionSettings,
  XCAST(StartSession)DirectPlay3AImpl_StartSession,
  XCAST(GetGroupFlags)DirectPlay3AImpl_GetGroupFlags,
  XCAST(GetGroupParent)DirectPlay3AImpl_GetGroupParent,
  XCAST(GetPlayerAccount)DirectPlay3AImpl_GetPlayerAccount,
  XCAST(GetPlayerFlags)DirectPlay3AImpl_GetPlayerFlags,

  DirectPlay4AImpl_GetGroupOwner,
  DirectPlay4AImpl_SetGroupOwner,
  DirectPlay4AImpl_SendEx,
  DirectPlay4AImpl_GetMessageQueue,
  DirectPlay4AImpl_CancelMessage,
  DirectPlay4AImpl_CancelPriority
};
#undef XCAST

extern
HRESULT DP_GetSPPlayerData( IDirectPlay2Impl* lpDP,
                            DPID idPlayer,
                            LPVOID* lplpData )
{
  lpPlayerList lpPlayer = DP_FindPlayer( lpDP, idPlayer );

  if( lpPlayer == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  *lplpData = lpPlayer->lpPData->lpSPPlayerData;

  return DP_OK;
}

extern
HRESULT DP_SetSPPlayerData( IDirectPlay2Impl* lpDP,
                            DPID idPlayer,
                            LPVOID lpData )
{
  lpPlayerList lpPlayer = DP_FindPlayer( lpDP, idPlayer );

  if( lpPlayer == NULL )
  {
    return DPERR_INVALIDPLAYER;
  }

  lpPlayer->lpPData->lpSPPlayerData = lpData;

  return DP_OK;
}

/***************************************************************************
 *  DirectPlayEnumerateAW
 *
 *  The pointer to the structure lpContext will be filled with the
 *  appropriate data for each service offered by the OS. These services are
 *  not necessarily available on this particular machine but are defined
 *  as simple service providers under the "Service Providers" registry key.
 *  This structure is then passed to lpEnumCallback for each of the different
 *  services.
 *
 *  This API is useful only for applications written using DirectX3 or
 *  worse. It is superseded by IDirectPlay3::EnumConnections which also
 *  gives information on the actual connections.
 *
 * defn of a service provider:
 * A dynamic-link library used by DirectPlay to communicate over a network.
 * The service provider contains all the network-specific code required
 * to send and receive messages. Online services and network operators can
 * supply service providers to use specialized hardware, protocols, communications
 * media, and network resources.
 *
 */
static HRESULT DirectPlayEnumerateAW(LPDPENUMDPCALLBACKA lpEnumCallbackA,
                                     LPDPENUMDPCALLBACKW lpEnumCallbackW,
                                     LPVOID lpContext)
{
    HKEY   hkResult;
    static const WCHAR searchSubKey[] = {
	'S', 'O', 'F', 'T', 'W', 'A', 'R', 'E', '\\',
	'M', 'i', 'c', 'r', 'o', 's', 'o', 'f', 't', '\\',
	'D', 'i', 'r', 'e', 'c', 't', 'P', 'l', 'a', 'y', '\\',
	'S', 'e', 'r', 'v', 'i', 'c', 'e', ' ', 'P', 'r', 'o', 'v', 'i', 'd', 'e', 'r', 's', 0 };
    static const WCHAR guidKey[] = { 'G', 'u', 'i', 'd', 0 };
    static const WCHAR descW[] = { 'D', 'e', 's', 'c', 'r', 'i', 'p', 't', 'i', 'o', 'n', 'W', 0 };

    DWORD  dwIndex;
    FILETIME filetime;

    char  *descriptionA = NULL;
    DWORD max_sizeOfDescriptionA = 0;
    WCHAR *descriptionW = NULL;
    DWORD max_sizeOfDescriptionW = 0;

    if (!lpEnumCallbackA && !lpEnumCallbackW)
    {
	return DPERR_INVALIDPARAMS;
    }

    /* Need to loop over the service providers in the registry */
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, searchSubKey,
		      0, KEY_READ, &hkResult) != ERROR_SUCCESS)
    {
	/* Hmmm. Does this mean that there are no service providers? */
	ERR(": no service provider key in the registry - check your Wine installation !!!\n");
	return DPERR_GENERIC;
    }

    /* Traverse all the service providers we have available */
    dwIndex = 0;
    while (1)
    {
	WCHAR subKeyName[255]; /* 255 is the maximum key size according to MSDN */
	DWORD sizeOfSubKeyName = sizeof(subKeyName) / sizeof(WCHAR);
	HKEY  hkServiceProvider;
	GUID  serviceProviderGUID;
	WCHAR guidKeyContent[(2 * 16) + 1 + 6 /* This corresponds to '{....-..-..-..-......}' */ ];
	DWORD sizeOfGuidKeyContent = sizeof(guidKeyContent);
	LONG  ret_value;

	ret_value = RegEnumKeyExW(hkResult, dwIndex, subKeyName, &sizeOfSubKeyName,
				  NULL, NULL, NULL, &filetime);
	if (ret_value == ERROR_NO_MORE_ITEMS)
	    break;
	else if (ret_value != ERROR_SUCCESS)
	{
	    ERR(": could not enumerate on service provider key.\n");
	    return DPERR_EXCEPTION;
	}
	TRACE(" this time through sub-key %s.\n", debugstr_w(subKeyName));

	/* Open the key for this service provider */
	if (RegOpenKeyExW(hkResult, subKeyName, 0, KEY_READ, &hkServiceProvider) != ERROR_SUCCESS)
	{
	    ERR(": could not open registry key for service provider %s.\n", debugstr_w(subKeyName));
	    continue;
	}

	/* Get the GUID from the registry */
	if (RegQueryValueExW(hkServiceProvider, guidKey,
			     NULL, NULL, (LPBYTE) guidKeyContent, &sizeOfGuidKeyContent) != ERROR_SUCCESS)
	{
	    ERR(": missing GUID registry data member for service provider %s.\n", debugstr_w(subKeyName));
	    continue;
	}
	if (sizeOfGuidKeyContent != sizeof(guidKeyContent))
	{
	    ERR(": invalid format for the GUID registry data member for service provider %s (%s).\n", debugstr_w(subKeyName), debugstr_w(guidKeyContent));
	    continue;
	}
	CLSIDFromString(guidKeyContent, &serviceProviderGUID );

	/* The enumeration will return FALSE if we are not to continue.
	 *
	 * Note: on my windows box, major / minor version is 6 / 0 for all service providers
	 *       and have no relations to any of the two dwReserved1 and dwReserved2 keys.
	 *       I think that it simply means that they are in-line with DirectX 6.0
	 */
	if (lpEnumCallbackA)
	{
	    DWORD sizeOfDescription = 0;

	    /* Note that this the the A case of this function, so use the A variant to get the description string */
	    if (RegQueryValueExA(hkServiceProvider, "DescriptionA",
				 NULL, NULL, NULL, &sizeOfDescription) != ERROR_SUCCESS)
	    {
		ERR(": missing 'DescriptionA' registry data member for service provider %s.\n", debugstr_w(subKeyName));
		continue;
	    }
	    if (sizeOfDescription > max_sizeOfDescriptionA)
	    {
		HeapFree(GetProcessHeap(), 0, descriptionA);
		max_sizeOfDescriptionA = sizeOfDescription;
		descriptionA = HeapAlloc(GetProcessHeap(), 0, max_sizeOfDescriptionA);
	    }
	    descriptionA = HeapAlloc(GetProcessHeap(), 0, sizeOfDescription);
	    RegQueryValueExA(hkServiceProvider, "DescriptionA",
			     NULL, NULL, (LPBYTE) descriptionA, &sizeOfDescription);

	    if (!lpEnumCallbackA(&serviceProviderGUID, descriptionA, 6, 0, lpContext))
		goto end;
	}
	else
	{
	    DWORD sizeOfDescription = 0;

	    if (RegQueryValueExW(hkServiceProvider, descW,
				 NULL, NULL, NULL, &sizeOfDescription) != ERROR_SUCCESS)
	    {
		ERR(": missing 'DescriptionW' registry data member for service provider %s.\n", debugstr_w(subKeyName));
		continue;
	    }
	    if (sizeOfDescription > max_sizeOfDescriptionW)
	    {
		HeapFree(GetProcessHeap(), 0, descriptionW);
		max_sizeOfDescriptionW = sizeOfDescription;
		descriptionW = HeapAlloc(GetProcessHeap(), 0, max_sizeOfDescriptionW);
	    }
	    descriptionW = HeapAlloc(GetProcessHeap(), 0, sizeOfDescription);
	    RegQueryValueExW(hkServiceProvider, descW,
			     NULL, NULL, (LPBYTE) descriptionW, &sizeOfDescription);

	    if (!lpEnumCallbackW(&serviceProviderGUID, descriptionW, 6, 0, lpContext))
		goto end;
	}

      dwIndex++;
  }

 end:
    HeapFree(GetProcessHeap(), 0, descriptionA);
    HeapFree(GetProcessHeap(), 0, descriptionW);

    return DP_OK;
}

/***************************************************************************
 *  DirectPlayEnumerate  [DPLAYX.9]
 *  DirectPlayEnumerateA [DPLAYX.2]
 */
HRESULT WINAPI DirectPlayEnumerateA(LPDPENUMDPCALLBACKA lpEnumCallback, LPVOID lpContext )
{
    TRACE("(%p,%p)\n", lpEnumCallback, lpContext);

    return DirectPlayEnumerateAW(lpEnumCallback, NULL, lpContext);
}

/***************************************************************************
 *  DirectPlayEnumerateW [DPLAYX.3]
 */
HRESULT WINAPI DirectPlayEnumerateW(LPDPENUMDPCALLBACKW lpEnumCallback, LPVOID lpContext )
{
    TRACE("(%p,%p)\n", lpEnumCallback, lpContext);

    return DirectPlayEnumerateAW(NULL, lpEnumCallback, lpContext);
}

typedef struct tagCreateEnum
{
  LPVOID  lpConn;
  LPCGUID lpGuid;
} CreateEnumData, *lpCreateEnumData;

/* Find and copy the matching connection for the SP guid */
static BOOL CALLBACK cbDPCreateEnumConnections(
    LPCGUID     lpguidSP,
    LPVOID      lpConnection,
    DWORD       dwConnectionSize,
    LPCDPNAME   lpName,
    DWORD       dwFlags,
    LPVOID      lpContext)
{
  lpCreateEnumData lpData = (lpCreateEnumData)lpContext;

  if( IsEqualGUID( lpguidSP, lpData->lpGuid ) )
  {
    TRACE( "Found SP entry with guid %s\n", debugstr_guid(lpData->lpGuid) );

    lpData->lpConn = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                dwConnectionSize );
    CopyMemory( lpData->lpConn, lpConnection, dwConnectionSize );

    /* Found the record that we were looking for */
    return FALSE;
  }

  /* Haven't found what were looking for yet */
  return TRUE;
}


/***************************************************************************
 *  DirectPlayCreate [DPLAYX.1]
 *
 */
HRESULT WINAPI DirectPlayCreate
( LPGUID lpGUID, LPDIRECTPLAY2 *lplpDP, IUnknown *pUnk)
{
  HRESULT hr;
  LPDIRECTPLAY3A lpDP3A;
  CreateEnumData cbData;

  TRACE( "lpGUID=%s lplpDP=%p pUnk=%p\n", debugstr_guid(lpGUID), lplpDP, pUnk );

  if( pUnk != NULL )
  {
    return CLASS_E_NOAGGREGATION;
  }

  /* Create an IDirectPlay object. We don't support that so we'll cheat and
     give them an IDirectPlay2A object and hope that doesn't cause problems */
  if( DP_CreateInterface( &IID_IDirectPlay2A, (LPVOID*)lplpDP ) != DP_OK )
  {
    return DPERR_UNAVAILABLE;
  }

  if( IsEqualGUID( &GUID_NULL, lpGUID ) )
  {
    /* The GUID_NULL means don't bind a service provider. Just return the
       interface as is */
    return DP_OK;
  }

  /* Bind the desired service provider since lpGUID is non NULL */
  TRACE( "Service Provider binding for %s\n", debugstr_guid(lpGUID) );

  /* We're going to use a DP3 interface */
  hr = IDirectPlayX_QueryInterface( *lplpDP, &IID_IDirectPlay3A,
                                    (LPVOID*)&lpDP3A );
  if( FAILED(hr) )
  {
    ERR( "Failed to get DP3 interface: %s\n", DPLAYX_HresultToString(hr) );
    return hr;
  }

  cbData.lpConn = NULL;
  cbData.lpGuid = lpGUID;

  /* We were given a service provider, find info about it... */
  hr = IDirectPlayX_EnumConnections( lpDP3A, NULL, cbDPCreateEnumConnections,
                                     &cbData, DPCONNECTION_DIRECTPLAY );
  if( ( FAILED(hr) ) ||
      ( cbData.lpConn == NULL )
    )
  {
    ERR( "Failed to get Enum for SP: %s\n", DPLAYX_HresultToString(hr) );
    IDirectPlayX_Release( lpDP3A );
    return DPERR_UNAVAILABLE;
  }

  /* Initialize the service provider */
  hr = IDirectPlayX_InitializeConnection( lpDP3A, cbData.lpConn, 0 );
  if( FAILED(hr) )
  {
    ERR( "Failed to Initialize SP: %s\n", DPLAYX_HresultToString(hr) );
    HeapFree( GetProcessHeap(), 0, cbData.lpConn );
    IDirectPlayX_Release( lpDP3A );
    return hr;
  }

  /* Release our version of the interface now that we're done with it */
  IDirectPlayX_Release( lpDP3A );
  HeapFree( GetProcessHeap(), 0, cbData.lpConn );

  return DP_OK;
}
