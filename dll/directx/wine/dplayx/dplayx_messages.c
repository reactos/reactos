/* DirectPlay & DirectPlayLobby messaging implementation
 *
 * Copyright 2000,2001 - Peter Hunnisett
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
 *
 * NOTES
 *  o Messaging interface required for both DirectPlay and DirectPlayLobby.
 */

#include <stdarg.h>
#include <string.h>
#include <limits.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winerror.h"

#include "dplayx_messages.h"
#include "dplay_global.h"
#include "dplayx_global.h"
#include "name_server.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

typedef struct tagMSGTHREADINFO
{
  HANDLE hStart;
  HANDLE hDeath;
  HANDLE hSettingRead;
  HANDLE hNotifyEvent;
} MSGTHREADINFO, *LPMSGTHREADINFO;

static DWORD CALLBACK DPL_MSG_ThreadMain( LPVOID lpContext );
static HRESULT DP_MSG_ExpectReply( IDirectPlayImpl *This, DPSP_SENDEXDATA *data, DWORD dwWaitTime,
        WORD *replyCommandIds, DWORD replyCommandIdCount, void **lplpReplyMsg,
        DWORD *lpdwMsgBodySize, void **replyMsgHeader );


/* Create the message reception thread to allow the application to receive
 * asynchronous message reception
 */
DWORD CreateLobbyMessageReceptionThread( HANDLE hNotifyEvent, HANDLE hStart,
                                         HANDLE hDeath, HANDLE hConnRead )
{
  DWORD           dwMsgThreadId;
  LPMSGTHREADINFO lpThreadInfo;
  HANDLE          hThread;

  lpThreadInfo = malloc( sizeof( *lpThreadInfo ) );
  if( lpThreadInfo == NULL )
  {
    return 0;
  }

  /* The notify event may or may not exist. Depends if async comm or not */
  if( hNotifyEvent &&
      !DuplicateHandle( GetCurrentProcess(), hNotifyEvent,
                        GetCurrentProcess(), &lpThreadInfo->hNotifyEvent,
                        0, FALSE, DUPLICATE_SAME_ACCESS ) )
  {
    ERR( "Unable to duplicate event handle\n" );
    goto error;
  }

  /* These 3 handles don't need to be duplicated because we don't keep a
   * reference to them where they're created. They're created specifically
   * for the message thread
   */
  lpThreadInfo->hStart       = hStart;
  lpThreadInfo->hDeath       = hDeath;
  lpThreadInfo->hSettingRead = hConnRead;

  hThread = CreateThread( NULL,                  /* Security attribs */
                          0,                     /* Stack */
                          DPL_MSG_ThreadMain,    /* Msg reception function */
                          lpThreadInfo,          /* Msg reception func parameter */
                          0,                     /* Flags */
                          &dwMsgThreadId         /* Updated with thread id */
                        );
  if ( hThread == NULL )
  {
    ERR( "Unable to create msg thread\n" );
    goto error;
  }

  CloseHandle(hThread);

  return dwMsgThreadId;

error:

  free( lpThreadInfo );

  return 0;
}

static DWORD CALLBACK DPL_MSG_ThreadMain( LPVOID lpContext )
{
  LPMSGTHREADINFO lpThreadInfo = lpContext;
  DWORD dwWaitResult;

  TRACE( "Msg thread created. Waiting on app startup\n" );

  /* Wait to ensure that the lobby application is started w/ 1 min timeout */
  dwWaitResult = WaitForSingleObject( lpThreadInfo->hStart, 10000 /* 10 sec */ );
  if( dwWaitResult == WAIT_TIMEOUT )
  {
    FIXME( "Should signal app/wait creation failure (0x%08lx)\n", dwWaitResult );
    goto end_of_thread;
  }

  /* Close this handle as it's not needed anymore */
  CloseHandle( lpThreadInfo->hStart );
  lpThreadInfo->hStart = 0;

  /* Wait until the lobby knows what it is */
  dwWaitResult = WaitForSingleObject( lpThreadInfo->hSettingRead, INFINITE );
  if( dwWaitResult == WAIT_TIMEOUT )
  {
    ERR( "App Read connection setting timeout fail (0x%08lx)\n", dwWaitResult );
  }

  /* Close this handle as it's not needed anymore */
  CloseHandle( lpThreadInfo->hSettingRead );
  lpThreadInfo->hSettingRead = 0;

  TRACE( "App created && initialized starting main message reception loop\n" );

  for ( ;; )
  {
    MSG lobbyMsg;
    GetMessageW( &lobbyMsg, 0, 0, 0 );
  }

end_of_thread:
  TRACE( "Msg thread exiting!\n" );
  free( lpThreadInfo );

  return 0;
}

/* DP messaging stuff */
static HANDLE DP_MSG_BuildAndLinkReplyStruct( IDirectPlayImpl *This,
        DP_MSG_REPLY_STRUCT_LIST *lpReplyStructList, WORD *replyCommandIds,
        DWORD replyCommandIdCount )
{
  lpReplyStructList->replyExpected.hReceipt           = CreateEventW( NULL, FALSE, FALSE, NULL );
  lpReplyStructList->replyExpected.expectedReplies    = replyCommandIds;
  lpReplyStructList->replyExpected.expectedReplyCount = replyCommandIdCount;
  lpReplyStructList->replyExpected.lpReplyMsg         = NULL;
  lpReplyStructList->replyExpected.dwMsgBodySize      = 0;

  /* Insert into the message queue while locked */
  EnterCriticalSection( &This->lock );
    DPQ_INSERT( This->dp2->repliesExpected, lpReplyStructList, repliesExpected );
  LeaveCriticalSection( &This->lock );

  return lpReplyStructList->replyExpected.hReceipt;
}

