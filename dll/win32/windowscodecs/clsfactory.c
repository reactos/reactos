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

#include <stdarg.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "objbase.h"
#include "ocidl.h"
#include "wincodec.h"
#include "wincodecsdk.h"
#include "initguid.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

extern HRESULT WINAPI WIC_DllGetClassObject(REFCLSID, REFIID, LPVOID *);

typedef struct {
    REFCLSID classid;
    class_constructor constructor;
} classinfo;

static const classinfo wic_classes[] = {
    {&CLSID_WICImagingFactory, ImagingFactory_CreateInstance},
    {&CLSID_WICImagingFactory2, ImagingFactory_CreateInstance},
    {&CLSID_WICBmpDecoder, BmpDecoder_CreateInstance},
    {&CLSID_WICPngDecoder, PngDecoder_CreateInstance},
    {&CLSID_WICPngDecoder2, PngDecoder_CreateInstance},
    {&CLSID_WICPngEncoder, PngEncoder_CreateInstance},
    {&CLSID_WICBmpEncoder, BmpEncoder_CreateInstance},
    {&CLSID_WICGifDecoder, GifDecoder_CreateInstance},
    {&CLSID_WICGifEncoder, GifEncoder_CreateInstance},
    {&CLSID_WICIcoDecoder, IcoDecoder_CreateInstance},
    {&CLSID_WICJpegDecoder, JpegDecoder_CreateInstance},
    {&CLSID_WICJpegEncoder, JpegEncoder_CreateInstance},
    {&CLSID_WICTiffDecoder, TiffDecoder_CreateInstance},
    {&CLSID_WICTiffEncoder, TiffEncoder_CreateInstance},
    {&CLSID_WICDdsDecoder, DdsDecoder_CreateInstance},
    {&CLSID_WICDdsEncoder, DdsEncoder_CreateInstance},
    {&CLSID_WICDefaultFormatConverter, FormatConverter_CreateInstance},
    {&CLSID_WineTgaDecoder, TgaDecoder_CreateInstance},
    {&CLSID_WICUnknownMetadataReader, UnknownMetadataReader_CreateInstance},
    {&CLSID_WICIfdMetadataReader, IfdMetadataReader_CreateInstance},
    {&CLSID_WICPngChrmMetadataReader, PngChrmReader_CreateInstance},
    {&CLSID_WICPngGamaMetadataReader, PngGamaReader_CreateInstance},
    {&CLSID_WICPngHistMetadataReader, PngHistReader_CreateInstance},
    {&CLSID_WICPngTextMetadataReader, PngTextReader_CreateInstance},
    {&CLSID_WICPngTimeMetadataReader, PngTimeReader_CreateInstance},
    {&CLSID_WICLSDMetadataReader, LSDReader_CreateInstance},
    {&CLSID_WICIMDMetadataReader, IMDReader_CreateInstance},
    {&CLSID_WICGCEMetadataReader, GCEReader_CreateInstance},
    {&CLSID_WICAPEMetadataReader, APEReader_CreateInstance},
    {&CLSID_WICGifCommentMetadataReader, GifCommentReader_CreateInstance},
    {0}};

typedef struct {
    IClassFactory           IClassFactory_iface;
    LONG                    ref;
    const classinfo         *info;
} ClassFactoryImpl;

static inline ClassFactoryImpl *impl_from_IClassFactory(IClassFactory *iface)
{
    return CONTAINING_RECORD(iface, ClassFactoryImpl, IClassFactory_iface);
}

static HRESULT WINAPI ClassFactoryImpl_QueryInterface(IClassFactory *iface,
    REFIID iid, void **ppv)
{
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IClassFactory, iid))
    {
        *ppv = &This->IClassFactory_iface;
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
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI ClassFactoryImpl_Release(IClassFactory *iface)
{
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
        free(This);

    return ref;
}

static HRESULT WINAPI ClassFactoryImpl_CreateInstance(IClassFactory *iface,
    IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    ClassFactoryImpl *This = impl_from_IClassFactory(iface);

    *ppv = NULL;

    if (pUnkOuter) return CLASS_E_NOAGGREGATION;

    return This->info->constructor(riid, ppv);
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

static HRESULT ClassFactoryImpl_Constructor(const classinfo *info, REFIID riid, LPVOID *ppv)
{
    ClassFactoryImpl *This;
    HRESULT ret;

    *ppv = NULL;

    This = malloc(sizeof(ClassFactoryImpl));
    if (!This) return E_OUTOFMEMORY;

    This->IClassFactory_iface.lpVtbl = &ClassFactoryImpl_Vtbl;
    This->ref = 1;
    This->info = info;

    ret = IClassFactory_QueryInterface(&This->IClassFactory_iface, riid, ppv);
    IClassFactory_Release(&This->IClassFactory_iface);

    return ret;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    HRESULT ret;
    const classinfo *info=NULL;
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
        ret = WIC_DllGetClassObject(rclsid, iid, ppv);

    TRACE("<-- %08lX\n", ret);
    return ret;
}

HRESULT create_instance(const CLSID *clsid, const IID *iid, void **ppv)
{
    int i;

    for (i=0; wic_classes[i].classid; i++)
        if (IsEqualCLSID(wic_classes[i].classid, clsid))
            return wic_classes[i].constructor(iid, ppv);

    return CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, iid, ppv);
}
