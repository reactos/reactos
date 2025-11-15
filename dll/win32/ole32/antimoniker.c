/*
 *	                      AntiMonikers implementation
 *
 *               Copyright 1999  Noomen Hamza
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
#include "objbase.h"
#include "wine/debug.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* AntiMoniker data structure */
typedef struct AntiMonikerImpl{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    LONG refcount;
    IUnknown *pMarshal; /* custom marshaler */
    DWORD count;
} AntiMonikerImpl;

static inline AntiMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, AntiMonikerImpl, IMoniker_iface);
}

static inline AntiMonikerImpl *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, AntiMonikerImpl, IROTData_iface);
}

static AntiMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface);

BOOL is_anti_moniker(IMoniker *iface, DWORD *order)
{
    AntiMonikerImpl *moniker = unsafe_impl_from_IMoniker(iface);

    if (!moniker)
    {
        *order = 0;
        return FALSE;
    }

    *order = moniker->count;

    return TRUE;
}

/*******************************************************************************
 *        AntiMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_QueryInterface(IMoniker* iface,REFIID riid,void** ppvObject)
{
    AntiMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    if ( ppvObject==0 )
	return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid) ||
        IsEqualGUID(&CLSID_AntiMoniker, riid))
    {
        *ppvObject = iface;
    }
    else if (IsEqualIID(&IID_IROTData, riid))
        *ppvObject = &This->IROTData_iface;
    else if (IsEqualIID(&IID_IMarshal, riid))
    {
        HRESULT hr = S_OK;
        if (!This->pMarshal)
            hr = MonikerMarshal_Create(iface, &This->pMarshal);
        if (hr != S_OK)
            return hr;
        return IUnknown_QueryInterface(This->pMarshal, riid, ppvObject);
    }

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI AntiMonikerImpl_AddRef(IMoniker *iface)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedIncrement(&moniker->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    return refcount;
}

/******************************************************************************
 *        AntiMoniker_Release
 ******************************************************************************/
static ULONG WINAPI AntiMonikerImpl_Release(IMoniker *iface)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->refcount);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->pMarshal) IUnknown_Release(moniker->pMarshal);
        free(moniker);
    }

    return refcount;
}

/******************************************************************************
 *        AntiMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_AntiMoniker;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

/******************************************************************************
 *        AntiMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_Load(IMoniker *iface, IStream *stream)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface);
    DWORD count = 0;
    HRESULT hr;

    TRACE("%p, %p.\n", iface, stream);

    if (FAILED(hr = IStream_Read(stream, &count, sizeof(count), NULL)))
        return hr;

    if (count > 0xfffff)
        return E_INVALIDARG;

    moniker->count = count;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_Save(IMoniker *iface, IStream *stream, BOOL clear_dirty)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %p, %d.\n", iface, stream, clear_dirty);

    return IStream_Write(stream, &moniker->count, sizeof(moniker->count), NULL);
}

/******************************************************************************
 *        AntiMoniker_GetSizeMax
 *
 * PARAMS
 * pcbSize [out] Pointer to size of stream needed to save object
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    /* for more details see AntiMonikerImpl_Save comments */

    /*
     * Normally the sizemax must be sizeof DWORD, but
     * I tested this function it usually return 16 bytes
     * more than the number of bytes used by AntiMoniker::Save function
     */
    pcbSize->u.LowPart =  sizeof(DWORD)+16;

    pcbSize->u.HighPart=0;

    return S_OK;
}

