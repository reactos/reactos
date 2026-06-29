/*
 *	                      ItemMonikers implementation
 *
 *           Copyright 1999  Noomen Hamza
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
#include "winnls.h"
#include "wine/debug.h"
#include "ole2.h"
#include "moniker.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* ItemMoniker data structure */
typedef struct ItemMonikerImpl{
    IMoniker IMoniker_iface;  /* VTable relative to the IMoniker interface.*/
    IROTData IROTData_iface;  /* VTable relative to the IROTData interface.*/
    LONG ref;
    LPOLESTR itemName; /* item name identified by this ItemMoniker */
    LPOLESTR itemDelimiter; /* Delimiter string */
    IUnknown *pMarshal; /* custom marshaler */
} ItemMonikerImpl;

static inline ItemMonikerImpl *impl_from_IMoniker(IMoniker *iface)
{
    return CONTAINING_RECORD(iface, ItemMonikerImpl, IMoniker_iface);
}

static ItemMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface);

static inline ItemMonikerImpl *impl_from_IROTData(IROTData *iface)
{
    return CONTAINING_RECORD(iface, ItemMonikerImpl, IROTData_iface);
}

struct container_lock
{
    IUnknown IUnknown_iface;
    LONG refcount;
    IOleItemContainer *container;
};

static struct container_lock *impl_lock_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct container_lock, IUnknown_iface);
}

static HRESULT WINAPI container_lock_QueryInterface(IUnknown *iface, REFIID riid, void **obj)
{
    if (IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = iface;
        IUnknown_AddRef(iface);
        return S_OK;
    }

    WARN("Unsupported interface %s.\n", debugstr_guid(riid));
    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI container_lock_AddRef(IUnknown *iface)
{
    struct container_lock *lock = impl_lock_from_IUnknown(iface);
    return InterlockedIncrement(&lock->refcount);
}

static ULONG WINAPI container_lock_Release(IUnknown *iface)
{
    struct container_lock *lock = impl_lock_from_IUnknown(iface);
    ULONG refcount = InterlockedDecrement(&lock->refcount);

    if (!refcount)
    {
        IOleItemContainer_LockContainer(lock->container, FALSE);
        IOleItemContainer_Release(lock->container);
        free(lock);
    }

    return refcount;
}

static const IUnknownVtbl container_lock_vtbl =
{
    container_lock_QueryInterface,
    container_lock_AddRef,
    container_lock_Release,
};

static HRESULT set_container_lock(IOleItemContainer *container, IBindCtx *pbc)
{
    struct container_lock *lock;
    HRESULT hr;

    if (!(lock = malloc(sizeof(*lock))))
        return E_OUTOFMEMORY;

    if (FAILED(hr = IOleItemContainer_LockContainer(container, TRUE)))
    {
        free(lock);
        return hr;
    }

    lock->IUnknown_iface.lpVtbl = &container_lock_vtbl;
    lock->refcount = 1;
    lock->container = container;
    IOleItemContainer_AddRef(lock->container);

    hr = IBindCtx_RegisterObjectBound(pbc, &lock->IUnknown_iface);
    IUnknown_Release(&lock->IUnknown_iface);
    return hr;
}

/*******************************************************************************
 *        ItemMoniker_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_QueryInterface(IMoniker *iface, REFIID riid, void **ppvObject)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppvObject);

    if (!ppvObject)
        return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IPersist, riid) ||
        IsEqualIID(&IID_IPersistStream, riid) ||
        IsEqualIID(&IID_IMoniker, riid) ||
        IsEqualGUID(&CLSID_ItemMoniker, riid))
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
    else
    {
        *ppvObject = NULL;
        return E_NOINTERFACE;
    }

    IMoniker_AddRef(iface);
    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_AddRef
 ******************************************************************************/
static ULONG WINAPI ItemMonikerImpl_AddRef(IMoniker* iface)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/******************************************************************************
 *        ItemMoniker_Release
 ******************************************************************************/
static ULONG WINAPI ItemMonikerImpl_Release(IMoniker* iface)
{
    ItemMonikerImpl *moniker = impl_from_IMoniker(iface);
    ULONG refcount = InterlockedDecrement(&moniker->ref);

    TRACE("%p, refcount %lu.\n", iface, refcount);

    if (!refcount)
    {
        if (moniker->pMarshal) IUnknown_Release(moniker->pMarshal);
        free(moniker->itemName);
        free(moniker->itemDelimiter);
        free(moniker);
    }

    return refcount;
}

/******************************************************************************
 *        ItemMoniker_GetClassID
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_GetClassID(IMoniker* iface,CLSID *pClassID)
{
    TRACE("(%p,%p)\n",iface,pClassID);

    if (pClassID==NULL)
        return E_POINTER;

    *pClassID = CLSID_ItemMoniker;

    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_IsDirty
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_IsDirty(IMoniker* iface)
{
    /* Note that the OLE-provided implementations of the IPersistStream::IsDirty
       method in the OLE-provided moniker interfaces always return S_FALSE because
       their internal state never changes. */

    TRACE("(%p)\n",iface);

    return S_FALSE;
}

