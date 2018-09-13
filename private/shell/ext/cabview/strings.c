//*******************************************************************************************
//
// Filename : strings.c
//	
//				DBCS aware string routines...
//
// Copyright (c) 1994 - 1996 Microsoft Corporation. All rights reserved
//
//*******************************************************************************************

#include "pch.h"
#include "strings.h"

#ifdef UNICODE
#define _StrEndN    _StrEndNW
#define ChrCmp      ChrCmpW
#define ChrCmpI     ChrCmpIW
#else
#define _StrEndN    _StrEndNA
#define ChrCmp      ChrCmpA
#define ChrCmpI     ChrCmpIA
#endif


/*
 * StrEndN - Find the end of a string, but no more than n bytes
 * Assumes   pStart points to start of null terminated string
 *           nBufSize is the maximum length
 * returns ptr to just after the last byte to be included
 */
LPSTR _StrEndNA(LPCSTR pStart, int nBufSize)
{
  LPCSTR pEnd;

  for (pEnd = pStart + nBufSize; *pStart && pStart < pEnd; pStart = AnsiNext(pStart))
    continue;   /* just getting to the end of the string */
  if (pStart > pEnd)
    {
      /* We can only get here if the last byte before pEnd was a lead byte
       */
      pStart -= 2;
    }
  return (LPSTR)pStart;
}

LPWSTR _StrEndNW(LPCWSTR pStart, int nBufSize)
{
#ifdef UNICODE
  LPCWSTR pEnd;

  for (pEnd = pStart + nBufSize; *pStart && (pStart < pEnd);
	pStart++)
    continue;   /* just getting to the end of the string */

  return((LPWSTR)pStart);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}


/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
BOOL ChrCmpA(WORD w1, WORD wMatch)
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

BOOL ChrCmpW(WORD w1, WORD wMatch)
{
#ifdef UNICODE
   return(!(w1 == wMatch));
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

  *(WORD *)sz2 = wMatch;
  sz2[2] = '\0';
  return lstrcmpiA(sz1, sz2);
}


