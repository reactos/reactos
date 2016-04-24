/*
 * Implementation of scripting for Microsoft Installer (msi.dll)
 *
 * Copyright 2007 Misha Koshelev
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

#include "msipriv.h"

#include <activscp.h>

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#ifdef _WIN64

#define IActiveScriptParse_Release IActiveScriptParse64_Release
#define IActiveScriptParse_InitNew IActiveScriptParse64_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse64_ParseScriptText

#else

#define IActiveScriptParse_Release IActiveScriptParse32_Release
#define IActiveScriptParse_InitNew IActiveScriptParse32_InitNew
#define IActiveScriptParse_ParseScriptText IActiveScriptParse32_ParseScriptText

#endif

static const WCHAR szJScript[] = { 'J','S','c','r','i','p','t',0};
static const WCHAR szVBScript[] = { 'V','B','S','c','r','i','p','t',0};
static const WCHAR szSession[] = {'S','e','s','s','i','o','n',0};

/*
 * MsiActiveScriptSite - Our IActiveScriptSite implementation.
 */
typedef struct {
    IActiveScriptSite IActiveScriptSite_iface;
    IDispatch *installer;
    IDispatch *session;
    LONG ref;
} MsiActiveScriptSite;

static inline MsiActiveScriptSite *impl_from_IActiveScriptSite( IActiveScriptSite *iface )
{
    return CONTAINING_RECORD(iface, MsiActiveScriptSite, IActiveScriptSite_iface);
}

/*
 * MsiActiveScriptSite
 */
static HRESULT WINAPI MsiActiveScriptSite_QueryInterface(IActiveScriptSite* iface, REFIID riid, void** obj)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IActiveScriptSite))
    {
        IActiveScriptSite_AddRef(iface);
        *obj = iface;
        return S_OK;
    }

    *obj = NULL;

    return E_NOINTERFACE;
}

static ULONG WINAPI MsiActiveScriptSite_AddRef(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p)->(%d)\n", This, ref);
    return ref;
}

static ULONG WINAPI MsiActiveScriptSite_Release(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p)->(%d)\n", This, ref);

    if (!ref)
        msi_free(This);

    return ref;
}

static HRESULT WINAPI MsiActiveScriptSite_GetLCID(IActiveScriptSite* iface, LCID* plcid)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)->(%p)\n", This, plcid);
    return E_NOTIMPL;  /* Script will use system-defined locale */
}

static HRESULT WINAPI MsiActiveScriptSite_GetItemInfo(IActiveScriptSite* iface, LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);

    TRACE("(%p)->(%p, %d, %p, %p)\n", This, pstrName, dwReturnMask, ppiunkItem, ppti);

    /* Determine the kind of pointer that is requested, and make sure placeholder is valid */
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
        if (!ppti) return E_INVALIDARG;
        *ppti = NULL;
    }
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
        if (!ppiunkItem) return E_INVALIDARG;
        *ppiunkItem = NULL;
    }

    /* Are we looking for the session object? */
    if (!strcmpW(szSession, pstrName)) {
        if (dwReturnMask & SCRIPTINFO_ITYPEINFO) {
            HRESULT hr = get_typeinfo(Session_tid, ppti);
            if (SUCCEEDED(hr))
                ITypeInfo_AddRef(*ppti);
            return hr;
        }
        else if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
            IDispatch_QueryInterface(This->session, &IID_IUnknown, (void **)ppiunkItem);
            return S_OK;
        }
    }

    return TYPE_E_ELEMENTNOTFOUND;
}

static HRESULT WINAPI MsiActiveScriptSite_GetDocVersionString(IActiveScriptSite* iface, BSTR* pbstrVersion)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)->(%p)\n", This, pbstrVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI MsiActiveScriptSite_OnScriptTerminate(IActiveScriptSite* iface, const VARIANT* pvarResult, const EXCEPINFO* pexcepinfo)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)->(%p, %p)\n", This, pvarResult, pexcepinfo);
    return S_OK;
}