static HRESULT item_moniker_load_string_record(IStream *stream, WCHAR **ret)
{
    DWORD str_len, read_len, lenW, i;
    HRESULT hr = S_OK;
    char *buffer;
    WCHAR *str;

    IStream_Read(stream, &str_len, sizeof(str_len), &read_len);
    if (read_len != sizeof(str_len))
        return E_FAIL;

    if (!str_len)
    {
        free(*ret);
        *ret = NULL;
        return S_OK;
    }

    if (!(buffer = malloc(str_len)))
        return E_OUTOFMEMORY;

    IStream_Read(stream, buffer, str_len, &read_len);
    if (read_len != str_len)
    {
        free(buffer);
        return E_FAIL;
    }

    /* Skip ansi buffer, it must be null terminated. */
    i = 0;
    while (i < str_len && buffer[i])
        i++;

    if (buffer[i])
    {
        WARN("Expected null terminated ansi name.\n");
        hr = E_FAIL;
        goto end;
    }

    if (i < str_len - 1)
    {
        str_len -= i + 1;

        if (str_len % sizeof(WCHAR))
        {
            WARN("Unexpected Unicode name length %ld.\n", str_len);
            hr = E_FAIL;
            goto end;
        }

        str = malloc(str_len + sizeof(WCHAR));
        if (str)
        {
            memcpy(str, &buffer[i + 1], str_len);
            str[str_len / sizeof(WCHAR)] = 0;
        }
    }
    else
    {
        lenW = MultiByteToWideChar(CP_ACP, 0, buffer, -1, NULL, 0);
        str = malloc(lenW * sizeof(WCHAR));
        if (str)
            MultiByteToWideChar(CP_ACP, 0, buffer, -1, str, lenW);
    }

    if (str)
    {
        free(*ret);
        *ret = str;
    }
    else
        hr = E_OUTOFMEMORY;

end:
    free(buffer);

    return hr;
}

/******************************************************************************
 *        ItemMoniker_Load
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_Load(IMoniker *iface, IStream *stream)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    HRESULT hr;

    TRACE("(%p, %p)\n", iface, stream);

    /* Delimiter and name use the same record structure: 4 bytes byte-length field, followed by
       string data. Data starts with single byte null-terminated string, WCHAR non-terminated
       string optionally follows. Length of WCHAR string is determined as a difference between total
       byte-length and single byte string length. */

    hr = item_moniker_load_string_record(stream, &This->itemDelimiter);
    if (SUCCEEDED(hr))
        hr = item_moniker_load_string_record(stream, &This->itemName);

    return hr;
}

