/*
 * Copyright 2005 Jacek Caban
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
#include "objbase.h"
#include "oaidl.h"
#include "oleauto.h"
#include "variant.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

typedef struct {
    enum VARENUM vt;
    VARKIND varkind;
    ULONG offset;
    BSTR name;
} fieldstr;

typedef struct {
    IRecordInfo IRecordInfo_iface;
    LONG ref;

    GUID guid;
    UINT lib_index;
    WORD n_vars;
    ULONG size;
    BSTR name;
    fieldstr *fields;
    ITypeInfo *pTypeInfo;
} IRecordInfoImpl;

static inline IRecordInfoImpl *impl_from_IRecordInfo(IRecordInfo *iface)
{
    return CONTAINING_RECORD(iface, IRecordInfoImpl, IRecordInfo_iface);
}

static HRESULT copy_to_variant(void *src, VARIANT *pvar, enum VARENUM vt)
{
    TRACE("%p %p %d\n", src, pvar, vt);

#define CASE_COPY(x) \
    case VT_ ## x: \
        memcpy(&V_ ## x(pvar), src, sizeof(V_ ## x(pvar))); \
        break 

    switch(vt) {
        CASE_COPY(I2);
        CASE_COPY(I4);
        CASE_COPY(R4);
        CASE_COPY(R8);
        CASE_COPY(CY);
        CASE_COPY(DATE);
        CASE_COPY(BSTR);
        CASE_COPY(ERROR);
        CASE_COPY(BOOL);
        CASE_COPY(DECIMAL);
        CASE_COPY(I1);
        CASE_COPY(UI1);
        CASE_COPY(UI2);
        CASE_COPY(UI4);
        CASE_COPY(I8);
        CASE_COPY(UI8);
        CASE_COPY(INT);
        CASE_COPY(UINT);
        CASE_COPY(INT_PTR);
        CASE_COPY(UINT_PTR);
    default:
        FIXME("Not supported type: %d\n", vt);
        return E_NOTIMPL;
    };
#undef CASE_COPY

    V_VT(pvar) = vt;
    return S_OK;
}

static HRESULT copy_from_variant(VARIANT *src, void *dest, enum VARENUM vt)
{
    VARIANT var;
    HRESULT hres;

    TRACE("(%p(%d) %p %d)\n", src, V_VT(src), dest, vt);

    hres = VariantChangeType(&var, src, 0, vt);
    if(FAILED(hres))
        return hres;

#define CASE_COPY(x) \
    case VT_ ## x: \
        memcpy(dest, &V_ ## x(&var), sizeof(V_ ## x(&var))); \
        break

    switch(vt) {
        CASE_COPY(I2);
        CASE_COPY(I4);
        CASE_COPY(R4);
        CASE_COPY(R8);
        CASE_COPY(CY);
        CASE_COPY(DATE);
        CASE_COPY(BSTR);
        CASE_COPY(ERROR);
        CASE_COPY(BOOL);
        CASE_COPY(DECIMAL);
        CASE_COPY(I1);
        CASE_COPY(UI1);
        CASE_COPY(UI2);
        CASE_COPY(UI4);
        CASE_COPY(I8);
        CASE_COPY(UI8);
        CASE_COPY(INT);
        CASE_COPY(UINT);
        CASE_COPY(INT_PTR);
        CASE_COPY(UINT_PTR);
    default:
        FIXME("Not supported type: %d\n", V_VT(&var));
        return E_NOTIMPL;
    };
#undef CASE_COPY
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_QueryInterface(IRecordInfo *iface, REFIID riid,
                                                void **ppvObject)
{
    TRACE("(%p)->(%s %p)\n", iface, debugstr_guid(riid), ppvObject);

    *ppvObject = NULL;

    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IRecordInfo, riid)) {
       *ppvObject = iface;
       IRecordInfo_AddRef(iface);
       return S_OK;
    }

    FIXME("Not supported interface: %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI IRecordInfoImpl_AddRef(IRecordInfo *iface)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    ULONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p, refcount %lu.\n", iface, ref);
    return ref;
}

static ULONG WINAPI IRecordInfoImpl_Release(IRecordInfo *iface)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if(!ref) {
        int i;
        for(i=0; i<This->n_vars; i++)
            SysFreeString(This->fields[i].name);
        SysFreeString(This->name);
        free(This->fields);
        ITypeInfo_Release(This->pTypeInfo);
        free(This);
    }
    return ref;
}

static HRESULT WINAPI IRecordInfoImpl_RecordInit(IRecordInfo *iface, PVOID pvNew)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    TRACE("(%p)->(%p)\n", This, pvNew);

    if(!pvNew)
        return E_INVALIDARG;

    memset(pvNew, 0, This->size);
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_RecordClear(IRecordInfo *iface, PVOID pvExisting)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    int i;
    PVOID var;

    TRACE("(%p)->(%p)\n", This, pvExisting);

    if(!pvExisting)
        return E_INVALIDARG;

    for(i=0; i<This->n_vars; i++) {
        if(This->fields[i].varkind != VAR_PERINSTANCE) {
            ERR("varkind != VAR_PERINSTANCE\n");
            continue;
        }
        var = ((PBYTE)pvExisting)+This->fields[i].offset;
        switch(This->fields[i].vt) {
            case VT_BSTR:
               SysFreeString(*(BSTR*)var);
                *(BSTR*)var = NULL;
                break;
            case VT_I2:
            case VT_I4:
            case VT_R4:
            case VT_R8:
            case VT_CY:
            case VT_DATE:
            case VT_ERROR:
            case VT_BOOL:
            case VT_DECIMAL:
            case VT_I1:
            case VT_UI1:
            case VT_UI2:
            case VT_UI4:
            case VT_I8:
            case VT_UI8:
            case VT_INT:
            case VT_UINT:
            case VT_HRESULT:
                break;
            case VT_INT_PTR:
            case VT_UINT_PTR:
                *(void**)var = NULL;
                break;
            case VT_SAFEARRAY:
                SafeArrayDestroy(var);
                break;
            case VT_UNKNOWN:
            case VT_DISPATCH:
            {
                IUnknown *unk = *(IUnknown**)var;
                if (unk)
                    IUnknown_Release(unk);
                *(void**)var = NULL;
                break;
            }
            default:
                FIXME("Not supported vt = %d\n", This->fields[i].vt);
                break;
        }
    }
    
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_RecordCopy(IRecordInfo *iface, void *src_rec, void *dest_rec)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    HRESULT hr = S_OK;
    int i;

    TRACE("(%p)->(%p %p)\n", This, src_rec, dest_rec);

    if(!src_rec || !dest_rec)
        return E_INVALIDARG;

    /* release already stored data */
    IRecordInfo_RecordClear(iface, dest_rec);

    for (i = 0; i < This->n_vars; i++)
    {
        void *src, *dest;

        if (This->fields[i].varkind != VAR_PERINSTANCE) {
            ERR("varkind != VAR_PERINSTANCE\n");
            continue;
        }

        src  = ((BYTE*)src_rec) + This->fields[i].offset;
        dest = ((BYTE*)dest_rec) + This->fields[i].offset;
        switch (This->fields[i].vt)
        {
            case VT_BSTR:
            {
                BSTR src_str = *(BSTR*)src;

                if (src_str)
                {
                    BSTR str = SysAllocString(*(BSTR*)src);
                    if (!str) hr = E_OUTOFMEMORY;

                    *(BSTR*)dest = str;
                }
                else
                    *(BSTR*)dest = NULL;
                break;
            }
            case VT_UNKNOWN:
            case VT_DISPATCH:
            {
                IUnknown *unk = *(IUnknown**)src;
                *(IUnknown**)dest = unk;
                if (unk) IUnknown_AddRef(unk);
                break;
            }
            case VT_SAFEARRAY:
                hr = SafeArrayCopy(src, dest);
                break;
            default:
            {
                /* copy directly for types that don't need deep copy */
                int len = get_type_size(NULL, This->fields[i].vt);
                memcpy(dest, src, len);
                break;
            }
        }

        if (FAILED(hr)) break;
    }

    if (FAILED(hr))
        IRecordInfo_RecordClear(iface, dest_rec);

    return hr;
}

