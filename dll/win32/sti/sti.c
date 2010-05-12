/*
 * Copyright (C) 2002 Aric Stewart for CodeWeavers
 * Copyright (C) 2009 Damjan Jovanovic
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
#include "winreg.h"
#include "winerror.h"
#include "objbase.h"
#include "sti.h"

#include "sti_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"

WINE_DEFAULT_DEBUG_CHANNEL(sti);

static const WCHAR registeredAppsLaunchPath[] = {
    'S','O','F','T','W','A','R','E','\\',
    'M','i','c','r','o','s','o','f','t','\\',
    'W','i','n','d','o','w','s','\\',
    'C','u','r','r','e','n','t','V','e','r','s','i','o','n','\\',
    'S','t','i','l','l','I','m','a','g','e','\\',
    'R','e','g','i','s','t','e','r','e','d',' ','A','p','p','l','i','c','a','t','i','o','n','s',0
};

static inline stillimage *impl_from_StillImageW(IStillImageW *iface)
{
    return (stillimage *)((char*)iface - FIELD_OFFSET(stillimage, lpVtbl));
}

static HRESULT WINAPI stillimagew_QueryInterface(IStillImageW *iface, REFIID riid, void **ppvObject)
{
    stillimage *This = impl_from_StillImageW(iface);
    TRACE("(%p %s %p)\n", This, debugstr_guid(riid), ppvObject);
    return IUnknown_QueryInterface(This->pUnkOuter, riid, ppvObject);
}

static ULONG WINAPI stillimagew_AddRef(IStillImageW *iface)
{
    stillimage *This = impl_from_StillImageW(iface);
    return IUnknown_AddRef(This->pUnkOuter);
}

static ULONG WINAPI stillimagew_Release(IStillImageW *iface)
{
    stillimage *This = impl_from_StillImageW(iface);
    return IUnknown_Release(This->pUnkOuter);
}

static HRESULT WINAPI stillimagew_Initialize(IStillImageW *iface, HINSTANCE hinst, DWORD dwVersion)
{
    stillimage *This = impl_from_StillImageW(iface);
    TRACE("(%p, %p, 0x%X)\n", This, hinst, dwVersion);
    return S_OK;
}

static HRESULT WINAPI stillimagew_GetDeviceList(IStillImageW *iface, DWORD dwType, DWORD dwFlags,
                                                DWORD *pdwItemsReturned, LPVOID *ppBuffer)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %u, 0x%X, %p, %p): stub\n", This, dwType, dwFlags, pdwItemsReturned, ppBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_GetDeviceInfo(IStillImageW *iface, LPWSTR pwszDeviceName,
                                                LPVOID *ppBuffer)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszDeviceName), ppBuffer);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_CreateDevice(IStillImageW *iface, LPWSTR pwszDeviceName, DWORD dwMode,
                                               PSTIDEVICEW *pDevice, LPUNKNOWN pUnkOuter)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %u, %p, %p): stub\n", This, debugstr_w(pwszDeviceName), dwMode, pDevice, pUnkOuter);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_GetDeviceValue(IStillImageW *iface, LPWSTR pwszDeviceName, LPWSTR pValueName,
                                                 LPDWORD pType, LPBYTE pData, LPDWORD cbData)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %s, %p, %p, %p): stub\n", This, debugstr_w(pwszDeviceName), debugstr_w(pValueName),
        pType, pData, cbData);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_SetDeviceValue(IStillImageW *iface, LPWSTR pwszDeviceName, LPWSTR pValueName,
                                                 DWORD type, LPBYTE pData, DWORD cbData)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %s, %u, %p, %u): stub\n", This, debugstr_w(pwszDeviceName), debugstr_w(pValueName),
        type, pData, cbData);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_GetSTILaunchInformation(IStillImageW *iface, LPWSTR pwszDeviceName,
                                                          DWORD *pdwEventCode, LPWSTR pwszEventName)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %p, %p, %p): stub\n", This, pwszDeviceName,
        pdwEventCode, pwszEventName);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_RegisterLaunchApplication(IStillImageW *iface, LPWSTR pwszAppName,
                                                            LPWSTR pwszCommandLine)
{
    static const WCHAR format[] = {'%','s',' ','%','s',0};
    static const WCHAR commandLineSuffix[] = {
        '/','S','t','i','D','e','v','i','c','e',':','%','1',' ',
        '/','S','t','i','E','v','e','n','t',':','%','2',0};
    HKEY registeredAppsKey = NULL;
    DWORD ret;
    HRESULT hr = S_OK;
    stillimage *This = impl_from_StillImageW(iface);

    TRACE("(%p, %s, %s)\n", This, debugstr_w(pwszAppName), debugstr_w(pwszCommandLine));

    ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, registeredAppsLaunchPath, &registeredAppsKey);
    if (ret == ERROR_SUCCESS)
    {
        WCHAR *value = HeapAlloc(GetProcessHeap(), 0,
            (lstrlenW(pwszCommandLine) + 1 + lstrlenW(commandLineSuffix) + 1) * sizeof(WCHAR));
        if (value)
        {
            sprintfW(value, format, pwszCommandLine, commandLineSuffix);
            ret = RegSetValueExW(registeredAppsKey, pwszAppName, 0,
                REG_SZ, (BYTE*)value, (lstrlenW(value)+1)*sizeof(WCHAR));
            if (ret != ERROR_SUCCESS)
                hr = HRESULT_FROM_WIN32(ret);
            HeapFree(GetProcessHeap(), 0, value);
        }
        else
            hr = E_OUTOFMEMORY;
        RegCloseKey(registeredAppsKey);
    }
    else
        hr = HRESULT_FROM_WIN32(ret);
    return hr;
}

static HRESULT WINAPI stillimagew_UnregisterLaunchApplication(IStillImageW *iface, LPWSTR pwszAppName)
{
    stillimage *This = impl_from_StillImageW(iface);
    HKEY registeredAppsKey = NULL;
    DWORD ret;
    HRESULT hr = S_OK;

    TRACE("(%p, %s)\n", This, debugstr_w(pwszAppName));

    ret = RegCreateKeyW(HKEY_LOCAL_MACHINE, registeredAppsLaunchPath, &registeredAppsKey);
    if (ret == ERROR_SUCCESS)
    {
        ret = RegDeleteValueW(registeredAppsKey, pwszAppName);
        if (ret != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(ret);
        RegCloseKey(registeredAppsKey);
    }
    else
        hr = HRESULT_FROM_WIN32(ret);
    return hr;
}

static HRESULT WINAPI stillimagew_EnableHwNotifications(IStillImageW *iface, LPCWSTR pwszDeviceName,
                                                        BOOL bNewState)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %u): stub\n", This, debugstr_w(pwszDeviceName), bNewState);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_GetHwNotificationState(IStillImageW *iface, LPCWSTR pwszDeviceName,
                                                         BOOL *pbCurrentState)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %p): stub\n", This, debugstr_w(pwszDeviceName), pbCurrentState);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_RefreshDeviceBus(IStillImageW *iface, LPCWSTR pwszDeviceName)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s): stub\n", This, debugstr_w(pwszDeviceName));
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_LaunchApplicationForDevice(IStillImageW *iface, LPWSTR pwszDeviceName,
                                                             LPWSTR pwszAppName, LPSTINOTIFY pStiNotify)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %s, %s, %p): stub\n", This, debugstr_w(pwszDeviceName), debugstr_w(pwszAppName),
        pStiNotify);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_SetupDeviceParameters(IStillImageW *iface, PSTI_DEVICE_INFORMATIONW pDevInfo)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %p): stub\n", This, pDevInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI stillimagew_WriteToErrorLog(IStillImageW *iface, DWORD dwMessageType, LPCWSTR pszMessage)
{
    stillimage *This = impl_from_StillImageW(iface);
    FIXME("(%p, %u, %s): stub\n", This, dwMessageType, debugstr_w(pszMessage));
    return E_NOTIMPL;
}

static const struct IStillImageWVtbl stillimagew_vtbl =
{
    stillimagew_QueryInterface,
    stillimagew_AddRef,
    stillimagew_Release,
    stillimagew_Initialize,
    stillimagew_GetDeviceList,
    stillimagew_GetDeviceInfo,
    stillimagew_CreateDevice,
    stillimagew_GetDeviceValue,
    stillimagew_SetDeviceValue,
    stillimagew_GetSTILaunchInformation,
    stillimagew_RegisterLaunchApplication,
    stillimagew_UnregisterLaunchApplication,
    stillimagew_EnableHwNotifications,
    stillimagew_GetHwNotificationState,
    stillimagew_RefreshDeviceBus,
    stillimagew_LaunchApplicationForDevice,
    stillimagew_SetupDeviceParameters,
    stillimagew_WriteToErrorLog
};

static inline stillimage *impl_from_InternalUnknown(IUnknown *iface)
{
    return (stillimage *)((char*)iface - FIELD_OFFSET(stillimage, lpInternalUnkVtbl));
}

static HRESULT WINAPI Internal_QueryInterface(IUnknown *iface, REFIID riid, void **ppvObject)
{
    stillimage *This = impl_from_InternalUnknown(iface);

    TRACE("(%p %s %p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown))
        *ppvObject = iface;
    else if (IsEqualGUID(riid, &IID_IStillImageW))
        *ppvObject = &This->lpVtbl;
    else
    {
        if (IsEqualGUID(riid, &IID_IStillImageA))
            FIXME("interface IStillImageA is unsupported on Windows Vista too, please report if it's needed\n");
        else
            FIXME("interface %s not implemented\n", debugstr_guid(riid));
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*) *ppvObject);
    return S_OK;
}

static ULONG WINAPI Internal_AddRef(IUnknown *iface)
{
    stillimage *This = impl_from_InternalUnknown(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI Internal_Release(IUnknown *iface)
{
    ULONG ref;
    stillimage *This = impl_from_InternalUnknown(iface);

    ref = InterlockedDecrement(&This->ref);
    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static const struct IUnknownVtbl internal_unk_vtbl =
{
    Internal_QueryInterface,
    Internal_AddRef,
    Internal_Release
};

/******************************************************************************
 *           StiCreateInstanceA   (STI.@)
 */
