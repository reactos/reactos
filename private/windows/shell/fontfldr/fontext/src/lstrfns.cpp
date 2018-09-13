///////////////////////////////////////////////////////////////////////////////
//
// lstrfns.cpp
//      Explorer Font Folder extension routines
//
//
// History:
//      31 May 95 SteveCat
//          Ported to Windows NT and Unicode, cleaned up
//
//
// NOTE/BUGS
//
//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
//
// Copyright © 1993, 1994  Microsoft Corporation.  All Rights Reserved.
//
//    MODULE:        lstrfns.c
//
//    PURPOSE:    Specialized string manipulation functions
//
//    PLATFORMS:    Chicago
//
//    FUNCTIONS:
//
//    Private Functions: these are near calls
//      ChrCmp      - Case sensitive character comparison for DBCS
//      ChrCmpI     - Case insensitive character comparison for DBCS
//      StrEndN     - Find the end of a string, but no more than n bytes
//      ReverseScan - Find last occurrence of a byte in a string
//
//    Public functions: these will be near calls if compiled small
//    model, far calls otherwise.
//      StrChr     - Find first occurrence of character in string
//      StrChrI    - Find first occurrence of character in string, case insensitive
//      StrRChr    - Find last occurrence of character in string
//      StrRChrI   - Find last occurrence of character in string, case insensitive
//      StrNCmp    - Compare n characters
//      StrNCmpI   - Compare n characters, case insensitive
//      StrNCpy    - Copy n characters
//      StrCmpN    - Compare n bytes
//      StrCmpNI   - Compare n bytes, case insensitive
//      StrCpyN    - Copy up to n bytes, don't end in LeadByte for DB char
//      StrStr     - Search for substring
//      StrStrI    - Search for substring case insensitive
//      StrRStr    - Reverse search for substring
//      StrRStrI   - Reverse search for substring case insensitive
//
//    SPECIAL INSTRUCTIONS: N/A
//
//***************************************************************************
// $lgb$
// 1.0     7-Mar-94 eric Initial revision.
// $lge$
//*************************************************************************** 
//
//  Copyright (C) 1992-1995 Microsoft Corporation
//
///////////////////////////////////////////////////////////////////////////////

//==========================================================================
//                              Include files
//==========================================================================

#include "priv.h"

// #include "incall.h"

#define ALL
//
//  Define STATIC in the makefile to get these two symbols
//

#ifndef STATIC
#define STATIC static
#endif

//
//  Define DBCS if you want the actual functions called
//

#ifndef DBCS
#define DBCS
#endif

#ifdef DBCS

#define FASTCALL PASCAL

#else

#define FASTCALL _fastcall
#define IsDBCSLeadByte(x) ((x), FALSE)

#endif

//
//  Use all case sensitive functions; define INSENS also to get all fns
//

#ifdef ALL

#define CHR
#define CMP
#define CPY
#define STR
#define NCHARS

#endif

#include "lstrfns.h"

//
//  This is so we can compare offsets if we know the segments are equal
//

//#define OFFSET(x) (LOWORD((DWORD)(x)))


//
//  I'm cheating here to make some functions a little faster;
//  we won't have to push a word on the stack every time
//

STATIC WORD gwMatch;


#if defined(CHR) || defined(STR)
// 
//  ChrCmp -  Case sensitive character comparison for DBCS
//  Assumes   w1, gwMatch are characters to be compared
//  Return    FALSE if they match, TRUE if no match
//

STATIC BOOL NEAR PASCAL ChrCmp( WORD w1 )
{
    //
    //  Most of the time this won't match, so test it first for speed.
    //

    if( LOBYTE( w1 ) == LOBYTE( gwMatch ) )
    {
        if( IsDBCSLeadByte( LOBYTE( w1 ) ) )
        {
            return( w1 != gwMatch );
        }
        return FALSE;
    }
    return TRUE;
}


