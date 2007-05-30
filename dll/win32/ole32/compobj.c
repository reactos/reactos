/*
 *	COMPOBJ library
 *
 *	Copyright 1995	Martin von Loewis
 *	Copyright 1998	Justin Bradford
 *      Copyright 1999  Francis Beaudet
 *      Copyright 1999  Sylvain St-Germain
 *      Copyright 2002  Marcus Meissner
 *      Copyright 2004  Mike Hearn
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
 * Note
 * 1. COINIT_MULTITHREADED is 0; it is the lack of COINIT_APARTMENTTHREADED
 *    Therefore do not test against COINIT_MULTITHREADED
 *
 * TODO list:           (items bunched together depend on each other)
 *
 *   - Implement the service control manager (in rpcss) to keep track
 *     of registered class objects: ISCM::ServerRegisterClsid et al
 *   - Implement the OXID resolver so we don't need magic endpoint names for
 *     clients and servers to meet up
 *
 *   - Make all ole interface marshaling use NDR to be wire compatible with
 *     native DCOM
 *
 */

#include "config.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"

#include "compobj_private.h"

#include "wine/unicode.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HINSTANCE OLE32_hInstance = 0; /* FIXME: make static ... */

#define ARRAYSIZE(array) (sizeof(array)/sizeof((array)[0]))

/****************************************************************************
 * This section defines variables internal to the COM module.
 *
 * TODO: Most of these things will have to be made thread-safe.
 */

static HRESULT COM_GetRegisteredClassObject(REFCLSID rclsid, DWORD dwClsContext, LPUNKNOWN*  ppUnk);
static void COM_RevokeAllClasses(void);
static HRESULT get_inproc_class_object(HKEY hkeydll, REFCLSID rclsid, REFIID riid, void **ppv);

static APARTMENT *MTA; /* protected by csApartment */
static APARTMENT *MainApartment; /* the first STA apartment */
static struct list apts = LIST_INIT( apts ); /* protected by csApartment */

static CRITICAL_SECTION csApartment;
static CRITICAL_SECTION_DEBUG critsect_debug =
{
    0, 0, &csApartment,
    { &critsect_debug.ProcessLocksList, &critsect_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csApartment") }
};
static CRITICAL_SECTION csApartment = { &critsect_debug, -1, 0, 0, 0, 0 };

struct registered_psclsid
{
    struct list entry;
    IID iid;
    CLSID clsid;
};

/*
 * This lock count counts the number of times CoInitialize is called. It is
 * decreased every time CoUninitialize is called. When it hits 0, the COM
 * libraries are freed
 */
static LONG s_COMLockCount = 0;
/* Reference count used by CoAddRefServerProcess/CoReleaseServerProcess */
static LONG s_COMServerProcessReferences = 0;

/*
 * This linked list contains the list of registered class objects. These
 * are mostly used to register the factories for out-of-proc servers of OLE
 * objects.
 *
 * TODO: Make this data structure aware of inter-process communication. This
 *       means that parts of this will be exported to the Wine Server.
 */
typedef struct tagRegisteredClass
{
  struct list entry;
  CLSID     classIdentifier;
  LPUNKNOWN classObject;
  DWORD     runContext;
  DWORD     connectFlags;
  DWORD     dwCookie;
  LPSTREAM  pMarshaledData; /* FIXME: only really need to store OXID and IPID */
  void     *RpcRegistration;
} RegisteredClass;

static struct list RegisteredClassList = LIST_INIT(RegisteredClassList);

static CRITICAL_SECTION csRegisteredClassList;
static CRITICAL_SECTION_DEBUG class_cs_debug =
{
    0, 0, &csRegisteredClassList,
    { &class_cs_debug.ProcessLocksList, &class_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csRegisteredClassList") }
};
static CRITICAL_SECTION csRegisteredClassList = { &class_cs_debug, -1, 0, 0, 0, 0 };

/*****************************************************************************
 * This section contains OpenDllList definitions
 *
 * The OpenDllList contains only handles of dll loaded by CoGetClassObject or
 * other functions that do LoadLibrary _without_ giving back a HMODULE.
 * Without this list these handles would never be freed.
 *
 * FIXME: a DLL that says OK when asked for unloading is unloaded in the
 * next unload-call but not before 600 sec.
 */

typedef HRESULT (CALLBACK *DllGetClassObjectFunc)(REFCLSID clsid, REFIID iid, LPVOID *ppv);
typedef HRESULT (WINAPI *DllCanUnloadNowFunc)(void);

typedef struct tagOpenDll
{
  LONG refs;
  LPWSTR library_name;
  HANDLE library;
  DllGetClassObjectFunc DllGetClassObject;
  DllCanUnloadNowFunc DllCanUnloadNow;
  struct list entry;
} OpenDll;

static struct list openDllList = LIST_INIT(openDllList);

static CRITICAL_SECTION csOpenDllList;
static CRITICAL_SECTION_DEBUG dll_cs_debug =
{
    0, 0, &csOpenDllList,
    { &dll_cs_debug.ProcessLocksList, &dll_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": csOpenDllList") }
};
static CRITICAL_SECTION csOpenDllList = { &dll_cs_debug, -1, 0, 0, 0, 0 };

struct apartment_loaded_dll
{
    struct list entry;
    OpenDll *dll;
};

static const WCHAR wszAptWinClass[] = {'O','l','e','M','a','i','n','T','h','r','e','a','d','W','n','d','C','l','a','s','s',' ',
                                       '0','x','#','#','#','#','#','#','#','#',' ',0};
static LRESULT CALLBACK apartment_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static HRESULT apartment_getclassobject(struct apartment *apt, LPCWSTR dllpath,
                                        REFCLSID rclsid, REFIID riid, void **ppv);

static HRESULT COMPOBJ_DllList_Add(LPCWSTR library_name, OpenDll **ret);
static OpenDll *COMPOBJ_DllList_Get(LPCWSTR library_name);
static void COMPOBJ_DllList_ReleaseRef(OpenDll *entry);

static DWORD COM_RegReadPath(HKEY hkeyroot, const WCHAR *keyname, const WCHAR *valuename, WCHAR * dst, DWORD dstlen);

static void COMPOBJ_InitProcess( void )
{
    WNDCLASSW wclass;

    /* Dispatching to the correct thread in an apartment is done through
     * window messages rather than RPC transports. When an interface is
     * marshalled into another apartment in the same process, a window of the
     * following class is created. The *caller* of CoMarshalInterface (ie the
     * application) is responsible for pumping the message loop in that thread.
     * The WM_USER messages which point to the RPCs are then dispatched to
     * COM_AptWndProc by the user's code from the apartment in which the interface
     * was unmarshalled.
     */
    memset(&wclass, 0, sizeof(wclass));
    wclass.lpfnWndProc = apartment_wndproc;
    wclass.hInstance = OLE32_hInstance;
    wclass.lpszClassName = wszAptWinClass;
    RegisterClassW(&wclass);
}

static void COMPOBJ_UninitProcess( void )
{
    UnregisterClassW(wszAptWinClass, OLE32_hInstance);
}

static void COM_TlsDestroy(void)
{
    struct oletls *info = NtCurrentTeb()->ReservedForOle;
    if (info)
    {
        if (info->apt) apartment_release(info->apt);
        if (info->errorinfo) IErrorInfo_Release(info->errorinfo);
        if (info->state) IUnknown_Release(info->state);
        HeapFree(GetProcessHeap(), 0, info);
        NtCurrentTeb()->ReservedForOle = NULL;
    }
}

/******************************************************************************
 * Manage apartments.
 */

/* allocates memory and fills in the necessary fields for a new apartment
 * object. must be called inside apartment cs */
static APARTMENT *apartment_construct(DWORD model)
{
    APARTMENT *apt;

    TRACE("creating new apartment, model=%d\n", model);

    apt = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*apt));
    apt->tid = GetCurrentThreadId();

    list_init(&apt->proxies);
    list_init(&apt->stubmgrs);
    list_init(&apt->psclsids);
    list_init(&apt->loaded_dlls);
    apt->ipidc = 0;
    apt->refs = 1;
    apt->remunk_exported = FALSE;
    apt->oidc = 1;
    InitializeCriticalSection(&apt->cs);
    DEBUG_SET_CRITSEC_NAME(&apt->cs, "apartment");

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

/* gets and existing apartment if one exists or otherwise creates an apartment
 * structure which stores OLE apartment-local information and stores a pointer
 * to it in the thread-local storage */
static APARTMENT *apartment_get_or_create(DWORD model)
{
    APARTMENT *apt = COM_CurrentApt();

    if (!apt)
    {
        if (model & COINIT_APARTMENTTHREADED)
        {
            EnterCriticalSection(&csApartment);

            apt = apartment_construct(model);
            if (!MainApartment)
            {
                MainApartment = apt;
                apt->main = TRUE;
                TRACE("Created main-threaded apartment with OXID %s\n", wine_dbgstr_longlong(apt->oxid));
            }

            LeaveCriticalSection(&csApartment);
        }
        else
        {
            EnterCriticalSection(&csApartment);

            /* The multi-threaded apartment (MTA) contains zero or more threads interacting
             * with free threaded (ie thread safe) COM objects. There is only ever one MTA
             * in a process */
            if (MTA)
            {
                TRACE("entering the multithreaded apartment %s\n", wine_dbgstr_longlong(MTA->oxid));
                apartment_addref(MTA);
            }
            else
                MTA = apartment_construct(model);

            apt = MTA;

            LeaveCriticalSection(&csApartment);
        }
        COM_CurrentInfo()->apt = apt;
    }

    return apt;
}

static inline BOOL apartment_is_model(APARTMENT *apt, DWORD model)
{
    return (apt->multi_threaded == !(model & COINIT_APARTMENTTHREADED));
}

DWORD apartment_addref(struct apartment *apt)
{
    DWORD refs = InterlockedIncrement(&apt->refs);
    TRACE("%s: before = %d\n", wine_dbgstr_longlong(apt->oxid), refs - 1);
    return refs;
}

DWORD apartment_release(struct apartment *apt)
{
    DWORD ret;

    EnterCriticalSection(&csApartment);

    ret = InterlockedDecrement(&apt->refs);
    TRACE("%s: after = %d\n", wine_dbgstr_longlong(apt->oxid), ret);
    /* destruction stuff that needs to happen under csApartment CS */
    if (ret == 0)
    {
        if (apt == MTA) MTA = NULL;
        else if (apt == MainApartment) MainApartment = NULL;
        list_remove(&apt->entry);
    }

    LeaveCriticalSection(&csApartment);

    if (ret == 0)
    {
        struct list *cursor, *cursor2;

        TRACE("destroying apartment %p, oxid %s\n", apt, wine_dbgstr_longlong(apt->oxid));

        /* no locking is needed for this apartment, because no other thread
         * can access it at this point */

        apartment_disconnectproxies(apt);

        if (apt->win) DestroyWindow(apt->win);

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

        LIST_FOR_EACH_SAFE(cursor, cursor2, &apt->psclsids)
        {
            struct registered_psclsid *registered_psclsid =
                LIST_ENTRY(cursor, struct registered_psclsid, entry);

            list_remove(&registered_psclsid->entry);
            HeapFree(GetProcessHeap(), 0, registered_psclsid);
        }

        /* if this assert fires, then another thread took a reference to a
         * stub manager without taking a reference to the containing
         * apartment, which it must do. */
        assert(list_empty(&apt->stubmgrs));

        if (apt->filter) IUnknown_Release(apt->filter);

        while ((cursor = list_head(&apt->loaded_dlls)))
        {
            struct apartment_loaded_dll *apartment_loaded_dll = LIST_ENTRY(cursor, struct apartment_loaded_dll, entry);
            COMPOBJ_DllList_ReleaseRef(apartment_loaded_dll->dll);
            list_remove(cursor);
            HeapFree(GetProcessHeap(), 0, apartment_loaded_dll);
        }

        DEBUG_CLEAR_CRITSEC_NAME(&apt->cs);
        DeleteCriticalSection(&apt->cs);

        HeapFree(GetProcessHeap(), 0, apt);
    }

    return ret;
}

/* The given OXID must be local to this process: 
 *
 * The ref parameter is here mostly to ensure people remember that
 * they get one, you should normally take a ref for thread safety.
 */
