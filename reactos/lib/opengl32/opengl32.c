/* $Id: opengl32.c,v 1.6 2004/02/02 17:59:23 navaraf Exp $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/opengl32/opengl32.c
 * PURPOSE:              OpenGL32 lib
 * PROGRAMMER:           Anich Gregor (blight), Royce Mitchell III
 * UPDATE HISTORY:
 *                       Feb 1, 2004: Created
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <winreg.h>

#include <string.h>
//#include <GL/gl.h>
#include "opengl32.h"

#define EXPORT __declspec(dllexport)
#define NAKED __attribute__((naked))

/* function prototypes */
static BOOL OPENGL32_LoadDrivers();
static void OPENGL32_AppendICD( GLDRIVERDATA *icd );
static void OPENGL32_RemoveICD( GLDRIVERDATA *icd );
static GLDRIVERDATA *OPENGL32_LoadDriver ( LPCWSTR regKey );
static DWORD OPENGL32_InitializeDriver( GLDRIVERDATA *icd );
static BOOL OPENGL32_UnloadDriver( GLDRIVERDATA *icd );

/* global vars */
const char* OPENGL32_funcnames[GLIDX_COUNT]
            __attribute__((section("shared"), shared)) =
{
#define X(X) #X,
	GLFUNCS_MACRO
#undef X
};

GLPROCESSDATA OPENGL32_processdata;
DWORD OPENGL32_tls;

static void OPENGL32_ThreadDetach()
{
        GLTHREADDATA *lpvData;

	/* FIXME - do we need to release some HDC or something? */
	lpvData = (GLTHREADDATA *)TlsGetValue ( OPENGL32_tls );
	if ( lpvData != NULL )
		LocalFree((HLOCAL) lpvData );
}


BOOL WINAPI DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
	GLTHREADDATA* lpData = NULL;
	DBGPRINT( "DllMain called!\n" );

	switch ( Reason )
	{
	/* The DLL is loading due to process
	 * initialization or a call to LoadLibrary.
	 */
	case DLL_PROCESS_ATTACH:
		OPENGL32_tls = TlsAlloc();
		if ( 0xFFFFFFFF == OPENGL32_tls )
			return FALSE;

		memset( &OPENGL32_processdata, 0, sizeof (OPENGL32_processdata) );

		/* get list of ICDs from registry: */
		OPENGL32_LoadDrivers();
		/* No break: Initialize the index for first thread. */

	/* The attached process creates a new thread. */
	case DLL_THREAD_ATTACH:
		lpData = (GLTHREADDATA*)LocalAlloc(LPTR, sizeof(GLTHREADDATA));
		if ( lpData != NULL )
		{
			memset ( lpData, 0, sizeof(GLTHREADDATA) );
			(void)TlsSetValue ( OPENGL32_tls, lpData );
		}
#if 0
		lpData->hdc = NULL;
		/* FIXME - defaulting to mesa3d, but shouldn't */
		lpData->list = OPENGL32_processdata.list[0];
#endif
		break;

	/* The thread of the attached process terminates. */
	case DLL_THREAD_DETACH:
		/* Release the allocated memory for this thread.*/
		OPENGL32_ThreadDetach();
		break;

	/* DLL unload due to process termination or FreeLibrary. */
	case DLL_PROCESS_DETACH:
		OPENGL32_ThreadDetach();
		/* FIXME: free resources */
		TlsFree(OPENGL32_tls);
		break;
	}
	return TRUE;
}


/* FUNCTION: Append ICD to linked list.
 * ARGUMENTS: [IN] icd: GLDRIVERDATA to append to list
 * TODO: protect from race conditions
 */
static void OPENGL32_AppendICD( GLDRIVERDATA *icd )
{
	if (!OPENGL32_processdata.driver_list)
		OPENGL32_processdata.driver_list = icd;
	else
	{
		GLDRIVERDATA *p = OPENGL32_processdata.driver_list;
		while (p->next)
			p = p->next;
		p->next = icd;
	}
}


/* FUNCTION: Remove ICD from linked list.
 * ARGUMENTS: [IN] icd: GLDRIVERDATA to remove from list
 * TODO: protect from race conditions
 */
static void OPENGL32_RemoveICD( GLDRIVERDATA *icd )
{
	if (icd == OPENGL32_processdata.driver_list)
		OPENGL32_processdata.driver_list = icd->next;
	else
	{
		GLDRIVERDATA *p = OPENGL32_processdata.driver_list;
		while (p)
		{
			if (p->next == icd)
			{
				p->next = icd->next;
				return;
			}
			p = p->next;
		}
		DBGPRINT( "RemoveICD: ICD 0x%08x not found in list!\n" );
	}
}

