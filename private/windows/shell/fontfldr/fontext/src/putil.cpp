///////////////////////////////////////////////////////////////////////////////
//
// putil.cpp
//      Explorer Font Folder extension routines.
//      Control panel utility function.
//      Contains Control Panel memory allocation routines.
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//      15 Aug 95 SteveCat
//          Added memory allocation routines for Type 1 font support
//
//
// NOTE/BUGS
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

// C Runtime
#include <string.h>
#include <memory.h>


#include "priv.h"
#include "globals.h"

#include <lzexpand.h>

#include "cpanel.h"
#include "dbutl.h"
#include "resource.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

//
//  local variables and constants
//

static FullPathName_t s_szSetupDir;

//
//  WIN.INI font section name
//

static TCHAR  szFonts[]  = TEXT( "fonts" );

//
//  Globally visible variables
//

FullPathName_t e_szDirOfSrc = { TEXT( '\0' ) };

FullPathName_t   s_szSharedDir;



BOOL FAR PASCAL bCPSetupFromSource( )
{
    return !lstrcmpi( e_szDirOfSrc, s_szSetupDir );
}


void FAR PASCAL vCPUpdateSourceDir( )
{
    lstrcpy( e_szDirOfSrc, s_szSetupDir );
}


void FAR PASCAL vCPWinIniFontChange( )
{
    PostMessage( HWND_BROADCAST, WM_WININICHANGE, NULL, (LPARAM)(LPTSTR)szFonts );
    PostMessage( HWND_BROADCAST, WM_FONTCHANGE,   NULL, 0L );
}


void FAR PASCAL vCPPanelInit( )
{
    s_wBrowseDoneMsg = RegisterWindowMessage( FILEOKSTRING );

    if( TRUE /* !s_hSetup && !s_hAutoInstall && !s_fDoUpgrade */)
    {
        TCHAR cDefDir[ PATHMAX ];

        LoadString( g_hInst, IDSI_MSG_DEFDIR, cDefDir, ARRAYSIZE( cDefDir ) );

        lstrcpy( s_szSetupDir, cDefDir );

        ::GetFontsDirectory( s_szSharedDir, ARRAYSIZE( s_szSharedDir ) );

        lpCPBackSlashTerm( s_szSharedDir );
    }

    vCPUpdateSourceDir( );
}


//
// Determine if a file is located in the fonts directory.
//
BOOL bFileIsInFontsDirectory(LPCTSTR lpszPath)
{
    TCHAR szTemp[MAX_PATH];
    BOOL bResult = FALSE;

    if (NULL != lpszPath)
    {
        //
        // Make a local copy.  String will be modified.
        //
        lstrcpyn(szTemp, lpszPath, ARRAYSIZE(szTemp));

        PathRemoveFileSpec(szTemp); // Strip to path part.
        PathAddBackslash(szTemp);   // Make sure it ends with backslash.

        bResult = (lstrcmpi(szTemp, s_szSharedDir) == 0);
    }
    return bResult;
}


VOID FAR PASCAL vCPStripBlanks( LPTSTR lpszString )
{
    LPTSTR lpszPosn;

    //
    //  strip leading blanks by finding the first non blank.    If this
    //  was a change, recopy the string.  BGK - Q. Why not CharNext here?
    //

    lpszPosn = lpszString;

    while( *lpszPosn == TEXT( ' ' ) )
        lpszPosn++;

    if( lpszPosn != lpszString )
        lstrcpy( lpszString, lpszPosn );

    //
    //  strip trailing blanks
    //

    if( ( lpszPosn = lpszString + lstrlen( lpszString ) ) != lpszString )
    {
        lpszPosn = CharPrev( lpszString, lpszPosn );

        while( *lpszPosn == TEXT( ' ' ) )
            lpszPosn = CharPrev( lpszString, lpszPosn );

        lpszPosn  = CharNext( lpszPosn );
        *lpszPosn = TEXT( '\0' );
    }
}


LPTSTR FAR PASCAL lpCPBackSlashTerm( LPTSTR lpszPath )
{
    LPTSTR lpszEnd = lpszPath + lstrlen( lpszPath );

    if( !*lpszPath )
        goto appendit;

    //
    //  Get the end of the source directory
    //

    if( *CharPrev( lpszPath, lpszEnd ) != TEXT( '\\' ) )
    {
appendit:
        *lpszEnd++ = TEXT( '\\' );
        *lpszEnd   = TEXT( '\0' );
    }

    return lpszEnd;
}

#ifdef WINNT

