#include "precomp.h"



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
int
STDCALL
GetTextCharset(
	HDC	hdc
	)
{
    /* MSDN docs say this is equivalent */
        return GetTextCharsetInfo(hdc, NULL, 0);        
}


/*
 * @implemented
 */
int
STDCALL
GetTextCharsetInfo(
	HDC		hdc,
	LPFONTSIGNATURE	lpSig,
	DWORD		dwFlags
	)
{
        return NtGdiGetTextCharsetInfo(hdc, lpSig, dwFlags);
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
  return NtGdiGetTextExtent(hdc, (LPWSTR)lpString, cbString, lpSize, 0);
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentExPointW(
	HDC		hdc,
	LPCWSTR		lpszStr,
	int		cchString,
	int		nMaxExtent,
	LPINT		lpnFit,
	LPINT		alpDx,
	LPSIZE		lpSize
	)
{
  return NtGdiGetTextExtentExPoint (
    hdc, lpszStr, cchString, nMaxExtent, lpnFit, alpDx, lpSize );
}


/*
 * @implemented
 */
BOOL
APIENTRY
GetTextExtentExPointA(
	HDC		hdc,
	LPCSTR		lpszStr,
	int		cchString,
	int		nMaxExtent,
	LPINT		lpnFit,
	LPINT		alpDx,
	LPSIZE		lpSize
	)
{
  NTSTATUS Status;
  LPWSTR lpszStrW;
  BOOL rc = 0;

  Status = HEAP_strdupA2W ( &lpszStrW, lpszStr );
  if (!NT_SUCCESS (Status))
    SetLastError (RtlNtStatusToDosError(Status));
  else
  {
    rc = NtGdiGetTextExtentExPoint (
      hdc, lpszStrW, cchString, nMaxExtent, lpnFit, alpDx, lpSize );

    HEAP_free ( lpszStrW );
  }

  return rc;
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
int
STDCALL
GetTextFaceW(
	HDC	a0,
	int	a1,
	LPWSTR	a2
	)
{
	return NtGdiGetTextFace(a0, a1, a2);
}


/*
 * @implemented
 */
int
STDCALL
GetTextFaceA( HDC hdc, INT count, LPSTR name )
{
    INT res = GetTextFaceW(hdc, 0, NULL);
    LPWSTR nameW = HeapAlloc( GetProcessHeap(), 0, res * 2 );
    GetTextFaceW( hdc, res, nameW );

    if (name)
    {
        if (count && !WideCharToMultiByte( CP_ACP, 0, nameW, -1, name, count, NULL, NULL))
            name[count-1] = 0;
        res = strlen(name);
    }
    else
        res = WideCharToMultiByte( CP_ACP, 0, nameW, -1, NULL, 0, NULL, NULL);
    HeapFree( GetProcessHeap(), 0, nameW );
    return res;
}


