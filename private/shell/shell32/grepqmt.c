/**********************************************************************
//  QMT.C
//
//      Copyright (c) 1992 - Microsoft Corp.
//      All rights reserved.
//      Microsoft Confidential
//
//
//  Functions for handling reqular expressions.
//
**********************************************************************/
#include "shellprv.h"
#pragma  hdrstop

#pragma warning( disable:4001 )         // Disable new type remark warning

//*********************************************************************
//  void bitset( LPSTR BitVec, int First, int Last, int BitVal)
//
//  ARGUMENTS:
//      BitVec  - Bit vector
//      First       - First character
//      Last        - Last character
//      BitVal  - Bit value (0 or 1)
//*********************************************************************

void bitset( LPBYTE BitVec, int First, int Last, int BitVal)
{
    int     BitNo;                              // Bit number

    BitVec += ((unsigned)First) >> 3;       // Point at first byte
    BitNo = First & 7;                          // Calculate first bit number
    while ( First <= Last )                     // Loop to set bits
    {                                                   // If we have a whole byte's worth
        if ( BitNo == 0 && (First + 8) <= Last )
        {                                               // Set the bits
            *BitVec++ = (unsigned char)(BitVal ? 0xff : '\0');
            First += 8;                             // Increment the counter
            continue;                               // Next iteration
        }
                                                        // Set the appropriate bit
        *BitVec = (unsigned char)( ((unsigned)*BitVec & (~((unsigned)1 << BitNo)) |
                                 ((unsigned)BitVal << BitNo)) );

        if ( ++BitNo == 8 )                     // If we wrap into next byte
        {
            BitVec++;                               // Increment pointer
            BitNo = 0;                              // Reset bit index
        }
        First++;                                        // Increment bit index
    }
}

//*********************************************************************
//  Parses a search expression and creates a qualified expression string
//  for which a pointer is returned to the caller.
//
//  LPSTR exprparse( register LPSTR szPattern, int *NewBufLen )
//
//  ARGUMENTS:
//      szPattern   - Ptr to raw pattern
//      NewBufLen   - Ptr to len of new buffer returned by this function
//  RETURNS:
//      LPSTR       - Ptr to a newly allocated buffer containg the
//                        qualified expression and the caller's NewBufLen
//                        is updated to reflect the returned buffer length.
//                        Returns NULL ptr if an invalid expression.
//*********************************************************************

