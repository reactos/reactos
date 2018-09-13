///////////////////////////////////////////////////////////////////////////////
//
// oeminf.cpp
//      Explorer Font Folder extension routines.
//    Functions for manipulating OEMxxxxx.INF files.  This module is
//    shared by Windows Setup and Control Panel.  The constant
//    WINSETUP is defined when compiling for Windows Setup.
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
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

#include "priv.h"
#include "globals.h"

#include "dbutl.h"

#define USE_OEMINF_DEFS
#include "oeminf.h"

#define ARRAYSIZE(a)    (sizeof(a)/sizeof(a[0]))

/* BOOL RunningFromNet( void );
 *
 * Checks to see if the user is running on a network Windows installation.
 *
 * ENTRY: void
 *
 * EXIT: BOOL - TRUE if the user is running on a network Windows
 *              installation.  FALSE if not or if one of the
 *              Get...Directory() calls fails.
 *
 */
BOOL FAR PASCAL RunningFromNet( void )
{
    TCHAR  pszWindowsDir[ MAX_NET_PATH ], pszSystemDir[ MAX_NET_PATH ];
    LPTSTR pszWindowsTemp, pszSystemTemp;


    //
    //  Check the results from GetSystemWindowsDirectory() and GetSystemDirectory().
    //  If the System directory is a direct subdirectory of the Windows
    //  directory, this is not a network installation.  Otherwise it is a
    //  network installation.
    //

    if( GetSystemWindowsDirectory( pszWindowsDir, ARRAYSIZE( pszWindowsDir ) ) == 0 )
        return( FALSE );

    if( GetSystemDirectory( pszSystemDir, ARRAYSIZE( pszSystemDir ) ) == 0 )
        return( FALSE );

    pszWindowsTemp = pszWindowsDir;
    pszSystemTemp  = pszSystemDir;

    CharUpper( pszWindowsTemp );
    CharUpper( pszSystemTemp );

    while( ( *pszWindowsTemp != TEXT( '\0' ) )
           && ( *pszWindowsTemp++ == *pszSystemTemp++ ) )
       ;

    //
    //  Did the path specifications match?
    //

    if( *pszWindowsTemp == TEXT( '\0' ) )
        return( FALSE );
    else
        return( TRUE );
}


/* HANDLE ReadFileIntoBuffer( int doshSource );
 *
 * Reads up to first (64K - 1) bytes of an input file into a buffer.
 *
 * ENTRY: doshSource - DOS file handle of file open for reading
 *
 * EXIT: HANDLE - Global handle to the file buffer filled from the input
 *                file.  NULL if an error occurs.
 *
 */

HANDLE FAR PASCAL ReadFileIntoBuffer( int doshSource )
{
    LONG lLength;
    HANDLE hBuffer;
    LPTSTR lpszBuffer, lpszTemp;
    int nBytesToRead;


    //
    //  How long is the file?
    //

    if( ( lLength = _llseek( doshSource, 0L, 2 ) ) < 0L )
    {

       //
       //  Return NULL on error.
       //

       return( NULL );
    }


    //
    //  Return to the beginning of the file.
    //

    if( _llseek( doshSource, 0L, 0 ) != 0L )
        return( NULL );

    //
    //  Don't overrun the .inf buffer bound.
    //

    if( lLength > MAX_INF_COMP_LEN )
        lLength = MAX_INF_COMP_LEN;

    //
    //  Allocate storage for the file.
    //

    if( ( hBuffer = GlobalAlloc( GHND, (DWORD) lLength ) ) == NULL )
        return( NULL );

    //
    //  Lock the buffer in place.
    //

    if( ( lpszTemp = lpszBuffer = (LPTSTR) GlobalLock( hBuffer ) ) == NULL )
        return( NULL );

    //
    //  Fill the buffer from the file.
    //

    while( lLength > 0 )
    {
        nBytesToRead = (int)min( lLength, MAX_INF_READ_SIZE );

        if( _lread( doshSource, lpszTemp, nBytesToRead ) != (WORD)nBytesToRead )
        {
            GlobalUnlock( hBuffer );
            GlobalFree( hBuffer );
            return( NULL );
        }

        lLength -= (LONG)nBytesToRead;
        lpszTemp += (LONG)nBytesToRead;
    }

    //
    //  Unlock the buffer.
    //

    GlobalUnlock( hBuffer );

    //
    //  File read in successfully.
    //

    return( hBuffer );
}


