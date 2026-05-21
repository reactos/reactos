/*
 * Shlwapi string functions
 *
 * Copyright 1998 Juergen Schmied
 * Copyright 2002 Jon Griffiths
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#define NO_SHLWAPI_REG
#define NO_SHLWAPI_STREAM
#include "shlwapi.h"
#include "wingdi.h"
#include "winuser.h"
#include "shlobj.h"
#include "mlang.h"
#include "ddeml.h"
#include "wine/unicode.h"
#include "wine/debug.h"

#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

extern HINSTANCE shlwapi_hInstance;

static HRESULT _SHStrDupAA(LPCSTR,LPSTR*);
static HRESULT _SHStrDupAW(LPCWSTR,LPSTR*);

DWORD WINAPI SHTruncateString(LPSTR lpStr, DWORD size);

static void FillNumberFmt(NUMBERFMTW *fmt, LPWSTR decimal_buffer, int decimal_bufwlen,
                          LPWSTR thousand_buffer, int thousand_bufwlen)
{
  WCHAR grouping[64];
  WCHAR *c;

  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_ILZERO|LOCALE_RETURN_NUMBER, (LPWSTR)&fmt->LeadingZero, sizeof(fmt->LeadingZero)/sizeof(WCHAR));
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_INEGNUMBER|LOCALE_RETURN_NUMBER, (LPWSTR)&fmt->LeadingZero, sizeof(fmt->NegativeOrder)/sizeof(WCHAR));
  fmt->NumDigits = 0;
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SDECIMAL, decimal_buffer, decimal_bufwlen);
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thousand_buffer, thousand_bufwlen);
  fmt->lpThousandSep = thousand_buffer;
  fmt->lpDecimalSep = decimal_buffer;

  /* 
   * Converting grouping string to number as described on 
   * http://blogs.msdn.com/oldnewthing/archive/2006/04/18/578251.aspx
   */
  fmt->Grouping = 0;
  GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, ARRAY_SIZE(grouping));
  for (c = grouping; *c; c++)
    if (*c >= '0' && *c < '9')
    {
      fmt->Grouping *= 10;
      fmt->Grouping += *c - '0';
    }

  if (fmt->Grouping % 10 == 0)
    fmt->Grouping /= 10;
  else
    fmt->Grouping *= 10;
}

/*************************************************************************
 * FormatInt   [internal]
 *
 * Format an integer according to the current locale
 *
 * RETURNS
 *  The number of characters written on success or 0 on failure
 */
static int FormatInt(LONGLONG qdwValue, LPWSTR pszBuf, int cchBuf)
{
  NUMBERFMTW fmt;
  WCHAR decimal[8], thousand[8];
  WCHAR buf[24];
  WCHAR *c;
  BOOL neg = (qdwValue < 0);

  FillNumberFmt(&fmt, decimal, ARRAY_SIZE(decimal), thousand, ARRAY_SIZE(thousand));

  c = &buf[24];
  *(--c) = 0;
  do
  {
    *(--c) = '0' + (qdwValue%10);
    qdwValue /= 10;
  } while (qdwValue > 0);
  if (neg)
    *(--c) = '-';
  
  return GetNumberFormatW(LOCALE_USER_DEFAULT, 0, c, &fmt, pszBuf, cchBuf);
}

/*************************************************************************
 * FormatDouble   [internal]
 *
 * Format an integer according to the current locale. Prints the specified number of digits
 * after the decimal point
 *
 * RETURNS
 *  The number of characters written on success or 0 on failure
 */
static int FormatDouble(double value, int decimals, LPWSTR pszBuf, int cchBuf)
{
  static const WCHAR flfmt[] = {'%','f',0};
  WCHAR buf[64];
  NUMBERFMTW fmt;
  WCHAR decimal[8], thousand[8];
  
  swprintf(buf, 64, flfmt, value);

  FillNumberFmt(&fmt, decimal, ARRAY_SIZE(decimal), thousand, ARRAY_SIZE(thousand));
  fmt.NumDigits = decimals;
  return GetNumberFormatW(LOCALE_USER_DEFAULT, 0, buf, &fmt, pszBuf, cchBuf);
}


/*************************************************************************
 * StrCatW	[SHLWAPI.@]
 *
 * Concatenate two strings.
 *
 * PARAMS
 *  lpszStr [O] Initial string
 *  lpszSrc [I] String to concatenate
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCatW(LPWSTR lpszStr, LPCWSTR lpszSrc)
{
  TRACE("(%s,%s)\n", debugstr_w(lpszStr), debugstr_w(lpszSrc));

  if (lpszStr && lpszSrc)
    lstrcatW(lpszStr, lpszSrc);
  return lpszStr;
}


/*************************************************************************
 * StrCpyW	[SHLWAPI.@]
 *
 * Copy a string to another string.
 *
 * PARAMS
 *  lpszStr [O] Destination string
 *  lpszSrc [I] Source string
 *
 * RETURNS
 *  lpszStr.
 */
LPWSTR WINAPI StrCpyW(LPWSTR lpszStr, LPCWSTR lpszSrc)
{
  TRACE("(%p,%s)\n", lpszStr, debugstr_w(lpszSrc));

  if (lpszStr && lpszSrc)
    lstrcpyW(lpszStr, lpszSrc);
  return lpszStr;
}

/*************************************************************************
 * StrRetToBufA					[SHLWAPI.@]
 *
 * Convert a STRRET to a normal string.
 *
 * PARAMS
 *  lpStrRet [O] STRRET to convert
 *  pIdl     [I] ITEMIDLIST for lpStrRet->uType == STRRET_OFFSET
 *  lpszDest [O] Destination for normal string
 *  dwLen    [I] Length of lpszDest
 *
 * RETURNS
 *  Success: S_OK. lpszDest contains up to dwLen characters of the string.
 *           If lpStrRet is of type STRRET_WSTR, its memory is freed with
 *           CoTaskMemFree() and its type set to STRRET_CSTRA.
 *  Failure: E_FAIL, if any parameters are invalid.
 */
