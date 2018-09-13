#ifndef WIN31
#define WIN31
#endif
#define UNICODE 1

#include "precomp.h"
#pragma  hdrstop


/*
 * Private Functions:
 * StrEndN      - Find the end of a string, but no more than n bytes
 *
 * Public functions:
 * StrNCmp      - Compare n characters
 * StrNCmpI     - Compare n characters, case insensitive
 * StrNCpy      - Copy n characters
 * StrCpyN      - Copy up to n bytes, don't end in LeadByte for DB char
 * StrRStr      - Reverse search for substring
 */


/*
 * StrEndN - Find the end of a string, but no more than n bytes
 * Assumes   lpStart points to start of null terminated string
 *           nBufSize is the maximum length
 * returns ptr to just after the last byte to be included
 */
LPWSTR StrEndNW(LPCWSTR lpStart, int nBufSize)
{
    LPCWSTR lpEnd;

    for (lpEnd = lpStart + nBufSize; *lpStart && lpStart < lpEnd; lpStart = CharNext(lpStart))
    {
        /* just getting to the end of the string */
        continue;
    }

    if (lpStart > lpEnd)
    {
      /* We can only get here if the last wchar before lpEnd was a lead byte
       */
      lpStart -= 2;
    }

    return((LPWSTR)lpStart);
}

LPSTR StrEndNA(LPCSTR lpStart, int nBufSize)
{
    LPCSTR lpEnd;

    for (lpEnd = lpStart + nBufSize; *lpStart && lpStart < lpEnd; lpStart = CharNextA(lpStart))
    {
        /* just getting to the end of the string */
        continue;
    }

    if (lpStart > lpEnd)
    {
        // We can only get here if the last byte before lpEnd was a lead byte
        lpStart -= 2;
    }

    return (LPSTR)lpStart;
}


/*
 * StrCpyN      - Copy up to N chars, don't end in LeadByte char
 *
 * Assumes   lpDest points to buffer of nBufSize bytes (including NULL)
 *           lpSource points to string to be copied.
 * returns   Number of bytes copied, NOT including NULL
 */
int Shell32_StrCpyNW(LPWSTR lpDest, LPWSTR lpSource, int nBufSize)
{
    LPWSTR lpEnd;
    WCHAR cHold;

    if (nBufSize < 0)
        return(nBufSize);

    lpEnd = StrEndNW(lpSource, nBufSize);
    cHold = *lpEnd;
    *lpEnd = WCHAR_NULL;
    lstrcpy(lpDest, lpSource);
    *lpEnd = cHold;

    return (int)(lpEnd - lpSource);
}

int Shell32_StrCpyNA(LPSTR lpDest, LPSTR lpSource, int nBufSize)
{
    LPSTR lpEnd;
    CHAR cHold;

    if (nBufSize < 0)
        return(nBufSize);

    lpEnd = StrEndNA(lpSource, nBufSize);
    cHold = *lpEnd;
    *lpEnd = '\0';
    lstrcpyA(lpDest, lpSource);
    *lpEnd = cHold;
    
    return (int)(lpEnd - lpSource);
}


/*
 * StrNCmp      - Compare n characters
 *
 * returns   See lstrcmp return values.
 */
int StrNCmpW(LPWSTR lpStr1, LPWSTR lpStr2, int nChar)
{
    WCHAR cHold1, cHold2;
    int i;
    LPWSTR lpsz1 = lpStr1, lpsz2 = lpStr2;

    for (i = 0; i < nChar; i++)
    {
        /* If we hit the end of either string before the given number
        * of bytes, just return the comparison
        */
        if (!*lpsz1 || !*lpsz2)
            return(wcscmp(lpStr1, lpStr2));

        lpsz1 = CharNextW(lpsz1);
        lpsz2 = CharNextW(lpsz2);
    }

    cHold1 = *lpsz1;
    cHold2 = *lpsz2;
    *lpsz1 = *lpsz2 = WCHAR_NULL;
    i = wcscmp(lpStr1, lpStr2);
    *lpsz1 = cHold1;
    *lpsz2 = cHold2;

    return(i);
}

int StrNCmpA(LPSTR lpStr1, LPSTR lpStr2, int nChar)
{
    CHAR cHold1, cHold2;
    int i;
    LPSTR lpsz1 = lpStr1, lpsz2 = lpStr2;

    for (i = 0; i < nChar; i++)
    {
        /* If we hit the end of either string before the given number
        * of bytes, just return the comparison
        */
        if (!*lpsz1 || !*lpsz2)
            return(lstrcmpA(lpStr1, lpStr2));

        lpsz1 = CharNextA(lpsz1);
        lpsz2 = CharNextA(lpsz2);
    }

    cHold1 = *lpsz1;
    cHold2 = *lpsz2;
    *lpsz1 = *lpsz2 = '\0';
    i = lstrcmpA(lpStr1, lpStr2);
    *lpsz1 = cHold1;
    *lpsz2 = cHold2;

    return i;
}


