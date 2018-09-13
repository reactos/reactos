///////////////////////////////////////////////////////////////////////////////
//
// fontcl.cpp
//      Explorer Font Folder extension routines.
//      module to handle classes defined in fontcl.h:
//      CFontClass and DirFilenameClass
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//   All routines for these classes are in this module, EXCEPT:
//   1) inline functions - in FONTCL.H of course
//
// $keywords: fontcl.cpp 1.7  4-May-94 5:24:41 PM$
//
//***************************************************************************
// $lgb$
// 1.0     7-Mar-94 eric Initial revision.
// 1.1     9-Mar-94 eric Background thread and g_hDBMutex
// 1.2     9-Mar-94 eric Added m_bFilledIn
// 1.3     7-Apr-94 eric Removed LoadLibrary on FOT files.
// 1.4     8-Apr-94 eric Added s_szFontsDir
// 1.5    13-Apr-94 eric Calling bFillIn appropriately
// 1.6    15-Apr-94 eric Rip control
// 1.7     4-May-94 build GetOTM changes
// $lge$
//***************************************************************************
//
//  Copyright (C) 1992-93 ElseWare Corporation. All rights reserved.
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
#include "globals.h"

#include <sys/types.h>
#include <sys/stat.h>

#include "fontcl.h"
#include "fdirvect.h"
#include "fontdir.h"
#include "resource.h"

#include "lstrfns.h"
#include "dbutl.h"
#include "cpanel.h"
#include "fontman.h"

#include <shlobjp.h>

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

#ifdef WINNT

#ifdef __cplusplus
extern "C" {
#endif

//
// [stevecat]   This used to reside in "wingdip.h" (included with <winp.h>)
//  6/29/95     but I have taken it out because of C++ name-mangling problems
//              with that header file that are not going to be fixed because
//              this file is going to change significantly (according to
//              EricK) when we switch over to Kernel mode GDI/User.
//
//
//#include <stddef.h>     //  Needed for winp.h
//#include <winp.h>       //  For private GDI entry point:  GetFontResourceInfo
//

// Private Control Panel entry point to enumerate fonts by file.

#define GFRI_NUMFONTS       0L
#define GFRI_DESCRIPTION    1L
#define GFRI_LOGFONTS       2L
#define GFRI_ISTRUETYPE     3L
#define GFRI_TTFILENAME     4L
#define GFRI_ISREMOVED      5L
#define GFRI_FONTMETRICS    6L


#include <winfont.h> //Type1 PFM file offsets and reader macros.

extern BOOL WINAPI GetFontResourceInfoW( LPWSTR  lpPathname,
                                         LPDWORD lpBytes,
                                         LPVOID  lpBuffer,
                                         DWORD   iType );

#ifdef __cplusplus
}
#endif

#endif


#define  BYTESTOK(Len) ((Len + 1023) / 1024)   // Convert from bytes to K

//
// Include file to define types and constants needed by the font manager
//   and font routines.
//

#define DEFAULTPOINTS        14

#define FONT_MAN_VERSION     0x0100
#define SMART_FONT_VERSION   0x0200
#define NEW_FONT_VERSION     0x0300

#ifdef ROM
HANDLE FAR PASCAL IsROMModule( LPTSTR lpName, BOOL fSelector );
#endif

CFDirVector * CFontClass::s_poDirList = 0;


BOOL bTTFFromFOT( LPTSTR lpFOTPath, LPTSTR lpTTF, WORD wLen )
{
    PATHNAME szTTFPath;
    BOOL     bValid;

#ifdef WINNT

    ULONG    iSize = MAX_PATH_LEN;


    bValid = GetFontResourceInfoW( lpFOTPath, &iSize, szTTFPath,
                                   GFRI_TTFILENAME );

#else

    HFILE    hfile;
    BOOL     bOpen;


    hfile  = _lopen( lpFOTPath, OF_READ );

    bValid = hfile != HFILE_ERROR;

    //
    //  Peek into the FOT file to find the TTF name
    //

    if( bValid )
    {
        bOpen  = TRUE;
        bValid = _llseek( hfile, 0x400, 0 ) != HFILE_ERROR;
    };

    if( bValid )
         bValid = _lread( hfile, szTTFPath, sizeof( szTTFPath ) ) != HFILE_ERROR;

    if( bOpen )
        _lclose( hfile );

#endif  //  WINNT

    //
    //  We make sure there's a terminating null at the end of the path name.
    //  Then make up a full name.
    //

    if( bValid )
    {
        szTTFPath[ MAX_PATH_LEN ] = TEXT( '\0' );

        bValid = bMakeFQName( lpTTF, szTTFPath, wLen );
    }

    return bValid;

}


/***************************************************************************
 * FUNCTION:
 *
 * PURPOSE:  Load the full directory/path/filename into the given string.
 *              The full name includes the directory name from the database,
 *              with the file name appended to it.
 *
 * RETURNS:  void
 ***************************************************************************/

