/*
 *	Monikers
 *
 *	Copyright 1998	Marcus Meissner
 *      Copyright 1999  Noomen Hamza
 *      Copyright 2005  Robert Shearman (for CodeWeavers)
 *      Copyright 2007  Robert Shearman
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

#include "winerror.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winsvc.h"
#include "wtypes.h"
#include "ole2.h"

#include "wine/list.h"
#include "wine/debug.h"
#include "wine/exception.h"

#include "compobj_private.h"
#include "moniker.h"
#include "irot.h"
#include "irpcss.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/* see MSDN docs for IROTData::GetComparisonData, which states what this
 * constant is
 */
#define MAX_COMPARISON_DATA 2048

static LONG WINAPI rpc_filter(EXCEPTION_POINTERS *eptr)
{
    return I_RpcExceptionFilter(eptr->ExceptionRecord->ExceptionCode);
}

/* define the structure of the running object table elements */
struct rot_entry
{
    struct list        entry;
    InterfaceData* object; /* marshaled running object*/
    MonikerComparisonData* moniker_data; /* moniker comparison data that identifies this object */
    DWORD              cookie; /* cookie identifying this object */
    FILETIME           last_modified;
    IrotContextHandle  ctxt_handle;
};

/* define the RunningObjectTableImpl structure */
typedef struct RunningObjectTableImpl
{
    IRunningObjectTable IRunningObjectTable_iface;
    LONG ref;

    struct list rot; /* list of ROT entries */
    CRITICAL_SECTION lock;
} RunningObjectTableImpl;

static RunningObjectTableImpl* runningObjectTableInstance = NULL;
static IrotHandle irot_handle;
static RPC_BINDING_HANDLE irpcss_handle;

/* define the EnumMonikerImpl structure */
typedef struct EnumMonikerImpl
{
    IEnumMoniker IEnumMoniker_iface;
    LONG ref;

    InterfaceList *moniker_list;
    ULONG pos;
} EnumMonikerImpl;

static inline RunningObjectTableImpl *impl_from_IRunningObjectTable(IRunningObjectTable *iface)
{
    return CONTAINING_RECORD(iface, RunningObjectTableImpl, IRunningObjectTable_iface);
}

static inline EnumMonikerImpl *impl_from_IEnumMoniker(IEnumMoniker *iface)
{
    return CONTAINING_RECORD(iface, EnumMonikerImpl, IEnumMoniker_iface);
}

/* IEnumMoniker Local functions*/
static HRESULT EnumMonikerImpl_CreateEnumROTMoniker(InterfaceList *moniker_list,
    ULONG pos, IEnumMoniker **ppenumMoniker);

static RPC_BINDING_HANDLE get_rpc_handle(unsigned short *protseq, unsigned short *endpoint)
{
    RPC_BINDING_HANDLE handle = NULL;
    RPC_STATUS status;
    RPC_WSTR binding;

    status = RpcStringBindingComposeW(NULL, protseq, NULL, endpoint, NULL, &binding);
    if (status == RPC_S_OK)
    {
        status = RpcBindingFromStringBindingW(binding, &handle);
        RpcStringFreeW(&binding);
    }

    return handle;
}

static IrotHandle get_irot_handle(void)
{
    if (!irot_handle)
    {
        unsigned short protseq[] = IROT_PROTSEQ;
        unsigned short endpoint[] = IROT_ENDPOINT;

        IrotHandle new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irot_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irot_handle;
}

static RPC_BINDING_HANDLE get_irpcss_handle(void)
{
    if (!irpcss_handle)
    {
        unsigned short protseq[] = IROT_PROTSEQ;
        unsigned short endpoint[] = IROT_ENDPOINT;

        RPC_BINDING_HANDLE new_handle = get_rpc_handle(protseq, endpoint);
        if (InterlockedCompareExchangePointer(&irpcss_handle, new_handle, NULL))
            /* another thread beat us to it */
            RpcBindingFree(&new_handle);
    }
    return irpcss_handle;
}

static BOOL start_rpcss(void)
{
    static const WCHAR rpcssW[] = {'R','p','c','S','s',0};
    SC_HANDLE scm, service;
    SERVICE_STATUS_PROCESS status;
    BOOL ret = FALSE;

    TRACE("\n");

    if (!(scm = OpenSCManagerW( NULL, NULL, 0 )))
    {
        ERR( "failed to open service manager\n" );
        return FALSE;
    }
    if (!(service = OpenServiceW( scm, rpcssW, SERVICE_START | SERVICE_QUERY_STATUS )))
    {
        ERR( "failed to open RpcSs service\n" );
        CloseServiceHandle( scm );
        return FALSE;
    }
    if (StartServiceW( service, 0, NULL ) || GetLastError() == ERROR_SERVICE_ALREADY_RUNNING)
    {
        ULONGLONG start_time = GetTickCount64();
        do
        {
            DWORD dummy;

            if (!QueryServiceStatusEx( service, SC_STATUS_PROCESS_INFO,
                                       (BYTE *)&status, sizeof(status), &dummy ))
                break;
            if (status.dwCurrentState == SERVICE_RUNNING)
            {
                ret = TRUE;
                break;
            }
            if (GetTickCount64() - start_time > 30000) break;
            Sleep( 100 );

        } while (status.dwCurrentState == SERVICE_START_PENDING);

        if (status.dwCurrentState != SERVICE_RUNNING)
            WARN( "RpcSs failed to start %u\n", status.dwCurrentState );
    }
    else ERR( "failed to start RpcSs service\n" );

    CloseServiceHandle( service );
    CloseServiceHandle( scm );
    return ret;
}

HRESULT __cdecl irpcss_get_thread_seq_id(handle_t h, DWORD *id)
{
    static LONG thread_seq_id;
    *id = InterlockedIncrement(&thread_seq_id);
    return S_OK;
}

DWORD rpcss_get_next_seqid(void)
{
    DWORD id = 0;
    HRESULT hr;

    for (;;)
    {
        __TRY
        {
            hr = irpcss_get_thread_seq_id(get_irpcss_handle(), &id);
        }
        __EXCEPT(rpc_filter)
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
        }
        __ENDTRY
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        {
            if (start_rpcss())
                continue;
        }
        break;
    }

    return id;
}

static HRESULT create_stream_on_mip_ro(const InterfaceData *mip, IStream **stream)
{
    HGLOBAL hglobal = GlobalAlloc(0, mip->ulCntData);
    void *pv = GlobalLock(hglobal);
    memcpy(pv, mip->abData, mip->ulCntData);
    GlobalUnlock(hglobal);
    return CreateStreamOnHGlobal(hglobal, TRUE, stream);
}