HANDLE PASCAL wCPOpenFileWithShare( LPTSTR lpszFile,
                                    LPTSTR lpszPath,
                                    WORD   wFlags )
{
    HANDLE  fHandle;


    if( ( fHandle = MyOpenFile( lpszFile, lpszPath, wFlags | OF_SHARE_DENY_NONE ) )
                  == (HANDLE) INVALID_HANDLE_VALUE )
        fHandle = MyOpenFile( lpszFile, lpszPath, wFlags );

    return fHandle;
}

#else

WORD FAR PASCAL wCPOpenFileWithShare( LPTSTR lpszFile,
                                      LPOFSTRUCT pof,
                                      WORD wFlags )
{
    int         fHandle;
    OFSTRUCT    of;

    if( !pof )
        pof = &of;

    if( ( fHandle = OpenFile( lpszFile, pof, wFlags|OF_SHARE_DENY_NONE ) )
                  == HFILE_ERROR )
        fHandle = OpenFile( lpszFile, pof, wFlags );

    return fHandle;
}

#endif  //  WINNT

//
//  This does what is necessary to bring up a dialog box
//

int FAR PASCAL DoDialogBoxParam( int nDlg, HWND hParent, DLGPROC lpProc,
                                 DWORD dwHelpContext, LPARAM dwParam )
{
#if 0 // EMR TODO FIX
    DWORD dwSave;

    dwSave = dwContext;
    dwContext = dwHelpContext;
#endif

    nDlg = (int)DialogBoxParam( g_hInst, MAKEINTRESOURCE( nDlg ), hParent,
                           lpProc, dwParam );

#if 0    // EMR TODO FIX
    dwContext = dwSave;

    if( nDlg == -1 )
        MyMessageBox( hParent, INITS, INITS+1, MB_OK );
#endif

    return( nDlg );
}


int FAR cdecl MyMessageBox (HWND hWnd, DWORD wText, DWORD wCaption, DWORD wType, ...)
{
    TCHAR   szText[ 4 * PATHMAX ], szCaption[ 2 * PATHMAX ];
    va_list parg;


    va_start( parg, wType );

    if( wText == IDS_MSG_NSFMEM /* INITS */)
        goto NoMem;

    if( !LoadString( g_hInst, wText, szCaption, ARRAYSIZE( szCaption ) ) )
        goto NoMem;

    wvsprintf( szText, szCaption, parg );

    if( !LoadString( g_hInst, wCaption, szCaption, ARRAYSIZE( szCaption ) ) )
        goto NoMem;

    wText = (DWORD) MessageBox( hWnd, szText, szCaption,
                                wType | MB_SETFOREGROUND );

    if( wText == (DWORD) -1 )
        goto NoMem;

    va_end( parg );

    return( (int) wText );


NoMem:
    va_end( parg );

    iUIErrMemDlg(hWnd);

    return( -1 );
}


//*****************************************************************
//
//   MyOpenFile()
//
//   Purpose     : To simulate the effects of OpenFile(),
//                 _lcreat and _lopen in a Uniode environment,
//                 but also to be used in non-Unicode environment
//                 as well.
//
//*****************************************************************

HANDLE MyOpenFile( LPTSTR lpszFile, TCHAR * lpszPath, DWORD fuMode )
{
    HANDLE   fh;
    DWORD    len;
    LPTSTR   lpszName;
    TCHAR    szPath[MAX_PATH];
    DWORD    accessMode  = 0;
    DWORD    shareMode   = 0;
    DWORD    createMode  = 0;
    DWORD    fileAttribs = FILE_ATTRIBUTE_NORMAL;


    if( !lpszFile )
        return( INVALID_HANDLE_VALUE );

    //
    //  fuMode of OF_EXIST is looking for the full path name if exist
    //

    if( fuMode & OF_EXIST )
    {
        len = SearchPath( NULL, lpszFile, NULL, MAX_PATH, szPath, &lpszName );

CopyPath:
        if( len )
        {
            if( lpszPath )
                lstrcpy( lpszPath, szPath );

            return( (HANDLE) 1 );
        }
        else
            return( INVALID_HANDLE_VALUE );
    }

    //
    //  fuMode of OF_PARSE is looking for the full path name by merging the
    //  current directory
    //

    if( fuMode & OF_PARSE )
    {
        len = GetFullPathName( lpszFile, MAX_PATH, szPath, &lpszName );
        goto CopyPath;
    }

    //
    //  set up all flags passed for create file.
    //
    //  file access flag

    if( fuMode & OF_WRITE )
        accessMode = GENERIC_WRITE;
    else if( fuMode & OF_READWRITE )
        accessMode = GENERIC_READ | GENERIC_WRITE;
    else
        accessMode = GENERIC_READ;

    //
    //  file sharing flag
    //

    if( fuMode & OF_SHARE_EXCLUSIVE )
        shareMode = 0;
    else if( fuMode & OF_SHARE_DENY_WRITE )
        shareMode = FILE_SHARE_READ;
    else if( fuMode & OF_SHARE_DENY_READ )
        shareMode = FILE_SHARE_WRITE;
    else
        shareMode = FILE_SHARE_READ | FILE_SHARE_WRITE;

    //
    //  set file creation flag
    //

    if( fuMode & OF_CREATE )
        createMode = CREATE_ALWAYS;
    else
        createMode = OPEN_EXISTING;

    //
    //  call CreateFile();
    //

    fh = CreateFile( lpszFile, accessMode, shareMode,
                     NULL, createMode, fileAttribs, NULL );

    if( lpszPath )
        lstrcpy( lpszPath, lpszFile );

    return( fh );

} // end of MyOpenFile()


