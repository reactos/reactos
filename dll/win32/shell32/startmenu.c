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
    const IMenuBandVtbl *menuBandVtbl;
    IUnknown *pUnkSite;
    LONG refCount;
    IBandSite * pBandSite;
} StartMenu, *LPStartMenu;

typedef struct _tagMenuBandSite {
    const IBandSiteVtbl * lpVtbl;
    LONG refCount;

    IUnknown ** Objects;
    LONG ObjectsCount;

} MenuBandSite, *LPMenuBandSite;

static const IMenuPopupVtbl StartMenuVtbl;
static const IObjectWithSiteVtbl StartMenu_ObjectWithSiteVtbl;
static const IInitializeObjectVtbl StartMenu_InitializeObjectVtbl;
static const IBandSiteVtbl StartMenu_BandSiteVtbl;
static const IMenuBandVtbl StartMenu_MenuBandVtbl;

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
    This->menuBandVtbl = &StartMenu_MenuBandVtbl;
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

    TRACE("StartMenu_QueryInterface (%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);

    if (IsEqualIID(iid, &IID_IUnknown) || IsEqualIID(iid, &IID_IOleWindow)
     || IsEqualIID(iid, &IID_IDeskBar) || IsEqualIID(iid, &IID_IMenuPopup))
    {
        *ppvOut = (void *)&This->vtbl;
    }
    else if (IsEqualIID(iid, &IID_IObjectWithSite))
    {
        *ppvOut = (void *)&This->objectSiteVtbl;
    }
    else if (IsEqualIID(iid, &IID_IInitializeObject))
    {
        *ppvOut = (void *)&This->initObjectVtbl;
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

    *ppunkClient = (IUnknown*)This->pBandSite;
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
    HRESULT hr;
    StartMenu *This = impl_from_IInitializeObject(iface);
    TRACE("StartMenu_IInitializeObject_Initialize (%p)\n", iface);

    hr = MenuBandSite_Constructor(NULL, &IID_IBandSite, (LPVOID*)&This->pBandSite);
    if (FAILED(hr))
        return hr;

    return IBandSite_AddBand(This->pBandSite, (IUnknown*)&This->menuBandVtbl);
}

static const IInitializeObjectVtbl StartMenu_InitializeObjectVtbl =
{
    StartMenu_IInitializeObject_QueryInterface,
    StartMenu_IInitializeObject_AddRef,
    StartMenu_IInitializeObject_Release,

    StartMenu_IInitializeObject_Initialize,
};

//--------------------------------------------------------------
// IMenuBand interface


static HRESULT STDMETHODCALLTYPE StartMenu_IMenuBand_QueryInterface(IMenuBand *iface, REFIID iid, LPVOID *ppvOut)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, menuBandVtbl);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IMenuBand))
    {
        *ppvOut = &This->menuBandVtbl;
        IUnknown_AddRef((IUnknown*)*ppvOut);
        return S_OK;
    }

    WARN("unsupported interface:(%p, %s, %p)\n", iface, debugstr_guid(iid), ppvOut);
    return E_NOINTERFACE;
}

static ULONG STDMETHODCALLTYPE StartMenu_IMenuBand_AddRef(IMenuBand *iface)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, menuBandVtbl);
    TRACE("StartMenu_IInitializeObject_AddRef(%p)\n", This);
    return StartMenu_AddRef((IMenuPopup *)This);
}

static ULONG STDMETHODCALLTYPE StartMenu_IMenuBand_Release(IMenuBand *iface)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, menuBandVtbl);
    TRACE("StartMenu_IInitializeObject_Release (%p)\n", This);
    return StartMenu_Release((IMenuPopup *)This);
}

HRESULT STDMETHODCALLTYPE StartMenu_IMenuBand_IsMenuMessage(IMenuBand *iface, MSG *pmsg)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, menuBandVtbl);
    TRACE("StartMenu_IMenuBand_IsMenuMessage Stub(%p)\n", This);
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE StartMenu_IMenuBand_TranslateMenuMessage(IMenuBand *iface, MSG *pmsg, LRESULT *plRet)
{
    StartMenu *This = (StartMenu*)CONTAINING_RECORD(iface, StartMenu, menuBandVtbl);
    TRACE("StartMenu_IMenuBand_TranslateMenuMessage Stub(%p)\n", This);
    return E_NOTIMPL;
}


static const IMenuBandVtbl StartMenu_MenuBandVtbl =
{
    /* IUnknown methods */
    StartMenu_IMenuBand_QueryInterface,
    StartMenu_IMenuBand_AddRef,
    StartMenu_IMenuBand_Release,
    /* IMenuBand methods */
    StartMenu_IMenuBand_IsMenuMessage,
    StartMenu_IMenuBand_TranslateMenuMessage,
};


//---------------------------------------------------------------------------------------------------------
// IBandSite interface


HRESULT WINAPI MenuBandSite_Constructor(IUnknown *pUnkOuter, REFIID riid, LPVOID *ppv)
{
    MenuBandSite *This;
    HRESULT hr;

    TRACE("StartMenu_Constructor(%p, %s, %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    if (pUnkOuter)
        return E_POINTER;

    This = CoTaskMemAlloc(sizeof(MenuBandSite));
    if (!This)
        return E_OUTOFMEMORY;

    ZeroMemory(This, sizeof(MenuBandSite));
    This->lpVtbl = &StartMenu_BandSiteVtbl;

    hr = IUnknown_QueryInterface((IUnknown*)&This->lpVtbl, riid, ppv);

    if (FAILED(hr))
    {
        CoTaskMemFree(This);
        return hr;
    }

    TRACE("StartMenu_Constructor returning %p\n", This);
    *ppv = (IUnknown *)This;
    return S_OK;
}

static HRESULT WINAPI BandSite_QueryInterface(IBandSite *iface, REFIID iid, LPVOID *ppvOut)
{
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IBandSite))
    {
        *ppvOut = &This->lpVtbl;
        IUnknown_AddRef((IUnknown*)*ppvOut);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI BandSite_AddRef(IBandSite *iface)
{
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);
    TRACE("BandSite_AddRef(%p)\n", iface);
    return InterlockedIncrement(&This->refCount);
}