static void rot_entry_delete(struct rot_entry *rot_entry)
{
    if (rot_entry->cookie)
    {
        InterfaceData *object = NULL;
        InterfaceData *moniker = NULL;
        __TRY
        {
            IrotRevoke(get_irot_handle(), rot_entry->cookie,
                       &rot_entry->ctxt_handle, &object, &moniker);
        }
        __EXCEPT(rpc_filter)
        {
        }
        __ENDTRY
        MIDL_user_free(object);
        if (moniker)
        {
            IStream *stream;
            HRESULT hr;
            hr = create_stream_on_mip_ro(moniker, &stream);
            if (hr == S_OK)
            {
                CoReleaseMarshalData(stream);
                IStream_Release(stream);
            }
        }
        MIDL_user_free(moniker);
    }
    if (rot_entry->object)
    {
        IStream *stream;
        HRESULT hr;
        hr = create_stream_on_mip_ro(rot_entry->object, &stream);
        if (hr == S_OK)
        {
            CoReleaseMarshalData(stream);
            IStream_Release(stream);
        }
    }
    HeapFree(GetProcessHeap(), 0, rot_entry->object);
    HeapFree(GetProcessHeap(), 0, rot_entry->moniker_data);
    HeapFree(GetProcessHeap(), 0, rot_entry);
}

/* moniker_data must be freed with HeapFree when no longer in use */
static HRESULT get_moniker_comparison_data(IMoniker *pMoniker, MonikerComparisonData **moniker_data)
{
    HRESULT hr;
    IROTData *pROTData = NULL;
    hr = IMoniker_QueryInterface(pMoniker, &IID_IROTData, (void *)&pROTData);
    if (SUCCEEDED(hr))
    {
        ULONG size = MAX_COMPARISON_DATA;
        *moniker_data = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MonikerComparisonData, abData[size]));
        if (!*moniker_data)
        {
            IROTData_Release(pROTData);
            return E_OUTOFMEMORY;
        }
        hr = IROTData_GetComparisonData(pROTData, (*moniker_data)->abData, size, &size);
        IROTData_Release(pROTData);
        if (hr != S_OK)
        {
            ERR("Failed to copy comparison data into buffer, hr = 0x%08x\n", hr);
            HeapFree(GetProcessHeap(), 0, *moniker_data);
            return hr;
        }
        (*moniker_data)->ulCntData = size;
    }
    else
    {
        IBindCtx *pbc;
        LPOLESTR pszDisplayName;
        CLSID clsid;
        int len;

        TRACE("generating comparison data from display name\n");

        hr = CreateBindCtx(0, &pbc);
        if (FAILED(hr))
            return hr;
        hr = IMoniker_GetDisplayName(pMoniker, pbc, NULL, &pszDisplayName);
        IBindCtx_Release(pbc);
        if (FAILED(hr))
            return hr;
        hr = IMoniker_GetClassID(pMoniker, &clsid);
        if (FAILED(hr))
        {
            CoTaskMemFree(pszDisplayName);
            return hr;
        }

        len = lstrlenW(pszDisplayName);
        *moniker_data = HeapAlloc(GetProcessHeap(), 0,
            FIELD_OFFSET(MonikerComparisonData, abData[sizeof(CLSID) + (len+1)*sizeof(WCHAR)]));
        if (!*moniker_data)
        {
            CoTaskMemFree(pszDisplayName);
            return E_OUTOFMEMORY;
        }
        (*moniker_data)->ulCntData = sizeof(CLSID) + (len+1)*sizeof(WCHAR);

        memcpy(&(*moniker_data)->abData[0], &clsid, sizeof(clsid));
        memcpy(&(*moniker_data)->abData[sizeof(clsid)], pszDisplayName, (len+1)*sizeof(WCHAR));
        CoTaskMemFree(pszDisplayName);
    }
    return S_OK;
}

static HRESULT reduce_moniker(IMoniker *pmk, IBindCtx *pbc, IMoniker **pmkReduced)
{
    IBindCtx *pbcNew = NULL;
    HRESULT hr;
    if (!pbc)
    {
        hr = CreateBindCtx(0, &pbcNew);
        if (FAILED(hr))
            return hr;
        pbc = pbcNew;
    }
    hr = IMoniker_Reduce(pmk, pbc, MKRREDUCE_ALL, NULL, pmkReduced);
    if (FAILED(hr))
        ERR("reducing moniker failed with error 0x%08x\n", hr);
    if (pbcNew) IBindCtx_Release(pbcNew);
    return hr;
}

/***********************************************************************
 *        RunningObjectTable_QueryInterface
 */
static HRESULT WINAPI
RunningObjectTableImpl_QueryInterface(IRunningObjectTable* iface,
                                      REFIID riid,void** ppvObject)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* validate arguments */

    if (ppvObject==0)
        return E_INVALIDARG;

    *ppvObject = 0;

    if (IsEqualIID(&IID_IUnknown, riid) ||
        IsEqualIID(&IID_IRunningObjectTable, riid))
        *ppvObject = &This->IRunningObjectTable_iface;

    if ((*ppvObject)==0)
        return E_NOINTERFACE;

    IRunningObjectTable_AddRef(iface);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_AddRef
 */
static ULONG WINAPI
RunningObjectTableImpl_AddRef(IRunningObjectTable* iface)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 *        RunningObjectTable_Destroy
 */
static HRESULT
RunningObjectTableImpl_Destroy(void)
{
    struct list *cursor, *cursor2;
    IrotHandle old_handle;

    TRACE("()\n");

    if (runningObjectTableInstance==NULL)
        return E_INVALIDARG;

    /* free the ROT table memory */
    LIST_FOR_EACH_SAFE(cursor, cursor2, &runningObjectTableInstance->rot)
    {
        struct rot_entry *rot_entry = LIST_ENTRY(cursor, struct rot_entry, entry);
        list_remove(&rot_entry->entry);
        rot_entry_delete(rot_entry);
    }

    DEBUG_CLEAR_CRITSEC_NAME(&runningObjectTableInstance->lock);
    DeleteCriticalSection(&runningObjectTableInstance->lock);

    /* free the ROT structure memory */
    HeapFree(GetProcessHeap(),0,runningObjectTableInstance);
    runningObjectTableInstance = NULL;

    old_handle = irot_handle;
    irot_handle = NULL;
    if (old_handle)
        RpcBindingFree(&old_handle);

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_Release
 */
static ULONG WINAPI
RunningObjectTableImpl_Release(IRunningObjectTable* iface)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* uninitialize ROT structure if there are no more references to it */
    if (ref == 0)
    {
        struct list *cursor, *cursor2;
        LIST_FOR_EACH_SAFE(cursor, cursor2, &This->rot)
        {
            struct rot_entry *rot_entry = LIST_ENTRY(cursor, struct rot_entry, entry);
            list_remove(&rot_entry->entry);
            rot_entry_delete(rot_entry);
        }
        /*  RunningObjectTable data structure will be not destroyed here ! the destruction will be done only
         *  when RunningObjectTableImpl_UnInitialize function is called
         */
    }

    return ref;
}

