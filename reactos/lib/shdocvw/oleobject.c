/*
 * Implementation of IOleObject interfaces for IE Web Browser
 *
 * - IOleObject
 * - IOleInPlaceObject
 * - IOleControl
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

#include <string.h>
#include "wine/debug.h"
#include "shdocvw.h"
#include "ole2.h"

WINE_DEFAULT_DEBUG_CHANNEL(shdocvw);

/**********************************************************************
 * Implement the IOleObject interface for the web browser component
 *
 * Based on DefaultHandler code in dlls/ole32/defaulthandler.c.
 */

static ULONG WINAPI WBOOBJ_AddRef(LPOLEOBJECT iface);
static ULONG WINAPI WBOOBJ_Release(LPOLEOBJECT iface);

/************************************************************************
 * WBOOBJ_QueryInterface (IUnknown)
 *
 * Interfaces we need to (at least pretend to) retrieve:
 *
 *   a6bc3ac0-dbaa-11ce-9de3-00aa004bb851  IID_IProvideClassInfo2
 *   b196b283-bab4-101a-b69c-00aa00341d07  IID_IProvideClassInfo
 *   cf51ed10-62fe-11cf-bf86-00a0c9034836  IID_IQuickActivate
 *   7fd52380-4e07-101b-ae2d-08002b2ec713  IID_IPersistStreamInit
 *   0000010a-0000-0000-c000-000000000046  IID_IPersistStorage
 *   b196b284-bab4-101a-b69c-00aa00341d07  IID_IConnectionPointContainer
 */
static HRESULT WINAPI WBOOBJ_QueryInterface(LPOLEOBJECT iface,
                                            REFIID riid, void** ppobj)
{
    IOleObjectImpl *This = (IOleObjectImpl *)iface;

    /*
     * Perform a sanity check on the parameters.
     */
    if ((This == NULL) || (ppobj == NULL) )
        return E_INVALIDARG;

    if (IsEqualGUID (&IID_IPersistStorage, riid))
    {
        TRACE("Returning IID_IPersistStorage interface\n");
        *ppobj = (LPVOID)&SHDOCVW_PersistStorage;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IPersistStreamInit, riid))
    {
        TRACE("Returning IID_IPersistStreamInit interface\n");
        *ppobj = (LPVOID)&SHDOCVW_PersistStreamInit;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IProvideClassInfo, riid))
    {
        TRACE("Returning IID_IProvideClassInfo interface\n");
        *ppobj = (LPVOID)&SHDOCVW_ProvideClassInfo;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IProvideClassInfo2, riid))
    {
        TRACE("Returning IID_IProvideClassInfo2 interface %p\n",
              &SHDOCVW_ProvideClassInfo2);
        *ppobj = (LPVOID)&SHDOCVW_ProvideClassInfo2;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IQuickActivate, riid))
    {
        TRACE("Returning IID_IQuickActivate interface\n");
        *ppobj = (LPVOID)&SHDOCVW_QuickActivate;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IConnectionPointContainer, riid))
    {
        TRACE("Returning IID_IConnectionPointContainer interface\n");
        *ppobj = (LPVOID)&SHDOCVW_ConnectionPointContainer;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IOleInPlaceObject, riid))
    {
        TRACE("Returning IID_IOleInPlaceObject interface\n");
        *ppobj = (LPVOID)&SHDOCVW_OleInPlaceObject;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IOleControl, riid))
    {
        TRACE("Returning IID_IOleControl interface\n");
        *ppobj = (LPVOID)&SHDOCVW_OleControl;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IWebBrowser, riid))
    {
        TRACE("Returning IID_IWebBrowser interface\n");
        *ppobj = (LPVOID)&SHDOCVW_WebBrowser;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }
    else if (IsEqualGUID (&IID_IDispatch, riid))
    {
        TRACE("Returning IID_IDispatch interface\n");
        *ppobj = (LPVOID)&SHDOCVW_WebBrowser;
        WBOOBJ_AddRef (iface);
        return S_OK;
    }

    TRACE ("Failed to find iid = %s\n", debugstr_guid(riid));

    return E_NOINTERFACE;
}

/************************************************************************
 * WBOOBJ_AddRef (IUnknown)
 */
static ULONG WINAPI WBOOBJ_AddRef(LPOLEOBJECT iface)
{
    IOleObjectImpl *This = (IOleObjectImpl *)iface;

    TRACE("\n");
    return ++(This->ref);
}

/************************************************************************
 * WBOOBJ_Release (IUnknown)
 */
