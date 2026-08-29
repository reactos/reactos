/*
 * Copyright (C) 2012 Alistair Leslie-Hughes
 * Copyright 2015 Nikolay Sivov for CodeWeavers
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

#ifdef __REACTOS__
#include <wine/config.h>
#include <wine/port.h>
#endif
#include <stdarg.h>
#include <math.h>

#include "windef.h"
#include "winbase.h"
#include "ole2.h"
#include "olectl.h"
#include "dispex.h"
#include "scrrun.h"
#include "scrrun_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(scrrun);

#define BUCKET_COUNT  509
#define DICT_HASH_MOD 1201

/* Implementation details

   Dictionary contains one list that links all pairs, this way
   order in which they were added is preserved. Each bucket has
   its own list to hold all pairs in this bucket. Initially all
   bucket lists are zeroed and we init them once we about to add
   first pair.

   When pair is removed it's unlinked from both lists; if it was
   a last pair in a bucket list it stays empty in initialized state.

   Preserving pair order is important for enumeration, so far testing
   indicates that pairs are not reordered basing on hash value.
 */

struct keyitem_pair
{
    struct  list entry;
    struct  list bucket;
    DWORD   hash;
    VARIANT key;
    VARIANT item;
};

struct dictionary
{
    struct provideclassinfo classinfo;
    IDictionary IDictionary_iface;
    LONG ref;

    CompareMethod method;
    LONG count;
    struct list pairs;
    struct list buckets[BUCKET_COUNT];
    struct list notifier;
};

struct dictionary_enum
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    struct dictionary *dict;
    struct list *cur;
    struct list notify;
};

static inline struct dictionary *impl_from_IDictionary(IDictionary *iface)
{
    return CONTAINING_RECORD(iface, struct dictionary, IDictionary_iface);
}

static inline struct dictionary_enum *impl_from_IEnumVARIANT(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, struct dictionary_enum, IEnumVARIANT_iface);
}

static inline struct list *get_bucket_head(struct dictionary *dict, DWORD hash)
{
    return &dict->buckets[hash % BUCKET_COUNT];
}

static inline BOOL is_string_key(const VARIANT *key)
{
    return V_VT(key) == VT_BSTR;
}

static inline BOOL is_ptr_key(const VARIANT *key)
{
    return V_VT(key) == VT_UNKNOWN || V_VT(key) == VT_DISPATCH;
}

static inline BOOL is_numeric_key(const VARIANT *key)
{
    switch (V_VT(key))
    {
        case VT_UI1:
        case VT_I2:
        case VT_I4:
        case VT_DATE:
        case VT_R4:
        case VT_R8:
            return TRUE;
        default:
            return FALSE;
    }
}

/* Only for VT_BSTR or VT_BSTR|VT_BYREF types */
static inline WCHAR *get_key_strptr(const VARIANT *key)
{
    if (V_VT(key) == VT_BSTR)
        return V_BSTR(key);

    if (V_BSTRREF(key))
        return *V_BSTRREF(key);

    return NULL;
}

/* should be used only when both keys are of string type, it's not checked */
static inline int strcmp_key(const struct dictionary *dict, const VARIANT *key1, const VARIANT *key2)
{
    const WCHAR *str1, *str2;

    str1 = get_key_strptr(key1);
    str2 = get_key_strptr(key2);
    return dict->method == BinaryCompare ? wcscmp(str1, str2) : wcsicmp(str1, str2);
}

static inline BOOL numeric_key_eq(const VARIANT *key1, const VARIANT *key2)
{
    VARIANT v1, v2;

    VariantInit(&v1);
    if (FAILED(VariantChangeType(&v1, key1, 0, VT_R4)))
        return FALSE;

    VariantInit(&v2);
    if (FAILED(VariantChangeType(&v2, key2, 0, VT_R4)))
        return FALSE;

    return V_R4(&v1) == V_R4(&v2);
}