BOOL ChrCmpIW(WORD w1, WORD wMatch)
{
#ifdef UNICODE
  TCHAR sz1[2], sz2[2];

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



/*
 * StrChr - Find first occurrence of character in string
 * Assumes   pStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR StrChrA(LPCSTR pStart, WORD wMatch)
{
  for ( ; *pStart; pStart = AnsiNext(pStart))
    {
      if (!ChrCmpA(*(UNALIGNED WORD *)pStart, wMatch))
      {
	  return((LPSTR)pStart);
      }
   }
   return (NULL);
}

LPWSTR StrChrW(LPCWSTR pStart, WORD wMatch)
{
#ifdef UNICODE

  for ( ; *pStart; pStart = AnsiNext(pStart))
  {
      // Need a tmp word since casting ptr to WORD * will
      // fault on MIPS, ALPHA

      WORD wTmp;
      memcpy(&wTmp, pStart, sizeof(WORD));

      if (!ChrCmpW(wTmp, wMatch))
      {
	  return((LPWSTR)pStart);
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
 * Assumes   pStart points to start of string
 *           pEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChrA(LPCSTR pStart, LPCSTR pEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!pEnd)
      pEnd = pStart + lstrlenA(pStart);

  for ( ; pStart < pEnd; pStart = AnsiNext(pStart))
    {
      if (!ChrCmpA(*(UNALIGNED WORD *)pStart, wMatch))
	  lpFound = pStart;
    }
  return ((LPSTR)lpFound);
}

LPWSTR StrRChrW(LPCWSTR pStart, LPCWSTR pEnd, WORD wMatch)
{
#ifdef UNICODE

  LPCWSTR lpFound = NULL;

  if (!pEnd)
      pEnd = pStart + lstrlenW(pStart);

  for ( ; pStart < pEnd; pStart++)
    {
      if (!ChrCmpW(*(UNALIGNED WORD *)pStart, wMatch))
	  lpFound = pStart;
    }
  return ((LPWSTR)lpFound);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}

/*
 * StrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   pStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR StrChrIA(LPCSTR pStart, WORD wMatch)
{
  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; *pStart; pStart = AnsiNext(pStart))
    {
      if (!ChrCmpIA(*(UNALIGNED WORD *)pStart, wMatch))
	  return((LPSTR)pStart);
    }
  return (NULL);
}

LPWSTR StrChrIW(LPCWSTR pStart, WORD wMatch)
{
#ifdef UNICODE

  for ( ; *pStart; pStart++)
    {
      if (!ChrCmpIW(*(WORD *)pStart, wMatch))
	  return((LPWSTR)pStart);
    }
  return (NULL);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}

/*
 * StrRChrI - Find last occurrence of character in string, case insensitive
 * Assumes   pStart points to start of string
 *           pEnd   points to end of string (NOT included in search)
 *           wMatch  is the character to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR StrRChrIA(LPCSTR pStart, LPCSTR pEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!pEnd)
      pEnd = pStart + lstrlenA(pStart);

  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; pStart < pEnd; pStart = AnsiNext(pStart))
    {
      if (!ChrCmpIA(*(UNALIGNED WORD *)pStart, wMatch))
	  lpFound = pStart;
    }
  return ((LPSTR)lpFound);
}

LPWSTR StrRChrIW(LPCWSTR pStart, LPCWSTR pEnd, WORD wMatch)
{
#ifdef UNICODE

  LPCWSTR lpFound = NULL;

  if (!pEnd)
      pEnd = pStart + lstrlenW(pStart);

  for ( ; pStart < pEnd; pStart++)
    {
      if (!ChrCmpIW(*(WORD *)pStart, wMatch))
	  lpFound = pStart;
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
int StrCSpnA(LPCSTR lpStr, LPCSTR lpSet)
{
	// nature of the beast: O(lpStr*lpSet) work
	LPCSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
 		if (StrChrA(lpSet, *(UNALIGNED WORD *)lp))
			return (int)(lp-lpStr);
		lp = AnsiNext(lp);
	}

	return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

int StrCSpnW(LPCWSTR lpStr, LPCWSTR lpSet)
{
#ifdef UNICODE

	// nature of the beast: O(lpStr*lpSet) work
	LPCWSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
		if (StrChrW(lpSet, *(WORD *)lp))
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
int StrCSpnIA(LPCSTR lpStr, LPCSTR lpSet)
{
	// nature of the beast: O(lpStr*lpSet) work
	LPCSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
		if (StrChrIA(lpSet, *(UNALIGNED WORD *)lp))
			return (int)(lp-lpStr);
		lp = AnsiNext(lp);
	}

	return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

int StrCSpnIW(LPCWSTR lpStr, LPCWSTR lpSet)
{
#ifdef UNICODE
	// nature of the beast: O(lpStr*lpSet) work
	LPCWSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
		if (StrChrIW(lpSet, *(WORD *)lp))
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
 * won't work if source strings are in ROM
 */
int StrCmpNA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    char sz1[4];
    char sz2[4];
    int i;
    LPCSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1 = AnsiNext(lpStr1), lpStr2 = AnsiNext(lpStr2)) {
        WORD wMatch;

        if (IsDBCSLeadByte(*lpStr2))
            lpStr2++;

        wMatch = (WORD) *lpStr2;

        i = ChrCmpA(*(UNALIGNED WORD *)lpStr1, wMatch);
        if (i) {
            int iRet;

            (*(WORD *)sz1) = *(UNALIGNED WORD *)lpStr1;
            (*(WORD *)sz2) = *(UNALIGNED WORD *)lpStr2;
            *AnsiNext(sz1) = 0;
            *AnsiNext(sz2) = 0;
            iRet = lstrcmpA(sz1, sz2);
            //DebugMsg(DM_TRACE, ".................... %d", iRet);
            return iRet;
        }
    }

    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