static DP_MSG_REPLY_STRUCT_LIST *DP_MSG_FindAndUnlinkReplyStruct( IDirectPlayImpl *This,
                                                                  WORD commandId )
{
    DP_MSG_REPLY_STRUCT_LIST *reply;
    DWORD i;

    EnterCriticalSection( &This->lock );

    for( reply = DPQ_FIRST( This->dp2->repliesExpected ); reply;
         reply = DPQ_NEXT( reply->repliesExpected ) )
    {
        for( i = 0; i < reply->replyExpected.expectedReplyCount; ++i )
        {
            if( reply->replyExpected.expectedReplies[ i ] == commandId )
            {
                DPQ_REMOVE( This->dp2->repliesExpected, reply, repliesExpected );
                LeaveCriticalSection( &This->lock );
                return reply;
            }
        }
    }

    LeaveCriticalSection( &This->lock );

    return NULL;
}

static
void DP_MSG_UnlinkReplyStruct( IDirectPlayImpl *This, DP_MSG_REPLY_STRUCT_LIST *lpReplyStructList )
{
  EnterCriticalSection( &This->lock );
    DPQ_REMOVE( This->dp2->repliesExpected, lpReplyStructList, repliesExpected );
  LeaveCriticalSection( &This->lock );
}

static
void DP_MSG_CleanReplyStruct( LPDP_MSG_REPLY_STRUCT_LIST lpReplyStructList,
                              LPVOID* lplpReplyMsg, LPDWORD lpdwMsgBodySize, void **replyMsgHeader )
{
  CloseHandle( lpReplyStructList->replyExpected.hReceipt );

  *lplpReplyMsg    = lpReplyStructList->replyExpected.lpReplyMsg;
  *lpdwMsgBodySize = lpReplyStructList->replyExpected.dwMsgBodySize;
  *replyMsgHeader  = lpReplyStructList->replyExpected.replyMsgHeader;
}

DWORD DP_MSG_ComputeMessageSize( SGBUFFER *buffers, DWORD bufferCount )
{
  DWORD size = 0;
  DWORD i;
  for ( i = 0; i < bufferCount; ++i )
    size += buffers[ i ].len;
  return size;
}

HRESULT DP_MSG_SendRequestPlayerId( IDirectPlayImpl *This, DWORD dwFlags, DPID *lpdpidAllocatedId )
{
  LPVOID                     lpMsg;
  void                      *msgHeader;
  DPMSG_REQUESTNEWPLAYERID   msgBody;
  DWORD                      dwMsgSize;
  HRESULT                    hr = DP_OK;

  /* Compose dplay message envelope */
  msgBody.envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  msgBody.envelope.wCommandId = DPMSGCMD_REQUESTNEWPLAYERID;
  msgBody.envelope.wVersion   = DPMSGVER_DP6;

  /* Compose the body of the message */
  msgBody.dwFlags = dwFlags;

  /* Send the message */
  {
    WORD replyCommand = DPMSGCMD_NEWPLAYERIDREPLY;
    SGBUFFER buffers[ 2 ] = { 0 };
    DPSP_SENDEXDATA data = { 0 };

    buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
    buffers[ 1 ].len = sizeof( msgBody );
    buffers[ 1 ].pData = (UCHAR *) &msgBody;

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = DPSEND_GUARANTEED;
    data.idPlayerTo     = 0; /* Name server */
    data.idPlayerFrom   = 0; /* Sending from DP */
    data.lpSendBuffers  = buffers;
    data.cBuffers       = ARRAYSIZE( buffers );
    data.dwMessageSize  = DP_MSG_ComputeMessageSize( data.lpSendBuffers, data.cBuffers );
    data.bSystemMessage = TRUE; /* Allow reply to be sent */

    TRACE( "Asking for player id w/ dwFlags 0x%08lx\n",
           msgBody.dwFlags );

    hr = DP_MSG_ExpectReply( This, &data, DPMSG_DEFAULT_WAIT_TIME, &replyCommand, 1,
                             &lpMsg, &dwMsgSize, &msgHeader );
  }

  /* Need to examine the data and extract the new player id */
  if( SUCCEEDED(hr) )
  {
    LPCDPMSG_NEWPLAYERIDREPLY lpcReply;

    if ( dwMsgSize < sizeof( DPMSG_NEWPLAYERIDREPLY ) )
    {
      free( msgHeader );
      free( lpMsg );
      return DPERR_GENERIC;
    }
    lpcReply = lpMsg;

    if ( FAILED( lpcReply->result ) )
    {
      hr = lpcReply->result;
      free( msgHeader );
      free( lpMsg );
      return hr;
    }

    *lpdpidAllocatedId = lpcReply->dpidNewPlayerId;

    TRACE( "Received reply for id = 0x%08lx\n", lpcReply->dpidNewPlayerId );

    free( msgHeader );
    free( lpMsg );
  }

  return hr;
}