int GetFontsDirectory( LPTSTR lpPath, int iLen )
{
    static   FullPathName_t s_szFontsDir;
    static   BOOL           s_Fetched = FALSE;
    static   int            iRet = 0;

    //
    //  Get the system directory, which we'll store and pre-fix whenever
    //  a file needs it. Note that there isn't a backslash unless the name
    //  is the root directory.
    //

    if( ( !s_Fetched &&
          SHGetSpecialFolderPath( NULL, s_szFontsDir, CSIDL_FONTS, FALSE ) ) )
    {
        DWORD dwAttr = GetFileAttributes( s_szFontsDir );

        if( dwAttr != (DWORD) -1 && ( dwAttr & FILE_ATTRIBUTE_DIRECTORY ) )
        {
            s_Fetched = TRUE;
            iRet = lstrlen( s_szFontsDir );
        }
    }


    if( !s_Fetched )
    {
        int wLen = GetSystemWindowsDirectory( s_szFontsDir, ARRAYSIZE( s_szFontsDir ) );

        s_Fetched = ( wLen > 0 ) && ( wLen <= ARRAYSIZE( s_szFontsDir ) );

        //
        //  Build up the Fonts directory. This is no longer the System folder.
        //

        if( s_Fetched )
        {
            //
            // Find the last TEXT( "\" ).
            //

            if( s_szFontsDir[ wLen-1 ] != TEXT( '\\' ) )
            {
                s_szFontsDir[ wLen ]   = TEXT( '\\' );
                s_szFontsDir[ wLen+1 ] = 0;
                wLen++;
            }

            LoadString( g_hInst,
                        IDS_FONTS_FOLDER,
                        s_szFontsDir + wLen,
                        ARRAYSIZE( s_szFontsDir ) - wLen );
        }

        iRet = lstrlen( s_szFontsDir );
    }

#ifdef UNICODE
    //
    //  Make an ANSI version of szSharedDir for t1instal call
    //
    if( WideCharToMultiByte( CP_ACP, 0, s_szFontsDir, -1,
                              g_szFontsDirA, PATHMAX, NULL, NULL ) == FALSE )
    {
        //
        //  Report initialization error
        //

        iUIMsgExclaim(NULL, MYFONT + 18, s_szFontsDir );

        return (FALSE);
    }
#else
    lstrcpy(g_szFontsDirA, s_szFontsDir);
#endif

    if( iRet <= iLen )
        lstrcpy( lpPath, s_szFontsDir );

    return iRet;
}


/***************************************************************************
 * DirFilenameClass routines:
 ***************************************************************************/

/***************************************************************************
 * FUNCTION: vGetFullName
 *
 * PURPOSE:  Load the full directory/path/filename into the given string.
 *              The full name includes the directory name from the database,
 *              with the file name appended to it.
 *
 * RETURNS:  void
 ***************************************************************************/

void DirFilenameClass :: vGetFullName( PTSTR pStr )
{
    ASSERT(NULL != m_poDir);
    lstrcpy( pStr, m_poDir->lpString( ) );

    if( pStr[ lstrlen( pStr ) - 1 ] != TEXT( '\\' ) )
        lstrcat( pStr, TEXT( "\\" ) );

    lstrcat( pStr, m_szFOnly );
}


/***************************************************************************
 * FUNCTION: rcStoreDirFN
 *
 * PURPOSE:  Store a directory path in our path list.
 *
 * RETURNS:  RC - NOERR unless directory list full or name too long
 ***************************************************************************/

CFontDir * CFontClass::poAddDir( LPTSTR lpPath, LPTSTR * lpName )
{
    *lpName = NULL;

    LPTSTR lpLastSlash;
    LPTSTR lpFileOnly;
    CFontDir * poDir = 0;

    //
    //  The first time through, allocate the directory struct.
    //

    if( !s_poDirList )
    {
        s_poDirList  = new CFDirVector( 64 );

        if( !s_poDirList )
            return 0;

        if( !s_poDirList->bInit( ) )
        {
            s_poDirList = 0;
            return 0;
        }

        //
        //  Load the default directory into the font dir list. It is expected
        //  to be at location 0, so we add it first.
        //

        FullPathName_t szBaseDir;

        int wLen = GetFontsDirectory( szBaseDir, ARRAYSIZE( szBaseDir ) );

        CFontDir * poDir = new CFontDir;

        if( poDir && poDir->bInit( szBaseDir, wLen ) )
        {
            poDir->vOnSysDir( TRUE );

            if( !s_poDirList->bAdd( poDir ) )
            {
                delete poDir;

                delete s_poDirList;

                s_poDirList = 0;

                return 0;
            }
        }
        else
        {
            delete s_poDirList;

            s_poDirList = 0;

            return 0;
        }

        //
        //  Add the <win>\system directory. We use it for compatibility reasons.
        // It is located in slot 1.
        //

        wLen = GetSystemDirectory( szBaseDir, ARRAYSIZE( szBaseDir ) );

        poDir = new CFontDir;

        if( poDir && poDir->bInit( szBaseDir, wLen ) )
        {
            poDir->vOnSysDir( TRUE );

            if( !s_poDirList->bAdd( poDir ) )
            {
                delete poDir;

                delete s_poDirList;

                s_poDirList = 0;

                return 0;
            }
        }
        else
        {
            delete s_poDirList;

            s_poDirList = 0;

            return 0;
        }
    }

    //
    //  If we find a backslash in the path name, the name includes a directory
    //  If so, we're going to store the directory name in a separate list.
    //  Otherwise, we reserve slot 0 for the default directory cases.
    //

    // Force same case file (?)
    // lstrcpy( lpPath, /* _strlwr */ (lpPath ) );

    lpLastSlash = StrRChr( lpPath, NULL, TEXT( '\\' ) );


    TCHAR szTempFile[ MAX_PATH_LEN ];


    if( !lpLastSlash )
    {
        if( !bMakeFQName( szTempFile, lpPath, MAX_PATH_LEN, TRUE ) )
        {
            return( NULL );
        }

        *lpName = lpPath;
        lpPath = szTempFile;

        lpLastSlash = StrRChr( lpPath, NULL, TEXT( '\\' ) );

        if( !lpLastSlash )
        {
            //
            // This should never happen
            //

            return( NULL );
        }
    }

    lpFileOnly = lpLastSlash+1;

    //
    // Try to find the directory in the list.
    //

    int iLen = (int)(lpFileOnly - lpPath - 1);

    poDir = s_poDirList->fdFindDir( lpPath, iLen, TRUE );

    if( !*lpName )
    {
        *lpName = lpFileOnly;
    }

    return poDir;
}


RC CFontClass :: rcStoreDirFN( LPTSTR lpszPath, DirFilenameClass& dirfn )
{
    RC     rc = ERR_FAIL;
    LPTSTR lpName;
    CFontDir * poDir = poAddDir( lpszPath, &lpName );

    if( poDir && ( lstrlen( lpName ) <= MAX_FILE_LEN ) )
    {
        dirfn.vSet( poDir, lpName );

        rc = NOERR;
    }

    return rc;
}


#if 0
/***************************************************************************
 * FUNCTION: vStuffPANOSE
 *
 * PURPOSE:  Load the PANOSE info for this font (if needed).
 *
 * RETURNS:  void
 ***************************************************************************/