static HRESULT WINAPI IRecordInfoImpl_GetGuid(IRecordInfo *iface, GUID *pguid)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);

    TRACE("(%p)->(%p)\n", This, pguid);

    if(!pguid)
        return E_INVALIDARG;

    *pguid = This->guid;
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetName(IRecordInfo *iface, BSTR *pbstrName)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);

    TRACE("(%p)->(%p)\n", This, pbstrName);

    if(!pbstrName)
        return E_INVALIDARG;

    *pbstrName = SysAllocString(This->name);
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetSize(IRecordInfo *iface, ULONG *pcbSize)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);

    TRACE("(%p)->(%p)\n", This, pcbSize);

    if(!pcbSize)
        return E_INVALIDARG;
    
    *pcbSize = This->size;
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetTypeInfo(IRecordInfo *iface, ITypeInfo **ppTypeInfo)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);

    TRACE("(%p)->(%p)\n", This, ppTypeInfo);

    if(!ppTypeInfo)
        return E_INVALIDARG;

    ITypeInfo_AddRef(This->pTypeInfo);
    *ppTypeInfo = This->pTypeInfo;

    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_GetField(IRecordInfo *iface, PVOID pvData,
                                                LPCOLESTR szFieldName, VARIANT *pvarField)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    int i;

    TRACE("(%p)->(%p %s %p)\n", This, pvData, debugstr_w(szFieldName), pvarField);

    if(!pvData || !szFieldName || !pvarField)
        return E_INVALIDARG;

    for(i=0; i<This->n_vars; i++)
        if(!wcscmp(This->fields[i].name, szFieldName))
            break;
    if(i == This->n_vars)
        return TYPE_E_FIELDNOTFOUND;
    
    VariantClear(pvarField);
    return copy_to_variant(((PBYTE)pvData)+This->fields[i].offset, pvarField,
            This->fields[i].vt);
}

