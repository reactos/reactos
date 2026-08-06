/* dplayx.dll global data implementation.
 *
 * Copyright 1999,2000 - Peter Hunnisett
 *
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
 *
 * NOTES: 
 *  o Implementation of all things which are associated with dplay on
 *    the computer - i.e. shared resources and such. Methods in this
 *    compilation unit should not call anything outside of this unit
 *    except base windows services and an interface to start the
 *    messaging thread.
 *  o Methods that begin with DPLAYX_ are used for dealing with
 *    dplayx.dll data which is accessible from all processes.
 *
 */

#include <stdarg.h>
#include <string.h>

#include "wine/debug.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

#include "wingdi.h"
#include "winuser.h"

#include "dplayx_global.h"
#include "dplayx_messages.h" /* For CreateMessageReceptionThread only */

WINE_DEFAULT_DEBUG_CHANNEL(dplay);

/* FIXME: Need to do all that fun other dll referencing type of stuff */

/* Static data for all processes */
static const char lpszDplayxSemaName[] = "WINE_DPLAYX_SM";
static HANDLE hDplayxSema;

static const char lpszDplayxFileMapping[] = "WINE_DPLAYX_FM";
static HANDLE hDplayxSharedMem;

static LPVOID lpSharedStaticData = NULL;


#define DPLAYX_AcquireSemaphore() TRACE( "Waiting for DPLAYX semaphore\n" ); \
                                  WaitForSingleObject( hDplayxSema, INFINITE );\
                                  TRACE( "Through wait\n" )

#define DPLAYX_ReleaseSemaphore() ReleaseSemaphore( hDplayxSema, 1, NULL ); \
                                  TRACE( "DPLAYX Semaphore released\n" ) /* FIXME: Is this correct? */


/* HACK for simple global data right now */
#define dwStaticSharedSize (128 * 1024) /* 128 KBytes */
#define dwDynamicSharedSize (512 * 1024) /* 512 KBytes */
#define dwTotalSharedSize  ( dwStaticSharedSize + dwDynamicSharedSize )


/* FIXME: Is there no easier way? */

/* Pretend the entire dynamic area is carved up into 512 byte blocks.
 * Each block has 4 bytes which are 0 unless used */
#define dwBlockSize 512
#define dwMaxBlock  (dwDynamicSharedSize/dwBlockSize)

typedef struct
{
  BOOL used;
  BYTE data[dwBlockSize - sizeof(BOOL)];
} DPLAYX_MEM_SLICE;
C_ASSERT(sizeof(DPLAYX_MEM_SLICE) == dwBlockSize);

static DPLAYX_MEM_SLICE* lpMemArea;

static void DPLAYX_PrivHeapFree( LPVOID addr )
{
  LPVOID lpAddrStart;
  DWORD dwBlockUsed;

  /* Handle getting passed a NULL */
  if( addr == NULL )
  {
    return;
  }

  lpAddrStart = CONTAINING_RECORD(addr, DPLAYX_MEM_SLICE, data); /* Find block header */
  dwBlockUsed =  ((BYTE*)lpAddrStart - (BYTE*)lpMemArea)/dwBlockSize;

  lpMemArea[ dwBlockUsed ].used = FALSE;
}

static LPVOID DPLAYX_PrivHeapAlloc( DWORD flags, DWORD size )
{
  LPVOID lpvArea = NULL;
  UINT   uBlockUsed;

  if( size > (dwBlockSize - sizeof(BOOL)) )
  {
    FIXME( "Size exceeded. Request of 0x%08lx\n", size );
    size = dwBlockSize - sizeof(BOOL);
  }

  /* Find blank area */
  uBlockUsed = 0;
  while( uBlockUsed < dwMaxBlock && lpMemArea[ uBlockUsed ].used ) { uBlockUsed++; }

  if( uBlockUsed < dwMaxBlock )
  {
    /* Set the area used */
    lpMemArea[ uBlockUsed ].used = TRUE;
    lpvArea = lpMemArea[ uBlockUsed ].data;
  }
  else
  {
    ERR( "No free block found\n" );
    return NULL;
  }

  if( flags & HEAP_ZERO_MEMORY )
  {
    ZeroMemory( lpvArea, size );
  }

  return lpvArea;
}


enum { numSupportedLobbies = 32, numSupportedSessions = 32 };
typedef struct tagDPLAYX_LOBBYDATA
{
  /* Points to lpConn + block of contiguous extra memory for dynamic parts
   * of the struct directly following
   */
  LPDPLCONNECTION lpConn;

  /* Information for dplobby interfaces */
  DWORD           dwAppID;
  DWORD           dwAppLaunchedFromID;

  /* Should this lobby app send messages to creator at important life
   * stages
   */
  HANDLE hInformOnAppStart;
  HANDLE hInformOnAppDeath;
  HANDLE hInformOnSettingRead;

  /* Sundries */
  BOOL bWaitForConnectionSettings;
  DWORD dwLobbyMsgThreadId;


} DPLAYX_LOBBYDATA, *LPDPLAYX_LOBBYDATA;