LPSTR exprparse( LPGREPINFO lpgi, LPSTR szPattern, int *NewBufLen )
{
    LPSTR           pChar;                          // Char pointer
    LPSTR           pChar2;                         // Char pointer
    LPSTR           lpChar;                         // Far Char pointer
    int             i;                              // Counter/index
    int             j;                              // Counter/index
    int             m;                              // Counter/index
    int             n;                              // Counter/index
    int             BitVal;                         // Bit value
    char            Buffer[ PATMAX ];               // Temporary buffer

    BitVal = 0;

    if ( !lpgi->CaseSen )                             // If case insensitive force to uprcase
        CharUpperBuffA( szPattern, (int)lstrlenA(szPattern) );

    pChar = Buffer;                             // Initialize pointer
    if ( *szPattern == '^' )
        *(pChar++) = *szPattern++;                  // Copy leading caret if any
    while ( *szPattern != '\0' )                // While not end of pattern
    {
        i = -2;                                     // Initialize

        for (n = 0;;)                               // Loop to delimit ordinary string
        {                                               // Look for a special character
            n += (int)StrCSpnA( szPattern + n, ".\\[*" );
            if ( szPattern[ n ] != '\\' )
                break;                              // Break if not backslash

            i = n;                                  // Remember where backslash is
            if ( szPattern[ ++n ] == '\0' )
                return( NULL );                 // Cannot be at very end
            n++;                                        // Skip escaped character
        }
        if ( szPattern[ n ] == '*' )            // If we found a *-expr.
        {
            if ( n-- == 0 )
                return( NULL );                 // Illegal first character
            if ( i == (n - 1) )
                n = i;                              // Escaped single-char. *-expr.
        }

        if ( n > 0 )                                // If we have string or single
        {
                                                        // If single character
            if ( n == 1 || (n == 2 && *szPattern == '\\') )
            {
                *pChar++ = T_SINGLE;            // Set type
                if ( *szPattern == '\\' )
                    szPattern++;                    // Skip escape if any
                *pChar++ = *szPattern++;        // Copy single character
            }
            else                                        // Else we have a string
            {
                *pChar++ = T_STRING;            // Set type
                pChar2 = pChar++;               // Save pointer to length byte
                while ( n-- > 0 )                   // While bytes to copy remain
                {
                    if (*szPattern == '\\')     // If escape found
                    {
                        szPattern++;                // Skip escape
                        n--;                            // Adjust length
                    }
                    *pChar++ = *szPattern++;        // Copy character
                }
                                                        // Set string length
                *pChar2 = (char) ((pChar - pChar2) - 1);
            }
        }
        if ( *szPattern == '\0' )
            break;                                  // Break if end of pattern

        if ( *szPattern == '.' )                // If matching any
        {
            if ( *++szPattern == '*' )      // If star follows any
            {
                szPattern++;                        // Skip star, too
                *pChar++ = T_STAR;              // Insert prefix ahead of token
            }
            *pChar++ = T_ANY;                   // Match any character
            continue;                               // Next iteration
        }

        if ( *szPattern == '[' )                // If character class
        {
            if (*++szPattern == '\0')           // Skip '['
                return(NULL);

            *pChar++ = T_CLASS;                 // Set type
                                                        // Clear the vector
            _fmemset( pChar, '\0', ASCII_LEN / 8 );
            BitVal = 1;                             // Assume we're setting bits

            if ( *szPattern == '^' )            // If inverted class
            {
                szPattern++;                        // Skip '^'
                                                        // Set all bits
                _fmemset( pChar, (char) -1, ASCII_LEN / 8 );
                bitset( (LPBYTE )pChar, EOS, EOS, 0 );  // All except end-of-string
                bitset( (LPBYTE )pChar, '\n', '\n', 0 );    // And linefeed!
                BitVal = 0;                             // Now we're clearing bits
            }

            while ( *szPattern != ']' )         // Loop to find ']'
            {
                if (*szPattern == '\0')
                    return( NULL );             // Check for malformed string */

                if ( *szPattern == '\\' )       // If escape found
                {
                    if ( *++szPattern == '\0' )// Skip escape
                        return(NULL);
                }

                i = (unsigned char)*szPattern++;        // Get first character in range

                                                        // If range found
                if ( *szPattern == '-' && szPattern[ 1 ] != '\0' &&
                      szPattern[ 1 ] != ']')
                {
                    szPattern++;                    // Skip hyphen
                    if ( *szPattern == '\\' && szPattern[ 1 ] != '\0' )
                        szPattern++;                // Skip escape character
                    j = (unsigned char)*szPattern++;    // Get end of range
                }
                else
                    j = i;                          // Else just one character

                                                        // Set bits in vector
                bitset( (LPBYTE )pChar, i, j, BitVal );

                if (!lpgi->CaseSen)                       // If ignoring case
                {
                    m = (i < 'A') ? 'A' : i;
                    // m = max(i,'A')

                    n = (j > 'Z') ? 'Z' : j;
                    // n = min(j,'Z')

                    if (m <= n)                     // Whack corresponding lower case
                        bitset( (LPBYTE )pChar,
                                LOWORD((DWORD_PTR)AnsiLower( (LPSTR)(DWORD_PTR)(DWORD)m )),
                                LOWORD((DWORD_PTR)AnsiLower( (LPSTR)(DWORD_PTR)(DWORD) n )),
                                BitVal );

                    m = (i < 'a') ? 'a' : i;
                    // m = max(i,'a')

                    n = (j > 'z') ? 'z' : j;
                    // n = min(j,'z')

                    if (m <= n)                     // Whack corresponding upper case
                        bitset( (LPBYTE )pChar,
                                LOWORD((DWORD_PTR)AnsiUpper((LPSTR)(DWORD_PTR)(DWORD)m )),
                                LOWORD((DWORD_PTR)AnsiUpper((LPSTR)(DWORD_PTR)(DWORD)n )),
                                BitVal );
                }
            }

            if (*(++szPattern) == '*')      // If repeated class
            {
                MoveMemory( pChar, pChar - 1, ASCII_LEN / 8 + 1 );

                                                        // Move vector forward 1 byte
                pChar[ -1 ] = T_STAR;           // Insert prefix
                pChar++;                                // Skip to start of vector
                szPattern++;                        // Skip star
            }
            pChar += ASCII_LEN / 8;             // Skip over vector
            continue;                               // Next iteration
        }

        *pChar++ = T_STAR;                      // Repeated single character
        *pChar++ = T_SINGLE;

        if ( *szPattern == '\\' )
            szPattern++;                            // Skip escape if any

        *pChar++ = *szPattern++;                    // Copy the character
        #ifndef NDEBUG
            ASSERT( *szPattern == '*' );            // Validate assumption
        #endif
        szPattern++;                                // Skip the star
    }

    *pChar++ = T_END;                           // Mark end of parsed expression

    n = (int) (pChar - (LPSTR)Buffer);                       // Determine new expression length
    *NewBufLen = n;                             // Update caller's ptr with buf len

    lpChar = AllocThrow( lpgi, (unsigned)n );          // Allocate buffer
    MoveMemory( lpChar, Buffer, (unsigned)n ); // Copy expression to buffer

    return( lpChar );                               // Return buffer pointer
}