static BOOL is_matching_key(const struct dictionary *dict, const struct keyitem_pair *pair, const VARIANT *key, DWORD hash)
{
    if (is_string_key(key) != is_string_key(&pair->key))
    {
        return FALSE;
    }
    else if (is_string_key(key) && is_string_key(&pair->key))
    {
        return hash == pair->hash && !strcmp_key(dict, key, &pair->key);
    }
    else if (is_ptr_key(key) != is_ptr_key(&pair->key))
    {
        return FALSE;
    }
    else if (is_ptr_key(key) && is_ptr_key(&pair->key))
    {
        return hash == pair->hash && V_UNKNOWN(key) == V_UNKNOWN(&pair->key);
    }
    else if (is_numeric_key(key) != is_numeric_key(&pair->key))
    {
        return FALSE;
    }
    else if (is_numeric_key(key) && is_numeric_key(&pair->key))
    {
        return hash == pair->hash && numeric_key_eq(key, &pair->key);
    }
    else if (V_VT(&pair->key) == VT_EMPTY || V_VT(&pair->key) == VT_NULL)
    {
        return V_VT(&pair->key) == V_VT(key);
    }
    else
    {
        WARN("Unexpected key type %#x.\n", V_VT(key));
        return FALSE;
    }
}

static struct keyitem_pair *get_keyitem_pair(struct dictionary *dict, VARIANT *key)
{
    struct keyitem_pair *pair;
    struct list *head, *entry;
    VARIANT hash, v;
    HRESULT hr;

    hr = IDictionary_get_HashVal(&dict->IDictionary_iface, key, &hash);
    if (FAILED(hr))
        return NULL;

    head = get_bucket_head(dict, V_I4(&hash));
    if (!head->next || list_empty(head))
        return NULL;

    VariantInit(&v);
    if (FAILED(VariantCopyInd(&v, key)))
        return NULL;

    entry = list_head(head);
    do
    {
        pair = LIST_ENTRY(entry, struct keyitem_pair, bucket);
        if (is_matching_key(dict, pair, &v, V_I4(&hash)))
        {
            VariantClear(&v);
            return pair;
        }
    } while ((entry = list_next(head, entry)));

    VariantClear(&v);

    return NULL;
}

static HRESULT add_keyitem_pair(struct dictionary *dict, VARIANT *key, VARIANT *item)
{
    struct keyitem_pair *pair;
    struct list *head;
    VARIANT hash;
    HRESULT hr;

    hr = IDictionary_get_HashVal(&dict->IDictionary_iface, key, &hash);
    if (FAILED(hr))
        return hr;

    if (!(pair = malloc(sizeof(*pair))))
        return E_OUTOFMEMORY;

    pair->hash = V_I4(&hash);
    VariantInit(&pair->key);
    VariantInit(&pair->item);

    hr = VariantCopyInd(&pair->key, key);
    if (FAILED(hr))
        goto failed;

    hr = VariantCopyInd(&pair->item, item);
    if (FAILED(hr))
        goto failed;

    head = get_bucket_head(dict, pair->hash);
    if (!head->next)
        /* this only happens once per bucket */
        list_init(head);

    /* link to bucket list and to full list */
    list_add_tail(head, &pair->bucket);
    list_add_tail(&dict->pairs, &pair->entry);
    dict->count++;
    return S_OK;

failed:
    VariantClear(&pair->key);
    VariantClear(&pair->item);
    free(pair);
    return hr;
}

static void free_keyitem_pair(struct keyitem_pair *pair)
{
    VariantClear(&pair->key);
    VariantClear(&pair->item);
    free(pair);
}

static HRESULT WINAPI dict_enum_QueryInterface(IEnumVARIANT *iface, REFIID riid, void **obj)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)->(%s, %p)\n", This, debugstr_guid(riid), obj);

    if (IsEqualIID(riid, &IID_IEnumVARIANT) || IsEqualIID(riid, &IID_IUnknown)) {
        *obj = iface;
        IEnumVARIANT_AddRef(iface);
        return S_OK;
    }
    else {
        WARN("interface not supported %s\n", debugstr_guid(riid));
        *obj = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG WINAPI dict_enum_AddRef(IEnumVARIANT *iface)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI dict_enum_Release(IEnumVARIANT *iface)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        list_remove(&This->notify);
        IDictionary_Release(&This->dict->IDictionary_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI dict_enum_Next(IEnumVARIANT *iface, ULONG count, VARIANT *keys, ULONG *fetched)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);
    struct keyitem_pair *pair;
    ULONG i = 0;

    TRACE("%p, %lu, %p, %p.\n", iface, count, keys, fetched);

    if (fetched)
        *fetched = 0;

    if (!count)
        return S_OK;

    while (This->cur && i < count) {
        pair = LIST_ENTRY(This->cur, struct keyitem_pair, entry);
        VariantCopy(&keys[i], &pair->key);
        This->cur = list_next(&This->dict->pairs, This->cur);
        i++;
    }

    if (fetched)
        *fetched = i;

    return i < count ? S_FALSE : S_OK;
}