#if defined(INSENS)

// 
//  ChrCmpI - Case insensitive character comparison for DBCS
//  Assumes   w1, gwMatch are characters to be compared;
//            HIBYTE of gwMatch is 0 if not a DBC
//  Return    FALSE if match, TRUE if not
//

STATIC BOOL NEAR PASCAL ChrCmpI( WORD w1 )
{
    TCHAR sz1[ 3 ], sz2[ 3 ];

    if( IsDBCSLeadByte( sz1[ 0 ] = LOBYTE( w1 ) ) )
    {
        sz1[ 1 ] = HIBYTE( w1 );
        sz1[ 2 ] = TEXT( '\0' );
    }
    else
        sz1[ 1 ] = TEXT( '\0' );

    *(WORD *)sz2 = gwMatch;

    sz2[ 2 ] = TEXT( '\0' );

    return( lstrcmpi( sz1, sz2 ) );
}
#endif

#endif


#if defined(CMP) || defined(CPY) || defined(STR)

// 
//  StrEndN - Find the end of a string, but no more than n bytes
//  Assumes   lpStart points to start of null terminated string
//            nBufSize is the maximum length
//  returns ptr to just after the last byte to be included
//

STATIC LPTSTR NEAR PASCAL StrEndN( LPTSTR lpStart, UINT nBufSize )
{
    LPTSTR lpEnd;

    for( lpEnd = lpStart + nBufSize;
         *lpStart && OFFSET( lpStart ) < OFFSET( lpEnd );
        lpStart = CharNext( lpStart ) )
    {
        //
        //  just getting to the end of the string
        //

        continue;
    }

    if( OFFSET( lpStart ) > OFFSET( lpEnd ) )
    {
        //
        // We can only get here if the last byte before lpEnd was a lead byte
        //

        lpStart -= 2;
    }

    return( lpStart );
}

#endif


#if defined(STR)

// 
//  ReverseScan - Find last occurrence of a byte in a string
//  Assumes   lpSource points to first byte to check (end of the string)
//            wLen is the number of bytes to check
//            bMatch is the byte to match
//  returns ptr to the last occurrence of ch in str, NULL if not found.
//

STATIC LPTSTR NEAR PASCAL ReverseScan( LPTSTR lpSource, UINT wLen, TCHAR bMatch )
{

    UINT    cbT     = lstrlen( lpSource );
    LPTSTR  lpszRet = NULL;

    if( cbT )
    {
        cbT = min( cbT, wLen );

        while( cbT-- && !lpszRet )
        {
            if( *lpSource == bMatch )
            {
                lpszRet = lpSource;
            }
            else
            {
                lpSource--;
            }
        }
    }

    return lpszRet;
}

#endif


#if defined(CHR) || defined(STR)

// 
//  StrChr - Find first occurrence of character in string
//  Assumes   lpStart points to start of null terminated string
//            wMatch  is the character to match
//  returns ptr to the first occurrence of ch in str, NULL if not found.
//

LPTSTR PASCAL StrChr( LPTSTR lpStart, WORD wMatch )
{
    gwMatch = wMatch;

    for(  ; *lpStart; lpStart = CharNext( lpStart ) )
    {
        if( !ChrCmp( *(LPWORD)lpStart ) )
            return( lpStart );
    }

    return( NULL );
}


//
//  StrRChr - Find last occurrence of character in string
//  Assumes   lpStart points to start of string
//            lpEnd   points to end of string (NOT included in search)
//            wMatch  is the character to match
//  returns ptr to the last occurrence of ch in str, NULL if not found.
//

LPTSTR PASCAL StrRChr( LPTSTR lpStart, LPTSTR lpEnd, WORD wMatch )
{
    LPTSTR lpFound = NULL;

    if( !lpEnd )
        lpEnd = lpStart + lstrlen( lpStart );

    gwMatch = wMatch;

    for(  ; OFFSET( lpStart ) < OFFSET( lpEnd ); lpStart = CharNext( lpStart ) )
    {
        if( !ChrCmp( *(LPWORD)lpStart ) )
            lpFound = lpStart;
    }

    return( lpFound );
}


