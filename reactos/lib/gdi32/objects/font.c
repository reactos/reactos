/* $Id: font.c,v 1.3 2004/04/09 20:03:13 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/gdi32/object/font.c
 * PURPOSE:         
 * PROGRAMMER:
 *
 */

#ifdef UNICODE
#undef UNICODE
#endif

#include <windows.h>
#include <rosrtl/logfont.h>
#include <ddk/ntddk.h>
#define NTOS_MODE_USER
#include <ntos.h>
#include <win32k/font.h>
#include <win32k/text.h>
#include <internal/font.h>

#define NDEBUG
#include <debug.h>

#define INITIAL_FAMILY_COUNT 64

static BOOL FASTCALL
MetricsCharConvert(WCHAR w, CHAR *b)
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

/*
 * @implemented
 */
BOOL
STDCALL
TranslateCharsetInfo(DWORD *Src, LPCHARSETINFO Cs, DWORD Flags)
{
  return NtGdiTranslateCharsetInfo(Src, Cs, Flags);
}

static int FASTCALL
IntEnumFontFamilies(HDC Dc, LPLOGFONTW LogFont, PVOID EnumProc, LPARAM lParam,
                    BOOL Unicode)
{
  int FontFamilyCount;
  unsigned FontFamilySize;
  PFONTFAMILYINFO Info;
  int Ret;
  unsigned i;
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
          RosRtlLogFontW2A(&EnumLogFontExA.elfLogFont, &Info[i].EnumLogFontEx.elfLogFont);
          WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfFullName, -1,
                              EnumLogFontExA.elfFullName, LF_FULLFACESIZE, NULL, NULL);
          WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfStyle, -1,
                              EnumLogFontExA.elfStyle, LF_FACESIZE, NULL, NULL);
          WideCharToMultiByte(CP_THREAD_ACP, 0, Info[i].EnumLogFontEx.elfScript, -1,
                              EnumLogFontExA.elfScript, LF_FACESIZE, NULL, NULL);
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
EnumFontFamiliesExW(HDC Dc, LPLOGFONTW LogFont, FONTENUMPROCW EnumFontFamProc,
                    LPARAM lParam, DWORD Flags)
{
  return IntEnumFontFamilies(Dc, LogFont, EnumFontFamProc, lParam, TRUE);
}


/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesW(HDC Dc, LPCWSTR Family, FONTENUMPROCW EnumFontFamProc,
                  LPARAM lParam)
{
  LOGFONTW LogFont;

  ZeroMemory(&LogFont, sizeof(LOGFONTW));
  LogFont.lfCharSet = DEFAULT_CHARSET;
  if (NULL != Family)
    {
      lstrcpynW(LogFont.lfFaceName, Family, LF_FACESIZE);
    }

  return IntEnumFontFamilies(Dc, &LogFont, EnumFontFamProc, lParam, TRUE);
}


/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesExA (HDC Dc, LPLOGFONTA LogFont, FONTENUMPROCA EnumFontFamProc,
                     LPARAM lParam, DWORD dwFlags)
{
  LOGFONTW LogFontW;

  RosRtlLogFontA2W(&LogFontW, LogFont);

  /* no need to convert LogFontW back to lpLogFont b/c it's an [in] parameter only */
  return IntEnumFontFamilies(Dc, &LogFontW, EnumFontFamProc, lParam, FALSE);
}


/*
 * @implemented
 */
int STDCALL
EnumFontFamiliesA(HDC Dc, LPCSTR Family, FONTENUMPROCA EnumFontFamProc,
                  LPARAM lParam)
{
  LOGFONTW LogFont;

  ZeroMemory(&LogFont, sizeof(LOGFONTW));
  LogFont.lfCharSet = DEFAULT_CHARSET;
  if (NULL != Family)
    {
      MultiByteToWideChar(CP_THREAD_ACP, 0, Family, -1, LogFont.lfFaceName, LF_FACESIZE);
    }

  return IntEnumFontFamilies(Dc, &LogFont, EnumFontFamProc, lParam, FALSE);
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
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return NtGdiGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
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
  /* FIXME what to do with iFirstChar and iLastChar ??? */
  return NtGdiGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
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
  return NtGdiGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
}


/*
 * @implemented
 */
BOOL
STDCALL
GetCharWidth32W(
	HDC	hdc,
	UINT	iFirstChar,
	UINT	iLastChar,
	LPINT	lpBuffer
	)
{
  return NtGdiGetCharWidth32 ( hdc, iFirstChar, iLastChar, lpBuffer );
}
