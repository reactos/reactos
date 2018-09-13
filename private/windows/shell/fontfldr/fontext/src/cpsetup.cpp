///////////////////////////////////////////////////////////////////////////////
//
// cpsetup.cpp
//      Explorer Font Folder extension routines
//      This file holds all the code for reading setup.inf
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//      2/24/96 [BrianAu]
//          Replaced INF parsing code with Win32 Setup API.
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
#include "cpanel.h"   // Needs "extern" declaration for exports.

#include "setupapi.h"

#define USE_WIN32_SETUP_API 1  // Replace INF parsing code with Win32 Profile API

#ifdef USE_WIN32_SETUP_API

//
// I have re-worked this code so that the original INF parsing code
// has been replaced with calls to the Win32 Setup API.  This not
// only greatly simplifies the code but also shields the font folder
// from any ANSI/DBCS/UNICODE parsing issues as well as compressed file
// issues.
// I have disabled the old code with the USE_WIN32_SETUP_API macro.
//
// You'll notice that the Setup API extracts fields from the INF section
// and we paste them back together to form a key=value string.  This is
// because the calling code previously used GetPrivateProfileSection() which
// returned information as key=value<nul>key=value<nul>key=value<nul><nul>.
// The function ReadSetupInfSection assembles the required information into
// the same format so that the calling code remains unchanged.
//
// [BrianAu 2/24/96]

//
// ReadSetupInfFieldKey
//
// Reads the key name from an Inf key=value pair.
//
// pContext - Pointer to Setup Inf Line context.
// pszBuf   - Pointer to destination buffer.
// cchBuf   - Size of destination buffer in characters.
//
// If destination buffer is not large enough for the name, function returns
// the number of characters required.  Otherwise, the number of characters
// read is returned.
//
DWORD ReadSetupInfFieldKey(INFCONTEXT *pContext, LPTSTR pszBuf, DWORD cchBuf)
{
    DWORD cchRequired = 0;

    if (!SetupGetStringField(pContext,
                             0,                  // Get key name
                             pszBuf,
                             cchBuf,
                             &cchRequired))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            cchRequired = 0;
    }
    return cchRequired;
}


//
// ReadSetupInfFieldText
//
// Reads the value text from an Inf key=value pair.
//
// pContext - Pointer to Setup Inf Line context.
// pszBuf   - Pointer to destination buffer.
// cchBuf   - Size of destination buffer in characters.
//
// If destination buffer is not large enough for text, function returns
// the number of characters required.  Otherwise, the number of characters
// read is returned.
//
DWORD ReadSetupInfFieldText(INFCONTEXT *pContext, LPTSTR pszBuf, DWORD cchBuf)
{
    DWORD cchRequired = 0;


    if (!SetupGetLineText(pContext,
                          NULL,
                          NULL,
                          NULL,
                          pszBuf,
                          cchBuf,
                          &cchRequired))
    {
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            cchRequired = 0;
    }

    return cchRequired;
}