static HRESULT DP_MSG_ReadString( char *data, DWORD *inoutOffset, DWORD maxSize, WCHAR **string )
{
    DWORD offset = *inoutOffset;
    DWORD maxLength;
    DWORD length;

    if( maxSize - offset < sizeof( WCHAR ) )
      return DPERR_GENERIC;

    maxLength = (maxSize - offset) / sizeof( WCHAR );
    length = wcsnlen( (WCHAR *) &data[ offset ], maxLength );
    if( length == maxLength )
      return DPERR_GENERIC;

    *string = (WCHAR *) &data[ offset ];
    *inoutOffset = offset + (length + 1) * sizeof( WCHAR );

    return DP_OK;
}

static HRESULT DP_MSG_ReadSizedString( char *data, DWORD *inoutOffset, DWORD maxSize, DWORD size,
                                       WCHAR **string )
{
  DWORD length = size / sizeof( WCHAR ) - 1;
  DWORD offset = *inoutOffset;

  if( maxSize - offset < size )
    return DPERR_GENERIC;

  if ( ((WCHAR *) &data[ offset ])[ length ] != L'\0' )
    return DPERR_GENERIC;

  *string = (WCHAR *) &data[ offset ];
  *inoutOffset = offset + size;

  return DP_OK;
}

static HRESULT DP_MSG_ReadInteger( char *data, DWORD *inoutOffset, DWORD maxSize, DWORD size,
                                   DWORD *value )
{
  DWORD offset = *inoutOffset;

  switch ( size )
  {
  case 0:
    *value = 0;
    break;
  case 1:
    if( maxSize - offset < 1 )
      return DPERR_GENERIC;
    *value = *(BYTE *) &data[ offset ];
    *inoutOffset = offset + 1;
    break;
  case 2:
    if( maxSize - offset < 2 )
      return DPERR_GENERIC;
    *value = *(WORD *) &data[ offset ];
    *inoutOffset = offset + 2;
    break;
  case 3:
    if( maxSize - offset < 4 )
      return DPERR_GENERIC;
    *value = *(DWORD *) &data[ offset ];
    *inoutOffset = offset + 4;
    break;
  }

  return DP_OK;
}

HRESULT DP_MSG_ReadPackedPlayer( char *data, DWORD *inoutOffset, DWORD maxSize,
                                 DPPLAYERINFO *playerInfo )
{
  DPLAYI_PACKEDPLAYER *packedPlayer;
  DWORD offset = *inoutOffset;
  HRESULT hr;

  memset( playerInfo, 0, sizeof( DPPLAYERINFO ) );

  if( maxSize - offset < sizeof( DPLAYI_PACKEDPLAYER ) )
    return DPERR_GENERIC;
  packedPlayer = (DPLAYI_PACKEDPLAYER *) &data[ offset ];
  offset += sizeof( DPLAYI_PACKEDPLAYER );

  playerInfo->flags = packedPlayer->flags;
  playerInfo->id = packedPlayer->id;
  playerInfo->versionOrSystemPlayerId = packedPlayer->version;
  playerInfo->playerDataLength = packedPlayer->playerDataLength;
  playerInfo->spDataLength = packedPlayer->spDataLength;
  playerInfo->playerCount = packedPlayer->playerCount;
  playerInfo->parentId = packedPlayer->parentId;

  if( packedPlayer->shortNameLength )
  {
    hr = DP_MSG_ReadSizedString( data, &offset, maxSize, packedPlayer->shortNameLength,
                                 &playerInfo->name.lpszShortName );
    if( FAILED( hr ) )
      return hr;
  }

  if( packedPlayer->longNameLength )
  {
    hr = DP_MSG_ReadSizedString( data, &offset, maxSize, packedPlayer->longNameLength,
                                 &playerInfo->name.lpszLongName );
    if( FAILED( hr ) )
      return hr;
  }

  if( playerInfo->spDataLength )
  {
    if( maxSize - offset < playerInfo->spDataLength )
      return DPERR_GENERIC;
    playerInfo->spData = &data[ offset ];
    offset += playerInfo->spDataLength;
  }

  if( playerInfo->playerDataLength )
  {
    if( maxSize - offset < playerInfo->playerDataLength )
      return DPERR_GENERIC;
    playerInfo->playerData = &data[ offset ];
    offset += playerInfo->playerDataLength;
  }

  if( playerInfo->playerCount )
  {
    if( UINT_MAX / sizeof( DPID ) < playerInfo->playerCount )
      return DPERR_GENERIC;
    if( maxSize - offset < playerInfo->playerCount * sizeof( DPID ) )
      return DPERR_GENERIC;
    playerInfo->playerIds = (DPID *) &data[ offset ];
    offset += playerInfo->playerCount * sizeof( DPID );
  }

  *inoutOffset = offset;

  return S_OK;
}

