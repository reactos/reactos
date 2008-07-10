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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * NOTES ON FILE:
 * - Private file where devenum globals are declared
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "winerror.h"

#define COBJMACROS
#define COM_NO_WINDOWS_H

#include "ole2.h"
#include "olectl.h"
#include "wine/unicode.h"
#include "uuids.h"
#include "strmif.h"
/**********************************************************************
 * Dll lifetime tracking declaration for devenum.dll
 */
extern LONG dll_refs;
static __inline void DEVENUM_LockModule(void) { InterlockedIncrement(&dll_refs); }
static __inline void DEVENUM_UnlockModule(void) { InterlockedDecrement(&dll_refs); }


/**********************************************************************
 * ClassFactory declaration for devenum.dll
 */
typedef struct
{
    IClassFactoryVtbl *lpVtbl;
} ClassFactoryImpl;

typedef struct
{
    ICreateDevEnumVtbl *lpVtbl;
} CreateDevEnumImpl;

typedef struct
{
    IParseDisplayNameVtbl *lpVtbl;
} ParseDisplayNameImpl;

typedef struct
{
    IEnumMonikerVtbl *lpVtbl;
    LONG ref;
    DWORD index;
    HKEY hkey;
} EnumMonikerImpl;

typedef struct
{
    IMonikerVtbl *lpVtbl;

    LONG ref;
    HKEY hkey;
} MediaCatMoniker;

MediaCatMoniker * DEVENUM_IMediaCatMoniker_Construct(void);
HRESULT DEVENUM_IEnumMoniker_Construct(HKEY hkey, IEnumMoniker ** ppEnumMoniker);
HRESULT WINAPI DEVENUM_ICreateDevEnum_CreateClassEnumerator(
    ICreateDevEnum * iface,
    REFCLSID clsidDeviceClass,
    IEnumMoniker **ppEnumMoniker,
    DWORD dwFlags);

extern ClassFactoryImpl DEVENUM_ClassFactory;
extern CreateDevEnumImpl DEVENUM_CreateDevEnum;
extern ParseDisplayNameImpl DEVENUM_ParseDisplayName;

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
