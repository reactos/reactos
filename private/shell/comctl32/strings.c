//============================================================================
//
// DBCS aware string routines...
//
//
//============================================================================

//#if defined(UNIX) && !defined(UNICODE)
//#define UNICODE
//#endif


#include "ctlspriv.h"

#ifdef WINNT
#include <winnlsp.h>    // Get private NORM_ flag for StrEqIntl()
#endif

// for those of us who don't ssync to nt's build headers
#ifndef NORM_STOP_ON_NULL
#define NORM_STOP_ON_NULL   0x10000000
#endif

// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand thid

#ifdef UNIX

#ifdef BIG_ENDIAN
#define READNATIVEWORD(x) MAKEWORD(*(char*)(x), *(char*)((char*)(x) + 1))
#else
#define READNATIVEWORD(x) MAKEWORD(*(char*)((char*)(x) + 1), *(char*)(x))
#endif

#else

#define READNATIVEWORD(x) (*(UNALIGNED WORD *)x)

#endif


/*
 * StrEndN - Find the end of a string, but no more than n bytes
 * Assumes   lpStart points to start of null terminated string
 *           nBufSize is the maximum length
 * returns ptr to just after the last byte to be included
 */
LPSTR lstrfns_StrEndNA(LPCSTR lpStart, int nBufSize)
{
  LPCSTR lpEnd;

  for (lpEnd = lpStart + nBufSize; *lpStart && OFFSETOF(lpStart) < OFFSETOF(lpEnd);
	lpStart = AnsiNext(lpStart))
    continue;   /* just getting to the end of the string */
  if (OFFSETOF(lpStart) > OFFSETOF(lpEnd))
    {
      /* We can only get here if the last byte before lpEnd was a lead byte
       */
      lpStart -= 2;
    }
  return((LPSTR)lpStart);
}

LPWSTR lstrfns_StrEndNW(LPCWSTR lpStart, int nBufSize)
{
#ifdef UNICODE
  LPCWSTR lpEnd;

  for (lpEnd = lpStart + nBufSize; *lpStart && (lpStart < lpEnd);
	lpStart++)
    continue;   /* just getting to the end of the string */

  return((LPWSTR)lpStart);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}


// REVIEW WIN32 HACK - Convert to 32bit asm.
#ifndef WIN32
/*
 * ReverseScan - Find last occurrence of a byte in a string
 * Assumes   lpSource points to first byte to check (end of the string)
 *           uLen is the number of bytes to check
 *           bMatch is the byte to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR PASCAL ReverseScan(LPCSTR lpSource, UINT uLen, BYTE bMatch)
{
  _asm
    {
	/* Load count */
	mov     cx,uLen
	jcxz ReverseScanFail    ; count is zero, return failure.

	/* Load up es:di, ax */
	les     di,lpSource
	mov     al,bMatch
	/* Set the direction flag based on bBackward
	 * Perform the search; return 0 if we reached the end of the string
	 * otherwise, return es:di+1
	 */
	std
	repne   scasb
	jne     ReverseScanFail     ; check result of last compare.

	inc     di
	mov     dx,es
	mov     ax,di
	jmp ReverseScanExit

ReverseScanFail:
	xor     ax,ax
	xor     dx,dx

	/* clear the direction flag and return
	 */
ReverseScanExit:
	cld
    }
    if (0) return 0;        // suppress warning, optimized out
}
#endif

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

BOOL ChrCmpA(WORD w1, WORD wMatch)
{
  return ChrCmpA_inline(w1, wMatch);
}

#ifdef UNICODE
__inline BOOL ChrCmpW_inline(WCHAR w1, WCHAR wMatch)
{
    return(!(w1 == wMatch));
}
#endif

BOOL ChrCmpW(WCHAR w1, WCHAR wMatch)
{
#ifdef UNICODE
   return ChrCmpW_inline(w1, wMatch);
#else
    SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
    return FALSE;
#endif
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

#if defined(BIG_ENDIAN)
  sz2[0] = LOBYTE(wMatch);
  sz2[1] = HIBYTE(wMatch);
#else
  *(WORD FAR *)sz2 = wMatch;
#endif
  sz2[2] = '\0';
  return lstrcmpiA(sz1, sz2);
}