static HRESULT DP_MSG_ReadSuperPackedPlayer( char *data, DWORD *inoutOffset, DWORD maxSize,
                                             DPPLAYERINFO *playerInfo )
{
  DPLAYI_SUPERPACKEDPLAYER *packedPlayer;
  DWORD offset = *inoutOffset;
  HRESULT hr;

  memset( playerInfo, 0, sizeof( DPPLAYERINFO ) );

  if( maxSize - offset < sizeof( DPLAYI_SUPERPACKEDPLAYER ) )
    return DPERR_GENERIC;
  packedPlayer = (DPLAYI_SUPERPACKEDPLAYER *) &data[ offset ];
  offset += sizeof( DPLAYI_SUPERPACKEDPLAYER );

  playerInfo->flags = packedPlayer->flags;
  playerInfo->id = packedPlayer->id;
  playerInfo->versionOrSystemPlayerId = packedPlayer->versionOrSystemPlayerId;

  if( packedPlayer->infoMask & DPLAYI_SUPERPACKEDPLAYER_SHORT_NAME_PRESENT )
  {
    hr = DP_MSG_ReadString( data, &offset, maxSize, &playerInfo->name.lpszShortName );
    if ( FAILED( hr ) )
      return hr;
  }

  if( packedPlayer->infoMask & DPLAYI_SUPERPACKEDPLAYER_LONG_NAME_PRESENT )
  {
    hr = DP_MSG_ReadString( data, &offset, maxSize, &playerInfo->name.lpszLongName );
    if ( FAILED( hr ) )
      return hr;
  }

  hr = DP_MSG_ReadInteger( data, &offset, maxSize,
                           DPLAYI_SUPERPACKEDPLAYER_PLAYER_DATA_LENGTH_SIZE( packedPlayer->infoMask ),
                           &playerInfo->playerDataLength );
  if ( FAILED( hr ) )
    return hr;

  if( playerInfo->playerDataLength )
  {
    if( maxSize - offset < playerInfo->playerDataLength )
      return DPERR_GENERIC;
    playerInfo->playerData = &data[ offset ];
    offset += playerInfo->playerDataLength;
  }

  hr = DP_MSG_ReadInteger( data, &offset, maxSize,
                           DPLAYI_SUPERPACKEDPLAYER_SP_DATA_LENGTH_SIZE( packedPlayer->infoMask ),
                           &playerInfo->spDataLength );
  if ( FAILED( hr ) )
    return hr;

  if( playerInfo->spDataLength )
  {
    if( maxSize - offset < playerInfo->spDataLength )
      return DPERR_GENERIC;
    playerInfo->spData = &data[ offset ];
    offset += playerInfo->spDataLength;
  }

  hr = DP_MSG_ReadInteger( data, &offset, maxSize,
                           DPLAYI_SUPERPACKEDPLAYER_PLAYER_COUNT_SIZE( packedPlayer->infoMask ),
                           &playerInfo->playerCount );
  if ( FAILED( hr ) )
    return hr;

  if( playerInfo->playerCount )
  {
    if( UINT_MAX / sizeof( DPID ) < playerInfo->playerCount )
      return DPERR_GENERIC;
    if( maxSize - offset < playerInfo->playerCount * sizeof( DPID ) )
      return DPERR_GENERIC;
    playerInfo->playerIds = (DPID *) &data[ offset ];
    offset += playerInfo->playerCount * sizeof( DPID );
  }

  if( packedPlayer->infoMask & DPLAYI_SUPERPACKEDPLAYER_PARENT_ID_PRESENT )
  {
    if( maxSize - offset < sizeof( DPID ) )
      return DPERR_GENERIC;
    playerInfo->parentId = *(DPID *) &data[ offset ];
    offset += sizeof( DPID );
  }

  hr = DP_MSG_ReadInteger( data, &offset, maxSize,
                           DPLAYI_SUPERPACKEDPLAYER_SHORTCUT_COUNT_SIZE( packedPlayer->infoMask ),
                           &playerInfo->shortcutCount );
  if ( FAILED( hr ) )
    return hr;

  if( playerInfo->shortcutCount )
  {
    if( UINT_MAX / sizeof( DPID ) < playerInfo->shortcutCount )
      return DPERR_GENERIC;
    if( maxSize - offset < playerInfo->shortcutCount * sizeof( DPID ) )
      return DPERR_GENERIC;
    playerInfo->shortcutIds = (DPID *) &data[ offset ];
    offset += playerInfo->shortcutCount * sizeof( DPID );
  }

  *inoutOffset = offset;

  return S_OK;
}

