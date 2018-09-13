/**********************************************************************
//  SEARCH.C
//
//      Copyright (c) 1992 - Microsoft Corp.
//      All rights reserved.
//      Microsoft Confidential
//
//  Entry point for string search criteria.
//
//  TABS = 3
// johnhe - 04-26-92
**********************************************************************/

#include "shellprv.h"
#pragma  hdrstop

#pragma warning( disable:4001 )         // Disable new type remark warning

//*********************************************************************
//  Local function prototypes.
//*********************************************************************

void BldSearchLsts   ( LPGREPINFO lpgi, LPSTR pStrLst, LPSTR pNotList );
static LPSTR s_pSrchStrLst = NULL;            // Ptr to normal search string list

//*********************************************************************
//  Builds allocates search buffers and builds search list trees.
//
//  The format of the pStrLst and pNotLst is multiple zero terminated
//  strings with a zero length string marking the end of the list.
//
//  "string1",0,"string2",0,"string3",0,"...",0,0
//
//  int InitGrepInfo( LPSTR pStrLst, LPSTR pNotList, unsigned uOpts )
//
//  ARGUMENTS:
//      pStrLst     - Ptr to list of string to try to match
//      pNotLst     - Ptr to list of string which must not match
//      uOpts           - Search options of FLG_REGULAR and FLG_CASE
//  RETURNS:
//      int         - OK is no errors else ERR_NOMEMORY, ERR_MEMCORUPT,
//                        ERR_STRLST_LEN, or ERR_SRCH_EXPRESSION
//
//*********************************************************************

LPGREPINFO InitGrepInfo( LPSTR pStrLst, LPSTR pNotList, unsigned uOpts )
{
    int     i;                                      // Loop indice
    LPGREPINFO lpgi;

            // Make sure there is at least one string to match in at least
            // one of the search lists and set flag if no normal srch strings.

    if ( pStrLst[ 0 ] == '\0' && pNotList[ 0 ] == '\0' )
        return( OK );

    s_pSrchStrLst = pStrLst;

    if ( (lpgi = InitGrepBufs()) == NULL )
        return( NULL );

    if ( (uOpts & FLG_REGULAR) != FALSE )   // Use regular expression (/R) ?
        lpgi->addstr = addexpr;                       // Then add expression to list
    else
        lpgi->addstr = addstring;                     // Else treat strings literally

                                                        // Set if case sensitivity
    lpgi->CaseSen = ((uOpts & FLG_CASE) ? TRUE : FALSE);

                                            // Set longjmp to allow aborting on errors
    _try
    {
        BldSearchLsts( lpgi, pStrLst, pNotList );

        if ( !(uOpts & FLG_REGULAR) )           // If not using expressions
        {
            lpgi->find = findlist;              // Assume finding many

            for ( i = 0; i < 2; i++ )
            {
                _fmemset(lpgi->ge.td1, (int)(lpgi->ge.ShortStrLen + 1), TRTABLEN);  // Init lpgi->ge.td1 tables
                enumstrings(lpgi);              // Build lpgi->ge.td1 table


                if (lpgi->ge.StrCount == 1 && lpgi->CaseSen)
                    lpgi->find = findone;       // Find one case-sensitive string
                else
                    lpgi->find = findlist;      // Assume finding many
                SwapSrchTables(lpgi);           // Do the data swap
            }
        }
        else if (lpgi->find == NULL)
            lpgi->find = findexpr;                        // Else find expressions
    }

    // Let the debugger get a chance at the exceptions first...
    _except(SetErrorMode(SEM_NOGPFAULTERRORBOX),UnhandledExceptionFilter(GetExceptionInformation()))
    {
        FreeGrepBufs(lpgi);
        return NULL ;
    }

    return( lpgi );
}

//*********************************************************************
//  Initializes the search tree data for both the search list and
//  not search list.
//
//  void BldSearchLsts( LPSTR pStrLst, LPSTR pNotList )
//
//  ARGUMENTS:
//      pStrLst - Ptr to list of strings or expression to match
//      pNotLst - Ptr to list of strings or expression which must not match
//  RETURNS:
//      void
//
//*********************************************************************