static HRESULT WINAPI dict_enum_Skip(IEnumVARIANT *iface, ULONG count)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);

    TRACE("%p, %lu.\n", iface, count);

    if (!count)
        return S_OK;

    if (!This->cur)
        return S_FALSE;

    while (count--) {
        This->cur = list_next(&This->dict->pairs, This->cur);
        if (!This->cur) break;
    }

    return count == 0 ? S_OK : S_FALSE;
}

static HRESULT WINAPI dict_enum_Reset(IEnumVARIANT *iface)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", This);

    This->cur = list_head(&This->dict->pairs);
    return S_OK;
}

static HRESULT create_dict_enum(struct dictionary *, IUnknown**);

static HRESULT WINAPI dict_enum_Clone(IEnumVARIANT *iface, IEnumVARIANT **cloned)
{
    struct dictionary_enum *This = impl_from_IEnumVARIANT(iface);
    TRACE("(%p)->(%p)\n", This, cloned);
    return create_dict_enum(This->dict, (IUnknown**)cloned);
}

static const IEnumVARIANTVtbl dictenumvtbl = {
    dict_enum_QueryInterface,
    dict_enum_AddRef,
    dict_enum_Release,
    dict_enum_Next,
    dict_enum_Skip,
    dict_enum_Reset,
    dict_enum_Clone
};

static HRESULT create_dict_enum(struct dictionary *dict, IUnknown **ret)
{
    struct dictionary_enum *object;

    *ret = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IEnumVARIANT_iface.lpVtbl = &dictenumvtbl;
    object->ref = 1;
    object->cur = list_head(&dict->pairs);
    list_add_tail(&dict->notifier, &object->notify);
    object->dict = dict;
    IDictionary_AddRef(&dict->IDictionary_iface);

    *ret = (IUnknown *)&object->IEnumVARIANT_iface;

    return S_OK;
}

static void notify_remove_pair(struct list *notifier, struct list *pair)
{
    struct dictionary_enum *dict_enum;
    struct list *cur;

    LIST_FOR_EACH(cur, notifier) {
        dict_enum = LIST_ENTRY(cur, struct dictionary_enum, notify);
        if (!pair)
            dict_enum->cur = list_head(&dict_enum->dict->pairs);
        else if (dict_enum->cur == pair) {
            dict_enum->cur = list_next(&dict_enum->dict->pairs, dict_enum->cur);
        }
    }
}