HRESULT DP_MSG_ForwardPlayerCreation( IDirectPlayImpl *This, DPID dpidServer, WCHAR *password )
{
  LPVOID                   lpMsg;
  DPMSG_FORWARDADDPLAYER   msgBody;
  DWORD                    dwMsgSize;
  DPLAYI_PACKEDPLAYER      playerInfo;
  void                    *spPlayerData;
  DWORD                    spPlayerDataSize;
  void                    *msgHeader;
  HRESULT                  hr;

  hr = IDirectPlaySP_GetSPPlayerData( This->dp2->spData.lpISP, dpidServer,
                                      &spPlayerData, &spPlayerDataSize, DPSET_REMOTE );
  if( FAILED( hr ) )
    return hr;

  msgBody.envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  msgBody.envelope.wCommandId = DPMSGCMD_FORWARDADDPLAYER;
  msgBody.envelope.wVersion   = DPMSGVER_DP6;
  msgBody.toId                = 0;
  msgBody.playerId            = dpidServer;
  msgBody.groupId             = 0;
  msgBody.createOffset        = sizeof( msgBody );
  msgBody.passwordOffset      = sizeof( msgBody ) + sizeof( playerInfo ) + spPlayerDataSize;

  playerInfo.size             = sizeof( playerInfo ) + spPlayerDataSize;
  playerInfo.flags            = DPLAYI_PLAYER_SYSPLAYER | DPLAYI_PLAYER_PLAYERLOCAL;
  playerInfo.id               = dpidServer;
  playerInfo.shortNameLength  = 0;
  playerInfo.longNameLength   = 0;
  playerInfo.spDataLength     = spPlayerDataSize;
  playerInfo.playerDataLength = 0;
  playerInfo.playerCount      = 0;
  playerInfo.systemPlayerId   = dpidServer;
  playerInfo.fixedSize        = sizeof( playerInfo );
  playerInfo.version          = DPMSGVER_DP6;
  playerInfo.parentId         = 0;

  /* Send the message */
  {
    WORD replyCommands[] = { DPMSGCMD_GETNAMETABLEREPLY, DPMSGCMD_SUPERENUMPLAYERSREPLY,
                             DPMSGCMD_FORWARDADDPLAYERNACK };
    SGBUFFER buffers[ 6 ] = { 0 };
    DPSP_SENDEXDATA data = { 0 };

    buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
    buffers[ 1 ].len = sizeof( msgBody );
    buffers[ 1 ].pData = (UCHAR *) &msgBody;
    buffers[ 2 ].len = sizeof( playerInfo );
    buffers[ 2 ].pData = (UCHAR *) &playerInfo;
    buffers[ 3 ].len = spPlayerDataSize;
    buffers[ 3 ].pData = (UCHAR *) spPlayerData;
    buffers[ 4 ].len = (password ? (lstrlenW( password ) + 1) : 1) * sizeof( WCHAR );
    buffers[ 4 ].pData = (UCHAR *) (password ? password : L"");
    buffers[ 5 ].len = sizeof( This->dp2->lpSessionDesc->dwReserved1 );
    buffers[ 5 ].pData = (UCHAR *) &This->dp2->lpSessionDesc->dwReserved1;

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = DPSEND_GUARANTEED;
    data.idPlayerTo     = 0; /* Name server */
    data.idPlayerFrom   = dpidServer; /* Sending from session server */
    data.lpSendBuffers  = buffers;
    data.cBuffers       = ARRAYSIZE( buffers );
    data.dwMessageSize  = DP_MSG_ComputeMessageSize( data.lpSendBuffers, data.cBuffers );
    data.bSystemMessage = TRUE; /* Allow reply to be sent */

    TRACE( "Sending forward player request with 0x%08lx\n", dpidServer );

    hr = DP_MSG_ExpectReply( This, &data,
                             DPMSG_WAIT_60_SECS,
                             replyCommands, ARRAYSIZE( replyCommands ),
                             &lpMsg, &dwMsgSize, &msgHeader );
  }

  /* Need to examine the data and extract the new player id */
  if( lpMsg != NULL )
  {
    DPMSG_SENDENVELOPE *envelope;

    if( dwMsgSize < sizeof( DPMSG_SENDENVELOPE ) )
    {
      free( msgHeader );
      free( lpMsg );
      return DPERR_GENERIC;
    }
    envelope = (DPMSG_SENDENVELOPE *) lpMsg;

    if( envelope->wCommandId == DPMSGCMD_SUPERENUMPLAYERSREPLY )
    {
      DPSP_MSG_SUPERENUMPLAYERSREPLY *enumPlayersReply;
      DWORD offset;
      int i;

      if( dwMsgSize < sizeof( DPSP_MSG_SUPERENUMPLAYERSREPLY ) )
      {
        free( msgHeader );
        free( lpMsg );
        return DPERR_GENERIC;
      }
      enumPlayersReply = (DPSP_MSG_SUPERENUMPLAYERSREPLY *) envelope;

      offset = enumPlayersReply->packedOffset;
      for( i = 0; i < enumPlayersReply->playerCount; ++i )
      {
        DPPLAYERINFO playerInfo;

        hr = DP_MSG_ReadSuperPackedPlayer( (char *) enumPlayersReply, &offset, dwMsgSize,
                                           &playerInfo );
        if( FAILED( hr ) )
        {
          free( msgHeader );
          free( lpMsg );
          return hr;
        }

        if ( playerInfo.id == dpidServer )
          continue;

        hr = DP_CreatePlayer( This, msgHeader, &playerInfo.id, &playerInfo.name,
            playerInfo.playerData, playerInfo.playerDataLength, playerInfo.spData,
            playerInfo.spDataLength, playerInfo.flags & ~DPLAYI_PLAYER_PLAYERLOCAL, NULL,
            NULL, FALSE );
        if( FAILED( hr ) )
        {
          free( msgHeader );
          free( lpMsg );
          return hr;
        }
      }
      for( i = 0; i < enumPlayersReply->groupCount; ++i )
      {
        DPPLAYERINFO playerInfo;
        int j;

        hr = DP_MSG_ReadSuperPackedPlayer( (char *) enumPlayersReply, &offset, dwMsgSize,
                                           &playerInfo );
        if( FAILED( hr ) )
        {
          free( msgHeader );
          free( lpMsg );
          return hr;
        }

        hr = DP_CreateGroup( This, msgHeader, &playerInfo.id, &playerInfo.name,
                             playerInfo.playerData, playerInfo.playerDataLength,
                             playerInfo.flags & ~DPLAYI_PLAYER_PLAYERLOCAL, playerInfo.parentId,
                             FALSE );
        if( FAILED( hr ) )
        {
          free( msgHeader );
          free( lpMsg );
          return hr;
        }

        for( j = 0; j < playerInfo.playerCount; ++j )
        {
          hr = DP_AddPlayerToGroup( This, playerInfo.id, playerInfo.playerIds[ j ] );
          if( FAILED( hr ) )
          {
            free( msgHeader );
            free( lpMsg );
            return hr;
          }
        }
      }
    }
    else if( envelope->wCommandId == DPMSGCMD_GETNAMETABLEREPLY )
    {
      FIXME( "Name Table reply received: stub\n" );
    }
    else if( envelope->wCommandId == DPMSGCMD_FORWARDADDPLAYERNACK )
    {
      DPSP_MSG_ADDFORWARDREPLY *addForwardReply;

      if( dwMsgSize < sizeof( DPSP_MSG_ADDFORWARDREPLY ) )
      {
        free( msgHeader );
        free( lpMsg );
        return DPERR_GENERIC;
      }
      addForwardReply = (DPSP_MSG_ADDFORWARDREPLY *) envelope;

      hr = addForwardReply->error;

      free( msgHeader );
      free( lpMsg );

      return hr;
    }
    free( msgHeader );
    free( lpMsg );
  }

  return hr;
}