static DPLAYX_LOBBYDATA* lobbyData = NULL;
/* static DPLAYX_LOBBYDATA lobbyData[ numSupportedLobbies ]; */

static DPSESSIONDESC2* sessionData = NULL;
/* static DPSESSIONDESC2* sessionData[ numSupportedSessions ]; */


static void DPLAYX_InitializeLobbyDataEntry( LPDPLAYX_LOBBYDATA lpData )
{
  ZeroMemory( lpData, sizeof( *lpData ) );
}

/* NOTE: This must be called with the semaphore acquired.
 * TRUE/FALSE with a pointer to its data returned. Pointer data is
 * is only valid if TRUE is returned.
 */
static BOOL DPLAYX_IsAppIdLobbied( DWORD dwAppID, LPDPLAYX_LOBBYDATA* lplpDplData )
{
  UINT i;

  *lplpDplData = NULL;

  if( dwAppID == 0 )
  {
    dwAppID = GetCurrentProcessId();
    TRACE( "Translated dwAppID == 0 into 0x%08lx\n", dwAppID );
  }

  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( lobbyData[ i ].dwAppID == dwAppID )
    {
      /* This process is lobbied */
      TRACE( "Found 0x%08lx @ %u\n", dwAppID, i );
      *lplpDplData = &lobbyData[ i ];
      return TRUE;
    }
  }

  return FALSE;
}

/* Reserve a spot for the new application. TRUE means success and FALSE failure.  */
BOOL DPLAYX_CreateLobbyApplication( DWORD dwAppID )
{
  UINT i;

  /* 0 is the marker for unused application data slots */
  if( dwAppID == 0 )
  {
    return FALSE;
  }

  DPLAYX_AcquireSemaphore();

  /* Find an empty space in the list and insert the data */
  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( lobbyData[ i ].dwAppID == 0 )
    {
      /* This process is now lobbied */
      TRACE( "Setting lobbyData[%u] for (0x%08lx,0x%08lx)\n",
              i, dwAppID, GetCurrentProcessId() );

      lobbyData[ i ].dwAppID              = dwAppID;
      lobbyData[ i ].dwAppLaunchedFromID  = GetCurrentProcessId();

      /* FIXME: Where is the best place for this? In interface or here? */
      lobbyData[ i ].hInformOnAppStart = 0;
      lobbyData[ i ].hInformOnAppDeath = 0;
      lobbyData[ i ].hInformOnSettingRead = 0;

      DPLAYX_ReleaseSemaphore();
      return TRUE;
    }
  }

  ERR( "No empty lobbies\n" );

  DPLAYX_ReleaseSemaphore();
  return FALSE;
}