static HRESULT WINAPI MsiActiveScriptSite_OnStateChange(IActiveScriptSite* iface, SCRIPTSTATE ssScriptState)
{
    switch (ssScriptState) {
        case SCRIPTSTATE_UNINITIALIZED:
              TRACE("State: Uninitialized.\n");
              break;

        case SCRIPTSTATE_INITIALIZED:
              TRACE("State: Initialized.\n");
              break;

        case SCRIPTSTATE_STARTED:
              TRACE("State: Started.\n");
              break;

        case SCRIPTSTATE_CONNECTED:
              TRACE("State: Connected.\n");
              break;

        case SCRIPTSTATE_DISCONNECTED:
              TRACE("State: Disconnected.\n");
              break;

        case SCRIPTSTATE_CLOSED:
              TRACE("State: Closed.\n");
              break;

        default:
              ERR("Unknown State: %d\n", ssScriptState);
              break;
    }

    return S_OK;
}

static HRESULT WINAPI MsiActiveScriptSite_OnScriptError(IActiveScriptSite* iface, IActiveScriptError* pscripterror)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    EXCEPINFO exception;
    HRESULT hr;

    TRACE("(%p)->(%p)\n", This, pscripterror);

    memset(&exception, 0, sizeof(EXCEPINFO));
    hr = IActiveScriptError_GetExceptionInfo(pscripterror, &exception);
    if (SUCCEEDED(hr))
    {
        ERR("script error: %s\n", debugstr_w(exception.bstrDescription));
        SysFreeString(exception.bstrSource);
        SysFreeString(exception.bstrDescription);
        SysFreeString(exception.bstrHelpFile);
    }

    return S_OK;
}

