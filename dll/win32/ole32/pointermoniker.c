/*
 * Pointer and Objref Monikers Implementation
 *
 * Copyright 1999 Noomen Hamza
 * Copyright 2008 Robert Shearman (for CodeWeavers)
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
#include <string.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "objbase.h"
#include "oleidl.h"
#include "wine/debug.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* PointerMoniker data structure */
typedef struct PointerMonikerImpl
{
    IMoniker IMoniker_iface;
    IMarshal IMarshal_iface;

    LONG refcount;

    IUnknown *pObject;
} PointerMonikerImpl;

static inline PointerMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, PointerMonikerImpl, IMoniker_iface);
}

static PointerMonikerImpl *impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, PointerMonikerImpl, IMarshal_iface);
}

static PointerMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface);

static HRESULT WINAPI PointerMonikerImpl_QueryInterface(IMoniker *iface, REFIID riid, void **ppvObject)
{
    PointerMonikerImpl *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid) ||
        IsEqualGUID(&CLSID_PointerMoniker, riid))
    {
        *ppvObject = iface;
    }
    else if (IsEqualIID(&IID_IMarshal, riid))
        *ppvObject = &moniker->IMarshal_iface;

    if (!*ppvObject)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI PointerMonikerImpl_AddRef(IMoniker *iface)
{
    PointerMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedIncrement(&moniker->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI PointerMonikerImpl_Release(IMoniker *iface)
{
    PointerMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->pObject) IUnknown_Release(moniker->pObject);
        free(moniker);
    }

    return refcount;
}

/******************************************************************************
 *        PointerMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_PointerMoniker;

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        PointerMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Load(IMoniker* iface,IStream* pStm)
{
    TRACE("(%p)\n", pStm);

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Save(IMoniker* iface, IStream* pStm, BOOL fClearDirty)
{
    TRACE("(%p, %d)\n", pStm, fClearDirty);

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_GetSizeMax
 *
 * PARAMS
 * pcbSize [out] Pointer to size of stream needed to save object
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    pcbSize->u.LowPart = 0;
    pcbSize->u.HighPart = 0;

    return E_NOTIMPL;
}

/******************************************************************************
 *                  PointerMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_BindToObject(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                             REFIID riid, VOID** ppvResult)
{
    PointerMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

    if (!This->pObject)
        return E_UNEXPECTED;

    return IUnknown_QueryInterface(This->pObject, riid, ppvResult);
}

/******************************************************************************
 *        PointerMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvResult)
{
    PointerMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

    if (!This->pObject)
        return E_UNEXPECTED;

    return IUnknown_QueryInterface(This->pObject, riid, ppvResult);
}

/******************************************************************************
 *        PointerMoniker_Reduce
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    TRACE("%p, %p, %ld, %p, %p.\n", iface, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    PointerMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI PointerMonikerImpl_ComposeWith(IMoniker *iface, IMoniker *right,
        BOOL only_if_not_generic, IMoniker **result)
{
    DWORD order;

    TRACE("%p, %p, %d, %p.\n", iface, right, only_if_not_generic, result);

    if (!result || !right)
        return E_POINTER;

    *result = NULL;

    if (is_anti_moniker(right, &order))
        return order > 1 ? create_anti_moniker(order - 1, result) : S_OK;

    return only_if_not_generic ? MK_E_NEEDGENERIC : CreateGenericComposite(iface, right, result);
}

/******************************************************************************
 *        PointerMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_Enum(IMoniker *iface, BOOL fForward, IEnumMoniker **ppenumMoniker)
{
    TRACE("%p, %d, %p.\n", iface, fForward, ppenumMoniker);

    if (!ppenumMoniker)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_IsEqual(IMoniker *iface, IMoniker *other)
{
    PointerMonikerImpl *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p.\n", iface, other);

    if (!other)
        return E_INVALIDARG;

    other_moniker = unsafe_impl_from_IMoniker(other);
    if (!other_moniker)
        return S_FALSE;

    return moniker->pObject == other_moniker->pObject ? S_OK : S_FALSE;
}

/******************************************************************************
 *        PointerMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    PointerMonikerImpl *This = impl_from_IMoniker(iface);

    if (pdwHash==NULL)
        return E_POINTER;

    *pdwHash = PtrToUlong(This->pObject);

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    return S_OK;
}

/******************************************************************************
 *        PointerMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pAntiTime)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pAntiTime);
    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p,%p)\n",iface,ppmk);

    return CreateAntiMoniker(ppmk);
}

/******************************************************************************
 *        PointerMoniker_CommonPrefixWith
 ******************************************************************************/