BOOL ChrCmpIW(WCHAR w1, WCHAR wMatch)
{
#ifdef UNICODE
  WCHAR sz1[2], sz2[2];

  sz1[0] = w1;
  sz1[1] = TEXT('\0');
  sz2[0] = wMatch;
  sz2[1] = TEXT('\0');

  return lstrcmpiW(sz1, sz2);

#else
  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return FALSE;

#endif
}

LPWSTR StrCpyW(LPWSTR psz1, LPCWSTR psz2)
{
    LPWSTR psz = psz1;
    do {
        *psz1 = *psz2;
        psz1++;
    } while(*psz2++);
    return psz;
}


LPWSTR StrCpyNW(LPWSTR psz1, LPCWSTR psz2, int cchMax)
{
    LPWSTR psz = psz1;

    ASSERT(psz1);
    ASSERT(psz2);

    if (0 < cchMax)
    {
        // Leave room for the null terminator
        while (0 < --cchMax)
        {
            if ( !(*psz1++ = *psz2++) )
                break;
        }

        if (0 == cchMax)
            *psz1 = '\0';
    }

    return psz;
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrChrA(LPCSTR lpStart, WORD wMatch)
{
  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
      {
	  return((LPSTR)lpStart);
      }
   }
   return (NULL);
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

LPWSTR FAR PASCAL StrChrW(LPCWSTR lpStart, WCHAR wMatch)
{
#ifdef UNICODE
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

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}

/*
 * StrRChr - Find last occurrence of character in string
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrRChrA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenA(lpStart);

  for ( ; OFFSETOF(lpStart) < OFFSETOF(lpEnd); lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpA_inline(READNATIVEWORD(lpStart), wMatch))
	  lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}

LPWSTR FAR PASCAL StrRChrW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch)
{
#ifdef UNICODE

  LPCWSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenW(lpStart);

  for ( ; lpStart < lpEnd; lpStart++)
    {
      if (!ChrCmpW_inline(*lpStart, wMatch))
	  lpFound = lpStart;
    }
  return ((LPWSTR)lpFound);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}


/*
 * StrRChrI - Find last occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of string
 *           lpEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrRChrIA(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenA(lpStart);

  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; OFFSETOF(lpStart) < OFFSETOF(lpEnd); lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
          lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}

LPWSTR FAR PASCAL StrRChrIW(LPCWSTR lpStart, LPCWSTR lpEnd, WCHAR wMatch)
{
#ifdef UNICODE

  LPCWSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlenW(lpStart);

  for ( ; lpStart < lpEnd; lpStart++)
    {
      if (!ChrCmpIW(*lpStart, wMatch))
          lpFound = lpStart;
    }
  return ((LPWSTR)lpFound);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}


// StrCSpn: return index to first char of lpStr that is present in lpSet.
// Includes the NUL in the comparison; if no lpSet chars are found, returns
// the index to the NUL in lpStr.
// Just like CRT strcspn.
//
int FAR PASCAL StrCSpnA(LPCSTR lpStr, LPCSTR lpSet)
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

int FAR PASCAL StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet)
{
#ifdef UNICODE

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

#else

        SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
	return -1;

#endif
}


// StrCSpnI: case-insensitive version of StrCSpn.
//
int FAR PASCAL StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet)
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

int FAR PASCAL StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet)
{
#ifdef UNICODE
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

#else

        SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
        return -1;

#endif
}


/*
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 * BUGBUG, won't work if source strings are in ROM
 */
int FAR PASCAL StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    char sz1[4];
    char sz2[4];
    LPCSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1 = AnsiNext(lpStr1), lpStr2 = AnsiNext(lpStr2)) {
        WORD wMatch;


        wMatch = (WORD) (*lpStr2 | (*(lpStr2+1)<<8));

        if (ChrCmpA_inline(READNATIVEWORD(lpStr1), wMatch)) {
            int iRet;

            (*(WORD FAR *)sz1) = READNATIVEWORD(lpStr1);
            (*(WORD FAR *)sz2) = wMatch;
#ifndef UNIX
            *AnsiNext(sz1) = 0;
            *AnsiNext(sz2) = 0;
#else
            *AnsiNext((LPWORD)sz1) = 0;
            *AnsiNext((LPWORD)sz2) = 0;
#endif

            iRet = lstrcmpA(sz1, sz2);
            //DebugMsg(DM_TRACE, ".................... %d", iRet);
            return iRet;
        }
    }

    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

