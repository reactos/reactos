/*
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

#ifndef __WINE_DPLAY_GLOBAL_INCLUDED
#define __WINE_DPLAY_GLOBAL_INCLUDED

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "wine/dplaysp.h"
#include "wine/list.h"
#include "lobbysp.h"
#include "dplayx_queue.h"

extern HRESULT DPL_EnumAddress( LPDPENUMADDRESSCALLBACK lpEnumAddressCallback,
                                LPCVOID lpAddress, DWORD dwAddressSize,
                                LPVOID lpContext );

typedef struct
{
    struct list entry;

    DWORD flags;
    GUID spGuid;
    void *address;
    DWORD addressSize;
    DPNAME name;
    DPNAME nameA;
    DWORD reserved1;
    DWORD reserved2;
    char *path;
} DPCONNECTION;

typedef struct tagEnumSessionAsyncCallbackData
{
  LPSPINITDATA lpSpData;
  GUID         requestGuid;
  DWORD        dwEnumSessionFlags;
  DWORD        dwTimeout;
  WCHAR       *password;
  HANDLE       hSuicideRequest;
} EnumSessionAsyncCallbackData;

typedef struct tagDP_MSG_REPLY_STRUCT
{
  HANDLE hReceipt;
  WORD  *expectedReplies;
  DWORD  expectedReplyCount;
  LPVOID lpReplyMsg;
  DWORD  dwMsgBodySize;
  void  *replyMsgHeader;
} DP_MSG_REPLY_STRUCT, *LPDP_MSG_REPLY_STRUCT;

typedef struct tagDP_MSG_REPLY_STRUCT_LIST
{
  DPQ_ENTRY(tagDP_MSG_REPLY_STRUCT_LIST) repliesExpected;
  DP_MSG_REPLY_STRUCT replyExpected;
} DP_MSG_REPLY_STRUCT_LIST, *LPDP_MSG_REPLY_STRUCT_LIST;

struct PlayerData
{
  /* Individual player information */
  DPID dpid;

  DPNAME *name;
  DPNAME *nameA;
  HANDLE hEvent;

  ULONG  uRef;  /* What is the reference count on this data? */

  /* View of local data */
  LPVOID lpLocalData;
  DWORD  dwLocalDataSize;

  /* View of remote data */
  LPVOID lpRemoteData;
  DWORD  dwRemoteDataSize;

  /* SP data on a per player basis */
  LPVOID lpSPPlayerData;

  DWORD  dwFlags; /* Special remarks about the type of player */
};
typedef struct PlayerData* lpPlayerData;

struct PlayerList
{
  DPQ_ENTRY(PlayerList) players;

  lpPlayerData lpPData;
};
typedef struct PlayerList* lpPlayerList;

struct GroupData
{
  /* Internal information */
  DPID parent; /* If parent == 0 it's a top level group */

  ULONG uRef; /* Reference count */

  DPQ_HEAD(GroupList)  groups;  /* A group has [0..n] groups */
  DPQ_HEAD(PlayerList) players; /* A group has [0..n] players */

  DPID idGroupOwner; /* ID of player who owns the group */

  DWORD dwFlags; /* Flags describing anything special about the group */

  DPID   dpid;
  DPNAME *name;
  DPNAME *nameA;

  /* View of local data */
  LPVOID lpLocalData;
  DWORD  dwLocalDataSize;

  /* View of remote data */
  LPVOID lpRemoteData;
  DWORD  dwRemoteDataSize;

  /* SP data on a per player basis */
  LPVOID lpSPPlayerData;
};
typedef struct GroupData  GroupData;
typedef struct GroupData* lpGroupData;

struct GroupList
{
  DPQ_ENTRY(GroupList) groups;

  lpGroupData lpGData;
};
typedef struct GroupList* lpGroupList;

typedef DWORD FN_COPY_MESSAGE( DPMSG_GENERIC *genericDst, DPMSG_GENERIC *genericSrc,
                               DWORD genericSize, BOOL ansi );

struct DPMSG
{
  DPQ_ENTRY( DPMSG ) msgs;
  DPID fromId;
  DPID toId;
  DPMSG_GENERIC* msg;
  FN_COPY_MESSAGE *copyMessage;
  DWORD genericSize;
};
typedef struct DPMSG* LPDPMSG;