void CFontClass :: vStuffPANOSE( )
{
    PANOSEBytesClass xUsePANOSE;    // Will initialize as dummy;

    //
    // The best thing would be to get the PANOSE info from the font itself,
    // so try that first.  Even if we can read it, we need to determine if
    // we really have a PANOSE number, or just a dummy.  The problem is,
    // the TT file will have the OS2 table, but the PANOSE number might
    // be bogus.  With any luck, it'll be something we can recognize as
    // invalid.
    //

    if( rcPANOSEFromTTF( xUsePANOSE ) == NOERR )
        m_fHavePANOSE = xUsePANOSE.bVerify( );
    else
        m_fHavePANOSE = FALSE;

    //
    //  If we don't have a valid PANOSE number, make sure we've got a clean one
    //

    if( !m_fHavePANOSE )
        xUsePANOSE.vClear( );

    m_jFamily = xUsePANOSE.jFamily( );

    memcpy( m_xPANOSE.m_ajNumMem,
            xUsePANOSE.m_ajBytes,
            sizeof( m_xPANOSE.m_ajNumMem ) );

}
#endif

DWORD CFontClass :: dCalcFileSize( )
{
    //
    //  First we get the size of the basis file
    //

    GetFileInfo( );

    return m_wFileK;
}


/***************************************************************************
 * Start of Public routines
 ***************************************************************************/

/***************************************************************************
 * FUNCTION: bAFR
 *
 * PURPOSE:  Add font resource
 *
 * RETURNS:  TRUE on success.
 ***************************************************************************/

BOOL CFontClass::bAFR( )
{
    if( !m_bAFR )
    {
        FullPathName_t szFile;
        LPTSTR pszResourceName = szFile;

#ifdef WINNT
        TCHAR szType1FontResourceName[MAX_TYPE1_FONT_RESOURCE];
#endif

        if( !bGetFOT( szFile, ARRAYSIZE( szFile ) ) )
        {
            bGetFQName( szFile, ARRAYSIZE( szFile ) );
        }

#ifdef WINNT

        if (bType1())
        {
            //
            // Font is a Type1.
            // Create a Type1 font resource name as:  "<pfm>|<pfb>"
            //
            TCHAR szPfbPath[MAX_PATH];

            if (bGetPFB(szPfbPath, ARRAYSIZE(szPfbPath)) &&
                BuildType1FontResourceName(szFile,
                                           szPfbPath,
                                           szType1FontResourceName,
                                           ARRAYSIZE(szType1FontResourceName)))
            {
                pszResourceName = szType1FontResourceName;
            }
        }
#endif // WINNT

        if( AddFontResource(pszResourceName) )
            m_bAFR = TRUE;
   }

   return m_bAFR;
}


void CFontClass::GetFileInfo( )
{
    TCHAR szPath[ MAX_PATH ];
    WIN32_FIND_DATA fd;

//
// TODO: [stevecat] Change this to check the FONT Type for this font.
//                  if bType1 == TRUE then add size of PFB file also
//                  to total font size for a more accurate representation
//                  of total Type1 font file sizes.
//


    if( !m_bFileInfoFetched )
    {
        m_ft.dwLowDateTime  = 0;
        m_ft.dwHighDateTime = 0;

        if( bGetFQName( szPath, ARRAYSIZE( szPath ) ) )
        {
            HANDLE hfind = FindFirstFile( szPath, &fd );

            m_bFileInfoFetched  = TRUE;

            if( hfind != INVALID_HANDLE_VALUE )
            {
                m_wFileK = (UINT) BYTESTOK( fd.nFileSizeLow );
                m_ft     = fd.ftLastWriteTime;

                FindClose( hfind );
            }
        }

//
// [stevecat] Add check for existing PFB file and add its size to this
//  FIXIFX - test this to be sure PFB filename is getting full path for
//           FindFile call.
//

        if( bPFB( ) )
        {
//            TCHAR szPfbFile[ MAX_PATH ];

//            if( bGetPFB( szPfbFile, ARRAYSIZE( szPfbFile ) )
//            {
//                if( bMakeFQName( szPath, szPfbFile, ARRAYSIZE( szPath ) ) )

            if( bMakeFQName( szPath, m_lpszPFB, ARRAYSIZE( szPath ) ) )
            {
                HANDLE hfind = FindFirstFile( szPath, &fd );

                m_bFileInfoFetched  = TRUE;

                if( hfind != INVALID_HANDLE_VALUE )
                {
                    m_wFileK += (UINT) BYTESTOK( fd.nFileSizeLow );

                    FindClose( hfind );
                }
            }
//            }
        }

    }
}


BOOL CFontClass::GetFileTime( FILETIME * pft )
{
    GetFileInfo( );

    *pft = m_ft;

    return( pft->dwLowDateTime || pft->dwHighDateTime );
}


/***************************************************************************
 * FUNCTION: bRFR
 *
 * PURPOSE:  Remove font resource
 *
 * RETURNS:  TRUE on success.
 ***************************************************************************/
BOOL CFontClass::bRFR( )
{
    if( m_bAFR )
    {
        FullPathName_t szFile;
        LPTSTR pszResourceName = szFile;

#ifdef WINNT
        TCHAR szType1FontResourceName[MAX_TYPE1_FONT_RESOURCE];
#endif

        //
        // GDI seems to be particular about full pathname and partial name.
        // Try both if necessary.
        //

        if( !bGetFOT( szFile, ARRAYSIZE( szFile ) ) )
        {
            bGetFQName( szFile, ARRAYSIZE( szFile ) );
        }

        m_bAFR = FALSE;

#ifdef WINNT

        if (bType1())
        {
            //
            // Font is a Type1.
            // Create a Type1 font resource name as:  "<pfm>|<pfb>"
            //
            TCHAR szPfbPath[MAX_PATH];

            if (bGetPFB(szPfbPath, ARRAYSIZE(szPfbPath)) &&
                BuildType1FontResourceName(szFile,
                                           szPfbPath,
                                           szType1FontResourceName,
                                           ARRAYSIZE(szType1FontResourceName)))
            {
                pszResourceName = szType1FontResourceName;
            }
        }

#endif // WINNT

        if( !RemoveFontResource( pszResourceName ) )
        {
            TCHAR szFN[ MAX_PATH_LEN ];

            vGetFileName( szFN );

            if( bFOT( ) || !RemoveFontResource( szFN ) )
            {
                //
                // If the file doesn't exist, then it couldn't be in GDI.
                //

                if( GetFileAttributes( szFile ) != 0xffffffff )
                    m_bAFR = TRUE;
            }
        }
    }

    return( !m_bAFR );
}