int FAR PASCAL StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
#ifdef UNICODE

    WCHAR sz1[2];
    WCHAR sz2[2];
    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        i = ChrCmpW_inline(*lpStr1, *lpStr2);
        if (i) {
            int iRet;

            sz1[0] = *lpStr1;
            sz2[0] = *lpStr2;
            sz1[1] = TEXT('\0');
            sz2[1] = TEXT('\0');
            iRet = lstrcmpW(sz1, sz2);
            //DebugMsg(DM_TRACE, ".................... %d", iRet);
            return iRet;
        }
    }

    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return -1;

#endif
}

/*
 * StrCmpNI     - Compare n bytes, case insensitive
 *
 * returns   See lstrcmpi return values.
 */

 #ifndef WINNT
__inline BOOL IsAsciiA(char ch)
{
    return !(ch & 0x80);
}

__inline char Ascii_ToLowerA(char ch)
{
    return (ch >= 'A' && ch <= 'Z') ? (ch - 'A' + 'a') : ch;
}

#ifdef UNICODE
__inline BOOL IsAsciiW(WCHAR ch)
{
    return ch < 128;
}

__inline WCHAR Ascii_ToLowerW(WCHAR ch)
{
    return (ch >= L'A' && ch <= L'Z') ? (ch - L'A' + L'a') : ch;
}
#endif

int FAR PASCAL StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    LPCSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpNI: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); (lpStr1 = AnsiNext(lpStr1)), (lpStr2 = AnsiNext(lpStr2))) {
        WORD wMatch;

        if (IsAsciiA(*lpStr1) && IsAsciiA(*lpStr2))
        {
            i = Ascii_ToLowerA(*lpStr1) - Ascii_ToLowerA(*lpStr2);
        }
        else
        {
#ifndef UNIX
            wMatch = (UINT)(IsDBCSLeadByte(*lpStr2)) ? *(WORD FAR *)lpStr2 : (WORD)(BYTE)(*lpStr2);
#else
            wMatch = (UINT)(IsDBCSLeadByte(*lpStr2)) ? (*lpStr2 | (*(lpStr2+1)<<8)) : (WORD)(BYTE)(*lpStr2);
#endif

            i = ChrCmpIA(READNATIVEWORD(lpStr1), wMatch);
        }
        if (i) {
            //DebugMsg(DM_TRACE, ".................... %d", i);
            if (i < 0)
            {
                return -1;
            }
            else
            {
                return 1;
            }
        }
    }
    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

int FAR PASCAL StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
#ifdef UNICODE

    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpNI: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        if (IsAsciiW(*lpStr1) && IsAsciiW(*lpStr2))
        {
            i = Ascii_ToLowerW(*lpStr1) - Ascii_ToLowerW(*lpStr2);
        }
        else
        {
            i = ChrCmpIW(*lpStr1, *lpStr2);
        }
        if (i) {
            //DebugMsg(DM_TRACE, ".................... %d", i);
           if (i < 0)
           {
               return -1;
           }
           else
           {
               return 1;
           }
           return i;
        }
    }
    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return -1;

#endif
}

#else // WINNT

int StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    
    //  Win95 doesn't support NORM_STOP_ON_NULL
    i = CompareStringA(GetThreadLocale(), NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                       lpStr1, nChar, lpStr2, nChar);

    if (!i)
    {
        i = CompareStringA(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                             lpStr1, nChar, lpStr2, nChar);
    }

    return i - CSTR_EQUAL;    
}

int StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
    int i;

    //  Win95 doesn't support NORM_STOP_ON_NULL
    i = CompareStringW(GetThreadLocale(), NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                       lpStr1, nChar, lpStr2, nChar);

    if (!i)
    {
        i = CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE | NORM_STOP_ON_NULL, 
                             lpStr1, nChar, lpStr2, nChar);
    }

    return i - CSTR_EQUAL;    
}

#endif //   WINNT


/*
 * IntlStrEq
 *
 * returns TRUE if strings are equal, FALSE if not
 */