HRESULT WINAPI StrRetToBufA (LPSTRRET src, const ITEMIDLIST *pidl, LPSTR dest, UINT len)
{
	/* NOTE:
	 *  This routine is identical to that in dlls/shell32/shellstring.c.
	 *  It was duplicated because not every version of Shlwapi.dll exports
	 *  StrRetToBufA. If you change one routine, change them both.
	 */
        TRACE("dest=%p len=0x%x strret=%p pidl=%p\n", dest, len, src, pidl);

	if (!src)
	{
	  WARN("Invalid lpStrRet would crash under Win32!\n");
	  if (dest)
	    *dest = '\0';
	  return E_FAIL;
	}

	if (!dest || !len)
	  return E_FAIL;

	*dest = '\0';

	switch (src->uType)
	{
	  case STRRET_WSTR:
	    WideCharToMultiByte(CP_ACP, 0, src->pOleStr, -1, dest, len, NULL, NULL);
	    CoTaskMemFree(src->pOleStr);
	    break;

	  case STRRET_CSTR:
            lstrcpynA(dest, src->cStr, len);
	    break;

	  case STRRET_OFFSET:
            lstrcpynA(dest, ((LPCSTR)&pidl->mkid)+src->uOffset, len);
	    break;

	  default:
	    FIXME("unknown type!\n");
	    return E_NOTIMPL;
	}
	return S_OK;
}

/*************************************************************************
 * StrRetToBufW	[SHLWAPI.@]
 *
 * See StrRetToBufA.
 */
HRESULT WINAPI StrRetToBufW (LPSTRRET src, const ITEMIDLIST *pidl, LPWSTR dest, UINT len)
{
    TRACE("dest=%p len=0x%x strret=%p pidl=%p\n", dest, len, src, pidl);

    if (!dest || !len)
        return E_FAIL;

    if (!src)
    {
        WARN("Invalid lpStrRet would crash under Win32!\n");
        if (dest)
            *dest = '\0';
        return E_FAIL;
    }

    *dest = '\0';

    switch (src->uType) {
    case STRRET_WSTR: {
        size_t dst_len;
        if (!src->pOleStr)
            return E_FAIL;
        dst_len = lstrlenW(src->pOleStr);
        memcpy(dest, src->pOleStr, min(dst_len, len-1) * sizeof(WCHAR));
        dest[min(dst_len, len-1)] = 0;
        CoTaskMemFree(src->pOleStr);
        if (len <= dst_len)
        {
            dest[0] = 0;
            return E_NOT_SUFFICIENT_BUFFER;
        }
        break;
    }

    case STRRET_CSTR:
        if (!MultiByteToWideChar( CP_ACP, 0, src->cStr, -1, dest, len ))
            dest[len-1] = 0;
        break;

    case STRRET_OFFSET:
        if (pidl)
	{
            if (!MultiByteToWideChar( CP_ACP, 0, ((LPCSTR)&pidl->mkid)+src->uOffset, -1,
                                      dest, len ))
                dest[len-1] = 0;
        }
        break;

    default:
        FIXME("unknown type!\n");
        return E_NOTIMPL;
    }

    return S_OK;
}

/*************************************************************************
 * StrRetToStrA					[SHLWAPI.@]
 *
 * Converts a STRRET to a normal string.
 *
 * PARAMS
 *  lpStrRet [O] STRRET to convert
 *  pidl     [I] ITEMIDLIST for lpStrRet->uType == STRRET_OFFSET
 *  ppszName [O] Destination for converted string
 *
 * RETURNS
 *  Success: S_OK. ppszName contains the new string, allocated with CoTaskMemAlloc().
 *  Failure: E_FAIL, if any parameters are invalid.
 */
HRESULT WINAPI StrRetToStrA(LPSTRRET lpStrRet, const ITEMIDLIST *pidl, LPSTR *ppszName)
{
  HRESULT hRet = E_FAIL;

  switch (lpStrRet->uType)
  {
  case STRRET_WSTR:
    hRet = _SHStrDupAW(lpStrRet->pOleStr, ppszName);
    CoTaskMemFree(lpStrRet->pOleStr);
    break;

  case STRRET_CSTR:
    hRet = _SHStrDupAA(lpStrRet->cStr, ppszName);
    break;

  case STRRET_OFFSET:
    hRet = _SHStrDupAA(((LPCSTR)&pidl->mkid) + lpStrRet->uOffset, ppszName);
    break;

  default:
    *ppszName = NULL;
  }

  return hRet;
}

/*************************************************************************
 * StrRetToStrW					[SHLWAPI.@]
 *
 * See StrRetToStrA.
 */
HRESULT WINAPI StrRetToStrW(LPSTRRET lpStrRet, const ITEMIDLIST *pidl, LPWSTR *ppszName)
{
  HRESULT hRet = E_FAIL;

  switch (lpStrRet->uType)
  {
  case STRRET_WSTR:
#ifdef __REACTOS__
    hRet = lpStrRet->pOleStr ? S_OK : E_FAIL;
    *ppszName = lpStrRet->pOleStr;
    lpStrRet->pOleStr = NULL; /* Windows does this, presumably in case someone calls SHFree */
#else
    hRet = SHStrDupW(lpStrRet->pOleStr, ppszName);
    CoTaskMemFree(lpStrRet->pOleStr);
#endif
    break;

  case STRRET_CSTR:
    hRet = SHStrDupA(lpStrRet->cStr, ppszName);
    break;

  case STRRET_OFFSET:
    hRet = SHStrDupA(((LPCSTR)&pidl->mkid) + lpStrRet->uOffset, ppszName);
    break;

  default:
    *ppszName = NULL;
  }

  return hRet;
}