void BldSearchLsts( LPGREPINFO lpgi, LPSTR pStrLst, LPSTR pNotList )
{
    LPSTR   szNext;                                // Ptr to next string to add
    int     i;                                      // Loop indice
    int     iStrLen;                                // Length of current string

        // Loop 3 times. First iteration add all search strings to normal
        // search list, 2nd iteration add all /NOT string to the normal
        // search list and then swap the search data so that on the
        // 3rd iteration all the /NOT string will be added to the /NOT
        // search list. Then swap the data again to restore the original.

    for ( szNext = pStrLst, i = 0; i < 3; i++ )
    {
        while ( *szNext != '\0' )
        {
            iStrLen = (int)lstrlenA( szNext );

                // If doing string search make sure this is not a duplicate
                // before adding it to the search tree.

            if ( lpgi->addstr == addexpr ||
                  findlist( lpgi, szNext, szNext + iStrLen ) == NULL )
                (*lpgi->addstr)( lpgi, szNext, iStrLen ); // Add string to list
            szNext += iStrLen + 1;
        }

        if ( i == 1 )                               // Is this the 2nd iteration
            SwapSrchTables(lpgi);                   // Do the data swap

                // Set string ptr to /NOT list for 2nd and 3rd iterations.
        szNext = pNotList;
    }

    SwapSrchTables(lpgi);                           // Restore the original data area
}

//*********************************************************************
//  Reads in portions of an open file and search the strings initialized
//  by BldSearchLsts().
//
// If (fFlags & FLG_FIND_FILE) will continue reading thru the file until
//  a matching search string is found or the end of the file is reached.
//  If there are string specified in the /NOT list the file will continue
//  to be scanned until either a string the the /NOT list is detected
//  or the end of the file is reached.
//
//  If !(fFlags & FLG_FIND_FILE) the file will be searched for lines
//  which match containing a matching string and then the user supplied
//  callback will be called for each line which contains a match or
//  does not contain a match, depending on the bits in fFlags.
//
//
//  NOTE:
//      InitSearchInfo() must be called to setup the search information
//      and intialize the search buffers before this function is called
//      the first time.
//
//  int FileFindGrep( HANDLE fh, unsigned fFlags, FIND_CALLBACK FindCb )
//
//  ARGUMENTS:
//      fHandle - Open DOS file handle of file to search
//      fFlags  - Operation flags can be a combination of:
//                    FLG_FIND_FILE  - Only try to locate the file (EXCLUSIVE)
//                    FLG_FIND_NOMATCH - Display non-matching lines
//                    FLG_FIND_MATCH   - Display matching lines
//                    FLG_FIND_COUNT     - Display on count of matching lines
//                    FLG_FIND_LINENO  - Display line numbers on output
//      FindCb  - Call function for displaying matching strings or NULL
//                    if (fFlags & FLG_FIND_FILE).
//  RETURNS:
//      int     - If (fFlags & FLG_FIND_FILE) returns TRUE if at least one
//                    string match from pStrLst and no string match from pNotLst
//                    else FALSE. If !(fFlags & FLG_FIND_FILE) returns OK
//                    if no errors else returns ERR_FILE_READ or any value
//                    returned by a callback function.
//
//*********************************************************************

