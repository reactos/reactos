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

#include <stdio.h>
//#include <stdarg.h>
//#include <math.h>

#define WIN32_NO_STATUS
#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define COBJMACROS

#include <windef.h>
#include <winbase.h>
#include <objbase.h>
//#include "wincodec.h"
#include <wincodecsdk.h>
#include <wine/test.h>

#include <initguid.h>
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);

static HRESULT get_component_info(const GUID *clsid, IWICComponentInfo **result)
{
    IWICImagingFactory *factory;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return hr;

    hr = IWICImagingFactory_CreateComponentInfo(factory, clsid, result);

    IWICImagingFactory_Release(factory);

    return hr;
}

static BOOL is_pixelformat(GUID *format)
{
    IWICComponentInfo *info;
    HRESULT hr;
    WICComponentType componenttype;

    hr = get_component_info(format, &info);
    if (FAILED(hr))
        return FALSE;

    hr = IWICComponentInfo_GetComponentType(info, &componenttype);

    IWICComponentInfo_Release(info);

    return SUCCEEDED(hr) && componenttype == WICPixelFormat;
}

static void test_decoder_info(void)
{
    IWICComponentInfo *info;
    IWICBitmapDecoderInfo *decoder_info;
    HRESULT hr;
    ULONG len;
    WCHAR value[256];
    const WCHAR expected_mimetype[] = {'i','m','a','g','e','/','b','m','p',0};
    const WCHAR expected_extensions[] = {'.','b','m','p',',','.','d','i','b',',','.','r','l','e',0};
    CLSID clsid;
    GUID pixelformats[20];
    UINT num_formats, count;
    int i;

    hr = get_component_info(&CLSID_WICBmpDecoder, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%x\n", hr);

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICBitmapDecoderInfo, (void**)&decoder_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetCLSID(decoder_info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetCLSID(decoder_info, &clsid);
    ok(hr == S_OK, "GetCLSID failed, hr=%x\n", hr);
    ok(IsEqualGUID(&CLSID_WICBmpDecoder, &clsid), "GetCLSID returned wrong result\n");

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 1, NULL, &len);
    ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, len, value, NULL);
    ok(hr == E_INVALIDARG, "GetMimeType failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 0, NULL, &len);
    ok(hr == S_OK, "GetMimeType failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    value[0] = 0;
    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, len, value, &len);
    ok(hr == S_OK, "GetMimeType failed, hr=%x\n", hr);
    ok(lstrcmpW(value, expected_mimetype) == 0, "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 1, value, &len);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetMimeType failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetMimeTypes(decoder_info, 256, value, &len);
    ok(hr == S_OK, "GetMimeType failed, hr=%x\n", hr);
    ok(lstrcmpW(value, expected_mimetype) == 0, "GetMimeType returned wrong value %s\n", wine_dbgstr_w(value));
    ok(len == lstrlenW(expected_mimetype)+1, "GetMimeType returned wrong len %i\n", len);

    num_formats = 0xdeadbeef;
    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 0, NULL, &num_formats);
    ok(hr == S_OK, "GetPixelFormats failed, hr=%x\n", hr);
    ok(num_formats < 20 && num_formats > 1, "got %d formats\n", num_formats);

    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetPixelFormats failed, hr=%x\n", hr);

    count = 0xdeadbeef;
    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 0, pixelformats, &count);
    ok(hr == S_OK, "GetPixelFormats failed, hr=%x\n", hr);
    ok(count == 0, "got %d formats\n", count);

    count = 0xdeadbeef;
    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 1, pixelformats, &count);
    ok(hr == S_OK, "GetPixelFormats failed, hr=%x\n", hr);
    ok(count == 1, "got %d formats\n", count);
    ok(is_pixelformat(&pixelformats[0]), "got invalid pixel format\n");

    count = 0xdeadbeef;
    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, num_formats, pixelformats, &count);
    ok(hr == S_OK, "GetPixelFormats failed, hr=%x\n", hr);
    ok(count == num_formats, "got %d formats, expected %d\n", count, num_formats);
    for (i=0; i<num_formats; i++)
        ok(is_pixelformat(&pixelformats[i]), "got invalid pixel format\n");

    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, num_formats, pixelformats, NULL);
    ok(hr == E_INVALIDARG, "GetPixelFormats failed, hr=%x\n", hr);

    count = 0xdeadbeef;
    hr = IWICBitmapDecoderInfo_GetPixelFormats(decoder_info, 20, pixelformats, &count);
    ok(hr == S_OK, "GetPixelFormats failed, hr=%x\n", hr);
    ok(count == num_formats, "got %d formats, expected %d\n", count, num_formats);

    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 1, NULL, &len);
    ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_extensions)+1, "GetFileExtensions returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, len, value, NULL);
    ok(hr == E_INVALIDARG, "GetFileExtensions failed, hr=%x\n", hr);

    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 0, NULL, &len);
    ok(hr == S_OK, "GetFileExtensions failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_extensions)+1, "GetFileExtensions returned wrong len %i\n", len);

    value[0] = 0;
    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, len, value, &len);
    ok(hr == S_OK, "GetFileExtensions failed, hr=%x\n", hr);
    ok(lstrcmpW(value, expected_extensions) == 0, "GetFileExtensions returned wrong value %s\n", wine_dbgstr_w(value));
    ok(len == lstrlenW(expected_extensions)+1, "GetFileExtensions returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 1, value, &len);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetFileExtensions failed, hr=%x\n", hr);
    ok(len == lstrlenW(expected_extensions)+1, "GetFileExtensions returned wrong len %i\n", len);

    hr = IWICBitmapDecoderInfo_GetFileExtensions(decoder_info, 256, value, &len);
    ok(hr == S_OK, "GetFileExtensions failed, hr=%x\n", hr);
    ok(lstrcmpW(value, expected_extensions) == 0, "GetFileExtensions returned wrong value %s\n", wine_dbgstr_w(value));
    ok(len == lstrlenW(expected_extensions)+1, "GetFileExtensions returned wrong len %i\n", len);

    IWICBitmapDecoderInfo_Release(decoder_info);

    IWICComponentInfo_Release(info);
}