/* int FilesMatch( int dosh1, int dosh2, unsigned int uLength );
 *
 * Compares two files.
 *
 * ENTRY: dosh1   - DOS file handle of first file open for reading
 *        dosh2   - DOS file handle of second file open for reading
 *        uLength - number of bytes to compare
 *
 * EXIT: int - TRUE if first (64K - 1) bytes of the files match exactly.
 *             FALSE if not.  (-1) if an error occurs.
 *
 * The buffers need not be null-terminated.  TEXT( '\0' )s are treated as just
 * another byte to compare.
 *
 */

int FAR PASCAL FilesMatch( HANDLE h1, HANDLE h2, unsigned uLength )
{
    int    nReturnCode = -1;
    LPTSTR lpsz1, lpsz2;

    if( ( lpsz1 = (LPTSTR) GlobalLock( h1 ) ) == NULL )
    {
        GlobalUnlock( h1 );
        return( nReturnCode );
    }

    if( ( lpsz2 = (LPTSTR) GlobalLock( h2 ) ) != NULL )
    {
        //
        //  See if the files match.
        //

        nReturnCode = !memcmp( lpsz1, lpsz2, uLength );
    }

    GlobalUnlock( h1 );
    GlobalUnlock( h2 );

    return( nReturnCode );
}


/* LPTSTR TruncateFileName( LPTSTR pszPathSpec );
 *
 * Finds the file name portion of a path specification.
 *
 * ENTRY: lpszPathSpec - path specification
 *
 * EXIT: LPTSTR - Pointer to file name portion of string.
 *
 */

LPTSTR FAR PASCAL TruncateFileName( LPTSTR lpszPathSpec )
{
    LPTSTR lpszStart = lpszPathSpec;

    //
    //  Find end of string.
    //

    while( *lpszPathSpec != TEXT( '\0' ) )
        lpszPathSpec = CharNext( lpszPathSpec );

    //
    //  Seek back until we find a path separator or the begining of the string.
    //

    while( !IS_PATH_SEPARATOR( *lpszPathSpec ) && lpszPathSpec != lpszStart )
        lpszPathSpec = CharPrev( lpszStart, lpszPathSpec );

    if( lpszPathSpec != lpszStart )
        lpszPathSpec = CharNext( lpszPathSpec );

    //
    //  Return pointer to file name.
    //

    return( lpszPathSpec );
}


/* int OpenFileAndGetLength( LPTSTR lpszSourceFile, PLONG plFileLength );
 *
 * Opens a file into a global buffer.  Returns a handle to the buffer and the
 * actual length of the file.
 *
 * ENTRY: lpszSourceFile - source file name
 *        plFileLength   - pointer to LONG to be filled with length of source
 *                         file
 *
 * EXIT: int           - Open DOS file handle if successful.  (-1) if
 *                       unsuccessful.
 *       *plFileLength - Filled in with length of source file if successful.
 *                       Undefined if unsuccessful.
 */

int FAR PASCAL OpenFileAndGetLength( LPTSTR lpszSourceFile,
                                     LPLONG plFileLength )
{
    int      doshSource;
    OFSTRUCT of;


#ifdef UNICODE

////////////////////////////////////////////////////////////////////
//
// BUGBUG [stevecat]  - Why are we munging INF files?  For now
//          just convert the filename to ASCII and use the current
//          fileio apis to munge thru the INF files.
//
////////////////////////////////////////////////////////////////////

    char    szFile[ PATHMAX ];

    WideCharToMultiByte( CP_ACP, 0, lpszSourceFile, -1, szFile,
                         PATHMAX, NULL, NULL );

    doshSource = OpenFile( szFile, &of, OF_READ );

#else

    doshSource = OpenFile( lpszSourceFile, &of, OF_READ );

#endif  //  UNICODE

    if( doshSource == -1 )
        return doshSource;

    //
    //  Keep track of the length of the new file.
    //

    if( ( *plFileLength = _llseek( doshSource, 0L, 2 ) ) < 0L )
    {
        _lclose( doshSource );
        return(-1);
    }

    return( doshSource );
}