static HRESULT WINAPI dictionary_QueryInterface(IDictionary *iface, REFIID riid, void **obj)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), obj);

    *obj = NULL;

    if(IsEqualIID(riid, &IID_IUnknown) ||
       IsEqualIID(riid, &IID_IDispatch) ||
       IsEqualIID(riid, &IID_IDictionary))
    {
        *obj = &dictionary->IDictionary_iface;
    }
    else if (IsEqualIID(riid, &IID_IProvideClassInfo))
    {
        *obj = &dictionary->classinfo.IProvideClassInfo_iface;
    }
    else if ( IsEqualGUID( riid, &IID_IDispatchEx ))
    {
        TRACE("Interface IDispatchEx not supported - returning NULL\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }
    else if ( IsEqualGUID( riid, &IID_IObjectWithSite ))
    {
        TRACE("Interface IObjectWithSite not supported - returning NULL\n");
        *obj = NULL;
        return E_NOINTERFACE;
    }
    else
    {
        WARN("interface %s not implemented\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*obj);
    return S_OK;
}

static ULONG WINAPI dictionary_AddRef(IDictionary *iface)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    ULONG ref = InterlockedIncrement(&dictionary->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    return ref;
}

static ULONG WINAPI dictionary_Release(IDictionary *iface)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    ULONG ref = InterlockedDecrement(&dictionary->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        IDictionary_RemoveAll(iface);
        free(dictionary);
    }

    return ref;
}

static HRESULT WINAPI dictionary_GetTypeInfoCount(IDictionary *iface, UINT *pctinfo)
{
    TRACE("%p, %p.\n", iface, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI dictionary_GetTypeInfo(IDictionary *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    TRACE("%p, %u, %lx, %p.\n", iface, iTInfo, lcid, ppTInfo);

    return get_typeinfo(IDictionary_tid, ppTInfo);
}

static HRESULT WINAPI dictionary_GetIDsOfNames(IDictionary *iface, REFIID riid, LPOLESTR *rgszNames,
                UINT cNames, LCID lcid, DISPID *rgDispId)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %s, %p, %u, %lx, %p.\n", iface, debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    hr = get_typeinfo(IDictionary_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_GetIDsOfNames(typeinfo, rgszNames, cNames, rgDispId);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dictionary_Invoke(IDictionary *iface, DISPID dispIdMember, REFIID riid,
                LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
                EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    ITypeInfo *typeinfo;
    HRESULT hr;

    TRACE("%p, %ld, %s, %lx, %d, %p, %p, %p, %p.\n", iface, dispIdMember, debugstr_guid(riid),
           lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    hr = get_typeinfo(IDictionary_tid, &typeinfo);
    if(SUCCEEDED(hr))
    {
        hr = ITypeInfo_Invoke(typeinfo, iface, dispIdMember, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
        ITypeInfo_Release(typeinfo);
    }

    return hr;
}

static HRESULT WINAPI dictionary_putref_Item(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("%p, %s, %s.\n", iface, debugstr_variant(key), debugstr_variant(item));

    if ((pair = get_keyitem_pair(dictionary, key)))
        return VariantCopyInd(&pair->item, item);

    return add_keyitem_pair(dictionary, key, item);
}

static HRESULT WINAPI dictionary_put_Item(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("%p, %s, %s.\n", iface, debugstr_variant(key), debugstr_variant(item));

    if ((pair = get_keyitem_pair(dictionary, key)))
        return VariantCopyInd(&pair->item, item);

    return add_keyitem_pair(dictionary, key, item);
}

static HRESULT WINAPI dictionary_get_Item(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("%p, %s, %p.\n", iface, debugstr_variant(key), item);

    if ((pair = get_keyitem_pair(dictionary, key)))
        VariantCopy(item, &pair->item);
    else {
        VariantInit(item);
        return IDictionary_Add(iface, key, item);
    }

    return S_OK;
}

static HRESULT WINAPI dictionary_Add(IDictionary *iface, VARIANT *key, VARIANT *item)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %s, %s.\n", iface, debugstr_variant(key), debugstr_variant(item));

    if (get_keyitem_pair(dictionary, key))
        return CTL_E_KEY_ALREADY_EXISTS;

    return add_keyitem_pair(dictionary, key, item);
}

static HRESULT WINAPI dictionary_get_Count(IDictionary *iface, LONG *count)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %p.\n", iface, count);

    *count = dictionary->count;
    return S_OK;
}

static HRESULT WINAPI dictionary_Exists(IDictionary *iface, VARIANT *key, VARIANT_BOOL *exists)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_variant(key), exists);

    if (!exists)
        return CTL_E_ILLEGALFUNCTIONCALL;

    *exists = get_keyitem_pair(dictionary, key) != NULL ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI dictionary_Items(IDictionary *iface, VARIANT *items)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;
    SAFEARRAYBOUND bound;
    SAFEARRAY *sa;
    VARIANT *v;
    HRESULT hr;
    LONG i;

    TRACE("%p, %p.\n", iface, items);

    if (!items)
        return S_OK;

    bound.lLbound = 0;
    bound.cElements = dictionary->count;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
    if (!sa)
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void**)&v);
    if (FAILED(hr)) {
        SafeArrayDestroy(sa);
        return hr;
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(pair, &dictionary->pairs, struct keyitem_pair, entry)
    {
        VariantCopy(&v[i], &pair->item);
        i++;
    }
    SafeArrayUnaccessData(sa);

    V_VT(items) = VT_ARRAY|VT_VARIANT;
    V_ARRAY(items) = sa;
    return S_OK;
}

static HRESULT WINAPI dictionary_put_Key(IDictionary *iface, VARIANT *key, VARIANT *newkey)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;
    VARIANT empty;
    HRESULT hr;

    TRACE("%p, %s, %s.\n", iface, debugstr_variant(key), debugstr_variant(newkey));

    if ((pair = get_keyitem_pair(dictionary, key)))
    {
        /* found existing pair for a key, add new pair with new key
           and old item and remove old pair after that */

        hr = IDictionary_Add(iface, newkey, &pair->item);
        if (FAILED(hr))
            return hr;

        return IDictionary_Remove(iface, key);
    }

    VariantInit(&empty);
    return IDictionary_Add(iface, newkey, &empty);
}