int StrCmpNW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
#ifdef UNICODE

    WCHAR sz1[2];
    WCHAR sz2[2];
    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        i = ChrCmpW(*lpStr1, *lpStr2);
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
int StrCmpNIA(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    LPCSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpNI: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); (lpStr1 = AnsiNext(lpStr1)), (lpStr2 = AnsiNext(lpStr2))) {
        WORD wMatch;

        wMatch = (UINT)(IsDBCSLeadByte(*lpStr2)) ? *(WORD *)lpStr2 : (WORD)(BYTE)(*lpStr2);

        i = ChrCmpIA(*(UNALIGNED WORD *)lpStr1, wMatch);
        if (i) {
            //DebugMsg(DM_TRACE, ".................... %d", i);
            return i;
        }
    }
    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

int StrCmpNIW(LPCWSTR lpStr1, LPCWSTR lpStr2, int nChar)
{
#ifdef UNICODE

    int i;
    LPCWSTR lpszEnd = lpStr1 + nChar;

    //DebugMsg(DM_TRACE, "StrCmpNI: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1++, lpStr2++) {
        i = ChrCmpIW(*lpStr1, *lpStr2);
        if (i) {
            //DebugMsg(DM_TRACE, ".................... %d", i);
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
    LPSTR pEnd;
    char cHold;

    if (!lpLast)
	lpLast = lpSource + lstrlenA(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
	return NULL;

    pEnd = _StrEndNA(lpLast, (UINT)(lstrlenA(lpSrch)-1));
    cHold = *pEnd;
    *pEnd = 0;

    while ((lpSource = StrStrIA(lpSource, lpSrch)) != 0 && lpSource < lpLast)
    {
	lpFound = lpSource;
	lpSource = AnsiNext(lpSource);
    }
    *pEnd = cHold;
    return((LPSTR)lpFound);
}

LPWSTR StrRStrIW(LPCWSTR lpSource, LPCWSTR lpLast, LPCWSTR lpSrch)
{
#ifdef UNICODE
    LPCWSTR lpFound = NULL;
    LPWSTR pEnd;
    WCHAR cHold;

    if (!lpLast)
	lpLast = lpSource + lstrlenW(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
	return NULL;

    pEnd = _StrEndNW(lpLast, (UINT)(lstrlenW(lpSrch)-1));
    cHold = *pEnd;
    *pEnd = 0;

    while ((lpSource = StrStrIW(lpSource, lpSrch))!=0 &&
	  lpSource < lpLast)
    {
	lpFound = lpSource;
	lpSource++;
    }
    *pEnd = cHold;
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
LPSTR StrStrA(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenA(lpSrch);
  wMatch = *(UNALIGNED WORD *)lpSrch;

  for ( ; (lpFirst=StrChrA(lpFirst, wMatch))!=0 && StrCmpNA(lpFirst, lpSrch, uLen);
	lpFirst=AnsiNext(lpFirst))
    continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

LPWSTR StrStrW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
#ifdef UNICODE

  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenW(lpSrch);
  wMatch = *(WORD *)lpSrch;

  for ( ; (lpFirst=StrChrW(lpFirst, wMatch))!=0 && StrCmpNW(lpFirst, lpSrch, uLen);
	lpFirst++)
    continue; /* continue until we hit the end of the string or get a match */

  return((LPWSTR)lpFirst);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
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
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenA(lpSrch);
  wMatch = *(UNALIGNED WORD *)lpSrch;

  for ( ; (lpFirst = StrChrIA(lpFirst, wMatch)) != 0 && StrCmpNIA(lpFirst, lpSrch, uLen);
	lpFirst=AnsiNext(lpFirst))
      continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

LPWSTR StrStrIW(LPCWSTR lpFirst, LPCWSTR lpSrch)
{
#ifdef UNICODE

  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlenW(lpSrch);
  wMatch = *(WORD *)lpSrch;

  for ( ; (lpFirst = StrChrIW(lpFirst, wMatch)) != 0 && StrCmpNIW(lpFirst, lpSrch, uLen);
	lpFirst++)
      continue; /* continue until we hit the end of the string or get a match */

  return((LPWSTR)lpFirst);

#else

  SetLastErrorEx(ERROR_CALL_NOT_IMPLEMENTED, SLE_WARNING);
  return NULL;

#endif
}