APARTMENT *apartment_findfromoxid(OXID oxid, BOOL ref)
{
    APARTMENT *result = NULL;
    struct list *cursor;

    EnterCriticalSection(&csApartment);
    LIST_FOR_EACH( cursor, &apts )
    {
        struct apartment *apt = LIST_ENTRY( cursor, struct apartment, entry );
        if (apt->oxid == oxid)
        {
            result = apt;
            if (ref) apartment_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&csApartment);

    return result;
}

/* gets the apartment which has a given creator thread ID. The caller must
 * release the reference from the apartment as soon as the apartment pointer
 * is no longer required. */
APARTMENT *apartment_findfromtid(DWORD tid)
{
    APARTMENT *result = NULL;
    struct list *cursor;

    EnterCriticalSection(&csApartment);
    LIST_FOR_EACH( cursor, &apts )
    {
        struct apartment *apt = LIST_ENTRY( cursor, struct apartment, entry );
        if (apt->tid == tid)
        {
            result = apt;
            apartment_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&csApartment);

    return result;
}

/* gets an apartment which has a given type. The caller must
 * release the reference from the apartment as soon as the apartment pointer
 * is no longer required. */
static APARTMENT *apartment_findfromtype(BOOL multi_threaded, BOOL main_apartment)
{
    APARTMENT *result = NULL;
    struct apartment *apt;

    EnterCriticalSection(&csApartment);

    if (!multi_threaded && main_apartment)
    {
        result = MainApartment;
        if (result) apartment_addref(result);
        LeaveCriticalSection(&csApartment);
        return result;
    }

    LIST_FOR_EACH_ENTRY( apt, &apts, struct apartment, entry )
    {
        if (apt->multi_threaded == multi_threaded)
        {
            result = apt;
            apartment_addref(result);
            break;
        }
    }
    LeaveCriticalSection(&csApartment);

    return result;
}

struct host_object_params
{
    HKEY hkeydll;
    CLSID clsid; /* clsid of object to marshal */
    IID iid; /* interface to marshal */
    IStream *stream; /* stream that the object will be marshaled into */
};

static HRESULT apartment_hostobject(struct apartment *apt,
                                    const struct host_object_params *params)
{
    IUnknown *object;
    HRESULT hr;
    static const LARGE_INTEGER llZero;
    WCHAR dllpath[MAX_PATH+1];

    TRACE("\n");

    if (COM_RegReadPath(params->hkeydll, NULL, NULL, dllpath, ARRAYSIZE(dllpath)) != ERROR_SUCCESS)
    {
        /* failure: CLSID is not found in registry */
        WARN("class %s not registered inproc\n", debugstr_guid(&params->clsid));
        return REGDB_E_CLASSNOTREG;
    }

    hr = apartment_getclassobject(apt, dllpath, &params->clsid, &params->iid, (void **)&object);
    if (FAILED(hr))
        return hr;

    hr = CoMarshalInterface(params->stream, &params->iid, object, MSHCTX_INPROC, NULL, MSHLFLAGS_NORMAL);
    if (FAILED(hr))
        IUnknown_Release(object);
    IStream_Seek(params->stream, llZero, STREAM_SEEK_SET, NULL);

    return hr;
}

static LRESULT CALLBACK apartment_wndproc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
    case DM_EXECUTERPC:
        RPC_ExecuteCall((struct dispatch_params *)lParam);
        return 0;
    case DM_HOSTOBJECT:
        return apartment_hostobject(COM_CurrentApt(), (const struct host_object_params *)lParam);
    default:
        return DefWindowProcW(hWnd, msg, wParam, lParam);
    }
}

HRESULT apartment_createwindowifneeded(struct apartment *apt)
{
    if (apt->multi_threaded)
        return S_OK;

    if (!apt->win)
    {
        HWND hwnd = CreateWindowW(wszAptWinClass, NULL, 0,
                                  0, 0, 0, 0,
                                  0, 0, OLE32_hInstance, NULL);
        if (!hwnd)
        {
            ERR("CreateWindow failed with error %d\n", GetLastError());
            return HRESULT_FROM_WIN32(GetLastError());
        }
        if (InterlockedCompareExchangePointer((PVOID *)&apt->win, hwnd, NULL))
            /* someone beat us to it */
            DestroyWindow(hwnd);
    }

    return S_OK;
}

HWND apartment_getwindow(struct apartment *apt)
{
    assert(!apt->multi_threaded);
    return apt->win;
}

void apartment_joinmta(void)
{
    apartment_addref(MTA);
    COM_CurrentInfo()->apt = MTA;
}