#ifndef    CHECKFILENAMEFIRST

BOOL CFontClass :: bInit( LPTSTR lpszDesc, LPTSTR lpFileName, LPTSTR lpCompanionFile )
{
    static const TCHAR    c_szPLOTTER     [] = TEXT( " (PLOTTER)" );
    static const TCHAR    c_szTRUETYPE    [] = TEXT( " (TRUETYPE)" );
    static const TCHAR    c_szTRUETYPEALT [] = TEXT( " (TRUE TYPE)" );
    static const TCHAR    c_szTYPE1       [] = TEXT( " (TYPE 1)" );
    static const TCHAR    c_szTYPE1ALT    [] = TEXT( " (POSTSCRIPT)" );
    static const TCHAR    c_szFOT         [] = TEXT( ".FOT" );
    static const TCHAR    c_szTTF         [] = TEXT( ".TTF" );
    static const TCHAR    c_szTTC         [] = TEXT( ".TTC" );
    static const TCHAR    c_szOTF         [] = TEXT( ".OTF" );
    static const TCHAR    c_szPFM         [] = TEXT( ".PFM" );
    static const TCHAR    c_szINF         [] = TEXT( ".INF" );

    LPTSTR  pTT;
    LPTSTR  lpszEn;
    BOOL    bSuccess = TRUE;


    LPCTSTR lpName = lpNamePart( lpFileName );


    if( !lpName )
    {
        lpName = lpFileName;
    }

    FullPathName_t szName;

    lstrcpyn( szName, lpName, ARRAYSIZE( szName ) );

    CharUpper( szName );

    m_bFileInfoFetched = FALSE;

    //
    // Store the file name.
    //

    if( rcStoreDirFN( lpFileName ) != NOERR )
    {
        delete this;
        return FALSE;
    }

    //
    // Figure out what type of font this is.
    //

    lpszEn = _tcsstr( lpszDesc, TEXT( " (" ) );

    if( lpszEn == NULL )
    {
        //
        //  There's no additional description, so set filetype based on
        //  extension.
        //

        m_wNameLen = (BYTE)lstrlen( lpszDesc );

        if( _tcsstr( szName, c_szTTF ) )
            vSetTrueType( FALSE );
        else if( _tcsstr( szName, c_szOTF ) )
            vSetOpenType( );
        else if( _tcsstr( szName, c_szTTC ) )
            vSetTTCType( );
        else if( _tcsstr( szName, c_szPFM ) )
        {
            vSetType1( );

            if( lpCompanionFile != NULL )
                bSetPFB( lpCompanionFile );
        }
        else if( _tcsstr( szName, c_szINF ) )
        {
            vSetType1( );

            if( lpCompanionFile != NULL )
                bSetPFB( lpCompanionFile );
        }
    }
    else
    {
        m_wNameLen = (BYTE)(lpszEn-lpszDesc);

        FontDesc_t szEn;

        lstrcpyn( szEn, lpszEn, ARRAYSIZE( szEn ) );

        CharUpper( szEn );

        pTT = _tcsstr( szEn, c_szTRUETYPE );

        if( !pTT )
            pTT = _tcsstr( szEn, c_szTRUETYPEALT );

        if( pTT )
        {
            //
            //  This is either a TTF or an FOT
            //

            BOOL bFOT = ( _tcsstr( szName, c_szFOT ) != (LPTSTR) NULL );

            if( bFOT )
            {
                FullPathName_t szTTF;
                FullPathName_t szFOT;

                if( bMakeFQName( szFOT, lpFileName, ARRAYSIZE( szFOT ) ) )
                {
                    bSuccess = bTTFFromFOT( szFOT, szTTF, ARRAYSIZE( szTTF ) );

                    if( bSuccess )
                    {
                        if( !bSetFOT( szFOT ) )
                        {
                            delete this;
                            return( FALSE );
                        }

                        return bInit( lpszDesc, szTTF, NULL );
                    }
                }

                //
                //  error
                //

                delete this;
                return FALSE;
            }

            vSetTrueType( bFOT );

            if( _tcsstr( szName, c_szTTC ) )
                vSetTTCType( );
        }
        else if( _tcsstr( szName, c_szOTF ) )
        {
            vSetOpenType( );
        }
        else if( _tcsstr( szName, c_szTTC ) )
        {
            vSetTTCType( );
        }
        else if( _tcsstr( szName, c_szTYPE1 ) )
        {
            vSetType1( );

            if( lpCompanionFile != NULL )
                bSetPFB( lpCompanionFile );
        }
        else if( _tcsstr( szName, c_szTYPE1ALT ) )
        {
            vSetType1( );

            if( lpCompanionFile != NULL )
                bSetPFB( lpCompanionFile );
        }
        else    //   if( _tcsstr( szEn, c_szPLOTTER ) == NULL )
        {
            vSetDeviceType( );
        }
    }

    lstrcpyn( m_szFontLHS, lpszDesc, ARRAYSIZE( m_szFontLHS ) );

    //
    //  It is assumed that this font is already installed.
    //  Set the flag to assume it.
    //

    m_bAFR = TRUE;

    //
    // Set this to be the main family font. This will be reset as necessary.
    //

    m_wFamIdx = IDX_NULL;

    vSetFamilyFont( );

    //
    // Invalidate the font object's cached file attributes.
    // They will be refreshed next time dwGetFileAttributes() is called for
    // this object.
    //
    InvalidateFileAttributes();

    return bSuccess;
}