static HRESULT WINAPI PointerMonikerImpl_CommonPrefixWith(IMoniker *iface, IMoniker *other, IMoniker **prefix)
{
    TRACE("%p, %p, %p.\n", iface, other, prefix);

    if (!prefix || !other)
        return E_INVALIDARG;

    *prefix = NULL;

    if (PointerMonikerImpl_IsEqual(iface, other) == S_OK)
    {
        IMoniker_AddRef(iface);

        *prefix = iface;

        return MK_S_US;
    }
    else
        return MK_E_NOPREFIX;
}

static HRESULT WINAPI PointerMonikerImpl_RelativePathTo(IMoniker *iface, IMoniker *other, IMoniker **result)
{
    TRACE("%p, %p, %p.\n", iface, other, result);

    if (!result)
        return E_INVALIDARG;

    *result = NULL;

    return other ? E_NOTIMPL : E_INVALIDARG;
}

static HRESULT WINAPI PointerMonikerImpl_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *toleft, LPOLESTR *name)
{
    TRACE("%p, %p, %p, %p.\n", iface, pbc, toleft, name);

    if (!name || !pbc)
    {
        if (name) *name = NULL;
        return E_INVALIDARG;
    }

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_ParseDisplayName(IMoniker* iface, IBindCtx* pbc,
                                 IMoniker* pmkToLeft, LPOLESTR pszDisplayName,
                                 ULONG* pchEaten, IMoniker** ppmkOut)
{
    PointerMonikerImpl *This = impl_from_IMoniker(iface);
    HRESULT hr;
    IParseDisplayName *pPDN;

    TRACE("(%p,%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);

    if (pmkToLeft)
        return MK_E_SYNTAX;

    if (!This->pObject)
        return E_UNEXPECTED;

    hr = IUnknown_QueryInterface(This->pObject, &IID_IParseDisplayName, (void **)&pPDN);
    if (FAILED(hr))
        return hr;

    hr = IParseDisplayName_ParseDisplayName(pPDN, pbc, pszDisplayName, pchEaten, ppmkOut);
    IParseDisplayName_Release(pPDN);

    return hr;
}

/******************************************************************************
 *        PointerMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    *pwdMksys = MKSYS_POINTERMONIKER;

    return S_OK;
}

/********************************************************************************/
/* Virtual function table for the PointerMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static const IMonikerVtbl VT_PointerMonikerImpl =
{
    PointerMonikerImpl_QueryInterface,
    PointerMonikerImpl_AddRef,
    PointerMonikerImpl_Release,
    PointerMonikerImpl_GetClassID,
    PointerMonikerImpl_IsDirty,
    PointerMonikerImpl_Load,
    PointerMonikerImpl_Save,
    PointerMonikerImpl_GetSizeMax,
    PointerMonikerImpl_BindToObject,
    PointerMonikerImpl_BindToStorage,
    PointerMonikerImpl_Reduce,
    PointerMonikerImpl_ComposeWith,
    PointerMonikerImpl_Enum,
    PointerMonikerImpl_IsEqual,
    PointerMonikerImpl_Hash,
    PointerMonikerImpl_IsRunning,
    PointerMonikerImpl_GetTimeOfLastChange,
    PointerMonikerImpl_Inverse,
    PointerMonikerImpl_CommonPrefixWith,
    PointerMonikerImpl_RelativePathTo,
    PointerMonikerImpl_GetDisplayName,
    PointerMonikerImpl_ParseDisplayName,
    PointerMonikerImpl_IsSystemMoniker
};

static PointerMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &VT_PointerMonikerImpl)
        return NULL;
    return CONTAINING_RECORD(iface, PointerMonikerImpl, IMoniker_iface);
}

static HRESULT WINAPI pointer_moniker_marshal_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppv);
}

static ULONG WINAPI pointer_moniker_marshal_AddRef(IMarshal *iface)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p.\n",iface);

    return IMoniker_AddRef(&moniker->IMoniker_iface);
}

static ULONG WINAPI pointer_moniker_marshal_Release(IMarshal *iface)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p.\n",iface);

    return IMoniker_Release(&moniker->IMoniker_iface);
}

static HRESULT WINAPI pointer_moniker_marshal_GetUnmarshalClass(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *clsid)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %lx, %p, %lx, %p.\n", iface, debugstr_guid(riid), pv, dwDestContext, pvDestContext,
            mshlflags, clsid);

    return IMoniker_GetClassID(&moniker->IMoniker_iface, clsid);
}

static HRESULT WINAPI pointer_moniker_marshal_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *size)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %ld, %p, %#lx, %p.\n", iface, debugstr_guid(riid), pv, dwDestContext, pvDestContext,
            mshlflags, size);

    return CoGetMarshalSizeMax(size, &IID_IUnknown, moniker->pObject, dwDestContext, pvDestContext, mshlflags);
}

static HRESULT WINAPI pointer_moniker_marshal_MarshalInterface(IMarshal *iface, IStream *stream, REFIID riid,
        void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %lx, %p, %lx.\n", stream, debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags);

    return CoMarshalInterface(stream, &IID_IUnknown, moniker->pObject, dwDestContext,
                pvDestContext, mshlflags);
}

static HRESULT WINAPI pointer_moniker_marshal_UnmarshalInterface(IMarshal *iface, IStream *stream,
        REFIID riid, void **ppv)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);
    IUnknown *object;
    HRESULT hr;

    TRACE("%p, %p, %s, %p.\n", iface, stream, debugstr_guid(riid), ppv);

    hr = CoUnmarshalInterface(stream, &IID_IUnknown, (void **)&object);
    if (FAILED(hr))
    {
        ERR("Couldn't unmarshal moniker, hr = %#lx.\n", hr);
        return hr;
    }

    if (moniker->pObject)
        IUnknown_Release(moniker->pObject);
    moniker->pObject = object;

    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppv);
}

static HRESULT WINAPI pointer_moniker_marshal_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    TRACE("%p, %p.\n", iface, stream);

    return S_OK;
}

static HRESULT WINAPI pointer_moniker_marshal_DisconnectObject(IMarshal *iface, DWORD reserved)
{
    TRACE("%p, %#lx.\n", iface, reserved);

    return S_OK;
}

static const IMarshalVtbl pointer_moniker_marshal_vtbl =
{
    pointer_moniker_marshal_QueryInterface,
    pointer_moniker_marshal_AddRef,
    pointer_moniker_marshal_Release,
    pointer_moniker_marshal_GetUnmarshalClass,
    pointer_moniker_marshal_GetMarshalSizeMax,
    pointer_moniker_marshal_MarshalInterface,
    pointer_moniker_marshal_UnmarshalInterface,
    pointer_moniker_marshal_ReleaseMarshalData,
    pointer_moniker_marshal_DisconnectObject
};

/***********************************************************************
 *           CreatePointerMoniker (OLE32.@)
 */
HRESULT WINAPI CreatePointerMoniker(IUnknown *object, IMoniker **ret)
{
    PointerMonikerImpl *moniker;

    TRACE("(%p, %p)\n", object, ret);

    if (!ret)
        return E_INVALIDARG;

    if (!(moniker = calloc(1, sizeof(*moniker))))
    {
        *ret = NULL;
        return E_OUTOFMEMORY;
    }

    moniker->IMoniker_iface.lpVtbl = &VT_PointerMonikerImpl;
    moniker->IMarshal_iface.lpVtbl = &pointer_moniker_marshal_vtbl;
    moniker->refcount = 1;
    moniker->pObject = object;
    if (moniker->pObject)
        IUnknown_AddRef(moniker->pObject);

    *ret = &moniker->IMoniker_iface;

    return S_OK;
}

HRESULT WINAPI PointerMoniker_CreateInstance(IClassFactory *iface,
    IUnknown *pUnk, REFIID riid, void **ppv)
{
    IMoniker *pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreatePointerMoniker(NULL, &pMoniker);
    if (FAILED(hr))
        return hr;

    hr = IMoniker_QueryInterface(pMoniker, riid, ppv);
    IMoniker_Release(pMoniker);

    return hr;
}

/* ObjrefMoniker implementation */

typedef struct
{
    IMoniker IMoniker_iface;
    IMarshal IMarshal_iface;

    LONG refcount;

    IUnknown *pObject;
} ObjrefMonikerImpl;

static inline ObjrefMonikerImpl *objref_impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, ObjrefMonikerImpl, IMoniker_iface);
}