static HRESULT apartment_getclassobject(struct apartment *apt, LPCWSTR dllpath,
                                        REFCLSID rclsid, REFIID riid, void **ppv)
{
    HRESULT hr = S_OK;
    BOOL found = FALSE;
    struct apartment_loaded_dll *apartment_loaded_dll;

    EnterCriticalSection(&apt->cs);

    LIST_FOR_EACH_ENTRY(apartment_loaded_dll, &apt->loaded_dlls, struct apartment_loaded_dll, entry)
        if (!strcmpiW(dllpath, apartment_loaded_dll->dll->library_name))
        {
            TRACE("found %s already loaded\n", debugstr_w(dllpath));
            found = TRUE;
            break;
        }

    if (!found)
    {
        apartment_loaded_dll = HeapAlloc(GetProcessHeap(), 0, sizeof(*apartment_loaded_dll));
        if (!apartment_loaded_dll)
            hr = E_OUTOFMEMORY;
        if (SUCCEEDED(hr))
        {
            hr = COMPOBJ_DllList_Add( dllpath, &apartment_loaded_dll->dll );
            if (FAILED(hr))
                HeapFree(GetProcessHeap(), 0, apartment_loaded_dll);
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
        TRACE("calling DllGetClassObject %p\n", apartment_loaded_dll->dll->DllGetClassObject);
        /* OK: get the ClassObject */
        hr = apartment_loaded_dll->dll->DllGetClassObject(rclsid, riid, ppv);

        if (hr != S_OK)
            ERR("DllGetClassObject returned error 0x%08x\n", hr);
    }

    return hr;
}

static void apartment_freeunusedlibraries(struct apartment *apt)
{
    struct apartment_loaded_dll *entry, *next;
    EnterCriticalSection(&apt->cs);
    LIST_FOR_EACH_ENTRY_SAFE(entry, next, &apt->loaded_dlls, struct apartment_loaded_dll, entry)
    {
	if (entry->dll->DllCanUnloadNow && (entry->dll->DllCanUnloadNow() == S_OK))
        {
            list_remove(&entry->entry);
            COMPOBJ_DllList_ReleaseRef(entry->dll);
            HeapFree(GetProcessHeap(), 0, entry);
        }
    }
    LeaveCriticalSection(&apt->cs);
}

/*****************************************************************************
 * This section contains OpenDllList implementation
 */

/* caller must ensure that library_name is not already in the open dll list */
static HRESULT COMPOBJ_DllList_Add(LPCWSTR library_name, OpenDll **ret)
{
    OpenDll *entry;
    int len;
    HRESULT hr = S_OK;
    HANDLE hLibrary;
    DllCanUnloadNowFunc DllCanUnloadNow;
    DllGetClassObjectFunc DllGetClassObject;

    TRACE("\n");

    *ret = COMPOBJ_DllList_Get(library_name);
    if (*ret) return S_OK;

    /* do this outside the csOpenDllList to avoid creating a lock dependency on
     * the loader lock */
    hLibrary = LoadLibraryExW(library_name, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!hLibrary)
    {
        ERR("couldn't load in-process dll %s\n", debugstr_w(library_name));
        /* failure: DLL could not be loaded */
        return E_ACCESSDENIED; /* FIXME: or should this be CO_E_DLLNOTFOUND? */
    }

    DllCanUnloadNow = GetProcAddress(hLibrary, "DllCanUnloadNow");
    /* Note: failing to find DllCanUnloadNow is not a failure */
    DllGetClassObject = GetProcAddress(hLibrary, "DllGetClassObject");
    if (!DllGetClassObject)
    {
        /* failure: the dll did not export DllGetClassObject */
        ERR("couldn't find function DllGetClassObject in %s\n", debugstr_w(library_name));
        FreeLibrary(hLibrary);
        return CO_E_DLLNOTFOUND;
    }

    EnterCriticalSection( &csOpenDllList );

    *ret = COMPOBJ_DllList_Get(library_name);
    if (*ret)
    {
        /* another caller to this function already added the dll while we
         * weren't in the critical section */
        FreeLibrary(hLibrary);
    }
    else
    {
        len = strlenW(library_name);
        entry = HeapAlloc(GetProcessHeap(),0, sizeof(OpenDll));
        if (entry)
            entry->library_name = HeapAlloc(GetProcessHeap(), 0, (len + 1)*sizeof(WCHAR));
        if (entry && entry->library_name)
        {
            memcpy(entry->library_name, library_name, (len + 1)*sizeof(WCHAR));
            entry->library = hLibrary;
            entry->refs = 1;
            entry->DllCanUnloadNow = DllCanUnloadNow;
            entry->DllGetClassObject = DllGetClassObject;
            list_add_tail(&openDllList, &entry->entry);
        }
        else
        {
            hr = E_OUTOFMEMORY;
            FreeLibrary(hLibrary);
        }
        *ret = entry;
    }

    LeaveCriticalSection( &csOpenDllList );

    return hr;
}

static OpenDll *COMPOBJ_DllList_Get(LPCWSTR library_name)
{
    OpenDll *ptr;
    OpenDll *ret = NULL;
    EnterCriticalSection(&csOpenDllList);
    LIST_FOR_EACH_ENTRY(ptr, &openDllList, OpenDll, entry)
    {
        if (!strcmpiW(library_name, ptr->library_name) &&
            (InterlockedIncrement(&ptr->refs) != 1) /* entry is being destroy if == 1 */)
        {
            ret = ptr;
            break;
        }
    }
    LeaveCriticalSection(&csOpenDllList);
    return ret;
}

static void COMPOBJ_DllList_ReleaseRef(OpenDll *entry)
{
    if (!InterlockedDecrement(&entry->refs))
    {
        EnterCriticalSection(&csOpenDllList);
        list_remove(&entry->entry);
        LeaveCriticalSection(&csOpenDllList);

        TRACE("freeing %p\n", entry->library);
        FreeLibrary(entry->library);

        HeapFree(GetProcessHeap(), 0, entry->library_name);
        HeapFree(GetProcessHeap(), 0, entry);
    }
}

/******************************************************************************
 *           CoBuildVersion [OLE32.@]
 *           CoBuildVersion [COMPOBJ.1]
 *
 * Gets the build version of the DLL.
 *
 * PARAMS
 *
 * RETURNS
 *	Current build version, hiword is majornumber, loword is minornumber
 */
DWORD WINAPI CoBuildVersion(void)
{
    TRACE("Returning version %d, build %d.\n", rmm, rup);
    return (rmm<<16)+rup;
}

/******************************************************************************
 *		CoInitialize	[OLE32.@]
 *
 * Initializes the COM libraries by calling CoInitializeEx with
 * COINIT_APARTMENTTHREADED, ie it enters a STA thread.
 *
 * PARAMS
 *  lpReserved [I] Pointer to IMalloc interface (obsolete, should be NULL).
 *
 * RETURNS
 *  Success: S_OK if not already initialized, S_FALSE otherwise.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *   CoInitializeEx
 */
HRESULT WINAPI CoInitialize(LPVOID lpReserved)
{
  /*
   * Just delegate to the newer method.
   */
  return CoInitializeEx(lpReserved, COINIT_APARTMENTTHREADED);
}

/******************************************************************************
 *		CoInitializeEx	[OLE32.@]
 *
 * Initializes the COM libraries.
 *
 * PARAMS
 *  lpReserved [I] Pointer to IMalloc interface (obsolete, should be NULL).
 *  dwCoInit   [I] One or more flags from the COINIT enumeration. See notes.
 *
 * RETURNS
 *  S_OK               if successful,
 *  S_FALSE            if this function was called already.
 *  RPC_E_CHANGED_MODE if a previous call to CoInitializeEx specified another
 *                     threading model.
 *
 * NOTES
 *
 * The behavior used to set the IMalloc used for memory management is
 * obsolete.
 * The dwCoInit parameter must specify one of the following apartment
 * threading models:
 *| COINIT_APARTMENTTHREADED - A single-threaded apartment (STA).
 *| COINIT_MULTITHREADED - A multi-threaded apartment (MTA).
 * The parameter may also specify zero or more of the following flags:
 *| COINIT_DISABLE_OLE1DDE - Don't use DDE for OLE1 support.
 *| COINIT_SPEED_OVER_MEMORY - Trade memory for speed.
 *
 * SEE ALSO
 *   CoUninitialize
 */
HRESULT WINAPI CoInitializeEx(LPVOID lpReserved, DWORD dwCoInit)
{
  HRESULT hr = S_OK;
  APARTMENT *apt;

  TRACE("(%p, %x)\n", lpReserved, (int)dwCoInit);

  if (lpReserved!=NULL)
  {
    ERR("(%p, %x) - Bad parameter passed-in %p, must be an old Windows Application\n", lpReserved, (int)dwCoInit, lpReserved);
  }

  /*
   * Check the lock count. If this is the first time going through the initialize
   * process, we have to initialize the libraries.
   *
   * And crank-up that lock count.
   */
  if (InterlockedExchangeAdd(&s_COMLockCount,1)==0)
  {
    /*
     * Initialize the various COM libraries and data structures.
     */
    TRACE("() - Initializing the COM libraries\n");

    /* we may need to defer this until after apartment initialisation */
    RunningObjectTableImpl_Initialize();
  }

  if (!(apt = COM_CurrentInfo()->apt))
  {
    apt = apartment_get_or_create(dwCoInit);
    if (!apt) return E_OUTOFMEMORY;
  }
  else if (!apartment_is_model(apt, dwCoInit))
  {
    /* Changing the threading model after it's been set is illegal. If this warning is triggered by Wine
       code then we are probably using the wrong threading model to implement that API. */
    ERR("Attempt to change threading model of this apartment from %s to %s\n",
        apt->multi_threaded ? "multi-threaded" : "apartment threaded",
        dwCoInit & COINIT_APARTMENTTHREADED ? "apartment threaded" : "multi-threaded");
    return RPC_E_CHANGED_MODE;
  }
  else
    hr = S_FALSE;

  COM_CurrentInfo()->inits++;

  return hr;
}

/* On COM finalization for a STA thread, the message queue is flushed to ensure no
   pending RPCs are ignored. Non-COM messages are discarded at this point.
 */
static void COM_FlushMessageQueue(void)
{
    MSG message;
    APARTMENT *apt = COM_CurrentApt();

    if (!apt || !apt->win) return;

    TRACE("Flushing STA message queue\n");

    while (PeekMessageA(&message, NULL, 0, 0, PM_REMOVE))
    {
        if (message.hwnd != apt->win)
        {
            WARN("discarding message 0x%x for window %p\n", message.message, message.hwnd);
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageA(&message);
    }
}

/***********************************************************************
 *           CoUninitialize   [OLE32.@]
 *
 * This method will decrement the refcount on the current apartment, freeing
 * the resources associated with it if it is the last thread in the apartment.
 * If the last apartment is freed, the function will additionally release
 * any COM resources associated with the process.
 *
 * PARAMS
 *
 * RETURNS
 *  Nothing.
 *
 * SEE ALSO
 *   CoInitializeEx
 */
void WINAPI CoUninitialize(void)
{
  struct oletls * info = COM_CurrentInfo();
  LONG lCOMRefCnt;

  TRACE("()\n");

  /* will only happen on OOM */
  if (!info) return;

  /* sanity check */
  if (!info->inits)
  {
    ERR("Mismatched CoUninitialize\n");
    return;
  }

  if (!--info->inits)
  {
    apartment_release(info->apt);
    info->apt = NULL;
  }

  /*
   * Decrease the reference count.
   * If we are back to 0 locks on the COM library, make sure we free
   * all the associated data structures.
   */
  lCOMRefCnt = InterlockedExchangeAdd(&s_COMLockCount,-1);
  if (lCOMRefCnt==1)
  {
    TRACE("() - Releasing the COM libraries\n");

    RunningObjectTableImpl_UnInitialize();

    /* Release the references to the registered class objects */
    COM_RevokeAllClasses();

    /* This will free the loaded COM Dlls  */
    CoFreeAllLibraries();

    /* This ensures we deal with any pending RPCs */
    COM_FlushMessageQueue();
  }
  else if (lCOMRefCnt<1) {
    ERR( "CoUninitialize() - not CoInitialized.\n" );
    InterlockedExchangeAdd(&s_COMLockCount,1); /* restore the lock count. */
  }
}

/******************************************************************************
 *		CoDisconnectObject	[OLE32.@]
 *
 * Disconnects all connections to this object from remote processes. Dispatches
 * pending RPCs while blocking new RPCs from occurring, and then calls
 * IMarshal::DisconnectObject on the given object.
 *
 * Typically called when the object server is forced to shut down, for instance by
 * the user.
 *
 * PARAMS
 *  lpUnk    [I] The object whose stub should be disconnected.
 *  reserved [I] Reserved. Should be set to 0.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoMarshalInterface, CoReleaseMarshalData, CoLockObjectExternal
 */
HRESULT WINAPI CoDisconnectObject( LPUNKNOWN lpUnk, DWORD reserved )
{
    HRESULT hr;
    IMarshal *marshal;
    APARTMENT *apt;

    TRACE("(%p, 0x%08x)\n", lpUnk, reserved);

    hr = IUnknown_QueryInterface(lpUnk, &IID_IMarshal, (void **)&marshal);
    if (hr == S_OK)
    {
        hr = IMarshal_DisconnectObject(marshal, reserved);
        IMarshal_Release(marshal);
        return hr;
    }

    apt = COM_CurrentApt();
    if (!apt)
        return CO_E_NOTINITIALIZED;

    apartment_disconnectobject(apt, lpUnk);

    /* Note: native is pretty broken here because it just silently
     * fails, without returning an appropriate error code if the object was
     * not found, making apps think that the object was disconnected, when
     * it actually wasn't */

    return S_OK;
}

/******************************************************************************
 *		CoCreateGuid [OLE32.@]
 *
 * Simply forwards to UuidCreate in RPCRT4.
 *
 * PARAMS
 *  pguid [O] Points to the GUID to initialize.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *   UuidCreate
 */
HRESULT WINAPI CoCreateGuid(GUID *pguid)
{
    return UuidCreate(pguid);
}

/******************************************************************************
 *		CLSIDFromString	[OLE32.@]
 *		IIDFromString   [OLE32.@]
 *
 * Converts a unique identifier from its string representation into
 * the GUID struct.
 *
 * PARAMS
 *  idstr [I] The string representation of the GUID.
 *  id    [O] GUID converted from the string.
 *
 * RETURNS
 *   S_OK on success
 *   CO_E_CLASSSTRING if idstr is not a valid CLSID
 *
 * SEE ALSO
 *  StringFromCLSID
 */
static HRESULT WINAPI __CLSIDFromString(LPCWSTR s, CLSID *id)
{
  int	i;
  BYTE table[256];

  if (!s) {
    memset( id, 0, sizeof (CLSID) );
    return S_OK;
  }

  /* validate the CLSID string */
  if (strlenW(s) != 38)
    return CO_E_CLASSSTRING;

  if ((s[0]!='{') || (s[9]!='-') || (s[14]!='-') || (s[19]!='-') || (s[24]!='-') || (s[37]!='}'))
    return CO_E_CLASSSTRING;

  for (i=1; i<37; i++) {
    if ((i == 9)||(i == 14)||(i == 19)||(i == 24)) continue;
    if (!(((s[i] >= '0') && (s[i] <= '9'))  ||
          ((s[i] >= 'a') && (s[i] <= 'f'))  ||
          ((s[i] >= 'A') && (s[i] <= 'F'))))
       return CO_E_CLASSSTRING;
  }

  TRACE("%s -> %p\n", debugstr_w(s), id);

  /* quick lookup table */
  memset(table, 0, 256);

  for (i = 0; i < 10; i++) {
    table['0' + i] = i;
  }
  for (i = 0; i < 6; i++) {
    table['A' + i] = i+10;
    table['a' + i] = i+10;
  }

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

  id->Data1 = (table[s[1]] << 28 | table[s[2]] << 24 | table[s[3]] << 20 | table[s[4]] << 16 |
               table[s[5]] << 12 | table[s[6]] << 8  | table[s[7]] << 4  | table[s[8]]);
  id->Data2 = table[s[10]] << 12 | table[s[11]] << 8 | table[s[12]] << 4 | table[s[13]];
  id->Data3 = table[s[15]] << 12 | table[s[16]] << 8 | table[s[17]] << 4 | table[s[18]];

  /* these are just sequential bytes */
  id->Data4[0] = table[s[20]] << 4 | table[s[21]];
  id->Data4[1] = table[s[22]] << 4 | table[s[23]];
  id->Data4[2] = table[s[25]] << 4 | table[s[26]];
  id->Data4[3] = table[s[27]] << 4 | table[s[28]];
  id->Data4[4] = table[s[29]] << 4 | table[s[30]];
  id->Data4[5] = table[s[31]] << 4 | table[s[32]];
  id->Data4[6] = table[s[33]] << 4 | table[s[34]];
  id->Data4[7] = table[s[35]] << 4 | table[s[36]];

  return S_OK;
}

/*****************************************************************************/

HRESULT WINAPI CLSIDFromString(LPOLESTR idstr, CLSID *id )
{
    HRESULT ret;

    if (!id)
        return E_INVALIDARG;

    ret = __CLSIDFromString(idstr, id);
    if(ret != S_OK) { /* It appears a ProgID is also valid */
        ret = CLSIDFromProgID(idstr, id);
    }
    return ret;
}

/* Converts a GUID into the respective string representation. */
HRESULT WINE_StringFromCLSID(
	const CLSID *id,	/* [in] GUID to be converted */
	LPSTR idstr		/* [out] pointer to buffer to contain converted guid */
) {
  static const char hex[] = "0123456789ABCDEF";
  char *s;
  int	i;

  if (!id)
	{ ERR("called with id=Null\n");
	  *idstr = 0x00;
	  return E_FAIL;
	}

  sprintf(idstr, "{%08X-%04X-%04X-%02X%02X-",
	  id->Data1, id->Data2, id->Data3,
	  id->Data4[0], id->Data4[1]);
  s = &idstr[25];

  /* 6 hex bytes */
  for (i = 2; i < 8; i++) {
    *s++ = hex[id->Data4[i]>>4];
    *s++ = hex[id->Data4[i] & 0xf];
  }

  *s++ = '}';
  *s++ = '\0';

  TRACE("%p->%s\n", id, idstr);

  return S_OK;
}


/******************************************************************************
 *		StringFromCLSID	[OLE32.@]
 *		StringFromIID   [OLE32.@]
 *
 * Converts a GUID into the respective string representation.
 * The target string is allocated using the OLE IMalloc.
 *
 * PARAMS
 *  id    [I] the GUID to be converted.
 *  idstr [O] A pointer to a to-be-allocated pointer pointing to the resulting string.
 *
 * RETURNS
 *   S_OK
 *   E_FAIL
 *
 * SEE ALSO
 *  StringFromGUID2, CLSIDFromString
 */
HRESULT WINAPI StringFromCLSID(REFCLSID id, LPOLESTR *idstr)
{
	char            buf[80];
	HRESULT       ret;
	LPMALLOC	mllc;

	if ((ret = CoGetMalloc(0,&mllc)))
		return ret;

	ret=WINE_StringFromCLSID(id,buf);
	if (!ret) {
            DWORD len = MultiByteToWideChar( CP_ACP, 0, buf, -1, NULL, 0 );
            *idstr = IMalloc_Alloc( mllc, len * sizeof(WCHAR) );
            MultiByteToWideChar( CP_ACP, 0, buf, -1, *idstr, len );
	}
	return ret;
}

/******************************************************************************
 *		StringFromGUID2	[OLE32.@]
 *		StringFromGUID2	[COMPOBJ.76]
 *
 * Modified version of StringFromCLSID that allows you to specify max
 * buffer size.
 *
 * PARAMS
 *  id   [I] GUID to convert to string.
 *  str  [O] Buffer where the result will be stored.
 *  cmax [I] Size of the buffer in characters.
 *
 * RETURNS
 *	Success: The length of the resulting string in characters.
 *  Failure: 0.
 */
INT WINAPI StringFromGUID2(REFGUID id, LPOLESTR str, INT cmax)
{
  char		xguid[80];

  if (WINE_StringFromCLSID(id,xguid))
  	return 0;
  return MultiByteToWideChar( CP_ACP, 0, xguid, -1, str, cmax );
}

/* open HKCR\\CLSID\\{string form of clsid}\\{keyname} key */
HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *subkey)
{
    static const WCHAR wszCLSIDSlash[] = {'C','L','S','I','D','\\',0};
    WCHAR path[CHARS_IN_GUID + ARRAYSIZE(wszCLSIDSlash) - 1];
    LONG res;
    HKEY key;

    strcpyW(path, wszCLSIDSlash);
    StringFromGUID2(clsid, path + strlenW(wszCLSIDSlash), CHARS_IN_GUID);
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, keyname ? KEY_READ : access, &key);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_CLASSNOTREG;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    if (!keyname)
    {
        *subkey = key;
        return S_OK;
    }

    res = RegOpenKeyExW(key, keyname, 0, access, subkey);
    RegCloseKey(key);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    return S_OK;
}

/* open HKCR\\AppId\\{string form of appid clsid} key */
HRESULT COM_OpenKeyForAppIdFromCLSID(REFCLSID clsid, REGSAM access, HKEY *subkey)
{
    static const WCHAR szAppId[] = { 'A','p','p','I','d',0 };
    static const WCHAR szAppIdKey[] = { 'A','p','p','I','d','\\',0 };
    DWORD res;
    WCHAR buf[CHARS_IN_GUID];
    WCHAR keyname[ARRAYSIZE(szAppIdKey) + CHARS_IN_GUID];
    DWORD size;
    HKEY hkey;
    DWORD type;
    HRESULT hr;

    /* read the AppID value under the class's key */
    hr = COM_OpenKeyForCLSID(clsid, NULL, KEY_READ, &hkey);
    if (FAILED(hr))
        return hr;

    size = sizeof(buf);
    res = RegQueryValueExW(hkey, szAppId, NULL, &type, (LPBYTE)buf, &size);
    RegCloseKey(hkey);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS || type!=REG_SZ)
        return REGDB_E_READREGDB;

    strcpyW(keyname, szAppIdKey);
    strcatW(keyname, buf);
    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, keyname, 0, access, subkey);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    return S_OK;
}