/***********************************************************************
 *        RunningObjectTable_Register
 *
 * PARAMS
 * grfFlags       [in] Registration options 
 * punkObject     [in] the object being registered
 * pmkObjectName  [in] the moniker of the object being registered
 * pdwRegister    [out] the value identifying the registration
 */
static HRESULT WINAPI
RunningObjectTableImpl_Register(IRunningObjectTable* iface, DWORD grfFlags,
               IUnknown *punkObject, IMoniker *pmkObjectName, DWORD *pdwRegister)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    struct rot_entry *rot_entry;
    HRESULT hr = S_OK;
    IStream *pStream = NULL;
    DWORD mshlflags;
    IBindCtx *pbc;
    InterfaceData *moniker = NULL;

    TRACE("(%p,%d,%p,%p,%p)\n",This,grfFlags,punkObject,pmkObjectName,pdwRegister);

    if (grfFlags & ~(ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT))
    {
        ERR("Invalid grfFlags: 0x%08x\n", grfFlags & ~(ROTFLAGS_REGISTRATIONKEEPSALIVE|ROTFLAGS_ALLOWANYCLIENT));
        return E_INVALIDARG;
    }

    if (punkObject==NULL || pmkObjectName==NULL || pdwRegister==NULL)
        return E_INVALIDARG;

    rot_entry = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*rot_entry));
    if (!rot_entry)
        return E_OUTOFMEMORY;

    /* marshal object */
    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }
    mshlflags = (grfFlags & ROTFLAGS_REGISTRATIONKEEPSALIVE) ? MSHLFLAGS_TABLESTRONG : MSHLFLAGS_TABLEWEAK;
    hr = CoMarshalInterface(pStream, &IID_IUnknown, punkObject, MSHCTX_LOCAL | MSHCTX_NOSHAREDMEM, NULL, mshlflags);
    /* FIXME: a cleaner way would be to create an IStream class that writes
     * directly to an MInterfacePointer */
    if (hr == S_OK)
    {
        HGLOBAL hglobal;
        hr = GetHGlobalFromStream(pStream, &hglobal);
        if (hr == S_OK)
        {
            SIZE_T size = GlobalSize(hglobal);
            const void *pv = GlobalLock(hglobal);
            rot_entry->object = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(MInterfacePointer, abData[size]));
            rot_entry->object->ulCntData = size;
            memcpy(rot_entry->object->abData, pv, size);
            GlobalUnlock(hglobal);
        }
    }
    IStream_Release(pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    hr = CreateBindCtx(0, &pbc);
    if (FAILED(hr))
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    hr = reduce_moniker(pmkObjectName, pbc, &pmkObjectName);
    if (FAILED(hr))
    {
        rot_entry_delete(rot_entry);
        IBindCtx_Release(pbc);
        return hr;
    }

    hr = IMoniker_GetTimeOfLastChange(pmkObjectName, pbc, NULL,
                                      &rot_entry->last_modified);
    IBindCtx_Release(pbc);
    if (FAILED(hr))
    {
        CoFileTimeNow(&rot_entry->last_modified);
        hr = S_OK;
    }

    hr = get_moniker_comparison_data(pmkObjectName,
                                     &rot_entry->moniker_data);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        IMoniker_Release(pmkObjectName);
        return hr;
    }

    hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
    if (hr != S_OK)
    {
        rot_entry_delete(rot_entry);
        IMoniker_Release(pmkObjectName);
        return hr;
    }
    /* marshal moniker */
    hr = CoMarshalInterface(pStream, &IID_IMoniker, (IUnknown *)pmkObjectName,
                            MSHCTX_LOCAL | MSHCTX_NOSHAREDMEM, NULL, MSHLFLAGS_TABLESTRONG);
    /* FIXME: a cleaner way would be to create an IStream class that writes
     * directly to an MInterfacePointer */
    if (hr == S_OK)
    {
        HGLOBAL hglobal;
        hr = GetHGlobalFromStream(pStream, &hglobal);
        if (hr == S_OK)
        {
            SIZE_T size = GlobalSize(hglobal);
            const void *pv = GlobalLock(hglobal);
            moniker = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(InterfaceData, abData[size]));
            moniker->ulCntData = size;
            memcpy(moniker->abData, pv, size);
            GlobalUnlock(hglobal);
        }
    }
    IStream_Release(pStream);
    IMoniker_Release(pmkObjectName);
    if (hr != S_OK)
    {
        HeapFree(GetProcessHeap(), 0, moniker);
        rot_entry_delete(rot_entry);
        return hr;
    }


    while (TRUE)
    {
        __TRY
        {
            hr = IrotRegister(get_irot_handle(), rot_entry->moniker_data,
                              rot_entry->object, moniker,
                              &rot_entry->last_modified, grfFlags,
                              &rot_entry->cookie, &rot_entry->ctxt_handle);
        }
        __EXCEPT(rpc_filter)
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
        }
        __ENDTRY
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        {
            if (start_rpcss())
                continue;
        }
        break;
    }
    HeapFree(GetProcessHeap(), 0, moniker);
    if (FAILED(hr))
    {
        rot_entry_delete(rot_entry);
        return hr;
    }

    /* gives a registration identifier to the registered object*/
    *pdwRegister = rot_entry->cookie;

    EnterCriticalSection(&This->lock);
    list_add_tail(&This->rot, &rot_entry->entry);
    LeaveCriticalSection(&This->lock);

    return hr;
}

/***********************************************************************
 *        RunningObjectTable_Revoke
 *
 * PARAMS
 *  dwRegister [in] Value identifying registration to be revoked
 */
