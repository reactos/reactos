#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <string.h>
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>

BOOL  
STDCALL 
TextOutA(
	HDC  hDC,
	int  XStart,
	int  YStart,
	LPCSTR  String,
	int  Count)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	if (NULL != String)
	{
		RtlInitAnsiString(&StringA, (LPSTR)String);
		RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
	} else
		StringU.Buffer = NULL;

	ret = TextOutW(hDC, XStart, YStart, StringU.Buffer, Count);
	RtlFreeUnicodeString(&StringU);
	return ret;
}

BOOL
STDCALL
TextOutW(
	HDC  hDC,
	int  XStart,
	int  YStart,
	LPCWSTR  String,
	int  Count)
{
	return W32kTextOut(hDC, XStart, YStart, String, Count);
}

COLORREF  STDCALL 
SetTextColor(HDC hDC, COLORREF color)
{
  return(W32kSetTextColor(hDC, color));
}

static BOOL
MetricsCharConvert(WCHAR w, BCHAR *b)
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

BOOL 
STDCALL 
GetTextMetricsA(
	HDC		hdc, 
	LPTEXTMETRICA	tm
	)
{
  TEXTMETRICW tmw;
  UNICODE_STRING WString;
  WCHAR WBuf[256];
  ANSI_STRING AString;
  CHAR ABuf[256];
  UINT Letter;
  NTSTATUS Status;

  if (! W32kGetTextMetrics(hdc, &tmw))
    {
      return FALSE;
    }

  tm->tmHeight = tmw.tmHeight;
  tm->tmAscent = tmw.tmAscent;
  tm->tmDescent = tmw.tmDescent;
  tm->tmInternalLeading = tmw.tmInternalLeading;
  tm->tmExternalLeading = tmw.tmExternalLeading;
  tm->tmAveCharWidth = tmw.tmAveCharWidth;
  tm->tmMaxCharWidth = tmw.tmMaxCharWidth;
  tm->tmWeight = tmw.tmWeight;
  tm->tmOverhang = tmw.tmOverhang;
  tm->tmDigitizedAspectX = tmw.tmDigitizedAspectX;
  tm->tmDigitizedAspectY = tmw.tmDigitizedAspectY;

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
  tm->tmFirstChar = '\0';
  if (L'\0' != tmw.tmFirstChar)
    {
      for (Letter = 1; Letter < 256; Letter++)
	{
	  if (tmw.tmFirstChar <= WBuf[Letter - 1] &&
	      WBuf[Letter - 1] <= tmw.tmLastChar)
	    {
	      tm->tmFirstChar = (CHAR) Letter;
	      break;
	    }
	}
    }

  /* Scan for the last ANSI character which maps to an Unicode character
     in the range */
  tm->tmLastChar = '\0';
  if (L'\0' != tmw.tmLastChar)
    {
      for (Letter = 255; 0 < Letter; Letter--)
	{
	  if (tmw.tmFirstChar <= WBuf[Letter - 1] &&
	      WBuf[Letter - 1] <= tmw.tmLastChar)
	    {
	      tm->tmLastChar = (CHAR) Letter;
	      break;
	    }
	}
    }

  if (! MetricsCharConvert(tmw.tmDefaultChar, &tm->tmDefaultChar) ||
      ! MetricsCharConvert(tmw.tmBreakChar, &tm->tmBreakChar))
    {
      return FALSE;
    }

  tm->tmItalic = tmw.tmItalic;
  tm->tmUnderlined = tmw.tmUnderlined;
  tm->tmStruckOut = tmw.tmStruckOut;
  tm->tmPitchAndFamily = tmw.tmPitchAndFamily;
  tm->tmCharSet = tmw.tmCharSet;

  return TRUE;
}

BOOL 
STDCALL 
GetTextMetricsW(
	HDC		hdc, 
	LPTEXTMETRICW	tm
	)
{
	return W32kGetTextMetrics(hdc, tm);
}

BOOL
APIENTRY
GetTextExtentPointA(
	HDC		hDC,
	LPCSTR		String,
	int		Count,
	LPSIZE		Size
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	RtlInitAnsiString(&StringA, (LPSTR)String);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

        ret = GetTextExtentPointW(hDC, StringU.Buffer, Count, Size);

	RtlFreeUnicodeString(&StringU);

	return ret;
}