/******************************************************************************
 *               ProgIDFromCLSID [OLE32.@]
 *
 * Converts a class id into the respective program ID.
 *
 * PARAMS
 *  clsid        [I] Class ID, as found in registry.
 *  ppszProgID [O] Associated ProgID.
 *
 * RETURNS
 *   S_OK
 *   E_OUTOFMEMORY
 *   REGDB_E_CLASSNOTREG if the given clsid has no associated ProgID
 */
HRESULT WINAPI ProgIDFromCLSID(REFCLSID clsid, LPOLESTR *ppszProgID)
{
    static const WCHAR wszProgID[] = {'P','r','o','g','I','D',0};
    HKEY     hkey;
    HRESULT  ret;
    LONG progidlen = 0;

    if (!ppszProgID)
    {
        ERR("ppszProgId isn't optional\n");
        return E_INVALIDARG;
    }

    *ppszProgID = NULL;
    ret = COM_OpenKeyForCLSID(clsid, wszProgID, KEY_READ, &hkey);
    if (FAILED(ret))
        return ret;

    if (RegQueryValueW(hkey, NULL, NULL, &progidlen))
      ret = REGDB_E_CLASSNOTREG;

    if (ret == S_OK)
    {
      *ppszProgID = CoTaskMemAlloc(progidlen * sizeof(WCHAR));
      if (*ppszProgID)
      {
        if (RegQueryValueW(hkey, NULL, *ppszProgID, &progidlen))
          ret = REGDB_E_CLASSNOTREG;
      }
      else
        ret = E_OUTOFMEMORY;
    }

    RegCloseKey(hkey);
    return ret;
}

/******************************************************************************
 *		CLSIDFromProgID	[OLE32.@]
 *
 * Converts a program id into the respective GUID.
 *
 * PARAMS
 *  progid [I] Unicode program ID, as found in registry.
 *  clsid  [O] Associated CLSID.
 *
 * RETURNS
 *	Success: S_OK
 *  Failure: CO_E_CLASSSTRING - the given ProgID cannot be found.
 */
HRESULT WINAPI CLSIDFromProgID(LPCOLESTR progid, LPCLSID clsid)
{
    static const WCHAR clsidW[] = { '\\','C','L','S','I','D',0 };
    WCHAR buf2[CHARS_IN_GUID];
    LONG buf2len = sizeof(buf2);
    HKEY xhkey;
    WCHAR *buf;

    if (!progid || !clsid)
    {
        ERR("neither progid (%p) nor clsid (%p) are optional\n", progid, clsid);
        return E_INVALIDARG;
    }

    /* initialise clsid in case of failure */
    memset(clsid, 0, sizeof(*clsid));

    buf = HeapAlloc( GetProcessHeap(),0,(strlenW(progid)+8) * sizeof(WCHAR) );
    strcpyW( buf, progid );
    strcatW( buf, clsidW );
    if (RegOpenKeyW(HKEY_CLASSES_ROOT,buf,&xhkey))
    {
        HeapFree(GetProcessHeap(),0,buf);
        WARN("couldn't open key for ProgID %s\n", debugstr_w(progid));
        return CO_E_CLASSSTRING;
    }
    HeapFree(GetProcessHeap(),0,buf);

    if (RegQueryValueW(xhkey,NULL,buf2,&buf2len))
    {
        RegCloseKey(xhkey);
        WARN("couldn't query clsid value for ProgID %s\n", debugstr_w(progid));
        return CO_E_CLASSSTRING;
    }
    RegCloseKey(xhkey);
    return CLSIDFromString(buf2,clsid);
}


/*****************************************************************************
 *             CoGetPSClsid [OLE32.@]
 *
 * Retrieves the CLSID of the proxy/stub factory that implements
 * IPSFactoryBuffer for the specified interface.
 *
 * PARAMS
 *  riid   [I] Interface whose proxy/stub CLSID is to be returned.
 *  pclsid [O] Where to store returned proxy/stub CLSID.
 * 
 * RETURNS
 *   S_OK
 *   E_OUTOFMEMORY
 *   REGDB_E_IIDNOTREG if no PSFactoryBuffer is associated with the IID, or it could not be parsed
 *
 * NOTES
 *
 * The standard marshaller activates the object with the CLSID
 * returned and uses the CreateProxy and CreateStub methods on its
 * IPSFactoryBuffer interface to construct the proxies and stubs for a
 * given object.
 *
 * CoGetPSClsid determines this CLSID by searching the
 * HKEY_CLASSES_ROOT\Interface\{string form of riid}\ProxyStubClsid32
 * in the registry and any interface id registered by
 * CoRegisterPSClsid within the current process.
 *
 * BUGS
 *
 * Native returns S_OK for interfaces with a key in HKCR\Interface, but
 * without a ProxyStubClsid32 key and leaves garbage in pclsid. This should be
 * considered a bug in native unless an application depends on this (unlikely).
 *
 * SEE ALSO
 *  CoRegisterPSClsid.
 */
HRESULT WINAPI CoGetPSClsid(REFIID riid, CLSID *pclsid)
{
    static const WCHAR wszInterface[] = {'I','n','t','e','r','f','a','c','e','\\',0};
    static const WCHAR wszPSC[] = {'\\','P','r','o','x','y','S','t','u','b','C','l','s','i','d','3','2',0};
    WCHAR path[ARRAYSIZE(wszInterface) - 1 + CHARS_IN_GUID - 1 + ARRAYSIZE(wszPSC)];
    WCHAR value[CHARS_IN_GUID];
    LONG len;
    HKEY hkey;
    APARTMENT *apt = COM_CurrentApt();
    struct registered_psclsid *registered_psclsid;

    TRACE("() riid=%s, pclsid=%p\n", debugstr_guid(riid), pclsid);

    if (!apt)
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (!pclsid)
    {
        ERR("pclsid isn't optional\n");
        return E_INVALIDARG;
    }

    EnterCriticalSection(&apt->cs);

    LIST_FOR_EACH_ENTRY(registered_psclsid, &apt->psclsids, struct registered_psclsid, entry)
        if (IsEqualIID(&registered_psclsid->iid, riid))
        {
            *pclsid = registered_psclsid->clsid;
            LeaveCriticalSection(&apt->cs);
            return S_OK;
        }

    LeaveCriticalSection(&apt->cs);

    /* Interface\\{string form of riid}\\ProxyStubClsid32 */
    strcpyW(path, wszInterface);
    StringFromGUID2(riid, path + ARRAYSIZE(wszInterface) - 1, CHARS_IN_GUID);
    strcpyW(path + ARRAYSIZE(wszInterface) - 1 + CHARS_IN_GUID - 1, wszPSC);

    /* Open the key.. */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_READ, &hkey))
    {
        WARN("No PSFactoryBuffer object is registered for IID %s\n", debugstr_guid(riid));
        return REGDB_E_IIDNOTREG;
    }

    /* ... Once we have the key, query the registry to get the
       value of CLSID as a string, and convert it into a
       proper CLSID structure to be passed back to the app */
    len = sizeof(value);
    if (ERROR_SUCCESS != RegQueryValueW(hkey, NULL, value, &len))
    {
        RegCloseKey(hkey);
        return REGDB_E_IIDNOTREG;
    }
    RegCloseKey(hkey);

    /* We have the CLSid we want back from the registry as a string, so
       lets convert it into a CLSID structure */
    if (CLSIDFromString(value, pclsid) != NOERROR)
        return REGDB_E_IIDNOTREG;

    TRACE ("() Returning CLSID=%s\n", debugstr_guid(pclsid));
    return S_OK;
}

/*****************************************************************************
 *             CoRegisterPSClsid [OLE32.@]
 *
 * Register a proxy/stub CLSID for the given interface in the current process
 * only.
 *
 * PARAMS
 *  riid   [I] Interface whose proxy/stub CLSID is to be registered.
 *  rclsid [I] CLSID of the proxy/stub.
 * 
 * RETURNS
 *   Success: S_OK
 *   Failure: E_OUTOFMEMORY
 *
 * NOTES
 *
 * This function does not add anything to the registry and the effects are
 * limited to the lifetime of the current process.
 *
 * SEE ALSO
 *  CoGetPSClsid.
 */
HRESULT WINAPI CoRegisterPSClsid(REFIID riid, REFCLSID rclsid)
{
    APARTMENT *apt = COM_CurrentApt();
    struct registered_psclsid *registered_psclsid;

    TRACE("(%s, %s)\n", debugstr_guid(riid), debugstr_guid(rclsid));

    if (!apt)
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    EnterCriticalSection(&apt->cs);

    LIST_FOR_EACH_ENTRY(registered_psclsid, &apt->psclsids, struct registered_psclsid, entry)
        if (IsEqualIID(&registered_psclsid->iid, riid))
        {
            registered_psclsid->clsid = *rclsid;
            LeaveCriticalSection(&apt->cs);
            return S_OK;
        }

    registered_psclsid = HeapAlloc(GetProcessHeap(), 0, sizeof(struct registered_psclsid));
    if (!registered_psclsid)
    {
        LeaveCriticalSection(&apt->cs);
        return E_OUTOFMEMORY;
    }

    registered_psclsid->iid = *riid;
    registered_psclsid->clsid = *rclsid;
    list_add_head(&apt->psclsids, &registered_psclsid->entry);

    LeaveCriticalSection(&apt->cs);

    return S_OK;
}


/***
 * COM_GetRegisteredClassObject
 *
 * This internal method is used to scan the registered class list to
 * find a class object.
 *
 * Params:
 *   rclsid        Class ID of the class to find.
 *   dwClsContext  Class context to match.
 *   ppv           [out] returns a pointer to the class object. Complying
 *                 to normal COM usage, this method will increase the
 *                 reference count on this object.
 */
static HRESULT COM_GetRegisteredClassObject(
	REFCLSID    rclsid,
	DWORD       dwClsContext,
	LPUNKNOWN*  ppUnk)
{
  HRESULT hr = S_FALSE;
  RegisteredClass *curClass;

  /*
   * Sanity check
   */
  assert(ppUnk!=0);

  EnterCriticalSection( &csRegisteredClassList );

  LIST_FOR_EACH_ENTRY(curClass, &RegisteredClassList, RegisteredClass, entry)
  {
    /*
     * Check if we have a match on the class ID and context.
     */
    if ((dwClsContext & curClass->runContext) &&
        IsEqualGUID(&(curClass->classIdentifier), rclsid))
    {
      /*
       * We have a match, return the pointer to the class object.
       */
      *ppUnk = curClass->classObject;

      IUnknown_AddRef(curClass->classObject);

      hr = S_OK;
      break;
    }
  }

  LeaveCriticalSection( &csRegisteredClassList );

  return hr;
}

/******************************************************************************
 *		CoRegisterClassObject	[OLE32.@]
 *
 * Registers the class object for a given class ID. Servers housed in EXE
 * files use this method instead of exporting DllGetClassObject to allow
 * other code to connect to their objects.
 *
 * PARAMS
 *  rclsid       [I] CLSID of the object to register.
 *  pUnk         [I] IUnknown of the object.
 *  dwClsContext [I] CLSCTX flags indicating the context in which to run the executable.
 *  flags        [I] REGCLS flags indicating how connections are made.
 *  lpdwRegister [I] A unique cookie that can be passed to CoRevokeClassObject.
 *
 * RETURNS
 *   S_OK on success,
 *   E_INVALIDARG if lpdwRegister or pUnk are NULL,
 *   CO_E_OBJISREG if the object is already registered. We should not return this.
 *
 * SEE ALSO
 *   CoRevokeClassObject, CoGetClassObject
 *
 * BUGS
 *  MSDN claims that multiple interface registrations are legal, but we
 *  can't do that with our current implementation.
 */
