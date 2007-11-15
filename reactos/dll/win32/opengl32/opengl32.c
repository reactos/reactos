/* $Id$
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
#define WIN32_NO_STATUS
#include <windows.h>
#include <winreg.h>
#include "teb.h"

#include <string.h>
#include "opengl32.h"


/* function prototypes */
static void OPENGL32_AppendICD( GLDRIVERDATA *icd );
static void OPENGL32_RemoveICD( GLDRIVERDATA *icd );
static GLDRIVERDATA *OPENGL32_LoadDriver( LPCWSTR regKey );
static DWORD OPENGL32_InitializeDriver( GLDRIVERDATA *icd );
static BOOL OPENGL32_UnloadDriver( GLDRIVERDATA *icd );
static DWORD OPENGL32_RegGetDriverInfo( LPCWSTR driver, GLDRIVERDATA *icd );


/* global vars */
/* Do not assume it have the free value -1 set, any value can be in here */
DWORD OPENGL32_tls = -1;
GLPROCESSDATA OPENGL32_processdata;


static BOOL
OPENGL32_ThreadAttach()
{
	GLTHREADDATA* lpData = NULL;
	PROC *dispatchTable = NULL;
	TEB *teb = NULL;

	dispatchTable = (PROC*)HeapAlloc( GetProcessHeap(),
	                                  HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
	                                  sizeof (((ICDTable *)(0))->dispatch_table) );
	if (dispatchTable == NULL)
	{
		DBGPRINT( "Error: Couldn't allocate GL dispatch table" );
		return FALSE;
	}

	lpData = (GLTHREADDATA*)HeapAlloc( GetProcessHeap(),
	                                   HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
	                                   sizeof (GLTHREADDATA) );
	if (lpData == NULL)
	{
		DBGPRINT( "Error: Couldn't allocate GLTHREADDATA" );
		HeapFree( GetProcessHeap(), 0, dispatchTable );
		return FALSE;
	}

	teb = NtCurrentTeb();

	/* initialize dispatch table with empty functions */
	#define X(func, ret, typeargs, args, icdidx, tebidx, stack)            \
		dispatchTable[icdidx] = (PROC)glEmptyFunc##stack;                  \
		if (tebidx >= 0)                                                   \
			teb->glDispatchTable[tebidx] = (PVOID)glEmptyFunc##stack;
	GLFUNCS_MACRO
	#undef X

	teb->glTable = dispatchTable;
	TlsSetValue( OPENGL32_tls, lpData );

	return TRUE;
}


static void
OPENGL32_ThreadDetach()
{
	GLTHREADDATA* lpData = NULL;
	PROC *dispatchTable = NULL;

	rosglMakeCurrent( NULL, NULL );

	lpData = (GLTHREADDATA*)TlsGetValue( OPENGL32_tls );
	if (lpData != NULL)
	{
		if (!HeapFree( GetProcessHeap(), 0, lpData ))
			DBGPRINT( "Warning: HeapFree() on GLTHREADDATA failed (%d)",
			          GetLastError() );
	}

	dispatchTable = NtCurrentTeb()->glTable;
	if (dispatchTable != NULL)
	{
		if (!HeapFree( GetProcessHeap(), 0, dispatchTable ))
			DBGPRINT( "Warning: HeapFree() on dispatch table failed (%d)",
			          GetLastError() );
	}
}


static BOOL
OPENGL32_ProcessAttach()
{
	SECURITY_ATTRIBUTES attrib = { sizeof (SECURITY_ATTRIBUTES), /* nLength */
	                               NULL, /* lpSecurityDescriptor */
	                               TRUE /* bInheritHandle */ };

	OPENGL32_tls = TlsAlloc();
	if (-1 == OPENGL32_tls)
		return FALSE;

	memset( &OPENGL32_processdata, 0, sizeof (OPENGL32_processdata) );

	/* create driver, glrc & dcdata list mutex */
	OPENGL32_processdata.driver_mutex = CreateMutex( &attrib, FALSE, NULL );
	if (OPENGL32_processdata.driver_mutex == NULL)
	{
		DBGPRINT( "Error: Couldn't create driver_list mutex (%d)",
		          GetLastError() );
		return FALSE;
	}
	OPENGL32_processdata.glrc_mutex = CreateMutex( &attrib, FALSE, NULL );
	if (OPENGL32_processdata.glrc_mutex == NULL)
	{
		DBGPRINT( "Error: Couldn't create glrc_list mutex (%d)",
		          GetLastError() );
		return FALSE;
	}
	OPENGL32_processdata.dcdata_mutex = CreateMutex( &attrib, FALSE, NULL );
	if (OPENGL32_processdata.dcdata_mutex == NULL)
	{
		DBGPRINT( "Error: Couldn't create dcdata_list mutex (%d)",
		          GetLastError() );
		return FALSE;
	}

	return TRUE;
}


