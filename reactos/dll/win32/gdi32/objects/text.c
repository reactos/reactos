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
  return NtGdiExtTextOut(hdc, nXStart, nYStart, 0, NULL, lpString, cbString, NULL);
}


/*
 * @implemented
 */
DWORD
STDCALL
GdiGetCodePage(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return 0;
  if (Dc_Attr->ulDirty_ & DIRTY_CHARSET) return LOWORD(NtGdiGetCharSet(hdc));
  return LOWORD(Dc_Attr->iCS_CP);
}


/*
 * @unimplemented
 */
int
STDCALL
GetTextCharacterExtra(
	HDC	hDc
	)
{
  PDC_ATTR Dc_Attr;

  if (!GdiGetHandleUserData((HGDIOBJ) hDc, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->lTextExtra;
// return GetDCDWord( hDc, GdiGetTextCharExtra, 0);
}


/*
 * @implemented
 */
int
STDCALL
GetTextCharset(HDC hdc)
{
    /* MSDN docs say this is equivalent */
    return NtGdiGetTextCharsetInfo(hdc,NULL,0);
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
  TMW_INTERNAL tmwi;

  if (! NtGdiGetTextMetricsW(hdc, &tmwi, sizeof(TMW_INTERNAL)))
  {
    return FALSE;
  }

  return TextMetricW2A(lptm, &tmwi.TextMetric);
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
  TMW_INTERNAL tmwi;

  if (! NtGdiGetTextMetricsW(hdc, &tmwi, sizeof(TMW_INTERNAL)))
  {
    return FALSE;
  }

  *lptm = tmwi.TextMetric;
  return TRUE;
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
  return NtGdiGetTextExtentExW (
    hdc, (LPWSTR)lpszStr, cchString, nMaxExtent, (PULONG)lpnFit, (PULONG)alpDx, lpSize, 0 );
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
    rc = NtGdiGetTextExtentExW (
      hdc, lpszStrW, cchString, nMaxExtent, (PULONG)lpnFit, (PULONG)alpDx, lpSize, 0 );

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
INT
STDCALL
GetTextFaceW(HDC hDC,
             int nCount,
             LPWSTR	lpFaceName)
{
    INT retValue = 0;
    if ((!lpFaceName) || (nCount))
    {
        retValue = NtGdiGetTextFaceW(hDC,nCount,lpFaceName,0);
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    return retValue;
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

BOOL
STDCALL
GetFontResourceInfoW(
    LPCWSTR lpFileName,
    DWORD *pdwBufSize,
    void* lpBuffer,
    DWORD dwType
    )
{
    BOOL bRet;
    UNICODE_STRING NtFileName;

    if (!lpFileName || !pdwBufSize || !lpBuffer)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!RtlDosPathNameToNtPathName_U(lpFileName,
                                      &NtFileName,
                                      NULL,
                                      NULL))
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return FALSE;
    }

    bRet = NtGdiGetFontResourceInfoInternalW(
        NtFileName.Buffer,
        NtFileName.Length,
        1,
        *pdwBufSize,
        pdwBufSize,
        lpBuffer,
        dwType);

    RtlFreeHeap(RtlGetProcessHeap(), 0, NtFileName.Buffer);

    if (!bRet)
    {
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
int
STDCALL
SetTextCharacterExtra(
	HDC	hDC,
	int	CharExtra
	)
{
  INT cExtra = 0x80000000;
  PDC_ATTR Dc_Attr;

  if (CharExtra == cExtra)
  {
     SetLastError(ERROR_INVALID_PARAMETER);
     return cExtra;
  }
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
  {
    return MFDRV_SetTextCharacterExtra( hDC, CharExtra ); // Wine port.
  }
#endif
  if (!GdiGetHandleUserData((HGDIOBJ) hDC, (PVOID) &Dc_Attr)) return cExtra;

  if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hDC)
  {
     if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
     {
       NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
       Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
     }
  }
  cExtra = Dc_Attr->lTextExtra;
  Dc_Attr->lTextExtra = CharExtra;
  return cExtra;
// return GetAndSetDCDWord( hDC, GdiGetSetTextCharExtra, CharExtra, 0, 0, 0 );
}

/*
 * @implemented
 *
 */
UINT
STDCALL
GetTextAlign(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->lTextAlign;
}


/*
 * @implemented
 *
 */
COLORREF
STDCALL
GetTextColor(HDC hdc)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return 0;
  return Dc_Attr->ulForegroundClr;
}



/*
 * @unimplemented
 */
UINT
STDCALL
SetTextAlign(HDC hdc,
             UINT fMode)
{
  PDC_ATTR Dc_Attr;
  INT OldMode = 0;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return OldMode;
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetTextAlign( hdc, fMode )
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        if return EMFDRV_SetTextAlign( hdc, fMode )
      }
    }
  }
#endif
  OldMode = Dc_Attr->lTextAlign;
  Dc_Attr->lTextAlign = fMode; // Raw
  Dc_Attr->flTextAlign = fMode & 0x1f;
  return OldMode;

}


/*
 * @implemented
 */
COLORREF
STDCALL
SetTextColor(
	HDC hdc,
	COLORREF crColor
)
{
  PDC_ATTR Dc_Attr;
  COLORREF OldColor = CLR_INVALID;

  if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return OldColor;
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetTextColor( hDC, crColor );
    else
    {
      PLDC pLDC = Dc_Attr->pvLDC;
      if ( !pLDC )
      {
         SetLastError(ERROR_INVALID_HANDLE);
         return FALSE;
      }
      if (pLDC->iType == LDC_EMFLDC)
      {
        if return EMFDRV_SetTextColor( hDC, crColor );
      }
    }
  }
#endif
  OldColor = (COLORREF) Dc_Attr->ulForegroundClr;
  Dc_Attr->ulForegroundClr = (ULONG) crColor;

  if ( Dc_Attr->crForegroundClr != crColor )
  {
     Dc_Attr->ulDirty_ |= DIRTY_TEXT;
     Dc_Attr->crForegroundClr = crColor;
  }
  return OldColor;
}

/*
 * @implemented
 */
BOOL
STDCALL
SetTextJustification(
	HDC	hdc,
	int	extra,
	int	breaks
	)
{
  PDC_ATTR Dc_Attr;
  if (!GdiGetHandleUserData((HGDIOBJ) hdc, (PVOID) &Dc_Attr)) return FALSE;
#if 0
  if (GDI_HANDLE_GET_TYPE(hDC) != GDI_OBJECT_TYPE_DC)
  {
    if (GDI_HANDLE_GET_TYPE(hDC) == GDI_OBJECT_TYPE_METADC)
      return MFDRV_SetTextJustification( hdc, extra, breaks )
    else
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }
#endif
  if (NtCurrentTeb()->GdiTebBatch.HDC == (ULONG)hdc)
  {
     if (Dc_Attr->ulDirty_ & DC_FONTTEXT_DIRTY)
     {
       NtGdiFlush(); // Sync up Dc_Attr from Kernel space.
       Dc_Attr->ulDirty_ &= ~(DC_MODE_DIRTY|DC_FONTTEXT_DIRTY);
     }
  }
  Dc_Attr->cBreak = breaks;
  Dc_Attr->lBreakExtra = extra;
  return TRUE;
}


