#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>
#include <rosrtl/logfont.h>

#define NDEBUG
#include <debug.h>

/*
 * @implemented
 */
DWORD
STDCALL
GetObjectType(
        HGDIOBJ         h
        )
{
  return NtGdiGetObjectType(h);
}



/*
 * @implemented
 */
BOOL
STDCALL
DPtoLP(
        HDC     hdc,
        LPPOINT lpPoints,
        int     nCount
        )
{
  return NtGdiDPtoLP(hdc, lpPoints, nCount);
}


/*
 * @implemented
 */
COLORREF
STDCALL
SetBkColor(
        HDC             hdc,
        COLORREF        crColor
        )
{
  return NtGdiSetBkColor(hdc, crColor);
}


/*
 * @implemented
 */
int
STDCALL
GetGraphicsMode(
        HDC     hdc
        )
{
  return NtGdiGetGraphicsMode(hdc);
}


/*
 * @implemented
 */
int
STDCALL
SetGraphicsMode(
        HDC     hdc,
        int     iMode
        )
{
  return NtGdiSetGraphicsMode(hdc, iMode);
}


/*
 * @implemented
 */
int
STDCALL
GetMapMode(
        HDC     hdc
        )
{
  return NtGdiGetMapMode(hdc);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetCurrentPositionEx(
        HDC     hdc,
        LPPOINT lpPoint
        )
{
  return NtGdiGetCurrentPositionEx(hdc, lpPoint);
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetBkColor(
        HDC     hdc
        )
{
  return NtGdiGetBkColor(hdc);
}


/*
 * @implemented
 */
int
STDCALL
GetBkMode(
        HDC     hdc
        )
{
  return NtGdiGetBkMode(hdc);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetBrushOrgEx(
        HDC     hdc,
        LPPOINT lppt
        )
{
  return NtGdiGetBrushOrgEx(hdc, lppt);
}


/*
 * @implemented
 */
int
STDCALL
GetROP2(
        HDC     hdc
        )
{
  return NtGdiGetROP2(hdc);
}


/*
 * @implemented
 */
int
STDCALL
GetStretchBltMode(
        HDC     hdc
        )
{
  return NtGdiGetStretchBltMode(hdc);
}



/*
 * @implemented
 */
UINT
STDCALL
GetTextAlign(
        HDC     hdc
        )
{
  return NtGdiGetTextAlign(hdc);
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetTextColor(
        HDC     hdc
        )
{
  return NtGdiGetTextColor(hdc);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetViewportExtEx(
        HDC     hdc,
        LPSIZE  lpSize
        )
{
  return NtGdiGetViewportExtEx(hdc, lpSize);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetViewportOrgEx(
        HDC             hdc,
        LPPOINT         lpPoint
        )
{
  return NtGdiGetViewportOrgEx(hdc, lpPoint);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetWindowExtEx(
        HDC             hdc,
        LPSIZE          lpSize
        )
{
  return NtGdiGetWindowExtEx(hdc, lpSize);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetWindowOrgEx(
        HDC             hdc,
        LPPOINT         lpPoint
        )
{
  return NtGdiGetWindowOrgEx(hdc, lpPoint);
}


/*
 * @implemented
 */
int
STDCALL
SetBkMode(
        HDC     hdc,
        int     iBkMode
        )
{
  return NtGdiSetBkMode(hdc, iBkMode);
}


/*
 * @implemented
 */
int
STDCALL
SetROP2(
        HDC     hdc,
        int     fnDrawMode
        )
{
  return NtGdiSetROP2(hdc, fnDrawMode);
}


/*
 * @implemented
 */
int
STDCALL
SetStretchBltMode(
        HDC     hdc,
        int     iStretchMode
        )
{
  return NtGdiSetStretchBltMode(hdc, iStretchMode);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetRelAbs(
         HDC  hdc,
         DWORD dwIgnore
           )
{
  return NtGdiGetRelAbs(hdc);
}


/*
 * @implemented
 */
HGDIOBJ STDCALL
GetStockObject(int fnObject)
{
  return NtGdiGetStockObject(fnObject);
}


/*
 * @implemented
 */
int STDCALL
GetClipBox(
	HDC hdc, 
	LPRECT lprc)
{
  return NtGdiGetClipBox(hdc, lprc);
}


/*
 * @implemented
 */
int
STDCALL
GetPolyFillMode(
	HDC	hdc
	)
{
  return NtGdiGetPolyFillMode(hdc);
}


/*
 * @implemented
 */
HDC
STDCALL
CreateDCA (
	LPCSTR		lpszDriver,
	LPCSTR		lpszDevice,
	LPCSTR		lpszOutput,
	CONST DEVMODEA	* lpInitData
	)
{
        ANSI_STRING DriverA, DeviceA, OutputA;
        UNICODE_STRING DriverU, DeviceU, OutputU;
	HDC	hDC;
	DEVMODEW *lpInitDataW;

	/*
	 * If needed, convert to Unicode
	 * any string parameter.
	 */

	if (NULL != lpszDriver)
	{
		RtlInitAnsiString(&DriverA, (LPSTR)lpszDriver);
		RtlAnsiStringToUnicodeString(&DriverU, &DriverA, TRUE);
	} else
		DriverU.Buffer = NULL;
	if (NULL != lpszDevice)
	{
		RtlInitAnsiString(&DeviceA, (LPSTR)lpszDevice);
		RtlAnsiStringToUnicodeString(&DeviceU, &DeviceA, TRUE);
	} else
		DeviceU.Buffer = NULL;
	if (NULL != lpszOutput)
	{
		RtlInitAnsiString(&OutputA, (LPSTR)lpszOutput);
		RtlAnsiStringToUnicodeString(&OutputU, &OutputA, TRUE);
	} else
		OutputU.Buffer = NULL;

	if (NULL != lpInitData)
	{
//		lpInitDataW = HeapAllocMem(
	} else
		lpInitDataW = NULL;

	/*
	 * Call the Unicode version
	 * of CreateDC.
	 */

	hDC = CreateDCW (
		DriverU.Buffer,
		DeviceU.Buffer,
		OutputU.Buffer,
		NULL);
//		lpInitDataW);
	/*
	 * Free Unicode parameters.
	 */
	RtlFreeUnicodeString(&DriverU);
	RtlFreeUnicodeString(&DeviceU);
	RtlFreeUnicodeString(&OutputU);

	/*
	 * Return the possible DC handle.
	 */

	return hDC;
}


/*
 * @implemented
 */
HDC
STDCALL
CreateDCW (
	LPCWSTR		lpwszDriver,
	LPCWSTR		lpwszDevice,
	LPCWSTR		lpwszOutput,
	CONST DEVMODEW	* lpInitData
	)
{
	UNICODE_STRING Driver, Device, Output;

	if(lpwszDriver)
		RtlInitUnicodeString(&Driver, lpwszDriver);
	if(lpwszDevice)
		RtlInitUnicodeString(&Driver, lpwszDevice);
	if(lpwszOutput)
		RtlInitUnicodeString(&Driver, lpwszOutput);

	return NtGdiCreateDC((lpwszDriver ? &Driver : NULL),
						 (lpwszDevice ? &Device : NULL),
						 (lpwszOutput ? &Output : NULL),
						 (PDEVMODEW)lpInitData);
}


/*
 * @implemented
 */
BOOL STDCALL DeleteDC( HDC hdc )
{
  return NtGdiDeleteDC(hdc);
}


/*
 * @implemented
 */
HDC
STDCALL
CreateCompatibleDC(
	HDC  hdc
	)
{
  return NtGdiCreateCompatableDC(hdc);
}


/*
 * @implemented
 */
HGDIOBJ
STDCALL
SelectObject(
	HDC	hdc,
	HGDIOBJ	hgdiobj
	)
{
  return NtGdiSelectObject(hdc, hgdiobj);
}


/*
 * @implemented
 */
int
STDCALL
SetMapMode(
	HDC	hdc,
	int	fnMapMode
	)
{
  return NtGdiSetMapMode(hdc, fnMapMode);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetViewportOrgEx(
	HDC	hdc,
	int	X,
	int	Y,
	LPPOINT	lpPoint
	)
{
  return NtGdiSetViewportOrgEx(hdc, X, Y, lpPoint);
}


/*
 * @implemented
 */
BOOL
STDCALL
OffsetViewportOrgEx(
	HDC	hdc,
	int	nXOffset,
	int	nYOffset,
	LPPOINT	lpPoint
	)
{
  return NtGdiOffsetViewportOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetWindowOrgEx(
	HDC	hdc,
	int	X,
	int	Y,
	LPPOINT	lpPoint
	)
{
  return NtGdiSetWindowOrgEx(hdc, X, Y, lpPoint);
}


/*
 * @implemented
 */
BOOL
STDCALL
DeleteObject(HGDIOBJ hObject)
{
  if (0 != ((DWORD) hObject & 0x00800000))
    {
      DPRINT1("Trying to delete system object 0x%x\n", hObject);
      return FALSE;
    }

  return NtGdiDeleteObject(hObject);
}


/*
 * @implemented
 */
HPALETTE
STDCALL
SelectPalette(
	HDC		hdc,
	HPALETTE	hpal,
	BOOL		bForceBackground
	)
{
  return NtGdiSelectPalette(hdc, hpal, bForceBackground);
}


/*
 * @implemented
 */
UINT
STDCALL
RealizePalette(
	HDC	hdc
	)
{
  return NtGdiRealizePalette(hdc);
}


/*
 * @implemented
 */
BOOL
STDCALL
LPtoDP(
	HDC	hdc,
	LPPOINT	lpPoints,
	int	nCount
	)
{
  return NtGdiLPtoDP(hdc, lpPoints, nCount);
}


/*
 * @implemented
 */
int
STDCALL
SetPolyFillMode(
	HDC	hdc,
	int	iPolyFillMode
	)
{
  return NtGdiSetPolyFillMode(hdc, iPolyFillMode);
}


/*
 * @implemented
 */
int
STDCALL
GetDeviceCaps(
	HDC	hdc,
	int	nIndex
	)
{
  return NtGdiGetDeviceCaps(hdc, nIndex);
}

/*
 * @implemented
 */
HPALETTE
STDCALL
CreatePalette(
	CONST LOGPALETTE	*lplgpl
	)
{
  return NtGdiCreatePalette((CONST PLOGPALETTE)lplgpl);
}

/*
 * @implemented
 */
COLORREF
STDCALL
GetNearestColor(
	HDC		hdc,
	COLORREF	crColor
	)
{
  return NtGdiGetNearestColor(hdc, crColor);
}

/*
 * @implemented
 */
UINT
STDCALL
GetNearestPaletteIndex(
	HPALETTE	hpal,
	COLORREF	crColor
	)
{
  return NtGdiGetNearestPaletteIndex(hpal, crColor);
}

/*
 * @implemented
 */
UINT
STDCALL
GetPaletteEntries(
	HPALETTE	hpal,
	UINT		iStartIndex,
	UINT		nEntries,
	LPPALETTEENTRY	lppe
	)
{
  return NtGdiGetPaletteEntries(hpal, iStartIndex, nEntries, lppe);
}

/*
 * @implemented
 */
UINT
STDCALL
GetSystemPaletteEntries(
	HDC		hdc,
	UINT		iStartIndex,
	UINT		nEntries,
	LPPALETTEENTRY	lppe
	)
{
  return NtGdiGetSystemPaletteEntries(hdc, iStartIndex, nEntries, lppe);
}

/*
 * @implemented
 */
BOOL
STDCALL
RestoreDC(
	HDC	hdc,
	int	nSavedDC
	)
{
  return NtGdiRestoreDC(hdc, nSavedDC);
}


/*
 * @implemented
 */
int
STDCALL
SaveDC(
	HDC	hdc
	)
{
  return NtGdiSaveDC(hdc);
}

/*
 * @implemented
 */
UINT
STDCALL
SetPaletteEntries(
	HPALETTE		hpal,
	UINT			iStart,
	UINT			cEntries,
	CONST PALETTEENTRY	*lppe
	)
{
  return NtGdiSetPaletteEntries(hpal, iStart, cEntries, (CONST PPALETTEENTRY)lppe);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetWorldTransform(
	HDC		hdc,
	LPXFORM		lpXform
	)
{
  return NtGdiGetWorldTransform(hdc, lpXform);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetWorldTransform(
	HDC		hdc,
	CONST XFORM	*lpXform
	)
{
  return NtGdiSetWorldTransform(hdc, (CONST PXFORM)lpXform);
}

/*
 * @implemented
 */
BOOL
STDCALL
ModifyWorldTransform(
	HDC		hdc,
	CONST XFORM	*lpXform,
	DWORD		iMode
	)
{
  return NtGdiModifyWorldTransform(hdc, (CONST PXFORM)lpXform, iMode);
}

/*
 * @implemented
 */
BOOL
STDCALL
CombineTransform(
	LPXFORM		lpxformResult,
	CONST XFORM	*lpxform1,
	CONST XFORM	*lpxform2
	)
{
  return NtGdiCombineTransform(lpxformResult, (CONST PXFORM)lpxform1, (CONST PXFORM)lpxform2);
}

/*
 * @implemented
 */
UINT
STDCALL
SetDIBColorTable(
	HDC		hdc,
	UINT		uStartIndex,
	UINT		cEntries,
	CONST RGBQUAD	*pColors
	)
{
  return NtGdiSetDIBColorTable(hdc, uStartIndex, cEntries, (RGBQUAD*)pColors);
}

/*
 * @implemented
 */
HPALETTE
STDCALL
CreateHalftonePalette(
	HDC	hdc
	)
{
  return NtGdiCreateHalftonePalette(hdc);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetViewportExtEx(
	HDC	hdc,
	int	nXExtent,
	int	nYExtent,
	LPSIZE	lpSize
	)
{
  return NtGdiSetViewportExtEx(hdc, nXExtent, nYExtent, lpSize);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetWindowExtEx(
	HDC	hdc,
	int	nXExtent,
	int	nYExtent,
	LPSIZE	lpSize
	)
{
  return NtGdiSetWindowExtEx(hdc, nXExtent, nYExtent, lpSize);
}

/*
 * @implemented
 */
BOOL
STDCALL
OffsetWindowOrgEx(
	HDC	hdc,
	int	nXOffset,
	int	nYOffset,
	LPPOINT	lpPoint
	)
{
  return NtGdiOffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetBitmapDimensionEx(
	HBITMAP	hBitmap,
	int	nWidth,
	int	nHeight,
	LPSIZE	lpSize
	)
{
 return NtGdiSetBitmapDimensionEx(hBitmap, nWidth, nHeight, lpSize);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetDCOrgEx(
	HDC	hdc,
	LPPOINT	lpPoint
	)
{
  return NtGdiGetDCOrgEx(hdc, lpPoint);
}

/*
 * @implemented
*/
LONG
STDCALL
GetDCOrg(
    HDC hdc
    )
{
  // Officially obsolete by Microsoft
  POINT Pt;
  if (!NtGdiGetDCOrgEx(hdc, &Pt))
    return 0;
  return(MAKELONG(Pt.x, Pt.y));
}

/*
 * @implemented
 */
BOOL
STDCALL
RectVisible(
	HDC		hdc,
	CONST RECT	*lprc
	)
{
	return NtGdiRectVisible(hdc, (RECT *)lprc);
}

/*
 * @implemented
 */
int
STDCALL
ExtEscape(
	HDC		hdc,
	int		nEscape,
	int		cbInput,
	LPCSTR		lpszInData,
	int		cbOutput,
	LPSTR		lpszOutData
	)
{
  return NtGdiExtEscape(hdc, nEscape, cbInput, lpszInData, cbOutput, lpszOutData);
}


/*
 * @unimplemented
 */
int   
STDCALL 
GetObjectA(HGDIOBJ Handle, int Size, LPVOID Buffer)
{
  LOGFONTW LogFontW;
  DWORD Type;
  int Result;

  Type = GetObjectType(Handle);
  if (0 == Type)
    {
      return 0;
    }

  if (OBJ_FONT == Type)
    {
      if (Size < sizeof(LOGFONTA))
        {
          SetLastError(ERROR_BUFFER_OVERFLOW);
          return 0;
        }
      Result = NtGdiGetObject(Handle, sizeof(LOGFONTW), &LogFontW);
      if (0 == Result)
        {
          return 0;
        }
      RosRtlLogFontW2A((LPLOGFONTA) Buffer, &LogFontW);
      Result = sizeof(LOGFONTA);
    }
  else
    {
      Result = NtGdiGetObject(Handle, Size, Buffer);
    }

  return Result;
}


/*
 * @unimplemented
 */
int   
STDCALL 
GetObjectW(HGDIOBJ Handle, int Size, LPVOID Buffer)
{
  return NtGdiGetObject(Handle, Size, Buffer);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetBrushOrgEx(
	HDC	hdc,
	int	nXOrg,
	int	nYOrg,
	LPPOINT	lppt
	)
{
  return NtGdiSetBrushOrgEx(hdc, nXOrg, nYOrg, lppt);
}


/*
 * @implemented
 */
BOOL
STDCALL
FixBrushOrgEx(
	HDC	hdc,
	int	nXOrg,
	int	nYOrg,
	LPPOINT	lppt
	)
{
  #if 0
  /* FIXME - Check if we're emulating win95, if so, forward to SetBrushOrgEx() */
  return SetBrushOrgEx(hdc, nXOrg, nYOrg, lppt);
  #endif
  
  return FALSE;
}


/*
 * @implemented
 */
HGDIOBJ
STDCALL
GetCurrentObject(
	HDC	hdc,
	UINT	uObjectType
	)
{
  return NtGdiGetCurrentObject(hdc, uObjectType);
}


/*
 * @implemented
 */
BOOL
STDCALL
PtVisible(
	HDC	hdc,
	int	X,
	int	Y
	)
{
  return NtGdiPtVisible(hdc, X, Y);
}