//*****************************************************************
//
//   MyCloseFile()
//
//   Purpose     : To simulate the effects of _lclose()
//                 in a Uniode environment.
//
//*****************************************************************

BOOL MyCloseFile( HANDLE  hFile )
{
#ifndef  WIN32
    return( !_lclose( (HFILE) hFile ) );
#else
    return( CloseHandle( hFile ) );
#endif

} // end of MyCloseFile()


//*****************************************************************
//
//   MyByteReadFile()
//
//   For Win16, will handle > 64k
//
//*****************************************************************

UINT MyByteReadFile( HANDLE  hFile, LPVOID  lpBuffer, DWORD  cbBuffer )
{
    UINT    cbRead      = (UINT)HFILE_ERROR;
    LPVOID  hpBuffer    = lpBuffer;
    DWORD   dwByteCount = cbBuffer;


#ifdef WIN32
    if (ReadFile( hFile, lpBuffer, cbBuffer, (ULONG *)&cbRead, NULL ))
        return cbRead;
    else
        return (UINT)HFILE_ERROR;
#else
    while( dwByteCount > MAX_UINT )
    {
        if( _lread( (HFILE) hFile, hpBuffer, MAX_UINT ) == MAX_UINT )
            return( (UINT)HFILE_ERROR );

        dwByteCount -= MAX_UINT;
        hpBuffer += MAX_UINT;
    }
    return( _lread( (HFILE) hFile, lpBuffer, dwBytesCount ) == dwBytesCount );
#endif

} // end of MyByteReadFile()


//*****************************************************************
//
//   MyAnsiReadFile()
//
//   Purpose     : To simulate the effects of _lread() in a Unicode
//                 environment by reading into an ANSI buffer and
//                 then converting to Uniode text.
//
//*****************************************************************

UINT MyAnsiReadFile( HANDLE  hFile,
                     UINT    uCodePage,
                     LPVOID  lpUnicode,
                     DWORD   cchUnicode)
{
    LPSTR lpAnsi  = NULL;
    UINT  cbRead  = (UINT)HFILE_ERROR;
    UINT  cbAnsi  = cchUnicode * sizeof(TCHAR);
    UINT  cchRead = 0;

    lpAnsi = (LPSTR) LocalAlloc(LPTR, cbAnsi);
    if (NULL != lpAnsi)
    {
        cbRead = MyByteReadFile( hFile, lpAnsi, cbAnsi );

        if( HFILE_ERROR != cbRead )
        {
            cchRead = MultiByteToWideChar( uCodePage,
                                           0,
                                           lpAnsi,
                                           cbRead,
                                           (LPWSTR)lpUnicode,
                                           cchUnicode);
        }
        LocalFree( lpAnsi );
    }

    return( cchRead );

} // end of MyAnsiReadFile()


//*****************************************************************
//
//   MyByteWriteFile()
//
//   For Win16, will handle > 64k
//
//*****************************************************************

UINT MyByteWriteFile( HANDLE hFile, LPVOID lpBuffer, DWORD cbBuffer )
{
    UINT    cbWritten   = (UINT)HFILE_ERROR;
    LPVOID  hpBuffer    = lpBuffer;
    DWORD   dwByteCount = cbBuffer;

#ifdef WIN32
    if (WriteFile( hFile, lpBuffer, cbBuffer, (ULONG *)&cbWritten, NULL ))
        return cbWritten;
    else
        return (UINT)HFILE_ERROR;
#else
    while( dwByteCount > MAX_UINT )
    {
        if( _lwrite( (HFILE) hFile, hpBuffer, MAX_UINT ) == MAX_UINT )
            return( (UINT)HFILE_ERROR );

        dwByteCount -= MAX_UINT;
        hpBuffer += MAX_UINT;
    }

    return( _lwrite( (HFILE) hFile, lpBuffer, dwBytesCount ) == dwBytesCount );
#endif

} // end of MyByteWriteFile()


