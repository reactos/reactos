/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    utils.c

Abstract:

    Implements various utilities.

Author:

    Calin Negreanu (calinn)  20-Jan-1999

Revision History:

    <alias> <date> <comments>

--*/

#include "utils.h"

#define CHARTYPE WORD
#define PCHARTYPE PWORD

CHARTYPE
pGetNextChar (
    PCTSTR Source
    )
{
    CHARTYPE result;
#ifdef UNICODE
    result = Source [0];
#else
    if (IsDBCSLeadByte (Source [0])) {
        result = *((PCHARTYPE)Source);
    } else {
        result = Source [0];
    }
#endif
    return result;
}

CHARTYPE
pCharLower (
    CHARTYPE ch
    )
{
    return ((CHARTYPE) (CharLower ((PTSTR) ch)));
}

BOOL
ShIsPatternMatch (
    IN     PCTSTR strPat,
    IN     PCTSTR strStr
    )
{
    CHARTYPE chSrc, chPat;

    while (*strStr) {
        chSrc = pCharLower (pGetNextChar (strStr));
        chPat = pCharLower (pGetNextChar (strPat));

        if (chPat == TEXT('*')) {

            // Check if asterisk is at the end.  If so, we have a match already.
            chPat = pCharLower (pGetNextChar (CharNext (strPat)));
            if (!chPat)
                return TRUE;

            // Otherwise check if next pattern char matches current char
            if (chPat == chSrc || chPat == TEXT('?')) {

                // do recursive check for rest of pattern
                if (ShIsPatternMatch (CharNext (strPat), strStr))
                    return TRUE;
            }

            //
            // Allow any character and continue
            //

            strStr = CharNext (strStr);
            continue;
        }

        if (chPat != TEXT('?')) {

            //
            // if next pattern character is not a question mark, src and pat
            // must be identical.
            //

            if (chSrc != chPat)
                return FALSE;
        }

        //
        // Advance when pattern character matches string character
        //

        strPat = CharNext (strPat);
        strStr = CharNext (strStr);
    }

    //
    // Fail when there is more pattern and pattern does not end in an asterisk
    //

    chPat = pCharLower (pGetNextChar (strPat));
    while (chPat == TEXT('*')) {
        strPat = CharNext (strPat);
        chPat = pCharLower (pGetNextChar (strPat));
    }
    if (chPat) {
        return FALSE;
    }

    return TRUE;
}

PCTSTR
ShGetLastChar (
    IN      PCTSTR String,
    IN      CHARTYPE Char
    )
{
    PCTSTR lastCharPtr = NULL;

    while (*String) {
        if (pGetNextChar (String) == Char) {
            lastCharPtr = String;
        }
        String = CharNext (String);
    }
    return lastCharPtr;
}

PCTSTR
ShGetFileNameFromPath (
    IN      PCTSTR PathSpec
    )
{
    PCTSTR p;

    p = ShGetLastChar (PathSpec, TEXT('\\'));
    if (p) {
        p = CharNext (p);
    } else {
        p = PathSpec;
    }

    return p;
}
