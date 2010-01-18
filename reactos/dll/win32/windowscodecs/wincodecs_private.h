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

#ifndef WINCODECS_PRIVATE_H
#define WINCODECS_PRIVATE_H

extern HRESULT FormatConverter_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT ImagingFactory_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT BmpDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT PngDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv);
extern HRESULT PngEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv);
extern HRESULT BmpEncoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv);
extern HRESULT GifDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID riid, void** ppv);
extern HRESULT IcoDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv);
extern HRESULT JpegDecoder_CreateInstance(IUnknown *pUnkOuter, REFIID iid, void** ppv);

extern HRESULT PaletteImpl_Create(IWICPalette **palette);
extern HRESULT StreamImpl_Create(IWICStream **stream);

extern HRESULT copy_pixels(UINT bpp, const BYTE *srcbuffer,
    UINT srcwidth, UINT srcheight, INT srcstride,
    const WICRect *rc, UINT dststride, UINT dstbuffersize, BYTE *dstbuffer);

extern HRESULT CreatePropertyBag2(IPropertyBag2 **ppPropertyBag2);

extern HRESULT CreateComponentInfo(REFCLSID clsid, IWICComponentInfo **ppIInfo);
extern HRESULT CreateComponentEnumerator(DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown);

#endif /* WINCODECS_PRIVATE_H */
