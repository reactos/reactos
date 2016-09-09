/*
 * Internet control panel applet
 *
 * Copyright 2010 Detlef Riekenberg
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
 *
 */

#ifndef __WINE_INETCPL__
#define __WINE_INETCPL__

#include <stdarg.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define CONST_VTABLE
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <ole2.h>
#include <commctrl.h>
#include <shlwapi.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(inetcpl);

extern HMODULE hcpl;
INT_PTR CALLBACK content_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK general_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;
INT_PTR CALLBACK security_dlgproc(HWND, UINT, WPARAM, LPARAM) DECLSPEC_HIDDEN;

/* ## Memory allocation functions ## */

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc( size_t len )
{
    return HeapAlloc( GetProcessHeap(), 0, len );
}

static inline void * __WINE_ALLOC_SIZE(1) heap_alloc_zero( size_t len )
{
    return HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, len );
}

static inline BOOL heap_free( void *mem )
{
    return HeapFree( GetProcessHeap(), 0, mem );
}

/* ######### */

#define NUM_PROPERTY_PAGES 8

#include "resource.h"

#endif /* __WINE_INETCPL__ */
