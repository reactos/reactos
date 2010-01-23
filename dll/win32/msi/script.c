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

#define COBJMACROS

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "msidefs.h"
#include "msipriv.h"
#include "activscp.h"
#include "oleauto.h"
#include "wine/debug.h"
#include "wine/unicode.h"

#include "msiserver.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

static const WCHAR szJScript[] = { 'J','S','c','r','i','p','t',0};
static const WCHAR szVBScript[] = { 'V','B','S','c','r','i','p','t',0};
static const WCHAR szSession[] = {'S','e','s','s','i','o','n',0};

/*
 * MsiActiveScriptSite - Our IActiveScriptSite implementation.
 */

typedef struct {
    IActiveScriptSite lpVtbl;
    IDispatch *pInstaller;
    IDispatch *pSession;
    LONG ref;
} MsiActiveScriptSite;

static const struct IActiveScriptSiteVtbl ASS_Vtbl;

static HRESULT create_ActiveScriptSite(IUnknown *pUnkOuter, LPVOID *ppObj)
{
    MsiActiveScriptSite* object;

    TRACE("(%p,%p)\n", pUnkOuter, ppObj);

    if( pUnkOuter )
        return CLASS_E_NOAGGREGATION;

    object = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MsiActiveScriptSite));

    object->lpVtbl.lpVtbl = &ASS_Vtbl;
    object->ref = 1;
    object->pInstaller = NULL;
    object->pSession = NULL;

    *ppObj = object;

    return S_OK;
}

/*
 * Call a script.
 */
DWORD call_script(MSIHANDLE hPackage, INT type, LPCWSTR script, LPCWSTR function, LPCWSTR action)
{
    HRESULT hr;
    IActiveScript *pActiveScript = NULL;
    IActiveScriptParse *pActiveScriptParse = NULL;
    MsiActiveScriptSite *pActiveScriptSite = NULL;
    IDispatch *pDispatch = NULL;
    DISPPARAMS dispparamsNoArgs = {NULL, NULL, 0, 0};
    DISPID dispid;
    CLSID clsid;
    VARIANT var;

    /* Return success by default (if Windows Script not installed) - not native behavior. This
     * should be here until we implement wine scripting. */
    DWORD ret = ERROR_SUCCESS;

    CoInitialize(NULL);

    /* Create MsiActiveScriptSite object */
    hr = create_ActiveScriptSite(NULL, (void **)&pActiveScriptSite);
    if (hr != S_OK) goto done;

    /* Create an installer object */
    hr = create_msiserver(NULL, (LPVOID *)&pActiveScriptSite->pInstaller);
    if (hr != S_OK) goto done;

    /* Create a session object */
    hr = create_session(hPackage, pActiveScriptSite->pInstaller, &pActiveScriptSite->pSession);
    if (hr != S_OK) goto done;

    /* Create the scripting engine */
    if ((type & 7) == msidbCustomActionTypeJScript)
        hr = CLSIDFromProgID(szJScript, &clsid);
    else if ((type & 7) == msidbCustomActionTypeVBScript)
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

    /* If we got this far, Windows Script is installed, so don't return success by default anymore */
    ret = ERROR_INSTALL_FAILURE;

    /* Get the IActiveScriptParse engine interface */
    hr = IActiveScript_QueryInterface(pActiveScript, &IID_IActiveScriptParse, (void **)&pActiveScriptParse);
    if (FAILED(hr)) goto done;

    /* Give our host to the engine */
    hr = IActiveScript_SetScriptSite(pActiveScript, (IActiveScriptSite *)pActiveScriptSite);
    if (FAILED(hr)) goto done;

    /* Initialize the script engine */
    hr = IActiveScriptParse64_InitNew(pActiveScriptParse);
    if (FAILED(hr)) goto done;

    /* Add the session object */
    hr = IActiveScript_AddNamedItem(pActiveScript, szSession, SCRIPTITEM_ISVISIBLE);

    /* Pass the script to the engine */
    hr = IActiveScriptParse64_ParseScriptText(pActiveScriptParse, script, NULL, NULL, NULL, 0, 0, 0L, NULL, NULL);
    if (FAILED(hr)) goto done;

    /* Start processing the script */
    hr = IActiveScript_SetScriptState(pActiveScript, SCRIPTSTATE_CONNECTED);
    if (FAILED(hr)) goto done;

    /* Call a function if necessary through the IDispatch interface */
    if (function != NULL && strlenW(function) > 0) {
        TRACE("Calling function %s\n", debugstr_w(function));

        hr = IActiveScript_GetScriptDispatch(pActiveScript, NULL, &pDispatch);
        if (FAILED(hr)) goto done;

        hr = IDispatch_GetIDsOfNames(pDispatch, &IID_NULL, (WCHAR **)&function, 1,LOCALE_USER_DEFAULT, &dispid);
        if (FAILED(hr)) goto done;

        hr = IDispatch_Invoke(pDispatch, dispid, &IID_NULL, LOCALE_USER_DEFAULT, DISPATCH_METHOD, &dispparamsNoArgs, &var, NULL, NULL);
        if (FAILED(hr)) goto done;

        /* Check return value, if it's not IDOK we failed */
        hr = VariantChangeType(&var, &var, 0, VT_I4);
        if (FAILED(hr)) goto done;

        if (V_I4(&var) == IDOK)
            ret = ERROR_SUCCESS;
        else ret = ERROR_INSTALL_FAILURE;

        VariantClear(&var);
    } else {
        /* If no function to be called, MSI behavior is to succeed */
        ret = ERROR_SUCCESS;
    }

done:

    /* Free everything that needs to be freed */
    if (pDispatch) IDispatch_Release(pDispatch);
    if (pActiveScript) IActiveScriptSite_Release(pActiveScript);
    if (pActiveScriptSite &&
        pActiveScriptSite->pSession) IUnknown_Release((IUnknown *)pActiveScriptSite->pSession);
    if (pActiveScriptSite &&
        pActiveScriptSite->pInstaller) IUnknown_Release((IUnknown *)pActiveScriptSite->pInstaller);
    if (pActiveScriptSite) IUnknown_Release((IUnknown *)pActiveScriptSite);

    CoUninitialize();    /* must call even if CoInitialize failed */

    return ret;
}

