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

BOOL 
STDCALL 
GetTextMetricsA(
	HDC		hdc, 
	LPTEXTMETRICA	tm
	)
{
	return W32kGetTextMetrics(hdc, (LPTEXTMETRICW) tm);
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

