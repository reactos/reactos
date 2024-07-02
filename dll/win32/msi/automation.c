/*
 * Implementation of OLE Automation for Microsoft Installer (msi.dll)
 *
 * Copyright 2007 Misha Koshelev
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

#include <stdarg.h>
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "winreg.h"
#include "msidefs.h"
#include "msipriv.h"
#include "activscp.h"
#include "oleauto.h"
#include "shlwapi.h"
#include "wine/debug.h"

#include "msiserver.h"
#include "msiserver_dispids.h"

WINE_DEFAULT_DEBUG_CHANNEL(msi);

#define REG_INDEX_CLASSES_ROOT 0
#define REG_INDEX_DYN_DATA 6

struct automation_object;

struct tid_id
{
    REFIID riid;
    /* function that is called from AutomationObject::Invoke, specific to this type of object */
    HRESULT (*fn_invoke)(struct automation_object *, DISPID, REFIID, LCID, WORD, DISPPARAMS *, VARIANT *,
                         EXCEPINFO *, UINT *);
    /* function that is called from AutomationObject::Release when the object is being freed
       to free any private data structures (or NULL) */
    void (*fn_free)(struct automation_object *);
};

static HRESULT database_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
static HRESULT installer_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
static HRESULT record_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
static HRESULT session_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
static HRESULT list_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
static void    list_free(struct automation_object*);
static HRESULT summaryinfo_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);
static HRESULT view_invoke(struct automation_object*,DISPID,REFIID,LCID,WORD,DISPPARAMS*,VARIANT*,EXCEPINFO*,UINT*);

static struct tid_id tid_ids[] =
{
    { &DIID_Database,    database_invoke },
    { &DIID_Installer,   installer_invoke },
    { &DIID_Record,      record_invoke },
    { &DIID_Session,     session_invoke },
    { &DIID_StringList,  list_invoke, list_free },
    { &DIID_SummaryInfo, summaryinfo_invoke },
    { &DIID_View,        view_invoke }
};

static ITypeLib  *typelib;
static ITypeInfo *typeinfos[LAST_tid];

static const IID *get_riid_from_tid(tid_t tid)
{
    return tid_ids[tid].riid;
}

HRESULT get_typeinfo(tid_t tid, ITypeInfo **typeinfo)
{
    HRESULT hr;

    if (!typelib)
    {
        ITypeLib *lib;

        hr = LoadRegTypeLib(&LIBID_WindowsInstaller, 1, 0, LOCALE_NEUTRAL, &lib);
        if (FAILED(hr)) {
            hr = LoadTypeLib(L"msiserver.tlb", &lib);
            if (FAILED(hr)) {
                ERR("Could not load msiserver.tlb\n");
                return hr;
            }
        }

        if (InterlockedCompareExchangePointer((void**)&typelib, lib, NULL))
            ITypeLib_Release(lib);
    }

    if (!typeinfos[tid])
    {
        ITypeInfo *ti;

        hr = ITypeLib_GetTypeInfoOfGuid(typelib, get_riid_from_tid(tid), &ti);
        if (FAILED(hr)) {
            ERR("Could not load ITypeInfo for %s\n", debugstr_guid(get_riid_from_tid(tid)));
            return hr;
        }

        if(InterlockedCompareExchangePointer((void**)(typeinfos+tid), ti, NULL))
            ITypeInfo_Release(ti);
    }

    *typeinfo = typeinfos[tid];
    return S_OK;
}

void release_typelib(void)
{
    unsigned i;

    for (i = 0; i < ARRAY_SIZE(typeinfos); i++)
        if (typeinfos[i])
            ITypeInfo_Release(typeinfos[i]);

    if (typelib)
        ITypeLib_Release(typelib);
}

/*
 * struct automation_object - "base" class for all automation objects. For each interface, we implement Invoke
 * function called from AutomationObject::Invoke.
 */
struct automation_object
{
    IDispatch IDispatch_iface;
    IProvideMultipleClassInfo IProvideMultipleClassInfo_iface;
    LONG ref;

    /* type id for this class */
    tid_t tid;

    /* The MSI handle of the current object */
    MSIHANDLE msiHandle;
};

struct list_object
{
    struct automation_object autoobj;
    int count;
    VARIANT *data;
};

static HRESULT create_database(MSIHANDLE, IDispatch**);
static HRESULT create_list_enumerator(struct list_object *, void **);
static HRESULT create_summaryinfo(MSIHANDLE, IDispatch**);
static HRESULT create_view(MSIHANDLE, IDispatch**);

/* struct list_enumerator - IEnumVARIANT implementation for MSI automation lists */
struct list_enumerator
{
    IEnumVARIANT IEnumVARIANT_iface;
    LONG ref;

    /* Current position and pointer to struct automation_object that stores actual data */
    ULONG pos;
    struct list_object *list;
};

struct session_object
{
    struct automation_object autoobj;
    IDispatch *installer;
};

static inline struct automation_object *impl_from_IProvideMultipleClassInfo( IProvideMultipleClassInfo *iface )
{
    return CONTAINING_RECORD(iface, struct automation_object, IProvideMultipleClassInfo_iface);
}

static inline struct automation_object *impl_from_IDispatch( IDispatch *iface )
{
    return CONTAINING_RECORD(iface, struct automation_object, IDispatch_iface);
}