/******************************************************************************
 *                  AntiMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_BindToObject(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                             REFIID riid, VOID** ppvResult)
{
    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_BindToStorage(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                              REFIID riid, VOID** ppvResult)
{
    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);
    return E_NOTIMPL;
}

static HRESULT WINAPI
AntiMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc, DWORD dwReduceHowFar,
                       IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    TRACE("%p, %p, %ld, %p, %p.\n", iface, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    AntiMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}
/******************************************************************************
 *        AntiMoniker_ComposeWith
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_ComposeWith(IMoniker* iface, IMoniker* pmkRight,
                            BOOL fOnlyIfNotGeneric, IMoniker** ppmkComposite)
{

    TRACE("(%p,%p,%d,%p)\n",iface,pmkRight,fOnlyIfNotGeneric,ppmkComposite);

    if ((ppmkComposite==NULL)||(pmkRight==NULL))
	return E_POINTER;

    *ppmkComposite=0;

    if (fOnlyIfNotGeneric)
        return MK_E_NEEDGENERIC;
    else
        return CreateGenericComposite(iface,pmkRight,ppmkComposite);
}

static HRESULT WINAPI AntiMonikerImpl_Enum(IMoniker *iface, BOOL forward, IEnumMoniker **ppenumMoniker)
{
    TRACE("%p, %d, %p.\n", iface, forward, ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_INVALIDARG;

    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_IsEqual(IMoniker *iface, IMoniker *other)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p.\n", iface, other);

    if (!other)
        return E_INVALIDARG;

    other_moniker = unsafe_impl_from_IMoniker(other);
    if (!other_moniker)
        return S_FALSE;

    return moniker->count == other_moniker->count ? S_OK : S_FALSE;
}

/******************************************************************************
 *        AntiMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_Hash(IMoniker *iface, DWORD *hash)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %p.\n", iface, hash);

    if (!hash)
        return E_POINTER;

    *hash = 0x80000000 | moniker->count;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsRunning(IMoniker* iface, IBindCtx* pbc, IMoniker* pmkToLeft,
                          IMoniker* pmkNewlyRunning)
{
    IRunningObjectTable* rot;
    HRESULT res;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    if (pbc==NULL)
        return E_INVALIDARG;

    res=IBindCtx_GetRunningObjectTable(pbc,&rot);

    if (FAILED(res))
        return res;

    res = IRunningObjectTable_IsRunning(rot,iface);

    IRunningObjectTable_Release(rot);

    return res;
}

/******************************************************************************
 *        AntiMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pAntiTime)
{
    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pAntiTime);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p,%p)\n",iface,ppmk);

    if (ppmk==NULL)
        return E_POINTER;

    *ppmk=0;

    return MK_E_NOINVERSE;
}

/******************************************************************************
 *        AntiMoniker_CommonPrefixWith
 ******************************************************************************/
static HRESULT WINAPI AntiMonikerImpl_CommonPrefixWith(IMoniker *iface, IMoniker *other, IMoniker **prefix)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface), *other_moniker;
    HRESULT hr;

    TRACE("%p, %p, %p.\n", iface, other, prefix);

    other_moniker = unsafe_impl_from_IMoniker(other);
    if (other_moniker)
    {
        if (moniker->count <= other_moniker->count)
        {
            *prefix = iface;
            hr = moniker->count == other_moniker->count ? MK_S_US : MK_S_ME;
        }
        else
        {
            *prefix = other;
            hr = MK_S_HIM;
        }

        IMoniker_AddRef(*prefix);
        return hr;
    }

    return MonikerCommonPrefixWith(iface, other, prefix);
}

static HRESULT WINAPI AntiMonikerImpl_RelativePathTo(IMoniker *iface, IMoniker *other, IMoniker **result)
{
    TRACE("%p, %p, %p.\n", iface, other, result);

    if (!other || !result)
        return E_INVALIDARG;

    IMoniker_AddRef(other);
    *result = other;

    return MK_S_HIM;
}

/******************************************************************************
 *        AntiMoniker_GetDisplayName
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_GetDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *displayname)
{
    AntiMonikerImpl *moniker = impl_from_IMoniker(iface);
    static const WCHAR nameW[] = {'\\','.','.'};
    WCHAR *ptrW;
    int i;

    TRACE("%p, %p, %p, %p.\n", iface, pbc, pmkToLeft, displayname);

    if (!displayname)
        return E_POINTER;

    if (pmkToLeft!=NULL){
        FIXME("() pmkToLeft!=NULL not implemented\n");
        return E_NOTIMPL;
    }

    *displayname = ptrW = CoTaskMemAlloc((moniker->count * ARRAY_SIZE(nameW) + 1) * sizeof(WCHAR));
    if (!*displayname)
        return E_OUTOFMEMORY;

    for (i = 0; i < moniker->count; ++i)
        memcpy(ptrW + i * ARRAY_SIZE(nameW), nameW, sizeof(nameW));
    ptrW[moniker->count * ARRAY_SIZE(nameW)] = 0;

    return S_OK;
}

/******************************************************************************
 *        AntiMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_ParseDisplayName(IMoniker* iface, IBindCtx* pbc,
                                 IMoniker* pmkToLeft, LPOLESTR pszDisplayName,
                                 ULONG* pchEaten, IMoniker** ppmkOut)
{
    TRACE("(%p,%p,%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pszDisplayName,pchEaten,ppmkOut);
    return E_NOTIMPL;
}

/******************************************************************************
 *        AntiMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_ANTIMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        AntiMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI
AntiMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{
    AntiMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppvObject);

    return AntiMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        AntiMonikerIROTData_AddRef
 */