static void
OPENGL32_ProcessDetach()
{
	GLDRIVERDATA *icd, *icd2;
	GLDCDATA *dcdata, *dcdata2;
	GLRC *glrc, *glrc2;

	/* free lists */
	for (dcdata = OPENGL32_processdata.dcdata_list; dcdata != NULL;)
	{
		dcdata2 = dcdata;
		dcdata = dcdata->next;
		if (!HeapFree( GetProcessHeap(), 0, dcdata2 ))
			DBGPRINT( "Warning: HeapFree() on DCDATA 0x%08x failed (%d)",
			          dcdata2, GetLastError() );
	}

	for (glrc = OPENGL32_processdata.glrc_list; glrc != NULL;)
	{
		glrc2 = glrc;
		glrc = glrc->next;
		if (!HeapFree( GetProcessHeap(), 0, glrc2 ))
			DBGPRINT( "Warning: HeapFree() on GLRC 0x%08x failed (%d)",
			          glrc2, GetLastError() );
	}

	for (icd = OPENGL32_processdata.driver_list; icd != NULL;)
	{
		icd2 = icd;
		icd = icd->next;
		if (!HeapFree( GetProcessHeap(), 0, icd2 ))
			DBGPRINT( "Warning: HeapFree() on DRIVERDATA 0x%08x failed (%d)",
			          icd2, GetLastError() );
	}

	/* free mutexes */
	if (OPENGL32_processdata.driver_mutex != NULL)
		CloseHandle( OPENGL32_processdata.driver_mutex );
	if (OPENGL32_processdata.glrc_mutex != NULL)
		CloseHandle( OPENGL32_processdata.glrc_mutex );
	if (OPENGL32_processdata.dcdata_mutex != NULL)
		CloseHandle( OPENGL32_processdata.dcdata_mutex );

	/* free TLS */
	if (OPENGL32_tls != -1)
		TlsFree(OPENGL32_tls);
}


BOOL WINAPI
DllMain(HINSTANCE hInstance, DWORD Reason, LPVOID Reserved)
{
	DBGPRINT( "Info: Called!" );
	switch ( Reason )
	{
	/* The DLL is loading due to process
	 * initialization or a call to LoadLibrary.
	 */
	case DLL_PROCESS_ATTACH:
		DBGTRACE( "Process attach" );
		if (!OPENGL32_ProcessAttach())
			return FALSE;
		/* No break: Initialize the index for first thread. */

	/* The attached process creates a new thread. */
	case DLL_THREAD_ATTACH:
		DBGTRACE( "Thread attach" );
		if (!OPENGL32_ThreadAttach())
			return FALSE;
		break;

	/* The thread of the attached process terminates. */
	case DLL_THREAD_DETACH:
		DBGTRACE( "Thread detach" );
		/* Release the allocated memory for this thread.*/
		OPENGL32_ThreadDetach();
		break;

	/* DLL unload due to process termination or FreeLibrary. */
	case DLL_PROCESS_DETACH:
		DBGTRACE( "Process detach" );
		OPENGL32_ThreadDetach();
		OPENGL32_ProcessDetach();
		break;
	}

	return TRUE;
}


/*! \brief Append ICD to linked list.
 *
 * \param icd  GLDRIVERDATA to append to list
 *
 * \note Only call this when you hold the driver_mutex.
 */
static void
OPENGL32_AppendICD( GLDRIVERDATA *icd )
{
	if (OPENGL32_processdata.driver_list == NULL)
		OPENGL32_processdata.driver_list = icd;
	else
	{
		GLDRIVERDATA *p = OPENGL32_processdata.driver_list;
		while (p->next != NULL)
			p = p->next;
		p->next = icd;
	}
}


