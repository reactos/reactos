/****************************** Module Header ******************************\
* Module Name: wstrings.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 03-20-91 IanJa      Created
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/* LATER these should be in a public header file!!!
 * Assorted defines used to support the standard Windows ANSI code page
 * (now known as code page 1252 and officially registered by IBM).
 * This is intended only for the PDK release.  Subsequent releases will
 * use the NLSAPI and Unicode.
 */
#define LATIN_CAPITAL_LETTER_A_GRAVE    (WCHAR)0xc0
#define LATIN_CAPITAL_LETTER_THORN      (WCHAR)0xde
#define LATIN_SMALL_LETTER_SHARP_S      (WCHAR)0xdf
#define LATIN_SMALL_LETTER_Y_DIAERESIS  (WCHAR)0xff
#define DIVISION_SIGN                   (WCHAR)0xf7
#define MULTIPLICATION_SIGN             (WCHAR)0xd7


/*
 * Temporary defines to support Unicode block 1 (0x0000 - 0x00ff).
 */
#define WCTOA(wch)  ((wch) & 0xff)
#define IS_UNICODE_BLK1(wch)  ((int)(wch) <= 0x00ff)


/***************************************************************************\
* CharLowerW (API)
*
* Convert either a single character or an entire string to lower case.  The
* two cases are differentiated by checking the high-word of pwsz.  If it is
* 0 then we just convert the low-word of pwsz.
*
* History:
* 06-24-91 GregoryW     Created.  Supports Unicode equivalent of code
*                       page 1252 (simple zero extension).  This is for
*                       the PDK release only.  After the PDK this routine
*                       will be modified to use the NLSAPI.
* 02-11-93 IanJa        Modified to use NLS API.
\***************************************************************************/

LPWSTR WINAPI CharLowerW(
    LPWSTR pwsz)
{
    /*
     * Early out for NULL string or '\0'
     */
    if (pwsz == NULL) {
        return pwsz;
    }

    if (!IS_PTR(pwsz)) {
        if (!LCMapStringW(
                 LOCALE_USER_DEFAULT,
                 LCMAP_LOWERCASE,
                 (LPWSTR)&pwsz,
                 1,
                 (LPWSTR)&pwsz,
                 1
                 )) {
            /*
             * We don't expect LCMapString to fail!  The caller is not expecting
             * failure, CharLowerW does not have a failure indicator, so we do
             * nothing.
             */
            RIPMSG1(RIP_WARNING, "CharLowerW(%#p): LCMapString failed\n", pwsz);
        }

        return pwsz;
    }

    /*
     * pwsz is a null-terminated string
     */
    CharLowerBuffW(pwsz, wcslen(pwsz)+1);
    return pwsz;
}


/***************************************************************************\
* CharUpperW (API)
*
* Convert either a single character or an entire string to upper case.  The
* two cases are differentiated by checking the high-word of pwsz.  If it is
* 0 then we just convert the low-word of pwsz.
*
* History:
* 06-24-91 GregoryW     Created.  Supports Unicode equivalent of code
*                       page 1252 (simple zero extension).  This is for
*                       the PDK release only.  After the PDK this routine
*                       will be modified to use the NLSAPI.
* 02-11-93 IanJa        Modified to use NLS API.
\***************************************************************************/

LPWSTR WINAPI CharUpperW(
    LPWSTR pwsz)
{
    /*
     * Early out for NULL string or '\0'
     */
    if (pwsz == NULL) {
        return pwsz;
    }

    if (!IS_PTR(pwsz)) {
        if (!LCMapStringW(
                 LOCALE_USER_DEFAULT,
                 LCMAP_UPPERCASE,
                 (LPWSTR)&pwsz,
                 1,
                 (LPWSTR)&pwsz,
                 1
                 )) {
            /*
             * We don't expect LCMapString to fail!  The caller is not expecting
             * failure, CharLowerW does not have a failure indicator, so we do
             * nothing.
             */
            RIPMSG1(RIP_WARNING, "CharUpperW(%#p): LCMapString failed", pwsz);
        }

        return pwsz;
    }

    /*
     * pwsz is a null-terminated string
     */
    CharUpperBuffW(pwsz, wcslen(pwsz)+1);
    return pwsz;
}


