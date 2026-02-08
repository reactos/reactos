/*
 *	                      Class Monikers
 *
 *           Copyright 1999  Noomen Hamza
 *           Copyright 2005-2007  Robert Shearman
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

#include <assert.h>
#include <stdarg.h>
#include <string.h>

#define COBJMACROS

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wine/debug.h"
#include "ole2.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#define CHARS_IN_GUID 39

/* ClassMoniker data structure */
typedef struct ClassMoniker
{
    IMoniker IMoniker_iface;
    IROTData IROTData_iface;
    LONG ref;

    struct
    {
        CLSID clsid;
        DWORD data_len;
    } header;
    WCHAR *data;

    IUnknown *pMarshal; /* custom marshaler */
} ClassMoniker;

static inline ClassMoniker *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, ClassMoniker, IMoniker_iface);
}

static inline ClassMoniker *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, ClassMoniker, IROTData_iface);
}

static const IMonikerVtbl ClassMonikerVtbl;

static ClassMoniker *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &ClassMonikerVtbl)
        return NULL;
    return CONTAINING_RECORD(iface, ClassMoniker, IMoniker_iface);
}

/*******************************************************************************
 *        ClassMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI ClassMoniker_QueryInterface(IMoniker *iface, REFIID riid, void **ppvObject)
{
    ClassMoniker *This = impl_from_IMoniker(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_POINTER;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid) ||
        IsEqualGUID(&CLSID_ClassMoniker, riid))
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

    if (!*ppvObject)
        return E_NOINTERFACE;

    IMoniker_AddRef(iface);

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI ClassMoniker_AddRef(IMoniker* iface)
{
    ClassMoniker *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI ClassMoniker_Release(IMoniker* iface)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);
    ULONG ref = InterlockedDecrement(&moniker->ref);

    TRACE("%p, refcount %lu.\n", iface, ref);

    if (!ref)
    {
        if (moniker->pMarshal) IUnknown_Release(moniker->pMarshal);
        free(moniker->data);
        free(moniker);
    }

    return ref;
}

/******************************************************************************
 *        ClassMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p, %p)\n", iface, pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_ClassMoniker;

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

static HRESULT WINAPI ClassMoniker_Load(IMoniker *iface, IStream *stream)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);
    ULONG length;
    HRESULT hr;

    TRACE("%p, %p\n", iface, stream);

    hr = IStream_Read(stream, &moniker->header, sizeof(moniker->header), &length);
    if (hr != S_OK || length != sizeof(moniker->header)) return STG_E_READFAULT;

    if (moniker->header.data_len)
    {
        free(moniker->data);
        if (!(moniker->data = malloc(moniker->header.data_len)))
        {
            WARN("Failed to allocate moniker data of size %lu.\n", moniker->header.data_len);
            moniker->header.data_len = 0;
            return E_OUTOFMEMORY;
        }
        hr = IStream_Read(stream, moniker->data, moniker->header.data_len, &length);
        if (hr != S_OK || length != moniker->header.data_len) return STG_E_READFAULT;
    }

    return S_OK;
}

static HRESULT WINAPI ClassMoniker_Save(IMoniker *iface, IStream *stream, BOOL clear_dirty)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);
    HRESULT hr;

    TRACE("%p, %p, %d\n", iface, stream, clear_dirty);

    hr = IStream_Write(stream, &moniker->header, sizeof(moniker->header), NULL);

    if (SUCCEEDED(hr) && moniker->header.data_len)
        hr = IStream_Write(stream, moniker->data, moniker->header.data_len, NULL);

    return hr;
}

static HRESULT WINAPI ClassMoniker_GetSizeMax(IMoniker *iface, ULARGE_INTEGER *size)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %p\n", iface, size);

    size->QuadPart = sizeof(moniker->header) + moniker->header.data_len;

    return S_OK;
}

/******************************************************************************
 *                  ClassMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_BindToObject(IMoniker* iface,
                                            IBindCtx* pbc,
                                            IMoniker* pmkToLeft,
                                            REFIID riid,
                                            VOID** ppvResult)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);
    BIND_OPTS2 bindopts;
    IClassActivator *pActivator;
    HRESULT hr;

    TRACE("(%p, %p, %s, %p)\n", pbc, pmkToLeft, debugstr_guid(riid), ppvResult);

    bindopts.cbStruct = sizeof(bindopts);
    IBindCtx_GetBindOptions(pbc, (BIND_OPTS *)&bindopts);

    if (!pmkToLeft)
        return CoGetClassObject(&moniker->header.clsid, bindopts.dwClassContext, NULL,
                                riid, ppvResult);
    else
    {
        hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IClassActivator,
                                   (void **)&pActivator);
        if (FAILED(hr)) return hr;

        hr = IClassActivator_GetClassObject(pActivator, &moniker->header.clsid,
                                            bindopts.dwClassContext,
                                            bindopts.locale, riid, ppvResult);

        IClassActivator_Release(pActivator);

        return hr;
    }
}

/******************************************************************************
 *        ClassMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_BindToStorage(IMoniker* iface,
                                             IBindCtx* pbc,
                                             IMoniker* pmkToLeft,
                                             REFIID riid,
                                             VOID** ppvResult)
{
    TRACE("(%p, %p, %s, %p)\n", pbc, pmkToLeft, debugstr_guid(riid), ppvResult);
    return IMoniker_BindToObject(iface, pbc, pmkToLeft, riid, ppvResult);
}

static HRESULT WINAPI ClassMoniker_Reduce(IMoniker* iface, IBindCtx *pbc,
        DWORD dwReduceHowFar, IMoniker **ppmkToLeft, IMoniker **ppmkReduced)
{
    TRACE("%p, %p, %ld, %p, %p.\n", iface, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if (!ppmkReduced)
        return E_POINTER;

    IMoniker_AddRef(iface);

    *ppmkReduced = iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI ClassMoniker_ComposeWith(IMoniker *iface, IMoniker *right,
        BOOL only_if_not_generic, IMoniker **result)
{
    DWORD order;

    TRACE("%p, %p, %d, %p.\n", iface, right, only_if_not_generic, result);

    if (!result || !right)
        return E_POINTER;

    *result = NULL;

    if (is_anti_moniker(right, &order))
        return S_OK;

    return only_if_not_generic ? MK_E_NEEDGENERIC : CreateGenericComposite(iface, right, result);
}

/******************************************************************************
 *        ClassMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

static HRESULT WINAPI ClassMoniker_IsEqual(IMoniker *iface, IMoniker *other)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p.\n", iface, other);

    if (!other)
        return E_INVALIDARG;

    other_moniker = unsafe_impl_from_IMoniker(other);
    if (!other_moniker)
        return S_FALSE;

    return IsEqualGUID(&moniker->header.clsid, &other_moniker->header.clsid) ? S_OK : S_FALSE;
}

static HRESULT WINAPI ClassMoniker_Hash(IMoniker *iface, DWORD *hash)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);

    TRACE("%p, %p\n", iface, hash);

    *hash = moniker->header.clsid.Data1;

    return S_OK;
}

/******************************************************************************
 *        ClassMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_IsRunning(IMoniker* iface,
                                         IBindCtx* pbc,
                                         IMoniker* pmkToLeft,
                                         IMoniker* pmkNewlyRunning)
{
    TRACE("(%p, %p, %p)\n", pbc, pmkToLeft, pmkNewlyRunning);

    /* as in native */
    return E_NOTIMPL;
}

