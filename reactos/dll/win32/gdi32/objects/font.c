/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/gdi32/object/font.c
 * PURPOSE:
 * PROGRAMMER:
 *
 */

#include "precomp.h"

#define NDEBUG
#include <debug.h>

/*
 *  For TranslateCharsetInfo
 */
#define FS(x) {{0,0,0,0},{0x1<<(x),0}}
#define MAXTCIINDEX 32
static const CHARSETINFO FONT_tci[MAXTCIINDEX] = {
  /* ANSI */
  { ANSI_CHARSET, 1252, FS(0)},
  { EASTEUROPE_CHARSET, 1250, FS(1)},
  { RUSSIAN_CHARSET, 1251, FS(2)},
  { GREEK_CHARSET, 1253, FS(3)},
  { TURKISH_CHARSET, 1254, FS(4)},
  { HEBREW_CHARSET, 1255, FS(5)},
  { ARABIC_CHARSET, 1256, FS(6)},
  { BALTIC_CHARSET, 1257, FS(7)},
  { VIETNAMESE_CHARSET, 1258, FS(8)},
  /* reserved by ANSI */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  /* ANSI and OEM */
  { THAI_CHARSET,  874,  FS(16)},
  { SHIFTJIS_CHARSET, 932, FS(17)},
  { GB2312_CHARSET, 936, FS(18)},
  { HANGEUL_CHARSET, 949, FS(19)},
  { CHINESEBIG5_CHARSET, 950, FS(20)},
  { JOHAB_CHARSET, 1361, FS(21)},
  /* reserved for alternate ANSI and OEM */
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  { DEFAULT_CHARSET, 0, FS(0)},
  /* reserved for system */
  { DEFAULT_CHARSET, 0, FS(0)},
  { SYMBOL_CHARSET, CP_SYMBOL, FS(31)},
};

#define INITIAL_FAMILY_COUNT 64

/***********************************************************************
 *              TEXTMETRIC conversion functions.
 */
static void FONT_TextMetricWToA(const TEXTMETRICW *ptmW, LPTEXTMETRICA ptmA )
{
    ptmA->tmHeight = ptmW->tmHeight;
    ptmA->tmAscent = ptmW->tmAscent;
    ptmA->tmDescent = ptmW->tmDescent;
    ptmA->tmInternalLeading = ptmW->tmInternalLeading;
    ptmA->tmExternalLeading = ptmW->tmExternalLeading;
    ptmA->tmAveCharWidth = ptmW->tmAveCharWidth;
    ptmA->tmMaxCharWidth = ptmW->tmMaxCharWidth;
    ptmA->tmWeight = ptmW->tmWeight;
    ptmA->tmOverhang = ptmW->tmOverhang;
    ptmA->tmDigitizedAspectX = ptmW->tmDigitizedAspectX;
    ptmA->tmDigitizedAspectY = ptmW->tmDigitizedAspectY;
    ptmA->tmFirstChar = ptmW->tmFirstChar > 255 ? 255 : ptmW->tmFirstChar;
    ptmA->tmLastChar = ptmW->tmLastChar > 255 ? 255 : ptmW->tmLastChar;
    ptmA->tmDefaultChar = ptmW->tmDefaultChar > 255 ? 255 : ptmW->tmDefaultChar;
    ptmA->tmBreakChar = ptmW->tmBreakChar > 255 ? 255 : ptmW->tmBreakChar;
    ptmA->tmItalic = ptmW->tmItalic;
    ptmA->tmUnderlined = ptmW->tmUnderlined;
    ptmA->tmStruckOut = ptmW->tmStruckOut;
    ptmA->tmPitchAndFamily = ptmW->tmPitchAndFamily;
    ptmA->tmCharSet = ptmW->tmCharSet;
}

/***********************************************************************
 *           FONT_mbtowc
 *
 * Returns a Unicode translation of str using the charset of the
 * currently selected font in hdc.  If count is -1 then str is assumed
 * to be '\0' terminated, otherwise it contains the number of bytes to
 * convert.  If plenW is non-NULL, on return it will point to the
 * number of WCHARs that have been written.  If pCP is non-NULL, on
 * return it will point to the codepage used in the conversion.  The
 * caller should free the returned LPWSTR from the process heap
 * itself.
 */
static LPWSTR FONT_mbtowc(HDC hdc, LPCSTR str, INT count, INT *plenW, UINT *pCP)
{
    UINT cp = CP_ACP; // GdiGetCodePage( hdc );
    INT lenW;
    LPWSTR strW;

    if(count == -1) count = strlen(str);
    lenW = MultiByteToWideChar(cp, 0, str, count, NULL, 0);
    strW = HeapAlloc(GetProcessHeap(), 0, lenW*sizeof(WCHAR));
    MultiByteToWideChar(cp, 0, str, count, strW, lenW);
    DPRINT1("mapped %s -> %s  \n", str, strW);
    if(plenW) *plenW = lenW;
    if(pCP) *pCP = cp;
    return strW;
}


static BOOL FASTCALL
MetricsCharConvert(WCHAR w, UCHAR *b)
  {
  UNICODE_STRING WString;
  WCHAR WBuf[2];
  ANSI_STRING AString;
  CHAR ABuf[2];
  NTSTATUS Status;

  if (L'\0' == w)
    {
      *b = '\0';
    }
  else
    {
      WBuf[0] = w;
      WBuf[1] = L'\0';
      RtlInitUnicodeString(&WString, WBuf);
      ABuf[0] = '*';
      ABuf[1] = L'\0';
      RtlInitAnsiString(&AString, ABuf);

      Status = RtlUnicodeStringToAnsiString(&AString, &WString, FALSE);
      if (! NT_SUCCESS(Status))
	{
	  SetLastError(RtlNtStatusToDosError(Status));
	  return FALSE;
	}
      *b = ABuf[0];
    }

  return TRUE;
}