/***************************************************************************\
* CharNextW (API)
*
* Move to next character in string unless already at '\0' terminator
*
* History:
* 06-24-91 GregoryW     Created.  This routine will not work for non-spacing
*                       characters!!  This version is only intended for
*                       limited use in the PDK release.
* 02-20-92 GregoryW     Modified to work with combining marks (formerly known
*                       as non-spacing).
* 09-21-93 JulieB       Added ALPHA to combining mark code.
\***************************************************************************/

LPWSTR WINAPI CharNextW(
    LPCWSTR lpwCurrentChar)
{
    WORD ctype3info;

    if (*lpwCurrentChar) {
        //
        // Examine each code element.  Skip all combining elements.
        //
        while (*(++lpwCurrentChar)) {
            if (!GetStringTypeW(
                    CT_CTYPE3,
                    lpwCurrentChar,
                    1,
                    &ctype3info)) {
                /*
                 * GetStringTypeW failed!  The caller is not expecting failure,
                 * CharNextW does not have a failure indicator, so just return
                 * a pointer to the character we couldn't analyze.
                 */
                RIPMSG2(RIP_WARNING, "CharNextW failed, L'\\x%.4x' at %#p",
                        *lpwCurrentChar, lpwCurrentChar);
                break;
            }
            if (!((ctype3info & C3_NONSPACING) && (!(ctype3info & C3_ALPHA)))) {
                break;
            }
        }
    }

    return (LPWSTR)lpwCurrentChar;
}


/***************************************************************************\
* CharPrevW (API)
*
* Move to previous character in string, unless already at start
*
* History:
* 06-24-91 GregoryW     Created.  This routine will not work for non-spacing
*                       characters!!  This version is only intended for
*                       limited use in the PDK release.
* 02-20-92 GregoryW     Modified to work with combining marks (formerly
*                       known as non-spacing).
* 09-21-93 JulieB       Added ALPHA to combining mark code.
* 12-06-93 JulieB       Fixed combining mark code.
\***************************************************************************/

LPWSTR WINAPI CharPrevW(
    LPCWSTR lpwStart,
    LPCWSTR lpwCurrentChar)
{
    WORD ctype3info;
    LPWSTR lpwValidChar = (LPWSTR)lpwCurrentChar;


    if (lpwCurrentChar > lpwStart) {
        //
        // Examine each code element.  Skip all combining elements.
        //
        while (lpwCurrentChar-- > lpwStart) {
            if (!GetStringTypeW(
                    CT_CTYPE3,
                    lpwCurrentChar,
                    1,
                    &ctype3info)) {
                /*
                 * GetStringTypeW failed!  The caller is not expecting failure,
                 * CharPrevW does not have a failure indicator, so just return
                 * a pointer to the character we couldn't analyze.
                 */
                RIPMSG2(RIP_WARNING, "CharPrevW failed, L'\\x%.4x' at %#p",
                        *lpwCurrentChar, lpwCurrentChar);
                break;
            }
            if (!((ctype3info & C3_NONSPACING) && (!(ctype3info & C3_ALPHA)))) {
                lpwValidChar = (LPWSTR)lpwCurrentChar;
                break;  // found non-combining code element
            }
        }
    }

    return (LPWSTR)lpwValidChar;
}


/***************************************************************************\
* CharLowerBuffW (API)
*
* History:
* 06-24-91 GregoryW     Created.  This version only supports Unicode
*                       block 1 (0x0000 - 0x00ff).  All other code points
*                       are copied verbatim.  This version is intended
*                       only for the PDK release.
* 02-11-93 IanJa        Modified to use NLS API.
\***************************************************************************/

