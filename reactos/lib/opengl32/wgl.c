/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/wgl.c
 * PURPOSE:              OpenGL32 lib, wglXXX functions
 * PROGRAMMER:           Anich Gregor (blight)
 * UPDATE HISTORY:
 *                       Feb 2, 2004: Created
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>

#include "opengl32.h"


BOOL WINAPI wglCopyContext( HGLRC src, HGLRC dst, UINT mask )
{
	return FALSE;
}


HGLRC WINAPI wglCreateContext( HDC hdc )
{
	return wglCreateLayerContext( hdc, 0 );
}


HGLRC WINAPI wglCreateLayerContext( HDC hdc, int layer )
{
	HKEY hKey;
	WCHAR subKey[1024] = L"SOFTWARE\\Microsoft\\Windows NT\\"
	                      "CurrentVersion\\OpenGLDrivers";
	LONG ret;
	WCHAR driver[256];
	DWORD size;
	DWORD dw;
	FILETIME time;

	GLDRIVERDATA *icd;
	HGLRC drvHglrc = NULL;
	GLRC *hglrc;

	if (GetObjectType( hdc ) != OBJ_DC)
	{
		DBGPRINT( "Wrong object type" );
		return NULL;
	}

	/* find out which icd to load */
	ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldn't open registry key '%ws'\n", subKey );
		return FALSE;
	}

	/* allocate our GLRC */
	hglrc = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (GLRC) );
	if (!hglrc)
		return NULL;

	for (dw = 0; ; dw++)
	{
		size = 256;
		ret = RegEnumKeyExW( hKey, dw, driver, &size, NULL, NULL, NULL, &time );
		if (ret != ERROR_SUCCESS )
			break;

		icd = OPENGL32_LoadICD( driver );
		if (!icd)
			continue;

		drvHglrc = icd->DrvCreateLayerContext( hdc, layer );
		if (!drvHglrc)
		{
			DBGPRINT( "Info: DrvCreateLayerContext (driver = %ws) failed: %d\n",
			          icd->driver_name, GetLastError() );
			OPENGL32_UnloadICD( icd );
			continue;
		}

		/* the ICD was loaded successfully and we got a HGLRC -- success! */
		break;
	}
	RegCloseKey( hKey );

	if (!drvHglrc)
	{
		/* FIXME: fallback to mesa */
		HeapFree( GetProcessHeap(), 0, hglrc );
		return NULL;
	}

	/* we have our GLRC in hglrc and the ICDs in drvHglrc */
	hglrc->hglrc = drvHglrc;
	hglrc->iFormat = -1; /* what is this used for? */
	hglrc->icd = icd;
	hglrc->threadid = 0xFFFFFFFF; /* TODO: make sure this is the "invalid" value */
	memcpy( hglrc->func_list, icd->func_list, sizeof (PVOID) * GLIDX_COUNT );

	/* FIXME: fill NULL-pointers in hglrc->func_list with mesa functions */

	/* FIXME: append hglrc to process-local list of contexts */
}


BOOL WINAPI wglDeleteContext( HGLRC hglrc )
{
	return FALSE;
}


BOOL WINAPI wglDescribeLayerPlane( HDC hdc, int iPixelFormat, int iLayerPlane,
                                   UINT nBytes, LPLAYERPLANEDESCRIPTOR plpd )
{
	return FALSE;
}


HGLRC WINAPI wglGetCurrentContext()
{
	return NULL;
}


HDC WINAPI wglGetCurrentDC()
{
	return NULL;
}


int WINAPI wglGetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart,
                                      int cEntries, COLORREF *pcr )
{
	return 0;
}


PROC WINAPI wglGetProcAddress( LPCSTR proc )
{
	return NULL;
}


BOOL WINAPI wglMakeCurrent( HDC hdc, HGLRC hglrc )
{
	return FALSE;
}


BOOL WINAPI wglRealizeLayerPalette( HDC hdc, int iLayerPlane, BOOL bRealize )
{
	return FALSE;
}


int WINAPI wglSetLayerPaletteEntries( HDC hdc, int iLayerPlane, int iStart,
                                      int cEntries, CONST COLORREF *pcr )
{
	return 0;
}


BOOL WINAPI wglShareLists( HGLRC hglrc1, HGLRC hglrc2 )
{
	return FALSE;
}


BOOL WINAPI wglSwapLayerBuffers( HDC hdc, UINT fuPlanes )
{
	return FALSE;
}


BOOL WINAPI wglUseFontBitmapsA( HDC hdc, DWORD  first, DWORD count, DWORD listBase )
{
	return FALSE;
}


BOOL WINAPI wglUseFontBitmapsW( HDC hdc, DWORD  first, DWORD count, DWORD listBase )
{
	return FALSE;
}


BOOL WINAPI wglUseFontOutlinesA( HDC hdc, DWORD first, DWORD count, DWORD listBase,
                                 FLOAT deviation, FLOAT extrusion, int  format,
                                 LPGLYPHMETRICSFLOAT  lpgmf )
{
	return FALSE;
}


BOOL WINAPI wglUseFontOutlinesW( HDC hdc, DWORD first, DWORD count, DWORD listBase,
                                 FLOAT deviation, FLOAT extrusion, int  format,
                                 LPGLYPHMETRICSFLOAT  lpgmf )
{
	return FALSE;
}