/* Makes a Unicode copy of an ANSI string using SysAllocString() */
static HRESULT _SHStrDupAToBSTR(LPCSTR src, BSTR *pBstrOut)
{
    *pBstrOut = NULL;

    if (src)
    {
        INT len = MultiByteToWideChar(CP_ACP, 0, src, -1, NULL, 0);
        WCHAR *szTemp = malloc(len * sizeof(WCHAR));

        if (szTemp)
        {
            MultiByteToWideChar(CP_ACP, 0, src, -1, szTemp, len);
            *pBstrOut = SysAllocString(szTemp);
            free(szTemp);

            if (*pBstrOut)
                return S_OK;
        }
    }
    return E_OUTOFMEMORY;
}

/*************************************************************************
 * StrRetToBSTR	[SHLWAPI.@]
 *
 * Converts a STRRET to a BSTR.
 *
 * PARAMS
 *  lpStrRet [O] STRRET to convert
 *  pidl     [I] ITEMIDLIST for lpStrRet->uType = STRRET_OFFSET
 *  pBstrOut [O] Destination for converted BSTR
 *
 * RETURNS
 *  Success: S_OK. pBstrOut contains the new string.
 *  Failure: E_FAIL, if any parameters are invalid.
 */
HRESULT WINAPI StrRetToBSTR(STRRET *lpStrRet, LPCITEMIDLIST pidl, BSTR* pBstrOut)
{
  HRESULT hRet = E_FAIL;

  switch (lpStrRet->uType)
  {
  case STRRET_WSTR:
    *pBstrOut = SysAllocString(lpStrRet->pOleStr);
    if (*pBstrOut)
      hRet = S_OK;
    CoTaskMemFree(lpStrRet->pOleStr);
    break;

  case STRRET_CSTR:
    hRet = _SHStrDupAToBSTR(lpStrRet->cStr, pBstrOut);
    break;

  case STRRET_OFFSET:
    hRet = _SHStrDupAToBSTR(((LPCSTR)&pidl->mkid) + lpStrRet->uOffset, pBstrOut);
    break;

  default:
    *pBstrOut = NULL;
  }

  return hRet;
}

/*************************************************************************
 * StrFormatKBSizeA	[SHLWAPI.@]
 *
 * Create a formatted string containing a byte count in Kilobytes.
 *
 * PARAMS
 *  llBytes  [I] Byte size to format
 *  lpszDest [I] Destination for formatted string
 *  cchMax   [I] Size of lpszDest
 *
 * RETURNS
 *  lpszDest.
 */
LPSTR WINAPI StrFormatKBSizeA(LONGLONG llBytes, LPSTR lpszDest, UINT cchMax)
{
  WCHAR wszBuf[256];
  
  if (!StrFormatKBSizeW(llBytes, wszBuf, 256))
    return NULL;
  if (!WideCharToMultiByte(CP_ACP, 0, wszBuf, -1, lpszDest, cchMax, NULL, NULL))
    return NULL;
  return lpszDest;
}

/*************************************************************************
 * StrFormatKBSizeW	[SHLWAPI.@]
 *
 * See StrFormatKBSizeA.
 */
LPWSTR WINAPI StrFormatKBSizeW(LONGLONG llBytes, LPWSTR lpszDest, UINT cchMax)
{
  static const WCHAR kb[] = {' ','K','B',0};
  LONGLONG llKB = (llBytes + 1023) >> 10;
  int len;

  TRACE("(0x%s,%p,%d)\n", wine_dbgstr_longlong(llBytes), lpszDest, cchMax);

  if (!FormatInt(llKB, lpszDest, cchMax))
    return NULL;

  len = lstrlenW(lpszDest);
  if (cchMax - len < 4)
      return NULL;
  lstrcatW(lpszDest, kb);
  return lpszDest;
}

/*************************************************************************
 * StrNCatA	[SHLWAPI.@]
 *
 * Concatenate two strings together.
 *
 * PARAMS
 *  lpszStr [O] String to concatenate to
 *  lpszCat [I] String to add to lpszCat
 *  cchMax  [I] Maximum number of characters to concatenate
 *
 * RETURNS
 *  lpszStr.
 *
 * NOTES
 *  cchMax determines the number of characters that are appended to lpszStr,
 *  not the total length of the string.
 */
LPSTR WINAPI StrNCatA(LPSTR lpszStr, LPCSTR lpszCat, INT cchMax)
{
  LPSTR lpszRet = lpszStr;

  TRACE("(%s,%s,%i)\n", debugstr_a(lpszStr), debugstr_a(lpszCat), cchMax);

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32!\n");
    return NULL;
  }

  StrCpyNA(lpszStr + strlen(lpszStr), lpszCat, cchMax);
  return lpszRet;
}

/*************************************************************************
 * StrNCatW	[SHLWAPI.@]
 *
 * See StrNCatA.
 */
LPWSTR WINAPI StrNCatW(LPWSTR lpszStr, LPCWSTR lpszCat, INT cchMax)
{
  LPWSTR lpszRet = lpszStr;

  TRACE("(%s,%s,%i)\n", debugstr_w(lpszStr), debugstr_w(lpszCat), cchMax);

  if (!lpszStr)
  {
    WARN("Invalid lpszStr would crash under Win32\n");
    return NULL;
  }

  StrCpyNW(lpszStr + lstrlenW(lpszStr), lpszCat, cchMax);
  return lpszRet;
}