DWORD WINAPI CharLowerBuffW(
    LPWSTR pwsz,
    DWORD cwch)
{
    int cwchT;
    DWORD i;

    if (cwch == 0) {
        return 0;
    }

    cwchT = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE,
                pwsz, cwch, pwsz, cwch);

    if (cwchT != 0) {
        return cwchT;
    }

    /*
     * LCMapString failed!  The caller is not expecting failure,
     * CharLowerBuffW does not have a failure indicator, so we
     * convert the buffer to lower case as best we can.
     */
    RIPMSG1(RIP_WARNING, "CharLowerBuffW(%ls) failed", pwsz);

    for (i=0; i < cwch; i++) {
        if (IS_UNICODE_BLK1(pwsz[i]) && IsCharUpperA((char)pwsz[i])) {
            pwsz[i] += 'a'-'A';
        }
    }

    return cwch;
}


/***************************************************************************\
* CharUpperBuffW (API)
*
* History:
* 06-24-91 GregoryW     Created.  This version only supports Unicode
*                       block 1 (0x0000 - 0x00ff).  All other code points
*                       are copied verbatim.  This version is intended
*                       only for the PDK release.
* 02-11-93 IanJa        Modified to use NLS API.
\***************************************************************************/

DWORD WINAPI CharUpperBuffW(
    LPWSTR pwsz,
    DWORD cwch)
{
    int cwchT;
    DWORD i;

    if (cwch == 0) {
        return 0;
    }

    cwchT = LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE,
                pwsz, cwch, pwsz, cwch);

    if (cwchT != 0) {
        return cwchT;
    }

    /*
     * LCMapString failed!  The caller is not expecting failure,
     * CharUpperBuffW does not have a failure indicator, so we
     * convert the buffer to upper case as best we can.
     */
    RIPMSG1(RIP_WARNING, "CharUpperBuffW(%ls) failed", pwsz);

    for (i=0; i < cwch; i++) {
        if (IS_UNICODE_BLK1(pwsz[i]) &&
                IsCharLowerA((char)pwsz[i]) &&
                (pwsz[i] != LATIN_SMALL_LETTER_SHARP_S) &&
                (pwsz[i] != LATIN_SMALL_LETTER_Y_DIAERESIS)) {
            pwsz[i] += (WCHAR)('A'-'a');
        }
    }

    return cwch;
}



/***************************************************************************\
* IsCharLowerW (API)
*
* History:
* 06-24-91 GregoryW     Created.  This version only supports Unicode
*                       block 1 (0x0000 - 0x00ff).  FALSE is returned
*                       for all other code points. This version is intended
*                       only for the PDK release.
* 02-20-92 GregoryW     Modified to use NLS API.
\***************************************************************************/

BOOL WINAPI IsCharLowerW(
    WCHAR wChar)
{
    WORD ctype1info;

    if (GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info)) {
        if (ctype1info & C1_LOWER) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    /*
     * GetStringTypeW failed!  The caller is not expecting
     * failure, IsCharLowerW does not have a failure indicator, so we
     * determine the case as best we can.
     */
    RIPMSG1(RIP_WARNING, "IsCharLowerW(L'\\x%.4lx') failed", wChar);

    if (IS_UNICODE_BLK1(wChar)) {
        return IsCharLowerA((CHAR)wChar);
    } else {
        return FALSE;
    }
}


/***************************************************************************\
* IsCharUpperW (API)
*
* History:
* 06-24-91 GregoryW     Created.  This version only supports Unicode
*                       block 1 (0x0000 - 0x00ff).  FALSE is returned
*                       for all other code points. This version is intended
*                       only for the PDK release.
* 02-20-92 GregoryW     Modified to use NLS API.
\***************************************************************************/

BOOL WINAPI IsCharUpperW(
    WCHAR wChar)
{
    WORD ctype1info;

    if (GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info)) {
        if (ctype1info & C1_UPPER) {
            return TRUE;
        } else {
            return FALSE;
        }
    }

    /*
     * GetStringTypeW failed!  The caller is not expecting
     * failure, IsCharLowerW does not have a failure indicator, so we
     * determine the case as best we can.
     */
    RIPMSG1(RIP_WARNING, "IsCharUpper(L'\\x%.4lx') failed", wChar);

    if (IS_UNICODE_BLK1(wChar)) {
        return IsCharUpperA((CHAR)wChar);
    } else {
        return FALSE;
    }
}