static HRESULT WINAPI
RunningObjectTableImpl_Revoke( IRunningObjectTable* iface, DWORD dwRegister) 
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    struct rot_entry *rot_entry;

    TRACE("(%p,%d)\n",This,dwRegister);

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if (rot_entry->cookie == dwRegister)
        {
            list_remove(&rot_entry->entry);
            LeaveCriticalSection(&This->lock);

            rot_entry_delete(rot_entry);
            return S_OK;
        }
    }
    LeaveCriticalSection(&This->lock);

    return E_INVALIDARG;
}

/***********************************************************************
 *        RunningObjectTable_IsRunning
 *
 * PARAMS
 *  pmkObjectName [in]  moniker of the object whose status is desired 
 */
static HRESULT WINAPI
RunningObjectTableImpl_IsRunning( IRunningObjectTable* iface, IMoniker *pmkObjectName)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    MonikerComparisonData *moniker_data;
    HRESULT hr;
    const struct rot_entry *rot_entry;

    TRACE("(%p,%p)\n",This,pmkObjectName);

    hr = reduce_moniker(pmkObjectName, NULL, &pmkObjectName);
    if (FAILED(hr))
        return hr;
    hr = get_moniker_comparison_data(pmkObjectName, &moniker_data);
    IMoniker_Release(pmkObjectName);
    if (hr != S_OK)
        return hr;

    hr = S_FALSE;
    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, const struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(moniker_data->abData, rot_entry->moniker_data->abData, moniker_data->ulCntData))
        {
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&This->lock);

    if (hr == S_FALSE)
    {
        while (TRUE)
        {
            __TRY
            {
                hr = IrotIsRunning(get_irot_handle(), moniker_data);
            }
            __EXCEPT(rpc_filter)
            {
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
            }
            __ENDTRY
            if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
            {
                if (start_rpcss())
                    continue;
            }
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, moniker_data);

    return hr;
}

/***********************************************************************
 *        RunningObjectTable_GetObject
 *
 * PARAMS
 * pmkObjectName [in] Pointer to the moniker on the object 
 * ppunkObject   [out] variable that receives the IUnknown interface pointer
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetObject( IRunningObjectTable* iface,
                     IMoniker *pmkObjectName, IUnknown **ppunkObject)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    MonikerComparisonData *moniker_data;
    InterfaceData *object = NULL;
    IrotCookie cookie;
    HRESULT hr;
    struct rot_entry *rot_entry;

    TRACE("(%p,%p,%p)\n",This,pmkObjectName,ppunkObject);

    if (ppunkObject == NULL)
        return E_POINTER;

    *ppunkObject = NULL;

    hr = reduce_moniker(pmkObjectName, NULL, &pmkObjectName);
    if (FAILED(hr))
        return hr;
    hr = get_moniker_comparison_data(pmkObjectName, &moniker_data);
    IMoniker_Release(pmkObjectName);
    if (hr != S_OK)
        return hr;

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(moniker_data->abData, rot_entry->moniker_data->abData, moniker_data->ulCntData))
        {
            IStream *pStream;
            hr = create_stream_on_mip_ro(rot_entry->object, &pStream);
            if (hr == S_OK)
            {
                hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)ppunkObject);
                IStream_Release(pStream);
            }

            LeaveCriticalSection(&This->lock);
            HeapFree(GetProcessHeap(), 0, moniker_data);

            return hr;
        }
    }
    LeaveCriticalSection(&This->lock);

    TRACE("moniker unavailable locally, calling SCM\n");

    while (TRUE)
    {
        __TRY
        {
            hr = IrotGetObject(get_irot_handle(), moniker_data, &object, &cookie);
        }
        __EXCEPT(rpc_filter)
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
        }
        __ENDTRY
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        {
            if (start_rpcss())
                continue;
        }
        break;
    }

    if (SUCCEEDED(hr))
    {
        IStream *pStream;
        hr = create_stream_on_mip_ro(object, &pStream);
        if (hr == S_OK)
        {
            hr = CoUnmarshalInterface(pStream, &IID_IUnknown, (void **)ppunkObject);
            IStream_Release(pStream);
        }
    }
    else
        WARN("Moniker unavailable, IrotGetObject returned 0x%08x\n", hr);

    HeapFree(GetProcessHeap(), 0, moniker_data);

    return hr;
}

/***********************************************************************
 *        RunningObjectTable_NoteChangeTime
 *
 * PARAMS
 *  dwRegister [in] Value identifying registration being updated
 *  pfiletime  [in] Pointer to structure containing object's last change time
 */
static HRESULT WINAPI
RunningObjectTableImpl_NoteChangeTime(IRunningObjectTable* iface,
                                      DWORD dwRegister, FILETIME *pfiletime)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    struct rot_entry *rot_entry;
    HRESULT hr = E_INVALIDARG;

    TRACE("(%p,%d,%p)\n",This,dwRegister,pfiletime);

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, struct rot_entry, entry)
    {
        if (rot_entry->cookie == dwRegister)
        {
            rot_entry->last_modified = *pfiletime;
            LeaveCriticalSection(&This->lock);

            while (TRUE)
            {
                __TRY
                {
                    hr = IrotNoteChangeTime(get_irot_handle(), dwRegister, pfiletime);
                }
                __EXCEPT(rpc_filter)
                {
                    hr = HRESULT_FROM_WIN32(GetExceptionCode());
                }
                __ENDTRY
                if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
                {
                    if (start_rpcss())
                        continue;
                }
                break;
            }

            goto done;
        }
    }
    LeaveCriticalSection(&This->lock);

done:
    TRACE("-- 0x08%x\n", hr);
    return hr;
}

/***********************************************************************
 *        RunningObjectTable_GetTimeOfLastChange
 *
 * PARAMS
 *  pmkObjectName  [in]  moniker of the object whose status is desired 
 *  pfiletime      [out] structure that receives object's last change time
 */
