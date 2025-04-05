/*
 * Pointer Moniker Implementation
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
#define NONAMELESSUNION

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winuser.h"
#include "objbase.h"
#include "oleidl.h"
#include "wine/debug.h"
#include "wine/heap.h"
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

    TRACE("%p, refcount %u.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI PointerMonikerImpl_Release(IMoniker *iface)
{
    PointerMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->refcount);

    TRACE("%p, refcount %u.\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->pObject) IUnknown_Release(moniker->pObject);
        heap_free(moniker);
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
    TRACE("(%p,%p,%d,%p,%p)\n",iface,pbc,dwReduceHowFar,ppmkToLeft,ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    PointerMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        PointerMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{

    HRESULT res=S_OK;
    DWORD mkSys,mkSys2, order;
    IEnumMoniker* penumMk=0;
    IMoniker *pmostLeftMk=0;
    IMoniker* tempMkComposite=0;

    TRACE("(%p,%d,%p)\n", pmkRight, fOnlyIfNotGeneric, ppmkComposite);

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    if (is_anti_moniker(pmkRight, &order))
    {
        return order > 1 ? create_anti_moniker(order - 1, ppmkComposite) : S_OK;
    }
    else
    {
        /* if pmkRight is a composite whose leftmost component is an anti-moniker,           */
        /* the returned moniker is the composite after the leftmost anti-moniker is removed. */
        IMoniker_IsSystemMoniker(pmkRight,&mkSys);

         if(mkSys==MKSYS_GENERICCOMPOSITE){

            res=IMoniker_Enum(pmkRight,TRUE,&penumMk);

            if (FAILED(res))
                return res;

            res=IEnumMoniker_Next(penumMk,1,&pmostLeftMk,NULL);

            IMoniker_IsSystemMoniker(pmostLeftMk,&mkSys2);

            if(mkSys2==MKSYS_ANTIMONIKER){

                IMoniker_Release(pmostLeftMk);

                tempMkComposite=iface;
                IMoniker_AddRef(iface);

                while(IEnumMoniker_Next(penumMk,1,&pmostLeftMk,NULL)==S_OK){

                    res=CreateGenericComposite(tempMkComposite,pmostLeftMk,ppmkComposite);

                    IMoniker_Release(tempMkComposite);
                    IMoniker_Release(pmostLeftMk);

                    tempMkComposite=*ppmkComposite;
                    IMoniker_AddRef(tempMkComposite);
                }
                return res;
            }
            else
                return CreateGenericComposite(iface,pmkRight,ppmkComposite);
         }
         /* If pmkRight is not an anti-moniker, the method combines the two monikers into a generic
          composite if fOnlyIfNotGeneric is FALSE; if fOnlyIfNotGeneric is TRUE, the method returns
          a NULL moniker and a return value of MK_E_NEEDGENERIC */
          else
            if (!fOnlyIfNotGeneric)
                return CreateGenericComposite(iface,pmkRight,ppmkComposite);

            else
                return MK_E_NEEDGENERIC;
    }
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

/******************************************************************************
 *        PointerMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    TRACE("(%p,%p,%p)\n",iface,pmOther,ppmkRelPath);

    if (ppmkRelPath==NULL)
        return E_POINTER;

    *ppmkRelPath = NULL;

    return E_NOTIMPL;
}

/******************************************************************************
 *        PointerMoniker_GetDisplayName
 ******************************************************************************/
static HRESULT WINAPI
PointerMonikerImpl_GetDisplayName(IMoniker* iface, IBindCtx* pbc,
                               IMoniker* pmkToLeft, LPOLESTR *ppszDisplayName)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName);

    if (ppszDisplayName==NULL)
        return E_POINTER;

    *ppszDisplayName = NULL;
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

    TRACE("%p, %s, %p, %x, %p, %x, %p.\n", iface, debugstr_guid(riid), pv, dwDestContext, pvDestContext,
            mshlflags, clsid);

    return IMoniker_GetClassID(&moniker->IMoniker_iface, clsid);
}

static HRESULT WINAPI pointer_moniker_marshal_GetMarshalSizeMax(IMarshal *iface, REFIID riid, void *pv,
        DWORD dwDestContext, void *pvDestContext, DWORD mshlflags, DWORD *size)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %d, %p, %#x, %p.\n", iface, debugstr_guid(riid), pv, dwDestContext, pvDestContext,
            mshlflags, size);

    return CoGetMarshalSizeMax(size, &IID_IUnknown, moniker->pObject, dwDestContext, pvDestContext, mshlflags);
}

static HRESULT WINAPI pointer_moniker_marshal_MarshalInterface(IMarshal *iface, IStream *stream, REFIID riid,
        void *pv, DWORD dwDestContext, void *pvDestContext, DWORD mshlflags)
{
    PointerMonikerImpl *moniker = impl_from_IMarshal(iface);

    TRACE("%p, %s, %p, %x, %p, %x.\n", stream, debugstr_guid(riid), pv,
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
        ERR("Couldn't unmarshal moniker, hr = %#x.\n", hr);
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
    TRACE("%p, %#x.\n", iface, reserved);

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

    moniker = heap_alloc(sizeof(*moniker));
    if (!moniker)
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
