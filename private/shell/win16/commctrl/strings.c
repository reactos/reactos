//============================================================================
//
// DBCS aware string routines...
//
//
//============================================================================

#include "ctlspriv.h"

// WARNING: all of these APIs do not setup DS, so you can not access
// any data in the default data seg of this DLL.
//
// do not create any global variables... talk to chrisg if you don't
// understand thid



#ifdef DBCS
#define FASTCALL PASCAL
#else
#define FASTCALL _fastcall
#endif


/*
 * StrEndN - Find the end of a string, but no more than n bytes
 * Assumes   lpStart points to start of null terminated string
 *           nBufSize is the maximum length
 * returns ptr to just after the last byte to be included
 */
LPSTR NEAR FASTCALL lstrfns_StrEndN(LPCSTR lpStart, int nBufSize)
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

// REVIEW WIN32 HACK - Convert to 32bit asm.
#ifndef WIN32
/*
 * ReverseScan - Find last occurrence of a byte in a string
 * Assumes   lpSource points to first byte to check (end of the string)
 *           uLen is the number of bytes to check
 *           bMatch is the byte to match
 * returns ptr to the last occurrence of ch in str, NULL if not found.
 */
LPSTR NEAR PASCAL ReverseScan(LPCSTR lpSource, UINT uLen, BYTE bMatch)
{
  _asm
    {
	/* Load count */
	mov	cx,uLen
	jcxz ReverseScanFail    ; count is zero, return failure.

	/* Load up es:di, ax */
	les	di,lpSource
	mov	al,bMatch
	/* Set the direction flag based on bBackward
	 * Perform the search; return 0 if we reached the end of the string
	 * otherwise, return es:di+1
	 */
	std
	repne	scasb
	jne	ReverseScanFail     ; check result of last compare.

	inc	di
	mov	dx,es
	mov	ax,di
	jmp ReverseScanExit

ReverseScanFail:
	xor	ax,ax
	xor	dx,dx

	/* clear the direction flag and return
	 */
ReverseScanExit:
	cld
    }
    if (0) return 0;	    // suppress warning, optimized out
}
#endif

/*
 * ChrCmp -  Case sensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared
 * Return    FALSE if they match, TRUE if no match
 */
BOOL NEAR FASTCALL ChrCmp(WORD w1, WORD wMatch)
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

/*
 * ChrCmpI - Case insensitive character comparison for DBCS
 * Assumes   w1, wMatch are characters to be compared;
 *           HIBYTE of wMatch is 0 if not a DBC
 * Return    FALSE if match, TRUE if not
 */
BOOL NEAR FASTCALL ChrCmpI(WORD w1, WORD wMatch)
{
  char sz1[3], sz2[3];

  if (IsDBCSLeadByte(sz1[0] = LOBYTE(w1)))
    {
      sz1[1] = HIBYTE(w1);
      sz1[2] = '\0';
    }
  else
      sz1[1] = '\0';

  *(WORD FAR *)sz2 = wMatch;
  sz2[2] = '\0';
  return lstrcmpi(sz1, sz2);
}

/*
 * StrChr - Find first occurrence of character in string
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrChr(LPCSTR lpStart, WORD wMatch)
{
  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmp(*(WORD FAR *)lpStart, wMatch))
          return((LPSTR)lpStart);
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
LPSTR FAR PASCAL StrRChr(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlen(lpStart);

  for ( ; OFFSETOF(lpStart) < OFFSETOF(lpEnd); lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmp(*(WORD FAR *)lpStart, wMatch))
          lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}
/*
 * StrChrI - Find first occurrence of character in string, case insensitive
 * Assumes   lpStart points to start of null terminated string
 *           wMatch  is the character to match
 * returns ptr to the first occurrence of ch in str, NULL if not found.
 */
