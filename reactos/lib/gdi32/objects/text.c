#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <string.h>
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>
#include <internal/font.h>
#include <rosrtl/logfont.h>


/*
 * @implemented
 */
UINT
STDCALL
SetTextAlign(
        HDC     a0,
        UINT    a1
        )
{
        return NtGdiSetTextAlign(a0, a1);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
STDCALL
TextOutW(
	HDC  hDC,
	int  XStart,
	int  YStart,
	LPCWSTR  String,
	int  Count)
{
	return NtGdiTextOut(hDC, XStart, YStart, String, Count);
}


/*
 * @implemented
 */
COLORREF  STDCALL 
SetTextColor(HDC hDC, COLORREF color)
{
  return(NtGdiSetTextColor(hDC, color));
}


/*
 * @implemented
 */
BOOL 
STDCALL 
GetTextMetricsA(
	HDC		hdc, 
	LPTEXTMETRICA	tm
	)
{
  TEXTMETRICW tmw;

  if (! NtGdiGetTextMetrics(hdc, &tmw))
    {
      return FALSE;
    }

  return TextMetricW2A(tm, &tmw);
}


/*
 * @implemented
 */
BOOL 
STDCALL 
GetTextMetricsW(
	HDC		hdc, 
	LPTEXTMETRICW	tm
	)
{
  return NtGdiGetTextMetrics(hdc, tm);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPointW(
	HDC		hDC,
	LPCWSTR		String,
	int		Count,
	LPSIZE		Size
	)
{
	return NtGdiGetTextExtentPoint(hDC, String, Count, Size);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPoint32W(
	HDC		hDC,
	LPCWSTR		String,
	int		Count,
	LPSIZE		Size
	)
{
  return NtGdiGetTextExtentPoint32(hDC, String, Count, Size);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
BOOL  
STDCALL 
ExtTextOutW(
	HDC		hdc, 
	int		X, 
	int		Y, 
	UINT		fuOptions,	 
	CONST RECT	*lpRect,
	LPCWSTR		lpString, 
	UINT		cbCount, 
	CONST INT	*lpDx
	)
{
  return NtGdiExtTextOut(hdc, X, Y, fuOptions, lpRect, lpString, cbCount, lpDx);
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontIndirectA(
	CONST LOGFONTA		*lf
	)
{
  LOGFONTW tlf;

  RosRtlLogFontA2W(&tlf, lf);

  return NtGdiCreateFontIndirect(&tlf);
}


/*
 * @implemented
 */
HFONT
STDCALL
CreateFontIndirectW(
	CONST LOGFONTW		*lf
	)
{
	return NtGdiCreateFontIndirect((CONST LPLOGFONTW)lf);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
	return NtGdiCreateFont(Height, Width, Escapement, Orientation, Weight, Italic, Underline, StrikeOut,
                          CharSet, OutputPrecision, ClipPrecision, Quality, PitchAndFamily, Face);
}