static ObjrefMonikerImpl *objref_impl_from_IMarshal(IMarshal *iface)
{
    return CONTAINING_RECORD(iface, ObjrefMonikerImpl, IMarshal_iface);
}

static HRESULT WINAPI ObjrefMonikerImpl_QueryInterface(IMoniker *iface, REFIID iid, void **obj)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMoniker(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), obj);

    if (!obj)
        return E_INVALIDARG;

    *obj = 0;

    if (IsEqualIID(iid, &IID_IUnknown) ||
        IsEqualIID(iid, &IID_IPersist) ||
        IsEqualIID(iid, &IID_IPersistStream) ||
        IsEqualIID(iid, &IID_IMoniker) ||
        IsEqualGUID(iid, &CLSID_ObjrefMoniker) ||
        IsEqualGUID(iid, &CLSID_PointerMoniker))
    {
        *obj = iface;
    }
    else if (IsEqualIID(iid, &IID_IMarshal))
        *obj = &moniker->IMarshal_iface;
    else
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

static ULONG WINAPI ObjrefMonikerImpl_AddRef(IMoniker *iface)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMoniker(iface);
    ULONG refcount = InterlockedIncrement(&moniker->refcount);

    TRACE("%p, refcount %lu\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI ObjrefMonikerImpl_Release(IMoniker *iface)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->refcount);

    TRACE("%p, refcount %lu\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->pObject) IUnknown_Release(moniker->pObject);
        free(moniker);
    }

    return refcount;
}