LPSTR FAR PASCAL StrChrI(LPCSTR lpStart, WORD wMatch)
{
  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; *lpStart; lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpI(*(WORD FAR *)lpStart, wMatch))
          return((LPSTR)lpStart);
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
LPSTR FAR PASCAL StrRChrI(LPCSTR lpStart, LPCSTR lpEnd, WORD wMatch)
{
  LPCSTR lpFound = NULL;

  if (!lpEnd)
      lpEnd = lpStart + lstrlen(lpStart);

  wMatch = (UINT)(IsDBCSLeadByte(LOBYTE(wMatch)) ? wMatch : LOBYTE(wMatch));

  for ( ; OFFSETOF(lpStart) < OFFSETOF(lpEnd); lpStart = AnsiNext(lpStart))
    {
      if (!ChrCmpI(*(WORD FAR *)lpStart, wMatch))
          lpFound = lpStart;
    }
  return ((LPSTR)lpFound);
}
// StrCSpn: return index to first char of lpStr that is present in lpSet.
// Includes the NUL in the comparison; if no lpSet chars are found, returns
// the index to the NUL in lpStr.
// Just like CRT strcspn.
//
int FAR PASCAL StrCSpn(LPCSTR lpStr, LPCSTR lpSet)
{
	// nature of the beast: O(lpStr*lpSet) work
	LPCSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
		if (StrChr(lpSet, *(WORD FAR *)lp))
			return (int)(lp-lpStr);
		lp = AnsiNext(lp);
	}

	return (int)(lp-lpStr); // ==lstrlen(lpStr)
}
// StrCSpnI: case-insensitive version of StrCSpn.
//
int FAR PASCAL StrCSpnI(LPCSTR lpStr, LPCSTR lpSet)
{
	// nature of the beast: O(lpStr*lpSet) work
	LPCSTR lp = lpStr;
	if (!lpStr || !lpSet)
		return 0;

	while (*lp)
	{
		if (StrChrI(lpSet, *(WORD FAR *)lp))
			return (int)(lp-lpStr);
		lp = AnsiNext(lp);
	}

	return (int)(lp-lpStr); // ==lstrlen(lpStr)
}

/*
 * StrCmpN      - Compare n bytes
 *
 * returns   See lstrcmp return values.
 * BUGBUG, won't work if source strings are in ROM
 */
int FAR PASCAL StrCmpN(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    TCHAR sz1[4]; 
    TCHAR sz2[4];
    int i;
    LPSTR lpszEnd = (LPSTR)(((LPBYTE)lpStr1) + nChar);

    //DebugMsg(DM_TRACE, "StrCmpN: %s %s %d returns:", lpStr1, lpStr2, nChar);
    
    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); lpStr1 = AnsiNext(lpStr1), lpStr2 = AnsiNext(lpStr2)) {
        WORD wMatch;
        wMatch = (UINT)(IsDBCSLeadByte(*lpStr2)) ? *(WORD FAR *)lpStr2 : (WORD)(BYTE)(*lpStr2);
        i = ChrCmp(*(WORD FAR *)lpStr1, wMatch);
        if (i) {
            int iRet;
            
            (*(WORD FAR *)sz1) = *(WORD FAR *)lpStr1;
            (*(WORD FAR *)sz2) = *(WORD FAR *)lpStr2;
            *AnsiNext(sz1) = 0;
            *AnsiNext(sz2) = 0;
            iRet = lstrcmp(sz1, sz2);
            //DebugMsg(DM_TRACE, ".................... %d", iRet);
            return iRet;
        }
    }
    
    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}
/*
 * StrCmpNI     - Compare n bytes, case insensitive
 *
 * returns   See lstrcmpi return values.
 */
int FAR PASCAL StrCmpNI(LPCSTR lpStr1, LPCSTR lpStr2, int nChar)
{
    int i;
    LPSTR lpszEnd = (LPSTR)(((LPBYTE)lpStr1) + nChar);
    //DebugMsg(DM_TRACE, "StrCmpNI: %s %s %d returns:", lpStr1, lpStr2, nChar);

    for ( ; (lpszEnd > lpStr1) && (*lpStr1 || *lpStr2); (lpStr1 = AnsiNext(lpStr1)), (lpStr2 = AnsiNext(lpStr2))) {
        WORD wMatch;
        wMatch = (UINT)(IsDBCSLeadByte(*lpStr2)) ? *(WORD FAR *)lpStr2 : (WORD)(BYTE)(*lpStr2);
        i = ChrCmpI(*(WORD FAR *)lpStr1, wMatch);
        if (i) {
            //DebugMsg(DM_TRACE, ".................... %d", i);
            return i;
        }
    }
    //DebugMsg(DM_TRACE, ".................... 0");
    return 0;
}

