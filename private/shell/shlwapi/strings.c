//============================================================================
//
// DBCS and UNICODE aware string routines
//
//
//============================================================================

#include "priv.h"
#include "ids.h"
#include <winnlsp.h>    // Get private NORM_ flag for StrEqIntl()
#include <shlobj.h>     // STRRET LPCITEMIDLIST
#include <w95wraps.h>

#include <mluisupp.h>


#define IS_DIGITA(ch)    InRange(ch, '0', '9')
#define IS_DIGITW(ch)    InRange(ch, L'0', L'9')


#define DM_INTERVAL 0

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else 
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif

__inline BOOL IsAsciiA(char ch)
{
    return !(ch & 0x80);
}

__inline BOOL IsAsciiW(WCHAR ch)
{
    return ch < 128;
}

__inline char Ascii_ToLowerA(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
}

__inline WCHAR Ascii_ToLowerW(WCHAR ch)
{
    return (ch >= L'A' && ch <= L'Z') ? (ch - L'A' + L'a') : ch;
}


// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand thid


/*
 * StrEndN - Find the end of a string, but no more than n bytes
 * Assumes   lpStart points to start of null terminated string
 *           nBufSize is the maximum length
 * returns ptr to just after the last byte to be included
 */
LPSTR
lstrfns_StrEndNA(
    LPCSTR lpStart,
    int nBufSize)
{
    LPCSTR lpEnd;

    for (lpEnd = lpStart + nBufSize; *lpStart && lpStart < lpEnd;
         lpStart = AnsiNext(lpStart))
        continue;   /* just getting to the end of the string */

    if (lpStart > lpEnd)
    {
        /* We can only get here if the last byte before lpEnd was a lead byte
        */
        lpStart -= 2;
    }
    return((LPSTR)lpStart);
}


LPWSTR lstrfns_StrEndNW(LPCWSTR lpStart, int nBufSize)
{
    LPCWSTR lpEnd;

    for (lpEnd = lpStart + nBufSize; *lpStart && (lpStart < lpEnd);
         lpStart++)
        continue;   /* just getting to the end of the string */

    return((LPWSTR)lpStart);
}


/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
__inline BOOL ChrCmpA_inline(WORD w1, WORD wMatch)
{
    /* Most of the time this won't match, so test it first for speed.
    */
    if (LOBYTE(w1) == LOBYTE(wMatch))
    {
        if (IsDBCSLeadByte(LOBYTE(w1)))
        {
            return(w1 != wMatch);
        }
        return FALSE;
    }
    return TRUE;
}


__inline BOOL ChrCmpW_inline(WCHAR w1, WCHAR wMatch)
{
    return(!(w1 == wMatch));
}


/*
 * ChrCmpI - Case insensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared;
 *           HIBYTE of wMatch is 0 if not a DBC
 * Return    FALSE if match, TRUE if not
 */
BOOL ChrCmpIA(WORD w1, WORD wMatch)
{
    char sz1[3], sz2[3];

    if (IsDBCSLeadByte(sz1[0] = LOBYTE(w1)))
    {
        sz1[1] = HIBYTE(w1);
        sz1[2] = '\0';
    }
    else
        sz1[1] = '\0';

#if defined(MWBIG_ENDIAN)
    sz2[0] = LOBYTE(wMatch);
    sz2[1] = HIBYTE(wMatch);
#else
    *(WORD *)sz2 = wMatch;
#endif
    sz2[2] = '\0';
    return lstrcmpiA(sz1, sz2);
}

BOOL ChrCmpIW(WCHAR w1, WCHAR wMatch)
{
    WCHAR sz1[2], sz2[2];

    sz1[0] = w1;
    sz1[1] = '\0';
    sz2[0] = wMatch;
    sz2[1] = '\0';

    return StrCmpIW(sz1, sz2);
}

LPWSTR StrCpyW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    LPWSTR psz = pszDst;

    ASSERT(pszDst);
    ASSERT(pszSrc);

    while (*pszDst++ = *pszSrc++)
        ;

    return psz;
}

//***   StrCpyNX[AW] -- just like StrCpyN[AW], but returns ptr to EOS
// NOTES
//  do we really need 'A' version?  (for now we do for shell32 on 'old'
//  platforms that we test on but don't ship)
LPSTR StrCpyNXA(LPSTR pszDst, LPCSTR pszSrc, int cchMax)
{
    RIPMSG(cchMax >= 0 && pszDst && IS_VALID_WRITE_BUFFER(pszDst, char, cchMax), "StrCpyNXA: Caller passed bad pszDst");
    RIPMSG(pszSrc && IS_VALID_STRING_PTRA(pszSrc, -1), "StrCpyNXA: Caller passed bad pszSrc");

    // NOTE: Cannot use DEBUGWhackPathBuffer before copying because src and
    // dest might overlap.  Must delay whacking until after we're done.

    if (0 < cchMax)
    {
        if (!pszSrc)
            goto NullItOut;

        // Leave room for the null terminator
        while (0 < --cchMax)
        {
            if (!(*pszDst++ = *pszSrc++))
            {
                --pszDst;
                break;
            }
        }

        cchMax++;
        // in the cchMax>1 case, pszDst already points at the NULL, but reassigning it doesn't hurt
NullItOut:
        // Whack the unused part of the buffer
        DEBUGWhackPathBufferA(pszDst, cchMax);
        *pszDst = '\0';
    }

    return pszDst;
}

LPWSTR StrCpyNXW(LPWSTR pszDst, LPCWSTR pszSrc, int cchMax)
{
    RIPMSG(cchMax >= 0 && pszDst && IS_VALID_WRITE_BUFFER(pszDst, WCHAR, cchMax), "StrCpyNXW: Caller passed bad pszDst");
    RIPMSG(pszSrc && IS_VALID_STRING_PTRW(pszSrc, -1), "StrCpyNXW: Caller passed bad pszSrc");

    // NOTE: Cannot use DEBUGWhackPathBuffer before copying because src and
    // dest might overlap.  Must delay whacking until after we're done.


    if (0 < cchMax)
    {
        if (!pszSrc) // a test app passed in a NULL src ptr and we faulted, let's not fault here.
            goto NullItOut;

        // Leave room for the null terminator
        while (0 < --cchMax)
        {
            if (!(*pszDst++ = *pszSrc++))
            {
                --pszDst;
                break;
            }
        }

        cchMax++;
        // in the cchMax>1 case, pszDst already points at the NULL, but reassigning it doesn't hurt
NullItOut:
        // Whack the unused part of the buffer
        DEBUGWhackPathBufferW(pszDst, cchMax);
        *pszDst = L'\0';
    }

    return pszDst;
}

LPWSTR StrCpyNW(LPWSTR pszDst, LPCWSTR pszSrc, int cchMax)
{
    StrCpyNXW(pszDst, pszSrc, cchMax);
    return pszDst;
}


LPWSTR StrCatW(LPWSTR pszDst, LPCWSTR pszSrc)
{
    LPWSTR psz = pszDst;

    ASSERT(pszDst);
    ASSERT(pszSrc);

    while (0 != *pszDst)
        pszDst++;

    while (*pszDst++ = *pszSrc++)
        ;

    return psz;
}