#if defined(INSENS)

// 
//  StrChrI - Find first occurrence of character in string, case insensitive
//  Assumes   lpStart points to start of null terminated string
//            wMatch  is the character to match
//  returns ptr to the first occurrence of ch in str, NULL if not found.
//

LPTSTR PASCAL StrChrI( LPTSTR lpStart, WORD wMatch )
{
    gwMatch = IsDBCSLeadByte( LOBYTE( wMatch ) ) ? wMatch : LOBYTE( wMatch );

    for(  ; *lpStart; lpStart = CharNext( lpStart ) )
    {
        if( !ChrCmpI( *(LPWORD)lpStart ) )
            return( lpStart );
    }

    return( NULL );
}


// 
//  StrRChrI - Find last occurrence of character in string, case insensitive
//  Assumes   lpStart points to start of string
//            lpEnd   points to end of string (NOT included in search)
//            wMatch  is the character to match
//  returns ptr to the last occurrence of ch in str, NULL if not found.
//

LPTSTR PASCAL StrRChrI( LPTSTR lpStart, LPTSTR lpEnd, WORD wMatch )
{
    LPTSTR lpFound = NULL;

    if( !lpEnd )
        lpEnd = lpStart + lstrlen( lpStart );

    gwMatch = IsDBCSLeadByte( LOBYTE( wMatch ) ) ? wMatch : LOBYTE( wMatch );

    for(  ; OFFSET( lpStart ) < OFFSET( lpEnd ); lpStart = CharNext( lpStart ) )
    {
        if( !ChrCmpI( *(LPWORD) lpStart ) )
            lpFound = lpStart;
    }

    return( lpFound );
}

#endif

#endif


#if defined(CMP)

//
//  StrCmpN      - Compare n bytes
//
//  returns   See lstrcmp return values.
//

short PASCAL StrCmpN( LPTSTR lpStr1, LPTSTR lpStr2, short nChar )
{
    TCHAR  cHold1, cHold2;
    short  i;
    LPTSTR lpEnd1, lpEnd2;
    
    cHold1 = *(lpEnd1 = StrEndN( lpStr1, nChar ) );

    cHold2 = *(lpEnd2 = StrEndN( lpStr2, nChar ) );

    *lpEnd1 = *lpEnd2 = TEXT( '\0' );

    i = lstrcmp( lpStr1, lpStr2 );

    *lpEnd1 = cHold1;

    *lpEnd2 = cHold2;

    return( i );
}


#if defined(INSENS)

//
//  StrCmpNI     - Compare n bytes, case insensitive
//
//  returns   See lstrcmpi return values.
//

short PASCAL StrCmpNI( LPTSTR lpStr1, LPTSTR lpStr2, short nChar )
{
    TCHAR  cHold1, cHold2;
    short  i;
    LPTSTR lpEnd1, lpEnd2;
    
    cHold1 = *(lpEnd1 = StrEndN( lpStr1, nChar ) );

    cHold2 = *(lpEnd2 = StrEndN( lpStr2, nChar ) );

    *lpEnd1 = *lpEnd2 = TEXT( '\0' );

    i = lstrcmpi( lpStr1, lpStr2 );

    *lpEnd1 = cHold1;

    *lpEnd2 = cHold2;

    return( i );
}

#endif

#endif


#if defined(CPY)

//
//  StrCpyN      - Copy up to N bytes, don't end in LeadByte for DB char
//
//  Assumes   lpDest points to buffer of nBufSize bytes (including NULL)
//            lpSource points to string to be copied.
//  returns   Number of bytes copied, NOT including NULL
//

