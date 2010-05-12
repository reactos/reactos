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
#include "wincodec.h"

#include "wincodecs_private.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

static WCHAR const pixelformats_keyname[] = {'P','i','x','e','l','F','o','r','m','a','t','s',0};

typedef struct {
    const IWICBitmapDecoderInfoVtbl *lpIWICBitmapDecoderInfoVtbl;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} BitmapDecoderInfo;

static HRESULT WINAPI BitmapDecoderInfo_QueryInterface(IWICBitmapDecoderInfo *iface, REFIID iid,
    void **ppv)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICComponentInfo, iid) ||
        IsEqualIID(&IID_IWICBitmapCodecInfo, iid) ||
        IsEqualIID(&IID_IWICBitmapDecoderInfo ,iid))
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

static ULONG WINAPI BitmapDecoderInfo_AddRef(IWICBitmapDecoderInfo *iface)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapDecoderInfo_Release(IWICBitmapDecoderInfo *iface)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        RegCloseKey(This->classkey);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapDecoderInfo_GetComponentType(IWICBitmapDecoderInfo *iface,
    WICComponentType *pType)
{
    TRACE("(%p,%p)\n", iface, pType);
    *pType = WICDecoder;
    return S_OK;
}

static HRESULT WINAPI BitmapDecoderInfo_GetCLSID(IWICBitmapDecoderInfo *iface, CLSID *pclsid)
{
    FIXME("(%p,%p): stub\n", iface, pclsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetSigningStatus(IWICBitmapDecoderInfo *iface, DWORD *pStatus)
{
    FIXME("(%p,%p): stub\n", iface, pStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetAuthor(IWICBitmapDecoderInfo *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchAuthor, wzAuthor, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetVendorGUID(IWICBitmapDecoderInfo *iface, GUID *pguidVendor)
{
    FIXME("(%p,%p): stub\n", iface, pguidVendor);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetVersion(IWICBitmapDecoderInfo *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchVersion, wzVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetSpecVersion(IWICBitmapDecoderInfo *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetFriendlyName(IWICBitmapDecoderInfo *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetContainerFormat(IWICBitmapDecoderInfo *iface,
    GUID *pguidContainerFormat)
{
    FIXME("(%p,%p): stub\n", iface, pguidContainerFormat);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetPixelFormats(IWICBitmapDecoderInfo *iface,
    UINT cFormats, GUID *pguidPixelFormats, UINT *pcActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cFormats, pguidPixelFormats, pcActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetColorManagementVersion(IWICBitmapDecoderInfo *iface,
    UINT cchColorManagementVersion, WCHAR *wzColorManagementVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchColorManagementVersion, wzColorManagementVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetDeviceManufacturer(IWICBitmapDecoderInfo *iface,
    UINT cchDeviceManufacturer, WCHAR *wzDeviceManufacturer, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchDeviceManufacturer, wzDeviceManufacturer, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetDeviceModels(IWICBitmapDecoderInfo *iface,
    UINT cchDeviceModels, WCHAR *wzDeviceModels, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchDeviceModels, wzDeviceModels, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetMimeTypes(IWICBitmapDecoderInfo *iface,
    UINT cchMimeTypes, WCHAR *wzMimeTypes, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchMimeTypes, wzMimeTypes, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetFileExtensions(IWICBitmapDecoderInfo *iface,
    UINT cchFileExtensions, WCHAR *wzFileExtensions, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchFileExtensions, wzFileExtensions, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportAnimation(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportAnimation)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportAnimation);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportChromaKey(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportChromaKey)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportChromaKey);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportLossless(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportLossless)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportLossless);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_DoesSupportMultiframe(IWICBitmapDecoderInfo *iface,
    BOOL *pfSupportMultiframe)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportMultiframe);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_MatchesMimeType(IWICBitmapDecoderInfo *iface,
    LPCWSTR wzMimeType, BOOL *pfMatches)
{
    FIXME("(%p,%s,%p): stub\n", iface, debugstr_w(wzMimeType), pfMatches);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetPatterns(IWICBitmapDecoderInfo *iface,
    UINT cbSizePatterns, WICBitmapPattern *pPatterns, UINT *pcPatterns, UINT *pcbPatternsActual)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;
    UINT pattern_count=0, patterns_size=0;
    WCHAR subkeyname[11];
    LONG res;
    HKEY patternskey, patternkey;
    static const WCHAR uintformatW[] = {'%','u',0};
    static const WCHAR patternsW[] = {'P','a','t','t','e','r','n','s',0};
    static const WCHAR positionW[] = {'P','o','s','i','t','i','o','n',0};
    static const WCHAR lengthW[] = {'L','e','n','g','t','h',0};
    static const WCHAR patternW[] = {'P','a','t','t','e','r','n',0};
    static const WCHAR maskW[] = {'M','a','s','k',0};
    static const WCHAR endofstreamW[] = {'E','n','d','O','f','S','t','r','e','a','m',0};
    HRESULT hr=S_OK;
    UINT i;
    BYTE *bPatterns=(BYTE*)pPatterns;
    DWORD length, valuesize;

    TRACE("(%p,%i,%p,%p,%p)\n", iface, cbSizePatterns, pPatterns, pcPatterns, pcbPatternsActual);

    res = RegOpenKeyExW(This->classkey, patternsW, 0, KEY_READ, &patternskey);
    if (res != ERROR_SUCCESS) return HRESULT_FROM_WIN32(res);

    res = RegQueryInfoKeyW(patternskey, NULL, NULL, NULL, &pattern_count, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
    if (res == ERROR_SUCCESS)
    {
        patterns_size = pattern_count * sizeof(WICBitmapPattern);

        for (i=0; i<pattern_count; i++)
        {
            snprintfW(subkeyname, 11, uintformatW, i);
            res = RegOpenKeyExW(patternskey, subkeyname, 0, KEY_READ, &patternkey);
            if (res == ERROR_SUCCESS)
            {
                valuesize = sizeof(ULONG);
                res = RegGetValueW(patternkey, NULL, lengthW, RRF_RT_DWORD, NULL,
                    &length, &valuesize);
                patterns_size += length*2;

                if ((cbSizePatterns >= patterns_size) && (res == ERROR_SUCCESS))
                {
                    pPatterns[i].Length = length;

                    pPatterns[i].EndOfStream = 0;
                    valuesize = sizeof(BOOL);
                    RegGetValueW(patternkey, NULL, endofstreamW, RRF_RT_DWORD, NULL,
                        &pPatterns[i].EndOfStream, &valuesize);

                    pPatterns[i].Position.QuadPart = 0;
                    valuesize = sizeof(ULARGE_INTEGER);
                    res = RegGetValueW(patternkey, NULL, positionW, RRF_RT_DWORD|RRF_RT_QWORD, NULL,
                        &pPatterns[i].Position, &valuesize);

                    if (res == ERROR_SUCCESS)
                    {
                        pPatterns[i].Pattern = bPatterns+patterns_size-length*2;
                        valuesize = length;
                        res = RegGetValueW(patternkey, NULL, patternW, RRF_RT_REG_BINARY, NULL,
                            pPatterns[i].Pattern, &valuesize);
                    }

                    if (res == ERROR_SUCCESS)
                    {
                        pPatterns[i].Mask = bPatterns+patterns_size-length;
                        valuesize = length;
                        res = RegGetValueW(patternkey, NULL, maskW, RRF_RT_REG_BINARY, NULL,
                            pPatterns[i].Mask, &valuesize);
                    }
                }

                RegCloseKey(patternkey);
            }
            if (res != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(res);
                break;
            }
        }
    }
    else hr = HRESULT_FROM_WIN32(res);

    RegCloseKey(patternskey);

    if (hr == S_OK)
    {
        *pcPatterns = pattern_count;
        *pcbPatternsActual = patterns_size;
        if (pPatterns && cbSizePatterns < patterns_size)
            hr = WINCODEC_ERR_INSUFFICIENTBUFFER;
    }

    return hr;
}

static HRESULT WINAPI BitmapDecoderInfo_MatchesPattern(IWICBitmapDecoderInfo *iface,
    IStream *pIStream, BOOL *pfMatches)
{
    WICBitmapPattern *patterns;
    UINT pattern_count=0, patterns_size=0;
    HRESULT hr;
    int i, pos;
    BYTE *data=NULL;
    ULONG datasize=0;
    ULONG bytesread;
    LARGE_INTEGER seekpos;

    TRACE("(%p,%p,%p)\n", iface, pIStream, pfMatches);

    hr = BitmapDecoderInfo_GetPatterns(iface, 0, NULL, &pattern_count, &patterns_size);
    if (FAILED(hr)) return hr;

    patterns = HeapAlloc(GetProcessHeap(), 0, patterns_size);
    if (!patterns) return E_OUTOFMEMORY;

    hr = BitmapDecoderInfo_GetPatterns(iface, patterns_size, patterns, &pattern_count, &patterns_size);
    if (FAILED(hr)) goto end;

    for (i=0; i<pattern_count; i++)
    {
        if (datasize < patterns[i].Length)
        {
            HeapFree(GetProcessHeap(), 0, data);
            datasize = patterns[i].Length;
            data = HeapAlloc(GetProcessHeap(), 0, patterns[i].Length);
            if (!data)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
        }

        if (patterns[i].EndOfStream)
            seekpos.QuadPart = -patterns[i].Position.QuadPart;
        else
            seekpos.QuadPart = patterns[i].Position.QuadPart;
        hr = IStream_Seek(pIStream, seekpos, patterns[i].EndOfStream ? STREAM_SEEK_END : STREAM_SEEK_SET, NULL);
        if (hr == STG_E_INVALIDFUNCTION) continue; /* before start of stream */
        if (FAILED(hr)) break;

        hr = IStream_Read(pIStream, data, patterns[i].Length, &bytesread);
        if (hr == S_FALSE || (hr == S_OK && bytesread != patterns[i].Length)) /* past end of stream */
            continue;
        if (FAILED(hr)) break;

        for (pos=0; pos<patterns[i].Length; pos++)
        {
            if ((data[pos] & patterns[i].Mask[pos]) != patterns[i].Pattern[pos])
                break;
        }
        if (pos == patterns[i].Length) /* matches pattern */
        {
            hr = S_OK;
            *pfMatches = TRUE;
            break;
        }
    }

    if (i == pattern_count) /* does not match any pattern */
    {
        hr = S_OK;
        *pfMatches = FALSE;
    }

end:
    HeapFree(GetProcessHeap(), 0, patterns);
    HeapFree(GetProcessHeap(), 0, data);

    return hr;
}

static HRESULT WINAPI BitmapDecoderInfo_CreateInstance(IWICBitmapDecoderInfo *iface,
    IWICBitmapDecoder **ppIBitmapDecoder)
{
    BitmapDecoderInfo *This = (BitmapDecoderInfo*)iface;

    TRACE("(%p,%p)\n", iface, ppIBitmapDecoder);

    return CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapDecoder, (void**)ppIBitmapDecoder);
}

static const IWICBitmapDecoderInfoVtbl BitmapDecoderInfo_Vtbl = {
    BitmapDecoderInfo_QueryInterface,
    BitmapDecoderInfo_AddRef,
    BitmapDecoderInfo_Release,
    BitmapDecoderInfo_GetComponentType,
    BitmapDecoderInfo_GetCLSID,
    BitmapDecoderInfo_GetSigningStatus,
    BitmapDecoderInfo_GetAuthor,
    BitmapDecoderInfo_GetVendorGUID,
    BitmapDecoderInfo_GetVersion,
    BitmapDecoderInfo_GetSpecVersion,
    BitmapDecoderInfo_GetFriendlyName,
    BitmapDecoderInfo_GetContainerFormat,
    BitmapDecoderInfo_GetPixelFormats,
    BitmapDecoderInfo_GetColorManagementVersion,
    BitmapDecoderInfo_GetDeviceManufacturer,
    BitmapDecoderInfo_GetDeviceModels,
    BitmapDecoderInfo_GetMimeTypes,
    BitmapDecoderInfo_GetFileExtensions,
    BitmapDecoderInfo_DoesSupportAnimation,
    BitmapDecoderInfo_DoesSupportChromaKey,
    BitmapDecoderInfo_DoesSupportLossless,
    BitmapDecoderInfo_DoesSupportMultiframe,
    BitmapDecoderInfo_MatchesMimeType,
    BitmapDecoderInfo_GetPatterns,
    BitmapDecoderInfo_MatchesPattern,
    BitmapDecoderInfo_CreateInstance
};

static HRESULT BitmapDecoderInfo_Constructor(HKEY classkey, REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    BitmapDecoderInfo *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapDecoderInfo));
    if (!This)
    {
        RegCloseKey(classkey);
        return E_OUTOFMEMORY;
    }

    This->lpIWICBitmapDecoderInfoVtbl = &BitmapDecoderInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

typedef struct {
    const IWICFormatConverterInfoVtbl *lpIWICFormatConverterInfoVtbl;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} FormatConverterInfo;

static HRESULT WINAPI FormatConverterInfo_QueryInterface(IWICFormatConverterInfo *iface, REFIID iid,
    void **ppv)
{
    FormatConverterInfo *This = (FormatConverterInfo*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICComponentInfo, iid) ||
        IsEqualIID(&IID_IWICFormatConverterInfo ,iid))
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

static ULONG WINAPI FormatConverterInfo_AddRef(IWICFormatConverterInfo *iface)
{
    FormatConverterInfo *This = (FormatConverterInfo*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI FormatConverterInfo_Release(IWICFormatConverterInfo *iface)
{
    FormatConverterInfo *This = (FormatConverterInfo*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        RegCloseKey(This->classkey);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI FormatConverterInfo_GetComponentType(IWICFormatConverterInfo *iface,
    WICComponentType *pType)
{
    TRACE("(%p,%p)\n", iface, pType);
    *pType = WICPixelFormatConverter;
    return S_OK;
}

static HRESULT WINAPI FormatConverterInfo_GetCLSID(IWICFormatConverterInfo *iface, CLSID *pclsid)
{
    FIXME("(%p,%p): stub\n", iface, pclsid);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetSigningStatus(IWICFormatConverterInfo *iface, DWORD *pStatus)
{
    FIXME("(%p,%p): stub\n", iface, pStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetAuthor(IWICFormatConverterInfo *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchAuthor, wzAuthor, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetVendorGUID(IWICFormatConverterInfo *iface, GUID *pguidVendor)
{
    FIXME("(%p,%p): stub\n", iface, pguidVendor);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetVersion(IWICFormatConverterInfo *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchVersion, wzVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetSpecVersion(IWICFormatConverterInfo *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetFriendlyName(IWICFormatConverterInfo *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetPixelFormats(IWICFormatConverterInfo *iface,
    UINT cFormats, GUID *pguidPixelFormats, UINT *pcActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cFormats, pguidPixelFormats, pcActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_CreateInstance(IWICFormatConverterInfo *iface,
    IWICFormatConverter **ppIFormatConverter)
{
    FormatConverterInfo *This = (FormatConverterInfo*)iface;

    TRACE("(%p,%p)\n", iface, ppIFormatConverter);

    return CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICFormatConverter, (void**)ppIFormatConverter);
}

static BOOL ConverterSupportsFormat(IWICFormatConverterInfo *iface, const WCHAR *formatguid)
{
    LONG res;
    FormatConverterInfo *This = (FormatConverterInfo*)iface;
    HKEY formats_key, guid_key;

    /* Avoid testing using IWICFormatConverter_GetPixelFormats because that
        would be O(n). A registry test should do better. */

    res = RegOpenKeyExW(This->classkey, pixelformats_keyname, 0, KEY_READ, &formats_key);
    if (res != ERROR_SUCCESS) return FALSE;

    res = RegOpenKeyExW(formats_key, formatguid, 0, KEY_READ, &guid_key);
    if (res == ERROR_SUCCESS) RegCloseKey(guid_key);

    RegCloseKey(formats_key);

    return (res == ERROR_SUCCESS);
}

static const IWICFormatConverterInfoVtbl FormatConverterInfo_Vtbl = {
    FormatConverterInfo_QueryInterface,
    FormatConverterInfo_AddRef,
    FormatConverterInfo_Release,
    FormatConverterInfo_GetComponentType,
    FormatConverterInfo_GetCLSID,
    FormatConverterInfo_GetSigningStatus,
    FormatConverterInfo_GetAuthor,
    FormatConverterInfo_GetVendorGUID,
    FormatConverterInfo_GetVersion,
    FormatConverterInfo_GetSpecVersion,
    FormatConverterInfo_GetFriendlyName,
    FormatConverterInfo_GetPixelFormats,
    FormatConverterInfo_CreateInstance
};

static HRESULT FormatConverterInfo_Constructor(HKEY classkey, REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    FormatConverterInfo *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(FormatConverterInfo));
    if (!This)
    {
        RegCloseKey(classkey);
        return E_OUTOFMEMORY;
    }

    This->lpIWICFormatConverterInfoVtbl = &FormatConverterInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

static WCHAR const clsid_keyname[] = {'C','L','S','I','D',0};
static WCHAR const instance_keyname[] = {'I','n','s','t','a','n','c','e',0};

struct category {
    WICComponentType type;
    const GUID *catid;
    HRESULT (*constructor)(HKEY,REFCLSID,IWICComponentInfo**);
};

static const struct category categories[] = {
    {WICDecoder, &CATID_WICBitmapDecoders, BitmapDecoderInfo_Constructor},
    {WICPixelFormatConverter, &CATID_WICFormatConverters, FormatConverterInfo_Constructor},
    {0}
};

HRESULT CreateComponentInfo(REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    HKEY clsidkey;
    HKEY classkey;
    HKEY catidkey;
    HKEY instancekey;
    WCHAR guidstring[39];
    LONG res;
    const struct category *category;
    int found=0;
    HRESULT hr;

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, KEY_READ, &clsidkey);
    if (res != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(res);

    for (category=categories; category->type; category++)
    {
        StringFromGUID2(category->catid, guidstring, 39);
        res = RegOpenKeyExW(clsidkey, guidstring, 0, KEY_READ, &catidkey);
        if (res == ERROR_SUCCESS)
        {
            res = RegOpenKeyExW(catidkey, instance_keyname, 0, KEY_READ, &instancekey);
            if (res == ERROR_SUCCESS)
            {
                StringFromGUID2(clsid, guidstring, 39);
                res = RegOpenKeyExW(instancekey, guidstring, 0, KEY_READ, &classkey);
                if (res == ERROR_SUCCESS)
                {
                    RegCloseKey(classkey);
                    found = 1;
                }
                RegCloseKey(instancekey);
            }
            RegCloseKey(catidkey);
        }
        if (found) break;
    }

    if (found)
    {
        res = RegOpenKeyExW(clsidkey, guidstring, 0, KEY_READ, &classkey);
        if (res == ERROR_SUCCESS)
            hr = category->constructor(classkey, clsid, ppIInfo);
        else
            hr = HRESULT_FROM_WIN32(res);
    }
    else
        hr = E_FAIL;

    RegCloseKey(clsidkey);

    return hr;
}

typedef struct {
    const IEnumUnknownVtbl *IEnumUnknown_Vtbl;
    LONG ref;
    struct list objects;
    struct list *cursor;
    CRITICAL_SECTION lock; /* Must be held when reading or writing cursor */
} ComponentEnum;

typedef struct {
    struct list entry;
    IUnknown *unk;
} ComponentEnumItem;

static const IEnumUnknownVtbl ComponentEnumVtbl;

static HRESULT WINAPI ComponentEnum_QueryInterface(IEnumUnknown *iface, REFIID iid,
    void **ppv)
{
    ComponentEnum *This = (ComponentEnum*)iface;
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) || IsEqualIID(&IID_IEnumUnknown, iid))
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

static ULONG WINAPI ComponentEnum_AddRef(IEnumUnknown *iface)
{
    ComponentEnum *This = (ComponentEnum*)iface;
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ComponentEnum_Release(IEnumUnknown *iface)
{
    ComponentEnum *This = (ComponentEnum*)iface;
    ULONG ref = InterlockedDecrement(&This->ref);
    ComponentEnumItem *cursor, *cursor2;

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &This->objects, ComponentEnumItem, entry)
        {
            IUnknown_Release(cursor->unk);
            list_remove(&cursor->entry);
            HeapFree(GetProcessHeap(), 0, cursor);
        }
        This->lock.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&This->lock);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI ComponentEnum_Next(IEnumUnknown *iface, ULONG celt,
    IUnknown **rgelt, ULONG *pceltFetched)
{
    ComponentEnum *This = (ComponentEnum*)iface;
    int num_fetched=0;
    ComponentEnumItem *item;
    HRESULT hr=S_OK;

    TRACE("(%p,%u,%p,%p)\n", iface, celt, rgelt, pceltFetched);

    EnterCriticalSection(&This->lock);
    while (num_fetched<celt)
    {
        if (!This->cursor)
        {
            hr = S_FALSE;
            break;
        }
        item = LIST_ENTRY(This->cursor, ComponentEnumItem, entry);
        IUnknown_AddRef(item->unk);
        rgelt[num_fetched] = item->unk;
        num_fetched++;
        This->cursor = list_next(&This->objects, This->cursor);
    }
    LeaveCriticalSection(&This->lock);
    *pceltFetched = num_fetched;
    return hr;
}

static HRESULT WINAPI ComponentEnum_Skip(IEnumUnknown *iface, ULONG celt)
{
    ComponentEnum *This = (ComponentEnum*)iface;
    int i;
    HRESULT hr=S_OK;

    TRACE("(%p,%u)\n", iface, celt);

    EnterCriticalSection(&This->lock);
    for (i=0; i<celt; i++)
    {
        if (!This->cursor)
        {
            hr = S_FALSE;
            break;
        }
        This->cursor = list_next(&This->objects, This->cursor);
    }
    LeaveCriticalSection(&This->lock);
    return hr;
}

static HRESULT WINAPI ComponentEnum_Reset(IEnumUnknown *iface)
{
    ComponentEnum *This = (ComponentEnum*)iface;

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);
    This->cursor = list_head(&This->objects);
    LeaveCriticalSection(&This->lock);
    return S_OK;
}

static HRESULT WINAPI ComponentEnum_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    ComponentEnum *This = (ComponentEnum*)iface;
    ComponentEnum *new_enum;
    ComponentEnumItem *old_item, *new_item;
    HRESULT ret=S_OK;
    struct list *old_cursor;

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(ComponentEnum));
    if (!new_enum)
    {
        *ppenum = NULL;
        return E_OUTOFMEMORY;
    }

    new_enum->IEnumUnknown_Vtbl = &ComponentEnumVtbl;
    new_enum->ref = 1;
    new_enum->cursor = NULL;
    list_init(&new_enum->objects);
    InitializeCriticalSection(&new_enum->lock);
    new_enum->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ComponentEnum.lock");

    EnterCriticalSection(&This->lock);
    old_cursor = This->cursor;
    LeaveCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY(old_item, &This->objects, ComponentEnumItem, entry)
    {
        new_item = HeapAlloc(GetProcessHeap(), 0, sizeof(ComponentEnumItem));
        if (!new_item)
        {
            ret = E_OUTOFMEMORY;
            break;
        }
        new_item->unk = old_item->unk;
        list_add_tail(&new_enum->objects, &new_item->entry);
        IUnknown_AddRef(new_item->unk);
        if (&old_item->entry == old_cursor) new_enum->cursor = &new_item->entry;
    }

    if (FAILED(ret))
    {
        IUnknown_Release((IUnknown*)new_enum);
        *ppenum = NULL;
    }
    else
        *ppenum = (IEnumUnknown*)new_enum;

    return ret;
}

static const IEnumUnknownVtbl ComponentEnumVtbl = {
    ComponentEnum_QueryInterface,
    ComponentEnum_AddRef,
    ComponentEnum_Release,
    ComponentEnum_Next,
    ComponentEnum_Skip,
    ComponentEnum_Reset,
    ComponentEnum_Clone
};

HRESULT CreateComponentEnumerator(DWORD componentTypes, DWORD options, IEnumUnknown **ppIEnumUnknown)
{
    ComponentEnum *This;
    ComponentEnumItem *item;
    const struct category *category;
    HKEY clsidkey, catidkey, instancekey;
    WCHAR guidstring[39];
    LONG res;
    int i;
    HRESULT hr=S_OK;
    CLSID clsid;

    if (options) FIXME("ignoring flags %x\n", options);

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, clsid_keyname, 0, KEY_READ, &clsidkey);
    if (res != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(res);

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(ComponentEnum));
    if (!This)
    {
        RegCloseKey(clsidkey);
        return E_OUTOFMEMORY;
    }

    This->IEnumUnknown_Vtbl = &ComponentEnumVtbl;
    This->ref = 1;
    list_init(&This->objects);
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": ComponentEnum.lock");

    for (category=categories; category->type && hr == S_OK; category++)
    {
        if ((category->type & componentTypes) == 0) continue;
        StringFromGUID2(category->catid, guidstring, 39);
        res = RegOpenKeyExW(clsidkey, guidstring, 0, KEY_READ, &catidkey);
        if (res == ERROR_SUCCESS)
        {
            res = RegOpenKeyExW(catidkey, instance_keyname, 0, KEY_READ, &instancekey);
            if (res == ERROR_SUCCESS)
            {
                i=0;
                for (;;i++)
                {
                    DWORD guidstring_size = 39;
                    res = RegEnumKeyExW(instancekey, i, guidstring, &guidstring_size, NULL, NULL, NULL, NULL);
                    if (res != ERROR_SUCCESS) break;

                    item = HeapAlloc(GetProcessHeap(), 0, sizeof(ComponentEnumItem));
                    if (!item) { hr = E_OUTOFMEMORY; break; }

                    hr = CLSIDFromString(guidstring, &clsid);
                    if (SUCCEEDED(hr))
                    {
                        hr = CreateComponentInfo(&clsid, (IWICComponentInfo**)&item->unk);
                        if (SUCCEEDED(hr))
                            list_add_tail(&This->objects, &item->entry);
                    }

                    if (FAILED(hr))
                    {
                        HeapFree(GetProcessHeap(), 0, item);
                        hr = S_OK;
                    }
                }
                RegCloseKey(instancekey);
            }
            RegCloseKey(catidkey);
        }
        if (res != ERROR_SUCCESS && res != ERROR_NO_MORE_ITEMS)
            hr = HRESULT_FROM_WIN32(res);
    }
    RegCloseKey(clsidkey);

    if (SUCCEEDED(hr))
    {
        IEnumUnknown_Reset((IEnumUnknown*)This);
        *ppIEnumUnknown = (IEnumUnknown*)This;
    }
    else
    {
        *ppIEnumUnknown = NULL;
        IUnknown_Release((IUnknown*)This);
    }

    return hr;
}

HRESULT WINAPI WICConvertBitmapSource(REFWICPixelFormatGUID dstFormat, IWICBitmapSource *pISrc, IWICBitmapSource **ppIDst)
{
    HRESULT res;
    IEnumUnknown *enumconverters;
    IUnknown *unkconverterinfo;
    IWICFormatConverterInfo *converterinfo=NULL;
    IWICFormatConverter *converter=NULL;
    GUID srcFormat;
    WCHAR srcformatstr[39], dstformatstr[39];
    BOOL canconvert;
    ULONG num_fetched;

    res = IWICBitmapSource_GetPixelFormat(pISrc, &srcFormat);
    if (FAILED(res)) return res;

    if (IsEqualGUID(&srcFormat, dstFormat))
    {
        IWICBitmapSource_AddRef(pISrc);
        *ppIDst = pISrc;
        return S_OK;
    }

    StringFromGUID2(&srcFormat, srcformatstr, 39);
    StringFromGUID2(dstFormat, dstformatstr, 39);

    res = CreateComponentEnumerator(WICPixelFormatConverter, 0, &enumconverters);
    if (FAILED(res)) return res;

    while (!converter)
    {
        res = IEnumUnknown_Next(enumconverters, 1, &unkconverterinfo, &num_fetched);

        if (res == S_OK)
        {
            res = IUnknown_QueryInterface(unkconverterinfo, &IID_IWICFormatConverterInfo, (void**)&converterinfo);

            if (SUCCEEDED(res))
            {
                canconvert = ConverterSupportsFormat(converterinfo, srcformatstr);

                if (canconvert)
                    canconvert = ConverterSupportsFormat(converterinfo, dstformatstr);

                if (canconvert)
                {
                    res = IWICFormatConverterInfo_CreateInstance(converterinfo, &converter);

                    if (SUCCEEDED(res))
                        res = IWICFormatConverter_CanConvert(converter, &srcFormat, dstFormat, &canconvert);

                    if (SUCCEEDED(res) && canconvert)
                        res = IWICFormatConverter_Initialize(converter, pISrc, dstFormat, WICBitmapDitherTypeNone,
                            NULL, 0.0, WICBitmapPaletteTypeCustom);

                    if (FAILED(res) || !canconvert)
                    {
                        if (converter)
                        {
                            IWICFormatConverter_Release(converter);
                            converter = NULL;
                        }
                        res = S_OK;
                    }
                }

                IWICFormatConverterInfo_Release(converterinfo);
            }

            IUnknown_Release(unkconverterinfo);
        }
        else
            break;
    }

    IEnumUnknown_Release(enumconverters);

    if (converter)
    {
        *ppIDst = (IWICBitmapSource*)converter;
        return S_OK;
    }
    else
    {
        FIXME("cannot convert %s to %s\n", debugstr_guid(&srcFormat), debugstr_guid(dstFormat));
        *ppIDst = NULL;
        return WINCODEC_ERR_COMPONENTNOTFOUND;
    }
}