/*************************************************************************
 *      _SHStrDupAA	[INTERNAL]
 *
 * Duplicates a ANSI string to ANSI. The destination buffer is allocated.
 */
static HRESULT _SHStrDupAA(LPCSTR src, LPSTR * dest)
{
	HRESULT hr;
	int len = 0;

	if (src) {
	    len = lstrlenA(src) + 1;
	    *dest = CoTaskMemAlloc(len);
	} else {
	    *dest = NULL;
	}

	if (*dest) {
	    lstrcpynA(*dest,src, len);
	    hr = S_OK;
	} else {
	    hr = E_OUTOFMEMORY;
	}

	TRACE("%s->(%p)\n", debugstr_a(src), *dest);
	return hr;
}

/*************************************************************************
 * SHStrDupA	[SHLWAPI.@]
 *
 * Return a Unicode copy of a string, in memory allocated by CoTaskMemAlloc().
 *
 * PARAMS
 *  lpszStr   [I] String to copy
 *  lppszDest [O] Destination for the new string copy
 *
 * RETURNS
 *  Success: S_OK. lppszDest contains the new string in Unicode format.
 *  Failure: E_OUTOFMEMORY, If any arguments are invalid or memory allocation
 *           fails.
 */
HRESULT WINAPI SHStrDupA(LPCSTR lpszStr, LPWSTR * lppszDest)
{
  HRESULT hRet;
  int len = 0;

  if (lpszStr)
  {
    len = MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, NULL, 0) * sizeof(WCHAR);
    *lppszDest = CoTaskMemAlloc(len);
  }
  else
    *lppszDest = NULL;

  if (*lppszDest)
  {
    MultiByteToWideChar(CP_ACP, 0, lpszStr, -1, *lppszDest, len/sizeof(WCHAR));
    hRet = S_OK;
  }
  else
    hRet = E_OUTOFMEMORY;

  TRACE("%s->(%p)\n", debugstr_a(lpszStr), *lppszDest);
  return hRet;
}

/*************************************************************************
 *      _SHStrDupAW	[INTERNAL]
 *
 * Duplicates a UNICODE to a ANSI string. The destination buffer is allocated.
 */
static HRESULT _SHStrDupAW(LPCWSTR src, LPSTR * dest)
{
	HRESULT hr;
	int len = 0;

	if (src) {
	    len = WideCharToMultiByte(CP_ACP, 0, src, -1, NULL, 0, NULL, NULL);
	    *dest = CoTaskMemAlloc(len);
	} else {
	    *dest = NULL;
	}

	if (*dest) {
	    WideCharToMultiByte(CP_ACP, 0, src, -1, *dest, len, NULL, NULL);
	    hr = S_OK;
	} else {
	    hr = E_OUTOFMEMORY;
	}

	TRACE("%s->(%p)\n", debugstr_w(src), *dest);
	return hr;
}

/*************************************************************************
 * SHStrDupW	[SHLWAPI.@]
 *
 * See SHStrDupA.
 */
HRESULT WINAPI SHStrDupW(LPCWSTR src, LPWSTR * dest)
{
	HRESULT hr;
	int len = 0;

	if (src) {
	    len = (lstrlenW(src) + 1) * sizeof(WCHAR);
	    *dest = CoTaskMemAlloc(len);
	} else {
	    *dest = NULL;
	}

	if (*dest) {
	    memcpy(*dest, src, len);
	    hr = S_OK;
	} else {
	    hr = E_OUTOFMEMORY;
	}

	TRACE("%s->(%p)\n", debugstr_w(src), *dest);
	return hr;
}

/*************************************************************************
 * SHLWAPI_WriteReverseNum
 *
 * Internal helper for SHLWAPI_WriteTimeClass.
 */
static inline LPWSTR SHLWAPI_WriteReverseNum(LPWSTR lpszOut, DWORD dwNum)
{
  *lpszOut-- = '\0';

  /* Write a decimal number to a string, backwards */
  do
  {
    DWORD dwNextDigit = dwNum % 10;
    *lpszOut-- = '0' + dwNextDigit;
    dwNum = (dwNum - dwNextDigit) / 10;
  } while (dwNum > 0);

  return lpszOut;
}

/*************************************************************************
 * SHLWAPI_FormatSignificant
 *
 * Internal helper for SHLWAPI_WriteTimeClass.
 */
static inline int SHLWAPI_FormatSignificant(LPWSTR lpszNum, int dwDigits)
{
  /* Zero non significant digits, return remaining significant digits */
  while (*lpszNum)
  {
    lpszNum++;
    if (--dwDigits == 0)
    {
      while (*lpszNum)
        *lpszNum++ = '0';
      return 0;
    }
  }
  return dwDigits;
}

/*************************************************************************
 * SHLWAPI_WriteTimeClass
 *
 * Internal helper for StrFromTimeIntervalW.
 */
static int SHLWAPI_WriteTimeClass(LPWSTR lpszOut, DWORD dwValue,
                                  UINT uClassStringId, int iDigits)
{
  WCHAR szBuff[64], *szOut = szBuff + 32;

  szOut = SHLWAPI_WriteReverseNum(szOut, dwValue);
  iDigits = SHLWAPI_FormatSignificant(szOut + 1, iDigits);
  *szOut = ' ';
  LoadStringW(shlwapi_hInstance, uClassStringId, szBuff + 32, 32);
  lstrcatW(lpszOut, szOut);
  return iDigits;
}