static HRESULT WINAPI ObjrefMonikerImpl_GetClassID(IMoniker *iface, CLSID *clsid)
{
    TRACE("(%p,%p)\n", iface, clsid);

    if (!clsid)
        return E_POINTER;

    *clsid = CLSID_ObjrefMoniker;
    return S_OK;
}

static HRESULT WINAPI ObjrefMonikerImpl_IsDirty(IMoniker *iface)
{
    FIXME("(%p): stub\n", iface);
    return S_FALSE;
}

static HRESULT WINAPI ObjrefMonikerImpl_Load(IMoniker *iface, IStream *stream)
{
    FIXME("(%p,%p): stub\n", iface, stream);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_Save(IMoniker *iface, IStream *stream, BOOL dirty)
{
    FIXME("(%p,%p,%d): stub\n", iface, stream, dirty);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *size)
{
    FIXME("(%p,%p): stub\n", iface, size);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_BindToObject(IMoniker *iface, IBindCtx *pbc, IMoniker *left,
        REFIID riid, void **result)
{
    FIXME("(%p,%p,%p,%s,%p): stub\n", iface, pbc, left, debugstr_guid(riid), result);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *left,
        REFIID riid, void **result)
{
    FIXME("(%p,%p,%p,%s,%p): stub\n", iface, pbc, left, debugstr_guid(riid), result);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_Reduce(IMoniker *iface, IBindCtx *pbc, DWORD howfar,
        IMoniker **left, IMoniker **reduced)
{
    FIXME("%p, %p, %ld, %p, %p: stub\n", iface, pbc, howfar, left, reduced);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_ComposeWith(IMoniker *iface, IMoniker *right,
        BOOL only_if_not_generic, IMoniker **result)
{
    FIXME("(%p,%p,%d,%p): stub\n", iface, right, only_if_not_generic, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_Enum(IMoniker *iface, BOOL forward, IEnumMoniker **enummoniker)
{
    TRACE("(%p,%d,%p)\n", iface, forward, enummoniker);

    if (!enummoniker)
        return E_POINTER;

    *enummoniker = NULL;
    return S_OK;
}

static HRESULT WINAPI ObjrefMonikerImpl_IsEqual(IMoniker *iface, IMoniker *other)
{
    FIXME("(%p,%p): stub\n", iface, other);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_Hash(IMoniker *iface, DWORD *hash)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMoniker(iface);

    TRACE("(%p,%p)\n", iface, hash);

    if (!hash)
        return E_POINTER;

    *hash = PtrToUlong(moniker->pObject);

    return S_OK;
}

static HRESULT WINAPI ObjrefMonikerImpl_IsRunning(IMoniker *iface, IBindCtx *pbc, IMoniker *left,
        IMoniker *running)
{
    FIXME("(%p,%p,%p,%p): stub\n", iface, pbc, left, running);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_GetTimeOfLastChange(IMoniker *iface,
                               IBindCtx *pbc, IMoniker *left, FILETIME *time)
{
    FIXME("(%p,%p,%p,%p): stub\n", iface, pbc, left, time);
    return MK_E_UNAVAILABLE;
}

static HRESULT WINAPI ObjrefMonikerImpl_Inverse(IMoniker *iface, IMoniker **moniker)
{
    FIXME("(%p,%p): stub\n", iface, moniker);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_CommonPrefixWith(IMoniker *iface, IMoniker *other, IMoniker **prefix)
{
    FIXME("(%p,%p,%p): stub\n", iface, other, prefix);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_RelativePathTo(IMoniker *iface, IMoniker *other, IMoniker **result)
{
    FIXME("(%p,%p,%p): stub\n", iface, other, result);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_GetDisplayName(IMoniker *iface, IBindCtx *pbc,
                               IMoniker *left, LPOLESTR *name)
{
    FIXME("(%p,%p,%p,%p): stub\n", iface, pbc, left, name);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *left, LPOLESTR name, ULONG *eaten, IMoniker **out)
{
    FIXME("(%p,%p,%p,%p,%p,%p): stub\n", iface, pbc, left, name, eaten, out);
    return E_NOTIMPL;
}

static HRESULT WINAPI ObjrefMonikerImpl_IsSystemMoniker(IMoniker *iface, DWORD *mksys)
{
    TRACE("(%p,%p)\n", iface, mksys);

    if (!mksys)
        return E_POINTER;

    *mksys = MKSYS_OBJREFMONIKER;
    return S_OK;
}

static const IMonikerVtbl VT_ObjrefMonikerImpl =
{
    ObjrefMonikerImpl_QueryInterface,
    ObjrefMonikerImpl_AddRef,
    ObjrefMonikerImpl_Release,
    ObjrefMonikerImpl_GetClassID,
    ObjrefMonikerImpl_IsDirty,
    ObjrefMonikerImpl_Load,
    ObjrefMonikerImpl_Save,
    ObjrefMonikerImpl_GetSizeMax,
    ObjrefMonikerImpl_BindToObject,
    ObjrefMonikerImpl_BindToStorage,
    ObjrefMonikerImpl_Reduce,
    ObjrefMonikerImpl_ComposeWith,
    ObjrefMonikerImpl_Enum,
    ObjrefMonikerImpl_IsEqual,
    ObjrefMonikerImpl_Hash,
    ObjrefMonikerImpl_IsRunning,
    ObjrefMonikerImpl_GetTimeOfLastChange,
    ObjrefMonikerImpl_Inverse,
    ObjrefMonikerImpl_CommonPrefixWith,
    ObjrefMonikerImpl_RelativePathTo,
    ObjrefMonikerImpl_GetDisplayName,
    ObjrefMonikerImpl_ParseDisplayName,
    ObjrefMonikerImpl_IsSystemMoniker
};

static HRESULT WINAPI objref_moniker_marshal_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);

    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(riid), ppv);

    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppv);
}

static ULONG WINAPI objref_moniker_marshal_AddRef(IMarshal *iface)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);

    TRACE("(%p)\n", iface);

    return IMoniker_AddRef(&moniker->IMoniker_iface);
}

static ULONG WINAPI objref_moniker_marshal_Release(IMarshal *iface)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);

    TRACE("(%p)\n", iface);

    return IMoniker_Release(&moniker->IMoniker_iface);
}

static HRESULT WINAPI objref_moniker_marshal_GetUnmarshalClass(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, CLSID *clsid)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %#lx, %p, %#lx, %p.\n", iface, debugstr_guid(riid), pv, dwDestContext, pvDestContext,
            mshlflags, clsid);

    return IMoniker_GetClassID(&moniker->IMoniker_iface, clsid);
}

