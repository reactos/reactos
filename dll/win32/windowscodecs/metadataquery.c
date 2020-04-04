/*
 * Copyright 2016 Andrew Eikum for CodeWeavers
 * Copyright 2017 Dmitry Timoshkov
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "objbase.h"
#include "propvarutil.h"

#include "wincodecs_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(wincodecs);

static const WCHAR *map_shortname_to_schema(const GUID *format, const WCHAR *name);

typedef struct {
    IWICMetadataQueryReader IWICMetadataQueryReader_iface;
    LONG ref;
    IWICMetadataBlockReader *block;
    WCHAR *root;
} QueryReader;

static inline QueryReader *impl_from_IWICMetadataQueryReader(IWICMetadataQueryReader *iface)
{
    return CONTAINING_RECORD(iface, QueryReader, IWICMetadataQueryReader_iface);
}

static HRESULT WINAPI mqr_QueryInterface(IWICMetadataQueryReader *iface, REFIID riid,
        void **ppvObject)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);

    TRACE("(%p,%s,%p)\n", This, debugstr_guid(riid), ppvObject);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IWICMetadataQueryReader))
        *ppvObject = &This->IWICMetadataQueryReader_iface;
    else
        *ppvObject = NULL;

    if (*ppvObject)
    {
        IUnknown_AddRef((IUnknown*)*ppvObject);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mqr_AddRef(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("(%p) refcount=%u\n", This, ref);
    return ref;
}

static ULONG WINAPI mqr_Release(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) refcount=%u\n", This, ref);
    if (!ref)
    {
        IWICMetadataBlockReader_Release(This->block);
        HeapFree(GetProcessHeap(), 0, This->root);
        HeapFree(GetProcessHeap(), 0, This);
    }
    return ref;
}

static HRESULT WINAPI mqr_GetContainerFormat(IWICMetadataQueryReader *iface, GUID *format)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);

    TRACE("(%p,%p)\n", This, format);

    return IWICMetadataBlockReader_GetContainerFormat(This->block, format);
}

static HRESULT WINAPI mqr_GetLocation(IWICMetadataQueryReader *iface, UINT len, WCHAR *location, UINT *ret_len)
{
    static const WCHAR rootW[] = { '/',0 };
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    const WCHAR *root;
    UINT actual_len;

    TRACE("(%p,%u,%p,%p)\n", This, len, location, ret_len);

    if (!ret_len) return E_INVALIDARG;

    root = This->root ? This->root : rootW;
    actual_len = lstrlenW(root) + 1;

    if (location)
    {
        if (len < actual_len)
            return WINCODEC_ERR_INSUFFICIENTBUFFER;

        memcpy(location, root, actual_len * sizeof(WCHAR));
    }

    *ret_len = actual_len;

    return S_OK;
}

struct string_t
{
    const WCHAR *str;
    int len;
};

static const struct
{
    int len;
    WCHAR str[10];
    VARTYPE vt;
} str2vt[] =
{
    { 4, {'c','h','a','r'}, VT_I1 },
    { 5, {'u','c','h','a','r'}, VT_UI1 },
    { 5, {'s','h','o','r','t'}, VT_I2 },
    { 6, {'u','s','h','o','r','t'}, VT_UI2 },
    { 4, {'l','o','n','g'}, VT_I4 },
    { 5, {'u','l','o','n','g'}, VT_UI4 },
    { 3, {'i','n','t'}, VT_I4 },
    { 4, {'u','i','n','t'}, VT_UI4 },
    { 8, {'l','o','n','g','l','o','n','g'}, VT_I8 },
    { 9, {'u','l','o','n','g','l','o','n','g'}, VT_UI8 },
    { 5, {'f','l','o','a','t'}, VT_R4 },
    { 6, {'d','o','u','b','l','e'}, VT_R8 },
    { 3, {'s','t','r'}, VT_LPSTR },
    { 4, {'w','s','t','r'}, VT_LPWSTR },
    { 4, {'g','u','i','d'}, VT_CLSID },
    { 4, {'b','o','o','l'}, VT_BOOL }
};

static VARTYPE map_type(struct string_t *str)
{
    UINT i;

    for (i = 0; i < ARRAY_SIZE(str2vt); i++)
    {
        if (str2vt[i].len == str->len)
        {
            if (CompareStringW(LOCALE_SYSTEM_DEFAULT, NORM_IGNORECASE,
                str->str, str->len, str2vt[i].str, str2vt[i].len) == CSTR_EQUAL)
                return str2vt[i].vt;
        }
    }

    WARN("type %s is not recognized\n", wine_dbgstr_wn(str->str, str->len));

    return VT_ILLEGAL;
}

static HRESULT get_token(struct string_t *elem, PROPVARIANT *id, PROPVARIANT *schema, int *idx)
{
    const WCHAR *start, *end, *p;
    WCHAR *bstr;
    struct string_t next_elem;
    HRESULT hr;

    TRACE("%s, len %d\n", wine_dbgstr_wn(elem->str, elem->len), elem->len);

    PropVariantInit(id);
    PropVariantInit(schema);

    if (!elem->len) return S_OK;

    start = elem->str;

    if (*start == '[')
    {
        WCHAR *idx_end;

        if (start[1] < '0' || start[1] > '9') return DISP_E_TYPEMISMATCH;

        *idx = strtolW(start + 1, &idx_end, 10);
        if (idx_end > elem->str + elem->len) return WINCODEC_ERR_INVALIDQUERYREQUEST;
        if (*idx_end != ']') return WINCODEC_ERR_INVALIDQUERYREQUEST;
        if (*idx < 0) return WINCODEC_ERR_INVALIDQUERYREQUEST;
        end = idx_end + 1;

        next_elem.str = end;
        next_elem.len = elem->len - (end - start);
        hr = get_token(&next_elem, id, schema, idx);
        if (hr != S_OK)
        {
            TRACE("get_token error %#x\n", hr);
            return hr;
        }
        elem->len = (end - start) + next_elem.len;

        TRACE("indexed %s [%d]\n", wine_dbgstr_wn(elem->str, elem->len), *idx);
        return S_OK;
    }
    else if (*start == '{')
    {
        VARTYPE vt;
        PROPVARIANT next_token;

        end = memchrW(start + 1, '=', elem->len - 1);
        if (!end) return WINCODEC_ERR_INVALIDQUERYREQUEST;
        if (end > elem->str + elem->len) return WINCODEC_ERR_INVALIDQUERYREQUEST;

        next_elem.str = start + 1;
        next_elem.len = end - start - 1;
        vt = map_type(&next_elem);
        TRACE("type %s => %d\n", wine_dbgstr_wn(next_elem.str, next_elem.len), vt);
        if (vt == VT_ILLEGAL) return WINCODEC_ERR_WRONGSTATE;

        next_token.vt = VT_BSTR;
        next_token.u.bstrVal = SysAllocStringLen(NULL, elem->len - (end - start) + 1);
        if (!next_token.u.bstrVal) return E_OUTOFMEMORY;

        bstr = next_token.u.bstrVal;

        end++;
        while (*end && *end != '}' && end - start < elem->len)
        {
            if (*end == '\\') end++;
            *bstr++ = *end++;
        }
        if (*end != '}')
        {
            PropVariantClear(&next_token);
            return WINCODEC_ERR_INVALIDQUERYREQUEST;
        }
        *bstr = 0;
        TRACE("schema/id %s\n", wine_dbgstr_w(next_token.u.bstrVal));

        if (vt == VT_CLSID)
        {
            id->vt = VT_CLSID;
            id->u.puuid = CoTaskMemAlloc(sizeof(GUID));
            if (!id->u.puuid)
            {
                PropVariantClear(&next_token);
                return E_OUTOFMEMORY;
            }

            hr = UuidFromStringW(next_token.u.bstrVal, id->u.puuid);
        }
        else
            hr = PropVariantChangeType(id, &next_token, 0, vt);
        PropVariantClear(&next_token);
        if (hr != S_OK)
        {
            PropVariantClear(id);
            PropVariantClear(schema);
            return hr;
        }

        end++;
        if (*end == ':')
        {
            PROPVARIANT next_id, next_schema;
            int next_idx = 0;

            next_elem.str = end + 1;
            next_elem.len = elem->len - (end - start + 1);
            hr = get_token(&next_elem, &next_id, &next_schema, &next_idx);
            if (hr != S_OK)
            {
                TRACE("get_token error %#x\n", hr);
                return hr;
            }
            elem->len = (end - start + 1) + next_elem.len;

            TRACE("id %s [%d]\n", wine_dbgstr_wn(elem->str, elem->len), *idx);

            if (next_schema.vt != VT_EMPTY)
            {
                PropVariantClear(&next_id);
                PropVariantClear(&next_schema);
                return WINCODEC_ERR_WRONGSTATE;
            }

            *schema = *id;
            *id = next_id;

            return S_OK;
        }

        elem->len = end - start;
        return S_OK;
    }

    end = memchrW(start, '/', elem->len);
    if (!end) end = start + elem->len;

    p = memchrW(start, ':', end - start);
    if (p)
    {
        next_elem.str = p + 1;
        next_elem.len = end - p - 1;

        elem->len = p - start;
    }
    else
        elem->len = end - start;

    id->vt = VT_BSTR;
    id->u.bstrVal = SysAllocStringLen(NULL, elem->len + 1);
    if (!id->u.bstrVal) return E_OUTOFMEMORY;

    bstr = id->u.bstrVal;
    p = elem->str;
    while (p - elem->str < elem->len)
    {
        if (*p == '\\') p++;
        *bstr++ = *p++;
    }
    *bstr = 0;
    TRACE("%s [%d]\n", wine_dbgstr_variant((VARIANT *)id), *idx);

    if (*p == ':')
    {
        PROPVARIANT next_id, next_schema;
        int next_idx = 0;

        hr = get_token(&next_elem, &next_id, &next_schema, &next_idx);
        if (hr != S_OK)
        {
            TRACE("get_token error %#x\n", hr);
            PropVariantClear(id);
            PropVariantClear(schema);
            return hr;
        }
        elem->len += next_elem.len + 1;

        TRACE("id %s [%d]\n", wine_dbgstr_wn(elem->str, elem->len), *idx);

        if (next_schema.vt != VT_EMPTY)
        {
            PropVariantClear(&next_id);
            PropVariantClear(&next_schema);
            PropVariantClear(id);
            PropVariantClear(schema);
            return WINCODEC_ERR_WRONGSTATE;
        }

        *schema = *id;
        *id = next_id;
    }

    return S_OK;
}

static HRESULT find_reader_from_block(IWICMetadataBlockReader *block_reader, UINT index,
                                      GUID *guid, IWICMetadataReader **reader)
{
    HRESULT hr;
    GUID format;
    IWICMetadataReader *new_reader;
    UINT count, i, matched_index;

    *reader = NULL;

    hr = IWICMetadataBlockReader_GetCount(block_reader, &count);
    if (hr != S_OK) return hr;

    matched_index = 0;

    for (i = 0; i < count; i++)
    {
        hr = IWICMetadataBlockReader_GetReaderByIndex(block_reader, i, &new_reader);
        if (hr != S_OK) return hr;

        hr = IWICMetadataReader_GetMetadataFormat(new_reader, &format);
        if (hr == S_OK)
        {
            if (IsEqualGUID(&format, guid))
            {
                if (matched_index == index)
                {
                    *reader = new_reader;
                    return S_OK;
                }

                matched_index++;
            }
        }

        IWICMetadataReader_Release(new_reader);
        if (hr != S_OK) return hr;
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}

static HRESULT get_next_reader(IWICMetadataReader *reader, UINT index,
                               GUID *guid, IWICMetadataReader **new_reader)
{
    HRESULT hr;
    PROPVARIANT schema, id, value;

    *new_reader = NULL;

    PropVariantInit(&schema);
    PropVariantInit(&id);
    PropVariantInit(&value);

    if (index)
    {
        schema.vt = VT_UI2;
        schema.u.uiVal = index;
    }

    id.vt = VT_CLSID;
    id.u.puuid = guid;
    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    if (hr != S_OK) return hr;

    if (value.vt == VT_UNKNOWN)
        hr = IUnknown_QueryInterface(value.u.punkVal, &IID_IWICMetadataReader, (void **)new_reader);
    else
        hr = WINCODEC_ERR_UNEXPECTEDMETADATATYPE;

    PropVariantClear(&value);
    return hr;
}

static HRESULT WINAPI mqr_GetMetadataByName(IWICMetadataQueryReader *iface, LPCWSTR query, PROPVARIANT *value)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    struct string_t elem;
    WCHAR *full_query;
    const WCHAR *p;
    int index, len;
    PROPVARIANT tk_id, tk_schema, new_value;
    GUID guid;
    IWICMetadataReader *reader;
    HRESULT hr = S_OK;

    TRACE("(%p,%s,%p)\n", This, wine_dbgstr_w(query), value);

    len = lstrlenW(query) + 1;
    if (This->root) len += lstrlenW(This->root);
    full_query = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    full_query[0] = 0;
    if (This->root)
        lstrcpyW(full_query, This->root);
    lstrcatW(full_query, query);

    PropVariantInit(&tk_id);
    PropVariantInit(&tk_schema);
    PropVariantInit(&new_value);

    reader = NULL;
    p = full_query;

    while (*p)
    {
        if (*p != '/')
        {
            WARN("query should start with '/'\n");
            hr = WINCODEC_ERR_PROPERTYNOTSUPPORTED;
            break;
        }

        p++;

        index = 0;
        elem.str = p;
        elem.len = lstrlenW(p);
        hr = get_token(&elem, &tk_id, &tk_schema, &index);
        if (hr != S_OK)
        {
            WARN("get_token error %#x\n", hr);
            break;
        }
        TRACE("parsed %d characters: %s, index %d\n", elem.len, wine_dbgstr_wn(elem.str, elem.len), index);
        TRACE("id %s, schema %s\n", wine_dbgstr_variant((VARIANT *)&tk_id), wine_dbgstr_variant((VARIANT *)&tk_schema));

        if (!elem.len) break;

        if (tk_id.vt == VT_CLSID || (tk_id.vt == VT_BSTR && WICMapShortNameToGuid(tk_id.u.bstrVal, &guid) == S_OK))
        {
            WCHAR *root;

            if (tk_schema.vt != VT_EMPTY)
            {
                FIXME("unsupported schema vt %u\n", tk_schema.vt);
                PropVariantClear(&tk_schema);
            }

            if (tk_id.vt == VT_CLSID) guid = *tk_id.u.puuid;

            if (reader)
            {
                IWICMetadataReader *new_reader;

                hr = get_next_reader(reader, index, &guid, &new_reader);
                IWICMetadataReader_Release(reader);
                reader = new_reader;
            }
            else
                hr = find_reader_from_block(This->block, index, &guid, &reader);

            if (hr != S_OK) break;

            root = SysAllocStringLen(NULL, elem.str + elem.len - full_query + 2);
            if (!root)
            {
                hr = E_OUTOFMEMORY;
                break;
            }
            lstrcpynW(root, full_query, p - full_query + elem.len + 1);

            PropVariantClear(&new_value);
            new_value.vt = VT_UNKNOWN;
            hr = MetadataQueryReader_CreateInstance(This->block, root, (IWICMetadataQueryReader **)&new_value.u.punkVal);
            SysFreeString(root);
            if (hr != S_OK) break;
        }
        else
        {
            PROPVARIANT schema, id;

            if (!reader)
            {
                hr = WINCODEC_ERR_INVALIDQUERYREQUEST;
                break;
            }

            if (tk_schema.vt == VT_BSTR)
            {
                hr = IWICMetadataReader_GetMetadataFormat(reader, &guid);
                if (hr != S_OK) break;

                schema.vt = VT_LPWSTR;
                schema.u.pwszVal = (LPWSTR)map_shortname_to_schema(&guid, tk_schema.u.bstrVal);
                if (!schema.u.pwszVal)
                    schema.u.pwszVal = tk_schema.u.bstrVal;
            }
            else
                schema = tk_schema;

            if (tk_id.vt == VT_BSTR)
            {
                id.vt = VT_LPWSTR;
                id.u.pwszVal = tk_id.u.bstrVal;
            }
            else
                id = tk_id;

            PropVariantClear(&new_value);
            hr = IWICMetadataReader_GetValue(reader, &schema, &id, &new_value);
            if (hr != S_OK) break;
        }

        p += elem.len;

        PropVariantClear(&tk_id);
        PropVariantClear(&tk_schema);
    }

    if (reader)
        IWICMetadataReader_Release(reader);

    PropVariantClear(&tk_id);
    PropVariantClear(&tk_schema);

    if (hr == S_OK && value)
        *value = new_value;
    else
        PropVariantClear(&new_value);

    HeapFree(GetProcessHeap(), 0, full_query);

    return hr;
}

static HRESULT WINAPI mqr_GetEnumerator(IWICMetadataQueryReader *iface,
        IEnumString **ppIEnumString)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    FIXME("(%p,%p)\n", This, ppIEnumString);
    return E_NOTIMPL;
}

static IWICMetadataQueryReaderVtbl mqr_vtbl = {
    mqr_QueryInterface,
    mqr_AddRef,
    mqr_Release,
    mqr_GetContainerFormat,
    mqr_GetLocation,
    mqr_GetMetadataByName,
    mqr_GetEnumerator
};

HRESULT MetadataQueryReader_CreateInstance(IWICMetadataBlockReader *mbr, const WCHAR *root, IWICMetadataQueryReader **out)
{
    QueryReader *obj;

    obj = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IWICMetadataQueryReader_iface.lpVtbl = &mqr_vtbl;
    obj->ref = 1;

    IWICMetadataBlockReader_AddRef(mbr);
    obj->block = mbr;

    obj->root = root ? heap_strdupW(root) : NULL;

    *out = &obj->IWICMetadataQueryReader_iface;

    return S_OK;
}

static const WCHAR bmpW[] = { 'b','m','p',0 };
static const WCHAR pngW[] = { 'p','n','g',0 };
static const WCHAR icoW[] = { 'i','c','o',0 };
static const WCHAR jpgW[] = { 'j','p','g',0 };
static const WCHAR tiffW[] = { 't','i','f','f',0 };
static const WCHAR gifW[] = { 'g','i','f',0 };
static const WCHAR wmphotoW[] = { 'w','m','p','h','o','t','o',0 };
static const WCHAR unknownW[] = { 'u','n','k','n','o','w','n',0 };
static const WCHAR ifdW[] = { 'i','f','d',0 };
static const WCHAR subW[] = { 's','u','b',0 };
static const WCHAR exifW[] = { 'e','x','i','f',0 };
static const WCHAR gpsW[] = { 'g','p','s',0 };
static const WCHAR interopW[] = { 'i','n','t','e','r','o','p',0 };
static const WCHAR app0W[] = { 'a','p','p','0',0 };
static const WCHAR app1W[] = { 'a','p','p','1',0 };
static const WCHAR app13W[] = { 'a','p','p','1','3',0 };
static const WCHAR iptcW[] = { 'i','p','t','c',0 };
static const WCHAR irbW[] = { 'i','r','b',0 };
static const WCHAR _8bimiptcW[] = { '8','b','i','m','i','p','t','c',0 };
static const WCHAR _8bimResInfoW[] = { '8','b','i','m','R','e','s','I','n','f','o',0 };
static const WCHAR _8bimiptcdigestW[] = { '8','b','i','m','i','p','t','c','d','i','g','e','s','t',0 };
static const WCHAR xmpW[] = { 'x','m','p',0 };
static const WCHAR thumbW[] = { 't','h','u','m','b',0 };
static const WCHAR tEXtW[] = { 't','E','X','t',0 };
static const WCHAR xmpstructW[] = { 'x','m','p','s','t','r','u','c','t',0 };
static const WCHAR xmpbagW[] = { 'x','m','p','b','a','g',0 };
static const WCHAR xmpseqW[] = { 'x','m','p','s','e','q',0 };
static const WCHAR xmpaltW[] = { 'x','m','p','a','l','t',0 };
static const WCHAR logscrdescW[] = { 'l','o','g','s','c','r','d','e','s','c',0 };
static const WCHAR imgdescW[] = { 'i','m','g','d','e','s','c',0 };
static const WCHAR grctlextW[] = { 'g','r','c','t','l','e','x','t',0 };
static const WCHAR appextW[] = { 'a','p','p','e','x','t',0 };
static const WCHAR chrominanceW[] = { 'c','h','r','o','m','i','n','a','n','c','e',0 };
static const WCHAR luminanceW[] = { 'l','u','m','i','n','a','n','c','e',0 };
static const WCHAR comW[] = { 'c','o','m',0 };
static const WCHAR commentextW[] = { 'c','o','m','m','e','n','t','e','x','t',0 };
static const WCHAR gAMAW[] = { 'g','A','M','A',0 };
static const WCHAR bKGDW[] = { 'b','K','G','D',0 };
static const WCHAR iTXtW[] = { 'i','T','X','t',0 };
static const WCHAR cHRMW[] = { 'c','H','R','M',0 };
static const WCHAR hISTW[] = { 'h','I','S','T',0 };
static const WCHAR iCCPW[] = { 'i','C','C','P',0 };
static const WCHAR sRGBW[] = { 's','R','G','B',0 };
static const WCHAR tIMEW[] = { 't','I','M','E',0 };

static const struct
{
    const GUID *guid;
    const WCHAR *name;
} guid2name[] =
{
    { &GUID_ContainerFormatBmp, bmpW },
    { &GUID_ContainerFormatPng, pngW },
    { &GUID_ContainerFormatIco, icoW },
    { &GUID_ContainerFormatJpeg, jpgW },
    { &GUID_ContainerFormatTiff, tiffW },
    { &GUID_ContainerFormatGif, gifW },
    { &GUID_ContainerFormatWmp, wmphotoW },
    { &GUID_MetadataFormatUnknown, unknownW },
    { &GUID_MetadataFormatIfd, ifdW },
    { &GUID_MetadataFormatSubIfd, subW },
    { &GUID_MetadataFormatExif, exifW },
    { &GUID_MetadataFormatGps, gpsW },
    { &GUID_MetadataFormatInterop, interopW },
    { &GUID_MetadataFormatApp0, app0W },
    { &GUID_MetadataFormatApp1, app1W },
    { &GUID_MetadataFormatApp13, app13W },
    { &GUID_MetadataFormatIPTC, iptcW },
    { &GUID_MetadataFormatIRB, irbW },
    { &GUID_MetadataFormat8BIMIPTC, _8bimiptcW },
    { &GUID_MetadataFormat8BIMResolutionInfo, _8bimResInfoW },
    { &GUID_MetadataFormat8BIMIPTCDigest, _8bimiptcdigestW },
    { &GUID_MetadataFormatXMP, xmpW },
    { &GUID_MetadataFormatThumbnail, thumbW },
    { &GUID_MetadataFormatChunktEXt, tEXtW },
    { &GUID_MetadataFormatXMPStruct, xmpstructW },
    { &GUID_MetadataFormatXMPBag, xmpbagW },
    { &GUID_MetadataFormatXMPSeq, xmpseqW },
    { &GUID_MetadataFormatXMPAlt, xmpaltW },
    { &GUID_MetadataFormatLSD, logscrdescW },
    { &GUID_MetadataFormatIMD, imgdescW },
    { &GUID_MetadataFormatGCE, grctlextW },
    { &GUID_MetadataFormatAPE, appextW },
    { &GUID_MetadataFormatJpegChrominance, chrominanceW },
    { &GUID_MetadataFormatJpegLuminance, luminanceW },
    { &GUID_MetadataFormatJpegComment, comW },
    { &GUID_MetadataFormatGifComment, commentextW },
    { &GUID_MetadataFormatChunkgAMA, gAMAW },
    { &GUID_MetadataFormatChunkbKGD, bKGDW },
    { &GUID_MetadataFormatChunkiTXt, iTXtW },
    { &GUID_MetadataFormatChunkcHRM, cHRMW },
    { &GUID_MetadataFormatChunkhIST, hISTW },
    { &GUID_MetadataFormatChunkiCCP, iCCPW },
    { &GUID_MetadataFormatChunksRGB, sRGBW },
    { &GUID_MetadataFormatChunktIME, tIMEW }
};

HRESULT WINAPI WICMapGuidToShortName(REFGUID guid, UINT len, WCHAR *name, UINT *ret_len)
{
    UINT i;

    TRACE("%s,%u,%p,%p\n", wine_dbgstr_guid(guid), len, name, ret_len);

    if (!guid) return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(guid2name); i++)
    {
        if (IsEqualGUID(guid, guid2name[i].guid))
        {
            if (name)
            {
                if (!len) return E_INVALIDARG;

                len = min(len - 1, lstrlenW(guid2name[i].name));
                memcpy(name, guid2name[i].name, len * sizeof(WCHAR));
                name[len] = 0;

                if (len < lstrlenW(guid2name[i].name))
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }
            if (ret_len) *ret_len = lstrlenW(guid2name[i].name) + 1;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}

HRESULT WINAPI WICMapShortNameToGuid(PCWSTR name, GUID *guid)
{
    UINT i;

    TRACE("%s,%p\n", debugstr_w(name), guid);

    if (!name || !guid) return E_INVALIDARG;

    for (i = 0; i < ARRAY_SIZE(guid2name); i++)
    {
        if (!lstrcmpiW(name, guid2name[i].name))
        {
            *guid = *guid2name[i].guid;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}

static const WCHAR rdf[] = { 'r','d','f',0 };
static const WCHAR rdf_scheme[] = { 'h','t','t','p',':','/','/','w','w','w','.','w','3','.','o','r','g','/','1','9','9','9','/','0','2','/','2','2','-','r','d','f','-','s','y','n','t','a','x','-','n','s','#',0 };
static const WCHAR dc[] = { 'd','c',0 };
static const WCHAR dc_scheme[] = { 'h','t','t','p',':','/','/','p','u','r','l','.','o','r','g','/','d','c','/','e','l','e','m','e','n','t','s','/','1','.','1','/',0 };
static const WCHAR xmp[] = { 'x','m','p',0 };
static const WCHAR xmp_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/',0 };
static const WCHAR xmpidq[] = { 'x','m','p','i','d','q',0 };
static const WCHAR xmpidq_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','m','p','/','I','d','e','n','t','i','f','i','e','r','/','q','u','a','l','/','1','.','0','/',0 };
static const WCHAR xmpRights[] = { 'x','m','p','R','i','g','h','t','s',0 };
static const WCHAR xmpRights_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','r','i','g','h','t','s','/',0 };
static const WCHAR xmpMM[] = { 'x','m','p','M','M',0 };
static const WCHAR xmpMM_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','m','m','/',0 };
static const WCHAR xmpBJ[] = { 'x','m','p','B','J',0 };
static const WCHAR xmpBJ_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','b','j','/',0 };
static const WCHAR xmpTPg[] = { 'x','m','p','T','P','g',0 };
static const WCHAR xmpTPg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','t','/','p','g','/',0 };
static const WCHAR pdf[] = { 'p','d','f',0 };
static const WCHAR pdf_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','p','d','f','/','1','.','3','/',0 };
static const WCHAR photoshop[] = { 'p','h','o','t','o','s','h','o','p',0 };
static const WCHAR photoshop_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','p','h','o','t','o','s','h','o','p','/','1','.','0','/',0 };
static const WCHAR tiff[] = { 't','i','f','f',0 };
static const WCHAR tiff_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','t','i','f','f','/','1','.','0','/',0 };
static const WCHAR exif[] = { 'e','x','i','f',0 };
static const WCHAR exif_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','e','x','i','f','/','1','.','0','/',0 };
static const WCHAR stDim[] = { 's','t','D','i','m',0 };
static const WCHAR stDim_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','D','i','m','e','n','s','i','o','n','s','#',0 };
static const WCHAR xapGImg[] = { 'x','a','p','G','I','m','g',0 };
static const WCHAR xapGImg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','g','/','i','m','g','/',0 };
static const WCHAR stEvt[] = { 's','t','E','v','t',0 };
static const WCHAR stEvt_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','R','e','s','o','u','r','c','e','E','v','e','n','t','#',0 };
static const WCHAR stRef[] = { 's','t','R','e','f',0 };
static const WCHAR stRef_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','R','e','s','o','u','r','c','e','R','e','f','#',0 };
static const WCHAR stVer[] = { 's','t','V','e','r',0 };
static const WCHAR stVer_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','V','e','r','s','i','o','n','#',0 };
static const WCHAR stJob[] = { 's','t','J','o','b',0 };
static const WCHAR stJob_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','a','p','/','1','.','0','/','s','T','y','p','e','/','J','o','b','#',0 };
static const WCHAR aux[] = { 'a','u','x',0 };
static const WCHAR aux_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','e','x','i','f','/','1','.','0','/','a','u','x','/',0 };
static const WCHAR crs[] = { 'c','r','s',0 };
static const WCHAR crs_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','c','a','m','e','r','a','-','r','a','w','-','s','e','t','t','i','n','g','s','/','1','.','0','/',0 };
static const WCHAR xmpDM[] = { 'x','m','p','D','M',0 };
static const WCHAR xmpDM_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','a','d','o','b','e','.','c','o','m','/','x','m','p','/','1','.','0','/','D','y','n','a','m','i','c','M','e','d','i','a','/',0 };
static const WCHAR Iptc4xmpCore[] = { 'I','p','t','c','4','x','m','p','C','o','r','e',0 };
static const WCHAR Iptc4xmpCore_scheme[] = { 'h','t','t','p',':','/','/','i','p','t','c','.','o','r','g','/','s','t','d','/','I','p','t','c','4','x','m','p','C','o','r','e','/','1','.','0','/','x','m','l','n','s','/',0 };
static const WCHAR MicrosoftPhoto[] = { 'M','i','c','r','o','s','o','f','t','P','h','o','t','o',0 };
static const WCHAR MicrosoftPhoto_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','0','/',0 };
static const WCHAR MP[] = { 'M','P',0 };
static const WCHAR MP_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/',0 };
static const WCHAR MPRI[] = { 'M','P','R','I',0 };
static const WCHAR MPRI_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/','t','/','R','e','g','i','o','n','I','n','f','o','#',0 };
static const WCHAR MPReg[] = { 'M','P','R','e','g',0 };
static const WCHAR MPReg_scheme[] = { 'h','t','t','p',':','/','/','n','s','.','m','i','c','r','o','s','o','f','t','.','c','o','m','/','p','h','o','t','o','/','1','.','2','/','t','/','R','e','g','i','o','n','#',0 };

static const struct
{
    const WCHAR *name;
    const WCHAR *schema;
} name2schema[] =
{
    { rdf, rdf_scheme },
    { dc, dc_scheme },
    { xmp, xmp_scheme },
    { xmpidq, xmpidq_scheme },
    { xmpRights, xmpRights_scheme },
    { xmpMM, xmpMM_scheme },
    { xmpBJ, xmpBJ_scheme },
    { xmpTPg, xmpTPg_scheme },
    { pdf, pdf_scheme },
    { photoshop, photoshop_scheme },
    { tiff, tiff_scheme },
    { exif, exif_scheme },
    { stDim, stDim_scheme },
    { xapGImg, xapGImg_scheme },
    { stEvt, stEvt_scheme },
    { stRef, stRef_scheme },
    { stVer, stVer_scheme },
    { stJob, stJob_scheme },
    { aux, aux_scheme },
    { crs, crs_scheme },
    { xmpDM, xmpDM_scheme },
    { Iptc4xmpCore, Iptc4xmpCore_scheme },
    { MicrosoftPhoto, MicrosoftPhoto_scheme },
    { MP, MP_scheme },
    { MPRI, MPRI_scheme },
    { MPReg, MPReg_scheme }
};

static const WCHAR *map_shortname_to_schema(const GUID *format, const WCHAR *name)
{
    UINT i;

    /* It appears that the only metadata formats
     * that support schemas are xmp and xmpstruct.
     */
    if (!IsEqualGUID(format, &GUID_MetadataFormatXMP) &&
        !IsEqualGUID(format, &GUID_MetadataFormatXMPStruct))
        return NULL;

    for (i = 0; i < ARRAY_SIZE(name2schema); i++)
    {
        if (!lstrcmpW(name2schema[i].name, name))
            return name2schema[i].schema;
    }

    return NULL;
}