/*
 * StrNCmpI     - Compare n characters, case insensitive
 *
 * returns   See lstrcmpi return values.
 */
int StrNCmpIW(LPWSTR lpStr1, LPWSTR lpStr2, int nChar)
{
    WCHAR cHold1, cHold2;
    int i;
    LPWSTR lpsz1 = lpStr1, lpsz2 = lpStr2;

    for (i = 0; i < nChar; i++)
    {
        /* If we hit the end of either string before the given number
        * of bytes, just return the comparison
        */
        if (!*lpsz1 || !*lpsz2)
            return(lstrcmpi(lpStr1, lpStr2));

        lpsz1 = CharNext(lpsz1);
        lpsz2 = CharNext(lpsz2);
    }

    cHold1 = *lpsz1;
    cHold2 = *lpsz2;
    *lpsz1 = *lpsz2 = WCHAR_NULL;
    i = _wcsicmp(lpStr1, lpStr2);
    *lpsz1 = cHold1;
    *lpsz2 = cHold2;

    return i;
}

int StrNCmpIA(LPSTR lpStr1, LPSTR lpStr2, int nChar)
{
    CHAR cHold1, cHold2;
    int i;
    LPSTR lpsz1 = lpStr1, lpsz2 = lpStr2;

    for (i = 0; i < nChar; i++)
    {
        /* If we hit the end of either string before the given number
        * of bytes, just return the comparison
        */
        if (!*lpsz1 || !*lpsz2)
            return(lstrcmpiA(lpStr1, lpStr2));

        lpsz1 = CharNextA(lpsz1);
        lpsz2 = CharNextA(lpsz2);
    }

    cHold1 = *lpsz1;
    cHold2 = *lpsz2;
    *lpsz1 = *lpsz2 = '\0';
    i = lstrcmpiA(lpStr1, lpStr2);
    *lpsz1 = cHold1;
    *lpsz2 = cHold2;

    return i;
}


/*
 * StrNCpy      - Copy n characters
 *
 * returns   Actual number of characters copied
 */
int StrNCpyW(LPWSTR lpDest, LPWSTR lpSource, int nChar)
{
    WCHAR cHold;
    int i;
    LPWSTR lpch = lpSource;

    if (nChar < 0)
        return(nChar);

    for (i = 0; i < nChar; i++)
    {
        if (!*lpch)
            break;

        lpch = CharNext(lpch);
    }

    cHold = *lpch;
    *lpch = WCHAR_NULL;
    wcscpy(lpDest, lpSource);
    *lpch = cHold;

    return i;
}

int StrNCpyA(LPSTR lpDest, LPSTR lpSource,int nChar)
{
    CHAR cHold;
    int i;
    LPSTR lpch = lpSource;

    if (nChar < 0)
        return(nChar);

    for (i = 0; i < nChar; i++)
    {
        if (!*lpch)
            break;

        lpch = CharNextA(lpch);
    }

    cHold = *lpch;
    *lpch = '\0';
    lstrcpyA(lpDest, lpSource);
    *lpch = cHold;
    
    return i;
}


/*
 * StrRStr      - Search for last occurrence of a substring
 *
 * Assumes   lpSource points to the null terminated source string
 *           lpLast points to where to search from in the source string
 *           lpLast is not included in the search
 *           lpSrch points to string to search for
 * returns   last occurrence of string if successful; NULL otherwise
 */
LPWSTR StrRStrW(LPWSTR lpSource, LPWSTR lpLast, LPWSTR lpSrch)
{
    int iLen;

    iLen = lstrlen(lpSrch);

    if (!lpLast)
    {
        lpLast = lpSource + lstrlen(lpSource);
    }

    do
    {
        /* Return NULL if we hit the exact beginning of the string
        */
        if (lpLast == lpSource)
            return(NULL);

        --lpLast;

        /* Break if we hit the beginning of the string
        */
        if (!lpLast)
            break;

        /* Break if we found the string, and its first byte is not a tail byte
        */
        if (!StrCmpNW(lpLast, lpSrch, iLen) && (lpLast==StrEndNW(lpSource, (int)(lpLast-lpSource))))
            break;
    }
    while (1);

    return lpLast;
}

LPSTR StrRStrA(LPSTR lpSource, LPSTR lpLast, LPSTR lpSrch)
{
    int iLen;

    iLen = lstrlenA(lpSrch);

    if (!lpLast)
    {
        lpLast = lpSource + lstrlenA(lpSource);
    }

    do
    {
        /* Return NULL if we hit the exact beginning of the string
        */
        if (lpLast == lpSource)
            return(NULL);

        --lpLast;

        /* Break if we hit the beginning of the string
        */
        if (!lpLast)
            break;

        /* Break if we found the string, and its first byte is not a tail byte
        */
        if (!StrCmpNA(lpLast, lpSrch, iLen) &&(lpLast==StrEndNA(lpSource, (int)(lpLast-lpSource))))
        {
            break;
        }
    }
    while (1);

    return lpLast;
}