#else    // !CHECKFILENAMEFIRST

BOOL CFontClass :: bInit( LPTSTR lpszDesc, LPTSTR lpFileName, LPTSTR lpCompanionFile )
{
    static const TCHAR    c_szTRUETYPE    [] = TEXT( " (TRUETYPE)" );
    static const TCHAR    c_szTRUETYPEALT [] = TEXT( " (TRUE TYPE)" );
    static const TCHAR    c_szTYPE1       [] = TEXT( " (TYPE 1)" );
    static const TCHAR    c_szTYPE1ALT    [] = TEXT( " (POSTSCRIPT)" );
    static const TCHAR    c_szFOT         [] = TEXT( ".FOT" );
    static const TCHAR    c_szTTF         [] = TEXT( ".TTF" );
    static const TCHAR    c_szTTC         [] = TEXT( ".TTC" );
    static const TCHAR    c_szFON         [] = TEXT( ".FON" );
    static const TCHAR    c_szPFM         [] = TEXT( ".PFM" );
    static const TCHAR    c_szINF         [] = TEXT( ".INF" );

    BOOL    bFoundType = FALSE;

    m_bFileInfoFetched = FALSE;

    //
    //  Store the file name.
    //

    if( rcStoreDirFN( lpFileName ) != NOERR )
    {
        delete this;
        return FALSE;
    }

    //
    //  Figure out what type of font this is.
    //

    LPTSTR lpExt = PathFindExtension( lpFileName );

    if( lpExt )
    {
        bFoundType = TRUE;

        FullPathName_t szExt;

        lstrcpyn( szExt, lpExt, ARRAYSIZE( szExt ) );

        PathRemoveBlanks( szExt );

        if( _tcsicmp( szExt, c_szTTF ) == 0 )
        {
            vSetTrueType( FALSE );
        }
        else if( _tcsicmp( szName, c_szTTC ) == 0 )
        {
            vSetTTCType( );
        }
        else if( _tcsicmp( szName, c_szFON ) == 0 )
        {
            vSetDeviceType( );
        }
        else if( _tcsicmp( szName, c_szPFM ) == 0 )
        {
            vSetType1( );

            if( lpCompanionFile != NULL )
                bSetPFB( lpCompanionFile );
        }
        else if( _tcsicmp( szName, c_szINF ) == 0 )
        {
            vSetType1( );

            if( lpCompanionFile != NULL )
                bSetPFB( lpCompanionFile );
        }
        else if( _tcsicmp( szName, c_szFOT ) == 0 )
        {
            FullPathName_t szTTF;
            FullPathName_t szFOT;

            if( bMakeFQName( szFOT, lpFileName, ARRAYSIZE( szFOT ) ) )
            {
                if( bTTFFromFOT( szFOT, szTTF, ARRAYSIZE( szTTF ) ) )
                {
                    if( bSetFOT( szFOT ) )
                    {
                        delete this;
                        return bInit( lpszDesc, szTTF, NULL );
                    }
                }
            }

            //
            //  Bad thing if we got here
            //

            delete this;
            return( FALSE );
        }
        else
        {
            bFoundType = FALSE;
        }
    }

    LPTSTR lpszEn = _tcsstr( lpszDesc, TEXT( " (" ) );

    if( lpszEn == NULL )
    {
        m_wNameLen = lstrlen( lpszDesc );
    }
    else
    {
        m_wNameLen = lpszEn-lpszDesc;

        if( !bFoundType )
        {
            FontDesc_t szEn;

            lstrcpyn( szEn, lpszEn, ARRAYSIZE( szEn ) );

            CharUpper( szEn );

            LPTSTR pTT = _tcsstr( szEn, szTRUETYPE );

            if( !pTT )
            {
                pTT = _tcsstr( szEn, szTRUETYPEALT );
            }

            if( pTT )
            {
                vSetTrueType( FALSE );
            }
            else if( _tcsstr( szName, c_szTYPE1 ) )
            {
                vSetType1( );

                if( lpCompanionFile != NULL )
                    bSetPFB( lpCompanionFile );
            }
            else if( _tcsstr( szName, c_szTYPE1ALT ) )
            {
                vSetType1( );

                if( lpCompanionFile != NULL )
                    bSetPFB( lpCompanionFile );
            }
            else
            {
                vSetDeviceType( );
            }
        }
    }

    lstrcpy( m_szFontLHS, lpszDesc );

    //
    //  It is assumed that this font is already installed.
    //  Set the flag to assume it.
    //

    m_bAFR = TRUE;

    //
    //  Set this to be the main family font. This will be reset as necessary.
    //

    m_wFamIdx = IDX_NULL;

    vSetFamilyFont( );

    //
    // Invalidate the font object's cached file attributes.
    // They will be refreshed next time dwGetFileAttributes() is called for
    // this object.
    //
    InvalidateFileAttributes();

    return( TRUE );
}
#endif    // !CHECKFILENAMEFIRST


//
// Retrieve the font object's cached file attributes.
// If invalid, refresh attribute value from the file system.
//
DWORD CFontClass::dwGetFileAttributes(void)
{
    if (!m_bAttributesValid)
    {
        //
        // Cached value is invalid.
        // Refresh from file system.
        //
        TCHAR szPath[MAX_PATH] = { TEXT('\0') };

        //
        // Get full path to file.
        //
        if (!bGetFileToDel(szPath))   // Gets path if local font file.
            vGetDirFN(szPath);        // Gets path if remote font file.

        if (TEXT('\0') != szPath[0])
        {
            DWORD dwAttr = GetFileAttributes(szPath);
            if ((DWORD)~0 != dwAttr)
                m_dwFileAttributes = dwAttr;
        }
        m_bAttributesValid;            
    }
    return m_dwFileAttributes;
}


#ifdef WINNT