//
// ReadSetupInfSection
//
// pszInfPath - Name of INF file to read.
// pszSection - Name of INF file section to read.
// ppszItems  - Address of pointer to receive address of
//              buffer containing INF items.  If *ppszItems
//              is non-null, the addressed buffer contains items read from
//              section in INF.  Each item is nul-terminated with a double
//              nul terminating the entire list.  The caller is responsible for
//              freeing this buffer with LocalFree( ).
//
// Returns: Number of characters read from INF section.  Count includes nul
//          separators and double-nul terminator.
//          0 = Section not found or section empty or couldn't allocate buffer.
//              *ppszItems will be NULL.
//
// The information returned through *ppszItems is in the format:
//
//      key=value<nul>key=value<nul>key=value<nul><nul>
//
DWORD ReadSetupInfSection( LPTSTR pszInfPath, LPTSTR pszSection, LPTSTR *ppszItems )
{
    DWORD cchTotalRead = 0;

    //
    // Input pointers must be non-NULL.
    //
    if (NULL != pszInfPath && NULL != pszSection && NULL != ppszItems)
    {
        HANDLE hInf = INVALID_HANDLE_VALUE;

        //
        // Initialize caller's buffer pointer.
        //
        *ppszItems = NULL;

        hInf = SetupOpenInfFile(pszInfPath,         // Path to inf file.
                                NULL,               // Allow any inf type.
                                INF_STYLE_OLDNT,    // Old-style text format.
                                NULL);              // Don't care where error happens.

        if (INVALID_HANDLE_VALUE != hInf)
        {
            INFCONTEXT FirstLineContext;            // Context for first line in sect.
            INFCONTEXT ScanningContext;             // Used while scanning.
            INFCONTEXT *pContext        = NULL;     // The one we're using.
            LPTSTR     pszLines         = NULL;     // Buffer for sections.
            DWORD      cchTotalRequired = 0;        // Bytes reqd for section.

            if (SetupFindFirstLine(hInf,         
                                   pszSection,      // Section name.
                                   NULL,            // No key.  Find first line.
                                   &FirstLineContext))
            {
                //
                // Make a copy of context so we can re-use the original later.
                // Start using the copy.
                //
                CopyMemory(&ScanningContext, &FirstLineContext, sizeof(INFCONTEXT));
                pContext = &ScanningContext;

                //
                // Find how large buffer needs to be to hold section text.
                // The value returned by each of these ReadSetupXXXXX calls 
                // includes a terminating nul character.
                //
                do
                {
                    cchTotalRequired += ReadSetupInfFieldKey(pContext,
                                                             NULL,
                                                             0);
                    cchTotalRequired += ReadSetupInfFieldText(pContext,
                                                              NULL,
                                                              0);
                }
                while(SetupFindNextLine(pContext, pContext));

                cchTotalRequired++;  // For terminating double nul.

                //
                // Allocate the buffer.
                //
                pszLines = (LPTSTR)LocalAlloc(LPTR, cchTotalRequired * sizeof(TCHAR));
                if (NULL != pszLines)
                {
                    LPTSTR pszWrite     = pszLines;
                    DWORD  cchAvailable = cchTotalRequired;
                    DWORD  cchThisPart  = 0;        

                    //
                    // We can use the first line context now.
                    // Doesn't matter if we alter it.
                    //
                    pContext = &FirstLineContext;

                    do
                    {
                        cchThisPart = ReadSetupInfFieldKey(pContext,
                                                           pszWrite,
                                                           cchAvailable);

                        if (cchThisPart <= cchAvailable)
                        {
                            cchAvailable -= cchThisPart;  // Decr avail counter.
                            pszWrite     += cchThisPart;  // Adv write pointer.
                            *(pszWrite - 1) = TEXT('=');  // Replace nul with '='
                            cchTotalRead += cchThisPart;  // Adv total counter.
                        }
                        else
                        {
                            //
                            // Something went wrong and we tried to overflow
                            // buffer.  This shouldn't happen.
                            //
                            cchTotalRead = 0;
                            goto InfReadError;
                        }

                        cchThisPart = ReadSetupInfFieldText(pContext,
                                                            pszWrite,
                                                            cchAvailable);

                        if (cchThisPart <= cchAvailable)
                        {
                            cchAvailable -= cchThisPart;  // Decr avail counter.
                            pszWrite     += cchThisPart;  // Adv write pointer.
                            cchTotalRead += cchThisPart;  // Adv total counter.
                        }
                        else
                        {
                            //
                            // Something went wrong and we tried to overflow
                            // buffer.  This shouldn't happen.
                            //
                            cchTotalRead = 0;
                            goto InfReadError;
                        }
                    }
                    while(SetupFindNextLine(pContext, pContext));

                    if (cchAvailable > 0)
                    {
                        //
                        // SUCCESS! Section read without errors.
                        // Return address of buffer to caller.
                        // By allocating buffer with LPTR, text is already 
                        // double-nul terminated.
                        //
                        *ppszItems = pszLines;   
                    }
                    else
                    {
                        //
                        // Something went wrong and we tried to overflow
                        // buffer.  This shouldn't happen.
                        //
                        cchTotalRead = 0;
                    }
                }
            }

InfReadError:

            SetupCloseInfFile(hInf);
        }
    }
    return cchTotalRead;
}




