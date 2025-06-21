/* DPLAYX.DLL name server implementation
 *
 * Copyright 2000-2001 - Peter Hunnisett
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

/* NOTE: Methods with the NS_ prefix are name server methods */

#include <stdarg.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/debug.h"
#include "mmsystem.h"

#include "dplayx_global.h"
#include "name_server.h"
#include "wine/dplaysp.h"
#include "dplayx_messages.h"
#include "dplayx_queue.h"

/* FIXME: Need to create a crit section, store and use it */

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* NS specific structures */
struct NSCacheData
{
  DPQ_ENTRY(NSCacheData) walkNext;
  DPQ_ENTRY(NSCacheData) next;

  LONG ref;

  DWORD dwTime; /* Time at which data was last known valid */
  LPDPSESSIONDESC2 data;
  LPDPSESSIONDESC2 dataA;

  LPVOID lpNSAddrHdr;

};
typedef struct NSCacheData NSCacheData, *lpNSCacheData;

struct NSCache
{
  lpNSCacheData present; /* keep track of what is to be looked at when walking */

  DPQ_HEAD(NSCacheData) walkFirst;
  DPQ_HEAD(NSCacheData) first;

  BOOL bNsIsLocal;
  LPVOID lpLocalAddrHdr;  /* FIXME: Not yet used */
  LPVOID lpRemoteAddrHdr; /* FIXME: Not yet used */
};
typedef struct NSCache NSCache, *lpNSCache;

/* Function prototypes */
static DPQ_DECL_DELETECB( cbReleaseNSNode, lpNSCacheData );

/* Name Server functions
 * ---------------------
 */
void NS_SetLocalComputerAsNameServer( LPCDPSESSIONDESC2 lpsd, LPVOID lpNSInfo )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  lpCache->bNsIsLocal = TRUE;
}

static DPQ_DECL_COMPARECB( cbUglyPig, GUID )
{
  return IsEqualGUID( elem1, elem2 );
}

/* Store the given NS remote address for future reference */
void NS_AddRemoteComputerAsNameServer( LPCVOID                      lpcNSAddrHdr,
                                       DWORD                        dwHdrSize,
                                       LPCDPMSG_ENUMSESSIONSREPLY   lpcMsg,
                                       DWORD                        msgSize,
                                       LPVOID                       lpNSInfo )
{
  lpNSCache     lpCache = (lpNSCache)lpNSInfo;
  lpNSCacheData lpCacheNode;
  DPSESSIONDESC2 dpsd;
  DWORD maxNameLength;
  DWORD nameLength;

  TRACE( "%p, %p, %p\n", lpcNSAddrHdr, lpcMsg, lpNSInfo );

  if ( msgSize < sizeof( DPMSG_ENUMSESSIONSREPLY ) + sizeof( WCHAR ) )
    return;

  maxNameLength = (msgSize - sizeof( DPMSG_ENUMSESSIONSREPLY )) / sizeof( WCHAR );
  nameLength = wcsnlen( (WCHAR *) (lpcMsg + 1), maxNameLength );
  if ( nameLength == maxNameLength )
    return;

  /* See if we can find this session. If we can, remove it as it's a dup */
  DPQ_REMOVE_ENTRY_CB( lpCache->first, next, data->guidInstance, cbUglyPig,
                       lpcMsg->sd.guidInstance, lpCacheNode );

  if( lpCacheNode != NULL )
  {
    TRACE( "Duplicate session entry for %s removed - updated version kept\n",
           debugstr_guid( &lpCacheNode->data->guidInstance ) );
    cbReleaseNSNode( lpCacheNode );
  }

  /* Add this to the list */
  lpCacheNode = calloc( 1, sizeof( *lpCacheNode ) );

  if( lpCacheNode == NULL )
  {
    ERR( "no memory for NS node\n" );
    return;
  }

  lpCacheNode->lpNSAddrHdr = malloc( dwHdrSize );
  CopyMemory( lpCacheNode->lpNSAddrHdr, lpcNSAddrHdr, dwHdrSize );

  dpsd = lpcMsg->sd;
  dpsd.lpszSessionName = (WCHAR *) (lpcMsg + 1);
  dpsd.lpszPassword = NULL;

  lpCacheNode->data = DP_DuplicateSessionDesc( &dpsd, FALSE, FALSE );

  if( lpCacheNode->data == NULL )
  {
    ERR( "no memory for SESSIONDESC2\n" );
    free( lpCacheNode );
    return;
  }

  lpCacheNode->dataA = DP_DuplicateSessionDesc( &dpsd, TRUE, FALSE );

  if( lpCacheNode->dataA == NULL )
  {
    ERR( "no memory for SESSIONDESC2\n" );
    free( lpCacheNode->data );
    free( lpCacheNode );
    return;
  }

  lpCacheNode->ref = 1;
  lpCacheNode->dwTime = timeGetTime();

  DPQ_INSERT(lpCache->first, lpCacheNode, next );

  /* Use this message as an opportunity to weed out any old sessions so
   * that we don't enum them again
   */
  NS_PruneSessionCache( lpNSInfo );
}

