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
        HGDIOBJ         a0
        )
{
        return NtGdiGetObjectType(a0);
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
        return NtGdiDPtoLP(a0, a1, a2);
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
        return NtGdiSetBkColor(a0, a1);
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
        return NtGdiGetGraphicsMode(a0);
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
        HDC     a0
        )
{
        return NtGdiGetMapMode(a0);
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
        return NtGdiGetCurrentPositionEx(a0, a1);
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
        return NtGdiGetBkColor(a0);
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
        return NtGdiGetBkMode(a0);
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
        return NtGdiGetBrushOrgEx(a0, a1);
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
        return NtGdiGetROP2(a0);

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
        return NtGdiGetStretchBltMode(a0);

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
        return NtGdiGetTextAlign(hDc);

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
        return NtGdiGetTextColor(hDc);

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
        return NtGdiGetViewportExtEx(hDc, lpSize);

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
        return NtGdiGetViewportOrgEx(hDc, lpPoint);

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
        return NtGdiGetWindowExtEx(hDc, lpSize);
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
        return NtGdiGetWindowOrgEx(hDc, lpPoint);
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
        return NtGdiSetBkMode(a0, a1);

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
        return NtGdiSetROP2(a0, a1);
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
        return NtGdiSetStretchBltMode(a0, a1);

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
        return NtGdiGetRelAbs(a0);

}


/*
 * @implemented
 */
HGDIOBJ STDCALL
GetStockObject(int Index)
{
  return(NtGdiGetStockObject(Index));
}


/*
 * @implemented
 */
int STDCALL
GetClipBox(HDC hDc, LPRECT Rect)
{
  return(NtGdiGetClipBox(hDc, Rect));
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
	return NtGdiGetPolyFillMode(a0);
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
BOOL STDCALL DeleteDC( HDC hDC )
{
  return NtGdiDeleteDC( hDC );
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
	return NtGdiCreateCompatableDC(hDC);
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
	return NtGdiSelectObject(hDC, hGDIObj);
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
  return NtGdiSetMapMode( a0, a1 );
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
  return NtGdiSetViewportOrgEx( a0, a1, a2, a3 );
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
  return NtGdiOffsetViewportOrgEx(DC, XOffset, YOffset, Point);
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
  return NtGdiSetWindowOrgEx( a0, a1, a2, a3 );
}


/*
 * @implemented
 */
BOOL
STDCALL
DeleteObject(HGDIOBJ Obj)
{
  if (0 != ((DWORD) Obj & 0x00800000))
    {
      DPRINT1("Trying to delete system object 0x%x\n", Obj);
      return FALSE;
    }

  return NtGdiDeleteObject(Obj);
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
	return NtGdiSelectPalette( a0, a1,a2 );
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
	return NtGdiRealizePalette( a0 );
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
	return NtGdiLPtoDP(a0, a1, a2);
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
	return NtGdiSetPolyFillMode(a0, a1);
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
	return NtGdiGetDeviceCaps(DC, Index);
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
	return NtGdiCreatePalette((CONST PLOGPALETTE)a0);
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
	return NtGdiGetNearestColor(a0,a1);
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
	return NtGdiGetNearestPaletteIndex(a0,a1);
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
	return NtGdiGetPaletteEntries(a0,a1,a2,a3);
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
	return NtGdiGetSystemPaletteEntries(a0,a1,a2,a3);
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
	return NtGdiRestoreDC(a0,a1);
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
	return NtGdiSaveDC(a0);
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
	return NtGdiSetPaletteEntries(a0,a1,a2,(CONST PPALETTEENTRY)a3);
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
	return NtGdiGetWorldTransform(hdc,a1);
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
	return NtGdiSetWorldTransform(a0,(CONST PXFORM)a1);
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
	return NtGdiModifyWorldTransform(a0,(CONST PXFORM)a1,a2);
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
	return NtGdiCombineTransform(a0,(CONST PXFORM)a1,(CONST PXFORM)a2);
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
	return NtGdiSetDIBColorTable(hdc,a1,a2,(RGBQUAD*)a3);
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
	HDC	a0,
	int	a1,
	int	a2,
	LPSIZE	a3
	)
{
	return NtGdiSetViewportExtEx(a0,a1,a2,a3);
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
	return NtGdiSetWindowExtEx(a0,a1,a2,a3);
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
	return NtGdiOffsetWindowOrgEx(a0,a1,a2,a3);
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
	return NtGdiSetBitmapDimensionEx(a0,a1,a2,a3);
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
	return NtGdiGetDCOrgEx(a0,a1);
}

/*
 * @implemented
*/
LONG
STDCALL
GetDCOrg(
    HDC a0
    )
{
    // Officially obsolete by Microsoft
    POINT Pt;
    if (!NtGdiGetDCOrgEx(a0,&Pt))
        return 0;
    return(MAKELONG(Pt.x, Pt.y));
}

/*
 * @implemented
 */
BOOL
STDCALL
RectVisible(
	HDC		a0,
	CONST RECT	*a1
	)
{
	return NtGdiRectVisible(a0,(RECT *)a1);
}

/*
 * @implemented
 */
int
STDCALL
ExtEscape(
	HDC		a0,
	int		a1,
	int		a2,
	LPCSTR		a3,
	int		a4,
	LPSTR		a5
	)
{
	return NtGdiExtEscape(a0, a1, a2, a3, a4, a5);
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