/******************************************************************************
 *        ItemMoniker_Save
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_Save(IMoniker *iface, IStream *stream, BOOL fClearDirty)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    int str_len;
    HRESULT hr;
    char *str;

    TRACE("(%p, %p, %d)\n", iface, stream, fClearDirty);

    /* data written by this function are : 1) DWORD : size of item delimiter string ('\0' included ) */
    /*                                    2) String (type A): item delimiter string ('\0' included)          */
    /*                                    3) DWORD : size of item name string ('\0' included)       */
    /*                                    4) String (type A): item name string ('\0' included)               */
    if (This->itemDelimiter)
    {
        str_len = WideCharToMultiByte(CP_ACP, 0, This->itemDelimiter, -1, NULL, 0, NULL, NULL);
        str = malloc(str_len);
        WideCharToMultiByte(CP_ACP, 0, This->itemDelimiter, -1, str, str_len, NULL, NULL);

        hr = IStream_Write(stream, &str_len, sizeof(str_len), NULL);
        hr = IStream_Write(stream, str, str_len, NULL);

        free(str);
    }
    else
    {
        str_len = 0;
        hr = IStream_Write(stream, &str_len, sizeof(str_len), NULL);
    }

    str_len = WideCharToMultiByte(CP_ACP, 0, This->itemName, -1, NULL, 0, NULL, NULL);
    str = malloc(str_len);
    WideCharToMultiByte(CP_ACP, 0, This->itemName, -1, str, str_len, NULL, NULL);
    hr = IStream_Write(stream, &str_len, sizeof(str_len), NULL);
    hr = IStream_Write(stream, str, str_len, NULL);
    free(str);

    return hr;
}

/******************************************************************************
 *        ItemMoniker_GetSizeMax
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_GetSizeMax(IMoniker* iface, ULARGE_INTEGER* pcbSize)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    DWORD nameLength=lstrlenW(This->itemName)+1;

    TRACE("(%p,%p)\n",iface,pcbSize);

    if (!pcbSize)
        return E_POINTER;

    /* for more details see ItemMonikerImpl_Save comments */

    pcbSize->u.LowPart =  sizeof(DWORD) + /* DWORD which contains delimiter length */
                        sizeof(DWORD) + /* DWORD which contains item name length */
                        nameLength*4 + /* item name string */
                        18; /* strange, but true */
    if (This->itemDelimiter)
        pcbSize->u.LowPart += (lstrlenW(This->itemDelimiter) + 1) * 4;

    pcbSize->u.HighPart=0;

    return S_OK;
}

static DWORD get_bind_speed_from_bindctx(IBindCtx *pbc)
{
    DWORD bind_speed = BINDSPEED_INDEFINITE;
    BIND_OPTS bind_opts;

    bind_opts.cbStruct = sizeof(bind_opts);
    if (SUCCEEDED(IBindCtx_GetBindOptions(pbc, &bind_opts)) && bind_opts.dwTickCountDeadline)
        bind_speed = bind_opts.dwTickCountDeadline < 2500 ? BINDSPEED_IMMEDIATE : BINDSPEED_MODERATE;

    return bind_speed;
}

