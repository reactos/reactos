/*
 *    INSENG Implementation
 *
 * Copyright 2006 Mike McCormack
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

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <config.h>

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>
//#include "winuser.h"
#include <ole2.h>
#include <rpcproxy.h>
#include <initguid.h>
#include <inseng.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(inseng);

static inline void *heap_alloc(size_t len)
{
    return HeapAlloc(GetProcessHeap(), 0, len);
}

static inline BOOL heap_free(void *mem)
{
    return HeapFree(GetProcessHeap(), 0, mem);
}

static HINSTANCE instance;

struct InstallEngine {
    IInstallEngine2 IInstallEngine2_iface;
    LONG ref;
};

static inline InstallEngine *impl_from_IInstallEngine2(IInstallEngine2 *iface)
{
    return CONTAINING_RECORD(iface, InstallEngine, IInstallEngine2_iface);
}

static HRESULT WINAPI InstallEngine_QueryInterface(IInstallEngine2 *iface, REFIID riid, void **ppv)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IInstallEngine2_iface;
    }else if(IsEqualGUID(&IID_IInstallEngine, riid)) {
        TRACE("(%p)->(IID_IInstallEngine %p)\n", This, ppv);
        *ppv = &This->IInstallEngine2_iface;
    }else if(IsEqualGUID(&IID_IInstallEngine2, riid)) {
        TRACE("(%p)->(IID_IInstallEngine2 %p)\n", This, ppv);
        *ppv = &This->IInstallEngine2_iface;
    }else {
        TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI InstallEngine_AddRef(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI InstallEngine_Release(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI InstallEngine_GetEngineStatus(IInstallEngine2 *iface, DWORD *status)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%p)\n", This, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetCifFile(IInstallEngine2 *iface, const char *cab_name, const char *cif_name)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_a(cab_name), debugstr_a(cif_name));
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_DownloadComponents(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%x)\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_InstallComponents(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%x)\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_EnumInstallIDs(IInstallEngine2 *iface, UINT index, char **id)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%d %p)\n", This, index, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_EnumDownloadIDs(IInstallEngine2 *iface, UINT index, char **id)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%d %p)\n", This, index, id);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_IsComponentInstalled(IInstallEngine2 *iface, const char *id, DWORD *status)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_a(id), status);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_RegisterInstallEngineCallback(IInstallEngine2 *iface, IInstallEngineCallback *callback)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%p)\n", This, callback);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_UnregisterInstallEngineCallback(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetAction(IInstallEngine2 *iface, const char *id, DWORD action, DWORD priority)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s %d %d)\n", This, debugstr_a(id), action, priority);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_GetSizes(IInstallEngine2 *iface, const char *id, COMPONENT_SIZES *sizes)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s %p)\n", This, debugstr_a(id), sizes);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_LaunchExtraCommand(IInstallEngine2 *iface, const char *inf_name, const char *section)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_a(inf_name), debugstr_a(section));
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_GetDisplayName(IInstallEngine2 *iface, const char *id, const char *name)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s %s)\n", This, debugstr_a(id), debugstr_a(name));
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetBaseUrl(IInstallEngine2 *iface, const char *base_name)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_a(base_name));
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetDownloadDir(IInstallEngine2 *iface, const char *download_dir)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_a(download_dir));
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetInstallDrive(IInstallEngine2 *iface, char drive)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%c)\n", This, drive);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetInstallOptions(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%x)\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetHWND(IInstallEngine2 *iface, HWND hwnd)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%p)\n", This, hwnd);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_SetIStream(IInstallEngine2 *iface, IStream *stream)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%p)\n", This, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_Abort(IInstallEngine2 *iface, DWORD flags)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%x)\n", This, flags);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_Suspend(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine_Resume(IInstallEngine2 *iface)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)\n", This);
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine2_SetLocalCif(IInstallEngine2 *iface, const char *cif)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%s)\n", This, debugstr_a(cif));
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallEngine2_GetICifFile(IInstallEngine2 *iface, ICifFile **cif_file)
{
    InstallEngine *This = impl_from_IInstallEngine2(iface);
    FIXME("(%p)->(%p)\n", This, cif_file);
    return E_NOTIMPL;
}

static const IInstallEngine2Vtbl InstallEngine2Vtbl = {
    InstallEngine_QueryInterface,
    InstallEngine_AddRef,
    InstallEngine_Release,
    InstallEngine_GetEngineStatus,
    InstallEngine_SetCifFile,
    InstallEngine_DownloadComponents,
    InstallEngine_InstallComponents,
    InstallEngine_EnumInstallIDs,
    InstallEngine_EnumDownloadIDs,
    InstallEngine_IsComponentInstalled,
    InstallEngine_RegisterInstallEngineCallback,
    InstallEngine_UnregisterInstallEngineCallback,
    InstallEngine_SetAction,
    InstallEngine_GetSizes,
    InstallEngine_LaunchExtraCommand,
    InstallEngine_GetDisplayName,
    InstallEngine_SetBaseUrl,
    InstallEngine_SetDownloadDir,
    InstallEngine_SetInstallDrive,
    InstallEngine_SetInstallOptions,
    InstallEngine_SetHWND,
    InstallEngine_SetIStream,
    InstallEngine_Abort,
    InstallEngine_Suspend,
    InstallEngine_Resume,
    InstallEngine2_SetLocalCif,
    InstallEngine2_GetICifFile
};

static HRESULT WINAPI ClassFactory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    *ppv = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", iface, ppv);
        *ppv = iface;
    }else if(IsEqualGUID(&IID_IClassFactory, riid)) {
        TRACE("(%p)->(IID_IClassFactory %p)\n", iface, ppv);
        *ppv = iface;
    }

    if(*ppv) {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI ClassFactory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI ClassFactory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI ClassFactory_LockServer(IClassFactory *iface, BOOL fLock)
{
    return S_OK;
}

static HRESULT WINAPI InstallEngineCF_CreateInstance(IClassFactory *iface, IUnknown *outer,
        REFIID riid, void **ppv)
{
    InstallEngine *engine;
    HRESULT hres;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    engine = heap_alloc(sizeof(*engine));
    if(!engine)
        return E_OUTOFMEMORY;

    engine->IInstallEngine2_iface.lpVtbl = &InstallEngine2Vtbl;
    engine->ref = 1;

    hres = IInstallEngine2_QueryInterface(&engine->IInstallEngine2_iface, riid, ppv);
    IInstallEngine2_Release(&engine->IInstallEngine2_iface);
    return hres;
}

static const IClassFactoryVtbl InstallEngineCFVtbl = {
    ClassFactory_QueryInterface,
    ClassFactory_AddRef,
    ClassFactory_Release,
    InstallEngineCF_CreateInstance,
    ClassFactory_LockServer
};

static IClassFactory InstallEngineCF = { &InstallEngineCFVtbl };

BOOL WINAPI DllMain(HINSTANCE hInstDLL, DWORD fdwReason, LPVOID lpv)
{
    switch(fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        instance = hInstDLL;
        DisableThreadLibraryCalls(hInstDLL);
        break;
    }
    return TRUE;
}

/***********************************************************************
 *             DllGetClassObject (INSENG.@)
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    if(IsEqualGUID(rclsid, &CLSID_InstallEngine)) {
        TRACE("(CLSID_InstallEngine %s %p)\n", debugstr_guid(iid), ppv);
        return IClassFactory_QueryInterface(&InstallEngineCF, iid, ppv);
    }

    FIXME("(%s %s %p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *              DllCanUnloadNow (INSENG.@)
 */
HRESULT WINAPI DllCanUnloadNow(void)
{
    return S_FALSE;
}

/***********************************************************************
 *		DllRegisterServer (INSENG.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return __wine_register_resources( instance );
}

/***********************************************************************
 *		DllUnregisterServer (INSENG.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources( instance );
}

BOOL WINAPI CheckTrustEx( LPVOID a, LPVOID b, LPVOID c, LPVOID d, LPVOID e )
{
    FIXME("%p %p %p %p %p\n", a, b, c, d, e );
    return TRUE;
}

/***********************************************************************
 *  DllInstall (INSENG.@)
 */
HRESULT WINAPI DllInstall(BOOL bInstall, LPCWSTR cmdline)
{
    FIXME("(%s, %s): stub\n", bInstall ? "TRUE" : "FALSE", debugstr_w(cmdline));
    return S_OK;
}
