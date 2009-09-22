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

#include <precomp.h>

WINE_DEFAULT_DEBUG_CHANNEL(shell32start);

typedef struct _tagStartMenu {
    const IMenuPopupVtbl *vtbl;
    const IObjectWithSiteVtbl *objectSiteVtbl;
    const IInitializeObjectVtbl *initObjectVtbl;
    const IBandSiteVtbl *bandSiteVtbl;
    IUnknown *pUnkSite;
    LONG refCount;
} StartMenu, *LPStartMenu;

static const IMenuPopupVtbl StartMenuVtbl;
static const IObjectWithSiteVtbl StartMenu_ObjectWithSiteVtbl;
static const IInitializeObjectVtbl StartMenu_InitializeObjectVtbl;
static const IBandSiteVtbl StartMenu_BandSiteVtbl;

static LPStartMenu __inline impl_from_IMenuPopup(IMenuPopup *iface)
{
    return (LPStartMenu)((char *)iface - FIELD_OFFSET(StartMenu, vtbl));
}

static LPStartMenu __inline impl_from_IObjectWithSite(IObjectWithSite *iface)
{
    return (LPStartMenu)((char *)iface - FIELD_OFFSET(StartMenu, objectSiteVtbl));
}

static LPStartMenu __inline impl_from_IInitializeObject(IInitializeObject *iface)
{
    return (LPStartMenu)((char *)iface - FIELD_OFFSET(StartMenu, initObjectVtbl));
}

HRESULT WINAPI StartMenu_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    StartMenu *This;

    TRACE("StartMenu_Constructor(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    if (pUnkOuter)
        return E_POINTER;

    This = CoTaskMemAlloc(sizeof(StartMenu));
    if (!This)
        return E_OUTOFMEMORY;
    ZeroMemory(This, sizeof(*This));
    This->vtbl = &StartMenuVtbl;
    This->objectSiteVtbl = &StartMenu_ObjectWithSiteVtbl;
    This->initObjectVtbl = &StartMenu_InitializeObjectVtbl;
    This->bandSiteVtbl = &StartMenu_BandSiteVtbl;
    This->refCount = 1;

    TRACE("StartMenu_Constructor returning %p\n", This);
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

    TRACE("StartMenu_Constructor (%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);

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
    else if (IsEqualIID(iid, &IID_IBandSite))
    {
        *ppvOut = &This->bandSiteVtbl;
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
    TRACE("StartMenu_AddRef(%p)\n", iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI StartMenu_Release(IMenuPopup *iface)
{
    StartMenu *This = impl_from_IMenuPopup(iface);
    ULONG ret;

    TRACE("StartMenu_Release(%p)\n", iface);

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
    StartMenu * This = (StartMenu*)iface;
    TRACE("StartMenu_GetClient (%p, %p)\n", iface, ppunkClient);
    *ppunkClient = (IUnknown*)&This->bandSiteVtbl;
    IUnknown_AddRef(*ppunkClient);
    return S_OK;
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
    /* IUnknown */
    StartMenu_QueryInterface,
    StartMenu_AddRef,
    StartMenu_Release,

    /* IOleWindow */
    StartMenu_GetWindow,
    StartMenu_ContextSensitiveHelp,

    /* IDeskBar */
    StartMenu_SetClient,
    StartMenu_GetClient,
    StartMenu_OnPosRectChangeDB,

    /* IMenuPopup */
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
    TRACE("StartMenu_IObjectWithSite_Release (%p)\n", iface);
    return StartMenu_Release((IMenuPopup *)This);
}

static HRESULT WINAPI StartMenu_IObjectWithSite_SetSite(IObjectWithSite *iface, IUnknown *pUnkSite)
{
    StartMenu *This = impl_from_IObjectWithSite(iface);

    TRACE("StartMenu_IObjectWithSite_SetSite(%p, %p)\n", iface, pUnkSite);

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
    TRACE("StartMenu_IInitializeObject_AddRef(%p)\n", iface);
    return StartMenu_AddRef((IMenuPopup *)This);
}

static ULONG WINAPI StartMenu_IInitializeObject_Release(IInitializeObject *iface)
{
    StartMenu *This = impl_from_IInitializeObject(iface);
    TRACE("StartMenu_IInitializeObject_Release (%p)\n", iface);
    return StartMenu_Release((IMenuPopup *)This);
}

static HRESULT WINAPI StartMenu_IInitializeObject_Initialize(IInitializeObject *iface)
{
    FIXME("StartMenu_IInitializeObject_Initialize Stub\n");
    return S_OK;
}

static const IInitializeObjectVtbl StartMenu_InitializeObjectVtbl =
{
    StartMenu_IInitializeObject_QueryInterface,
    StartMenu_IInitializeObject_AddRef,
    StartMenu_IInitializeObject_Release,

    StartMenu_IInitializeObject_Initialize,
};

//---------------------------------------------------------------------------------------------------------
// IBandSite interface

static HRESULT WINAPI StartMenu_IBandSite_QueryInterface(IBandSite *iface, REFIID iid, LPVOID *ppvOut)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, bandSiteVtbl);
    TRACE("StartMenu_IBandSite_QueryInterface(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return StartMenu_QueryInterface((IMenuPopup *)This, iid, ppvOut);
}

static ULONG WINAPI StartMenu_IBandSite_AddRef(IBandSite *iface)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, bandSiteVtbl);
    TRACE("StartMenu_IBandSite_AddRef(%p)\n", iface);
    return StartMenu_AddRef((IMenuPopup *)This);
}

static ULONG WINAPI StartMenu_IBandSite_Release(IBandSite *iface)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, bandSiteVtbl);
    TRACE("StartMenu_IBandSite_Release (%p)\n", iface);
    return StartMenu_Release((IMenuPopup *)This);
}