BOOL IntlStrEqWorkerA(BOOL fCaseSens, LPCSTR lpString1, LPCSTR lpString2, int nChar) {
    int retval;
    DWORD dwFlags = fCaseSens ? LOCALE_USE_CP_ACP : (NORM_IGNORECASE | LOCALE_USE_CP_ACP);

#ifdef WINNT
    //
    // On NT we can tell CompareString to stop at a '\0' if one is found before nChar chars
    //
    dwFlags |= NORM_STOP_ON_NULL;
#else
    //
    // On Win9x we have to do the check manually
    //
    if (nChar != -1) {
        LPCSTR psz1, psz2;
        int cch = 0;

        psz1 = lpString1;
        psz2 = lpString2;

        while( *psz1 != '\0' && *psz2 != '\0' && cch < nChar) {
#ifdef DBCS
            psz1 = CharNextA(psz1);
            psz2 = CharNextA(psz2);

            cch = min(psz1 - lpString1, psz2 - lpString2);
#else
            psz1++;
            psz2++;
            cch++;
#endif
        }

        // add one in for terminating '\0'
        cch++;

        if (cch < nChar) {
            nChar = cch;
        }
    }
#endif

    retval = CompareStringA( GetThreadLocale(),
                             dwFlags,
                             lpString1,
                             nChar,
                             lpString2,
                             nChar );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringA( GetSystemDefaultLCID(),
                                 dwFlags,
                                 lpString1,
                                 nChar,
                                 lpString2,
                                 nChar );
    }

    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            if (fCaseSens)
                retval = StrCmpNA(lpString1, lpString2, nChar) + 2;
            else {
                retval = StrCmpNIA(lpString1, lpString2, nChar) + 2;
            }
        }
        else
        {
            retval = 2;
        }
    }

    return (retval == 2);

}