BOOL
APIENTRY
GetTextExtentPointW(
	HDC		hDC,
	LPCWSTR		String,
	int		Count,
	LPSIZE		Size
	)
{
	return W32kGetTextExtentPoint(hDC, String, Count, Size);
}


BOOL
APIENTRY
GetTextExtentPoint32A(
	HDC		hDC,
	LPCSTR		String,
	int		Count,
	LPSIZE		Size
	)
{
  ANSI_STRING StringA;
  UNICODE_STRING StringU;
  BOOL ret;

  RtlInitAnsiString(&StringA, (LPSTR)String);
  RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

  ret = GetTextExtentPoint32W(hDC, StringU.Buffer, Count, Size);

  RtlFreeUnicodeString(&StringU);

  return ret;
}


BOOL
APIENTRY
GetTextExtentPoint32W(
	HDC		hDC,
	LPCWSTR		String,
	int		Count,
	LPSIZE		Size
	)
{
  return W32kGetTextExtentPoint32(hDC, String, Count, Size);
}

BOOL  
STDCALL 
ExtTextOutA(
	HDC		hDC, 
	int		X, 
	int		Y, 
	UINT		Options, 
	CONST RECT	*Rect,
	LPCSTR		String, 
	UINT		Count, 
	CONST INT	*Spacings
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	RtlInitAnsiString(&StringA, (LPSTR)String);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

        ret = ExtTextOutW(hDC, X, Y, Options, Rect, StringU.Buffer, Count, Spacings);

	RtlFreeUnicodeString(&StringU);

	return ret;
}

BOOL  
STDCALL 
ExtTextOutW(
	HDC		hDC, 
	int		X, 
	int		Y, 
	UINT		Options,	 
	CONST RECT	*Rect,
	LPCWSTR		String, 
	UINT		Count, 
	CONST INT	*Spacings
	)
{
	return W32kTextOut(hDC, X, Y, String, Count);
}

HFONT
STDCALL
CreateFontIndirectA(
	CONST LOGFONTA		*lf
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	HFONT ret;
        LOGFONTW tlf;

	RtlInitAnsiString(&StringA, (LPSTR)lf->lfFaceName);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
        memcpy(&tlf, lf, sizeof(LOGFONTA));
        memcpy(&tlf.lfFaceName, &StringU.Buffer, StringU.Length);

        ret = CreateFontIndirectW(&tlf);

	RtlFreeUnicodeString(&StringU);

	return ret;
}

HFONT
STDCALL
CreateFontIndirectW(
	CONST LOGFONTW		*lf
	)
{
	return W32kCreateFontIndirect((CONST LPLOGFONTW)lf);
}

HFONT
STDCALL
CreateFontA(
	int	Height,
	int	Width,
	int	Escapement,
	int	Orientation,
	int	Weight,
	DWORD	Italic,
	DWORD	Underline,
	DWORD	StrikeOut,
	DWORD	CharSet,
	DWORD	OutputPrecision,
	DWORD	ClipPrecision,
	DWORD	Quality,
	DWORD	PitchAndFamily,
	LPCSTR	Face
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	HFONT ret;

	RtlInitAnsiString(&StringA, (LPSTR)Face);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

        ret = CreateFontW(Height, Width, Escapement, Orientation, Weight, Italic, Underline, StrikeOut,
                          CharSet, OutputPrecision, ClipPrecision, Quality, PitchAndFamily, StringU.Buffer);

	RtlFreeUnicodeString(&StringU);

	return ret;
}

HFONT
STDCALL
CreateFontW(
	int	Height,
	int	Width,
	int	Escapement,
	int	Orientation,
	int	Weight,
	DWORD	Italic,
	DWORD	Underline,
	DWORD	StrikeOut,
	DWORD	CharSet,
	DWORD	OutputPrecision,
	DWORD	ClipPrecision,
	DWORD	Quality,
	DWORD	PitchAndFamily,
	LPCWSTR	Face
	)
{
	return W32kCreateFont(Height, Width, Escapement, Orientation, Weight, Italic, Underline, StrikeOut,
                          CharSet, OutputPrecision, ClipPrecision, Quality, PitchAndFamily, Face);
}

