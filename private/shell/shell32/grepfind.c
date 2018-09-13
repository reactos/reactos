/**********************************************************************
//  NEWFIND.C
//
//      Copyright (c) 1992 - Microsoft Corp.
//      All rights reserved.
//      Microsoft Confidential
//
//  TABS = 3
**********************************************************************/

#include "shellprv.h"
#pragma  hdrstop

#pragma warning( disable:4001 )         // Disable new type remark warning

//*********************************************************************
//
//  Miscellaneous constants and macros
//
//*********************************************************************

//*********************************************************************
//  Global data
//*********************************************************************

// BUGBUG (DavePl) This code throws exceptions, and I don't see anyone
//                 trying to catch them (for allocation failures)

//*********************************************************************
//  Intializes all global buffers and variables. aGlobalBufs[] is an
//  array of pointers to the global pointers which are allocated to
//  the sizes in the array aBufLens[]. DummyFirst and nDummyFirst
//  are the first wordss of the block of variables associated with
//  normal and /NOT searches. Each block of variables is initialized
//  by copying the static block starting with InitialSearchData
//  to the repective data areas.
//
//  int InitGrepBufs( void )
//
//  ARGUMENTS:
//      NONE
//  RETURNS:
//      int     - OK if no errors ELSE ERR_NOMEMORY.
//
//*********************************************************************

LPGREPINFO InitGrepBufs( void )
{
    //
    // We first want to allocate a global structure for this Grep instance
    //
    LPGREPINFO lpgi;

    lpgi = (LPGREPINFO)Alloc(SIZEOF(GREPINFO));
    if (lpgi == NULL)
        return(NULL);

    // Our Alloc function initializes the data to zero.

    //lpgi->Flags = 0;
    lpgi->CaseSen = 1;                                // Assume case-sensitivity

    // Initialize Search data
    lpgi->ge.TblEntriesUsed = 1;
    //lpgi->ge.ExprEntriesUsed = 0;
    //lpgi->ge.StrCount = 0;
    //lpgi->ge.TargetLen = 0;
    //lpgi->ge.MaxChar = 0;
    lpgi->ge.MinChar = 0xffff;
    lpgi->ge.ShortStrLen = 0xffff;

    // Initialize Not Search data
    lpgi->geNot.TblEntriesUsed = 1;
    //lpgi->geNotnExprEntriesUsed = 0;
    //lpgi->geNotnStrCount = 0;
    //lpgi->geNotnTargetLen = 0;
    //lpgi->geNotnMaxChar = 0;
    lpgi->geNot.MinChar = 0xffff;
    lpgi->geNot.ShortStrLen = 0xffff;

    //lpgi->addstr = NULL;                         // Initialize function pointers
    //lpgi->find = NULL;

    _fmemset( lpgi->ge.td1, 1, TRTABLEN );         // Set up TD1 for startup
    _fmemset( lpgi->geNot.td1, 1, TRTABLEN );     // Set up /NOT TD1 for startup

    return( lpgi );
}

//*********************************************************************
//  Frees all previously allocated global buffers. aPtrLst is an array
//  of pointers to the all of the global pointers. Only those pointers
//  which are not NULL are freed and then set to NULL.
//
//  Must first free all of the memory blocks allocated with alloc()
//  for the linked lists in both the normal and /NOT search trees.
//
//  int FreeGrepBufs( LPGREPIFIN lpgi )
//
//  ARGUMENTS:
//      NONE
//  RETURNS:
//      int     - OK (0) if successfull else ERR_MEM_CORRUPT
//
//*********************************************************************

int FreeGrepBufs(LPGREPINFO lpgi )
{
    int     i;
    int     x;

    for ( x = 0; x < 2; x++ )
    {
        for ( i = 0; i < lpgi->ge.TblEntriesUsed; i++ )
            freenode( lpgi, lpgi->ge.StringList[ i ] );

        for ( i = 0; i < lpgi->ge.ExprEntriesUsed; i++ )
            if ( !Free(lpgi->ge.ExprStrList[ i ] ))
                return( ERR_MEM_CORRUPT );

        SwapSrchTables(lpgi);
    }

    // Now lets free the actual header struture
    if (!Free(lpgi))
        return( ERR_MEM_CORRUPT );

    return( OK );
}