//*********************************************************************
//int istoken( LPSTR pStr, int StrLen )
//
//  ARGUMENTS:
//      pStr        - String
//      StrLen      - Length
//*********************************************************************

int istoken( LPSTR pStr, int StrLen )
{
    if ( StrLen >= 2 && pStr[ 0 ] == '\\' && pStr[ 1 ] == '<' )
        return( 1 );                                // Token if starts with '\<'

    while ( StrLen-- > 0 )                      // Loop to find end of string
    {
        if ( *pStr++ == '\\' )                  // If escape found
        {
            if ( --StrLen == 0 && *pStr == '>' )
                return( 1 );                        // Token if ends with '\>'

            pStr++;                                     // Skip escaped character
        }
    }
    return( 0 );                                    // Not a token
}


//*********************************************************************
//  int isexpr( LPSTR pStr, int ExprLen )
//
//  ARGUMENTS:
//      pStr            - String
//      ExprLen     - Length
//*********************************************************************

int isexpr( LPGREPINFO lpgi, LPSTR pStr, int ExprLen )
{
    LPSTR       pChar;                             // Char pointer
    int     status;                                 // Return status
    char        Buffer[ BUFLEN ];               // Temporary buffer

    if ( istoken( pStr, ExprLen ) )
        return( 1 );                                // Tokens are exprs

                                                        // Copy string to buffer
    MoveMemory( Buffer, pStr, (unsigned)ExprLen );
    Buffer[ ExprLen ] = '\0';                   // Null-terminate string

    if ( (pStr = exprparse( lpgi, Buffer, &ExprLen )) == NULL )
        return( 0 );                                // Not an expression if parse fails

    status = 1;                                     // Assume we have an expression
    if ( *pStr != '^' && *pStr != T_END )   // If no caret and not empty
    {
        status = 0;                                 // Assume not an expression
        pChar = pStr;                                   // Initialize
        do                                          // Loop to find special tokens
        {
            switch ( *pChar++ )                     // Switch on token type
            {
                case T_STAR:                        // Repeat prefix
                case T_CLASS:                       // Character class
                case T_ANY :                        // Any character
                    status++;                       // This is an expression
                    break;

                case T_SINGLE:                      // Single character
                    pChar++;                            // Skip character
                    break;

                case T_STRING:                      // String
                    pChar += *pChar + 1;        // Skip string
                    break;

                default:
                    break;

            }
        }
        while ( !status && *pChar != T_END );   // Do while not at end of expr.
    }

    if ( !Free( pStr ) )                        // Free expression
        RaiseException(ERROR_INVALID_BLOCK, 0, 0, 0);

    return( status );                               // Return status
}