//*****************************************************************
//
//   MyAnsiWriteFile()
//
//   Purpose     : To simulate the effects of _lwrite() in a Unicode
//                 environment by converting to ANSI buffer and
//                 writing out the ANSI text.
//
//*****************************************************************

UINT MyAnsiWriteFile( HANDLE  hFile,
                      UINT uCodePage,
                      LPVOID lpUnicode,
                      DWORD cchUnicode)
{
    LPSTR   lpAnsi    = NULL;
    UINT    cbAnsi    = 0;
    UINT    cbWritten = (UINT)HFILE_ERROR;

    //
    // Calculate byte requirement for ansi buffer.
    //
    cbAnsi = WideCharToMultiByte (uCodePage,
                                  0,
                                  (LPWSTR)lpUnicode,
                                  cchUnicode,
                                  NULL,
                                  0,
                                  NULL,
                                  NULL);

    //
    // Allocate the ansi buffer and convert characters to ansi.
    //
    lpAnsi = (LPSTR) LocalAlloc(LPTR, cbAnsi);
    if (NULL != lpAnsi)
    {
        WideCharToMultiByte( uCodePage,
                             0,
                             (LPWSTR)lpUnicode,
                             cchUnicode,
                             lpAnsi,
                             cbAnsi,
                             NULL,
                             NULL );

        cbWritten = MyByteWriteFile( hFile, lpAnsi, cbAnsi );

        LocalFree( lpAnsi );
    }

    return( cbWritten );

} // end of MyAnsiWriteFile()


//*****************************************************************
//
//   MyFileSeek()
//
//   Purpose     : To simulate the effects of _lseek() in a Unicode
//                 environment.
//
//*****************************************************************

LONG MyFileSeek( HANDLE hFile, LONG lDistanceToMove, DWORD dwMoveMethod )
{
#ifdef WIN32
    return( SetFilePointer( hFile, lDistanceToMove, NULL, dwMoveMethod ) );
#else
    return( _llseek( (HFILE) hFile, lDistanceToMove, (int) dwMoveMethod ) );
#endif

} // end of MyFileSeek()



/////////////////////////////////////////////////////////////////////////////
//
// AllocMem
//
//
// Routine Description:
//
//     This function will allocate local memory. It will possibly allocate
//     extra memory and fill this with debugging information for the
//     debugging version.
//
// Arguments:
//
//     cb - The amount of memory to allocate
//
// Return Value:
//
//     NON-NULL - A pointer to the allocated memory
//
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
//
/////////////////////////////////////////////////////////////////////////////

LPVOID AllocMem( DWORD cb )
{
    LPDWORD  pMem;
    DWORD    cbNew;


    cbNew = cb + 2 * sizeof( DWORD );

    if( cbNew & 3 )
        cbNew += sizeof( DWORD ) - ( cbNew & 3 );

    pMem = (LPDWORD) LocalAlloc( LPTR, cbNew );

    if( !pMem )
        return NULL;

    // memset (pMem, 0, cbNew);     // This might go later if done in NT

    *pMem = cb;

    *(LPDWORD) ( (LPBYTE) pMem + cbNew - sizeof( DWORD ) ) = 0xdeadbeef;

    return (LPVOID)( pMem + 1 );
}


/////////////////////////////////////////////////////////////////////////////
//
// FreeMem
//
//
// Routine Description:
//
//     This function will allocate local memory. It will possibly allocate
//     extra memory and fill this with debugging information for the
//     debugging version.
//
// Arguments:
//
//     pMem - Pointer to memory to free
//     cb   - size of memory block to free
//
// Return Value:
//
//     NON-NULL - Memory successfully freed
//
//     FALSE/NULL - The operation failed. Extended error status is available
//                  using GetLastError.
//
/////////////////////////////////////////////////////////////////////////////