/******************************************************************************
 *        ClassMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_GetTimeOfLastChange(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   FILETIME* pItemTime)
{
    TRACE("(%p, %p, %p)\n", pbc, pmkToLeft, pItemTime);

    return MK_E_UNAVAILABLE;
}

/******************************************************************************
 *        ClassMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p)\n",ppmk);

    if (!ppmk)
        return E_POINTER;

    return CreateAntiMoniker(ppmk);
}

static HRESULT WINAPI ClassMoniker_CommonPrefixWith(IMoniker *iface, IMoniker *other, IMoniker **prefix)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p, %p\n", iface, other, prefix);

    *prefix = NULL;

    other_moniker = unsafe_impl_from_IMoniker(other);

    if (other_moniker)
    {
        if (!IsEqualGUID(&moniker->header.clsid, &other_moniker->header.clsid)) return MK_E_NOPREFIX;

        *prefix = iface;
        IMoniker_AddRef(iface);

        return MK_S_US;
    }

    return MonikerCommonPrefixWith(iface, other, prefix);
}

/******************************************************************************
 *        ClassMoniker_RelativePathTo
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_RelativePathTo(IMoniker* iface,IMoniker* pmOther, IMoniker** ppmkRelPath)
{
    TRACE("(%p, %p)\n",pmOther,ppmkRelPath);

    if (!ppmkRelPath)
        return E_POINTER;

    *ppmkRelPath = NULL;

    return MK_E_NOTBINDABLE;
}

static HRESULT WINAPI ClassMoniker_GetDisplayName(IMoniker *iface,
        IBindCtx *pbc, IMoniker *pmkToLeft, LPOLESTR *name)
{
    ClassMoniker *moniker = impl_from_IMoniker(iface);
    static const int name_len = CHARS_IN_GUID + 5 /* prefix */;
    const GUID *guid = &moniker->header.clsid;

    TRACE("%p, %p, %p, %p.\n", iface, pbc, pmkToLeft, name);

    if (!name)
        return E_POINTER;

    if (pmkToLeft)
        return E_INVALIDARG;

    if (!(*name = CoTaskMemAlloc(name_len * sizeof(WCHAR) + moniker->header.data_len)))
        return E_OUTOFMEMORY;

    swprintf(*name, name_len, L"clsid:%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
            guid->Data1, guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1], guid->Data4[2],
            guid->Data4[3], guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);

    if (moniker->header.data_len)
        lstrcatW(*name, moniker->data);
    lstrcatW(*name, L":");

    TRACE("Returning %s\n", debugstr_w(*name));

    return S_OK;
}