//*********************************************************************
//  LPSTR get1stcharset( LPSTR pExpr, LPBYTE BitVec )
//
//  ARGUMENTS:
//      pExpr       - Pointer to expression string
//      BitVec  - Pointer to bit vector
//*********************************************************************

LPSTR get1stcharset( LPGREPINFO lpgi, LPSTR pExpr, LPBYTE BitVec )
{
    LPSTR   pChar;                             // Char pointer
    int     i;                                 // Index/counter
    int     star;                              // Repeat prefix flag

    if ( *pExpr == '^' )
        pExpr++;                                        // Skip leading caret if any

    _fmemset( BitVec, '\0', ASCII_LEN / 8 );  // Clear bit vector
    pChar = pExpr;                                  // Initialize

    while ( *pExpr != T_END )                   // Loop to process leading *-expr.s
    {
        star = 0;                                   // Assume no repeat prefix
        if ( *pExpr == T_STAR )                 // If repeat prefix found
        {
            star++;                                     // Set flag
            pExpr++;                                // Skip repeat prefix
        }
        switch ( *pExpr++ )                     // Switch on token type
        {
            case T_END :                            // End of expression
            case T_STAR :                           // Repeat prefix
                RaiseException(ERROR_INVALID_PARAMETER, 0, 0, 0);
                // fall through

            case T_STRING :                     // String
                if ( star || *pExpr++ == '\0' ) // If repeat prefix or zero count
                    RaiseException(ERROR_INVALID_PARAMETER, 0, 0, 0);
                // fall through

            case T_SINGLE :                     // Single character
                                                        // Set the bit
                bitset( (LPBYTE )BitVec, (unsigned char)*pExpr, (unsigned char)*pExpr, 1 );
                pExpr++;                            // Skip the character
                break;

            case T_ANY :                            // Match any
                                                        // Set all the bits
                _fmemset( BitVec, (char)-1, ASCII_LEN / 8 );
                bitset( (LPBYTE )BitVec, EOS, EOS, 0 ); // Except end-of-string
                bitset( (LPBYTE )BitVec, '\n', '\n', 0 );   // And linefeed!
                break;

            case T_CLASS :
                for ( i = 0; i < ASCII_LEN / 8; i++ )
                    BitVec[ i ] |= (unsigned char)*(pExpr++);   // Or in all the bits
                break;

            default:
                break;
        }

        if ( !star )
            break;                                  // Break if not repeated
        pChar = pExpr;                              // Update pointer
    }
    return( pChar );                                // Point to 1st non-repeated expr.
}

//*********************************************************************
//  LPSTR findall( LPSTR Buffer, LPSTR bufend )
//
//  ARGUMENTS:
//      Buffer      - Buffer in which to search
//      bufend      - End of buffer
//*********************************************************************

LPSTR findall( LPGREPINFO lpgi, LPSTR Buffer, LPSTR bufend )
{
                                                    // Fail only on empty buffer
    return( ((LPSTR )Buffer < bufend) ? Buffer : NULL );
}

//*********************************************************************
//  void addtoken( LPSTR pExprStr, int ExprLen )
//
//  ARGUMENTS:
//      pExprStr        - Raw token expression
//      ExprLen     - Length of expression
//*********************************************************************

