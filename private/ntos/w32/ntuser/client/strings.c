/****************************** Module Header ******************************\
* Module Name: strings.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the string handling APIs and functions.  Since
* they don't access server-specific data they belong here in the client DLL.
*
* History:
* 10-18-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/* LATER these should be in a public header file!!!
 * Assorted defines used to support the standard Windows ANSI code page
 * (now known as code page 1252 and officially registered by IBM).
 * This is intended only for the PDK release.  Subsequent releases will
 * use the NLSAPI and Unicode.
 */
#define LATIN_CAPITAL_LETTER_A_GRAVE    (CHAR)0xc0
#define LATIN_CAPITAL_LETTER_THORN      (CHAR)0xde
#define LATIN_SMALL_LETTER_SHARP_S      (CHAR)0xdf
#define LATIN_SMALL_LETTER_Y_DIAERESIS  (CHAR)0xff
#define DIVISION_SIGN                   (CHAR)0xf7
#define MULTIPLICATION_SIGN             (CHAR)0xd7


/***************************************************************************\
* CharLowerA (API)
*
* Convert either a single character or an entire string to lower case.  The
* two cases are differentiated by checking the high-word of psz.  If it is
* 0 then we just convert the low-word of psz.
*
* History:
* 11-26-90 DarrinM      Created non-NLS version.
* 06-22-91 GregoryW     Modified to support code page 1252.  This is for
*                       the PDK release only.  After the PDK this routine
*                       will be modified to use the NLSAPI.  Also renamed
*                       API to conform to new naming conventions.  AnsiLower
*                       is now a #define which resolves to this routine.
\***************************************************************************/

LPSTR WINAPI CharLowerA(
    LPSTR psz)
{
    NTSTATUS st;

    /*
     * Early out for NULL string or '\0'
     */
    if (psz == NULL) {
        return psz;
    }

    if (!IS_PTR(psz)) {
        WCHAR wch;

#ifdef FE_SB // CharLowerA()
        /*
         * if only DBCS Leadbyte was passed, just return the character.
         * Same behavior as Windows 3.1J and Windows 95 FarEast version.
         */
        if (IS_DBCS_ENABLED() && IsDBCSLeadByte((BYTE)(ULONG_PTR)psz)) {
            return psz;
        }
#endif // FE_SB

        //
        // LATER 14 Feb 92 GregoryW
        //    For DBCS code pages is a double byte character ever
        //    passed in the low word of psz or is the high nibble
        //    of the low word always ignored?
        //
        st = RtlMultiByteToUnicodeN(&wch, sizeof(WCHAR), NULL, (PCH)&psz, sizeof(CHAR));
        if (!NT_SUCCESS(st)) {
            /*
             * Failed!  Caller is not expecting failure, CharLowerA does not
             * have a failure indicator, so just return the original character.
             */
            RIPMSG1(RIP_WARNING, "CharLowerA(%#p) failed\n", psz);
        } else {
            /*
             * The next two calls never fail.
             */
            LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, &wch, 1, &wch, 1);
            RtlUnicodeToMultiByteN((PCH)&psz, sizeof(CHAR), NULL, &wch, sizeof(WCHAR));
        }
        return psz;

    }

    /*
     * psz is a null-terminated string
     */
    CharLowerBuffA(psz, strlen(psz)+1);
    return psz;
}


/***************************************************************************\
* CharUpperA (API)
*
* Convert either a single character or an entire string to upper case.  The
* two cases are differentiated by checking the high-word of psz.  If it is
* 0 then we just convert the low-word of psz.
*
* History:
* 12-03-90 IanJa        derived from DarrinM's non-NLS AnsiLower
* 06-22-91 GregoryW     Modified to support code page 1252.  This is for
*                       the PDK release only.  After the PDK this routine
*                       will be modified to use the NLSAPI.  Also renamed
*                       API to conform to new naming conventions.  AnsiUpper
*                       is now a #define which resolves to this routine.
\***************************************************************************/

