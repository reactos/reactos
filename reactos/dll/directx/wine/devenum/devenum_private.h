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

#ifndef RC_INVOKED
#include <stdarg.h>
#endif

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#include <windef.h>
#include <winbase.h>
#include <wingdi.h>
//#include "winuser.h"
#include <winreg.h>
//#include "winerror.h"

#define COBJMACROS

#include <ole2.h>
#include <strmif.h>
//#include "olectl.h"
#include <uuids.h>

#ifndef RC_INVOKED
#include <wine/unicode.h>
#endif

/**********************************************************************
 * Dll lifetime tracking declaration for devenum.dll
 */
extern LONG dll_refs;
static inline void DEVENUM_LockModule(void) { InterlockedIncrement(&dll_refs); }
static inline void DEVENUM_UnlockModule(void) { InterlockedDecrement(&dll_refs); }


/**********************************************************************
 * ClassFactory declaration for devenum.dll
 */
typedef struct
{
    const IClassFactoryVtbl *lpVtbl;
} ClassFactoryImpl;

typedef struct
{
    const ICreateDevEnumVtbl *lpVtbl;
} CreateDevEnumImpl;

typedef struct
{
    const IParseDisplayNameVtbl *lpVtbl;
} ParseDisplayNameImpl;

typedef struct
{
    const IEnumMonikerVtbl *lpVtbl;
    LONG ref;
    DWORD index;
    HKEY hkey;
} EnumMonikerImpl;

typedef struct
{
    const IMonikerVtbl *lpVtbl;
    LONG ref;
    HKEY hkey;
} MediaCatMoniker;

MediaCatMoniker * DEVENUM_IMediaCatMoniker_Construct(void);
HRESULT DEVENUM_IEnumMoniker_Construct(HKEY hkey, IEnumMoniker ** ppEnumMoniker);

extern ClassFactoryImpl DEVENUM_ClassFactory;
extern CreateDevEnumImpl DEVENUM_CreateDevEnum;
extern ParseDisplayNameImpl DEVENUM_ParseDisplayName;

/**********************************************************************
 * Private helper function to get AM filter category key location
 */
HRESULT DEVENUM_GetCategoryKey(REFCLSID clsidDeviceClass, HKEY *pBaseKey, WCHAR *wszRegKeyName, UINT maxLen);

/**********************************************************************
 * Global string constant declarations
 */
extern const WCHAR clsid_keyname[6];
extern const WCHAR wszInstanceKeyName[];
#define CLSID_STR_LEN (sizeof(clsid_keyname) / sizeof(WCHAR))

/**********************************************************************
 * Resource IDs
 */
#define IDS_DEVENUM_DSDEFAULT 7
#define IDS_DEVENUM_DS        8
#define IDS_DEVENUM_WODEFAULT 9
#define IDS_DEVENUM_MIDEFAULT 10
#define IDS_DEVENUM_KSDEFAULT 11
#define IDS_DEVENUM_KS        12