//
// ReadSetupInfCB
//
// pszSection   - Name of INF section without surrounding [].
// lpfnNextLine - Address of callback function called for each item in the section.
// pData        - Data item contains info stored in dialog listbox.
//
// Returns:  0 = Success.
//          -1 = Item callback failed.
//          INSTALL+14 = No INF section found.
//
WORD ReadSetupInfCB(LPTSTR pszInfPath,
                    LPTSTR pszSection,
                    WORD (*lpfnNextLine)(LPTSTR, LPVOID),
                    LPVOID pData)
{
    LPTSTR lpBuffer  = NULL;
    WORD   wResult   = INSTALL+14;       // This is the "no file" message

    //
    // Read in the section from the INF file.
    //
    ReadSetupInfSection(pszInfPath, pszSection, &lpBuffer);

    if (NULL != lpBuffer)
    {
        //
        // Got a buffer full of section text.
        // Each item is nul-terminated with a double nul
        // terminating the entire set of items.
        // Now iterate over the set, calling the callback function
        // for each item.
        //
        LPTSTR pInfEntry = lpBuffer;
        wResult = 0;

        while(TEXT('\0') != *pInfEntry)
        {
            wResult = (*lpfnNextLine)(pInfEntry, pData);
            if ((-1) == wResult)
                break;

            pInfEntry += lstrlen(pInfEntry) + 1;
        }
        LocalFree(lpBuffer);
    }
    return wResult;
}


#else                                 

//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//                                                                      //
//         OBSOLETE CODE - Replaced with Win32 Setup API abov  e        //
//                                                                      //
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
#include "dbutl.h"
#include "lstrfns.h"

extern TCHAR szSetupInfPath[ PATHMAX ];

#define DEBUGMESSAGES 0

#define READ_BUFSIZE       1024
#define MAX_SCHEMESIZE     180

//
// For Windows NT version, we use UNICODE instead of DBCS and already
// know how to handle AnsiPrev/Next and IsDBCSLeadByte.
//

#ifndef WINNT

#define DBCS
#ifndef DBCS
#define AnsiNext(x) ((x)+1)
#define AnsiPrev(y,x) ((x)-1)
#define IsDBCSLeadByte(x) (FALSE)
#endif
#endif // WINNT


// Function Prototypes

///////////////////////////////////////////////////////////////////////////////
//
// OpenSetupInf() takes the string held in szSetupInfPath as the path
//                to find SETUP.INF and attempts to open it.  The
//                global structure SetupInf is filled.
//  return   -1 indicates failure (see OpenFile doc)
//
///////////////////////////////////////////////////////////////////////////////

#ifdef WINNT

HANDLE FAR PASCAL OpenSetupInf( void )
{
    HANDLE fh;

    DEBUGMSG( (DM_TRACE1, TEXT( "OpenSetupInf: opening %s" ), szSetupInfPath ) );


    if( ( fh = FOPEN( (LPTSTR) szSetupInfPath, NULL ) )
                    == (HANDLE) INVALID_HANDLE_VALUE )
        return( (HANDLE) -1 );
    else
        return( fh );
}

#else

int FAR PASCAL OpenSetupInf( void )
{
    OFSTRUCT os;

    DEBUGMSG( (DM_TRACE1, TEXT( "OpenSetupInf: opening %s" ), szSetupInfPath ) );

    return LZOpenFile( (LPTSTR) szSetupInfPath, (LPOFSTRUCT) &os, OF_READ );
}

#endif  // WINNT

///////////////////////////////////////////////////////////////////////////////
//
// SkipWhite is real straightforward.  Jump over space, tab, carraige
//            returns and newlines.
//
// return:  Pointer to next non-white char or end of string
//
///////////////////////////////////////////////////////////////////////////////

