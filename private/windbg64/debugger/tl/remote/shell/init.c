/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    init.c

Abstract:

    Code copied from windbg

Author:

    Carlos Klapp 1998

Environment:

    Utilities

--*/

#include "precomp.h"
#pragma hdrstop

int
CPCopyString(
    LPSTR * lplps,
    LPSTR lpT,
    char  chEscape,
    BOOL  fQuote
    )
/*++

Routine Description:

    Scan and copy an optionally quoted C-style string.  If the first character is
    a quote, a matching quote will terminate the string, otherwise the scanning will
    stop at the first whitespace encountered.  The target string will be null
    terminated if any characters are copied.

Arguments:

    lplps    - Supplies a pointer to a pointer to the source string

    lpt      - Supplies a pointer to the target string

    chEscape - Supplies the escape character (typically '\\')

    fQuote   - Supplies a flag indicating whether the first character is a quote

Return Value:

    The number of characters copied into lpt[].  If an error occurs, -1 is returned.

--*/
{
    LPSTR lps = *lplps;
    LPSTR lpt = lpT;
    int   i;
    int   n;
    int   err = 0;
    char  cQuote = '\0';

    if (fQuote) {
        if (*lps) cQuote = *lps++;
    }

    while (!err) {

        if (*lps == 0)
        {
            if (fQuote) err = 1;
            else        *lpt = '\0';
            break;
        }
        else if (fQuote && *lps == cQuote)
        {
            *lpt = '\0';
            // eat the quote
            lps++;
            break;
        }
        else if (!fQuote &&  (!*lps || *lps == ' ' || *lps == '\t' || *lps == '\r' || *lps == '\n'))
        {
            *lpt = '\0';
            break;
        }

        else if (*lps != chEscape)
        {
            *lpt++ = *lps++;
        }
        else
        {
            switch (*++lps) {
              case 0:
                err = 1;
                --lps;
                break;

              default:     // any char - usually escape or quote
                *lpt++ = *lps;
                break;

              case 'b':    // backspace
                *lpt++ = '\b';
                break;

              case 'f':    // formfeed
                *lpt++ = '\f';
                break;

              case 'n':    // newline
                *lpt++ = '\n';
                break;

              case 'r':    // return
                *lpt++ = '\r';
                break;

              case 's':    // space
                *lpt++ = ' ';
                break;

              case 't':    // tab
                *lpt++ = '\t';
                break;

              case '0':    // octal escape
                for (n = 0, i = 0; i < 3; i++) {
                    ++lps;
                    if (*lps < '0' || *lps > '7') {
                        --lps;
                        break;
                    }
                    n = (n<<3) + *lps - '0';
                }
                *lpt++ = (UCHAR)(n & 0xff);
                break;
            }
            lps++;    // skip char from switch
        }

    }  // while

    if (err) {
        return -1;
    } else {
        *lplps = lps;
        return lpt - lpT;
    }
}

LPSTR
GetArg(
    LPSTR *lpp
    )
{
    static PSTR pszBuffer = NULL;
    int r;
    LPSTR p1 = *lpp;

    while (*p1 == ' ' || *p1 == '\t') {
        p1++;
    }

    if (pszBuffer) {
        free(pszBuffer);
    }
    pszBuffer = (PSTR) calloc(strlen(p1) +1, 1);

    r = CPCopyString(&p1, pszBuffer, 0, (*p1 == '\'' || *p1 == '"'));
    if (r >= 0) {
        *lpp = p1;
    }
    return pszBuffer;
}