/*************************************************************************
 * StrFromTimeIntervalA	[SHLWAPI.@]
 *
 * Format a millisecond time interval into a string
 *
 * PARAMS
 *  lpszStr  [O] Output buffer for formatted time interval
 *  cchMax   [I] Size of lpszStr
 *  dwMS     [I] Number of milliseconds
 *  iDigits  [I] Number of digits to print
 *
 * RETURNS
 *  The length of the formatted string, or 0 if any parameter is invalid.
 *
 * NOTES
 *  This implementation mimics the Win32 behaviour of always writing a leading
 *  space before the time interval begins.
 *
 *  iDigits is used to provide approximate times if accuracy is not important.
 *  This number of digits will be written of the first non-zero time class
 *  (hours/minutes/seconds). If this does not complete the time classification,
 *  the remaining digits are changed to zeros (i.e. The time is _not_ rounded).
 *  If there are digits remaining following the writing of a time class, the
 *  next time class will be written.
 *
 *  For example, given dwMS represents 138 hours,43 minutes and 15 seconds, the
 *  following will result from the given values of iDigits:
 *
 *|  iDigits    1        2        3        4               5               ...
 *|  lpszStr   "100 hr" "130 hr" "138 hr" "138 hr 40 min" "138 hr 43 min"  ...
 */
INT WINAPI StrFromTimeIntervalA(LPSTR lpszStr, UINT cchMax, DWORD dwMS,
                                int iDigits)
{
  INT iRet = 0;

  TRACE("(%p,%d,%ld,%d)\n", lpszStr, cchMax, dwMS, iDigits);

  if (lpszStr && cchMax)
  {
    WCHAR szBuff[128];
    StrFromTimeIntervalW(szBuff, ARRAY_SIZE(szBuff), dwMS, iDigits);
    WideCharToMultiByte(CP_ACP,0,szBuff,-1,lpszStr,cchMax,0,0);
  }
  return iRet;
}


/*************************************************************************
 * StrFromTimeIntervalW	[SHLWAPI.@]
 *
 * See StrFromTimeIntervalA.
 */
INT WINAPI StrFromTimeIntervalW(LPWSTR lpszStr, UINT cchMax, DWORD dwMS,
                                int iDigits)
{
  INT iRet = 0;

  TRACE("(%p,%d,%ld,%d)\n", lpszStr, cchMax, dwMS, iDigits);

  if (lpszStr && cchMax)
  {
    WCHAR szCopy[128];
    DWORD dwHours, dwMinutes;

    if (!iDigits || cchMax == 1)
    {
      *lpszStr = '\0';
      return 0;
    }

    /* Calculate the time classes */
    dwMS = (dwMS + 500) / 1000;
    dwHours = dwMS / 3600;
    dwMS -= dwHours * 3600;
    dwMinutes = dwMS / 60;
    dwMS -= dwMinutes * 60;

    szCopy[0] = '\0';

    if (dwHours)
      iDigits = SHLWAPI_WriteTimeClass(szCopy, dwHours, IDS_TIME_INTERVAL_HOURS, iDigits);

    if (dwMinutes && iDigits)
      iDigits = SHLWAPI_WriteTimeClass(szCopy, dwMinutes, IDS_TIME_INTERVAL_MINUTES, iDigits);

    if (iDigits) /* Always write seconds if we have significant digits */
      SHLWAPI_WriteTimeClass(szCopy, dwMS, IDS_TIME_INTERVAL_SECONDS, iDigits);

    lstrcpynW(lpszStr, szCopy, cchMax);
    iRet = lstrlenW(lpszStr);
  }
  return iRet;
}

/* Structure for formatting byte strings */
typedef struct tagSHLWAPI_BYTEFORMATS
{
  LONGLONG dLimit;
  double   dDivisor;
  double   dNormaliser;
  int      nDecimals;
#ifdef __REACTOS__
  UINT     nFormatID;
#else
  WCHAR     wPrefix;
#endif
} SHLWAPI_BYTEFORMATS;

/*************************************************************************
 * StrFormatByteSizeW	[SHLWAPI.@]
 *
 * Create a string containing an abbreviated byte count of up to 2^63-1.
 *
 * PARAMS
 *  llBytes  [I] Byte size to format
 *  lpszDest [I] Destination for formatted string
 *  cchMax   [I] Size of lpszDest
 *
 * RETURNS
 *  lpszDest.
 *
 * NOTES
 *  There is no StrFormatByteSize64W function, it is called StrFormatByteSizeW().
 */
LPWSTR WINAPI StrFormatByteSizeW(LONGLONG llBytes, LPWSTR lpszDest, UINT cchMax)
{
  HRESULT hr;

  TRACE("(0x%s,%p,%d)\n", wine_dbgstr_longlong(llBytes), lpszDest, cchMax);

  if (!lpszDest || !cchMax)
    return lpszDest;

  hr = StrFormatByteSizeEx(llBytes, SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS,
                           lpszDest, cchMax);

  if (FAILED(hr))
    return NULL;

  return lpszDest;
}

/*************************************************************************
 * StrFormatByteSizeEx  [SHLWAPI.@]
 *
 */

