/**************************** Module Header ********************************\
* Module Name: oemxlate.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* ANSI/UNICODE(U+00--) to/from OEM translation routines for CP 437
*
* The goal of this module is to translate strings from ANSI/U+00-- to Oem
* character set or the opposite. If there is no equivalent character
* we use the followings rules:
*
*  1) we put a similar character (e.g. character without accent)
*  2) In OemToChar, graphics vertical, horizontal, and junction characters
*     are usually translated to '|', '-', and '+' characters, as appropriate,
*     unless the ANSI set is expanded to include such graphics.
*  3) Otherwise we put underscore "_".
*
* History:
* IanJa 4/10/91  from Win3.1 \\pucus\win31ro!drivers\keyboard\xlat*.*
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* CharToOemA
*
* CharToOemA(pSrc, pDst) - Translates the ANSI string at pSrc into
* the OEM string at pDst.  pSrc == pDst is legal.
* Always returns TRUE
*
\***************************************************************************/
BOOL WINAPI CharToOemA(
    LPCSTR pSrc,
    LPSTR pDst)
{
    UserAssert(gpsi);

    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    }

    do {
        *pDst++ = gpsi->acAnsiToOem[(UCHAR)*pSrc];
    } while (*pSrc++);

    return TRUE;
}

/***************************************************************************\
* CharToOemBuffA
*
* CharToOemBuffA(pSrc, pDst, nLength) - Translates nLength characters from
* the ANSI string at pSrc into OEM characters in the buffer at pDst.
* pSrc == pDst is legal.
*
* History:
\***************************************************************************/
BOOL WINAPI CharToOemBuffA(
    LPCSTR pSrc,
    LPSTR pDst,
    DWORD nLength)
{
    UserAssert(gpsi);

    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    }

    while (nLength--) {
        *pDst++ = gpsi->acAnsiToOem[(UCHAR)*pSrc++];
    }

    return TRUE;
}


/***************************************************************************\
* OemToCharA
*
* OemToCharA(pSrc, pDst) - Translates the OEM string at pSrc into
* the ANSI string at pDst.  pSrc == pDst is legal.
*
* Always returns TRUE
*
* History:
\***************************************************************************/
BOOL WINAPI OemToCharA(
    LPCSTR pSrc,
    LPSTR pDst)
{
    UserAssert(gpsi);

    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    }

    do {
        *pDst++ = gpsi->acOemToAnsi[(UCHAR)*pSrc];
    } while (*pSrc++);

    return TRUE;
}


/***************************************************************************\
* OemToCharBuffA
*
* OemToCharBuffA(pSrc, pDst, nLength) - Translates nLength OEM characters from
* the buffer at pSrc into ANSI characters in the buffer at pDst.
* pSrc == pDst is legal.
*
* Always returns TRUE
*
* History:
\***************************************************************************/
BOOL WINAPI OemToCharBuffA(
    LPCSTR pSrc,
    LPSTR pDst,
    DWORD nLength)
{
    UserAssert(gpsi);

    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    }

    while (nLength--) {
        *pDst++ = gpsi->acOemToAnsi[(UCHAR)*pSrc++];
    }

    return TRUE;
}


/***************************************************************************\
* CharToOemW
*
* CharToOemW(pSrc, pDst) - Translates the Unicode string at pSrc into
* the OEM string at pDst.  pSrc == pDst is legal.
*
* History:
\***************************************************************************/
BOOL WINAPI CharToOemW(
    LPCWSTR pSrc,
    LPSTR pDst)
{
    int cch;
    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    } else if (pSrc == (LPCWSTR)pDst) {
        /*
         * WideCharToMultiByte() requires pSrc != pDst: fail this call.
         * LATER: Is this really true?
         */
        return FALSE;
    }

    cch = wcslen(pSrc) + 1;

    WideCharToMultiByte(
            CP_OEMCP,           // Unicode -> OEM
            0,                  // gives best visual match
            (LPWSTR)pSrc, cch,  // source & length
            pDst,               // dest
            cch * 2,            // max poss.length (DBCS may * 2)
            "_",                // default char
            NULL);              // (don't care whether defaulted)

    return TRUE;
}