static ULONG WINAPI AntiMonikerROTDataImpl_AddRef(IROTData *iface)
{
    AntiMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return AntiMonikerImpl_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        AntiMonikerIROTData_Release
 */
static ULONG WINAPI AntiMonikerROTDataImpl_Release(IROTData* iface)
{
    AntiMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return AntiMonikerImpl_Release(&This->IMoniker_iface);
}

/******************************************************************************
 *        AntiMonikerIROTData_GetComparisonData
 ******************************************************************************/
static HRESULT WINAPI
AntiMonikerROTDataImpl_GetComparisonData(IROTData *iface, BYTE *data, ULONG data_len, ULONG *data_req)
{
    AntiMonikerImpl *moniker = impl_from_IROTData(iface);

    TRACE("%p, %p, %lu, %p.\n", iface, data, data_len, data_req);

    *data_req = sizeof(CLSID) + sizeof(DWORD);
    if (data_len < *data_req)
        return E_OUTOFMEMORY;

    memcpy(data, &CLSID_AntiMoniker, sizeof(CLSID));
    memcpy(data + sizeof(CLSID), &moniker->count, sizeof(moniker->count));

    return S_OK;
}

/********************************************************************************/
/* Virtual function table for the AntiMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static const IMonikerVtbl VT_AntiMonikerImpl =
{
    AntiMonikerImpl_QueryInterface,
    AntiMonikerImpl_AddRef,
    AntiMonikerImpl_Release,
    AntiMonikerImpl_GetClassID,
    AntiMonikerImpl_IsDirty,
    AntiMonikerImpl_Load,
    AntiMonikerImpl_Save,
    AntiMonikerImpl_GetSizeMax,
    AntiMonikerImpl_BindToObject,
    AntiMonikerImpl_BindToStorage,
    AntiMonikerImpl_Reduce,
    AntiMonikerImpl_ComposeWith,
    AntiMonikerImpl_Enum,
    AntiMonikerImpl_IsEqual,
    AntiMonikerImpl_Hash,
    AntiMonikerImpl_IsRunning,
    AntiMonikerImpl_GetTimeOfLastChange,
    AntiMonikerImpl_Inverse,
    AntiMonikerImpl_CommonPrefixWith,
    AntiMonikerImpl_RelativePathTo,
    AntiMonikerImpl_GetDisplayName,
    AntiMonikerImpl_ParseDisplayName,
    AntiMonikerImpl_IsSystemMoniker
};

static AntiMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &VT_AntiMonikerImpl)
        return NULL;
    return CONTAINING_RECORD(iface, AntiMonikerImpl, IMoniker_iface);
}

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl VT_ROTDataImpl =
{
    AntiMonikerROTDataImpl_QueryInterface,
    AntiMonikerROTDataImpl_AddRef,
    AntiMonikerROTDataImpl_Release,
    AntiMonikerROTDataImpl_GetComparisonData
};

HRESULT create_anti_moniker(DWORD order, IMoniker **ret)
{
    AntiMonikerImpl *moniker;

    if (!(moniker = calloc(1, sizeof(*moniker))))
        return E_OUTOFMEMORY;

    moniker->IMoniker_iface.lpVtbl = &VT_AntiMonikerImpl;
    moniker->IROTData_iface.lpVtbl = &VT_ROTDataImpl;
    moniker->refcount = 1;
    moniker->count = order;

    *ret = &moniker->IMoniker_iface;

    return S_OK;
}

/******************************************************************************
 *        CreateAntiMoniker	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateAntiMoniker(IMoniker **moniker)
{
    TRACE("%p.\n", moniker);

    return create_anti_moniker(1, moniker);
}

HRESULT WINAPI AntiMoniker_CreateInstance(IClassFactory *iface,
    IUnknown *pUnk, REFIID riid, void **ppv)
{
    IMoniker *pMoniker;
    HRESULT  hr;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateAntiMoniker(&pMoniker);
    if (FAILED(hr))
        return hr;

    hr = IMoniker_QueryInterface(pMoniker, riid, ppv);
    IMoniker_Release(pMoniker);

    return hr;
}
