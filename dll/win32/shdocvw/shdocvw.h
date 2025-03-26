/*
 * Header includes for shdocvw.dll
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
 * Copyright 2005-2006 Jacek Caban for CodeWeavers
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

#ifndef __WINE_SHDOCVW_H
#define __WINE_SHDOCVW_H

#define COBJMACROS

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "ole2.h"
#include "shlobj.h"
#include "exdisp.h"

#include "wine/heap.h"
#include "wine/list.h"

/**********************************************************************
 * Shell Instance Objects
 */
extern HRESULT SHDOCVW_GetShellInstanceObjectClassObject(REFCLSID rclsid, 
    REFIID riid, LPVOID *ppvClassObj) DECLSPEC_HIDDEN;

/**********************************************************************
 * Dll lifetime tracking declaration for shdocvw.dll
 */
#ifdef __REACTOS__
# ifdef __cplusplus
EXTERN_C
# else
extern
# endif
LONG SHDOCVW_refCount;
#else
extern LONG SHDOCVW_refCount DECLSPEC_HIDDEN;
#endif
static inline void SHDOCVW_LockModule(void) { InterlockedIncrement( &SHDOCVW_refCount ); }
static inline void SHDOCVW_UnlockModule(void) { InterlockedDecrement( &SHDOCVW_refCount ); }

#ifdef __REACTOS__
#include "resource.h"
#include "objects.h"
#define ARRAY_SIZE(array) _countof(array)
EXTERN_C HINSTANCE instance;
#endif // def __REACTOS__

#endif /* __WINE_SHDOCVW_H */