///////////////////////////////////////////////////////////////////////////////
// FUNCTION: CFontClass::GetLogFontInfo
//
//  DESCRIP: Retrieves a list of LOGFONT structures for a font resource.
//
//     ARGS: pszPath
//              Pointer to the font file path string.
//
//           ppLogFontInfo
//              Address of a pointer to an array of LOGFONT structures.
//              The function writes the address of the LOGFONT array
//              to this location.
//
//  RETURNS: Number of LOGFONT structures in returned array.
//
//    NOTES: If *ppLogFontInfo is non-NULL on return,
//           caller must delete array of LOGFONT structures using
//           LocalFree() when finished with it.
//
///////////////////////////////////////////////////////////////////////////////
DWORD CFontClass::GetLogFontInfo(LPTSTR pszPath, LOGFONT **ppLogFontInfo)
{
    DWORD dwNumFonts = 0;
    DWORD dwBufSize = 0;

    ASSERT(NULL != pszPath);
    ASSERT(NULL != ppLogFontInfo);

    dwBufSize = sizeof(dwNumFonts);
    //
    // Get the number of fonts in the font resource.
    //
    if ( NULL != pszPath &&
         NULL != ppLogFontInfo &&
         GetFontResourceInfoW(pszPath,
                              &dwBufSize,
                              &dwNumFonts,
                              GFRI_NUMFONTS) )
    {
        *ppLogFontInfo = (LPLOGFONT)LocalAlloc(LPTR, sizeof(LOGFONT) * dwNumFonts);

        if ( NULL != *ppLogFontInfo )
        {
            dwBufSize = sizeof(LOGFONT) * dwNumFonts;
            //
            // Now get the array of LOGFONT structures.
            //
            if (!GetFontResourceInfoW(pszPath,
                                      &dwBufSize,
                                      *ppLogFontInfo,
                                      GFRI_LOGFONTS))
            {
                //
                // GetFontResourceInfo failed.
                // Clean up and adjust return value to indicate failure.
                //
                LocalFree(*ppLogFontInfo);
                *ppLogFontInfo = NULL;
                dwNumFonts     = 0;
            }
        }
    }
    return dwNumFonts;
}


///////////////////////////////////////////////////////////////////////////////
// FUNCTION: CFontClass::GetType1Info
//
//  DESCRIP: Retrieves the family name, style and weight metrics from a Type 1
//           PFM (printer font metrics) file.  Maps a view of the file and
//           reads the required information.
//           Offsets into the PFM file are obtained from the gdi file winfont.h.
//
//           The macros READ_WORD( ) and READ_DWORD( ) handle byte-ordering
//           differences between the Type1 file and memory.
//
//     ARGS: pszPath
//              Pointer to the font file path string.
//
//           pszFamilyBuf
//              Address of destination buffer for family name string.
//              Can be NULL.
//
//           nBufChars
//              Number of characters in family name buffer.
//              Ignored if pszFamilyBuf is NULL.
//
//           pdwStyle
//              Address of DWORD where style value is written.
//              Style will be FDI_S_ITALIC or FDI_S_REGULAR.
//              Can be NULL.
//
//           pwWeight
//              Address of WORD where weight value is written.
//              Can be NULL.
//
//  RETURNS: SUCCESS
//           or Win32 Error code.
//
///////////////////////////////////////////////////////////////////////////////
DWORD CFontClass::GetType1Info(LPCTSTR pszPath,
                               LPTSTR pszFamilyBuf,
                               UINT nBufChars,
                               LPDWORD pdwStyle,
                               LPWORD pwWeight)
{
    HANDLE hFile = INVALID_HANDLE_VALUE;
    DWORD dwResult = ERROR_SUCCESS;

    ASSERT(NULL != pszPath);

    if (NULL != pszPath &&
       (hFile = CreateFile(pszPath,
                           GENERIC_READ,
                           0,
                           NULL,
                           OPEN_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL)) != INVALID_HANDLE_VALUE )
    {
        HANDLE hFileMapping = INVALID_HANDLE_VALUE;

        if ((hFileMapping = CreateFileMapping(hFile,
                                              NULL,
                                              PAGE_READONLY,
                                              0,
                                              0,
                                              NULL)) != NULL)
        {
            LPCSTR pbFile = NULL;

            if ((pbFile = (LPCSTR)MapViewOfFile(hFileMapping,
                                                FILE_MAP_READ,
                                                0,
                                                0,
                                                0)) != NULL)
            {
                //
                // Get font style.
                //
                if ( NULL != pdwStyle )
                    *pdwStyle = READ_DWORD(&pbFile[OFF_Italic]) ? FDI_S_ITALIC : FDI_S_REGULAR;

                //
                // Get font weight.
                //
                if ( NULL != pwWeight )
                    *pwWeight = READ_WORD(&pbFile[OFF_Weight]);

                //
                // Get family (face) name string.
                //
                if ( NULL != pszFamilyBuf )
                {
                    LPCSTR pszFaceName = (LPCSTR)(pbFile + READ_DWORD(&pbFile[OFF_Face]));

                    MultiByteToWideChar(CP_ACP,
                                        0,
                                        pszFaceName,
                                        -1,
                                        pszFamilyBuf,
                                        nBufChars);
                }
                UnmapViewOfFile(pbFile);
            }
            else
                dwResult = GetLastError();

            CloseHandle(hFileMapping);
        }
        else
            dwResult = GetLastError();

        CloseHandle(hFile);
    }
    else
        dwResult = GetLastError();

    return dwResult;
}

#endif // WINNT


/***************************************************************************
 * FUNCTION: bFillIn
 *
 * PURPOSE:  This functions is used to fill in values that may not be
 *           necessary right away. This includes: Panose number and
 *           family name.
 *
 *
 * RETURNS:  TRUE if value is successfully filled in.
 ***************************************************************************/
