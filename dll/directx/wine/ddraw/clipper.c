/* DirectDrawClipper implementation
 *
 * Copyright 2000 (c) Marcus Meissner
 * Copyright 2000 (c) TransGaming Technologies Inc.
 * Copyright 2006 (c) Stefan Dösinger
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
#include <stdlib.h>
#include <string.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "ddraw.h"
#include "winerror.h"

#include "ddraw_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ddraw);

/*****************************************************************************
 * IUnknown methods
 *****************************************************************************/

/*****************************************************************************
 * IDirectDrawClipper::QueryInterface
 *
 * Can query the IUnknown and IDirectDrawClipper interface from a
 * Clipper object. The IUnknown Interface is equal to the IDirectDrawClipper
 * interface. Can't create other interfaces.
 *
 * Arguments:
 *  riid: Interface id asked for
 *  ppvObj: Returns the pointer to the interface
 *
 * Return values:
 *  DD_OK on success
 *  E_NOINTERFACE if the requested interface wasn't found.
 *
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_QueryInterface(
    LPDIRECTDRAWCLIPPER iface, REFIID riid, LPVOID* ppvObj
) {
    if (IsEqualGUID(&IID_IUnknown, riid)
	|| IsEqualGUID(&IID_IDirectDrawClipper, riid))
    {
        IUnknown_AddRef(iface);
        *ppvObj = iface;
        return S_OK;
    }
    else
    {
	return E_NOINTERFACE;
    }
}

/*****************************************************************************
 * IDirectDrawClipper::AddRef
 *
 * Increases the reference count of the interface, returns the new count
 *
 *****************************************************************************/
static ULONG WINAPI IDirectDrawClipperImpl_AddRef( LPDIRECTDRAWCLIPPER iface )
{
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p)->() incrementing from %u.\n", This, ref - 1);

    return ref;
}

/*****************************************************************************
 * IDirectDrawClipper::Release
 *
 * Decreases the reference count of the interface, returns the new count
 * If the refcount is decreased to 0, the interface is destroyed.
 *
 *****************************************************************************/
static ULONG WINAPI IDirectDrawClipperImpl_Release(IDirectDrawClipper *iface) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->() decrementing from %u.\n", This, ref + 1);

    if (ref == 0)
    {
        EnterCriticalSection(&ddraw_cs);
        IWineD3DClipper_Release(This->wineD3DClipper);
        HeapFree(GetProcessHeap(), 0, This);
        LeaveCriticalSection(&ddraw_cs);
        return 0;
    }
    else return ref;
}

/*****************************************************************************
 * IDirectDrawClipper::SetHwnd
 *
 * Assigns a hWnd to the clipper interface.
 *
 * Arguments:
 *  Flags: Unsupported so far
 *  hWnd: The hWnd to set
 *
 * Return values:
 *  DD_OK on success
 *  DDERR_INVALIDPARAMS if Flags was != 0
 *
 *****************************************************************************/

static HRESULT WINAPI IDirectDrawClipperImpl_SetHwnd(
    LPDIRECTDRAWCLIPPER iface, DWORD dwFlags, HWND hWnd
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%08x,%p)\n", This, dwFlags, hWnd);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DClipper_SetHWnd(This->wineD3DClipper,
                                 dwFlags,
                                 hWnd);
    LeaveCriticalSection(&ddraw_cs);
    switch(hr)
    {
        case WINED3DERR_INVALIDCALL:        return DDERR_INVALIDPARAMS;
        default:                            return hr;
    }
}