LPSTR WINAPI CharUpperA(
    LPSTR psz)
{
    NTSTATUS st;

    /*
     * Early out for NULL string or '\0'
     */
    if (psz == NULL) {
        return psz;
    }

    if (!IS_PTR(psz)) {
        WCHAR wch;

#ifdef FE_SB // CharLowerA()
        /*
         * if only DBCS Leadbyte was passed, just return the character.
         * Same behavior as Windows 3.1J and Windows 95 FarEast version.
         */
        if (IS_DBCS_ENABLED() && IsDBCSLeadByte((BYTE)(ULONG_PTR)psz)) {
            return psz;
        }
#endif // FE_SB

        //
        // LATER 14 Feb 92 GregoryW
        //    For DBCS code pages is a double byte character ever
        //    passed in the low word of psz or is the high nibble
        //    of the low word always ignored?
        //
        st = RtlMultiByteToUnicodeN(&wch, sizeof(WCHAR), NULL, (PCH)&psz, sizeof(CHAR));
        if (!NT_SUCCESS(st)) {
            /*
             * Failed!  Caller is not expecting failure, CharUpperA does not
             * have a failure indicator, so return the original character.
             */
            RIPMSG1(RIP_WARNING, "CharUpperA(%#p) failed\n", psz);
        } else {
            /*
             * The next two calls never fail.
             */
            LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, &wch, 1, &wch, 1);
            RtlUnicodeToMultiByteN((PCH)&psz, sizeof(CHAR), NULL, &wch, sizeof(WCHAR));
        }
        return psz;

    }

    /*
     * psz is a null-terminated string
     */
    CharUpperBuffA(psz, strlen(psz)+1);
    return psz;
}


/***************************************************************************\
* CharNextA (API)
*
* Move to next character in string unless already at '\0' terminator
* DOES NOT WORK CORRECTLY FOR DBCS (eg: Japanese)
*
* History:
* 12-03-90 IanJa        Created non-NLS version.
* 06-22-91 GregoryW     Renamed API to conform to new naming conventions.
*                       AnsiNext is now a #define which resolves to this
*                       routine.  This routine is only intended to support
*                       code page 1252 for the PDK release.
\***************************************************************************/

LPSTR WINAPI CharNextA(
    LPCSTR lpCurrentChar)
{
#ifdef FE_SB // CharNextA(): dbcs enabling
    if (IS_DBCS_ENABLED() && IsDBCSLeadByte(*lpCurrentChar)) {
        lpCurrentChar++;
    }
    /*
     * if we have only DBCS LeadingByte, we will point string-terminaler.
     */
#endif // FE_SB

    if (*lpCurrentChar) {
        lpCurrentChar++;
    }
    return (LPSTR)lpCurrentChar;
}


/***************************************************************************\
* CharNextExA (API)
*
* Move to next character in string unless already at '\0' terminator.
*
* History:
* 05-01-95 GregoryW     Ported from Win95.
\***************************************************************************/

LPSTR WINAPI CharNextExA(
    WORD CodePage,
    LPCSTR lpCurrentChar,
    DWORD dwFlags)
{
    if (lpCurrentChar == (LPSTR)NULL)
    {
        return (LPSTR)lpCurrentChar;
    }

    if (IsDBCSLeadByteEx(CodePage, *lpCurrentChar))
    {
        lpCurrentChar++;
    }

    if (*lpCurrentChar)
    {
        lpCurrentChar++;
    }
    return (LPSTR)lpCurrentChar;

    UNREFERENCED_PARAMETER(dwFlags);
}


/***************************************************************************\
* CharPrevA (API)
*
* Move to previous character in string, unless already at start
* DOES NOT WORK CORRECTLY FOR DBCS (eg: Japanese)
*
* History:
* 12-03-90 IanJa        Created non-NLS version.
* 06-22-91 GregoryW     Renamed API to conform to new naming conventions.
*                       AnsiPrev is now a #define which resolves to this
*                       routine.  This routine is only intended to support
*                       code page 1252 for the PDK release.
\***************************************************************************/