BOOL CFontClass :: bFillIn( )
{
    FONTDESCINFO   fdi;

    if( !m_bFilledIn )
    {
        //
        //  If this is a TTC file, we don't care about PANOSE numbers and
        //  family names.
        //

        if( bTTC( ) )
        {
            // vSetFamName( szGetDesc( ) );

            m_lpszFamName = m_szFontLHS;

            vSetFamilyFont( );
        }
        else if( bTrueType( ) || bOpenType( ) )
        {
            if( !bGetFQName( fdi.szFile, ARRAYSIZE( fdi.szFile ) ) )
                goto errout1;

            fdi.dwFlags = FDI_ALL;

            //
            //  Set this as a family font. (We set it so it
            //  doesn't disappear on "hide variations". It will get reset as
            //  soon as possible.)
            //

            vSetFamilyFont( );

            if( !bIsTrueType( &fdi ) )
            {
                //
                // Couldn't open font file for Type1 info.
                // One reason is a font shortcut who's link has been broken.
                // All we have for a name is the LHS string from the registry.
                // Remove the decoration and use it.
                //
                lstrcpyn(m_szFamName, m_szFontLHS, ARRAYSIZE(m_szFamName));
                RemoveDecoration(m_szFamName, TRUE);
                goto errout1;
            }

            //
            //  Copy over font info.
            //

            memcpy( m_xPANOSE.m_ajBytes, &fdi.jPanose, PANOSE_LEN );

            lstrcpy( m_szFamName, fdi.szFamily );

            m_wWeight = fdi.wWeight;

            // m_fItalic = fdi.dwStyle & FDI_S_ITALIC;

            m_dwStyle = fdi.dwStyle;

            //
            //  Verify the PANOSE number.
            //

            if( !m_xPANOSE.bVerify( ) )
            {
                m_xPANOSE.vClear( );

                // m_fHavePANOSE = FALSE;
            }
            else
                // m_fHavePANOSE = TRUE;
                m_jFamily = m_xPANOSE.jFamily( );
        }
#ifdef WINNT
        else if ( bType1() )
        {
            if( ! bGetFQName( fdi.szFile, ARRAYSIZE( fdi.szFile ) ) )
                goto errout1;

            //
            // Make sure we're dealing with a PFM file.
            // GetType1Info only knows how to read a PFM.
            //
            LPTSTR pszDot = StrRChr(fdi.szFile, NULL, TEXT('.'));

            if (NULL != pszDot && lstrcmpi(pszDot+1, TEXT("PFM")) == 0)
            {
                if (ERROR_SUCCESS != GetType1Info(fdi.szFile,
                                                  m_szFamName,
                                                  ARRAYSIZE(m_szFamName),
                                                  &m_dwStyle,
                                                  &m_wWeight))
                {
                    //
                    // Couldn't open font file for Type1 info.
                    // One reason is a font shortcut who's link has been broken.
                    // All we have for a name is the LHS string from the registry.
                    // Remove the decoration and use it.
                    //
                    lstrcpyn(m_szFamName, m_szFontLHS, ARRAYSIZE(m_szFamName));
                    RemoveDecoration(m_szFamName, TRUE);
                }
            }
            else
            {
                //
                // If this code is hit, it means that we have installed
                // something other than a PFM file as a Type1 font.  This is
                // an error that must be corrected.
                // Fill in with some safe values so we don't have just garbage.
                // During development, complain about it.
                //
                DEBUGMSG((DM_TRACE1, TEXT("Non-PFM file (%s) installed for Type1 font."),
                                     fdi.szFile));
                ASSERT(0);

                m_szFamName[0] = TEXT('\0');
                m_dwStyle      = 0;
                m_wWeight      = 0;
            }
        }
#else
        //
        // Win95 doesn't support Type 1 fonts (yet).
        //
#endif
        //
        //  FNT files.
        //
        else
        {
            if( ! bGetFQName( fdi.szFile, ARRAYSIZE( fdi.szFile ) ) )
                goto errout1;

            fdi.dwFlags = FDI_ALL;

            vSetFamilyFont( );

            if( bIsNewExe( &fdi ) )
            {
                //
                // Copy over font info.
                //

                lstrcpy( m_szFamName, fdi.szFamily );

                m_wWeight = fdi.wWeight;

                m_dwStyle = fdi.dwStyle;
            }
#ifdef WINNT
            else
            {
                //
                // Probably a 32-bit font resource.
                // Even if there are multiple fonts in resource,
                // just use info from first font.
                //
                LOGFONT *paLogFontInfo = NULL;
                DWORD dwNumLogFonts    = 0;

                dwNumLogFonts = GetLogFontInfo(fdi.szFile, &paLogFontInfo);
                if ( 0 != dwNumLogFonts && NULL != paLogFontInfo)
                {
                    lstrcpy(m_szFamName, (paLogFontInfo + 0)->lfFaceName);
                    m_wWeight = (WORD)((paLogFontInfo + 0)->lfWeight);
                    m_dwStyle = ((paLogFontInfo + 0)->lfItalic ? FDI_S_ITALIC : FDI_S_REGULAR);
                    LocalFree(paLogFontInfo);
                }
                else
                    goto errout1;
            }
#else
            else
                goto errout1;
#endif

        }

        m_bFilledIn = TRUE;
    }

errout1:
    return m_bFilledIn;
}


/***************************************************************************
 * FUNCTION: bGetFQName
 *
 * PURPOSE:  Get the fully qualified FOT filename of the file associated with
 *              the input font record.  This is the FQ version of the filename
 *              found in WIN.INI.
 *
 *           Assumes: lpsz is of size wLen + 1
 *
 * RETURNS:  TRUE if successful
 ***************************************************************************/
BOOL CFontClass :: bGetFQName( LPTSTR lpsz, WORD wLen )
{
    //
    //  Get the font's directory path, and make the fully qualified name from
    //  that.
    //

    PATHNAME  szPath;

    vGetDirFN( szPath );

    return bMakeFQName( lpsz, szPath, wLen );
}