LWSTDAPI_(LPWSTR) StrCatBuffW(LPWSTR pszDest, LPCWSTR pszSrc, int cchDestBuffSize)
{
    LPWSTR psz;

    ASSERT(pszDest);
    ASSERT(pszSrc);
    ASSERT(cchDestBuffSize >= 0);
    VDATEINPUTBUF(pszDest, TCHAR, cchDestBuffSize);

    psz = pszDest;

    // we walk forward till we find the end of pszDest, subtracting
    // from cchDestBuffSize as we go.
    while (*psz)
    {
        psz++;
        cchDestBuffSize--;
    }

    if (cchDestBuffSize <= 0)
    {
        // check to make sure that the pszDest hadn't already overflowed its buffer!
        ASSERT(cchDestBuffSize == 0);
        return pszDest;
    }

    // call the shlwapi function here because win95 does not have lstrcpynW
    StrCpyNW(psz, pszSrc, cchDestBuffSize);

    return pszDest;
}

 
LWSTDAPI_(LPSTR) StrCatBuffA(LPSTR pszDest, LPCSTR pszSrc, int cchDestBuffSize)
{
    LPSTR psz;
   
    ASSERT(pszDest);
    ASSERT(pszSrc);
    ASSERT(cchDestBuffSize >= 0);
    VDATEINPUTBUF(pszDest, CHAR, cchDestBuffSize);

    psz = pszDest;
    
    // we walk forward till we find the end of pszDest, subtracting
    // from cchDestBuffSize as we go.
    while (*psz)
    {
        psz++;
        cchDestBuffSize--;
    }

    if (cchDestBuffSize <= 0)
    {
        // check to make sure that the pszDest hadn't already overflowed its buffer!
        ASSERT(cchDestBuffSize == 0);
        return pszDest;
    }

    // Let kernel do the work for us. 
    //
    // WARNING: We might generate a truncated DBCS sting becuase kernel's lstrcpynA
    // dosent check for this. Ask me if I care.
    lstrcpynA(psz, pszSrc, cchDestBuffSize);

    return pszDest;
}
   

/* StrNCat(front, back, count) - append count chars of back onto front
 */
LPSTR StrNCatA(LPSTR front, LPCSTR back, int cchMax)
{
    LPSTR start = front;

    while (*front++)
                ;
    front--;

    lstrcpyn(front, back, cchMax);

    return(start);    
}

LPWSTR StrNCatW(LPWSTR front, LPCWSTR back, int cchMax)
{
    LPWSTR start = front;

    while (*front++)
                ;
    front--;

    StrCpyNW(front, back, cchMax);
    
    return(start);    
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR _StrChrA(LPCSTR lpStart, WORD wMatch, BOOL fMBCS)
{
    if (fMBCS) {
        for ( ; *lpStart; lpStart = AnsiNext(lpStart))
        {
            if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
                return((LPSTR)lpStart);
        }
    } else {
        for ( ; *lpStart; lpStart++)
        {
            if ((BYTE)*lpStart == LOBYTE(wMatch)) {
                return((LPSTR)lpStart);
            }
        }
    }
    return (NULL);
}

LPSTR StrChrA(LPCSTR lpStart, WORD wMatch)
{
    CPINFO cpinfo;
    return _StrChrA(lpStart, wMatch, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}

#ifdef ALIGNMENT_SCENARIO

LPWSTR StrChrSlowW(const UNALIGNED WCHAR *lpStart, WCHAR wMatch)
{
    for ( ; *lpStart; lpStart++)
    {
      if (!ChrCmpW_inline(*lpStart, wMatch))
        {
            return((LPWSTR)lpStart);
        }
    }
}
#endif

LPWSTR StrChrW(LPCWSTR lpStart, WCHAR wMatch)
{
    //
    //  BUGBUG raymondc
    //  Apparently, somebody is passing unaligned strings to StrChrW.
    //  Find out who and make them stop.
    //
    ASSERT(!((ULONG_PTR)lpStart & 1)); // Assert alignedness

#ifdef ALIGNMENT_SCENARIO
    //
    //  Since unaligned strings arrive so rarely, put the slow
    //  version in a separate function so the common case stays
    //  fast.  Believe it or not, we call StrChrW so often that
    //  it is now a performance-sensitive function!
    //
    if ((ULONG_PTR)lpStart & 1)
        return StrChrSlowW(lpStart, wMatch);
#endif

    for ( ; *lpStart; lpStart++)
    {
      if (!ChrCmpW_inline(*lpStart, wMatch))
        {
            return((LPWSTR)lpStart);
        }
    }
    return (NULL);
}

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    ASSERT(lpStart);
    ASSERT(!lpEnd || lpEnd <= lpStart + lstrlenA(lpStart));

    if (!lpEnd)
        lpEnd = lpStart + lstrlenA(lpStart);

    for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
        // (ChrCmp returns FALSE when characters match)

        if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}

LPWSTR StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch)
{
    LPCWSTR lpFound = NULL;

    if (!lpEnd)
        lpEnd = lpStart + lstrlenW(lpStart);

    for ( ; lpStart < lpEnd; lpStart++)
    {
        if (!ChrCmpW_inline(*lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPWSTR)lpFound);
}

/*
 * StrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR StrChrIA(LPCSTR lpStart, WORD wMatch)
{
    wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

    for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
        if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
            return((LPSTR)lpStart);
    }
    return (NULL);
}

LPWSTR StrChrIW(LPCWSTR lpStart, WCHAR wMatch)
{
    for ( ; *lpStart; lpStart++)
    {
        if (!ChrCmpIW(*lpStart, wMatch))
            return((LPWSTR)lpStart);
    }
    return (NULL);
}

/*
 * StrRChrI - Find last occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChrIA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
    LPCSTR lpFound = NULL;

    ASSERT(lpStart);
    ASSERT(!lpEnd || lpEnd <= lpStart + lstrlenA(lpStart));

    if (!lpEnd)
        lpEnd = lpStart + lstrlenA(lpStart);

    wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

    for ( ; lpStart < lpEnd; lpStart = AnsiNext(lpStart))
    {
        if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
            lpFound = lpStart;
    }
    return ((LPSTR)lpFound);
}


LPWSTR StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch)
{
    LPCWSTR lpFound = NULL;

    if (!lpEnd)
        lpEnd = lpStart + lstrlenW(lpStart);

    for ( ; lpStart < lpEnd; lpStart++)
    {
        if (!ChrCmpIW(*lpStart, wMatch))
            lpFound = lpStart;
    }
    return ((LPWSTR)lpFound);
}


/*----------------------------------------------------------
Purpose: Returns a pointer to the first occurrence of a character
         in psz that belongs to the set of characters in pszSet.
         The search does not include the null terminator.

         If psz contains no characters that are in the set of
         characters in pszSet, this function returns NULL.

         This function is DBCS-safe.

Returns: see above
Cond:    --
*/
LPSTR
WINAPI
StrPBrkA(LPCSTR psz, LPCSTR pszSet)
{
    LPCSTR pszSetT;

    ASSERT(psz);
    ASSERT(pszSet);

    while (*psz)
    {
        for (pszSetT = pszSet; *pszSetT; pszSetT = CharNextA(pszSetT))
        {
            if (*psz == *pszSetT)
            {
                // Found first character that matches
                return (LPSTR)psz;      // Const -> non-const
            }
        }
        psz = CharNextA(psz);
    }

    return NULL;
}


/*----------------------------------------------------------
Purpose: Returns a pointer to the first occurrence of a character
         in psz that belongs to the set of characters in pszSet.
         The search does not include the null terminator.

Returns: see above
Cond:    --
*/
LPWSTR WINAPI StrPBrkW(LPCWSTR psz, LPCWSTR pszSet)
{
    LPCWSTR pszSetT;

    ASSERT(psz);
    ASSERT(pszSet);

    // Go thru the string to be inspected

    while (*psz)
    {
        // Go thru the char set

        for (pszSetT = pszSet; *pszSetT; pszSetT++)
        {
            if (*psz == *pszSetT)
            {
                // Found first character that matches
                return (LPWSTR)psz;     // Const -> non-const
            }
        }
        psz++;
    }

    return NULL;
}


int WINAPI StrToIntA(LPCSTR lpSrc)
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == '-') {
        bNeg = TRUE;
        lpSrc++;
    }

    while (IS_DIGITA(*lpSrc)) {
        n *= 10;
        n += *lpSrc - '0';
        lpSrc++;
    }
    return bNeg ? -n : n;
}