//*********************************************************************
//  Swaps all variables directly associated with the normal search
//  with those used in the /NOT search.
//
//  void SwapSrchTables( void )
//
//  ARGUMENTS:
//      NONE
//  RETURNS:
//      void
//*********************************************************************

void SwapSrchTables( LPGREPINFO lpgi )
{
    GREPELEMENTS geTemp;

    geTemp = lpgi->ge;
    lpgi->ge = lpgi->geNot;
    lpgi->geNot = geTemp;
}

//*********************************************************************
//  Frees the memory allocated to previously allocated node and frees
//  all nodes attached to the specified node.
//
//  void freenode( register STRINGNODE FAR *pNode )
//
//  ARGUMENTS:
//      pNode   - Pointer to node to free
//  RETURNS:
//      void
//
//*********************************************************************

void freenode(LPGREPINFO lpgi,  register STRINGNODE *pNode )
{
    register STRINGNODE *pTmpNode;          // Pointer to next node in list

    while ( pNode != NULL )                     // While not at end of list
    {
        if ( pNode->s_suf != NULL )             // Free suffix list if not end
            freenode(lpgi, pNode->s_suf );
        else
            lpgi->ge.StrCount--;                // Else decrement string count

        pTmpNode = pNode;                       // Save pointer
        pNode = pNode->s_alt;                   // Move down the list

        if ( pTmpNode != NULL && !Free( pTmpNode ) )
            RaiseException(ERROR_INVALID_BLOCK, 0, 0, 0);
    }
}


//*********************************************************************
//  Allocates memory if this fails we thrown an exception...
//  This is basically lazy programming and should be changed later...
//
//  LPVOID AllocThrow ( long cb )
//
//  ARGUMENTS:
//      cb                  - Number of bytes to allocate
//
//  RETURNS:
//      LPVOID - Ptr to new memory
//*********************************************************************

LPVOID AllocThrow (LPGREPINFO lpgi, long cb)
{
    LPVOID lp;

    lp = Alloc(cb);
    if (lp == NULL)
        RaiseException(ERROR_NOT_ENOUGH_MEMORY, 0, 0, 0);
    return lp;
}



//*********************************************************************
//  Allocates memory for a new node large enough to hold a copy of
//  a caller specifed string and then initializing the new node with
//  the caller supplied string.
//
//  STRINGNODE FAR *newnode( LPSTR String, int StrLen )
//
//  ARGUMENTS:
//      *String             - String to include in new node
//      StrLen              - Length of string argument
//  RETURNS:
//      STRINGNODE* - Ptr to a new node
//*********************************************************************

STRINGNODE FAR* newnode( LPGREPINFO lpgi, LPSTR String, int StrLen )
{
    register STRINGNODE *pNewNode;          // Pointer to new node

                                                        // Allocate string node
    pNewNode = (STRINGNODE *)AllocThrow(
            lpgi,  SIZEOF( STRINGNODE ) + (unsigned)StrLen +
                                              (unsigned)(StrLen & 1) + 1);

    pNewNode->s_alt = NULL;                     // No alternates yet
    pNewNode->s_suf = NULL;                     // No suffixes yet
    pNewNode->s_must = StrLen;                  // Set string length
                                                        // Copy string text into node buffer
    lstrcpynA( s_text( pNewNode ), String, (unsigned)StrLen + 1);
    return( pNewNode );                         // Return pointer to new node
}

//*********************************************************************
//  Updates an existing node with a new string passed by the caller.
//
//  WARNING:
//      The new string must be shorter than the original string for
//      the specified node or the buffer will overflow.
//
//  ARGUMENTS:
//      pNode       - Pointer to node
//      s           - String
//      n           - Length of string
//*********************************************************************

STRINGNODE FAR* reallocnode( register STRINGNODE *pNode, LPSTR pStr, int StrLen )
{
    #ifndef NDEBUG
        ASSERT( StrLen < pNode->s_must );
    #endif

    pNode->s_must = StrLen;                         // Set new length
                                                        // Copy new text
    hmemcpy( s_text( pNode ), pStr, (unsigned)StrLen );
    return( pNode );                                // Return pointer to original node
}