static ULONG WINAPI WBOOBJ_Release(LPOLEOBJECT iface)
{
    IOleObjectImpl *This = (IOleObjectImpl *)iface;

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/************************************************************************
 * WBOOBJ_SetClientSite (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_SetClientSite(LPOLEOBJECT iface,
                                           LPOLECLIENTSITE pClientSite)
{
    FIXME("stub: (%p, %p)\n", iface, pClientSite);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_GetClientSite (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_GetClientSite(LPOLEOBJECT iface,
                                           LPOLECLIENTSITE* ppClientSite)
{
    FIXME("stub: (%p)\n", *ppClientSite);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_SetHostNames (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_SetHostNames(LPOLEOBJECT iface, LPCOLESTR szContainerApp,
                                          LPCOLESTR szContainerObj)
{
    FIXME("stub: (%p, %s, %s)\n", iface, debugstr_w(szContainerApp),
          debugstr_w(szContainerObj));
    return S_OK;
}

/************************************************************************
 * WBOOBJ_Close (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Close(LPOLEOBJECT iface, DWORD dwSaveOption)
{
    FIXME("stub: ()\n");
    return S_OK;
}

/************************************************************************
 * WBOOBJ_SetMoniker (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_SetMoniker(LPOLEOBJECT iface,
                                        DWORD dwWhichMoniker, IMoniker* pmk)
{
    FIXME("stub: (%p, %ld, %p)\n", iface, dwWhichMoniker, pmk);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_GetMoniker (IOleObject)
 *
 * Delegate this request to the client site if we have one.
 */
static HRESULT WINAPI WBOOBJ_GetMoniker(LPOLEOBJECT iface, DWORD dwAssign,
                                        DWORD dwWhichMoniker, LPMONIKER *ppmk)
{
    FIXME("stub (%p, %ld, %ld, %p)\n", iface, dwAssign, dwWhichMoniker, ppmk);
    return E_FAIL;
}

/************************************************************************
 * WBOOBJ_InitFromData (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_InitFromData(LPOLEOBJECT iface, LPDATAOBJECT pDataObject,
                                          BOOL fCreation, DWORD dwReserved)
{
    FIXME("stub: (%p, %p, %d, %ld)\n", iface, pDataObject, fCreation, dwReserved);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_GetClipboardData (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_GetClipboardData(LPOLEOBJECT iface, DWORD dwReserved,
                                              LPDATAOBJECT *ppDataObject)
{
    FIXME("stub: (%p, %ld, %p)\n", iface, dwReserved, ppDataObject);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_DoVerb (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_DoVerb(LPOLEOBJECT iface, LONG iVerb, struct tagMSG* lpmsg,
                                    LPOLECLIENTSITE pActiveSite, LONG lindex,
                                    HWND hwndParent, LPCRECT lprcPosRect)
{
    FIXME(": stub iVerb = %ld\n", iVerb);
    switch (iVerb)
    {
    case OLEIVERB_INPLACEACTIVATE:
        FIXME ("stub for OLEIVERB_INPLACEACTIVATE\n");
        break;
    case OLEIVERB_HIDE:
        FIXME ("stub for OLEIVERB_HIDE\n");
        break;
    }

    return S_OK;
}

/************************************************************************
 * WBOOBJ_EnumVerbs (IOleObject)
 *
 * Delegate to OleRegEnumVerbs.
 */
static HRESULT WINAPI WBOOBJ_EnumVerbs(LPOLEOBJECT iface,
                                       IEnumOLEVERB** ppEnumOleVerb)
{
    TRACE("(%p, %p)\n", iface, ppEnumOleVerb);

    return OleRegEnumVerbs(&CLSID_WebBrowser, ppEnumOleVerb);
}

/************************************************************************
 * WBOOBJ_EnumVerbs (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Update(LPOLEOBJECT iface)
{
    FIXME(": Stub\n");
    return E_NOTIMPL;
}

/************************************************************************
 * WBOOBJ_IsUpToDate (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_IsUpToDate(LPOLEOBJECT iface)
{
    FIXME("(%p)\n", iface);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_GetUserClassID (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_GetUserClassID(LPOLEOBJECT iface, CLSID* pClsid)
{
    FIXME("stub: (%p, %p)\n", iface, pClsid);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_GetUserType (IOleObject)
 *
 * Delegate to OleRegGetUserType.
 */
static HRESULT WINAPI WBOOBJ_GetUserType(LPOLEOBJECT iface, DWORD dwFormOfType,
                                         LPOLESTR* pszUserType)
{
    TRACE("(%p, %ld, %p)\n", iface, dwFormOfType, pszUserType);

    return OleRegGetUserType(&CLSID_WebBrowser, dwFormOfType, pszUserType);
}

/************************************************************************
 * WBOOBJ_SetExtent (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_SetExtent(LPOLEOBJECT iface, DWORD dwDrawAspect,
                                       SIZEL* psizel)
{
    FIXME("stub: (%p, %lx, (%ld x %ld))\n", iface, dwDrawAspect,
          psizel->cx, psizel->cy);
    return OLE_E_NOTRUNNING;
}

/************************************************************************
 * WBOOBJ_GetExtent (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_GetExtent(LPOLEOBJECT iface, DWORD dwDrawAspect,
                                       SIZEL* psizel)
{
    FIXME("stub: (%p, %lx, %p)\n", iface, dwDrawAspect, psizel);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_Advise (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Advise(LPOLEOBJECT iface, IAdviseSink* pAdvSink,
                                    DWORD* pdwConnection)
{
    FIXME("stub: (%p, %p, %p)\n", iface, pAdvSink, pdwConnection);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_Unadvise (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_Unadvise(LPOLEOBJECT iface, DWORD dwConnection)
{
    FIXME("stub: (%p, %ld)\n", iface, dwConnection);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_EnumAdvise (IOleObject)
 */
static HRESULT WINAPI WBOOBJ_EnumAdvise(LPOLEOBJECT iface, IEnumSTATDATA** ppenumAdvise)
{
    FIXME("stub: (%p, %p)\n", iface, ppenumAdvise);
    return S_OK;
}

/************************************************************************
 * WBOOBJ_GetMiscStatus (IOleObject)
 *
 * Delegate to OleRegGetMiscStatus.
 */
static HRESULT WINAPI WBOOBJ_GetMiscStatus(LPOLEOBJECT iface, DWORD dwAspect,
                                           DWORD* pdwStatus)
{
    HRESULT hres;

    TRACE("(%p, %lx, %p)\n", iface, dwAspect, pdwStatus);

    hres = OleRegGetMiscStatus(&CLSID_WebBrowser, dwAspect, pdwStatus);

    if (FAILED(hres))
        *pdwStatus = 0;

    return S_OK;
}

/************************************************************************
 * WBOOBJ_SetColorScheme (IOleObject)
 *
 * This method is meaningless if the server is not running
 */
static HRESULT WINAPI WBOOBJ_SetColorScheme(LPOLEOBJECT iface,
                                            struct tagLOGPALETTE* pLogpal)
{
    FIXME("stub: (%p, %p))\n", iface, pLogpal);
    return OLE_E_NOTRUNNING;
}

/**********************************************************************
 * IOleObject virtual function table for IE Web Browser component
 */

static IOleObjectVtbl WBOOBJ_Vtbl =
{
    WBOOBJ_QueryInterface,
    WBOOBJ_AddRef,
    WBOOBJ_Release,
    WBOOBJ_SetClientSite,
    WBOOBJ_GetClientSite,
    WBOOBJ_SetHostNames,
    WBOOBJ_Close,
    WBOOBJ_SetMoniker,
    WBOOBJ_GetMoniker,
    WBOOBJ_InitFromData,
    WBOOBJ_GetClipboardData,
    WBOOBJ_DoVerb,
    WBOOBJ_EnumVerbs,
    WBOOBJ_Update,
    WBOOBJ_IsUpToDate,
    WBOOBJ_GetUserClassID,
    WBOOBJ_GetUserType,
    WBOOBJ_SetExtent,
    WBOOBJ_GetExtent,
    WBOOBJ_Advise,
    WBOOBJ_Unadvise,
    WBOOBJ_EnumAdvise,
    WBOOBJ_GetMiscStatus,
    WBOOBJ_SetColorScheme
};

IOleObjectImpl SHDOCVW_OleObject = { &WBOOBJ_Vtbl, 1 };


/**********************************************************************
 * Implement the IOleInPlaceObject interface
 */

static HRESULT WINAPI WBOIPO_QueryInterface(LPOLEINPLACEOBJECT iface,
                                            REFIID riid, LPVOID *ppobj)
{
    IOleInPlaceObjectImpl *This = (IOleInPlaceObjectImpl *)iface;

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBOIPO_AddRef(LPOLEINPLACEOBJECT iface)
{
    IOleInPlaceObjectImpl *This = (IOleInPlaceObjectImpl *)iface;

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBOIPO_Release(LPOLEINPLACEOBJECT iface)
{
    IOleInPlaceObjectImpl *This = (IOleInPlaceObjectImpl *)iface;

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

static HRESULT WINAPI WBOIPO_GetWindow(LPOLEINPLACEOBJECT iface, HWND* phwnd)
{
#if 0
    /* Create a fake window to fool MFC into believing that we actually
     * have an implemented browser control.  Avoids the assertion.
     */
    HWND hwnd;
    hwnd = CreateWindowA("BUTTON", "Web Control",
                        WS_HSCROLL | WS_VSCROLL | WS_OVERLAPPEDWINDOW,
                        CW_USEDEFAULT, CW_USEDEFAULT, 600,
                        400, NULL, NULL, NULL, NULL);

    *phwnd = hwnd;
    TRACE ("Returning hwnd = %d\n", hwnd);
#endif

    FIXME("stub HWND* = %p\n", phwnd);
    return S_OK;
}

static HRESULT WINAPI WBOIPO_ContextSensitiveHelp(LPOLEINPLACEOBJECT iface,
                                                  BOOL fEnterMode)
{
    FIXME("stub fEnterMode = %d\n", fEnterMode);
    return S_OK;
}

static HRESULT WINAPI WBOIPO_InPlaceDeactivate(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WBOIPO_UIDeactivate(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WBOIPO_SetObjectRects(LPOLEINPLACEOBJECT iface,
                                            LPCRECT lprcPosRect, LPCRECT lprcClipRect)
{
    FIXME("stub PosRect = %p, ClipRect = %p\n", lprcPosRect, lprcClipRect);
    return S_OK;
}

static HRESULT WINAPI WBOIPO_ReactivateAndUndo(LPOLEINPLACEOBJECT iface)
{
    FIXME("stub \n");
    return S_OK;
}

/**********************************************************************
 * IOleInPlaceObject virtual function table for IE Web Browser component
 */

static IOleInPlaceObjectVtbl WBOIPO_Vtbl =
{
    WBOIPO_QueryInterface,
    WBOIPO_AddRef,
    WBOIPO_Release,
    WBOIPO_GetWindow,
    WBOIPO_ContextSensitiveHelp,
    WBOIPO_InPlaceDeactivate,
    WBOIPO_UIDeactivate,
    WBOIPO_SetObjectRects,
    WBOIPO_ReactivateAndUndo
};

IOleInPlaceObjectImpl SHDOCVW_OleInPlaceObject = { &WBOIPO_Vtbl, 1 };


/**********************************************************************
 * Implement the IOleControl interface
 */

static HRESULT WINAPI WBOC_QueryInterface(LPOLECONTROL iface,
                                          REFIID riid, LPVOID *ppobj)
{
    IOleControlImpl *This = (IOleControlImpl *)iface;

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WBOC_AddRef(LPOLECONTROL iface)
{
    IOleControlImpl *This = (IOleControlImpl *)iface;

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WBOC_Release(LPOLECONTROL iface)
{
    IOleControlImpl *This = (IOleControlImpl *)iface;

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

static HRESULT WINAPI WBOC_GetControlInfo(LPOLECONTROL iface, LPCONTROLINFO pCI)
{
    FIXME("stub: LPCONTROLINFO = %p\n", pCI);
    return S_OK;
}

static HRESULT WINAPI WBOC_OnMnemonic(LPOLECONTROL iface, struct tagMSG *pMsg)
{
    FIXME("stub: MSG* = %p\n", pMsg);
    return S_OK;
}

static HRESULT WINAPI WBOC_OnAmbientPropertyChange(LPOLECONTROL iface, DISPID dispID)
{
    FIXME("stub: DISPID = %ld\n", dispID);
    return S_OK;
}

static HRESULT WINAPI WBOC_FreezeEvents(LPOLECONTROL iface, BOOL bFreeze)
{
    FIXME("stub: bFreeze = %d\n", bFreeze);
    return S_OK;
}

/**********************************************************************
 * IOleControl virtual function table for IE Web Browser component
 */

static IOleControlVtbl WBOC_Vtbl =
{
    WBOC_QueryInterface,
    WBOC_AddRef,
    WBOC_Release,
    WBOC_GetControlInfo,
    WBOC_OnMnemonic,
    WBOC_OnAmbientPropertyChange,
    WBOC_FreezeEvents
};

IOleControlImpl SHDOCVW_OleControl = { &WBOC_Vtbl, 1 };