static HRESULT WINAPI dictionary_Keys(IDictionary *iface, VARIANT *keys)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;
    SAFEARRAYBOUND bound;
    SAFEARRAY *sa;
    VARIANT *v;
    HRESULT hr;
    LONG i;

    TRACE("%p, %p.\n", iface, keys);

    if (!keys)
        return S_OK;

    bound.lLbound = 0;
    bound.cElements = dictionary->count;
    sa = SafeArrayCreate(VT_VARIANT, 1, &bound);
    if (!sa)
        return E_OUTOFMEMORY;

    hr = SafeArrayAccessData(sa, (void**)&v);
    if (FAILED(hr)) {
        SafeArrayDestroy(sa);
        return hr;
    }

    i = 0;
    LIST_FOR_EACH_ENTRY(pair, &dictionary->pairs, struct keyitem_pair, entry)
    {
        VariantCopy(&v[i], &pair->key);
        i++;
    }
    SafeArrayUnaccessData(sa);

    V_VT(keys) = VT_ARRAY|VT_VARIANT;
    V_ARRAY(keys) = sa;
    return S_OK;
}

static HRESULT WINAPI dictionary_Remove(IDictionary *iface, VARIANT *key)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair;

    TRACE("%p, %s.\n", iface, debugstr_variant(key));

    if (!(pair = get_keyitem_pair(dictionary, key)))
        return CTL_E_ELEMENT_NOT_FOUND;

    notify_remove_pair(&dictionary->notifier, &pair->entry);
    list_remove(&pair->entry);
    list_remove(&pair->bucket);
    dictionary->count--;

    free_keyitem_pair(pair);
    return S_OK;
}

static HRESULT WINAPI dictionary_RemoveAll(IDictionary *iface)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);
    struct keyitem_pair *pair, *pair2;

    TRACE("%p.\n", iface);

    if (!dictionary->count)
        return S_OK;

    notify_remove_pair(&dictionary->notifier, NULL);
    LIST_FOR_EACH_ENTRY_SAFE(pair, pair2, &dictionary->pairs, struct keyitem_pair, entry)
    {
        list_remove(&pair->entry);
        list_remove(&pair->bucket);
        free_keyitem_pair(pair);
    }
    dictionary->count = 0;

    return S_OK;
}

static HRESULT WINAPI dictionary_put_CompareMode(IDictionary *iface, CompareMethod method)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %d.\n", iface, method);

    if (dictionary->count)
        return CTL_E_ILLEGALFUNCTIONCALL;

    dictionary->method = method;
    return S_OK;
}

static HRESULT WINAPI dictionary_get_CompareMode(IDictionary *iface, CompareMethod *method)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %p.\n", iface, method);

    *method = dictionary->method;
    return S_OK;
}

static HRESULT WINAPI dictionary__NewEnum(IDictionary *iface, IUnknown **ret)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %p.\n", iface, ret);

    return create_dict_enum(dictionary, ret);
}

static DWORD get_str_hash(const WCHAR *str, CompareMethod method)
{
    DWORD hash = 0;

    if (str) {
        while (*str) {
            WCHAR ch;

            ch = (method == TextCompare || method == DatabaseCompare) ? towlower(*str) : *str;

            hash += (hash << 4) + ch;
            str++;
        }
    }

    return hash % DICT_HASH_MOD;
}

static DWORD get_num_hash(FLOAT num)
{
    return (*((DWORD*)&num)) % DICT_HASH_MOD;
}

static HRESULT get_flt_hash(FLOAT flt, LONG *hash)
{
    if (isinf(flt)) {
        *hash = 0;
        return S_OK;
    }
    else if (!isnan(flt)) {
        *hash = get_num_hash(flt);
        return S_OK;
    }

    /* NaN case */
    *hash = ~0u;
    return CTL_E_ILLEGALFUNCTIONCALL;
}

static DWORD get_ptr_hash(void *ptr)
{
    DWORD hash = PtrToUlong(ptr);
#ifdef _WIN64
    hash ^= (ULONG_PTR)ptr >> 32;
#endif
    return hash % 1201;
}