static HRESULT WINAPI
RunningObjectTableImpl_GetTimeOfLastChange(IRunningObjectTable* iface,
                            IMoniker *pmkObjectName, FILETIME *pfiletime)
{
    HRESULT hr = MK_E_UNAVAILABLE;
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    MonikerComparisonData *moniker_data;
    const struct rot_entry *rot_entry;

    TRACE("(%p,%p,%p)\n",This,pmkObjectName,pfiletime);

    if (pmkObjectName==NULL || pfiletime==NULL)
        return E_INVALIDARG;

    hr = reduce_moniker(pmkObjectName, NULL, &pmkObjectName);
    if (FAILED(hr))
        return hr;
    hr = get_moniker_comparison_data(pmkObjectName, &moniker_data);
    IMoniker_Release(pmkObjectName);
    if (hr != S_OK)
        return hr;

    hr = MK_E_UNAVAILABLE;

    EnterCriticalSection(&This->lock);
    LIST_FOR_EACH_ENTRY(rot_entry, &This->rot, const struct rot_entry, entry)
    {
        if ((rot_entry->moniker_data->ulCntData == moniker_data->ulCntData) &&
            !memcmp(moniker_data->abData, rot_entry->moniker_data->abData, moniker_data->ulCntData))
        {
            *pfiletime = rot_entry->last_modified;
            hr = S_OK;
            break;
        }
    }
    LeaveCriticalSection(&This->lock);

    if (hr != S_OK)
    {
        while (TRUE)
        {
            __TRY
            {
                hr = IrotGetTimeOfLastChange(get_irot_handle(), moniker_data, pfiletime);
            }
            __EXCEPT(rpc_filter)
            {
                hr = HRESULT_FROM_WIN32(GetExceptionCode());
            }
            __ENDTRY
            if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
            {
                if (start_rpcss())
                    continue;
            }
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, moniker_data);

    TRACE("-- 0x%08x\n", hr);
    return hr;
}

/***********************************************************************
 *        RunningObjectTable_EnumRunning
 *
 * PARAMS
 *  ppenumMoniker  [out]  receives the IEnumMoniker interface pointer 
 */
static HRESULT WINAPI
RunningObjectTableImpl_EnumRunning(IRunningObjectTable* iface,
                                   IEnumMoniker **ppenumMoniker)
{
    RunningObjectTableImpl *This = impl_from_IRunningObjectTable(iface);
    InterfaceList *interface_list = NULL;
    HRESULT hr;

    TRACE("(%p, %p)\n", This, ppenumMoniker);

    *ppenumMoniker = NULL;

    while (TRUE)
    {
        __TRY
        {
            hr = IrotEnumRunning(get_irot_handle(), &interface_list);
        }
        __EXCEPT(rpc_filter)
        {
            hr = HRESULT_FROM_WIN32(GetExceptionCode());
        }
        __ENDTRY
        if (hr == HRESULT_FROM_WIN32(RPC_S_SERVER_UNAVAILABLE))
        {
            if (start_rpcss())
                continue;
        }
        break;
    }

    if (SUCCEEDED(hr))
        hr = EnumMonikerImpl_CreateEnumROTMoniker(interface_list,
                                                  0, ppenumMoniker);

    return hr;
}

/* Virtual function table for the IRunningObjectTable class. */
static const IRunningObjectTableVtbl VT_RunningObjectTableImpl =
{
    RunningObjectTableImpl_QueryInterface,
    RunningObjectTableImpl_AddRef,
    RunningObjectTableImpl_Release,
    RunningObjectTableImpl_Register,
    RunningObjectTableImpl_Revoke,
    RunningObjectTableImpl_IsRunning,
    RunningObjectTableImpl_GetObject,
    RunningObjectTableImpl_NoteChangeTime,
    RunningObjectTableImpl_GetTimeOfLastChange,
    RunningObjectTableImpl_EnumRunning
};

/***********************************************************************
 *        RunningObjectTable_Initialize
 */
HRESULT WINAPI RunningObjectTableImpl_Initialize(void)
{
    TRACE("\n");

    /* create the unique instance of the RunningObjectTableImpl structure */
    runningObjectTableInstance = HeapAlloc(GetProcessHeap(), 0, sizeof(RunningObjectTableImpl));

    if (!runningObjectTableInstance)
        return E_OUTOFMEMORY;

    /* initialize the virtual table function */
    runningObjectTableInstance->IRunningObjectTable_iface.lpVtbl = &VT_RunningObjectTableImpl;

    /* the initial reference is set to "1" so that it isn't destroyed after its
     * first use until the process is destroyed, as the running object table is
     * a process-wide cache of a global table */
    runningObjectTableInstance->ref = 1;

    list_init(&runningObjectTableInstance->rot);
    InitializeCriticalSection(&runningObjectTableInstance->lock);
    DEBUG_SET_CRITSEC_NAME(&runningObjectTableInstance->lock, "RunningObjectTableImpl.lock");

    return S_OK;
}

/***********************************************************************
 *        RunningObjectTable_UnInitialize
 */
HRESULT WINAPI RunningObjectTableImpl_UnInitialize(void)
{
    TRACE("\n");

    if (runningObjectTableInstance==NULL)
        return E_POINTER;

    RunningObjectTableImpl_Release(&runningObjectTableInstance->IRunningObjectTable_iface);

    RunningObjectTableImpl_Destroy();

    return S_OK;
}

/***********************************************************************
 *           GetRunningObjectTable (OLE32.@)
 *
 * Retrieves the global running object table.
 *
 * PARAMS
 *  reserved [I] Reserved. Set to 0.
 *  pprot    [O] Address that receives the pointer to the running object table.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code.
 */
HRESULT WINAPI
GetRunningObjectTable(DWORD reserved, LPRUNNINGOBJECTTABLE *pprot)
{
    IID riid=IID_IRunningObjectTable;
    HRESULT res;

    TRACE("()\n");

    if (reserved!=0)
        return E_UNEXPECTED;

    if(runningObjectTableInstance==NULL)
        return CO_E_NOTINITIALIZED;

    res = IRunningObjectTable_QueryInterface(&runningObjectTableInstance->IRunningObjectTable_iface,
                                             &riid,(void**)pprot);

    return res;
}

static HRESULT get_moniker_for_progid_display_name(LPBC pbc,
                                                   LPCOLESTR szDisplayName,
                                                   LPDWORD pchEaten,
                                                   LPMONIKER *ppmk)
{
    CLSID clsid;
    HRESULT hr;
    LPWSTR progid;
    LPCWSTR start = szDisplayName;
    LPCWSTR end;
    int len;
    IMoniker *class_moniker;

    if (*start == '@')
        start++;

    /* find end delimiter */
    for (end = start; *end; end++)
        if (*end == ':')
            break;

    len = end - start;

    /* must start with '@' or have a ':' somewhere and mustn't be one character
     * long (since that looks like an absolute path) */
    if (((start == szDisplayName) && (*end == '\0')) || (len <= 1))
        return MK_E_SYNTAX;

    progid = HeapAlloc(GetProcessHeap(), 0, (len + 1) * sizeof(WCHAR));
    if (progid)
    {
        memcpy(progid, start, len * sizeof(WCHAR));
        progid[len] = '\0';
    }
    hr = CLSIDFromProgID(progid, &clsid);
    HeapFree(GetProcessHeap(), 0, progid);
    if (FAILED(hr))
        return MK_E_SYNTAX;

    hr = CreateClassMoniker(&clsid, &class_moniker);
    if (SUCCEEDED(hr))
    {
        IParseDisplayName *pdn;
        hr = IMoniker_BindToObject(class_moniker, pbc, NULL,
                                   &IID_IParseDisplayName, (void **)&pdn);
        /* fallback to using IClassFactory to get IParseDisplayName -
         * adsldp.dll depends on this */
        if (FAILED(hr))
        {
            IClassFactory *pcf;
            hr = IMoniker_BindToObject(class_moniker, pbc, NULL,
                                       &IID_IClassFactory, (void **)&pcf);
            if (SUCCEEDED(hr))
            {
                hr = IClassFactory_CreateInstance(pcf, NULL,
                                                  &IID_IParseDisplayName,
                                                  (void **)&pdn);
                IClassFactory_Release(pcf);
            }
        }
        IMoniker_Release(class_moniker);
        if (SUCCEEDED(hr))
        {
            hr = IParseDisplayName_ParseDisplayName(pdn, pbc,
                                                    (LPOLESTR)szDisplayName,
                                                    pchEaten, ppmk);
            IParseDisplayName_Release(pdn);
        }
    }
    return hr;
}

/******************************************************************************
 *              MkParseDisplayName        [OLE32.@]
 */
HRESULT WINAPI MkParseDisplayName(LPBC pbc, LPCOLESTR szDisplayName,
				LPDWORD pchEaten, LPMONIKER *ppmk)
{
    HRESULT hr = MK_E_SYNTAX;
    static const WCHAR wszClsidColon[] = {'c','l','s','i','d',':'};
    IMoniker *moniker;
    DWORD chEaten;

    TRACE("(%p, %s, %p, %p)\n", pbc, debugstr_w(szDisplayName), pchEaten, ppmk);

    if (!pbc || !IsValidInterface((LPUNKNOWN) pbc))
        return E_INVALIDARG;

    if (!szDisplayName || !*szDisplayName)
        return E_INVALIDARG;

    if (!pchEaten || !ppmk)
        return E_INVALIDARG;

    *pchEaten = 0;
    *ppmk = NULL;

    if (!_wcsnicmp(szDisplayName, wszClsidColon, ARRAY_SIZE(wszClsidColon)))
    {
        hr = ClassMoniker_CreateFromDisplayName(pbc, szDisplayName, &chEaten, &moniker);
        if (FAILED(hr) && (hr != MK_E_SYNTAX))
            return hr;
    }
    else
    {
        hr = get_moniker_for_progid_display_name(pbc, szDisplayName, &chEaten, &moniker);
        if (FAILED(hr) && (hr != MK_E_SYNTAX))
            return hr;
    }

    if (FAILED(hr))
    {
        hr = FileMoniker_CreateFromDisplayName(pbc, szDisplayName, &chEaten, &moniker);
        if (FAILED(hr) && (hr != MK_E_SYNTAX))
            return hr;
    }

    if (SUCCEEDED(hr))
    {
        while (TRUE)
        {
            IMoniker *next_moniker;
            *pchEaten += chEaten;
            szDisplayName += chEaten;
            if (!*szDisplayName)
            {
                *ppmk = moniker;
                return S_OK;
            }
            chEaten = 0;
            hr = IMoniker_ParseDisplayName(moniker, pbc, NULL,
                                           (LPOLESTR)szDisplayName, &chEaten,
                                           &next_moniker);
            IMoniker_Release(moniker);
            if (FAILED(hr))
            {
                *pchEaten = 0;
                break;
            }
            moniker = next_moniker;
        }
    }

    return hr;
}

/***********************************************************************
 *        GetClassFile (OLE32.@)
 *
 * Retrieves the class ID associated with the given filename.
 *
 * PARAMS
 *  filePathName [I] Filename to retrieve the class ID for.
 *  pclsid       [O] Address that receives the class ID for the file.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: Any HRESULT code.
 */
HRESULT WINAPI GetClassFile(LPCOLESTR filePathName,CLSID *pclsid)
{
    IStorage *pstg=0;
    HRESULT res;
    int nbElm, length, i;
    LONG sizeProgId, ret;
    LPOLESTR *pathDec=0,absFile=0,progId=0;
    LPWSTR extension;
    static const WCHAR bkslashW[] = {'\\',0};
    static const WCHAR dotW[] = {'.',0};

    TRACE("%s, %p\n", debugstr_w(filePathName), pclsid);

    /* if the file contain a storage object the return the CLSID written by IStorage_SetClass method*/
    if((StgIsStorageFile(filePathName))==S_OK){

        res=StgOpenStorage(filePathName,NULL,STGM_READ | STGM_SHARE_DENY_WRITE,NULL,0,&pstg);

        if (SUCCEEDED(res)) {
            res=ReadClassStg(pstg,pclsid);
            IStorage_Release(pstg);
        }

        return res;
    }
    /* If the file is not a storage object then attempt to match various bits in the file against a
       pattern in the registry. This case is not frequently used, so I present only the pseudocode for
       this case.

     for(i=0;i<nFileTypes;i++)

        for(i=0;j<nPatternsForType;j++){

            PATTERN pat;
            HANDLE  hFile;

            pat=ReadPatternFromRegistry(i,j);
            hFile=CreateFileW(filePathName,,,,,,hFile);
            SetFilePosition(hFile,pat.offset);
            ReadFile(hFile,buf,pat.size,&r,NULL);
            if (memcmp(buf&pat.mask,pat.pattern.pat.size)==0){

                *pclsid=ReadCLSIDFromRegistry(i);
                return S_OK;
            }
        }
     */

    /* if the above strategies fail then search for the extension key in the registry */

    /* get the last element (absolute file) in the path name */
    nbElm=FileMonikerImpl_DecomposePath(filePathName,&pathDec);
    absFile=pathDec[nbElm-1];

    /* failed if the path represents a directory and not an absolute file name*/
    if (!wcscmp(absFile, bkslashW)) {
        CoTaskMemFree(pathDec);
        return MK_E_INVALIDEXTENSION;
    }

    /* get the extension of the file */
    extension = NULL;
    length=lstrlenW(absFile);
    for(i = length-1; (i >= 0) && *(extension = &absFile[i]) != '.'; i--)
        /* nothing */;

    if (!extension || !wcscmp(extension, dotW)) {
        CoTaskMemFree(pathDec);
        return MK_E_INVALIDEXTENSION;
    }

    ret = RegQueryValueW(HKEY_CLASSES_ROOT, extension, NULL, &sizeProgId);
    if (!ret) {
        /* get the progId associated to the extension */
        progId = CoTaskMemAlloc(sizeProgId);
        ret = RegQueryValueW(HKEY_CLASSES_ROOT, extension, progId, &sizeProgId);
        if (!ret)
            /* return the clsid associated to the progId */
            res = CLSIDFromProgID(progId, pclsid);
        else
            res = HRESULT_FROM_WIN32(ret);
        CoTaskMemFree(progId);
    }
    else
        res = HRESULT_FROM_WIN32(ret);

    for(i=0; pathDec[i]!=NULL;i++)
        CoTaskMemFree(pathDec[i]);
    CoTaskMemFree(pathDec);

    return res != S_OK ? MK_E_INVALIDEXTENSION : res;
}

/***********************************************************************
 *        EnumMoniker_QueryInterface
 */
static HRESULT WINAPI EnumMonikerImpl_QueryInterface(IEnumMoniker* iface,REFIID riid,void** ppvObject)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p,%s,%p)\n",This,debugstr_guid(riid),ppvObject);

    /* validate arguments */
    if (ppvObject == NULL)
        return E_INVALIDARG;

    *ppvObject = NULL;

    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IEnumMoniker, riid))
        *ppvObject = &This->IEnumMoniker_iface;
    else
        return E_NOINTERFACE;

    IEnumMoniker_AddRef(iface);
    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_AddRef
 */