BOOL FASTCALL
TextMetricW2A(TEXTMETRICA *tma, TEXTMETRICW *tmw)
{
  UNICODE_STRING WString;
  WCHAR WBuf[256];
  ANSI_STRING AString;
  CHAR ABuf[256];
  UINT Letter;
  NTSTATUS Status;

  tma->tmHeight = tmw->tmHeight;
  tma->tmAscent = tmw->tmAscent;
  tma->tmDescent = tmw->tmDescent;
  tma->tmInternalLeading = tmw->tmInternalLeading;
  tma->tmExternalLeading = tmw->tmExternalLeading;
  tma->tmAveCharWidth = tmw->tmAveCharWidth;
  tma->tmMaxCharWidth = tmw->tmMaxCharWidth;
  tma->tmWeight = tmw->tmWeight;
  tma->tmOverhang = tmw->tmOverhang;
  tma->tmDigitizedAspectX = tmw->tmDigitizedAspectX;
  tma->tmDigitizedAspectY = tmw->tmDigitizedAspectY;

  /* The Unicode FirstChar/LastChar need not correspond to the ANSI
     FirstChar/LastChar. For example, if the font contains glyphs for
     letters A-Z and an accented version of the letter e, the Unicode
     FirstChar would be A and the Unicode LastChar would be the accented
     e. If you just translate those to ANSI, the range would become
     letters A-E instead of A-Z.
     We translate all possible ANSI chars to Unicode and find the first
     and last translated character which fall into the Unicode FirstChar/
     LastChar range and return the corresponding ANSI char. */

  /* Setup an Ansi string containing all possible letters (note: skip '\0' at
     the beginning since that would be interpreted as end-of-string, handle
     '\0' special later */
  for (Letter = 1; Letter < 256; Letter++)
    {
    ABuf[Letter - 1] = (CHAR) Letter;
    WBuf[Letter - 1] = L' ';
    }
  ABuf[255] = '\0';
  WBuf[255] = L'\0';
  RtlInitAnsiString(&AString, ABuf);
  RtlInitUnicodeString(&WString, WBuf);

  /* Find the corresponding Unicode characters */
  Status = RtlAnsiStringToUnicodeString(&WString, &AString, FALSE);
  if (! NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }

  /* Scan for the first ANSI character which maps to an Unicode character
     in the range */
  tma->tmFirstChar = '\0';
  if (L'\0' != tmw->tmFirstChar)
    {
      for (Letter = 1; Letter < 256; Letter++)
	{
	  if (tmw->tmFirstChar <= WBuf[Letter - 1] &&
	      WBuf[Letter - 1] <= tmw->tmLastChar)
	    {
	      tma->tmFirstChar = (CHAR) Letter;
	      break;
	    }
	}
    }

  /* Scan for the last ANSI character which maps to an Unicode character
     in the range */
  tma->tmLastChar = '\0';
  if (L'\0' != tmw->tmLastChar)
    {
      for (Letter = 255; 0 < Letter; Letter--)
	{
	  if (tmw->tmFirstChar <= WBuf[Letter - 1] &&
	      WBuf[Letter - 1] <= tmw->tmLastChar)
	    {
	      tma->tmLastChar = (CHAR) Letter;
	      break;
	    }
	}
    }

  if (! MetricsCharConvert(tmw->tmDefaultChar, &tma->tmDefaultChar) ||
      ! MetricsCharConvert(tmw->tmBreakChar, &tma->tmBreakChar))
    {
      return FALSE;
    }

  tma->tmItalic = tmw->tmItalic;
  tma->tmUnderlined = tmw->tmUnderlined;
  tma->tmStruckOut = tmw->tmStruckOut;
  tma->tmPitchAndFamily = tmw->tmPitchAndFamily;
  tma->tmCharSet = tmw->tmCharSet;

  return TRUE;
}

BOOL FASTCALL
NewTextMetricW2A(NEWTEXTMETRICA *tma, NEWTEXTMETRICW *tmw)
{
  if (! TextMetricW2A((TEXTMETRICA *) tma, (TEXTMETRICW *) tmw))
    {
      return FALSE;
    }

  tma->ntmFlags = tmw->ntmFlags;
  tma->ntmSizeEM = tmw->ntmSizeEM;
  tma->ntmCellHeight = tmw->ntmCellHeight;
  tma->ntmAvgWidth = tmw->ntmAvgWidth;

  return TRUE;
}

BOOL FASTCALL
NewTextMetricExW2A(NEWTEXTMETRICEXA *tma, NEWTEXTMETRICEXW *tmw)
{
  if (! NewTextMetricW2A(&tma->ntmTm, &tmw->ntmTm))
    {
      return FALSE;
    }

  tma->ntmFontSig = tmw->ntmFontSig;

  return TRUE;
}

static int FASTCALL
IntEnumFontFamilies(HDC Dc, LPLOGFONTW LogFont, PVOID EnumProc, LPARAM lParam,
                    BOOL Unicode)
{
  int FontFamilyCount;
  int FontFamilySize;
  PFONTFAMILYINFO Info;
  int Ret = 0;
  int i;
  ENUMLOGFONTEXA EnumLogFontExA;
  NEWTEXTMETRICEXA NewTextMetricExA;

  Info = RtlAllocateHeap(GetProcessHeap(), 0,
                         INITIAL_FAMILY_COUNT * sizeof(FONTFAMILYINFO));
  if (NULL == Info)
    {
      return 0;
    }
  FontFamilyCount = NtGdiGetFontFamilyInfo(Dc, LogFont, Info, INITIAL_FAMILY_COUNT);
  if (FontFamilyCount < 0)
    {
      RtlFreeHeap(GetProcessHeap(), 0, Info);
      return 0;
    }
  if (INITIAL_FAMILY_COUNT < FontFamilyCount)
    {
      FontFamilySize = FontFamilyCount;
      RtlFreeHeap(GetProcessHeap(), 0, Info);
      Info = RtlAllocateHeap(GetProcessHeap(), 0,
                             FontFamilyCount * sizeof(FONTFAMILYINFO));
      if (NULL == Info)
        {
          return 0;
        }
      FontFamilyCount = NtGdiGetFontFamilyInfo(Dc, LogFont, Info, FontFamilySize);
      if (FontFamilyCount < 0 || FontFamilySize < FontFamilyCount)
        {
          RtlFreeHeap(GetProcessHeap(), 0, Info);
          return 0;
        }
    }

  for (i = 0; i < FontFamilyCount; i++)
    {
      if (Unicode)
        {
          Ret = ((FONTENUMPROCW) EnumProc)(
            &Info[i].EnumLogFontEx,
            &Info[i].NewTextMetricEx,
            Info[i].FontType, lParam);
        }
      else
        { // Could use EnumLogFontExW2A here?
          LogFontW2A(&EnumLogFontExA.elfLogFont, &Info[i].EnumLogFontEx.elfLogFont);
          WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfFullName, -1,
                              (LPSTR)EnumLogFontExA.elfFullName, LF_FULLFACESIZE, NULL, NULL);
          WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfStyle, -1,
                              (LPSTR)EnumLogFontExA.elfStyle, LF_FACESIZE, NULL, NULL);
          WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfScript, -1,
                              (LPSTR)EnumLogFontExA.elfScript, LF_FACESIZE, NULL, NULL);
          NewTextMetricExW2A(&NewTextMetricExA,
                             &Info[i].NewTextMetricEx);
          Ret = ((FONTENUMPROCA) EnumProc)(
            &EnumLogFontExA,
            &NewTextMetricExA,
            Info[i].FontType, lParam);
        }
    }

  RtlFreeHeap(GetProcessHeap(), 0, Info);

  return Ret;
}