#define INF_YES (1)
#define INF_NO  (0)
#define INF_ERR (-1)


/* int IsNewFile( LPTSTR lpszSourceFile, LPTSTR lpszDestDir );
 *
 * Checks to see if a given file already exists as a file matching a given
 * file specification.
 *
 * ENTRY: pszSourceFile - path name of the new file
 *        pszSearchSpec - target directory and file specification (may
 *                        include wildcards) to use to search for duplicates
 *                        (e.g., "c:\win\system\*.inf")
 *
 * EXIT: int - TRUE if the new file doesn't already exist as a file matching
 *             the given file specification.  FALSE if it does.  (-1) if an
 *             error occurs.
 *
 */

int FAR PASCAL IsNewFile( LPTSTR lpszSourceFile, LPTSTR lpszSearchSpec )
{
    int    nReturnCode = INF_ERR;
    int    nTargetLen, nMatchRet;
    HANDLE hFind;
    WIN32_FIND_DATA   sFind;
    LPTSTR lpszReplace;
    TCHAR  pszTargetFileName[ MAX_NET_PATH + FILEMAX ];
    int    doshSource, doshTarget;
    HANDLE hSourceBuf, hTargetBuf;
    LONG   lSourceLength, lTargetLength;


    //
    //  How much storage do we need for the destination file name?
    //

    lpszReplace = TruncateFileName( lpszSearchSpec );

    //
    // [stevecat] The following statement is evil and parochial. It
    //            will not work correctly in a UNICODE environment.
    //
    //       nTargetLen = lpszReplace - lpszSearchSpec + FILEMAX;
    //
    //  Replace with a better way to calculate string length.
    //

    nTargetLen = (LONG)(lpszReplace - lpszSearchSpec) / sizeof( TCHAR ) + FILEMAX;

    //
    //  Don't overflow the buffer.
    //

    if( nTargetLen > ARRAYSIZE( pszTargetFileName ) )
        return( INF_ERR );

    //
    //  Keep track of the start of the file name in the destination path
    //  specification.
    //

    lstrcpy( pszTargetFileName, lpszSearchSpec );

    lpszReplace = (LPTSTR)pszTargetFileName + ( lpszReplace - lpszSearchSpec );

    //
    //  Are there any files to process?
    //

    hFind = FindFirstFile( lpszSearchSpec, &sFind );


    //
    //  Only open the source file if there are any existing files to compare
    //  against.
    //

    if( hFind == INVALID_HANDLE_VALUE )
        return( INF_YES );

    if( ( doshSource = OpenFileAndGetLength( lpszSourceFile, &lSourceLength ) ) == NULL )
    {
        FindClose( hFind );
        return( INF_ERR );
    }

    hSourceBuf = ReadFileIntoBuffer( doshSource );

    _lclose( doshSource );

    if( hSourceBuf == NULL )
    {
        FindClose( hFind );
        return( INF_ERR );
    }

    //
    //  Check all the matching files.
    //

    while( hFind != INVALID_HANDLE_VALUE )
    {
        //
        //  Replace the wildcard file specification with the file name of the
        //  target file.
        //  lstrcpy( lpszReplace, fcbSearch.szName );
        //

        lstrcpy( lpszReplace, sFind.cAlternateFileName );

        //
        //  Open the target file.
        //

        if( ( doshTarget = OpenFileAndGetLength( pszTargetFileName,
                                                 &lTargetLength ) ) == NULL )
           goto IsNewFileExit;

        //
        //  Is the target file the same size as the new file?
        //

        if( lTargetLength == lSourceLength )
        {
           //
           //  Yes.  Read in the target file.
           //

           hTargetBuf = ReadFileIntoBuffer( doshTarget );

           _lclose( doshTarget );

           if( hTargetBuf == NULL )
                goto IsNewFileExit;

           //
           //  ReadFileIntoBuffer( ) has already checked to make sure the files
           //  aren't longer than (64K - 1) bytes long.
           //  Assert: lSourceLength fits in an unsigned int.
           //

           nMatchRet = FilesMatch( hSourceBuf, hTargetBuf,
                                  (unsigned int)lSourceLength );

           GlobalFree( hTargetBuf );

           if( nMatchRet == -1 )
                goto IsNewFileExit;
           else if( nMatchRet == TRUE )
           {
                lstrcpy( lpszSourceFile, pszTargetFileName );
                nReturnCode = INF_NO;
                goto IsNewFileExit;
           }
        }

        //
        //  Look for the next matching file.
        //  bFound = OEMInfDosFindNext( &fcbSearch );
        //

        if( !FindNextFile( hFind, &sFind ) )
        {
            FindClose( hFind );
            hFind = INVALID_HANDLE_VALUE;
        }
    }

    nReturnCode = INF_YES;

IsNewFileExit:

    if( hFind != INVALID_HANDLE_VALUE )
        FindClose( hFind );


    GlobalFree( hSourceBuf );

    return( nReturnCode );
}