BOOL FreeMem( LPVOID pMem, DWORD  cb )
{
    DWORD   cbNew;
    LPDWORD pNewMem;


    if( !pMem )
        return TRUE;

    pNewMem = (LPDWORD) pMem;
    pNewMem--;


#ifdef NO_COUNT_NEEDED
    if( cb == 0 )
    {
        cb = cbNew = *pNewMem;
    }
    else
    {
        cbNew = cb + 2 * sizeof( DWORD );

        if( cbNew & 3 )
            cbNew += sizeof( DWORD ) - ( cbNew & 3 );
    }
#else

    cbNew = cb + 2 * sizeof( DWORD );

    if( cbNew & 3 )
        cbNew += sizeof( DWORD ) - ( cbNew & 3 );


#endif  // NO_COUNT_NEEDED


    if( ( *pNewMem != cb ) ||
        ( *(LPDWORD) ( (LPBYTE) pNewMem + cbNew - sizeof( DWORD ) ) != 0xdeadbeef ) )
    {
        DEBUGMSG( (DM_TRACE1, TEXT("Corrupt Memory in FontFolder : %0lx\n"), pMem ) );
    }

    return ( ( (HLOCAL) pNewMem == LocalFree( (LPVOID) pNewMem ) ) );
}


LPVOID ReallocMem( LPVOID lpOldMem, DWORD cbOld, DWORD cbNew )
{
   LPVOID lpNewMem;

   lpNewMem = AllocMem( cbNew );

   if( lpOldMem )
   {
      memcpy( lpNewMem, lpOldMem, min( cbNew, cbOld ) );

      FreeMem( lpOldMem, cbOld );
   }

   return lpNewMem;
}


/////////////////////////////////////////////////////////////////////////////
//
// Routine Description:
//
//     These functions will allocate or reallocate enough local memory to
//     store the specified  string, and copy that string to the allocated
//     memory.  The FreeStr function frees memory that was initially
//     allocated by AllocStr.
//
// Arguments:
//
//     lpStr - Pointer to the string that needs to be allocated and stored
//
// Return Value:
//
//     NON-NULL - A pointer to the allocated memory containing the string
//
//     FALSE/NULL - The operation failed. Extended error status is available
//         using GetLastError.
//
/////////////////////////////////////////////////////////////////////////////

LPTSTR AllocStr( LPTSTR lpStr )
{
   LPTSTR lpMem;

   if( !lpStr )
      return NULL;

   if( lpMem = (LPTSTR) AllocMem( ( lstrlen( lpStr ) + 1 ) * sizeof( TCHAR ) ) )
      lstrcpy( lpMem, lpStr );

   return lpMem;
}


BOOL FreeStr( LPTSTR lpStr )
{
   return lpStr ? FreeMem( lpStr, ( lstrlen( lpStr ) + 1 ) * sizeof( TCHAR ) )
                : FALSE;
}


BOOL ReallocStr( LPTSTR *plpStr, LPTSTR lpStr )
{
   FreeStr( *plpStr );

   *plpStr = AllocStr( lpStr );

   return TRUE;
}

//////////////////////////////////////////////////////////////////////////////
//
// CentreWindow
//
// Purpose : Positions a window so that it is centred in its parent
//
// History:
// 12-09-91 Davidc       Created.
//
//////////////////////////////////////////////////////////////////////////////

VOID CentreWindow( HWND hwnd )
{
    RECT    rect;
    RECT    rectParent;
    HWND    hwndParent;
    LONG    dx, dy;
    LONG    dxParent, dyParent;
    LONG    Style;


    //
    //  Get window rect
    //

    GetWindowRect( hwnd, &rect );

    dx = rect.right - rect.left;
    dy = rect.bottom - rect.top;

    //
    //  Get parent rect
    //

    Style = GetWindowLong( hwnd, GWL_STYLE );

    if( (Style & WS_CHILD) == 0 )
    {
        hwndParent = GetDesktopWindow( );
    }
    else
    {
        hwndParent = GetParent( hwnd );

        if( hwndParent == NULL )
        {
            hwndParent = GetDesktopWindow( );
        }
    }

    GetWindowRect( hwndParent, &rectParent );

    dxParent = rectParent.right - rectParent.left;
    dyParent = rectParent.bottom - rectParent.top;

    //
    //  Centre the child in the parent
    //

    rect.left = ( dxParent - dx ) / 2;
    rect.top  = ( dyParent - dy ) / 3;

    //
    //  Move the child into position
    //

    SetWindowPos( hwnd, NULL, rect.left, rect.top, 0, 0,
                  SWP_NOSIZE | SWP_NOZORDER );

    SetForegroundWindow( hwnd );
}


///////////////////////////////////////////////////////////////////////////////
//
// FUNCTION: GetFirstAncestor
//
// DESCRIP:  Locates the first ancestor of a given window.
//
//////////////////////////////////////////////////////////////////////////////
HWND GetFirstAncestor(HWND hWnd)
{
    HWND hwndAncestor = NULL;

    //
    // Find the top-level parent of the window that was passed in.
    //
    while ((hwndAncestor = GetParent(hWnd)) != NULL)
        hWnd = hwndAncestor;

    return hWnd;
}


