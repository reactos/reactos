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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_DPLAYX_MESSAGES__
#define __WINE_DPLAYX_MESSAGES__

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "dplay.h"
#include "rpc.h" /* For GUID */

#include "dplay_global.h"

DWORD CreateLobbyMessageReceptionThread( HANDLE hNotifyEvent, HANDLE hStart,
                                         HANDLE hDeath, HANDLE hConnRead );

HRESULT DP_MSG_SendRequestPlayerId( IDirectPlay2AImpl* This, DWORD dwFlags,
                                    LPDPID lpdipidAllocatedId );
HRESULT DP_MSG_ForwardPlayerCreation( IDirectPlay2AImpl* This, DPID dpidServer );

void DP_MSG_ReplyReceived( IDirectPlay2AImpl* This, WORD wCommandId,
                           LPCVOID lpMsgBody, DWORD dwMsgBodySize );
void DP_MSG_ErrorReceived( IDirectPlay2AImpl* This, WORD wCommandId,
                           LPCVOID lpMsgBody, DWORD dwMsgBodySize );
void DP_MSG_ToSelf( IDirectPlay2AImpl* This, DPID dpidSelf );

/* Timings -> 1000 ticks/sec */
#define DPMSG_WAIT_5_SECS   5000
#define DPMSG_WAIT_30_SECS 30000
#define DPMSG_WAIT_60_SECS 60000
#define DPMSG_DEFAULT_WAIT_TIME DPMSG_WAIT_30_SECS

/* Message types etc. */
#include "pshpack1.h"

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

#define DPMSGCMD_ENUMGROUPS           17

#define DPMSGCMD_FORWARDADDPLAYER     19

#define DPMSGCMD_PLAYERCHAT           22

#define DPMSGCMD_FORWARDADDPLAYERNACK 36

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

  DWORD dwPasswordSize; /* A Guess. This is 0x00000000. */
                        /* This might be the name server DPID which
                           is needed for the reply */

  DWORD dwFlags; /* dwFlags from EnumSessions */

} DPMSG_ENUMSESSIONSREQUEST, *LPDPMSG_ENUMSESSIONSREQUEST;
typedef const DPMSG_ENUMSESSIONSREQUEST* LPCDPMSG_ENUMSESSIONSREQUEST;

/* Size is 146 received - with 18 or 20 bytes header = ~128 bytes */
typedef struct tagDPMSG_CREATESESSION
{
  DPMSG_SENDENVELOPE envelope;
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

  /* Assume that this is data that is tacked on to the end of the message
   * that comes from the SP remote data stored that needs to be propagated.
   */
  BYTE unknown[36];     /* This appears to always be 0 - not sure though */
} DPMSG_NEWPLAYERIDREPLY, *LPDPMSG_NEWPLAYERIDREPLY;
typedef const DPMSG_NEWPLAYERIDREPLY* LPCDPMSG_NEWPLAYERIDREPLY;

typedef struct tagDPMSG_FORWARDADDPLAYER
{
  DPMSG_SENDENVELOPE envelope;

  DWORD unknown; /* 0 */

  DPID  dpidAppServer; /* Remote application server id */
  DWORD unknown2[5]; /* 0x0, 0x1c, 0x6c, 0x50, 0x9 */

  DPID  dpidAppServer2; /* Remote application server id again !? */
  DWORD unknown3[5]; /* 0x0, 0x0, 0x20, 0x0, 0x0 */

  DPID  dpidAppServer3; /* Remote application server id again !? */

  DWORD unknown4[12]; /* ??? - Is this a clump of 5 and then 8? */
                      /* NOTE: 1 byte in front of the two 0x??090002 entries changes!
                      *       Is it a timestamp of some sort? 1st always smaller than
                      *       other...
                      */
#define FORWARDADDPLAYER_UNKNOWN4_INIT { 0x30, 0xb, 0x0, 0x1e090002, 0x0, 0x0, 0x0, 0x32090002, 0x0, 0x0, 0x0, 0x0 }

  BYTE unknown5[2]; /* 2 bytes at the end. This may be a part of something! ( 0x0, 0x0) */

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

#include "poppack.h"

#endif
