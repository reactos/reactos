#ifdef UNICODE
#undef UNICODE
#endif

#undef WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <ddk/ntddk.h>
#include <win32k/kapi.h>

HGDIOBJ STDCALL
GetStockObject(int Index)
{
  return(W32kGetStockObject(Index));
}


int STDCALL
GetClipBox(HDC hDc, LPRECT Rect)
{
  return(W32kGetClipBox(hDc, Rect));
}


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

BOOL STDCALL DeleteDC( HDC hDC )
{
  return W32kDeleteDC( hDC );
}


HDC
STDCALL
CreateCompatibleDC(
	HDC  hDC
	)
{
	return W32kCreateCompatableDC(hDC);
}

HGDIOBJ
STDCALL
SelectObject(
	HDC	hDC,
	HGDIOBJ	hGDIObj
	)
{
	return W32kSelectObject(hDC, hGDIObj);
}

int
STDCALL
SetMapMode(
	HDC	a0,
	int	a1
	)
{
  return W32kSetMapMode( a0, a1 );
}

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


BOOL
STDCALL
DeleteObject(
	HGDIOBJ		a0
	)
{
	return W32kDeleteObject(a0);
}

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

UINT
STDCALL
RealizePalette(
	HDC	a0
	)
{
	return W32kRealizePalette( a0 );
}