BOOL CFontClass::bGetFileToDel( LPTSTR szFileName )
{
    if( bFOT( ) )
    {
        LPTSTR lpPath = m_lpszFOT;
        LPTSTR lpLastSlash = StrRChr( lpPath, NULL, TEXT( '\\' ) );

        TCHAR szTempFile[ MAX_PATH_LEN ];

        if( !lpLastSlash )
        {
            if( !bMakeFQName( szTempFile, lpPath, MAX_PATH_LEN, TRUE ) )
            {
                return( FALSE );
            }

            lpPath = szTempFile;

            lpLastSlash = StrRChr( lpPath, NULL, TEXT( '\\' ) );

            if( !lpLastSlash )
            {
                //
                //  This should never happen
                //
                return( FALSE );
            }
        }

        LPTSTR lpFileOnly = lpLastSlash + 1;

        //
        //  Try to find the directory in the list, but do not add it
        //

        int iLen = (int)(lpFileOnly - lpPath - 1);

        CFontDir *poDir = s_poDirList->fdFindDir( lpPath, iLen, FALSE );

        if( poDir && poDir->bOnSysDir( ) )
        {
            bGetFOT( szFileName, sizeof( FullPathName_t ) / sizeof( TCHAR ) );
            return( TRUE );
        }
    }
    else if( bOnSysDir( ) )
    {
        vGetDirFN( szFileName );
        return( TRUE );
    }

    return( FALSE );
}

int CFontClass::s_cFonts = 0;
ULONG CFontClass::AddRef(void)
{
    ULONG ulReturn = m_cRef + 1;

    InterlockedIncrement(&m_cRef);

    return ulReturn;
}

ULONG CFontClass::Release(void)
{
    ULONG ulReturn = m_cRef - 1;

    if (InterlockedDecrement(&m_cRef) == 0)
    {
        delete this;
        ulReturn = 0;
    }
    return ulReturn;
}



/*****************************************************************************
    Local functions:
*****************************************************************************/

typedef enum
{
    DC_ERROR,
    DC_YES,
    DC_NO,
} DC_RETURN;

DC_RETURN bDirContains( LPCTSTR szInName,
                        LPCTSTR szDir,
                        BOOL bCheckExist,
                        LPTSTR lpszName,
                        DWORD dwNameLen )
{
    size_t wInNameLen = lstrlen( szInName );

    //
    //  If the path doesn't have a disk or directory specifier in it, start
    //  the resulting name with the system directory (this is ready for
    //  appending).  Otherwise, start with nothing - we'll append the entire
    //  input path.
    //

    lstrcpyn( lpszName, szDir, dwNameLen );

    //
    //  Make sure there's room to append the input path name to the output
    //  name.  If not (shouldn't ever happen), clear the output name and fail.
    //

    if( ( lstrlen( lpszName ) + wInNameLen ) >= dwNameLen )
    {
        return( DC_ERROR );
    }

    // DEBUGMSG( (DM_TRACE1,TEXT( "About to strcat % onto the end of %s" ), szInName, lpszName ) );

    lstrcat( lpszName, szInName );

    if( !bCheckExist )
    {
        return( DC_YES );
    }

#ifdef WINNT

    return( MyOpenFile( lpszName, NULL, OF_EXIST )
                        != (HANDLE) INVALID_HANDLE_VALUE ? DC_YES : DC_NO );

#else

    OFSTRUCT os;

    return( OpenFile( lpszName, &os, OF_EXIST )!=HFILE_ERROR ? DC_YES : DC_NO );

#endif  //  WINNT
}


/***************************************************************************
 * FUNCTION: bMakeFQName
 *
 * PURPOSE:  Build a fully qualified filename based on the input name
 *              and the system directory (obtained from win.ini).  If there's
 *              already a device and/or directory, don't append the sys dir.
 *
 * RETURNS:  TRUE if FQ name will fit in return, error FALSE
 ***************************************************************************/
BOOL PASCAL bMakeFQName( LPTSTR lpszName,
                         LPTSTR szInName,
                         DWORD  dwFQNameLen,
                         BOOL   bSearchPath )
{
    size_t wInNameLen = lstrlen( szInName );

    if( _tcscspn( szInName, TEXT( ":\\" ) ) != wInNameLen )
    {
        //
        //  (Presumably) fully qualified; no need to check anything
        //

        if( wInNameLen >= dwFQNameLen )
        {
            return( FALSE );
        }

        lstrcpy( lpszName, szInName );
        return( TRUE );
    }

    FullPathName_t szDir;

    if( bSearchPath )
    {
        //
        //  Set the current dir to the Fonts folder so it will get searched
        //  first
        //

        if( !GetFontsDirectory( szDir, ARRAYSIZE( szDir ) ) )
        {
            return( FALSE );
        }

        SetCurrentDirectory( szDir );

        //
        //  Check to see if the file exists on the path.
        //

#ifdef WINNT

        TCHAR szPathName[ PATHMAX ];

        if( MyOpenFile( szInName, szPathName, OF_EXIST )
                    != (HANDLE) INVALID_HANDLE_VALUE )
        {
            if( (DWORD) lstrlen( szPathName ) >= dwFQNameLen )
            {
                return( FALSE );
            }

            lstrcpy( lpszName, szPathName );
            return( TRUE );
        }
#else

        OFSTRUCT os;

        if( OpenFile( szInName, &os, OF_EXIST ) != HFILE_ERROR )
        {
            if( (DWORD) lstrlen( os.szPathName ) >= dwFQNameLen )
            {
                return( FALSE );
            }

            lstrcpy( lpszName, os.szPathName );
            return( TRUE );
        }
#endif  // WINNT

        //
        //  If not on the path, we will fall through and just fill in the
        //  Fonts dir
    }
    else
    {
        //
        //  First check in the system directory; always check for existence
        //

        if( !GetSystemDirectory( szDir, ARRAYSIZE( szDir ) ) )
        {
            return( FALSE );
        }

        lpCPBackSlashTerm( szDir );

        switch( bDirContains( szInName, szDir, TRUE, lpszName, dwFQNameLen ) )
        {
        case DC_YES:
            return( TRUE );
            break;
        }

        //
        //  Next check in the fonts directory; only check for existence if we
        //  are really looking for the file to exist
        //

        if( !GetFontsDirectory( szDir, ARRAYSIZE( szDir ) ) )
        {
            return( FALSE );
        }
    }

    lpCPBackSlashTerm( szDir );

    switch( bDirContains( szInName, szDir, FALSE, lpszName, dwFQNameLen ) )
    {
    case DC_ERROR:
        return( FALSE );
        break;
    }

    return( TRUE );
}


