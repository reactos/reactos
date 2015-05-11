/*
 * Copyright 2009 Vincent Povirk for CodeWeavers
 * Copyright 2012 Dmitry Timoshkov
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

#include "wincodecs_private.h"

#include <wine/list.h>

static const WCHAR mimetypes_valuename[] = {'M','i','m','e','T','y','p','e','s',0};
static const WCHAR author_valuename[] = {'A','u','t','h','o','r',0};
static const WCHAR friendlyname_valuename[] = {'F','r','i','e','n','d','l','y','N','a','m','e',0};
static const WCHAR pixelformats_keyname[] = {'P','i','x','e','l','F','o','r','m','a','t','s',0};
static const WCHAR formats_keyname[] = {'F','o','r','m','a','t','s',0};
static const WCHAR containerformat_valuename[] = {'C','o','n','t','a','i','n','e','r','F','o','r','m','a','t',0};
static const WCHAR metadataformat_valuename[] = {'M','e','t','a','d','a','t','a','F','o','r','m','a','t',0};
static const WCHAR vendor_valuename[] = {'V','e','n','d','o','r',0};
static const WCHAR version_valuename[] = {'V','e','r','s','i','o','n',0};
static const WCHAR specversion_valuename[] = {'S','p','e','c','V','e','r','s','i','o','n',0};
static const WCHAR bitsperpixel_valuename[] = {'B','i','t','L','e','n','g','t','h',0};
static const WCHAR channelcount_valuename[] = {'C','h','a','n','n','e','l','C','o','u','n','t',0};
static const WCHAR channelmasks_keyname[] = {'C','h','a','n','n','e','l','M','a','s','k','s',0};
static const WCHAR numericrepresentation_valuename[] = {'N','u','m','e','r','i','c','R','e','p','r','e','s','e','n','t','a','t','i','o','n',0};
static const WCHAR supportstransparency_valuename[] = {'S','u','p','p','o','r','t','s','T','r','a','n','s','p','a','r','e','n','c','y',0};
static const WCHAR requiresfullstream_valuename[] = {'R','e','q','u','i','r','e','s','F','u','l','l','S','t','r','e','a','m',0};
static const WCHAR supportspadding_valuename[] = {'S','u','p','p','o','r','t','s','P','a','d','d','i','n','g',0};
static const WCHAR fileextensions_valuename[] = {'F','i','l','e','E','x','t','e','n','s','i','o','n','s',0};
static const WCHAR containers_keyname[] = {'C','o','n','t','a','i','n','e','r','s',0};

static HRESULT ComponentInfo_GetStringValue(HKEY classkey, LPCWSTR value,
    UINT buffer_size, WCHAR *buffer, UINT *actual_size)
{
    LONG ret;
    DWORD cbdata=buffer_size * sizeof(WCHAR);

    if (!actual_size)
        return E_INVALIDARG;

    ret = RegGetValueW(classkey, NULL, value, RRF_RT_REG_SZ|RRF_NOEXPAND, NULL,
        buffer, &cbdata);

    if (ret == ERROR_FILE_NOT_FOUND)
    {
        *actual_size = 0;
        return S_OK;
    }

    if (ret == 0 || ret == ERROR_MORE_DATA)
        *actual_size = cbdata/sizeof(WCHAR);

    if (!buffer && buffer_size != 0)
        /* Yes, native returns the correct size in this case. */
        return E_INVALIDARG;

    if (ret == ERROR_MORE_DATA)
        return WINCODEC_ERR_INSUFFICIENTBUFFER;

    return HRESULT_FROM_WIN32(ret);
}

static HRESULT ComponentInfo_GetGUIDValue(HKEY classkey, LPCWSTR value,
    GUID *result)
{
    LONG ret;
    WCHAR guid_string[39];
    DWORD cbdata = sizeof(guid_string);
    HRESULT hr;

    if (!result)
        return E_INVALIDARG;

    ret = RegGetValueW(classkey, NULL, value, RRF_RT_REG_SZ|RRF_NOEXPAND, NULL,
        guid_string, &cbdata);

    if (ret != ERROR_SUCCESS)
        return HRESULT_FROM_WIN32(ret);

    if (cbdata < sizeof(guid_string))
    {
        ERR("incomplete GUID value\n");
        return E_FAIL;
    }

    hr = CLSIDFromString(guid_string, result);

    return hr;
}

static HRESULT ComponentInfo_GetDWORDValue(HKEY classkey, LPCWSTR value,
    DWORD *result)
{
    LONG ret;
    DWORD cbdata = sizeof(DWORD);

    if (!result)
        return E_INVALIDARG;

    ret = RegGetValueW(classkey, NULL, value, RRF_RT_DWORD, NULL,
        result, &cbdata);

    if (ret == ERROR_FILE_NOT_FOUND)
    {
        *result = 0;
        return S_OK;
    }

    return HRESULT_FROM_WIN32(ret);
}