LPTSTR NEAR PASCAL SkipWhite( LPTSTR lpch )
{
    for( ; ; lpch = CharNext( lpch ) )
    {
        switch( *lpch )
        {
            case TEXT( ' ' ):
            case TEXT( '\t' ):
            case TEXT( '\r' ):
            case TEXT( '\n' ):
                break;

            default:
                return( lpch );
        }
    }
}


///////////////////////////////////////////////////////////////////////////////
//
//  Terminate a line on an unquoted ';', or an EOL.
//  Return 0 if neither one is found, the length otherwise.
//
///////////////////////////////////////////////////////////////////////////////

LPTSTR NEAR PASCAL TermLine( LPTSTR lpPos, LPTSTR pEnd )
{
    BOOL bQuoted = FALSE;

    for( ; ; lpPos = CharNext( lpPos ) )
    {
        if( lpPos >= pEnd )
            return( lpPos );

        switch( *lpPos )
        {
            case TEXT( ';' ):
                if(  bQuoted )
                    break;

            //
            //  This allows a comment at the end of a line. Notice the comment
            //  will actually be skipped when reading the next line.
            //

            case TEXT( '\r' ):
            case TEXT( '\n' ):
                return( lpPos );

            case TEXT( '\0' ):
                return( NULL );

            case TEXT( '"' ):
                bQuoted = !bQuoted;
                break;
        }
    }
}


//
// Locates a given section in an INF file.
// 
// fh - Handle to open INF file.
// pszSect - Section name string.
//
// Returns - Character offset (in file) of first character in section name.
//
DWORD FAR PASCAL FindSection(
#ifdef WINNT
                              HANDLE fh,
#else
                              short fh,
#endif  // WINNT
                              LPTSTR pszSect )
{
    HANDLE  hGlobalBuf  = NULL;
    LPTSTR  lpGlobalBuf = NULL;
    LPTSTR  pch         = NULL;
    LPTSTR  pTmp        = NULL;
    DWORD   dwPos       = 0;
    WORD    cchLen      = 0;
    WORD    cchSize     = 0;
    WORD    cchRead     = 0;
    TCHAR   buffer[ 200 ];
    
    cchLen = lstrlen(pszSect);
    
    //
    // Go to beginning of file
    //

    FSEEK( fh, 0L, SEEK_BEG );

    cchSize = READ_BUFSIZE + 1;
    
    if( !( hGlobalBuf = GlobalAlloc( GMEM_MOVEABLE, cchSize * sizeof(TCHAR) ) )
         || !( lpGlobalBuf = (LPTSTR) GlobalLock( hGlobalBuf ) ) )
    {
        lpGlobalBuf = buffer;
        cchSize = ARRAYSIZE(buffer);
    }
    
    cchSize -= cchLen + (2 + 1);

    FREAD( fh, lpGlobalBuf, cchLen + 2);
    
    do
    {
        //
        // in case of DBCS, it is possible that the last read is a leading byte.
        //
        if (g_bDBCS)
        {
            cchRead = FREAD( fh, lpGlobalBuf + cchLen + 2, cchSize - 1 );
        }
        else
        {
            cchRead = FREAD( fh, lpGlobalBuf + cchLen + 2, cchSize );
        }
        if (0 != cchRead)
        {
            BOOL bEOF = FALSE;

            lpGlobalBuf[ cchRead + cchLen + 2 ] = TEXT( '\0' );

            if (g_bDBCS)
            {
                //
                // in case of DBCS, it is possible that the last read is a leading
                // byte. So appending an extra character for CharNext.
                //
                lpGlobalBuf[ cchRead + cchLen + 3 ] = TEXT( '\0' );
            }

            //
            // Continue as long as I have a complete line.
            //

            pch = lpGlobalBuf;

            while( pTmp = StrChr( pch, TEXT( '\n' ) ) )
            {
               ++pTmp;

               if( *(pch++)==TEXT( '[' ) && !StrCmpN( pch, pszSect, cchLen ) )
               {
                   dwPos += pTmp - lpGlobalBuf;
                   goto FoundIt;
               }

               pch = pTmp;
            }
        
            //
            // Bail out when we reach the end of the file.
            //
            if (g_bDBCS)
            {
                //
                // in case of DBCS, it is possible that the last read is a leading byte.
                //
                bEOF = (cchRead + 1 < cchSize);
            }
            else
            {
                bEOF = cchRead < cchSize;
            }
            if (bEOF)
                break;

            dwPos += cchRead;

            //
            //  Note that we put a TEXT( '\0' ) at offset nRead+wLen+2 and that
            //  TEXT( '\0' ) is illegal in an inf file, so this will copy the last
            //  wLen+2 bytes from the end to the beginning of the buffer
            //

            lstrcpy( lpGlobalBuf, lpGlobalBuf + cchRead );
        }
    }
    while(0 != cchRead);

FoundIt:

    if( lpGlobalBuf != (LPTSTR) buffer )
        GlobalUnlock( hGlobalBuf );

    if( hGlobalBuf )
        GlobalFree( hGlobalBuf );

    return( pTmp ? dwPos : 0 );
}