int WINAPI StrToIntW(LPCWSTR lpSrc)
{
    int n = 0;
    BOOL bNeg = FALSE;

    if (*lpSrc == L'-') {
        bNeg = TRUE;
        lpSrc++;
    }

    while (IS_DIGITW(*lpSrc)) {
        n *= 10;
        n += *lpSrc - L'0';
        lpSrc++;
    }
    return bNeg ? -n : n;
}


/*----------------------------------------------------------
Purpose: Special verion of atoi.  Supports hexadecimal too.

         If this function returns FALSE, *piRet is set to 0.

Returns: TRUE if the string is a number, or contains a partial number
         FALSE if the string is not a number

        dwFlags are STIF_ bitfield
Cond:    --
*/
BOOL WINAPI StrToIntExA(LPCSTR pszString, DWORD dwFlags, int *piRet)
{
    BOOL bRet;
    int n;
    BOOL bNeg = FALSE;
    LPCSTR psz;
    LPCSTR pszAdj;

    // Skip leading whitespace
    //
    for (psz = pszString; *psz == ' ' || *psz == '\n' || *psz == '\t'; psz = CharNextA(psz))
        ;

    // Determine possible explicit signage
    //
    if (*psz == '+' || *psz == '-')
    {
        bNeg = (*psz == '+') ? FALSE : TRUE;
        psz++;
    }

    // Or is this hexadecimal?
    //
    pszAdj = CharNextA(psz);
    if ((STIF_SUPPORT_HEX & dwFlags) &&
        *psz == '0' && (*pszAdj == 'x' || *pszAdj == 'X'))
    {
        // Yes

        // (Never allow negative sign with hexadecimal numbers)
        bNeg = FALSE;
        psz = CharNextA(pszAdj);

        pszAdj = psz;

        // Do the conversion
        //
        for (n = 0; ; psz = CharNextA(psz))
        {
            if (IS_DIGITA(*psz))
                n = 0x10 * n + *psz - '0';
            else
            {
                CHAR ch = *psz;
                int n2;

                if (ch >= 'a')
                    ch -= 'a' - 'A';

                n2 = ch - 'A' + 0xA;
                if (n2 >= 0xA && n2 <= 0xF)
                    n = 0x10 * n + n2;
                else
                    break;
            }
        }

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
    }
    else
    {
        // No
        pszAdj = psz;

        // Do the conversion
        for (n = 0; IS_DIGITA(*psz); psz = CharNextA(psz))
            n = 10 * n + *psz - '0';

        // Return TRUE if there was at least one digit
        bRet = (psz != pszAdj);
    }

    *piRet = bNeg ? -n : n;

    return bRet;
}


/*----------------------------------------------------------
 Purpose: Wide-char wrapper for StrToIntExA.

 Returns: see StrToIntExA
 Cond:    --
 */
BOOL WINAPI StrToIntExW(
    LPCWSTR   pwszString,
    DWORD     dwFlags,          // STIF_ bitfield
    int FAR * piRet)
{
    // Most strings will simply use this temporary buffer, but AnsiFromUnicode
    // will allocate a buffer if the supplied string is bigger.
    CHAR szBuf[MAX_PATH];

    LPSTR pszString;
    BOOL bRet = AnsiFromUnicode(&pszString, pwszString, szBuf, SIZECHARS(szBuf));

    if (bRet)
    {
        bRet = StrToIntExA(pszString, dwFlags, piRet);
        AnsiFromUnicode(&pszString, NULL, szBuf, 0);
    }
    return bRet;
}


/*----------------------------------------------------------
 Purpose: Returns an integer value specifying the length of
 the substring in psz that consists entirely of
 characters in pszSet.  If psz begins with a character
 not in pszSet, then this function returns 0.

 This is a DBCS-safe version of the CRT strspn().

 Returns: see above
 Cond:    --
 */
int
WINAPI
StrSpnA(
        LPCSTR psz,
        LPCSTR pszSet)
{
    LPCSTR pszT;
    LPCSTR pszSetT;

    // Go thru the string to be inspected

    for (pszT = psz; *pszT; pszT = CharNextA(pszT))
    {
        // Go thru the char set

        for (pszSetT = pszSet; *pszSetT; pszSetT = CharNextA(pszSetT))
        {
            if (*pszSetT == *pszT)
            {
                if ( !IsDBCSLeadByte(*pszSetT) )
                {
                    break;      // Chars match
                }
                else if (pszSetT[1] == pszT[1])
                {
                    break;      // Chars match
                }
            }
        }

        // End of char set?
        if (0 == *pszSetT)
        {
            break;      // Yes, no match on this inspected char
        }
    }

    return (int)(pszT - psz);
}


/*----------------------------------------------------------
 Purpose: Returns an integer value specifying the length of
 the substring in psz that consists entirely of
 characters in pszSet.  If psz begins with a character
 not in pszSet, then this function returns 0.

 This is a DBCS-safe version of the CRT strspn().

 Returns: see above
 Cond:    --
 */
STDAPI_(int) StrSpnW(LPCWSTR psz, LPCWSTR pszSet)
{
    LPCWSTR pszT;
    LPCWSTR pszSetT;

    ASSERT(psz);
    ASSERT(pszSet);

    // Go thru the string to be inspected

    for (pszT = psz; *pszT; pszT++)
    {
        // Go thru the char set

        for (pszSetT = pszSet; *pszSetT != *pszT; pszSetT++)
        {
            if (0 == *pszSetT)
            {
                // Reached end of char set without finding a match
                return (int)(pszT - psz);
            }
        }
    }

    return (int)(pszT - psz);
}


// StrCSpn: return index to first char of lpStr that is present in lpSet.
// Includes the NUL in the comparison; if no lpSet chars are found, returns
// the index to the NUL in lpStr.
// Just like CRT strcspn.
//
int StrCSpnA(LPCSTR lpStr, LPCSTR lpSet)
{
    // nature of the beast: O(lpStr*lpSet) work
    LPCSTR lp = lpStr;
    if (!lpStr || !lpSet)
        return 0;

    while (*lp)
    {
        if (StrChrA(lpSet, READNATIVEWORD(lp)))
            return (int)(lp-lpStr);
        lp = AnsiNext(lp);
    }

    return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

int StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet)
{
    // nature of the beast: O(lpStr*lpSet) work
    LPCWSTR lp = lpStr;
    if (!lpStr || !lpSet)
        return 0;

    while (*lp)
    {
        if (StrChrW(lpSet, *lp))
            return (int)(lp-lpStr);
        lp++;
    }

    return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

// StrCSpnI: case-insensitive version of StrCSpn.
//
int StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet)
{
    // nature of the beast: O(lpStr*lpSet) work
    LPCSTR lp = lpStr;
    if (!lpStr || !lpSet)
        return 0;

    while (*lp)
    {
        if (StrChrIA(lpSet, READNATIVEWORD(lp)))
            return (int)(lp-lpStr);
        lp = AnsiNext(lp);
    }

    return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

int StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet)
{
    // nature of the beast: O(lpStr*lpSet) work
    LPCWSTR lp = lpStr;
    if (!lpStr || !lpSet)
        return 0;

    while (*lp)
    {
        if (StrChrIW(lpSet, *lp))
            return (int)(lp-lpStr);
        lp++;
    }

    return (int)(lp-lpStr); // ==lstrlen(lpStr)
}


