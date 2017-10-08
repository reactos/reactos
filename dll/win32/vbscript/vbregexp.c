/*
 * Copyright 2013 Piotr Caban for CodeWeavers
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

#include "vbscript.h"

#include <initguid.h>
#include <vbsregexp55.h>

#define REGEXP_TID_LIST \
    XDIID(RegExp2), \
    XDIID(Match2), \
    XDIID(MatchCollection2), \
    XDIID(SubMatches)

typedef enum {
#define XDIID(iface) iface ## _tid
    REGEXP_TID_LIST,
#undef XDIID
    REGEXP_LAST_tid
} regexp_tid_t;

static REFIID tid_ids[] = {
#define XDIID(iface) &IID_I ## iface
    REGEXP_TID_LIST
#undef XDIID
};

static ITypeLib *typelib;
static ITypeInfo *typeinfos[REGEXP_LAST_tid];

static HRESULT init_regexp_typeinfo(regexp_tid_t tid)
{
    HRESULT hres;

    if(!typelib) {
        static const WCHAR vbscript_dll3W[] = {'v','b','s','c','r','i','p','t','.','d','l','l','\\','3',0};
        ITypeLib *tl;

        hres = LoadTypeLib(vbscript_dll3W, &tl);
        if(FAILED(hres)) {
            ERR("LoadRegTypeLib failed: %08x\n", hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)&typelib, tl, NULL))
            ITypeLib_Release(tl);
    }

    if(!typeinfos[tid]) {
        ITypeInfo *ti;

        hres = ITypeLib_GetTypeInfoOfGuid(typelib, tid_ids[tid], &ti);
        if(FAILED(hres)) {
            ERR("GetTypeInfoOfGuid(%s) failed: %08x\n", debugstr_guid(tid_ids[tid]), hres);
            return hres;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    return S_OK;
}

struct SubMatches {
    ISubMatches ISubMatches_iface;

    LONG ref;

    WCHAR *match;
    match_state_t *result;
};

typedef struct Match2 {
    IMatch2 IMatch2_iface;
    IMatch IMatch_iface;

    LONG ref;

    DWORD index;
    SubMatches *sub_matches;
} Match2;

typedef struct MatchCollectionEnum {
    IEnumVARIANT IEnumVARIANT_iface;

    LONG ref;

    IMatchCollection2 *mc;
    LONG pos;
    LONG count;
} MatchCollectionEnum;

typedef struct MatchCollection2 {
    IMatchCollection2 IMatchCollection2_iface;
    IMatchCollection IMatchCollection_iface;

    LONG ref;

    IMatch2 **matches;
    DWORD count;
    DWORD size;
} MatchCollection2;

typedef struct RegExp2 {
    IRegExp2 IRegExp2_iface;
    IRegExp IRegExp_iface;

    LONG ref;

    WCHAR *pattern;
    regexp_t *regexp;
    heap_pool_t pool;
    WORD flags;
} RegExp2;

static inline SubMatches* impl_from_ISubMatches(ISubMatches *iface)
{
    return CONTAINING_RECORD(iface, SubMatches, ISubMatches_iface);
}

static HRESULT WINAPI SubMatches_QueryInterface(
        ISubMatches *iface, REFIID riid, void **ppv)
{
    SubMatches *This = impl_from_ISubMatches(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->ISubMatches_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->ISubMatches_iface;
    }else if(IsEqualGUID(riid, &IID_ISubMatches)) {
        TRACE("(%p)->(IID_ISubMatches %p)\n", This, ppv);
        *ppv = &This->ISubMatches_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatchEx)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI SubMatches_AddRef(ISubMatches *iface)
{
    SubMatches *This = impl_from_ISubMatches(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI SubMatches_Release(ISubMatches *iface)
{
    SubMatches *This = impl_from_ISubMatches(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This->match);
        heap_free(This->result);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI SubMatches_GetTypeInfoCount(ISubMatches *iface, UINT *pctinfo)
{
    SubMatches *This = impl_from_ISubMatches(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI SubMatches_GetTypeInfo(ISubMatches *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    SubMatches *This = impl_from_ISubMatches(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI SubMatches_GetIDsOfNames(ISubMatches *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    SubMatches *This = impl_from_ISubMatches(iface);

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(typeinfos[SubMatches_tid], rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI SubMatches_Invoke(ISubMatches *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    SubMatches *This = impl_from_ISubMatches(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return ITypeInfo_Invoke(typeinfos[SubMatches_tid], iface, dispIdMember, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI SubMatches_get_Item(ISubMatches *iface,
        LONG index, VARIANT *pSubMatch)
{
    SubMatches *This = impl_from_ISubMatches(iface);

    TRACE("(%p)->(%d %p)\n", This, index, pSubMatch);

    if(!pSubMatch)
        return E_POINTER;

    if(!This->result || index<0 || index>=This->result->paren_count)
        return E_INVALIDARG;

    if(This->result->parens[index].index == -1) {
        V_VT(pSubMatch) = VT_EMPTY;
    }else {
        V_VT(pSubMatch) = VT_BSTR;
        V_BSTR(pSubMatch) = SysAllocStringLen(
                This->match+This->result->parens[index].index,
                This->result->parens[index].length);

        if(!V_BSTR(pSubMatch))
            return E_OUTOFMEMORY;
    }

    return S_OK;
}

static HRESULT WINAPI SubMatches_get_Count(ISubMatches *iface, LONG *pCount)
{
    SubMatches *This = impl_from_ISubMatches(iface);

    TRACE("(%p)->(%p)\n", This, pCount);

    if(!pCount)
        return E_POINTER;

    if(!This->result)
        *pCount = 0;
    else
        *pCount = This->result->paren_count;
    return S_OK;
}

static HRESULT WINAPI SubMatches_get__NewEnum(ISubMatches *iface, IUnknown **ppEnum)
{
    SubMatches *This = impl_from_ISubMatches(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const ISubMatchesVtbl SubMatchesVtbl = {
    SubMatches_QueryInterface,
    SubMatches_AddRef,
    SubMatches_Release,
    SubMatches_GetTypeInfoCount,
    SubMatches_GetTypeInfo,
    SubMatches_GetIDsOfNames,
    SubMatches_Invoke,
    SubMatches_get_Item,
    SubMatches_get_Count,
    SubMatches_get__NewEnum
};

static HRESULT create_sub_matches(DWORD pos, match_state_t *result, SubMatches **sub_matches)
{
    SubMatches *ret;
    DWORD i;
    HRESULT hres;

    hres = init_regexp_typeinfo(SubMatches_tid);
    if(FAILED(hres))
        return hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->ISubMatches_iface.lpVtbl = &SubMatchesVtbl;

    ret->result = result;
    if(result) {
        ret->match = heap_alloc((result->match_len+1) * sizeof(WCHAR));
        if(!ret->match) {
            heap_free(ret);
            return E_OUTOFMEMORY;
        }
        memcpy(ret->match, result->cp-result->match_len, result->match_len*sizeof(WCHAR));
        ret->match[result->match_len] = 0;

        result->cp = NULL;
        for(i=0; i<result->paren_count; i++)
            if(result->parens[i].index != -1)
                result->parens[i].index -= pos;
    }else {
        ret->match = NULL;
    }

    ret->ref = 1;
    *sub_matches = ret;
    return hres;
}

static inline Match2* impl_from_IMatch2(IMatch2 *iface)
{
    return CONTAINING_RECORD(iface, Match2, IMatch2_iface);
}

static HRESULT WINAPI Match2_QueryInterface(
        IMatch2 *iface, REFIID riid, void **ppv)
{
    Match2 *This = impl_from_IMatch2(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IMatch2_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IMatch2_iface;
    }else if(IsEqualGUID(riid, &IID_IMatch2)) {
        TRACE("(%p)->(IID_IMatch2 %p)\n", This, ppv);
        *ppv = &This->IMatch2_iface;
    }else if(IsEqualGUID(riid, &IID_IMatch)) {
        TRACE("(%p)->(IID_IMatch %p)\n", This, ppv);
        *ppv = &This->IMatch_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatchEx)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Match2_AddRef(IMatch2 *iface)
{
    Match2 *This = impl_from_IMatch2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI Match2_Release(IMatch2 *iface)
{
    Match2 *This = impl_from_IMatch2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        ISubMatches_Release(&This->sub_matches->ISubMatches_iface);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI Match2_GetTypeInfoCount(IMatch2 *iface, UINT *pctinfo)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI Match2_GetTypeInfo(IMatch2 *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    Match2 *This = impl_from_IMatch2(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI Match2_GetIDsOfNames(IMatch2 *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(typeinfos[Match2_tid], rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI Match2_Invoke(IMatch2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return ITypeInfo_Invoke(typeinfos[Match2_tid], iface, dispIdMember, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI Match2_get_Value(IMatch2 *iface, BSTR *pValue)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%p)\n", This, pValue);

    if(!pValue)
        return E_POINTER;

    if(!This->sub_matches->match) {
        *pValue = NULL;
        return S_OK;
    }

    *pValue = SysAllocString(This->sub_matches->match);
    return *pValue ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI Match2_get_FirstIndex(IMatch2 *iface, LONG *pFirstIndex)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%p)\n", This, pFirstIndex);

    if(!pFirstIndex)
        return E_POINTER;

    *pFirstIndex = This->index;
    return S_OK;
}

static HRESULT WINAPI Match2_get_Length(IMatch2 *iface, LONG *pLength)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%p)\n", This, pLength);

    if(!pLength)
        return E_POINTER;

    if(This->sub_matches->result)
        *pLength = This->sub_matches->result->match_len;
    else
        *pLength = 0;
    return S_OK;
}

static HRESULT WINAPI Match2_get_SubMatches(IMatch2 *iface, IDispatch **ppSubMatches)
{
    Match2 *This = impl_from_IMatch2(iface);

    TRACE("(%p)->(%p)\n", This, ppSubMatches);

    if(!ppSubMatches)
        return E_POINTER;

    *ppSubMatches = (IDispatch*)&This->sub_matches->ISubMatches_iface;
    ISubMatches_AddRef(&This->sub_matches->ISubMatches_iface);
    return S_OK;
}

static const IMatch2Vtbl Match2Vtbl = {
    Match2_QueryInterface,
    Match2_AddRef,
    Match2_Release,
    Match2_GetTypeInfoCount,
    Match2_GetTypeInfo,
    Match2_GetIDsOfNames,
    Match2_Invoke,
    Match2_get_Value,
    Match2_get_FirstIndex,
    Match2_get_Length,
    Match2_get_SubMatches
};

static inline Match2 *impl_from_IMatch(IMatch *iface)
{
    return CONTAINING_RECORD(iface, Match2, IMatch_iface);
}

static HRESULT WINAPI Match_QueryInterface(IMatch *iface, REFIID riid, void **ppv)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_QueryInterface(&This->IMatch2_iface, riid, ppv);
}

static ULONG WINAPI Match_AddRef(IMatch *iface)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_AddRef(&This->IMatch2_iface);
}

static ULONG WINAPI Match_Release(IMatch *iface)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_Release(&This->IMatch2_iface);
}

static HRESULT WINAPI Match_GetTypeInfoCount(IMatch *iface, UINT *pctinfo)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_GetTypeInfoCount(&This->IMatch2_iface, pctinfo);
}

static HRESULT WINAPI Match_GetTypeInfo(IMatch *iface, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_GetTypeInfo(&This->IMatch2_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI Match_GetIDsOfNames(IMatch *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_GetIDsOfNames(&This->IMatch2_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI Match_Invoke(IMatch *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_Invoke(&This->IMatch2_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI Match_get_Value(IMatch *iface, BSTR *pValue)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_get_Value(&This->IMatch2_iface, pValue);
}

static HRESULT WINAPI Match_get_FirstIndex(IMatch *iface, LONG *pFirstIndex)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_get_FirstIndex(&This->IMatch2_iface, pFirstIndex);
}

static HRESULT WINAPI Match_get_Length(IMatch *iface, LONG *pLength)
{
    Match2 *This = impl_from_IMatch(iface);
    return IMatch2_get_Length(&This->IMatch2_iface, pLength);
}

static IMatchVtbl MatchVtbl = {
    Match_QueryInterface,
    Match_AddRef,
    Match_Release,
    Match_GetTypeInfoCount,
    Match_GetTypeInfo,
    Match_GetIDsOfNames,
    Match_Invoke,
    Match_get_Value,
    Match_get_FirstIndex,
    Match_get_Length
};

static HRESULT create_match2(DWORD pos, match_state_t **result, IMatch2 **match)
{
    Match2 *ret;
    HRESULT hres;

    hres = init_regexp_typeinfo(Match2_tid);
    if(FAILED(hres))
        return hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->index = pos;
    hres = create_sub_matches(pos, result ? *result : NULL, &ret->sub_matches);
    if(FAILED(hres)) {
        heap_free(ret);
        return hres;
    }
    if(result)
        *result = NULL;

    ret->IMatch2_iface.lpVtbl = &Match2Vtbl;
    ret->IMatch_iface.lpVtbl = &MatchVtbl;

    ret->ref = 1;
    *match = &ret->IMatch2_iface;
    return hres;
}

static inline MatchCollectionEnum* impl_from_IMatchCollectionEnum(IEnumVARIANT *iface)
{
    return CONTAINING_RECORD(iface, MatchCollectionEnum, IEnumVARIANT_iface);
}

static HRESULT WINAPI MatchCollectionEnum_QueryInterface(
        IEnumVARIANT *iface, REFIID riid, void **ppv)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IEnumVARIANT_iface;
    }else if(IsEqualGUID(riid, &IID_IEnumVARIANT)) {
        TRACE("(%p)->(IID_IEnumVARIANT %p)\n", This, ppv);
        *ppv = &This->IEnumVARIANT_iface;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MatchCollectionEnum_AddRef(IEnumVARIANT *iface)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI MatchCollectionEnum_Release(IEnumVARIANT *iface)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        IMatchCollection2_Release(This->mc);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI MatchCollectionEnum_Next(IEnumVARIANT *iface,
        ULONG celt, VARIANT *rgVar, ULONG *pCeltFetched)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);
    DWORD i;
    HRESULT hres = S_OK;

    TRACE("(%p)->(%u %p %p)\n", This, celt, rgVar, pCeltFetched);

    if(This->pos>=This->count) {
        if(pCeltFetched)
            *pCeltFetched = 0;
        return S_FALSE;
    }

    for(i=0; i<celt && This->pos+i<This->count; i++) {
        V_VT(rgVar+i) = VT_DISPATCH;
        hres = IMatchCollection2_get_Item(This->mc, This->pos+i, &V_DISPATCH(rgVar+i));
        if(FAILED(hres))
            break;
    }
    if(FAILED(hres)) {
        while(i--)
            VariantClear(rgVar+i);
        return hres;
    }

    if(pCeltFetched)
        *pCeltFetched = i;
    This->pos += i;
    return S_OK;
}

static HRESULT WINAPI MatchCollectionEnum_Skip(IEnumVARIANT *iface, ULONG celt)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);

    TRACE("(%p)->(%u)\n", This, celt);

    if(This->pos+celt <= This->count)
        This->pos += celt;
    else
        This->pos = This->count;
    return S_OK;
}

static HRESULT WINAPI MatchCollectionEnum_Reset(IEnumVARIANT *iface)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);

    TRACE("(%p)\n", This);

    This->pos = 0;
    return S_OK;
}

static HRESULT WINAPI MatchCollectionEnum_Clone(IEnumVARIANT *iface, IEnumVARIANT **ppEnum)
{
    MatchCollectionEnum *This = impl_from_IMatchCollectionEnum(iface);
    FIXME("(%p)->(%p)\n", This, ppEnum);
    return E_NOTIMPL;
}

static const IEnumVARIANTVtbl MatchCollectionEnum_Vtbl = {
    MatchCollectionEnum_QueryInterface,
    MatchCollectionEnum_AddRef,
    MatchCollectionEnum_Release,
    MatchCollectionEnum_Next,
    MatchCollectionEnum_Skip,
    MatchCollectionEnum_Reset,
    MatchCollectionEnum_Clone
};

static HRESULT create_enum_variant_mc2(IMatchCollection2 *mc, ULONG pos, IEnumVARIANT **enum_variant)
{
    MatchCollectionEnum *ret;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IEnumVARIANT_iface.lpVtbl = &MatchCollectionEnum_Vtbl;
    ret->ref = 1;
    ret->pos = pos;
    IMatchCollection2_get_Count(mc, &ret->count);
    ret->mc = mc;
    IMatchCollection2_AddRef(mc);

    *enum_variant = &ret->IEnumVARIANT_iface;
    return S_OK;
}

static inline MatchCollection2* impl_from_IMatchCollection2(IMatchCollection2 *iface)
{
    return CONTAINING_RECORD(iface, MatchCollection2, IMatchCollection2_iface);
}

static HRESULT WINAPI MatchCollection2_QueryInterface(
        IMatchCollection2 *iface, REFIID riid, void **ppv)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IMatchCollection2_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IMatchCollection2_iface;
    }else if(IsEqualGUID(riid, &IID_IMatchCollection2)) {
        TRACE("(%p)->(IID_IMatchCollection2 %p)\n", This, ppv);
        *ppv = &This->IMatchCollection2_iface;
    }else if(IsEqualGUID(riid, &IID_IMatchCollection)) {
        TRACE("(%p)->(IID_IMatchCollection %p)\n", This, ppv);
        *ppv = &This->IMatchCollection_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatchEx)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI MatchCollection2_AddRef(IMatchCollection2 *iface)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI MatchCollection2_Release(IMatchCollection2 *iface)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        DWORD i;

        for(i=0; i<This->count; i++)
            IMatch2_Release(This->matches[i]);
        heap_free(This->matches);

        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI MatchCollection2_GetTypeInfoCount(IMatchCollection2 *iface, UINT *pctinfo)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI MatchCollection2_GetTypeInfo(IMatchCollection2 *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI MatchCollection2_GetIDsOfNames(IMatchCollection2 *iface,
        REFIID riid, LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(typeinfos[MatchCollection2_tid], rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI MatchCollection2_Invoke(IMatchCollection2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return ITypeInfo_Invoke(typeinfos[MatchCollection2_tid], iface, dispIdMember, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI MatchCollection2_get_Item(IMatchCollection2 *iface,
        LONG index, IDispatch **ppMatch)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->()\n", This);

    if(!ppMatch)
        return E_POINTER;

    if(index<0 || index>=This->count)
        return E_INVALIDARG;

    *ppMatch = (IDispatch*)This->matches[index];
    IMatch2_AddRef(This->matches[index]);
    return S_OK;
}

static HRESULT WINAPI MatchCollection2_get_Count(IMatchCollection2 *iface, LONG *pCount)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->()\n", This);

    if(!pCount)
        return E_POINTER;

    *pCount = This->count;
    return S_OK;
}

static HRESULT WINAPI MatchCollection2_get__NewEnum(IMatchCollection2 *iface, IUnknown **ppEnum)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->(%p)\n", This, ppEnum);

    if(!ppEnum)
        return E_POINTER;

    return create_enum_variant_mc2(&This->IMatchCollection2_iface, 0, (IEnumVARIANT**)ppEnum);
}

static const IMatchCollection2Vtbl MatchCollection2Vtbl = {
    MatchCollection2_QueryInterface,
    MatchCollection2_AddRef,
    MatchCollection2_Release,
    MatchCollection2_GetTypeInfoCount,
    MatchCollection2_GetTypeInfo,
    MatchCollection2_GetIDsOfNames,
    MatchCollection2_Invoke,
    MatchCollection2_get_Item,
    MatchCollection2_get_Count,
    MatchCollection2_get__NewEnum
};

static inline MatchCollection2 *impl_from_IMatchCollection(IMatchCollection *iface)
{
    return CONTAINING_RECORD(iface, MatchCollection2, IMatchCollection_iface);
}

static HRESULT WINAPI MatchCollection_QueryInterface(IMatchCollection *iface, REFIID riid, void **ppv)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_QueryInterface(&This->IMatchCollection2_iface, riid, ppv);
}

static ULONG WINAPI MatchCollection_AddRef(IMatchCollection *iface)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_AddRef(&This->IMatchCollection2_iface);
}

static ULONG WINAPI MatchCollection_Release(IMatchCollection *iface)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_Release(&This->IMatchCollection2_iface);
}

static HRESULT WINAPI MatchCollection_GetTypeInfoCount(IMatchCollection *iface, UINT *pctinfo)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_GetTypeInfoCount(&This->IMatchCollection2_iface, pctinfo);
}

static HRESULT WINAPI MatchCollection_GetTypeInfo(IMatchCollection *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_GetTypeInfo(&This->IMatchCollection2_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI MatchCollection_GetIDsOfNames(IMatchCollection *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_GetIDsOfNames(&This->IMatchCollection2_iface,
            riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI MatchCollection_Invoke(IMatchCollection *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_Invoke(&This->IMatchCollection2_iface, dispIdMember,
            riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI MatchCollection_get_Item(IMatchCollection *iface, LONG index, IDispatch **ppMatch)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_get_Item(&This->IMatchCollection2_iface, index, ppMatch);
}

static HRESULT WINAPI MatchCollection_get_Count(IMatchCollection *iface, LONG *pCount)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_get_Count(&This->IMatchCollection2_iface, pCount);
}

static HRESULT WINAPI MatchCollection_get__NewEnum(IMatchCollection *iface, IUnknown **ppEnum)
{
    MatchCollection2 *This = impl_from_IMatchCollection(iface);
    return IMatchCollection2_get__NewEnum(&This->IMatchCollection2_iface, ppEnum);
}

static const IMatchCollectionVtbl MatchCollectionVtbl = {
    MatchCollection_QueryInterface,
    MatchCollection_AddRef,
    MatchCollection_Release,
    MatchCollection_GetTypeInfoCount,
    MatchCollection_GetTypeInfo,
    MatchCollection_GetIDsOfNames,
    MatchCollection_Invoke,
    MatchCollection_get_Item,
    MatchCollection_get_Count,
    MatchCollection_get__NewEnum
};

static HRESULT add_match(IMatchCollection2 *iface, IMatch2 *add)
{
    MatchCollection2 *This = impl_from_IMatchCollection2(iface);

    TRACE("(%p)->(%p)\n", This, add);

    if(!This->size) {
        This->matches = heap_alloc(8*sizeof(IMatch*));
        if(!This->matches)
            return E_OUTOFMEMORY;
        This->size = 8;
    }else if(This->size == This->count) {
        IMatch2 **new_matches = heap_realloc(This->matches, 2*This->size*sizeof(IMatch*));
        if(!new_matches)
            return E_OUTOFMEMORY;

        This->matches = new_matches;
        This->size <<= 1;
    }

    This->matches[This->count++] = add;
    IMatch2_AddRef(add);
    return S_OK;
}

static HRESULT create_match_collection2(IMatchCollection2 **match_collection)
{
    MatchCollection2 *ret;
    HRESULT hres;

    hres = init_regexp_typeinfo(MatchCollection2_tid);
    if(FAILED(hres))
        return hres;

    ret = heap_alloc_zero(sizeof(*ret));
    if(!ret)
        return E_OUTOFMEMORY;

    ret->IMatchCollection2_iface.lpVtbl = &MatchCollection2Vtbl;
    ret->IMatchCollection_iface.lpVtbl = &MatchCollectionVtbl;

    ret->ref = 1;
    *match_collection = &ret->IMatchCollection2_iface;
    return S_OK;
}

static inline RegExp2 *impl_from_IRegExp2(IRegExp2 *iface)
{
    return CONTAINING_RECORD(iface, RegExp2, IRegExp2_iface);
}

static HRESULT WINAPI RegExp2_QueryInterface(IRegExp2 *iface, REFIID riid, void **ppv)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    if(IsEqualGUID(riid, &IID_IUnknown)) {
        TRACE("(%p)->(IID_IUnknown %p)\n", This, ppv);
        *ppv = &This->IRegExp2_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatch)) {
        TRACE("(%p)->(IID_IDispatch %p)\n", This, ppv);
        *ppv = &This->IRegExp2_iface;
    }else if(IsEqualGUID(riid, &IID_IRegExp2)) {
        TRACE("(%p)->(IID_IRegExp2 %p)\n", This, ppv);
        *ppv = &This->IRegExp2_iface;
    }else if(IsEqualGUID(riid, &IID_IRegExp)) {
        TRACE("(%p)->(IID_IRegExp %p)\n", This, ppv);
        *ppv = &This->IRegExp_iface;
    }else if(IsEqualGUID(riid, &IID_IDispatchEx)) {
        TRACE("(%p)->(IID_IDispatchEx %p)\n", This, ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }else {
        FIXME("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI RegExp2_AddRef(IRegExp2 *iface)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI RegExp2_Release(IRegExp2 *iface)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        heap_free(This->pattern);
        if(This->regexp)
            regexp_destroy(This->regexp);
        heap_pool_free(&This->pool);
        heap_free(This);
    }

    return ref;
}

static HRESULT WINAPI RegExp2_GetTypeInfoCount(IRegExp2 *iface, UINT *pctinfo)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pctinfo);

    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI RegExp2_GetTypeInfo(IRegExp2 *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%u %u %p)\n", This, iTInfo, lcid, ppTInfo);
    return E_NOTIMPL;
}

static HRESULT WINAPI RegExp2_GetIDsOfNames(IRegExp2 *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%s %p %u %u %p)\n", This, debugstr_guid(riid),
            rgszNames, cNames, lcid, rgDispId);

    return ITypeInfo_GetIDsOfNames(typeinfos[RegExp2_tid], rgszNames, cNames, rgDispId);
}

static HRESULT WINAPI RegExp2_Invoke(IRegExp2 *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
        VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%d %s %d %d %p %p %p %p)\n", This, dispIdMember, debugstr_guid(riid),
            lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);

    return ITypeInfo_Invoke(typeinfos[RegExp2_tid], iface, dispIdMember, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI RegExp2_get_Pattern(IRegExp2 *iface, BSTR *pPattern)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pPattern);

    if(!pPattern)
        return E_POINTER;

    if(!This->pattern) {
        *pPattern = NULL;
        return S_OK;
    }

    *pPattern = SysAllocString(This->pattern);
    return *pPattern ? S_OK : E_OUTOFMEMORY;
}

static HRESULT WINAPI RegExp2_put_Pattern(IRegExp2 *iface, BSTR pattern)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    WCHAR *new_pattern;

    TRACE("(%p)->(%s)\n", This, wine_dbgstr_w(pattern));

    if(pattern && *pattern) {
        SIZE_T size = (SysStringLen(pattern)+1) * sizeof(WCHAR);
        new_pattern = heap_alloc(size);
        if(!new_pattern)
            return E_OUTOFMEMORY;
        memcpy(new_pattern, pattern, size);
    }else {
        new_pattern = NULL;
    }

    heap_free(This->pattern);
    This->pattern = new_pattern;

    if(This->regexp) {
        regexp_destroy(This->regexp);
        This->regexp = NULL;
    }
    return S_OK;
}

static HRESULT WINAPI RegExp2_get_IgnoreCase(IRegExp2 *iface, VARIANT_BOOL *pIgnoreCase)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pIgnoreCase);

    if(!pIgnoreCase)
        return E_POINTER;

    *pIgnoreCase = This->flags & REG_FOLD ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI RegExp2_put_IgnoreCase(IRegExp2 *iface, VARIANT_BOOL ignoreCase)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%s)\n", This, ignoreCase ? "true" : "false");

    if(ignoreCase)
        This->flags |= REG_FOLD;
    else
        This->flags &= ~REG_FOLD;
    return S_OK;
}

static HRESULT WINAPI RegExp2_get_Global(IRegExp2 *iface, VARIANT_BOOL *pGlobal)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pGlobal);

    if(!pGlobal)
        return E_POINTER;

    *pGlobal = This->flags & REG_GLOB ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI RegExp2_put_Global(IRegExp2 *iface, VARIANT_BOOL global)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%s)\n", This, global ? "true" : "false");

    if(global)
        This->flags |= REG_GLOB;
    else
        This->flags &= ~REG_GLOB;
    return S_OK;
}

static HRESULT WINAPI RegExp2_get_Multiline(IRegExp2 *iface, VARIANT_BOOL *pMultiline)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%p)\n", This, pMultiline);

    if(!pMultiline)
        return E_POINTER;

    *pMultiline = This->flags & REG_MULTILINE ? VARIANT_TRUE : VARIANT_FALSE;
    return S_OK;
}

static HRESULT WINAPI RegExp2_put_Multiline(IRegExp2 *iface, VARIANT_BOOL multiline)
{
    RegExp2 *This = impl_from_IRegExp2(iface);

    TRACE("(%p)->(%s)\n", This, multiline ? "true" : "false");

    if(multiline)
        This->flags |= REG_MULTILINE;
    else
        This->flags &= ~REG_MULTILINE;
    return S_OK;
}

static HRESULT WINAPI RegExp2_Execute(IRegExp2 *iface,
        BSTR sourceString, IDispatch **ppMatches)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    match_state_t *result;
    const WCHAR *pos;
    IMatchCollection2 *match_collection;
    IMatch2 *add = NULL;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(sourceString), ppMatches);

    if(!This->pattern) {
        DWORD i, len = SysStringLen(sourceString);

        hres = create_match_collection2(&match_collection);
        if(FAILED(hres))
            return hres;

        for(i=0; i<=len; i++) {
            hres = create_match2(i, NULL, &add);
            if(FAILED(hres))
                break;

            hres = add_match(match_collection, add);
            if(FAILED(hres))
                break;
            IMatch2_Release(add);

            if(!(This->flags & REG_GLOB))
                break;
        }

        if(FAILED(hres)) {
            IMatchCollection2_Release(match_collection);
            return hres;
        }

        *ppMatches = (IDispatch*)match_collection;
        return S_OK;
    }

    if(!This->regexp) {
        This->regexp = regexp_new(NULL, &This->pool, This->pattern,
                strlenW(This->pattern), This->flags, FALSE);
        if(!This->regexp)
            return E_FAIL;
    }else {
        hres = regexp_set_flags(&This->regexp, NULL, &This->pool, This->flags);
        if(FAILED(hres))
            return hres;
    }

    hres = create_match_collection2(&match_collection);
    if(FAILED(hres))
        return hres;

    pos = sourceString;
    while(1) {
        result = alloc_match_state(This->regexp, NULL, pos);
        if(!result) {
            hres = E_OUTOFMEMORY;
            break;
        }

        hres = regexp_execute(This->regexp, NULL, &This->pool,
                sourceString, SysStringLen(sourceString), result);
        if(hres != S_OK) {
            heap_free(result);
            break;
        }
        pos = result->cp;

        hres = create_match2(result->cp-result->match_len-sourceString, &result, &add);
        heap_free(result);
        if(FAILED(hres))
            break;
        hres = add_match(match_collection, add);
        IMatch2_Release(add);
        if(FAILED(hres))
            break;

        if(!(This->flags & REG_GLOB))
            break;
    }

    if(FAILED(hres)) {
        IMatchCollection2_Release(match_collection);
        return hres;
    }

    *ppMatches = (IDispatch*)match_collection;
    return S_OK;
}

static HRESULT WINAPI RegExp2_Test(IRegExp2 *iface, BSTR sourceString, VARIANT_BOOL *pMatch)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    match_state_t *result;
    heap_pool_t *mark;
    HRESULT hres;

    TRACE("(%p)->(%s %p)\n", This, debugstr_w(sourceString), pMatch);

    if(!This->pattern) {
        *pMatch = VARIANT_TRUE;
        return S_OK;
    }

    if(!This->regexp) {
        This->regexp = regexp_new(NULL, &This->pool, This->pattern,
                strlenW(This->pattern), This->flags, FALSE);
        if(!This->regexp)
            return E_FAIL;
    }else {
        hres = regexp_set_flags(&This->regexp, NULL, &This->pool, This->flags);
        if(FAILED(hres))
            return hres;
    }

    mark = heap_pool_mark(&This->pool);
    result = alloc_match_state(This->regexp, &This->pool, sourceString);
    if(!result) {
        heap_pool_clear(mark);
        return E_OUTOFMEMORY;
    }

    hres = regexp_execute(This->regexp, NULL, &This->pool,
            sourceString, SysStringLen(sourceString), result);

    heap_pool_clear(mark);

    if(hres == S_OK) {
        *pMatch = VARIANT_TRUE;
    }else if(hres == S_FALSE) {
        *pMatch = VARIANT_FALSE;
        hres = S_OK;
    }
    return hres;
}

static HRESULT WINAPI RegExp2_Replace(IRegExp2 *iface, BSTR sourceString,
        VARIANT replaceVar, BSTR *pDestString)
{
    RegExp2 *This = impl_from_IRegExp2(iface);
    FIXME("(%p)->(%s %s %p)\n", This, debugstr_w(sourceString),
            debugstr_variant(&replaceVar), pDestString);
    return E_NOTIMPL;
}

static const IRegExp2Vtbl RegExp2Vtbl = {
    RegExp2_QueryInterface,
    RegExp2_AddRef,
    RegExp2_Release,
    RegExp2_GetTypeInfoCount,
    RegExp2_GetTypeInfo,
    RegExp2_GetIDsOfNames,
    RegExp2_Invoke,
    RegExp2_get_Pattern,
    RegExp2_put_Pattern,
    RegExp2_get_IgnoreCase,
    RegExp2_put_IgnoreCase,
    RegExp2_get_Global,
    RegExp2_put_Global,
    RegExp2_get_Multiline,
    RegExp2_put_Multiline,
    RegExp2_Execute,
    RegExp2_Test,
    RegExp2_Replace
};

static inline RegExp2 *impl_from_IRegExp(IRegExp *iface)
{
    return CONTAINING_RECORD(iface, RegExp2, IRegExp_iface);
}

static HRESULT WINAPI RegExp_QueryInterface(IRegExp *iface, REFIID riid, void **ppv)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_QueryInterface(&This->IRegExp2_iface, riid, ppv);
}

static ULONG WINAPI RegExp_AddRef(IRegExp *iface)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_AddRef(&This->IRegExp2_iface);
}

static ULONG WINAPI RegExp_Release(IRegExp *iface)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Release(&This->IRegExp2_iface);
}

static HRESULT WINAPI RegExp_GetTypeInfoCount(IRegExp *iface, UINT *pctinfo)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_GetTypeInfoCount(&This->IRegExp2_iface, pctinfo);
}

static HRESULT WINAPI RegExp_GetTypeInfo(IRegExp *iface,
        UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_GetTypeInfo(&This->IRegExp2_iface, iTInfo, lcid, ppTInfo);
}

static HRESULT WINAPI RegExp_GetIDsOfNames(IRegExp *iface, REFIID riid,
        LPOLESTR *rgszNames, UINT cNames, LCID lcid, DISPID *rgDispId)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_GetIDsOfNames(&This->IRegExp2_iface, riid, rgszNames, cNames, lcid, rgDispId);
}

static HRESULT WINAPI RegExp_Invoke(IRegExp *iface, DISPID dispIdMember,
        REFIID riid, LCID lcid, WORD wFlags, DISPPARAMS *pDispParams,
                VARIANT *pVarResult, EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Invoke(&This->IRegExp2_iface, dispIdMember, riid, lcid,
            wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT WINAPI RegExp_get_Pattern(IRegExp *iface, BSTR *pPattern)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_get_Pattern(&This->IRegExp2_iface, pPattern);
}

static HRESULT WINAPI RegExp_put_Pattern(IRegExp *iface, BSTR pPattern)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_put_Pattern(&This->IRegExp2_iface, pPattern);
}

static HRESULT WINAPI RegExp_get_IgnoreCase(IRegExp *iface, VARIANT_BOOL *pIgnoreCase)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_get_IgnoreCase(&This->IRegExp2_iface, pIgnoreCase);
}

static HRESULT WINAPI RegExp_put_IgnoreCase(IRegExp *iface, VARIANT_BOOL pIgnoreCase)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_put_IgnoreCase(&This->IRegExp2_iface, pIgnoreCase);
}

static HRESULT WINAPI RegExp_get_Global(IRegExp *iface, VARIANT_BOOL *pGlobal)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_get_Global(&This->IRegExp2_iface, pGlobal);
}

static HRESULT WINAPI RegExp_put_Global(IRegExp *iface, VARIANT_BOOL pGlobal)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_put_Global(&This->IRegExp2_iface, pGlobal);
}

static HRESULT WINAPI RegExp_Execute(IRegExp *iface,
        BSTR sourceString, IDispatch **ppMatches)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Execute(&This->IRegExp2_iface, sourceString, ppMatches);
}

static HRESULT WINAPI RegExp_Test(IRegExp *iface, BSTR sourceString, VARIANT_BOOL *pMatch)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    return IRegExp2_Test(&This->IRegExp2_iface, sourceString, pMatch);
}

static HRESULT WINAPI RegExp_Replace(IRegExp *iface, BSTR sourceString,
        BSTR replaceString, BSTR *pDestString)
{
    RegExp2 *This = impl_from_IRegExp(iface);
    VARIANT replace;

    V_VT(&replace) = VT_BSTR;
    V_BSTR(&replace) = replaceString;
    return IRegExp2_Replace(&This->IRegExp2_iface, sourceString, replace, pDestString);
}

static IRegExpVtbl RegExpVtbl = {
    RegExp_QueryInterface,
    RegExp_AddRef,
    RegExp_Release,
    RegExp_GetTypeInfoCount,
    RegExp_GetTypeInfo,
    RegExp_GetIDsOfNames,
    RegExp_Invoke,
    RegExp_get_Pattern,
    RegExp_put_Pattern,
    RegExp_get_IgnoreCase,
    RegExp_put_IgnoreCase,
    RegExp_get_Global,
    RegExp_put_Global,
    RegExp_Execute,
    RegExp_Test,
    RegExp_Replace
};

HRESULT create_regexp(IDispatch **ret)
{
    RegExp2 *regexp;
    HRESULT hres;

    hres = init_regexp_typeinfo(RegExp2_tid);
    if(FAILED(hres))
        return hres;

    regexp = heap_alloc_zero(sizeof(*regexp));
    if(!regexp)
        return E_OUTOFMEMORY;

    regexp->IRegExp2_iface.lpVtbl = &RegExp2Vtbl;
    regexp->IRegExp_iface.lpVtbl = &RegExpVtbl;
    regexp->ref = 1;
    heap_pool_init(&regexp->pool);

    *ret = (IDispatch*)&regexp->IRegExp2_iface;
    return S_OK;
}

HRESULT WINAPI VBScriptRegExpFactory_CreateInstance(IClassFactory *iface, IUnknown *pUnkOuter, REFIID riid, void **ppv)
{
    IDispatch *regexp;
    HRESULT hres;

    TRACE("(%p %s %p)\n", pUnkOuter, debugstr_guid(riid), ppv);

    hres = create_regexp(&regexp);
    if(FAILED(hres))
        return hres;

    hres = IDispatch_QueryInterface(regexp, riid, ppv);
    IDispatch_Release(regexp);
    return hres;
}

void release_regexp_typelib(void)
{
    DWORD i;

    for(i=0; i<REGEXP_LAST_tid; i++) {
        if(typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);
    }
    if(typelib)
        ITypeLib_Release(typelib);
}
