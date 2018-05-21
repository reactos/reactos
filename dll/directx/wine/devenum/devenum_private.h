/*
 *	includes for devenum.dll
 *
 * Copyright (C) 2002 John K. Hohm
 * Copyright (C) 2002 Robert Shearman
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
 * NOTES ON FILE:
 * - Private file where devenum globals are declared
 */

#ifndef __WINE_DEVENUM_H
#define __WINE_DEVENUM_H

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS
#define NONAMELESSSTRUCT
#define NONAMELESSUNION

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
#include <winreg.h>
#include <objbase.h>
#include <oleidl.h>
#include <strmif.h>
#include <uuids.h>

#include <wine/debug.h>
WINE_DEFAULT_DEBUG_CHANNEL(devenum);

#ifndef RC_INVOKED
#include <wine/unicode.h>
#endif

/**********************************************************************
 * Dll lifetime tracking declaration for devenum.dll
 */
extern LONG dll_refs DECLSPEC_HIDDEN;
static inline void DEVENUM_LockModule(void) { InterlockedIncrement(&dll_refs); }
static inline void DEVENUM_UnlockModule(void) { InterlockedDecrement(&dll_refs); }


/**********************************************************************
 * ClassFactory declaration for devenum.dll
 */
typedef struct
{
    IClassFactory IClassFactory_iface;
} ClassFactoryImpl;

typedef struct
{
    IMoniker IMoniker_iface;
    LONG ref;
    HKEY hkey;
} MediaCatMoniker;

MediaCatMoniker * DEVENUM_IMediaCatMoniker_Construct(void) DECLSPEC_HIDDEN;
HRESULT DEVENUM_IEnumMoniker_Construct(HKEY hkey, HKEY special_hkey, IEnumMoniker ** ppEnumMoniker) DECLSPEC_HIDDEN;

extern ClassFactoryImpl DEVENUM_ClassFactory DECLSPEC_HIDDEN;
extern ICreateDevEnum DEVENUM_CreateDevEnum DECLSPEC_HIDDEN;
extern IParseDisplayName DEVENUM_ParseDisplayName DECLSPEC_HIDDEN;

/**********************************************************************
 * Private helper function to get AM filter category key location
 */
HRESULT DEVENUM_GetCategoryKey(REFCLSID clsidDeviceClass, HKEY *pBaseKey, WCHAR *wszRegKeyName, UINT maxLen) DECLSPEC_HIDDEN;

/**********************************************************************
 * Global string constant declarations
 */
extern const WCHAR clsid_keyname[6] DECLSPEC_HIDDEN;
extern const WCHAR wszInstanceKeyName[] DECLSPEC_HIDDEN;
#define CLSID_STR_LEN (sizeof(clsid_keyname) / sizeof(WCHAR))

#endif /* __WINE_DEVENUM_H */