/*
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 */
int _StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar, BOOL fMBCS)
{
    LPCSTR lpszEnd = lpStr1 + nChar;
    char sz1[4];
    char sz2[4];

    if (fMBCS) {
        for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1 = AnsiNext(lpStr1), lpStr2 = AnsiNext(lpStr2)) {
            WORD w1;
            WORD w2;
    
            // If either pointer is at the null terminator already,
            // we want to copy just one byte to make sure we don't read 
            // past the buffer (might be at a page boundary).
    
            w1 = (*lpStr1) ? READNATIVEWORD(lpStr1) : 0;
            w2 = (*lpStr2) ? READNATIVEWORD(lpStr2) : 0;
    
            // (ChrCmpA returns FALSE if the characters match)
    
            // Do the characters match?
            if (ChrCmpA_inline(w1, w2)) 
            {
                // No; determine the lexical value of the comparison
                // (since ChrCmp just returns true/false).
    
                // Since the character may be a DBCS character; we
                // copy two bytes into each temporary buffer 
                // (in preparation for the lstrcmp call).
    
                (*(WORD *)sz1) = w1;
                (*(WORD *)sz2) = w2;
    
                // Add null terminators to temp buffers
                *AnsiNext(sz1) = 0;
                *AnsiNext(sz2) = 0;
                return lstrcmpA(sz1, sz2);
            }
        }
    } else {
        for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
            if (*lpStr1 != *lpStr2) {
                // No; determine the lexical value of the comparison
                // (since ChrCmp just returns true/false).
                sz1[0] = *lpStr1;
                sz2[0] = *lpStr2;
                sz1[1] = sz2[1] = '\0';
                return lstrcmpA(sz1, sz2);
            }
        }
    }

    return 0;
}

STDAPI_(int) StrCmpNA(LPCSTR psz1, LPCSTR psz2, int nChar)
{
    CPINFO cpinfo;
    return _StrCmpNA(psz1, psz2, nChar, GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0]);
}

// cch1 and cch2 are the maximum # of chars to compare

int _StrCmpLocaleW(DWORD dwFlags, LPCWSTR psz1, int cch1, LPCWSTR psz2, int cch2)
{
    int i = CompareStringW(GetThreadLocale(), dwFlags, psz1, cch1, psz2, cch2);
    if (!i)
    {
        i = CompareStringW(LOCALE_SYSTEM_DEFAULT, dwFlags, psz1, cch1, psz2, cch2);
    }
    return i - CSTR_EQUAL;
}

int _StrCmpLocaleA(DWORD dwFlags, LPCSTR psz1, int cch1, LPCSTR psz2, int cch2)
{
    int i = CompareStringA(GetThreadLocale(), dwFlags, psz1, cch1, psz2, cch2);
    if (!i)
    {
        i = CompareStringA(LOCALE_SYSTEM_DEFAULT, dwFlags, psz1, cch1, psz2, cch2);
    }
    return i - CSTR_EQUAL;
}


STDAPI_(int) StrCmpNW(LPCWSTR psz1, LPCWSTR psz2, int nChar)
{
    return _StrCmpLocaleW(NORM_STOP_ON_NULL, psz1, nChar, psz2, nChar);
}

/*
 * Compare n bytes, case insensitive
 *
 * returns   See lstrcmpi return values.
 */

int StrCmpNIA(LPCSTR psz1, LPCSTR psz2, int nChar)
{
    if (g_bRunningOnNT)
    {
        return _StrCmpLocaleA(NORM_IGNORECASE | NORM_STOP_ON_NULL,  psz1, nChar, psz2, nChar);
    }
    else
    {
        //  Win95 doesn't support NORM_STOP_ON_NULL
        int i;
        LPCSTR lpszEnd = psz1 + nChar;

        for ( ; (lpszEnd > psz1) && (*psz1 || *psz2); (psz1 = AnsiNext(psz1)), (psz2 = AnsiNext(psz2))) 
        {
            WORD w1, w2;

            // If either pointer is at the null terminator already,
            // we want to copy just one byte to make sure we don't read 
            // past the buffer (might be at a page boundary).

            if (IsAsciiA(*psz1) && IsAsciiA(*psz2))
            {
                i = Ascii_ToLowerA(*psz1) - Ascii_ToLowerA(*psz2);
            }
            else
            {
                w1 = (*psz1) ? READNATIVEWORD(psz1) : 0;
                w2 = (UINT)(IsDBCSLeadByte(*psz2)) ? (UINT)READNATIVEWORD(psz2) : (WORD)(BYTE)(*psz2);

                i = ChrCmpIA(w1, w2);
            }
            if (i)
            {
                if (i < 0)
                    return -1;
                else
                    return 1;
            }
        }
        return 0;
    }
}

int StrCmpNIW(LPCWSTR psz1, LPCWSTR psz2, int nChar)
{
    return _StrCmpLocaleW(NORM_IGNORECASE | NORM_STOP_ON_NULL, psz1, nChar, psz2, nChar);
}


/*
 * StrRStrI      - Search for last occurrence of a substring
 *
 * Assumes   lpSource points to the null terminated source string
 *           lpLast points to where to search from in the source string
 *           lpLast is not included in the search
 *           lpSrch points to string to search for
 * returns   last occurrence of string if successful; NULL otherwise
 */
LPSTR StrRStrIA(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch)
{
    LPCSTR lpFound = NULL;
    WORD   wMatch;
    UINT   uLen = 0;
    LPCSTR  lpStart = NULL;
    
    if (!lpLast)
        lpLast = lpSource + lstrlenA(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    wMatch = READNATIVEWORD(lpSrch);
    wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));
    
    uLen = lstrlenA(lpSrch);
    lpStart = lpSource;
    while (*lpStart && (lpStart < lpLast))
    {
        if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
        {   
            if (StrCmpNIA(lpStart, lpSrch, uLen) == 0)
                lpFound = lpStart;
        }   
        lpStart = AnsiNext(lpStart);
    }
    
    return((LPSTR)lpFound);
}

