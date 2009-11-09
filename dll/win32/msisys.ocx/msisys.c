/*
 * Stub implementation of MSISYS.OCX to prevent MSINFO32.EXE from crashing.
 *
 * Copyright 2002 Mike McCormack for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>

#include "windef.h"
#include "winerror.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "objbase.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(msisys);


/***********************************************************************
 *		MSISYS_InitProcess (internal)
 */
static BOOL MSISYS_InitProcess( void )
{
	TRACE("()\n");

	return TRUE;
}

/***********************************************************************
 *		MSISYS_UninitProcess (internal)
 */
static void MSISYS_UninitProcess( void )
{
	TRACE("()\n");
}

/***********************************************************************
 *		DllMain for MSISYS
 */
BOOL WINAPI DllMain(
	HINSTANCE hInstDLL,
	DWORD fdwReason,
	LPVOID lpvReserved )
{
	TRACE("(%p,%d,%p)\n",hInstDLL,fdwReason,lpvReserved);

	switch ( fdwReason )
	{
	case DLL_PROCESS_ATTACH:
                DisableThreadLibraryCalls(hInstDLL);
		if ( !MSISYS_InitProcess() )
			return FALSE;
		break;
	case DLL_PROCESS_DETACH:
		MSISYS_UninitProcess();
		break;
	}

	return TRUE;
}


/***********************************************************************
 *		DllCanUnloadNow (MSISYS.@)
 *
 * RETURNS
 *    Success: S_OK
 *    Failure: S_FALSE
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
	return S_OK;
}

/***********************************************************************
 *		DllGetClassObject (MSISYS.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID pclsid, REFIID piid, LPVOID *ppv)
{
        FIXME("\n");

	return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllRegisterServer (MSISYS.@)
 */

HRESULT WINAPI DllRegisterServer( void )
{
	FIXME( "(): stub\n" );
	return E_FAIL;
}

/***********************************************************************
 *		DllUnregisterServer (MSISYS.@)
 */

HRESULT WINAPI DllUnregisterServer( void )
{
	FIXME( "(): stub\n" );
	return E_FAIL;
}