void addtoken( LPGREPINFO lpgi, LPSTR pExprStr, int ExprLen )
{
    static const char     achpref[] = "^";                        // Prefix
    static const char     achprefsuf[] = "[^A-Za-z0-9_]"; // Prefix/suffix
    static const char     achsuf[] = "$";                     // Suffix
    char                Buffer[ BUFLEN ];                   // Temporary buffer

    #ifndef NDEBUG
        ASSERT( ExprLen >= 2 );                 // Must have at least two characters
    #endif
                                                        // If begin token
    if ( pExprStr[ 0 ] == '\\' && pExprStr[ 1 ] == '<' )
    {
        if ( !(lpgi->Flags & BEGLINE) )               // If not matching at beginning only
        {
                                                        // Copy first prefix
            _fmemcpy( Buffer, achprefsuf, SIZEOF(achprefsuf) - 1 );
                                                        // Attach expression
            _fmemcpy( Buffer + SIZEOF(achprefsuf) - 1, pExprStr + 2,
                      (unsigned)ExprLen - 2 );
                                                        // Add expression
            addexpr( lpgi, Buffer, ExprLen + (int)SIZEOF(achprefsuf) - 3 );
        }
                                                        // Copy second prefix
        _fmemcpy( Buffer, achpref, SIZEOF(achpref) - 1 );
                                                        // Attach expression
        _fmemcpy( Buffer + SIZEOF(achpref) - 1, pExprStr + 2,
                  (unsigned)ExprLen - 2 );
                                                        // Add expression
        addexpr( lpgi, Buffer, ExprLen + (int)SIZEOF( achpref ) - 3 );
        return;                                     // Done
    }
                                                        // Must be end token
    #ifndef NDEBUG
        ASSERT( pExprStr[ ExprLen - 2 ] == '\\' &&
                                pExprStr[ ExprLen - 1 ] == '>' );
    #endif

    if ( !(lpgi->Flags & ENDLINE) )                   // If not matching at end only
    {
                                                        // Copy expression
        _fmemcpy( Buffer, pExprStr, (unsigned)ExprLen - 2 );
                                                        // Attach first suffix
        _fmemcpy( Buffer + ExprLen - 2, achprefsuf, SIZEOF(achprefsuf) - 1 );
                                                        // Add expression
        addexpr( lpgi, Buffer, ExprLen + (int)SIZEOF( achprefsuf ) - 3 );
    }
                                                        // Copy expression
    _fmemcpy( Buffer, pExprStr, (unsigned)ExprLen - 2 );
                                                        // Attach second suffix
    _fmemcpy( Buffer + ExprLen - 2, achsuf, SIZEOF(achsuf) - 1);
                                                        // Add expression
    addexpr( lpgi, Buffer, ExprLen + (int)SIZEOF( achsuf ) - 3);
}

//*********************************************************************
//  void addexpr( LPGREPINFO lpgi, LPSTR pExprStr, int ExprLen )
//
//  ARGUMENTS:
//      pExprStr        - Expression to add
//      ExprLen     - Length of expression
//*********************************************************************