/* PTSTR MakeUniqueFilename( PTSTR pszDirName, PTSTR pszPrefix,
 *                                 PTSTR pszExtension );
 *
 * Creates a unique filename in a directory given a prefix for the base of
 * the filename and an extension.  The prefix must be three characters long.
 * The extension may be zero to three characters long.  The extension should
 * not include a period.  E.g., prefix "oem" and extension "inf".
 *
 * pszDirName's buffer must have space for up to 13 extra characters to be
 * appended (a slash plus 8.3).
 *
 *
 * ENTRY: pszDirName   - buffer holding target directory, unique filename
 *                       will be appended
 *        pszPrefix    - three-character base filename prefix to use
 *        pszExtension - filename extension to use
 *
 * EXIT: PTSTR - Pointer to modified path specification if successful.  NULL
 *              if unsuccessful.
 *
 */

LPTSTR FAR PASCAL MakeUniqueFilename( LPTSTR lpszDirName,
                                      LPTSTR lpszPrefix,
                                      LPTSTR lpszExtension )
{
    TCHAR   szOriginalDir[ MAX_NET_PATH ];
    TCHAR   szUniqueName[ FILEMAX ];
    ULONG   ulUnique = 0UL;
    LPTSTR  lpszTemp;
    BOOL    bFoundUniqueName = FALSE;


    DEBUGMSG( (DM_TRACE1, TEXT( "MakeUniqueFilename() " ) ) );

    //
    //  Check form of arguments.
    //

    if( lstrlen( lpszPrefix ) != 3 || lstrlen( lpszExtension ) > 3 )
        return( NULL );

    //
    //  Save current directory.
    //  if( OEMInfDosCwd( szOriginalDir ) != 0 )
    //

    if( !GetCurrentDirectory( ARRAYSIZE( szOriginalDir ), szOriginalDir ) )
        return( NULL );

    //
    //  Move to target directory.
    //  if( OEMInfDosChDir( lpszDirName ) != 0 )
    //

    if( !SetCurrentDirectory( lpszDirName ) )
        return( NULL );

    //
    //  Make file specification.
    //

    lstrcpy( szUniqueName, lpszPrefix );
    lpszTemp = szUniqueName + 3;

    //
    //  Try to create a unique filename.
    //

    while( !bFoundUniqueName && ulUnique <= MAX_5_DEC_DIGITS )
    {
        //
        //  Hack together next filename to try.
        //

        wsprintf( lpszTemp, TEXT( "%lu.%s" ), ulUnique, lpszExtension );

        //
        //  Is this name being used?
        //  if( OEMInfDosFindFirst( & fcbSearch, szUniqueName, ATTR_ALL_FD ) == 0 )
        //

        if( GetFileAttributes( szUniqueName ) == 0xffffffff )
        {
            //
            //  Nope.
            //

            bFoundUniqueName = TRUE;
            break;
        }
        else
            //
            // Yes.  Keep trying.
            //

            ulUnique++;
    }

    //
    //  Have all 100,000 possibilties been exhausted?
    //

    if( !bFoundUniqueName )
        return( FALSE );

    //
    //  Add new unique filename on to end of path specification buffer.
    //

    //
    //  Check for ending slash.
    //

    lpszTemp = lpszDirName + lstrlen( lpszDirName );

    if( !IS_SLASH( *(lpszTemp - 1 ) ) && *(lpszTemp - 1 ) != TEXT( ':' ) )
       *lpszTemp++ = TEXT( '\\' );

    //
    //  Append unique filename.
    //

    lstrcpy( lpszTemp, szUniqueName );

    //
    //  Return pointer to modified buffer.
    //

    DEBUGMSG( (DM_TRACE1,TEXT( "MakeUniqueFilename returning: %s %s" ),
              lpszDirName, lpszTemp) );

    return( lpszDirName );
}


