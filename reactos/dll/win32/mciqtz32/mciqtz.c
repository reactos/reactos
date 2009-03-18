/*
 * DirectShow MCI Driver
 *
 * Copyright 2009 Christian Costa
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "mmddk.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mciqtz);

/*======================================================================*
 *                  	    MCI QTZ implementation			*
 *======================================================================*/

HINSTANCE MCIQTZ_hInstance = 0;

/***********************************************************************
 *		DllMain (MCIQTZ.0)
 */
BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    switch (fdwReason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hInstDLL);
	MCIQTZ_hInstance = hInstDLL;
	break;
    }
    return TRUE;
}

/*======================================================================*
 *                  	    MCI QTZ entry points			*
 *======================================================================*/

/**************************************************************************
 * 				DriverProc (MCIQTZ.@)
 */
LRESULT CALLBACK MCIQTZ_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
                                   LPARAM dwParam1, LPARAM dwParam2)
{
    FIXME("(%08lX, %p, %08X, %08lX, %08lX): Stub!\n",
	  dwDevID, hDriv, wMsg, dwParam1, dwParam2);

    return MCIERR_UNRECOGNIZED_COMMAND;
}