static void test_pixelformat_info(void)
{
    IWICComponentInfo *info;
    IWICPixelFormatInfo *pixelformat_info;
    IWICPixelFormatInfo2 *pixelformat_info2;
    HRESULT hr;
    ULONG len, known_len;
    WCHAR value[256];
    GUID guid;
    WICComponentType componenttype;
    WICPixelFormatNumericRepresentation numericrepresentation;
    DWORD signing;
    UINT uiresult;
    BYTE abbuffer[256];
    BOOL supportstransparency;

    hr = get_component_info(&GUID_WICPixelFormat32bppBGRA, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%x\n", hr);

    if (FAILED(hr))
        return;

    hr = IWICComponentInfo_GetAuthor(info, 0, NULL, 0);
    ok(hr == E_INVALIDARG, "GetAuthor failed, hr=%x\n", hr);

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetAuthor(info, 0, NULL, &len);
    ok(hr == S_OK, "GetAuthor failed, hr=%x\n", hr);
    ok(len < 255 && len > 0, "invalid length 0x%x\n", len);
    known_len = len;

    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, len-1, value, NULL);
    ok(hr == E_INVALIDARG, "GetAuthor failed, hr=%x\n", hr);
    ok(value[0] == 0xaaaa, "string modified\n");

    len = 0xdeadbeef;
    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, known_len-1, value, &len);
    ok(hr == WINCODEC_ERR_INSUFFICIENTBUFFER, "GetAuthor failed, hr=%x\n", hr);
    ok(len == known_len, "got length of 0x%x, expected 0x%x\n", len, known_len);
    ok(value[known_len-1] == 0xaaaa, "string modified past given length\n");
    ok(value[0] == 0xaaaa, "string modified\n");

    len = 0xdeadbeef;
    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, known_len, value, &len);
    ok(hr == S_OK, "GetAuthor failed, hr=%x\n", hr);
    ok(len == known_len, "got length of 0x%x, expected 0x%x\n", len, known_len);
    ok(value[known_len-1] == 0, "string not terminated at expected length\n");
    ok(value[known_len-2] != 0xaaaa, "string not modified at given length\n");

    len = 0xdeadbeef;
    memset(value, 0xaa, 256 * sizeof(WCHAR));
    hr = IWICComponentInfo_GetAuthor(info, known_len+1, value, &len);
    ok(hr == S_OK, "GetAuthor failed, hr=%x\n", hr);
    ok(len == known_len, "got length of 0x%x, expected 0x%x\n", len, known_len);
    ok(value[known_len] == 0xaaaa, "string modified past end\n");
    ok(value[known_len-1] == 0, "string not terminated at expected length\n");
    ok(value[known_len-2] != 0xaaaa, "string not modified at given length\n");

    hr = IWICComponentInfo_GetCLSID(info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%x\n", hr);

    memset(&guid, 0xaa, sizeof(guid));
    hr = IWICComponentInfo_GetCLSID(info, &guid);
    ok(hr == S_OK, "GetCLSID failed, hr=%x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_WICPixelFormat32bppBGRA), "unexpected CLSID %s\n", wine_dbgstr_guid(&guid));

    hr = IWICComponentInfo_GetComponentType(info, NULL);
    ok(hr == E_INVALIDARG, "GetComponentType failed, hr=%x\n", hr);

    hr = IWICComponentInfo_GetComponentType(info, &componenttype);
    ok(hr == S_OK, "GetComponentType failed, hr=%x\n", hr);
    ok(componenttype == WICPixelFormat, "unexpected component type 0x%x\n", componenttype);

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetFriendlyName(info, 0, NULL, &len);
    ok(hr == S_OK, "GetFriendlyName failed, hr=%x\n", hr);
    ok(len < 255 && len > 0, "invalid length 0x%x\n", len);

    hr = IWICComponentInfo_GetSigningStatus(info, NULL);
    ok(hr == E_INVALIDARG, "GetSigningStatus failed, hr=%x\n", hr);

    hr = IWICComponentInfo_GetSigningStatus(info, &signing);
    ok(hr == S_OK, "GetSigningStatus failed, hr=%x\n", hr);
    ok(signing == WICComponentSigned, "unexpected signing status 0x%x\n", signing);

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetSpecVersion(info, 0, NULL, &len);
    ok(hr == S_OK, "GetSpecVersion failed, hr=%x\n", hr);
    ok(len == 0, "invalid length 0x%x\n", len); /* spec version does not apply to pixel formats */

    memset(&guid, 0xaa, sizeof(guid));
    hr = IWICComponentInfo_GetVendorGUID(info, &guid);
    ok(hr == S_OK, "GetVendorGUID failed, hr=%x\n", hr);
    ok(IsEqualGUID(&guid, &GUID_VendorMicrosoft) ||
       broken(IsEqualGUID(&guid, &GUID_NULL)) /* XP */, "unexpected GUID %s\n", wine_dbgstr_guid(&guid));

    len = 0xdeadbeef;
    hr = IWICComponentInfo_GetVersion(info, 0, NULL, &len);
    ok(hr == S_OK, "GetVersion failed, hr=%x\n", hr);
    ok(len == 0, "invalid length 0x%x\n", len); /* version does not apply to pixel formats */

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo, (void**)&pixelformat_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    if (SUCCEEDED(hr))
    {
        hr = IWICPixelFormatInfo_GetBitsPerPixel(pixelformat_info, NULL);
        ok(hr == E_INVALIDARG, "GetBitsPerPixel failed, hr=%x\n", hr);

        hr = IWICPixelFormatInfo_GetBitsPerPixel(pixelformat_info, &uiresult);
        ok(hr == S_OK, "GetBitsPerPixel failed, hr=%x\n", hr);
        ok(uiresult == 32, "unexpected bpp %i\n", uiresult);

        hr = IWICPixelFormatInfo_GetChannelCount(pixelformat_info, &uiresult);
        ok(hr == S_OK, "GetChannelCount failed, hr=%x\n", hr);
        ok(uiresult == 4, "unexpected channel count %i\n", uiresult);

        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 0, NULL, NULL);
        ok(hr == E_INVALIDARG, "GetChannelMask failed, hr=%x\n", hr);

        uiresult = 0xdeadbeef;
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 0, NULL, &uiresult);
        ok(hr == S_OK, "GetChannelMask failed, hr=%x\n", hr);
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, known_len, abbuffer, NULL);
        ok(hr == E_INVALIDARG, "GetChannelMask failed, hr=%x\n", hr);
        ok(abbuffer[0] == 0xaa, "buffer modified\n");

        uiresult = 0xdeadbeef;
        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 3, abbuffer, &uiresult);
        ok(hr == E_INVALIDARG, "GetChannelMask failed, hr=%x\n", hr);
        ok(abbuffer[0] == 0xaa, "buffer modified\n");
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 4, abbuffer, &uiresult);
        ok(hr == S_OK, "GetChannelMask failed, hr=%x\n", hr);
        ok(*((ULONG*)abbuffer) == 0xff, "unexpected mask 0x%x\n", *((ULONG*)abbuffer));
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(abbuffer, 0xaa, sizeof(abbuffer));
        hr = IWICPixelFormatInfo_GetChannelMask(pixelformat_info, 0, 5, abbuffer, &uiresult);
        ok(hr == S_OK, "GetChannelMask failed, hr=%x\n", hr);
        ok(*((ULONG*)abbuffer) == 0xff, "unexpected mask 0x%x\n", *((ULONG*)abbuffer));
        ok(abbuffer[4] == 0xaa, "buffer modified past actual length\n");
        ok(uiresult == 4, "unexpected length %i\n", uiresult);

        memset(&guid, 0xaa, sizeof(guid));
        hr = IWICPixelFormatInfo_GetFormatGUID(pixelformat_info, &guid);
        ok(hr == S_OK, "GetFormatGUID failed, hr=%x\n", hr);
        ok(IsEqualGUID(&guid, &GUID_WICPixelFormat32bppBGRA), "unexpected GUID %s\n", wine_dbgstr_guid(&guid));

        IWICPixelFormatInfo_Release(pixelformat_info);
    }

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICPixelFormatInfo2, (void**)&pixelformat_info2);

    if (FAILED(hr))
        win_skip("IWICPixelFormatInfo2 not supported\n");
    else
    {
        hr = IWICPixelFormatInfo2_GetNumericRepresentation(pixelformat_info2, NULL);
        ok(hr == E_INVALIDARG, "GetNumericRepresentation failed, hr=%x\n", hr);

        numericrepresentation = 0xdeadbeef;
        hr = IWICPixelFormatInfo2_GetNumericRepresentation(pixelformat_info2, &numericrepresentation);
        ok(hr == S_OK, "GetNumericRepresentation failed, hr=%x\n", hr);
        ok(numericrepresentation == WICPixelFormatNumericRepresentationUnsignedInteger, "unexpected numeric representation %i\n", numericrepresentation);

        hr = IWICPixelFormatInfo2_SupportsTransparency(pixelformat_info2, NULL);
        ok(hr == E_INVALIDARG, "SupportsTransparency failed, hr=%x\n", hr);

        supportstransparency = 0xdeadbeef;
        hr = IWICPixelFormatInfo2_SupportsTransparency(pixelformat_info2, &supportstransparency);
        ok(hr == S_OK, "SupportsTransparency failed, hr=%x\n", hr);
        ok(supportstransparency == 1, "unexpected value %i\n", supportstransparency);

        IWICPixelFormatInfo2_Release(pixelformat_info2);
    }

    IWICComponentInfo_Release(info);
}