BOOL DPLAYX_SetLobbyHandles( DWORD dwAppID,
                             HANDLE hStart, HANDLE hDeath, HANDLE hConnRead )
{
  LPDPLAYX_LOBBYDATA lpLData;

  /* Need to explicitly give lobby application. Can't set for yourself */
  if( dwAppID == 0 )
  {
    return FALSE;
  }

  DPLAYX_AcquireSemaphore();

  if( !DPLAYX_IsAppIdLobbied( dwAppID, &lpLData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }

  lpLData->hInformOnAppStart    = hStart;
  lpLData->hInformOnAppDeath    = hDeath;
  lpLData->hInformOnSettingRead = hConnRead;

  DPLAYX_ReleaseSemaphore();

  return TRUE;
}

static BOOL DPLAYX_GetThisLobbyHandles( LPHANDLE lphStart,
                                        LPHANDLE lphDeath,
                                        LPHANDLE lphConnRead,
                                        BOOL     bClearSetHandles )
{
  LPDPLAYX_LOBBYDATA lpLData;

  DPLAYX_AcquireSemaphore();

  if( !DPLAYX_IsAppIdLobbied( 0, &lpLData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }

  if( lphStart != NULL )
  {
    if( lpLData->hInformOnAppStart == 0 )
    {
      DPLAYX_ReleaseSemaphore();
      return FALSE;
    }

    *lphStart = lpLData->hInformOnAppStart;

    if( bClearSetHandles )
    {
      CloseHandle( lpLData->hInformOnAppStart );
      lpLData->hInformOnAppStart = 0;
    }
  }

  if( lphDeath != NULL )
  {
    if( lpLData->hInformOnAppDeath == 0 )
    {
      DPLAYX_ReleaseSemaphore();
      return FALSE;
    }

    *lphDeath = lpLData->hInformOnAppDeath;

    if( bClearSetHandles )
    {
      CloseHandle( lpLData->hInformOnAppDeath );
      lpLData->hInformOnAppDeath = 0;
    }
  }

  if( lphConnRead != NULL )
  {
    if( lpLData->hInformOnSettingRead == 0 )
    {
      DPLAYX_ReleaseSemaphore();
      return FALSE;
    }

    *lphConnRead = lpLData->hInformOnSettingRead;

    if( bClearSetHandles )
    {
      CloseHandle( lpLData->hInformOnSettingRead );
      lpLData->hInformOnSettingRead = 0;
    }
  }

  DPLAYX_ReleaseSemaphore();

  return TRUE;
}

/***************************************************************************
 * Called to initialize the global data. This will only be used on the
 * loading of the dll
 ***************************************************************************/
BOOL DPLAYX_ConstructData(void)
{
  SECURITY_ATTRIBUTES s_attrib;
  BOOL                bInitializeSharedMemory = FALSE;
  LPVOID              lpDesiredMemoryMapStart = (LPVOID)0x50000000;
  HANDLE              hInformOnStart;

  TRACE( "DPLAYX dll loaded - construct called\n" );

  /* Create a semaphore to block access to DPLAYX global data structs */

  s_attrib.bInheritHandle       = TRUE;
  s_attrib.lpSecurityDescriptor = NULL;
  s_attrib.nLength              = sizeof(s_attrib);

  hDplayxSema = CreateSemaphoreA( &s_attrib, 0, 1, lpszDplayxSemaName );

  /* First instance creates the semaphore. Others just use it */
  if( GetLastError() == ERROR_SUCCESS )
  {
    TRACE( "Semaphore %p created\n", hDplayxSema );

    /* The semaphore creator will also build the shared memory */
    bInitializeSharedMemory = TRUE;
  }
  else if ( GetLastError() == ERROR_ALREADY_EXISTS )
  {
    TRACE( "Found semaphore handle %p\n", hDplayxSema );
    DPLAYX_AcquireSemaphore();
  }
  else
  {
    ERR( ": semaphore error %ld\n", GetLastError() );
    return FALSE;
  }

  SetLastError( ERROR_SUCCESS );

  hDplayxSharedMem = CreateFileMappingA( INVALID_HANDLE_VALUE,
                                         &s_attrib,
                                         PAGE_READWRITE | SEC_COMMIT,
                                         0,
                                         dwTotalSharedSize,
                                         lpszDplayxFileMapping );

  if( GetLastError() == ERROR_SUCCESS )
  {
    TRACE( "File mapped %p created\n", hDplayxSharedMem );
  }
  else if ( GetLastError() == ERROR_ALREADY_EXISTS )
  {
    TRACE( "Found FileMapping handle %p\n", hDplayxSharedMem );
  }
  else
  {
    ERR( ": unable to create shared memory (%ld)\n", GetLastError() );
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }

  lpSharedStaticData = MapViewOfFileEx( hDplayxSharedMem,
                                        FILE_MAP_WRITE,
                                        0, 0, 0, lpDesiredMemoryMapStart );

  if( lpSharedStaticData == NULL && GetLastError() == ERROR_INVALID_ADDRESS )
  {
    /* We couldn't map the data where we wanted. Try again, allowing any
     * location. */
    lpSharedStaticData = MapViewOfFile( hDplayxSharedMem,
                                        FILE_MAP_WRITE,
                                        0, 0, 0 );
  }

  if( lpSharedStaticData == NULL )
  {
    ERR( ": unable to map static data into process memory space (%ld)\n",
         GetLastError() );
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }
  else
  {
    if( lpDesiredMemoryMapStart == lpSharedStaticData )
    {
      TRACE( "File mapped to %p\n", lpSharedStaticData );
    }
    else
    {
      /* Presently the shared data structures use pointers. If the
       * files are not mapped into the same area, the pointers will no
       * longer make any sense :(
       * FIXME: In the future make the shared data structures have some
       *        sort of fixup to make them independent between data spaces.
       *        This will also require a rework of the session data stuff.
       */
      ERR( "File mapped to %p (not %p). Expect failure\n",
            lpSharedStaticData, lpDesiredMemoryMapStart );
    }
  }

  /* Dynamic area starts just after the static area */
  lpMemArea = (LPVOID)((BYTE*)lpSharedStaticData + dwStaticSharedSize);

  /* FIXME: Crude hack */
  lobbyData   = lpSharedStaticData;
  sessionData = (DPSESSIONDESC2*)((BYTE*)lpSharedStaticData + (dwStaticSharedSize/2));

  /* Initialize shared data segments. */
  if( bInitializeSharedMemory )
  {
    UINT i;

    TRACE( "Initializing shared memory\n" );

    /* Set all lobbies to be "empty" */
    for( i=0; i < numSupportedLobbies; i++ )
    {
      DPLAYX_InitializeLobbyDataEntry( &lobbyData[ i ] );
    }

    /* Set all sessions to be "empty" */
    for( i=0; i < numSupportedSessions; i++ )
    {
      sessionData[i].dwSize = 0;
    }

    /* Zero out the dynamic area */
    ZeroMemory( lpMemArea, dwDynamicSharedSize );

    /* Just for fun sync the whole data area */
    FlushViewOfFile( lpSharedStaticData, dwTotalSharedSize );
  }

  DPLAYX_ReleaseSemaphore();

  /* Everything was created correctly. Signal the lobby client that
   * we started up correctly
   */
  if( DPLAYX_GetThisLobbyHandles( &hInformOnStart, NULL, NULL, FALSE ) &&
      hInformOnStart
    )
  {
    BOOL bSuccess;
    bSuccess = SetEvent( hInformOnStart );
    TRACE( "Signalling lobby app start event %p %s\n",
             hInformOnStart, bSuccess ? "succeed" : "failed" );

    /* Close out handle */
    DPLAYX_GetThisLobbyHandles( &hInformOnStart, NULL, NULL, TRUE );
  }

  return TRUE;
}

/***************************************************************************
 * Called to destroy all global data. This will only be used on the
 * unloading of the dll
 ***************************************************************************/
BOOL DPLAYX_DestructData(void)
{
  HANDLE hInformOnDeath;

  TRACE( "DPLAYX dll unloaded - destruct called\n" );

  /* If required, inform that this app is dying */
  if( DPLAYX_GetThisLobbyHandles( NULL, &hInformOnDeath, NULL, FALSE ) &&
      hInformOnDeath
    )
  {
    BOOL bSuccess;
    bSuccess = SetEvent( hInformOnDeath );
    TRACE( "Signalling lobby app death event %p %s\n",
             hInformOnDeath, bSuccess ? "succeed" : "failed" );

    /* Close out handle */
    DPLAYX_GetThisLobbyHandles( NULL, &hInformOnDeath, NULL, TRUE );
  }

  /* DO CLEAN UP (LAST) */

  /* Delete the semaphore */
  CloseHandle( hDplayxSema );

  /* Delete shared memory file mapping */
  UnmapViewOfFile( lpSharedStaticData );
  CloseHandle( hDplayxSharedMem );

  return FALSE;
}


/* Assumption: Enough contiguous space was allocated at dest */
static void DPLAYX_CopyConnStructA( LPDPLCONNECTION dest, const DPLCONNECTION *src )
{
  BYTE* lpStartOfFreeSpace;

  *dest = *src;

  lpStartOfFreeSpace = ((BYTE*)dest) + sizeof( DPLCONNECTION );

  /* Copy the LPDPSESSIONDESC2 structure if it exists */
  if( src->lpSessionDesc )
  {
    dest->lpSessionDesc = (LPDPSESSIONDESC2)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPSESSIONDESC2 );
    *dest->lpSessionDesc = *src->lpSessionDesc;

    /* Session names may or may not exist */
    if( src->lpSessionDesc->lpszSessionNameA )
    {
      strcpy( (LPSTR)lpStartOfFreeSpace, src->lpSessionDesc->lpszSessionNameA );
      dest->lpSessionDesc->lpszSessionNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=
        strlen( dest->lpSessionDesc->lpszSessionNameA ) + 1;
    }

    if( src->lpSessionDesc->lpszPasswordA )
    {
      strcpy( (LPSTR)lpStartOfFreeSpace, src->lpSessionDesc->lpszPasswordA );
      dest->lpSessionDesc->lpszPasswordA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=
        strlen( dest->lpSessionDesc->lpszPasswordA ) + 1;
    }
  }

  /* DPNAME structure is optional */
  if( src->lpPlayerName )
  {
    dest->lpPlayerName = (LPDPNAME)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPNAME );
    *dest->lpPlayerName = *src->lpPlayerName;

    if( src->lpPlayerName->lpszShortNameA )
    {
      strcpy( (LPSTR)lpStartOfFreeSpace, src->lpPlayerName->lpszShortNameA );
      dest->lpPlayerName->lpszShortNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=
        strlen( dest->lpPlayerName->lpszShortNameA ) + 1;
    }

    if( src->lpPlayerName->lpszLongNameA )
    {
      strcpy( (LPSTR)lpStartOfFreeSpace, src->lpPlayerName->lpszLongNameA );
      dest->lpPlayerName->lpszLongNameA = (LPSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=
        strlen( (LPSTR)dest->lpPlayerName->lpszLongName ) + 1 ;
    }

  }

  /* Copy address if it exists */
  if( src->lpAddress )
  {
    dest->lpAddress = lpStartOfFreeSpace;
    CopyMemory( lpStartOfFreeSpace, src->lpAddress, src->dwAddressSize );
    /* No need to advance lpStartOfFreeSpace as there is no more "dynamic" data */
  }
}

/* Assumption: Enough contiguous space was allocated at dest */
static void DPLAYX_CopyConnStructW( LPDPLCONNECTION dest, const DPLCONNECTION *src )
{
  BYTE*              lpStartOfFreeSpace;

  *dest = *src;

  lpStartOfFreeSpace = ( (BYTE*)dest) + sizeof( DPLCONNECTION );

  /* Copy the LPDPSESSIONDESC2 structure if it exists */
  if( src->lpSessionDesc )
  {
    dest->lpSessionDesc = (LPDPSESSIONDESC2)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPSESSIONDESC2 );
    *dest->lpSessionDesc = *src->lpSessionDesc;

    /* Session names may or may not exist */
    if( src->lpSessionDesc->lpszSessionName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpSessionDesc->lpszSessionName );
      dest->lpSessionDesc->lpszSessionName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( dest->lpSessionDesc->lpszSessionName ) + 1 );
    }

    if( src->lpSessionDesc->lpszPassword )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpSessionDesc->lpszPassword );
      dest->lpSessionDesc->lpszPassword = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( dest->lpSessionDesc->lpszPassword ) + 1 );
    }
  }

  /* DPNAME structure is optional */
  if( src->lpPlayerName )
  {
    dest->lpPlayerName = (LPDPNAME)lpStartOfFreeSpace;
    lpStartOfFreeSpace += sizeof( DPNAME );
    *dest->lpPlayerName = *src->lpPlayerName;

    if( src->lpPlayerName->lpszShortName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpPlayerName->lpszShortName );
      dest->lpPlayerName->lpszShortName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( dest->lpPlayerName->lpszShortName ) + 1 );
    }

    if( src->lpPlayerName->lpszLongName )
    {
      lstrcpyW( (LPWSTR)lpStartOfFreeSpace, src->lpPlayerName->lpszLongName );
      dest->lpPlayerName->lpszLongName = (LPWSTR)lpStartOfFreeSpace;
      lpStartOfFreeSpace +=  sizeof(WCHAR) *
        ( lstrlenW( dest->lpPlayerName->lpszLongName ) + 1 );
    }

  }

  /* Copy address if it exists */
  if( src->lpAddress )
  {
    dest->lpAddress = lpStartOfFreeSpace;
    CopyMemory( lpStartOfFreeSpace, src->lpAddress, src->dwAddressSize );
    /* No need to advance lpStartOfFreeSpace as there is no more "dynamic" data */
  }

}