///////////////////////////////////////////////////////////////////////////////
//
//  Read a line into pPos; the line will start with non-white, and will
//  not contain a comment
//
///////////////////////////////////////////////////////////////////////////////

WORD FAR PASCAL ReadSetupInfCB( LPTSTR pszSection,
                                WORD (FAR PASCAL *lpfnNextLine)( LPTSTR, LPVOID ),
                                LPVOID pData )
{
    HANDLE   hLocal;
    LPTSTR   pBegin, pNext, pEnd, pTemp;
    short    nSize, nReadSize;
    BOOL     bInComment = FALSE;
    DWORD    dwPos;
    WORD     wRet = INSTALL+14;       // This is the "no file" message
    TCHAR    cTemp;


#ifdef WINNT

    HANDLE   fhSetupInf;

    if( ( fhSetupInf = OpenSetupInf( ) ) == (HANDLE) -1 )
        goto Error1;
#else

    int      fhSetupInf;

    if( ( fhSetupInf = OpenSetupInf( ) ) == -1 )
        goto Error1;

#endif  //  WINNT
    
    if( !( dwPos = FindSection( fhSetupInf, pszSection ) ) )
        goto Error2;
    
    nSize = 0;

    wRet = PRN + 3;

GetMoreMem:

    if( !(hLocal = LocalAlloc( LMEM_MOVEABLE | LMEM_ZEROINIT,
                                nSize += READ_BUFSIZE ) ) )
        goto Error2;

    pBegin = (LPTSTR) LocalLock( hLocal );
    
    //
    //  Account for a terminating NULL
    //

    nReadSize = nSize - 1;

    pNext = pBegin;

    pEnd = pBegin + nReadSize;
    
SeekAndRead:

    dwPos += pNext - pBegin;

    //
    //  Get to file position; If we get to EOF, return TRUE
    //

    if( pNext != pEnd )
        FSEEK( fhSetupInf, dwPos, SEEK_BEG );

    if( ( nReadSize = FREAD( fhSetupInf, (LPTSTR)(pNext = pBegin), nReadSize ) ) <= 0 )
        goto FoundEOS;

    *(pEnd = pBegin + nReadSize) = TEXT( '\0' );
    
    while( 1 )
    {
        //
        //  If we were skipping a comment, continue doing so
        //

        if( bInComment )
            goto SkipComment;
    
        while( 1 )
        {
            if( (pNext = SkipWhite( pNext ) ) >= pEnd )
                goto SeekAndRead;
    
            //
            //  We have found a good line if this is not a comment
            //

            if( !( bInComment = (*pNext == TEXT( ';' ) ) ) )
                break;
    
SkipComment:
            for( ; *pNext!=TEXT( '\n' ); pNext = CharNext( pNext ) )
            {
                if( pNext >= pEnd )
                {
                    if (g_bDBCS)
                    {
                        //
                        //  This can only happen if the last byte is a DBCS
                        //  lead byte
                        //

                        if( pNext > pEnd )
                            pNext -= 2;
                    }
                    goto SeekAndRead;
                  }
            }
            bInComment = FALSE;
        }
    
        //
        //  Return TRUE if we have come to the end of the section
        //

        if( *pNext == TEXT( '[' ) )
            break;
    
        //
        //  pTemp==0 means I encountered a TEXT( '\0' ) in the middle of
        //  the file
        //

        if( !( pTemp = TermLine( pNext, pEnd ) ) )
            break;
    
        //
        //  I'm using nSize-1 != nReadSize to indicate EOF
        //  so read again if we have not hit EOL and we are not at EOF
        //

        if( pTemp >= pEnd && nReadSize == nSize-1 )
        {
            if( pNext == pBegin )
            {
                LocalUnlock( hLocal );
                LocalFree( hLocal );
                goto GetMoreMem;
            }
            else
                goto SeekAndRead;
        }
    
        cTemp = *pTemp;

        *pTemp = TEXT( '\0' );

        if( wRet = (*lpfnNextLine )( pNext, pData ) )
            goto Error3;

        *(pNext = pTemp) = cTemp;
    }
    
FoundEOS:
    wRet = NULL;

Error3:
    LocalUnlock( hLocal );
    LocalFree( hLocal );

Error2:
    FCLOSE( fhSetupInf );

Error1:
    return( wRet );
}