LPWSTR StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch)
{
    LPCWSTR lpFound = NULL;
    WCHAR   wMatch;
    UINT    uLen = 0;
    LPCWSTR  lpStart = NULL;

    if (!lpLast)
        lpLast = lpSource + lstrlenW(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    wMatch = *lpSrch;
    uLen = lstrlenW(lpSrch);
    lpStart = lpSource;
    while (*lpStart && (lpStart < lpLast))
    {
        if (!ChrCmpIW(*lpStart, wMatch))
        {   
            if (StrCmpNIW(lpStart, lpSrch, uLen) == 0)
                lpFound = lpStart;
        }   
        lpStart++;
    }
    
    return((LPWSTR)lpFound);
}

/*
 * StrStr      - Search for first occurrence of a substring
 *
 * Assumes   lpSource points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR StrStrA(LPCSTR lpFirst, LPCSTR lpSrch)
{
    UINT uLen;
    WORD wMatch;
    CPINFO cpinfo;
    BOOL fMBCS = GetCPInfo(CP_ACP, &cpinfo) && cpinfo.LeadByte[0];

    uLen = (UINT)lstrlenA(lpSrch);
    wMatch = READNATIVEWORD(lpSrch);

    for ( ; (lpFirst=_StrChrA(lpFirst, wMatch, fMBCS))!=0 && _StrCmpNA(lpFirst, lpSrch, uLen, fMBCS);
         lpFirst=AnsiNext(lpFirst))
        continue; /* continue until we hit the end of the string or get a match */

    return((LPSTR)lpFirst);
}

LPWSTR StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
    UINT uLen;
    WCHAR wMatch;

    if (!lpSrch || !lpFirst)
        return NULL;
		
    uLen = (UINT)lstrlenW(lpSrch);
    wMatch = *lpSrch;
	
    for ( ; (lpFirst=StrChrW(lpFirst, wMatch))!=0 && StrCmpNW(lpFirst, lpSrch, uLen);
         lpFirst++)
        continue; /* continue until we hit the end of the string or get a match */

    return (LPWSTR)lpFirst;
}

/*
 * StrStrI   - Search for first occurrence of a substring, case insensitive
 *
 * Assumes   lpFirst points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch)
{
    UINT uLen = (UINT)lstrlenA(lpSrch);
    WORD wMatch = READNATIVEWORD(lpSrch);

    for ( ; (lpFirst = StrChrIA(lpFirst, wMatch)) != 0 && StrCmpNIA(lpFirst, lpSrch, uLen);
         lpFirst=AnsiNext(lpFirst))
        continue; /* continue until we hit the end of the string or get a match */

    return (LPSTR)lpFirst;
}

LPWSTR StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
    UINT uLen = (UINT)lstrlenW(lpSrch);
    WCHAR wMatch = *lpSrch;

    for ( ; (lpFirst = StrChrIW(lpFirst, wMatch)) != 0 && StrCmpNIW(lpFirst, lpSrch, uLen);
         lpFirst++)
        continue; /* continue until we hit the end of the string or get a match */

    return (LPWSTR)lpFirst;
}

LPSTR StrDupA(LPCSTR psz)
{
    LPSTR pszRet = (LPSTR)LocalAlloc(LPTR, (lstrlenA(psz) + 1) * sizeof(*pszRet));
    if (pszRet) 
    {
        lstrcpyA(pszRet, psz);
    }
    return pszRet;
}

LPWSTR StrDupW(LPCWSTR psz)
{
    LPWSTR pszRet = (LPWSTR)LocalAlloc(LPTR, (lstrlenW(psz) + 1) * sizeof(*pszRet));
    if (pszRet) 
    {
        StrCpyW(pszRet, psz);
    }
    return pszRet;
}

void _StrOut(LPSTR* ppszBuf, HMODULE hmod, UINT idRes, DWORD* pdwTimeS, int* pdigits, UINT iDiv)
{
    if (*pdigits)
    {
        DWORD dwCur = *pdwTimeS/iDiv;

        if (dwCur || iDiv==1) 
        {
            DWORD dwBase;
            CHAR szBuf[64], szTemplate[64];
            LPSTR pszBuf = szBuf;

            *pdwTimeS -= dwCur*iDiv;
            for (dwBase=1; dwCur/(dwBase*10); dwBase*=10);

            DebugMsg(DM_INTERVAL, TEXT("dwCur, dwBase, *pdwTimeS = %d, %d, %d"), dwCur, dwBase, *pdwTimeS);

            //
            // LATER: We could use atoi if we mathematically trancate
            //  the numbers based on digits.
            //
            for (;dwBase; dwBase/=10, pszBuf++) 
            {
                if (*pdigits) 
                {
                    DWORD i = dwCur/dwBase;
                    dwCur -= i*dwBase;
                    *pszBuf = '0'+(unsigned short)i;
                    (*pdigits)--;
                } 
                else 
                {
                    *pszBuf = '0';
                }
            }
            *pszBuf = '\0';

            LoadStringA(hmod, idRes, szTemplate, ARRAYSIZE(szTemplate));
            wsprintfA(*ppszBuf, szTemplate, szBuf);
            (*ppszBuf) += lstrlenA(*ppszBuf);
        }
    }
}

BOOL _StrFromTimeInterval(LPSTR szBuf, DWORD dwTimeMS, int digits)
{
    DWORD dwTimeS = (dwTimeMS+500)/1000;
    LPSTR pszBuf = szBuf;
    DebugMsg(DM_INTERVAL, TEXT("dwTimeS = %d"), dwTimeS);

    szBuf = '\0';

    _StrOut(&pszBuf, g_hinst, IDS_HOUR, &dwTimeS, &digits, 3600);
    _StrOut(&pszBuf, g_hinst, IDS_MIN, &dwTimeS, &digits, 60);
    _StrOut(&pszBuf, g_hinst, IDS_SEC, &dwTimeS, &digits, 1);

    return TRUE;
}

//
//  This API converts a given time-interval (in msec) into a human readable
// string.
//
// Parameters:
//  pszOut   -- Specifies the string buffer. NULL is valid to query size.
//  cchMax   -- Specifies the size of buffer in char/WCHAR
//  dwTimeMS -- Specifies the time interval in msec
//  digits   -- Specifies the minimum number of digits to be displayed
//
// Returns:
//  Number of characters in the buffer (not including the terminator).
//
// Exmaples:
//  dwTimeMS digits     output
//   34000     3         34 sec
//   34000     2         34 sec
//   34000     1         30 sec
//   74000     3         1 min 14 sec
//   74000     2         1 min 10 sec
//   74000     1         1 min
//
int StrFromTimeIntervalA(LPSTR pszOut, UINT cchMax, DWORD dwTimeMS, int digits)
{
    CHAR szBuf[256];
    int cchRet = 0;
    if (_StrFromTimeInterval(szBuf, dwTimeMS, digits)) 
    {
        if (pszOut) 
        {
            lstrcpynA(pszOut, szBuf, cchMax);
            cchRet = lstrlenA(pszOut);
        }
        else 
        {
            cchRet = lstrlenA(szBuf);
        }
    }
    return cchRet;
}

int StrFromTimeIntervalW(LPWSTR pwszOut, UINT cchMax, DWORD dwTimeMS, int digits)
{
    CHAR szBuf[256];
    int cchRet = 0;
    if (_StrFromTimeInterval(szBuf, dwTimeMS, digits)) 
    {
        // - 1 because MultiByteToWideChar() also counts the NULL termination
        cchRet = MultiByteToWideChar(CP_ACP, 0, szBuf, -1, pwszOut, cchMax) - 1;
    }
    return cchRet;
}

/*
 * IntlStrEq
 *
 * returns TRUE if strings are equal, FALSE if not
 */