/******************************************************************************
 *                  ItemMoniker_BindToObject
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_BindToObject(IMoniker* iface,
                                                   IBindCtx* pbc,
                                                   IMoniker* pmkToLeft,
                                                   REFIID riid,
                                                   VOID** ppvResult)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    IOleItemContainer *container;
    HRESULT hr;

    TRACE("(%p,%p,%p,%s,%p)\n",iface,pbc,pmkToLeft,debugstr_guid(riid),ppvResult);

    if(ppvResult ==NULL)
        return E_POINTER;

    if(pmkToLeft==NULL)
        return E_INVALIDARG;

    *ppvResult=0;

    hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IOleItemContainer, (void **)&container);
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = set_container_lock(container, pbc)))
            WARN("Failed to lock container, hr %#lx.\n", hr);

        hr = IOleItemContainer_GetObject(container, This->itemName, get_bind_speed_from_bindctx(pbc), pbc,
                riid, ppvResult);
        IOleItemContainer_Release(container);
    }

    return hr;
}

/******************************************************************************
 *        ItemMoniker_BindToStorage
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_BindToStorage(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft, REFIID riid,
        void **ppvResult)
{
    ItemMonikerImpl *moniker = impl_from_IMoniker(iface);
    IOleItemContainer *container;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p.\n", iface, pbc, pmkToLeft, debugstr_guid(riid), ppvResult);

    *ppvResult = 0;

    if (!pmkToLeft)
        return E_INVALIDARG;

    hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IOleItemContainer, (void **)&container);
    if (SUCCEEDED(hr))
    {
        if (FAILED(hr = set_container_lock(container, pbc)))
            WARN("Failed to lock container, hr %#lx.\n", hr);

        hr = IOleItemContainer_GetObjectStorage(container, moniker->itemName, pbc, riid, ppvResult);
        IOleItemContainer_Release(container);
    }

    return hr;
}

static HRESULT WINAPI ItemMonikerImpl_Reduce(IMoniker* iface, IBindCtx* pbc,
        DWORD dwReduceHowFar, IMoniker** ppmkToLeft, IMoniker** ppmkReduced)
{
    TRACE("%p, %p, %ld, %p, %p.\n", iface, pbc, dwReduceHowFar, ppmkToLeft, ppmkReduced);

    if (ppmkReduced==NULL)
        return E_POINTER;

    ItemMonikerImpl_AddRef(iface);

    *ppmkReduced=iface;

    return MK_S_REDUCED_TO_SELF;
}

static HRESULT WINAPI ItemMonikerImpl_ComposeWith(IMoniker *iface, IMoniker *right,
        BOOL only_if_not_generic, IMoniker **result)
{
    DWORD order;

    TRACE("%p, %p, %d, %p\n", iface, right, only_if_not_generic, result);

    if (!result || !right)
        return E_POINTER;

    *result = NULL;

    if (is_anti_moniker(right, &order))
        return order > 1 ? create_anti_moniker(order - 1, result) : S_OK;

    return only_if_not_generic ? MK_E_NEEDGENERIC : CreateGenericComposite(iface, right, result);
}

/******************************************************************************
 *        ItemMoniker_Enum
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_Enum(IMoniker* iface,BOOL fForward, IEnumMoniker** ppenumMoniker)
{
    TRACE("(%p,%d,%p)\n",iface,fForward,ppenumMoniker);

    if (ppenumMoniker == NULL)
        return E_POINTER;

    *ppenumMoniker = NULL;

    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_IsEqual
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_IsEqual(IMoniker *iface, IMoniker *other)
{
    ItemMonikerImpl *moniker = impl_from_IMoniker(iface), *other_moniker;

    TRACE("%p, %p.\n", iface, other);

    if (!other)
        return E_INVALIDARG;

    other_moniker = unsafe_impl_from_IMoniker(other);
    if (!other_moniker)
        return S_FALSE;

    return !wcsicmp(moniker->itemName, other_moniker->itemName) ? S_OK : S_FALSE;
}

/******************************************************************************
 *        ItemMoniker_Hash
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_Hash(IMoniker* iface,DWORD* pdwHash)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    DWORD h = 0;
    int  i,len;
    int  off = 0;
    LPOLESTR val;

    if (pdwHash==NULL)
        return E_POINTER;

    val =  This->itemName;
    len = lstrlenW(val);

    for (i = len ; i > 0; i--)
        h = (h * 3) ^ towupper(val[off++]);

    *pdwHash=h;

    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_IsRunning
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_IsRunning(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        IMoniker *pmkNewlyRunning)
{
    ItemMonikerImpl *moniker = impl_from_IMoniker(iface);
    IOleItemContainer *container;
    IRunningObjectTable* rot;
    HRESULT hr;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pmkNewlyRunning);

    if (!pbc)
        return E_INVALIDARG;

    if (!pmkToLeft)
    {
        if (pmkNewlyRunning)
        {
            return IMoniker_IsEqual(iface, pmkNewlyRunning);
        }
        else
        {
            hr = IBindCtx_GetRunningObjectTable(pbc, &rot);
            if (SUCCEEDED(hr))
            {
                hr = IRunningObjectTable_IsRunning(rot, iface);
                IRunningObjectTable_Release(rot);
            }
        }
    }
    else
    {
        /* Container itself must be running too. */
        hr = IMoniker_IsRunning(pmkToLeft, pbc, NULL, NULL);
        if (hr != S_OK)
            return hr;

        hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IOleItemContainer, (void **)&container);
        if (SUCCEEDED(hr))
        {
            hr = IOleItemContainer_IsRunning(container, moniker->itemName);
            IOleItemContainer_Release(container);
        }
    }

    return hr;
}