static HRESULT WINAPI IRecordInfoImpl_GetFieldNoCopy(IRecordInfo *iface, PVOID pvData,
                            LPCOLESTR szFieldName, VARIANT *pvarField, PVOID *ppvDataCArray)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    int i;

    TRACE("(%p)->(%p %s %p %p)\n", This, pvData, debugstr_w(szFieldName), pvarField, ppvDataCArray);

    if(!pvData || !szFieldName || !pvarField)
        return E_INVALIDARG;

    for(i=0; i<This->n_vars; i++)
        if(!wcscmp(This->fields[i].name, szFieldName))
            break;
    if(i == This->n_vars)
        return TYPE_E_FIELDNOTFOUND;

    VariantClear(pvarField);
    V_VT(pvarField) = VT_BYREF|This->fields[i].vt;
    V_BYREF(pvarField) = ((PBYTE)pvData)+This->fields[i].offset;
    *ppvDataCArray = NULL;
    return S_OK;
}

static HRESULT WINAPI IRecordInfoImpl_PutField(IRecordInfo *iface, ULONG wFlags, PVOID pvData,
                                            LPCOLESTR szFieldName, VARIANT *pvarField)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    int i;

    TRACE("%p, %#lx, %p, %s, %p.\n", iface, wFlags, pvData, debugstr_w(szFieldName), pvarField);

    if(!pvData || !szFieldName || !pvarField
            || (wFlags != INVOKE_PROPERTYPUTREF && wFlags != INVOKE_PROPERTYPUT))
        return E_INVALIDARG;

    if(wFlags == INVOKE_PROPERTYPUTREF) {
        FIXME("wFlag == INVOKE_PROPERTYPUTREF not supported\n");
        return E_NOTIMPL;
    }

    for(i=0; i<This->n_vars; i++)
        if(!wcscmp(This->fields[i].name, szFieldName))
            break;
    if(i == This->n_vars)
        return TYPE_E_FIELDNOTFOUND;

    return copy_from_variant(pvarField, ((PBYTE)pvData)+This->fields[i].offset,
            This->fields[i].vt);
}

static HRESULT WINAPI IRecordInfoImpl_PutFieldNoCopy(IRecordInfo *iface, ULONG wFlags,
                PVOID pvData, LPCOLESTR szFieldName, VARIANT *pvarField)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    int i;

    FIXME("%p, %#lx, %p, %s, %p stub\n", iface, wFlags, pvData, debugstr_w(szFieldName), pvarField);

    if(!pvData || !szFieldName || !pvarField
            || (wFlags != INVOKE_PROPERTYPUTREF && wFlags != INVOKE_PROPERTYPUT))
        return E_INVALIDARG;

    for(i=0; i<This->n_vars; i++)
        if(!wcscmp(This->fields[i].name, szFieldName))
            break;
    if(i == This->n_vars)
        return TYPE_E_FIELDNOTFOUND;

    return E_NOTIMPL;
}

