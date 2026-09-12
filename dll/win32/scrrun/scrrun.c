/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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
#include "ole2.h"
#include "olectl.h"
#include "rpcproxy.h"

#include <initguid.h>
#include "scrrun.h"
#include "scrrun_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

static inline struct provideclassinfo *impl_from_IProvideClassInfo(IProvideClassInfo *iface)
{
    return CONTAINING_RECORD(iface, struct provideclassinfo, IProvideClassInfo_iface);
}

typedef HRESULT (*fnCreateInstance)(LPVOID *ppObj);

static HRESULT WINAPI scrruncf_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppv )
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

    WARN("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

static ULONG WINAPI scrruncf_AddRef(IClassFactory *iface )
{
    TRACE("(%p)\n", iface);
    return 2;
}

static ULONG WINAPI scrruncf_Release(IClassFactory *iface )
{
    TRACE("(%p)\n", iface);
    return 1;
}

static HRESULT WINAPI scrruncf_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("(%p)->(%x)\n", iface, fLock);
    return S_OK;
}

static const struct IClassFactoryVtbl scrruncf_vtbl =
{
    scrruncf_QueryInterface,
    scrruncf_AddRef,
    scrruncf_Release,
    FileSystem_CreateInstance,
    scrruncf_LockServer
};

static const struct IClassFactoryVtbl dictcf_vtbl =
{
    scrruncf_QueryInterface,
    scrruncf_AddRef,
    scrruncf_Release,
    Dictionary_CreateInstance,
    scrruncf_LockServer
};

static IClassFactory FileSystemFactory = { &scrruncf_vtbl };
static IClassFactory DictionaryFactory = { &dictcf_vtbl };

static ITypeLib *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static REFIID tid_ids[] = {
    &IID_NULL,
    &IID_IDictionary,
    &IID_IDrive,
    &IID_IDriveCollection,
    &IID_IFile,
    &IID_IFileCollection,
    &IID_IFileSystem3,
    &IID_IFolder,
    &IID_IFolderCollection,
    &IID_ITextStream
};

static HRESULT load_typelib(void)
{
    HRESULT hres;
    ITypeLib *tl;

    if(typelib)
        return S_OK;

    hres = LoadRegTypeLib(&LIBID_Scripting, 1, 0, LOCALE_SYSTEM_DEFAULT, &tl);
    if(FAILED(hres)) {
        ERR("LoadRegTypeLib failed: %#lx.\n", hres);
        return hres;
    }

    if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
        ITypeLib_Release(tl);
    return hres;
}

static HRESULT get_typeinfo_of_guid(const GUID *guid, ITypeInfo **tinfo)
{
    HRESULT hres;

    if(FAILED(hres = load_typelib()))
        return hres;

    return ITypeLib_GetTypeInfoOfGuid(typelib, guid, tinfo);
}

HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hres;

    if (FAILED(hres = load_typelib()))
        return hres;

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %#lx.\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    ITypeInfo_AddRef(typeinfos[tid]);
    return S_OK;
}

static void release_typelib(void)
{
    unsigned i;

    if(!typelib)
        return;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    ITypeLib_Release(typelib);
}

static HRESULT WINAPI provideclassinfo_QueryInterface(IProvideClassInfo *iface, REFIID riid, void **obj)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IProvideClassInfo)) {
        *obj = iface;
        IProvideClassInfo_AddRef(iface);
        return S_OK;
    }
    else
        return IUnknown_QueryInterface(This->outer, riid, obj);
}

static ULONG WINAPI provideclassinfo_AddRef(IProvideClassInfo *iface)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);
    return IUnknown_AddRef(This->outer);
}

static ULONG WINAPI provideclassinfo_Release(IProvideClassInfo *iface)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);
    return IUnknown_Release(This->outer);
}

static HRESULT WINAPI provideclassinfo_GetClassInfo(IProvideClassInfo *iface, ITypeInfo **ti)
{
    struct provideclassinfo *This = impl_from_IProvideClassInfo(iface);

    TRACE("(%p)->(%p)\n", This, ti);

    return get_typeinfo_of_guid(This->guid, ti);
}

static const IProvideClassInfoVtbl provideclassinfovtbl = {
    provideclassinfo_QueryInterface,
    provideclassinfo_AddRef,
    provideclassinfo_Release,
    provideclassinfo_GetClassInfo
};

void init_classinfo(const GUID *guid, IUnknown *outer, struct provideclassinfo *classinfo)
{
    classinfo->IProvideClassInfo_iface.lpVtbl = &provideclassinfovtbl;
    classinfo->outer = outer;
    classinfo->guid = guid;
}

BOOL WINAPI DllMain( HINSTANCE hinst, DWORD reason, LPVOID reserved )
{
    TRACE("%p, %lu, %p\n", hinst, reason, reserved);

    switch (reason)
    {
        case DLL_PROCESS_ATTACH:
            DisableThreadLibraryCalls( hinst );
            break;
        case DLL_PROCESS_DETACH:
            if (reserved) break;
            release_typelib();
            break;
    }
    return TRUE;
}

/***********************************************************************
 *      DllGetClassObject (scrrun.@)
 */

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    if(IsEqualGUID(&CLSID_FileSystemObject, rclsid)) {
        TRACE("(CLSID_FileSystemObject %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&FileSystemFactory, riid, ppv);
    }
    else if(IsEqualGUID(&CLSID_Dictionary, rclsid)) {
        TRACE("(CLSID_Dictionary %s %p)\n", debugstr_guid(riid), ppv);
        return IClassFactory_QueryInterface(&DictionaryFactory, riid, ppv);
    }

    FIXME("%s %s %p\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return CLASS_E_CLASSNOTAVAILABLE;
}
