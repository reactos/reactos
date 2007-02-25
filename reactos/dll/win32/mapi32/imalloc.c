/*
 * MAPI Default IMalloc implementation
 *
 * Copyright 2004 Jon Griffiths
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

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winuser.h"
#include "winerror.h"
#include "winternl.h"
#include "objbase.h"
#include "shlwapi.h"
#include "mapiutil.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(mapi);

static const IMallocVtbl MAPI_IMalloc_vt;

typedef struct
{
  const IMallocVtbl *lpVtbl;
  LONG lRef;
} MAPI_IMALLOC;

static MAPI_IMALLOC MAPI_IMalloc = { &MAPI_IMalloc_vt, 0u };

extern LONG MAPI_ObjectCount; /* In mapi32_main.c */

/*************************************************************************
 * MAPIGetDefaultMalloc@0 (MAPI32.59)
 *
 * Get the default MAPI IMalloc interface.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  A pointer to the MAPI default allocator.
 */
LPMALLOC WINAPI MAPIGetDefaultMalloc(void)
{
    TRACE("()\n");

    IMalloc_AddRef((LPMALLOC)&MAPI_IMalloc);
    return (LPMALLOC)&MAPI_IMalloc;
}

/**************************************************************************
 * IMAPIMalloc_QueryInterface
 */
static HRESULT WINAPI IMAPIMalloc_fnQueryInterface(LPMALLOC iface, REFIID refiid,
                                                   LPVOID *ppvObj)
{
    TRACE("(%s,%p)\n", debugstr_guid(refiid), ppvObj);

    if (IsEqualIID(refiid, &IID_IUnknown) ||
        IsEqualIID(refiid, &IID_IMalloc))
    {
        *ppvObj = (LPMALLOC) &MAPI_IMalloc;
        TRACE("Returning IMalloc (%p)\n", *ppvObj);
        return S_OK;
    }
    TRACE("Returning E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

/**************************************************************************
 * IMAPIMalloc_AddRef
 */
static ULONG WINAPI IMAPIMalloc_fnAddRef(LPMALLOC iface)
{
    TRACE("(%p)\n", iface);
    InterlockedIncrement(&MAPI_ObjectCount);
    return 1u;
}

/**************************************************************************
 * IMAPIMalloc_Release
 */
static ULONG WINAPI IMAPIMalloc_fnRelease(LPMALLOC iface)
{
    TRACE("(%p)\n", iface);
    InterlockedDecrement(&MAPI_ObjectCount);
    return 1u;
}

/**************************************************************************
 * IMAPIMalloc_Alloc
 */
static LPVOID WINAPI IMAPIMalloc_fnAlloc(LPMALLOC iface, DWORD cb)
{
    TRACE("(%p)->(%d)\n", iface, cb);

    return LocalAlloc(LMEM_FIXED, cb);
}

/**************************************************************************
 * IMAPIMalloc_Realloc
 */
static LPVOID WINAPI IMAPIMalloc_fnRealloc(LPMALLOC iface, LPVOID pv, DWORD cb)
{
    TRACE("(%p)->(%p, %d)\n", iface, pv, cb);

    if (!pv)
        return LocalAlloc(LMEM_FIXED, cb);

    if (cb)
        return LocalReAlloc((HANDLE) pv, cb, LMEM_MOVEABLE);

    LocalFree((HANDLE) pv);
    return NULL;
}

/**************************************************************************
 * IMAPIMalloc_Free
 */
static void WINAPI IMAPIMalloc_fnFree(LPMALLOC iface, LPVOID pv)
{
    TRACE("(%p)->(%p)\n", iface, pv);
    LocalFree((HANDLE) pv);
}

/**************************************************************************
 * IMAPIMalloc_GetSize
 */
static DWORD WINAPI IMAPIMalloc_fnGetSize(LPMALLOC iface, LPVOID pv)
{
    TRACE("(%p)->(%p)\n", iface, pv);
    return LocalSize((HANDLE) pv);
}

/**************************************************************************
 * IMAPIMalloc_DidAlloc
 */
static INT WINAPI IMAPIMalloc_fnDidAlloc(LPMALLOC iface, LPVOID pv)
{
    TRACE("(%p)->(%p)\n", iface, pv);
    return -1;
}

/**************************************************************************
 * IMAPIMalloc_HeapMinimize
 */
static void WINAPI IMAPIMalloc_fnHeapMinimize(LPMALLOC iface)
{
    TRACE("(%p)\n", iface);
}

static const IMallocVtbl MAPI_IMalloc_vt =
{
    IMAPIMalloc_fnQueryInterface,
    IMAPIMalloc_fnAddRef,
    IMAPIMalloc_fnRelease,
    IMAPIMalloc_fnAlloc,
    IMAPIMalloc_fnRealloc,
    IMAPIMalloc_fnFree,
    IMAPIMalloc_fnGetSize,
    IMAPIMalloc_fnDidAlloc,
    IMAPIMalloc_fnHeapMinimize
};