HRESULT DP_MSG_SendCreatePlayer( IDirectPlayImpl *This, DPID toId, DPID id, DWORD flags,
                                 DPNAME *name, void *playerData, DWORD playerDataSize,
                                 DPID systemPlayerId )
{
  DPSP_SENDTOGROUPEXDATA sendData;
  DPLAYI_PACKEDPLAYER playerInfo;
  SGBUFFER buffers[ 8 ] = { 0 };
  UCHAR reserved[ 6 ] = { 0 };
  DWORD shortNameLength = 0;
  DWORD longNameLength = 0;
  DPMSG_CREATESESSION msg;
  DWORD spPlayerDataSize;
  void *spPlayerData;
  HRESULT hr;

  hr = IDirectPlaySP_GetSPPlayerData( This->dp2->spData.lpISP, id, &spPlayerData, &spPlayerDataSize,
                                      DPSET_REMOTE );
  if( FAILED( hr ) )
    return hr;

  if ( name->lpszShortName )
    shortNameLength = ( lstrlenW( name->lpszShortName ) + 1 ) * sizeof( WCHAR );

  if ( name->lpszLongName )
    longNameLength = ( lstrlenW( name->lpszLongName ) + 1 ) * sizeof( WCHAR );

  msg.envelope.dwMagic = DPMSGMAGIC_DPLAYMSG;
  msg.envelope.wCommandId = DPMSGCMD_CREATESESSION;
  msg.envelope.wVersion = DPMSGVER_DP6;
  msg.toId = 0;
  msg.playerId = id;
  msg.groupId = 0;
  msg.createOffset = sizeof( DPMSG_CREATESESSION );
  msg.passwordOffset = 0;

  playerInfo.size = sizeof( DPLAYI_PACKEDPLAYER ) + shortNameLength + longNameLength
                  + spPlayerDataSize + playerDataSize;
  playerInfo.flags = flags;
  playerInfo.id = id;
  playerInfo.shortNameLength = shortNameLength;
  playerInfo.longNameLength = longNameLength;
  playerInfo.spDataLength = spPlayerDataSize;
  playerInfo.playerDataLength = playerDataSize;
  playerInfo.playerCount = 0;
  playerInfo.systemPlayerId = systemPlayerId;
  playerInfo.fixedSize = sizeof( DPLAYI_PACKEDPLAYER );
  playerInfo.version = DPMSGVER_DP6;
  playerInfo.parentId = 0;

  buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
  buffers[ 0 ].pData = NULL;
  buffers[ 1 ].len = sizeof( msg );
  buffers[ 1 ].pData = (UCHAR *)&msg;
  buffers[ 2 ].len = sizeof( playerInfo );
  buffers[ 2 ].pData = (UCHAR *)&playerInfo;
  buffers[ 3 ].len = shortNameLength;
  buffers[ 3 ].pData = (UCHAR *)name->lpszShortName;
  buffers[ 4 ].len = longNameLength;
  buffers[ 4 ].pData = (UCHAR *)name->lpszLongName;
  buffers[ 5 ].len = spPlayerDataSize;
  buffers[ 5 ].pData = spPlayerData;
  buffers[ 6 ].len = playerDataSize;
  buffers[ 6 ].pData = playerData;
  buffers[ 7 ].len = sizeof( reserved );
  buffers[ 7 ].pData = reserved;

  sendData.lpISP = This->dp2->spData.lpISP;
  sendData.dwFlags = DPSEND_GUARANTEED;
  sendData.idGroupTo = toId;
  sendData.idPlayerFrom = This->dp2->systemPlayerId;
  sendData.lpSendBuffers = buffers;
  sendData.cBuffers = ARRAYSIZE( buffers );
  sendData.dwMessageSize = DP_MSG_ComputeMessageSize( sendData.lpSendBuffers, sendData.cBuffers );
  sendData.dwPriority = 0;
  sendData.dwTimeout = 0;
  sendData.lpDPContext = NULL;
  sendData.lpdwSPMsgID = NULL;

  hr = (*This->dp2->spData.lpCB->SendToGroupEx)( &sendData );
  if( FAILED( hr ) )
  {
    ERR( "SendToGroupEx failed: %s\n", DPLAYX_HresultToString( hr ) );
    return hr;
  }

  return DP_OK;
}

