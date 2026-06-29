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

#include <stdarg.h>
#include <wchar.h>

#define COBJMACROS
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
    TRACE("(%p) refcount=%lu\n", This, ref);
    return ref;
}

static ULONG WINAPI mqr_Release(IWICMetadataQueryReader *iface)
{
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    ULONG ref = InterlockedDecrement(&This->ref);
    TRACE("(%p) refcount=%lu\n", This, ref);
    if (!ref)
    {
        IWICMetadataBlockReader_Release(This->block);
        free(This->root);
        free(This);
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
    QueryReader *This = impl_from_IWICMetadataQueryReader(iface);
    const WCHAR *root;
    UINT actual_len;

    TRACE("(%p,%u,%p,%p)\n", This, len, location, ret_len);

    if (!ret_len) return E_INVALIDARG;

    root = This->root ? This->root : L"/";
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

        *idx = wcstol(start + 1, &idx_end, 10);
        if (idx_end > elem->str + elem->len) return WINCODEC_ERR_INVALIDQUERYREQUEST;
        if (*idx_end != ']') return WINCODEC_ERR_INVALIDQUERYREQUEST;
        if (*idx < 0) return WINCODEC_ERR_INVALIDQUERYREQUEST;
        end = idx_end + 1;

        next_elem.str = end;
        next_elem.len = elem->len - (end - start);
        hr = get_token(&next_elem, id, schema, idx);
        if (hr != S_OK)
        {
            TRACE("get_token error %#lx\n", hr);
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

        end = wmemchr(start + 1, '=', elem->len - 1);
        if (!end) return WINCODEC_ERR_INVALIDQUERYREQUEST;
        if (end > elem->str + elem->len) return WINCODEC_ERR_INVALIDQUERYREQUEST;

        next_elem.str = start + 1;
        next_elem.len = end - start - 1;
        vt = map_type(&next_elem);
        TRACE("type %s => %d\n", wine_dbgstr_wn(next_elem.str, next_elem.len), vt);
        if (vt == VT_ILLEGAL) return WINCODEC_ERR_WRONGSTATE;

        next_token.vt = VT_BSTR;
        next_token.bstrVal = SysAllocStringLen(NULL, elem->len - (end - start) + 1);
        if (!next_token.bstrVal) return E_OUTOFMEMORY;

        bstr = next_token.bstrVal;

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
        TRACE("schema/id %s\n", wine_dbgstr_w(next_token.bstrVal));

        if (vt == VT_CLSID)
        {
            id->vt = VT_CLSID;
            id->puuid = CoTaskMemAlloc(sizeof(GUID));
            if (!id->puuid)
            {
                PropVariantClear(&next_token);
                return E_OUTOFMEMORY;
            }

            hr = UuidFromStringW(next_token.bstrVal, id->puuid);
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
                TRACE("get_token error %#lx\n", hr);
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

    end = wmemchr(start, '/', elem->len);
    if (!end) end = start + elem->len;

    p = wmemchr(start, ':', end - start);
    if (p)
    {
        next_elem.str = p + 1;
        next_elem.len = end - p - 1;

        elem->len = p - start;
    }
    else
        elem->len = end - start;

    id->vt = VT_BSTR;
    id->bstrVal = SysAllocStringLen(NULL, elem->len + 1);
    if (!id->bstrVal) return E_OUTOFMEMORY;

    bstr = id->bstrVal;
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
            TRACE("get_token error %#lx\n", hr);
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
        schema.uiVal = index;
    }

    id.vt = VT_CLSID;
    id.puuid = guid;
    hr = IWICMetadataReader_GetValue(reader, &schema, &id, &value);
    if (hr != S_OK) return hr;

    if (value.vt == VT_UNKNOWN)
        hr = IUnknown_QueryInterface(value.punkVal, &IID_IWICMetadataReader, (void **)new_reader);
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
    full_query = malloc(len * sizeof(WCHAR));
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
            WARN("get_token error %#lx\n", hr);
            break;
        }
        TRACE("parsed %d characters: %s, index %d\n", elem.len, wine_dbgstr_wn(elem.str, elem.len), index);
        TRACE("id %s, schema %s\n", wine_dbgstr_variant((VARIANT *)&tk_id), wine_dbgstr_variant((VARIANT *)&tk_schema));

        if (!elem.len) break;

        if (tk_id.vt == VT_CLSID || (tk_id.vt == VT_BSTR && WICMapShortNameToGuid(tk_id.bstrVal, &guid) == S_OK))
        {
            WCHAR *root;

            if (tk_schema.vt != VT_EMPTY)
            {
                FIXME("unsupported schema vt %u\n", tk_schema.vt);
                PropVariantClear(&tk_schema);
            }

            if (tk_id.vt == VT_CLSID) guid = *tk_id.puuid;

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
            hr = MetadataQueryReader_CreateInstance(This->block, root, (IWICMetadataQueryReader **)&new_value.punkVal);
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
                schema.pwszVal = (LPWSTR)map_shortname_to_schema(&guid, tk_schema.bstrVal);
                if (!schema.pwszVal)
                    schema.pwszVal = tk_schema.bstrVal;
            }
            else
                schema = tk_schema;

            if (tk_id.vt == VT_BSTR)
            {
                id.vt = VT_LPWSTR;
                id.pwszVal = tk_id.bstrVal;
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

    free(full_query);

    return hr;
}

struct string_enumerator
{
    IEnumString IEnumString_iface;
    LONG ref;
};

static struct string_enumerator *impl_from_IEnumString(IEnumString *iface)
{
    return CONTAINING_RECORD(iface, struct string_enumerator, IEnumString_iface);
}

static HRESULT WINAPI string_enumerator_QueryInterface(IEnumString *iface, REFIID riid, void **ppv)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);

    TRACE("iface %p, riid %s, ppv %p.\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualGUID(riid, &IID_IEnumString) || IsEqualGUID(riid, &IID_IUnknown))
        *ppv = &this->IEnumString_iface;
    else
    {
        WARN("Unknown riid %s.\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef(&this->IEnumString_iface);
    return S_OK;
}

static ULONG WINAPI string_enumerator_AddRef(IEnumString *iface)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);
    ULONG ref = InterlockedIncrement(&this->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI string_enumerator_Release(IEnumString *iface)
{
    struct string_enumerator *this = impl_from_IEnumString(iface);
    ULONG ref = InterlockedDecrement(&this->ref);

    TRACE("iface %p, ref %lu.\n", iface, ref);

    if (!ref)
        free(this);

    return ref;
}

static HRESULT WINAPI string_enumerator_Next(IEnumString *iface, ULONG count, LPOLESTR *strings, ULONG *ret)
{
    FIXME("iface %p, count %lu, strings %p, ret %p stub.\n", iface, count, strings, ret);

    if (!strings || !ret)
        return E_INVALIDARG;

    *ret = 0;
    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI string_enumerator_Reset(IEnumString *iface)
{
    TRACE("iface %p.\n", iface);

    return S_OK;
}

static HRESULT WINAPI string_enumerator_Skip(IEnumString *iface, ULONG count)
{
    FIXME("iface %p, count %lu stub.\n", iface, count);

    return count ? S_FALSE : S_OK;
}

static HRESULT WINAPI string_enumerator_Clone(IEnumString *iface, IEnumString **out)
{
    FIXME("iface %p, out %p stub.\n", iface, out);

    *out = NULL;
    return E_NOTIMPL;
}

static const IEnumStringVtbl string_enumerator_vtbl =
{
    string_enumerator_QueryInterface,
    string_enumerator_AddRef,
    string_enumerator_Release,
    string_enumerator_Next,
    string_enumerator_Skip,
    string_enumerator_Reset,
    string_enumerator_Clone
};

static HRESULT string_enumerator_create(IEnumString **enum_string)
{
    struct string_enumerator *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumString_iface.lpVtbl = &string_enumerator_vtbl;
    object->ref = 1;

    *enum_string = &object->IEnumString_iface;

    return S_OK;
}

static HRESULT WINAPI mqr_GetEnumerator(IWICMetadataQueryReader *iface,
        IEnumString **enum_string)
{
    TRACE("iface %p, enum_string %p.\n", iface, enum_string);

    return string_enumerator_create(enum_string);
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

    obj = calloc(1, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IWICMetadataQueryReader_iface.lpVtbl = &mqr_vtbl;
    obj->ref = 1;

    IWICMetadataBlockReader_AddRef(mbr);
    obj->block = mbr;

    obj->root = wcsdup(root);

    *out = &obj->IWICMetadataQueryReader_iface;

    return S_OK;
}

typedef struct
{
    IWICMetadataQueryWriter IWICMetadataQueryWriter_iface;
    LONG ref;
    IWICMetadataBlockWriter *block;
    WCHAR *root;
}
QueryWriter;

static inline QueryWriter *impl_from_IWICMetadataQueryWriter(IWICMetadataQueryWriter *iface)
{
    return CONTAINING_RECORD(iface, QueryWriter, IWICMetadataQueryWriter_iface);
}

static HRESULT WINAPI mqw_QueryInterface(IWICMetadataQueryWriter *iface, REFIID riid,
        void **object)
{
    QueryWriter *writer = impl_from_IWICMetadataQueryWriter(iface);

    TRACE("writer %p, riid %s, object %p.\n", writer, debugstr_guid(riid), object);

    if (IsEqualGUID(riid, &IID_IUnknown)
            || IsEqualGUID(riid, &IID_IWICMetadataQueryWriter)
            || IsEqualGUID(riid, &IID_IWICMetadataQueryReader))
        *object = &writer->IWICMetadataQueryWriter_iface;
    else
        *object = NULL;

    if (*object)
    {
        IUnknown_AddRef((IUnknown *)*object);
        return S_OK;
    }

    return E_NOINTERFACE;
}

static ULONG WINAPI mqw_AddRef(IWICMetadataQueryWriter *iface)
{
    QueryWriter *writer = impl_from_IWICMetadataQueryWriter(iface);
    ULONG ref = InterlockedIncrement(&writer->ref);

    TRACE("writer %p, refcount=%lu\n", writer, ref);

    return ref;
}

static ULONG WINAPI mqw_Release(IWICMetadataQueryWriter *iface)
{
    QueryWriter *writer = impl_from_IWICMetadataQueryWriter(iface);
    ULONG ref = InterlockedDecrement(&writer->ref);

    TRACE("writer %p, refcount=%lu.\n", writer, ref);

    if (!ref)
    {
        IWICMetadataBlockWriter_Release(writer->block);
        free(writer->root);
        free(writer);
    }
    return ref;
}

static HRESULT WINAPI mqw_GetContainerFormat(IWICMetadataQueryWriter *iface, GUID *container_format)
{
    FIXME("iface %p, container_format %p stub.\n", iface, container_format);

    return E_NOTIMPL;
}

static HRESULT WINAPI mqw_GetEnumerator(IWICMetadataQueryWriter *iface, IEnumString **enum_string)
{
    TRACE("iface %p, enum_string %p.\n", iface, enum_string);

    return string_enumerator_create(enum_string);
}

static HRESULT WINAPI mqw_GetLocation(IWICMetadataQueryWriter *iface, UINT max_length, WCHAR *namespace, UINT *actual_length)
{
    FIXME("iface %p, max_length %u, namespace %s, actual_length %p stub.\n",
            iface, max_length, debugstr_w(namespace), actual_length);

    return E_NOTIMPL;
}

static HRESULT WINAPI mqw_GetMetadataByName(IWICMetadataQueryWriter *iface, LPCWSTR name, PROPVARIANT *value)
{
    FIXME("name %s, value %p stub.\n", debugstr_w(name), value);

    return E_NOTIMPL;
}

static HRESULT WINAPI mqw_SetMetadataByName(IWICMetadataQueryWriter *iface, LPCWSTR name, const PROPVARIANT *value)
{
    FIXME("iface %p, name %s, value %p stub.\n", iface, debugstr_w(name), value);

    return S_OK;
}

static HRESULT WINAPI mqw_RemoveMetadataByName(IWICMetadataQueryWriter *iface, LPCWSTR name)
{
    FIXME("iface %p, name %s stub.\n", iface, debugstr_w(name));

    return E_NOTIMPL;
}

static const IWICMetadataQueryWriterVtbl mqw_vtbl =
{
    mqw_QueryInterface,
    mqw_AddRef,
    mqw_Release,
    mqw_GetContainerFormat,
    mqw_GetLocation,
    mqw_GetMetadataByName,
    mqw_GetEnumerator,
    mqw_SetMetadataByName,
    mqw_RemoveMetadataByName,
};

HRESULT MetadataQueryWriter_CreateInstance(IWICMetadataBlockWriter *mbw, const WCHAR *root, IWICMetadataQueryWriter **out)
{
    QueryWriter *obj;

    obj = calloc(1, sizeof(*obj));
    if (!obj)
        return E_OUTOFMEMORY;

    obj->IWICMetadataQueryWriter_iface.lpVtbl = &mqw_vtbl;
    obj->ref = 1;

    IWICMetadataBlockWriter_AddRef(mbw);
    obj->block = mbw;

    obj->root = wcsdup(root);

    *out = &obj->IWICMetadataQueryWriter_iface;

    return S_OK;
}

static const struct
{
    const GUID *guid;
    const WCHAR *name;
} guid2name[] =
{
    { &GUID_ContainerFormatBmp, L"bmp" },
    { &GUID_ContainerFormatPng, L"png" },
    { &GUID_ContainerFormatIco, L"ico" },
    { &GUID_ContainerFormatJpeg, L"jpg" },
    { &GUID_ContainerFormatTiff, L"tiff" },
    { &GUID_ContainerFormatGif, L"gif" },
    { &GUID_ContainerFormatWmp, L"wmphoto" },
    { &GUID_MetadataFormatUnknown, L"unknown" },
    { &GUID_MetadataFormatIfd, L"ifd" },
    { &GUID_MetadataFormatSubIfd, L"sub" },
    { &GUID_MetadataFormatExif, L"exif" },
    { &GUID_MetadataFormatGps, L"gps" },
    { &GUID_MetadataFormatInterop, L"interop" },
    { &GUID_MetadataFormatApp0, L"app0" },
    { &GUID_MetadataFormatApp1, L"app1" },
    { &GUID_MetadataFormatApp13, L"app13" },
    { &GUID_MetadataFormatIPTC, L"iptc" },
    { &GUID_MetadataFormatIRB, L"irb" },
    { &GUID_MetadataFormat8BIMIPTC, L"8bimiptc" },
    { &GUID_MetadataFormat8BIMResolutionInfo, L"8bimResInfo" },
    { &GUID_MetadataFormat8BIMIPTCDigest, L"8bimiptcdigest" },
    { &GUID_MetadataFormatXMP, L"xmp" },
    { &GUID_MetadataFormatThumbnail, L"thumb" },
    { &GUID_MetadataFormatChunktEXt, L"tEXt" },
    { &GUID_MetadataFormatXMPStruct, L"xmpstruct" },
    { &GUID_MetadataFormatXMPBag, L"xmpbag" },
    { &GUID_MetadataFormatXMPSeq, L"xmpseq" },
    { &GUID_MetadataFormatXMPAlt, L"xmpalt" },
    { &GUID_MetadataFormatLSD, L"logscrdesc" },
    { &GUID_MetadataFormatIMD, L"imgdesc" },
    { &GUID_MetadataFormatGCE, L"grctlext" },
    { &GUID_MetadataFormatAPE, L"appext" },
    { &GUID_MetadataFormatJpegChrominance, L"chrominance" },
    { &GUID_MetadataFormatJpegLuminance, L"luminance" },
    { &GUID_MetadataFormatJpegComment, L"com" },
    { &GUID_MetadataFormatGifComment, L"commentext" },
    { &GUID_MetadataFormatChunkgAMA, L"gAMA" },
    { &GUID_MetadataFormatChunkbKGD, L"bKGD" },
    { &GUID_MetadataFormatChunkiTXt, L"iTXt" },
    { &GUID_MetadataFormatChunkcHRM, L"cHRM" },
    { &GUID_MetadataFormatChunkhIST, L"hIST" },
    { &GUID_MetadataFormatChunkiCCP, L"iCCP" },
    { &GUID_MetadataFormatChunksRGB, L"sRGB" },
    { &GUID_MetadataFormatChunktIME, L"tIME" }
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

static const struct
{
    const WCHAR *name;
    const WCHAR *schema;
} name2schema[] =
{
    { L"rdf", L"http://www.w3.org/1999/02/22-rdf-syntax-ns#" },
    { L"dc", L"http://purl.org/dc/elements/1.1/" },
    { L"xmp", L"http://ns.adobe.com/xap/1.0/" },
    { L"xmpidq", L"http://ns.adobe.com/xmp/Identifier/qual/1.0/" },
    { L"xmpRights", L"http://ns.adobe.com/xap/1.0/rights/" },
    { L"xmpMM", L"http://ns.adobe.com/xap/1.0/mm/" },
    { L"xmpBJ", L"http://ns.adobe.com/xap/1.0/bj/" },
    { L"xmpTPg", L"http://ns.adobe.com/xap/1.0/t/pg/" },
    { L"pdf", L"http://ns.adobe.com/pdf/1.3/" },
    { L"photoshop", L"http://ns.adobe.com/photoshop/1.0/" },
    { L"tiff", L"http://ns.adobe.com/tiff/1.0/" },
    { L"exif", L"http://ns.adobe.com/exif/1.0/" },
    { L"stDim", L"http://ns.adobe.com/xap/1.0/sType/Dimensions#" },
    { L"xapGImg", L"http://ns.adobe.com/xap/1.0/g/img/" },
    { L"stEvt", L"http://ns.adobe.com/xap/1.0/sType/ResourceEvent#" },
    { L"stRef", L"http://ns.adobe.com/xap/1.0/sType/ResourceRef#" },
    { L"stVer", L"http://ns.adobe.com/xap/1.0/sType/Version#" },
    { L"stJob", L"http://ns.adobe.com/xap/1.0/sType/Job#" },
    { L"aux", L"http://ns.adobe.com/exif/1.0/aux/" },
    { L"crs", L"http://ns.adobe.com/camera-raw-settings/1.0/" },
    { L"xmpDM", L"http://ns.adobe.com/xmp/1.0/DynamicMedia/" },
    { L"Iptc4xmpCore", L"http://iptc.org/std/Iptc4xmpCore/1.0/xmlns/" },
    { L"MicrosoftPhoto", L"http://ns.microsoft.com/photo/1.0/" },
    { L"MP", L"http://ns.microsoft.com/photo/1.2/" },
    { L"MPRI", L"http://ns.microsoft.com/photo/1.2/t/RegionInfo#" },
    { L"MPReg", L"http://ns.microsoft.com/photo/1.2/t/Region#" }
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
        if (!wcscmp(name2schema[i].name, name))
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
        if (!wcscmp(name2schema[i].schema, schema))
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