/* AutomationObject methods */
static HRESULT WINAPI AutomationObject_QueryInterface(IDispatch* iface, REFIID riid, void** ppvObject)
{
    struct automation_object *This = impl_from_IDispatch(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (ppvObject == NULL)
      return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualGUID(riid, &IID_IUnknown)  ||
        IsEqualGUID(riid, &IID_IDispatch) ||
        IsEqualGUID(riid, get_riid_from_tid(This->tid)))
        *ppvObject = &This->IDispatch_iface;
    else if (IsEqualGUID(riid, &IID_IProvideClassInfo) ||
             IsEqualGUID(riid, &IID_IProvideClassInfo2) ||
             IsEqualGUID(riid, &IID_IProvideMultipleClassInfo))
        *ppvObject = &This->IProvideMultipleClassInfo_iface;
    else
    {
        TRACE("() : asking for unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IDispatch_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI AutomationObject_AddRef(IDispatch* iface)
{
    struct automation_object *This = impl_from_IDispatch(iface);

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI AutomationObject_Release(IDispatch* iface)
{
    struct automation_object *This = impl_from_IDispatch(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
    {
        if (tid_ids[This->tid].fn_free) tid_ids[This->tid].fn_free(This);
        MsiCloseHandle(This->msiHandle);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI AutomationObject_GetTypeInfoCount(
        IDispatch* iface,
        UINT* pctinfo)
{
    struct automation_object *This = impl_from_IDispatch(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pctinfo);
    *pctinfo = 1;
    return S_OK;
}

static HRESULT WINAPI AutomationObject_GetTypeInfo(
        IDispatch* iface,
        UINT iTInfo,
        LCID lcid,
        ITypeInfo** ppTInfo)
{
    struct automation_object *This = impl_from_IDispatch(iface);
    HRESULT hr;

    TRACE( "(%p/%p)->(%u, %ld, %p)\n", iface, This, iTInfo, lcid, ppTInfo );

    hr = get_typeinfo(This->tid, ppTInfo);
    if (FAILED(hr))
        return hr;

    ITypeInfo_AddRef(*ppTInfo);
    return hr;
}

static HRESULT WINAPI AutomationObject_GetIDsOfNames(
        IDispatch* iface,
        REFIID riid,
        LPOLESTR* rgszNames,
        UINT cNames,
        LCID lcid,
        DISPID* rgDispId)
{
    struct automation_object *This = impl_from_IDispatch(iface);
    ITypeInfo *ti;
    HRESULT hr;

    TRACE("(%p/%p)->(%s, %p, %u, %ld, %p)\n", iface, This,
            debugstr_guid(riid), rgszNames, cNames, lcid, rgDispId);

    if (!IsEqualGUID(riid, &IID_NULL)) return E_INVALIDARG;

    hr = get_typeinfo(This->tid, &ti);
    if (FAILED(hr))
        return hr;

    hr = ITypeInfo_GetIDsOfNames(ti, rgszNames, cNames, rgDispId);
    if (hr == DISP_E_UNKNOWNNAME)
    {
        UINT idx;
        for (idx=0; idx<cNames; idx++)
        {
            if (rgDispId[idx] == DISPID_UNKNOWN)
                FIXME("Unknown member %s, clsid %s\n", debugstr_w(rgszNames[idx]), debugstr_guid(get_riid_from_tid(This->tid)));
        }
    }
    return hr;
}

/* Maximum number of allowed function parameters+1 */
#define MAX_FUNC_PARAMS 20

/* Some error checking is done here to simplify individual object function invocation */
static HRESULT WINAPI AutomationObject_Invoke(
        IDispatch* iface,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    struct automation_object *This = impl_from_IDispatch(iface);
    HRESULT hr;
    unsigned int uArgErr;
    VARIANT varResultDummy;
    BSTR bstrName = NULL;
    ITypeInfo *ti;

    TRACE("(%p/%p)->(%ld, %s, %ld, %d, %p, %p, %p, %p)\n", iface, This,
            dispIdMember, debugstr_guid(riid), lcid, wFlags,
            pDispParams, pVarResult, pExcepInfo, puArgErr);

    if (!IsEqualIID(riid, &IID_NULL))
    {
        ERR("riid was %s instead of IID_NULL\n", debugstr_guid(riid));
        return DISP_E_UNKNOWNNAME;
    }

    if (wFlags & DISPATCH_PROPERTYGET && !pVarResult)
    {
        ERR("NULL pVarResult not allowed when DISPATCH_PROPERTYGET specified\n");
        return DISP_E_PARAMNOTOPTIONAL;
    }

    /* This simplifies our individual object invocation functions */
    if (puArgErr == NULL) puArgErr = &uArgErr;
    if (pVarResult == NULL) pVarResult = &varResultDummy;

    hr = get_typeinfo(This->tid, &ti);
    if (FAILED(hr))
        return hr;

    /* Assume return type is void unless determined otherwise */
    VariantInit(pVarResult);

    /* If we are tracing, we want to see the name of the member we are invoking */
    if (TRACE_ON(msi))
    {
        ITypeInfo_GetDocumentation(ti, dispIdMember, &bstrName, NULL, NULL, NULL);
        TRACE("method %ld, %s\n", dispIdMember, debugstr_w(bstrName));
    }

    hr = tid_ids[This->tid].fn_invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr);

    if (hr == DISP_E_MEMBERNOTFOUND) {
        if (bstrName == NULL) ITypeInfo_GetDocumentation(ti, dispIdMember, &bstrName, NULL, NULL, NULL);
        FIXME("method %ld, %s wflags %d not implemented, clsid %s\n", dispIdMember, debugstr_w(bstrName), wFlags,
            debugstr_guid(get_riid_from_tid(This->tid)));
    }
    else if (pExcepInfo &&
             (hr == DISP_E_PARAMNOTFOUND ||
              hr == DISP_E_EXCEPTION)) {
        WCHAR szExceptionDescription[MAX_PATH];
        BSTR bstrParamNames[MAX_FUNC_PARAMS];
        unsigned namesNo, i;
        BOOL bFirst = TRUE;

        if (FAILED(ITypeInfo_GetNames(ti, dispIdMember, bstrParamNames,
                                      MAX_FUNC_PARAMS, &namesNo)))
        {
            TRACE("failed to retrieve names for dispIdMember %ld\n", dispIdMember);
        }
        else
        {
            memset(szExceptionDescription, 0, sizeof(szExceptionDescription));
            for (i=0; i<namesNo; i++)
            {
                if (bFirst) bFirst = FALSE;
                else {
                    lstrcpyW(&szExceptionDescription[lstrlenW(szExceptionDescription)], L",");
                }
                lstrcpyW(&szExceptionDescription[lstrlenW(szExceptionDescription)], bstrParamNames[i]);
                SysFreeString(bstrParamNames[i]);
            }

            memset(pExcepInfo, 0, sizeof(EXCEPINFO));
            pExcepInfo->wCode = 1000;
            pExcepInfo->bstrSource = SysAllocString(L"Msi API Error");
            pExcepInfo->bstrDescription = SysAllocString(szExceptionDescription);
            hr = DISP_E_EXCEPTION;
        }
    }

    /* Make sure we free the return variant if it is our dummy variant */
    if (pVarResult == &varResultDummy) VariantClear(pVarResult);

    /* Free function name if we retrieved it */
    SysFreeString(bstrName);

    TRACE("returning %#lx, %s\n", hr, SUCCEEDED(hr) ? "ok" : "not ok");

    return hr;
}

static const struct IDispatchVtbl AutomationObjectVtbl =
{
    AutomationObject_QueryInterface,
    AutomationObject_AddRef,
    AutomationObject_Release,
    AutomationObject_GetTypeInfoCount,
    AutomationObject_GetTypeInfo,
    AutomationObject_GetIDsOfNames,
    AutomationObject_Invoke
};

/*
 * IProvideMultipleClassInfo methods
 */

static HRESULT WINAPI ProvideMultipleClassInfo_QueryInterface(
  IProvideMultipleClassInfo* iface,
  REFIID     riid,
  VOID**     ppvoid)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);
    return IDispatch_QueryInterface(&This->IDispatch_iface, riid, ppvoid);
}

static ULONG WINAPI ProvideMultipleClassInfo_AddRef(IProvideMultipleClassInfo* iface)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);
    return IDispatch_AddRef(&This->IDispatch_iface);
}

static ULONG WINAPI ProvideMultipleClassInfo_Release(IProvideMultipleClassInfo* iface)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);
    return IDispatch_Release(&This->IDispatch_iface);
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetClassInfo(IProvideMultipleClassInfo* iface, ITypeInfo** ppTI)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);
    HRESULT hr;

    TRACE("(%p/%p)->(%p)\n", iface, This, ppTI);

    hr = get_typeinfo(This->tid, ppTI);
    if (SUCCEEDED(hr))
        ITypeInfo_AddRef(*ppTI);

    return hr;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetGUID(IProvideMultipleClassInfo* iface, DWORD dwGuidKind, GUID* pGUID)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);
    TRACE("(%p/%p)->(%lu, %s)\n", iface, This, dwGuidKind, debugstr_guid(pGUID));

    if (dwGuidKind != GUIDKIND_DEFAULT_SOURCE_DISP_IID)
        return E_INVALIDARG;
    else {
        *pGUID = *get_riid_from_tid(This->tid);
        return S_OK;
    }
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetMultiTypeInfoCount(IProvideMultipleClassInfo* iface, ULONG* pcti)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);

    TRACE("(%p/%p)->(%p)\n", iface, This, pcti);
    *pcti = 1;
    return S_OK;
}

static HRESULT WINAPI ProvideMultipleClassInfo_GetInfoOfIndex(IProvideMultipleClassInfo* iface,
        ULONG iti,
        DWORD dwFlags,
        ITypeInfo** ti,
        DWORD* pdwTIFlags,
        ULONG* pcdispidReserved,
        IID* piidPrimary,
        IID* piidSource)
{
    struct automation_object *This = impl_from_IProvideMultipleClassInfo(iface);

    TRACE("(%p/%p)->(%lu, %#lx, %p, %p, %p, %p, %p)\n", iface, This, iti, dwFlags, ti, pdwTIFlags, pcdispidReserved,
          piidPrimary, piidSource);

    if (iti != 0)
        return E_INVALIDARG;

    if (dwFlags & MULTICLASSINFO_GETTYPEINFO)
    {
        HRESULT hr = get_typeinfo(This->tid, ti);
        if (FAILED(hr))
            return hr;

        ITypeInfo_AddRef(*ti);
    }

    if (dwFlags & MULTICLASSINFO_GETNUMRESERVEDDISPIDS)
    {
        *pdwTIFlags = 0;
        *pcdispidReserved = 0;
    }

    if (dwFlags & MULTICLASSINFO_GETIIDPRIMARY)
        *piidPrimary = *get_riid_from_tid(This->tid);

    if (dwFlags & MULTICLASSINFO_GETIIDSOURCE)
        *piidSource = *get_riid_from_tid(This->tid);

    return S_OK;
}

static const IProvideMultipleClassInfoVtbl ProvideMultipleClassInfoVtbl =
{
    ProvideMultipleClassInfo_QueryInterface,
    ProvideMultipleClassInfo_AddRef,
    ProvideMultipleClassInfo_Release,
    ProvideMultipleClassInfo_GetClassInfo,
    ProvideMultipleClassInfo_GetGUID,
    ProvideMultipleClassInfo_GetMultiTypeInfoCount,
    ProvideMultipleClassInfo_GetInfoOfIndex
};

static void init_automation_object(struct automation_object *This, MSIHANDLE msiHandle, tid_t tid)
{
    TRACE("%p, %lu, %s\n", This, msiHandle, debugstr_guid(get_riid_from_tid(tid)));

    This->IDispatch_iface.lpVtbl = &AutomationObjectVtbl;
    This->IProvideMultipleClassInfo_iface.lpVtbl = &ProvideMultipleClassInfoVtbl;
    This->ref = 1;
    This->msiHandle = msiHandle;
    This->tid = tid;
}