HRESULT DP_MSG_SendAddPlayerToGroup( IDirectPlayImpl *This, DPID toId, DPID playerId, DPID groupId )
{
  DPSP_SENDTOGROUPEXDATA sendData;
  DPSP_MSG_ADDPLAYERTOGROUP msg;
  SGBUFFER buffers[ 2 ] = { 0 };
  HRESULT hr;

  msg.envelope.dwMagic = DPMSGMAGIC_DPLAYMSG;
  msg.envelope.wCommandId = DPMSGCMD_ADDPLAYERTOGROUP;
  msg.envelope.wVersion = DPMSGVER_DP6;
  msg.toId = 0;
  msg.playerId = playerId;
  msg.groupId = groupId;
  msg.createOffset = 0;
  msg.passwordOffset = 0;

  buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
  buffers[ 0 ].pData = NULL;
  buffers[ 1 ].len = sizeof( msg );
  buffers[ 1 ].pData = (UCHAR *)&msg;

  sendData.lpISP = This->dp2->spData.lpISP;
  sendData.dwFlags = DPSEND_GUARANTEED;
  sendData.idGroupTo = toId;
  sendData.idPlayerFrom = This->dp2->systemPlayerId;
  sendData.lpSendBuffers = buffers;
  sendData.cBuffers = ARRAYSIZE( buffers );
  sendData.dwMessageSize = DP_MSG_ComputeMessageSize( sendData.lpSendBuffers, sendData.cBuffers );
  sendData.dwPriority = 0;
  sendData.dwTimeout = 0;
  sendData.lpDPContext = NULL;
  sendData.lpdwSPMsgID = NULL;

  hr = (*This->dp2->spData.lpCB->SendToGroupEx)( &sendData );
  if( FAILED( hr ) )
  {
    ERR( "SendToGroupEx failed: %s\n", DPLAYX_HresultToString( hr ) );
    return hr;
  }

  return DP_OK;
}

HRESULT DP_MSG_SendPingReply( IDirectPlayImpl *This, DPID toId, DPID fromId, DWORD tickCount )
{
  SGBUFFER buffers[ 2 ] = { 0 };
  DPSP_SENDEXDATA sendData;
  DPSP_MSG_PING msg;
  HRESULT hr;

  msg.envelope.dwMagic = DPMSGMAGIC_DPLAYMSG;
  msg.envelope.wCommandId = DPMSGCMD_PINGREPLY;
  msg.envelope.wVersion = DPMSGVER_DP6;
  msg.fromId = fromId;
  msg.tickCount = tickCount;

  buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
  buffers[ 0 ].pData = NULL;
  buffers[ 1 ].len = sizeof( msg );
  buffers[ 1 ].pData = (UCHAR *)&msg;

  sendData.lpISP = This->dp2->spData.lpISP;
  sendData.dwFlags = 0;
  sendData.idPlayerTo = toId;
  sendData.idPlayerFrom = This->dp2->systemPlayerId;
  sendData.lpSendBuffers = buffers;
  sendData.cBuffers = ARRAYSIZE( buffers );
  sendData.dwMessageSize = DP_MSG_ComputeMessageSize( sendData.lpSendBuffers, sendData.cBuffers );
  sendData.dwPriority = 0;
  sendData.dwTimeout = 0;
  sendData.lpDPContext = NULL;
  sendData.lpdwSPMsgID = NULL;
  sendData.bSystemMessage = TRUE;

  hr = (*This->dp2->spData.lpCB->SendEx)( &sendData );
  if( FAILED( hr ) )
  {
    ERR( "Send failed: %s\n", DPLAYX_HresultToString( hr ) );
    return hr;
  }

  return DP_OK;
}

HRESULT DP_MSG_SendAddForwardAck( IDirectPlayImpl *This, DPID id )
{
  SGBUFFER buffers[ 2 ] = { 0 };
  DPSP_MSG_ADDFORWARDACK msg;
  DPSP_SENDEXDATA sendData;
  HRESULT hr;

  msg.envelope.dwMagic = DPMSGMAGIC_DPLAYMSG;
  msg.envelope.wCommandId = DPMSGCMD_ADDFORWARDACK;
  msg.envelope.wVersion = DPMSGVER_DP6;
  msg.id = id;

  buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
  buffers[ 0 ].pData = NULL;
  buffers[ 1 ].len = sizeof( msg );
  buffers[ 1 ].pData = (UCHAR *)&msg;

  sendData.lpISP = This->dp2->spData.lpISP;
  sendData.dwFlags = DPSEND_GUARANTEED;
  sendData.idPlayerTo = 0;
  sendData.idPlayerFrom = This->dp2->systemPlayerId;
  sendData.lpSendBuffers = buffers;
  sendData.cBuffers = ARRAYSIZE( buffers );
  sendData.dwMessageSize = DP_MSG_ComputeMessageSize( sendData.lpSendBuffers, sendData.cBuffers );
  sendData.dwPriority = 0;
  sendData.dwTimeout = 0;
  sendData.lpDPContext = NULL;
  sendData.lpdwSPMsgID = NULL;
  sendData.bSystemMessage = TRUE;

  hr = (*This->dp2->spData.lpCB->SendEx)( &sendData );
  if( FAILED( hr ) )
  {
    ERR( "Send failed: %s\n", DPLAYX_HresultToString( hr ) );
    return hr;
  }

  return DP_OK;
}

/* Queue up a structure indicating that we want a reply of type wReplyCommandId. DPlay does
 * not seem to offer any way of uniquely differentiating between replies of the same type
 * relative to the request sent. There is an implicit assumption that there will be no
 * ordering issues on sends and receives from the opposite machine. No wonder MS is not
 * a networking company.
 */