static HRESULT ComponentInfo_GetGuidList(HKEY classkey, LPCWSTR subkeyname,
    UINT buffersize, GUID *buffer, UINT *actual_size)
{
    LONG ret;
    HKEY subkey;
    UINT items_returned;
    WCHAR guid_string[39];
    DWORD guid_string_size;
    HRESULT hr=S_OK;

    if (!actual_size)
        return E_INVALIDARG;

    ret = RegOpenKeyExW(classkey, subkeyname, 0, KEY_READ, &subkey);
    if (ret == ERROR_FILE_NOT_FOUND)
    {
        *actual_size = 0;
        return S_OK;
    }
    else if (ret != ERROR_SUCCESS) return HRESULT_FROM_WIN32(ret);

    if (buffer)
    {
        items_returned = 0;
        guid_string_size = 39;
        while (items_returned < buffersize)
        {
            ret = RegEnumKeyExW(subkey, items_returned, guid_string,
                &guid_string_size, NULL, NULL, NULL, NULL);

            if (ret != ERROR_SUCCESS)
            {
                hr = HRESULT_FROM_WIN32(ret);
                break;
            }

            if (guid_string_size != 38)
            {
                hr = E_FAIL;
                break;
            }

            hr = CLSIDFromString(guid_string, &buffer[items_returned]);
            if (FAILED(hr))
                break;

            items_returned++;
            guid_string_size = 39;
        }

        if (ret == ERROR_NO_MORE_ITEMS)
            hr = S_OK;

        *actual_size = items_returned;
    }
    else
    {
        ret = RegQueryInfoKeyW(subkey, NULL, NULL, NULL, actual_size, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (ret != ERROR_SUCCESS)
            hr = HRESULT_FROM_WIN32(ret);
    }

    RegCloseKey(subkey);

    return hr;
}

typedef struct {
    IWICBitmapDecoderInfo IWICBitmapDecoderInfo_iface;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} BitmapDecoderInfo;

static inline BitmapDecoderInfo *impl_from_IWICBitmapDecoderInfo(IWICBitmapDecoderInfo *iface)
{
    return CONTAINING_RECORD(iface, BitmapDecoderInfo, IWICBitmapDecoderInfo_iface);
}

static HRESULT WINAPI BitmapDecoderInfo_QueryInterface(IWICBitmapDecoderInfo *iface, REFIID iid,
    void **ppv)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
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
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapDecoderInfo_Release(IWICBitmapDecoderInfo *iface)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
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
    if (!pType) return E_INVALIDARG;
    *pType = WICDecoder;
    return S_OK;
}

static HRESULT WINAPI BitmapDecoderInfo_GetCLSID(IWICBitmapDecoderInfo *iface, CLSID *pclsid)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
    TRACE("(%p,%p)\n", iface, pclsid);

    if (!pclsid)
        return E_INVALIDARG;

    memcpy(pclsid, &This->clsid, sizeof(CLSID));

    return S_OK;
}

static HRESULT WINAPI BitmapDecoderInfo_GetSigningStatus(IWICBitmapDecoderInfo *iface, DWORD *pStatus)
{
    FIXME("(%p,%p): stub\n", iface, pStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapDecoderInfo_GetAuthor(IWICBitmapDecoderInfo *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchAuthor, wzAuthor, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, author_valuename,
        cchAuthor, wzAuthor, pcchActual);
}

static HRESULT WINAPI BitmapDecoderInfo_GetVendorGUID(IWICBitmapDecoderInfo *iface, GUID *pguidVendor)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%p)\n", iface, pguidVendor);

    return ComponentInfo_GetGUIDValue(This->classkey, vendor_valuename, pguidVendor);
}

static HRESULT WINAPI BitmapDecoderInfo_GetVersion(IWICBitmapDecoderInfo *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchVersion, wzVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, version_valuename,
        cchVersion, wzVersion, pcchActual);
}

static HRESULT WINAPI BitmapDecoderInfo_GetSpecVersion(IWICBitmapDecoderInfo *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, specversion_valuename,
        cchSpecVersion, wzSpecVersion, pcchActual);
}

static HRESULT WINAPI BitmapDecoderInfo_GetFriendlyName(IWICBitmapDecoderInfo *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, friendlyname_valuename,
        cchFriendlyName, wzFriendlyName, pcchActual);
}

static HRESULT WINAPI BitmapDecoderInfo_GetContainerFormat(IWICBitmapDecoderInfo *iface,
    GUID *pguidContainerFormat)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
    TRACE("(%p,%p)\n", iface, pguidContainerFormat);
    return ComponentInfo_GetGUIDValue(This->classkey, containerformat_valuename, pguidContainerFormat);
}

static HRESULT WINAPI BitmapDecoderInfo_GetPixelFormats(IWICBitmapDecoderInfo *iface,
    UINT cFormats, GUID *pguidPixelFormats, UINT *pcActual)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
    TRACE("(%p,%u,%p,%p)\n", iface, cFormats, pguidPixelFormats, pcActual);
    return ComponentInfo_GetGuidList(This->classkey, formats_keyname, cFormats, pguidPixelFormats, pcActual);
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
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchMimeTypes, wzMimeTypes, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, mimetypes_valuename,
        cchMimeTypes, wzMimeTypes, pcchActual);
}

static HRESULT WINAPI BitmapDecoderInfo_GetFileExtensions(IWICBitmapDecoderInfo *iface,
    UINT cchFileExtensions, WCHAR *wzFileExtensions, UINT *pcchActual)
{
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchFileExtensions, wzFileExtensions, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, fileextensions_valuename,
        cchFileExtensions, wzFileExtensions, pcchActual);
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
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);
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
    UINT i;
    ULONG pos;
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
    BitmapDecoderInfo *This = impl_from_IWICBitmapDecoderInfo(iface);

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

    This->IWICBitmapDecoderInfo_iface.lpVtbl = &BitmapDecoderInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

typedef struct {
    IWICBitmapEncoderInfo IWICBitmapEncoderInfo_iface;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} BitmapEncoderInfo;

static inline BitmapEncoderInfo *impl_from_IWICBitmapEncoderInfo(IWICBitmapEncoderInfo *iface)
{
    return CONTAINING_RECORD(iface, BitmapEncoderInfo, IWICBitmapEncoderInfo_iface);
}

static HRESULT WINAPI BitmapEncoderInfo_QueryInterface(IWICBitmapEncoderInfo *iface, REFIID iid,
    void **ppv)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICComponentInfo, iid) ||
        IsEqualIID(&IID_IWICBitmapCodecInfo, iid) ||
        IsEqualIID(&IID_IWICBitmapEncoderInfo ,iid))
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

static ULONG WINAPI BitmapEncoderInfo_AddRef(IWICBitmapEncoderInfo *iface)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI BitmapEncoderInfo_Release(IWICBitmapEncoderInfo *iface)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        RegCloseKey(This->classkey);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI BitmapEncoderInfo_GetComponentType(IWICBitmapEncoderInfo *iface,
    WICComponentType *pType)
{
    TRACE("(%p,%p)\n", iface, pType);
    if (!pType) return E_INVALIDARG;
    *pType = WICEncoder;
    return S_OK;
}

static HRESULT WINAPI BitmapEncoderInfo_GetCLSID(IWICBitmapEncoderInfo *iface, CLSID *pclsid)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);
    TRACE("(%p,%p)\n", iface, pclsid);

    if (!pclsid)
        return E_INVALIDARG;

    memcpy(pclsid, &This->clsid, sizeof(CLSID));

    return S_OK;
}