/*
 * ListEnumerator methods
 */

static inline struct list_enumerator *impl_from_IEnumVARIANT(IEnumVARIANT* iface)
{
    return CONTAINING_RECORD(iface, struct list_enumerator, IEnumVARIANT_iface);
}

static HRESULT WINAPI ListEnumerator_QueryInterface(IEnumVARIANT* iface, REFIID riid,
        void** ppvObject)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p/%p)->(%s,%p)\n", iface, This, debugstr_guid(riid), ppvObject);

    if (ppvObject == NULL)
      return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IEnumVARIANT))
    {
        *ppvObject = &This->IEnumVARIANT_iface;
    }
    else
    {
        TRACE("() : asking for unsupported interface %s\n",debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IEnumVARIANT_AddRef(iface);
    return S_OK;
}

static ULONG WINAPI ListEnumerator_AddRef(IEnumVARIANT* iface)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p/%p)\n", iface, This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ListEnumerator_Release(IEnumVARIANT* iface)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p/%p)\n", iface, This);

    if (!ref)
    {
        if (This->list) IDispatch_Release(&This->list->autoobj.IDispatch_iface);
        free(This);
    }

    return ref;
}

static HRESULT WINAPI ListEnumerator_Next(IEnumVARIANT* iface, ULONG celt, VARIANT* rgVar,
        ULONG* fetched)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);
    ULONG i, local;

    TRACE("%p, %lu, %p, %p\n", iface, celt, rgVar, fetched);

    if (fetched) *fetched = 0;

    if (!rgVar)
        return S_FALSE;

    for (local = 0; local < celt; local++)
        VariantInit(&rgVar[local]);

    for (i = This->pos, local = 0; i < This->list->count && local < celt; i++, local++)
        VariantCopy(&rgVar[local], &This->list->data[i]);

    if (fetched) *fetched = local;
    This->pos = i;

    return (local < celt) ? S_FALSE : S_OK;
}

static HRESULT WINAPI ListEnumerator_Skip(IEnumVARIANT* iface, ULONG celt)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);

    TRACE("%p, %lu\n", iface, celt);

    This->pos += celt;
    if (This->pos >= This->list->count)
    {
        This->pos = This->list->count;
        return S_FALSE;
    }

    return S_OK;
}

static HRESULT WINAPI ListEnumerator_Reset(IEnumVARIANT* iface)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);

    TRACE("(%p)\n", iface);

    This->pos = 0;
    return S_OK;
}

static HRESULT WINAPI ListEnumerator_Clone(IEnumVARIANT* iface, IEnumVARIANT **ppEnum)
{
    struct list_enumerator *This = impl_from_IEnumVARIANT(iface);
    HRESULT hr;

    TRACE("(%p,%p)\n", iface, ppEnum);

    if (ppEnum == NULL)
        return S_FALSE;

    *ppEnum = NULL;
    hr = create_list_enumerator(This->list, (LPVOID *)ppEnum);
    if (FAILED(hr))
    {
        if (*ppEnum) IEnumVARIANT_Release(*ppEnum);
        return hr;
    }

    return S_OK;
}

static const struct IEnumVARIANTVtbl ListEnumerator_Vtbl =
{
    ListEnumerator_QueryInterface,
    ListEnumerator_AddRef,
    ListEnumerator_Release,
    ListEnumerator_Next,
    ListEnumerator_Skip,
    ListEnumerator_Reset,
    ListEnumerator_Clone
};

/* Create a list enumerator, placing the result in the pointer ppObj.  */
static HRESULT create_list_enumerator(struct list_object *list, void **ppObj)
{
    struct list_enumerator *object;

    TRACE("(%p, %p)\n", list, ppObj);

    object = malloc(sizeof(*object));

    /* Set all the VTable references */
    object->IEnumVARIANT_iface.lpVtbl = &ListEnumerator_Vtbl;
    object->ref = 1;

    /* Store data that was passed */
    object->pos = 0;
    object->list = list;
    if (list) IDispatch_AddRef(&list->autoobj.IDispatch_iface);

    *ppObj = object;
    return S_OK;
}

/*
 * Individual Object Invocation Functions
 */

/* Helper function that copies a passed parameter instead of using VariantChangeType like the actual DispGetParam.
   This function is only for VARIANT type parameters that have several types that cannot be properly discriminated
   using DispGetParam/VariantChangeType. */
static HRESULT DispGetParam_CopyOnly(
        DISPPARAMS *pdispparams, /* [in] Parameter list */
        UINT        *position,    /* [in] Position of parameter to copy in pdispparams; on return will contain calculated position */
        VARIANT    *pvarResult)  /* [out] Destination for resulting variant */
{
    /* position is counted backwards */
    UINT pos;

    TRACE("position=%d, cArgs=%d, cNamedArgs=%d\n",
          *position, pdispparams->cArgs, pdispparams->cNamedArgs);
    if (*position < pdispparams->cArgs) {
      /* positional arg? */
      pos = pdispparams->cArgs - *position - 1;
    } else {
      /* FIXME: is this how to handle named args? */
      for (pos=0; pos<pdispparams->cNamedArgs; pos++)
        if (pdispparams->rgdispidNamedArgs[pos] == *position) break;

      if (pos==pdispparams->cNamedArgs)
        return DISP_E_PARAMNOTFOUND;
    }
    *position = pos;
    return VariantCopyInd(pvarResult,
                        &pdispparams->rgvarg[pos]);
}

