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

#ifndef __WINE_DPLAYX_MESSAGES__
#define __WINE_DPLAYX_MESSAGES__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dplay.h"
#include "rpc.h" /* For GUID */

#include "dplay_global.h"

typedef struct
{
  DWORD flags;
  DPID id;
  DWORD versionOrSystemPlayerId;
  DPNAME name;
  DWORD playerDataLength;
  void *playerData;
  DWORD spDataLength;
  void *spData;
  DWORD playerCount;
  DPID *playerIds;
  DPID parentId;
  DWORD shortcutCount;
  DPID *shortcutIds;
} DPPLAYERINFO;

DWORD CreateLobbyMessageReceptionThread( HANDLE hNotifyEvent, HANDLE hStart,
                                         HANDLE hDeath, HANDLE hConnRead );

DWORD DP_MSG_ComputeMessageSize( SGBUFFER *buffers, DWORD bufferCount );
HRESULT DP_MSG_SendRequestPlayerId( IDirectPlayImpl *This, DWORD dwFlags,
                                    LPDPID lpdipidAllocatedId );
HRESULT DP_MSG_ReadPackedPlayer( char *data, DWORD *offset, DWORD maxSize,
                                 DPPLAYERINFO *playerInfo );
HRESULT DP_MSG_ForwardPlayerCreation( IDirectPlayImpl *This, DPID dpidServer, WCHAR *password );
HRESULT DP_MSG_SendCreatePlayer( IDirectPlayImpl *This, DPID toId, DPID id, DWORD flags,
                                 DPNAME *name, void *playerData, DWORD playerDataSize,
                                 DPID systemPlayerId );
HRESULT DP_MSG_SendAddPlayerToGroup( IDirectPlayImpl *This, DPID toId, DPID playerId,
                                     DPID groupId );
HRESULT DP_MSG_SendPingReply( IDirectPlayImpl *This, DPID toId, DPID fromId, DWORD tickCount );
HRESULT DP_MSG_SendAddForwardAck( IDirectPlayImpl *This, DPID id );

void DP_MSG_ReplyReceived( IDirectPlayImpl *This, WORD wCommandId,
                           LPCVOID lpMsgBody, DWORD dwMsgBodySize,
                           const void *msgHeader );
void DP_MSG_ToSelf( IDirectPlayImpl *This, DPID dpidSelf );

/* Timings -> 1000 ticks/sec */
#define DPMSG_WAIT_5_SECS   5000
#define DPMSG_WAIT_30_SECS 30000
#define DPMSG_WAIT_60_SECS 60000
#define DPMSG_DEFAULT_WAIT_TIME DPMSG_WAIT_30_SECS

/* Message types etc. */
#include "pshpack1.h"

typedef struct
{
  DWORD size;
  DWORD flags;
  DPID id;
  DWORD shortNameLength;
  DWORD longNameLength;
  DWORD spDataLength;
  DWORD playerDataLength;
  DWORD playerCount;
  DPID systemPlayerId;
  DWORD fixedSize;
  DWORD version;
  DPID parentId;
} DPLAYI_PACKEDPLAYER;

/* Non provided messages for DPLAY - guess work which may be wrong :( */
#define DPMSGCMD_ENUMSESSIONSREPLY    1
#define DPMSGCMD_ENUMSESSIONSREQUEST  2
#define DPMSGCMD_GETNAMETABLEREPLY    3  /* Contains all existing players in session */

#define DPMSGCMD_REQUESTNEWPLAYERID   5

#define DPMSGCMD_NEWPLAYERIDREPLY     7
#define DPMSGCMD_CREATESESSION        8 /* Might be a create nameserver or new player msg */
#define DPMSGCMD_CREATENEWPLAYER      9
#define DPMSGCMD_SYSTEMMESSAGE        10
#define DPMSGCMD_DELETEPLAYER         11
#define DPMSGCMD_DELETEGROUP          12
#define DPMSGCMD_ADDPLAYERTOGROUP     13

#define DPMSGCMD_GROUPDATACHANGED     17

#define DPMSGCMD_FORWARDADDPLAYER     19