/* BOOL CopyNewOEMInfFile( PTSTR pszOEMInfPath );
 *
 * Copies a new OEMSetup.inf file into the user's Windows (network) or System
 * (non-network) directory.  Gives the new .inf file a unique name of the
 * form 'OEMxxxxx.INF'.  Only copies it in if it is really a new .inf file.
 *
 * ENTRY: pszOEMInfPath - path name of the new .inf file to be copied
 *
 * EXIT: BOOL - TRUE if the new .inf file was copied successfully or had
 *              already been added.  0 if the copy failed.
 *
 */

BOOL FAR PASCAL CopyNewOEMInfFile( LPTSTR lpszOEMInfPath )
{
    BOOL   bRunningFromNet;
    TCHAR  pszDest[ MAX_NET_PATH + FILEMAX ];
    LPTSTR pszTemp;

    //
    //  Where should we put the new .inf file?
    //

    if( bRunningFromNet = RunningFromNet( ) )
    {
        //
        //  Put new .inf file in Windows directory.
        //

        if( GetWindowsDirectory( pszDest, MAX_NET_PATH ) == 0 )
            return( FALSE );
    }
    else
    {
        //
        //  Put new .inf file in System directory.
        //

        if( GetSystemDirectory( pszDest, MAX_NET_PATH ) == 0 )
            return( FALSE );
    }

    //
    //  Make file specification for IsNewFile( ).
    //

    pszTemp = pszDest + lstrlen( pszDest );

    //
    //  N.b., we depend on pszDest not ending in a slash here.
    //

    lstrcpy( pszTemp, OEM_STAR_DOT_INF );

    //
    //  Has this .inf file already been copied to the user's Windows or System
    //  directory?
    //

    switch( IsNewFile( lpszOEMInfPath, pszDest ) )
    {
    case INF_ERR:
        return( FALSE );

    case INF_YES:
        //
        //  Trim TEXT( "\*.inf" ) off end of pszDest.
        //

        *pszTemp = TEXT( '\0' );

        // Create a unique name for the new .inf file. We could use
        // SHFileOperation() to create a unique file, but we don't want to
        // copy the file more than once -- we want IsNewFile() to only check
        // for OEMxxxx.INF files.
        //

        if( MakeUniqueFilename( pszDest, INF_PREFIX, INF_EXTENSION ) == NULL )
            return( FALSE );
//
// #if 0   [stevecat] 6/15/95 - try to use shell op part vs. alternate
#if 1
        //
        //  Copy .inf file.
        //

        SHFILEOPSTRUCT fop;

        memset( &fop, 0, sizeof( fop ) );

        //
        // Append NUL onto both path strings.
        // SHFileOperation requires a double-nul terminator.
        //
        *(lpszOEMInfPath + lstrlen(lpszOEMInfPath) + 1) = TEXT('\0');
        *(pszDest + lstrlen(pszDest) + 1) = TEXT('\0');

        fop.wFunc  = FO_COPY;
        fop.pFrom  = lpszOEMInfPath;
        fop.pTo    = pszDest;
        fop.fFlags = FOF_SILENT | FOF_NOCONFIRMATION;

        SHFileOperation( &fop );
#else
        int  doshSource, doshDest;
        BOOL bCopied;

        if( ( doshSource = FOPEN( lpszOEMInfPath ) ) == -1 )
            return( FALSE );

        if( ( doshDest = FCREATE( pszDest ) ) == -1 )
        {
            FCLOSE( doshSource );
            return( FALSE );
        }

        //
        //  All LZERROR_ return codes are < 0.
        //

        bCopied = ( LZCopy( doshSource, doshDest ) >= 0L );

        //
        //  Close the files.
        //

        FCLOSE( doshSource );
        FCLOSE( doshDest );

        if( !bCopied )
            return( FALSE );
#endif
        //
        //  Copy the new file name back, so the calling function can use it
        //

        lstrcpy( lpszOEMInfPath, pszDest );

    default:
        break;
    }

    //
    //  New .inf file already existed or copied successfully.
    //

    return( TRUE );
}

