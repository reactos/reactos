#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>


/*
 * @implemented
 */
DWORD
STDCALL
GetObjectType(
        HGDIOBJ         a0
        )
{
        return W32kGetObjectType(a0);
}



/*
 * @implemented
 */
BOOL
STDCALL
DPtoLP(
        HDC     a0,
        LPPOINT a1,
        int     a2
        )
{
        return W32kDPtoLP(a0, a1, a2);
}


/*
 * @implemented
 */
COLORREF
STDCALL
SetBkColor(
        HDC             a0,
        COLORREF        a1
        )
{
        return W32kSetBkColor(a0, a1);
}


/*
 * @implemented
 */
int
STDCALL
GetGraphicsMode(
        HDC     a0
        )
{
        return W32kGetGraphicsMode(a0);
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
        return W32kSetGraphicsMode(hdc, iMode);
}


/*
 * @implemented
 */
int
STDCALL
GetMapMode(
        HDC     a0
        )
{
        return W32kGetMapMode(a0);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetCurrentPositionEx(
        HDC     a0,
        LPPOINT a1
        )
{
        return W32kGetCurrentPositionEx(a0, a1);
}


/*
 * @implemented
 */
COLORREF
STDCALL
GetBkColor(
        HDC     a0
        )
{
        return W32kGetBkColor(a0);
}


/*
 * @implemented
 */
int
STDCALL
GetBkMode(
        HDC     a0
        )
{
        return W32kGetBkMode(a0);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetBrushOrgEx(
        HDC     a0,
        LPPOINT a1
        )
{
        return W32kGetBrushOrgEx(a0, a1);
}


/*
 * @implemented
 */
int
STDCALL
GetROP2(
        HDC     a0
        )
{
        return W32kGetROP2(a0);

}


/*
 * @implemented
 */
int
STDCALL
GetStretchBltMode(
        HDC     a0
        )
{
        return W32kGetStretchBltMode(a0);

}



/*
 * @implemented
 */
UINT
STDCALL
GetTextAlign(
        HDC     hDc
        )
{
        return W32kGetTextAlign(hDc);

}


/*
 * @implemented
 */
COLORREF
STDCALL
GetTextColor(
        HDC     hDc
        )
{
        return W32kGetTextColor(hDc);

}


/*
 * @implemented
 */
BOOL
STDCALL
GetViewportExtEx(
        HDC     hDc,
        LPSIZE  lpSize
        )
{
        return W32kGetViewportExtEx(hDc, lpSize);

}


/*
 * @implemented
 */
BOOL
STDCALL
GetViewportOrgEx(
        HDC             hDc,
        LPPOINT         lpPoint
        )
{
        return W32kGetViewportOrgEx(hDc, lpPoint);

}


/*
 * @implemented
 */
BOOL
STDCALL
GetWindowExtEx(
        HDC             hDc,
        LPSIZE          lpSize
        )
{
        return W32kGetWindowExtEx(hDc, lpSize);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetWindowOrgEx(
        HDC             hDc,
        LPPOINT         lpPoint
        )
{
        return W32kGetWindowOrgEx(hDc, lpPoint);
}


/*
 * @implemented
 */
int
STDCALL
SetBkMode(
        HDC     a0,
        int     a1
        )
{
        return W32kSetBkMode(a0, a1);

}


/*
 * @implemented
 */
int
STDCALL
SetROP2(
        HDC     a0,
        int     a1
        )
{
        return W32kSetROP2(a0, a1);
}


/*
 * @implemented
 */
int
STDCALL
SetStretchBltMode(
        HDC     a0,
        int     a1
        )
{
        return W32kSetStretchBltMode(a0, a1);

}


/*
 * @implemented
 */
DWORD
STDCALL
GetRelAbs(
         HDC  a0,
         DWORD a1
           )
{
        return W32kGetRelAbs(a0);

}


/*
 * @implemented
 */
HGDIOBJ STDCALL
GetStockObject(int Index)
{
  return(W32kGetStockObject(Index));
}


/*
 * @implemented
 */
int STDCALL
GetClipBox(HDC hDc, LPRECT Rect)
{
  return(W32kGetClipBox(hDc, Rect));
}


/*
 * @implemented
 */
int
STDCALL
GetPolyFillMode(
	HDC	a0
	)
{
	return W32kGetPolyFillMode(a0);
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
	return W32kCreateDC (
			lpwszDriver,
			lpwszDevice,
			lpwszOutput,
			(PDEVMODEW)lpInitData
			);
}


/*
 * @implemented
 */
BOOL STDCALL DeleteDC( HDC hDC )
{
  return W32kDeleteDC( hDC );
}


/*
 * @implemented
 */
HDC
STDCALL
CreateCompatibleDC(
	HDC  hDC
	)
{
	return W32kCreateCompatableDC(hDC);
}


/*
 * @implemented
 */
HGDIOBJ
STDCALL
SelectObject(
	HDC	hDC,
	HGDIOBJ	hGDIObj
	)
{
	return W32kSelectObject(hDC, hGDIObj);
}


/*
 * @implemented
 */
int
STDCALL
SetMapMode(
	HDC	a0,
	int	a1
	)
{
  return W32kSetMapMode( a0, a1 );
}


/*
 * @implemented
 */
BOOL
STDCALL
SetViewportOrgEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPPOINT	a3
	)
{
  return W32kSetViewportOrgEx( a0, a1, a2, a3 );
}


/*
 * @implemented
 */
BOOL
STDCALL
OffsetViewportOrgEx(
	HDC	DC,
	int	XOffset,
	int	YOffset,
	LPPOINT	Point
	)
{
  return W32kOffsetViewportOrgEx(DC, XOffset, YOffset, Point);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetWindowOrgEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPPOINT	a3
	)
{
  return W32kSetWindowOrgEx( a0, a1, a2, a3 );
}


/*
 * @implemented
 */
BOOL
STDCALL
DeleteObject(
	HGDIOBJ		a0
	)
{
	return W32kDeleteObject(a0);
}


/*
 * @implemented
 */
HPALETTE
STDCALL
SelectPalette(
	HDC		a0,
	HPALETTE	a1,
	BOOL		a2
	)
{
	return W32kSelectPalette( a0, a1,a2 );
}


/*
 * @implemented
 */
UINT
STDCALL
RealizePalette(
	HDC	a0
	)
{
	return W32kRealizePalette( a0 );
}


/*
 * @implemented
 */
BOOL
STDCALL
LPtoDP(
	HDC	a0,
	LPPOINT	a1,
	int	a2
	)
{
	return W32kLPtoDP(a0, a1, a2);
}


/*
 * @implemented
 */
int
STDCALL
SetPolyFillMode(
	HDC	a0,
	int	a1
	)
{
	return W32kSetPolyFillMode(a0, a1);
}


/*
 * @implemented
 */
int
STDCALL
GetDeviceCaps(
	HDC	DC,
	int	Index
	)
{
	return W32kGetDeviceCaps(DC, Index);
}

/*
 * @implemented
 */
HPALETTE
STDCALL
CreatePalette(
	CONST LOGPALETTE	*a0
	)
{
	return W32kCreatePalette((CONST PLOGPALETTE)a0);
}

/*
 * @implemented
 */
COLORREF
STDCALL
GetNearestColor(
	HDC		a0,
	COLORREF	a1
	)
{
	return W32kGetNearestColor(a0,a1);
}

/*
 * @implemented
 */
UINT
STDCALL
GetNearestPaletteIndex(
	HPALETTE	a0,
	COLORREF	a1
	)
{
	return W32kGetNearestPaletteIndex(a0,a1);
}

/*
 * @implemented
 */
UINT
STDCALL
GetPaletteEntries(
	HPALETTE	a0,
	UINT		a1,
	UINT		a2,
	LPPALETTEENTRY	a3
	)
{
	return W32kGetPaletteEntries(a0,a1,a2,a3);
}

/*
 * @implemented
 */
UINT
STDCALL
GetSystemPaletteEntries(
	HDC		a0,
	UINT		a1,
	UINT		a2,
	LPPALETTEENTRY	a3
	)
{
	return W32kGetSystemPaletteEntries(a0,a1,a2,a3);
}

/*
 * @implemented
 */
BOOL
STDCALL
RestoreDC(
	HDC	a0,
	int	a1
	)
{
	return W32kRestoreDC(a0,a1);
}


/*
 * @implemented
 */
int
STDCALL
SaveDC(
	HDC	a0
	)
{
	return W32kSaveDC(a0);
}

/*
 * @implemented
 */
UINT
STDCALL
SetPaletteEntries(
	HPALETTE		a0,
	UINT			a1,
	UINT			a2,
	CONST PALETTEENTRY	*a3
	)
{
	return W32kSetPaletteEntries(a0,a1,a2,(CONST PPALETTEENTRY)a3);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetWorldTransform(
	HDC		hdc,
	LPXFORM		a1
	)
{
	return W32kGetWorldTransform(hdc,a1);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetWorldTransform(
	HDC		a0,
	CONST XFORM	*a1
	)
{
	return W32kSetWorldTransform(a0,(CONST PXFORM)a1);
}

/*
 * @implemented
 */
BOOL
STDCALL
ModifyWorldTransform(
	HDC		a0,
	CONST XFORM	*a1,
	DWORD		a2
	)
{
	return W32kModifyWorldTransform(a0,(CONST PXFORM)a1,a2);
}

/*
 * @implemented
 */
BOOL
STDCALL
CombineTransform(
	LPXFORM		a0,
	CONST XFORM	*a1,
	CONST XFORM	*a2
	)
{
	return W32kCombineTransform(a0,(CONST PXFORM)a1,(CONST PXFORM)a2);
}

/*
 * @implemented
 */
UINT
STDCALL
SetDIBColorTable(
	HDC		hdc,
	UINT		a1,
	UINT		a2,
	CONST RGBQUAD	*a3
	)
{
	return W32kSetDIBColorTable(hdc,a1,a2,(CONST PRGBQUAD)a3);
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
	return W32kCreateHalftonePalette(hdc);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetViewportExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	return W32kSetViewportExtEx(a0,a1,a2,a3);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetWindowExtEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	return W32kSetWindowExtEx(a0,a1,a2,a3);
}

/*
 * @implemented
 */
BOOL
STDCALL
OffsetWindowOrgEx(
	HDC	a0,
	int	a1,
	int	a2,
	LPPOINT	a3
	)
{
	return W32kOffsetWindowOrgEx(a0,a1,a2,a3);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetBitmapDimensionEx(
	HBITMAP	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	return W32kSetBitmapDimensionEx(a0,a1,a2,a3);
}

/*
 * @implemented
 */
BOOL
STDCALL
GetDCOrgEx(
	HDC	a0,
	LPPOINT	a1
	)
{
	return W32kGetDCOrgEx(a0,a1);
}
