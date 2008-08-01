/*
 *	Start menu object
 *
 *	Copyright 2007 Hervé Poussineau
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

#define COBJMACROS

#include "wine/debug.h"

#include "windef.h"
#include "shlobj.h"
#include "base/shell/explorer-new/todo.h"

#include "shell32_main.h"

WINE_DEFAULT_DEBUG_CHANNEL(shell);

const GUID CLSID_StartMenu = { 0x4622AD11, 0xFF23, 0x11D0, {0x8D,0x34,0x00,0xA0,0xC9,0x0F,0x27,0x19} };

typedef struct _tagStartMenu {
    const IMenuPopupVtbl *vtbl;
    const IObjectWithSiteVtbl *objectSiteVtbl;
    const IInitializeObjectVtbl *initObjectVtbl;
    IUnknown *pUnkSite;
    LONG refCount;
} StartMenu;

static const IMenuPopupVtbl StartMenuVtbl;
static const IObjectWithSiteVtbl StartMenu_ObjectWithSiteVtbl;
static const IInitializeObjectVtbl StartMenu_InitializeObjectVtbl;

static inline StartMenu *impl_from_IMenuPopup(IMenuPopup *iface)
{
    return (StartMenu *)((char *)iface - FIELD_OFFSET(StartMenu, vtbl));
}

static inline StartMenu *impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return (StartMenu *)((char *)iface - FIELD_OFFSET(StartMenu, objectSiteVtbl));
}

static inline StartMenu *impl_from_IInitializeObject(IInitializeObject *iface)
{
    return (StartMenu *)((char *)iface - FIELD_OFFSET(StartMenu, initObjectVtbl));
}

HRESULT WINAPI StartMenu_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    StartMenu *This;

    TRACE("(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    if (pUnkOuter)
        return E_POINTER;

    This = CoTaskMemAlloc(sizeof(StartMenu));
    if (!This)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->vtbl = &StartMenuVtbl;
    This->objectSiteVtbl = &StartMenu_ObjectWithSiteVtbl;
    This->initObjectVtbl = &StartMenu_InitializeObjectVtbl;
    This->refCount = 1;

    TRACE("returning %p\n", This);
    *ppv = (IUnknown *)This;
    return S_OK;
}

static void WINAPI StartMenu_Destructor(StartMenu *This)
{
    TRACE("destroying %p\n", This);
    if (This->pUnkSite) IUnknown_Release(This->pUnkSite);
    CoTaskMemFree(This);
}

static HRESULT WINAPI StartMenu_QueryInterface(IMenuPopup *iface, REFIID iid, LPVOID *ppvOut)
{
    StartMenu *This = impl_from_IMenuPopup(iface);
    *ppvOut = NULL;

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IOleWindow)
     || IsEqualIID(iid, &IID_IDeskBar) || IsEqualIID(iid, &IID_IMenuPopup))
    {
        *ppvOut = &This->vtbl;
    }
    else if (IsEqualIID(iid, &IID_IObjectWithSite))
    {
        *ppvOut = &This->objectSiteVtbl;
    }
    else if (IsEqualIID(iid, &IID_IInitializeObject))
    {
        *ppvOut = &This->initObjectVtbl;
    }

    if (*ppvOut)
    {
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("unsupported interface: %s\n", debugstr_guid(iid));
    return E_NOINTERFACE;
}

static ULONG WINAPI StartMenu_AddRef(IMenuPopup *iface)
{
    StartMenu *This = impl_from_IMenuPopup(iface);
    TRACE("(%p)\n", iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI StartMenu_Release(IMenuPopup *iface)
{
    StartMenu *This = impl_from_IMenuPopup(iface);
    ULONG ret;

    TRACE("(%p)\n", iface);

    ret = InterlockedDecrement(&This->refCount);
    if (ret == 0)
        StartMenu_Destructor(This);
    return ret;
}

static HRESULT WINAPI StartMenu_GetWindow(IMenuPopup *iface, HWND *phwnd)
{
    FIXME("(%p, %p)\n", iface, phwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_ContextSensitiveHelp(IMenuPopup *iface, BOOL fEnterMode)
{
    FIXME("(%p, %d)\n", iface, fEnterMode);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_SetClient(IMenuPopup *iface, IUnknown *punkClient)
{
    FIXME("(%p, %p)\n", iface, punkClient);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_GetClient(IMenuPopup *iface, IUnknown **ppunkClient)
{
    FIXME("(%p, %p)\n", iface, ppunkClient);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_OnPosRectChangeDB(IMenuPopup *iface, LPRECT prc)
{
    FIXME("(%p, %p)\n", iface, prc);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_Popup(IMenuPopup *iface, POINTL *ppt, RECTL *prcExclude, MP_POPUPFLAGS dwFlags)
{
    FIXME("(%p, %p, %p, %x)\n", iface, ppt, prcExclude, dwFlags);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_OnSelect(IMenuPopup *iface, DWORD dwSelectType)
{
    FIXME("(%p, %u)\n", iface, dwSelectType);
    return E_NOTIMPL;
}

static HRESULT WINAPI StartMenu_SetSubMenu(IMenuPopup *iface, IMenuPopup *pmp, BOOL fSet)
{
    FIXME("(%p, %p, %d)\n", iface, pmp, fSet);
    return E_NOTIMPL;
}

static const IMenuPopupVtbl StartMenuVtbl =
{
    StartMenu_QueryInterface,
    StartMenu_AddRef,
    StartMenu_Release,

    StartMenu_GetWindow,
    StartMenu_ContextSensitiveHelp,

    StartMenu_SetClient,
    StartMenu_GetClient,
    StartMenu_OnPosRectChangeDB,

    StartMenu_Popup,
    StartMenu_OnSelect,
    StartMenu_SetSubMenu,
};

static HRESULT WINAPI StartMenu_IObjectWithSite_QueryInterface(IObjectWithSite *iface, REFIID iid, LPVOID *ppvOut)
{
    StartMenu *This = impl_from_IObjectWithSite(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return StartMenu_QueryInterface((IMenuPopup *)This, iid, ppvOut);
}

static ULONG WINAPI StartMenu_IObjectWithSite_AddRef(IObjectWithSite *iface)
{
    StartMenu *This = impl_from_IObjectWithSite(iface);
    TRACE("(%p)\n", iface);
    return StartMenu_AddRef((IMenuPopup *)This);
}

static ULONG WINAPI StartMenu_IObjectWithSite_Release(IObjectWithSite *iface)
{
    StartMenu *This = impl_from_IObjectWithSite(iface);
    TRACE("(%p)\n", iface);
    return StartMenu_Release((IMenuPopup *)This);
}

static HRESULT WINAPI StartMenu_IObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    StartMenu *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p, %p)\n", iface, pUnkSite);

    if (This->pUnkSite)
        IUnknown_Release(This->pUnkSite);
    This->pUnkSite = pUnkSite;
    if (This->pUnkSite)
        IUnknown_AddRef(This->pUnkSite);
    return S_OK;
}

static HRESULT WINAPI StartMenu_IObjectWithSite_GetSite(IObjectWithSite *iface, REFIID riid, void **ppvSite)
{
    StartMenu *This = impl_from_IObjectWithSite(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppvSite);

    if (!This->pUnkSite)
        return E_FAIL;

    return IUnknown_QueryInterface(This->pUnkSite, riid, ppvSite);
}

static const IObjectWithSiteVtbl StartMenu_ObjectWithSiteVtbl =
{
    StartMenu_IObjectWithSite_QueryInterface,
    StartMenu_IObjectWithSite_AddRef,
    StartMenu_IObjectWithSite_Release,

    StartMenu_IObjectWithSite_SetSite,
    StartMenu_IObjectWithSite_GetSite,
};

static HRESULT WINAPI StartMenu_IInitializeObject_QueryInterface(IInitializeObject *iface, REFIID iid, LPVOID *ppvOut)
{
    StartMenu *This = impl_from_IInitializeObject(iface);
    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return StartMenu_QueryInterface((IMenuPopup *)This, iid, ppvOut);
}

static ULONG WINAPI StartMenu_IInitializeObject_AddRef(IInitializeObject *iface)
{
    StartMenu *This = impl_from_IInitializeObject(iface);
    TRACE("(%p)\n", iface);
    return StartMenu_AddRef((IMenuPopup *)This);
}

static ULONG WINAPI StartMenu_IInitializeObject_Release(IInitializeObject *iface)
{
    StartMenu *This = impl_from_IInitializeObject(iface);
    TRACE("(%p)\n", iface);
    return StartMenu_Release((IMenuPopup *)This);
}

static HRESULT WINAPI StartMenu_IInitializeObject_Initialize(IInitializeObject *iface)
{
    FIXME("Stub\n");
    return S_OK;
}

static const IInitializeObjectVtbl StartMenu_InitializeObjectVtbl =
{
    StartMenu_IInitializeObject_QueryInterface,
    StartMenu_IInitializeObject_AddRef,
    StartMenu_IInitializeObject_Release,

    StartMenu_IInitializeObject_Initialize,
};