int FileFindGrep( LPGREPINFO lpgi, HANDLE fh, unsigned fFlags,
                  long (far pascal *AppCb)( int Func,
                                             unsigned uArg0,
                                             void far *pArg1,
                                             unsigned long ulArg2 ) )
{
    register LPSTR  BufPtr;             // Buffer pointer
    LPSTR           EndBuf;             // End of buffer
    LPSTR           pchChar;            // Ptr to matching string in buffer

#ifdef DO_CALLBACK
    TCHAR            *pchBegin;          // Ptr to start of line
    TCHAR            *pchEnd;            // Ptr to end of line
#endif
    int             iStatus;            //
    DWORD           ByteCount;          // Byte count
    unsigned        TailLen;            // Length of buffer tail
    unsigned long   ulLineNum;          // Current line number in the file
    unsigned long   ulMatchCnt;         // Count of lines containing match

    lpgi->ReadBuf[ 3 ] = '\n';          // Mark beginning with newline
    BufPtr = lpgi->ReadBuf + 4;         // Set buf ptr to after newline
    TailLen = 0;                        // No buffer tail yet
    ulLineNum = 0UL;                    // Start with line 1
    ulMatchCnt = 0UL;                   // Zero the match count
    iStatus = OK;                       // Assume no errors to start

    // Loop filling the buffer and then searching it
    while ( ReadFile(fh, BufPtr, SectorBased( FILBUFLEN - TailLen),
                     &ByteCount, NULL)
                && (ByteCount + TailLen != 0))
    {
        if ( ByteCount == 0 )
        {                                               // If buffer tail is all that's left
            TailLen = 0;                            // Set tail length to zero
            *BufPtr++ = '\r';                       // Add end of line sequence
            *BufPtr++ = '\n';
            EndBuf = BufPtr;                        // Note end of buffer
        }
        else                                            // Else start next read
        {                                               // Find length of partial line
            TailLen = (unsigned)preveol( BufPtr + ByteCount - 1 );
            if ( TailLen == ByteCount )     // Find tail length after last LF
            {
                TailLen = 0;                        // No linefeeds in buffer
                EndBuf = BufPtr + ByteCount;
            }
            else
                EndBuf = BufPtr + ByteCount - TailLen;
        }

        pchChar = lpgi->ReadBuf + 4;

            // Loop searching thru the buffer for matching strings

        while ( pchChar != NULL &&
                EndBuf > pchChar &&
                (EndBuf - pchChar) >= lpgi->ge.ShortStrLen )
        {
            if ( !(fFlags & FIND_FILE) )
            {
#ifdef DO_CALLBACKS
                ulLineNum++;                        // Increment the line count
                pchBegin = pchChar;
                pchEnd = NextEol( pchChar, EndBuf );
                if ( (pchChar = (*find)( pchChar, pchEnd )) != NULL )
                {
                    ulMatchCnt++;
                    if ( !(fFlags & FIND_NOMATCH) &&
                          !(fFlags & FIND_COUNT) )
                        iStatus = CB_FindMatch( pchEnd - pchBegin,
                                                        pchBegin, ulLineNum );
                }
                else if ( (fFlags & FLG_FIND_NOMATCH) )
                    iStatus = CB_FindMatch( pchEnd - pchBegin, pchBegin,
                                                    ulLineNum );
                if ( iStatus != OK )
                    return( iStatus );
                pchChar = pchEnd;
#endif // DO_CALLBACKS
            }

            else if ( (pchChar = (*lpgi->find)( lpgi, pchChar, EndBuf )) != NULL )
            {
                    // If we looking for /NOT strings the
                    // findfile fails unless NoSrchStr == TRUE, if we are not
                    // looking for /NOT strings it means we found a normal
                    // string so see if there are any not strings.

                if ( (fFlags & FIND_NOT) )
                {
                    SwapSrchTables(lpgi);   // Restore the normal data area
                    return( FALSE );        // Always FALSE if /NOT strs found
                }
                else if ( lpgi->geNot.TblEntriesUsed <= 1 )    // Are there any /NOT strings
                    return( TRUE );         // Found a match and no /NOT strings
                else
                {
                    SwapSrchTables(lpgi);   // Need to search for /NOT strings
                    fFlags |= FIND_NOT;     // Signal we've changed the data
                }
            }
                                            // Copy tail to head of buffer
        }
        if ( TailLen != 0 )
            hmemcpy( lpgi->ReadBuf + 4, EndBuf, TailLen );
        BufPtr = lpgi->ReadBuf + TailLen + 4; // Skip over tail

        // If our last read returned no bytes
    }


    if ( !(fFlags & FIND_FILE) )
#ifdef DOCALLBACKS
        return( CB_FindCount( ulMatchCnt ) );
#else
        return(1);
#endif //CALLBACK
    else if ( !(fFlags & FIND_NOT) )
        return( (s_pSrchStrLst[ 0 ] == '\0') ? TRUE : FALSE );

    SwapSrchTables(lpgi);                   // Restore the normal data area
    return( TRUE );                         // No /NOT strings found success
}

//*********************************************************************
//*********************************************************************

LPSTR NextEol( LPSTR pchChar, LPSTR EndBuf )
{
    pchChar++;                                      // String starts with /r

    while( pchChar < EndBuf && *pchChar != 0x0A )
        pchChar++;

    return( pchChar );
}
