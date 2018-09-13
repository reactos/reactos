//
//
// BUGBUG!!!!
//
//  This file makes several calls to StrCmpN functions in comctl32.
//  The StrCmpN functions are broken!  They do not return the correct
//  international ordering for strings because they do a character by
//  character comparison of the two strings.  Luckily, the functions
//  in this file are just building a sorted search tree and appear to
//  always use the same functions for building and searching; therefore,
//  this code should work as is.
//
//

// BUGBUG: ccover chokes on this , do we need this anyways
#ifndef CCOVER
extern INT StrCmpNIA(LPSTR lptr1, LPSTR lpWStr2, INT nChar);
extern INT StrCmpNA(LPCSTR lptr1, LPCSTR lpWStr2, INT nChar);
#endif

/**********************************************************************
//  Grepstrs
//
//      Copyright (c) 1992 - Microsoft Corp.
//      All rights reserved.
//      Microsoft Confidential
//
//
//  String search functions which duplciate those in QSUB.ASM.
//
**********************************************************************/

#include "shellprv.h"
#pragma  hdrstop

#pragma warning( disable:4001 )         // Disable new type remark warning

// *********************************************************************
    // String table

static int toklen[] =                       // Table of token lengths
{
    32767,                                      // T_END: invalid
    32767,                                      // T_STRING: invalid
    2,                                          // T_SINGLE
    ASCII_LEN / 8+1,                            // T_CLASS
    1,                                          // T_ANY
    32767                                       // T_STAR: invalid
};

// *********************************************************************
//  Local function prototypes
// *********************************************************************

LPSTR findsub( LPGREPINFO, LPSTR , LPSTR );         // Findlist() worker
LPSTR findsubi( LPGREPINFO, LPSTR , LPSTR );        // Findlist() worker

LPSTR (*flworker[])( LPGREPINFO, LPSTR , LPSTR ) = {    findsubi, findsub };


// *********************************************************************
//  void matchstrings( LPSTR s1, LPSTR s2, int len, int *nmatched, int *leg)
//
//  ARGUMENTS:
//      s1          - First string
//      s2          - Second string
//      len     - Length
//      nmatched    - Number of bytes matched
//      leg     - Less than, equal, greater than
// *********************************************************************

void matchstrings( LPGREPINFO lpgi, LPSTR s1, LPSTR s2, int len, int *nmatched, int *leg)
{
    register LPSTR  cp;                            // Char pointer
                                                        // Comparison function pointer
    register int (*cmp)( LPCSTR , LPCSTR , int);

    cmp = (lpgi->CaseSen ? StrCmpNA : StrCmpNIA);   // Set pointer
                                                    // If strings don't match
    if ( (*leg = (*cmp)(s1, s2, (unsigned)len)) != 0 )
    {
                                                        // Find mismatch
        for ( cp = s1; (*cmp)( cp, s2++, 1 ) == 0; cp++ )
            ;

        *nmatched = (int) (cp - s1);                    // Return number matched
    }
    else
        *nmatched = len;                            // Else all matched
}


// *********************************************************************
//  int preveol( LPSTR s )
//
//  ARGUMENTS:
//      s           - String to search
// *********************************************************************

int preveol( LPSTR s )
{
    register LPSTR  cp;                        // Char pointer

    cp = s + 1;                                 // Initialize pointer
    while ( *(--cp) != '\n' )               // Find previous end-of-line
        ;

    return( (int) (s - cp) );                     // Return distance to match
}

// *********************************************************************
//  int countlines( register LPSTR start, LPSTR finish )
//
//  ARGUMENTS:
//      start           - Start of buffer
//      finish          - End of buffer
// *********************************************************************

int countlines( register LPSTR start, LPSTR finish )
{
    register int    count;                  // Line count

    for ( count = 0; start < finish; )
    {                                               // Loop to count lines
        if ( *(start++) == '\n' )
            count++;                                // Increment count if linefeed found
    }
    return( count );                            // Return count
}

// *********************************************************************
//  Scans a buffer for the first character which does not match
// one of the characters in a specified list.
//
//  int strnspn( LPSTR s, LPSTR t, int n)
//
//  ARGUMENTS:
//      s           - String to search
//      t           - Target list
//      n           - Length of s
// *********************************************************************

int strnspn( LPSTR s, LPSTR t, int n)
{
    LPSTR       s1;                                    // String pointer
    LPSTR       t1;                                    // String pointer

    for ( s1 = s; n-- != 0; s1++ )          // While not at end of s
    {
        for ( t1 = t; *t1 != '\0'; t1++ )   // While not at end of t
        {
            if ( *s1 == *t1 )
                break;                              // Break if match found
        }
        if ( *t1 == '\0' )
            break;                                  // Break if no match found
    }
    return( (int) (s1 - s) );                               // Return length
}