///////////////////////////////////////////////////////////////////////////////
//
//  Read a line into pPos; the line will start with non-white, and will
//  not contain a comment
//
///////////////////////////////////////////////////////////////////////////////

short NEAR PASCAL ReadLine(
#ifdef WINNT
                            HANDLE fh,
#else
                            short fh,
#endif  // WINNT
                            DWORD dwPos,
                            LPTSTR pPos,
                            short nSize )
{
    DWORD  dwSavePos = dwPos;
    LPTSTR pStart, pEnd, pTemp;
    short  nSaveSize;
    
    //
    //  Account for a terminating NULL
    //

    nSaveSize = --nSize;
    pStart = pPos;
    
SeekAndRead:
    dwPos += pPos - pStart;

    //
    //  Get to file position
    //

    FSEEK( fh, dwPos, SEEK_BEG );

    if( (nSize = FREAD( fh, (LPTSTR)( pPos = pStart ), nSize ) ) <= 0 )
        return( RL_SECTION_END );

    *(pEnd = pPos + nSize) = TEXT( '\0' );

    while( 1 )
    {
        if( (pPos = OFFSET( SkipWhite( pPos ) ) ) >= pEnd )
            goto SeekAndRead;

        //
        //  We have found a good line if this is not a comment
        //

        if( *pPos != TEXT( ';' ) )
            break;

        for(  ; *pPos != TEXT( '\n' ); pPos = CharNext( pPos ) )
        {
            if( pPos >= pEnd )
            {
                dwPos += pPos - pStart;
                if (g_bDBCS)
                {
                    if( pPos > pEnd )
                    {
                        dwPos -= 2;
                        FSEEK( fh, dwPos, SEEK_BEG );
                    }
                }

                if( (nSize = FREAD( fh, (LPTSTR)( pPos = pStart ), nSize ) )
                             <= 0 )
                {
                    return( RL_SECTION_END );
                }

                *(pEnd = pPos + nSize) = TEXT( '\0' );
            }
        }
    }

    if( *pPos == TEXT( '[' ) )
        return( RL_SECTION_END );

    //
    //  This means I encountered a TEXT( '\0' ) in the middle of the file
    //

    if( !(pTemp = OFFSET( TermLine( pPos, pEnd ) ) ) )
        return( RL_SECTION_END );

    //
    //  I'm using nSize != nSaveSize to indicate EOF
    //

    if( pTemp >= pEnd && nSize == nSaveSize )
    {
        if( pPos == pStart )
            return( RL_MORE_MEM );
        else
            goto SeekAndRead;
    }

    *pTemp = TEXT( '\0' );

    lstrcpy( pStart, pPos );

    return( (int)( dwPos - dwSavePos ) + pTemp - pStart );
}