static HRESULT summaryinfo_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    UINT ret;
    VARIANTARG varg0, varg1;
    FILETIME ft, ftlocal;
    SYSTEMTIME st;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
        case DISPID_SUMMARYINFO_PROPERTY:
            if (wFlags & DISPATCH_PROPERTYGET)
            {
                UINT type;
                INT value;
                DWORD size = 0;
                DATE date;
                LPWSTR str;

                static WCHAR szEmpty[] = L"";

                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                ret = MsiSummaryInfoGetPropertyW(This->msiHandle, V_I4(&varg0), &type, &value,
                                                 &ft, szEmpty, &size);
                if (ret != ERROR_SUCCESS &&
                    ret != ERROR_MORE_DATA)
                {
                    ERR("MsiSummaryInfoGetProperty returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }

                switch (type)
                {
                    case VT_EMPTY:
                        break;

                    case VT_I2:
                    case VT_I4:
                        V_VT(pVarResult) = VT_I4;
                        V_I4(pVarResult) = value;
                        break;

                    case VT_LPSTR:
                        if (!(str = malloc(++size * sizeof(WCHAR))))
                            ERR("Out of memory\n");
                        else if ((ret = MsiSummaryInfoGetPropertyW(This->msiHandle, V_I4(&varg0), &type, NULL,
                                                                   NULL, str, &size)) != ERROR_SUCCESS)
                            ERR("MsiSummaryInfoGetProperty returned %d\n", ret);
                        else
                        {
                            V_VT(pVarResult) = VT_BSTR;
                            V_BSTR(pVarResult) = SysAllocString(str);
                        }
                        free(str);
                        break;

                    case VT_FILETIME:
                        FileTimeToLocalFileTime(&ft, &ftlocal);
                        FileTimeToSystemTime(&ftlocal, &st);
                        SystemTimeToVariantTime(&st, &date);

                        V_VT(pVarResult) = VT_DATE;
                        V_DATE(pVarResult) = date;
                        break;

                    default:
                        ERR("Unhandled variant type %d\n", type);
                }
            }
            else if (wFlags & DISPATCH_PROPERTYPUT)
            {
                UINT posValue = DISPID_PROPERTYPUT;

                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam_CopyOnly(pDispParams, &posValue, &varg1);
                if (FAILED(hr))
                {
                    *puArgErr = posValue;
                    return hr;
                }

                switch (V_VT(&varg1))
                {
                    case VT_I2:
                    case VT_I4:
                        ret = MsiSummaryInfoSetPropertyW(This->msiHandle, V_I4(&varg0), V_VT(&varg1), V_I4(&varg1), NULL, NULL);
                        break;

                    case VT_DATE:
                        VariantTimeToSystemTime(V_DATE(&varg1), &st);
                        SystemTimeToFileTime(&st, &ftlocal);
                        LocalFileTimeToFileTime(&ftlocal, &ft);
                        ret = MsiSummaryInfoSetPropertyW(This->msiHandle, V_I4(&varg0), VT_FILETIME, 0, &ft, NULL);
                        break;

                    case VT_BSTR:
                        ret = MsiSummaryInfoSetPropertyW(This->msiHandle, V_I4(&varg0), VT_LPSTR, 0, NULL, V_BSTR(&varg1));
                        break;

                    default:
                        FIXME("Unhandled variant type %d\n", V_VT(&varg1));
                        VariantClear(&varg1);
                        return DISP_E_EXCEPTION;
                }

                if (ret != ERROR_SUCCESS)
                {
                    ERR("MsiSummaryInfoSetPropertyW returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SUMMARYINFO_PROPERTYCOUNT:
            if (wFlags & DISPATCH_PROPERTYGET) {
                UINT count;
                if ((ret = MsiSummaryInfoGetPropertyCount(This->msiHandle, &count)) != ERROR_SUCCESS)
                    ERR("MsiSummaryInfoGetPropertyCount returned %d\n", ret);
                else
                {
                    V_VT(pVarResult) = VT_I4;
                    V_I4(pVarResult) = count;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        default:
            return DISP_E_MEMBERNOTFOUND;
    }

    VariantClear(&varg1);
    VariantClear(&varg0);

    return S_OK;
}

static HRESULT record_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    WCHAR *szString;
    DWORD dwLen = 0;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
        case DISPID_RECORD_FIELDCOUNT:
            if (wFlags & DISPATCH_PROPERTYGET) {
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = MsiRecordGetFieldCount(This->msiHandle);
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_RECORD_STRINGDATA:
            if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_BSTR;
                V_BSTR(pVarResult) = NULL;
                if ((ret = MsiRecordGetStringW(This->msiHandle, V_I4(&varg0), NULL, &dwLen)) == ERROR_SUCCESS)
                {
                    if (!(szString = malloc((++dwLen) * sizeof(WCHAR))))
                        ERR("Out of memory\n");
                    else if ((ret = MsiRecordGetStringW(This->msiHandle, V_I4(&varg0), szString, &dwLen)) == ERROR_SUCCESS)
                        V_BSTR(pVarResult) = SysAllocString(szString);
                    free(szString);
                }
                if (ret != ERROR_SUCCESS)
                    ERR("MsiRecordGetString returned %d\n", ret);
            } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_BSTR, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
                if ((ret = MsiRecordSetStringW(This->msiHandle, V_I4(&varg0), V_BSTR(&varg1))) != ERROR_SUCCESS)
                {
                    VariantClear(&varg1);
                    ERR("MsiRecordSetString returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_RECORD_INTEGERDATA:
            if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = MsiRecordGetInteger(This->msiHandle, V_I4(&varg0));
            } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_I4, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
                if ((ret = MsiRecordSetInteger(This->msiHandle, V_I4(&varg0), V_I4(&varg1))) != ERROR_SUCCESS)
                {
                    ERR("MsiRecordSetInteger returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    VariantClear(&varg1);
    VariantClear(&varg0);

    return S_OK;
}

static HRESULT create_record(MSIHANDLE msiHandle, IDispatch **disp)
{
    struct automation_object *record;

    record = malloc(sizeof(*record));
    if (!record) return E_OUTOFMEMORY;

    init_automation_object(record, msiHandle, Record_tid);

    *disp = &record->IDispatch_iface;

    return S_OK;
}

static HRESULT list_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    struct list_object *list = CONTAINING_RECORD(This, struct list_object, autoobj);
    IUnknown *pUnk = NULL;
    HRESULT hr;

    switch (dispIdMember)
    {
         case DISPID_LIST__NEWENUM:
             if (wFlags & DISPATCH_METHOD) {
                 V_VT(pVarResult) = VT_UNKNOWN;
                 if (SUCCEEDED(hr = create_list_enumerator(list, (LPVOID *)&pUnk)))
                     V_UNKNOWN(pVarResult) = pUnk;
                 else
                     ERR("failed to create IEnumVARIANT object, hresult %#lx\n", hr);
             }
             else return DISP_E_MEMBERNOTFOUND;
             break;

         case DISPID_LIST_ITEM:
             if (wFlags & DISPATCH_PROPERTYGET) {
                VARIANTARG index;

                VariantInit(&index);
                hr = DispGetParam(pDispParams, 0, VT_I4, &index, puArgErr);
                if (FAILED(hr)) return hr;
                if (V_I4(&index) < 0 || V_I4(&index) >= list->count)
                    return DISP_E_BADINDEX;
                VariantCopy(pVarResult, &list->data[V_I4(&index)]);
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

         case DISPID_LIST_COUNT:
            if (wFlags & DISPATCH_PROPERTYGET) {
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = list->count;
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    return S_OK;
}

static void list_free(struct automation_object *This)
{
    struct list_object *list = CONTAINING_RECORD(This, struct list_object, autoobj);
    int i;

    for (i = 0; i < list->count; i++)
        VariantClear(&list->data[i]);
    free(list->data);
}

static HRESULT get_products_count(const WCHAR *product, int *len)
{
    int i = 0;

    while (1)
    {
        WCHAR dataW[GUID_SIZE];
        UINT ret;

        /* all or related only */
        if (product)
            ret = MsiEnumRelatedProductsW(product, 0, i, dataW);
        else
            ret = MsiEnumProductsW(i, dataW);

        if (ret == ERROR_NO_MORE_ITEMS) break;

        if (ret != ERROR_SUCCESS)
            return DISP_E_EXCEPTION;

        i++;
    }

    *len = i;

    return S_OK;
}

static HRESULT create_list(const WCHAR *product, IDispatch **dispatch)
{
    struct list_object *list;
    HRESULT hr;
    int i;

    list = calloc(1, sizeof(*list));
    if (!list) return E_OUTOFMEMORY;

    init_automation_object(&list->autoobj, 0, StringList_tid);

    *dispatch = &list->autoobj.IDispatch_iface;

    hr = get_products_count(product, &list->count);
    if (hr != S_OK)
    {
        IDispatch_Release(*dispatch);
        return hr;
    }

    list->data = malloc(list->count * sizeof(VARIANT));
    if (!list->data)
    {
        IDispatch_Release(*dispatch);
        return E_OUTOFMEMORY;
    }

    for (i = 0; i < list->count; i++)
    {
        WCHAR dataW[GUID_SIZE];
        UINT ret;

        /* all or related only */
        if (product)
            ret = MsiEnumRelatedProductsW(product, 0, i, dataW);
        else
            ret = MsiEnumProductsW(i, dataW);

        if (ret == ERROR_NO_MORE_ITEMS) break;

        V_VT(&list->data[i]) = VT_BSTR;
        V_BSTR(&list->data[i]) = SysAllocString(dataW);
    }

    return S_OK;
}

static HRESULT view_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    MSIHANDLE msiHandle;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
        case DISPID_VIEW_EXECUTE:
            if (wFlags & DISPATCH_METHOD)
            {
                hr = DispGetParam(pDispParams, 0, VT_DISPATCH, &varg0, puArgErr);
                if (SUCCEEDED(hr) && V_DISPATCH(&varg0) != NULL)
                    MsiViewExecute(This->msiHandle, ((struct automation_object *)V_DISPATCH(&varg0))->msiHandle);
                else
                    MsiViewExecute(This->msiHandle, 0);
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_VIEW_FETCH:
            if (wFlags & DISPATCH_METHOD)
            {
                V_VT(pVarResult) = VT_DISPATCH;
                if ((ret = MsiViewFetch(This->msiHandle, &msiHandle)) == ERROR_SUCCESS)
                {
                    if (FAILED(hr = create_record(msiHandle, &V_DISPATCH(pVarResult))))
                        ERR("failed to create Record object, hresult %#lx\n", hr);
                }
                else if (ret == ERROR_NO_MORE_ITEMS)
                    V_DISPATCH(pVarResult) = NULL;
                else
                {
                    ERR("MsiViewFetch returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_VIEW_MODIFY:
            if (wFlags & DISPATCH_METHOD)
            {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_DISPATCH, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
                if (!V_DISPATCH(&varg1)) return DISP_E_EXCEPTION;
                if ((ret = MsiViewModify(This->msiHandle, V_I4(&varg0),
                                         ((struct automation_object *)V_DISPATCH(&varg1))->msiHandle)) != ERROR_SUCCESS)
                {
                    VariantClear(&varg1);
                    ERR("MsiViewModify returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_VIEW_CLOSE:
            if (wFlags & DISPATCH_METHOD)
            {
                MsiViewClose(This->msiHandle);
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    VariantClear(&varg1);
    VariantClear(&varg0);

    return S_OK;
}

static HRESULT DatabaseImpl_LastErrorRecord(WORD wFlags,
                                            DISPPARAMS* pDispParams,
                                            VARIANT* pVarResult,
                                            EXCEPINFO* pExcepInfo,
                                            UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

HRESULT database_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    IDispatch *dispatch = NULL;
    MSIHANDLE msiHandle;
    UINT ret;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
        case DISPID_DATABASE_SUMMARYINFORMATION:
            if (wFlags & DISPATCH_PROPERTYGET)
            {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr))
                    V_I4(&varg0) = 0;

                V_VT(pVarResult) = VT_DISPATCH;
                if ((ret = MsiGetSummaryInformationW(This->msiHandle, NULL, V_I4(&varg0), &msiHandle)) == ERROR_SUCCESS)
                {
                    hr = create_summaryinfo(msiHandle, &dispatch);
                    if (SUCCEEDED(hr))
                        V_DISPATCH(pVarResult) = dispatch;
                    else
                        ERR("failed to create SummaryInfo object: %#lx\n", hr);
                }
                else
                {
                    ERR("MsiGetSummaryInformation returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_DATABASE_OPENVIEW:
            if (wFlags & DISPATCH_METHOD)
            {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_DISPATCH;
                if ((ret = MsiDatabaseOpenViewW(This->msiHandle, V_BSTR(&varg0), &msiHandle)) == ERROR_SUCCESS)
                {
                    if (SUCCEEDED(hr = create_view(msiHandle, &dispatch)))
                        V_DISPATCH(pVarResult) = dispatch;
                    else
                        ERR("failed to create View object, hresult %#lx\n", hr);
                }
                else
                {
                    VariantClear(&varg0);
                    ERR("MsiDatabaseOpenView returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_INSTALLER_LASTERRORRECORD:
            return DatabaseImpl_LastErrorRecord(wFlags, pDispParams,
                                                pVarResult, pExcepInfo,
                                                puArgErr);

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    VariantClear(&varg1);
    VariantClear(&varg0);

    return S_OK;
}

static HRESULT session_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    struct session_object *session = CONTAINING_RECORD(This, struct session_object, autoobj);
    WCHAR *szString;
    DWORD dwLen = 0;
    MSIHANDLE msiHandle;
    LANGID langId;
    UINT ret;
    INSTALLSTATE iInstalled, iAction;
    VARIANTARG varg0, varg1;
    HRESULT hr;

    VariantInit(&varg0);
    VariantInit(&varg1);

    switch (dispIdMember)
    {
        case DISPID_SESSION_INSTALLER:
            if (wFlags & DISPATCH_PROPERTYGET) {
                V_VT(pVarResult) = VT_DISPATCH;
                IDispatch_AddRef(session->installer);
                V_DISPATCH(pVarResult) = session->installer;
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_PROPERTY:
            if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_BSTR;
                V_BSTR(pVarResult) = NULL;
                if ((ret = MsiGetPropertyW(This->msiHandle, V_BSTR(&varg0), NULL, &dwLen)) == ERROR_SUCCESS)
                {
                    if (!(szString = malloc((++dwLen) * sizeof(WCHAR))))
                        ERR("Out of memory\n");
                    else if ((ret = MsiGetPropertyW(This->msiHandle, V_BSTR(&varg0), szString, &dwLen)) == ERROR_SUCCESS)
                        V_BSTR(pVarResult) = SysAllocString(szString);
                    free(szString);
                }
                if (ret != ERROR_SUCCESS)
                    ERR("MsiGetProperty returned %d\n", ret);
            } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_BSTR, &varg1, puArgErr);
                if (FAILED(hr)) {
                    VariantClear(&varg0);
                    return hr;
                }
                if ((ret = MsiSetPropertyW(This->msiHandle, V_BSTR(&varg0), V_BSTR(&varg1))) != ERROR_SUCCESS)
                {
                    VariantClear(&varg0);
                    VariantClear(&varg1);
                    ERR("MsiSetProperty returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_LANGUAGE:
            if (wFlags & DISPATCH_PROPERTYGET) {
                langId = MsiGetLanguage(This->msiHandle);
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = langId;
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_MODE:
            if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_BOOL;
                V_BOOL(pVarResult) = MsiGetMode(This->msiHandle, V_I4(&varg0)) ? VARIANT_TRUE : VARIANT_FALSE;
            } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_BOOL, &varg1, puArgErr);
                if (FAILED(hr)) return hr;
                if ((ret = MsiSetMode(This->msiHandle, V_I4(&varg0), V_BOOL(&varg1))) != ERROR_SUCCESS)
                {
                    ERR("MsiSetMode returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_DATABASE:
            if (wFlags & DISPATCH_PROPERTYGET) {
                V_VT(pVarResult) = VT_DISPATCH;
                if ((msiHandle = MsiGetActiveDatabase(This->msiHandle)))
                {
                    IDispatch *dispatch;

                    if (SUCCEEDED(hr = create_database(msiHandle, &dispatch)))
                        V_DISPATCH(pVarResult) = dispatch;
                    else
                        ERR("failed to create Database object, hresult %#lx\n", hr);
                }
                else
                {
                    ERR("MsiGetActiveDatabase failed\n");
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_DOACTION:
            if (wFlags & DISPATCH_METHOD) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                ret = MsiDoActionW(This->msiHandle, V_BSTR(&varg0));
                V_VT(pVarResult) = VT_I4;
                switch (ret)
                {
                    case ERROR_FUNCTION_NOT_CALLED:
                        V_I4(pVarResult) = msiDoActionStatusNoAction;
                        break;
                    case ERROR_SUCCESS:
                        V_I4(pVarResult) = msiDoActionStatusSuccess;
                        break;
                    case ERROR_INSTALL_USEREXIT:
                        V_I4(pVarResult) = msiDoActionStatusUserExit;
                        break;
                    case ERROR_INSTALL_FAILURE:
                        V_I4(pVarResult) = msiDoActionStatusFailure;
                        break;
                    case ERROR_INSTALL_SUSPEND:
                        V_I4(pVarResult) = msiDoActionStatusSuspend;
                        break;
                    case ERROR_MORE_DATA:
                        V_I4(pVarResult) = msiDoActionStatusFinished;
                        break;
                    case ERROR_INVALID_HANDLE_STATE:
                        V_I4(pVarResult) = msiDoActionStatusWrongState;
                        break;
                    case ERROR_INVALID_DATA:
                        V_I4(pVarResult) = msiDoActionStatusBadActionData;
                        break;
                    default:
                        VariantClear(&varg0);
                        FIXME("MsiDoAction returned unhandled value %d\n", ret);
                        return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_EVALUATECONDITION:
            if (wFlags & DISPATCH_METHOD) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_I4;
                V_I4(pVarResult) = MsiEvaluateConditionW(This->msiHandle, V_BSTR(&varg0));
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_MESSAGE:
            if(!(wFlags & DISPATCH_METHOD))
                return DISP_E_MEMBERNOTFOUND;

            hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
            if (FAILED(hr)) return hr;
            hr = DispGetParam(pDispParams, 1, VT_DISPATCH, &varg1, puArgErr);
            if (FAILED(hr)) return hr;

            V_VT(pVarResult) = VT_I4;
            V_I4(pVarResult) =
                MsiProcessMessage(This->msiHandle, V_I4(&varg0), ((struct automation_object *)V_DISPATCH(&varg1))->msiHandle);
            break;

        case DISPID_SESSION_SETINSTALLLEVEL:
            if (wFlags & DISPATCH_METHOD) {
                hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                if ((ret = MsiSetInstallLevel(This->msiHandle, V_I4(&varg0))) != ERROR_SUCCESS)
                {
                    ERR("MsiSetInstallLevel returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_FEATURECURRENTSTATE:
            if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_I4;
                if ((ret = MsiGetFeatureStateW(This->msiHandle, V_BSTR(&varg0), &iInstalled, &iAction)) == ERROR_SUCCESS)
                    V_I4(pVarResult) = iInstalled;
                else
                {
                    ERR("MsiGetFeatureState returned %d\n", ret);
                    V_I4(pVarResult) = msiInstallStateUnknown;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

        case DISPID_SESSION_FEATUREREQUESTSTATE:
            if (wFlags & DISPATCH_PROPERTYGET) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                V_VT(pVarResult) = VT_I4;
                if ((ret = MsiGetFeatureStateW(This->msiHandle, V_BSTR(&varg0), &iInstalled, &iAction)) == ERROR_SUCCESS)
                    V_I4(pVarResult) = iAction;
                else
                {
                    ERR("MsiGetFeatureState returned %d\n", ret);
                    V_I4(pVarResult) = msiInstallStateUnknown;
                }
            } else if (wFlags & DISPATCH_PROPERTYPUT) {
                hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
                if (FAILED(hr)) return hr;
                hr = DispGetParam(pDispParams, 1, VT_I4, &varg1, puArgErr);
                if (FAILED(hr)) {
                    VariantClear(&varg0);
                    return hr;
                }
                if ((ret = MsiSetFeatureStateW(This->msiHandle, V_BSTR(&varg0), V_I4(&varg1))) != ERROR_SUCCESS)
                {
                    VariantClear(&varg0);
                    ERR("MsiSetFeatureState returned %d\n", ret);
                    return DISP_E_EXCEPTION;
                }
            }
            else return DISP_E_MEMBERNOTFOUND;
            break;

         default:
            return DISP_E_MEMBERNOTFOUND;
    }

    VariantClear(&varg1);
    VariantClear(&varg0);

    return S_OK;
}

/* Fill the variant pointed to by pVarResult with the value & size returned by RegQueryValueEx as dictated by the
 * registry value type. Used by Installer::RegistryValue. */
static void variant_from_registry_value(VARIANT *pVarResult, DWORD dwType, LPBYTE lpData, DWORD dwSize)
{
    WCHAR *szString = (WCHAR *)lpData;
    LPWSTR szNewString = NULL;
    DWORD dwNewSize = 0;
    int idx;

    switch (dwType)
    {
        /* Registry strings may not be null terminated so we must use SysAllocStringByteLen/Len */
        case REG_MULTI_SZ: /* Multi SZ change internal null characters to newlines */
            idx = (dwSize/sizeof(WCHAR))-1;
            while (idx >= 0 && !szString[idx]) idx--;
            for (; idx >= 0; idx--)
                if (!szString[idx]) szString[idx] = '\n';
            /* fall through */
        case REG_SZ:
            V_VT(pVarResult) = VT_BSTR;
            V_BSTR(pVarResult) = SysAllocStringByteLen((LPCSTR)szString, dwSize);
            break;

        case REG_EXPAND_SZ:
            if (!(dwNewSize = ExpandEnvironmentStringsW(szString, szNewString, dwNewSize)))
                ERR("ExpandEnvironmentStrings returned error %lu\n", GetLastError());
            else if (!(szNewString = malloc(dwNewSize * sizeof(WCHAR))))
                ERR("Out of memory\n");
            else if (!(dwNewSize = ExpandEnvironmentStringsW(szString, szNewString, dwNewSize)))
                ERR("ExpandEnvironmentStrings returned error %lu\n", GetLastError());
            else
            {
                V_VT(pVarResult) = VT_BSTR;
                V_BSTR(pVarResult) = SysAllocStringLen(szNewString, dwNewSize);
            }
            free(szNewString);
            break;

        case REG_DWORD:
            V_VT(pVarResult) = VT_I4;
            V_I4(pVarResult) = *((DWORD *)lpData);
            break;

        case REG_QWORD:
            V_VT(pVarResult) = VT_BSTR;
            V_BSTR(pVarResult) = SysAllocString(L"(REG_\?\?)");   /* Weird string, don't know why native returns it */
            break;

        case REG_BINARY:
            V_VT(pVarResult) = VT_BSTR;
            V_BSTR(pVarResult) = SysAllocString(L"(REG_BINARY)");
            break;

        case REG_NONE:
            V_VT(pVarResult) = VT_EMPTY;
            break;

        default:
            FIXME("Unhandled registry value type %lu\n", dwType);
    }
}

static HRESULT InstallerImpl_CreateRecord(WORD wFlags,
                                          DISPPARAMS* pDispParams,
                                          VARIANT* pVarResult,
                                          EXCEPINFO* pExcepInfo,
                                          UINT* puArgErr)
{
    HRESULT hr;
    VARIANTARG varg0;
    MSIHANDLE hrec;

    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    V_VT(pVarResult) = VT_DISPATCH;

    hrec = MsiCreateRecord(V_I4(&varg0));
    if (!hrec)
        return DISP_E_EXCEPTION;

    return create_record(hrec, &V_DISPATCH(pVarResult));
}

static HRESULT InstallerImpl_OpenPackage(struct automation_object *This,
                                         WORD wFlags,
                                         DISPPARAMS* pDispParams,
                                         VARIANT* pVarResult,
                                         EXCEPINFO* pExcepInfo,
                                         UINT* puArgErr)
{
    UINT ret;
    HRESULT hr;
    MSIHANDLE hpkg;
    IDispatch* dispatch;
    VARIANTARG varg0, varg1;

    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    if (pDispParams->cArgs == 0)
        return DISP_E_TYPEMISMATCH;

    if (V_VT(&pDispParams->rgvarg[pDispParams->cArgs - 1]) != VT_BSTR)
        return DISP_E_TYPEMISMATCH;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    VariantInit(&varg1);
    if (pDispParams->cArgs == 2)
    {
        hr = DispGetParam(pDispParams, 1, VT_I4, &varg1, puArgErr);
        if (FAILED(hr))
            goto done;
    }
    else
    {
        V_VT(&varg1) = VT_I4;
        V_I4(&varg1) = 0;
    }

    V_VT(pVarResult) = VT_DISPATCH;

    ret = MsiOpenPackageExW(V_BSTR(&varg0), V_I4(&varg1), &hpkg);
    if (ret != ERROR_SUCCESS)
    {
        hr = DISP_E_EXCEPTION;
        goto done;
    }

    hr = create_session(hpkg, &This->IDispatch_iface, &dispatch);
    if (SUCCEEDED(hr))
        V_DISPATCH(pVarResult) = dispatch;

done:
    VariantClear(&varg0);
    VariantClear(&varg1);
    return hr;
}

static HRESULT InstallerImpl_OpenProduct(WORD wFlags,
                                         DISPPARAMS* pDispParams,
                                         VARIANT* pVarResult,
                                         EXCEPINFO* pExcepInfo,
                                         UINT* puArgErr)
{
    HRESULT hr;
    VARIANTARG varg0;

    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    FIXME("%s\n", debugstr_w(V_BSTR(&varg0)));

    VariantInit(pVarResult);

    VariantClear(&varg0);
    return S_OK;
}

static HRESULT InstallerImpl_OpenDatabase(WORD wFlags,
                                          DISPPARAMS* pDispParams,
                                          VARIANT* pVarResult,
                                          EXCEPINFO* pExcepInfo,
                                          UINT* puArgErr)
{
    UINT ret;
    HRESULT hr;
    MSIHANDLE hdb;
    IDispatch* dispatch;
    VARIANTARG varg0, varg1;

    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    VariantInit(&varg1);
    hr = DispGetParam(pDispParams, 1, VT_BSTR, &varg1, puArgErr);
    if (FAILED(hr))
        goto done;

    V_VT(pVarResult) = VT_DISPATCH;

    ret = MsiOpenDatabaseW(V_BSTR(&varg0), V_BSTR(&varg1), &hdb);
    if (ret != ERROR_SUCCESS)
    {
        hr = DISP_E_EXCEPTION;
        goto done;
    }

    hr = create_database(hdb, &dispatch);
    if (SUCCEEDED(hr))
        V_DISPATCH(pVarResult) = dispatch;

done:
    VariantClear(&varg0);
    VariantClear(&varg1);
    return hr;
}

static HRESULT InstallerImpl_SummaryInformation(WORD wFlags,
                                                DISPPARAMS* pDispParams,
                                                VARIANT* pVarResult,
                                                EXCEPINFO* pExcepInfo,
                                                UINT* puArgErr)
{
    UINT ret;
    HRESULT hr;
    MSIHANDLE hsuminfo;
    IDispatch *dispatch;
    VARIANTARG varg0, varg1;

    if (!(wFlags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg1);
    hr = DispGetParam(pDispParams, 1, VT_I4, &varg1, puArgErr);
    if (FAILED(hr))
        return hr;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    ret = MsiGetSummaryInformationW(0, V_BSTR(&varg0), V_I4(&varg1), &hsuminfo);
    VariantClear(&varg0);
    if (ret != ERROR_SUCCESS)
        return DISP_E_EXCEPTION;

    hr = create_summaryinfo(hsuminfo, &dispatch);
    if (FAILED(hr))
        return hr;

    V_VT(pVarResult) = VT_DISPATCH;
    V_DISPATCH(pVarResult) = dispatch;
    return S_OK;
}

static HRESULT InstallerImpl_UILevel(WORD wFlags,
                                     DISPPARAMS* pDispParams,
                                     VARIANT* pVarResult,
                                     EXCEPINFO* pExcepInfo,
                                     UINT* puArgErr)
{
    HRESULT hr;
    VARIANTARG varg0;
    INSTALLUILEVEL ui;

    if (!(wFlags & DISPATCH_PROPERTYPUT) && !(wFlags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    if (wFlags & DISPATCH_PROPERTYPUT)
    {
        VariantInit(&varg0);
        hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
        if (FAILED(hr))
            return hr;

        ui = MsiSetInternalUI(V_I4(&varg0), NULL);
        if (ui == INSTALLUILEVEL_NOCHANGE)
            return DISP_E_EXCEPTION;
    }
    else if (wFlags & DISPATCH_PROPERTYGET)
    {
        ui = MsiSetInternalUI(INSTALLUILEVEL_NOCHANGE, NULL);
        if (ui == INSTALLUILEVEL_NOCHANGE)
            return DISP_E_EXCEPTION;

        V_VT(pVarResult) = VT_I4;
        V_I4(pVarResult) = ui;
    }

    return S_OK;
}

static HRESULT InstallerImpl_EnableLog(WORD wFlags,
                                       DISPPARAMS* pDispParams,
                                       VARIANT* pVarResult,
                                       EXCEPINFO* pExcepInfo,
                                       UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

static HRESULT InstallerImpl_InstallProduct(WORD wFlags,
                                            DISPPARAMS* pDispParams,
                                            VARIANT* pVarResult,
                                            EXCEPINFO* pExcepInfo,
                                            UINT* puArgErr)
{
    UINT ret;
    HRESULT hr;
    VARIANTARG varg0, varg1;

    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    VariantInit(&varg1);
    hr = DispGetParam(pDispParams, 1, VT_BSTR, &varg1, puArgErr);
    if (FAILED(hr))
        goto done;

    ret = MsiInstallProductW(V_BSTR(&varg0), V_BSTR(&varg1));
    if (ret != ERROR_SUCCESS)
    {
        hr = DISP_E_EXCEPTION;
        goto done;
    }

done:
    VariantClear(&varg0);
    VariantClear(&varg1);
    return hr;
}

static HRESULT InstallerImpl_Version(WORD wFlags,
                                     VARIANT* pVarResult,
                                     EXCEPINFO* pExcepInfo,
                                     UINT* puArgErr)
{
    HRESULT hr;
    DLLVERSIONINFO verinfo;
    WCHAR version[MAX_PATH];

    if (!(wFlags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    verinfo.cbSize = sizeof(DLLVERSIONINFO);
    hr = DllGetVersion(&verinfo);
    if (FAILED(hr))
        return hr;

    swprintf(version, ARRAY_SIZE(version), L"%d.%d.%d.%d", verinfo.dwMajorVersion, verinfo.dwMinorVersion,
             verinfo.dwBuildNumber, verinfo.dwPlatformID);

    V_VT(pVarResult) = VT_BSTR;
    V_BSTR(pVarResult) = SysAllocString(version);
    return S_OK;
}

static HRESULT InstallerImpl_LastErrorRecord(WORD wFlags,
                                             DISPPARAMS* pDispParams,
                                             VARIANT* pVarResult,
                                             EXCEPINFO* pExcepInfo,
                                             UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

static HRESULT InstallerImpl_RegistryValue(WORD wFlags,
                                           DISPPARAMS* pDispParams,
                                           VARIANT* pVarResult,
                                           EXCEPINFO* pExcepInfo,
                                           UINT* puArgErr)
{
    UINT ret;
    HKEY hkey = NULL;
    HRESULT hr;
    UINT posValue;
    DWORD type, size;
    LPWSTR szString = NULL;
    VARIANTARG varg0, varg1, varg2;

    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_I4, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    VariantInit(&varg1);
    hr = DispGetParam(pDispParams, 1, VT_BSTR, &varg1, puArgErr);
    if (FAILED(hr))
        goto done;

    /* Save valuePos so we can save puArgErr if we are unable to do our type
     * conversions.
     */
    posValue = 2;
    VariantInit(&varg2);
    hr = DispGetParam_CopyOnly(pDispParams, &posValue, &varg2);
    if (FAILED(hr))
        goto done;

    if (V_I4(&varg0) >= REG_INDEX_CLASSES_ROOT &&
        V_I4(&varg0) <= REG_INDEX_DYN_DATA)
    {
        V_I4(&varg0) |= (UINT_PTR)HKEY_CLASSES_ROOT;
    }

    ret = RegOpenKeyW((HKEY)(UINT_PTR)V_I4(&varg0), V_BSTR(&varg1), &hkey);

    /* Only VT_EMPTY case can do anything if the key doesn't exist. */
    if (ret != ERROR_SUCCESS && V_VT(&varg2) != VT_EMPTY)
    {
        hr = DISP_E_BADINDEX;
        goto done;
    }

    /* Third parameter can be VT_EMPTY, VT_I4, or VT_BSTR */
    switch (V_VT(&varg2))
    {
        /* Return VT_BOOL clarifying whether registry key exists or not. */
        case VT_EMPTY:
            V_VT(pVarResult) = VT_BOOL;
            V_BOOL(pVarResult) = (ret == ERROR_SUCCESS) ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        /* Return the value of specified key if it exists. */
        case VT_BSTR:
            ret = RegQueryValueExW(hkey, V_BSTR(&varg2),
                                   NULL, NULL, NULL, &size);
            if (ret != ERROR_SUCCESS)
            {
                hr = DISP_E_BADINDEX;
                goto done;
            }

            szString = malloc(size);
            if (!szString)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            ret = RegQueryValueExW(hkey, V_BSTR(&varg2), NULL,
                                   &type, (LPBYTE)szString, &size);
            if (ret != ERROR_SUCCESS)
            {
                free(szString);
                hr = DISP_E_BADINDEX;
                goto done;
            }

            variant_from_registry_value(pVarResult, type,
                                        (LPBYTE)szString, size);
            free(szString);
            break;

        /* Try to make it into VT_I4, can use VariantChangeType for this. */
        default:
            hr = VariantChangeType(&varg2, &varg2, 0, VT_I4);
            if (FAILED(hr))
            {
                if (hr == DISP_E_TYPEMISMATCH)
                    *puArgErr = posValue;

                goto done;
            }

            /* Retrieve class name or maximum value name or subkey name size. */
            if (!V_I4(&varg2))
                ret = RegQueryInfoKeyW(hkey, NULL, &size, NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL);
            else if (V_I4(&varg2) > 0)
                ret = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, NULL,
                                       NULL, NULL, &size, NULL, NULL, NULL);
            else /* V_I4(&varg2) < 0 */
                ret = RegQueryInfoKeyW(hkey, NULL, NULL, NULL, NULL, &size,
                                       NULL, NULL, NULL, NULL, NULL, NULL);

            if (ret != ERROR_SUCCESS)
                goto done;

            szString = malloc(++size * sizeof(WCHAR));
            if (!szString)
            {
                hr = E_OUTOFMEMORY;
                goto done;
            }

            if (!V_I4(&varg2))
                ret = RegQueryInfoKeyW(hkey, szString, &size,NULL, NULL, NULL,
                                       NULL, NULL, NULL, NULL, NULL, NULL);
            else if (V_I4(&varg2) > 0)
                ret = RegEnumValueW(hkey, V_I4(&varg2)-1, szString,
                                    &size, 0, 0, NULL, NULL);
            else /* V_I4(&varg2) < 0 */
                ret = RegEnumKeyW(hkey, -1 - V_I4(&varg2), szString, size);

            if (ret == ERROR_SUCCESS)
            {
                V_VT(pVarResult) = VT_BSTR;
                V_BSTR(pVarResult) = SysAllocString(szString);
            }

            free(szString);
    }

done:
    VariantClear(&varg0);
    VariantClear(&varg1);
    VariantClear(&varg2);
    RegCloseKey(hkey);
    return hr;
}

static HRESULT InstallerImpl_Environment(WORD wFlags,
                                         DISPPARAMS* pDispParams,
                                         VARIANT* pVarResult,
                                         EXCEPINFO* pExcepInfo,
                                         UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

static HRESULT InstallerImpl_FileAttributes(WORD wFlags,
                                            DISPPARAMS* pDispParams,
                                            VARIANT* pVarResult,
                                            EXCEPINFO* pExcepInfo,
                                            UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

static HRESULT InstallerImpl_FileSize(WORD wFlags,
                                      DISPPARAMS* pDispParams,
                                      VARIANT* pVarResult,
                                      EXCEPINFO* pExcepInfo,
                                      UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

static HRESULT InstallerImpl_FileVersion(WORD wFlags,
                                         DISPPARAMS* pDispParams,
                                         VARIANT* pVarResult,
                                         EXCEPINFO* pExcepInfo,
                                         UINT* puArgErr)
{
    if (!(wFlags & DISPATCH_METHOD))
        return DISP_E_MEMBERNOTFOUND;

    FIXME("\n");

    VariantInit(pVarResult);
    return S_OK;
}

static HRESULT InstallerImpl_ProductState(WORD wFlags,
                                          DISPPARAMS* pDispParams,
                                          VARIANT* pVarResult,
                                          EXCEPINFO* pExcepInfo,
                                          UINT* puArgErr)
{
    HRESULT hr;
    VARIANTARG varg0;

    if (!(wFlags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    V_VT(pVarResult) = VT_I4;
    V_I4(pVarResult) = MsiQueryProductStateW(V_BSTR(&varg0));

    VariantClear(&varg0);
    return S_OK;
}

static HRESULT InstallerImpl_ProductInfo(WORD wFlags,
                                         DISPPARAMS* pDispParams,
                                         VARIANT* pVarResult,
                                         EXCEPINFO* pExcepInfo,
                                         UINT* puArgErr)
{
    UINT ret;
    HRESULT hr;
    DWORD size;
    LPWSTR str = NULL;
    VARIANTARG varg0, varg1;

    if (!(wFlags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&varg0);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &varg0, puArgErr);
    if (FAILED(hr))
        return hr;

    VariantInit(&varg1);
    hr = DispGetParam(pDispParams, 1, VT_BSTR, &varg1, puArgErr);
    if (FAILED(hr))
        goto done;

    V_VT(pVarResult) = VT_BSTR;
    V_BSTR(pVarResult) = NULL;

    ret = MsiGetProductInfoW(V_BSTR(&varg0), V_BSTR(&varg1), NULL, &size);
    if (ret != ERROR_SUCCESS)
    {
        hr = DISP_E_EXCEPTION;
        goto done;
    }

    str = malloc(++size * sizeof(WCHAR));
    if (!str)
    {
        hr = E_OUTOFMEMORY;
        goto done;
    }

    ret = MsiGetProductInfoW(V_BSTR(&varg0), V_BSTR(&varg1), str, &size);
    if (ret != ERROR_SUCCESS)
    {
        hr = DISP_E_EXCEPTION;
        goto done;
    }

    V_BSTR(pVarResult) = SysAllocString(str);
    hr = S_OK;

done:
    free(str);
    VariantClear(&varg0);
    VariantClear(&varg1);
    return hr;
}

static HRESULT InstallerImpl_Products(WORD flags,
                                      DISPPARAMS* pDispParams,
                                      VARIANT* result,
                                      EXCEPINFO* pExcepInfo,
                                      UINT* puArgErr)
{
    IDispatch *dispatch;
    HRESULT hr;

    if (!(flags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    hr = create_list(NULL, &dispatch);
    if (FAILED(hr))
        return hr;

    V_VT(result) = VT_DISPATCH;
    V_DISPATCH(result) = dispatch;

    return hr;
}

static HRESULT InstallerImpl_RelatedProducts(WORD flags,
                                             DISPPARAMS* pDispParams,
                                             VARIANT* result,
                                             EXCEPINFO* pExcepInfo,
                                             UINT* puArgErr)
{
    IDispatch* dispatch;
    VARIANTARG related;
    HRESULT hr;

    if (!(flags & DISPATCH_PROPERTYGET))
        return DISP_E_MEMBERNOTFOUND;

    VariantInit(&related);
    hr = DispGetParam(pDispParams, 0, VT_BSTR, &related, puArgErr);
    if (FAILED(hr))
        return hr;

    hr = create_list(V_BSTR(&related), &dispatch);
    VariantClear(&related);

    V_VT(result) = VT_DISPATCH;
    V_DISPATCH(result) = dispatch;

    return hr;
}

static HRESULT installer_invoke(
        struct automation_object *This,
        DISPID dispIdMember,
        REFIID riid,
        LCID lcid,
        WORD wFlags,
        DISPPARAMS* pDispParams,
        VARIANT* pVarResult,
        EXCEPINFO* pExcepInfo,
        UINT* puArgErr)
{
    switch (dispIdMember)
    {
        case DISPID_INSTALLER_CREATERECORD:
            return InstallerImpl_CreateRecord(wFlags, pDispParams,
                                              pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_OPENPACKAGE:
            return InstallerImpl_OpenPackage(This, wFlags, pDispParams,
                                             pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_OPENPRODUCT:
            return InstallerImpl_OpenProduct(wFlags, pDispParams,
                                             pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_OPENDATABASE:
            return InstallerImpl_OpenDatabase(wFlags, pDispParams,
                                              pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_SUMMARYINFORMATION:
            return InstallerImpl_SummaryInformation(wFlags, pDispParams,
                                                    pVarResult, pExcepInfo,
                                                    puArgErr);

        case DISPID_INSTALLER_UILEVEL:
            return InstallerImpl_UILevel(wFlags, pDispParams,
                                         pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_ENABLELOG:
            return InstallerImpl_EnableLog(wFlags, pDispParams,
                                           pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_INSTALLPRODUCT:
            return InstallerImpl_InstallProduct(wFlags, pDispParams,
                                                pVarResult, pExcepInfo,
                                                puArgErr);

        case DISPID_INSTALLER_VERSION:
            return InstallerImpl_Version(wFlags, pVarResult,
                                         pExcepInfo, puArgErr);

        case DISPID_INSTALLER_LASTERRORRECORD:
            return InstallerImpl_LastErrorRecord(wFlags, pDispParams,
                                                 pVarResult, pExcepInfo,
                                                 puArgErr);

        case DISPID_INSTALLER_REGISTRYVALUE:
            return InstallerImpl_RegistryValue(wFlags, pDispParams,
                                               pVarResult, pExcepInfo,
                                               puArgErr);

        case DISPID_INSTALLER_ENVIRONMENT:
            return InstallerImpl_Environment(wFlags, pDispParams,
                                             pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_FILEATTRIBUTES:
            return InstallerImpl_FileAttributes(wFlags, pDispParams,
                                                pVarResult, pExcepInfo,
                                                puArgErr);

        case DISPID_INSTALLER_FILESIZE:
            return InstallerImpl_FileSize(wFlags, pDispParams,
                                          pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_FILEVERSION:
            return InstallerImpl_FileVersion(wFlags, pDispParams,
                                             pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_PRODUCTSTATE:
            return InstallerImpl_ProductState(wFlags, pDispParams,
                                              pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_PRODUCTINFO:
            return InstallerImpl_ProductInfo(wFlags, pDispParams,
                                             pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_PRODUCTS:
            return InstallerImpl_Products(wFlags, pDispParams,
                                          pVarResult, pExcepInfo, puArgErr);

        case DISPID_INSTALLER_RELATEDPRODUCTS:
            return InstallerImpl_RelatedProducts(wFlags, pDispParams,
                                                 pVarResult, pExcepInfo,
                                                 puArgErr);

        default:
            return DISP_E_MEMBERNOTFOUND;
    }
}

HRESULT create_msiserver(IUnknown *outer, void **ppObj)
{
    struct automation_object *installer;

    TRACE("(%p %p)\n", outer, ppObj);

    if (outer)
        return CLASS_E_NOAGGREGATION;

    installer = malloc(sizeof(*installer));
    if (!installer) return E_OUTOFMEMORY;

    init_automation_object(installer, 0, Installer_tid);

    *ppObj = &installer->IDispatch_iface;

    return S_OK;
}

HRESULT create_session(MSIHANDLE msiHandle, IDispatch *installer, IDispatch **disp)
{
    struct session_object *session;

    session = malloc(sizeof(*session));
    if (!session) return E_OUTOFMEMORY;

    init_automation_object(&session->autoobj, msiHandle, Session_tid);

    session->installer = installer;
    *disp = &session->autoobj.IDispatch_iface;

    return S_OK;
}

static HRESULT create_database(MSIHANDLE msiHandle, IDispatch **dispatch)
{
    struct automation_object *database;

    TRACE("%lu %p\n", msiHandle, dispatch);

    database = malloc(sizeof(*database));
    if (!database) return E_OUTOFMEMORY;

    init_automation_object(database, msiHandle, Database_tid);

    *dispatch = &database->IDispatch_iface;

    return S_OK;
}

static HRESULT create_view(MSIHANDLE msiHandle, IDispatch **dispatch)
{
    struct automation_object *view;

    TRACE("%lu %p\n", msiHandle, dispatch);

    view = malloc(sizeof(*view));
    if (!view) return E_OUTOFMEMORY;

    init_automation_object(view, msiHandle, View_tid);

    *dispatch = &view->IDispatch_iface;

    return S_OK;
}

static HRESULT create_summaryinfo(MSIHANDLE msiHandle, IDispatch **disp)
{
    struct automation_object *info;

    info = malloc(sizeof(*info));
    if (!info) return E_OUTOFMEMORY;

    init_automation_object(info, msiHandle, SummaryInfo_tid);

    *disp = &info->IDispatch_iface;

    return S_OK;
}
