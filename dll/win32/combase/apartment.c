/*
 *      Copyright 1995 Martin von Loewis
 *      Copyright 1998 Justin Bradford
 *      Copyright 1999 Francis Beaudet
 *      Copyright 1999 Sylvain St-Germain
 *      Copyright 2002 Marcus Meissner
 *      Copyright 2004 Mike Hearn
 *      Copyright 2005-2006 Robert Shearman (for CodeWeavers)
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
 *
 */

#include <stdarg.h>
#include <assert.h>

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "servprov.h"

#include "combase_private.h"

#include "wine/debug.h"
#include "wine/list.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

enum comclass_threadingmodel
{
    ThreadingModel_Apartment = 1,
    ThreadingModel_Free      = 2,
    ThreadingModel_No        = 3,
    ThreadingModel_Both      = 4,
    ThreadingModel_Neutral   = 5
};

static struct apartment *mta;
static struct apartment *main_sta; /* the first STA */
static struct list apts = LIST_INIT(apts);

static CRITICAL_SECTION apt_cs;
static CRITICAL_SECTION_DEBUG apt_cs_debug =
{
    0, 0, &apt_cs,
    { &apt_cs_debug.ProcessLocksList, &apt_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": apt_cs") }
};
static CRITICAL_SECTION apt_cs = { &apt_cs_debug, -1, 0, 0, 0, 0 };

static struct list dlls = LIST_INIT(dlls);

static CRITICAL_SECTION dlls_cs;
static CRITICAL_SECTION_DEBUG dlls_cs_debug =
{
    0, 0, &dlls_cs,
    { &dlls_cs_debug.ProcessLocksList, &dlls_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": dlls_cs") }
};
static CRITICAL_SECTION dlls_cs = { &dlls_cs_debug, -1, 0, 0, 0, 0 };

typedef HRESULT (WINAPI *DllGetClassObjectFunc)(REFCLSID clsid, REFIID iid, void **obj);
typedef HRESULT (WINAPI *DllCanUnloadNowFunc)(void);

struct opendll
{
    LONG refs;
    LPWSTR library_name;
    HANDLE library;
    DllGetClassObjectFunc DllGetClassObject;
    DllCanUnloadNowFunc DllCanUnloadNow;
    struct list entry;
};

struct apartment_loaded_dll
{
    struct list entry;
    struct opendll *dll;
    DWORD unload_time;
    BOOL multi_threaded;
};

static struct opendll *apartment_get_dll(const WCHAR *library_name)
{
    struct opendll *ptr, *ret = NULL;

    EnterCriticalSection(&dlls_cs);
    LIST_FOR_EACH_ENTRY(ptr, &dlls, struct opendll, entry)
    {
        if (!wcsicmp(library_name, ptr->library_name) &&
            (InterlockedIncrement(&ptr->refs) != 1) /* entry is being destroyed if == 1 */)
        {
            ret = ptr;
            break;
        }
    }
    LeaveCriticalSection(&dlls_cs);

    return ret;
}

/* caller must ensure that library_name is not already in the open dll list */
static HRESULT apartment_add_dll(const WCHAR *library_name, struct opendll **ret)
{
    struct opendll *entry;
    int len;
    HRESULT hr = S_OK;
    HANDLE hLibrary;
    DllCanUnloadNowFunc DllCanUnloadNow;
    DllGetClassObjectFunc DllGetClassObject;

    TRACE("%s\n", debugstr_w(library_name));

    *ret = apartment_get_dll(library_name);
    if (*ret) return S_OK;

    /* Load outside of dlls lock to avoid dependency on the loader lock */
    hLibrary = LoadLibraryExW(library_name, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hLibrary)
    {
        ERR("couldn't load in-process dll %s\n", debugstr_w(library_name));
        /* failure: DLL could not be loaded */
        return E_ACCESSDENIED; /* FIXME: or should this be CO_E_DLLNOTFOUND? */
    }

    /* DllCanUnloadNow is optional */
    DllCanUnloadNow = (void *)GetProcAddress(hLibrary, "DllCanUnloadNow");
    DllGetClassObject = (void *)GetProcAddress(hLibrary, "DllGetClassObject");
    if (!DllGetClassObject)
    {
        /* failure: the dll did not export DllGetClassObject */
        ERR("couldn't find function DllGetClassObject in %s\n", debugstr_w(library_name));
        FreeLibrary(hLibrary);
        return CO_E_DLLNOTFOUND;
    }

    EnterCriticalSection(&dlls_cs);

    *ret = apartment_get_dll(library_name);
    if (*ret)
    {
        /* another caller to this function already added the dll while we
         * weren't in the critical section */
        FreeLibrary(hLibrary);
    }
    else
    {
        len = lstrlenW(library_name);
        entry = malloc(sizeof(*entry));
        if (entry)
            entry->library_name = malloc((len + 1) * sizeof(WCHAR));
        if (entry && entry->library_name)
        {
            memcpy(entry->library_name, library_name, (len + 1)*sizeof(WCHAR));
            entry->library = hLibrary;
            entry->refs = 1;
            entry->DllCanUnloadNow = DllCanUnloadNow;
            entry->DllGetClassObject = DllGetClassObject;
            list_add_tail(&dlls, &entry->entry);
            *ret = entry;
        }
        else
        {
            free(entry);
            hr = E_OUTOFMEMORY;
            FreeLibrary(hLibrary);
        }
    }

    LeaveCriticalSection(&dlls_cs);

    return hr;
}

