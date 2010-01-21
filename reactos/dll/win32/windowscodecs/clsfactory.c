/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
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

#include "config.h"

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "ocidl.h"
#include "initguid.h"
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

typedef struct {
    REFCLSID classid;
    HRESULT (*constructor)(IUnknown*,REFIID,void**);
} classinfo;

static classinfo wic_classes[] = {
    {&CLSID_WICImagingFactory, ImagingFactory_CreateInstance},
    {&CLSID_WICBmpDecoder, BmpDecoder_CreateInstance},
    {&CLSID_WICPngDecoder, PngDecoder_CreateInstance},
    {&CLSID_WICPngEncoder, PngEncoder_CreateInstance},
    {&CLSID_WICBmpEncoder, BmpEncoder_CreateInstance},
    {&CLSID_WICGifDecoder, GifDecoder_CreateInstance},
    {&CLSID_WICIcoDecoder, IcoDecoder_CreateInstance},
    {&CLSID_WICJpegDecoder, JpegDecoder_CreateInstance},
    {&CLSID_WICDefaultFormatConverter, FormatConverter_CreateInstance},
    {0}};

typedef struct {
    const IClassFactoryVtbl *lpIClassFactoryVtbl;
    LONG                    ref;
    classinfo               *info;
} ClassFactoryImpl;

static HRESULT WINAPI ClassFactoryImpl_QueryInterface(IClassFactory *iface,
    REFIID iid, void **ppv)
{
    ClassFactoryImpl *This = (ClassFactoryImpl*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IClassFactory, iid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ClassFactoryImpl_AddRef(IClassFactory *iface)
{
    ClassFactoryImpl *This = (ClassFactoryImpl*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ClassFactoryImpl_Release(IClassFactory *iface)
{
    ClassFactoryImpl *This = (ClassFactoryImpl*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
        HeapFree(GetProcessHeap(), 0, This);

    return ref;
}

static HRESULT WINAPI ClassFactoryImpl_CreateInstance(IClassFactory *iface,
    IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    ClassFactoryImpl *This = (ClassFactoryImpl*)iface;

    return This->info->constructor(pUnkOuter, riid, ppv);
}

static HRESULT WINAPI ClassFactoryImpl_LockServer(IClassFactory *iface, BOOL lock)
{
    TRACE("(%p, %i): stub\n", iface, lock);
    return E_NOTIMPL;
}

static const IClassFactoryVtbl ClassFactoryImpl_Vtbl = {
    ClassFactoryImpl_QueryInterface,
    ClassFactoryImpl_AddRef,
    ClassFactoryImpl_Release,
    ClassFactoryImpl_CreateInstance,
    ClassFactoryImpl_LockServer
};

static HRESULT ClassFactoryImpl_Constructor(classinfo *info, REFIID riid, LPVOID *ppv)
{
    ClassFactoryImpl *This;
    HRESULT ret;

    *ppv = NULL;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ClassFactoryImpl));
    if (!This) return E_OUTOFMEMORY;

    This->lpIClassFactoryVtbl = &ClassFactoryImpl_Vtbl;
    This->ref = 1;
    This->info = info;

    ret = IClassFactory_QueryInterface((IClassFactory*)This, riid, ppv);
    IClassFactory_Release((IClassFactory*)This);

    return ret;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    HRESULT ret;
    classinfo *info=NULL;
    int i;

    TRACE("(%s,%s,%p)\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    if (!rclsid || !iid || !ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    for (i=0; wic_classes[i].classid; i++)
    {
        if (IsEqualCLSID(wic_classes[i].classid, rclsid))
        {
            info = &wic_classes[i];
            break;
        }
    }

    if (info)
        ret = ClassFactoryImpl_Constructor(info, iid, ppv);
    else
        ret = CLASS_E_CLASSNOTAVAILABLE;

    TRACE("<-- %08X\n", ret);
    return ret;
}