static DWORD DPLAYX_SizeOfLobbyDataA( const DPLCONNECTION *lpConn )
{
  DWORD dwTotalSize = sizeof( DPLCONNECTION );

  /* Just a safety check */
  if( lpConn == NULL )
  {
    ERR( "lpConn is NULL\n" );
    return 0;
  }

  if( lpConn->lpSessionDesc != NULL )
  {
    dwTotalSize += sizeof( DPSESSIONDESC2 );

    if( lpConn->lpSessionDesc->lpszSessionNameA )
    {
      dwTotalSize += strlen( lpConn->lpSessionDesc->lpszSessionNameA ) + 1;
    }

    if( lpConn->lpSessionDesc->lpszPasswordA )
    {
      dwTotalSize += strlen( lpConn->lpSessionDesc->lpszPasswordA ) + 1;
    }
  }

  if( lpConn->lpPlayerName != NULL )
  {
    dwTotalSize += sizeof( DPNAME );

    if( lpConn->lpPlayerName->lpszShortNameA )
    {
      dwTotalSize += strlen( lpConn->lpPlayerName->lpszShortNameA ) + 1;
    }

    if( lpConn->lpPlayerName->lpszLongNameA )
    {
      dwTotalSize += strlen( lpConn->lpPlayerName->lpszLongNameA ) + 1;
    }

  }

  dwTotalSize += lpConn->dwAddressSize;

  return dwTotalSize;
}