static void test_reader_info(void)
{
    IWICImagingFactory *factory;
    IWICComponentInfo *info;
    IWICMetadataReaderInfo *reader_info;
    HRESULT hr;
    CLSID clsid;
    GUID container_formats[10];
    UINT count, size;
    WICMetadataPattern *patterns;

    hr = CoCreateInstance(&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICImagingFactory, (void**)&factory);
    ok(hr == S_OK, "CoCreateInstance failed, hr=%x\n", hr);
    if (FAILED(hr)) return;

    hr = IWICImagingFactory_CreateComponentInfo(factory, &CLSID_WICUnknownMetadataReader, &info);
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%x\n", hr);

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICMetadataReaderInfo, (void**)&reader_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%x\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, &clsid);
    ok(hr == S_OK, "GetCLSID failed, hr=%x\n", hr);
    ok(IsEqualGUID(&CLSID_WICUnknownMetadataReader, &clsid), "GetCLSID returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetMetadataFormat(reader_info, &clsid);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&GUID_MetadataFormatUnknown, &clsid), "GetMetadataFormat returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetContainerFormats failed, hr=%x\n", hr);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%x\n", hr);
    ok(count == 0, "unexpected count %d\n", count);

    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_ContainerFormatPng,
        0, NULL, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetPatterns failed, hr=%x\n", hr);

    count = size = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_ContainerFormatPng,
        0, NULL, &count, &size);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND || broken(hr == S_OK) /* Windows XP */,
        "GetPatterns failed, hr=%x\n", hr);
    ok(count == 0xdeadbeef, "unexpected count %d\n", count);
    ok(size == 0xdeadbeef, "unexpected size %d\n", size);

    IWICMetadataReaderInfo_Release(reader_info);

    IWICComponentInfo_Release(info);

    hr = IWICImagingFactory_CreateComponentInfo(factory, &CLSID_WICXMPStructMetadataReader, &info);