//*********************************************************************
// maketd1 - add entry for TD1 shift table
//
//   This function fills in the TD1 table for the given
//   search string.  The idea is adapted from Daniel M.
//   Sunday's QuickSearch algorithm as described in an
//   article in the August 1990 issue of "Communications
//   of the ACM".  As described, the algorithm is suitable
//   for single-string searches.    The idea to extend it for
//   multiple search strings is mine and is described below.
//
//       Think of searching for a match as shifting the search
//       pattern p of length n over the source text s until the
//       search pattern is aligned with matching text or until
//       the end of the source text is reached.
//
//       At any point when we find a mismatch, we know
//       we will shift our pattern to the right in the
//       source text at least one position.  Thus,
//       whenever we find a mismatch, we know the character
//       s[n] will figure in our next attempt to match.
//
//       For some character c, TD1[c] is the 1-based index
//       from right to left of the first occurrence of c
//       in p.  Put another way, it is the count of places
//       to shift p to the right on s so that the rightmost
//       c in p is aligned with s[n].  If p does not contain
//       c, then TD1[c] = n + 1, meaning we shift p to align
//       p[0] with s[n + 1] and try our next match there.
//
//       Computing TD1 for a single string is easy:
//
//           _fmemset(TD1,n + 1,SIZEOF TD1);
//           for (i = 0; i < n; ++i) {
//                TD1[p[i]] = n - i;
//           }
//
//       Generalizing this computation to a case where there
//       are multiple strings of differing lengths is trickier.
//       The key is to generate a TD1 that is as conservative
//       as necessary, meaning that no shift value can be larger
//       than one plus the length of the shortest string for
//       which you are looking.  The other key is to realize
//       that you must treat each string as though it were only
//       as long as the shortest string.  This is best illustrated
//       with an example.  Consider the following two strings:
//
//       DYNAMIC PROCEDURE
//       7654321 927614321
//
//       The numbers under each letter indicate the values of the
//       TD1 entries if we computed the array for each string
//       separately.  Taking the union of these two sets, and taking
//       the smallest value where there are conflicts would yield
//       the following TD1:
//
//       DYNAMICPODURE
//       7654321974321
//
//       Note that TD1['P'] equals 9; since n, the length of our
//       shortest string is 7, we know we should not have any
//       shift value larger than 8.  If we clamp our shift values
//       to this value, then we get
//
//       DYNAMICPODURE
//       7654321874321
//
//       Already, this looks fishy, but let's try it out on
//       s = "DYNAMPROCEDURE".  We know we should match on
//       the trailing procedure, but watch:
//
//       DYNAMPROCEDURE
//       ^^^^^^^|
//
//       Since DYNAMPR doesn't match one of our search strings,
//       we look at TD1[s[n]] == TD1['O'] == 7.  Applying this
//       shift, we get
//
//       DYNAMPROCEDURE
//                ^^^^^^^
//
//       As you can see, by shifting 7, we have gone too far, and
//       we miss our match. When computing TD1 for "PROCEDURE",
//       we must take only the first 7 characters, "PROCEDU".
//       Any trailing characters can be ignored (!) since they
//       have no effect on matching the first 7 characters of
//       the string.  Our modified TD1 then becomes
//
//       DYNAMICPODURE
//       7654321752163
//
//       When applied to s, we get TD1[s[n]] == TD1['O'] == 5,
//       leaving us with
//
//       DYNAMPROCEDURE
//              ^^^^^^^
//       which is just where we need to be to match on "PROCEDURE".
//
//   Going to this algorithm has speeded qgrep up on multi-string
//   searches from 20-30%.  The all-C version with this algorithm
//   became as fast or faster than the C+ASM version of the old
//   algorithm.  Thank you, Daniel Sunday, for your inspiration!
//
//   Note: if we are case-insensitive, then we expect the input
//   string to be upper-cased on entry to this routine.
//
//   Pete Stewart, August 14, 1990.
//*********************************************************************

void maketd1( LPGREPINFO lpgi, LPBYTE pch, int cch, int cchstart )
{
    int     ch;                             // Character
    int     i;                              // String index

    if ( (cch += cchstart) > lpgi->ge.ShortStrLen )
        cch = lpgi->ge.ShortStrLen;         // Use smaller count

    for ( i = cchstart; i < cch; i++ )
    {                                       // Examine each char left to right
        ch = (int)((unsigned char)*pch++);          // Get the character
        for (;;)
        {                                   // Loop to set up entries
            if ( ch < lpgi->ge.MinChar )
                lpgi->ge.MinChar = ch;      // Remember if smallest

            if ( ch > lpgi->ge.MaxChar )
                lpgi->ge.MaxChar = ch;      // Remember if largest

            if ( lpgi->ge.ShortStrLen - i < (int)lpgi->ge.td1[ ch ] )
                lpgi->ge.td1[ ch ] = (unsigned char)(lpgi->ge.ShortStrLen - i);

                                            // Set value if smaller than previous
            if (lpgi->CaseSen || !IsCharUpper( (char)ch ) )
                break;                      // Exit loop if done
            ch = LOWORD((DWORD_PTR)CharLowerA( (LPSTR)(DWORD_PTR)ch ));  // Force to lower case
        }
    }
}