/*****************************************************************************
 * IDirectDrawClipper::GetClipList
 *
 * Retrieve a copy of the clip list
 *
 * Arguments:
 *  Rect: Rectangle to be used to clip the clip list or NULL for the
 *        entire clip list
 *  ClipList: structure for the resulting copy of the clip list.
 *            If NULL, fills Size up to the number of bytes necessary to hold
 *            the entire clip.
 *  Size: Size of resulting clip list; size of the buffer at ClipList
 *        or, if ClipList is NULL, receives the required size of the buffer
 *        in bytes
 *
 * RETURNS
 *  Either DD_OK or DDERR_*
 ************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_GetClipList(
    LPDIRECTDRAWCLIPPER iface, LPRECT lpRect, LPRGNDATA lpClipList,
    LPDWORD lpdwSize)
{
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    HRESULT hr;
    TRACE("(%p,%p,%p,%p)\n", This, lpRect, lpClipList, lpdwSize);

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DClipper_GetClipList(This->wineD3DClipper,
                                     lpRect,
                                     lpClipList,
                                     lpdwSize);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawClipper::SetClipList
 *
 * Sets or deletes (if lprgn is NULL) the clip list
 *
 * This implementation is a stub and returns DD_OK always to make the app
 * happy.
 *
 * PARAMS
 *  lprgn   Pointer to a LRGNDATA structure or NULL
 *  dwFlags not used, must be 0
 * RETURNS
 *  Either DD_OK or DDERR_*
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_SetClipList(
    LPDIRECTDRAWCLIPPER iface,LPRGNDATA lprgn,DWORD dwFlag
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    HRESULT hr;

    EnterCriticalSection(&ddraw_cs);
    hr = IWineD3DClipper_SetClipList(This->wineD3DClipper,
                                     lprgn,
                                     dwFlag);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawClipper::GetHwnd
 *
 * Returns the hwnd assigned with SetHwnd
 *
 * Arguments:
 *  hWndPtr: Address to store the HWND at
 *
 * Return values:
 *  Always returns DD_OK;
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_GetHWnd(
    LPDIRECTDRAWCLIPPER iface, HWND* hWndPtr
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    HRESULT hr;
    TRACE("(%p)->(%p)\n", This, hWndPtr);

    EnterCriticalSection(&ddraw_cs);
    hr =  IWineD3DClipper_GetHWnd(This->wineD3DClipper,
                                  hWndPtr);
    LeaveCriticalSection(&ddraw_cs);
    return hr;
}

/*****************************************************************************
 * IDirectDrawClipper::Initialize
 *
 * Initializes the interface. Well, there isn't much to do for this
 * implementation, but it stores the DirectDraw Interface.
 *
 * Arguments:
 *  DD: Pointer to a IDirectDraw interface
 *  Flags: Unsupported by now
 *
 * Return values:
 *  DD_OK on success
 *  DDERR_ALREADYINITIALIZED if this interface isn't initialized already
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_Initialize(
     LPDIRECTDRAWCLIPPER iface, LPDIRECTDRAW lpDD, DWORD dwFlags
) {
    IDirectDrawImpl* pOwner;
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    TRACE("(%p)->(%p,0x%08x)\n", This, lpDD, dwFlags);

    EnterCriticalSection(&ddraw_cs);
    if (This->ddraw_owner != NULL)
    {
        LeaveCriticalSection(&ddraw_cs);
        return DDERR_ALREADYINITIALIZED;
    }

    pOwner = lpDD ? ddraw_from_ddraw1(lpDD) : NULL;
    This->ddraw_owner = pOwner;

    LeaveCriticalSection(&ddraw_cs);
    return DD_OK;
}

/*****************************************************************************
 * IDirectDrawClipper::IsClipListChanged
 *
 * This function is a stub
 *
 * Arguments:
 *  Changed:
 *
 * Return values:
 *  DD_OK, because it's a stub
 *****************************************************************************/
static HRESULT WINAPI IDirectDrawClipperImpl_IsClipListChanged(
    LPDIRECTDRAWCLIPPER iface, BOOL* lpbChanged
) {
    IDirectDrawClipperImpl *This = (IDirectDrawClipperImpl *)iface;
    FIXME("(%p)->(%p),stub!\n",This,lpbChanged);

    /* XXX What is safest? */
    *lpbChanged = FALSE;

    return DD_OK;
}

/*****************************************************************************
 * The VTable
 *****************************************************************************/
const IDirectDrawClipperVtbl IDirectDrawClipper_Vtbl =
{
    IDirectDrawClipperImpl_QueryInterface,
    IDirectDrawClipperImpl_AddRef,
    IDirectDrawClipperImpl_Release,
    IDirectDrawClipperImpl_GetClipList,
    IDirectDrawClipperImpl_GetHWnd,
    IDirectDrawClipperImpl_Initialize,
    IDirectDrawClipperImpl_IsClipListChanged,
    IDirectDrawClipperImpl_SetClipList,
    IDirectDrawClipperImpl_SetHwnd
};