HRESULT WINAPI CoRegisterClassObject(
    REFCLSID rclsid,
    LPUNKNOWN pUnk,
    DWORD dwClsContext,
    DWORD flags,
    LPDWORD lpdwRegister)
{
  RegisteredClass* newClass;
  LPUNKNOWN        foundObject;
  HRESULT          hr;

  TRACE("(%s,%p,0x%08x,0x%08x,%p)\n",
	debugstr_guid(rclsid),pUnk,dwClsContext,flags,lpdwRegister);

  if ( (lpdwRegister==0) || (pUnk==0) )
    return E_INVALIDARG;

  if (!COM_CurrentApt())
  {
      ERR("COM was not initialized\n");
      return CO_E_NOTINITIALIZED;
  }

  *lpdwRegister = 0;

  /* REGCLS_MULTIPLEUSE implies registering as inproc server. This is what
   * differentiates the flag from REGCLS_MULTI_SEPARATE. */
  if (flags & REGCLS_MULTIPLEUSE)
    dwClsContext |= CLSCTX_INPROC_SERVER;

  /*
   * First, check if the class is already registered.
   * If it is, this should cause an error.
   */
  hr = COM_GetRegisteredClassObject(rclsid, dwClsContext, &foundObject);
  if (hr == S_OK) {
    if (flags & REGCLS_MULTIPLEUSE) {
      if (dwClsContext & CLSCTX_LOCAL_SERVER)
        hr = CoLockObjectExternal(foundObject, TRUE, FALSE);
      IUnknown_Release(foundObject);
      return hr;
    }
    IUnknown_Release(foundObject);
    ERR("object already registered for class %s\n", debugstr_guid(rclsid));
    return CO_E_OBJISREG;
  }

  newClass = HeapAlloc(GetProcessHeap(), 0, sizeof(RegisteredClass));
  if ( newClass == NULL )
    return E_OUTOFMEMORY;

  newClass->classIdentifier = *rclsid;
  newClass->runContext      = dwClsContext;
  newClass->connectFlags    = flags;
  newClass->pMarshaledData  = NULL;
  newClass->RpcRegistration = NULL;

  /*
   * Use the address of the chain node as the cookie since we are sure it's
   * unique. FIXME: not on 64-bit platforms.
   */
  newClass->dwCookie        = (DWORD)newClass;

  /*
   * Since we're making a copy of the object pointer, we have to increase its
   * reference count.
   */
  newClass->classObject     = pUnk;
  IUnknown_AddRef(newClass->classObject);

  EnterCriticalSection( &csRegisteredClassList );
  list_add_tail(&RegisteredClassList, &newClass->entry);
  LeaveCriticalSection( &csRegisteredClassList );

  *lpdwRegister = newClass->dwCookie;

  if (dwClsContext & CLSCTX_LOCAL_SERVER) {
      IClassFactory *classfac;

      hr = IUnknown_QueryInterface(newClass->classObject, &IID_IClassFactory,
                                   (LPVOID*)&classfac);
      if (hr) return hr;

      hr = CreateStreamOnHGlobal(0, TRUE, &newClass->pMarshaledData);
      if (hr) {
          FIXME("Failed to create stream on hglobal, %x\n", hr);
          IUnknown_Release(classfac);
          return hr;
      }
      hr = CoMarshalInterface(newClass->pMarshaledData, &IID_IClassFactory,
                              (LPVOID)classfac, MSHCTX_LOCAL, NULL,
                              MSHLFLAGS_TABLESTRONG);
      if (hr) {
          FIXME("CoMarshalInterface failed, %x!\n",hr);
          IUnknown_Release(classfac);
          return hr;
      }

      IUnknown_Release(classfac);

      hr = RPC_StartLocalServer(&newClass->classIdentifier,
                                newClass->pMarshaledData,
                                flags & (REGCLS_MULTIPLEUSE|REGCLS_MULTI_SEPARATE),
                                &newClass->RpcRegistration);
  }
  return S_OK;
}

/***********************************************************************
 *           CoRevokeClassObject [OLE32.@]
 *
 * Removes a class object from the class registry.
 *
 * PARAMS
 *  dwRegister [I] Cookie returned from CoRegisterClassObject().
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoRegisterClassObject
 */
HRESULT WINAPI CoRevokeClassObject(
        DWORD dwRegister)
{
  HRESULT hr = E_INVALIDARG;
  RegisteredClass *curClass;

  TRACE("(%08x)\n",dwRegister);

  EnterCriticalSection( &csRegisteredClassList );

  LIST_FOR_EACH_ENTRY(curClass, &RegisteredClassList, RegisteredClass, entry)
  {
    /*
     * Check if we have a match on the cookie.
     */
    if (curClass->dwCookie == dwRegister)
    {
      list_remove(&curClass->entry);

      if (curClass->runContext & CLSCTX_LOCAL_SERVER)
        RPC_StopLocalServer(curClass->RpcRegistration);

      /*
       * Release the reference to the class object.
       */
      IUnknown_Release(curClass->classObject);

      if (curClass->pMarshaledData)
      {
        LARGE_INTEGER zero;
        memset(&zero, 0, sizeof(zero));
        IStream_Seek(curClass->pMarshaledData, zero, STREAM_SEEK_SET, NULL);
        CoReleaseMarshalData(curClass->pMarshaledData);
      }

      /*
       * Free the memory used by the chain node.
       */
      HeapFree(GetProcessHeap(), 0, curClass);

      hr = S_OK;
      break;
    }
  }

  LeaveCriticalSection( &csRegisteredClassList );

  return hr;
}

/***********************************************************************
 *	COM_RegReadPath	[internal]
 *
 *	Reads a registry value and expands it when necessary
 */
static DWORD COM_RegReadPath(HKEY hkeyroot, const WCHAR *keyname, const WCHAR *valuename, WCHAR * dst, DWORD dstlen)
{
	DWORD ret;
	HKEY key;
	DWORD keytype;
	WCHAR src[MAX_PATH];
	DWORD dwLength = dstlen * sizeof(WCHAR);

	if((ret = RegOpenKeyExW(hkeyroot, keyname, 0, KEY_READ, &key)) == ERROR_SUCCESS) {
          if( (ret = RegQueryValueExW(key, NULL, NULL, &keytype, (LPBYTE)src, &dwLength)) == ERROR_SUCCESS ) {
            if (keytype == REG_EXPAND_SZ) {
              if (dstlen <= ExpandEnvironmentStringsW(src, dst, dstlen)) ret = ERROR_MORE_DATA;
            } else {
              lstrcpynW(dst, src, dstlen);
            }
	  }
          RegCloseKey (key);
	}
	return ret;
}

static void get_threading_model(HKEY key, LPWSTR value, DWORD len)
{
    static const WCHAR wszThreadingModel[] = {'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
    DWORD keytype;
    DWORD ret;
    DWORD dwLength = len * sizeof(WCHAR);

    ret = RegQueryValueExW(key, wszThreadingModel, NULL, &keytype, (LPBYTE)value, &dwLength);
    if ((ret != ERROR_SUCCESS) || (keytype != REG_SZ))
        value[0] = '\0';
}

static HRESULT get_inproc_class_object(HKEY hkeydll, REFCLSID rclsid, REFIID riid, void **ppv)
{
    static const WCHAR wszApartment[] = {'A','p','a','r','t','m','e','n','t',0};
    static const WCHAR wszFree[] = {'F','r','e','e',0};
    static const WCHAR wszBoth[] = {'B','o','t','h',0};
    WCHAR dllpath[MAX_PATH+1];
    WCHAR threading_model[10 /* strlenW(L"apartment")+1 */];
    HRESULT hr;
    APARTMENT *apt = COM_CurrentApt();

    get_threading_model(hkeydll, threading_model, ARRAYSIZE(threading_model));
    /* "Apartment" */
    if (!strcmpiW(threading_model, wszApartment))
    {
        if (apt->multi_threaded)
        {
            /* try to find an STA */
            APARTMENT *host_apt = apartment_findfromtype(FALSE, FALSE);
            if (!host_apt)
                FIXME("create a host apartment for apartment-threaded object %s\n", debugstr_guid(rclsid));
            if (host_apt)
            {
                struct host_object_params params;
                HWND hwnd = apartment_getwindow(host_apt);

                params.hkeydll = hkeydll;
                params.clsid = *rclsid;
                params.iid = *riid;
                hr = CreateStreamOnHGlobal(NULL, TRUE, &params.stream);
                if (FAILED(hr))
                    return hr;
                hr = SendMessageW(hwnd, DM_HOSTOBJECT, 0, (LPARAM)&params);
                if (SUCCEEDED(hr))
                    hr = CoUnmarshalInterface(params.stream, riid, ppv);
                IStream_Release(params.stream);
                return hr;
            }
        }
    }
    /* "Free" */
    else if (!strcmpiW(threading_model, wszFree))
    {
        if (!apt->multi_threaded)
        {
            FIXME("should create object %s in multi-threaded apartment\n",
                debugstr_guid(rclsid));
        }
    }
    /* everything except "Apartment", "Free" and "Both" */
    else if (strcmpiW(threading_model, wszBoth))
    {
        /* everything else is main-threaded */
        if (threading_model[0])
            FIXME("unrecognised threading model %s for object %s, should be main-threaded?\n",
                debugstr_w(threading_model), debugstr_guid(rclsid));

        if (apt->multi_threaded || !apt->main)
        {
            /* try to find an STA */
            APARTMENT *host_apt = apartment_findfromtype(FALSE, TRUE);
            if (!host_apt)
                FIXME("create a host apartment for main-threaded object %s\n", debugstr_guid(rclsid));
            if (host_apt)
            {
                struct host_object_params params;
                HWND hwnd = apartment_getwindow(host_apt);

                params.hkeydll = hkeydll;
                params.clsid = *rclsid;
                params.iid = *riid;
                hr = CreateStreamOnHGlobal(NULL, TRUE, &params.stream);
                if (FAILED(hr))
                    return hr;
                hr = SendMessageW(hwnd, DM_HOSTOBJECT, 0, (LPARAM)&params);
                if (SUCCEEDED(hr))
                    hr = CoUnmarshalInterface(params.stream, riid, ppv);
                IStream_Release(params.stream);
                return hr;
            }
        }
    }

    if (COM_RegReadPath(hkeydll, NULL, NULL, dllpath, ARRAYSIZE(dllpath)) != ERROR_SUCCESS)
    {
        /* failure: CLSID is not found in registry */
        WARN("class %s not registered inproc\n", debugstr_guid(rclsid));
        return REGDB_E_CLASSNOTREG;
    }

    return apartment_getclassobject(apt, dllpath, rclsid, riid, ppv);
}

/***********************************************************************
 *           CoGetClassObject [OLE32.@]
 *
 * Creates an object of the specified class.
 *
 * PARAMS
 *  rclsid       [I] Class ID to create an instance of.
 *  dwClsContext [I] Flags to restrict the location of the created instance.
 *  pServerInfo  [I] Optional. Details for connecting to a remote server.
 *  iid          [I] The ID of the interface of the instance to return.
 *  ppv          [O] On returns, contains a pointer to the specified interface of the object.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: HRESULT code.
 *
 * NOTES
 *  The dwClsContext parameter can be one or more of the following:
 *| CLSCTX_INPROC_SERVER - Use an in-process server, such as from a DLL.
 *| CLSCTX_INPROC_HANDLER - Use an in-process object which handles certain functions for an object running in another process.
 *| CLSCTX_LOCAL_SERVER - Connect to an object running in another process.
 *| CLSCTX_REMOTE_SERVER - Connect to an object running on another machine.
 *
 * SEE ALSO
 *  CoCreateInstance()
 */
HRESULT WINAPI CoGetClassObject(
    REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo,
    REFIID iid, LPVOID *ppv)
{
    LPUNKNOWN	regClassObject;
    HRESULT	hres = E_UNEXPECTED;

    TRACE("\n\tCLSID:\t%s,\n\tIID:\t%s\n", debugstr_guid(rclsid), debugstr_guid(iid));

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    if (!COM_CurrentApt())
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (pServerInfo) {
	FIXME("\tpServerInfo: name=%s\n",debugstr_w(pServerInfo->pwszName));
	FIXME("\t\tpAuthInfo=%p\n",pServerInfo->pAuthInfo);
    }

    /*
     * First, try and see if we can't match the class ID with one of the
     * registered classes.
     */
    if (S_OK == COM_GetRegisteredClassObject(rclsid, dwClsContext, &regClassObject))
    {
      /* Get the required interface from the retrieved pointer. */
      hres = IUnknown_QueryInterface(regClassObject, iid, ppv);

      /*
       * Since QI got another reference on the pointer, we want to release the
       * one we already have. If QI was unsuccessful, this will release the object. This
       * is good since we are not returning it in the "out" parameter.
       */
      IUnknown_Release(regClassObject);

      return hres;
    }

    /* First try in-process server */
    if (CLSCTX_INPROC_SERVER & dwClsContext)
    {
        static const WCHAR wszInprocServer32[] = {'I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};
        HKEY hkey;

        if (IsEqualCLSID(rclsid, &CLSID_InProcFreeMarshaler))
            return FTMarshalCF_Create(iid, ppv);

        hres = COM_OpenKeyForCLSID(rclsid, wszInprocServer32, KEY_READ, &hkey);
        if (FAILED(hres))
        {
            if (hres == REGDB_E_CLASSNOTREG)
                ERR("class %s not registered\n", debugstr_guid(rclsid));
            else
                WARN("class %s not registered as in-proc server\n", debugstr_guid(rclsid));
        }

        if (SUCCEEDED(hres))
        {
            hres = get_inproc_class_object(hkey, rclsid, iid, ppv);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hres))
            return hres;
    }

    /* Next try in-process handler */
    if (CLSCTX_INPROC_HANDLER & dwClsContext)
    {
        static const WCHAR wszInprocHandler32[] = {'I','n','p','r','o','c','H','a','n','d','l','e','r','3','2',0};
        HKEY hkey;

        hres = COM_OpenKeyForCLSID(rclsid, wszInprocHandler32, KEY_READ, &hkey);
        if (FAILED(hres))
        {
            if (hres == REGDB_E_CLASSNOTREG)
                ERR("class %s not registered\n", debugstr_guid(rclsid));
            else
                WARN("class %s not registered in-proc handler\n", debugstr_guid(rclsid));
        }

        if (SUCCEEDED(hres))
        {
            hres = get_inproc_class_object(hkey, rclsid, iid, ppv);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hres))
            return hres;
    }

    /* Next try out of process */
    if (CLSCTX_LOCAL_SERVER & dwClsContext)
    {
        hres = RPC_GetLocalClassObject(rclsid,iid,ppv);
        if (SUCCEEDED(hres))
            return hres;
    }

    /* Finally try remote: this requires networked DCOM (a lot of work) */
    if (CLSCTX_REMOTE_SERVER & dwClsContext)
    {
        FIXME ("CLSCTX_REMOTE_SERVER not supported\n");
        hres = E_NOINTERFACE;
    }

    if (FAILED(hres))
        ERR("no class object %s could be created for context 0x%x\n",
            debugstr_guid(rclsid), dwClsContext);
    return hres;
}