//*********************************************************************
//  static int newstring( LPGREPINFO lpgi, LPBYTE s, int StrLen )
//
//  ARGUMENTS:
//      pStr        - String to add
//      StrLen  - Length of string
//  RETURNS:
//
//*********************************************************************

int newstring( LPGREPINFO lpgi, LPBYTE pStr, int StrLen )
{
    register STRINGNODE *pCurNode;          // Current string
    register STRINGNODE * *ppPrevNode;  // Pointer to previous link
    STRINGNODE          *pNewNode;          // New string
    int                     i;                      // Index
    int                     iNumMatched;        // Count of matched chars in 2 strings
    int                     iRelation;          // Count

    if ( lpgi->ge.ShortStrLen == -1 || StrLen < lpgi->ge.ShortStrLen )
        lpgi->ge.ShortStrLen = StrLen;                   // Remember length of shortest string

    if ( (i = lpgi->ge.TransTable[ *pStr ]) == 0 )   // If no existing list
    {
                                                        //  We have to start a new list
                                                        // Die if too many string lists
        if ( (i = lpgi->ge.TblEntriesUsed++) >= ASCII_LEN )
            RaiseException(ERROR_INVALID_PARAMETER, 0, 0, 0);

        #ifndef NDEBUG
            ASSERT( lpgi->ge.StringList[ i ] == NULL );  // Assert already initialized
        #endif

        lpgi->ge.TransTable[ *pStr ] = (unsigned char)i;     // Set pointer to new list

                                                        // Set pointer for other case
        if ( !lpgi->CaseSen && IsCharAlpha( *pStr ) )
            lpgi->ge.TransTable[ *pStr ^ '\040' ] = (unsigned char)i;
    }
    else if ( lpgi->ge.StringList[ i ] == NULL )
        return( 0 );

    if ( --StrLen == 0)                             // If 1-byte string
    {
        freenode( lpgi, lpgi->ge.StringList[ i ] ); // Free any existing stuff
        lpgi->ge.StringList[ i ] = NULL;            // No record here
        lpgi->ge.StrCount++;                        // We have a new string
        return( 1 );                                // String added
    }

    pStr++;                                         // Skip first char
    ppPrevNode = lpgi->ge.StringList + i;           // Get pointer to link
    pCurNode = *ppPrevNode;                     // Get pointer to node

    while ( pCurNode != NULL )                  // Loop to traverse match tree
    {
                                                        // Find minimum of string lengths
        i = (StrLen > pCurNode->s_must) ? pCurNode->s_must : StrLen;
                                                        // Compare the strings

        matchstrings( lpgi, (LPSTR )pStr, s_text( pCurNode ),
                          i, &iNumMatched, &iRelation );

        if ( iNumMatched == 0 )                 // If complete mismatch
        {
            if ( iRelation < 0 )                    // Was pStr <  s_text( pCurNode )
                break;                              // Break if insertion point found

            ppPrevNode = &(pCurNode->s_alt); // Get pointer to alternate link
            pCurNode = *ppPrevNode;             // Follow the link
        }

        else if ( i == iNumMatched )                        // Else if strings matched
        {
            if ( i == StrLen )                  // If new is prefix of current
            {
                                                        // Shorten text of node
                pCurNode = reallocnode(pCurNode, s_text( pCurNode ), StrLen);
                *ppPrevNode = pCurNode;
                                                        // If there are suffixes
                if ( pCurNode->s_suf != NULL )
                {
                    freenode( lpgi, pCurNode->s_suf );
                                                        // Suffixes no longer needed
                    pCurNode->s_suf = NULL;
                    lpgi->ge.StrCount++;                     // Account for this string
                }
                return( 1 );                        // String added
            }
            ppPrevNode = &(pCurNode->s_suf); // Get pointer to suffix link

            if ( (pCurNode = *ppPrevNode) == NULL )
                return (0);                         // Done if current is prefix of new

            pStr += i;                              // Skip matched portion
            StrLen -= i;
        }

        else                                            // Else partial match
        {
                //  We must split an existing node.
                //  This is the trickiest case.

            pNewNode = newnode( lpgi, s_text( pCurNode ) + iNumMatched,
                                      pCurNode->s_must - iNumMatched );

                                                        // Unmatched part of current string
            pCurNode = reallocnode( pCurNode, s_text( pCurNode ), iNumMatched );
            *ppPrevNode = pCurNode;

                                                        // Set length to matched portion
            pNewNode->s_suf = pCurNode->s_suf;  // Current string's suffixes

            if ( iRelation < 0 )                    // If new preceded current
            {                                           // First suffix is new string
                pCurNode->s_suf = newnode( lpgi, (LPSTR )pStr + iNumMatched, StrLen - iNumMatched );
                                                        // Alternate is part of current
                pCurNode->s_suf->s_alt = pNewNode;
            }
            else                                        // Else new followed current
            {                                           // Unmatched new string is alternate
                pNewNode->s_alt = newnode( lpgi, (LPSTR )(pStr + iNumMatched), StrLen - iNumMatched );
                pCurNode->s_suf = pNewNode;     // New suffix list
            }
            lpgi->ge.StrCount++;                             // One more string
            return( 1 );                            // String added
        }
    }

                                                        // Set pointer to new node
    *ppPrevNode = newnode( lpgi, (LPSTR )pStr, StrLen );
    (*ppPrevNode)->s_alt = pCurNode;        // Attach alternates
    lpgi->ge.StrCount++;                                     // One more string
    return( 1 );                                    // String added
}