enum SPSTATE
{
  NO_PROVIDER = 0,
  DP_SERVICE_PROVIDER = 1,
  DP_LOBBY_PROVIDER = 2
};

/* Contains all data members. FIXME: Rename me */
typedef struct tagDirectPlay2Data
{
  BOOL   bConnectionOpen;

  /* For async EnumSessions requests */
  HANDLE hEnumSessionThread;
  HANDLE hKillEnumSessionThreadEvent;
  DWORD  dwEnumSessionLock;

  LPVOID lpNameServerData; /* DPlay interface doesn't know contents */

  BOOL bHostInterface; /* Did this interface create the session */

  lpGroupData lpSysGroup; /* System group with _everything_ in it */

  DPID systemPlayerId;

  LPDPSESSIONDESC2 lpSessionDesc;

  /* I/O Msg queues */
  DPQ_HEAD( DPMSG ) receiveMsgs; /* Msg receive queue */

  /* Information about the service provider active on this connection */
  SPINITDATA spData;
  BOOL       bSPInitialized;

  /* Information about the lobby server that's attached to this DP object */
  SPDATA_INIT dplspData;
  BOOL        bDPLSPInitialized;

  /* Our service provider */
  HMODULE hServiceProvider;

  /* Our DP lobby provider */
  HMODULE hDPLobbyProvider;

  enum SPSTATE connectionInitialized;

  /* Expected messages queue */
  DPQ_HEAD( tagDP_MSG_REPLY_STRUCT_LIST ) repliesExpected;
} DirectPlay2Data;

typedef struct IDirectPlayImpl
{
  IDirectPlay IDirectPlay_iface;
  IDirectPlay2A IDirectPlay2A_iface;
  IDirectPlay2 IDirectPlay2_iface;
  IDirectPlay3A IDirectPlay3A_iface;
  IDirectPlay3 IDirectPlay3_iface;
  IDirectPlay4A IDirectPlay4A_iface;
  IDirectPlay4  IDirectPlay4_iface;
  LONG ref;
  CRITICAL_SECTION lock;
  DirectPlay2Data *dp2;
} IDirectPlayImpl;

HRESULT DP_HandleMessage( IDirectPlayImpl *This, void *messageBody,
        DWORD  dwMessageBodySize, void *messageHeader, WORD wCommandId, WORD wVersion,
        void **lplpReply, DWORD *lpdwMsgSize );
DPSESSIONDESC2 *DP_DuplicateSessionDesc( const DPSESSIONDESC2 *src, BOOL dstAnsi, BOOL srcAnsi );
HRESULT DP_HandleGameMessage( IDirectPlayImpl *This, void *messageBody, DWORD messageBodySize,
                              DPID fromId, DPID toId );
HRESULT DP_CreatePlayer( IDirectPlayImpl *This, void *msgHeader, DPID *lpid, DPNAME *lpName,
                         void *data, DWORD dataSize, void *spData, DWORD spDataSize, DWORD dwFlags,
                         HANDLE hEvent, struct PlayerData **playerData, BOOL bAnsi );
HRESULT DP_CreateGroup( IDirectPlayImpl *This, void *msgHeader, const DPID *lpid,
                        const DPNAME *lpName, void *data, DWORD dataSize, DWORD dwFlags,
                        DPID idParent, BOOL bAnsi );
HRESULT DP_AddPlayerToGroup( IDirectPlayImpl *This, DPID group, DPID player );

void DP_FreeConnections(void);

/* DP SP external interfaces into DirectPlay */
extern HRESULT DP_GetSPPlayerData( IDirectPlayImpl *lpDP, DPID idPlayer, void **lplpData );
extern HRESULT DP_SetSPPlayerData( IDirectPlayImpl *lpDP, DPID idPlayer, void *lpData );

/* DP external interfaces to call into DPSP interface */
extern LPVOID DPSP_CreateSPPlayerData(void);

extern HRESULT dplay_create( REFIID riid, void **ppv );
extern HRESULT dplobby_create( REFIID riid, void **ppv );
extern HRESULT dplaysp_create( REFIID riid, void **ppv, IDirectPlayImpl *dp );
extern HRESULT dplobbysp_create( REFIID riid, void **ppv, IDirectPlayImpl *dp );

#endif /* __WINE_DPLAY_GLOBAL_INCLUDED */