void NS_SetLocalAddr( LPVOID lpNSInfo, LPCVOID lpHdr, DWORD dwHdrSize )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  lpCache->lpLocalAddrHdr = malloc( dwHdrSize );

  CopyMemory( lpCache->lpLocalAddrHdr, lpHdr, dwHdrSize );
}

/* This function is responsible for sending a request for all other known
   nameservers to send us what sessions they have registered locally
 */
HRESULT NS_SendSessionRequestBroadcast( LPCGUID lpcGuid,
                                        DWORD dwFlags,
                                        WCHAR *password,
                                        const SPINITDATA *lpSpData )

{
  DPSP_ENUMSESSIONSDATA data;
  LPDPMSG_ENUMSESSIONSREQUEST lpMsg;
  DWORD passwordSize = 0;

  TRACE( "enumerating for guid %s\n", debugstr_guid( lpcGuid ) );

  if ( password )
    passwordSize = (wcslen( password ) + 1) * sizeof( WCHAR );

  data.dwMessageSize = lpSpData->dwSPHeaderSize + sizeof( *lpMsg ) + passwordSize;
  data.lpMessage = calloc( 1, data.dwMessageSize );
  data.lpISP = lpSpData->lpISP;
  data.bReturnStatus = (dwFlags & DPENUMSESSIONS_RETURNSTATUS) != 0;


  lpMsg = (LPDPMSG_ENUMSESSIONSREQUEST)(((BYTE*)data.lpMessage)+lpSpData->dwSPHeaderSize);

  /* Setup EnumSession request message */
  lpMsg->envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  lpMsg->envelope.wCommandId = DPMSGCMD_ENUMSESSIONSREQUEST;
  lpMsg->envelope.wVersion   = DPMSGVER_DP6;

  lpMsg->passwordOffset = password ? sizeof( DPMSG_ENUMSESSIONSREQUEST ) : 0;
  lpMsg->dwFlags        = dwFlags;

  lpMsg->guidApplication = *lpcGuid;

  memcpy( lpMsg + 1, password, passwordSize );

  return (lpSpData->lpCB->EnumSessions)( &data );
}

/* Delete a name server node which has been allocated on the heap */
static DPQ_DECL_DELETECB( cbReleaseNSNode, lpNSCacheData )
{
  LONG ref = InterlockedDecrement( &elem->ref );

  if ( ref )
    return;

  free( elem->dataA );
  free( elem->data );
  free( elem->lpNSAddrHdr );
  free( elem );
}

/* Render all data in a session cache invalid */
void NS_InvalidateSessionCache( LPVOID lpNSInfo )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  if( lpCache == NULL )
  {
    ERR( ": invalidate nonexistent cache\n" );
    return;
  }

  DPQ_DELETEQ( lpCache->first, next, lpNSCacheData, cbReleaseNSNode );

  lpCache->bNsIsLocal = FALSE;

}

/* Create and initialize a session cache */
BOOL NS_InitializeSessionCache( LPVOID* lplpNSInfo )
{
  lpNSCache lpCache = calloc( 1, sizeof( *lpCache ) );

  *lplpNSInfo = lpCache;

  if( lpCache == NULL )
  {
    return FALSE;
  }

  DPQ_INIT(lpCache->walkFirst);
  DPQ_INIT(lpCache->first);
  lpCache->present = NULL;

  lpCache->bNsIsLocal = FALSE;

  return TRUE;
}

/* Delete a session cache */
void NS_DeleteSessionCache( LPVOID lpNSInfo )
{
  NS_InvalidateSessionCache( (lpNSCache)lpNSInfo );
  NS_ResetSessionEnumeration( (lpNSCache)lpNSInfo );
}