//*********************************************************************
//  void addstring( LPGREPINFO lpgi, register LPSTR pStr, int StrLen )
//
//  ARGUMENTS:
//      pStr        - String to add
//      StrLen  - Length of string
//  RETURNS:
//      void
//*********************************************************************

void addstring( LPGREPINFO lpgi, register LPSTR pStr, int StrLen )
{
    register LPSTR pchChar;                     // Char pointer
    int             EndLine;                    // Match-at-end-of-line flag
    char            szTemp[MAXSTRLEN];          // Used as a scratchpad...
    BOOL            fOemDiff;                   // Is the oem characters different

    EndLine = lpgi->Flags & ENDLINE;            // Initialize flag
    pchChar = lpgi->Target;                     // Initialize pointer
    while ( StrLen-- > 0 )                      // While not at end of string
    {
        if (StrLen > 0 && IsDBCSLeadByte(*pStr))
        {
            // skip special character check
            *pchChar++ = *(pStr++);
            *pchChar++ = *(pStr++);
            StrLen--;
        }
        else
        {
            switch ( *pchChar = *(pStr++) )       // Switch on character
            {
                case '\\':                        // Escape
                                                  // If next character "special"
                    if ( StrLen > 0 && !IsCharAlphaNumeric( *pStr ) )
                    {
                        StrLen--;                 // Decrement counter
                        *pchChar = *(pStr++);     // Copy next character
                    }
                    pchChar++;                    // Increment pointer
                    break;

                case '$':                         // Special end character
                    if ( StrLen == 0 )            // If end of string
                    {
                        EndLine = ENDLINE;        // Set flag
                        break;                    // Exit switch
                    }
                                                  // Drop through
                default:                          // All others
                    pchChar++;                    // Increment pointer
                    break;
            }
        }
    }
    if ( EndLine )
        *pchChar++ = EOS;                       // Add end character if needed
    lpgi->ge.TargetLen = (int) (pchChar - lpgi->Target);// Compute Target string length

    CharToOemBuffA(lpgi->Target, szTemp, lpgi->ge.TargetLen);
    fOemDiff = memcmp(lpgi->Target, szTemp, lpgi->ge.TargetLen) != 0;

    if ( !lpgi->CaseSen )
        CharUpperBuffA( lpgi->Target, lpgi->ge.TargetLen );     // Force to upper case if necessary
                                                // Add the string
    newstring( lpgi, (LPBYTE )lpgi->Target, lpgi->ge.TargetLen );

    // So far we have worked with Ansi characters.  Now lets try converting
    // this to OEM.  If the strings don't match we also add the OEM version
    // as to allow files typically made by Dos Apps to find their strings...

    if (fOemDiff)
    {
        DebugMsg(DM_TRACE, TEXT("Grep add OEM version of string %s"), szTemp);
        newstring( lpgi, (LPBYTE )szTemp, lpgi->ge.TargetLen );
    }



}