BOOL StrIsIntlEqualA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar) 
{
    DWORD dwFlags = fCaseSens ? LOCALE_USE_CP_ACP : (NORM_IGNORECASE | LOCALE_USE_CP_ACP);

    if (g_bRunningOnNT)
    {
        dwFlags |= NORM_STOP_ON_NULL;   // only supported on NT
    }
    else if (nChar != -1)
    {
        // On Win9x we have to do the check manually
        //
        int cch = 0;
        LPCSTR psz1 = lpString1;
        LPCSTR psz2 = lpString2;

        while(*psz1 != 0 && *psz2 != 0 && cch < nChar) 
        {
            psz1 = CharNextA(psz1);
            psz2 = CharNextA(psz2);
            cch = (int) min(psz1 - lpString1, psz2 - lpString2);
        }

        // add one in for terminating '\0'
        cch++;
        if (cch < nChar)
            nChar = cch;
    }
    return 0 == _StrCmpLocaleA(dwFlags, lpString1, nChar, lpString2, nChar);
}

BOOL StrIsIntlEqualW(BOOL fCaseSens, LPCWSTR psz1, LPCWSTR psz2, int nChar) 
{
    return 0 == _StrCmpLocaleW(fCaseSens ? NORM_STOP_ON_NULL : NORM_IGNORECASE | NORM_STOP_ON_NULL, 
        psz1, nChar, psz2, nChar);
}

// This is stolen from shell32 - util.c

#define LODWORD(_qw)    (DWORD)(_qw)

const short c_aOrders[] = {IDS_BYTES, IDS_ORDERKB, IDS_ORDERMB,
                          IDS_ORDERGB, IDS_ORDERTB, IDS_ORDERPB, IDS_ORDEREB};

void Int64ToStr(LONGLONG n, LPWSTR lpBuffer)
{
    WCHAR szTemp[40];
    LONGLONG  iChr;

    iChr = 0;

    do {
        szTemp[iChr++] = L'0' + (WCHAR)(n % 10);
        n = n / 10;
    } while (n != 0);

    do {
        iChr--;
        *lpBuffer++ = szTemp[iChr];
    } while (iChr != 0);

    *lpBuffer++ = L'\0';
}

//
//  Obtain NLS info about how numbers should be grouped.
//
//  The annoying thing is that LOCALE_SGROUPING and NUMBERFORMAT
//  have different ways of specifying number grouping.
//
//          LOCALE      NUMBERFMT      Sample   Country
//
//          3;0         3           1,234,567   United States
//          3;2;0       32          12,34,567   India
//          3           30           1234,567   ??
//
//  Not my idea.  That's the way it works.
//
//  Bonus treat - Win9x doesn't support complex number formats,
//  so we return only the first number.
//
UINT GetNLSGrouping(void)
{
    UINT grouping;
    LPWSTR psz;
    WCHAR szGrouping[32];

    // If no locale info, then assume Western style thousands
    if (!GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, szGrouping, ARRAYSIZE(szGrouping)))
        return 3;

    grouping = 0;
    psz = szGrouping;
    if (g_bRunningOnNT)
    {
        for (;;)
        {
            if (*psz == L'0') break;             // zero - stop

            else if ((UINT)(*psz - L'0') < 10)   // digit - accumulate it
                grouping = grouping * 10 + (UINT)(*psz - L'0');

            else if (*psz)                      // punctuation - ignore it
                { }

            else                                // end of string, no "0" found
            {
                grouping = grouping * 10;       // put zero on end (see examples)
                break;                          // and finished
            }

            psz++;
        }
    }
    else
    {
        // Win9x - take only the first grouping
        grouping = StrToIntW(szGrouping);
    }
    return grouping;
}

// Sizes of various stringized numbers
#define MAX_INT64_SIZE  30              // 2^64 is less than 30 chars long
#define MAX_COMMA_NUMBER_SIZE   (MAX_INT64_SIZE + 10)

// takes a DWORD add commas etc to it and puts the result in the buffer
LPWSTR CommifyString(LONGLONG n, LPWSTR pszBuf, UINT cchBuf)
{
    WCHAR szNum[MAX_COMMA_NUMBER_SIZE], szSep[5];
    NUMBERFMTW nfmt;

    nfmt.NumDigits = 0;
    nfmt.LeadingZero = 0;
    nfmt.Grouping = GetNLSGrouping();
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, szSep, ARRAYSIZE(szSep));
    nfmt.lpDecimalSep = nfmt.lpThousandSep = szSep;
    nfmt.NegativeOrder = 0;

    Int64ToStr(n, szNum);

    if (GetNumberFormatW(LOCALE_USER_DEFAULT, 0, szNum, &nfmt, pszBuf, cchBuf) == 0)
        StrCpyNW(pszBuf, szNum, cchBuf);

    return pszBuf;
}

/* converts numbers into sort formats
 *      532     -> 523 bytes
 *      1340    -> 1.3KB
 *      23506   -> 23.5KB
 *              -> 2.4MB
 *              -> 5.2GB
 */

LPWSTR StrFormatByteSizeW(LONGLONG n, LPWSTR pszBuf, UINT cchBuf)
{
    WCHAR szWholeNum[32], szOrder[32];
    int iOrder;

    // If the size is less than 1024, then the order should be bytes we have nothing
    // more to figure out
    if (n < 1024) 
    {
        wnsprintfW(szWholeNum, ARRAYSIZE(szWholeNum), L"%d", LODWORD(n));
        iOrder = 0;
    }
    else
    {
        UINT uInt, uLen, uDec;
        WCHAR szFormat[8];

        // Find the right order
        for (iOrder = 1; iOrder < ARRAYSIZE(c_aOrders) -1 && n >= 1000L * 1024L; n >>= 10, iOrder++);
            /* do nothing */

        uInt = LODWORD(n >> 10);
        CommifyString(uInt, szWholeNum, ARRAYSIZE(szWholeNum));
        uLen = lstrlenW(szWholeNum);
        if (uLen < 3)
        {
            uDec = LODWORD(n - (LONGLONG)uInt * 1024L) * 1000 / 1024;
            // At this point, uDec should be between 0 and 1000
            // we want get the top one (or two) digits.
            uDec /= 10;
            if (uLen == 2)
                uDec /= 10;

            // Note that we need to set the format before getting the
            // intl char.
            StrCpyW(szFormat, L"%02d");

            szFormat[2] = TEXT('0') + 3 - uLen;
            GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL,
                           szWholeNum + uLen, ARRAYSIZE(szWholeNum) - uLen);
            uLen = lstrlenW(szWholeNum);
            wnsprintfW(szWholeNum + uLen, ARRAYSIZE(szWholeNum) - uLen, szFormat, uDec);
        }
    }

    MLLoadStringW(c_aOrders[iOrder], szOrder, ARRAYSIZE(szOrder));
    wnsprintfW(pszBuf, cchBuf, szOrder, szWholeNum);
    return pszBuf;
}

// dw - the nubmer to be converted
// pszBuf - buffer for the resulting string
// cchBuf - Max characters in Buffer

LPSTR StrFormatByteSize64A(LONGLONG dw, LPSTR pszBuf, UINT cchBuf)
{
    WCHAR szT[32];

    StrFormatByteSizeW(dw, szT, SIZECHARS(szT));

    SHUnicodeToAnsi(szT, pszBuf, cchBuf);
    return pszBuf;
}

LPSTR StrFormatByteSizeA(DWORD dw, LPSTR pszBuf, UINT cchBuf)
{
    return StrFormatByteSize64A((LONGLONG)dw, pszBuf, cchBuf);
}