static DWORD DPLAYX_SizeOfLobbyDataW( const DPLCONNECTION *lpConn )
{
  DWORD dwTotalSize = sizeof( DPLCONNECTION );

  /* Just a safety check */
  if( lpConn == NULL )
  {
    ERR( "lpConn is NULL\n" );
    return 0;
  }

  if( lpConn->lpSessionDesc != NULL )
  {
    dwTotalSize += sizeof( DPSESSIONDESC2 );

    if( lpConn->lpSessionDesc->lpszSessionName )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpSessionDesc->lpszSessionName ) + 1 );
    }

    if( lpConn->lpSessionDesc->lpszPassword )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpSessionDesc->lpszPassword ) + 1 );
    }
  }

  if( lpConn->lpPlayerName != NULL )
  {
    dwTotalSize += sizeof( DPNAME );

    if( lpConn->lpPlayerName->lpszShortName )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpPlayerName->lpszShortName ) + 1 );
    }

    if( lpConn->lpPlayerName->lpszLongName )
    {
      dwTotalSize += sizeof( WCHAR ) *
        ( lstrlenW( lpConn->lpPlayerName->lpszLongName ) + 1 );
    }

  }

  dwTotalSize += lpConn->dwAddressSize;

  return dwTotalSize;
}

HRESULT DPLAYX_GetConnectionSettingsA
( DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  LPDPLAYX_LOBBYDATA lpDplData;
  DWORD              dwRequiredDataSize = 0;
  HANDLE             hInformOnSettingRead;

  DPLAYX_AcquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();

    TRACE( "Application 0x%08lx is not lobbied\n", dwAppID );
    return DPERR_NOTLOBBIED;
  }

  dwRequiredDataSize = DPLAYX_SizeOfLobbyDataA( lpDplData->lpConn );

  /* Do they want to know the required buffer size or is the provided buffer
   * big enough?
   */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < dwRequiredDataSize )
     )
  {
    DPLAYX_ReleaseSemaphore();

    *lpdwDataSize = DPLAYX_SizeOfLobbyDataA( lpDplData->lpConn );

    return DPERR_BUFFERTOOSMALL;
  }

  DPLAYX_CopyConnStructA( lpData, lpDplData->lpConn );

  DPLAYX_ReleaseSemaphore();

  /* They have gotten the information - signal the event if required */
  if( DPLAYX_GetThisLobbyHandles( NULL, NULL, &hInformOnSettingRead, FALSE ) &&
      hInformOnSettingRead
    )
  {
    BOOL bSuccess;
    bSuccess = SetEvent( hInformOnSettingRead );
    TRACE( "Signalling setting read event %p %s\n",
             hInformOnSettingRead, bSuccess ? "succeed" : "failed" );

    /* Close out handle */
    DPLAYX_GetThisLobbyHandles( NULL, NULL, &hInformOnSettingRead, TRUE );
  }

  return DP_OK;
}

