/*
 * Windows Codecs Extensions
 *
 * Copyright 2013 Austin English
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
#include "initguid.h"
#include "wincodec.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

/******************************************************************
 * DllGetClassObject
 */
HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID iid, LPVOID *ppv)
{
    FIXME("(%s,%s,%p) stub\n", debugstr_guid(rclsid), debugstr_guid(iid), ppv);

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI WICCreateColorTransform_Proxy(IWICColorTransform **ppIWICColorTransform)
{
    HRESULT hr, init;
    IWICImagingFactory *factory;

    TRACE("(%p)\n", ppIWICColorTransform);

    if (!ppIWICColorTransform) return E_INVALIDARG;

    init = CoInitializeEx(NULL, COINIT_MULTITHREADED);

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
                          &IID_IWICImagingFactory, (void **)&factory);
    if (FAILED(hr))
    {
        if (SUCCEEDED(init)) CoUninitialize();
        return hr;
    }
    hr = IWICImagingFactory_CreateColorTransformer(factory, ppIWICColorTransform);
    IWICImagingFactory_Release(factory);

    if (SUCCEEDED(init)) CoUninitialize();
    return hr;
}

HRESULT WINAPI IWICColorTransform_Initialize_Proxy_W(IWICColorTransform *iface,
    IWICBitmapSource *pIBitmapSource, IWICColorContext *pIContextSource,
    IWICColorContext *pIContextDest, REFWICPixelFormatGUID pixelFmtDest)
{
    TRACE("(%p,%p,%p,%p,%s)\n", iface, pIBitmapSource, pIContextSource, pIContextDest,
          debugstr_guid(pixelFmtDest));

    return IWICColorTransform_Initialize(iface, pIBitmapSource, pIContextSource,
        pIContextDest, pixelFmtDest);
}