static HRESULT WINAPI BitmapEncoderInfo_GetSigningStatus(IWICBitmapEncoderInfo *iface, DWORD *pStatus)
{
    FIXME("(%p,%p): stub\n", iface, pStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_GetAuthor(IWICBitmapEncoderInfo *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchAuthor, wzAuthor, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, author_valuename,
        cchAuthor, wzAuthor, pcchActual);
}

static HRESULT WINAPI BitmapEncoderInfo_GetVendorGUID(IWICBitmapEncoderInfo *iface, GUID *pguidVendor)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%p)\n", iface, pguidVendor);

    return ComponentInfo_GetGUIDValue(This->classkey, vendor_valuename, pguidVendor);
}

static HRESULT WINAPI BitmapEncoderInfo_GetVersion(IWICBitmapEncoderInfo *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchVersion, wzVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, version_valuename,
        cchVersion, wzVersion, pcchActual);
}

static HRESULT WINAPI BitmapEncoderInfo_GetSpecVersion(IWICBitmapEncoderInfo *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, specversion_valuename,
        cchSpecVersion, wzSpecVersion, pcchActual);
}

static HRESULT WINAPI BitmapEncoderInfo_GetFriendlyName(IWICBitmapEncoderInfo *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, friendlyname_valuename,
        cchFriendlyName, wzFriendlyName, pcchActual);
}

static HRESULT WINAPI BitmapEncoderInfo_GetContainerFormat(IWICBitmapEncoderInfo *iface,
    GUID *pguidContainerFormat)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);
    TRACE("(%p,%p)\n", iface, pguidContainerFormat);
    return ComponentInfo_GetGUIDValue(This->classkey, containerformat_valuename, pguidContainerFormat);
}

static HRESULT WINAPI BitmapEncoderInfo_GetPixelFormats(IWICBitmapEncoderInfo *iface,
    UINT cFormats, GUID *pguidPixelFormats, UINT *pcActual)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);
    TRACE("(%p,%u,%p,%p)\n", iface, cFormats, pguidPixelFormats, pcActual);
    return ComponentInfo_GetGuidList(This->classkey, formats_keyname, cFormats, pguidPixelFormats, pcActual);
}

static HRESULT WINAPI BitmapEncoderInfo_GetColorManagementVersion(IWICBitmapEncoderInfo *iface,
    UINT cchColorManagementVersion, WCHAR *wzColorManagementVersion, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchColorManagementVersion, wzColorManagementVersion, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_GetDeviceManufacturer(IWICBitmapEncoderInfo *iface,
    UINT cchDeviceManufacturer, WCHAR *wzDeviceManufacturer, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchDeviceManufacturer, wzDeviceManufacturer, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_GetDeviceModels(IWICBitmapEncoderInfo *iface,
    UINT cchDeviceModels, WCHAR *wzDeviceModels, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchDeviceModels, wzDeviceModels, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_GetMimeTypes(IWICBitmapEncoderInfo *iface,
    UINT cchMimeTypes, WCHAR *wzMimeTypes, UINT *pcchActual)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchMimeTypes, wzMimeTypes, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, mimetypes_valuename,
        cchMimeTypes, wzMimeTypes, pcchActual);
}

static HRESULT WINAPI BitmapEncoderInfo_GetFileExtensions(IWICBitmapEncoderInfo *iface,
    UINT cchFileExtensions, WCHAR *wzFileExtensions, UINT *pcchActual)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, cchFileExtensions, wzFileExtensions, pcchActual);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_DoesSupportAnimation(IWICBitmapEncoderInfo *iface,
    BOOL *pfSupportAnimation)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportAnimation);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_DoesSupportChromaKey(IWICBitmapEncoderInfo *iface,
    BOOL *pfSupportChromaKey)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportChromaKey);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_DoesSupportLossless(IWICBitmapEncoderInfo *iface,
    BOOL *pfSupportLossless)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportLossless);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_DoesSupportMultiframe(IWICBitmapEncoderInfo *iface,
    BOOL *pfSupportMultiframe)
{
    FIXME("(%p,%p): stub\n", iface, pfSupportMultiframe);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_MatchesMimeType(IWICBitmapEncoderInfo *iface,
    LPCWSTR wzMimeType, BOOL *pfMatches)
{
    FIXME("(%p,%s,%p): stub\n", iface, debugstr_w(wzMimeType), pfMatches);
    return E_NOTIMPL;
}

static HRESULT WINAPI BitmapEncoderInfo_CreateInstance(IWICBitmapEncoderInfo *iface,
    IWICBitmapEncoder **ppIBitmapEncoder)
{
    BitmapEncoderInfo *This = impl_from_IWICBitmapEncoderInfo(iface);

    TRACE("(%p,%p)\n", iface, ppIBitmapEncoder);

    return CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICBitmapEncoder, (void**)ppIBitmapEncoder);
}

static const IWICBitmapEncoderInfoVtbl BitmapEncoderInfo_Vtbl = {
    BitmapEncoderInfo_QueryInterface,
    BitmapEncoderInfo_AddRef,
    BitmapEncoderInfo_Release,
    BitmapEncoderInfo_GetComponentType,
    BitmapEncoderInfo_GetCLSID,
    BitmapEncoderInfo_GetSigningStatus,
    BitmapEncoderInfo_GetAuthor,
    BitmapEncoderInfo_GetVendorGUID,
    BitmapEncoderInfo_GetVersion,
    BitmapEncoderInfo_GetSpecVersion,
    BitmapEncoderInfo_GetFriendlyName,
    BitmapEncoderInfo_GetContainerFormat,
    BitmapEncoderInfo_GetPixelFormats,
    BitmapEncoderInfo_GetColorManagementVersion,
    BitmapEncoderInfo_GetDeviceManufacturer,
    BitmapEncoderInfo_GetDeviceModels,
    BitmapEncoderInfo_GetMimeTypes,
    BitmapEncoderInfo_GetFileExtensions,
    BitmapEncoderInfo_DoesSupportAnimation,
    BitmapEncoderInfo_DoesSupportChromaKey,
    BitmapEncoderInfo_DoesSupportLossless,
    BitmapEncoderInfo_DoesSupportMultiframe,
    BitmapEncoderInfo_MatchesMimeType,
    BitmapEncoderInfo_CreateInstance
};