HRESULT WINAPI StrFormatByteSizeEx(LONGLONG llBytes, SFBS_FLAGS flags, LPWSTR lpszDest,
                                   UINT cchMax)
{
#define KB ((ULONGLONG)1024)
#define MB (KB*KB)
#define GB (KB*KB*KB)
#define TB (KB*KB*KB*KB)
#define PB (KB*KB*KB*KB*KB)

  static const SHLWAPI_BYTEFORMATS bfFormats[] =
  {
#ifdef __REACTOS__
    { 10*KB, 10.24, 100.0, 2, IDS_KB_FORMAT }, /* 10 KB */
    { 100*KB, 102.4, 10.0, 1, IDS_KB_FORMAT }, /* 100 KB */
    { 1000*KB, 1024.0, 1.0, 0, IDS_KB_FORMAT }, /* 1000 KB */
    { 10*MB, 10485.76, 100.0, 2, IDS_MB_FORMAT }, /* 10 MB */
    { 100*MB, 104857.6, 10.0, 1, IDS_MB_FORMAT }, /* 100 MB */
    { 1000*MB, 1048576.0, 1.0, 0, IDS_MB_FORMAT }, /* 1000 MB */
    { 10*GB, 10737418.24, 100.0, 2, IDS_GB_FORMAT }, /* 10 GB */
    { 100*GB, 107374182.4, 10.0, 1, IDS_GB_FORMAT }, /* 100 GB */
    { 1000*GB, 1073741824.0, 1.0, 0, IDS_GB_FORMAT }, /* 1000 GB */
    { 10*TB, 10485.76, 100.0, 2, IDS_TB_FORMAT }, /* 10 TB */
    { 100*TB, 104857.6, 10.0, 1, IDS_TB_FORMAT }, /* 100 TB */
    { 1000*TB, 1048576.0, 1.0, 0, IDS_TB_FORMAT }, /* 1000 TB */
    { 10*PB, 10737418.24, 100.00, 2, IDS_PB_FORMAT }, /* 10 PB */
    { 100*PB, 107374182.4, 10.00, 1, IDS_PB_FORMAT }, /* 100 PB */
    { 1000*PB, 1073741824.0, 1.00, 0, IDS_PB_FORMAT }, /* 1000 PB */
    { 0, 10995116277.76, 100.00, 2, IDS_EB_FORMAT } /* EB's, catch all */
#else
    { 10*KB, 10.24, 100.0, 2, 'K' }, /* 10 KB */
    { 100*KB, 102.4, 10.0, 1, 'K' }, /* 100 KB */
    { 1000*KB, 1024.0, 1.0, 0, 'K' }, /* 1000 KB */
    { 10*MB, 10485.76, 100.0, 2, 'M' }, /* 10 MB */
    { 100*MB, 104857.6, 10.0, 1, 'M' }, /* 100 MB */
    { 1000*MB, 1048576.0, 1.0, 0, 'M' }, /* 1000 MB */
    { 10*GB, 10737418.24, 100.0, 2, 'G' }, /* 10 GB */
    { 100*GB, 107374182.4, 10.0, 1, 'G' }, /* 100 GB */
    { 1000*GB, 1073741824.0, 1.0, 0, 'G' }, /* 1000 GB */
    { 10*TB, 10485.76, 100.0, 2, 'T' }, /* 10 TB */
    { 100*TB, 104857.6, 10.0, 1, 'T' }, /* 100 TB */
    { 1000*TB, 1048576.0, 1.0, 0, 'T' }, /* 1000 TB */
    { 10*PB, 10737418.24, 100.00, 2, 'P' }, /* 10 PB */
    { 100*PB, 107374182.4, 10.00, 1, 'P' }, /* 100 PB */
    { 1000*PB, 1073741824.0, 1.00, 0, 'P' }, /* 1000 PB */
    { 0, 10995116277.76, 100.00, 2, 'E' } /* EB's, catch all */
#endif
  };
#ifdef __REACTOS__
  WCHAR szBuff[40], wszFormat[40];
#else
  WCHAR wszAdd[] = {' ','?','B',0};
#endif
  double dBytes;
  UINT i = 0;

  TRACE("(0x%s,%d,%p,%d)\n", wine_dbgstr_longlong(llBytes), flags, lpszDest, cchMax);

  if (!cchMax)
    return E_INVALIDARG;

  if (llBytes < 1024)  /* 1K */
  {
    WCHAR wszBytesFormat[64];
    LoadStringW(shlwapi_hInstance, IDS_BYTES_FORMAT, wszBytesFormat, 64);
    swprintf(lpszDest, cchMax, wszBytesFormat, (int)llBytes);
    return S_OK;
  }

  /* Note that if this loop completes without finding a match, i will be
   * pointing at the last entry, which is a catch all for > 1000 PB
   */
  while (i < ARRAY_SIZE(bfFormats) - 1)
  {
    if (llBytes < bfFormats[i].dLimit)
      break;
    i++;
  }
  /* Above 1 TB we encounter problems with FP accuracy. So for amounts above
   * this number we integer shift down by 1 MB first. The table above has
   * the divisors scaled down from the '< 10 TB' entry onwards, to account
   * for this. We also add a small fudge factor to get the correct result for
   * counts that lie exactly on a 1024 byte boundary.
   */
  if (i > 8)
    dBytes = (double)(llBytes >> 20) + 0.001; /* Scale down by 1 MB */
  else
    dBytes = (double)llBytes + 0.00001;

  switch(flags)
  {
  case SFBS_FLAGS_ROUND_TO_NEAREST_DISPLAYED_DIGIT:
      dBytes = round(dBytes / bfFormats[i].dDivisor) / bfFormats[i].dNormaliser;
      break;
  case SFBS_FLAGS_TRUNCATE_UNDISPLAYED_DECIMAL_DIGITS:
      dBytes = floor(dBytes / bfFormats[i].dDivisor) / bfFormats[i].dNormaliser;
      break;
  default:
      return E_INVALIDARG;
  }

#ifdef __REACTOS__
  if (!FormatDouble(dBytes, bfFormats[i].nDecimals, szBuff, ARRAYSIZE(szBuff)))
    return E_FAIL;
  LoadStringW(shlwapi_hInstance, bfFormats[i].nFormatID, wszFormat, ARRAYSIZE(wszFormat));
  snprintfW(lpszDest, cchMax, wszFormat, szBuff);
#else
  if (!FormatDouble(dBytes, bfFormats[i].nDecimals, lpszDest, cchMax))
    return E_FAIL;

  wszAdd[1] = bfFormats[i].wPrefix;
  StrCatBuffW(lpszDest, wszAdd, cchMax);
#endif
  return S_OK;
}