////////////////////////////////////////////////////////////////////////////////
//
//  This reads a section of setup.inf (control.inf) into a listbox
//  and puts the names into a listbox or combobox; just pass in
//  LB(CB)_ADDSTRING(INSERTSTRING).  If the box is sorted, then
//  ADDSTRING will put it in sorted order.  INSERTSTRING will put
//  it in setup.inf order.
//
///////////////////////////////////////////////////////////////////////////////

int FAR PASCAL ReadSetupInfIntoLBs( HWND hLBName,
                                    HWND hLBDBase,
                                    WORD wAddMsg,
                                    LPTSTR pszSection,
                                    WORD (FAR PASCAL *lpfnGetName)(LPTSTR, LPTSTR ) )
{
#ifdef WINNT
    HANDLE   fhSetupInf;
#else
    int      fhSetupInf;
#endif  //  WINNT
    int    nEntries = 0;
    int    nPlace;
    short  nSuccess;
    DWORD  dwFileLength, dwFilePos;
    HANDLE hLocal, hTemp;
    PTSTR  pLocal;
    TCHAR  szName[ 256 ];
    WORD   wDelMsg, wSize;
    
    //
    //  Determine the delete message (listbox or combobox ).
    //

    wDelMsg = (wAddMsg == LB_ADDSTRING || wAddMsg == LB_INSERTSTRING ) ?
              LB_DELETESTRING : CB_DELETESTRING;
    
    //
    //  Open the file and search for the given section and determine the length.
    //
    
#ifdef WINNT

    if( (fhSetupInf = OpenSetupInf( ) ) == (HANDLE) -1 )
        goto Error1;

#else

    if( (fhSetupInf = OpenSetupInf( ) ) == -1 )
        goto Error1;

#endif  //  WINNT
    
    if( !(dwFilePos = FindSection( fhSetupInf, pszSection ) ) )
        goto Error2;
    
    dwFileLength = FSEEK( fhSetupInf, 0L, SEEK_END );
    
    //
    //  Allocate some memory for reading a line.
    //
    
    if( !(hLocal = LocalAlloc( LMEM_MOVEABLE, wSize = READ_BUFSIZE ) ) )
        goto Error2;
    
    pLocal = (LPTSTR) LocalLock( hLocal );
    
    //
    //  Read a line at a time and add it to the listbox.
    //
    
    while( 1 )
    {
        switch( nSuccess = ReadLine( fhSetupInf, dwFilePos, pLocal, wSize ) )
        {
        case 0:
            goto FileEndReached;
    
        case RL_MORE_MEM:
            //
            //  We should never get to here with reasonable length lines.
            //  If we do get here and are unable to grow the buffer, we must
            //  exit.
            //

            LocalUnlock( hLocal );

            if( !(hTemp = LocalReAlloc( hLocal, wSize += READ_BUFSIZE, LMEM_MOVEABLE ) ) )
                goto Error3;

            pLocal = (LPTSTR) LocalLock( hLocal = hTemp );
            break;
    
        case RL_SECTION_END:
            goto FileEndReached;
            break;
    
        default:
            //
            //  Get the name; add the name (possibly in sorted order);
            //  add the inf string, and increment nEntries if successful,
            //  delete the name otherwise; update the file position.
            //

            (*lpfnGetName )((LPTSTR)szName, (LPTSTR) pLocal );

            if( (nPlace = (int)SendMessage( hLBName, wAddMsg, (UINT) -1,
                                            (DWORD)(LPTSTR) szName ) )
                       >= 0 )
            {
               if( (int) SendMessage( hLBDBase, LB_INSERTSTRING, nPlace,
                                      (DWORD)(LPTSTR) pLocal )
                            >= 0 )
                  ++nEntries;
               else
                  SendMessage( hLBName, wDelMsg, nPlace, 0L );
            }
            if( (dwFilePos += nSuccess) >= dwFileLength )
               goto FileEndReached;
    
            break;
        }
    }
    
FileEndReached:
    LocalUnlock( hLocal );
    
Error3:
    LocalFree( hLocal );
    
Error2:
    FCLOSE( fhSetupInf );
    
Error1:
    return( nEntries );
}


#endif // USE_WIN32_SETUP_API