#define DPMSGCMD_PING                 22
#define DPMSGCMD_PINGREPLY            23

#define DPMSGCMD_FORWARDADDPLAYERNACK 36

#define DPMSGCMD_SUPERENUMPLAYERSREPLY 41

#define DPMSGCMD_ADDFORWARD           46
#define DPMSGCMD_ADDFORWARDACK        47

#define DPMSGCMD_JUSTENVELOPE         1000
#define DPMSGCMD_JUSTENVELOPEREPLY    1001

/* This is what DP 6 defines it as. Don't know what it means. All messages
 * defined below are DPMSGVER_DP6.
 */
#define DPMSGVER_DP6 11

/* MAGIC number at the start of all dplay packets ("play" in ASCII) */
#define DPMSGMAGIC_DPLAYMSG  0x79616c70

/* All messages sent from the system are sent with this at the beginning of
 * the message.
 * Size is 8 bytes
 */
typedef struct tagDPMSG_SENDENVELOPE
{
  DWORD dwMagic;
  WORD  wCommandId;
  WORD  wVersion;
} DPMSG_SENDENVELOPE, *LPDPMSG_SENDENVELOPE;
typedef const DPMSG_SENDENVELOPE* LPCDPMSG_SENDENVELOPE;

/* System messages exchanged between players seems to have this
 * payload envelope on top of the basic envelope
 */
typedef struct tagDPMSG_SYSMSGENVELOPE
{
  DWORD dwPlayerFrom;
  DWORD dwPlayerTo;
} DPMSG_SYSMSGENVELOPE, *LPDPMSG_SYSMSGENVELOPE;
typedef const DPMSG_SYSMSGENVELOPE* LPCDPMSG_SYSMSGENVELOPE;

/* Reply sent in response to an enumsession request */
typedef struct tagDPMSG_ENUMSESSIONSREPLY
{
  DPMSG_SENDENVELOPE envelope;

#if 0
  DWORD dwSize;  /* Size of DPSESSIONDESC2 struct */
  DWORD dwFlags; /* Sessions flags */

  GUID guidInstance; /* Not 100% sure this is what it is... */

  GUID guidApplication;

  DWORD dwMaxPlayers;
  DWORD dwCurrentPlayers;

  BYTE unknown[36];
#else
  DPSESSIONDESC2 sd;
#endif

  DWORD dwUnknown;  /* Seems to be equal to 0x5c which is a "\\" */
                    /* Encryption package string? */

  /* At the end we have ... */
  /* WCHAR wszSessionName[1];  Var length with NULL terminal */

} DPMSG_ENUMSESSIONSREPLY, *LPDPMSG_ENUMSESSIONSREPLY;
typedef const DPMSG_ENUMSESSIONSREPLY* LPCDPMSG_ENUMSESSIONSREPLY;

/* Msg sent to find out what sessions are available */
typedef struct tagDPMSG_ENUMSESSIONSREQUEST
{
  DPMSG_SENDENVELOPE envelope;

  GUID  guidApplication;

  DWORD passwordOffset;

  DWORD dwFlags; /* dwFlags from EnumSessions */

} DPMSG_ENUMSESSIONSREQUEST, *LPDPMSG_ENUMSESSIONSREQUEST;
typedef const DPMSG_ENUMSESSIONSREQUEST* LPCDPMSG_ENUMSESSIONSREQUEST;

/* Size is 146 received - with 18 or 20 bytes header = ~128 bytes */
typedef struct tagDPMSG_CREATESESSION
{
  DPMSG_SENDENVELOPE envelope;
  DPID toId;
  DPID playerId;
  DPID groupId;
  DWORD createOffset;
  DWORD passwordOffset;
} DPMSG_CREATESESSION, *LPDPMSG_CREATESESSION;
typedef const DPMSG_CREATESESSION* LPCDPMSG_CREATESESSION;

/* 12 bytes msg */
typedef struct tagDPMSG_REQUESTNEWPLAYERID
{
  DPMSG_SENDENVELOPE envelope;

  DWORD dwFlags;  /* dwFlags used for CreatePlayer */

} DPMSG_REQUESTNEWPLAYERID, *LPDPMSG_REQUESTNEWPLAYERID;
typedef const DPMSG_REQUESTNEWPLAYERID* LPCDPMSG_REQUESTNEWPLAYERID;

