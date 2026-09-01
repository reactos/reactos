/*
 * Copyright 2011 Hans Leidekker for CodeWeavers
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
#include "transact.h"
#include "initguid.h"
#include "txdtc.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(xolehlp);

/* Resource manager start */

typedef struct {
    IResourceManager IResourceManager_iface;
    LONG ref;
} ResourceManager;

static inline ResourceManager *impl_from_IResourceManager(IResourceManager *iface)
{
    return CONTAINING_RECORD(iface, ResourceManager, IResourceManager_iface);
}

static HRESULT WINAPI ResourceManager_QueryInterface(IResourceManager *iface, REFIID iid,
    void **ppv)
{
    ResourceManager *This = impl_from_IResourceManager(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_IResourceManager, iid))
    {
        *ppv = &This->IResourceManager_iface;
    }
    else
    {
        FIXME("(%s): not implemented\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ResourceManager_AddRef(IResourceManager *iface)
{
    ResourceManager *This = impl_from_IResourceManager(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI ResourceManager_Release(IResourceManager *iface)
{
    ResourceManager *This = impl_from_IResourceManager(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}
static HRESULT WINAPI ResourceManager_Enlist(IResourceManager *iface,
	ITransaction *pTransaction,ITransactionResourceAsync *pRes,XACTUOW *pUOW,
	LONG *pisoLevel,ITransactionEnlistmentAsync **ppEnlist)
{
    FIXME("(%p, %p, %p, %p, %p, %p): stub\n", iface, pTransaction,pRes,pUOW,
        pisoLevel,ppEnlist);
    return E_NOTIMPL;
}
static HRESULT WINAPI ResourceManager_Reenlist(IResourceManager *iface,
        byte *pPrepInfo,ULONG cbPrepInfo,DWORD lTimeout,XACTSTAT *pXactStat)
{
    FIXME("(%p, %p, %lu, %lu, %p): stub\n", iface, pPrepInfo, cbPrepInfo, lTimeout, pXactStat);
    return E_NOTIMPL;
}
static HRESULT WINAPI ResourceManager_ReenlistmentComplete(IResourceManager *iface)
{
    FIXME("(%p): stub\n", iface);
    return S_OK;
}
static HRESULT WINAPI ResourceManager_GetDistributedTransactionManager(IResourceManager *iface,
        REFIID iid,void **ppvObject)
{
    FIXME("(%p, %s, %p): stub\n", iface, debugstr_guid(iid), ppvObject);
    return E_NOTIMPL;
}

static const IResourceManagerVtbl ResourceManager_Vtbl = {
    ResourceManager_QueryInterface,
    ResourceManager_AddRef,
    ResourceManager_Release,
    ResourceManager_Enlist,
    ResourceManager_Reenlist,
    ResourceManager_ReenlistmentComplete,
    ResourceManager_GetDistributedTransactionManager
};

static HRESULT ResourceManager_Create(REFIID riid, void **ppv)
{
    ResourceManager *This;
    HRESULT ret;

    if (!ppv) return E_INVALIDARG;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->IResourceManager_iface.lpVtbl = &ResourceManager_Vtbl;
    This->ref = 1;

    ret = IResourceManager_QueryInterface(&This->IResourceManager_iface, riid, ppv);
    IResourceManager_Release(&This->IResourceManager_iface);

    return ret;
}

/* Resource manager end */

/* Transaction options start */

typedef struct {
    ITransactionOptions ITransactionOptions_iface;
    LONG ref;
    XACTOPT opts;
} TransactionOptions;

static inline TransactionOptions *impl_from_ITransactionOptions(ITransactionOptions *iface)
{
    return CONTAINING_RECORD(iface, TransactionOptions, ITransactionOptions_iface);
}

static HRESULT WINAPI TransactionOptions_QueryInterface(ITransactionOptions *iface, REFIID iid,
    void **ppv)
{
    TransactionOptions *This = impl_from_ITransactionOptions(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_ITransactionOptions, iid))
    {
        *ppv = &This->ITransactionOptions_iface;
    }
    else
    {
        FIXME("(%s): not implemented\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TransactionOptions_AddRef(ITransactionOptions *iface)
{
    TransactionOptions *This = impl_from_ITransactionOptions(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI TransactionOptions_Release(ITransactionOptions *iface)
{
    TransactionOptions *This = impl_from_ITransactionOptions(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}
static HRESULT WINAPI TransactionOptions_SetOptions(ITransactionOptions *iface,
    XACTOPT *pOptions)
{
    TransactionOptions *This = impl_from_ITransactionOptions(iface);

    if (!pOptions) return E_INVALIDARG;
    TRACE("(%p, %lu, %s)\n", iface, pOptions->ulTimeout, debugstr_a(pOptions->szDescription));
    This->opts = *pOptions;
    return S_OK;
}
static HRESULT WINAPI TransactionOptions_GetOptions(ITransactionOptions *iface,
    XACTOPT *pOptions)
{
    TransactionOptions *This = impl_from_ITransactionOptions(iface);

    TRACE("(%p, %p)\n", iface, pOptions);
    if (!pOptions) return E_INVALIDARG;
    *pOptions = This->opts;
    return S_OK;
}

static const ITransactionOptionsVtbl TransactionOptions_Vtbl = {
    TransactionOptions_QueryInterface,
    TransactionOptions_AddRef,
    TransactionOptions_Release,
    TransactionOptions_SetOptions,
    TransactionOptions_GetOptions
};

static HRESULT TransactionOptions_Create(ITransactionOptions **ppv)
{
    TransactionOptions *This;

    if (!ppv) return E_INVALIDARG;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->ITransactionOptions_iface.lpVtbl = &TransactionOptions_Vtbl;
    This->ref = 1;

    *ppv = &This->ITransactionOptions_iface;

    return S_OK;
}

/* Transaction options end */

/* Transaction start */

typedef struct {
    ITransaction ITransaction_iface;
    LONG ref;
    XACTTRANSINFO info;
} Transaction;

static inline Transaction *impl_from_ITransaction(ITransaction *iface)
{
    return CONTAINING_RECORD(iface, Transaction, ITransaction_iface);
}

static HRESULT WINAPI Transaction_QueryInterface(ITransaction *iface, REFIID iid,
    void **ppv)
{
    Transaction *This = impl_from_ITransaction(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_ITransaction, iid))
    {
        *ppv = &This->ITransaction_iface;
    }
    else
    {
        FIXME("(%s): not implemented\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI Transaction_AddRef(ITransaction *iface)
{
    Transaction *This = impl_from_ITransaction(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI Transaction_Release(ITransaction *iface)
{
    Transaction *This = impl_from_ITransaction(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}
static HRESULT WINAPI Transaction_Commit(ITransaction *iface,
    BOOL fRetaining, DWORD grfTC, DWORD grfRM)
{
    FIXME("(%p, %d, %08lx, %08lx): stub\n", iface, fRetaining, grfTC, grfRM);
    return E_NOTIMPL;
}
static HRESULT WINAPI Transaction_Abort(ITransaction *iface,
    BOID *pboidReason, BOOL fRetaining, BOOL fAsync)
{
    FIXME("(%p, %p, %d, %d): stub\n", iface, pboidReason, fRetaining, fAsync);
    return E_NOTIMPL;
}
static HRESULT WINAPI Transaction_GetTransactionInfo(ITransaction *iface,
    XACTTRANSINFO *pinfo)
{
    Transaction *This = impl_from_ITransaction(iface);
    TRACE("(%p, %p)\n", iface, pinfo);
    if (!pinfo) return E_INVALIDARG;
    *pinfo = This->info;
    return S_OK;
}

static const ITransactionVtbl Transaction_Vtbl = {
    Transaction_QueryInterface,
    Transaction_AddRef,
    Transaction_Release,
    Transaction_Commit,
    Transaction_Abort,
    Transaction_GetTransactionInfo
};

static HRESULT Transaction_Create(ISOLEVEL isoLevel, ULONG isoFlags,
        ITransactionOptions *pOptions, ITransaction **ppv)
{
    Transaction *This;

    if (!ppv) return E_INVALIDARG;

    This = calloc(1, sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->ITransaction_iface.lpVtbl = &Transaction_Vtbl;
    This->ref = 1;
    This->info.isoLevel = isoLevel;
    This->info.isoFlags = isoFlags;

    *ppv = &This->ITransaction_iface;

    return S_OK;
}

/* Transaction end */

/* DTC Proxy Core Object start */

typedef struct {
    ITransactionDispenser ITransactionDispenser_iface;
    LONG ref;
    IResourceManagerFactory2 IResourceManagerFactory2_iface;
    ITransactionImportWhereabouts ITransactionImportWhereabouts_iface;
    ITransactionImport ITransactionImport_iface;
} TransactionManager;

static inline TransactionManager *impl_from_ITransactionDispenser(ITransactionDispenser *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, ITransactionDispenser_iface);
}

static HRESULT WINAPI TransactionDispenser_QueryInterface(ITransactionDispenser *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_ITransactionDispenser(iface);
    TRACE("(%p,%s,%p)\n", iface, debugstr_guid(iid), ppv);

    if (!ppv) return E_INVALIDARG;

    if (IsEqualIID(&IID_IUnknown, iid) ||
        IsEqualIID(&IID_ITransactionDispenser, iid))
    {
        *ppv = &This->ITransactionDispenser_iface;
    }
    else if (IsEqualIID(&IID_IResourceManagerFactory, iid) ||
        IsEqualIID(&IID_IResourceManagerFactory2, iid))
    {
        *ppv = &This->IResourceManagerFactory2_iface;
    }
    else if (IsEqualIID(&IID_ITransactionImportWhereabouts, iid))
    {
        *ppv = &This->ITransactionImportWhereabouts_iface;
    }
    else if (IsEqualIID(&IID_ITransactionImport, iid))
    {
        *ppv = &This->ITransactionImport_iface;
    }
    else
    {
        FIXME("(%s): not implemented\n", debugstr_guid(iid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI TransactionDispenser_AddRef(ITransactionDispenser *iface)
{
    TransactionManager *This = impl_from_ITransactionDispenser(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI TransactionDispenser_Release(ITransactionDispenser *iface)
{
    TransactionManager *This = impl_from_ITransactionDispenser(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI TransactionDispenser_GetOptionsObject(ITransactionDispenser *iface,
        ITransactionOptions **ppOptions)
{
    TRACE("(%p, %p)\n", iface, ppOptions);

    if (!ppOptions) return E_INVALIDARG;
    return TransactionOptions_Create(ppOptions);
}
static HRESULT WINAPI TransactionDispenser_BeginTransaction(ITransactionDispenser *iface,
        IUnknown *punkOuter,
        ISOLEVEL isoLevel,
        ULONG isoFlags,
        ITransactionOptions *pOptions,
        ITransaction **ppTransaction)
{
    FIXME("(%p, %p, %08lx, %08lx, %p, %p): semi-stub\n", iface, punkOuter,
        isoLevel, isoFlags, pOptions, ppTransaction);

    if (!ppTransaction) return E_INVALIDARG;
    if (punkOuter) return CLASS_E_NOAGGREGATION;
    return Transaction_Create(isoLevel, isoFlags, pOptions, ppTransaction);
}
static const ITransactionDispenserVtbl TransactionDispenser_Vtbl = {
    TransactionDispenser_QueryInterface,
    TransactionDispenser_AddRef,
    TransactionDispenser_Release,
    TransactionDispenser_GetOptionsObject,
    TransactionDispenser_BeginTransaction
};

static inline TransactionManager *impl_from_IResourceManagerFactory2(IResourceManagerFactory2 *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, IResourceManagerFactory2_iface);
}

static HRESULT WINAPI ResourceManagerFactory2_QueryInterface(IResourceManagerFactory2 *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_IResourceManagerFactory2(iface);
    return TransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, iid, ppv);
}

static ULONG WINAPI ResourceManagerFactory2_AddRef(IResourceManagerFactory2 *iface)
{
    TransactionManager *This = impl_from_IResourceManagerFactory2(iface);
    return TransactionDispenser_AddRef(&This->ITransactionDispenser_iface);
}

static ULONG WINAPI ResourceManagerFactory2_Release(IResourceManagerFactory2 *iface)
{
    TransactionManager *This = impl_from_IResourceManagerFactory2(iface);
    return TransactionDispenser_Release(&This->ITransactionDispenser_iface);
}
static HRESULT WINAPI ResourceManagerFactory2_Create(IResourceManagerFactory2 *iface,
        GUID *pguidRM, CHAR *pszRMName, IResourceManagerSink *pIResMgrSink, IResourceManager **ppResMgr)
{
    FIXME("(%p, %s, %s, %p, %p): semi-stub\n", iface, debugstr_guid(pguidRM),
        debugstr_a(pszRMName), pIResMgrSink, ppResMgr);
    return ResourceManager_Create(&IID_IResourceManager, (void**)ppResMgr);
}
static HRESULT WINAPI ResourceManagerFactory2_CreateEx(IResourceManagerFactory2 *iface,
        GUID *pguidRM, CHAR *pszRMName, IResourceManagerSink *pIResMgrSink, REFIID riidRequested, void **ppResMgr)
{
    FIXME("(%p, %s, %s, %p, %s, %p): semi-stub\n", iface, debugstr_guid(pguidRM),
        debugstr_a(pszRMName), pIResMgrSink, debugstr_guid(riidRequested), ppResMgr);

    return ResourceManager_Create(riidRequested, ppResMgr);
}
static const IResourceManagerFactory2Vtbl ResourceManagerFactory2_Vtbl = {
    ResourceManagerFactory2_QueryInterface,
    ResourceManagerFactory2_AddRef,
    ResourceManagerFactory2_Release,
    ResourceManagerFactory2_Create,
    ResourceManagerFactory2_CreateEx
};

static inline TransactionManager *impl_from_ITransactionImportWhereabouts(ITransactionImportWhereabouts *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, ITransactionImportWhereabouts_iface);
}

static HRESULT WINAPI TransactionImportWhereabouts_QueryInterface(ITransactionImportWhereabouts *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_ITransactionImportWhereabouts(iface);
    return TransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, iid, ppv);
}

static ULONG WINAPI TransactionImportWhereabouts_AddRef(ITransactionImportWhereabouts *iface)
{
    TransactionManager *This = impl_from_ITransactionImportWhereabouts(iface);
    return TransactionDispenser_AddRef(&This->ITransactionDispenser_iface);
}

static ULONG WINAPI TransactionImportWhereabouts_Release(ITransactionImportWhereabouts *iface)
{
    TransactionManager *This = impl_from_ITransactionImportWhereabouts(iface);
    return TransactionDispenser_Release(&This->ITransactionDispenser_iface);
}
static HRESULT WINAPI TransactionImportWhereabouts_GetWhereaboutsSize(ITransactionImportWhereabouts *iface,
        ULONG *pcbWhereabouts)
{
    FIXME("(%p, %p): stub returning fake value\n", iface, pcbWhereabouts);

    if (!pcbWhereabouts) return E_INVALIDARG;
    *pcbWhereabouts = 1;
    return S_OK;
}
static HRESULT WINAPI TransactionImportWhereabouts_GetWhereabouts(ITransactionImportWhereabouts *iface,
        ULONG cbWhereabouts, BYTE *rgbWhereabouts,ULONG *pcbUsed)
{
    FIXME("(%p, %lu, %p, %p): stub returning fake value\n", iface, cbWhereabouts, rgbWhereabouts, pcbUsed);

    if (!rgbWhereabouts || !pcbUsed) return E_INVALIDARG;
    *rgbWhereabouts = 0;
    *pcbUsed = 1;
    return S_OK;
}
static const ITransactionImportWhereaboutsVtbl TransactionImportWhereabouts_Vtbl = {
    TransactionImportWhereabouts_QueryInterface,
    TransactionImportWhereabouts_AddRef,
    TransactionImportWhereabouts_Release,
    TransactionImportWhereabouts_GetWhereaboutsSize,
    TransactionImportWhereabouts_GetWhereabouts
};

static inline TransactionManager *impl_from_ITransactionImport(ITransactionImport *iface)
{
    return CONTAINING_RECORD(iface, TransactionManager, ITransactionImport_iface);
}

static HRESULT WINAPI TransactionImport_QueryInterface(ITransactionImport *iface, REFIID iid,
    void **ppv)
{
    TransactionManager *This = impl_from_ITransactionImport(iface);
    return TransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, iid, ppv);
}

static ULONG WINAPI TransactionImport_AddRef(ITransactionImport *iface)
{
    TransactionManager *This = impl_from_ITransactionImport(iface);
    return TransactionDispenser_AddRef(&This->ITransactionDispenser_iface);
}

static ULONG WINAPI TransactionImport_Release(ITransactionImport *iface)
{
    TransactionManager *This = impl_from_ITransactionImport(iface);
    return TransactionDispenser_Release(&This->ITransactionDispenser_iface);
}
static HRESULT WINAPI TransactionImport_Import(ITransactionImport *iface,
    ULONG cbTransactionCookie, byte *rgbTransactionCookie, IID *piid, void **ppvTransaction)
{
    FIXME("(%p, %lu, %p, %s, %p): stub\n", iface, cbTransactionCookie, rgbTransactionCookie, debugstr_guid(piid), ppvTransaction);

    if (!rgbTransactionCookie || !piid || !ppvTransaction) return E_INVALIDARG;
    return E_NOTIMPL;
}
static const ITransactionImportVtbl TransactionImport_Vtbl = {
    TransactionImport_QueryInterface,
    TransactionImport_AddRef,
    TransactionImport_Release,
    TransactionImport_Import
};

static HRESULT TransactionManager_Create(REFIID riid, void **ppv)
{
    TransactionManager *This;
    HRESULT ret;

    This = malloc(sizeof(*This));
    if (!This) return E_OUTOFMEMORY;

    This->ITransactionDispenser_iface.lpVtbl = &TransactionDispenser_Vtbl;
    This->IResourceManagerFactory2_iface.lpVtbl = &ResourceManagerFactory2_Vtbl;
    This->ITransactionImportWhereabouts_iface.lpVtbl = &TransactionImportWhereabouts_Vtbl;
    This->ITransactionImport_iface.lpVtbl = &TransactionImport_Vtbl;
    This->ref = 1;

    ret = ITransactionDispenser_QueryInterface(&This->ITransactionDispenser_iface, riid, ppv);
    ITransactionDispenser_Release(&This->ITransactionDispenser_iface);

    return ret;
}
/* DTC Proxy Core Object end */

static BOOL is_local_machineA( const CHAR *server )
{
    static const CHAR dot[] = ".";
    CHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = ARRAY_SIZE( buffer );

    if (!server || !strcmp( server, dot )) return TRUE;
    if (GetComputerNameA( buffer, &len ) && !lstrcmpiA( server, buffer )) return TRUE;
    return FALSE;
}
static BOOL is_local_machineW( const WCHAR *server )
{
    WCHAR buffer[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD len = ARRAY_SIZE( buffer );

    if (!server || !wcscmp( server, L"." )) return TRUE;
    if (GetComputerNameW( buffer, &len ) && !wcsicmp( server, buffer )) return TRUE;
    return FALSE;
}

HRESULT CDECL DtcGetTransactionManager(char *host, char *tm_name, REFIID riid,
        DWORD dwReserved1, WORD wcbReserved2, void *pvReserved2, void **ppv)
{
    TRACE("(%s, %s, %s, %ld, %d, %p, %p)\n", debugstr_a(host), debugstr_a(tm_name),
          debugstr_guid(riid), dwReserved1, wcbReserved2, pvReserved2, ppv);

    if (!is_local_machineA(host))
    {
        FIXME("remote computer not supported\n");
        return E_NOTIMPL;
    }
    return TransactionManager_Create(riid, ppv);
}

HRESULT CDECL DtcGetTransactionManagerExA(CHAR *host, CHAR *tm_name, REFIID riid,
        DWORD options, void *config, void **ppv)
{
    TRACE("(%s, %s, %s, %ld, %p, %p)\n", debugstr_a(host), debugstr_a(tm_name),
          debugstr_guid(riid), options, config, ppv);

    if (!is_local_machineA(host))
    {
        FIXME("remote computer not supported\n");
        return E_NOTIMPL;
    }
    return TransactionManager_Create(riid, ppv);
}

HRESULT CDECL DtcGetTransactionManagerExW(WCHAR *host, WCHAR *tm_name, REFIID riid,
        DWORD options, void *config, void **ppv)
{
    TRACE("(%s, %s, %s, %ld, %p, %p)\n", debugstr_w(host), debugstr_w(tm_name),
            debugstr_guid(riid), options, config, ppv);

    if (!is_local_machineW(host))
    {
        FIXME("remote computer not supported\n");
        return E_NOTIMPL;
    }
    return TransactionManager_Create(riid, ppv);
}