//*********************************************************************
//
//  int addstrings( register LPSTR pBuffer, register LPSTR pBufEnd,
//                       LPSTR pSepList, int *IsFirst )
//
//  ARGUMENTS:
//      pBuffer     - String list buffer containing search strings
//      pBufEnd     - End of search string buffer
//      pSepList        - List of search string separators
//      IsFirst     - Unused
//  RETURNS:
//      int         - Always returns 0
//
//*********************************************************************

int addstrings( LPGREPINFO lpgi, register LPSTR pBuffer, register LPSTR pBufEnd,
                     LPSTR pSepList, int *IsFirst )
{
    int StrLen;                                     // String length

    while ( pBuffer < pBufEnd )                 // While buffer not empty
    {
                                                        // Count leading separators
        StrLen = strnspn( pBuffer, pSepList, (int) (pBufEnd - pBuffer) );

                                                        // Skip leading separators
        if ( (pBuffer += StrLen) >= pBufEnd )
            break;
                                                        // Get length of search string
        StrLen = strncspn( pBuffer, pSepList, (int) (pBufEnd - pBuffer) );

                                                        // Select search string type
        if ( lpgi->addstr == NULL )
            lpgi->addstr = isexpr( lpgi, pBuffer, StrLen ) ? addexpr : addstring;

                                                        // If no match within string
        if ( lpgi->addstr == addexpr || (lpgi->Flags & (BEGLINE | COLNOS)) ||
              findlist( lpgi, pBuffer, pBuffer + StrLen ) == NULL )
            (*lpgi->addstr)( lpgi, pBuffer, StrLen );       // Add string to list

        pBuffer += StrLen;                      // Skip the string
    }

    return( 0 );
}

//*********************************************************************
//  int enumlist( LPGREPINFO lpgi, register STRINGNODE FAR *pNode, int cchprev )
//
//  ARGUMENTS:
//      pNode       - Pointer to list to dump
//      cchprev - Count of preceding characters
//  RETURNS:
//      int     - Number of strings in pNode
//
//*********************************************************************

int enumlist( LPGREPINFO lpgi, register STRINGNODE *pNode, int cchprev )
{
    int StrCnt;                                     // String count

    StrCnt = 0;                                     // Initialize string count
    while ( pNode != NULL )                     // While not at end of list
    {
                                                        // Make TD1 entries
        maketd1( lpgi, (LPBYTE )s_text( pNode ), (int)pNode->s_must, cchprev );
                                                        // Recurse to do suffixes
        StrCnt += (pNode->s_suf != NULL) ?
                enumlist( lpgi, pNode->s_suf, cchprev + pNode->s_must ) : 1;
        pNode = pNode->s_alt;                   // Do next alternate in list
    }
    return( StrCnt ? StrCnt : 1 );          // Return string count
}

//*********************************************************************
//  int enumstrings( LPGREPINFO lpgi )
//
//  ARGUMENTS:
//      NONE
//  RETURNS:
//      int     - Total number of strings in all StringList nodes
//*********************************************************************

int enumstrings( LPGREPINFO lpgi )
{
    unsigned char   uchChar;                    // Character
    int             i;                          // Index
    int             StrCnt;                     // String count

    StrCnt = 0;                                 // Initialize
    for ( i = 0; i < TRTABLEN; i++ )            // Loop through translation table
    {
        if ( lpgi->CaseSen || !IsCharLower( (unsigned char)i ) )
        {                                       // If case sensitive or not lower
            if (lpgi->ge.TransTable[ i ] == 0)
                continue;                       // Skip null entries

            uchChar = (unsigned char)i;                 // Get character
            maketd1( lpgi, &uchChar, 1, 0);     // Make TD1 entry

                                                        // Enumerate the list
            StrCnt += enumlist( lpgi,
                    lpgi->ge.StringList[ lpgi->ge.TransTable[ i ] ], 1 );
        }
    }
    return( StrCnt );                               // Return string count
}