static HRESULT WINAPI objref_moniker_marshal_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *size)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %#lx, %p, %#lx, %p.\n", iface, debugstr_guid(riid), pv, dwDestContext, pvDestContext,
            mshlflags, size);

    return CoGetMarshalSizeMax(size, &IID_IUnknown, moniker->pObject, dwDestContext, pvDestContext, mshlflags);
}

static HRESULT WINAPI objref_moniker_marshal_MarshalInterface(IMarshal *iface, IStream *stream, REFIID riid,
        void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %#lx, %p, %#lx\n", stream, debugstr_guid(riid), pv, dwDestContext, pvDestContext, mshlflags);

    return CoMarshalInterface(stream, &IID_IUnknown, moniker->pObject, dwDestContext, pvDestContext, mshlflags);
}

static HRESULT WINAPI objref_moniker_marshal_UnmarshalInterface(IMarshal *iface, IStream *stream,
        REFIID riid, void **ppv)
{
    ObjrefMonikerImpl *moniker = objref_impl_from_IMarshal(iface);
    IUnknown *object;
    HRESULT hr;

    TRACE("(%p,%p,%s,%p)\n", iface, stream, debugstr_guid(riid), ppv);

    hr = CoUnmarshalInterface(stream, &IID_IUnknown, (void **)&object);
    if (FAILED(hr))
    {
        ERR("Couldn't unmarshal moniker, hr = %#lx.\n", hr);
        return hr;
    }

    if (moniker->pObject)
        IUnknown_Release(moniker->pObject);
    moniker->pObject = object;

    return IMoniker_QueryInterface(&moniker->IMoniker_iface, riid, ppv);
}

