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