static HRESULT BitmapEncoderInfo_Constructor(HKEY classkey, REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    BitmapEncoderInfo *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(BitmapEncoderInfo));
    if (!This)
    {
        RegCloseKey(classkey);
        return E_OUTOFMEMORY;
    }

    This->IWICBitmapEncoderInfo_iface.lpVtbl = &BitmapEncoderInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

typedef struct {
    IWICFormatConverterInfo IWICFormatConverterInfo_iface;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} FormatConverterInfo;

static inline FormatConverterInfo *impl_from_IWICFormatConverterInfo(IWICFormatConverterInfo *iface)
{
    return CONTAINING_RECORD(iface, FormatConverterInfo, IWICFormatConverterInfo_iface);
}

static HRESULT WINAPI FormatConverterInfo_QueryInterface(IWICFormatConverterInfo *iface, REFIID iid,
    void **ppv)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);
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
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI FormatConverterInfo_Release(IWICFormatConverterInfo *iface)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);
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
    if (!pType) return E_INVALIDARG;
    *pType = WICPixelFormatConverter;
    return S_OK;
}

static HRESULT WINAPI FormatConverterInfo_GetCLSID(IWICFormatConverterInfo *iface, CLSID *pclsid)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);
    TRACE("(%p,%p)\n", iface, pclsid);

    if (!pclsid)
        return E_INVALIDARG;

    memcpy(pclsid, &This->clsid, sizeof(CLSID));

    return S_OK;
}

static HRESULT WINAPI FormatConverterInfo_GetSigningStatus(IWICFormatConverterInfo *iface, DWORD *pStatus)
{
    FIXME("(%p,%p): stub\n", iface, pStatus);
    return E_NOTIMPL;
}

static HRESULT WINAPI FormatConverterInfo_GetAuthor(IWICFormatConverterInfo *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchAuthor, wzAuthor, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, author_valuename,
        cchAuthor, wzAuthor, pcchActual);
}

static HRESULT WINAPI FormatConverterInfo_GetVendorGUID(IWICFormatConverterInfo *iface, GUID *pguidVendor)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);

    TRACE("(%p,%p)\n", iface, pguidVendor);

    return ComponentInfo_GetGUIDValue(This->classkey, vendor_valuename, pguidVendor);
}

static HRESULT WINAPI FormatConverterInfo_GetVersion(IWICFormatConverterInfo *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchVersion, wzVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, version_valuename,
        cchVersion, wzVersion, pcchActual);
}

static HRESULT WINAPI FormatConverterInfo_GetSpecVersion(IWICFormatConverterInfo *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, specversion_valuename,
        cchSpecVersion, wzSpecVersion, pcchActual);
}

static HRESULT WINAPI FormatConverterInfo_GetFriendlyName(IWICFormatConverterInfo *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, friendlyname_valuename,
        cchFriendlyName, wzFriendlyName, pcchActual);
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
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);

    TRACE("(%p,%p)\n", iface, ppIFormatConverter);

    return CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
        &IID_IWICFormatConverter, (void**)ppIFormatConverter);
}

static BOOL ConverterSupportsFormat(IWICFormatConverterInfo *iface, const WCHAR *formatguid)
{
    LONG res;
    FormatConverterInfo *This = impl_from_IWICFormatConverterInfo(iface);
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

    This->IWICFormatConverterInfo_iface.lpVtbl = &FormatConverterInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

typedef struct {
    IWICPixelFormatInfo2 IWICPixelFormatInfo2_iface;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} PixelFormatInfo;

static inline PixelFormatInfo *impl_from_IWICPixelFormatInfo2(IWICPixelFormatInfo2 *iface)
{
    return CONTAINING_RECORD(iface, PixelFormatInfo, IWICPixelFormatInfo2_iface);
}

static HRESULT WINAPI PixelFormatInfo_QueryInterface(IWICPixelFormatInfo2 *iface, REFIID iid,
    void **ppv)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IWICComponentInfo, iid) ||
        IsEqualIID(&IID_IWICPixelFormatInfo, iid) ||
        IsEqualIID(&IID_IWICPixelFormatInfo2 ,iid))
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

static ULONG WINAPI PixelFormatInfo_AddRef(IWICPixelFormatInfo2 *iface)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI PixelFormatInfo_Release(IWICPixelFormatInfo2 *iface)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        RegCloseKey(This->classkey);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI PixelFormatInfo_GetComponentType(IWICPixelFormatInfo2 *iface,
    WICComponentType *pType)
{
    TRACE("(%p,%p)\n", iface, pType);
    if (!pType) return E_INVALIDARG;
    *pType = WICPixelFormat;
    return S_OK;
}

static HRESULT WINAPI PixelFormatInfo_GetCLSID(IWICPixelFormatInfo2 *iface, CLSID *pclsid)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);
    TRACE("(%p,%p)\n", iface, pclsid);

    if (!pclsid)
        return E_INVALIDARG;

    memcpy(pclsid, &This->clsid, sizeof(CLSID));

    return S_OK;
}

static HRESULT WINAPI PixelFormatInfo_GetSigningStatus(IWICPixelFormatInfo2 *iface, DWORD *pStatus)
{
    TRACE("(%p,%p)\n", iface, pStatus);

    if (!pStatus)
        return E_INVALIDARG;

    /* Pixel formats don't require code, so they are considered signed. */
    *pStatus = WICComponentSigned;

    return S_OK;
}

static HRESULT WINAPI PixelFormatInfo_GetAuthor(IWICPixelFormatInfo2 *iface, UINT cchAuthor,
    WCHAR *wzAuthor, UINT *pcchActual)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchAuthor, wzAuthor, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, author_valuename,
        cchAuthor, wzAuthor, pcchActual);
}

static HRESULT WINAPI PixelFormatInfo_GetVendorGUID(IWICPixelFormatInfo2 *iface, GUID *pguidVendor)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%p)\n", iface, pguidVendor);

    return ComponentInfo_GetGUIDValue(This->classkey, vendor_valuename, pguidVendor);
}