HRESULT WINAPI StiCreateInstanceA(HINSTANCE hinst, DWORD dwVer, PSTIA *ppSti, LPUNKNOWN pUnkOuter)
{
    FIXME("(%p, %u, %p, %p): stub, unimplemented on Windows Vista too, please report if it's needed\n", hinst, dwVer, ppSti, pUnkOuter);
    return STG_E_UNIMPLEMENTEDFUNCTION;
}

/******************************************************************************
 *           StiCreateInstanceW   (STI.@)
 */
HRESULT WINAPI StiCreateInstanceW(HINSTANCE hinst, DWORD dwVer, PSTIW *ppSti, LPUNKNOWN pUnkOuter)
{
    stillimage *This;
    HRESULT hr;

    TRACE("(%p, %u, %p, %p)\n", hinst, dwVer, ppSti, pUnkOuter);

    This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(stillimage));
    if (This)
    {
        This->lpVtbl = &stillimagew_vtbl;
        This->lpInternalUnkVtbl = &internal_unk_vtbl;
        if (pUnkOuter)
            This->pUnkOuter = pUnkOuter;
        else
            This->pUnkOuter = (IUnknown*) &This->lpInternalUnkVtbl;
        This->ref = 1;

        hr = IStillImage_Initialize((IStillImageW*) &This->lpVtbl, hinst, dwVer);
        if (SUCCEEDED(hr))
        {
            if (pUnkOuter)
                *ppSti = (IStillImageW*) &This->lpInternalUnkVtbl;
            else
                *ppSti = (IStillImageW*) &This->lpVtbl;
        }
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}