static ULONG WINAPI BandSite_Release(IBandSite *iface)
{
    LONG ret;
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);

    ret = InterlockedDecrement(&This->refCount);
    TRACE("BandSite_Release refCount %u\n", ret);

    if (ret == 0)
    {
        CoTaskMemFree(This->Objects);
        CoTaskMemFree(This);
    }

    return ret;
}


static HRESULT STDMETHODCALLTYPE BandSite_AddBand(IBandSite *iface, IUnknown *punk)
{
    IUnknown ** Objects;
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);

    TRACE("StartMenu_IBandSite_AddBand Stub punk %p\n", punk);

    if (!punk)
        return E_FAIL;

    Objects = (IUnknown**) CoTaskMemAlloc(sizeof(IUnknown*) * (This->ObjectsCount + 1));
    if (!Objects)
        return E_FAIL;

    RtlMoveMemory(Objects, This->Objects, sizeof(IUnknown*) * This->ObjectsCount);

    CoTaskMemFree(This->Objects);

    This->Objects = Objects;
    Objects[This->ObjectsCount] = punk;

    IUnknown_AddRef(punk);

    This->ObjectsCount++;


    return S_OK;
}

static HRESULT STDMETHODCALLTYPE BandSite_EnumBands(IBandSite *iface, UINT uBand, DWORD *pdwBandID)
{
    ULONG Index, ObjectCount;
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);

    TRACE("StartMenu_IBandSite_EnumBands Stub uBand %uu pdwBandID %p\n", uBand, pdwBandID);

    if (uBand == (UINT)-1)
        return This->ObjectsCount;

    ObjectCount = 0;

    for(Index = 0; Index < This->ObjectsCount; Index++)
    {
        if (This->Objects[Index] != NULL)
        {
            if (uBand == ObjectCount)
            {
                *pdwBandID = Index;
                return S_OK;
            }
            ObjectCount++;
        }
    }
    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE BandSite_QueryBand(IBandSite *iface, DWORD dwBandID, IDeskBand **ppstb, DWORD *pdwState, LPWSTR pszName, int cchName)
{
    FIXME("StartMenu_IBandSite_QueryBand Stub dwBandID %u IDeskBand %p pdwState %p Name %p cchName %u\n", dwBandID, ppstb, pdwState, pszName, cchName);
    return E_FAIL;
}

static HRESULT STDMETHODCALLTYPE BandSite_SetBandState(IBandSite *iface, DWORD dwBandID, DWORD dwMask, DWORD dwState)
{
    FIXME("StartMenu_IBandSite_SetBandState Stub dwBandID %u dwMask %x dwState %u\n", dwBandID, dwMask, dwState);
    return E_FAIL;
}
static HRESULT STDMETHODCALLTYPE BandSite_RemoveBand(IBandSite *iface, DWORD dwBandID)
{
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);
    TRACE("StartMenu_IBandSite_RemoveBand Stub dwBandID %u\n", dwBandID);

    if (This->ObjectsCount <= dwBandID)
        return E_FAIL;

    if (This->Objects[dwBandID])
    {
        This->Objects[dwBandID]->lpVtbl->Release(This->Objects[dwBandID]);
        This->Objects[dwBandID] = NULL;
    }

    return S_OK;
}

static HRESULT STDMETHODCALLTYPE BandSite_GetBandObject(IBandSite *iface, DWORD dwBandID, REFIID riid, void **ppv)
{
    MenuBandSite *This = (MenuBandSite*)CONTAINING_RECORD(iface, MenuBandSite, lpVtbl);

    TRACE("StartMenu_IBandSite_GetBandObject Stub dwBandID %u riid %p ppv %p\n", dwBandID, riid, ppv);

    if (This->ObjectsCount <= dwBandID)
        return E_FAIL;

    if (This->Objects[dwBandID])
    {
        return IUnknown_QueryInterface(This->Objects[dwBandID], riid, ppv);
    }

    return E_FAIL;
}
static HRESULT STDMETHODCALLTYPE BandSite_SetBandSiteInfo(IBandSite *iface, const BANDSITEINFO *pbsinfo)
{
    FIXME("StartMenu_IBandSite_SetBandSiteInfo Stub pbsinfo %p\n", pbsinfo);
    return E_FAIL;

}
static HRESULT STDMETHODCALLTYPE BandSite_GetBandSiteInfo(IBandSite *iface, BANDSITEINFO *pbsinfo)
{
    FIXME("StartMenu_IBandSite_GetBandSiteInfo Stub pbsinfo %p\n", pbsinfo);
    return E_FAIL;
}

static const IBandSiteVtbl StartMenu_BandSiteVtbl =
{
    BandSite_QueryInterface,
    BandSite_AddRef,
    BandSite_Release,
    BandSite_AddBand,
    BandSite_EnumBands,
    BandSite_QueryBand,
    BandSite_SetBandState,
    BandSite_RemoveBand,
    BandSite_GetBandObject,
    BandSite_SetBandSiteInfo,
    BandSite_GetBandSiteInfo
};