static ULONG   WINAPI EnumMonikerImpl_AddRef(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)\n",This);

    return InterlockedIncrement(&This->ref);
}

/***********************************************************************
 *        EnumMoniker_release
 */
static ULONG   WINAPI EnumMonikerImpl_Release(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    ULONG ref;

    TRACE("(%p)\n",This);

    ref = InterlockedDecrement(&This->ref);

    /* uninitialize ROT structure if there are no more references to it */
    if (ref == 0)
    {
        ULONG i;

        TRACE("(%p) Deleting\n",This);

        for (i = 0; i < This->moniker_list->size; i++)
            HeapFree(GetProcessHeap(), 0, This->moniker_list->interfaces[i]);
        HeapFree(GetProcessHeap(), 0, This->moniker_list);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}
/***********************************************************************
 *        EnumMoniker_Next
 */
static HRESULT   WINAPI EnumMonikerImpl_Next(IEnumMoniker* iface, ULONG celt, IMoniker** rgelt, ULONG * pceltFetched)
{
    ULONG i;
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    HRESULT hr = S_OK;

    TRACE("(%p) TabCurrentPos %d Tablastindx %d\n", This, This->pos, This->moniker_list->size);

    /* retrieve the requested number of moniker from the current position */
    for(i = 0; (This->pos < This->moniker_list->size) && (i < celt); i++)
    {
        IStream *stream;
        hr = create_stream_on_mip_ro(This->moniker_list->interfaces[This->pos++], &stream);
        if (hr != S_OK) break;
        hr = CoUnmarshalInterface(stream, &IID_IMoniker, (void **)&rgelt[i]);
        IStream_Release(stream);
        if (hr != S_OK) break;
    }

    if (pceltFetched != NULL)
        *pceltFetched= i;

    if (hr != S_OK)
        return hr;

    if (i == celt)
        return S_OK;
    else
        return S_FALSE;

}

/***********************************************************************
 *        EnumMoniker_Skip
 */
static HRESULT   WINAPI EnumMonikerImpl_Skip(IEnumMoniker* iface, ULONG celt)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    TRACE("(%p)\n",This);

    if  (This->pos + celt >= This->moniker_list->size)
        return S_FALSE;

    This->pos += celt;

    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_Reset
 */
static HRESULT   WINAPI EnumMonikerImpl_Reset(IEnumMoniker* iface)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);

    This->pos = 0;	/* set back to start of list */

    TRACE("(%p)\n",This);

    return S_OK;
}