/*! \brief Remove ICD from linked list.
 *
 * \param icd  GLDRIVERDATA to remove from list
 *
 * \note Only call this when you hold the driver_mutex.
 */
static void
OPENGL32_RemoveICD( GLDRIVERDATA *icd )
{
	if (icd == OPENGL32_processdata.driver_list)
		OPENGL32_processdata.driver_list = icd->next;
	else
	{
		GLDRIVERDATA *p = OPENGL32_processdata.driver_list;
		while (p != NULL)
		{
			if (p->next == icd)
			{
				p->next = icd->next;
				return;
			}
			p = p->next;
		}
		DBGPRINT( "Error: ICD 0x%08x not found in list!", icd );
	}
}


/*! \brief  Load an ICD.
 *
 * \param driver  Name of installable client driver.
 *
 * \return Pointer to an allocated GLDRIVERDATA struct.
 * \retval NULL  Failure.
 *
 * \todo Call SetLastError() where appropriate.
 */
static GLDRIVERDATA *
OPENGL32_LoadDriver( LPCWSTR driver )
{
	LONG ret;
	GLDRIVERDATA *icd;

	DBGPRINT( "Info: Loading driver %ws...", driver );

	/* allocate driver data */
	icd = (GLDRIVERDATA*)HeapAlloc( GetProcessHeap(),
	                                HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY,
	                                sizeof (GLDRIVERDATA) );
	if (icd == NULL)
	{
		DBGPRINT( "Error: Couldn't allocate GLDRIVERDATA! (%d)", GetLastError() );
		return NULL;
	}

	ret = OPENGL32_RegGetDriverInfo( driver, icd );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldn't query driver information (%d)", ret );
		if (!HeapFree( GetProcessHeap(), 0, icd ))
			DBGPRINT( "Error: HeapFree() returned false, error code = %d",
			          GetLastError() );
		return NULL;
	}

	DBGPRINT( "Info: Dll = %ws", icd->dll );
	DBGPRINT( "Info: Version = 0x%08x", icd->version );
	DBGPRINT( "Info: DriverVersion = 0x%08x", icd->driver_version );
	DBGPRINT( "Info: Flags = 0x%08x", icd->flags );

	/* load/initialize ICD */
	ret = OPENGL32_InitializeDriver( icd );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldnt initialize ICD!" );
		if (!HeapFree( GetProcessHeap(), 0, icd ))
			DBGPRINT( "Error: HeapFree() returned false, error code = %d",
			          GetLastError() );
		return NULL;
	}

	/* append ICD to list */
	OPENGL32_AppendICD( icd );
	DBGPRINT( "Info: ICD loaded." );

	return icd;
}


/*! \brief Initialize a driver (Load DLL and DrvXxx procs)
 *
 * \param icd  ICD to initialize with the dll, version, driverVersion
 *             and flags already filled.
 * \return Error code.
 * \retval ERROR_SUCCESS  Success
 */