/***********************************************************************
 *        CoResumeClassObjects (OLE32.@)
 *
 * Resumes all class objects registered with REGCLS_SUSPENDED.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoResumeClassObjects(void)
{
       FIXME("stub\n");
	return S_OK;
}

/***********************************************************************
 *           CoCreateInstance [OLE32.@]
 *
 * Creates an instance of the specified class.
 *
 * PARAMS
 *  rclsid       [I] Class ID to create an instance of.
 *  pUnkOuter    [I] Optional outer unknown to allow aggregation with another object.
 *  dwClsContext [I] Flags to restrict the location of the created instance.
 *  iid          [I] The ID of the interface of the instance to return.
 *  ppv          [O] On returns, contains a pointer to the specified interface of the instance.
 *
 * RETURNS
 *  Success: S_OK
 *  Failure: HRESULT code.
 *
 * NOTES
 *  The dwClsContext parameter can be one or more of the following:
 *| CLSCTX_INPROC_SERVER - Use an in-process server, such as from a DLL.
 *| CLSCTX_INPROC_HANDLER - Use an in-process object which handles certain functions for an object running in another process.
 *| CLSCTX_LOCAL_SERVER - Connect to an object running in another process.
 *| CLSCTX_REMOTE_SERVER - Connect to an object running on another machine.
 *
 * Aggregation is the concept of deferring the IUnknown of an object to another
 * object. This allows a separate object to behave as though it was part of
 * the object and to allow this the pUnkOuter parameter can be set. Note that
 * not all objects support having an outer of unknown.
 *
 * SEE ALSO
 *  CoGetClassObject()
 */
HRESULT WINAPI CoCreateInstance(
	REFCLSID rclsid,
	LPUNKNOWN pUnkOuter,
	DWORD dwClsContext,
	REFIID iid,
	LPVOID *ppv)
{
  HRESULT hres;
  LPCLASSFACTORY lpclf = 0;

  TRACE("(rclsid=%s, pUnkOuter=%p, dwClsContext=%08x, riid=%s, ppv=%p)\n", debugstr_guid(rclsid),
        pUnkOuter, dwClsContext, debugstr_guid(iid), ppv);

  /*
   * Sanity check
   */
  if (ppv==0)
    return E_POINTER;

  /*
   * Initialize the "out" parameter
   */
  *ppv = 0;

  if (!COM_CurrentApt())
  {
      ERR("apartment not initialised\n");
      return CO_E_NOTINITIALIZED;
  }

  /*
   * The Standard Global Interface Table (GIT) object is a process-wide singleton.
   * Rather than create a class factory, we can just check for it here
   */
  if (IsEqualIID(rclsid, &CLSID_StdGlobalInterfaceTable)) {
    if (StdGlobalInterfaceTableInstance == NULL)
      StdGlobalInterfaceTableInstance = StdGlobalInterfaceTable_Construct();
    hres = IGlobalInterfaceTable_QueryInterface( (IGlobalInterfaceTable*) StdGlobalInterfaceTableInstance, iid, ppv);
    if (hres) return hres;

    TRACE("Retrieved GIT (%p)\n", *ppv);
    return S_OK;
  }

  /*
   * Get a class factory to construct the object we want.
   */
  hres = CoGetClassObject(rclsid,
			  dwClsContext,
			  NULL,
			  &IID_IClassFactory,
			  (LPVOID)&lpclf);

  if (FAILED(hres))
    return hres;

  /*
   * Create the object and don't forget to release the factory
   */
	hres = IClassFactory_CreateInstance(lpclf, pUnkOuter, iid, ppv);
	IClassFactory_Release(lpclf);
	if(FAILED(hres))
          FIXME("no instance created for interface %s of class %s, hres is 0x%08x\n",
		debugstr_guid(iid), debugstr_guid(rclsid),hres);

	return hres;
}

/***********************************************************************
 *           CoCreateInstanceEx [OLE32.@]
 */
HRESULT WINAPI CoCreateInstanceEx(
  REFCLSID      rclsid,
  LPUNKNOWN     pUnkOuter,
  DWORD         dwClsContext,
  COSERVERINFO* pServerInfo,
  ULONG         cmq,
  MULTI_QI*     pResults)
{
  IUnknown* pUnk = NULL;
  HRESULT   hr;
  ULONG     index;
  ULONG     successCount = 0;

  /*
   * Sanity check
   */
  if ( (cmq==0) || (pResults==NULL))
    return E_INVALIDARG;

  if (pServerInfo!=NULL)
    FIXME("() non-NULL pServerInfo not supported!\n");

  /*
   * Initialize all the "out" parameters.
   */
  for (index = 0; index < cmq; index++)
  {
    pResults[index].pItf = NULL;
    pResults[index].hr   = E_NOINTERFACE;
  }

  /*
   * Get the object and get its IUnknown pointer.
   */
  hr = CoCreateInstance(rclsid,
			pUnkOuter,
			dwClsContext,
			&IID_IUnknown,
			(VOID**)&pUnk);

  if (hr)
    return hr;

  /*
   * Then, query for all the interfaces requested.
   */
  for (index = 0; index < cmq; index++)
  {
    pResults[index].hr = IUnknown_QueryInterface(pUnk,
						 pResults[index].pIID,
						 (VOID**)&(pResults[index].pItf));

    if (pResults[index].hr == S_OK)
      successCount++;
  }

  /*
   * Release our temporary unknown pointer.
   */
  IUnknown_Release(pUnk);

  if (successCount == 0)
    return E_NOINTERFACE;

  if (successCount!=cmq)
    return CO_S_NOTALLINTERFACES;

  return S_OK;
}

/***********************************************************************
 *           CoLoadLibrary (OLE32.@)
 *
 * Loads a library.
 *
 * PARAMS
 *  lpszLibName [I] Path to library.
 *  bAutoFree   [I] Whether the library should automatically be freed.
 *
 * RETURNS
 *  Success: Handle to loaded library.
 *  Failure: NULL.
 *
 * SEE ALSO
 *  CoFreeLibrary, CoFreeAllLibraries, CoFreeUnusedLibraries
 */
HINSTANCE WINAPI CoLoadLibrary(LPOLESTR lpszLibName, BOOL bAutoFree)
{
    TRACE("(%s, %d)\n", debugstr_w(lpszLibName), bAutoFree);

    return LoadLibraryExW(lpszLibName, 0, LOAD_WITH_ALTERED_SEARCH_PATH);
}

/***********************************************************************
 *           CoFreeLibrary [OLE32.@]
 *
 * Unloads a library from memory.
 *
 * PARAMS
 *  hLibrary [I] Handle to library to unload.
 *
 * RETURNS
 *  Nothing
 *
 * SEE ALSO
 *  CoLoadLibrary, CoFreeAllLibraries, CoFreeUnusedLibraries
 */
void WINAPI CoFreeLibrary(HINSTANCE hLibrary)
{
    FreeLibrary(hLibrary);
}


/***********************************************************************
 *           CoFreeAllLibraries [OLE32.@]
 *
 * Function for backwards compatibility only. Does nothing.
 *
 * RETURNS
 *  Nothing.
 *
 * SEE ALSO
 *  CoLoadLibrary, CoFreeLibrary, CoFreeUnusedLibraries
 */
void WINAPI CoFreeAllLibraries(void)
{
    /* NOP */
}


/***********************************************************************
 *           CoFreeUnusedLibraries [OLE32.@]
 *           CoFreeUnusedLibraries [COMPOBJ.17]
 *
 * Frees any unused libraries. Unused are identified as those that return
 * S_OK from their DllCanUnloadNow function.
 *
 * RETURNS
 *  Nothing.
 *
 * SEE ALSO
 *  CoLoadLibrary, CoFreeAllLibraries, CoFreeLibrary
 */
void WINAPI CoFreeUnusedLibraries(void)
{
    struct apartment *apt = COM_CurrentApt();
    if (!apt)
    {
        ERR("apartment not initialised\n");
        return;
    }

    apartment_freeunusedlibraries(apt);
}

/***********************************************************************
 *           CoFileTimeNow [OLE32.@]
 *           CoFileTimeNow [COMPOBJ.82]
 *
 * Retrieves the current time in FILETIME format.
 *
 * PARAMS
 *  lpFileTime [O] The current time.
 *
 * RETURNS
 *	S_OK.
 */
HRESULT WINAPI CoFileTimeNow( FILETIME *lpFileTime )
{
    GetSystemTimeAsFileTime( lpFileTime );
    return S_OK;
}

static void COM_RevokeAllClasses(void)
{
  EnterCriticalSection( &csRegisteredClassList );

  while (list_head(&RegisteredClassList))
  {
    RegisteredClass *curClass = LIST_ENTRY(list_head(&RegisteredClassList),
                                           RegisteredClass, entry);
    CoRevokeClassObject(curClass->dwCookie);
  }

  LeaveCriticalSection( &csRegisteredClassList );
}

/******************************************************************************
 *		CoLockObjectExternal	[OLE32.@]
 *
 * Increments or decrements the external reference count of a stub object.
 *
 * PARAMS
 *  pUnk                [I] Stub object.
 *  fLock               [I] If TRUE then increments the external ref-count,
 *                          otherwise decrements.
 *  fLastUnlockReleases [I] If TRUE then the last unlock has the effect of
 *                          calling CoDisconnectObject.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  If fLock is TRUE and an object is passed in that doesn't have a stub
 *  manager then a new stub manager is created for the object.
 */
HRESULT WINAPI CoLockObjectExternal(
    LPUNKNOWN pUnk,
    BOOL fLock,
    BOOL fLastUnlockReleases)
{
    struct stub_manager *stubmgr;
    struct apartment *apt;

    TRACE("pUnk=%p, fLock=%s, fLastUnlockReleases=%s\n",
          pUnk, fLock ? "TRUE" : "FALSE", fLastUnlockReleases ? "TRUE" : "FALSE");

    apt = COM_CurrentApt();
    if (!apt) return CO_E_NOTINITIALIZED;

    stubmgr = get_stub_manager_from_object(apt, pUnk);
    
    if (stubmgr)
    {
        if (fLock)
            stub_manager_ext_addref(stubmgr, 1);
        else
            stub_manager_ext_release(stubmgr, 1, fLastUnlockReleases);
        
        stub_manager_int_release(stubmgr);

        return S_OK;
    }
    else if (fLock)
    {
        stubmgr = new_stub_manager(apt, pUnk);

        if (stubmgr)
        {
            stub_manager_ext_addref(stubmgr, 1);
            stub_manager_int_release(stubmgr);
        }

        return S_OK;
    }
    else
    {
        WARN("stub object not found %p\n", pUnk);
        /* Note: native is pretty broken here because it just silently
         * fails, without returning an appropriate error code, making apps
         * think that the object was disconnected, when it actually wasn't */
        return S_OK;
    }
}

/***********************************************************************
 *           CoInitializeWOW (OLE32.@)
 *
 * WOW equivalent of CoInitialize?
 *
 * PARAMS
 *  x [I] Unknown.
 *  y [I] Unknown.
 *
 * RETURNS
 *  Unknown.
 */
HRESULT WINAPI CoInitializeWOW(DWORD x,DWORD y)
{
    FIXME("(0x%08x,0x%08x),stub!\n",x,y);
    return 0;
}

/***********************************************************************
 *           CoGetState [OLE32.@]
 *
 * Retrieves the thread state object previously stored by CoSetState().
 *
 * PARAMS
 *  ppv [I] Address where pointer to object will be stored.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_OUTOFMEMORY.
 *
 * NOTES
 *  Crashes on all invalid ppv addresses, including NULL.
 *  If the function returns a non-NULL object then the caller must release its
 *  reference on the object when the object is no longer required.
 *
 * SEE ALSO
 *  CoSetState().
 */
HRESULT WINAPI CoGetState(IUnknown ** ppv)
{
    struct oletls *info = COM_CurrentInfo();
    if (!info) return E_OUTOFMEMORY;

    *ppv = NULL;

    if (info->state)
    {
        IUnknown_AddRef(info->state);
        *ppv = info->state;
        TRACE("apt->state=%p\n", info->state);
    }

    return S_OK;
}

/***********************************************************************
 *           CoSetState [OLE32.@]
 *
 * Sets the thread state object.
 *
 * PARAMS
 *  pv [I] Pointer to state object to be stored.
 *
 * NOTES
 *  The system keeps a reference on the object while the object stored.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: E_OUTOFMEMORY.
 */