// REVIEW WIN32 HACK - Cut a dependancy on ReverseScan (it's in asm).
#ifndef WIN32
/*
 * StrRStr      - Search for last occurrence of a substring
 *
 * Assumes   lpSource points to the null terminated source string
 *           lpLast points to where to search from in the source string
 *           lpLast is not included in the search
 *           lpSrch points to string to search for
 * returns   last occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrRStr(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch)
{
  UINT uLen;
  BYTE bMatch;

  uLen = (UINT)lstrlen(lpSrch);
  bMatch = *lpSrch;

  if (!lpLast)
      lpLast = lpSource + lstrlen(lpSource);

  for ( ; ; )
    {
      /* Return NULL if we hit the exact beginning of the string
       */
      if (lpLast == lpSource)
	  return(NULL);

      --lpLast;

      /* Break if we hit the beginning of the string
       */
      if ((lpLast=ReverseScan(lpLast, (UINT)(OFFSETOF(lpLast)-OFFSETOF(lpSource)+1),
	    bMatch)) == 0)
	  break;

      /* Break if we found the string, and its first byte is not a tail byte
       */
      if (!StrCmpN(lpLast, lpSrch, uLen) &&
	    (lpLast==lstrfns_StrEndN(lpSource, (int)(OFFSETOF(lpLast)-OFFSETOF(lpSource)))))
	  break;
    }

  return((LPSTR)lpLast);
}
#endif

/*
 * StrRStrI      - Search for last occurrence of a substring
 *
 * Assumes   lpSource points to the null terminated source string
 *           lpLast points to where to search from in the source string
 *           lpLast is not included in the search
 *           lpSrch points to string to search for
 * returns   last occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrRStrI(LPCSTR lpSource, LPCSTR lpLast, LPCSTR lpSrch)
{
    LPCSTR lpFound = NULL;
    LPSTR lpEnd;
    char cHold;

    if (!lpLast)
        lpLast = lpSource + lstrlen(lpSource);

    if (lpSource >= lpLast || *lpSrch == 0)
        return NULL;

    lpEnd = lstrfns_StrEndN(lpLast, (UINT)(lstrlen(lpSrch)-1));
    cHold = *lpEnd;
    *lpEnd = 0;

    while ((lpSource = StrStrI(lpSource, lpSrch))!=0 &&
          OFFSETOF(lpSource) < OFFSETOF(lpLast))
    {
        lpFound = lpSource;
        lpSource = AnsiNext(lpSource);
    }
    *lpEnd = cHold;
    return((LPSTR)lpFound);
}
/*
 * StrStr      - Search for first occurrence of a substring
 *
 * Assumes   lpSource points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrStr(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlen(lpSrch);
  wMatch = *(WORD FAR *)lpSrch;

  for ( ; (lpFirst=StrChr(lpFirst, wMatch))!=0 && StrCmpN(lpFirst, lpSrch, uLen);
	lpFirst=AnsiNext(lpFirst))
    continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}
/*
 * StrStrI   - Search for first occurrence of a substring, case insensitive
 *
 * Assumes   lpFirst points to source string
 *           lpSrch points to string to search for
 * returns   first occurrence of string if successful; NULL otherwise
 */
LPSTR FAR PASCAL StrStrI(LPCSTR lpFirst, LPCSTR lpSrch)
{
  UINT uLen;
  WORD wMatch;

  uLen = (UINT)lstrlen(lpSrch);
  wMatch = *(WORD FAR *)lpSrch;

  for ( ; (lpFirst = StrChrI(lpFirst, wMatch)) != 0 && StrCmpNI(lpFirst, lpSrch, uLen);
	lpFirst=AnsiNext(lpFirst))
      continue; /* continue until we hit the end of the string or get a match */

  return((LPSTR)lpFirst);
}