/* 48 bytes msg */
typedef struct tagDPMSG_NEWPLAYERIDREPLY
{
  DPMSG_SENDENVELOPE envelope;

  DPID dpidNewPlayerId;

  DPSECURITYDESC secDesc;
  DWORD sspiProviderOffset;
  DWORD capiProviderOffset;
  HRESULT result;
} DPMSG_NEWPLAYERIDREPLY, *LPDPMSG_NEWPLAYERIDREPLY;
typedef const DPMSG_NEWPLAYERIDREPLY* LPCDPMSG_NEWPLAYERIDREPLY;

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  DPID toId;
  DPID playerId;
  DPID groupId;
  DWORD createOffset;
  DWORD passwordOffset;
} DPSP_MSG_ADDPLAYERTOGROUP;

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  DPID toId;
  DPID groupId;
  DWORD dataSize;
  DWORD dataOffset;
} DPSP_MSG_GROUPDATACHANGED;

typedef struct tagDPMSG_FORWARDADDPLAYER
{
  DPMSG_SENDENVELOPE envelope;

  DPID toId;
  DPID playerId;
  DPID groupId;
  DWORD createOffset;
  DWORD passwordOffset;
} DPMSG_FORWARDADDPLAYER, *LPDPMSG_FORWARDADDPLAYER;
typedef const DPMSG_FORWARDADDPLAYER* LPCDPMSG_FORWARDADDPLAYER;

/* This is an error message that can be received. Not sure if this is
 * specifically for a forward add player or for all errors
 */
typedef struct tagDPMSG_FORWARDADDPLAYERNACK
{
  DPMSG_SENDENVELOPE envelope;
  HRESULT errorCode;
} DPMSG_FORWARDADDPLAYERNACK, *LPDPMSG_FORWARDADDPLAYERNACK;
typedef const DPMSG_FORWARDADDPLAYERNACK* LPCDPMSG_FORWARDADDPLAYERNACK;

typedef struct
{
  DWORD size;
  DWORD flags;
  DPID id;
  DWORD infoMask;
  DWORD versionOrSystemPlayerId;
} DPLAYI_SUPERPACKEDPLAYER;

#define DPLAYI_SUPERPACKEDPLAYER_SHORT_NAME_PRESENT              0x001
#define DPLAYI_SUPERPACKEDPLAYER_LONG_NAME_PRESENT               0x002
#define DPLAYI_SUPERPACKEDPLAYER_SP_DATA_LENGTH_SIZE( mask )     (((mask) >> 2) & 0x3)
#define DPLAYI_SUPERPACKEDPLAYER_PLAYER_DATA_LENGTH_SIZE( mask ) (((mask) >> 4) & 0x3)
#define DPLAYI_SUPERPACKEDPLAYER_PLAYER_COUNT_SIZE( mask )       (((mask) >> 6) & 0x3)
#define DPLAYI_SUPERPACKEDPLAYER_PARENT_ID_PRESENT               0x100
#define DPLAYI_SUPERPACKEDPLAYER_SHORTCUT_COUNT_SIZE( mask )     (((mask) >> 9) & 0x3)

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  DPID fromId;
  DWORD tickCount;
} DPSP_MSG_PING;

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  HRESULT error;
} DPSP_MSG_ADDFORWARDREPLY;

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  DWORD playerCount;
  DWORD groupCount;
  DWORD packedOffset;
  DWORD shortcutCount;
  DWORD descriptionOffset;
  DWORD nameOffset;
  DWORD passwordOffset;
} DPSP_MSG_SUPERENUMPLAYERSREPLY;

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  DPID toId;
  DPID playerId;
  DPID groupId;
  DWORD createOffset;
  DWORD passwordOffset;
} DPSP_MSG_ADDFORWARD;

typedef struct
{
  DPMSG_SENDENVELOPE envelope;
  DPID id;
} DPSP_MSG_ADDFORWARDACK;

#include "poppack.h"

#endif