LPSTR WINAPI CharPrevA(
    LPCSTR lpStart,
    LPCSTR lpCurrentChar)
{
#ifdef FE_SB // CharPrevA : dbcs enabling
    if (lpCurrentChar > lpStart) {
        if (IS_DBCS_ENABLED()) {
            LPCSTR lpChar;
            BOOL bDBC = FALSE;

            for (lpChar = --lpCurrentChar - 1 ; lpChar >= lpStart ; lpChar--) {
                if (!IsDBCSLeadByte(*lpChar))
                    break;
                bDBC = !bDBC;
            }

            if (bDBC)
                lpCurrentChar--;
        }
        else
            lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
#else
    if (lpCurrentChar > lpStart) {
        lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
#endif // FE_SB
}

/***************************************************************************\
* CharPrevExA (API)
*
* Move to previous character in string, unless already at start.
*
* History:
* 05-01-95 GregoryW     Ported from Win95.
\***************************************************************************/

LPSTR WINAPI CharPrevExA(
    WORD CodePage,
    LPCSTR lpStart,
    LPCSTR lpCurrentChar,
    DWORD dwFlags)
{
    if (lpCurrentChar > lpStart) {
        LPCSTR lpChar;
        BOOL bDBC = FALSE;

        for (lpChar = --lpCurrentChar - 1 ; lpChar >= lpStart ; lpChar--) {
            if (!IsDBCSLeadByteEx(CodePage, *lpChar))
                break;
            bDBC = !bDBC;
        }

        if (bDBC)
            lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;

    UNREFERENCED_PARAMETER(dwFlags);
}


/***************************************************************************\
* CharLowerBuffA (API)
*
* History:
* 14-Jan-1991 mikeke from win 3.0
* 06-22-91 GregoryW     Renamed API to conform to new naming conventions.
*                       AnsiLowerBuff is now a #define which resolves to this
*                       routine.  This routine is only intended to support
*                       code page 1252 for the PDK release.
* 02-20-1992 GregoryW   Modified to use NLS API.
\***************************************************************************/
#define CCH_LOCAL_BUFF 256

DWORD WINAPI CharLowerBuffA(
    LPSTR psz,
    DWORD nLength)
{
    ULONG cb;
    WCHAR awchLocal[CCH_LOCAL_BUFF];
    LPWSTR pwszT = awchLocal;
    int cwch;

    if (nLength == 0) {
        return(0);
    }

    /*
     * Convert ANSI to Unicode.
     * Use awchLocal if it is big enough, otherwise allocate space.
     */
    cwch = MBToWCS(
            psz,       // ANSI buffer
            nLength,   // length of buffer
            &pwszT,    // address of Unicode string
            (nLength > CCH_LOCAL_BUFF ? -1 : nLength),
            (nLength > CCH_LOCAL_BUFF) );

    if (cwch != 0) {
        CharLowerBuffW(pwszT, cwch);

        /*
         * This can't fail
         */
        RtlUnicodeToMultiByteN(
                  psz,                   // ANSI string
                  nLength,               // given to us
                  &cb,                   // result length
                  pwszT,                 // Unicode string
                  cwch * sizeof(WCHAR)); // length IN BYTES

        if (pwszT != awchLocal) {
            UserLocalFree(pwszT);
        }

        return (DWORD)cb;
    }

    /*
     * MBToWCS failed!  The caller is not expecting failure,
     * so we convert the string to lower case as best we can.
     */
    RIPMSG2(RIP_WARNING,
            "CharLowerBuffA(%#p, %lx) failed\n", psz, nLength);

    for (cb=0; cb < nLength; cb++) {
#ifdef FE_SB // CharLowerBuffA(): skip double byte character
        if (IS_DBCS_ENABLED() && IsDBCSLeadByte(psz[cb])) {
            cb++;
        } else if (IsCharUpperA(psz[cb])) {
            psz[cb] += 'a'-'A';
        }
#else
        if (IsCharUpperA(psz[cb])) {
            psz[cb] += 'a'-'A';
        }
#endif // FE_SB
    }

    return nLength;
}


/***************************************************************************\
* CharUpperBuffA (API)
*
* History:
* 14-Jan-1991 mikeke from win 3.0
* 06-22-91 GregoryW     Renamed API to conform to new naming conventions.
*                       AnsiUpperBuff is now a #define which resolves to this
*                       routine.  This routine is only intended to support
*                       code page 1252 for the PDK release.
* 02-Feb-1992 GregoryW   Modified to use NLS API.
\***************************************************************************/

DWORD WINAPI CharUpperBuffA(
    LPSTR psz,
    DWORD nLength)
{
    DWORD cb;
    WCHAR awchLocal[CCH_LOCAL_BUFF];
    LPWSTR pwszT = awchLocal;
    int cwch;

    if (nLength==0) {
        return(0);
    }

    /*
     * Convert ANSI to Unicode.
     * Use awchLocal if it is big enough, otherwise allocate space.
     */
    cwch = MBToWCS(
            psz,       // ANSI buffer
            nLength,   // length of buffer
            &pwszT,    // address of Unicode string
            (nLength > CCH_LOCAL_BUFF ? -1 : nLength),
            (nLength > CCH_LOCAL_BUFF) );

    if (cwch != 0) {
        CharUpperBuffW(pwszT, cwch);

        RtlUnicodeToMultiByteN(
                  psz,                   // address of ANSI string
                  nLength,               // given to us
                  &cb,                   // result length
                  pwszT,                 // Unicode string
                  cwch * sizeof(WCHAR)); // length IN BYTES

        if (pwszT != awchLocal) {
            UserLocalFree(pwszT);
        }

        return (DWORD)cb;
    }

    /*
     * MBToWCS failed!  The caller is not expecting failure,
     * so we convert the string to upper case as best we can.
     */
    RIPMSG2(RIP_WARNING,
            "CharLowerBuffA(%#p, %lx) failed\n", psz, nLength);

    for (cb=0; cb < nLength; cb++) {
#ifdef FE_SB // CharUpperBuffA(): skip double byte characters
        if (IS_DBCS_ENABLED() && IsDBCSLeadByte(psz[cb])) {
            cb++;
        } else if (IsCharLowerA(psz[cb]) &&
                   /*
                    * Sometime, LATIN_xxxx code is DBCS LeadingByte depending on ACP.
                    * In that case, we never come here...
                    */
                   (psz[cb] != LATIN_SMALL_LETTER_SHARP_S) &&
                   (psz[cb] != LATIN_SMALL_LETTER_Y_DIAERESIS)) {
            psz[cb] += 'A'-'a';
        }
#else
        if (IsCharLowerA(psz[cb]) &&
            (psz[cb] != LATIN_SMALL_LETTER_SHARP_S) &&
            (psz[cb] != LATIN_SMALL_LETTER_Y_DIAERESIS)) {
            psz[cb] += 'A'-'a';
        }
#endif // FE_SB
    }

    return nLength;
}


/***************************************************************************\
* IsCharLowerA (API)
*
* History:
* 14-Jan-1991 mikeke from win 3.0
* 22-Jun-1991 GregoryW   Modified to support code page 1252 (Windows ANSI
*                        code page).  This is for the PDK only.  After the
*                        PDK this routine will be rewritten to use the
*                        NLSAPI.
* 02-Feb-1992 GregoryW   Modified to use NLS API.
\***************************************************************************/

BOOL WINAPI IsCharLowerA(
    char cChar)
{
    WORD ctype1info = 0;
    WCHAR wChar = 0;

#ifdef FE_SB // IsCharLowerA()
    /*
     * if only DBCS Leadbyte was passed, just return FALSE.
     * Same behavior as Windows 3.1J and Windows 95 FarEast version.
     */
    if (IS_DBCS_ENABLED() && IsDBCSLeadByte(cChar)) {
        return FALSE;
    }
#endif // FE_SB

    /*
     * The following 2 calls cannot fail here
     */
    RtlMultiByteToUnicodeN(&wChar, sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
    GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info);
    return (ctype1info & C1_LOWER) == C1_LOWER;
}


/***************************************************************************\
* IsCharUpperA (API)
*
* History:
* 22-Jun-1991 GregoryW   Created to support code page 1252 (Windows ANSI
*                        code page).  This is for the PDK only.  After the
*                        PDK this routine will be rewritten to use the
*                        NLSAPI.
* 02-Feb-1992 GregoryW   Modified to use NLS API.
\***************************************************************************/

BOOL WINAPI IsCharUpperA(
    char cChar)
{
    WORD ctype1info = 0;
    WCHAR wChar = 0;

#ifdef FE_SB // IsCharUpperA()
    /*
     * if only DBCS Leadbyte was passed, just return FALSE.
     * Same behavior as Windows 3.1J and Windows 95 FarEast version.
     */
    if (IS_DBCS_ENABLED() && IsDBCSLeadByte(cChar)) {
        return FALSE;
    }
#endif // FE_SB

    /*
     * The following 2 calls cannot fail here
     */
    RtlMultiByteToUnicodeN(&wChar, sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
    GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info);
    return (ctype1info & C1_UPPER) == C1_UPPER;
}


/***************************************************************************\
* IsCharAlphaNumericA (API)
*
* Returns TRUE if character is alphabetical or numerical, otherwise FALSE
*
* History:
* 12-03-90 IanJa        Created non-NLS stub version.
* 06-22-91 GregoryW     Modified to support code page 1252 (Windows ANSI
*                       code page).  This is for the PDK only.  After the
*                       PDK this routine will be rewritten to use the
*                       NLSAPI.
* 02-20-92 GregoryW     Modified to use the NLS API.
\***************************************************************************/

BOOL WINAPI IsCharAlphaNumericA(
    char cChar)
{
    WORD ctype1info = 0;
    WCHAR wChar = 0;

    /*
     * The following 2 calls cannot fail here
     */
    RtlMultiByteToUnicodeN(&wChar, sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
    GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info);
#ifdef FE_SB // IsCharAlphaNumericA()
    if (ctype1info & C1_ALPHA) {
        WORD ctype3info = 0;
        if (!IS_DBCS_ENABLED()) {
            return TRUE;
        }
        /*
         * We don't want to return TRUE for halfwidth katakana.
         * Katakana is linguistic character (C1_ALPHA), but it is not
         * alphabet character.
         */
        GetStringTypeW(CT_CTYPE3, &wChar, 1, &ctype3info);
        return ((ctype3info & (C3_KATAKANA|C3_HIRAGANA)) ? FALSE : TRUE);
    }
    /* Otherwise, it might be digits ? */
    return !!(ctype1info & C1_DIGIT);
#else
    return (ctype1info & C1_ALPHA) || (ctype1info & C1_DIGIT);
#endif // FE_SB
}


/***************************************************************************\
* IsCharAlphaA (API)
*
* Returns TRUE if character is alphabetical, otherwise FALSE
*
* History:
* 06-22-91 GregoryW     Created to support code page 1252 (Windows ANSI
*                       code page).  This is for the PDK only.  After the
*                       PDK this routine will be rewritten to use the
*                       NLSAPI.
* 02-20-92 GregoryW     Modified to use the NLS API.
\***************************************************************************/

BOOL WINAPI IsCharAlphaA(
    char cChar)
{
    WORD ctype1info = 0;
    WCHAR wChar = 0;

    /*
     * The following 2 calls cannot fail here
     */
    RtlMultiByteToUnicodeN(&wChar, sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
    GetStringTypeW(CT_CTYPE1, &wChar, 1, &ctype1info);
#ifdef FE_SB // IsCharAlphaA()
    if ((ctype1info & C1_ALPHA) == C1_ALPHA) {
        WORD ctype3info = 0;
        if (!IS_DBCS_ENABLED()) {
            return TRUE;
        }
        /*
         * We don't want to return TRUE for halfwidth katakana.
         * Katakana is linguistic character (C1_ALPHA), but it is not
         * alphabet character.
         */
        GetStringTypeW(CT_CTYPE3, &wChar, 1, &ctype3info);
        return ((ctype3info & (C3_KATAKANA|C3_HIRAGANA)) ? FALSE : TRUE);
    }
    return (FALSE);
#else
    return (ctype1info & C1_ALPHA) == C1_ALPHA;
#endif // FE_SB
}

