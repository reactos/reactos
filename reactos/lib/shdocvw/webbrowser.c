/*
 * Implementation of IWebBrowser interface for IE Web Browser control
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
 * Implement the IWebBrowser interface
 */

static HRESULT WINAPI WB_QueryInterface(IWebBrowser *iface, REFIID riid, LPVOID *ppobj)
{
    IWebBrowserImpl *This = (IWebBrowserImpl *)iface;

    FIXME("(%p)->(%s,%p),stub!\n", This, debugstr_guid(riid), ppobj);
    return E_NOINTERFACE;
}

static ULONG WINAPI WB_AddRef(IWebBrowser *iface)
{
    IWebBrowserImpl *This = (IWebBrowserImpl *)iface;

    TRACE("\n");
    return ++(This->ref);
}

static ULONG WINAPI WB_Release(IWebBrowser *iface)
{
    IWebBrowserImpl *This = (IWebBrowserImpl *)iface;

    /* static class, won't be freed */
    TRACE("\n");
    return --(This->ref);
}

/* IDispatch methods */
static HRESULT WINAPI WB_GetTypeInfoCount(IWebBrowser *iface, UINT *pctinfo)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GetTypeInfo(IWebBrowser *iface, UINT iTInfo, LCID lcid,
                                     LPTYPEINFO *ppTInfo)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GetIDsOfNames(IWebBrowser *iface, REFIID riid,
                                       LPOLESTR *rgszNames, UINT cNames,
                                       LCID lcid, DISPID *rgDispId)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_Invoke(IWebBrowser *iface, DISPID dispIdMember,
                                REFIID riid, LCID lcid, WORD wFlags,
                                DISPPARAMS *pDispParams, VARIANT *pVarResult,
                                EXCEPINFO *pExepInfo, UINT *puArgErr)
{
    FIXME("stub dispIdMember = %d, IID = %s\n", (int)dispIdMember, debugstr_guid(riid));
    return S_OK;
}

/* IWebBrowser methods */
static HRESULT WINAPI WB_GoBack(IWebBrowser *iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GoForward(IWebBrowser *iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GoHome(IWebBrowser *iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_GoSearch(IWebBrowser *iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_Navigate(IWebBrowser *iface, BSTR URL,
                                  VARIANT *Flags, VARIANT *TargetFrameName,
                                  VARIANT *PostData, VARIANT *Headers)
{
    FIXME("stub: URL = %p (%p, %p, %p, %p)\n", URL, Flags, TargetFrameName,
          PostData, Headers);
    return S_OK;
}

static HRESULT WINAPI WB_Refresh(IWebBrowser *iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_Refresh2(IWebBrowser *iface, VARIANT *Level)
{
    FIXME("stub: %p\n", Level);
    return S_OK;
}

static HRESULT WINAPI WB_Stop(IWebBrowser *iface)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Application(IWebBrowser *iface, IDispatch **ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Parent(IWebBrowser *iface, IDispatch **ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Container(IWebBrowser *iface, IDispatch **ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Document(IWebBrowser *iface, IDispatch **ppDisp)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_TopLevelContainer(IWebBrowser *iface, VARIANT_BOOL *pBool)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Type(IWebBrowser *iface, BSTR *Type)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Left(IWebBrowser *iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Left(IWebBrowser *iface, long Left)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Top(IWebBrowser *iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Top(IWebBrowser *iface, long Top)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Width(IWebBrowser *iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Width(IWebBrowser *iface, long Width)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Height(IWebBrowser *iface, long *pl)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_put_Height(IWebBrowser *iface, long Height)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_LocationName(IWebBrowser *iface, BSTR *LocationName)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_LocationURL(IWebBrowser *iface, BSTR *LocationURL)
{
    FIXME("stub \n");
    return S_OK;
}

static HRESULT WINAPI WB_get_Busy(IWebBrowser *iface, VARIANT_BOOL *pBool)
{
    FIXME("stub \n");
    return S_OK;
}

/**********************************************************************
 * IWebBrowser virtual function table for IE Web Browser component
 */

static IWebBrowserVtbl WB_Vtbl =
{
    WB_QueryInterface,
    WB_AddRef,
    WB_Release,
    WB_GetTypeInfoCount,
    WB_GetTypeInfo,
    WB_GetIDsOfNames,
    WB_Invoke,
    WB_GoBack,
    WB_GoForward,
    WB_GoHome,
    WB_GoSearch,
    WB_Navigate,
    WB_Refresh,
    WB_Refresh2,
    WB_Stop,
    WB_get_Application,
    WB_get_Parent,
    WB_get_Container,
    WB_get_Document,
    WB_get_TopLevelContainer,
    WB_get_Type,
    WB_get_Left,
    WB_put_Left,
    WB_get_Top,
    WB_put_Top,
    WB_get_Width,
    WB_put_Width,
    WB_get_Height,
    WB_put_Height,
    WB_get_LocationName,
    WB_get_LocationURL,
    WB_get_Busy
};

IWebBrowserImpl SHDOCVW_WebBrowser = { &WB_Vtbl, 1 };