HRESULT DPLAYX_GetConnectionSettingsW
( DWORD dwAppID,
  LPVOID lpData,
  LPDWORD lpdwDataSize )
{
  LPDPLAYX_LOBBYDATA lpDplData;
  DWORD              dwRequiredDataSize = 0;
  HANDLE             hInformOnSettingRead;

  DPLAYX_AcquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return DPERR_NOTLOBBIED;
  }

  dwRequiredDataSize = DPLAYX_SizeOfLobbyDataW( lpDplData->lpConn );

  /* Do they want to know the required buffer size or is the provided buffer
   * big enough?
   */
  if ( ( lpData == NULL ) ||
       ( *lpdwDataSize < dwRequiredDataSize )
     )
  {
    DPLAYX_ReleaseSemaphore();

    *lpdwDataSize = DPLAYX_SizeOfLobbyDataW( lpDplData->lpConn );

    return DPERR_BUFFERTOOSMALL;
  }

  DPLAYX_CopyConnStructW( lpData, lpDplData->lpConn );

  DPLAYX_ReleaseSemaphore();

  /* They have gotten the information - signal the event if required */
  if( DPLAYX_GetThisLobbyHandles( NULL, NULL, &hInformOnSettingRead, FALSE ) &&
      hInformOnSettingRead
    )
  {
    BOOL bSuccess;
    bSuccess = SetEvent( hInformOnSettingRead );
    TRACE( "Signalling setting read event %p %s\n",
             hInformOnSettingRead, bSuccess ? "succeed" : "failed" );

    /* Close out handle */
    DPLAYX_GetThisLobbyHandles( NULL, NULL, &hInformOnSettingRead, TRUE );
  }

  return DP_OK;
}

/* Store the structure into the shared data structure. Ensure that allocs for
 * variable length strings come from the shared data structure.
 * FIXME: We need to free information as well.
 */
HRESULT DPLAYX_SetConnectionSettingsA
( DWORD dwFlags,
  DWORD dwAppID,
  const DPLCONNECTION *lpConn )
{
  LPDPLAYX_LOBBYDATA lpDplData;

  /* Parameter check */
  if( dwFlags || !lpConn )
  {
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR(": old/new DPLCONNECTION type? Size=%08lx\n", lpConn->dwSize );

    return DPERR_INVALIDPARAMS;
  }

  DPLAYX_AcquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();

    return DPERR_NOTLOBBIED;
  }

  if(  (!lpConn->lpSessionDesc ) ||
       ( lpConn->lpSessionDesc->dwSize != sizeof( DPSESSIONDESC2 ) )
    )
  {
    DPLAYX_ReleaseSemaphore();

    ERR("DPSESSIONDESC passed in? Size=%lu\n",
        lpConn->lpSessionDesc?lpConn->lpSessionDesc->dwSize:0 );

    return DPERR_INVALIDPARAMS;
  }

  /* Free the existing memory */
  DPLAYX_PrivHeapFree( lpDplData->lpConn );

  lpDplData->lpConn = DPLAYX_PrivHeapAlloc( HEAP_ZERO_MEMORY,
                                            DPLAYX_SizeOfLobbyDataA( lpConn ) );

  DPLAYX_CopyConnStructA( lpDplData->lpConn, lpConn );


  DPLAYX_ReleaseSemaphore();

  /* FIXME: Send a message - I think */

  return DP_OK;
}

/* Store the structure into the shared data structure. Ensure that allocs for
 * variable length strings come from the shared data structure.
 * FIXME: We need to free information as well
 */