/* pass FALSE for free_entry to release a reference without destroying the
 * entry if it reaches zero or TRUE otherwise */
static void apartment_release_dll(struct opendll *entry, BOOL free_entry)
{
    if (!InterlockedDecrement(&entry->refs) && free_entry)
    {
        EnterCriticalSection(&dlls_cs);
        list_remove(&entry->entry);
        LeaveCriticalSection(&dlls_cs);

        TRACE("freeing %p\n", entry->library);
        FreeLibrary(entry->library);

        free(entry->library_name);
        free(entry);
    }
}

/* frees memory associated with active dll list */
static void apartment_release_dlls(void)
{
    struct opendll *entry, *cursor2;
    EnterCriticalSection(&dlls_cs);
    LIST_FOR_EACH_ENTRY_SAFE(entry, cursor2, &dlls, struct opendll, entry)
    {
        list_remove(&entry->entry);
        free(entry->library_name);
        free(entry);
    }
    LeaveCriticalSection(&dlls_cs);
    DeleteCriticalSection(&dlls_cs);
}

/*
 * This is a marshallable object exposing registered local servers.
 * IServiceProvider is used only because it happens meet requirements
 * and already has proxy/stub code. If more functionality is needed,
 * a custom interface may be used instead.
 */
struct local_server
{
    IServiceProvider IServiceProvider_iface;
    LONG refcount;
    struct apartment *apt;
    IStream *marshal_stream;
};

static inline struct local_server *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, struct local_server, IServiceProvider_iface);
}

static HRESULT WINAPI local_server_QueryInterface(IServiceProvider *iface, REFIID riid, void **obj)
{
    struct local_server *local_server = impl_from_IServiceProvider(iface);

    TRACE("%p, %s, %p\n", iface, debugstr_guid(riid), obj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
            IsEqualGUID(riid, &IID_IServiceProvider))
    {
        *obj = &local_server->IServiceProvider_iface;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*obj);
    return S_OK;
}