static HRESULT WINAPI PixelFormatInfo_GetVersion(IWICPixelFormatInfo2 *iface, UINT cchVersion,
    WCHAR *wzVersion, UINT *pcchActual)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchVersion, wzVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, version_valuename,
        cchVersion, wzVersion, pcchActual);
}

static HRESULT WINAPI PixelFormatInfo_GetSpecVersion(IWICPixelFormatInfo2 *iface, UINT cchSpecVersion,
    WCHAR *wzSpecVersion, UINT *pcchActual)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchSpecVersion, wzSpecVersion, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, specversion_valuename,
        cchSpecVersion, wzSpecVersion, pcchActual);
}

static HRESULT WINAPI PixelFormatInfo_GetFriendlyName(IWICPixelFormatInfo2 *iface, UINT cchFriendlyName,
    WCHAR *wzFriendlyName, UINT *pcchActual)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, cchFriendlyName, wzFriendlyName, pcchActual);

    return ComponentInfo_GetStringValue(This->classkey, friendlyname_valuename,
        cchFriendlyName, wzFriendlyName, pcchActual);
}

static HRESULT WINAPI PixelFormatInfo_GetFormatGUID(IWICPixelFormatInfo2 *iface,
    GUID *pFormat)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);
    TRACE("(%p,%p)\n", iface, pFormat);

    if (!pFormat)
        return E_INVALIDARG;

    *pFormat = This->clsid;

    return S_OK;
}

static HRESULT WINAPI PixelFormatInfo_GetColorContext(IWICPixelFormatInfo2 *iface,
    IWICColorContext **ppIColorContext)
{
    FIXME("(%p,%p): stub\n", iface, ppIColorContext);
    return E_NOTIMPL;
}

static HRESULT WINAPI PixelFormatInfo_GetBitsPerPixel(IWICPixelFormatInfo2 *iface,
    UINT *puiBitsPerPixel)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%p)\n", iface, puiBitsPerPixel);

    return ComponentInfo_GetDWORDValue(This->classkey, bitsperpixel_valuename, puiBitsPerPixel);
}

static HRESULT WINAPI PixelFormatInfo_GetChannelCount(IWICPixelFormatInfo2 *iface,
    UINT *puiChannelCount)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%p)\n", iface, puiChannelCount);

    return ComponentInfo_GetDWORDValue(This->classkey, channelcount_valuename, puiChannelCount);
}

static HRESULT WINAPI PixelFormatInfo_GetChannelMask(IWICPixelFormatInfo2 *iface,
    UINT uiChannelIndex, UINT cbMaskBuffer, BYTE *pbMaskBuffer, UINT *pcbActual)
{
    static const WCHAR uintformatW[] = {'%','u',0};
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);
    UINT channel_count;
    HRESULT hr;
    LONG ret;
    WCHAR valuename[11];
    DWORD cbData;

    TRACE("(%p,%u,%u,%p,%p)\n", iface, uiChannelIndex, cbMaskBuffer, pbMaskBuffer, pcbActual);

    if (!pcbActual)
        return E_INVALIDARG;

    hr = PixelFormatInfo_GetChannelCount(iface, &channel_count);

    if (SUCCEEDED(hr) && uiChannelIndex >= channel_count)
        hr = E_INVALIDARG;

    if (SUCCEEDED(hr))
    {
        snprintfW(valuename, 11, uintformatW, uiChannelIndex);

        cbData = cbMaskBuffer;

        ret = RegGetValueW(This->classkey, channelmasks_keyname, valuename, RRF_RT_REG_BINARY, NULL, pbMaskBuffer, &cbData);

        if (ret == ERROR_SUCCESS || ret == ERROR_MORE_DATA)
            *pcbActual = cbData;

        if (ret == ERROR_MORE_DATA)
            hr = E_INVALIDARG;
        else
            hr = HRESULT_FROM_WIN32(ret);
    }

    return hr;
}

static HRESULT WINAPI PixelFormatInfo_SupportsTransparency(IWICPixelFormatInfo2 *iface,
    BOOL *pfSupportsTransparency)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%p)\n", iface, pfSupportsTransparency);

    return ComponentInfo_GetDWORDValue(This->classkey, supportstransparency_valuename, (DWORD*)pfSupportsTransparency);
}

static HRESULT WINAPI PixelFormatInfo_GetNumericRepresentation(IWICPixelFormatInfo2 *iface,
    WICPixelFormatNumericRepresentation *pNumericRepresentation)
{
    PixelFormatInfo *This = impl_from_IWICPixelFormatInfo2(iface);

    TRACE("(%p,%p)\n", iface, pNumericRepresentation);

    return ComponentInfo_GetDWORDValue(This->classkey, numericrepresentation_valuename, pNumericRepresentation);
}

static const IWICPixelFormatInfo2Vtbl PixelFormatInfo_Vtbl = {
    PixelFormatInfo_QueryInterface,
    PixelFormatInfo_AddRef,
    PixelFormatInfo_Release,
    PixelFormatInfo_GetComponentType,
    PixelFormatInfo_GetCLSID,
    PixelFormatInfo_GetSigningStatus,
    PixelFormatInfo_GetAuthor,
    PixelFormatInfo_GetVendorGUID,
    PixelFormatInfo_GetVersion,
    PixelFormatInfo_GetSpecVersion,
    PixelFormatInfo_GetFriendlyName,
    PixelFormatInfo_GetFormatGUID,
    PixelFormatInfo_GetColorContext,
    PixelFormatInfo_GetBitsPerPixel,
    PixelFormatInfo_GetChannelCount,
    PixelFormatInfo_GetChannelMask,
    PixelFormatInfo_SupportsTransparency,
    PixelFormatInfo_GetNumericRepresentation
};

static HRESULT PixelFormatInfo_Constructor(HKEY classkey, REFCLSID clsid, IWICComponentInfo **ppIInfo)
{
    PixelFormatInfo *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(PixelFormatInfo));
    if (!This)
    {
        RegCloseKey(classkey);
        return E_OUTOFMEMORY;
    }

    This->IWICPixelFormatInfo2_iface.lpVtbl = &PixelFormatInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    memcpy(&This->clsid, clsid, sizeof(CLSID));

    *ppIInfo = (IWICComponentInfo*)This;
    return S_OK;
}