HRESULT DPLAYX_SetConnectionSettingsW
( DWORD dwFlags,
  DWORD dwAppID,
  const DPLCONNECTION *lpConn )
{
  LPDPLAYX_LOBBYDATA lpDplData;

  /* Parameter check */
  if( dwFlags || !lpConn )
  {
    ERR("invalid parameters.\n");
    return DPERR_INVALIDPARAMS;
  }

  /* Store information */
  if(  lpConn->dwSize != sizeof(DPLCONNECTION) )
  {
    ERR(": old/new DPLCONNECTION type? Size=%lu\n", lpConn->dwSize );

    return DPERR_INVALIDPARAMS;
  }

  DPLAYX_AcquireSemaphore();

  if ( ! DPLAYX_IsAppIdLobbied( dwAppID, &lpDplData ) )
  {
    DPLAYX_ReleaseSemaphore();

    return DPERR_NOTLOBBIED;
  }

  /* Free the existing memory */
  DPLAYX_PrivHeapFree( lpDplData->lpConn );

  lpDplData->lpConn = DPLAYX_PrivHeapAlloc( HEAP_ZERO_MEMORY,
                                            DPLAYX_SizeOfLobbyDataW( lpConn ) );

  DPLAYX_CopyConnStructW( lpDplData->lpConn, lpConn );


  DPLAYX_ReleaseSemaphore();

  /* FIXME: Send a message - I think */

  return DP_OK;
}

