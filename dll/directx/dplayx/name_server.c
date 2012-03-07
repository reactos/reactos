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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/* NOTE: Methods with the NS_ prefix are name server methods */

#include <stdarg.h>
#include <string.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "wine/unicode.h"
#include "wine/debug.h"
#include "mmsystem.h"

#include "dplayx_global.h"
#include "name_server.h"
#include "dplaysp.h"
#include "dplayx_messages.h"
#include "dplayx_queue.h"

/* FIXME: Need to create a crit section, store and use it */

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* NS specific structures */
struct NSCacheData
{
  DPQ_ENTRY(NSCacheData) next;

  DWORD dwTime; /* Time at which data was last known valid */
  LPDPSESSIONDESC2 data;

  LPVOID lpNSAddrHdr;

};
typedef struct NSCacheData NSCacheData, *lpNSCacheData;

struct NSCache
{
  lpNSCacheData present; /* keep track of what is to be looked at when walking */

  DPQ_HEAD(NSCacheData) first;

  BOOL bNsIsLocal;
  LPVOID lpLocalAddrHdr;  /* FIXME: Not yet used */
  LPVOID lpRemoteAddrHdr; /* FIXME: Not yet used */
};
typedef struct NSCache NSCache, *lpNSCache;

/* Function prototypes */
DPQ_DECL_DELETECB( cbDeleteNSNodeFromHeap, lpNSCacheData );

/* Name Server functions
 * ---------------------
 */
void NS_SetLocalComputerAsNameServer( LPCDPSESSIONDESC2 lpsd, LPVOID lpNSInfo )
{
#if 0
  /* FIXME: Remove this method? */
  DPLAYX_SetLocalSession( lpsd );
#endif
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  lpCache->bNsIsLocal = TRUE;
}

void NS_SetRemoteComputerAsNameServer( LPCDPSESSIONDESC2 lpsd, LPVOID lpNSInfo )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  lpCache->bNsIsLocal = FALSE;
}

static DPQ_DECL_COMPARECB( cbUglyPig, GUID )
{
  return IsEqualGUID( elem1, elem2 );
}

/* Store the given NS remote address for future reference */
/* FIXME: LPDPMSG_ENUMSESSIONSREPLY should be const */
void NS_AddRemoteComputerAsNameServer( LPCVOID                   lpcNSAddrHdr,
                                       DWORD                     dwHdrSize,
                                       LPDPMSG_ENUMSESSIONSREPLY lpMsg,
                                       LPVOID                    lpNSInfo )
{
  DWORD len;
  lpNSCache     lpCache = (lpNSCache)lpNSInfo;
  lpNSCacheData lpCacheNode;

  TRACE( "%p, %p, %p\n", lpcNSAddrHdr, lpMsg, lpNSInfo );

  /* See if we can find this session. If we can, remove it as it's a dup */
  DPQ_REMOVE_ENTRY_CB( lpCache->first, next, data->guidInstance, cbUglyPig,
                       lpMsg->sd.guidInstance, lpCacheNode );

  if( lpCacheNode != NULL )
  {
    TRACE( "Duplicate session entry for %s removed - updated version kept\n",
           debugstr_guid( &lpCacheNode->data->guidInstance ) );
    cbDeleteNSNodeFromHeap( lpCacheNode );
  }

  /* Add this to the list */
  lpCacheNode = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpCacheNode ) );

  if( lpCacheNode == NULL )
  {
    ERR( "no memory for NS node\n" );
    return;
  }

  lpCacheNode->lpNSAddrHdr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                                        dwHdrSize );
  CopyMemory( lpCacheNode->lpNSAddrHdr, lpcNSAddrHdr, dwHdrSize );

  lpCacheNode->data = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *(lpCacheNode->data) ) );

  if( lpCacheNode->data == NULL )
  {
    ERR( "no memory for SESSIONDESC2\n" );
    return;
  }

  CopyMemory( lpCacheNode->data, &lpMsg->sd, sizeof( *lpCacheNode->data ) );
  len = WideCharToMultiByte( CP_ACP, 0, (LPWSTR)(lpMsg+1), -1, NULL, 0, NULL, NULL );
  if ((lpCacheNode->data->lpszSessionNameA = HeapAlloc( GetProcessHeap(), 0, len )))
  {
      WideCharToMultiByte( CP_ACP, 0, (LPWSTR)(lpMsg+1), -1,
                           lpCacheNode->data->lpszSessionNameA, len, NULL, NULL );
  }

  lpCacheNode->dwTime = timeGetTime();

  DPQ_INSERT(lpCache->first, lpCacheNode, next );

  lpCache->present = lpCacheNode;

  /* Use this message as an opportunity to weed out any old sessions so
   * that we don't enum them again
   */
  NS_PruneSessionCache( lpNSInfo );
}