// *********************************************************************
//  Scans a buffer for the first character which matches a character
// from a specified list of characters
//
//  int strncspn( LPSTR s, LPSTR t, int n )
//
//  ARGUMENTS:
//      s           - String to search
//      t           - Target list
//      n           - Length of s
// *********************************************************************

int strncspn( LPSTR s, LPSTR t, int n )
{
    LPSTR   s1;                                        // String pointer
    LPSTR   t1;                                        // String pointer

    for ( s1 = s; n-- != 0; s1++ )          // While not at end of s
    {
        for ( t1 = t; *t1 != '\0'; t1++ )   // While not at end of t
        {
            if ( *s1 == *t1 )
                return( (int) (s1 - s) );                   // Return if match found
        }
    }
    return( (int) (s1 - s) );                               // Return length
}


// *************************************************************************

// *********************************************************************
//
//  This is an implementation of the QuickSearch algorith described
//  by Daniel M. Sunday in the August 1990 issue of CACM.  The TD1
//  table is computed before this routine is called.
//
//  LPSTR findone( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend )
//
//  ARGUMENTS:
//      buffer          - Buffer in which to search
//      bufend          - End of buffer
// *********************************************************************

LPSTR findone( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend )
{
    if ( (bufend -= lpgi->ge.TargetLen - 1) <= buffer )
        return( (LPSTR )0 );                    // Fail if buffer too small

    while ( buffer < bufend )
    {                                                   // While space remains
        int             cch;                        // Character count
        register LPSTR  pch1;                  // Char pointer
        register LPSTR  pch2;                  // Char pointer

        pch1 = lpgi->Target;                          // Point at pattern
        pch2 = buffer;                          // Point at buffer
                                                        // Loop to try match
        for ( cch = lpgi->ge.TargetLen; cch > 0; cch-- )
        {
            if ( *(pch1++) != *(pch2++) )
                break;                              // Exit loop on mismatch
        }
        if ( cch == 0 )
            return( buffer );                       // Return pointer to match

        buffer += lpgi->ge.td1[ (unsigned char)(buffer[ lpgi->ge.TargetLen ]) ];  // Skip ahead

    }
    return( NULL );                                 // No match
}

// *********************************************************************
//  LPSTR findlist( LPSTR buffer, LPSTR bufend )
//
//  ARGUMENTS:
//      buffer          - Buffer to search
//      bufend          - End of buffer
// *********************************************************************

LPSTR findlist( LPGREPINFO lpgi, LPSTR buffer, LPSTR bufend )
{
    LPSTR   szMatch;                               // Pointer to matching string
    char    endbyte;                                    // First byte past end

    endbyte = *bufend;                          // Save byte
    *bufend = '\177';                           // Mark end of buffer

                                                        // Call worker
    szMatch = (*flworker[ lpgi->CaseSen ])( lpgi, buffer, bufend );
    *bufend = endbyte;                          // Restore end of buffer
    return( szMatch );                          // Return matching string
}

// *********************************************************************
//  LPSTR findsub( register LPSTR buffer, LPSTR bufend)
//
//  ARGUMENTS:
//      buffer          - Pointer to buffer
//      bufend          - End of buffer
// *********************************************************************

LPSTR findsub( LPGREPINFO lpgi, register LPSTR buffer, LPSTR bufend)
{
    register LPSTR  cp;                            // Char pointer
    STRINGNODE  *s;                             // String node pointer
    int             i;                              // Index

    if ( lpgi->ge.ShortStrLen == -1 ||
          (bufend - buffer) < lpgi->ge.ShortStrLen )
        return( NULL );

    bufend -= (lpgi->ge.ShortStrLen - 1);                // Compute effective buffer length

    while( buffer < bufend )                    // Loop to find match
    {
        if ( (i = lpgi->ge.TransTable[ (unsigned char)*buffer ]) != 0 )
        {                                           // If valid first character
            if ( (s = lpgi->ge.StringList[ i ]) == NULL )
                return (buffer);                    // Check for 1-byte match
            for ( cp = buffer + 1;; )               // Loop to search list
            {
                if ( (i = _fmemcmp( cp, s_text( s ), (unsigned)s->s_must)) == 0 )
                {                                   // If portions match
                    cp += s->s_must;                // Skip matching portion
                    if ( (s = s->s_suf) == NULL )
                        return( buffer );           // Return match if end of list
                    continue;                       // Else continue
                }
                if ( i < 0 || (s = s->s_alt) == NULL )
                    break;                          // Break if not in this list
            }
        }
                                                        // Shift as much as possible
        buffer += lpgi->ge.td1[ (unsigned char)(buffer[ lpgi->ge.ShortStrLen ]) ];
    }
    return( NULL );                                 // No match
}