/* Reinitialize the present pointer for this cache */
void NS_ResetSessionEnumeration( LPVOID lpNSInfo )
{
  NSCache *cache = (NSCache *) lpNSInfo;
  NSCacheData *data;

  DPQ_DELETEQ( cache->walkFirst, walkNext, lpNSCacheData, cbReleaseNSNode );

  for( data = DPQ_FIRST( cache->first ); data; data = DPQ_NEXT( data->next ) )
  {
    InterlockedIncrement( &data->ref );
    DPQ_INSERT( cache->walkFirst, data, walkNext );
  }

  ((lpNSCache)lpNSInfo)->present = ((lpNSCache)lpNSInfo)->walkFirst.lpQHFirst;
}

LPDPSESSIONDESC2 NS_WalkSessions( LPVOID lpNSInfo, void **spMessageHeader, BOOL ansi )
{
  LPDPSESSIONDESC2 lpSessionDesc;
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  /* Test for end of the list */
  if( lpCache->present == NULL )
  {
    return NULL;
  }

  lpSessionDesc = ansi ? lpCache->present->dataA : lpCache->present->data;

  if( spMessageHeader )
    *spMessageHeader = lpCache->present->lpNSAddrHdr;

  /* Advance tracking pointer */
  lpCache->present = lpCache->present->walkNext.lpQNext;

  return lpSessionDesc;
}

/* This method should check to see if there are any sessions which are
 * older than the criteria. If so, just delete that information.
 */
/* FIXME: This needs to be called by some periodic timer */
void NS_PruneSessionCache( LPVOID lpNSInfo )
{
  lpNSCache     lpCache = lpNSInfo;

  const DWORD dwPresentTime = timeGetTime();
  const DWORD dwPrunePeriod = DPMSG_WAIT_60_SECS; /* is 60 secs enough? */

  /* This silly little algorithm is based on the fact we keep entries in
   * the queue in a time based order. It also assumes that it is not possible
   * to wrap around over yourself (which is not unreasonable).
   * The if statements verify if the first entry in the queue is less
   * than dwPrunePeriod old depending on the "clock" roll over.
   */
  for( ;; )
  {
    lpNSCacheData lpFirstData;

    if( DPQ_IS_EMPTY(lpCache->first) )
    {
      /* Nothing to prune */
      break;
    }

    /* Deal with time in a wrap around safe manner - unsigned arithmetic.
     * Check the difference in time */
    if( (dwPresentTime - (DPQ_FIRST(lpCache->first)->dwTime)) < dwPrunePeriod )
    {
      /* First entry has not expired yet; don't prune */
      break;
    }

    lpFirstData = DPQ_FIRST(lpCache->first);
    DPQ_REMOVE( lpCache->first, DPQ_FIRST(lpCache->first), next );
    cbReleaseNSNode( lpFirstData );
  }

}

/* NAME SERVER Message stuff */
void NS_ReplyToEnumSessionsRequest( const void *lpcMsg, void **lplpReplyData, DWORD *lpdwReplySize,
        IDirectPlayImpl *lpDP )
{
  LPDPMSG_ENUMSESSIONSREPLY rmsg;
  DWORD dwVariableSize;
  DWORD dwVariableLen;
  /* LPCDPMSG_ENUMSESSIONSREQUEST msg = (LPDPMSG_ENUMSESSIONSREQUEST)lpcMsg; */

  /* FIXME: Should handle ANSI or WIDECHAR input. Currently just ANSI input */
  FIXME( ": few fixed + need to check request for response, might need UNICODE input ability.\n" );

  dwVariableLen = MultiByteToWideChar( CP_ACP, 0,
                                       lpDP->dp2->lpSessionDesc->lpszSessionNameA,
                                       -1, NULL, 0 );
  dwVariableSize = dwVariableLen * sizeof( WCHAR );

  *lpdwReplySize = lpDP->dp2->spData.dwSPHeaderSize +
                     sizeof( *rmsg ) + dwVariableSize;
  *lplpReplyData = calloc( 1, *lpdwReplySize );

  rmsg = (LPDPMSG_ENUMSESSIONSREPLY)( (BYTE*)*lplpReplyData +
                                             lpDP->dp2->spData.dwSPHeaderSize);

  rmsg->envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  rmsg->envelope.wCommandId = DPMSGCMD_ENUMSESSIONSREPLY;
  rmsg->envelope.wVersion   = DPMSGVER_DP6;

  CopyMemory( &rmsg->sd, lpDP->dp2->lpSessionDesc,
              lpDP->dp2->lpSessionDesc->dwSize );
  rmsg->dwUnknown = 0x0000005c;
  MultiByteToWideChar( CP_ACP, 0, lpDP->dp2->lpSessionDesc->lpszSessionNameA, -1,
                       (LPWSTR)(rmsg+1), dwVariableLen );
}