static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_AddBand(IBandSite *iface, IUnknown *punk)
{
    FIXME("StartMenu_IBandSite_AddBand Stub punk %p\n", punk);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_EnumBands(IBandSite *iface, UINT uBand, DWORD *pdwBandID)
{
    FIXME("StartMenu_IBandSite_EnumBands Stub uBand %uu pdwBandID %p\n", uBand, pdwBandID);
    return E_FAIL;
}
static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_QueryBand(IBandSite *iface, DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    FIXME("StartMenu_IBandSite_QueryBand Stub dwBandID %u IDeskBand %p pdwState %p Name %p cchName %u\n", dwBandID, ppstb, pdwState, pszName, cchName);
    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_SetBandState(IBandSite *iface, DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    FIXME("StartMenu_IBandSite_SetBandState Stub dwBandID %u dwMask %x dwState %u\n", dwBandID, dwMask, dwState);
    return E_FAIL;
}
static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_RemoveBand(IBandSite *iface, DWORD dwBandID)
{
    FIXME("StartMenu_IBandSite_RemoveBand Stub dwBandID %p\n", dwBandID);
    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_GetBandObject(IBandSite *iface, DWORD dwBandID, REFIID riid, void **ppv)
{
    FIXME("StartMenu_IBandSite_GetBandObject Stub dwBandID %u riid %p ppv %p\n", dwBandID, riid, ppv);
    return E_FAIL;
}
static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_SetBandSiteInfo(IBandSite *iface, const BANDSITEINFO *pbsinfo)
{
    FIXME("StartMenu_IBandSite_SetBandSiteInfo Stub pbsinfo %p\n", pbsinfo);
    return E_FAIL;

}
static HRESULT STDMETHODCALLTYPE StartMenu_IBandSite_GetBandSiteInfo(IBandSite *iface, BANDSITEINFO *pbsinfo)
{
    FIXME("StartMenu_IBandSite_GetBandSiteInfo Stub pbsinfo %p\n", pbsinfo);
    return E_FAIL;
}

static const IBandSiteVtbl StartMenu_BandSiteVtbl =
{
    StartMenu_IBandSite_QueryInterface,
    StartMenu_IBandSite_AddRef,
    StartMenu_IBandSite_Release,
    StartMenu_IBandSite_AddBand,
    StartMenu_IBandSite_EnumBands,
    StartMenu_IBandSite_QueryBand,
    StartMenu_IBandSite_SetBandState,
    StartMenu_IBandSite_RemoveBand,
    StartMenu_IBandSite_GetBandObject,
    StartMenu_IBandSite_SetBandSiteInfo,
    StartMenu_IBandSite_GetBandSiteInfo
};