// *********************************************************************
//  LPSTR findsubi( LPGREPINFO lpgi, register LPSTR buffer, LPSTR bufend)
//
//  ARGUMENTS:
//      buffer          - Pointer to buffer
//      bufend          - End of buffer
// *********************************************************************

LPSTR findsubi( LPGREPINFO lpgi, register LPSTR buffer, LPSTR bufend)
{
    register LPSTR  cp;                            // Char pointer
    STRINGNODE  *s;                             // String node pointer
    int             i;                              // Index
    int             iTail;

//
// We really need IsDBCSLeadByte API here.
//
#ifdef IsDBCSLeadByte
#undef IsDBCSLeadByte
#endif

    if ( lpgi->ge.ShortStrLen == -1 ||
          (bufend - buffer) < lpgi->ge.ShortStrLen )
        return( NULL );

    bufend -= (lpgi->ge.ShortStrLen - 1);                // Compute effective buffer length

    while ( buffer < bufend )                   // Loop to find match
    {
        if ( (i = lpgi->ge.TransTable[ (unsigned char)*buffer ]) != 0 )
        {                                               // If valid first character
            if ( (s = lpgi->ge.StringList[ i ]) == NULL )
                return( buffer );                   // Check for 1-byte match
            for ( cp = buffer + 1;; )           // Loop to search list
            {
                iTail = 0;
                if ( IsDBCSLeadByte( *buffer ) )
                {
                    if ( *cp == *s_text( s ) )
                        iTail = 1;
                    else
                        break;
                }
                if ( s->s_must - iTail == 0 ||
                   (i = StrCmpNIA( cp + iTail, s_text( s ) + iTail, (unsigned)s->s_must - iTail)) == 0 )
                {                                       // If portions match
                    cp += s->s_must;                // Skip matching portion

                    if ( (s = s->s_suf) == NULL )
                        return( buffer );           // Return match if end of list
                    continue;                   // And continue
                }
                if ( i < 0 || (s = s->s_alt) == 0 )
                    break;                          // Break if not in this list
            }
        }
                                                        // Shift as much as possible
        buffer += lpgi->ge.td1[ (unsigned char)(buffer[ lpgi->ge.ShortStrLen ]) ];
    }
    return( NULL );                                 // No match
}



// *********************************************************************
//  LPSTR findexpr( register LPSTR buffer, LPSTR bufend )
//
//  ARGUMENTS:
//      buffer      - Buffer in which to search
//      bufend      - End of buffer
// *********************************************************************

LPSTR findexpr( LPGREPINFO lpgi, register LPSTR buffer, LPSTR bufend )
{
    register EXPR  *expr;                      // Expression list pointer
    register LPSTR  pattern;                   // Pattern
    int             i;                              // Index

    while ( buffer < bufend )                   // Loop to find match
    {
        if ( (i = lpgi->ge.TransTable[ (unsigned char)*(buffer++) ]) == 0 )
            continue;                               // Continue if not valid 1st char

        expr = (EXPR *)((LPSTR )lpgi->ge.StringList[ i ] );
        #ifndef NDEBUG
            ASSERT( expr != NULL );
        #endif
        buffer--;                                   // Back up to first character

        while ( expr != NULL )                  // Loop to find match
        {
            pattern = expr->ex_pattern;     // Point to pattern
            expr = expr->ex_next;               // Point to next record
            if (pattern[ 0 ] == '^')            // If match begin line
            {
                pattern++;                          // Skip caret
                if ( buffer[ -1 ] != '\n' )
                    continue;                       // Don't bother if not at beginning

            }
            if ( exprmatch(lpgi, buffer, pattern) )   // Return pointer if match found
                return( buffer );
        }
        buffer++;                                   // Skip first character
    }
    return( NULL );                                 // No match
}

// *********************************************************************
//  int ncmpi( const LPSTR sany,
//                const LPSTR supper,
//                 int n )
//
//  ARGUMENTS:
//      sany        - String whose case is unknown
//      supper  - Upper case string
//      n           - Number of characters to compare
// *********************************************************************

int ncmpi( LPCTSTR sany, LPCTSTR supper, unsigned n )
{
    int     i;                                      // Difference

    while ( n-- > 0 )                               // While not at end of strings
    {
        if ( (i = LOWORD((DWORD_PTR)CharUpperA( (LPSTR)(DWORD_PTR)(DWORD)*sany ) - *(supper++) ))
                != 0 )
            return( i );                            // Return difference if mismatch

        if ( *sany++ == '\0' )
            break;                                  // Break if end of string reached
    }
    return( 0 );                                    // Strings match
}

