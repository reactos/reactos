/*
 * Driver Environment functions
 *
 * Note: This has NOTHING to do with the task/process environment!
 *
 * Copyright 1997 Marcus Meissner
 * Copyright 1998 Andreas Mohr
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
#include <stdio.h>
#include <string.h>

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/wingdi16.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(gdi);

typedef struct {
        ATOM atom;
	HGLOBAL16 handle;
} ENVTABLE;

static ENVTABLE EnvTable[20];

static ENVTABLE *SearchEnvTable(ATOM atom)
{
    INT16 i;

    for (i = 19; i >= 0; i--) {
      if (EnvTable[i].atom == atom)
	return &EnvTable[i];
    }
    return NULL;
}

static ATOM GDI_GetNullPortAtom(void)
{
    static ATOM NullPortAtom = 0;
    if (!NullPortAtom)
    {
        char NullPort[256];

        GetProfileStringA( "windows", "nullport", "none",
                             NullPort, sizeof(NullPort) );
        NullPortAtom = AddAtomA( NullPort );
    }
    return NullPortAtom;
}

static ATOM PortNameToAtom(LPCSTR lpPortName, BOOL16 add)
{
    char buffer[256];

    lstrcpynA( buffer, lpPortName, sizeof(buffer) );

    if (buffer[0] && buffer[strlen(buffer)-1] == ':') buffer[strlen(buffer)-1] = 0;

    if (add)
        return AddAtomA(buffer);
    else
        return FindAtomA(buffer);
}


/***********************************************************************
 *           GetEnvironment   (GDI.133)
 */
INT16 WINAPI GetEnvironment16(LPCSTR lpPortName, LPDEVMODEA lpdev, UINT16 nMaxSize)
{
    ATOM atom;
    LPCSTR p;
    ENVTABLE *env;
    WORD size;

    TRACE("('%s', %p, %d)\n", lpPortName, lpdev, nMaxSize);

    if (!(atom = PortNameToAtom(lpPortName, FALSE)))
	return 0;
    if (atom == GDI_GetNullPortAtom())
	if (!(atom = FindAtomA((LPCSTR)lpdev)))
	    return 0;
    if (!(env = SearchEnvTable(atom)))
	return 0;
    size = GlobalSize16(env->handle);
    if (!lpdev) return 0;
    if (size < nMaxSize) nMaxSize = size;
    if (!(p = GlobalLock16(env->handle))) return 0;
    memcpy(lpdev, p, nMaxSize);
    GlobalUnlock16(env->handle);
    return nMaxSize;
}


/***********************************************************************
 *          SetEnvironment   (GDI.132)
 */
INT16 WINAPI SetEnvironment16(LPCSTR lpPortName, LPDEVMODEA lpdev, UINT16 nCount)
{
    ATOM atom;
    BOOL16 nullport = FALSE;
    LPCSTR port_name;
    LPSTR device_mode;
    ENVTABLE *env;
    HGLOBAL16 handle;

    TRACE("('%s', %p, %d)\n", lpPortName, lpdev, nCount);

    if ((atom = PortNameToAtom(lpPortName, FALSE))) {
	if (atom == GDI_GetNullPortAtom()) {
	    nullport = TRUE;
	    atom = FindAtomA((LPCSTR)lpdev);
	}
	env = SearchEnvTable(atom);
        GlobalFree16(env->handle);
        env->atom = 0;
    }
    if (nCount) { /* store DEVMODE struct */
	if (nullport)
	    port_name = (LPSTR)lpdev;
        else
	    port_name = lpPortName;

        if ((atom = PortNameToAtom(port_name, TRUE))
	 && (env = SearchEnvTable(0))
	 && (handle = GlobalAlloc16(GMEM_SHARE|GMEM_MOVEABLE, nCount))) {
	    if (!(device_mode = GlobalLock16(handle))) {
	        GlobalFree16(handle);
	        return 0;
	    }
	    env->atom = atom;
	    env->handle = handle;
            memcpy(device_mode, lpdev, nCount);
            GlobalUnlock16(handle);
            return handle;
	}
	else return 0;
    }
    else return -1;
}