typedef struct
{
    IWICMetadataReaderInfo IWICMetadataReaderInfo_iface;
    LONG ref;
    HKEY classkey;
    CLSID clsid;
} MetadataReaderInfo;

static inline MetadataReaderInfo *impl_from_IWICMetadataReaderInfo(IWICMetadataReaderInfo *iface)
{
    return CONTAINING_RECORD(iface, MetadataReaderInfo, IWICMetadataReaderInfo_iface);
}

static HRESULT WINAPI MetadataReaderInfo_QueryInterface(IWICMetadataReaderInfo *iface,
    REFIID riid, void **ppv)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IWICComponentInfo, riid) ||
        IsEqualIID(&IID_IWICMetadataHandlerInfo, riid) ||
        IsEqualIID(&IID_IWICMetadataReaderInfo, riid))
    {
        *ppv = This;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI MetadataReaderInfo_AddRef(IWICMetadataReaderInfo *iface)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);
    return ref;
}

static ULONG WINAPI MetadataReaderInfo_Release(IWICMetadataReaderInfo *iface)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (!ref)
    {
        RegCloseKey(This->classkey);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI MetadataReaderInfo_GetComponentType(IWICMetadataReaderInfo *iface,
    WICComponentType *type)
{
    TRACE("(%p,%p)\n", iface, type);

    if (!type) return E_INVALIDARG;
    *type = WICMetadataReader;
    return S_OK;
}

static HRESULT WINAPI MetadataReaderInfo_GetCLSID(IWICMetadataReaderInfo *iface,
    CLSID *clsid)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%p)\n", iface, clsid);

    if (!clsid) return E_INVALIDARG;
    *clsid = This->clsid;
    return S_OK;
}

static HRESULT WINAPI MetadataReaderInfo_GetSigningStatus(IWICMetadataReaderInfo *iface,
    DWORD *status)
{
    FIXME("(%p,%p): stub\n", iface, status);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataReaderInfo_GetAuthor(IWICMetadataReaderInfo *iface,
    UINT length, WCHAR *author, UINT *actual_length)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, length, author, actual_length);

    return ComponentInfo_GetStringValue(This->classkey, author_valuename,
                                        length, author, actual_length);
}

static HRESULT WINAPI MetadataReaderInfo_GetVendorGUID(IWICMetadataReaderInfo *iface,
    GUID *vendor)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%p)\n", iface, vendor);

    return ComponentInfo_GetGUIDValue(This->classkey, vendor_valuename, vendor);
}

static HRESULT WINAPI MetadataReaderInfo_GetVersion(IWICMetadataReaderInfo *iface,
    UINT length, WCHAR *version, UINT *actual_length)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, length, version, actual_length);

    return ComponentInfo_GetStringValue(This->classkey, version_valuename,
                                        length, version, actual_length);
}

static HRESULT WINAPI MetadataReaderInfo_GetSpecVersion(IWICMetadataReaderInfo *iface,
    UINT length, WCHAR *version, UINT *actual_length)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, length, version, actual_length);

    return ComponentInfo_GetStringValue(This->classkey, specversion_valuename,
                                        length, version, actual_length);
}

static HRESULT WINAPI MetadataReaderInfo_GetFriendlyName(IWICMetadataReaderInfo *iface,
    UINT length, WCHAR *name, UINT *actual_length)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%u,%p,%p)\n", iface, length, name, actual_length);

    return ComponentInfo_GetStringValue(This->classkey, friendlyname_valuename,
                                        length, name, actual_length);
}

static HRESULT WINAPI MetadataReaderInfo_GetMetadataFormat(IWICMetadataReaderInfo *iface,
    GUID *format)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    TRACE("(%p,%p)\n", iface, format);
    return ComponentInfo_GetGUIDValue(This->classkey, metadataformat_valuename, format);
}

static HRESULT WINAPI MetadataReaderInfo_GetContainerFormats(IWICMetadataReaderInfo *iface,
    UINT length, GUID *formats, UINT *actual_length)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    TRACE("(%p,%u,%p,%p)\n", iface, length, formats, actual_length);

    return ComponentInfo_GetGuidList(This->classkey, containers_keyname, length,
        formats, actual_length);
}

static HRESULT WINAPI MetadataReaderInfo_GetDeviceManufacturer(IWICMetadataReaderInfo *iface,
    UINT length, WCHAR *manufacturer, UINT *actual_length)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, length, manufacturer, actual_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataReaderInfo_GetDeviceModels(IWICMetadataReaderInfo *iface,
    UINT length, WCHAR *models, UINT *actual_length)
{
    FIXME("(%p,%u,%p,%p): stub\n", iface, length, models, actual_length);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataReaderInfo_DoesRequireFullStream(IWICMetadataReaderInfo *iface,
    BOOL *param)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    TRACE("(%p,%p)\n", iface, param);
    return ComponentInfo_GetDWORDValue(This->classkey, requiresfullstream_valuename, (DWORD *)param);
}

static HRESULT WINAPI MetadataReaderInfo_DoesSupportPadding(IWICMetadataReaderInfo *iface,
    BOOL *param)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    TRACE("(%p,%p)\n", iface, param);
    return ComponentInfo_GetDWORDValue(This->classkey, supportspadding_valuename, (DWORD *)param);
}

static HRESULT WINAPI MetadataReaderInfo_DoesRequireFixedSize(IWICMetadataReaderInfo *iface,
    BOOL *param)
{
    FIXME("(%p,%p): stub\n", iface, param);
    return E_NOTIMPL;
}