UINT PASCAL StrCpyN( LPTSTR lpDest, LPTSTR lpSource, short nBufSize )
{
    LPTSTR lpEnd;
    TCHAR cHold;

    if( nBufSize < 0 )
        return( nBufSize );

    lpEnd = StrEndN( lpSource, nBufSize );

    cHold = *lpEnd;

    *lpEnd = TEXT( '\0' );

    lstrcpy( lpDest, lpSource );

    *lpEnd = cHold;

    return( lpEnd - lpSource );
}

#endif


#if defined(NCHARS)

#if defined(CMP)

#if 0    // Not used

//
//  StrNCmp      - Compare n characters
//
//  returns   See lstrcmp return values.
//

short PASCAL StrNCmp( LPTSTR lpStr1, LPTSTR lpStr2, short nChar )
{
    TCHAR  cHold1, cHold2;
    short  i;
    LPTSTR lpsz1 = lpStr1, lpsz2 = lpStr2;
    
    for( i = 0; i < nChar; i++ )
    {
        //
        //  If we hit the end of either string before the given number
        //  of bytes, just return the comparison
        //

        if( !*lpsz1 || !*lpsz2 )
            return( lstrcmp( lpStr1, lpStr2 ) );

        lpsz1 = CharNext( lpsz1 );
        lpsz2 = CharNext( lpsz2 );
    }

    cHold1 = *lpsz1;
    cHold2 = *lpsz2;

    *lpsz1 = *lpsz2 = TEXT( '\0' );

    i = lstrcmp( lpStr1, lpStr2 );

    *lpsz1 = cHold1;
    *lpsz2 = cHold2;

    return( i );
}

#endif    // Not used


#if defined(INSENS)

//
//  StrNCmpI     - Compare n characters, case insensitive
//
//  returns   See lstrcmpi return values.
//

short PASCAL StrNCmpI( LPTSTR lpStr1, LPTSTR lpStr2, short nChar )
{
    TCHAR  cHold1, cHold2;
    short  i;
    LPTSTR lpsz1 = lpStr1, lpsz2 = lpStr2;

    for( i = 0; i < nChar; i++ )
    {
        //
        //  If we hit the end of either string before the given number
        //  of bytes, just return the comparison
        //

        if( !*lpsz1 || !*lpsz2 )
            return( lstrcmpi( lpStr1, lpStr2 ) );

        lpsz1 = CharNext( lpsz1 );
        lpsz2 = CharNext( lpsz2 );
    }

    cHold1 = *lpsz1;
    cHold2 = *lpsz2;

    *lpsz1 = *lpsz2 = TEXT( '\0' );

    i = lstrcmpi( lpStr1, lpStr2 );

    *lpsz1 = cHold1;
    *lpsz2 = cHold2;

    return( i );
}
#endif
#endif


#if defined(CPY)

#if 0    // #defined as lstrcpyn

//
//  StrNCpy      - Copy n characters
//
//  returns   Actual number of characters copied
//

short PASCAL StrNCpy( LPTSTR lpDest, LPTSTR lpSource, short nChar )
{
    TCHAR  cHold;
    short  i;
    LPTSTR lpch = lpSource;
    
    if( nChar < 0 )
        return( nChar );
    
    for( i = 0; i < nChar; i++ )
    {
        if( !*lpch )
            break;

        lpch = CharNext( lpch );
    }

    cHold = *lpch;

    *lpch = TEXT( '\0' );

    lstrcpy( lpDest, lpSource );

    *lpch = cHold;

    return( i );
}

#endif    // #defined as lstrcpyn

#endif

#endif


#if defined(STR)

//
//  StrStr      - Search for first occurrence of a substring
//
//  Assumes   lpSource points to source string
//            lpSrch points to string to search for
//  returns   first occurrence of string if successful; NULL otherwise
//