BOOL IntlStrEqWorkerW(BOOL fCaseSens, LPCWSTR lpString1, LPCWSTR lpString2, int nChar) {

#ifdef UNICODE
    int retval;
    DWORD dwFlags = fCaseSens ? 0 : NORM_IGNORECASE;

#ifdef WINNT
    //
    // On NT we can tell CompareString to stop at a '\0' if one is found before nChar chars
    //
    dwFlags |= NORM_STOP_ON_NULL;
#else
    //
    // On Win9x we have to do the check manually
    //
    if (nChar != -1) {
        LPCWSTR psz1, psz2;
        int cch = 0;

        psz1 = lpString1;
        psz2 = lpString2;

        while( *psz1 != TEXT('\0') && *psz2 != TEXT('\0') && cch < nChar) {
            psz1++;
            psz2++;
            cch++;
        }

        // add one in for terminating '\0'
        cch++;

        if (cch < nChar) {
            nChar = cch;
        }
    }
#endif


    retval = CompareStringW( GetThreadLocale(),
                             dwFlags,
                             lpString1,
                             nChar,
                             lpString2,
                             nChar );
    if (retval == 0)
    {
        //
        // The caller is not expecting failure.  Try the system
        // default locale id.
        //
        retval = CompareStringW( GetSystemDefaultLCID(),
                                 dwFlags,
                                 lpString1,
                                 nChar,
                                 lpString2,
                                 nChar );
    }

    if (retval == 0)
    {
        if (lpString1 && lpString2)
        {
            //
            // The caller is not expecting failure.  We've never had a
            // failure indicator before.  We'll do a best guess by calling
            // the C runtimes to do a non-locale sensitive compare.
            //
            if (fCaseSens)
                retval = StrCmpNW(lpString1, lpString2, nChar) + 2;
            else {
                retval = StrCmpNIW(lpString1, lpString2, nChar) + 2;
            }
        }
        else
        {
            retval = 2;
        }
    }

    return (retval == 2);

#else
  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return FALSE;
#endif
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
LPSTR FAR PASCAL StrRStrIA(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch)
{
    LPCSTR lpFound = NULL;
    LPSTR lpEnd;
    char cHold;

    if (!lpLast)
        lpLast = lpSource + lstrlenA(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    lpEnd = lstrfns_StrEndNA(lpLast, (UINT)(lstrlenA(lpSrch)-1));
    cHold = *lpEnd;
    *lpEnd = 0;

    while ((lpSource = StrStrIA(lpSource, lpSrch))!=0 &&
          OFFSETOF(lpSource) < OFFSETOF(lpLast))
    {
        lpFound = lpSource;
        lpSource = AnsiNext(lpSource);
    }
    *lpEnd = cHold;
    return((LPSTR)lpFound);
}

LPWSTR FAR PASCAL StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch)
{
#ifdef UNICODE
    LPCWSTR lpFound = NULL;
    LPWSTR lpEnd;
    WCHAR cHold;

    if (!lpLast)
        lpLast = lpSource + lstrlenW(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    lpEnd = lstrfns_StrEndNW(lpLast, (UINT)(lstrlenW(lpSrch)-1));
    cHold = *lpEnd;
    *lpEnd = 0;

    while ((lpSource = StrStrIW(lpSource, lpSrch))!=0 &&
          lpSource < lpLast)
    {
        lpFound = lpSource;
        lpSource++;
    }
    *lpEnd = cHold;
    return((LPWSTR)lpFound);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}



/*
 * StrStr      - Search for first occurrence of a substring
 *
 * Assumes   lpSource points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrStrA(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenA(lpSrch);
  wMatch = READNATIVEWORD(lpSrch);

  for ( ; (lpFirst=StrChrA(lpFirst, wMatch))!=0 && !IntlStrEqNA(lpFirst, lpSrch, uLen);
        lpFirst=AnsiNext(lpFirst))
    continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

LPWSTR FAR PASCAL StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
#ifdef UNICODE

  UINT uLen;
  WCHAR wMatch;

  uLen = (UINT)lstrlenW(lpSrch);
  wMatch = *lpSrch;

  for ( ; (lpFirst=StrChrW(lpFirst, wMatch))!=0 && !IntlStrEqNW(lpFirst, lpSrch, uLen);
        lpFirst++)
    continue; /* continue until we hit the end of the string or get a match */

  return((LPWSTR)lpFirst);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}

/*
 * StrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrChrIA(LPCSTR lpStart, WORD wMatch)
{
  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpIA(READNATIVEWORD(lpStart), wMatch))
	  return((LPSTR)lpStart);
    }
  return (NULL);
}

LPWSTR FAR PASCAL StrChrIW(LPCWSTR lpStart, WCHAR wMatch)
{
  for ( ; *lpStart; lpStart++)
    {
      if (!ChrCmpIW(*lpStart, wMatch))
	  return((LPWSTR)lpStart);
    }
  return (NULL);
}


/*
 * StrStrI   - Search for first occurrence of a substring, case insensitive
 *
 * Assumes   lpFirst points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrStrIA(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenA(lpSrch);
  wMatch = READNATIVEWORD(lpSrch);

  for ( ; (lpFirst = StrChrIA(lpFirst, wMatch)) != 0 && !IntlStrEqNIA(lpFirst, lpSrch, uLen);
        lpFirst=AnsiNext(lpFirst))
      continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

LPWSTR FAR PASCAL StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
#ifdef UNICODE

  UINT uLen;
  WCHAR wMatch;

  uLen = (UINT)lstrlenW(lpSrch);
  wMatch = *lpSrch;

  for ( ; (lpFirst = StrChrIW(lpFirst, wMatch)) != 0 && !IntlStrEqNIW(lpFirst, lpSrch, uLen);
        lpFirst++)
      continue; /* continue until we hit the end of the string or get a match */

  return((LPWSTR)lpFirst);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}
#ifndef UNICODE

// TruncateString - bugbug: the same logic in shdocvw
//
// purpose: cut a string at the given length in dbcs safe manner
//          the string may be truncated at cch-1 if the sz[cch] points
//          to a lead byte that would result in cutting in the middle
//          of double byte character.
//          It is caller's responsibility to reserve enough buffer for
//          sz so we can put sz[cch]=0 safely.
//
//          *Note this logic is not much perf hit when called with sbcs
//          string, as it just bails out at the tail character always.
//
void  TruncateString(char *sz, int cchBufferSize)
{
    int cch = cchBufferSize - 1;
    LPSTR psz = &sz[cch];
    
    if (!sz || cchBufferSize <= 0) return;
    
    while (psz > sz)
    {
        psz--;
        if (!IsDBCSLeadByte(*psz))
        {
            // Found non-leadbyte for the first time.
            // This is either a trail byte of double byte char
            // or a single byte character we've first seen.
            // Thus, the next pointer must be at either of a leadbyte
            // or &sz[cch]
            psz++;
            break;
        }
    }
    if (((&sz[cch] - psz) & 1) && cch > 0)
    {
        // we're truncating the string in the middle of dbcs
        cch--;
    }
    sz[cch] = '\0';
    return;
}
#endif