/*************************************************************************
 * StrFormatByteSize64A	[SHLWAPI.@]
 *
 * See StrFormatByteSizeW.
 */
LPSTR WINAPI StrFormatByteSize64A(LONGLONG llBytes, LPSTR lpszDest, UINT cchMax)
{
  WCHAR wszBuff[32];

  StrFormatByteSizeW(llBytes, wszBuff, ARRAY_SIZE(wszBuff));

  if (lpszDest)
    WideCharToMultiByte(CP_ACP, 0, wszBuff, -1, lpszDest, cchMax, 0, 0);
  return lpszDest;
}

/*************************************************************************
 * StrFormatByteSizeA	[SHLWAPI.@]
 *
 * Create a string containing an abbreviated byte count of up to 2^31-1.
 *
 * PARAMS
 *  dwBytes  [I] Byte size to format
 *  lpszDest [I] Destination for formatted string
 *  cchMax   [I] Size of lpszDest
 *
 * RETURNS
 *  lpszDest.
 *
 * NOTES
 *  The ANSI and Unicode versions of this function accept a different
 *  integer type for dwBytes. See StrFormatByteSize64A().
 */
LPSTR WINAPI StrFormatByteSizeA(DWORD dwBytes, LPSTR lpszDest, UINT cchMax)
{
  TRACE("(%ld,%p,%d)\n", dwBytes, lpszDest, cchMax);

  return StrFormatByteSize64A(dwBytes, lpszDest, cchMax);
}

/*************************************************************************
 *      @	[SHLWAPI.203]
 *
 * Remove a single non-trailing ampersand ('&') from a string.
 *
 * PARAMS
 *  lpszStr [I/O] String to remove ampersand from.
 *
 * RETURNS
 *  The character after the first ampersand in lpszStr, or the first character
 *  in lpszStr if there is no ampersand in the string.
 */
char WINAPI SHStripMneumonicA(LPCSTR lpszStr)
{
  LPSTR lpszIter, lpszTmp;
  char ch;

  TRACE("(%s)\n", debugstr_a(lpszStr));

  ch = *lpszStr;

  if ((lpszIter = StrChrA(lpszStr, '&')))
  {
    lpszTmp = CharNextA(lpszIter);
    if (*lpszTmp)
    {
      if (*lpszTmp != '&')
        ch =  *lpszTmp;

      memmove( lpszIter, lpszTmp, strlen(lpszTmp) + 1 );
    }
  }

  return ch;
}

/*************************************************************************
 *      @	[SHLWAPI.225]
 *
 * Unicode version of SHStripMneumonicA.
 */
WCHAR WINAPI SHStripMneumonicW(LPCWSTR lpszStr)
{
  LPWSTR lpszIter, lpszTmp;
  WCHAR ch;

  TRACE("(%s)\n", debugstr_w(lpszStr));

  ch = *lpszStr;

  if ((lpszIter = StrChrW(lpszStr, '&')))
  {
    lpszTmp = lpszIter + 1;
    if (*lpszTmp)
    {
      if (*lpszTmp != '&')
        ch =  *lpszTmp;

      memmove( lpszIter, lpszTmp, (lstrlenW(lpszTmp) + 1) * sizeof(WCHAR) );
    }
  }

  return ch;
}

/*************************************************************************
 *      @	[SHLWAPI.216]
 *
 * Convert an ANSI string to Unicode.
 *
 * PARAMS
 * dwCp     [I] Code page for the conversion
 * lpSrcStr [I] Source ANSI string to convert
 * lpDstStr [O] Destination for converted Unicode string
 * iLen     [I] Length of lpDstStr
 *
 * RETURNS
 *  The return value of the MultiByteToWideChar() function called on lpSrcStr.
 */
DWORD WINAPI SHAnsiToUnicodeCP(DWORD dwCp, LPCSTR lpSrcStr, LPWSTR lpDstStr, int iLen)
{
  DWORD dwRet;

  dwRet = MultiByteToWideChar(dwCp, 0, lpSrcStr, -1, lpDstStr, iLen);
  TRACE("%s->%s,ret=%ld\n", debugstr_a(lpSrcStr), debugstr_w(lpDstStr), dwRet);
  return dwRet;
}

/*************************************************************************
 *      @	[SHLWAPI.215]
 *
 * Convert an ANSI string to Unicode.
 *
 * PARAMS
 * lpSrcStr [I] Source ANSI string to convert
 * lpDstStr [O] Destination for converted Unicode string
 * iLen     [I] Length of lpDstStr
 *
 * RETURNS
 *  The return value of the MultiByteToWideChar() function called on lpSrcStr.
 *
 * NOTES
 *  This function simply calls SHAnsiToUnicodeCP with code page CP_ACP.
 */
DWORD WINAPI SHAnsiToUnicode(LPCSTR lpSrcStr, LPWSTR lpDstStr, int iLen)
{
  return SHAnsiToUnicodeCP(CP_ACP, lpSrcStr, lpDstStr, iLen);
}

/*************************************************************************
 *      @	[SHLWAPI.218]
 *
 * Convert a Unicode string to ANSI.
 *
 * PARAMS
 *  CodePage [I] Code page to use for the conversion
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDstStr [O] Destination for converted ANSI string
 *  dstlen   [I] Length of buffer at lpDstStr
 *
 * RETURNS
 *  Success: The length in bytes of the result at lpDstStr (including the terminator)
 *  Failure: When using CP_UTF8, CP_UTF7 or 0xc350 as codePage, 0 is returned and
 *           the result is not nul-terminated.
 *           When using a different codepage, the length in bytes of the truncated
 *           result at lpDstStr (including the terminator) is returned and
 *           lpDstStr is always nul-terminated.
 *
 */