/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesExW(HDC hdc, LPLOGFONTW lpLogfont, FONTENUMPROCW lpEnumFontFamExProc,
                    LPARAM lParam, DWORD dwFlags)
{
  return IntEnumFontFamilies(hdc, lpLogfont, lpEnumFontFamExProc, lParam, TRUE);
}


/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesW(HDC hdc, LPCWSTR lpszFamily, FONTENUMPROCW lpEnumFontFamProc,
                  LPARAM lParam)
{
  LOGFONTW LogFont;

  ZeroMemory(&LogFont, sizeof(LOGFONTW));
  LogFont.lfCharSet = DEFAULT_CHARSET;
  if (NULL != lpszFamily)
    {
      lstrcpynW(LogFont.lfFaceName, lpszFamily, LF_FACESIZE);
    }

  return IntEnumFontFamilies(hdc, &LogFont, lpEnumFontFamProc, lParam, TRUE);
}


/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesExA (HDC hdc, LPLOGFONTA lpLogfont, FONTENUMPROCA lpEnumFontFamExProc,
                     LPARAM lParam, DWORD dwFlags)
{
  LOGFONTW LogFontW;

  LogFontA2W(&LogFontW, lpLogfont);

  /* no need to convert LogFontW back to lpLogFont b/c it's an [in] parameter only */
  return IntEnumFontFamilies(hdc, &LogFontW, lpEnumFontFamExProc, lParam, FALSE);
}


/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesA(HDC hdc, LPCSTR lpszFamily, FONTENUMPROCA lpEnumFontFamProc,
                  LPARAM lParam)
{
  LOGFONTW LogFont;

  ZeroMemory(&LogFont, sizeof(LOGFONTW));
  LogFont.lfCharSet = DEFAULT_CHARSET;
  if (NULL != lpszFamily)
    {
      MultiByteToWideChar(CP_THREAD_ACP, 0, lpszFamily, -1, LogFont.lfFaceName, LF_FACESIZE);
    }

  return IntEnumFontFamilies(hdc, &LogFont, lpEnumFontFamProc, lParam, FALSE);
}

/*
 * @implemented
 */