static HRESULT WINAPI MetadataReaderInfo_GetPatterns(IWICMetadataReaderInfo *iface,
    REFGUID container, UINT length, WICMetadataPattern *patterns, UINT *count, UINT *actual_length)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);
    HRESULT hr=S_OK;
    LONG res;
    UINT pattern_count=0, patterns_size=0;
    DWORD valuesize, patternsize;
    BYTE *bPatterns=(BYTE*)patterns;
    HKEY containers_key, guid_key, pattern_key;
    WCHAR subkeyname[11];
    WCHAR guidkeyname[39];
    int i;
    static const WCHAR uintformatW[] = {'%','u',0};
    static const WCHAR patternW[] = {'P','a','t','t','e','r','n',0};
    static const WCHAR positionW[] = {'P','o','s','i','t','i','o','n',0};
    static const WCHAR maskW[] = {'M','a','s','k',0};
    static const WCHAR dataoffsetW[] = {'D','a','t','a','O','f','f','s','e','t',0};

    TRACE("(%p,%s,%u,%p,%p,%p)\n", iface, debugstr_guid(container), length, patterns, count, actual_length);

    if (!actual_length || !container) return E_INVALIDARG;

    res = RegOpenKeyExW(This->classkey, containers_keyname, 0, KEY_READ, &containers_key);
    if (res == ERROR_SUCCESS)
    {
        StringFromGUID2(container, guidkeyname, 39);

        res = RegOpenKeyExW(containers_key, guidkeyname, 0, KEY_READ, &guid_key);
        if (res == ERROR_FILE_NOT_FOUND) hr = WINCODEC_ERR_COMPONENTNOTFOUND;
        else if (res != ERROR_SUCCESS) hr = HRESULT_FROM_WIN32(res);

        RegCloseKey(containers_key);
    }
    else if (res == ERROR_FILE_NOT_FOUND) hr = WINCODEC_ERR_COMPONENTNOTFOUND;
    else hr = HRESULT_FROM_WIN32(res);

    if (SUCCEEDED(hr))
    {
        res = RegQueryInfoKeyW(guid_key, NULL, NULL, NULL, &pattern_count, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
        if (res != ERROR_SUCCESS) hr = HRESULT_FROM_WIN32(res);

        if (SUCCEEDED(hr))
        {
            patterns_size = pattern_count * sizeof(WICMetadataPattern);

            for (i=0; i<pattern_count; i++)
            {
                snprintfW(subkeyname, 11, uintformatW, i);
                res = RegOpenKeyExW(guid_key, subkeyname, 0, KEY_READ, &pattern_key);
                if (res == ERROR_SUCCESS)
                {
                    res = RegGetValueW(pattern_key, NULL, patternW, RRF_RT_REG_BINARY, NULL,
                        NULL, &patternsize);
                    patterns_size += patternsize*2;

                    if ((length >= patterns_size) && (res == ERROR_SUCCESS))
                    {
                        patterns[i].Length = patternsize;

                        patterns[i].DataOffset.QuadPart = 0;
                        valuesize = sizeof(ULARGE_INTEGER);
                        RegGetValueW(pattern_key, NULL, dataoffsetW, RRF_RT_DWORD|RRF_RT_QWORD, NULL,
                            &patterns[i].DataOffset, &valuesize);

                        patterns[i].Position.QuadPart = 0;
                        valuesize = sizeof(ULARGE_INTEGER);
                        res = RegGetValueW(pattern_key, NULL, positionW, RRF_RT_DWORD|RRF_RT_QWORD, NULL,
                            &patterns[i].Position, &valuesize);

                        if (res == ERROR_SUCCESS)
                        {
                            patterns[i].Pattern = bPatterns+patterns_size-patternsize*2;
                            valuesize = patternsize;
                            res = RegGetValueW(pattern_key, NULL, patternW, RRF_RT_REG_BINARY, NULL,
                                patterns[i].Pattern, &valuesize);
                        }

                        if (res == ERROR_SUCCESS)
                        {
                            patterns[i].Mask = bPatterns+patterns_size-patternsize;
                            valuesize = patternsize;
                            res = RegGetValueW(pattern_key, NULL, maskW, RRF_RT_REG_BINARY, NULL,
                                patterns[i].Mask, &valuesize);
                        }
                    }

                    RegCloseKey(pattern_key);
                }
                if (res != ERROR_SUCCESS)
                {
                    hr = HRESULT_FROM_WIN32(res);
                    break;
                }
            }
        }

        RegCloseKey(guid_key);
    }

    if (hr == S_OK)
    {
        *count = pattern_count;
        *actual_length = patterns_size;
        if (patterns && length < patterns_size)
            hr = WINCODEC_ERR_INSUFFICIENTBUFFER;
    }

    return hr;
}

static HRESULT WINAPI MetadataReaderInfo_MatchesPattern(IWICMetadataReaderInfo *iface,
    REFGUID container, IStream *stream, BOOL *matches)
{
    HRESULT hr;
    WICMetadataPattern *patterns;
    UINT pattern_count=0, patterns_size=0;
    ULONG datasize=0;
    BYTE *data=NULL;
    ULONG bytesread;
    UINT i;
    LARGE_INTEGER seekpos;
    ULONG pos;

    TRACE("(%p,%s,%p,%p)\n", iface, debugstr_guid(container), stream, matches);

    hr = MetadataReaderInfo_GetPatterns(iface, container, 0, NULL, &pattern_count, &patterns_size);
    if (FAILED(hr)) return hr;

    patterns = HeapAlloc(GetProcessHeap(), 0, patterns_size);
    if (!patterns) return E_OUTOFMEMORY;

    hr = MetadataReaderInfo_GetPatterns(iface, container, patterns_size, patterns, &pattern_count, &patterns_size);
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

        seekpos.QuadPart = patterns[i].Position.QuadPart;
        hr = IStream_Seek(stream, seekpos, STREAM_SEEK_SET, NULL);
        if (FAILED(hr)) break;

        hr = IStream_Read(stream, data, patterns[i].Length, &bytesread);
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
            *matches = TRUE;
            break;
        }
    }

    if (i == pattern_count) /* does not match any pattern */
    {
        hr = S_OK;
        *matches = FALSE;
    }

end:
    HeapFree(GetProcessHeap(), 0, patterns);
    HeapFree(GetProcessHeap(), 0, data);

    return hr;
}