LPVOID NS_GetNSAddr( LPVOID lpNSInfo )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  FIXME( ":quick stub\n" );

  /* Ok. Cheat and don't search for the correct stuff just take the first.
   * FIXME: In the future how are we to know what is _THE_ enum we used?
   *        This is going to have to go into dplay somehow. Perhaps it
   *        comes back with app server id for the join command! Oh...that
   *        must be it. That would make this method obsolete once that's
   *        in place.
   */
#if 1
  return lpCache->first.lpQHFirst->lpNSAddrHdr;
#else
  /* FIXME: Should convert over to this */
  return lpCache->bNsIsLocal ? lpCache->lpLocalAddrHdr
                             : lpCache->lpRemoteAddrHdr;
#endif
}

/* Get the magic number associated with the Name Server */
DWORD NS_GetNsMagic( LPVOID lpNSInfo )
{
  LPDWORD lpHdrInfo = (LPDWORD)NS_GetNSAddr( lpNSInfo );

  return lpHdrInfo[1];
}

/* Get the magic number associated with the non NS end */
DWORD NS_GetOtherMagic( LPVOID lpNSInfo )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  return ((LPDWORD)lpCache->lpLocalAddrHdr)[1];
}

void NS_SetLocalAddr( LPVOID lpNSInfo, LPCVOID lpHdr, DWORD dwHdrSize )
{
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  lpCache->lpLocalAddrHdr = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, dwHdrSize );

  CopyMemory( lpCache->lpLocalAddrHdr, lpHdr, dwHdrSize );
}

/* This function is responsible for sending a request for all other known
   nameservers to send us what sessions they have registered locally
 */
HRESULT NS_SendSessionRequestBroadcast( LPCGUID lpcGuid,
                                        DWORD dwFlags,
                                        LPSPINITDATA lpSpData )

{
  DPSP_ENUMSESSIONSDATA data;
  LPDPMSG_ENUMSESSIONSREQUEST lpMsg;

  TRACE( "enumerating for guid %s\n", debugstr_guid( lpcGuid ) );

  /* Get the SP to deal with sending the EnumSessions request */
  FIXME( ": not all data fields are correct\n" );

  data.dwMessageSize = lpSpData->dwSPHeaderSize + sizeof( *lpMsg ); /*FIXME!*/
  data.lpMessage = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              data.dwMessageSize );
  data.lpISP = lpSpData->lpISP;
  data.bReturnStatus = (dwFlags & DPENUMSESSIONS_RETURNSTATUS) ? TRUE : FALSE;


  lpMsg = (LPDPMSG_ENUMSESSIONSREQUEST)(((BYTE*)data.lpMessage)+lpSpData->dwSPHeaderSize);

  /* Setup EnumSession request message */
  lpMsg->envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  lpMsg->envelope.wCommandId = DPMSGCMD_ENUMSESSIONSREQUEST;
  lpMsg->envelope.wVersion   = DPMSGVER_DP6;

  lpMsg->dwPasswordSize = 0; /* FIXME: If enumerating passwords..? */
  lpMsg->dwFlags        = dwFlags;

  CopyMemory( &lpMsg->guidApplication, lpcGuid, sizeof( *lpcGuid ) );

  return (lpSpData->lpCB->EnumSessions)( &data );
}

/* Delete a name server node which has been allocated on the heap */
DPQ_DECL_DELETECB( cbDeleteNSNodeFromHeap, lpNSCacheData )
{
  /* NOTE: This proc doesn't deal with the walking pointer */

  /* FIXME: Memory leak on data (contained ptrs) */
  HeapFree( GetProcessHeap(), 0, elem->data );
  HeapFree( GetProcessHeap(), 0, elem->lpNSAddrHdr );
  HeapFree( GetProcessHeap(), 0, elem );
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

  DPQ_DELETEQ( lpCache->first, next, lpNSCacheData, cbDeleteNSNodeFromHeap );

  /* NULL out the walking pointer */
  lpCache->present = NULL;

  lpCache->bNsIsLocal = FALSE;

}