/* FIXME - I'm assuming we want to return TRUE if we find at least *one* ICD */
static BOOL OPENGL32_LoadDrivers()
{
	const WCHAR* OpenGLDrivers = L"SOFTWARE\\Microsoft\\Windows NT\\"
		"CurrentVersion\\OpenGLDrivers\\";
	HKEY hkey;
	WCHAR name[1024];
	int i;

	/* FIXME - detect if we've already done this from another process */
	/* FIXME - special-case load of MESA3D as generic implementation ICD. */

	if ( ERROR_SUCCESS != RegOpenKeyW( HKEY_LOCAL_MACHINE, OpenGLDrivers, &hkey ) )
		return FALSE;
	for ( i = 0; RegEnumKeyW(hkey,i,name,sizeof(name)/sizeof(name[0])) == ERROR_SUCCESS; i++ )
	{
		/* ignoring return value, because OPENGL32_LoadICD() is doing *all* the work... */
		/* GLDRIVERDATA* gldd =*/ OPENGL32_LoadICD ( name );
	}
	RegCloseKey ( hkey );
	if ( i > 0 )
		return TRUE;
	else
		return FALSE;
}

/* FUNCTION:  Load an ICD.
 * ARGUMENTS: [IN] driver:  Name of display driver.
 * RETURNS:   error code; ERROR_SUCCESS on success
 *
 * TODO: call SetLastError() where appropriate
 */
static GLDRIVERDATA *OPENGL32_LoadDriver ( LPCWSTR driver )
{
	HKEY hKey;
	WCHAR subKey[1024] = L"SOFTWARE\\Microsoft\\Windows NT\\"
	                      "CurrentVersion\\OpenGLDrivers\\";
	LONG ret;
	DWORD type, size;

	DWORD version, driverVersion, flags; /* registry values */
	WCHAR dll[256];
	GLDRIVERDATA *icd;

	DBGPRINT( "Loading driver %ws...\n", driver );

	/* open registry key */
	wcsncat( subKey, driver, 1024 );
	ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldn't open registry key '%ws'\n", subKey );
		return 0;
	}

	/* query values */
	size = sizeof (dll);
	ret = RegQueryValueExW( hKey, L"Dll", 0, &type, (LPBYTE)dll, &size );
	if (ret != ERROR_SUCCESS || type != REG_SZ)
	{
		DBGPRINT( "Error: Couldn't query Dll value or not a string\n" );
		RegCloseKey( hKey );
		return 0;
	}

	size = sizeof (DWORD);
	ret = RegQueryValueExW( hKey, L"Version", 0, &type, (LPBYTE)&version, &size );
	if (ret != ERROR_SUCCESS || type != REG_DWORD)
	{
		DBGPRINT( "Warning: Couldn't query Version value or not a DWORD\n" );
		version = 0;
	}

	size = sizeof (DWORD);
	ret = RegQueryValueExW( hKey, L"DriverVersion", 0, &type,
	                        (LPBYTE)&driverVersion, &size );
	if (ret != ERROR_SUCCESS || type != REG_DWORD)
	{
		DBGPRINT( "Warning: Couldn't query DriverVersion value or not a DWORD\n" );
		driverVersion = 0;
	}

	size = sizeof (DWORD);
	ret = RegQueryValueExW( hKey, L"Flags", 0, &type, (LPBYTE)&flags, &size );
	if (ret != ERROR_SUCCESS || type != REG_DWORD)
	{
		DBGPRINT( "Warning: Couldn't query Flags value or not a DWORD\n" );
		flags = 0;
	}

	/* close key */
	RegCloseKey( hKey );

	DBGPRINT( "Dll = %ws\n", dll );
	DBGPRINT( "Version = 0x%08x\n", version );
	DBGPRINT( "DriverVersion = 0x%08x\n", driverVersion );
	DBGPRINT( "Flags = 0x%08x\n", flags );

	/* allocate driver data */
	icd = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof (GLDRIVERDATA) );
	if (!icd)
	{
		DBGPRINT( "Error: Couldnt allocate GLDRIVERDATA!\n" );
		return 0;
	}
	wcsncpy( icd->driver_name, driver, 256 );
	wcsncpy( icd->dll, dll, 256 );
	icd->version = version;
	icd->driver_version = driverVersion;
	icd->flags = flags;

	/* load ICD */
	ret = OPENGL32_InitializeDriver( icd );
	if (ret != ERROR_SUCCESS)
	{
		if (!HeapFree( GetProcessHeap(), 0, icd ))
			DBGPRINT( "Error: HeapFree() returned false, error code = %d\n",
			          GetLastError() );
		DBGPRINT( "Error: Couldnt initialize ICD!\n" );
		return 0;
	}

	/* append ICD to list */
	OPENGL32_AppendICD( icd );
	DBGPRINT( "ICD loaded.\n" );

	return icd;
}


