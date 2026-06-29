/*
 * Implementation of the Active Directory Service Interface
 *
 * Copyright 2005 Detlef Riekenberg
 * Copyright 2019 Dmitry Timoshkov
 *
 * This file contains only stubs to get the printui.dll up and running
 * activeds.dll is much much more than this
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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winuser.h"

#include "objbase.h"
#include "initguid.h"
#include "iads.h"
#include "adshlp.h"
#include "adserr.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(activeds);

/*****************************************************
 * ADsGetObject     [ACTIVEDS.3]
 */
HRESULT WINAPI ADsGetObject(LPCWSTR path, REFIID riid, void **obj)
{
    HRESULT hr;

    hr = ADsOpenObject(path, NULL, NULL, ADS_SECURE_AUTHENTICATION, riid, obj);
    if (hr != S_OK)
        hr = ADsOpenObject(path, NULL, NULL, 0, riid, obj);
    return hr;
}

/*****************************************************
 * ADsBuildEnumerator    [ACTIVEDS.4]
 */
HRESULT WINAPI ADsBuildEnumerator(IADsContainer * pADsContainer, IEnumVARIANT** ppEnumVariant)
{
    FIXME("(%p)->(%p)!stub\n",pADsContainer, ppEnumVariant);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsFreeEnumerator     [ACTIVEDS.5]
 */
HRESULT WINAPI ADsFreeEnumerator(IEnumVARIANT* pEnumVariant)
{
    FIXME("(%p)!stub\n",pEnumVariant);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsEnumerateNext     [ACTIVEDS.6]
 */
HRESULT WINAPI ADsEnumerateNext(IEnumVARIANT* pEnumVariant, ULONG cElements, VARIANT* pvar, ULONG * pcElementsFetched)
{
    FIXME("(%p)->(%lu, %p, %p)!stub\n",pEnumVariant, cElements, pvar, pcElementsFetched);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsBuildVarArrayStr     [ACTIVEDS.7]
 */
HRESULT WINAPI ADsBuildVarArrayStr(LPWSTR *str, DWORD count, VARIANT *var)
{
    HRESULT hr;
    SAFEARRAY *sa;
    LONG idx, end = count;

    TRACE("(%p, %lu, %p)\n", str, count, var);

    if (!var) return E_ADS_BAD_PARAMETER;

    sa = SafeArrayCreateVector(VT_VARIANT, 0, count);
    if (!sa) return E_OUTOFMEMORY;

    VariantInit(var);
    for (idx = 0; idx < end; idx++)
    {
        VARIANT item;

        V_VT(&item) = VT_BSTR;
        V_BSTR(&item) = SysAllocString(str[idx]);
        if (!V_BSTR(&item))
        {
            hr = E_OUTOFMEMORY;
            goto fail;
        }

        hr = SafeArrayPutElement(sa, &idx, &item);
        SysFreeString(V_BSTR(&item));
        if (hr != S_OK) goto fail;
    }

    V_VT(var) = VT_ARRAY | VT_VARIANT;
    V_ARRAY(var) = sa;
    return S_OK;

fail:
    SafeArrayDestroy(sa);
    return hr;
}

/*****************************************************
 * ADsBuildVarArrayInt     [ACTIVEDS.8]
 */
HRESULT WINAPI ADsBuildVarArrayInt(LPDWORD lpdwObjectTypes, DWORD dwObjectTypes, VARIANT* pvar)
{
    FIXME("(%p, %ld, %p)!stub\n",lpdwObjectTypes, dwObjectTypes, pvar);
    return E_NOTIMPL;
}

/*****************************************************
 * ADsOpenObject     [ACTIVEDS.9]
 */
HRESULT WINAPI ADsOpenObject(LPCWSTR path, LPCWSTR user, LPCWSTR password, DWORD reserved, REFIID riid, void **obj)
{
    HRESULT hr;
    HKEY hkey, hprov;
    WCHAR provider[MAX_PATH], progid[MAX_PATH];
    DWORD idx = 0;

    TRACE("(%s,%s,%lu,%s,%p)\n", debugstr_w(path), debugstr_w(user), reserved, debugstr_guid(riid), obj);

    if (!path || !riid || !obj)
        return E_INVALIDARG;

    hr = E_FAIL;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\Microsoft\\ADs\\Providers", 0, KEY_READ, &hkey))
        return hr;

    for (;;)
    {
        if (RegEnumKeyW(hkey, idx++, provider, ARRAY_SIZE(provider)))
            break;

        TRACE("provider %s\n", debugstr_w(provider));

        if (!wcsnicmp(path, provider, wcslen(provider)) && path[wcslen(provider)] == ':')
        {
            LONG size;

            if (RegOpenKeyExW(hkey, provider, 0, KEY_READ, &hprov))
                break;

            size = ARRAY_SIZE(progid);
            if (!RegQueryValueW(hprov, NULL, progid, &size))
            {
                CLSID clsid;

                if (CLSIDFromProgID(progid, &clsid) == S_OK)
                {
                    IADsOpenDSObject *adsopen;
                    IDispatch *disp;

                    TRACE("ns %s => clsid %s\n", debugstr_w(progid), wine_dbgstr_guid(&clsid));
                    if (CoCreateInstance(&clsid, 0, CLSCTX_INPROC_SERVER, &IID_IADsOpenDSObject, (void **)&adsopen) == S_OK)
                    {
                        BSTR bpath, buser, bpassword;

                        bpath = SysAllocString(path);
                        buser = SysAllocString(user);
                        bpassword = SysAllocString(password);

                        hr = IADsOpenDSObject_OpenDSObject(adsopen, bpath, buser, bpassword, reserved, &disp);
                        if (hr == S_OK)
                        {
                            hr = IDispatch_QueryInterface(disp, riid, obj);
                            IDispatch_Release(disp);
                        }

                        SysFreeString(bpath);
                        SysFreeString(buser);
                        SysFreeString(bpassword);

                        IADsOpenDSObject_Release(adsopen);
                    }
                }
            }

            RegCloseKey(hprov);
            break;
        }
    }

    RegCloseKey(hkey);

    return hr;
}

/*****************************************************
 * ADsSetLastError    [ACTIVEDS.12]
 */
VOID WINAPI ADsSetLastError(DWORD dwErr, LPWSTR pszError, LPWSTR pszProvider)
{
    FIXME("(%ld,%p,%p)!stub\n", dwErr, pszError, pszProvider);
}

/*****************************************************
 * ADsGetLastError    [ACTIVEDS.13]
 */
HRESULT WINAPI ADsGetLastError(LPDWORD perror, LPWSTR errorbuf, DWORD errorbuflen, LPWSTR namebuf, DWORD namebuflen)
{
    FIXME("(%p,%p,%ld,%p,%ld)!stub\n", perror, errorbuf, errorbuflen, namebuf, namebuflen);
    return E_NOTIMPL;
}

/*****************************************************
 * AllocADsMem             [ACTIVEDS.14]
 */
LPVOID WINAPI AllocADsMem(DWORD cb)
{
    return malloc(cb);
}

/*****************************************************
 * FreeADsMem             [ACTIVEDS.15]
 */
BOOL WINAPI FreeADsMem(LPVOID pMem)
{
    free(pMem);
    return TRUE;
}

/*****************************************************
 * ReallocADsMem             [ACTIVEDS.16]
 */
LPVOID WINAPI ReallocADsMem(LPVOID pOldMem, DWORD cbOld, DWORD cbNew)
{
    return realloc(pOldMem, cbNew);
}

/*****************************************************
 * AllocADsStr             [ACTIVEDS.17]
 */
LPWSTR WINAPI AllocADsStr(LPWSTR pStr)
{
    TRACE("(%p)\n", pStr);
    return wcsdup(pStr);
}

/*****************************************************
 * FreeADsStr             [ACTIVEDS.18]
 */
BOOL WINAPI FreeADsStr(LPWSTR pStr)
{
    TRACE("(%p)\n", pStr);

    return FreeADsMem(pStr);
}

/*****************************************************
 * ReallocADsStr             [ACTIVEDS.19]
 */
BOOL WINAPI ReallocADsStr(LPWSTR *ppStr, LPWSTR pStr)
{
    FIXME("(%p,%p)!stub\n",*ppStr, pStr);
    return FALSE;
}

/*****************************************************
 * ADsEncodeBinaryData     [ACTIVEDS.20]
 */
HRESULT WINAPI ADsEncodeBinaryData(PBYTE pbSrcData, DWORD dwSrcLen, LPWSTR *ppszDestData)
{
    FIXME("(%p,%ld,%p)!stub\n", pbSrcData, dwSrcLen, *ppszDestData);
    return E_NOTIMPL;
}
