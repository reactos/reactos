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
        HDC     hdc,
        UINT    fMode
        )
{
  return NtGdiSetTextAlign(hdc, fMode);
}


/*
 * @implemented
 */
BOOL  
STDCALL 
TextOutA(
	HDC  hdc,
	int  nXStart,
	int  nYStart,
	LPCSTR  lpString,
	int  cbString)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	if (NULL != lpString)
	{
		RtlInitAnsiString(&StringA, (LPSTR)lpString);
		RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);
	} else
		StringU.Buffer = NULL;

	ret = TextOutW(hdc, nXStart, nYStart, StringU.Buffer, cbString);
	RtlFreeUnicodeString(&StringU);
	return ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
TextOutW(
	HDC  hdc,
	int  nXStart,
	int  nYStart,
	LPCWSTR  lpString,
	int  cbString)
{
  return NtGdiTextOut(hdc, nXStart, nYStart, lpString, cbString);
}


/*
 * @implemented
 */
COLORREF  STDCALL 
SetTextColor(HDC hdc, COLORREF crColor)
{
  return NtGdiSetTextColor(hdc, crColor);
}


/*
 * @implemented
 */
BOOL 
STDCALL 
GetTextMetricsA(
	HDC		hdc, 
	LPTEXTMETRICA	lptm
	)
{
  TEXTMETRICW tmw;

  if (! NtGdiGetTextMetrics(hdc, &tmw))
    {
      return FALSE;
    }

  return TextMetricW2A(lptm, &tmw);
}


/*
 * @implemented
 */
BOOL 
STDCALL 
GetTextMetricsW(
	HDC		hdc, 
	LPTEXTMETRICW	lptm
	)
{
  return NtGdiGetTextMetrics(hdc, lptm);
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPointA(
	HDC		hdc,
	LPCSTR		lpString,
	int		cbString,
	LPSIZE		lpSize
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	RtlInitAnsiString(&StringA, (LPSTR)lpString);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

        ret = GetTextExtentPointW(hdc, StringU.Buffer, cbString, lpSize);

	RtlFreeUnicodeString(&StringU);

	return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPointW(
	HDC		hdc,
	LPCWSTR		lpString,
	int		cbString,
	LPSIZE		lpSize
	)
{
  return NtGdiGetTextExtentPoint(hdc, lpString, cbString, lpSize);
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPoint32A(
	HDC		hdc,
	LPCSTR		lpString,
	int		cbString,
	LPSIZE		lpSize
	)
{
  ANSI_STRING StringA;
  UNICODE_STRING StringU;
  BOOL ret;

  RtlInitAnsiString(&StringA, (LPSTR)lpString);
  RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

  ret = GetTextExtentPoint32W(hdc, StringU.Buffer, cbString, lpSize);

  RtlFreeUnicodeString(&StringU);

  return ret;
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentPoint32W(
	HDC		hdc,
	LPCWSTR		lpString,
	int		cbString,
	LPSIZE		lpSize
	)
{
  return NtGdiGetTextExtentPoint32(hdc, lpString, cbString, lpSize);
}


/*
 * @implemented
 */
BOOL  
STDCALL 
ExtTextOutA(
	HDC		hdc, 
	int		X, 
	int		Y, 
	UINT		fuOptions, 
	CONST RECT	*lprc,
	LPCSTR		lpString, 
	UINT		cbCount, 
	CONST INT	*lpDx
	)
{
        ANSI_STRING StringA;
        UNICODE_STRING StringU;
	BOOL ret;

	RtlInitAnsiString(&StringA, (LPSTR)lpString);
	RtlAnsiStringToUnicodeString(&StringU, &StringA, TRUE);

        ret = ExtTextOutW(hdc, X, Y, fuOptions, lprc, StringU.Buffer, cbCount, lpDx);

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
	CONST RECT	*lprc,
	LPCWSTR		lpString, 
	UINT		cbCount, 
	CONST INT	*lpDx
	)
{
  return NtGdiExtTextOut(hdc, X, Y, fuOptions, lprc, lpString, cbCount, lpDx);
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

  RosRtlLogFontA2W(&tlf, lplf);

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