/* FUNCTION:  Initialize a driver (Load DLL, DrvXXX and glXXX procs)
 * ARGUMENTS: [IN] icd:  ICD to initialize with the dll, version, driverVersion
 *                       and flags already filled.
 * RETURNS:   error code; ERROR_SUCCESS on success
 */
#define LOAD_DRV_PROC( icd, proc, required ) \
	icd->proc = GetProcAddress( icd->handle, #proc ); \
	if (required && !icd->proc) { \
		DBGPRINT( "Error: GetProcAddress(\"%s\") failed!", #proc ); \
		return GetLastError(); \
	}

static DWORD OPENGL32_InitializeDriver( GLDRIVERDATA *icd )
{
	UINT i;

	/* load dll */
	icd->handle = LoadLibraryW( icd->dll );
	if (!icd->handle)
	{
		DBGPRINT( "Error: Couldn't load DLL! (%d)\n", GetLastError() );
		return GetLastError();
	}

	/* load DrvXXX procs */
	LOAD_DRV_PROC(icd, DrvCopyContext, TRUE);
	LOAD_DRV_PROC(icd, DrvCreateContext, TRUE);
	LOAD_DRV_PROC(icd, DrvCreateLayerContext, TRUE);
	LOAD_DRV_PROC(icd, DrvDeleteContext, TRUE);
	LOAD_DRV_PROC(icd, DrvDescribeLayerPlane, TRUE);
	LOAD_DRV_PROC(icd, DrvDescribePixelFormat, TRUE);
	LOAD_DRV_PROC(icd, DrvGetLayerPaletteEntries, TRUE);
	LOAD_DRV_PROC(icd, DrvGetProcAddress, TRUE);
	LOAD_DRV_PROC(icd, DrvReleaseContext, TRUE);
	LOAD_DRV_PROC(icd, DrvRealizeLayerPalette, TRUE);
	LOAD_DRV_PROC(icd, DrvSetContext, TRUE);
	LOAD_DRV_PROC(icd, DrvSetLayerPaletteEntries, TRUE);
	LOAD_DRV_PROC(icd, DrvSetPixelFormat, TRUE);
	LOAD_DRV_PROC(icd, DrvShareLists, TRUE);
	LOAD_DRV_PROC(icd, DrvSwapBuffers, TRUE);
	LOAD_DRV_PROC(icd, DrvSwapLayerBuffers, TRUE);
	LOAD_DRV_PROC(icd, DrvValidateVersion, TRUE);

	/* now load the glXXX functions */
	for (i = 0; i < GLIDX_COUNT; i++)
	{
		icd->func_list[i] = icd->DrvGetProcAddress( OPENGL32_funcnames[i] );
#ifdef DEBUG_OPENGL32_ICD_EXPORTS
		if ( icd->func_list[i] )
		{
			DBGPRINT( "Found function %s in ICD.\n", OPENGL32_funcnames[i] );
		}
#endif
	}

	return ERROR_SUCCESS;
}


/* FUNCTION: Unload loaded ICD.
 * RETURNS:  TRUE on success, FALSE otherwise.
 */
static BOOL OPENGL32_UnloadDriver( GLDRIVERDATA *icd )
{
	BOOL allOk = TRUE;

	DBGPRINT( "Unloading driver %ws...\n", icd->driver_name );
	if (icd->refcount)
		DBGPRINT( "Warning: ICD refcount = %d (should be 0)\n", icd->refcount );

	/* unload dll */
	if (!FreeLibrary( icd->handle ))
	{
		allOk = FALSE;
		DBGPRINT( "Warning: FreeLibrary on ICD %ws failed!\n", icd->dll );
	}

	/* free resources */
	OPENGL32_RemoveICD( icd );
	HeapFree( GetProcessHeap(), 0, icd );

	return allOk;
}


/* FUNCTION: Load ICD (shared ICD data)
 * RETURNS:  GLDRIVERDATA pointer on success, NULL otherwise.
 */
GLDRIVERDATA *OPENGL32_LoadICD ( LPCWSTR driver )
{
	GLDRIVERDATA *icd;

	/* look if ICD is already loaded */
	for (icd = OPENGL32_processdata.driver_list; icd; icd = icd->next)
	{
		if (!_wcsicmp( driver, icd->driver_name )) /* found */
		{
			icd->refcount++;
			return icd;
		}
	}

	/* not found - try to load */
	icd = OPENGL32_LoadDriver ( driver );
	if (icd)
		icd->refcount = 1;
	return icd;
}


/* FUNCTION: Unload ICD (shared ICD data)
 * RETURNS:  TRUE on success, FALSE otherwise.
 */
BOOL OPENGL32_UnloadICD( GLDRIVERDATA *icd )
{
	icd->refcount--;
	if (icd->refcount == 0)
		return OPENGL32_UnloadDriver( icd );

	return TRUE;
}