LPTSTR PASCAL StrStr( LPTSTR lpFirst, LPTSTR lpSrch )
{
    WORD wLen;
    WORD wMatch;
    
    wLen = lstrlen( lpSrch );

    wMatch = *(LPWORD)lpSrch;
    
    for(  ; (lpFirst = StrChr( lpFirst, wMatch ) )
              && StrCmpN( lpFirst, lpSrch, wLen );
              lpFirst = CharNext( lpFirst ) )
    {
        //
        //  continue until we hit the end of the string or get a match
        //
        continue;
    }
    
    return( lpFirst );
}


//
//  StrRStr      - Search for last occurrence of a substring
//
//  Assumes   lpSource points to the null terminated source string
//            lpLast points to where to search from in the source string
//            lpLast is not included in the search
//            lpSrch points to string to search for
//  returns   last occurrence of string if successful; NULL otherwise
//

LPTSTR PASCAL StrRStr( LPTSTR lpSource, LPTSTR lpLast, LPTSTR lpSrch )
{
    WORD  wLen;
    TCHAR bMatch;
    
    wLen = lstrlen( lpSrch );

    bMatch = *lpSrch;
    
    if( !lpLast )
        lpLast = lpSource + lstrlen( lpSource );
    
    do
    {
        //
        //  Return NULL if we hit the exact beginning of the string
        //

        if( lpLast == lpSource )
            return( NULL );
    
        --lpLast;
    
        //
        // Break if we hit the beginning of the string
        //

        if( !(lpLast = ReverseScan( lpLast, OFFSET( lpLast )-OFFSET( lpSource )+1,
                                    bMatch ) ) )
            break;
    
        //
        //  Break if we found the string, and its first byte is not a tail byte
        //

        if( !StrCmpN( lpLast, lpSrch, wLen ) &&
            (lpLast == StrEndN( lpSource, OFFSET( lpLast )-OFFSET( lpSource ) ) ) )
            break;

    } while( 1 ) ;
    
    return( lpLast );
}


#if defined(INSENS)

//
//  StrStrI   - Search for first occurrence of a substring, case insensitive
//
//  Assumes   lpFirst points to source string
//            lpSrch points to string to search for
//  returns   first occurrence of string if successful; NULL otherwise
//

LPTSTR PASCAL StrStrI( LPTSTR lpFirst, LPTSTR lpSrch )
{
    WORD wLen;
    WORD wMatch;

    wLen = lstrlen( lpSrch );

    wMatch = *(LPWORD)lpSrch;

    for(  ; (lpFirst = StrChrI( lpFirst, wMatch ) )
             && StrCmpNI( lpFirst, lpSrch, wLen );
            lpFirst = CharNext( lpFirst ) )
    {
        //
        //  continue until we hit the end of the string or get a match
        //

        continue;
    }

    return( lpFirst );
}


//
//  StrRStrI      - Search for last occurrence of a substring
//
//  Assumes   lpSource points to the null terminated source string
//            lpLast points to where to search from in the source string
//            lpLast is not included in the search
//            lpSrch points to string to search for
//  returns   last occurrence of string if successful; NULL otherwise
//

LPTSTR PASCAL StrRStrI( LPTSTR lpSource, LPTSTR lpLast, LPTSTR lpSrch )
{
    LPTSTR lpFound = NULL, lpEnd;
    TCHAR cHold;


    if( !lpLast )
        lpLast = lpSource + lstrlen( lpSource );

    if( lpSource >= lpLast || *lpSrch == 0 )
        return NULL;

    lpEnd = StrEndN( lpLast, lstrlen( lpSrch )-1 );

    cHold = *lpEnd;

    *lpEnd = 0;

    while( ( lpSource = StrStrI( lpSource, lpSrch ) ) &&
            OFFSET( lpSource ) < OFFSET( lpLast ) )
    {
        lpFound = lpSource;
        lpSource = CharNext( lpSource );
    }

    *lpEnd = cHold;

    return lpFound;
}
#endif

#endif