/***********************************************************************
 *        EnumMoniker_Clone
 */
static HRESULT   WINAPI EnumMonikerImpl_Clone(IEnumMoniker* iface, IEnumMoniker ** ppenum)
{
    EnumMonikerImpl *This = impl_from_IEnumMoniker(iface);
    InterfaceList *moniker_list;
    ULONG i;

    TRACE("(%p)\n",This);

    *ppenum = NULL;

    moniker_list = HeapAlloc(GetProcessHeap(), 0, FIELD_OFFSET(InterfaceList, interfaces[This->moniker_list->size]));
    if (!moniker_list)
        return E_OUTOFMEMORY;

    moniker_list->size = This->moniker_list->size;
    for (i = 0; i < This->moniker_list->size; i++)
    {
        SIZE_T size = FIELD_OFFSET(InterfaceData, abData[This->moniker_list->interfaces[i]->ulCntData]);
        moniker_list->interfaces[i] = HeapAlloc(GetProcessHeap(), 0, size);
        if (!moniker_list->interfaces[i])
        {
            ULONG end = i;
            for (i = 0; i < end; i++)
                HeapFree(GetProcessHeap(), 0, moniker_list->interfaces[i]);
            HeapFree(GetProcessHeap(), 0, moniker_list);
            return E_OUTOFMEMORY;
        }
        memcpy(moniker_list->interfaces[i], This->moniker_list->interfaces[i], size);
    }

    /* copy the enum structure */ 
    return EnumMonikerImpl_CreateEnumROTMoniker(moniker_list, This->pos, ppenum);
}

/* Virtual function table for the IEnumMoniker class. */
static const IEnumMonikerVtbl VT_EnumMonikerImpl =
{
    EnumMonikerImpl_QueryInterface,
    EnumMonikerImpl_AddRef,
    EnumMonikerImpl_Release,
    EnumMonikerImpl_Next,
    EnumMonikerImpl_Skip,
    EnumMonikerImpl_Reset,
    EnumMonikerImpl_Clone
};

/***********************************************************************
 *        EnumMonikerImpl_CreateEnumROTMoniker
 *        Used by EnumRunning to create the structure and EnumClone
 *	  to copy the structure
 */