/******************************************************************************
 *        ItemMoniker_GetTimeOfLastChange
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_GetTimeOfLastChange(IMoniker* iface,
                                                          IBindCtx* pbc,
                                                          IMoniker* pmkToLeft,
                                                          FILETIME* pItemTime)
{
    IRunningObjectTable* rot;
    HRESULT res;
    IMoniker *compositeMk;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,pItemTime);

    if (pItemTime==NULL)
        return E_INVALIDARG;

    /* If pmkToLeft is NULL, this method returns MK_E_NOTBINDABLE */
    if (pmkToLeft==NULL)

        return MK_E_NOTBINDABLE;
    else {

        /* Otherwise, the method creates a composite of pmkToLeft and this moniker and uses the ROT to access  */
        /* the time of last change. If the object is not in the ROT, the method calls                          */
        /* IMoniker::GetTimeOfLastChange on the pmkToLeft parameter.                                            */

        res=CreateGenericComposite(pmkToLeft,iface,&compositeMk);
        if (FAILED(res))
            return res;

        res=IBindCtx_GetRunningObjectTable(pbc,&rot);
        if (FAILED(res)) {
            IMoniker_Release(compositeMk);
            return res;
        }

        if (IRunningObjectTable_GetTimeOfLastChange(rot,compositeMk,pItemTime)!=S_OK)

            res=IMoniker_GetTimeOfLastChange(pmkToLeft,pbc,NULL,pItemTime);

        IMoniker_Release(compositeMk);
    }

    return res;
}

/******************************************************************************
 *        ItemMoniker_Inverse
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_Inverse(IMoniker* iface,IMoniker** ppmk)
{
    TRACE("(%p,%p)\n",iface,ppmk);

    if (ppmk==NULL)
        return E_POINTER;

    return CreateAntiMoniker(ppmk);
}

static HRESULT WINAPI ItemMonikerImpl_CommonPrefixWith(IMoniker *iface, IMoniker *other,
        IMoniker **prefix)
{
    TRACE("%p, %p, %p\n", iface, other, prefix);

    if (IMoniker_IsEqual(iface, other) == S_OK)
    {
        *prefix = iface;
        IMoniker_AddRef(iface);
        return MK_S_US;
    }

    return MonikerCommonPrefixWith(iface, other, prefix);
}

static HRESULT WINAPI ItemMonikerImpl_RelativePathTo(IMoniker *iface, IMoniker *other, IMoniker **result)
{
    TRACE("%p, %p, %p.\n", iface, other, result);

    if (!other || !result)
        return E_INVALIDARG;

    *result = NULL;

    return MK_E_NOTBINDABLE;
}

/******************************************************************************
 *        ItemMoniker_GetDisplayName
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_GetDisplayName(IMoniker* iface,
                                                     IBindCtx* pbc,
                                                     IMoniker* pmkToLeft,
                                                     LPOLESTR *ppszDisplayName)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    SIZE_T size;

    TRACE("(%p,%p,%p,%p)\n",iface,pbc,pmkToLeft,ppszDisplayName);

    if (ppszDisplayName==NULL)
        return E_POINTER;

    if (pmkToLeft!=NULL){
        return E_INVALIDARG;
    }

    size = lstrlenW(This->itemName) + 1;
    if (This->itemDelimiter)
        size += lstrlenW(This->itemDelimiter);
    size *= sizeof(WCHAR);

    *ppszDisplayName = CoTaskMemAlloc(size);
    if (*ppszDisplayName==NULL)
        return E_OUTOFMEMORY;

    (*ppszDisplayName)[0] = 0;
    if (This->itemDelimiter)
        lstrcatW(*ppszDisplayName, This->itemDelimiter);
    lstrcatW(*ppszDisplayName,This->itemName);

    TRACE("-- %s\n", debugstr_w(*ppszDisplayName));

    return S_OK;
}

/******************************************************************************
 *        ItemMoniker_ParseDisplayName
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_ParseDisplayName(IMoniker *iface, IBindCtx *pbc, IMoniker *pmkToLeft,
        LPOLESTR displayname, ULONG *eaten, IMoniker **ppmkOut)
{
    ItemMonikerImpl *This = impl_from_IMoniker(iface);
    IOleItemContainer *container;
    IParseDisplayName *parser;
    HRESULT hr;

    TRACE("%p, %p, %p, %s, %p, %p.\n", iface, pbc, pmkToLeft, debugstr_w(displayname), eaten, ppmkOut);

    if (!pmkToLeft)
        return MK_E_SYNTAX;

    hr = IMoniker_BindToObject(pmkToLeft, pbc, NULL, &IID_IOleItemContainer, (void **)&container);
    if (SUCCEEDED(hr))
    {
        if (SUCCEEDED(hr = set_container_lock(container, pbc)))
        {
            hr = IOleItemContainer_GetObject(container, This->itemName, get_bind_speed_from_bindctx(pbc), pbc,
                    &IID_IParseDisplayName, (void **)&parser);
            if (SUCCEEDED(hr))
            {
                hr = IParseDisplayName_ParseDisplayName(parser, pbc, displayname, eaten, ppmkOut);
                IParseDisplayName_Release(parser);
            }
        }
        IOleItemContainer_Release(container);
    }

    return hr;
}

/******************************************************************************
 *        ItemMoniker_IsSystemMoniker
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerImpl_IsSystemMoniker(IMoniker* iface,DWORD* pwdMksys)
{
    TRACE("(%p,%p)\n",iface,pwdMksys);

    if (!pwdMksys)
        return E_POINTER;

    (*pwdMksys)=MKSYS_ITEMMONIKER;

    return S_OK;
}

/*******************************************************************************
 *        ItemMonikerIROTData_QueryInterface
 *******************************************************************************/