/*
 * MsiActiveScriptSite
 */

/*** IUnknown methods ***/
static HRESULT WINAPI MsiActiveScriptSite_QueryInterface(IActiveScriptSite* iface, REFIID riid, void** ppvObject)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IActiveScriptSite))
    {
        IClassFactory_AddRef(iface);
        *ppvObject = This;
        return S_OK;
    }

    TRACE("(%p)->(%s,%p),not found\n",This,debugstr_guid(riid),ppvObject);

    return E_NOINTERFACE;
}

static ULONG WINAPI MsiActiveScriptSite_AddRef(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MsiActiveScriptSite_Release(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

/*** IActiveScriptSite methods **/
static HRESULT WINAPI MsiActiveScriptSite_GetLCID(IActiveScriptSite* iface, LCID* plcid)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, plcid);
    return E_NOTIMPL;  /* Script will use system-defined locale */
}

static HRESULT WINAPI MsiActiveScriptSite_GetItemInfo(IActiveScriptSite* iface, LPCOLESTR pstrName, DWORD dwReturnMask, IUnknown** ppiunkItem, ITypeInfo** ppti)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    TRACE("(%p/%p)->(%p,%d,%p,%p)\n", This, iface, pstrName, dwReturnMask, ppiunkItem, ppti);

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
        if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
            return load_type_info(This->pSession, ppti, &DIID_Session, 0);
        else if (dwReturnMask & SCRIPTINFO_IUNKNOWN) {
            IDispatch_QueryInterface(This->pSession, &IID_IUnknown, (void **)ppiunkItem);
            return S_OK;
        }
    }

    return TYPE_E_ELEMENTNOTFOUND;
}

static HRESULT WINAPI MsiActiveScriptSite_GetDocVersionString(IActiveScriptSite* iface, BSTR* pbstrVersion)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    TRACE("(%p/%p)->(%p)\n", This, iface, pbstrVersion);
    return E_NOTIMPL;
}

static HRESULT WINAPI MsiActiveScriptSite_OnScriptTerminate(IActiveScriptSite* iface, const VARIANT* pvarResult, const EXCEPINFO* pexcepinfo)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    TRACE("(%p/%p)->(%p,%p)\n", This, iface, pvarResult, pexcepinfo);
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
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    EXCEPINFO exception;
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", This, iface, pscripterror);

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
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    TRACE("(%p/%p)\n", This, iface);
    return S_OK;
}

static HRESULT WINAPI MsiActiveScriptSite_OnLeaveScript(IActiveScriptSite* iface)
{
    MsiActiveScriptSite *This = (MsiActiveScriptSite *)iface;
    TRACE("(%p/%p)\n", This, iface);
    return S_OK;
}

static const struct IActiveScriptSiteVtbl ASS_Vtbl =
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