static HRESULT WINAPI IRecordInfoImpl_GetFieldNames(IRecordInfo *iface, ULONG *pcNames,
                                                BSTR *rgBstrNames)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    ULONG n = This->n_vars, i;

    TRACE("(%p)->(%p %p)\n", This, pcNames, rgBstrNames);

    if(!pcNames)
        return E_INVALIDARG;

    if(*pcNames < n)
        n =  *pcNames;

    if(rgBstrNames) {
        for(i=0; i<n; i++)
            rgBstrNames[i] = SysAllocString(This->fields[i].name);
    }
    
    *pcNames = n;
    return S_OK;
}

static BOOL WINAPI IRecordInfoImpl_IsMatchingType(IRecordInfo *iface, IRecordInfo *info2)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    GUID guid2;

    TRACE( "(%p)->(%p)\n", This, info2 );

    IRecordInfo_GetGuid( info2, &guid2 );
    return IsEqualGUID( &This->guid, &guid2 );
}

static PVOID WINAPI IRecordInfoImpl_RecordCreate(IRecordInfo *iface)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    void *record;

    TRACE("(%p)\n", This);

    record = CoTaskMemAlloc(This->size);
    IRecordInfo_RecordInit(iface, record);
    TRACE("created record at %p\n", record);
    return record;
}

static HRESULT WINAPI IRecordInfoImpl_RecordCreateCopy(IRecordInfo *iface, PVOID pvSource,
                                                    PVOID *ppvDest)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);

    TRACE("(%p)->(%p %p)\n", This, pvSource, ppvDest);

    if(!pvSource || !ppvDest)
        return E_INVALIDARG;
    
    *ppvDest = IRecordInfo_RecordCreate(iface);
    return IRecordInfo_RecordCopy(iface, pvSource, *ppvDest);
}

static HRESULT WINAPI IRecordInfoImpl_RecordDestroy(IRecordInfo *iface, PVOID pvRecord)
{
    IRecordInfoImpl *This = impl_from_IRecordInfo(iface);
    HRESULT hres;

    TRACE("(%p)->(%p)\n", This, pvRecord);

    hres = IRecordInfo_RecordClear(iface, pvRecord);
    if(FAILED(hres))
        return hres;

    CoTaskMemFree(pvRecord);
    return S_OK;
}

static const IRecordInfoVtbl IRecordInfoImplVtbl = {
    IRecordInfoImpl_QueryInterface,
    IRecordInfoImpl_AddRef,
    IRecordInfoImpl_Release,
    IRecordInfoImpl_RecordInit,
    IRecordInfoImpl_RecordClear,
    IRecordInfoImpl_RecordCopy,
    IRecordInfoImpl_GetGuid,
    IRecordInfoImpl_GetName,
    IRecordInfoImpl_GetSize,
    IRecordInfoImpl_GetTypeInfo,
    IRecordInfoImpl_GetField,
    IRecordInfoImpl_GetFieldNoCopy,
    IRecordInfoImpl_PutField,
    IRecordInfoImpl_PutFieldNoCopy,
    IRecordInfoImpl_GetFieldNames,
    IRecordInfoImpl_IsMatchingType,
    IRecordInfoImpl_RecordCreate,
    IRecordInfoImpl_RecordCreateCopy,
    IRecordInfoImpl_RecordDestroy
};

/******************************************************************************
 *      GetRecordInfoFromGuids  [OLEAUT32.322]
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: E_INVALIDARG, if any argument is invalid.
 */
HRESULT WINAPI GetRecordInfoFromGuids(REFGUID rGuidTypeLib, ULONG uVerMajor,
                        ULONG uVerMinor, LCID lcid, REFGUID rGuidTypeInfo, IRecordInfo** ppRecInfo)
{
    ITypeInfo *pTypeInfo;
    ITypeLib *pTypeLib;
    HRESULT hres;

    TRACE("%s, %lu, %lu, %#lx, %s, %p.\n", debugstr_guid(rGuidTypeLib), uVerMajor, uVerMinor,
            lcid, debugstr_guid(rGuidTypeInfo), ppRecInfo);

    hres = LoadRegTypeLib(rGuidTypeLib, uVerMajor, uVerMinor, lcid, &pTypeLib);
    if(FAILED(hres)) {
        WARN("LoadRegTypeLib failed!\n");
        return hres;
    }

    hres = ITypeLib_GetTypeInfoOfGuid(pTypeLib, rGuidTypeInfo, &pTypeInfo);
    ITypeLib_Release(pTypeLib);
    if(FAILED(hres)) {
        WARN("GetTypeInfoOfGuid failed!\n");
        return hres;
    }

    hres = GetRecordInfoFromTypeInfo(pTypeInfo, ppRecInfo);
    ITypeInfo_Release(pTypeInfo);
    return hres;
}