static HRESULT WINAPI ItemMonikerROTDataImpl_QueryInterface(IROTData *iface,REFIID riid,
                                                            void **ppvObject)
{

    ItemMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p,%s,%p)\n",iface,debugstr_guid(riid),ppvObject);

    return ItemMonikerImpl_QueryInterface(&This->IMoniker_iface, riid, ppvObject);
}

/***********************************************************************
 *        ItemMonikerIROTData_AddRef
 */
static ULONG   WINAPI ItemMonikerROTDataImpl_AddRef(IROTData *iface)
{
    ItemMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return ItemMonikerImpl_AddRef(&This->IMoniker_iface);
}

/***********************************************************************
 *        ItemMonikerIROTData_Release
 */
static ULONG   WINAPI ItemMonikerROTDataImpl_Release(IROTData* iface)
{
    ItemMonikerImpl *This = impl_from_IROTData(iface);

    TRACE("(%p)\n",iface);

    return ItemMonikerImpl_Release(&This->IMoniker_iface);
}

/******************************************************************************
 *        ItemMonikerIROTData_GetComparisonData
 ******************************************************************************/
static HRESULT WINAPI ItemMonikerROTDataImpl_GetComparisonData(IROTData *iface, BYTE *buffer, ULONG max_len,
        ULONG *data_len)
{
    ItemMonikerImpl *This = impl_from_IROTData(iface);
    int name_len = lstrlenW(This->itemName);
    int delim_len, i;
    WCHAR *ptrW;

    TRACE("%p, %p, %lu, %p.\n", iface, buffer, max_len, data_len);

    delim_len = This->itemDelimiter && This->itemDelimiter[0] ? lstrlenW(This->itemDelimiter) : 0;
    *data_len = sizeof(CLSID) + sizeof(WCHAR) + (delim_len + name_len) * sizeof(WCHAR);
    if (max_len < *data_len)
        return E_OUTOFMEMORY;

    /* write CLSID */
    memcpy(buffer, &CLSID_ItemMoniker, sizeof(CLSID));
    buffer += sizeof(CLSID);

    /* write delimiter */
    for (i = 0, ptrW = (WCHAR *)buffer; i < delim_len; ++i)
        ptrW[i] = towupper(This->itemDelimiter[i]);
    buffer += (delim_len * sizeof(WCHAR));

    /* write name */
    for (i = 0, ptrW = (WCHAR *)buffer; i < name_len; ++i)
        ptrW[i] = towupper(This->itemName[i]);
    ptrW[i] = 0;

    return S_OK;
}