#define LOAD_DRV_PROC( icd, proc, required ) \
	*(char**)&icd->proc = (char*)GetProcAddress( icd->handle, #proc ); \
	if (required && icd->proc == NULL) { \
		DBGPRINT( "Error: GetProcAddress(\"%s\") failed!", #proc ); \
		FreeLibrary( icd->handle ); \
		return GetLastError(); \
	}

static DWORD
OPENGL32_InitializeDriver( GLDRIVERDATA *icd )
{
	/* check version */
	if (icd->version > 2)
		DBGPRINT( "Warning: ICD version > 2 (%d)", icd->version );

	/* load dll */
	icd->handle = LoadLibraryW( icd->dll );
	if (icd->handle == NULL)
	{
		DWORD err = GetLastError();
		DBGPRINT( "Error: Couldn't load DLL! (%d)", err );
		return err;
	}

	/* validate version */
	if (icd->driver_version > 1)
	{
		LOAD_DRV_PROC(icd, DrvValidateVersion, FALSE);
		if (icd->DrvValidateVersion != NULL)
		{
			if (!icd->DrvValidateVersion( icd->driver_version ))
			{
				DBGPRINT( "Error: DrvValidateVersion failed!" );
				DBGBREAK();
				FreeLibrary( icd->handle );
				return ERROR_INVALID_FUNCTION; /* FIXME: use better error code */
			}
		}
		else
			DBGPRINT( "Info: DrvValidateVersion not exported by ICD" );
	}

	/* load DrvXXX procs */
	LOAD_DRV_PROC(icd, DrvCopyContext, TRUE);
	LOAD_DRV_PROC(icd, DrvCreateContext, FALSE);
	LOAD_DRV_PROC(icd, DrvCreateLayerContext, FALSE);
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

	/* we require at least one of DrvCreateContext and DrvCreateLayerContext */
	if (icd->DrvCreateContext == NULL && icd->DrvCreateLayerContext == NULL)
	{
		DBGPRINT( "Error: One of DrvCreateContext/DrvCreateLayerContext is required!" );
		FreeLibrary( icd->handle );
		return ERROR_INVALID_FUNCTION; /* FIXME: use better error code... */
	}

	return ERROR_SUCCESS;
}


/*! \brief Unload ICD.
 *
 * \retval  TRUE  Success.
 * \retval  FALSE Failure.
 */
static BOOL
OPENGL32_UnloadDriver( GLDRIVERDATA *icd )
{
	BOOL allOk = TRUE;

	DBGPRINT( "Info: Unloading driver %ws...", icd->driver_name );
	if (icd->refcount != 0)
		DBGPRINT( "Warning: ICD refcount = %d (should be 0)", icd->refcount );

	/* unload dll */
	if (!FreeLibrary( icd->handle ))
	{
		allOk = FALSE;
		DBGPRINT( "Warning: FreeLibrary on ICD %ws failed! (%d)", icd->dll,
		          GetLastError() );
	}

	/* free resources */
	OPENGL32_RemoveICD( icd );
	if (!HeapFree( GetProcessHeap(), 0, icd ))
	{
		allOk = FALSE;
		DBGPRINT( "Warning: HeapFree() returned FALSE, error code = %d",
		          GetLastError() );
	}

	return allOk;
}


/*! \brief Load ICD (shared ICD data)
 *
 * \return Pointer to an allocated GLDRIVERDATA on success.
 * \retval NULL  Failure.
 */
GLDRIVERDATA *
OPENGL32_LoadICD( LPCWSTR driver )
{
	GLDRIVERDATA *icd;

	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.driver_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return NULL; /* FIXME: do we have to expect such an error and handle it? */
	}

	/* look if ICD is already loaded */
	for (icd = OPENGL32_processdata.driver_list; icd; icd = icd->next)
	{
		if (!_wcsicmp( driver, icd->driver_name )) /* found */
		{
			icd->refcount++;

			/* release mutex */
			if (!ReleaseMutex( OPENGL32_processdata.driver_mutex ))
				DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );

			return icd;
		}
	}

	/* not found - try to load */
	icd = OPENGL32_LoadDriver( driver );
	if (icd != NULL)
		icd->refcount = 1;

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.driver_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );

	return icd;
}


/*! \brief Unload ICD (shared ICD data)
 *
 * \retval TRUE   Success.
 * \retval FALSE  Failure.
 */
BOOL
OPENGL32_UnloadICD( GLDRIVERDATA *icd )
{
	BOOL ret = TRUE;

	/* synchronize */
	if (WaitForSingleObject( OPENGL32_processdata.driver_mutex, INFINITE ) ==
	    WAIT_FAILED)
	{
		DBGPRINT( "Error: WaitForSingleObject() failed (%d)", GetLastError() );
		return FALSE; /* FIXME: do we have to expect such an error and handle it? */
	}

	if (--icd->refcount == 0)
		ret = OPENGL32_UnloadDriver( icd );

	/* release mutex */
	if (!ReleaseMutex( OPENGL32_processdata.driver_mutex ))
		DBGPRINT( "Error: ReleaseMutex() failed (%d)", GetLastError() );

	return ret;
}


/*! \brief Enumerate OpenGLDrivers (from registry)
 *
 * \param idx    Index of the driver to get information about.
 * \param name   Pointer to an array of WCHARs (can be NULL)
 * \param cName  Pointer to a DWORD. Input is len of name array.
 *               Output is length of the drivername.
 *               Can be NULL if name is NULL.
 *
 * \return Error code
 * \retval ERROR_NO_MORE_ITEMS  End of driver list.
 * \retval ERROR_SUCCESS        Success.
 */
