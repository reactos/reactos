/*
 * Implementation of miscellaneous interfaces for IE Web Browser control:
 *
 *  - IQuickActivate
 *
 * Copyright 2001 John R. Sheets (for CodeWeavers)
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
 */

#include "wine/debug.h"
#include "shdocvw.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IQuickActivate interface
 */

static HRESULT WINAPI WBQA_QueryInterface(LPQUICKACTIVATE iface,
                                          REFIID riid, LPVOID *ppobj)
{
    FIXME("- no interface\n\tIID:\t%s\n", debugstr_guid(riid));

    if (ppobj == NULL) return E_POINTER;
    
    return E_NOINTERFACE;
}

static ULONG WINAPI WBQA_AddRef(LPQUICKACTIVATE iface)
{
    SHDOCVW_LockModule();

    return 2; /* non-heap based object */
}

static ULONG WINAPI WBQA_Release(LPQUICKACTIVATE iface)
{
    SHDOCVW_UnlockModule();

    return 1; /* non-heap based object */
}

/* Alternative interface for quicker, easier activation of a control. */
static HRESULT WINAPI WBQA_QuickActivate(LPQUICKACTIVATE iface,
                                         QACONTAINER *pQaContainer,
                                         QACONTROL *pQaControl)
{
    FIXME("stub: QACONTAINER = %p, QACONTROL = %p\n", pQaContainer, pQaControl);
    return S_OK;
}

static HRESULT WINAPI WBQA_SetContentExtent(LPQUICKACTIVATE iface, LPSIZEL pSizel)
{
    FIXME("stub: LPSIZEL = %p\n", pSizel);
    return E_NOINTERFACE;
}

static HRESULT WINAPI WBQA_GetContentExtent(LPQUICKACTIVATE iface, LPSIZEL pSizel)
{
    FIXME("stub: LPSIZEL = %p\n", pSizel);
    return E_NOINTERFACE;
}

/**********************************************************************
 * IQuickActivate virtual function table for IE Web Browser component
 */

static const IQuickActivateVtbl WBQA_Vtbl =
{
    WBQA_QueryInterface,
    WBQA_AddRef,
    WBQA_Release,
    WBQA_QuickActivate,
    WBQA_SetContentExtent,
    WBQA_GetContentExtent
};

IQuickActivateImpl SHDOCVW_QuickActivate = {&WBQA_Vtbl};

/**********************************************************************
 * OpenURL  (SHDOCVW.@)
 */
void WINAPI OpenURL(HWND hWnd, HINSTANCE hInst, LPCSTR lpcstrUrl, int nShowCmd)
{
    FIXME("%p %p %s %d\n", hWnd, hInst, debugstr_a(lpcstrUrl), nShowCmd);
}