/***************************************************************************\
* IsCharAlphaNumericW (API)
*
* Returns TRUE if character is alphabetical or numerical, otherwise FALSE
*
* History:
* 06-24-91 GregoryW     Created.  This version only supports Unicode
*                       block 1 (0x0000 - 0x00ff).
*                       This version is intended only for the PDK release.
* 02-20-92 GregoryW     Modified to use NLS API.
\***************************************************************************/

BOOL WINAPI IsCharAlphaNumericW(
    WCHAR wChar)
{
    WORD ctype1info;

    if (!GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info)) {
        //
        // GetStringTypeW returned an error!  IsCharAlphaNumericW has no
        // provision for returning an error...  The best we can do is to
        // return FALSE
        //
        UserAssert(FALSE);
        return FALSE;
    }
    //
    // LATER 20 Feb 92 GregoryW
    //    We may need to check ctype 3 info if we want to check for
    //    digits other than ASCII '0'-'9' (such as Lao digits or
    //    Tibetan digits, etc.).
    //
#ifdef FE_SB // IsCharAlphaNumericW()
    if (ctype1info & C1_ALPHA) {
        WORD ctype3info = 0;
        /*
         * We don't want to return TRUE for halfwidth katakana.
         * Katakana is linguistic character (C1_ALPHA), but it is not
         * alphabet character.
         */
        if (!GetStringTypeW(CT_CTYPE3, &wChar, 1, &ctype3info)) {
            UserAssert(FALSE);
            /*
             * Assume, it is alphabet character, because it has
             * C1_ALPHA attribute.
             */
            return TRUE;
        }

        if (ctype3info & (C3_KATAKANA|C3_HIRAGANA)) {
            /*
             * This is 'Katakana'.
             */
            return FALSE;
        } else {
            return TRUE;
        }
    } else if (ctype1info & C1_DIGIT) {
        return TRUE;
    } else {
        return FALSE;
    }
#else
    if ((ctype1info & C1_ALPHA) || (ctype1info & C1_DIGIT)) {
        return TRUE;
    } else {
        return FALSE;
    }
#endif // FE_SB
}


/***************************************************************************\
* IsCharAlphaW (API)
*
* Returns TRUE if character is alphabetical, otherwise FALSE
*
* History:
* 06-24-91 GregoryW     Created.  This version only supports Unicode
*                       block 1 (0x0000 - 0x00ff).
*                       This version is intended only for the PDK release.
* 02-20-92 GregoryW     Modified to use NLS API.
\***************************************************************************/

BOOL WINAPI IsCharAlphaW(
    WCHAR wChar)
{
    WORD ctype1info;

    if (!GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info)) {
        //
        // GetStringTypeW returned an error!  IsCharAlphaW has no
        // provision for returning an error...  The best we can do
        // is to return FALSE
        //
        UserAssert(FALSE);
        return FALSE;
    }
    if (ctype1info & C1_ALPHA) {
#ifdef FE_SB // IsCharAlphaA()
        WORD ctype3info = 0;
        /*
         * We don't want to return TRUE for halfwidth katakana.
         * Katakana is linguistic character (C1_ALPHA), but it is not
         * alphabet character.
         */
        if (!GetStringTypeW(CT_CTYPE3, &wChar, 1, &ctype3info)) {
            UserAssert(FALSE);
            /*
             * Assume, it is alphabet character, because it has
             * C1_ALPHA attribute.
             */
            return TRUE;
        }

        if (ctype3info & (C3_KATAKANA|C3_HIRAGANA)) {
            /*
             * This is 'Katakana'.
             */
            return FALSE;
        } else {
            return TRUE;
        }
#else
        return TRUE;
#endif // FE_SB
    } else {
        return FALSE;
    }
}