void addexpr( LPGREPINFO lpgi, LPSTR pExprStr, int ExprLen )
{
    LPSTR           pBufStart;
    EXPR        *expr;                  // Expression node pointer
    int             i;                      // Index
    int             j;                      // Index
    int             LocalFlgs;              // Local copy of flags
    unsigned char   BitVec[ ASCII_LEN / 8 ];// First char. bit vector
    char                Buffer[ BUFLEN ];   // Temporary buffer

    if ( lpgi->find == findall )
        return;                             // Return if matching everything

    if ( istoken( pExprStr, ExprLen ) )     // If expr is token
    {
        addtoken( lpgi, pExprStr, ExprLen );// Convert and add tokens
        return;                             // Done
    }

    LocalFlgs = lpgi->Flags;                // Initialize local copy
    if ( *pExprStr == '^' )
        LocalFlgs |= BEGLINE;               // Set flag if match must begin line

    j = -2;                                 // Assume no escapes in string

    for ( i = 0; i < ExprLen - 1; i++ )     // Loop to find last escape
    {
        if ( pExprStr[ i ] == '\\' )
            j = i++;                        // Save index of last escape
    }

    if ( ExprLen > 0 && pExprStr[ ExprLen - 1 ] == '$' && j != ExprLen - 2 )
    {                                       // If expr. ends in unescaped '$'
        ExprLen--;                          // Skip dollar sign
        LocalFlgs |= ENDLINE;               // Match must be at end
    }
                                            // Copy pattern to buffer
    _fmemcpy( Buffer, pExprStr, (unsigned)ExprLen );
    if ( LocalFlgs & ENDLINE )
        Buffer[ ExprLen++ ] = EOS;          // Add end character if needed

    Buffer[ ExprLen ] = '\0';               // Null-terminate string

    ASSERT(lstrlenA(Buffer)==ExprLen);

    if ( (pExprStr = exprparse( lpgi, Buffer, &ExprLen )) == NULL )
        return;                             // Return if invalid expression
    pBufStart = pExprStr;                   // Save base of original alloc()

    lpgi->ge.StrCount++;                    // Increment string count

    if ( !(LocalFlgs & BEGLINE) )           // If match needn't be at beginning
        pExprStr = get1stcharset( lpgi, pExprStr, BitVec ); // Remove leading *-expr.s

        //  pExpStr now points to a Buffer containing a preprocessed expression.
        // We need to find the set of allowable first characters and make
        // the appropriate entries in the string node table.

    if ( *get1stcharset( lpgi, pExprStr, BitVec ) == T_END )
    {                                       // If expression will match anything
        lpgi->find = findall;                     // Match everything
                                            // Make sure memory is freed.
        if ( !Free( pBufStart ) )
            RaiseException(ERROR_INVALID_BLOCK, 0, 0, 0);
        return;                             // All done
    }

    if ( lpgi->ge.ExprEntriesUsed >= ASCII_LEN )
        RaiseException(ERROR_INVALID_PARAMETER, 0, 0, 0);

        // Copy the expression to the start of it's buffer and add it to
        // the expression string list. The call to get1stcharset() may
        // have returned a ptr to an offset in the original buffer so
        // that is why we may have to move the expression string.

    lpgi->ge.ExprStrList[ lpgi->ge.ExprEntriesUsed++ ] = pBufStart;
    if ( pExprStr != pBufStart )
        MoveMemory( pBufStart, pExprStr, (unsigned)ExprLen );

    for ( j = 0; j < ASCII_LEN; j++ )       // Loop to examine bit vector
    {
        if ( BitVec[ (unsigned)j >> 3 ] & (1 << (j &7)) )
        {                                   // If the bit is set
            // Allocate a new node and point it to the expression string
            expr = (EXPR *) AllocThrow( lpgi, SIZEOF( EXPR ) );
            expr->ex_pattern = pBufStart;
            expr->ex_dummy = NULL;

            if ( (i = lpgi->ge.TransTable[ j ]) == 0 )   // If no existing list
            {
                if ( (i = lpgi->ge.TblEntriesUsed++) >= ASCII_LEN )
                    RaiseException(ERROR_INVALID_PARAMETER, 0, 0, 0);

                #ifndef NDEBUG
                    ASSERT( lpgi->ge.StringList[ i ] == NULL );
                #endif

                lpgi->ge.TransTable[ j ] = (unsigned char) i;    // Set pointer to new list

                if ( !lpgi->CaseSen && IsCharAlpha( (unsigned char)j ) )
                    lpgi->ge.TransTable[ j ^ 0x20 ] = (unsigned char)i; // Set pointer for other case
            }
                                                        // Link new record into table
            expr->ex_next = (struct exprnode *)
                    ((LPSTR)lpgi->ge.StringList[ i ]);
            lpgi->ge.StringList[ i ] = (STRINGNODE *)((LPSTR)expr);
        }
    }
}