/***************************************************************************\
* CharToOemBuffW
*
* CharToOemBuffW(pSrc, pDst, nLength) - Translates nLength characters from
* the Unicode string at pSrc into OEM characters in the buffer at pDst.
* pSrc == pDst is legal.
*
* History:
\***************************************************************************/
BOOL WINAPI CharToOemBuffW(
    LPCWSTR pSrc,
    LPSTR pDst,
    DWORD nLength)
{
    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    } else if (pSrc == (LPCWSTR)pDst) {
        /*
         * WideCharToMultiByte() requires pSrc != pDst: fail this call.
         * LATER: Is this really true?
         */
        return FALSE;
    }

    WideCharToMultiByte(
            CP_OEMCP,                   // Unicode -> OEM
            0,                          // gives best visual match
            (LPWSTR)pSrc, (int)nLength, // source & length
            pDst,                       // dest
            (int)nLength * 2,           // max poss. length (DBCS may * 2)
            "_",                        // default char
            NULL);                      // (don't care whether defaulted)

    return TRUE;
}

/***************************************************************************\
* OemToCharW
*
* OemToCharW(pSrc, pDst) - Translates the OEM string at pSrc into
* the Unicode string at pDst.  pSrc == pDst is not legal.
*
* History:
\***************************************************************************/
BOOL WINAPI OemToCharW(
    LPCSTR pSrc,
    LPWSTR pDst)
{
    int cch;
    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    } else if (pSrc == (LPCSTR)pDst) {
        /*
         * MultiByteToWideChar() requires pSrc != pDst: fail this call.
         * LATER: Is this really true?
         */
        return FALSE;
    }

    cch = strlen(pSrc) + 1;

    MultiByteToWideChar(
            CP_OEMCP,                          // Unicode -> OEM
            MB_PRECOMPOSED | MB_USEGLYPHCHARS, // visual map to precomposed
            (LPSTR)pSrc, cch,                  // source & length
            pDst,                              // destination
            cch);                              // max poss. precomposed length

    return TRUE;
}

/***************************************************************************\
* OemToCharBuffW
*
* OemToCharBuffW(pSrc, pDst, nLength) - Translates nLength OEM characters from
* the buffer at pSrc into Unicode characters in the buffer at pDst.
* pSrc == pDst is not legal.
*
* History:
\***************************************************************************/
BOOL WINAPI OemToCharBuffW(
    LPCSTR pSrc,
    LPWSTR pDst,
    DWORD nLength)
{
    if (pSrc == NULL || pDst == NULL) {
        return FALSE;
    } else if (pSrc == (LPCSTR)pDst) {
        /*
         * MultiByteToWideChar() requires pSrc != pDst: fail this call.
         * LATER: Is this really true?
         */
        return FALSE;
    }

    MultiByteToWideChar(
            CP_OEMCP,                          // Unicode -> OEM
            MB_PRECOMPOSED | MB_USEGLYPHCHARS, // visual map to precomposed
            (LPSTR)pSrc, nLength,              // source & length
            pDst,                              // destination
            nLength);                          // max poss. precomposed length

    return TRUE;
}

/***************************************************************************\
* OemKeyScan (API)
*
* Converts an OEM character into a scancode plus shift state, returning
* scancode in low byte, shift state in high byte.
*
* Returns -1 on error.
*
\***************************************************************************/

DWORD WINAPI OemKeyScan(
    WORD wOemChar)
{
    WCHAR wchOem;
    SHORT sVk;
    UINT dwRet;

#ifdef FE_SB // OemKeyScan()
    /*
     * Return 0xFFFFFFFF for DBCS LeadByte character.
     */
    if (IsDBCSLeadByte(LOBYTE(wOemChar))) {
        return 0xFFFFFFFF;
    }
#endif // FE_SB

    if (!OemToCharBuffW((LPCSTR)&wOemChar, &wchOem, 1)) {
        return 0xFFFFFFFF;
    }

    sVk = VkKeyScanW(wchOem);
    if ((dwRet = MapVirtualKeyW(LOBYTE(sVk), 0)) == 0) {
        return 0xFFFFFFFF;
    }
    return dwRet | ((sVk & 0xFF00) << 8);
}