/* Create and initialize a session cache */
BOOL NS_InitializeSessionCache( LPVOID* lplpNSInfo )
{
  lpNSCache lpCache = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof( *lpCache ) );

  *lplpNSInfo = lpCache;

  if( lpCache == NULL )
  {
    return FALSE;
  }

  DPQ_INIT(lpCache->first);
  lpCache->present = NULL;

  lpCache->bNsIsLocal = FALSE;

  return TRUE;
}

/* Delete a session cache */
void NS_DeleteSessionCache( LPVOID lpNSInfo )
{
  NS_InvalidateSessionCache( (lpNSCache)lpNSInfo );
}

/* Reinitialize the present pointer for this cache */
void NS_ResetSessionEnumeration( LPVOID lpNSInfo )
{
  ((lpNSCache)lpNSInfo)->present = ((lpNSCache)lpNSInfo)->first.lpQHFirst;
}

LPDPSESSIONDESC2 NS_WalkSessions( LPVOID lpNSInfo )
{
  LPDPSESSIONDESC2 lpSessionDesc;
  lpNSCache lpCache = (lpNSCache)lpNSInfo;

  /* FIXME: The pointers could disappear when walking if a prune happens */

  /* Test for end of the list */
  if( lpCache->present == NULL )
  {
    return NULL;
  }

  lpSessionDesc = lpCache->present->data;

  /* Advance tracking pointer */
  lpCache->present = lpCache->present->next.lpQNext;

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
    cbDeleteNSNodeFromHeap( lpFirstData );
  }

}

/* NAME SERVER Message stuff */
void NS_ReplyToEnumSessionsRequest( LPCVOID lpcMsg,
                                    LPVOID* lplpReplyData,
                                    LPDWORD lpdwReplySize,
                                    IDirectPlay2Impl* lpDP )
{
  LPDPMSG_ENUMSESSIONSREPLY rmsg;
  DWORD dwVariableSize;
  DWORD dwVariableLen;
  /* LPCDPMSG_ENUMSESSIONSREQUEST msg = (LPDPMSG_ENUMSESSIONSREQUEST)lpcMsg; */
  BOOL bAnsi = TRUE; /* FIXME: This needs to be in the DPLAY interface */

  FIXME( ": few fixed + need to check request for response\n" );

  if (bAnsi)
  {
      dwVariableLen = MultiByteToWideChar( CP_ACP, 0,
                                           lpDP->dp2->lpSessionDesc->lpszSessionNameA,
                                           -1, NULL, 0 );
  }
  else
  {
      dwVariableLen = strlenW( lpDP->dp2->lpSessionDesc->lpszSessionName ) + 1;
  }

  dwVariableSize = dwVariableLen * sizeof( WCHAR );

  *lpdwReplySize = lpDP->dp2->spData.dwSPHeaderSize +
                     sizeof( *rmsg ) + dwVariableSize;
  *lplpReplyData = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY,
                              *lpdwReplySize );

  rmsg = (LPDPMSG_ENUMSESSIONSREPLY)( (BYTE*)*lplpReplyData +
                                             lpDP->dp2->spData.dwSPHeaderSize);

  rmsg->envelope.dwMagic    = DPMSGMAGIC_DPLAYMSG;
  rmsg->envelope.wCommandId = DPMSGCMD_ENUMSESSIONSREPLY;
  rmsg->envelope.wVersion   = DPMSGVER_DP6;

  CopyMemory( &rmsg->sd, lpDP->dp2->lpSessionDesc,
              sizeof( lpDP->dp2->lpSessionDesc->dwSize ) );
  rmsg->dwUnknown = 0x0000005c;
  if( bAnsi )
  {
      MultiByteToWideChar( CP_ACP, 0, lpDP->dp2->lpSessionDesc->lpszSessionNameA, -1,
                           (LPWSTR)(rmsg+1), dwVariableLen );
  }
  else
  {
      strcpyW( (LPWSTR)(rmsg+1), lpDP->dp2->lpSessionDesc->lpszSessionName );
  }
}