// *********************************************************************
//  LPSTR simpleprefix( LPGREPINFO lpgi, register LPSTR s, LPSTR *pp )
//
//  ARGUMENTS:
//      s       - String pointer
//      pp      - Pointer to pattern pointer
// *********************************************************************

LPSTR simpleprefix( LPGREPINFO lpgi, register LPSTR s, LPSTR *pp )
{
    register LPSTR  p;                         // Simple pattern pointer
    register int    c;                              // Single character

    p = *pp;                                        // Initialize
    while ( *p != T_END && *p != T_STAR )   // While not at end of pattern
    {
        switch( *(p++) )                            // Switch on token type
        {
            case T_STRING:                          // String to compare
                if ( (*lpgi->ncmp)( s, p + 1, (unsigned)((int)*p) ) != 0 )
                    return( NULL );             // Fail if mismatch found

                s += *p;                            // Skip matched portion
                p += *p + 1;                        // Skip to next token
                break;

            case T_SINGLE:                          // Single character
                c = *s++;                           // Get character
                if ( !lpgi->CaseSen )
                    c = LOWORD((DWORD_PTR)CharUpperA( (LPSTR)(DWORD_PTR)(DWORD)c )); // Map to upper case if necessary

                if ( c != *p++ )
                    return( NULL );             // Fail if mismatch found

                break;

            case T_CLASS:                           // Class of characters
                if ( !(p[ *((LPBYTE )s) >> 3] & ( 1 << (*((LPBYTE )s) &7))))
                    return( NULL );             // Failure if bit not set

                p += ASCII_LEN / 8;             // Skip bit vector
                s++;                                    // Skip character
                break;

            case T_ANY:                             // Any character
                if ( *s++ == EOS )
                    return( NULL );             // Match all but end of string
                break;

            default:
                break;
        }
    }
    *pp = p;                                        // Update pointer
    return( s );                                    // Pattern is prefix of s
}

// *********************************************************************
//  int exprmatch( LPGREPINFO lpgi, LPSTR s, LPSTR p)
//
//  ARGUMENTS:
//      s       - String
//      p       - Pattern
// *********************************************************************

int exprmatch( LPGREPINFO lpgi, LPSTR s, LPSTR p )
{
    lpgi->ncmp = StrCmpNA;                             // Assume case-sensitive
    if( !lpgi->CaseSen )
        lpgi->ncmp = StrCmpNIA;                        // Be case-insensitive if flag set

    return( match( lpgi, s, p ) );                    // See if pattern matches string
}

// *********************************************************************
//
//
//  int match( register LPSTR pStr, LPSTR pPattern )
//
//  ARGUMENTS:
//      pStr            - String to match
//      pPattern        - Pattern to match against
//
// *********************************************************************

int match( LPGREPINFO lpgi, register LPSTR pStr, LPSTR pPattern )
{
    register LPSTR  pTmp1;                         // Temporary pointer
    LPSTR           pTmp2;                         // Temporary pointer
    register int    iChar;                      // Character

    if ( *pPattern != T_END && *pPattern != T_STAR &&
          (pStr = simpleprefix( lpgi, pStr, &pPattern )) == NULL )
        return ( 0 );                               // Failure if prefix mismatch

    if ( *pPattern++ == T_END )
        return( 1 );                                // Match if end of pattern

    pTmp1 = pTmp2 = pPattern;                   // Point to repeated token
    pTmp2 += toklen[ *((LPBYTE )pTmp1) ];   // Skip repeated token

    switch( *pTmp1++ )                          // Switch on token type
    {
        case T_ANY:                                 // Any character
                                                        // While match not found
            while ( exprmatch( lpgi, pStr, pTmp2 ) == 0 )
                if ( *pStr++ == EOS)
                    return( 0 );                    // Match all but end of string
            return( 1);                             // Success

        case T_SINGLE:                              // Single character
            while ( exprmatch( lpgi, pStr, pTmp2 ) == 0 ) // While match not found
            {
                iChar = *pStr++;                    // Get character
                if ( !lpgi->CaseSen )
                    iChar = LOWORD((DWORD_PTR)CharUpperA( (LPSTR)(DWORD_PTR)(DWORD)iChar ));   // Map to upper case if necessary

                if ( (char) iChar != *pTmp1 )
                    return( 0);                     // Fail if mismatch found

            }
            return( 1 );                            // Success

        case T_CLASS:                               // Class of characters
            while ( exprmatch( lpgi, pStr, pTmp2 ) == 0 ) // While match not found
            {
                if (!(pTmp1[ *((LPBYTE )pStr)>>3 ] &
                        (1 << (*((LPBYTE )pStr) & 7))) )
                    return( 0 );                    // Fail if bit not set
                pStr++;                                 // Else skip character
            }
            return( 1 );                            // Success

        default:
            break;
    }
    return( 0 );                                    // Return failure
}