/******************************************************************************
 *      GetRecordInfoFromTypeInfo [OLEAUT32.332]
 */
HRESULT WINAPI GetRecordInfoFromTypeInfo(ITypeInfo* pTI, IRecordInfo** ppRecInfo) {
    HRESULT hres;
    TYPEATTR *typeattr;
    IRecordInfoImpl *ret;
    ITypeInfo *pTypeInfo;
    int i;
    GUID guid;

    TRACE("(%p %p)\n", pTI, ppRecInfo);

    if(!pTI || !ppRecInfo)
        return E_INVALIDARG;
    
    hres = ITypeInfo_GetTypeAttr(pTI, &typeattr);
    if(FAILED(hres) || !typeattr) {
        WARN("GetTypeAttr failed: %#lx.\n", hres);
        return hres;
    }

    if(typeattr->typekind == TKIND_ALIAS) {
        hres = ITypeInfo_GetRefTypeInfo(pTI, typeattr->tdescAlias.hreftype, &pTypeInfo);
        guid = typeattr->guid;
        ITypeInfo_ReleaseTypeAttr(pTI, typeattr);
        if(FAILED(hres)) {
            WARN("GetRefTypeInfo failed: %#lx.\n", hres);
            return hres;
        }
        hres = ITypeInfo_GetTypeAttr(pTypeInfo, &typeattr);
        if(FAILED(hres)) {
            ITypeInfo_Release(pTypeInfo);
            WARN("GetTypeAttr failed for referenced type: %#lx.\n", hres);
            return hres;
        }
    }else  {
        pTypeInfo = pTI;
        ITypeInfo_AddRef(pTypeInfo);
        guid = typeattr->guid;
    }

    if(typeattr->typekind != TKIND_RECORD) {
        WARN("typekind != TKIND_RECORD\n");
        ITypeInfo_ReleaseTypeAttr(pTypeInfo, typeattr);
        ITypeInfo_Release(pTypeInfo);
        return E_INVALIDARG;
    }

    ret = calloc(1, sizeof(*ret));
    ret->IRecordInfo_iface.lpVtbl = &IRecordInfoImplVtbl;
    ret->ref = 1;
    ret->pTypeInfo = pTypeInfo;
    ret->n_vars = typeattr->cVars;
    ret->size = typeattr->cbSizeInstance;
    ITypeInfo_ReleaseTypeAttr(pTypeInfo, typeattr);

    ret->guid = guid;

    /* NOTE: Windows implementation calls ITypeInfo::GetCantainingTypeLib and
     *       ITypeLib::GetLibAttr, but we currently don't need this.
     */

    hres = ITypeInfo_GetDocumentation(pTypeInfo, MEMBERID_NIL, &ret->name, NULL, NULL, NULL);
    if(FAILED(hres)) {
        WARN("ITypeInfo::GetDocumentation failed\n");
        ret->name = NULL;
    }

    ret->fields = calloc(ret->n_vars, sizeof(fieldstr));
    for(i = 0; i<ret->n_vars; i++) {
        VARDESC *vardesc;
        hres = ITypeInfo_GetVarDesc(pTypeInfo, i, &vardesc);
        if(FAILED(hres)) {
            WARN("GetVarDesc failed\n");
            continue;
        }
        ret->fields[i].vt = vardesc->elemdescVar.tdesc.vt;
        ret->fields[i].varkind = vardesc->varkind;
        ret->fields[i].offset = vardesc->oInst;
        hres = ITypeInfo_GetDocumentation(pTypeInfo, vardesc->memid, &ret->fields[i].name,
                NULL, NULL, NULL);
        if(FAILED(hres))
            WARN("GetDocumentation failed: %#lx.\n", hres);
        TRACE("field=%s, offset=%ld\n", debugstr_w(ret->fields[i].name), ret->fields[i].offset);
        ITypeInfo_ReleaseVarDesc(pTypeInfo, vardesc);
    }

    *ppRecInfo = &ret->IRecordInfo_iface;

    return S_OK;
}