static HRESULT DP_MSG_ExpectReply( IDirectPlayImpl *This, DPSP_SENDEXDATA *lpData, DWORD dwWaitTime,
        WORD *replyCommandIds, DWORD replyCommandIdCount, void **lplpReplyMsg,
        DWORD *lpdwMsgBodySize, void **replyMsgHeader )
{
  HRESULT                  hr;
  HANDLE                   hMsgReceipt;
  DP_MSG_REPLY_STRUCT_LIST replyStructList;
  DWORD                    dwWaitReturn;

  /* Setup for receipt */
  hMsgReceipt = DP_MSG_BuildAndLinkReplyStruct( This, &replyStructList,
                                                replyCommandIds, replyCommandIdCount );

  TRACE( "Sending msg and expecting reply within %lu ticks\n", dwWaitTime );
  hr = (*This->dp2->spData.lpCB->SendEx)( lpData );

  if( FAILED(hr) )
  {
    ERR( "Send failed: %s\n", DPLAYX_HresultToString( hr ) );
    DP_MSG_UnlinkReplyStruct( This, &replyStructList );
    DP_MSG_CleanReplyStruct( &replyStructList, lplpReplyMsg, lpdwMsgBodySize, replyMsgHeader );
    return hr;
  }

  /* The reply message will trigger the hMsgReceipt event effectively switching
   * control back to this thread. See DP_MSG_ReplyReceived.
   */
  dwWaitReturn = WaitForSingleObject( hMsgReceipt, dwWaitTime );
  if( dwWaitReturn != WAIT_OBJECT_0 )
  {
    ERR( "Wait failed 0x%08lx\n", dwWaitReturn );
    DP_MSG_UnlinkReplyStruct( This, &replyStructList );
    DP_MSG_CleanReplyStruct( &replyStructList, lplpReplyMsg, lpdwMsgBodySize, replyMsgHeader );
    return DPERR_TIMEOUT;
  }

  /* Clean Up */
  DP_MSG_CleanReplyStruct( &replyStructList, lplpReplyMsg, lpdwMsgBodySize, replyMsgHeader );
  return DP_OK;
}

/* Determine if there is a matching request for this incoming message and then copy
 * all important data. It is quite silly to have to copy the message, but the documents
 * indicate that a copy is taken. Silly really.
 */
void DP_MSG_ReplyReceived( IDirectPlayImpl *This, WORD wCommandId, const void *lpcMsgBody,
        DWORD dwMsgBodySize, const void *msgHeader )
{
  LPDP_MSG_REPLY_STRUCT_LIST lpReplyList;

#if 0
  if( wCommandId == DPMSGCMD_FORWARDADDPLAYER )
  {
    DebugBreak();
  }
#endif

  /* Find, and immediately remove (to avoid double triggering), the appropriate entry. Call locked to
   * avoid problems.
   */
  lpReplyList = DP_MSG_FindAndUnlinkReplyStruct( This, wCommandId );

  if( lpReplyList != NULL )
  {
    lpReplyList->replyExpected.dwMsgBodySize = dwMsgBodySize;
    lpReplyList->replyExpected.lpReplyMsg = malloc( dwMsgBodySize );
    lpReplyList->replyExpected.replyMsgHeader = malloc( This->dp2->spData.dwSPHeaderSize );
    CopyMemory( lpReplyList->replyExpected.lpReplyMsg,
                lpcMsgBody, dwMsgBodySize );
    CopyMemory( lpReplyList->replyExpected.replyMsgHeader,
                msgHeader, This->dp2->spData.dwSPHeaderSize );

    /* Signal the thread which sent the message that it has a reply */
    SetEvent( lpReplyList->replyExpected.hReceipt );
  }
  else
  {
    ERR( "No receipt event set - only expecting in reply mode\n" );
    DebugBreak();
  }
}

void DP_MSG_ToSelf( IDirectPlayImpl *This, DPID dpidSelf )
{
  LPVOID                   lpMsg;
  void                    *msgHeader;
  DPMSG_SENDENVELOPE       msgBody;
  DWORD                    dwMsgSize;

  /* Compose dplay message envelope */
  msgBody.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  msgBody.wCommandId = DPMSGCMD_JUSTENVELOPE;
  msgBody.wVersion   = DPMSGVER_DP6;

  /* Send the message to ourselves */
  {
    WORD replyCommand = DPMSGCMD_JUSTENVELOPE;
    SGBUFFER buffers[ 2 ] = { 0 };
    DPSP_SENDEXDATA data = { 0 };

    buffers[ 0 ].len = This->dp2->spData.dwSPHeaderSize;
    buffers[ 1 ].len = sizeof( msgBody );
    buffers[ 1 ].pData = (UCHAR *) &msgBody;

    data.lpISP          = This->dp2->spData.lpISP;
    data.dwFlags        = 0;
    data.idPlayerTo     = dpidSelf; /* Sending to session server */
    data.idPlayerFrom   = 0; /* Sending from session server */
    data.lpSendBuffers  = buffers;
    data.cBuffers       = ARRAYSIZE( buffers );
    data.dwMessageSize  = DP_MSG_ComputeMessageSize( data.lpSendBuffers, data.cBuffers );
    data.bSystemMessage = TRUE; /* Allow reply to be sent */

    DP_MSG_ExpectReply( This, &data,
                        DPMSG_WAIT_5_SECS,
                        &replyCommand, 1,
                        &lpMsg, &dwMsgSize, &msgHeader );
  }
}