LPWSTR StrFormatKBSizeW(LONGLONG n, LPWSTR pszBuf, UINT cchBuf)
{
    static WCHAR s_szOrder[16] = {0};
    WCHAR szNum[64];

    if (s_szOrder[0] == TEXT('\0'))
        LoadStringW(HINST_THISDLL, IDS_ORDERKB, s_szOrder, ARRAYSIZE(s_szOrder));

    CommifyString((n + 1023) / 1024, szNum, ARRAYSIZE(szNum));

    wnsprintfW(pszBuf, cchBuf, s_szOrder, szNum);
    return pszBuf;
}

LPSTR StrFormatKBSizeA(LONGLONG n, LPSTR pszBuf, UINT cchBuf)
{
    WCHAR szNum[64];

    StrFormatKBSizeW(n, szNum, ARRAYSIZE(szNum));

    SHUnicodeToAnsi(szNum, pszBuf, cchBuf);
    return pszBuf;
}

//  Win95 does not support the wide-char version of lstrcmp, lstrcmpi
//  Wrapper for lstrcmpW so it works on Win95

int StrCmpW(LPCWSTR pwsz1, LPCWSTR pwsz2)
{
    return _StrCmpLocaleW(0, pwsz1, -1, pwsz2, -1);
}

// Wrapper for lstrcmpiW so it works on Win95

int StrCmpIW(LPCWSTR pwsz1, LPCWSTR pwsz2)
{
    return _StrCmpLocaleW(NORM_IGNORECASE, pwsz1, -1, pwsz2, -1);
}


/*----------------------------------------------------------
Purpose: Trim the string pszTrimMe of any leading or trailing
         characters that are in pszTrimChars.

Returns: TRUE if anything was stripped

*/
STDAPI_(BOOL) StrTrimA(IN OUT LPSTR pszTrimMe, LPCSTR pszTrimChars)
{
    BOOL bRet = FALSE;
    LPSTR psz;
    LPSTR pszStartMeat;
    LPSTR pszMark = NULL;
    
    ASSERT(IS_VALID_STRING_PTRA(pszTrimMe, -1));
    ASSERT(IS_VALID_STRING_PTRA(pszTrimChars, -1));
    
    if (pszTrimMe)
    {
        /* Trim leading characters. */
        
        psz = pszTrimMe;
        
        while (*psz && StrChrA(pszTrimChars, *psz))
            psz = CharNextA(psz);
        
        pszStartMeat = psz;
        
        /* Trim trailing characters. */
        
        // (The old algorithm used to start from the end and go
        // backwards, but that is piggy because DBCS version of
        // CharPrev iterates from the beginning of the string
        // on every call.)
        
        while (*psz)
        {
            if (StrChrA(pszTrimChars, *psz))
            {
                if (!pszMark)
                {
                    pszMark = psz;
                }
            }
            else
            {
                pszMark = NULL;
            }
            psz = CharNextA(psz);
        }
        
        // Any trailing characters to clip?
        if (pszMark)
        {
            // Yes
            *pszMark = '\0';
            bRet = TRUE;
        }
        
        /* Relocate stripped string. */
        
        if (pszStartMeat > pszTrimMe)
        {
            /* (+ 1) for null terminator. */
            MoveMemory(pszTrimMe, pszStartMeat, CbFromCchA(lstrlenA(pszStartMeat) + 1));
            bRet = TRUE;
        }
        else
            ASSERT(pszStartMeat == pszTrimMe);
        
        ASSERT(IS_VALID_STRING_PTRA(pszTrimMe, -1));
    }
    
    return bRet;
}


/*----------------------------------------------------------
Purpose: Trim the string pszTrimMe of any leading or trailing
         characters that are in pszTrimChars.

Returns: TRUE if anything was stripped

*/
STDAPI_(BOOL) StrTrimW(IN OUT LPWSTR  pszTrimMe, LPCWSTR pszTrimChars)
{
    BOOL bRet = FALSE;
    LPWSTR psz;
    LPWSTR pszStartMeat;
    LPWSTR pszMark = NULL;
    
    ASSERT(IS_VALID_STRING_PTRW(pszTrimMe, -1));
    ASSERT(IS_VALID_STRING_PTRW(pszTrimChars, -1));
    
    if (pszTrimMe)
    {
        /* Trim leading characters. */
        
        psz = pszTrimMe;
        
        while (*psz && StrChrW(pszTrimChars, *psz))
            psz++;
        
        pszStartMeat = psz;
        
        /* Trim trailing characters. */
        
        // (The old algorithm used to start from the end and go
        // backwards, but that is piggy because DBCS version of
        // CharPrev iterates from the beginning of the string
        // on every call.)
        
        while (*psz)
        {
            if (StrChrW(pszTrimChars, *psz))
            {
                if (!pszMark)
                {
                    pszMark = psz;
                }
            }
            else
            {
                pszMark = NULL;
            }
            psz++;
        }
        
        // Any trailing characters to clip?
        if (pszMark)
        {
            // Yes
            *pszMark = '\0';
            bRet = TRUE;
        }
        
        /* Relocate stripped string. */
        
        if (pszStartMeat > pszTrimMe)
        {
            /* (+ 1) for null terminator. */
            MoveMemory(pszTrimMe, pszStartMeat, CbFromCchW(lstrlenW(pszStartMeat) + 1));
            bRet = TRUE;
        }
        else
            ASSERT(pszStartMeat == pszTrimMe);
        
        ASSERT(IS_VALID_STRING_PTRW(pszTrimMe, -1));
    }
    
    return bRet;
}