static HRESULT WINAPI dictionary_get_HashVal(IDictionary *iface, VARIANT *key, VARIANT *hash)
{
    struct dictionary *dictionary = impl_from_IDictionary(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_variant(key), hash);

    V_VT(hash) = VT_I4;
    switch (V_VT(key))
    {
    case VT_BSTR|VT_BYREF:
    case VT_BSTR:
        V_I4(hash) = get_str_hash(get_key_strptr(key), dictionary->method);
        break;
    case VT_UI1|VT_BYREF:
    case VT_UI1:
        V_I4(hash) = get_num_hash(V_VT(key) & VT_BYREF ? *V_UI1REF(key) : V_UI1(key));
        break;
    case VT_I2|VT_BYREF:
    case VT_I2:
        V_I4(hash) = get_num_hash(V_VT(key) & VT_BYREF ? *V_I2REF(key) : V_I2(key));
        break;
    case VT_I4|VT_BYREF:
    case VT_I4:
        V_I4(hash) = get_num_hash(V_VT(key) & VT_BYREF ? *V_I4REF(key) : V_I4(key));
        break;
    case VT_UNKNOWN|VT_BYREF:
    case VT_DISPATCH|VT_BYREF:
    case VT_UNKNOWN:
    case VT_DISPATCH:
    {
        IUnknown *src = (V_VT(key) & VT_BYREF) ? *V_UNKNOWNREF(key) : V_UNKNOWN(key);
        IUnknown *unk = NULL;

        if (!src) {
            V_I4(hash) = 0;
            return S_OK;
        }

        IUnknown_QueryInterface(src, &IID_IUnknown, (void**)&unk);
        if (!unk) {
            V_I4(hash) = ~0u;
            return CTL_E_ILLEGALFUNCTIONCALL;
        }
        V_I4(hash) = get_ptr_hash(unk);
        IUnknown_Release(unk);
        break;
    }
    case VT_DATE|VT_BYREF:
    case VT_DATE:
        return get_flt_hash(V_VT(key) & VT_BYREF ? *V_DATEREF(key) : V_DATE(key), &V_I4(hash));
    case VT_R4|VT_BYREF:
    case VT_R4:
        return get_flt_hash(V_VT(key) & VT_BYREF ? *V_R4REF(key) : V_R4(key), &V_I4(hash));
    case VT_R8|VT_BYREF:
    case VT_R8:
        return get_flt_hash(V_VT(key) & VT_BYREF ? *V_R8REF(key) : V_R8(key), &V_I4(hash));
    case VT_EMPTY:
    case VT_NULL:
        V_I4(hash) = 0;
        return S_OK;
    case VT_INT:
    case VT_UINT:
    case VT_I1:
    case VT_I8:
    case VT_UI2:
    case VT_UI4:
        V_I4(hash) = ~0u;
        return CTL_E_ILLEGALFUNCTIONCALL;
    default:
        FIXME("not implemented for type %d\n", V_VT(key));
        return E_NOTIMPL;
    }

    return S_OK;
}

static const struct IDictionaryVtbl dictionary_vtbl =
{
    dictionary_QueryInterface,
    dictionary_AddRef,
    dictionary_Release,
    dictionary_GetTypeInfoCount,
    dictionary_GetTypeInfo,
    dictionary_GetIDsOfNames,
    dictionary_Invoke,
    dictionary_putref_Item,
    dictionary_put_Item,
    dictionary_get_Item,
    dictionary_Add,
    dictionary_get_Count,
    dictionary_Exists,
    dictionary_Items,
    dictionary_put_Key,
    dictionary_Keys,
    dictionary_Remove,
    dictionary_RemoveAll,
    dictionary_put_CompareMode,
    dictionary_get_CompareMode,
    dictionary__NewEnum,
    dictionary_get_HashVal
};

HRESULT WINAPI Dictionary_CreateInstance(IClassFactory *factory, IUnknown *outer, REFIID riid, void **ret)
{
    struct dictionary *object;

    TRACE("(%p, %p, %s, %p)\n", factory, outer, debugstr_guid(riid), ret);

    *ret = NULL;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IDictionary_iface.lpVtbl = &dictionary_vtbl;
    object->ref = 1;
    object->method = BinaryCompare;
    list_init(&object->pairs);
    list_init(&object->notifier);

    init_classinfo(&CLSID_Dictionary, (IUnknown *)&object->IDictionary_iface, &object->classinfo);

    *ret = &object->IDictionary_iface;

    return S_OK;
}