DWORD WINAPI SHUnicodeToAnsiCP(UINT CodePage, LPCWSTR lpSrcStr, LPSTR lpDstStr, int dstlen)
{
  static const WCHAR emptyW[] = { '\0' };
  int len , reqLen;
  LPSTR mem;

  if (!lpDstStr || !dstlen)
    return 0;

  if (!lpSrcStr)
    lpSrcStr = emptyW;

  *lpDstStr = '\0';

  len = lstrlenW(lpSrcStr) + 1;

  switch (CodePage)
  {
  case CP_WINUNICODE:
    CodePage = CP_UTF8; /* Fall through... */
  case 0x0000C350: /* FIXME: CP_ #define */
  case CP_UTF7:
  case CP_UTF8:
    {
      DWORD dwMode = 0;
      INT lenW = len - 1;
      INT needed = dstlen - 1;
      HRESULT hr;

      /* try the user supplied buffer first */
      hr = ConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &lenW, lpDstStr, &needed);
      if (hr == S_OK)
      {
        lpDstStr[needed] = '\0';
        return needed + 1;
      }

      /* user buffer too small. exclude termination and copy as much as possible */
      lenW = len;
      hr = ConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &lenW, NULL, &needed);
      needed++;
      mem = malloc(needed);
      if (!mem)
        return 0;

      hr = ConvertINetUnicodeToMultiByte(&dwMode, CodePage, lpSrcStr, &len, mem, &needed);
      if (hr == S_OK)
      {
          reqLen = SHTruncateString(mem, dstlen);
          if (reqLen > 0) memcpy(lpDstStr, mem, reqLen-1);
      }
      free(mem);
      return 0;
    }
  default:
    break;
  }

  /* try the user supplied buffer first */
  reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, lpDstStr, dstlen, NULL, NULL);

  if (!reqLen && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
  {
    reqLen = WideCharToMultiByte(CodePage, 0, lpSrcStr, len, NULL, 0, NULL, NULL);
    if (reqLen)
    {
      mem = malloc(reqLen);
      if (mem)
      {
        WideCharToMultiByte(CodePage, 0, lpSrcStr, len, mem, reqLen, NULL, NULL);

        reqLen = SHTruncateString(mem, dstlen -1);
        reqLen++;

        lstrcpynA(lpDstStr, mem, reqLen);
        free(mem);
        lpDstStr[reqLen-1] = '\0';
      }
    }
  }
  return reqLen;
}

/*************************************************************************
 *      @	[SHLWAPI.217]
 *
 * Convert a Unicode string to ANSI.
 *
 * PARAMS
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDstStr [O] Destination for converted ANSI string
 *  iLen     [O] Length of lpDstStr in characters
 *
 * RETURNS
 *  See SHUnicodeToAnsiCP

 * NOTES
 *  This function simply calls SHUnicodeToAnsiCP() with CodePage = CP_ACP.
 */
INT WINAPI SHUnicodeToAnsi(LPCWSTR lpSrcStr, LPSTR lpDstStr, INT iLen)
{
    return SHUnicodeToAnsiCP(CP_ACP, lpSrcStr, lpDstStr, iLen);
}

/*************************************************************************
 *      @	[SHLWAPI.364]
 *
 * Determine if an ANSI string converts to Unicode and back identically.
 *
 * PARAMS
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDst    [O] Destination for resulting ANSI string
 *  iLen     [I] Length of lpDst in characters
 *
 * RETURNS
 *  TRUE, since ANSI strings always convert identically.
 */
BOOL WINAPI DoesStringRoundTripA(LPCSTR lpSrcStr, LPSTR lpDst, INT iLen)
{
  lstrcpynA(lpDst, lpSrcStr, iLen);
  return TRUE;
}

/*************************************************************************
 *      @	[SHLWAPI.365]
 *
 * Determine if a Unicode string converts to ANSI and back identically.
 *
 * PARAMS
 *  lpSrcStr [I] Source Unicode string to convert
 *  lpDst    [O] Destination for resulting ANSI string
 *  iLen     [I] Length of lpDst in characters
 *
 * RETURNS
 *  TRUE, if lpSrcStr converts to ANSI and back identically,
 *  FALSE otherwise.
 */
BOOL WINAPI DoesStringRoundTripW(LPCWSTR lpSrcStr, LPSTR lpDst, INT iLen)
{
    WCHAR szBuff[MAX_PATH];

    SHUnicodeToAnsi(lpSrcStr, lpDst, iLen);
    SHAnsiToUnicode(lpDst, szBuff, MAX_PATH);
    return !wcscmp(lpSrcStr, szBuff);
}

BOOL WINAPI IsCharSpaceA(CHAR c)
{
    WORD CharType;
    return GetStringTypeA(GetSystemDefaultLCID(), CT_CTYPE1, &c, 1, &CharType) && (CharType & C1_SPACE);
}

/*************************************************************************
 *      @	[SHLWAPI.29]
 *
 * Determine if a Unicode character is a space.
 *
 * PARAMS
 *  wc [I] Character to check.
 *
 * RETURNS
 *  TRUE, if wc is a space,
 *  FALSE otherwise.
 */
BOOL WINAPI IsCharSpaceW(WCHAR wc)
{
    WORD CharType;

    return GetStringTypeW(CT_CTYPE1, &wc, 1, &CharType) && (CharType & C1_SPACE);
}