#if 0 /* unused */
DWORD
OPENGL32_RegEnumDrivers( DWORD idx, LPWSTR name, LPDWORD cName )
{
	HKEY hKey;
	LPCWSTR subKey = OPENGL_DRIVERS_SUBKEY;
	LONG ret;
	DWORD size;
	WCHAR driver[256];

	if (name == NULL)
		return ERROR_SUCCESS; /* nothing to do */

	if (cName == NULL)
		return ERROR_INVALID_FUNCTION; /* we need cName when name is given */

	/* open OpenGLDrivers registry key */
	ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldn't open registry key '%ws'", subKey );
		return ret;
	}

	/* get subkey name */
	size = sizeof (driver) / sizeof (driver[0]);
	ret = RegEnumKeyW( hKey, idx, name, *cName );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldn't get OpenGLDrivers subkey name (%d)", ret );
		RegCloseKey( hKey );
		return ret;
	}
	*cName = wcslen( name );

	/* close key */
	RegCloseKey( hKey );
	return ERROR_SUCCESS;
}
#endif /* 0 -- unused */


/*! \brief Get registry values for a driver given a name.
 *
 * \param driver  Name of the driver to get information about.
 * \param icd     Pointer to GLDRIVERDATA.
 *
 * \return Error code.
 * \retval ERROR_SUCCESS  Success.
 *
 * \note On success the following fields of \a icd are filled: \a driver_name,
 *       \a dll, \a version, \a driver_version and \a flags.
 */
static DWORD
OPENGL32_RegGetDriverInfo( LPCWSTR driver, GLDRIVERDATA *icd )
{
	HKEY hKey;
	WCHAR subKey[1024] = OPENGL_DRIVERS_SUBKEY"\\";
	LONG ret;
	DWORD type, size;

	/* drivers registry values */
	DWORD version = 1, driverVersion = 0, flags = 0;
	WCHAR dll[256];

	/* open driver registry key */
	wcsncat( subKey, driver, 1024 );
	ret = RegOpenKeyExW( HKEY_LOCAL_MACHINE, subKey, 0, KEY_READ, &hKey );
	if (ret != ERROR_SUCCESS)
	{
		DBGPRINT( "Error: Couldn't open registry key '%ws'", subKey );
		return ret;
	}

	/* query values */
	size = sizeof (dll);
	ret = RegQueryValueExW( hKey, L"Dll", 0, &type, (LPBYTE)dll, &size );
	if (ret != ERROR_SUCCESS || type != REG_SZ)
	{
		DBGPRINT( "Error: Couldn't query Dll value or not a string" );
		RegCloseKey( hKey );
		return ret;
	}

	size = sizeof (DWORD);
	ret = RegQueryValueExW( hKey, L"Version", 0, &type, (LPBYTE)&version, &size );
	if (ret != ERROR_SUCCESS || type != REG_DWORD)
		DBGPRINT( "Warning: Couldn't query Version value or not a DWORD" );

	size = sizeof (DWORD);
	ret = RegQueryValueExW( hKey, L"DriverVersion", 0, &type,
							(LPBYTE)&driverVersion, &size );
	if (ret != ERROR_SUCCESS || type != REG_DWORD)
		DBGPRINT( "Warning: Couldn't query DriverVersion value or not a DWORD" );

	size = sizeof (DWORD);
	ret = RegQueryValueExW( hKey, L"Flags", 0, &type, (LPBYTE)&flags, &size );
	if (ret != ERROR_SUCCESS || type != REG_DWORD)
		DBGPRINT( "Warning: Couldn't query Flags value or not a DWORD" );

	/* close key */
	RegCloseKey( hKey );

	/* output data */
	/* FIXME: NUL-terminate strings? */
	wcsncpy( icd->driver_name, driver,
	         sizeof (icd->driver_name) / sizeof (icd->driver_name[0]) - 1 );
	wcsncpy( icd->dll, dll,
	         sizeof (icd->dll) / sizeof (icd->dll[0]) );
	icd->version = version;
	icd->driver_version = driverVersion;
	icd->flags = flags;

	return ERROR_SUCCESS;
}

/* EOF */