static HRESULT WINAPI objref_moniker_marshal_ReleaseMarshalData(IMarshal *iface, IStream *stream)
{
    TRACE("(%p,%p)\n", iface, stream);
    return S_OK;
}

static HRESULT WINAPI objref_moniker_marshal_DisconnectObject(IMarshal *iface, DWORD reserved)
{
    TRACE("%p, %#lx.\n", iface, reserved);
    return S_OK;
}

static const IMarshalVtbl objref_moniker_marshal_vtbl =
{
    objref_moniker_marshal_QueryInterface,
    objref_moniker_marshal_AddRef,
    objref_moniker_marshal_Release,
    objref_moniker_marshal_GetUnmarshalClass,
    objref_moniker_marshal_GetMarshalSizeMax,
    objref_moniker_marshal_MarshalInterface,
    objref_moniker_marshal_UnmarshalInterface,
    objref_moniker_marshal_ReleaseMarshalData,
    objref_moniker_marshal_DisconnectObject
};

/***********************************************************************
 *           CreateObjrefMoniker (OLE32.@)
 */
HRESULT WINAPI CreateObjrefMoniker(IUnknown *obj, IMoniker **ret)
{
    ObjrefMonikerImpl *moniker;

    TRACE("(%p,%p)\n", obj, ret);

    if (!ret)
        return E_INVALIDARG;

    if (!(moniker = calloc(1, sizeof(*moniker))))
    {
        *ret = NULL;
        return E_OUTOFMEMORY;
    }

    moniker->IMoniker_iface.lpVtbl = &VT_ObjrefMonikerImpl;
    moniker->IMarshal_iface.lpVtbl = &objref_moniker_marshal_vtbl;
    moniker->refcount = 1;
    moniker->pObject = obj;
    if (moniker->pObject)
        IUnknown_AddRef(moniker->pObject);

    *ret = &moniker->IMoniker_iface;

    return S_OK;
}

HRESULT WINAPI ObjrefMoniker_CreateInstance(IClassFactory *iface, IUnknown *unk, REFIID iid, void **obj)
{
    IMoniker *moniker;
    HRESULT  hr;

    TRACE("(%p,%s,%p)\n", unk, debugstr_guid(iid), obj);

    *obj = NULL;

    if (unk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateObjrefMoniker(NULL, &moniker);
    if (FAILED(hr))
        return hr;

    hr = IMoniker_QueryInterface(moniker, iid, obj);
    IMoniker_Release(moniker);

    return hr;
}