HRESULT WINAPI CoSetState(IUnknown * pv)
{
    struct oletls *info = COM_CurrentInfo();
    if (!info) return E_OUTOFMEMORY;

    if (pv) IUnknown_AddRef(pv);

    if (info->state)
    {
        TRACE("-- release %p now\n", info->state);
        IUnknown_Release(info->state);
    }

    info->state = pv;

    return S_OK;
}


/******************************************************************************
 *              CoTreatAsClass        [OLE32.@]
 *
 * Sets the TreatAs value of a class.
 *
 * PARAMS
 *  clsidOld [I] Class to set TreatAs value on.
 *  clsidNew [I] The class the clsidOld should be treated as.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoGetTreatAsClass
 */
HRESULT WINAPI CoTreatAsClass(REFCLSID clsidOld, REFCLSID clsidNew)
{
    static const WCHAR wszAutoTreatAs[] = {'A','u','t','o','T','r','e','a','t','A','s',0};
    static const WCHAR wszTreatAs[] = {'T','r','e','a','t','A','s',0};
    HKEY hkey = NULL;
    WCHAR szClsidNew[CHARS_IN_GUID];
    HRESULT res = S_OK;
    WCHAR auto_treat_as[CHARS_IN_GUID];
    LONG auto_treat_as_size = sizeof(auto_treat_as);
    CLSID id;

    res = COM_OpenKeyForCLSID(clsidOld, NULL, KEY_READ | KEY_WRITE, &hkey);
    if (FAILED(res))
        goto done;
    if (!memcmp( clsidOld, clsidNew, sizeof(*clsidOld) ))
    {
       if (!RegQueryValueW(hkey, wszAutoTreatAs, auto_treat_as, &auto_treat_as_size) &&
           !CLSIDFromString(auto_treat_as, &id))
       {
           if (RegSetValueW(hkey, wszTreatAs, REG_SZ, auto_treat_as, sizeof(auto_treat_as)))
           {
               res = REGDB_E_WRITEREGDB;
               goto done;
           }
       }
       else
       {
           RegDeleteKeyW(hkey, wszTreatAs);
           goto done;
       }
    }
    else if (!StringFromGUID2(clsidNew, szClsidNew, ARRAYSIZE(szClsidNew)) &&
             !RegSetValueW(hkey, wszTreatAs, REG_SZ, szClsidNew, sizeof(szClsidNew)))
    {
        res = REGDB_E_WRITEREGDB;
	goto done;
    }

done:
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************************
 *              CoGetTreatAsClass        [OLE32.@]
 *
 * Gets the TreatAs value of a class.
 *
 * PARAMS
 *  clsidOld [I] Class to get the TreatAs value of.
 *  clsidNew [I] The class the clsidOld should be treated as.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoSetTreatAsClass
 */
HRESULT WINAPI CoGetTreatAsClass(REFCLSID clsidOld, LPCLSID clsidNew)
{
    static const WCHAR wszTreatAs[] = {'T','r','e','a','t','A','s',0};
    HKEY hkey = NULL;
    WCHAR szClsidNew[CHARS_IN_GUID];
    HRESULT res = S_OK;
    LONG len = sizeof(szClsidNew);

    FIXME("(%s,%p)\n", debugstr_guid(clsidOld), clsidNew);
    memcpy(clsidNew,clsidOld,sizeof(CLSID)); /* copy over old value */

    res = COM_OpenKeyForCLSID(clsidOld, wszTreatAs, KEY_READ, &hkey);
    if (FAILED(res))
        goto done;
    if (RegQueryValueW(hkey, NULL, szClsidNew, &len))
    {
        res = S_FALSE;
	goto done;
    }
    res = CLSIDFromString(szClsidNew,clsidNew);
    if (FAILED(res))
        ERR("Failed CLSIDFromStringA(%s), hres 0x%08x\n", debugstr_w(szClsidNew), res);
done:
    if (hkey) RegCloseKey(hkey);
    return res;
}

/******************************************************************************
 *		CoGetCurrentProcess	[OLE32.@]
 *		CoGetCurrentProcess	[COMPOBJ.34]
 *
 * Gets the current process ID.
 *
 * RETURNS
 *  The current process ID.
 *
 * NOTES
 *   Is DWORD really the correct return type for this function?
 */
DWORD WINAPI CoGetCurrentProcess(void)
{
	return GetCurrentProcessId();
}

/******************************************************************************
 *		CoRegisterMessageFilter	[OLE32.@]
 *
 * Registers a message filter.
 *
 * PARAMS
 *  lpMessageFilter [I] Pointer to interface.
 *  lplpMessageFilter [O] Indirect pointer to prior instance if non-NULL.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  Both lpMessageFilter and lplpMessageFilter are optional. Passing in a NULL
 *  lpMessageFilter removes the message filter.
 *
 *  If lplpMessageFilter is not NULL the previous message filter will be
 *  returned in the memory pointer to this parameter and the caller is
 *  responsible for releasing the object.
 *
 *  The current thread be in an apartment otherwise the function will crash.
 */
HRESULT WINAPI CoRegisterMessageFilter(
    LPMESSAGEFILTER lpMessageFilter,
    LPMESSAGEFILTER *lplpMessageFilter)
{
    struct apartment *apt;
    IMessageFilter *lpOldMessageFilter;

    TRACE("(%p, %p)\n", lpMessageFilter, lplpMessageFilter);

    apt = COM_CurrentApt();

    /* can't set a message filter in a multi-threaded apartment */
    if (!apt || apt->multi_threaded)
    {
        WARN("can't set message filter in MTA or uninitialized apt\n");
        return CO_E_NOT_SUPPORTED;
    }

    if (lpMessageFilter)
        IMessageFilter_AddRef(lpMessageFilter);

    EnterCriticalSection(&apt->cs);

    lpOldMessageFilter = apt->filter;
    apt->filter = lpMessageFilter;

    LeaveCriticalSection(&apt->cs);

    if (lplpMessageFilter)
        *lplpMessageFilter = lpOldMessageFilter;
    else if (lpOldMessageFilter)
        IMessageFilter_Release(lpOldMessageFilter);

    return S_OK;
}

/***********************************************************************
 *           CoIsOle1Class [OLE32.@]
 *
 * Determines whether the specified class an OLE v1 class.
 *
 * PARAMS
 *  clsid [I] Class to test.
 *
 * RETURNS
 *  TRUE if the class is an OLE v1 class, or FALSE otherwise.
 */
BOOL WINAPI CoIsOle1Class(REFCLSID clsid)
{
  FIXME("%s\n", debugstr_guid(clsid));
  return FALSE;
}

/***********************************************************************
 *           IsEqualGUID [OLE32.@]
 *
 * Compares two Unique Identifiers.
 *
 * PARAMS
 *  rguid1 [I] The first GUID to compare.
 *  rguid2 [I] The other GUID to compare.
 *
 * RETURNS
 *	TRUE if equal
 */
#undef IsEqualGUID
BOOL WINAPI IsEqualGUID(
     REFGUID rguid1,
     REFGUID rguid2)
{
    return !memcmp(rguid1,rguid2,sizeof(GUID));
}

/***********************************************************************
 *           CoInitializeSecurity [OLE32.@]
 */
HRESULT WINAPI CoInitializeSecurity(PSECURITY_DESCRIPTOR pSecDesc, LONG cAuthSvc,
                                    SOLE_AUTHENTICATION_SERVICE* asAuthSvc,
                                    void* pReserved1, DWORD dwAuthnLevel,
                                    DWORD dwImpLevel, void* pReserved2,
                                    DWORD dwCapabilities, void* pReserved3)
{
  FIXME("(%p,%d,%p,%p,%d,%d,%p,%d,%p) - stub!\n", pSecDesc, cAuthSvc,
        asAuthSvc, pReserved1, dwAuthnLevel, dwImpLevel, pReserved2,
        dwCapabilities, pReserved3);
  return S_OK;
}

/***********************************************************************
 *           CoSuspendClassObjects [OLE32.@]
 *
 * Suspends all registered class objects to prevent further requests coming in
 * for those objects.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoSuspendClassObjects(void)
{
    FIXME("\n");
    return S_OK;
}

/***********************************************************************
 *           CoAddRefServerProcess [OLE32.@]
 *
 * Helper function for incrementing the reference count of a local-server
 * process.
 *
 * RETURNS
 *  New reference count.
 *
 * SEE ALSO
 *  CoReleaseServerProcess().
 */
ULONG WINAPI CoAddRefServerProcess(void)
{
    ULONG refs;

    TRACE("\n");

    EnterCriticalSection(&csRegisteredClassList);
    refs = ++s_COMServerProcessReferences;
    LeaveCriticalSection(&csRegisteredClassList);

    TRACE("refs before: %d\n", refs - 1);

    return refs;
}

/***********************************************************************
 *           CoReleaseServerProcess [OLE32.@]
 *
 * Helper function for decrementing the reference count of a local-server
 * process.
 *
 * RETURNS
 *  New reference count.
 *
 * NOTES
 *  When reference count reaches 0, this function suspends all registered
 *  classes so no new connections are accepted.
 *
 * SEE ALSO
 *  CoAddRefServerProcess(), CoSuspendClassObjects().
 */
ULONG WINAPI CoReleaseServerProcess(void)
{
    ULONG refs;

    TRACE("\n");

    EnterCriticalSection(&csRegisteredClassList);

    refs = --s_COMServerProcessReferences;
    /* FIXME: if (!refs) COM_SuspendClassObjects(); */

    LeaveCriticalSection(&csRegisteredClassList);

    TRACE("refs after: %d\n", refs);

    return refs;
}

/***********************************************************************
 *           CoIsHandlerConnected [OLE32.@]
 *
 * Determines whether a proxy is connected to a remote stub.
 *
 * PARAMS
 *  pUnk [I] Pointer to object that may or may not be connected.
 *
 * RETURNS
 *  TRUE if pUnk is not a proxy or if pUnk is connected to a remote stub, or
 *  FALSE otherwise.
 */
BOOL WINAPI CoIsHandlerConnected(IUnknown *pUnk)
{
    FIXME("%p\n", pUnk);

    return TRUE;
}

/***********************************************************************
 *           CoAllowSetForegroundWindow [OLE32.@]
 *
 */
HRESULT WINAPI CoAllowSetForegroundWindow(IUnknown *pUnk, void *pvReserved)
{
    FIXME("(%p, %p): stub\n", pUnk, pvReserved);
    return S_OK;
}
 
/***********************************************************************
 *           CoQueryProxyBlanket [OLE32.@]
 *
 * Retrieves the security settings being used by a proxy.
 *
 * PARAMS
 *  pProxy        [I] Pointer to the proxy object.
 *  pAuthnSvc     [O] The type of authentication service.
 *  pAuthzSvc     [O] The type of authorization service.
 *  ppServerPrincName [O] Optional. The server prinicple name.
 *  pAuthnLevel   [O] The authentication level.
 *  pImpLevel     [O] The impersonation level.
 *  ppAuthInfo    [O] Information specific to the authorization/authentication service.
 *  pCapabilities [O] Flags affecting the security behaviour.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoCopyProxy, CoSetProxyBlanket.
 */
HRESULT WINAPI CoQueryProxyBlanket(IUnknown *pProxy, DWORD *pAuthnSvc,
    DWORD *pAuthzSvc, OLECHAR **ppServerPrincName, DWORD *pAuthnLevel,
    DWORD *pImpLevel, void **ppAuthInfo, DWORD *pCapabilities)
{
    IClientSecurity *pCliSec;
    HRESULT hr;

    TRACE("%p\n", pProxy);

    hr = IUnknown_QueryInterface(pProxy, &IID_IClientSecurity, (void **)&pCliSec);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_QueryBlanket(pCliSec, pProxy, pAuthnSvc,
                                          pAuthzSvc, ppServerPrincName,
                                          pAuthnLevel, pImpLevel, ppAuthInfo,
                                          pCapabilities);
        IClientSecurity_Release(pCliSec);
    }

    if (FAILED(hr)) ERR("-- failed with 0x%08x\n", hr);
    return hr;
}

/***********************************************************************
 *           CoSetProxyBlanket [OLE32.@]
 *
 * Sets the security settings for a proxy.
 *
 * PARAMS
 *  pProxy       [I] Pointer to the proxy object.
 *  AuthnSvc     [I] The type of authentication service.
 *  AuthzSvc     [I] The type of authorization service.
 *  pServerPrincName [I] The server prinicple name.
 *  AuthnLevel   [I] The authentication level.
 *  ImpLevel     [I] The impersonation level.
 *  pAuthInfo    [I] Information specific to the authorization/authentication service.
 *  Capabilities [I] Flags affecting the security behaviour.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoQueryProxyBlanket, CoCopyProxy.
 */