/********************************************************************************/
/* Virtual function table for the ItemMonikerImpl class which  include IPersist,*/
/* IPersistStream and IMoniker functions.                                       */
static const IMonikerVtbl VT_ItemMonikerImpl =
    {
    ItemMonikerImpl_QueryInterface,
    ItemMonikerImpl_AddRef,
    ItemMonikerImpl_Release,
    ItemMonikerImpl_GetClassID,
    ItemMonikerImpl_IsDirty,
    ItemMonikerImpl_Load,
    ItemMonikerImpl_Save,
    ItemMonikerImpl_GetSizeMax,
    ItemMonikerImpl_BindToObject,
    ItemMonikerImpl_BindToStorage,
    ItemMonikerImpl_Reduce,
    ItemMonikerImpl_ComposeWith,
    ItemMonikerImpl_Enum,
    ItemMonikerImpl_IsEqual,
    ItemMonikerImpl_Hash,
    ItemMonikerImpl_IsRunning,
    ItemMonikerImpl_GetTimeOfLastChange,
    ItemMonikerImpl_Inverse,
    ItemMonikerImpl_CommonPrefixWith,
    ItemMonikerImpl_RelativePathTo,
    ItemMonikerImpl_GetDisplayName,
    ItemMonikerImpl_ParseDisplayName,
    ItemMonikerImpl_IsSystemMoniker
};

static ItemMonikerImpl *unsafe_impl_from_IMoniker(IMoniker *iface)
{
    if (iface->lpVtbl != &VT_ItemMonikerImpl)
        return NULL;
    return CONTAINING_RECORD(iface, ItemMonikerImpl, IMoniker_iface);
}

/********************************************************************************/
/* Virtual function table for the IROTData class.                               */
static const IROTDataVtbl VT_ROTDataImpl =
{
    ItemMonikerROTDataImpl_QueryInterface,
    ItemMonikerROTDataImpl_AddRef,
    ItemMonikerROTDataImpl_Release,
    ItemMonikerROTDataImpl_GetComparisonData
};

/******************************************************************************
 *        CreateItemMoniker	[OLE32.@]
 ******************************************************************************/
HRESULT WINAPI CreateItemMoniker(const WCHAR *delimiter, const WCHAR *name, IMoniker **ret)
{
    ItemMonikerImpl *moniker;
    int str_len;
    HRESULT hr;

    TRACE("%s, %s, %p.\n", debugstr_w(delimiter), debugstr_w(name), ret);

    if (!(moniker = calloc(1, sizeof(*moniker))))
        return E_OUTOFMEMORY;

    moniker->IMoniker_iface.lpVtbl = &VT_ItemMonikerImpl;
    moniker->IROTData_iface.lpVtbl = &VT_ROTDataImpl;
    moniker->ref = 1;

    str_len = (lstrlenW(name) + 1) * sizeof(WCHAR);
    moniker->itemName = malloc(str_len);
    if (!moniker->itemName)
    {
        hr = E_OUTOFMEMORY;
        goto failed;
    }
    memcpy(moniker->itemName, name, str_len);

    if (delimiter)
    {
        str_len = (lstrlenW(delimiter) + 1) * sizeof(WCHAR);
        moniker->itemDelimiter = malloc(str_len);
        if (!moniker->itemDelimiter)
        {
            hr = E_OUTOFMEMORY;
            goto failed;
        }
        memcpy(moniker->itemDelimiter, delimiter, str_len);
    }

    *ret = &moniker->IMoniker_iface;

    return S_OK;

failed:
    IMoniker_Release(&moniker->IMoniker_iface);

    return hr;
}

HRESULT WINAPI ItemMoniker_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    IMoniker *moniker;
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", outer, debugstr_guid(riid), ppv);

    *ppv = NULL;

    if (outer)
        return CLASS_E_NOAGGREGATION;

    if (FAILED(hr = CreateItemMoniker(L"", L"", &moniker)))
        return hr;

    hr = IMoniker_QueryInterface(moniker, riid, ppv);
    IMoniker_Release(moniker);

    return hr;
}