static HRESULT WINAPI MsiActiveScriptSite_OnEnterScript(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static HRESULT WINAPI MsiActiveScriptSite_OnLeaveScript(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = impl_from_IActiveScriptSite(iface);
    TRACE("(%p)\n", This);
    return S_OK;
}

static const struct IActiveScriptSiteVtbl activescriptsitevtbl =
{
    MsiActiveScriptSite_QueryInterface,
    MsiActiveScriptSite_AddRef,
    MsiActiveScriptSite_Release,
    MsiActiveScriptSite_GetLCID,
    MsiActiveScriptSite_GetItemInfo,
    MsiActiveScriptSite_GetDocVersionString,
    MsiActiveScriptSite_OnScriptTerminate,
    MsiActiveScriptSite_OnStateChange,
    MsiActiveScriptSite_OnScriptError,
    MsiActiveScriptSite_OnEnterScript,
    MsiActiveScriptSite_OnLeaveScript
};

static HRESULT create_activescriptsite(MsiActiveScriptSite **obj)
{
    MsiActiveScriptSite* object;

    TRACE("(%p)\n", obj);

    *obj = NULL;

    object = msi_alloc( sizeof(MsiActiveScriptSite) );
    if (!object)
        return E_OUTOFMEMORY;

    object->IActiveScriptSite_iface.lpVtbl = &activescriptsitevtbl;
    object->ref = 1;
    object->installer = NULL;
    object->session = NULL;

    *obj = object;

    return S_OK;
}

static UINT map_return_value(LONG val)
{
    switch (val)
    {
    case 0:
    case IDOK:
    case IDIGNORE:  return ERROR_SUCCESS;
    case IDCANCEL:  return ERROR_INSTALL_USEREXIT;
    case IDRETRY:   return ERROR_INSTALL_SUSPEND;
    case IDABORT:
    default:        return ERROR_INSTALL_FAILURE;
    }
}

/*
 * Call a script.
 */
DWORD call_script(MSIHANDLE hPackage, INT type, LPCWSTR script, LPCWSTR function, LPCWSTR action)
{
    HRESULT hr;
    IActiveScript *pActiveScript = NULL;
    IActiveScriptParse *pActiveScriptParse = NULL;
    MsiActiveScriptSite *scriptsite;
    IDispatch *pDispatch = NULL;
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    DISPID dispid;
    CLSID clsid;
    VARIANT var;
    DWORD ret = ERROR_INSTALL_FAILURE;

    CoInitialize(NULL);

    /* Create MsiActiveScriptSite object */
    hr = create_activescriptsite(&scriptsite);
    if (hr != S_OK) goto done;

    /* Create an installer object */
    hr = create_msiserver(NULL, (void**)&scriptsite->installer);
    if (hr != S_OK) goto done;

    /* Create a session object */
    hr = create_session(hPackage, scriptsite->installer, &scriptsite->session);
    if (hr != S_OK) goto done;

    /* Create the scripting engine */
    type &= msidbCustomActionTypeJScript|msidbCustomActionTypeVBScript;
    if (type == msidbCustomActionTypeJScript)
        hr = CLSIDFromProgID(szJScript, &clsid);
    else if (type == msidbCustomActionTypeVBScript)
        hr = CLSIDFromProgID(szVBScript, &clsid);
    else {
        ERR("Unknown script type %d\n", type);
        goto done;
    }
    if (FAILED(hr)) {
        ERR("Could not find CLSID for Windows Script\n");
        goto done;
    }
    hr = CoCreateInstance(&clsid, NULL, CLSCTX_INPROC_SERVER, &IID_IActiveScript, (void **)&pActiveScript);
    if (FAILED(hr)) {
        ERR("Could not instantiate class for Windows Script\n");
        goto done;
    }

    hr = IActiveScript_QueryInterface(pActiveScript, &IID_IActiveScriptParse, (void **)&pActiveScriptParse);
    if (FAILED(hr)) goto done;

    hr = IActiveScript_SetScriptSite(pActiveScript, &scriptsite->IActiveScriptSite_iface);
    if (FAILED(hr)) goto done;

    hr = IActiveScriptParse_InitNew(pActiveScriptParse);
    if (FAILED(hr)) goto done;

    hr = IActiveScript_AddNamedItem(pActiveScript, szSession, SCRIPTITEM_GLOBALMEMBERS|SCRIPTITEM_ISVISIBLE);
    if (FAILED(hr)) goto done;

    hr = IActiveScriptParse_ParseScriptText(pActiveScriptParse, script, NULL, NULL, NULL, 0, 0, 0L, NULL, NULL);
    if (FAILED(hr)) goto done;

    hr = IActiveScript_SetScriptState(pActiveScript, SCRIPTSTATE_CONNECTED);
    if (FAILED(hr)) goto done;

    /* Call a function if necessary through the IDispatch interface */
    if (function && function[0]) {
        TRACE("Calling function %s\n", debugstr_w(function));

        hr = IActiveScript_GetScriptDispatch(pActiveScript, NULL, &pDispatch);
        if (FAILED(hr)) goto done;

        hr = IDispatch_GetIDsOfNames(pDispatch, &IID_NULL, (WCHAR **)&function, 1,LOCALE_USER_DEFAULT, &dispid);
        if (FAILED(hr)) goto done;

        hr = IDispatch_Invoke(pDispatch, dispid, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, &var, NULL, NULL);
        if (FAILED(hr)) goto done;

        hr = VariantChangeType(&var, &var, 0, VT_I4);
        if (FAILED(hr)) goto done;

        ret = map_return_value(V_I4(&var));

        VariantClear(&var);
    } else {
        /* If no function to be called, MSI behavior is to succeed */
        ret = ERROR_SUCCESS;
    }

done:

    if (pDispatch) IDispatch_Release(pDispatch);
    if (pActiveScript) IActiveScript_Release(pActiveScript);
    if (pActiveScriptParse) IActiveScriptParse_Release(pActiveScriptParse);
    if (scriptsite)
    {
        if (scriptsite->session) IDispatch_Release(scriptsite->session);
        if (scriptsite->installer) IDispatch_Release(scriptsite->installer);
        IActiveScriptSite_Release(&scriptsite->IActiveScriptSite_iface);
    }
    CoUninitialize();    /* must call even if CoInitialize failed */
    return ret;
}