/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2
*/
LWSTDAPI_(int) StrCmpNCA(LPCSTR pch1, LPCSTR pch2, int n)
{
    if (n == 0)
        return 0;

    while (--n && *pch1 && *pch1 == *pch2)
    {
        pch1++;
        pch2++;
    }

    return *(unsigned char *)pch1 - *(unsigned char *)pch2;
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpNCW(LPCWSTR pch1, LPCWSTR pch2, int n)
{
    if (n == 0)
        return 0;

    while (--n && *pch1 && *pch1 == *pch2)
    {
        pch1++;
        pch2++;
    }

    return *pch1 - *pch2;
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpNICA(LPCSTR pch1, LPCSTR pch2, int n)
{
    int ch1, ch2;

    if (n != 0)
    {
        do {

            ch1 = *pch1++;
            if (ch1 >= 'A' && ch1 <= 'Z')
                ch1 += 'a' - 'A';

            ch2 = *pch2++;
            if (ch2 >= 'A' && ch2 <= 'Z')
                ch2 += 'a' - 'A';

        } while ( --n && ch1 && (ch1 == ch2) );

        return ch1 - ch2;
    }
    else
    {
        return 0;
    }
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpNICW(LPCWSTR pch1, LPCWSTR pch2, int n)
{
    int ch1, ch2;

    if (n != 0)
    {

        do {

            ch1 = *pch1++;
            if (ch1 >= L'A' && ch1 <= L'Z')
                ch1 += L'a' - L'A';

            ch2 = *pch2++;
            if (ch2 >= L'A' && ch2 <= L'Z')
                ch2 += L'a' - L'A';

        } while ( --n && ch1 && (ch1 == ch2) );

        return ch1 - ch2;
    }
    else
    {
        return 0;
    }
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpCA(LPCSTR pch1, LPCSTR pch2)
{
    while (*pch1 && (*pch1 == *pch2))
    {
        ++pch1;
        ++pch2;
    }   

    return *(unsigned char *)pch1 - *(unsigned char *)pch2;
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpCW(LPCWSTR pch1, LPCWSTR pch2)
{
    while (*pch1 && (*pch1 == *pch2))
    {
        ++pch1;
        ++pch2;
    }   

    return *pch1 - *pch2;
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpICA(LPCSTR pch1, LPCSTR pch2)
{
    int ch1, ch2;

    do {

        ch1 = *pch1++;
        if (ch1 >= 'A' && ch1 <= 'Z')
            ch1 += 'a' - 'A';

        ch2 = *pch2++;
        if (ch2 >= 'A' && ch2 <= 'Z')
            ch2 += 'a' - 'A';

    } while (ch1 && (ch1 == ch2));

    return ch1 - ch2;
}

/*----------------------------------------------------------
Purpose: Compare strings using C runtime (ASCII) collation rules.

Returns: < 0 if pch1 <  pch2
         = 0 if pch1 == pch2
         > 0 if pch1 >  pch2

*/
LWSTDAPI_(int) StrCmpICW(LPCWSTR pch1, LPCWSTR pch2)
{
    int ch1, ch2;

    do {

        ch1 = *pch1++;
        if (ch1 >= L'A' && ch1 <= L'Z')
            ch1 += L'a' - L'A';

        ch2 = *pch2++;
        if (ch2 >= L'A' && ch2 <= L'Z')
            ch2 += L'a' - L'A';

    } while (ch1 && (ch1 == ch2));

    return ch1 - ch2;
}

LWSTDAPI StrRetToStrW(STRRET *psr, LPCITEMIDLIST pidl, WCHAR **ppsz)
{
    HRESULT hres = S_OK;

    switch (psr->uType)
    {
    case STRRET_WSTR:
        *ppsz = psr->pOleStr;
        psr->pOleStr = NULL;   // avoid alias
        hres = *ppsz ? S_OK : E_FAIL;
        break;

    case STRRET_OFFSET:
        hres = SHStrDupA(STRRET_OFFPTR(pidl, psr), ppsz);
        break;

    case STRRET_CSTR:
        hres = SHStrDupA(psr->cStr, ppsz);
        break;

    default:
        *ppsz = NULL;
        hres = E_FAIL;
    }
    return hres;
}

HRESULT DupWideToAnsi(LPCWSTR pwsz, LPSTR *ppsz)
{
    UINT cch = WideCharToMultiByte(CP_ACP, 0, pwsz, -1, NULL, 0, NULL, NULL) + 1;
    *ppsz = CoTaskMemAlloc(cch * sizeof(**ppsz));
    if (*ppsz)
    {
        SHUnicodeToAnsi(pwsz, *ppsz, cch);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT DupAnsiToAnsi(LPCSTR psz, LPSTR *ppsz)
{
    *ppsz = (LPSTR)CoTaskMemAlloc((lstrlenA(psz) + 1) * sizeof(**ppsz));
    if (*ppsz) 
    {
        lstrcpyA(*ppsz, psz);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

LWSTDAPI StrRetToStrA(STRRET *psr, LPCITEMIDLIST pidl, CHAR **ppsz)
{
    HRESULT hres;
    LPWSTR pwsz;

    switch (psr->uType)
    {
    case STRRET_WSTR:
        hres = DupWideToAnsi(psr->pOleStr, ppsz);
        pwsz = psr->pOleStr;
        psr->pOleStr = NULL;   // avoid alias
        CoTaskMemFree(pwsz);
        break;

    case STRRET_OFFSET:
        hres = DupAnsiToAnsi(STRRET_OFFPTR(pidl, psr), ppsz);
        break;

    case STRRET_CSTR:
        hres = DupAnsiToAnsi(psr->cStr, ppsz);
        break;

    default:
        *ppsz = NULL;
        hres = E_FAIL;
    }
    return hres;
}

STDAPI StrRetToBufA(STRRET *psr, LPCITEMIDLIST pidl, LPSTR pszBuf, UINT cchBuf)
{
    HRESULT hres = E_FAIL;

    switch (psr->uType)
    {
    case STRRET_WSTR:
        {
            LPWSTR pszStr = psr->pOleStr;   // temp copy because SHUnicodeToAnsi may overwrite buffer
            if (pszStr)
            {
                SHUnicodeToAnsi(pszStr, pszBuf, cchBuf);
                CoTaskMemFree(pszStr);
                psr->uType = STRRET_CSTR;   // Make sure no one thinks things are allocated still
                hres = S_OK;
            }
        }
        break;

    case STRRET_CSTR:
        SHAnsiToAnsi(psr->cStr, pszBuf, cchBuf);
        hres = S_OK;
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            // BUGBUG (DavePl) Alignment problems here
            SHAnsiToAnsi(STRRET_OFFPTR(pidl, psr), pszBuf, cchBuf);
            hres = S_OK;
        }
        break;
    }

    if (FAILED(hres) && cchBuf)
        *pszBuf = 0;

    return hres;
}

STDAPI StrRetToBufW(STRRET *psr, LPCITEMIDLIST pidl, LPWSTR pszBuf, UINT cchBuf)
{
    HRESULT hres = E_FAIL;
    
    switch (psr->uType)
    {
    case STRRET_WSTR:
        {
            LPWSTR pwszTmp = psr->pOleStr;
            if (pwszTmp)
            {
                StrCpyNW(pszBuf, pwszTmp, cchBuf);
                CoTaskMemFree(pwszTmp);
                psr->uType = STRRET_CSTR;   // Make sure no one thinks things are allocated still
                hres = S_OK;
            }
        }
        break;

    case STRRET_CSTR:
        SHAnsiToUnicode(psr->cStr, pszBuf, cchBuf);
        hres = S_OK;
        break;

    case STRRET_OFFSET:
        if (pidl)
        {
            // BUGBUG (DavePl) Alignment problems here

            SHAnsiToUnicode(STRRET_OFFPTR(pidl, psr), pszBuf, cchBuf);
            hres = S_OK;
        }
        break;
    }

    if (FAILED(hres) && cchBuf)
        *pszBuf = 0;

    return hres;
}

// dupe a string using the task allocator for returing from a COM interface
// These functions use SHAlloc, so they cannot go into shlwapi.

STDAPI SHStrDupA(LPCSTR psz, WCHAR **ppwsz)
{
    DWORD cch = MultiByteToWideChar(CP_ACP, 0, psz, -1, NULL, 0);
    *ppwsz = (WCHAR *)CoTaskMemAlloc((cch + 1) * SIZEOF(WCHAR));
    if (*ppwsz)
    {
        MultiByteToWideChar(CP_ACP, 0, psz, -1, *ppwsz, cch);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

// dupe a string using the task allocator for returing from a COM interface
// Sometimes, due to structure packing, the pointer we get is not properly
// aligned for Win64, so we have to do UNALIGNED64.

STDAPI SHStrDupW(LPCWSTR psz, WCHAR **ppwsz)
{
    WCHAR *pwsz;
    pwsz = (WCHAR *)CoTaskMemAlloc((lstrlenW(psz) + 1) * SIZEOF(WCHAR));
    
    *((PVOID UNALIGNED64 *) ppwsz) = pwsz;

    if (pwsz)
    {
        StrCpyW(pwsz, psz);
        return S_OK;
    }
    return E_OUTOFMEMORY;
}

 