DWORD
STDCALL
GetCharacterPlacementW(
	HDC hdc,
	LPCWSTR lpString,
	INT uCount,
	INT nMaxExtent,
	GCP_RESULTSW *lpResults,
	DWORD dwFlags
	)
{
  DWORD ret=0;
  SIZE size;
  UINT i, nSet;
  DPRINT("GetCharacterPlacementW\n");

  if(dwFlags&(~GCP_REORDER)) DPRINT("flags 0x%08lx ignored\n", dwFlags);
  if(lpResults->lpClass) DPRINT("classes not implemented\n");
  if (lpResults->lpCaretPos && (dwFlags & GCP_REORDER))
    DPRINT("Caret positions for complex scripts not implemented\n");

  nSet = (UINT)uCount;
  if(nSet > lpResults->nGlyphs)
    nSet = lpResults->nGlyphs;

  /* return number of initialized fields */
  lpResults->nGlyphs = nSet;

/*if((dwFlags&GCP_REORDER)==0 || !BidiAvail)
  {*/
    /* Treat the case where no special handling was requested in a fastpath way */
    /* copy will do if the GCP_REORDER flag is not set */
    if(lpResults->lpOutString)
      lstrcpynW( lpResults->lpOutString, lpString, nSet );

    if(lpResults->lpGlyphs)
      lstrcpynW( lpResults->lpGlyphs, lpString, nSet );

    if(lpResults->lpOrder)
    {
      for(i = 0; i < nSet; i++)
      lpResults->lpOrder[i] = i;
    }
/*} else
  {
      BIDI_Reorder( lpString, uCount, dwFlags, WINE_GCPW_FORCE_LTR, lpResults->lpOutString,
                    nSet, lpResults->lpOrder );
  }*/

  /* FIXME: Will use the placement chars */
  if (lpResults->lpDx)
  {
    int c;
    for (i = 0; i < nSet; i++)
    {
      if (GetCharWidth32W(hdc, lpString[i], lpString[i], &c))
        lpResults->lpDx[i]= c;
    }
  }

  if (lpResults->lpCaretPos && !(dwFlags & GCP_REORDER))
  {
    int pos = 0;

    lpResults->lpCaretPos[0] = 0;
    for (i = 1; i < nSet; i++)
      if (GetTextExtentPoint32W(hdc, &(lpString[i - 1]), 1, &size))
        lpResults->lpCaretPos[i] = (pos += size.cx);
  }

  /*if(lpResults->lpGlyphs)
    NtGdiGetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);*/

  if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
    ret = MAKELONG(size.cx, size.cy);

  return ret;
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharABCWidthsFloatW(HDC hdc,
                       UINT FirstChar,
                       UINT LastChar,
                       LPABCFLOAT abcF)
{
DPRINT("GetCharABCWidthsFloatW\n");
 if ((!abcF) || (FirstChar > LastChar))
 {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
 }
 return NtGdiGetCharABCWidthsW( hdc,
                          FirstChar,
  (ULONG)(LastChar - FirstChar + 1),
                      (PWCHAR) NULL,
                                  0,
                        (PVOID)abcF);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharWidthFloatW(HDC hdc,
                   UINT iFirstChar,
                   UINT iLastChar,
                   PFLOAT pxBuffer)
{
DPRINT("GetCharWidthsFloatW\n");
 if ((!pxBuffer) || (iFirstChar > iLastChar))
 {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
 }
 return NtGdiGetCharWidthW( hdc,
                     iFirstChar,
  (ULONG)(iLastChar - iFirstChar + 1),
                  (PWCHAR) NULL,
                              0, 
	       (PVOID) pxBuffer);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharWidthW(HDC hdc,
              UINT iFirstChar,
              UINT iLastChar,
              LPINT lpBuffer)
{
DPRINT("GetCharWidthsW\n");
 if ((!lpBuffer) || (iFirstChar > iLastChar))
 {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
 }
 return NtGdiGetCharWidthW( hdc,
                     iFirstChar,
  (ULONG)(iLastChar - iFirstChar + 1),
                  (PWCHAR) NULL,
                    GCW_NOFLOAT,
	       (PVOID) lpBuffer);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharWidth32W(HDC hdc,
               UINT iFirstChar,
               UINT iLastChar,
               LPINT lpBuffer)
{
DPRINT("GetCharWidths32W\n");
 if ((!lpBuffer) || (iFirstChar > iLastChar))
 {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
 }
 return NtGdiGetCharWidthW( hdc,
                     iFirstChar,
  (ULONG)(iLastChar - iFirstChar + 1),
                  (PWCHAR) NULL,
          GCW_NOFLOAT|GCW_WIN32,
	       (PVOID) lpBuffer);
}


/*
 * @implemented
 *
 */
BOOL
STDCALL
GetCharABCWidthsW(HDC hdc,
                  UINT FirstChar,
                  UINT LastChar,
                  LPABC lpabc)
{
DPRINT("GetCharABCWidthsW\n");
 if ((!lpabc) || (FirstChar > LastChar))
 {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
 }
 return NtGdiGetCharABCWidthsW( hdc,
                          FirstChar,
  (ULONG)(LastChar - FirstChar + 1),
                      (PWCHAR) NULL,
                     GCABCW_NOFLOAT,
		       (PVOID)lpabc);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidthA(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
DPRINT("GetCharWidthsA\n");
    INT i, wlen, count = (INT)(iLastChar - iFirstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(iFirstChar + i);

    wstr = FONT_mbtowc(NULL, str, count, &wlen, NULL);

    ret = NtGdiGetCharWidthW( hdc,
                          wstr[0],
		    (ULONG) count,
		    (PWCHAR) wstr,
		      GCW_NOFLOAT,
		  (PVOID) lpBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidth32A(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
DPRINT("GetCharWidths32A\n");
    INT i, wlen, count = (INT)(iLastChar - iFirstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(iFirstChar + i);

    wstr = FONT_mbtowc(NULL, str, count, &wlen, NULL);

    ret = NtGdiGetCharWidthW( hdc,
                          wstr[0],
	            (ULONG) count,
	            (PWCHAR) wstr,
	    GCW_NOFLOAT|GCW_WIN32, 
                 (PVOID) lpBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharWidthFloatA(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	PFLOAT	pxBuffer
	)
{
DPRINT("GetCharWidthsFloatA\n");
    INT i, wlen, count = (INT)(iLastChar - iFirstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(iFirstChar + i);

    wstr = FONT_mbtowc(NULL, str, count, &wlen, NULL);

    ret = NtGdiGetCharWidthW( hdc, wstr[0], (ULONG) count, (PWCHAR) wstr, 0, (PVOID) pxBuffer);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsA(
	HDC	hdc,
	UINT	uFirstChar,
	UINT	uLastChar,
	LPABC	lpabc
	)
{
DPRINT("GetCharABCWidthsA\n");
    INT i, wlen, count = (INT)(uLastChar - uFirstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if(count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);
    for(i = 0; i < count; i++)
	str[i] = (BYTE)(uFirstChar + i);

     wstr = FONT_mbtowc(hdc, str, count, &wlen, NULL);

    ret = NtGdiGetCharABCWidthsW( hdc, 
                              wstr[0],
                         (ULONG)count,
                         (PWCHAR)wstr,
		       GCABCW_NOFLOAT,
		         (PVOID)lpabc);

    HeapFree(GetProcessHeap(), 0, str);
    HeapFree(GetProcessHeap(), 0, wstr);

    return ret;
}

/*
 * @implemented
 */
BOOL
APIENTRY
GetCharABCWidthsFloatA(
	HDC		hdc,
	UINT		iFirstChar,
	UINT		iLastChar,
	LPABCFLOAT	lpABCF
	)
{
DPRINT("GetCharABCWidthsFloatA\n");
    INT i, wlen, count = (INT)(iLastChar - iFirstChar + 1);
    LPSTR str;
    LPWSTR wstr;
    BOOL ret = TRUE;

    if (count <= 0) return FALSE;

    str = HeapAlloc(GetProcessHeap(), 0, count);

    for(i = 0; i < count; i++)
        str[i] = (BYTE)(iFirstChar + i);

    wstr = FONT_mbtowc( hdc, str, count, &wlen, NULL );

    ret = NtGdiGetCharABCWidthsW( hdc,wstr[0],(ULONG)count, (PWCHAR)wstr, 0, (PVOID)lpABCF);

    HeapFree( GetProcessHeap(), 0, str );
    HeapFree( GetProcessHeap(), 0, wstr );

    return ret;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetCharABCWidthsI(HDC hdc,
                  UINT giFirst,
                  UINT cgi,
                  LPWORD pgi,
                  LPABC lpabc)
{
DPRINT("GetCharABCWidthsI\n");
 return NtGdiGetCharABCWidthsW( hdc,
                            giFirst,
                        (ULONG) cgi,
                       (PWCHAR) pgi,
      GCABCW_NOFLOAT|GCABCW_INDICES,
                       (PVOID)lpabc);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidthI(HDC hdc,
              UINT giFirst,
              UINT cgi,
              LPWORD pgi,
              LPINT lpBuffer
)
{
DPRINT("GetCharWidthsI\n");
 if (!lpBuffer || (!pgi && (giFirst == MAXUSHORT))) // Cannot be at max.
 {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
 }
 if (!cgi) return TRUE;
 return NtGdiGetCharWidthW( hdc,
                        giFirst,
                            cgi,
	           (PWCHAR) pgi,
    GCW_INDICES|GCW_NOFLOAT|GCW_WIN32,
               (PVOID) lpBuffer );
}

/*
 * @implemented
 */
DWORD
STDCALL
GetGlyphIndicesA(
        HDC hdc,
        LPCSTR lpstr,
        INT count,
        LPWORD pgi,
        DWORD flags
        )
{
    DWORD Ret;
    WCHAR *lpstrW;
    INT countW;

    lpstrW = FONT_mbtowc(hdc, lpstr, count, &countW, NULL);
    Ret = NtGdiGetGlyphIndicesW(hdc, lpstrW, countW, pgi, flags);
    HeapFree(GetProcessHeap(), 0, lpstrW);
    return Ret;
}

/*
 * @implemented
 */
DWORD
STDCALL
GetGlyphOutlineA(
	HDC		hdc,
	UINT		uChar,
	UINT		uFormat,
	LPGLYPHMETRICS	lpgm,
	DWORD		cbBuffer,
	LPVOID		lpvBuffer,
	CONST MAT2	*lpmat2
	)
{

    LPWSTR p = NULL;
    DWORD ret;
    UINT c;
    DPRINT1("GetGlyphOutlineA  uChar %x\n", uChar);
    if(!(uFormat & GGO_GLYPH_INDEX)) {
        int len;
        char mbchs[2];
        if(uChar > 0xff) { /* but, 2 bytes character only */
            len = 2;
            mbchs[0] = (uChar & 0xff00) >> 8;
            mbchs[1] = (uChar & 0xff);
        } else {
            len = 1;
            mbchs[0] = (uChar & 0xff);
        }
        p = FONT_mbtowc(NULL, mbchs, len, NULL, NULL);
	c = p[0];
    } else
        c = uChar;
    ret = NtGdiGetGlyphOutline(hdc, c, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
    HeapFree(GetProcessHeap(), 0, p);
    return ret;
}


/*
 * @implemented
 */
DWORD
STDCALL
GetGlyphOutlineW(
	HDC		hdc,
	UINT		uChar,
	UINT		uFormat,
	LPGLYPHMETRICS	lpgm,
	DWORD		cbBuffer,
	LPVOID		lpvBuffer,
	CONST MAT2	*lpmat2
	)
{
  DPRINT("GetGlyphOutlineW  uChar %x\n", uChar);
  if (!lpgm & !lpmat2) return GDI_ERROR;
  if (!lpvBuffer) cbBuffer = 0;
  return NtGdiGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
}


/*
 * @implemented
 */
UINT
APIENTRY
GetOutlineTextMetricsA(
	HDC			hdc,
	UINT			cbData,
	LPOUTLINETEXTMETRICA	lpOTM
	)
{
    char buf[512], *ptr;
    UINT ret, needed;
    OUTLINETEXTMETRICW *lpOTMW = (OUTLINETEXTMETRICW *)buf;
    OUTLINETEXTMETRICA *output = lpOTM;
    INT left, len;

    if((ret = GetOutlineTextMetricsW(hdc, 0, NULL)) == 0)
        return 0;
    if(ret > sizeof(buf))
	lpOTMW = HeapAlloc(GetProcessHeap(), 0, ret);
    GetOutlineTextMetricsW(hdc, ret, lpOTMW);

    needed = sizeof(OUTLINETEXTMETRICA);
    if(lpOTMW->otmpFamilyName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFamilyName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFaceName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFaceName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpStyleName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpStyleName), -1,
				      NULL, 0, NULL, NULL);
    if(lpOTMW->otmpFullName)
        needed += WideCharToMultiByte(CP_ACP, 0,
	   (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFullName), -1,
				      NULL, 0, NULL, NULL);

    if(!lpOTM) {
        ret = needed;
	goto end;
    }

    DPRINT("needed = %d\n", needed);
    if(needed > cbData)
        /* Since the supplied buffer isn't big enough, we'll alloc one
           that is and memcpy the first cbData bytes into the lpOTM at
           the end. */
        output = HeapAlloc(GetProcessHeap(), 0, needed);

    ret = output->otmSize = min(needed, cbData);
    FONT_TextMetricWToA( &lpOTMW->otmTextMetrics, &output->otmTextMetrics );
    output->otmFiller = 0;
    output->otmPanoseNumber = lpOTMW->otmPanoseNumber;
    output->otmfsSelection = lpOTMW->otmfsSelection;
    output->otmfsType = lpOTMW->otmfsType;
    output->otmsCharSlopeRise = lpOTMW->otmsCharSlopeRise;
    output->otmsCharSlopeRun = lpOTMW->otmsCharSlopeRun;
    output->otmItalicAngle = lpOTMW->otmItalicAngle;
    output->otmEMSquare = lpOTMW->otmEMSquare;
    output->otmAscent = lpOTMW->otmAscent;
    output->otmDescent = lpOTMW->otmDescent;
    output->otmLineGap = lpOTMW->otmLineGap;
    output->otmsCapEmHeight = lpOTMW->otmsCapEmHeight;
    output->otmsXHeight = lpOTMW->otmsXHeight;
    output->otmrcFontBox = lpOTMW->otmrcFontBox;
    output->otmMacAscent = lpOTMW->otmMacAscent;
    output->otmMacDescent = lpOTMW->otmMacDescent;
    output->otmMacLineGap = lpOTMW->otmMacLineGap;
    output->otmusMinimumPPEM = lpOTMW->otmusMinimumPPEM;
    output->otmptSubscriptSize = lpOTMW->otmptSubscriptSize;
    output->otmptSubscriptOffset = lpOTMW->otmptSubscriptOffset;
    output->otmptSuperscriptSize = lpOTMW->otmptSuperscriptSize;
    output->otmptSuperscriptOffset = lpOTMW->otmptSuperscriptOffset;
    output->otmsStrikeoutSize = lpOTMW->otmsStrikeoutSize;
    output->otmsStrikeoutPosition = lpOTMW->otmsStrikeoutPosition;
    output->otmsUnderscoreSize = lpOTMW->otmsUnderscoreSize;
    output->otmsUnderscorePosition = lpOTMW->otmsUnderscorePosition;


    ptr = (char*)(output + 1);
    left = needed - sizeof(*output);

    if(lpOTMW->otmpFamilyName) {
        output->otmpFamilyName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFamilyName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFamilyName = 0;

    if(lpOTMW->otmpFaceName) {
        output->otmpFaceName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFaceName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpFaceName = 0;

    if(lpOTMW->otmpStyleName) {
        output->otmpStyleName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpStyleName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
	ptr += len;
    } else
        output->otmpStyleName = 0;

    if(lpOTMW->otmpFullName) {
        output->otmpFullName = (LPSTR)(ptr - (char*)output);
	len = WideCharToMultiByte(CP_ACP, 0,
	     (WCHAR*)((char*)lpOTMW + (int)lpOTMW->otmpFullName), -1,
				  ptr, left, NULL, NULL);
	left -= len;
    } else
        output->otmpFullName = 0;

    assert(left == 0);

    if(output != lpOTM) {
        memcpy(lpOTM, output, cbData);
        HeapFree(GetProcessHeap(), 0, output);

        /* check if the string offsets really fit into the provided size */
        /* FIXME: should we check string length as well? */
        if ((UINT_PTR)lpOTM->otmpFamilyName >= lpOTM->otmSize)
            lpOTM->otmpFamilyName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpFaceName >= lpOTM->otmSize)
            lpOTM->otmpFaceName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpStyleName >= lpOTM->otmSize)
            lpOTM->otmpStyleName = 0; /* doesn't fit */

        if ((UINT_PTR)lpOTM->otmpFullName >= lpOTM->otmSize)
            lpOTM->otmpFullName = 0; /* doesn't fit */
    }

end:
    if(lpOTMW != (OUTLINETEXTMETRICW *)buf)
        HeapFree(GetProcessHeap(), 0, lpOTMW);

    return ret;
}


/*
 * @implemented
 */
UINT
APIENTRY
GetOutlineTextMetricsW(
	HDC			hdc,
	UINT			cbData,
	LPOUTLINETEXTMETRICW	lpOTM
	)
{
  TMDIFF Tmd;   // Should not be zero.

  return NtGdiGetOutlineTextMetricsInternalW(hdc, cbData, lpOTM, &Tmd);
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontIndirectExA(const ENUMLOGFONTEXDVA *elfexd)
{
  if (elfexd)
  {
    ENUMLOGFONTEXDVW Logfont;

    EnumLogFontExW2A( (LPENUMLOGFONTEXA) elfexd,
                                      &Logfont.elfEnumLogfontEx );

    RtlCopyMemory( &Logfont.elfDesignVector,
                           (PVOID) &elfexd->elfDesignVector,
                                             sizeof(DESIGNVECTOR));

    return NtGdiHfontCreate( &Logfont, 0, 0, 0, NULL);
  }
  else return NULL;
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontIndirectExW(const ENUMLOGFONTEXDVW *elfexd)
{
  /* Msdn: Note, this function ignores the elfDesignVector member in
           ENUMLOGFONTEXDV.
   */
  if ( elfexd )
  {
   return NtGdiHfontCreate((PENUMLOGFONTEXDVW) elfexd, 0, 0, 0, NULL );
  }
  else return NULL;
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontIndirectA(
	CONST LOGFONTA		*lplf
	)
{
  if (lplf)
    {
      LOGFONTW tlf;

      LogFontA2W(&tlf, lplf);
      return CreateFontIndirectW(&tlf);
    }
  else return NULL;
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontIndirectW(
	CONST LOGFONTW		*lplf
	)
{
 if (lplf)
 {
    ENUMLOGFONTEXDVW Logfont;

    RtlCopyMemory( &Logfont.elfEnumLogfontEx.elfLogFont, lplf, sizeof(LOGFONTW));
    // Need something other than just cleaning memory here.
    // Guess? Use caller data to determine the rest.
    RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfFullName,
                                 sizeof(Logfont.elfEnumLogfontEx.elfFullName));
    RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfStyle,
                                 sizeof(Logfont.elfEnumLogfontEx.elfStyle));
    RtlZeroMemory( &Logfont.elfEnumLogfontEx.elfScript,
                                 sizeof(Logfont.elfEnumLogfontEx.elfScript));

    RtlZeroMemory( &Logfont.elfDesignVector, sizeof(DESIGNVECTOR));

    return CreateFontIndirectExW(&Logfont);
 }
 else return NULL;
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontA(
	int	nHeight,
	int	nWidth,
	int	nEscapement,
	int	nOrientation,
	int	fnWeight,
	DWORD	fdwItalic,
	DWORD	fdwUnderline,
	DWORD	fdwStrikeOut,
	DWORD	fdwCharSet,
	DWORD	fdwOutputPrecision,
	DWORD	fdwClipPrecision,
	DWORD	fdwQuality,
	DWORD	fdwPitchAndFamily,
	LPCSTR	lpszFace
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	HFONT ret;

	RtlInitAnsiString(&StringA, (LPSTR)lpszFace);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

        ret = CreateFontW(nHeight,
                           nWidth,
                      nEscapement,
                     nOrientation,
                         fnWeight,
                        fdwItalic,
                     fdwUnderline,
                     fdwStrikeOut,
                       fdwCharSet,
               fdwOutputPrecision,
                 fdwClipPrecision,
                       fdwQuality,
                fdwPitchAndFamily,
                   StringU.Buffer);

	RtlFreeUnicodeString(&StringU);

	return ret;
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontW(
	int	nHeight,
	int	nWidth,
	int	nEscapement,
	int	nOrientation,
	int	nWeight,
	DWORD	fnItalic,
	DWORD	fdwUnderline,
	DWORD	fdwStrikeOut,
	DWORD	fdwCharSet,
	DWORD	fdwOutputPrecision,
	DWORD	fdwClipPrecision,
	DWORD	fdwQuality,
	DWORD	fdwPitchAndFamily,
	LPCWSTR	lpszFace
	)
{
  LOGFONTW logfont;

  logfont.lfHeight = nHeight;
  logfont.lfWidth = nWidth;
  logfont.lfEscapement = nEscapement;
  logfont.lfOrientation = nOrientation;
  logfont.lfWeight = nWeight;
  logfont.lfItalic = fnItalic;
  logfont.lfUnderline = fdwUnderline;
  logfont.lfStrikeOut = fdwStrikeOut;
  logfont.lfCharSet = fdwCharSet;
  logfont.lfOutPrecision = fdwOutputPrecision;
  logfont.lfClipPrecision = fdwClipPrecision;
  logfont.lfQuality = fdwQuality;
  logfont.lfPitchAndFamily = fdwPitchAndFamily;

  if (NULL != lpszFace)
  {
    int Size = sizeof(logfont.lfFaceName) / sizeof(WCHAR);
    wcsncpy((wchar_t *)logfont.lfFaceName, lpszFace, Size - 1);
    /* Be 101% sure to have '\0' at end of string */
    logfont.lfFaceName[Size - 1] = '\0';
  }
  else
  {
    logfont.lfFaceName[0] = L'\0';
  }

  return CreateFontIndirectW(&logfont);
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateScalableFontResourceW(
	DWORD		fdwHidden,
	LPCWSTR		lpszFontRes,
	LPCWSTR		lpszFontFile,
	LPCWSTR		lpszCurrentPath
	)
{
  return NtGdiCreateScalableFontResource ( fdwHidden,
					  lpszFontRes,
					  lpszFontFile,
					  lpszCurrentPath );
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateScalableFontResourceA(
	DWORD		fdwHidden,
	LPCSTR		lpszFontRes,
	LPCSTR		lpszFontFile,
	LPCSTR		lpszCurrentPath
	)
{
  NTSTATUS Status;
  LPWSTR lpszFontResW, lpszFontFileW, lpszCurrentPathW;
  BOOL rc = FALSE;

  Status = HEAP_strdupA2W ( &lpszFontResW, lpszFontRes );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      Status = HEAP_strdupA2W ( &lpszFontFileW, lpszFontFile );
      if (!NT_SUCCESS (Status))
	SetLastError (RtlNtStatusToDosError(Status));
      else
	{
	  Status = HEAP_strdupA2W ( &lpszCurrentPathW, lpszCurrentPath );
	  if (!NT_SUCCESS (Status))
	    SetLastError (RtlNtStatusToDosError(Status));
	  else
	    {
	      rc = NtGdiCreateScalableFontResource ( fdwHidden,
						    lpszFontResW,
						    lpszFontFileW,
						    lpszCurrentPathW );

	      HEAP_free ( lpszCurrentPathW );
	    }

	  HEAP_free ( lpszFontFileW );
	}

      HEAP_free ( lpszFontResW );
    }
  return rc;
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceExW ( LPCWSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
    if (fl & ~(FR_PRIVATE | FR_NOT_ENUM))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    return GdiAddFontResourceW(lpszFilename, fl,0);
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceExA ( LPCSTR lpszFilename, DWORD fl, PVOID pvReserved )
{
    NTSTATUS Status;
    PWSTR FilenameW;
    int rc;

    if (fl & ~(FR_PRIVATE | FR_NOT_ENUM))
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }

    Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
    if ( !NT_SUCCESS (Status) )
    {
        SetLastError (RtlNtStatusToDosError(Status));
        return 0;
    }

    rc = GdiAddFontResourceW ( FilenameW, fl, 0 );
    HEAP_free ( FilenameW );
    return rc;
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceA ( LPCSTR lpszFilename )
{
    NTSTATUS Status;
    PWSTR FilenameW;
    int rc = 0;

    Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
    if ( !NT_SUCCESS (Status) )
    {
        SetLastError (RtlNtStatusToDosError(Status));
    }
    else
    {
      rc = GdiAddFontResourceW ( FilenameW, 0, 0);

      HEAP_free ( FilenameW );
    }
    return rc;
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceW ( LPCWSTR lpszFilename )
{
    return GdiAddFontResourceW ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceW(LPCWSTR lpFileName)
{
    return RemoveFontResourceExW(lpFileName,0,0);
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceA(LPCSTR lpFileName)
{
    return RemoveFontResourceExA(lpFileName,0,0);
}

/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceExA(LPCSTR lpFileName,
                      DWORD fl,
                      PVOID pdv
)
{
    NTSTATUS Status;
    LPWSTR lpFileNameW;
    BOOL rc = 0;

    /* FIXME the flags */
    /* FIXME the pdv */
    /* FIXME NtGdiRemoveFontResource handle flags and pdv */

    Status = HEAP_strdupA2W ( &lpFileNameW, lpFileName );
    if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
    else
    {
        rc = NtGdiRemoveFontResource ( lpFileNameW );

        HEAP_free ( lpFileNameW );
    }

  return rc;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RemoveFontResourceExW(LPCWSTR lpFileName,
                      DWORD fl,
                      PVOID pdv)
{
    /* FIXME the flags */
    /* FIXME the pdv */
    /* FIXME NtGdiRemoveFontResource handle flags and pdv */
    return NtGdiRemoveFontResource ( lpFileName);
}


/***********************************************************************
 *           GdiGetCharDimensions
 *
 * Gets the average width of the characters in the English alphabet.
 *
 * PARAMS
 *  hdc    [I] Handle to the device context to measure on.
 *  lptm   [O] Pointer to memory to store the text metrics into.
 *  height [O] On exit, the maximum height of characters in the English alphabet.
 *
 * RETURNS
 *  The average width of characters in the English alphabet.
 *
 * NOTES
 *  This function is used by the dialog manager to get the size of a dialog
 *  unit. It should also be used by other pieces of code that need to know
 *  the size of a dialog unit in logical units without having access to the
 *  window handle of the dialog.
 *  Windows caches the font metrics from this function, but we don't and
 *  there doesn't appear to be an immediate advantage to do so.
 *
 * SEE ALSO
 *  GetTextExtentPointW, GetTextMetricsW, MapDialogRect.
 *
 * Despite most of MSDN insisting that the horizontal base unit is
 * tmAveCharWidth it isn't.  Knowledge base article Q145994
 * "HOWTO: Calculate Dialog Units When Not Using the System Font",
 * says that we should take the average of the 52 English upper and lower
 * case characters.
 */
/*
 * @implemented
 */
DWORD
STDCALL
GdiGetCharDimensions(HDC hdc, LPTEXTMETRICW lptm, LONG *height)
{
    SIZE sz;
    static const WCHAR alphabet[] = {
        'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q',
        'r','s','t','u','v','w','x','y','z','A','B','C','D','E','F','G','H',
        'I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z',0};

    if(lptm && !GetTextMetricsW(hdc, lptm)) return 0;

    if(!GetTextExtentPointW(hdc, alphabet, 52, &sz)) return 0;

    if (height) *height = sz.cy;
    return (sz.cx / 26 + 1) / 2;
}

/*************************************************************************
 * TranslateCharsetInfo [GDI32.@]
 *
 * Fills a CHARSETINFO structure for a character set, code page, or
 * font. This allows making the correspondance between different labelings
 * (character set, Windows, ANSI, and OEM codepages, and Unicode ranges)
 * of the same encoding.
 *
 * Only one codepage will be set in lpCs->fs. If TCI_SRCFONTSIG is used,
 * only one codepage should be set in *lpSrc.
 *
 * RETURNS
 *   TRUE on success, FALSE on failure.
 *
 */
/*
 * @unimplemented
 */
BOOL
STDCALL
TranslateCharsetInfo(
  LPDWORD lpSrc, /* [in]
       if flags == TCI_SRCFONTSIG: pointer to fsCsb of a FONTSIGNATURE
       if flags == TCI_SRCCHARSET: a character set value
       if flags == TCI_SRCCODEPAGE: a code page value
		 */
  LPCHARSETINFO lpCs, /* [out] structure to receive charset information */
  DWORD flags /* [in] determines interpretation of lpSrc */)
{
    int index = 0;
    switch (flags) {
    case TCI_SRCFONTSIG:
	while (!(*lpSrc>>index & 0x0001) && index<MAXTCIINDEX) index++;
      break;
    case TCI_SRCCODEPAGE:
      while (PtrToUlong(lpSrc) != FONT_tci[index].ciACP && index < MAXTCIINDEX) index++;
      break;
    case TCI_SRCCHARSET:
      while (PtrToUlong(lpSrc) != FONT_tci[index].ciCharset && index < MAXTCIINDEX) index++;
      break;
    default:
      return FALSE;
    }
    if (index >= MAXTCIINDEX || FONT_tci[index].ciCharset == DEFAULT_CHARSET) return FALSE;
    memcpy(lpCs, &FONT_tci[index], sizeof(CHARSETINFO));
    return TRUE;
}


/*
 * @unimplemented
 */
int
STDCALL
EnumFontsW(
	HDC  hDC,
	LPCWSTR lpFaceName,
	FONTENUMPROCW  FontFunc,
	LPARAM  lParam
	)
{
#if 0
  return NtGdiEnumFonts ( hDC, lpFaceName, FontFunc, lParam );
#else
  return EnumFontFamiliesW( hDC, lpFaceName, FontFunc, lParam );
#endif
}

/*
 * @unimplemented
 */
int
STDCALL
EnumFontsA (
	HDC  hDC,
	LPCSTR lpFaceName,
	FONTENUMPROCA  FontFunc,
	LPARAM  lParam
	)
{
#if 0
  NTSTATUS Status;
  LPWSTR lpFaceNameW;
  int rc = 0;

  Status = HEAP_strdupA2W ( &lpFaceNameW, lpFaceName );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = NtGdiEnumFonts ( hDC, lpFaceNameW, FontFunc, lParam );

      HEAP_free ( lpFaceNameW );
    }
  return rc;
#else
  return EnumFontFamiliesA( hDC, lpFaceName, FontFunc, lParam );
#endif
}

#define EfdFontFamilies 3

INT
STDCALL
NewEnumFontFamiliesExW(
    HDC hDC,
    LPLOGFONTW lpLogfont,
    FONTENUMPROCW lpEnumFontFamExProcW,
    LPARAM lParam,
    DWORD dwFlags)
{
	ULONG_PTR idEnum, cbDataSize, cbRetSize;
	PENUMFONTDATAW pEfdw;
	PBYTE pBuffer;
	PBYTE pMax;
	INT ret = 1;

	/* Open enumeration handle and find out how much memory we need */
	idEnum = NtGdiEnumFontOpen(hDC,
	                           EfdFontFamilies,
	                           0,
	                           LF_FACESIZE,
	                           (lpLogfont && lpLogfont->lfFaceName[0])? lpLogfont->lfFaceName : NULL,
	                           lpLogfont? lpLogfont->lfCharSet : DEFAULT_CHARSET,
	                           &cbDataSize);
	if (idEnum == 0)
	{
		return 0;
	}
	if (cbDataSize == 0)
	{
		NtGdiEnumFontClose(idEnum);
		return 0;
	}

	/* Allocate memory */
	pBuffer = HeapAlloc(GetProcessHeap(), 0, cbDataSize);
	if (pBuffer == NULL)
	{
		SetLastError(ERROR_NOT_ENOUGH_MEMORY);
		NtGdiEnumFontClose(idEnum);
		return 0;
	}

	/* Do the enumeration */
	if (!NtGdiEnumFontChunk(hDC, idEnum, cbDataSize, &cbRetSize, (PVOID)pBuffer))
	{
		HeapFree(GetProcessHeap(), 0, pBuffer);
		NtGdiEnumFontClose(idEnum);
		return 0;
	}

    /* Get start and end address */
    pEfdw = (PENUMFONTDATAW)pBuffer;
	pMax = pBuffer + cbDataSize;

    /* Iterate through the structures */
    while ((PBYTE)pEfdw < pMax && ret)
    {
        PNTMW_INTERNAL pNtmwi = (PNTMW_INTERNAL)((ULONG_PTR)pEfdw + pEfdw->ulNtmwiOffset);

        ret = lpEnumFontFamExProcW(&pEfdw->elfexdv.elfEnumLogfontEx,
                                   &pNtmwi->ntmw,
                                   pEfdw->dwFontType,
                                   lParam);

        pEfdw = (PENUMFONTDATAW)((ULONG_PTR)pEfdw + pEfdw->cbSize);
	}

	/* Release the memory and close handle */
	HeapFree(GetProcessHeap(), 0, pBuffer);
	NtGdiEnumFontClose(idEnum);

	return ret;
}