HRESULT WINAPI CoSetProxyBlanket(IUnknown *pProxy, DWORD AuthnSvc,
    DWORD AuthzSvc, OLECHAR *pServerPrincName, DWORD AuthnLevel,
    DWORD ImpLevel, void *pAuthInfo, DWORD Capabilities)
{
    IClientSecurity *pCliSec;
    HRESULT hr;

    TRACE("%p\n", pProxy);

    hr = IUnknown_QueryInterface(pProxy, &IID_IClientSecurity, (void **)&pCliSec);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_SetBlanket(pCliSec, pProxy, AuthnSvc,
                                        AuthzSvc, pServerPrincName,
                                        AuthnLevel, ImpLevel, pAuthInfo,
                                        Capabilities);
        IClientSecurity_Release(pCliSec);
    }

    if (FAILED(hr)) ERR("-- failed with 0x%08x\n", hr);
    return hr;
}

/***********************************************************************
 *           CoCopyProxy [OLE32.@]
 *
 * Copies a proxy.
 *
 * PARAMS
 *  pProxy [I] Pointer to the proxy object.
 *  ppCopy [O] Copy of the proxy.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoQueryProxyBlanket, CoSetProxyBlanket.
 */
HRESULT WINAPI CoCopyProxy(IUnknown *pProxy, IUnknown **ppCopy)
{
    IClientSecurity *pCliSec;
    HRESULT hr;

    TRACE("%p\n", pProxy);

    hr = IUnknown_QueryInterface(pProxy, &IID_IClientSecurity, (void **)&pCliSec);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_CopyProxy(pCliSec, pProxy, ppCopy);
        IClientSecurity_Release(pCliSec);
    }

    if (FAILED(hr)) ERR("-- failed with 0x%08x\n", hr);
    return hr;
}


/***********************************************************************
 *           CoGetCallContext [OLE32.@]
 *
 * Gets the context of the currently executing server call in the current
 * thread.
 *
 * PARAMS
 *  riid [I] Context interface to return.
 *  ppv  [O] Pointer to memory that will receive the context on return.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoGetCallContext(REFIID riid, void **ppv)
{
    FIXME("(%s, %p): stub\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    return E_NOINTERFACE;
}

/***********************************************************************
 *           CoQueryClientBlanket [OLE32.@]
 *
 * Retrieves the authentication information about the client of the currently
 * executing server call in the current thread.
 *
 * PARAMS
 *  pAuthnSvc     [O] Optional. The type of authentication service.
 *  pAuthzSvc     [O] Optional. The type of authorization service.
 *  pServerPrincName [O] Optional. The server prinicple name.
 *  pAuthnLevel   [O] Optional. The authentication level.
 *  pImpLevel     [O] Optional. The impersonation level.
 *  pPrivs        [O] Optional. Information about the privileges of the client.
 *  pCapabilities [IO] Optional. Flags affecting the security behaviour.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoImpersonateClient, CoRevertToSelf, CoGetCallContext.
 */
HRESULT WINAPI CoQueryClientBlanket(
    DWORD *pAuthnSvc,
    DWORD *pAuthzSvc,
    OLECHAR **pServerPrincName,
    DWORD *pAuthnLevel,
    DWORD *pImpLevel,
    RPC_AUTHZ_HANDLE *pPrivs,
    DWORD *pCapabilities)
{
    IServerSecurity *pSrvSec;
    HRESULT hr;

    TRACE("(%p, %p, %p, %p, %p, %p, %p)\n",
        pAuthnSvc, pAuthzSvc, pServerPrincName, pAuthnLevel, pImpLevel,
        pPrivs, pCapabilities);

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&pSrvSec);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_QueryBlanket(
            pSrvSec, pAuthnSvc, pAuthzSvc, pServerPrincName, pAuthnLevel,
            pImpLevel, pPrivs, pCapabilities);
        IServerSecurity_Release(pSrvSec);
    }

    return hr;
}

/***********************************************************************
 *           CoImpersonateClient [OLE32.@]
 *
 * Impersonates the client of the currently executing server call in the
 * current thread.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * NOTES
 *  If this function fails then the current thread will not be impersonating
 *  the client and all actions will take place on behalf of the server.
 *  Therefore, it is important to check the return value from this function.
 *
 * SEE ALSO
 *  CoRevertToSelf, CoQueryClientBlanket, CoGetCallContext.
 */
HRESULT WINAPI CoImpersonateClient(void)
{
    IServerSecurity *pSrvSec;
    HRESULT hr;

    TRACE("\n");

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&pSrvSec);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_ImpersonateClient(pSrvSec);
        IServerSecurity_Release(pSrvSec);
    }

    return hr;
}

/***********************************************************************
 *           CoRevertToSelf [OLE32.@]
 *
 * Ends the impersonation of the client of the currently executing server
 * call in the current thread.
 *
 * PARAMS
 *  None.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  CoImpersonateClient, CoQueryClientBlanket, CoGetCallContext.
 */
HRESULT WINAPI CoRevertToSelf(void)
{
    IServerSecurity *pSrvSec;
    HRESULT hr;

    TRACE("\n");

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&pSrvSec);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_RevertToSelf(pSrvSec);
        IServerSecurity_Release(pSrvSec);
    }

    return hr;
}

static BOOL COM_PeekMessage(struct apartment *apt, MSG *msg)
{
    /* first try to retrieve messages for incoming COM calls to the apartment window */
    return PeekMessageW(msg, apt->win, WM_USER, WM_APP - 1, PM_REMOVE|PM_NOYIELD) ||
           /* next retrieve other messages necessary for the app to remain responsive */
           PeekMessageW(msg, NULL, 0, 0, PM_QS_PAINT|PM_QS_POSTMESSAGE|PM_REMOVE|PM_NOYIELD);
}

/***********************************************************************
 *           CoWaitForMultipleHandles [OLE32.@]
 *
 * Waits for one or more handles to become signaled.
 *
 * PARAMS
 *  dwFlags   [I] Flags. See notes.
 *  dwTimeout [I] Timeout in milliseconds.
 *  cHandles  [I] Number of handles pointed to by pHandles.
 *  pHandles  [I] Handles to wait for.
 *  lpdwindex [O] Index of handle that was signaled.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: RPC_S_CALLPENDING on timeout.
 *
 * NOTES
 *
 * The dwFlags parameter can be zero or more of the following:
 *| COWAIT_WAITALL - Wait for all of the handles to become signaled.
 *| COWAIT_ALERTABLE - Allows a queued APC to run during the wait.
 *
 * SEE ALSO
 *  MsgWaitForMultipleObjects, WaitForMultipleObjects.
 */
HRESULT WINAPI CoWaitForMultipleHandles(DWORD dwFlags, DWORD dwTimeout,
    ULONG cHandles, LPHANDLE pHandles, LPDWORD lpdwindex)
{
    HRESULT hr = S_OK;
    DWORD start_time = GetTickCount();
    APARTMENT *apt = COM_CurrentApt();
    BOOL message_loop = apt && !apt->multi_threaded;

    TRACE("(0x%08x, 0x%08x, %d, %p, %p)\n", dwFlags, dwTimeout, cHandles,
        pHandles, lpdwindex);

    while (TRUE)
    {
        DWORD now = GetTickCount();
        DWORD res;

        if ((dwTimeout != INFINITE) && (start_time + dwTimeout >= now))
        {
            hr = RPC_S_CALLPENDING;
            break;
        }

        if (message_loop)
        {
            DWORD wait_flags = (dwFlags & COWAIT_WAITALL) ? MWMO_WAITALL : 0 |
                    (dwFlags & COWAIT_ALERTABLE ) ? MWMO_ALERTABLE : 0;

            TRACE("waiting for rpc completion or window message\n");

            res = MsgWaitForMultipleObjectsEx(cHandles, pHandles,
                (dwTimeout == INFINITE) ? INFINITE : start_time + dwTimeout - now,
                QS_ALLINPUT, wait_flags);

            if (res == WAIT_OBJECT_0 + cHandles)  /* messages available */
            {
                MSG msg;

                /* call message filter */

                if (COM_CurrentApt()->filter)
                {
                    PENDINGTYPE pendingtype =
                        COM_CurrentInfo()->pending_call_count_server ?
                            PENDINGTYPE_NESTED : PENDINGTYPE_TOPLEVEL;
                    DWORD be_handled = IMessageFilter_MessagePending(
                        COM_CurrentApt()->filter, 0 /* FIXME */,
                        now - start_time, pendingtype);
                    TRACE("IMessageFilter_MessagePending returned %d\n", be_handled);
                    switch (be_handled)
                    {
                    case PENDINGMSG_CANCELCALL:
                        WARN("call canceled\n");
                        hr = RPC_E_CALL_CANCELED;
                        break;
                    case PENDINGMSG_WAITNOPROCESS:
                    case PENDINGMSG_WAITDEFPROCESS:
                    default:
                        /* FIXME: MSDN is very vague about the difference
                         * between WAITNOPROCESS and WAITDEFPROCESS - there
                         * appears to be none, so it is possibly a left-over
                         * from the 16-bit world. */
                        break;
                    }
                }

                /* note: using "if" here instead of "while" might seem less
                 * efficient, but only if we are optimising for quick delivery
                 * of pending messages, rather than quick completion of the
                 * COM call */
                if (COM_PeekMessage(apt, &msg))
                {
                    TRACE("received message whilst waiting for RPC: 0x%04x\n", msg.message);
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                    if (msg.message == WM_QUIT)
                    {
                        TRACE("resending WM_QUIT to outer message loop\n");
                        PostQuitMessage(msg.wParam);
                        /* no longer need to process messages */
                        message_loop = FALSE;
                    }
                }
                continue;
            }
        }
        else
        {
            TRACE("waiting for rpc completion\n");

            res = WaitForMultipleObjectsEx(cHandles, pHandles,
                (dwFlags & COWAIT_WAITALL) ? TRUE : FALSE,
                (dwTimeout == INFINITE) ? INFINITE : start_time + dwTimeout - now,
                (dwFlags & COWAIT_ALERTABLE) ? TRUE : FALSE);
        }

        if ((res >= WAIT_OBJECT_0) && (res < WAIT_OBJECT_0 + cHandles))
        {
            /* handle signaled, store index */
            *lpdwindex = (res - WAIT_OBJECT_0);
            break;
        }
        else if (res == WAIT_TIMEOUT)
        {
            hr = RPC_S_CALLPENDING;
            break;
        }
        else
        {
            ERR("Unexpected wait termination: %d, %d\n", res, GetLastError());
            hr = E_UNEXPECTED;
            break;
        }
    }
    TRACE("-- 0x%08x\n", hr);
    return hr;
}


/***********************************************************************
 *           CoGetObject [OLE32.@]
 *
 * Gets the object named by coverting the name to a moniker and binding to it.
 *
 * PARAMS
 *  pszName      [I] String representing the object.
 *  pBindOptions [I] Parameters affecting the binding to the named object.
 *  riid         [I] Interface to bind to on the objecct.
 *  ppv          [O] On output, the interface riid of the object represented
 *                   by pszName.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *  MkParseDisplayName.
 */
HRESULT WINAPI CoGetObject(LPCWSTR pszName, BIND_OPTS *pBindOptions,
    REFIID riid, void **ppv)
{
    IBindCtx *pbc;
    HRESULT hr;

    *ppv = NULL;

    hr = CreateBindCtx(0, &pbc);
    if (SUCCEEDED(hr))
    {
        if (pBindOptions)
            hr = IBindCtx_SetBindOptions(pbc, pBindOptions);

        if (SUCCEEDED(hr))
        {
            ULONG chEaten;
            IMoniker *pmk;

            hr = MkParseDisplayName(pbc, pszName, &chEaten, &pmk);
            if (SUCCEEDED(hr))
            {
                hr = IMoniker_BindToObject(pmk, pbc, NULL, riid, ppv);
                IMoniker_Release(pmk);
            }
        }

        IBindCtx_Release(pbc);
    }
    return hr;
}

/***********************************************************************
 *           CoRegisterChannelHook [OLE32.@]
 *
 * Registers a process-wide hook that is called during ORPC calls.
 *
 * PARAMS
 *  guidExtension [I] GUID of the channel hook to register.
 *  pChannelHook  [I] Channel hook object to register.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoRegisterChannelHook(REFGUID guidExtension, IChannelHook *pChannelHook)
{
    TRACE("(%s, %p)\n", debugstr_guid(guidExtension), pChannelHook);

    return RPC_RegisterChannelHook(guidExtension, pChannelHook);
}

/***********************************************************************
 *		DllMain (OLE32.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID fImpLoad)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, fImpLoad);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        OLE32_hInstance = hinstDLL;
        COMPOBJ_InitProcess();
	if (TRACE_ON(ole)) CoRegisterMallocSpy((LPVOID)-1);
	break;

    case DLL_PROCESS_DETACH:
        if (TRACE_ON(ole)) CoRevokeMallocSpy();
        OLEDD_UnInitialize();
        COMPOBJ_UninitProcess();
        RPC_UnregisterAllChannelHooks();
        OLE32_hInstance = 0;
	break;

    case DLL_THREAD_DETACH:
        COM_TlsDestroy();
        break;
    }
    return TRUE;
}

/* NOTE: DllRegisterServer and DllUnregisterServer are in regsvr.c */