static HRESULT WINAPI MetadataReaderInfo_CreateInstance(IWICMetadataReaderInfo *iface,
    IWICMetadataReader **reader)
{
    MetadataReaderInfo *This = impl_from_IWICMetadataReaderInfo(iface);

    TRACE("(%p,%p)\n", iface, reader);

    return CoCreateInstance(&This->clsid, NULL, CLSCTX_INPROC_SERVER,
                            &IID_IWICMetadataReader, (void **)reader);
}

static const IWICMetadataReaderInfoVtbl MetadataReaderInfo_Vtbl = {
    MetadataReaderInfo_QueryInterface,
    MetadataReaderInfo_AddRef,
    MetadataReaderInfo_Release,
    MetadataReaderInfo_GetComponentType,
    MetadataReaderInfo_GetCLSID,
    MetadataReaderInfo_GetSigningStatus,
    MetadataReaderInfo_GetAuthor,
    MetadataReaderInfo_GetVendorGUID,
    MetadataReaderInfo_GetVersion,
    MetadataReaderInfo_GetSpecVersion,
    MetadataReaderInfo_GetFriendlyName,
    MetadataReaderInfo_GetMetadataFormat,
    MetadataReaderInfo_GetContainerFormats,
    MetadataReaderInfo_GetDeviceManufacturer,
    MetadataReaderInfo_GetDeviceModels,
    MetadataReaderInfo_DoesRequireFullStream,
    MetadataReaderInfo_DoesSupportPadding,
    MetadataReaderInfo_DoesRequireFixedSize,
    MetadataReaderInfo_GetPatterns,
    MetadataReaderInfo_MatchesPattern,
    MetadataReaderInfo_CreateInstance
};

static HRESULT MetadataReaderInfo_Constructor(HKEY classkey, REFCLSID clsid, IWICComponentInfo **info)
{
    MetadataReaderInfo *This;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This)
    {
        RegCloseKey(classkey);
        return E_OUTOFMEMORY;
    }

    This->IWICMetadataReaderInfo_iface.lpVtbl = &MetadataReaderInfo_Vtbl;
    This->ref = 1;
    This->classkey = classkey;
    This->clsid = *clsid;

    *info = (IWICComponentInfo *)This;
    return S_OK;
}

static const WCHAR clsid_keyname[] = {'C','L','S','I','D',0};
static const WCHAR instance_keyname[] = {'I','n','s','t','a','n','c','e',0};

struct category {
    WICComponentType type;
    const GUID *catid;
    HRESULT (*constructor)(HKEY,REFCLSID,IWICComponentInfo**);
};

static const struct category categories[] = {
    {WICDecoder, &CATID_WICBitmapDecoders, BitmapDecoderInfo_Constructor},
    {WICEncoder, &CATID_WICBitmapEncoders, BitmapEncoderInfo_Constructor},
    {WICPixelFormatConverter, &CATID_WICFormatConverters, FormatConverterInfo_Constructor},
    {WICPixelFormat, &CATID_WICPixelFormats, PixelFormatInfo_Constructor},
    {WICMetadataReader, &CATID_WICMetadataReader, MetadataReaderInfo_Constructor},
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
    BOOL found = FALSE;
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
                    found = TRUE;
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
    {
        FIXME("%s is not supported\n", wine_dbgstr_guid(clsid));
        hr = E_FAIL;
    }

    RegCloseKey(clsidkey);

    return hr;
}

typedef struct {
    IEnumUnknown IEnumUnknown_iface;
    LONG ref;
    struct list objects;
    struct list *cursor;
    CRITICAL_SECTION lock; /* Must be held when reading or writing cursor */
} ComponentEnum;

static inline ComponentEnum *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, ComponentEnum, IEnumUnknown_iface);
}

typedef struct {
    struct list entry;
    IUnknown *unk;
} ComponentEnumItem;

static const IEnumUnknownVtbl ComponentEnumVtbl;

static HRESULT WINAPI ComponentEnum_QueryInterface(IEnumUnknown *iface, REFIID iid,
    void **ppv)
{
    ComponentEnum *This = impl_from_IEnumUnknown(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IEnumUnknown, iid))
    {
        *ppv = &This->IEnumUnknown_iface;
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
    ComponentEnum *This = impl_from_IEnumUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI ComponentEnum_Release(IEnumUnknown *iface)
{
    ComponentEnum *This = impl_from_IEnumUnknown(iface);
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
    ComponentEnum *This = impl_from_IEnumUnknown(iface);
    ULONG num_fetched=0;
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
    if (pceltFetched)
        *pceltFetched = num_fetched;
    return hr;
}

static HRESULT WINAPI ComponentEnum_Skip(IEnumUnknown *iface, ULONG celt)
{
    ComponentEnum *This = impl_from_IEnumUnknown(iface);
    ULONG i;
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
    ComponentEnum *This = impl_from_IEnumUnknown(iface);

    TRACE("(%p)\n", iface);

    EnterCriticalSection(&This->lock);
    This->cursor = list_head(&This->objects);
    LeaveCriticalSection(&This->lock);
    return S_OK;
}

static HRESULT WINAPI ComponentEnum_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    ComponentEnum *This = impl_from_IEnumUnknown(iface);
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

    new_enum->IEnumUnknown_iface.lpVtbl = &ComponentEnumVtbl;
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
        IEnumUnknown_Release(&new_enum->IEnumUnknown_iface);
        *ppenum = NULL;
    }
    else
        *ppenum = &new_enum->IEnumUnknown_iface;

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

    This->IEnumUnknown_iface.lpVtbl = &ComponentEnumVtbl;
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
        IEnumUnknown_Reset(&This->IEnumUnknown_iface);
        *ppIEnumUnknown = &This->IEnumUnknown_iface;
    }
    else
    {
        *ppIEnumUnknown = NULL;
        IEnumUnknown_Release(&This->IEnumUnknown_iface);
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
        res = IWICFormatConverter_QueryInterface(converter, &IID_IWICBitmapSource, (void **)ppIDst);
        IWICFormatConverter_Release(converter);
        return res;
    }
    else
    {
        FIXME("cannot convert %s to %s\n", debugstr_guid(&srcFormat), debugstr_guid(dstFormat));
        *ppIDst = NULL;
        return WINCODEC_ERR_COMPONENTNOTFOUND;
    }
}