static ULONG WINAPI local_server_AddRef(IServiceProvider *iface)
{
    struct local_server *local_server = impl_from_IServiceProvider(iface);
    LONG refcount = InterlockedIncrement(&local_server->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI local_server_Release(IServiceProvider *iface)
{
    struct local_server *local_server = impl_from_IServiceProvider(iface);
    LONG refcount = InterlockedDecrement(&local_server->refcount);

    TRACE("%p, refcount %ld\n", iface, refcount);

    if (!refcount)
    {
        assert(!local_server->apt);
        free(local_server);
    }

    return refcount;
}

static HRESULT WINAPI local_server_QueryService(IServiceProvider *iface, REFGUID guid, REFIID riid, void **obj)
{
    struct local_server *local_server = impl_from_IServiceProvider(iface);
    struct apartment *apt = com_get_current_apt();
    HRESULT hr = E_FAIL;
    IUnknown *unk;

    TRACE("%p, %s, %s, %p\n", iface, debugstr_guid(guid), debugstr_guid(riid), obj);

    if (!local_server->apt)
        return E_UNEXPECTED;

    if ((unk = com_get_registered_class_object(apt, guid, CLSCTX_LOCAL_SERVER)))
    {
        hr = IUnknown_QueryInterface(unk, riid, obj);
        IUnknown_Release(unk);
    }

    return hr;
}

static const IServiceProviderVtbl local_server_vtbl =
{
    local_server_QueryInterface,
    local_server_AddRef,
    local_server_Release,
    local_server_QueryService
};

HRESULT apartment_get_local_server_stream(struct apartment *apt, IStream **ret)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&apt->cs);

    if (!apt->local_server)
    {
        struct local_server *obj;

        obj = malloc(sizeof(*obj));
        if (obj)
        {
            obj->IServiceProvider_iface.lpVtbl = &local_server_vtbl;
            obj->refcount = 1;
            obj->apt = apt;

            hr = CreateStreamOnHGlobal(0, TRUE, &obj->marshal_stream);
            if (SUCCEEDED(hr))
            {
                hr = CoMarshalInterface(obj->marshal_stream, &IID_IServiceProvider, (IUnknown *)&obj->IServiceProvider_iface,
                        MSHCTX_LOCAL, NULL, MSHLFLAGS_TABLESTRONG);
                if (FAILED(hr))
                    IStream_Release(obj->marshal_stream);
            }

            if (SUCCEEDED(hr))
                apt->local_server = obj;
            else
                free(obj);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
        hr = IStream_Clone(apt->local_server->marshal_stream, ret);

    LeaveCriticalSection(&apt->cs);

    if (FAILED(hr))
        ERR("Failed: %#lx\n", hr);

    return hr;
}

/* Creates new apartment for given model */
static struct apartment *apartment_construct(DWORD model)
{
    struct apartment *apt;

    TRACE("creating new apartment, model %ld\n", model);

    apt = calloc(1, sizeof(*apt));
    apt->tid = GetCurrentThreadId();

    list_init(&apt->proxies);
    list_init(&apt->stubmgrs);
    list_init(&apt->loaded_dlls);
    list_init(&apt->usage_cookies);
    apt->ipidc = 0;
    apt->refs = 1;
    apt->remunk_exported = FALSE;
    apt->oidc = 1;
    InitializeCriticalSectionEx(&apt->cs, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
    apt->cs.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": apartment");

    apt->multi_threaded = !(model & COINIT_APARTMENTTHREADED);

    if (apt->multi_threaded)
    {
        /* FIXME: should be randomly generated by in an RPC call to rpcss */
        apt->oxid = ((OXID)GetCurrentProcessId() << 32) | 0xcafe;
    }
    else
    {
        /* FIXME: should be randomly generated by in an RPC call to rpcss */
        apt->oxid = ((OXID)GetCurrentProcessId() << 32) | GetCurrentThreadId();
    }

    TRACE("Created apartment on OXID %s\n", wine_dbgstr_longlong(apt->oxid));

    list_add_head(&apts, &apt->entry);

    return apt;
}

/* Frees unused libraries loaded into apartment */
void apartment_freeunusedlibraries(struct apartment *apt, DWORD delay)
{
    struct apartment_loaded_dll *entry, *next;

    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &apt->loaded_dlls, struct apartment_loaded_dll, entry)
    {
        if (entry->dll->DllCanUnloadNow && (entry->dll->DllCanUnloadNow() == S_OK))
        {
            DWORD real_delay = delay;

            if (real_delay == INFINITE)
            {
                /* DLLs that return multi-threaded objects aren't unloaded
                 * straight away to cope for programs that have races between
                 * last object destruction and threads in the DLLs that haven't
                 * finished, despite DllCanUnloadNow returning S_OK */
                if (entry->multi_threaded)
                    real_delay = 10 * 60 * 1000; /* 10 minutes */
                else
                    real_delay = 0;
            }

            if (!real_delay || (entry->unload_time && ((int)(GetTickCount() - entry->unload_time) > 0)))
            {
                list_remove(&entry->entry);
                apartment_release_dll(entry->dll, TRUE);
                free(entry);
            }
            else
            {
                entry->unload_time = GetTickCount() + real_delay;
                if (!entry->unload_time) entry->unload_time = 1;
            }
        }
        else if (entry->unload_time)
            entry->unload_time = 0;
    }
    LeaveCriticalSection(&apt->cs);
}

void apartment_release(struct apartment *apt)
{
    DWORD refcount;

    EnterCriticalSection(&apt_cs);

    refcount = InterlockedDecrement(&apt->refs);
    TRACE("%s: after = %ld\n", wine_dbgstr_longlong(apt->oxid), refcount);

    if (apt->being_destroyed)
    {
        LeaveCriticalSection(&apt_cs);
        return;
    }

    /* destruction stuff that needs to happen under global */
    if (!refcount)
    {
        apt->being_destroyed = TRUE;
        if (apt == mta) mta = NULL;
        else if (apt == main_sta) main_sta = NULL;
        list_remove(&apt->entry);
    }

    LeaveCriticalSection(&apt_cs);

    if (!refcount)
    {
        struct list *cursor, *cursor2;

        TRACE("destroying apartment %p, oxid %s\n", apt, wine_dbgstr_longlong(apt->oxid));

        if (apt->local_server)
        {
            struct local_server *local_server = apt->local_server;
            LARGE_INTEGER zero;

            memset(&zero, 0, sizeof(zero));
            IStream_Seek(local_server->marshal_stream, zero, STREAM_SEEK_SET, NULL);
            CoReleaseMarshalData(local_server->marshal_stream);
            IStream_Release(local_server->marshal_stream);
            local_server->marshal_stream = NULL;

            apt->local_server = NULL;
            local_server->apt = NULL;
            IServiceProvider_Release(&local_server->IServiceProvider_iface);
        }

        /* Release the references to the registered class objects */
        apartment_revoke_all_classes(apt);

        /* no locking is needed for this apartment, because no other thread
         * can access it at this point */

        apartment_disconnectproxies(apt);

        if (apt->win) DestroyWindow(apt->win);
        if (apt->host_apt_tid) PostThreadMessageW(apt->host_apt_tid, WM_QUIT, 0, 0);

        LIST_FOR_EACH_SAFE(cursor, cursor2, &apt->stubmgrs)
        {
            struct stub_manager *stubmgr = LIST_ENTRY(cursor, struct stub_manager, entry);
            /* release the implicit reference given by the fact that the
             * stub has external references (it must do since it is in the
             * stub manager list in the apartment and all non-apartment users
             * must have a ref on the apartment and so it cannot be destroyed).
             */
            stub_manager_int_release(stubmgr);
        }

        /* if this assert fires, then another thread took a reference to a
         * stub manager without taking a reference to the containing
         * apartment, which it must do. */
        assert(list_empty(&apt->stubmgrs));

        if (apt->filter) IMessageFilter_Release(apt->filter);

        /* free as many unused libraries as possible... */
        apartment_freeunusedlibraries(apt, 0);

        /* ... and free the memory for the apartment loaded dll entry and
         * release the dll list reference without freeing the library for the
         * rest */
        while ((cursor = list_head(&apt->loaded_dlls)))
        {
            struct apartment_loaded_dll *apartment_loaded_dll = LIST_ENTRY(cursor, struct apartment_loaded_dll, entry);
            apartment_release_dll(apartment_loaded_dll->dll, FALSE);
            list_remove(cursor);
            free(apartment_loaded_dll);
        }

        apt->cs.DebugInfo->Spare[0] = 0;
        DeleteCriticalSection(&apt->cs);

        free(apt);
    }
}

static DWORD apartment_addref(struct apartment *apt)
{
    DWORD refs = InterlockedIncrement(&apt->refs);
    TRACE("%s: before = %ld\n", wine_dbgstr_longlong(apt->oxid), refs - 1);
    return refs;
}

/* Gets existing apartment or creates a new one and enters it */
static struct apartment *apartment_get_or_create(DWORD model)
{
    struct apartment *apt = com_get_current_apt();
    struct tlsdata *data;

    if (!apt)
    {
        com_get_tlsdata(&data);

        if (model & COINIT_APARTMENTTHREADED)
        {
            EnterCriticalSection(&apt_cs);

            apt = apartment_construct(model);
            if (!main_sta)
            {
                main_sta = apt;
                apt->main = TRUE;
                TRACE("Created main-threaded apartment with OXID %s\n", wine_dbgstr_longlong(apt->oxid));
            }

            data->flags |= OLETLS_APARTMENTTHREADED;
            if (model & COINIT_DISABLE_OLE1DDE)
                data->flags |= OLETLS_DISABLE_OLE1DDE;

            LeaveCriticalSection(&apt_cs);

            if (apt->main)
                apartment_createwindowifneeded(apt);
        }
        else
        {
            EnterCriticalSection(&apt_cs);

            /* The multi-threaded apartment (MTA) contains zero or more threads interacting
             * with free threaded (ie thread safe) COM objects. There is only ever one MTA
             * in a process */
            if (mta)
            {
                TRACE("entering the multithreaded apartment %s\n", wine_dbgstr_longlong(mta->oxid));
                apartment_addref(mta);
            }
            else
                mta = apartment_construct(model);

            data->flags |= OLETLS_MULTITHREADED | OLETLS_DISABLE_OLE1DDE;

            apt = mta;

            LeaveCriticalSection(&apt_cs);
        }
        data->apt = apt;
    }

    return apt;
}

struct apartment * apartment_get_mta(void)
{
    struct apartment *apt;

    EnterCriticalSection(&apt_cs);

    if ((apt = mta))
        apartment_addref(apt);

    LeaveCriticalSection(&apt_cs);

    return apt;
}

/* Return the current apartment if it exists, or, failing that, the MTA. Caller
 * must free the returned apartment in either case. */
struct apartment * apartment_get_current_or_mta(void)
{
    struct apartment *apt = com_get_current_apt();
    if (apt)
    {
        apartment_addref(apt);
        return apt;
    }
    return apartment_get_mta();
}

/* The given OXID must be local to this process */
struct apartment * apartment_findfromoxid(OXID oxid)
{
    struct apartment *result = NULL, *apt;

    EnterCriticalSection(&apt_cs);
    LIST_FOR_EACH_ENTRY(apt, &apts, struct apartment, entry)
    {
        if (apt->oxid == oxid)
        {
            result = apt;
            apartment_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&apt_cs);

    return result;
}

/* gets the apartment which has a given creator thread ID. The caller must
 * release the reference from the apartment as soon as the apartment pointer
 * is no longer required. */
struct apartment * apartment_findfromtid(DWORD tid)
{
    struct apartment *result = NULL, *apt;

    EnterCriticalSection(&apt_cs);
    LIST_FOR_EACH_ENTRY(apt, &apts, struct apartment, entry)
    {
        if (apt != mta && apt->tid == tid)
        {
            result = apt;
            apartment_addref(result);
            break;
        }
    }

    if (!result && mta && mta->tid == tid)
    {
        result = mta;
        apartment_addref(result);
    }

    LeaveCriticalSection(&apt_cs);

    return result;
}

/* gets the main apartment if it exists. The caller must
 * release the reference from the apartment as soon as the apartment pointer
 * is no longer required. */
static struct apartment *apartment_findmain(void)
{
    struct apartment *result;

    EnterCriticalSection(&apt_cs);

    result = main_sta;
    if (result) apartment_addref(result);

    LeaveCriticalSection(&apt_cs);

    return result;
}

struct host_object_params
{
    struct class_reg_data regdata;
    CLSID clsid; /* clsid of object to marshal */
    IID iid; /* interface to marshal */
    HANDLE event; /* event signalling when ready for multi-threaded case */
    HRESULT hr; /* result for multi-threaded case */
    IStream *stream; /* stream that the object will be marshaled into */
    BOOL apartment_threaded; /* is the component purely apartment-threaded? */
};

/* Returns expanded dll path from the registry or activation context. */
static BOOL get_object_dll_path(const struct class_reg_data *regdata, WCHAR *dst, DWORD dstlen)
{
    DWORD ret;

    if (regdata->origin == CLASS_REG_REGISTRY)
    {
        DWORD keytype;
        WCHAR src[MAX_PATH];
        DWORD dwLength = dstlen * sizeof(WCHAR);

        if ((ret = RegQueryValueExW(regdata->u.hkey, NULL, NULL, &keytype, (BYTE*)src, &dwLength)) == ERROR_SUCCESS)
        {
            if (keytype == REG_EXPAND_SZ)
            {
                if (dstlen <= ExpandEnvironmentStringsW(src, dst, dstlen)) ret = ERROR_MORE_DATA;
            }
            else
            {
                const WCHAR *quote_start;
                quote_start = wcschr(src, '\"');
                if (quote_start)
                {
                    const WCHAR *quote_end = wcschr(quote_start + 1, '\"');
                    if (quote_end)
                    {
                        memmove(src, quote_start + 1, (quote_end - quote_start - 1) * sizeof(WCHAR));
                        src[quote_end - quote_start - 1] = '\0';
                    }
                }
                lstrcpynW(dst, src, dstlen);
            }
        }
        return !ret;
    }
    else
    {
        ULONG_PTR cookie;

        *dst = 0;
        ActivateActCtx(regdata->u.actctx.hactctx, &cookie);
        ret = SearchPathW(NULL, regdata->u.actctx.module_name, L".dll", dstlen, dst, NULL);
        DeactivateActCtx(0, cookie);
        return *dst != 0;
    }
}

/* gets the specified class object by loading the appropriate DLL, if
 * necessary and calls the DllGetClassObject function for the DLL */
static HRESULT apartment_getclassobject(struct apartment *apt, LPCWSTR dllpath,
                                        BOOL apartment_threaded,
                                        REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    BOOL found = FALSE;
    struct apartment_loaded_dll *apartment_loaded_dll;

    if (!wcsicmp(dllpath, L"ole32.dll"))
    {
        HRESULT (WINAPI *p_ole32_DllGetClassObject)(REFCLSID clsid, REFIID riid, void **obj);

        p_ole32_DllGetClassObject = (void *)GetProcAddress(GetModuleHandleW(L"ole32.dll"), "DllGetClassObject");

        /* we don't need to control the lifetime of this dll, so use the local
         * implementation of DllGetClassObject directly */
        TRACE("calling ole32!DllGetClassObject\n");
        hr = p_ole32_DllGetClassObject(rclsid, riid, ppv);

        if (hr != S_OK)
            ERR("DllGetClassObject returned error %#lx for dll %s\n", hr, debugstr_w(dllpath));

        return hr;
    }

    EnterCriticalSection(&apt->cs);

    LIST_FOR_EACH_ENTRY(apartment_loaded_dll, &apt->loaded_dlls, struct apartment_loaded_dll, entry)
        if (!wcsicmp(dllpath, apartment_loaded_dll->dll->library_name))
        {
            TRACE("found %s already loaded\n", debugstr_w(dllpath));
            found = TRUE;
            break;
        }

    if (!found)
    {
        apartment_loaded_dll = malloc(sizeof(*apartment_loaded_dll));
        if (!apartment_loaded_dll)
            hr = E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            apartment_loaded_dll->unload_time = 0;
            apartment_loaded_dll->multi_threaded = FALSE;
            hr = apartment_add_dll(dllpath, &apartment_loaded_dll->dll);
            if (FAILED(hr))
                free(apartment_loaded_dll);
        }
        if (SUCCEEDED(hr))
        {
            TRACE("added new loaded dll %s\n", debugstr_w(dllpath));
            list_add_tail(&apt->loaded_dlls, &apartment_loaded_dll->entry);
        }
    }

    LeaveCriticalSection(&apt->cs);

    if (SUCCEEDED(hr))
    {
        /* one component being multi-threaded overrides any number of
         * apartment-threaded components */
        if (!apartment_threaded)
            apartment_loaded_dll->multi_threaded = TRUE;

        TRACE("calling DllGetClassObject %p\n", apartment_loaded_dll->dll->DllGetClassObject);
        /* OK: get the ClassObject */
        hr = apartment_loaded_dll->dll->DllGetClassObject(rclsid, riid, ppv);

        if (hr != S_OK)
            ERR("DllGetClassObject returned error %#lx for dll %s\n", hr, debugstr_w(dllpath));
    }

    return hr;
}

static HRESULT apartment_hostobject(struct apartment *apt,
                                    const struct host_object_params *params);

struct host_thread_params
{
    COINIT threading_model;
    HANDLE ready_event;
    HWND apartment_hwnd;
};

/* thread for hosting an object to allow an object to appear to be created in
 * an apartment with an incompatible threading model */
static DWORD CALLBACK apartment_hostobject_thread(void *p)
{
    struct host_thread_params *params = p;
    MSG msg;
    HRESULT hr;
    struct apartment *apt;

    TRACE("\n");

    hr = CoInitializeEx(NULL, params->threading_model);
    if (FAILED(hr)) return hr;

    apt = com_get_current_apt();
    if (params->threading_model == COINIT_APARTMENTTHREADED)
    {
        apartment_createwindowifneeded(apt);
        params->apartment_hwnd = apartment_getwindow(apt);
    }
    else
        params->apartment_hwnd = NULL;

    /* force the message queue to be created before signaling parent thread */
    PeekMessageW(&msg, NULL, WM_USER, WM_USER, PM_NOREMOVE);

    SetEvent(params->ready_event);
    params = NULL; /* can't touch params after here as it may be invalid */

    while (GetMessageW(&msg, NULL, 0, 0))
    {
        if (!msg.hwnd && (msg.message == DM_HOSTOBJECT))
        {
            struct host_object_params *obj_params = (struct host_object_params *)msg.lParam;
            obj_params->hr = apartment_hostobject(apt, obj_params);
            SetEvent(obj_params->event);
        }
        else
        {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }

    TRACE("exiting\n");

    CoUninitialize();

    return S_OK;
}

/* finds or creates a host apartment, creates the object inside it and returns
 * a proxy to it so that the object can be used in the apartment of the
 * caller of this function */
static HRESULT apartment_hostobject_in_hostapt(struct apartment *apt, BOOL multi_threaded,
        BOOL main_apartment, const struct class_reg_data *regdata, REFCLSID rclsid, REFIID riid, void **ppv)
{
    struct host_object_params params;
    HWND apartment_hwnd = NULL;
    DWORD apartment_tid = 0;
    HRESULT hr;

    if (!multi_threaded && main_apartment)
    {
        struct apartment *host_apt = apartment_findmain();
        if (host_apt)
        {
            apartment_hwnd = apartment_getwindow(host_apt);
            apartment_release(host_apt);
        }
    }

    if (!apartment_hwnd)
    {
        EnterCriticalSection(&apt->cs);

        if (!apt->host_apt_tid)
        {
            struct host_thread_params thread_params;
            HANDLE handles[2];
            DWORD wait_value;

            thread_params.threading_model = multi_threaded ? COINIT_MULTITHREADED : COINIT_APARTMENTTHREADED;
            handles[0] = thread_params.ready_event = CreateEventW(NULL, FALSE, FALSE, NULL);
            thread_params.apartment_hwnd = NULL;
            handles[1] = CreateThread(NULL, 0, apartment_hostobject_thread, &thread_params, 0, &apt->host_apt_tid);
            if (!handles[1])
            {
                CloseHandle(handles[0]);
                LeaveCriticalSection(&apt->cs);
                return E_OUTOFMEMORY;
            }
            wait_value = WaitForMultipleObjects(2, handles, FALSE, INFINITE);
            CloseHandle(handles[0]);
            CloseHandle(handles[1]);
            if (wait_value == WAIT_OBJECT_0)
                apt->host_apt_hwnd = thread_params.apartment_hwnd;
            else
            {
                LeaveCriticalSection(&apt->cs);
                return E_OUTOFMEMORY;
            }
        }

        if (multi_threaded || !main_apartment)
        {
            apartment_hwnd = apt->host_apt_hwnd;
            apartment_tid = apt->host_apt_tid;
        }

        LeaveCriticalSection(&apt->cs);
    }

    /* another thread may have become the main apartment in the time it took
     * us to create the thread for the host apartment */
    if (!apartment_hwnd && !multi_threaded && main_apartment)
    {
        struct apartment *host_apt = apartment_findmain();
        if (host_apt)
        {
            apartment_hwnd = apartment_getwindow(host_apt);
            apartment_release(host_apt);
        }
    }

    params.regdata = *regdata;
    params.clsid = *rclsid;
    params.iid = *riid;
    hr = CreateStreamOnHGlobal(NULL, TRUE, &params.stream);
    if (FAILED(hr))
        return hr;
    params.apartment_threaded = !multi_threaded;
    if (multi_threaded)
    {
        params.hr = S_OK;
        params.event = CreateEventW(NULL, FALSE, FALSE, NULL);
        if (!PostThreadMessageW(apartment_tid, DM_HOSTOBJECT, 0, (LPARAM)&params))
            hr = E_OUTOFMEMORY;
        else
        {
            WaitForSingleObject(params.event, INFINITE);
            hr = params.hr;
        }
        CloseHandle(params.event);
    }
    else
    {
        if (!apartment_hwnd)
        {
            ERR("host apartment didn't create window\n");
            hr = E_OUTOFMEMORY;
        }
        else
            hr = SendMessageW(apartment_hwnd, DM_HOSTOBJECT, 0, (LPARAM)&params);
    }
    if (SUCCEEDED(hr))
        hr = CoUnmarshalInterface(params.stream, riid, ppv);
    IStream_Release(params.stream);
    return hr;
}

static enum comclass_threadingmodel get_threading_model(const struct class_reg_data *data)
{
    if (data->origin == CLASS_REG_REGISTRY)
    {
        WCHAR threading_model[10 /* lstrlenW(L"apartment")+1 */];
        DWORD dwLength = sizeof(threading_model);
        DWORD keytype;
        DWORD ret;

        ret = RegQueryValueExW(data->u.hkey, L"ThreadingModel", NULL, &keytype, (BYTE*)threading_model, &dwLength);
        if ((ret != ERROR_SUCCESS) || (keytype != REG_SZ))
            threading_model[0] = '\0';

        if (!wcsicmp(threading_model, L"Apartment")) return ThreadingModel_Apartment;
        if (!wcsicmp(threading_model, L"Free")) return ThreadingModel_Free;
        if (!wcsicmp(threading_model, L"Both")) return ThreadingModel_Both;

        /* there's not specific handling for this case */
        if (threading_model[0]) return ThreadingModel_Neutral;
        return ThreadingModel_No;
    }
    else
        return data->u.actctx.threading_model;
}

HRESULT apartment_get_inproc_class_object(struct apartment *apt, const struct class_reg_data *regdata,
        REFCLSID rclsid, REFIID riid, DWORD class_context, void **ppv)
{
    WCHAR dllpath[MAX_PATH+1];
    BOOL apartment_threaded;

    if (!(class_context & CLSCTX_PS_DLL))
    {
        enum comclass_threadingmodel model = get_threading_model(regdata);

        if (model == ThreadingModel_Apartment)
        {
            apartment_threaded = TRUE;
            if (apt->multi_threaded)
                return apartment_hostobject_in_hostapt(apt, FALSE, FALSE, regdata, rclsid, riid, ppv);
        }
        else if (model == ThreadingModel_Free)
        {
            apartment_threaded = FALSE;
            if (!apt->multi_threaded)
                return apartment_hostobject_in_hostapt(apt, TRUE, FALSE, regdata, rclsid, riid, ppv);
        }
        /* everything except "Apartment", "Free" and "Both" */
        else if (model != ThreadingModel_Both)
        {
            apartment_threaded = TRUE;
            /* everything else is main-threaded */
            if (model != ThreadingModel_No)
                FIXME("unrecognised threading model %d for object %s, should be main-threaded?\n", model, debugstr_guid(rclsid));

            if (apt->multi_threaded || !apt->main)
                return apartment_hostobject_in_hostapt(apt, FALSE, TRUE, regdata, rclsid, riid, ppv);
        }
        else
            apartment_threaded = FALSE;
    }
    else
        apartment_threaded = !apt->multi_threaded;

    if (!get_object_dll_path(regdata, dllpath, ARRAY_SIZE(dllpath)))
    {
        /* failure: CLSID is not found in registry */
        WARN("class %s not registered inproc\n", debugstr_guid(rclsid));
        return REGDB_E_CLASSNOTREG;
    }

    return apartment_getclassobject(apt, dllpath, apartment_threaded, rclsid, riid, ppv);
}

static HRESULT apartment_hostobject(struct apartment *apt, const struct host_object_params *params)
{
    static const LARGE_INTEGER llZero;
    WCHAR dllpath[MAX_PATH+1];
    IUnknown *object;
    HRESULT hr;

    TRACE("clsid %s, iid %s\n", debugstr_guid(&params->clsid), debugstr_guid(&params->iid));

    if (!get_object_dll_path(&params->regdata, dllpath, ARRAY_SIZE(dllpath)))
    {
        /* failure: CLSID is not found in registry */
        WARN("class %s not registered inproc\n", debugstr_guid(&params->clsid));
        return REGDB_E_CLASSNOTREG;
    }

    hr = apartment_getclassobject(apt, dllpath, params->apartment_threaded, &params->clsid, &params->iid, (void **)&object);
    if (FAILED(hr))
        return hr;

    hr = CoMarshalInterface(params->stream, &params->iid, object, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    if (FAILED(hr))
        IUnknown_Release(object);
    IStream_Seek(params->stream, llZero, STREAM_SEEK_SET, NULL);

    return hr;
}

struct dispatch_params;

static LRESULT CALLBACK apartment_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case DM_EXECUTERPC:
        rpc_execute_call((struct dispatch_params *)lParam);
        return 0;
    case DM_HOSTOBJECT:
        return apartment_hostobject(com_get_current_apt(), (const struct host_object_params *)lParam);
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

static BOOL apartment_is_model(const struct apartment *apt, DWORD model)
{
    return (apt->multi_threaded == !(model & COINIT_APARTMENTTHREADED));
}

HRESULT enter_apartment(struct tlsdata *data, DWORD model)
{
    HRESULT hr = S_OK;

    if (!data->apt)
    {
        if (!apartment_get_or_create(model))
            return E_OUTOFMEMORY;
    }
    else if (!apartment_is_model(data->apt, model))
    {
        WARN("Attempt to change threading model of this apartment from %s to %s\n",
              data->apt->multi_threaded ? "multi-threaded" : "apartment threaded",
              model & COINIT_APARTMENTTHREADED ? "apartment threaded" : "multi-threaded" );
        return RPC_E_CHANGED_MODE;
    }
    else
        hr = S_FALSE;

    data->inits++;

    return hr;
}

void leave_apartment(struct tlsdata *data)
{
    if (!--data->inits)
    {
        if (data->ole_inits)
            WARN( "Uninitializing apartment while Ole is still initialized\n" );
        apartment_release(data->apt);
        if (data->implicit_mta_cookie)
        {
            apartment_decrement_mta_usage(data->implicit_mta_cookie);
            data->implicit_mta_cookie = NULL;
        }
        data->apt = NULL;
        data->flags &= ~(OLETLS_DISABLE_OLE1DDE | OLETLS_APARTMENTTHREADED | OLETLS_MULTITHREADED);
    }
}

struct mta_cookie
{
    struct list entry;
};

HRESULT apartment_increment_mta_usage(CO_MTA_USAGE_COOKIE *cookie)
{
    struct mta_cookie *mta_cookie;

    *cookie = NULL;

    if (!(mta_cookie = malloc(sizeof(*mta_cookie))))
        return E_OUTOFMEMORY;

    EnterCriticalSection(&apt_cs);

    if (mta)
        apartment_addref(mta);
    else
        mta = apartment_construct(COINIT_MULTITHREADED);
    list_add_head(&mta->usage_cookies, &mta_cookie->entry);

    LeaveCriticalSection(&apt_cs);

    *cookie = (CO_MTA_USAGE_COOKIE)mta_cookie;

    return S_OK;
}

void apartment_decrement_mta_usage(CO_MTA_USAGE_COOKIE cookie)
{
    struct mta_cookie *mta_cookie = (struct mta_cookie *)cookie;

    EnterCriticalSection(&apt_cs);

    if (mta)
    {
        struct mta_cookie *cur;

        LIST_FOR_EACH_ENTRY(cur, &mta->usage_cookies, struct mta_cookie, entry)
        {
            if (mta_cookie == cur)
            {
                list_remove(&cur->entry);
                free(cur);
                apartment_release(mta);
                break;
            }
        }
    }

    LeaveCriticalSection(&apt_cs);
}

static const WCHAR aptwinclassW[] = L"OleMainThreadWndClass";
static ATOM apt_win_class;

static BOOL WINAPI register_class( INIT_ONCE *once, void *param, void **context )
{
    WNDCLASSW wclass;

    /* Dispatching to the correct thread in an apartment is done through
     * window messages rather than RPC transports. When an interface is
     * marshalled into another apartment in the same process, a window of the
     * following class is created. The *caller* of CoMarshalInterface (i.e., the
     * application) is responsible for pumping the message loop in that thread.
     * The WM_USER messages which point to the RPCs are then dispatched to
     * apartment_wndproc by the user's code from the apartment in which the
     * interface was unmarshalled.
     */
    memset(&wclass, 0, sizeof(wclass));
    wclass.lpfnWndProc = apartment_wndproc;
    wclass.hInstance = hProxyDll;
    wclass.lpszClassName = aptwinclassW;
    apt_win_class = RegisterClassW(&wclass);
    return TRUE;
}

/* create a window for the apartment or return the current one if one has
 * already been created */
HRESULT apartment_createwindowifneeded(struct apartment *apt)
{
    static INIT_ONCE class_init_once = INIT_ONCE_STATIC_INIT;

    if (apt->multi_threaded)
        return S_OK;

    if (!apt->win)
    {
        HWND hwnd;

        InitOnceExecuteOnce( &class_init_once, register_class, NULL, NULL );

        hwnd = CreateWindowW(aptwinclassW, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, 0, hProxyDll, NULL);
        if (!hwnd)
        {
            ERR("CreateWindow failed with error %ld\n", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
        if (InterlockedCompareExchangePointer((void **)&apt->win, hwnd, NULL))
            /* someone beat us to it */
            DestroyWindow(hwnd);
    }

    return S_OK;
}

/* retrieves the window for the main- or apartment-threaded apartment */
HWND apartment_getwindow(const struct apartment *apt)
{
    assert(!apt->multi_threaded);
    return apt->win;
}

OXID apartment_getoxid(const struct apartment *apt)
{
    return apt->oxid;
}

void apartment_global_cleanup(void)
{
    if (apt_win_class)
        UnregisterClassW((const WCHAR *)MAKEINTATOM(apt_win_class), hProxyDll);
    apartment_release_dlls();
    DeleteCriticalSection(&apt_cs);
}

HRESULT ensure_mta(void)
{
    struct apartment *apt;
    struct tlsdata *data;
    HRESULT hr;

    if (FAILED(hr = com_get_tlsdata(&data)))
        return hr;
    if ((apt = data->apt) && (data->implicit_mta_cookie || apt->multi_threaded))
        return S_OK;

    EnterCriticalSection(&apt_cs);
    if (apt || mta)
        hr = apartment_increment_mta_usage(&data->implicit_mta_cookie);
    else
        hr = CO_E_NOTINITIALIZED;
    LeaveCriticalSection(&apt_cs);

    if (FAILED(hr))
    {
        ERR("Failed, hr %#lx.\n", hr);
        return hr;
    }
    return S_OK;
}
