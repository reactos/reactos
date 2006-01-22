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

#define INITIAL_FAMILY_COUNT 64

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
            (LPLOGFONTW)&Info[i].EnumLogFontEx,
            (LPTEXTMETRICW)&Info[i].NewTextMetricEx,
            Info[i].FontType, lParam);
        }
      else
        {
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
            (LPLOGFONTA)&EnumLogFontExA,
            (LPTEXTMETRICA)&NewTextMetricExA,
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
BOOL
STDCALL
GetCharWidthA (
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  return GetCharWidth32A ( hdc, iFirstChar, iLastChar, lpBuffer );
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
   /* FIXME should be NtGdiGetCharWidthW */
   return NtGdiGetCharWidth32(hdc, iFirstChar, iLastChar, lpBuffer);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidthW (
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  /* FIXME should be NtGdiGetCharWidthW */
  return NtGdiGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
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
      if (NtGdiGetCharWidth32(hdc, lpString[i], lpString[i], &c))
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
    GetGlyphIndicesW(hdc, lpString, nSet, lpResults->lpGlyphs, 0);*/

  if (GetTextExtentPoint32W(hdc, lpString, uCount, &size))
    ret = MAKELONG(size.cx, size.cy);

  return ret;
}


/*
 * @unimplemented
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
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return NtGdiGetCharWidthFloat ( hdc, iFirstChar, iLastChar, pxBuffer );
}


/*
 * @unimplemented
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
  /* FIXME what to do with uFirstChar and uLastChar ??? */
  return NtGdiGetCharABCWidths ( hdc, uFirstChar, uLastChar, lpabc );
}


/*
 * @unimplemented
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
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return NtGdiGetCharABCWidthsFloat ( hdc, iFirstChar, iLastChar, lpABCF );
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
  return NtGdiGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
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
  return NtGdiGetGlyphOutline ( hdc, uChar, uFormat, lpgm, cbBuffer, lpvBuffer, (CONST LPMAT2)lpmat2, TRUE);
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
  LOGFONTW tlf;

  LogFontA2W(&tlf, lplf);

  return NtGdiCreateFontIndirect(&tlf);
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
	return NtGdiCreateFontIndirect((CONST LPLOGFONTW)lplf);
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

        ret = CreateFontW(nHeight, nWidth, nEscapement, nOrientation, fnWeight, fdwItalic, fdwUnderline, fdwStrikeOut,
                          fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, StringU.Buffer);

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
  return NtGdiCreateFont(nHeight, nWidth, nEscapement, nOrientation, nWeight, fnItalic, fdwUnderline, fdwStrikeOut,
                         fdwCharSet, fdwOutputPrecision, fdwClipPrecision, fdwQuality, fdwPitchAndFamily, lpszFace);
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
  UNICODE_STRING Filename;

  /* FIXME handle fl parameter */
  RtlInitUnicodeString(&Filename, lpszFilename);
  return NtGdiAddFontResource ( &Filename, fl );
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
  int rc = 0;

  Status = HEAP_strdupA2W ( &FilenameW, lpszFilename );
  if ( !NT_SUCCESS (Status) )
    SetLastError (RtlNtStatusToDosError(Status));
  else
    {
      rc = AddFontResourceExW ( FilenameW, fl, pvReserved );

      HEAP_free ( FilenameW );
    }
  return rc;
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceA ( LPCSTR lpszFilename )
{
  return AddFontResourceExA ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
int
STDCALL
AddFontResourceW ( LPCWSTR lpszFilename )
{
	return AddFontResourceExW ( lpszFilename, 0, 0 );
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceW(
	LPCWSTR	lpFileName
	)
{
  return NtGdiRemoveFontResource ( lpFileName );
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveFontResourceA(
	LPCSTR	lpFileName
	)
{
  NTSTATUS Status;
  LPWSTR lpFileNameW;
  BOOL rc = 0;

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




