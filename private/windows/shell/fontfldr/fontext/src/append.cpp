///////////////////////////////////////////////////////////////////////////////
//
// append.cpp
//      Explorer Font Folder extension routines
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
// These logic are originally in sdkinstall.
// I rewrote these as used in CP font installation for JAPAN spec. -yutakan
//
// Modifies: now fnAppendSplitFiles takes pszFiles param that contains
//           list of split files, and szDest which should have destination path.
//           - 06/10/1992 yutakan
//
//  Create: 06/09/1992 Yutaka Nakajima [mskk]
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"
#include "globals.h"
#include "cpanel.h"

#include "lstrfns.h"
#include "ui.h"

#define ALLOC( n )               (VOID *)LocalAlloc( LPTR, n )
#define FREE( p )                LocalFree( (HANDLE) p )


#define MAX_BUF         5120


//
// Import from dos.asm
// extern int FAR PASCAL OEMInfDosChDir( LPTSTR szDir );
//

#ifdef WINNT

BOOL AttachComponentFile( HANDLE fhDst, int fhSrc );


#else

BOOL AttachComponentFile( int fhDst, int fhSrc );

#endif  // WINNT


///////////////////////////////////////////////////////////////////////////////
//
// BOOL fnAppendSplitFiles( LPTSTR FAR *pszFiles, LPTSTR szDest );
//
// ASSUMES: The component files have already been LZ-copied to their
//          respective destination directories.
//          Explicitly uses LZxxxx functions for reading the component files.
//          Component files are compressed.
//
// ENTRY: LPTSTR FAR * pszFiles  .. has list of source files
//                                 files should be ordered in the order
//                                 in which they should be attached.
//        LPTSTR szDest          .. file name for destination.
//        int   nComp           .. number of component files we're attaching . 
//
// EXIT:  BOOL                  .. TRUE if succeed.
//
///////////////////////////////////////////////////////////////////////////////

BOOL far pascal fnAppendSplitFiles( LPTSTR FAR *pszFiles, LPTSTR szDest, int nComp )
{
    TCHAR    szCompFile[ 80 ];
    int      i;
    LPTSTR   lpDestFileName;
    LPTSTR   lpTemp;

#ifdef WINNT
    HANDLE   fhDst;
    TCHAR    lppath[ MAX_PATH ];
#else
    int      fhDst;
#endif  //  WINNT

    int      fhComp;
    OFSTRUCT ofstruct;

    if( lpDestFileName = StrRChr( szDest, NULL, TEXT( '\\' ) ) )
    {
        *lpDestFileName = TEXT( '\0' );
        lpDestFileName++;
    }
    else
        return FALSE;

    //
    // Change to destination directory.
    //

    // if (OEMInfDosChDir(szDest) != 0)

    if( !SetCurrentDirectory( szDest ) )
        return FALSE;

    //
    //  Create destination file for writing. If it already exists,
    //  it is truncated to zero length.
    //

    fhDst = FCREATE ( lpDestFileName );

    if ( !fhDst )
        return FALSE;

    //
    //  When appending files, Cursor should be in HourGlass.
    //   1992.12.22 by yutakas
    //

    {

        WaitCursor wait;

        //
        //  Append all the component files one-by one to the destination
        //  file
        //

        for( i = 0; i < nComp; i++)
        {
            if( lstrlen( pszFiles[ i ] ) < 2 ) 
                goto BadParam;

            //
            // Assume pszFiles has list of string already formated as x:name,y:name...
            //

            if( lpTemp = StrChr( pszFiles[ i ], TEXT( ':' ) ) )
                lstrcpy( szCompFile, lpTemp+1 );
            else
                lstrcpy( szCompFile, pszFiles[ i ] );

            fhComp = LZOpenFile(szCompFile, &ofstruct, OF_READ | OF_SHARE_DENY_WRITE);

            if ( fhComp < 0 )
            {
BadParam:
                FCLOSE ( fhDst );
                return FALSE;
            }

            if( !AttachComponentFile( fhDst, fhComp ) )
            {
                FCLOSE ( fhDst );
                LZClose(fhComp);
                return FALSE;
            }

            //
            //  delete component file
            //
            LZClose(fhComp);
            DeleteFile(szCompFile);
        }   

        //
        //  close destination file
        //

        FCLOSE ( fhDst );
    }

    return TRUE;
}


///////////////////////////////////////////////////////////////////////////////
//
// AttachComponentFile( fhDst, fhSrc )
//
// Appends a single file to another file using an intermediate buffer.
// Explicitly uses LZxxxx functions for reading the component file.
// Component files are compressed.
//
// ENTRY: int fhDst - handle of open destination file.
//        int fhSrc - handle of open component file.
//
// EXIT: BOOL
//     - TRUE on success, FALSE on failure  [ yutakan ] - 06/10/1992
//     - [ lalithar ] - 05/21/91
//
// NOTE: current buffer size set to 5K. The limit may be increased to a
//       larger value if needed.
//
///////////////////////////////////////////////////////////////////////////////

#ifdef WINNT

BOOL AttachComponentFile( HANDLE fhDst, int fhSrc )

#else

BOOL AttachComponentFile( int fhDst, int fhSrc )

#endif  // WINNT
{

#define BUF_STEP         1024

    char *pBuf, *pTmp;
    long   dwLen;
    int    wBufSize = MAX_BUF;

    //
    // determine the length of the component file
    //

    dwLen = LZSeek( fhSrc, 0L, SEEK_END );

    LZSeek(fhSrc, 0L, SEEK_SET);

    //
    // allocate a buffer of a reasonable size
    //

    pBuf = (LPSTR) ALLOC( wBufSize );

    for ( ; ( pBuf == NULL ) && wBufSize; wBufSize -= BUF_STEP )
    {
        pBuf = (LPSTR) ALLOC( wBufSize );
    }

    if( !pBuf )
        return FALSE;

    //
    //  Read MAX_BUF bytes of the component file into the buffer
    //  and write out the buffer to the destination file. Repeat this
    //  until the entire source file has been copied to the dest. file.
    //

    for( pTmp = pBuf; dwLen > wBufSize ; dwLen -= wBufSize )
    {
        LZRead( fhSrc, pTmp, wBufSize );
        FWRITEBYTES( fhDst, pTmp, wBufSize );
    }

    LZRead( fhSrc, pTmp, dwLen );
    FWRITEBYTES( fhDst, pTmp, dwLen );

    FREE ( pBuf );

   return TRUE;
}