static HRESULT EnumMonikerImpl_CreateEnumROTMoniker(InterfaceList *moniker_list,
                                                 ULONG current_pos,
                                                 IEnumMoniker **ppenumMoniker)
{
    EnumMonikerImpl* This = NULL;

    if (!ppenumMoniker)
        return E_INVALIDARG;

    This = HeapAlloc(GetProcessHeap(), 0, sizeof(EnumMonikerImpl));
    if (!This) return E_OUTOFMEMORY;

    TRACE("(%p)\n", This);

    /* initialize the virtual table function */
    This->IEnumMoniker_iface.lpVtbl = &VT_EnumMonikerImpl;

    /* the initial reference is set to "1" */
    This->ref = 1;			/* set the ref count to one         */
    This->pos = current_pos;		/* Set the list start posn */
    This->moniker_list = moniker_list;

    *ppenumMoniker =  &This->IEnumMoniker_iface;

    return S_OK;
}


/* Shared implementation of moniker marshaler based on saving and loading of
 * monikers */

typedef struct MonikerMarshal
{
    IUnknown IUnknown_iface;
    IMarshal IMarshal_iface;

    LONG ref;
    IMoniker *moniker;
} MonikerMarshal;

static inline MonikerMarshal *impl_from_IUnknown(IUnknown *iface)
{
    return CONTAINING_RECORD(iface, MonikerMarshal, IUnknown_iface);
}

static inline MonikerMarshal *impl_from_IMarshal( IMarshal *iface )
{
    return CONTAINING_RECORD(iface, MonikerMarshal, IMarshal_iface);
}

static HRESULT WINAPI MonikerMarshalInner_QueryInterface(IUnknown *iface, REFIID riid, LPVOID *ppv)
{
    MonikerMarshal *This = impl_from_IUnknown(iface);
    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);
    *ppv = NULL;
    if (IsEqualIID(&IID_IUnknown, riid) || IsEqualIID(&IID_IMarshal, riid))
    {
        *ppv = &This->IMarshal_iface;
        IMarshal_AddRef(&This->IMarshal_iface);
        return S_OK;
    }
    FIXME("No interface for %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI MonikerMarshalInner_AddRef(IUnknown *iface)
{
    MonikerMarshal *This = impl_from_IUnknown(iface);
    return InterlockedIncrement(&This->ref);
}

static ULONG WINAPI MonikerMarshalInner_Release(IUnknown *iface)
{
    MonikerMarshal *This = impl_from_IUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    if (!ref) HeapFree(GetProcessHeap(), 0, This);
    return ref;
}

static const IUnknownVtbl VT_MonikerMarshalInner =
{
    MonikerMarshalInner_QueryInterface,
    MonikerMarshalInner_AddRef,
    MonikerMarshalInner_Release
};

static HRESULT WINAPI MonikerMarshal_QueryInterface(IMarshal *iface, REFIID riid, LPVOID *ppv)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);
    return IMoniker_QueryInterface(This->moniker, riid, ppv);
}

static ULONG WINAPI MonikerMarshal_AddRef(IMarshal *iface)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);
    return IMoniker_AddRef(This->moniker);
}

static ULONG WINAPI MonikerMarshal_Release(IMarshal *iface)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);
    return IMoniker_Release(This->moniker);
}

static HRESULT WINAPI MonikerMarshal_GetUnmarshalClass(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, CLSID* pCid)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);

    TRACE("(%s, %p, %x, %p, %x, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pCid);

    return IMoniker_GetClassID(This->moniker, pCid);
}

static HRESULT WINAPI MonikerMarshal_GetMarshalSizeMax(
  LPMARSHAL iface, REFIID riid, void* pv, DWORD dwDestContext,
  void* pvDestContext, DWORD mshlflags, DWORD* pSize)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);
    HRESULT hr;
    ULARGE_INTEGER size;

    TRACE("(%s, %p, %x, %p, %x, %p)\n", debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags, pSize);

    hr = IMoniker_GetSizeMax(This->moniker, &size);
    if (hr == S_OK)
        *pSize = (DWORD)size.QuadPart;
    return hr;
}

static HRESULT WINAPI MonikerMarshal_MarshalInterface(LPMARSHAL iface, IStream *pStm, 
    REFIID riid, void* pv, DWORD dwDestContext,
    void* pvDestContext, DWORD mshlflags)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);

    TRACE("(%p, %s, %p, %x, %p, %x)\n", pStm, debugstr_guid(riid), pv,
        dwDestContext, pvDestContext, mshlflags);

    return IMoniker_Save(This->moniker, pStm, FALSE);
}

static HRESULT WINAPI MonikerMarshal_UnmarshalInterface(LPMARSHAL iface, IStream *pStm, REFIID riid, void **ppv)
{
    MonikerMarshal *This = impl_from_IMarshal(iface);
    HRESULT hr;

    TRACE("(%p, %s, %p)\n", pStm, debugstr_guid(riid), ppv);

    hr = IMoniker_Load(This->moniker, pStm);
    if (hr == S_OK)
        hr = IMoniker_QueryInterface(This->moniker, riid, ppv);
    return hr;
}

static HRESULT WINAPI MonikerMarshal_ReleaseMarshalData(LPMARSHAL iface, IStream *pStm)
{
    TRACE("()\n");
    /* can't release a state-based marshal as nothing on server side to
     * release */
    return S_OK;
}

static HRESULT WINAPI MonikerMarshal_DisconnectObject(LPMARSHAL iface, DWORD dwReserved)
{
    TRACE("()\n");
    /* can't disconnect a state-based marshal as nothing on server side to
     * disconnect from */
    return S_OK;
}

static const IMarshalVtbl VT_MonikerMarshal =
{
    MonikerMarshal_QueryInterface,
    MonikerMarshal_AddRef,
    MonikerMarshal_Release,
    MonikerMarshal_GetUnmarshalClass,
    MonikerMarshal_GetMarshalSizeMax,
    MonikerMarshal_MarshalInterface,
    MonikerMarshal_UnmarshalInterface,
    MonikerMarshal_ReleaseMarshalData,
    MonikerMarshal_DisconnectObject
};

HRESULT MonikerMarshal_Create(IMoniker *inner, IUnknown **outer)
{
    MonikerMarshal *This = HeapAlloc(GetProcessHeap(), 0, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IUnknown_iface.lpVtbl = &VT_MonikerMarshalInner;
    This->IMarshal_iface.lpVtbl = &VT_MonikerMarshal;
    This->ref = 1;
    This->moniker = inner;

    *outer = &This->IUnknown_iface;
    return S_OK;
}

void * __RPC_USER MIDL_user_allocate(SIZE_T size)
{
    return HeapAlloc(GetProcessHeap(), 0, size);
}

void __RPC_USER MIDL_user_free(void *p)
{
    HeapFree(GetProcessHeap(), 0, p);
}