todo_wine
    ok(hr == S_OK, "CreateComponentInfo failed, hr=%x\n", hr);

    if (FAILED(hr))
    {
        IWICImagingFactory_Release(factory);
        return;
    }

    hr = IWICComponentInfo_QueryInterface(info, &IID_IWICMetadataReaderInfo, (void**)&reader_info);
    ok(hr == S_OK, "QueryInterface failed, hr=%x\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, NULL);
    ok(hr == E_INVALIDARG, "GetCLSID failed, hr=%x\n", hr);

    hr = IWICMetadataReaderInfo_GetCLSID(reader_info, &clsid);
    ok(hr == S_OK, "GetCLSID failed, hr=%x\n", hr);
    ok(IsEqualGUID(&CLSID_WICXMPStructMetadataReader, &clsid), "GetCLSID returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetMetadataFormat(reader_info, &clsid);
    ok(hr == S_OK, "GetMetadataFormat failed, hr=%x\n", hr);
    ok(IsEqualGUID(&GUID_MetadataFormatXMPStruct, &clsid), "GetMetadataFormat returned wrong result\n");

    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, NULL);
    ok(hr == E_INVALIDARG, "GetContainerFormats failed, hr=%x\n", hr);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 0, NULL, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%x\n", hr);
    ok(count >= 2, "unexpected count %d\n", count);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 1, container_formats, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%x\n", hr);
    ok(count == 1, "unexpected count %d\n", count);

    count = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetContainerFormats(reader_info, 10, container_formats, &count);
    ok(hr == S_OK, "GetContainerFormats failed, hr=%x\n", hr);
    ok(count == min(count, 10), "unexpected count %d\n", count);

    count = size = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_ContainerFormatPng,
        0, NULL, &count, &size);
    ok(hr == WINCODEC_ERR_COMPONENTNOTFOUND || broken(hr == S_OK) /* Windows XP */,
        "GetPatterns failed, hr=%x\n", hr);
    ok(count == 0xdeadbeef, "unexpected count %d\n", count);
    ok(size == 0xdeadbeef, "unexpected size %d\n", size);

    count = size = 0xdeadbeef;
    hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_MetadataFormatXMP,
        0, NULL, &count, &size);
    ok(hr == S_OK, "GetPatterns failed, hr=%x\n", hr);
    ok(count == 1, "unexpected count %d\n", count);
    ok(size > sizeof(WICMetadataPattern), "unexpected size %d\n", size);

    if (hr == S_OK)
    {
        patterns = HeapAlloc(GetProcessHeap(), 0, size);

        count = size = 0xdeadbeef;
        hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_MetadataFormatXMP,
            size-1, patterns, &count, &size);
        ok(hr == S_OK, "GetPatterns failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %d\n", count);
        ok(size > sizeof(WICMetadataPattern), "unexpected size %d\n", size);

        count = size = 0xdeadbeef;
        hr = IWICMetadataReaderInfo_GetPatterns(reader_info, &GUID_MetadataFormatXMP,
            size, patterns, &count, &size);
        ok(hr == S_OK, "GetPatterns failed, hr=%x\n", hr);
        ok(count == 1, "unexpected count %d\n", count);
        ok(size == sizeof(WICMetadataPattern) + patterns->Length * 2, "unexpected size %d\n", size);

        HeapFree(GetProcessHeap(), 0, patterns);
    }

    IWICMetadataReaderInfo_Release(reader_info);

    IWICComponentInfo_Release(info);

    IWICImagingFactory_Release(factory);
}

START_TEST(info)
{
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

    test_decoder_info();
    test_reader_info();
    test_pixelformat_info();

    CoUninitialize();
}