HRESULT WINAPI WICMapSchemaToName(REFGUID format, LPWSTR schema, UINT len, WCHAR *name, UINT *ret_len)
{
    UINT i;

    TRACE("%s,%s,%u,%p,%p\n", wine_dbgstr_guid(format), debugstr_w(schema), len, name, ret_len);

    if (!format || !schema || !ret_len)
        return E_INVALIDARG;

    /* It appears that the only metadata formats
     * that support schemas are xmp and xmpstruct.
     */
    if (!IsEqualGUID(format, &GUID_MetadataFormatXMP) &&
        !IsEqualGUID(format, &GUID_MetadataFormatXMPStruct))
        return WINCODEC_ERR_PROPERTYNOTFOUND;

    for (i = 0; i < ARRAY_SIZE(name2schema); i++)
    {
        if (!lstrcmpW(name2schema[i].schema, schema))
        {
            if (name)
            {
                if (!len) return E_INVALIDARG;

                len = min(len - 1, lstrlenW(name2schema[i].name));
                memcpy(name, name2schema[i].name, len * sizeof(WCHAR));
                name[len] = 0;

                if (len < lstrlenW(name2schema[i].name))
                    return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            }

            *ret_len = lstrlenW(name2schema[i].name) + 1;
            return S_OK;
        }
    }

    return WINCODEC_ERR_PROPERTYNOTFOUND;
}