static HRESULT WINAPI ClassMoniker_ParseDisplayName(IMoniker *iface, IBindCtx *pbc,
        IMoniker *pmkToLeft, LPOLESTR display_name, ULONG *eaten, IMoniker **result)
{
    IParseDisplayName *parser;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p, %p\n", iface, pbc, pmkToLeft, debugstr_w(display_name), eaten, result);

    if (SUCCEEDED(hr = IMoniker_BindToObject(iface, pbc, pmkToLeft, &IID_IParseDisplayName, (void **)&parser)))
    {
        hr = IParseDisplayName_ParseDisplayName(parser, pbc, display_name, eaten, result);
        IParseDisplayName_Release(parser);
    }

    return hr;
}

/******************************************************************************
 *        ClassMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI ClassMoniker_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    *pwdMksys = MKSYS_CLASSMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        ClassMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI ClassMonikerROTData_QueryInterface(IROTData *iface,REFIID riid,VOID** ppvObject)
{

    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p, %s, %p)\n", iface, debugstr_guid(riid), ppvObject);

    return IMoniker_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        ClassMonikerIROTData_AddRef
 */
static ULONG WINAPI ClassMonikerROTData_AddRef(IROTData *iface)
{
    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        ClassMonikerIROTData_Release
 */
static ULONG WINAPI ClassMonikerROTData_Release(IROTData* iface)
{
    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return IMoniker_Release(&This->IMoniker_iface);
}

/******************************************************************************
 *        ClassMonikerIROTData_GetComparisonData
 ******************************************************************************/
static HRESULT WINAPI ClassMonikerROTData_GetComparisonData(IROTData* iface,
                                                         BYTE* pbData,
                                                         ULONG cbMax,
                                                         ULONG* pcbData)
{
    ClassMoniker *This = impl_from_IROTData(iface);

    TRACE("%p, %p, %lu, %p.\n", iface, pbData, cbMax, pcbData);

    *pcbData = 2*sizeof(CLSID);
    if (cbMax < *pcbData)
        return E_OUTOFMEMORY;

    /* write CLSID of the moniker */
    memcpy(pbData, &CLSID_ClassMoniker, sizeof(CLSID));
    /* write CLSID the moniker represents */
    memcpy(pbData+sizeof(CLSID), &This->header.clsid, sizeof(CLSID));

    return S_OK;
}

static const IMonikerVtbl ClassMonikerVtbl =
{
    ClassMoniker_QueryInterface,
    ClassMoniker_AddRef,
    ClassMoniker_Release,
    ClassMoniker_GetClassID,
    ClassMoniker_IsDirty,
    ClassMoniker_Load,
    ClassMoniker_Save,
    ClassMoniker_GetSizeMax,
    ClassMoniker_BindToObject,
    ClassMoniker_BindToStorage,
    ClassMoniker_Reduce,
    ClassMoniker_ComposeWith,
    ClassMoniker_Enum,
    ClassMoniker_IsEqual,
    ClassMoniker_Hash,
    ClassMoniker_IsRunning,
    ClassMoniker_GetTimeOfLastChange,
    ClassMoniker_Inverse,
    ClassMoniker_CommonPrefixWith,
    ClassMoniker_RelativePathTo,
    ClassMoniker_GetDisplayName,
    ClassMoniker_ParseDisplayName,
    ClassMoniker_IsSystemMoniker
};

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl ROTDataVtbl =
{
    ClassMonikerROTData_QueryInterface,
    ClassMonikerROTData_AddRef,
    ClassMonikerROTData_Release,
    ClassMonikerROTData_GetComparisonData
};

static HRESULT create_class_moniker(const CLSID *clsid, const WCHAR *data,
        unsigned int data_len, IMoniker **moniker)
{
    ClassMoniker *object;

    if (!(object = calloc(1, sizeof(*object))))
        return E_OUTOFMEMORY;

    object->IMoniker_iface.lpVtbl = &ClassMonikerVtbl;
    object->IROTData_iface.lpVtbl = &ROTDataVtbl;
    object->ref = 1;
    object->header.clsid = *clsid;
    if (data_len)
    {
        object->header.data_len = (data_len + 1) * sizeof(WCHAR);

        if (!(object->data = malloc(object->header.data_len)))
        {
            IMoniker_Release(&object->IMoniker_iface);
            return E_OUTOFMEMORY;
        }
        memcpy(object->data, data, data_len * sizeof(WCHAR));
        object->data[data_len] = 0;
    }

    *moniker = &object->IMoniker_iface;

    return S_OK;
}

/******************************************************************************
 *        CreateClassMoniker	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateClassMoniker(REFCLSID rclsid, IMoniker **moniker)
{
    TRACE("%s, %p\n", debugstr_guid(rclsid), moniker);

    return create_class_moniker(rclsid, NULL, 0, moniker);
}

HRESULT ClassMoniker_CreateFromDisplayName(LPBC pbc, const WCHAR *display_name,
        DWORD *eaten, IMoniker **moniker)
{
    const WCHAR *end, *s;
    BOOL has_braces;
    WCHAR uuid[37];
    CLSID clsid;
    HRESULT hr;
    int len;

    s = display_name;

    /* Skip prefix */
    if (wcsnicmp(s, L"clsid:", 6)) return MK_E_SYNTAX;
    s += 6;

    /* Terminating marker is optional */
    if (!(end = wcschr(s, ':')))
        end = s + lstrlenW(s);

    len = end - s;
    if (len < 36)
        return MK_E_SYNTAX;

    if ((has_braces = *s == '{')) s++;

    memcpy(uuid, s, 36 * sizeof(WCHAR));
    uuid[36] = 0;

    if (UuidFromStringW(uuid, &clsid))
    {
        WARN("Failed to parse clsid string.\n");
        return MK_E_SYNTAX;
    }

    s += 36;
    if (has_braces)
    {
        if (*s != '}') return MK_E_SYNTAX;
        s++;
    }

    /* Consume terminal marker */
    len = end - s;
    if (*end == ':') end++;

    hr = create_class_moniker(&clsid, len ? s : NULL, len, moniker);
    if (SUCCEEDED(hr))
        *eaten = end - display_name;
    return hr;
}

HRESULT WINAPI ClassMoniker_CreateInstance(IClassFactory *iface,
    IUnknown *pUnk, REFIID riid, void **ppv)
{
    HRESULT hr;
    IMoniker *pmk;

    TRACE("(%p, %s, %p)\n", pUnk, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (pUnk)
        return CLASS_E_NOAGGREGATION;

    hr = CreateClassMoniker(&CLSID_NULL, &pmk);
    if (FAILED(hr)) return hr;

    hr = IMoniker_QueryInterface(pmk, riid, ppv);
    IMoniker_Release(pmk);

    return hr;
}
