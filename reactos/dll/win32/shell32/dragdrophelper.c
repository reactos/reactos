/*
 *	file system folder
 *
 *	Copyright 1997			Marcus Meissner
 *	Copyright 1998, 1999, 2002	Juergen Schmied
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
#include "wine/port.h"

#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "wingdi.h"
#include "winuser.h"

#include "objbase.h"
#include "ole2.h"
#include "shlguid.h"
#include "shlobj.h"

#include "wine/debug.h"
#include "debughlp.h"

WINE_DEFAULT_DEBUG_CHANNEL (shell);

/***********************************************************************
*   IDropTargetHelper implementation
*/

typedef struct {
    const IDropTargetHelperVtbl *lpVtbl;
    LONG ref;
} IDropTargetHelperImpl;

static const IDropTargetHelperVtbl vt_IDropTargetHelper;

#define _IUnknown_(This) (IUnknown*)&(This->lpVtbl)
#define _IDropTargetHelper_(This) (IDropTargetHelper*)&(This->lpVtbl)

/**************************************************************************
*	IDropTargetHelper_Constructor
*/
HRESULT WINAPI IDropTargetHelper_Constructor (IUnknown * pUnkOuter, REFIID riid, LPVOID * ppv)
{
    IDropTargetHelperImpl *dth;

    TRACE ("unkOut=%p %s\n", pUnkOuter, shdebugstr_guid (riid));

    if (!ppv)
	return E_POINTER;
    if (pUnkOuter)
	return CLASS_E_NOAGGREGATION;

    dth = (IDropTargetHelperImpl *) LocalAlloc (LMEM_ZEROINIT, sizeof (IDropTargetHelperImpl));
    if (!dth) return E_OUTOFMEMORY;

    dth->ref = 0;
    dth->lpVtbl = &vt_IDropTargetHelper;

    if (!SUCCEEDED (IUnknown_QueryInterface (_IUnknown_ (dth), riid, ppv))) {
	IUnknown_Release (_IUnknown_ (dth));
	return E_NOINTERFACE;
    }

    TRACE ("--(%p)\n", dth);
    return S_OK;
}

/**************************************************************************
 *	IDropTargetHelper_fnQueryInterface
 */
static HRESULT WINAPI IDropTargetHelper_fnQueryInterface (IDropTargetHelper * iface, REFIID riid, LPVOID * ppvObj)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;

    TRACE ("(%p)->(%s,%p)\n", This, shdebugstr_guid (riid), ppvObj);

    *ppvObj = NULL;

    if (IsEqualIID (riid, &IID_IUnknown) || IsEqualIID (riid, &IID_IDropTargetHelper)) {
	*ppvObj = This;
    }

    if (*ppvObj) {
	IUnknown_AddRef ((IUnknown *) (*ppvObj));
	TRACE ("-- Interface: (%p)->(%p)\n", ppvObj, *ppvObj);
	return S_OK;
    }
    FIXME ("-- Interface: E_NOINTERFACE\n");
    return E_NOINTERFACE;
}

static ULONG WINAPI IDropTargetHelper_fnAddRef (IDropTargetHelper * iface)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    ULONG refCount = InterlockedIncrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount - 1);

    return refCount;
}

static ULONG WINAPI IDropTargetHelper_fnRelease (IDropTargetHelper * iface)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    ULONG refCount = InterlockedDecrement(&This->ref);

    TRACE ("(%p)->(count=%u)\n", This, refCount + 1);

    if (!refCount) {
        TRACE("-- destroying (%p)\n", This);
        LocalFree ((HLOCAL) This);
        return 0;
    }
    return refCount;
}

static HRESULT WINAPI IDropTargetHelper_fnDragEnter (
        IDropTargetHelper * iface,
	HWND hwndTarget,
	IDataObject* pDataObject,
	POINT* ppt,
	DWORD dwEffect)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    FIXME ("(%p)->(%p %p %p 0x%08x)\n", This,hwndTarget, pDataObject, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDragLeave (IDropTargetHelper * iface)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    FIXME ("(%p)->()\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDragOver (IDropTargetHelper * iface, POINT* ppt, DWORD dwEffect)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    FIXME ("(%p)->(%p 0x%08x)\n", This, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnDrop (IDropTargetHelper * iface, IDataObject* pDataObject, POINT* ppt, DWORD dwEffect)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    FIXME ("(%p)->(%p %p 0x%08x)\n", This, pDataObject, ppt, dwEffect);
    return E_NOTIMPL;
}

static HRESULT WINAPI IDropTargetHelper_fnShow (IDropTargetHelper * iface, BOOL fShow)
{
    IDropTargetHelperImpl *This = (IDropTargetHelperImpl *)iface;
    FIXME ("(%p)->(%u)\n", This, fShow);
    return E_NOTIMPL;
}

static const IDropTargetHelperVtbl vt_IDropTargetHelper =
{
	IDropTargetHelper_fnQueryInterface,
	IDropTargetHelper_fnAddRef,
	IDropTargetHelper_fnRelease,
	IDropTargetHelper_fnDragEnter,
	IDropTargetHelper_fnDragLeave,
        IDropTargetHelper_fnDragOver,
	IDropTargetHelper_fnDrop,
	IDropTargetHelper_fnShow
};