BOOL DPLAYX_WaitForConnectionSettings( BOOL bWait )
{
  LPDPLAYX_LOBBYDATA lpLobbyData;

  DPLAYX_AcquireSemaphore();

  if( !DPLAYX_IsAppIdLobbied( 0, &lpLobbyData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }

  lpLobbyData->bWaitForConnectionSettings = bWait;

  DPLAYX_ReleaseSemaphore();

  return TRUE;
}

BOOL DPLAYX_AnyLobbiesWaitingForConnSettings(void)
{
  UINT i;
  BOOL bFound = FALSE;

  DPLAYX_AcquireSemaphore();

  for( i=0; i < numSupportedLobbies; i++ )
  {
    if( ( lobbyData[ i ].dwAppID != 0 ) &&            /* lobby initialized */
        ( lobbyData[ i ].bWaitForConnectionSettings ) /* Waiting */
      )
    {
      bFound = TRUE;
      break;
    }
  }

  DPLAYX_ReleaseSemaphore();

  return bFound;
}

BOOL DPLAYX_SetLobbyMsgThreadId( DWORD dwAppId, DWORD dwThreadId )
{
  LPDPLAYX_LOBBYDATA lpLobbyData;

  DPLAYX_AcquireSemaphore();

  if( !DPLAYX_IsAppIdLobbied( dwAppId, &lpLobbyData ) )
  {
    DPLAYX_ReleaseSemaphore();
    return FALSE;
  }

  lpLobbyData->dwLobbyMsgThreadId = dwThreadId;

  DPLAYX_ReleaseSemaphore();

  return TRUE;
}

/* NOTE: This is potentially not thread safe. You are not guaranteed to end up
         with the correct string printed in the case where the HRESULT is not
         known. You will just get the last hr passed in. This can change
         over time if this method is used a lot :) */
LPCSTR DPLAYX_HresultToString(HRESULT hr)
{
  static char szTempStr[12];

  switch (hr)
  {
    case DP_OK:
      return "DP_OK";
    case DPERR_ALREADYINITIALIZED:
      return "DPERR_ALREADYINITIALIZED";
    case DPERR_ACCESSDENIED:
      return "DPERR_ACCESSDENIED";
    case DPERR_ACTIVEPLAYERS:
      return "DPERR_ACTIVEPLAYERS";
    case DPERR_BUFFERTOOSMALL:
      return "DPERR_BUFFERTOOSMALL";
    case DPERR_CANTADDPLAYER:
      return "DPERR_CANTADDPLAYER";
    case DPERR_CANTCREATEGROUP:
      return "DPERR_CANTCREATEGROUP";
    case DPERR_CANTCREATEPLAYER:
      return "DPERR_CANTCREATEPLAYER";
    case DPERR_CANTCREATESESSION:
      return "DPERR_CANTCREATESESSION";
    case DPERR_CAPSNOTAVAILABLEYET:
      return "DPERR_CAPSNOTAVAILABLEYET";
    case DPERR_EXCEPTION:
      return "DPERR_EXCEPTION";
    case DPERR_GENERIC:
      return "DPERR_GENERIC";
    case DPERR_INVALIDFLAGS:
      return "DPERR_INVALIDFLAGS";
    case DPERR_INVALIDOBJECT:
      return "DPERR_INVALIDOBJECT";
    case DPERR_INVALIDPARAMS:
      return "DPERR_INVALIDPARAMS";
    case DPERR_INVALIDPLAYER:
      return "DPERR_INVALIDPLAYER";
    case DPERR_INVALIDGROUP:
      return "DPERR_INVALIDGROUP";
    case DPERR_NOCAPS:
      return "DPERR_NOCAPS";
    case DPERR_NOCONNECTION:
      return "DPERR_NOCONNECTION";
    case DPERR_OUTOFMEMORY:
      return "DPERR_OUTOFMEMORY";
    case DPERR_NOMESSAGES:
      return "DPERR_NOMESSAGES";
    case DPERR_NONAMESERVERFOUND:
      return "DPERR_NONAMESERVERFOUND";
    case DPERR_NOPLAYERS:
      return "DPERR_NOPLAYERS";
    case DPERR_NOSESSIONS:
      return "DPERR_NOSESSIONS";
    case DPERR_PENDING:
      return "DPERR_PENDING";
    case DPERR_SENDTOOBIG:
      return "DPERR_SENDTOOBIG";
    case DPERR_TIMEOUT:
      return "DPERR_TIMEOUT";
    case DPERR_UNAVAILABLE:
      return "DPERR_UNAVAILABLE";
    case DPERR_UNSUPPORTED:
      return "DPERR_UNSUPPORTED";
    case DPERR_BUSY:
      return "DPERR_BUSY";
    case DPERR_USERCANCEL:
      return "DPERR_USERCANCEL";
    case DPERR_NOINTERFACE:
      return "DPERR_NOINTERFACE";
    case DPERR_CANNOTCREATESERVER:
      return "DPERR_CANNOTCREATESERVER";
    case DPERR_PLAYERLOST:
      return "DPERR_PLAYERLOST";
    case DPERR_SESSIONLOST:
      return "DPERR_SESSIONLOST";
    case DPERR_UNINITIALIZED:
      return "DPERR_UNINITIALIZED";
    case DPERR_NONEWPLAYERS:
      return "DPERR_NONEWPLAYERS";
    case DPERR_INVALIDPASSWORD:
      return "DPERR_INVALIDPASSWORD";
    case DPERR_CONNECTING:
      return "DPERR_CONNECTING";
    case DPERR_CONNECTIONLOST:
      return "DPERR_CONNECTIONLOST";
    case DPERR_UNKNOWNMESSAGE:
      return "DPERR_UNKNOWNMESSAGE";
    case DPERR_CANCELFAILED:
      return "DPERR_CANCELFAILED";
    case DPERR_INVALIDPRIORITY:
      return "DPERR_INVALIDPRIORITY";
    case DPERR_NOTHANDLED:
      return "DPERR_NOTHANDLED";
    case DPERR_CANCELLED:
      return "DPERR_CANCELLED";
    case DPERR_ABORTED:
      return "DPERR_ABORTED";
    case DPERR_BUFFERTOOLARGE:
      return "DPERR_BUFFERTOOLARGE";
    case DPERR_CANTCREATEPROCESS:
      return "DPERR_CANTCREATEPROCESS";
    case DPERR_APPNOTSTARTED:
      return "DPERR_APPNOTSTARTED";
    case DPERR_INVALIDINTERFACE:
      return "DPERR_INVALIDINTERFACE";
    case DPERR_NOSERVICEPROVIDER:
      return "DPERR_NOSERVICEPROVIDER";
    case DPERR_UNKNOWNAPPLICATION:
      return "DPERR_UNKNOWNAPPLICATION";
    case DPERR_NOTLOBBIED:
      return "DPERR_NOTLOBBIED";
    case DPERR_SERVICEPROVIDERLOADED:
      return "DPERR_SERVICEPROVIDERLOADED";
    case DPERR_ALREADYREGISTERED:
      return "DPERR_ALREADYREGISTERED";
    case DPERR_NOTREGISTERED:
      return "DPERR_NOTREGISTERED";
    case DPERR_AUTHENTICATIONFAILED:
      return "DPERR_AUTHENTICATIONFAILED";
    case DPERR_CANTLOADSSPI:
      return "DPERR_CANTLOADSSPI";
    case DPERR_ENCRYPTIONFAILED:
      return "DPERR_ENCRYPTIONFAILED";
    case DPERR_SIGNFAILED:
      return "DPERR_SIGNFAILED";
    case DPERR_CANTLOADSECURITYPACKAGE:
      return "DPERR_CANTLOADSECURITYPACKAGE";
    case DPERR_ENCRYPTIONNOTSUPPORTED:
      return "DPERR_ENCRYPTIONNOTSUPPORTED";
    case DPERR_CANTLOADCAPI:
      return "DPERR_CANTLOADCAPI";
    case DPERR_NOTLOGGEDIN:
      return "DPERR_NOTLOGGEDIN";
    case DPERR_LOGONDENIED:
      return "DPERR_LOGONDENIED";
    default:
      /* For errors not in the list, return HRESULT as a string
         This part is not thread safe */
      WARN( "Unknown error 0x%08lx\n", hr );
      wsprintfA( szTempStr, "0x%08x", hr );
      return szTempStr;
  }
}
