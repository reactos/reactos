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
 */

#include "precomp.h"

#include <ctxtcall.h>
#include <dde.h>

WINE_DEFAULT_DEBUG_CHANNEL(ole);

#undef ARRAYSIZE
#define ARRAYSIZE(array) (sizeof(array)/sizeof((array)[0]))

/****************************************************************************
 * This section defines variables internal to the COM module.
 */

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

enum comclass_threadingmodel
{
    ThreadingModel_Apartment = 1,
    ThreadingModel_Free      = 2,
    ThreadingModel_No        = 3,
    ThreadingModel_Both      = 4,
    ThreadingModel_Neutral   = 5
};

enum comclass_miscfields
{
    MiscStatus          = 1,
    MiscStatusIcon      = 2,
    MiscStatusContent   = 4,
    MiscStatusThumbnail = 8,
    MiscStatusDocPrint  = 16
};

struct comclassredirect_data
{
    ULONG size;
    BYTE  res;
    BYTE  miscmask;
    BYTE  res1[2];
    DWORD model;
    GUID  clsid;
    GUID  alias;
    GUID  clsid2;
    GUID  tlbid;
    ULONG name_len;
    ULONG name_offset;
    ULONG progid_len;
    ULONG progid_offset;
    ULONG clrdata_len;
    ULONG clrdata_offset;
    DWORD miscstatus;
    DWORD miscstatuscontent;
    DWORD miscstatusthumbnail;
    DWORD miscstatusicon;
    DWORD miscstatusdocprint;
};

struct ifacepsredirect_data
{
    ULONG size;
    DWORD mask;
    GUID  iid;
    ULONG nummethods;
    GUID  tlbid;
    GUID  base;
    ULONG name_len;
    ULONG name_offset;
};

struct progidredirect_data
{
    ULONG size;
    DWORD reserved;
    ULONG clsid_offset;
};

struct class_reg_data
{
    union
    {
        struct
        {
            struct comclassredirect_data *data;
            void *section;
            HANDLE hactctx;
        } actctx;
        HKEY hkey;
    } u;
    BOOL hkey;
};

struct registered_psclsid
{
    struct list entry;
    IID iid;
    CLSID clsid;
};

/*
 * This is a marshallable object exposing registered local servers.
 * IServiceProvider is used only because it happens meet requirements
 * and already has proxy/stub code. If more functionality is needed,
 * a custom interface may be used instead.
 */
struct LocalServer
{
    IServiceProvider IServiceProvider_iface;
    LONG ref;
    APARTMENT *apt;
    IStream *marshal_stream;
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
 *       means that parts of this will be exported to rpcss.
 */
typedef struct tagRegisteredClass
{
  struct list entry;
  CLSID     classIdentifier;
  OXID      apartment_id;
  LPUNKNOWN classObject;
  DWORD     runContext;
  DWORD     connectFlags;
  DWORD     dwCookie;
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

static inline enum comclass_miscfields dvaspect_to_miscfields(DWORD aspect)
{
    switch (aspect)
    {
    case DVASPECT_CONTENT:
        return MiscStatusContent;
    case DVASPECT_THUMBNAIL:
        return MiscStatusThumbnail;
    case DVASPECT_ICON:
        return MiscStatusIcon;
    case DVASPECT_DOCPRINT:
        return MiscStatusDocPrint;
    default:
        return MiscStatus;
    };
}

BOOL actctx_get_miscstatus(const CLSID *clsid, DWORD aspect, DWORD *status)
{
    ACTCTX_SECTION_KEYED_DATA data;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                              clsid, &data))
    {
        struct comclassredirect_data *comclass = (struct comclassredirect_data*)data.lpData;
        enum comclass_miscfields misc = dvaspect_to_miscfields(aspect);

        if (!(comclass->miscmask & misc))
        {
            if (!(comclass->miscmask & MiscStatus))
            {
                *status = 0;
                return TRUE;
            }
            misc = MiscStatus;
        }

        switch (misc)
        {
        case MiscStatus:
            *status = comclass->miscstatus;
            break;
        case MiscStatusIcon:
            *status = comclass->miscstatusicon;
            break;
        case MiscStatusContent:
            *status = comclass->miscstatuscontent;
            break;
        case MiscStatusThumbnail:
            *status = comclass->miscstatusthumbnail;
            break;
        case MiscStatusDocPrint:
            *status = comclass->miscstatusdocprint;
            break;
        default:
           ;
        };

        return TRUE;
    }
    else
        return FALSE;
}

/* wrapper for NtCreateKey that creates the key recursively if necessary */
static NTSTATUS create_key( HKEY *retkey, ACCESS_MASK access, OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS status = NtCreateKey( (HANDLE *)retkey, access, attr, 0, NULL, 0, NULL );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        HANDLE subkey, root = attr->RootDirectory;
        WCHAR *buffer = attr->ObjectName->Buffer;
        DWORD attrs, pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
        UNICODE_STRING str;

        while (i < len && buffer[i] != '\\') i++;
        if (i == len) return status;

        attrs = attr->Attributes;
        attr->ObjectName = &str;

        while (i < len)
        {
            str.Buffer = buffer + pos;
            str.Length = (i - pos) * sizeof(WCHAR);
            status = NtCreateKey( &subkey, access, attr, 0, NULL, 0, NULL );
            if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
            if (status) return status;
            attr->RootDirectory = subkey;
            while (i < len && buffer[i] == '\\') i++;
            pos = i;
            while (i < len && buffer[i] != '\\') i++;
        }
        str.Buffer = buffer + pos;
        str.Length = (i - pos) * sizeof(WCHAR);
        attr->Attributes = attrs;
        status = NtCreateKey( (PHANDLE)retkey, access, attr, 0, NULL, 0, NULL );
        if (attr->RootDirectory != root) NtClose( attr->RootDirectory );
    }
    return status;
}

#ifdef __REACTOS__
static const WCHAR classes_rootW[] = L"\\REGISTRY\\Machine\\Software\\Classes";
#else
static const WCHAR classes_rootW[] =
    {'M','a','c','h','i','n','e','\\','S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s',0};
#endif

static HKEY classes_root_hkey;

/* create the special HKEY_CLASSES_ROOT key */
static HKEY create_classes_root_hkey(DWORD access)
{
    HKEY hkey, ret = 0;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name;

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &name, classes_rootW );
    if (create_key( &hkey, access, &attr )) return 0;
    TRACE( "%s -> %p\n", debugstr_w(attr.ObjectName->Buffer), hkey );

    if (!(access & KEY_WOW64_64KEY))
    {
        if (!(ret = InterlockedCompareExchangePointer( (void **)&classes_root_hkey, hkey, 0 )))
            ret = hkey;
        else
            NtClose( hkey );  /* somebody beat us to it */
    }
    else
        ret = hkey;
    return ret;
}

/* map the hkey from special root to normal key if necessary */
static inline HKEY get_classes_root_hkey( HKEY hkey, REGSAM access )
{
    HKEY ret = hkey;
    const BOOL is_win64 = sizeof(void*) > sizeof(int);
    const BOOL force_wow32 = is_win64 && (access & KEY_WOW64_32KEY);

    if (hkey == HKEY_CLASSES_ROOT &&
        ((access & KEY_WOW64_64KEY) || !(ret = classes_root_hkey)))
        ret = create_classes_root_hkey(MAXIMUM_ALLOWED | (access & KEY_WOW64_64KEY));
    if (force_wow32 && ret && ret == classes_root_hkey)
    {
        static const WCHAR wow6432nodeW[] = {'W','o','w','6','4','3','2','N','o','d','e',0};
        access &= ~KEY_WOW64_32KEY;
        if (create_classes_key(classes_root_hkey, wow6432nodeW, access, &hkey))
            return 0;
        ret = hkey;
    }

    return ret;
}

LSTATUS create_classes_key( HKEY hkey, const WCHAR *name, REGSAM access, HKEY *retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (!(hkey = get_classes_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    return RtlNtStatusToDosError( create_key( retkey, access, &attr ) );
}

LSTATUS open_classes_key( HKEY hkey, const WCHAR *name, REGSAM access, HKEY *retkey )
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (!(hkey = get_classes_root_hkey( hkey, access ))) return ERROR_INVALID_HANDLE;

    attr.Length = sizeof(attr);
    attr.RootDirectory = hkey;
    attr.ObjectName = &nameW;
    attr.Attributes = 0;
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    return RtlNtStatusToDosError( NtOpenKey( (HANDLE *)retkey, access, &attr ) );
}

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
    DWORD unload_time;
    BOOL multi_threaded;
};

static const WCHAR wszAptWinClass[] = {'O','l','e','M','a','i','n','T','h','r','e','a','d','W','n','d','C','l','a','s','s',' ',
                                       '0','x','#','#','#','#','#','#','#','#',' ',0};

/*****************************************************************************
 * This section contains OpenDllList implementation
 */

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

/* caller must ensure that library_name is not already in the open dll list */
static HRESULT COMPOBJ_DllList_Add(LPCWSTR library_name, OpenDll **ret)
{
    OpenDll *entry;
    int len;
    HRESULT hr = S_OK;
    HANDLE hLibrary;
    DllCanUnloadNowFunc DllCanUnloadNow;
    DllGetClassObjectFunc DllGetClassObject;

    TRACE("%s\n", debugstr_w(library_name));

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

    DllCanUnloadNow = (void *)GetProcAddress(hLibrary, "DllCanUnloadNow");
    /* Note: failing to find DllCanUnloadNow is not a failure */
    DllGetClassObject = (void *)GetProcAddress(hLibrary, "DllGetClassObject");
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
            *ret = entry;
        }
        else
        {
            HeapFree(GetProcessHeap(), 0, entry);
            hr = E_OUTOFMEMORY;
            FreeLibrary(hLibrary);
        }
    }

    LeaveCriticalSection( &csOpenDllList );

    return hr;
}

/* pass FALSE for free_entry to release a reference without destroying the
 * entry if it reaches zero or TRUE otherwise */
static void COMPOBJ_DllList_ReleaseRef(OpenDll *entry, BOOL free_entry)
{
    if (!InterlockedDecrement(&entry->refs) && free_entry)
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

/* frees memory associated with active dll list */
static void COMPOBJ_DllList_Free(void)
{
    OpenDll *entry, *cursor2;
    EnterCriticalSection(&csOpenDllList);
    LIST_FOR_EACH_ENTRY_SAFE(entry, cursor2, &openDllList, OpenDll, entry)
    {
        list_remove(&entry->entry);

        HeapFree(GetProcessHeap(), 0, entry->library_name);
        HeapFree(GetProcessHeap(), 0, entry);
    }
    LeaveCriticalSection(&csOpenDllList);
    DeleteCriticalSection(&csOpenDllList);
}

/******************************************************************************
 * Manage apartments.
 */

static DWORD apartment_addref(struct apartment *apt)
{
    DWORD refs = InterlockedIncrement(&apt->refs);
    TRACE("%s: before = %d\n", wine_dbgstr_longlong(apt->oxid), refs - 1);
    return refs;
}

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

            if (apt->main)
                apartment_createwindowifneeded(apt);
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

static inline BOOL apartment_is_model(const APARTMENT *apt, DWORD model)
{
    return (apt->multi_threaded == !(model & COINIT_APARTMENTTHREADED));
}

static void COM_RevokeRegisteredClassObject(RegisteredClass *curClass)
{
    list_remove(&curClass->entry);

    if (curClass->runContext & CLSCTX_LOCAL_SERVER)
        RPC_StopLocalServer(curClass->RpcRegistration);

    IUnknown_Release(curClass->classObject);
    HeapFree(GetProcessHeap(), 0, curClass);
}

static void COM_RevokeAllClasses(const struct apartment *apt)
{
  RegisteredClass *curClass, *cursor;

  EnterCriticalSection( &csRegisteredClassList );

  LIST_FOR_EACH_ENTRY_SAFE(curClass, cursor, &RegisteredClassList, RegisteredClass, entry)
  {
    if (curClass->apartment_id == apt->oxid)
      COM_RevokeRegisteredClassObject(curClass);
  }

  LeaveCriticalSection( &csRegisteredClassList );
}

/******************************************************************************
 * Implementation of the manual reset event object. (CLSID_ManualResetEvent)
 */

typedef struct ManualResetEvent {
    ISynchronize        ISynchronize_iface;
    ISynchronizeHandle  ISynchronizeHandle_iface;
    LONG ref;
    HANDLE event;
} MREImpl;

static inline MREImpl *impl_from_ISynchronize(ISynchronize *iface)
{
    return CONTAINING_RECORD(iface, MREImpl, ISynchronize_iface);
}

static HRESULT WINAPI ISynchronize_fnQueryInterface(ISynchronize *iface, REFIID riid, void **ppv)
{
    MREImpl *This = impl_from_ISynchronize(iface);

    TRACE("%p (%s, %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_ISynchronize)) {
        *ppv = &This->ISynchronize_iface;
    }else if(IsEqualGUID(riid, &IID_ISynchronizeHandle)) {
        *ppv = &This->ISynchronizeHandle_iface;
    }else {
        ERR("Unknown interface %s requested.\n", debugstr_guid(riid));
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI ISynchronize_fnAddRef(ISynchronize *iface)
{
    MREImpl *This = impl_from_ISynchronize(iface);
    LONG ref = InterlockedIncrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    return ref;
}

static ULONG WINAPI ISynchronize_fnRelease(ISynchronize *iface)
{
    MREImpl *This = impl_from_ISynchronize(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p - ref %d\n", This, ref);

    if(!ref)
    {
        CloseHandle(This->event);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI ISynchronize_fnWait(ISynchronize *iface, DWORD dwFlags, DWORD dwMilliseconds)
{
    MREImpl *This = impl_from_ISynchronize(iface);
    UINT index;
    TRACE("%p (%08x, %08x)\n", This, dwFlags, dwMilliseconds);
    return CoWaitForMultipleHandles(dwFlags, dwMilliseconds, 1, &This->event, &index);
}

static HRESULT WINAPI ISynchronize_fnSignal(ISynchronize *iface)
{
    MREImpl *This = impl_from_ISynchronize(iface);
    TRACE("%p\n", This);
    SetEvent(This->event);
    return S_OK;
}

static HRESULT WINAPI ISynchronize_fnReset(ISynchronize *iface)
{
    MREImpl *This = impl_from_ISynchronize(iface);
    TRACE("%p\n", This);
    ResetEvent(This->event);
    return S_OK;
}

static ISynchronizeVtbl vt_ISynchronize = {
    ISynchronize_fnQueryInterface,
    ISynchronize_fnAddRef,
    ISynchronize_fnRelease,
    ISynchronize_fnWait,
    ISynchronize_fnSignal,
    ISynchronize_fnReset
};

static inline MREImpl *impl_from_ISynchronizeHandle(ISynchronizeHandle *iface)
{
    return CONTAINING_RECORD(iface, MREImpl, ISynchronizeHandle_iface);
}

static HRESULT WINAPI SynchronizeHandle_QueryInterface(ISynchronizeHandle *iface, REFIID riid, void **ppv)
{
    MREImpl *This = impl_from_ISynchronizeHandle(iface);
    return ISynchronize_QueryInterface(&This->ISynchronize_iface, riid, ppv);
}

static ULONG WINAPI SynchronizeHandle_AddRef(ISynchronizeHandle *iface)
{
    MREImpl *This = impl_from_ISynchronizeHandle(iface);
    return ISynchronize_AddRef(&This->ISynchronize_iface);
}

static ULONG WINAPI SynchronizeHandle_Release(ISynchronizeHandle *iface)
{
    MREImpl *This = impl_from_ISynchronizeHandle(iface);
    return ISynchronize_Release(&This->ISynchronize_iface);
}

static HRESULT WINAPI SynchronizeHandle_GetHandle(ISynchronizeHandle *iface, HANDLE *ph)
{
    MREImpl *This = impl_from_ISynchronizeHandle(iface);

    *ph = This->event;
    return S_OK;
}

static const ISynchronizeHandleVtbl SynchronizeHandleVtbl = {
    SynchronizeHandle_QueryInterface,
    SynchronizeHandle_AddRef,
    SynchronizeHandle_Release,
    SynchronizeHandle_GetHandle
};

static HRESULT ManualResetEvent_Construct(IUnknown *punkouter, REFIID iid, void **ppv)
{
    MREImpl *This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MREImpl));
    HRESULT hr;

    if(punkouter)
        FIXME("Aggregation not implemented.\n");

    This->ref = 1;
    This->ISynchronize_iface.lpVtbl = &vt_ISynchronize;
    This->ISynchronizeHandle_iface.lpVtbl = &SynchronizeHandleVtbl;
    This->event = CreateEventW(NULL, TRUE, FALSE, NULL);

    hr = ISynchronize_QueryInterface(&This->ISynchronize_iface, iid, ppv);
    ISynchronize_Release(&This->ISynchronize_iface);
    return hr;
}

static inline LocalServer *impl_from_IServiceProvider(IServiceProvider *iface)
{
    return CONTAINING_RECORD(iface, LocalServer, IServiceProvider_iface);
}

static HRESULT WINAPI LocalServer_QueryInterface(IServiceProvider *iface, REFIID riid, void **ppv)
{
    LocalServer *This = impl_from_IServiceProvider(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if(IsEqualGUID(riid, &IID_IUnknown) || IsEqualGUID(riid, &IID_IServiceProvider)) {
        *ppv = &This->IServiceProvider_iface;
    }else {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI LocalServer_AddRef(IServiceProvider *iface)
{
    LocalServer *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI LocalServer_Release(IServiceProvider *iface)
{
    LocalServer *This = impl_from_IServiceProvider(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if(!ref) {
        assert(!This->apt);
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI LocalServer_QueryService(IServiceProvider *iface, REFGUID guid, REFIID riid, void **ppv)
{
    LocalServer *This = impl_from_IServiceProvider(iface);
    APARTMENT *apt = COM_CurrentApt();
    RegisteredClass *iter;
    HRESULT hres = E_FAIL;

    TRACE("(%p)->(%s %s %p)\n", This, debugstr_guid(guid), debugstr_guid(riid), ppv);

    if(!This->apt)
        return E_UNEXPECTED;

    EnterCriticalSection(&csRegisteredClassList);

    LIST_FOR_EACH_ENTRY(iter, &RegisteredClassList, RegisteredClass, entry) {
        if(iter->apartment_id == apt->oxid
           && (iter->runContext & CLSCTX_LOCAL_SERVER)
           && IsEqualGUID(&iter->classIdentifier, guid)) {
            hres = IUnknown_QueryInterface(iter->classObject, riid, ppv);
            break;
        }
    }

    LeaveCriticalSection( &csRegisteredClassList );

    return hres;
}

static const IServiceProviderVtbl LocalServerVtbl = {
    LocalServer_QueryInterface,
    LocalServer_AddRef,
    LocalServer_Release,
    LocalServer_QueryService
};

static HRESULT get_local_server_stream(APARTMENT *apt, IStream **ret)
{
    HRESULT hres = S_OK;

    EnterCriticalSection(&apt->cs);

    if(!apt->local_server) {
        LocalServer *obj;

        obj = heap_alloc(sizeof(*obj));
        if(obj) {
            obj->IServiceProvider_iface.lpVtbl = &LocalServerVtbl;
            obj->ref = 1;
            obj->apt = apt;

            hres = CreateStreamOnHGlobal(0, TRUE, &obj->marshal_stream);
            if(SUCCEEDED(hres)) {
                hres = CoMarshalInterface(obj->marshal_stream, &IID_IServiceProvider, (IUnknown*)&obj->IServiceProvider_iface,
                        MSHCTX_LOCAL, NULL, MSHLFLAGS_TABLESTRONG);
                if(FAILED(hres))
                    IStream_Release(obj->marshal_stream);
            }

            if(SUCCEEDED(hres))
                apt->local_server = obj;
            else
                heap_free(obj);
        }else {
            hres = E_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hres))
        hres = IStream_Clone(apt->local_server->marshal_stream, ret);

    LeaveCriticalSection(&apt->cs);

    if(FAILED(hres))
        ERR("Failed: %08x\n", hres);
    return hres;
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
 * NOTES
 *  Must be called from the same apartment that called CoRegisterClassObject(),
 *  otherwise it will fail with RPC_E_WRONG_THREAD.
 *
 * SEE ALSO
 *  CoRegisterClassObject
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoRevokeClassObject(
        DWORD dwRegister)
{
  HRESULT hr = E_INVALIDARG;
  RegisteredClass *curClass;
  APARTMENT *apt;

  TRACE("(%08x)\n",dwRegister);

  apt = COM_CurrentApt();
  if (!apt)
  {
    ERR("COM was not initialized\n");
    return CO_E_NOTINITIALIZED;
  }

  EnterCriticalSection( &csRegisteredClassList );

  LIST_FOR_EACH_ENTRY(curClass, &RegisteredClassList, RegisteredClass, entry)
  {
    /*
     * Check if we have a match on the cookie.
     */
    if (curClass->dwCookie == dwRegister)
    {
      if (curClass->apartment_id == apt->oxid)
      {
          COM_RevokeRegisteredClassObject(curClass);
          hr = S_OK;
      }
      else
      {
          ERR("called from wrong apartment, should be called from %s\n",
              wine_dbgstr_longlong(curClass->apartment_id));
          hr = RPC_E_WRONG_THREAD;
      }
      break;
    }
  }

  LeaveCriticalSection( &csRegisteredClassList );

  return hr;
}

/* frees unused libraries loaded by apartment_getclassobject by calling the
 * DLL's DllCanUnloadNow entry point */
static void apartment_freeunusedlibraries(struct apartment *apt, DWORD delay)
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
                COMPOBJ_DllList_ReleaseRef(entry->dll, TRUE);
                HeapFree(GetProcessHeap(), 0, entry);
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

        if(apt->local_server) {
            LocalServer *local_server = apt->local_server;
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
        COM_RevokeAllClasses(apt);

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

        if (apt->filter) IMessageFilter_Release(apt->filter);

        /* free as many unused libraries as possible... */
        apartment_freeunusedlibraries(apt, 0);

        /* ... and free the memory for the apartment loaded dll entry and
         * release the dll list reference without freeing the library for the
         * rest */
        while ((cursor = list_head(&apt->loaded_dlls)))
        {
            struct apartment_loaded_dll *apartment_loaded_dll = LIST_ENTRY(cursor, struct apartment_loaded_dll, entry);
            COMPOBJ_DllList_ReleaseRef(apartment_loaded_dll->dll, FALSE);
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

/* gets the main apartment if it exists. The caller must
 * release the reference from the apartment as soon as the apartment pointer
 * is no longer required. */
static APARTMENT *apartment_findmain(void)
{
    APARTMENT *result;

    EnterCriticalSection(&csApartment);

    result = MainApartment;
    if (result) apartment_addref(result);

    LeaveCriticalSection(&csApartment);

    return result;
}

/* gets the multi-threaded apartment if it exists. The caller must
 * release the reference from the apartment as soon as the apartment pointer
 * is no longer required. */
static APARTMENT *apartment_find_multi_threaded(void)
{
    APARTMENT *result = NULL;
    struct list *cursor;

    EnterCriticalSection(&csApartment);

    LIST_FOR_EACH( cursor, &apts )
    {
        struct apartment *apt = LIST_ENTRY( cursor, struct apartment, entry );
        if (apt->multi_threaded)
        {
            result = apt;
            apartment_addref(result);
            break;
        }
    }

    LeaveCriticalSection(&csApartment);
    return result;
}

/* gets the specified class object by loading the appropriate DLL, if
 * necessary and calls the DllGetClassObject function for the DLL */
static HRESULT apartment_getclassobject(struct apartment *apt, LPCWSTR dllpath,
                                        BOOL apartment_threaded,
                                        REFCLSID rclsid, REFIID riid, void **ppv)
{
    static const WCHAR wszOle32[] = {'o','l','e','3','2','.','d','l','l',0};
    HRESULT hr = S_OK;
    BOOL found = FALSE;
    struct apartment_loaded_dll *apartment_loaded_dll;

    if (!strcmpiW(dllpath, wszOle32))
    {
        /* we don't need to control the lifetime of this dll, so use the local
         * implementation of DllGetClassObject directly */
        TRACE("calling ole32!DllGetClassObject\n");
        hr = DllGetClassObject(rclsid, riid, ppv);

        if (hr != S_OK)
            ERR("DllGetClassObject returned error 0x%08x\n", hr);

        return hr;
    }

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
            apartment_loaded_dll->unload_time = 0;
            apartment_loaded_dll->multi_threaded = FALSE;
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
        /* one component being multi-threaded overrides any number of
         * apartment-threaded components */
        if (!apartment_threaded)
            apartment_loaded_dll->multi_threaded = TRUE;

        TRACE("calling DllGetClassObject %p\n", apartment_loaded_dll->dll->DllGetClassObject);
        /* OK: get the ClassObject */
        hr = apartment_loaded_dll->dll->DllGetClassObject(rclsid, riid, ppv);

        if (hr != S_OK)
            ERR("DllGetClassObject returned error 0x%08x\n", hr);
    }

    return hr;
}

/***********************************************************************
 *	COM_RegReadPath	[internal]
 *
 *	Reads a registry value and expands it when necessary
 */
static DWORD COM_RegReadPath(const struct class_reg_data *regdata, WCHAR *dst, DWORD dstlen)
{
    DWORD ret;

    if (regdata->hkey)
    {
	DWORD keytype;
	WCHAR src[MAX_PATH];
	DWORD dwLength = dstlen * sizeof(WCHAR);

        if( (ret = RegQueryValueExW(regdata->u.hkey, NULL, NULL, &keytype, (BYTE*)src, &dwLength)) == ERROR_SUCCESS ) {
            if (keytype == REG_EXPAND_SZ) {
              if (dstlen <= ExpandEnvironmentStringsW(src, dst, dstlen)) ret = ERROR_MORE_DATA;
            } else {
              const WCHAR *quote_start;
              quote_start = strchrW(src, '\"');
              if (quote_start) {
                const WCHAR *quote_end = strchrW(quote_start + 1, '\"');
                if (quote_end) {
                  memmove(src, quote_start + 1,
                          (quote_end - quote_start - 1) * sizeof(WCHAR));
                  src[quote_end - quote_start - 1] = '\0';
                }
              }
              lstrcpynW(dst, src, dstlen);
            }
        }
	return ret;
    }
    else
    {
        ULONG_PTR cookie;
        WCHAR *nameW;

        *dst = 0;
        nameW = (WCHAR*)((BYTE*)regdata->u.actctx.section + regdata->u.actctx.data->name_offset);
        ActivateActCtx(regdata->u.actctx.hactctx, &cookie);
        ret = SearchPathW(NULL, nameW, NULL, dstlen, dst, NULL);
        DeactivateActCtx(0, cookie);
        return !*dst;
    }
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

static HRESULT apartment_hostobject(struct apartment *apt,
                                    const struct host_object_params *params)
{
    IUnknown *object;
    HRESULT hr;
    static const LARGE_INTEGER llZero;
    WCHAR dllpath[MAX_PATH+1];

    TRACE("clsid %s, iid %s\n", debugstr_guid(&params->clsid), debugstr_guid(&params->iid));

    if (COM_RegReadPath(&params->regdata, dllpath, ARRAYSIZE(dllpath)) != ERROR_SUCCESS)
    {
        /* failure: CLSID is not found in registry */
        WARN("class %s not registered inproc\n", debugstr_guid(&params->clsid));
        return REGDB_E_CLASSNOTREG;
    }

    hr = apartment_getclassobject(apt, dllpath, params->apartment_threaded,
                                  &params->clsid, &params->iid, (void **)&object);
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

struct host_thread_params
{
    COINIT threading_model;
    HANDLE ready_event;
    HWND apartment_hwnd;
};

/* thread for hosting an object to allow an object to appear to be created in
 * an apartment with an incompatible threading model */
static DWORD CALLBACK apartment_hostobject_thread(LPVOID p)
{
    struct host_thread_params *params = p;
    MSG msg;
    HRESULT hr;
    struct apartment *apt;

    TRACE("\n");

    hr = CoInitializeEx(NULL, params->threading_model);
    if (FAILED(hr)) return hr;

    apt = COM_CurrentApt();
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
static HRESULT apartment_hostobject_in_hostapt(
    struct apartment *apt, BOOL multi_threaded, BOOL main_apartment,
    const struct class_reg_data *regdata, REFCLSID rclsid, REFIID riid, void **ppv)
{
    struct host_object_params params;
    HWND apartment_hwnd = NULL;
    DWORD apartment_tid = 0;
    HRESULT hr;

    if (!multi_threaded && main_apartment)
    {
        APARTMENT *host_apt = apartment_findmain();
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
        APARTMENT *host_apt = apartment_findmain();
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
    wclass.lpszClassName = wszAptWinClass;
    RegisterClassW(&wclass);
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

        hwnd = CreateWindowW(wszAptWinClass, NULL, 0, 0, 0, 0, 0,
                             HWND_MESSAGE, 0, hProxyDll, NULL);
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

/* retrieves the window for the main- or apartment-threaded apartment */
HWND apartment_getwindow(const struct apartment *apt)
{
    assert(!apt->multi_threaded);
    return apt->win;
}

void apartment_joinmta(void)
{
    apartment_addref(MTA);
    COM_CurrentInfo()->apt = MTA;
}

static void COM_TlsDestroy(void)
{
    struct oletls *info = NtCurrentTeb()->ReservedForOle;
    if (info)
    {
        if (info->apt) apartment_release(info->apt);
        if (info->errorinfo) IErrorInfo_Release(info->errorinfo);
        if (info->state) IUnknown_Release(info->state);
        if (info->spy) IInitializeSpy_Release(info->spy);
        if (info->context_token) IObjContext_Release(info->context_token);
        HeapFree(GetProcessHeap(), 0, info);
        NtCurrentTeb()->ReservedForOle = NULL;
    }
}

/******************************************************************************
 *           CoBuildVersion [OLE32.@]
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
 *              CoRegisterInitializeSpy [OLE32.@]
 *
 * Add a Spy that watches CoInitializeEx calls
 *
 * PARAMS
 *  spy [I] Pointer to IUnknown interface that will be QueryInterface'd.
 *  cookie [II] cookie receiver
 *
 * RETURNS
 *  Success: S_OK if not already initialized, S_FALSE otherwise.
 *  Failure: HRESULT code.
 *
 * SEE ALSO
 *   CoInitializeEx
 */
HRESULT WINAPI CoRegisterInitializeSpy(IInitializeSpy *spy, ULARGE_INTEGER *cookie)
{
    struct oletls *info = COM_CurrentInfo();
    HRESULT hr;

    TRACE("(%p, %p)\n", spy, cookie);

    if (!spy || !cookie || !info)
    {
        if (!info)
            WARN("Could not allocate tls\n");
        return E_INVALIDARG;
    }

    if (info->spy)
    {
        FIXME("Already registered?\n");
        return E_UNEXPECTED;
    }

    hr = IInitializeSpy_QueryInterface(spy, &IID_IInitializeSpy, (void **) &info->spy);
    if (SUCCEEDED(hr))
    {
        cookie->QuadPart = (DWORD_PTR)spy;
        return S_OK;
    }
    return hr;
}

/******************************************************************************
 *              CoRevokeInitializeSpy [OLE32.@]
 *
 * Remove a spy that previously watched CoInitializeEx calls
 *
 * PARAMS
 *  cookie [I] The cookie obtained from a previous CoRegisterInitializeSpy call
 *
 * RETURNS
 *  Success: S_OK if a spy is removed
 *  Failure: E_INVALIDARG
 *
 * SEE ALSO
 *   CoInitializeEx
 */
HRESULT WINAPI CoRevokeInitializeSpy(ULARGE_INTEGER cookie)
{
    struct oletls *info = COM_CurrentInfo();
    TRACE("(%s)\n", wine_dbgstr_longlong(cookie.QuadPart));

    if (!info || !info->spy || cookie.QuadPart != (DWORD_PTR)info->spy)
        return E_INVALIDARG;

    IInitializeSpy_Release(info->spy);
    info->spy = NULL;
    return S_OK;
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
HRESULT WINAPI DECLSPEC_HOTPATCH CoInitializeEx(LPVOID lpReserved, DWORD dwCoInit)
{
  struct oletls *info = COM_CurrentInfo();
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

  if (info->spy)
      IInitializeSpy_PreInitialize(info->spy, dwCoInit, info->inits);

  if (!(apt = info->apt))
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

  info->inits++;

  if (info->spy)
      IInitializeSpy_PostInitialize(info->spy, hr, dwCoInit, info->inits);

  return hr;
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
void WINAPI DECLSPEC_HOTPATCH CoUninitialize(void)
{
  struct oletls * info = COM_CurrentInfo();
  LONG lCOMRefCnt;

  TRACE("()\n");

  /* will only happen on OOM */
  if (!info) return;

  if (info->spy)
      IInitializeSpy_PreUninitialize(info->spy, info->inits);

  /* sanity check */
  if (!info->inits)
  {
    ERR("Mismatched CoUninitialize\n");

    if (info->spy)
        IInitializeSpy_PostUninitialize(info->spy, info->inits);
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
  }
  else if (lCOMRefCnt<1) {
    ERR( "CoUninitialize() - not CoInitialized.\n" );
    InterlockedExchangeAdd(&s_COMLockCount,1); /* restore the lock count. */
  }
  if (info->spy)
      IInitializeSpy_PostUninitialize(info->spy, info->inits);
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
    struct stub_manager *manager;
    HRESULT hr;
    IMarshal *marshal;
    APARTMENT *apt;

    TRACE("(%p, 0x%08x)\n", lpUnk, reserved);

    if (!lpUnk) return E_INVALIDARG;

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

    manager = get_stub_manager_from_object(apt, lpUnk, FALSE);
    if (manager) {
        stub_manager_disconnect(manager);
        /* Release stub manager twice, to remove the apartment reference. */
        stub_manager_int_release(manager);
        stub_manager_int_release(manager);
    }

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
    DWORD status;

    if(!pguid) return E_INVALIDARG;

    status = UuidCreate(pguid);
    if (status == RPC_S_OK || status == RPC_S_UUID_LOCAL_ONLY) return S_OK;
    return HRESULT_FROM_WIN32( status );
}

static inline BOOL is_valid_hex(WCHAR c)
{
    if (!(((c >= '0') && (c <= '9'))  ||
          ((c >= 'a') && (c <= 'f'))  ||
          ((c >= 'A') && (c <= 'F'))))
        return FALSE;
    return TRUE;
}

static const BYTE guid_conv_table[256] =
{
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x00 */
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x10 */
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x20 */
  0,   1,   2,   3,   4,   5,   6, 7, 8, 9, 0, 0, 0, 0, 0, 0, /* 0x30 */
  0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x40 */
  0,   0,   0,   0,   0,   0,   0, 0, 0, 0, 0, 0, 0, 0, 0, 0, /* 0x50 */
  0, 0xa, 0xb, 0xc, 0xd, 0xe, 0xf                             /* 0x60 */
};

/* conversion helper for CLSIDFromString/IIDFromString */
static BOOL guid_from_string(LPCWSTR s, GUID *id)
{
  int	i;

  if (!s || s[0]!='{') {
    memset( id, 0, sizeof (CLSID) );
    if(!s) return TRUE;
    return FALSE;
  }

  TRACE("%s -> %p\n", debugstr_w(s), id);

  /* in form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

  id->Data1 = 0;
  for (i = 1; i < 9; i++) {
    if (!is_valid_hex(s[i])) return FALSE;
    id->Data1 = (id->Data1 << 4) | guid_conv_table[s[i]];
  }
  if (s[9]!='-') return FALSE;

  id->Data2 = 0;
  for (i = 10; i < 14; i++) {
    if (!is_valid_hex(s[i])) return FALSE;
    id->Data2 = (id->Data2 << 4) | guid_conv_table[s[i]];
  }
  if (s[14]!='-') return FALSE;

  id->Data3 = 0;
  for (i = 15; i < 19; i++) {
    if (!is_valid_hex(s[i])) return FALSE;
    id->Data3 = (id->Data3 << 4) | guid_conv_table[s[i]];
  }
  if (s[19]!='-') return FALSE;

  for (i = 20; i < 37; i+=2) {
    if (i == 24) {
      if (s[i]!='-') return FALSE;
      i++;
    }
    if (!is_valid_hex(s[i]) || !is_valid_hex(s[i+1])) return FALSE;
    id->Data4[(i-20)/2] = guid_conv_table[s[i]] << 4 | guid_conv_table[s[i+1]];
  }

  if (s[37] == '}' && s[38] == '\0')
    return TRUE;

  return FALSE;
}

/*****************************************************************************/

static HRESULT clsid_from_string_reg(LPCOLESTR progid, CLSID *clsid)
{
    static const WCHAR clsidW[] = { '\\','C','L','S','I','D',0 };
    WCHAR buf2[CHARS_IN_GUID];
    LONG buf2len = sizeof(buf2);
    HKEY xhkey;
    WCHAR *buf;

    memset(clsid, 0, sizeof(*clsid));
    buf = HeapAlloc( GetProcessHeap(),0,(strlenW(progid)+8) * sizeof(WCHAR) );
    if (!buf) return E_OUTOFMEMORY;
    strcpyW( buf, progid );
    strcatW( buf, clsidW );
    if (open_classes_key(HKEY_CLASSES_ROOT, buf, MAXIMUM_ALLOWED, &xhkey))
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
    return guid_from_string(buf2, clsid) ? S_OK : CO_E_CLASSSTRING;
}

/******************************************************************************
 *		CLSIDFromString	[OLE32.@]
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
HRESULT WINAPI CLSIDFromString(LPCOLESTR idstr, LPCLSID id )
{
    HRESULT ret = CO_E_CLASSSTRING;
    CLSID tmp_id;

    if (!id)
        return E_INVALIDARG;

    if (guid_from_string(idstr, id))
        return S_OK;

    /* It appears a ProgID is also valid */
    ret = clsid_from_string_reg(idstr, &tmp_id);
    if(SUCCEEDED(ret))
        *id = tmp_id;

    return ret;
}

/******************************************************************************
 *		IIDFromString   [OLE32.@]
 *
 * Converts an interface identifier from its string representation to
 * the IID struct.
 *
 * PARAMS
 *  idstr [I] The string representation of the GUID.
 *  id    [O] IID converted from the string.
 *
 * RETURNS
 *   S_OK on success
 *   CO_E_IIDSTRING if idstr is not a valid IID
 *
 * SEE ALSO
 *  StringFromIID
 */
HRESULT WINAPI IIDFromString(LPCOLESTR s, IID *iid)
{
  TRACE("%s -> %p\n", debugstr_w(s), iid);

  if (!s)
  {
      memset(iid, 0, sizeof(*iid));
      return S_OK;
  }

  /* length mismatch is a special case */
  if (strlenW(s) + 1 != CHARS_IN_GUID)
      return E_INVALIDARG;

  if (s[0] != '{')
      return CO_E_IIDSTRING;

  return guid_from_string(s, iid) ? S_OK : CO_E_IIDSTRING;
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
    if (!(*idstr = CoTaskMemAlloc(CHARS_IN_GUID * sizeof(WCHAR)))) return E_OUTOFMEMORY;
    StringFromGUID2( id, *idstr, CHARS_IN_GUID );
    return S_OK;
}

/******************************************************************************
 *		StringFromGUID2	[OLE32.@]
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
    static const WCHAR formatW[] = { '{','%','0','8','X','-','%','0','4','X','-',
                                     '%','0','4','X','-','%','0','2','X','%','0','2','X','-',
                                     '%','0','2','X','%','0','2','X','%','0','2','X','%','0','2','X',
                                     '%','0','2','X','%','0','2','X','}',0 };
    if (!id || cmax < CHARS_IN_GUID) return 0;
    sprintfW( str, formatW, id->Data1, id->Data2, id->Data3,
              id->Data4[0], id->Data4[1], id->Data4[2], id->Data4[3],
              id->Data4[4], id->Data4[5], id->Data4[6], id->Data4[7] );
    return CHARS_IN_GUID;
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
    res = open_classes_key(HKEY_CLASSES_ROOT, path, keyname ? KEY_READ : access, &key);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_CLASSNOTREG;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    if (!keyname)
    {
        *subkey = key;
        return S_OK;
    }

    res = open_classes_key(key, keyname, access, subkey);
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
    res = open_classes_key(HKEY_CLASSES_ROOT, keyname, access, subkey);
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
HRESULT WINAPI DECLSPEC_HOTPATCH ProgIDFromCLSID(REFCLSID clsid, LPOLESTR *ppszProgID)
{
    static const WCHAR wszProgID[] = {'P','r','o','g','I','D',0};
    ACTCTX_SECTION_KEYED_DATA data;
    HKEY     hkey;
    HRESULT  ret;
    LONG progidlen = 0;

    if (!ppszProgID)
        return E_INVALIDARG;

    *ppszProgID = NULL;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                              clsid, &data))
    {
        struct comclassredirect_data *comclass = (struct comclassredirect_data*)data.lpData;
        if (comclass->progid_len)
        {
            WCHAR *ptrW;

            *ppszProgID = CoTaskMemAlloc(comclass->progid_len + sizeof(WCHAR));
            if (!*ppszProgID) return E_OUTOFMEMORY;

            ptrW = (WCHAR*)((BYTE*)comclass + comclass->progid_offset);
            memcpy(*ppszProgID, ptrW, comclass->progid_len + sizeof(WCHAR));
            return S_OK;
        }
        else
            return REGDB_E_CLASSNOTREG;
    }

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
        if (RegQueryValueW(hkey, NULL, *ppszProgID, &progidlen)) {
          ret = REGDB_E_CLASSNOTREG;
          CoTaskMemFree(*ppszProgID);
          *ppszProgID = NULL;
        }
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
HRESULT WINAPI DECLSPEC_HOTPATCH CLSIDFromProgID(LPCOLESTR progid, LPCLSID clsid)
{
    ACTCTX_SECTION_KEYED_DATA data;

    if (!progid || !clsid)
        return E_INVALIDARG;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION,
                                 progid, &data))
    {
        struct progidredirect_data *progiddata = (struct progidredirect_data*)data.lpData;
        CLSID *alias = (CLSID*)((BYTE*)data.lpSectionBase + progiddata->clsid_offset);
        *clsid = *alias;
        return S_OK;
    }

    return clsid_from_string_reg(progid, clsid);
}

/******************************************************************************
 *              CLSIDFromProgIDEx [OLE32.@]
 */
HRESULT WINAPI CLSIDFromProgIDEx(LPCOLESTR progid, LPCLSID clsid)
{
    FIXME("%s,%p: semi-stub\n", debugstr_w(progid), clsid);

    return CLSIDFromProgID(progid, clsid);
}

static HRESULT get_ps_clsid_from_registry(const WCHAR* path, REGSAM access, CLSID *pclsid)
{
    HKEY hkey;
    WCHAR value[CHARS_IN_GUID];
    DWORD len;

    access |= KEY_READ;

    if (open_classes_key(HKEY_CLASSES_ROOT, path, access, &hkey))
        return REGDB_E_IIDNOTREG;

    len = sizeof(value);
    if (ERROR_SUCCESS != RegQueryValueExW(hkey, NULL, NULL, NULL, (BYTE *)value, &len))
        return REGDB_E_IIDNOTREG;
    RegCloseKey(hkey);

    if (CLSIDFromString(value, pclsid) != NOERROR)
        return REGDB_E_IIDNOTREG;

    return S_OK;
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
    APARTMENT *apt = COM_CurrentApt();
    struct registered_psclsid *registered_psclsid;
    ACTCTX_SECTION_KEYED_DATA data;
    HRESULT hr;
    REGSAM opposite = (sizeof(void*) > sizeof(int)) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    BOOL is_wow64;

    TRACE("() riid=%s, pclsid=%p\n", debugstr_guid(riid), pclsid);

    if (!apt)
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (!pclsid)
        return E_INVALIDARG;

    EnterCriticalSection(&apt->cs);

    LIST_FOR_EACH_ENTRY(registered_psclsid, &apt->psclsids, struct registered_psclsid, entry)
        if (IsEqualIID(&registered_psclsid->iid, riid))
        {
            *pclsid = registered_psclsid->clsid;
            LeaveCriticalSection(&apt->cs);
            return S_OK;
        }

    LeaveCriticalSection(&apt->cs);

    data.cbSize = sizeof(data);
    if (FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
                              riid, &data))
    {
        struct ifacepsredirect_data *ifaceps = (struct ifacepsredirect_data*)data.lpData;
        *pclsid = ifaceps->iid;
        return S_OK;
    }

    /* Interface\\{string form of riid}\\ProxyStubClsid32 */
    strcpyW(path, wszInterface);
    StringFromGUID2(riid, path + ARRAYSIZE(wszInterface) - 1, CHARS_IN_GUID);
    strcpyW(path + ARRAYSIZE(wszInterface) - 1 + CHARS_IN_GUID - 1, wszPSC);

    hr = get_ps_clsid_from_registry(path, 0, pclsid);
    if (FAILED(hr) && (opposite == KEY_WOW64_32KEY ||
                       (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
        hr = get_ps_clsid_from_registry(path, opposite, pclsid);

    if (hr == S_OK)
        TRACE ("() Returning CLSID=%s\n", debugstr_guid(pclsid));
    else
        WARN("No PSFactoryBuffer object is registered for IID %s\n", debugstr_guid(riid));

    return hr;
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
static HRESULT COM_GetRegisteredClassObject(const struct apartment *apt, REFCLSID rclsid,
                                            DWORD dwClsContext, LPUNKNOWN* ppUnk)
{
  HRESULT hr = S_FALSE;
  RegisteredClass *curClass;

  EnterCriticalSection( &csRegisteredClassList );

  LIST_FOR_EACH_ENTRY(curClass, &RegisteredClassList, RegisteredClass, entry)
  {
    /*
     * Check if we have a match on the class ID and context.
     */
    if ((apt->oxid == curClass->apartment_id) &&
        (dwClsContext & curClass->runContext) &&
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
 * NOTES
 *  In-process objects are only registered for the current apartment.
 *  CoGetClassObject() and CoCreateInstance() will not return objects registered
 *  in other apartments.
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
  static LONG next_cookie;
  RegisteredClass* newClass;
  LPUNKNOWN        foundObject;
  HRESULT          hr;
  APARTMENT *apt;

  TRACE("(%s,%p,0x%08x,0x%08x,%p)\n",
	debugstr_guid(rclsid),pUnk,dwClsContext,flags,lpdwRegister);

  if ( (lpdwRegister==0) || (pUnk==0) )
    return E_INVALIDARG;

  apt = COM_CurrentApt();
  if (!apt)
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
  hr = COM_GetRegisteredClassObject(apt, rclsid, dwClsContext, &foundObject);
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
  newClass->apartment_id    = apt->oxid;
  newClass->runContext      = dwClsContext;
  newClass->connectFlags    = flags;
  newClass->RpcRegistration = NULL;

  if (!(newClass->dwCookie = InterlockedIncrement( &next_cookie )))
      newClass->dwCookie = InterlockedIncrement( &next_cookie );

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
      IStream *marshal_stream;

      hr = get_local_server_stream(apt, &marshal_stream);
      if(FAILED(hr))
          return hr;

      hr = RPC_StartLocalServer(&newClass->classIdentifier,
                                marshal_stream,
                                flags & (REGCLS_MULTIPLEUSE|REGCLS_MULTI_SEPARATE),
                                &newClass->RpcRegistration);
      IStream_Release(marshal_stream);
  }
  return S_OK;
}

static enum comclass_threadingmodel get_threading_model(const struct class_reg_data *data)
{
    if (data->hkey)
    {
        static const WCHAR wszThreadingModel[] = {'T','h','r','e','a','d','i','n','g','M','o','d','e','l',0};
        static const WCHAR wszApartment[] = {'A','p','a','r','t','m','e','n','t',0};
        static const WCHAR wszFree[] = {'F','r','e','e',0};
        static const WCHAR wszBoth[] = {'B','o','t','h',0};
        WCHAR threading_model[10 /* strlenW(L"apartment")+1 */];
        DWORD dwLength = sizeof(threading_model);
        DWORD keytype;
        DWORD ret;

        ret = RegQueryValueExW(data->u.hkey, wszThreadingModel, NULL, &keytype, (BYTE*)threading_model, &dwLength);
        if ((ret != ERROR_SUCCESS) || (keytype != REG_SZ))
            threading_model[0] = '\0';

        if (!strcmpiW(threading_model, wszApartment)) return ThreadingModel_Apartment;
        if (!strcmpiW(threading_model, wszFree)) return ThreadingModel_Free;
        if (!strcmpiW(threading_model, wszBoth)) return ThreadingModel_Both;

        /* there's not specific handling for this case */
        if (threading_model[0]) return ThreadingModel_Neutral;
        return ThreadingModel_No;
    }
    else
        return data->u.actctx.data->model;
}

static HRESULT get_inproc_class_object(APARTMENT *apt, const struct class_reg_data *regdata,
                                       REFCLSID rclsid, REFIID riid,
                                       BOOL hostifnecessary, void **ppv)
{
    WCHAR dllpath[MAX_PATH+1];
    BOOL apartment_threaded;

    if (hostifnecessary)
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

    if (COM_RegReadPath(regdata, dllpath, ARRAYSIZE(dllpath)) != ERROR_SUCCESS)
    {
        /* failure: CLSID is not found in registry */
        WARN("class %s not registered inproc\n", debugstr_guid(rclsid));
        return REGDB_E_CLASSNOTREG;
    }

    return apartment_getclassobject(apt, dllpath, apartment_threaded,
                                    rclsid, riid, ppv);
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
HRESULT WINAPI DECLSPEC_HOTPATCH CoGetClassObject(
    REFCLSID rclsid, DWORD dwClsContext, COSERVERINFO *pServerInfo,
    REFIID iid, LPVOID *ppv)
{
    struct class_reg_data clsreg;
    IUnknown *regClassObject;
    HRESULT	hres = E_UNEXPECTED;
    APARTMENT  *apt;
    BOOL release_apt = FALSE;

    TRACE("CLSID: %s,IID: %s\n", debugstr_guid(rclsid), debugstr_guid(iid));

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    if (!(apt = COM_CurrentApt()))
    {
        if (!(apt = apartment_find_multi_threaded()))
        {
            ERR("apartment not initialised\n");
            return CO_E_NOTINITIALIZED;
        }
        release_apt = TRUE;
    }

    if (pServerInfo) {
	FIXME("pServerInfo->name=%s pAuthInfo=%p\n",
              debugstr_w(pServerInfo->pwszName), pServerInfo->pAuthInfo);
    }

    if (CLSCTX_INPROC_SERVER & dwClsContext)
    {
        if (IsEqualCLSID(rclsid, &CLSID_InProcFreeMarshaler))
        {
            if (release_apt) apartment_release(apt);
            return FTMarshalCF_Create(iid, ppv);
        }
    }

    if (CLSCTX_INPROC & dwClsContext)
    {
        ACTCTX_SECTION_KEYED_DATA data;

        data.cbSize = sizeof(data);
        /* search activation context first */
        if (FindActCtxSectionGuid(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                                  ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
                                  rclsid, &data))
        {
            struct comclassredirect_data *comclass = (struct comclassredirect_data*)data.lpData;

            clsreg.u.actctx.hactctx = data.hActCtx;
            clsreg.u.actctx.data = data.lpData;
            clsreg.u.actctx.section = data.lpSectionBase;
            clsreg.hkey = FALSE;

            hres = get_inproc_class_object(apt, &clsreg, &comclass->clsid, iid, !(dwClsContext & WINE_CLSCTX_DONT_HOST), ppv);
            ReleaseActCtx(data.hActCtx);
            if (release_apt) apartment_release(apt);
            return hres;
        }
    }

    /*
     * First, try and see if we can't match the class ID with one of the
     * registered classes.
     */
    if (S_OK == COM_GetRegisteredClassObject(apt, rclsid, dwClsContext,
                                             &regClassObject))
    {
      /* Get the required interface from the retrieved pointer. */
      hres = IUnknown_QueryInterface(regClassObject, iid, ppv);

      /*
       * Since QI got another reference on the pointer, we want to release the
       * one we already have. If QI was unsuccessful, this will release the object. This
       * is good since we are not returning it in the "out" parameter.
       */
      IUnknown_Release(regClassObject);
      if (release_apt) apartment_release(apt);
      return hres;
    }

    /* First try in-process server */
    if (CLSCTX_INPROC_SERVER & dwClsContext)
    {
        static const WCHAR wszInprocServer32[] = {'I','n','p','r','o','c','S','e','r','v','e','r','3','2',0};
        HKEY hkey;

        hres = COM_OpenKeyForCLSID(rclsid, wszInprocServer32, KEY_READ, &hkey);
        if (FAILED(hres))
        {
            if (hres == REGDB_E_CLASSNOTREG)
                ERR("class %s not registered\n", debugstr_guid(rclsid));
            else if (hres == REGDB_E_KEYMISSING)
            {
                WARN("class %s not registered as in-proc server\n", debugstr_guid(rclsid));
                hres = REGDB_E_CLASSNOTREG;
            }
        }

        if (SUCCEEDED(hres))
        {
            clsreg.u.hkey = hkey;
            clsreg.hkey = TRUE;

            hres = get_inproc_class_object(apt, &clsreg, rclsid, iid, !(dwClsContext & WINE_CLSCTX_DONT_HOST), ppv);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hres))
        {
            if (release_apt) apartment_release(apt);
            return hres;
        }
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
            else if (hres == REGDB_E_KEYMISSING)
            {
                WARN("class %s not registered in-proc handler\n", debugstr_guid(rclsid));
                hres = REGDB_E_CLASSNOTREG;
            }
        }

        if (SUCCEEDED(hres))
        {
            clsreg.u.hkey = hkey;
            clsreg.hkey = TRUE;

            hres = get_inproc_class_object(apt, &clsreg, rclsid, iid, !(dwClsContext & WINE_CLSCTX_DONT_HOST), ppv);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hres))
        {
            if (release_apt) apartment_release(apt);
            return hres;
        }
    }
    if (release_apt) apartment_release(apt);

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
        hres = REGDB_E_CLASSNOTREG;
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
HRESULT WINAPI DECLSPEC_HOTPATCH CoCreateInstance(
    REFCLSID rclsid,
    LPUNKNOWN pUnkOuter,
    DWORD dwClsContext,
    REFIID iid,
    LPVOID *ppv)
{
    MULTI_QI multi_qi = { iid };
    HRESULT hres;

    TRACE("(rclsid=%s, pUnkOuter=%p, dwClsContext=%08x, riid=%s, ppv=%p)\n", debugstr_guid(rclsid),
          pUnkOuter, dwClsContext, debugstr_guid(iid), ppv);

    if (ppv==0)
        return E_POINTER;

    hres = CoCreateInstanceEx(rclsid, pUnkOuter, dwClsContext, NULL, 1, &multi_qi);
    *ppv = multi_qi.pItf;
    return hres;
}

static void init_multi_qi(DWORD count, MULTI_QI *mqi)
{
  ULONG i;

  for (i = 0; i < count; i++)
  {
      mqi[i].pItf = NULL;
      mqi[i].hr = E_NOINTERFACE;
  }
}

static HRESULT return_multi_qi(IUnknown *unk, DWORD count, MULTI_QI *mqi, BOOL include_unk)
{
  ULONG index = 0, fetched = 0;

  if (include_unk)
  {
    mqi[0].hr = S_OK;
    mqi[0].pItf = unk;
    index = fetched = 1;
  }

  for (; index < count; index++)
  {
    mqi[index].hr = IUnknown_QueryInterface(unk, mqi[index].pIID, (void**)&mqi[index].pItf);
    if (mqi[index].hr == S_OK)
      fetched++;
  }

  if (!include_unk)
      IUnknown_Release(unk);

  if (fetched == 0)
    return E_NOINTERFACE;

  return fetched == count ? S_OK : CO_S_NOTALLINTERFACES;
}

/***********************************************************************
 *           CoCreateInstanceEx [OLE32.@]
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoCreateInstanceEx(
  REFCLSID      rclsid,
  LPUNKNOWN     pUnkOuter,
  DWORD         dwClsContext,
  COSERVERINFO* pServerInfo,
  ULONG         cmq,
  MULTI_QI*     pResults)
{
    IUnknown *unk = NULL;
    IClassFactory *cf;
    APARTMENT *apt;
    CLSID clsid;
    HRESULT hres;

    TRACE("(%s %p %x %p %u %p)\n", debugstr_guid(rclsid), pUnkOuter, dwClsContext, pServerInfo, cmq, pResults);

    if (!cmq || !pResults)
        return E_INVALIDARG;

    if (pServerInfo)
        FIXME("() non-NULL pServerInfo not supported!\n");

    init_multi_qi(cmq, pResults);

    hres = CoGetTreatAsClass(rclsid, &clsid);
    if(FAILED(hres))
        clsid = *rclsid;

    if (!(apt = COM_CurrentApt()))
    {
        if (!(apt = apartment_find_multi_threaded()))
        {
            ERR("apartment not initialised\n");
            return CO_E_NOTINITIALIZED;
        }
        apartment_release(apt);
    }

    /*
     * The Standard Global Interface Table (GIT) object is a process-wide singleton.
     */
    if (IsEqualIID(&clsid, &CLSID_StdGlobalInterfaceTable))
    {
        IGlobalInterfaceTable *git = get_std_git();
        TRACE("Retrieving GIT\n");
        return return_multi_qi((IUnknown*)git, cmq, pResults, FALSE);
    }

    if (IsEqualCLSID(&clsid, &CLSID_ManualResetEvent)) {
        hres = ManualResetEvent_Construct(pUnkOuter, pResults[0].pIID, (void**)&unk);
        if (FAILED(hres))
            return hres;
        return return_multi_qi(unk, cmq, pResults, TRUE);
    }

    /*
     * Get a class factory to construct the object we want.
     */
    hres = CoGetClassObject(&clsid, dwClsContext, NULL, &IID_IClassFactory, (void**)&cf);
    if (FAILED(hres))
        return hres;

    /*
     * Create the object and don't forget to release the factory
     */
    hres = IClassFactory_CreateInstance(cf, pUnkOuter, pResults[0].pIID, (void**)&unk);
    IClassFactory_Release(cf);
    if (FAILED(hres))
    {
        if (hres == CLASS_E_NOAGGREGATION && pUnkOuter)
            FIXME("Class %s does not support aggregation\n", debugstr_guid(&clsid));
        else
            FIXME("no instance created for interface %s of class %s, hres is 0x%08x\n",
                  debugstr_guid(pResults[0].pIID),
                  debugstr_guid(&clsid),hres);
        return hres;
    }

    return return_multi_qi(unk, cmq, pResults, TRUE);
}

/***********************************************************************
 *           CoGetInstanceFromFile [OLE32.@]
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoGetInstanceFromFile(
  COSERVERINFO *server_info,
  CLSID        *rclsid,
  IUnknown     *outer,
  DWORD         cls_context,
  DWORD         grfmode,
  OLECHAR      *filename,
  DWORD         count,
  MULTI_QI     *results
)
{
  IPersistFile *pf = NULL;
  IUnknown* unk = NULL;
  CLSID clsid;
  HRESULT hr;

  if (count == 0 || !results)
    return E_INVALIDARG;

  if (server_info)
    FIXME("() non-NULL server_info not supported\n");

  init_multi_qi(count, results);

  /* optionally get CLSID from a file */
  if (!rclsid)
  {
    hr = GetClassFile(filename, &clsid);
    if (FAILED(hr))
    {
      ERR("failed to get CLSID from a file\n");
      return hr;
    }

    rclsid = &clsid;
  }

  hr = CoCreateInstance(rclsid,
			outer,
			cls_context,
			&IID_IUnknown,
			(void**)&unk);

  if (hr != S_OK)
    return hr;

  /* init from file */
  hr = IUnknown_QueryInterface(unk, &IID_IPersistFile, (void**)&pf);
  if (FAILED(hr))
      ERR("failed to get IPersistFile\n");

  if (pf)
  {
      IPersistFile_Load(pf, filename, grfmode);
      IPersistFile_Release(pf);
  }

  return return_multi_qi(unk, count, results, FALSE);
}

/***********************************************************************
 *           CoGetInstanceFromIStorage [OLE32.@]
 */
HRESULT WINAPI CoGetInstanceFromIStorage(
  COSERVERINFO *server_info,
  CLSID        *rclsid,
  IUnknown     *outer,
  DWORD         cls_context,
  IStorage     *storage,
  DWORD         count,
  MULTI_QI     *results
)
{
  IPersistStorage *ps = NULL;
  IUnknown* unk = NULL;
  STATSTG stat;
  HRESULT hr;

  if (count == 0 || !results || !storage)
    return E_INVALIDARG;

  if (server_info)
    FIXME("() non-NULL server_info not supported\n");

  init_multi_qi(count, results);

  /* optionally get CLSID from a file */
  if (!rclsid)
  {
    memset(&stat.clsid, 0, sizeof(stat.clsid));
    hr = IStorage_Stat(storage, &stat, STATFLAG_NONAME);
    if (FAILED(hr))
    {
      ERR("failed to get CLSID from a file\n");
      return hr;
    }

    rclsid = &stat.clsid;
  }

  hr = CoCreateInstance(rclsid,
			outer,
			cls_context,
			&IID_IUnknown,
			(void**)&unk);

  if (hr != S_OK)
    return hr;

  /* init from IStorage */
  hr = IUnknown_QueryInterface(unk, &IID_IPersistStorage, (void**)&ps);
  if (FAILED(hr))
      ERR("failed to get IPersistStorage\n");

  if (ps)
  {
      IPersistStorage_Load(ps, storage);
      IPersistStorage_Release(ps);
  }

  return return_multi_qi(unk, count, results, FALSE);
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
 *           CoFreeUnusedLibrariesEx [OLE32.@]
 *
 * Frees any previously unused libraries whose delay has expired and marks
 * currently unused libraries for unloading. Unused are identified as those that
 * return S_OK from their DllCanUnloadNow function.
 *
 * PARAMS
 *  dwUnloadDelay [I] Unload delay in milliseconds.
 *  dwReserved    [I] Reserved. Set to 0.
 *
 * RETURNS
 *  Nothing.
 *
 * SEE ALSO
 *  CoLoadLibrary, CoFreeAllLibraries, CoFreeLibrary
 */
void WINAPI DECLSPEC_HOTPATCH CoFreeUnusedLibrariesEx(DWORD dwUnloadDelay, DWORD dwReserved)
{
    struct apartment *apt = COM_CurrentApt();
    if (!apt)
    {
        ERR("apartment not initialised\n");
        return;
    }

    apartment_freeunusedlibraries(apt, dwUnloadDelay);
}

/***********************************************************************
 *           CoFreeUnusedLibraries [OLE32.@]
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
void WINAPI DECLSPEC_HOTPATCH CoFreeUnusedLibraries(void)
{
    CoFreeUnusedLibrariesEx(INFINITE, 0);
}

/***********************************************************************
 *           CoFileTimeNow [OLE32.@]
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

    stubmgr = get_stub_manager_from_object(apt, pUnk, fLock);
    if (!stubmgr)
    {
        WARN("stub object not found %p\n", pUnk);
        /* Note: native is pretty broken here because it just silently
         * fails, without returning an appropriate error code, making apps
         * think that the object was disconnected, when it actually wasn't */
        return S_OK;
    }

    if (fLock)
        stub_manager_ext_addref(stubmgr, 1, FALSE);
    else
        stub_manager_ext_release(stubmgr, 1, FALSE, fLastUnlockReleases);

    stub_manager_int_release(stubmgr);
    return S_OK;
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

    if (IsEqualGUID( clsidOld, clsidNew ))
    {
       if (!RegQueryValueW(hkey, wszAutoTreatAs, auto_treat_as, &auto_treat_as_size) &&
           CLSIDFromString(auto_treat_as, &id) == S_OK)
       {
           if (RegSetValueW(hkey, wszTreatAs, REG_SZ, auto_treat_as, sizeof(auto_treat_as)))
           {
               res = REGDB_E_WRITEREGDB;
               goto done;
           }
       }
       else
       {
           if(RegDeleteKeyW(hkey, wszTreatAs))
               res = REGDB_E_WRITEREGDB;
           goto done;
       }
    }
    else
    {
        if(IsEqualGUID(clsidNew, &CLSID_NULL)){
           RegDeleteKeyW(hkey, wszTreatAs);
        }else{
            if(!StringFromGUID2(clsidNew, szClsidNew, ARRAYSIZE(szClsidNew))){
                WARN("StringFromGUID2 failed\n");
                res = E_FAIL;
                goto done;
            }

            if(RegSetValueW(hkey, wszTreatAs, REG_SZ, szClsidNew, sizeof(szClsidNew)) != ERROR_SUCCESS){
                WARN("RegSetValue failed\n");
                res = REGDB_E_WRITEREGDB;
                goto done;
            }
        }
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

    TRACE("(%s,%p)\n", debugstr_guid(clsidOld), clsidNew);
    *clsidNew = *clsidOld; /* copy over old value */

    res = COM_OpenKeyForCLSID(clsidOld, wszTreatAs, KEY_READ, &hkey);
    if (FAILED(res))
    {
        res = S_FALSE;
        goto done;
    }
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
    struct oletls *info = COM_CurrentInfo();

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    if (!info)
        return E_OUTOFMEMORY;

    if (!info->call_state)
        return RPC_E_CALL_COMPLETE;

    return IUnknown_QueryInterface(info->call_state, riid, ppv);
}

/***********************************************************************
 *           CoSwitchCallContext [OLE32.@]
 *
 * Switches the context of the currently executing server call in the current
 * thread.
 *
 * PARAMS
 *  pObject     [I] Pointer to new context object
 *  ppOldObject [O] Pointer to memory that will receive old context object pointer
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoSwitchCallContext(IUnknown *pObject, IUnknown **ppOldObject)
{
    struct oletls *info = COM_CurrentInfo();

    TRACE("(%p, %p)\n", pObject, ppOldObject);

    if (!info)
        return E_OUTOFMEMORY;

    *ppOldObject = info->call_state;
    info->call_state = pObject; /* CoSwitchCallContext does not addref nor release objects */

    return S_OK;
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
    return (apt->win && PeekMessageW(msg, apt->win, 0, 0, PM_REMOVE|PM_NOYIELD)) ||
           /* next retrieve other messages necessary for the app to remain responsive */
           PeekMessageW(msg, NULL, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE|PM_NOYIELD) ||
           PeekMessageW(msg, NULL, 0, 0, PM_QS_PAINT|PM_QS_SENDMESSAGE|PM_REMOVE|PM_NOYIELD);
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
    BOOL check_apc = (dwFlags & COWAIT_ALERTABLE) != 0;

    TRACE("(0x%08x, 0x%08x, %d, %p, %p)\n", dwFlags, dwTimeout, cHandles,
        pHandles, lpdwindex);

    if (!lpdwindex)
        return E_INVALIDARG;

    *lpdwindex = 0;

    if (!pHandles)
        return E_INVALIDARG;

    if (!cHandles)
        return RPC_E_NO_SYNC;

    while (TRUE)
    {
        DWORD now = GetTickCount();
        DWORD res;

        if (now - start_time > dwTimeout)
        {
            hr = RPC_S_CALLPENDING;
            break;
        }

        if (message_loop)
        {
            DWORD wait_flags = ((dwFlags & COWAIT_WAITALL) ? MWMO_WAITALL : 0) |
                    ((dwFlags & COWAIT_ALERTABLE ) ? MWMO_ALERTABLE : 0);

            TRACE("waiting for rpc completion or window message\n");

            res = WAIT_TIMEOUT;

            if (check_apc)
            {
                res = WaitForMultipleObjectsEx(cHandles, pHandles,
                    (dwFlags & COWAIT_WAITALL) != 0, 0, TRUE);
                check_apc = FALSE;
            }

            if (res == WAIT_TIMEOUT)
                res = MsgWaitForMultipleObjectsEx(cHandles, pHandles,
                    (dwTimeout == INFINITE) ? INFINITE : start_time + dwTimeout - now,
                    QS_SENDMESSAGE | QS_ALLPOSTMESSAGE | QS_PAINT, wait_flags);

            if (res == WAIT_OBJECT_0 + cHandles)  /* messages available */
            {
                MSG msg;
                int count = 0;

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

                /* some apps (e.g. Visio 2010) don't handle WM_PAINT properly and loop forever,
                 * so after processing 100 messages we go back to checking the wait handles */
                while (count++ < 100 && COM_PeekMessage(apt, &msg))
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
                        break;
                    }
                }
                continue;
            }
        }
        else
        {
            TRACE("waiting for rpc completion\n");

            res = WaitForMultipleObjectsEx(cHandles, pHandles, (dwFlags & COWAIT_WAITALL) != 0,
                (dwTimeout == INFINITE) ? INFINITE : start_time + dwTimeout - now,
                (dwFlags & COWAIT_ALERTABLE) != 0);
        }

        switch (res)
        {
        case WAIT_TIMEOUT:
            hr = RPC_S_CALLPENDING;
            break;
        case WAIT_FAILED:
            hr = HRESULT_FROM_WIN32( GetLastError() );
            break;
        default:
            *lpdwindex = res;
            break;
        }
        break;
    }
    TRACE("-- 0x%08x\n", hr);
    return hr;
}


/***********************************************************************
 *           CoGetObject [OLE32.@]
 *
 * Gets the object named by converting the name to a moniker and binding to it.
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

typedef struct Context
{
    IComThreadingInfo IComThreadingInfo_iface;
    IContextCallback IContextCallback_iface;
    IObjContext IObjContext_iface;
    LONG refs;
    APTTYPE apttype;
} Context;

static inline Context *impl_from_IComThreadingInfo( IComThreadingInfo *iface )
{
        return CONTAINING_RECORD(iface, Context, IComThreadingInfo_iface);
}

static inline Context *impl_from_IContextCallback( IContextCallback *iface )
{
        return CONTAINING_RECORD(iface, Context, IContextCallback_iface);
}

static inline Context *impl_from_IObjContext( IObjContext *iface )
{
        return CONTAINING_RECORD(iface, Context, IObjContext_iface);
}

static HRESULT Context_QueryInterface(Context *iface, REFIID riid, LPVOID *ppv)
{
    *ppv = NULL;

    if (IsEqualIID(riid, &IID_IComThreadingInfo) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *ppv = &iface->IComThreadingInfo_iface;
    }
    else if (IsEqualIID(riid, &IID_IContextCallback))
    {
        *ppv = &iface->IContextCallback_iface;
    }
    else if (IsEqualIID(riid, &IID_IObjContext))
    {
        *ppv = &iface->IObjContext_iface;
    }

    if (*ppv)
    {
        IUnknown_AddRef((IUnknown*)*ppv);
        return S_OK;
    }

    FIXME("interface not implemented %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG Context_AddRef(Context *This)
{
    return InterlockedIncrement(&This->refs);
}

static ULONG Context_Release(Context *This)
{
    ULONG refs = InterlockedDecrement(&This->refs);
    if (!refs)
        HeapFree(GetProcessHeap(), 0, This);
    return refs;
}

static HRESULT WINAPI Context_CTI_QueryInterface(IComThreadingInfo *iface, REFIID riid, LPVOID *ppv)
{
    Context *This = impl_from_IComThreadingInfo(iface);
    return Context_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI Context_CTI_AddRef(IComThreadingInfo *iface)
{
    Context *This = impl_from_IComThreadingInfo(iface);
    return Context_AddRef(This);
}

static ULONG WINAPI Context_CTI_Release(IComThreadingInfo *iface)
{
    Context *This = impl_from_IComThreadingInfo(iface);
    return Context_Release(This);
}

static HRESULT WINAPI Context_CTI_GetCurrentApartmentType(IComThreadingInfo *iface, APTTYPE *apttype)
{
    Context *This = impl_from_IComThreadingInfo(iface);

    TRACE("(%p)\n", apttype);

    *apttype = This->apttype;
    return S_OK;
}

static HRESULT WINAPI Context_CTI_GetCurrentThreadType(IComThreadingInfo *iface, THDTYPE *thdtype)
{
    Context *This = impl_from_IComThreadingInfo(iface);

    TRACE("(%p)\n", thdtype);

    switch (This->apttype)
    {
    case APTTYPE_STA:
    case APTTYPE_MAINSTA:
        *thdtype = THDTYPE_PROCESSMESSAGES;
        break;
    default:
        *thdtype = THDTYPE_BLOCKMESSAGES;
        break;
    }
    return S_OK;
}

static HRESULT WINAPI Context_CTI_GetCurrentLogicalThreadId(IComThreadingInfo *iface, GUID *logical_thread_id)
{
    FIXME("(%p): stub\n", logical_thread_id);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_CTI_SetCurrentLogicalThreadId(IComThreadingInfo *iface, REFGUID logical_thread_id)
{
    FIXME("(%s): stub\n", debugstr_guid(logical_thread_id));
    return E_NOTIMPL;
}

static const IComThreadingInfoVtbl Context_Threading_Vtbl =
{
    Context_CTI_QueryInterface,
    Context_CTI_AddRef,
    Context_CTI_Release,
    Context_CTI_GetCurrentApartmentType,
    Context_CTI_GetCurrentThreadType,
    Context_CTI_GetCurrentLogicalThreadId,
    Context_CTI_SetCurrentLogicalThreadId
};

static HRESULT WINAPI Context_CC_QueryInterface(IContextCallback *iface, REFIID riid, LPVOID *ppv)
{
    Context *This = impl_from_IContextCallback(iface);
    return Context_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI Context_CC_AddRef(IContextCallback *iface)
{
    Context *This = impl_from_IContextCallback(iface);
    return Context_AddRef(This);
}

static ULONG WINAPI Context_CC_Release(IContextCallback *iface)
{
    Context *This = impl_from_IContextCallback(iface);
    return Context_Release(This);
}

static HRESULT WINAPI Context_CC_ContextCallback(IContextCallback *iface, PFNCONTEXTCALL pCallback,
                            ComCallData *param, REFIID riid, int method, IUnknown *punk)
{
    Context *This = impl_from_IContextCallback(iface);

    FIXME("(%p/%p)->(%p, %p, %s, %d, %p)\n", This, iface, pCallback, param, debugstr_guid(riid), method, punk);
    return E_NOTIMPL;
}

static const IContextCallbackVtbl Context_Callback_Vtbl =
{
    Context_CC_QueryInterface,
    Context_CC_AddRef,
    Context_CC_Release,
    Context_CC_ContextCallback
};

static HRESULT WINAPI Context_OC_QueryInterface(IObjContext *iface, REFIID riid, LPVOID *ppv)
{
    Context *This = impl_from_IObjContext(iface);
    return Context_QueryInterface(This, riid, ppv);
}

static ULONG WINAPI Context_OC_AddRef(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    return Context_AddRef(This);
}

static ULONG WINAPI Context_OC_Release(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    return Context_Release(This);
}

static HRESULT WINAPI Context_OC_SetProperty(IObjContext *iface, REFGUID propid, CPFLAGS flags, IUnknown *punk)
{
    Context *This = impl_from_IObjContext(iface);

    FIXME("(%p/%p)->(%s, %x, %p)\n", This, iface, debugstr_guid(propid), flags, punk);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_OC_RemoveProperty(IObjContext *iface, REFGUID propid)
{
    Context *This = impl_from_IObjContext(iface);

    FIXME("(%p/%p)->(%s)\n", This, iface, debugstr_guid(propid));
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_OC_GetProperty(IObjContext *iface, REFGUID propid, CPFLAGS *flags, IUnknown **punk)
{
    Context *This = impl_from_IObjContext(iface);

    FIXME("(%p/%p)->(%s, %p, %p)\n", This, iface, debugstr_guid(propid), flags, punk);
    return E_NOTIMPL;
}

static HRESULT WINAPI Context_OC_EnumContextProps(IObjContext *iface, IEnumContextProps **props)
{
    Context *This = impl_from_IObjContext(iface);

    FIXME("(%p/%p)->(%p)\n", This, iface, props);
    return E_NOTIMPL;
}

static void WINAPI Context_OC_Reserved1(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static void WINAPI Context_OC_Reserved2(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static void WINAPI Context_OC_Reserved3(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static void WINAPI Context_OC_Reserved4(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static void WINAPI Context_OC_Reserved5(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static void WINAPI Context_OC_Reserved6(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static void WINAPI Context_OC_Reserved7(IObjContext *iface)
{
    Context *This = impl_from_IObjContext(iface);
    FIXME("(%p/%p)\n", This, iface);
}

static const IObjContextVtbl Context_Object_Vtbl =
{
    Context_OC_QueryInterface,
    Context_OC_AddRef,
    Context_OC_Release,
    Context_OC_SetProperty,
    Context_OC_RemoveProperty,
    Context_OC_GetProperty,
    Context_OC_EnumContextProps,
    Context_OC_Reserved1,
    Context_OC_Reserved2,
    Context_OC_Reserved3,
    Context_OC_Reserved4,
    Context_OC_Reserved5,
    Context_OC_Reserved6,
    Context_OC_Reserved7
};

/***********************************************************************
 *           CoGetObjectContext [OLE32.@]
 *
 * Retrieves an object associated with the current context (i.e. apartment).
 *
 * PARAMS
 *  riid [I] ID of the interface of the object to retrieve.
 *  ppv  [O] Address where object will be stored on return.
 *
 * RETURNS
 *  Success: S_OK.
 *  Failure: HRESULT code.
 */
HRESULT WINAPI CoGetObjectContext(REFIID riid, void **ppv)
{
    APARTMENT *apt = COM_CurrentApt();
    Context *context;
    HRESULT hr;

    TRACE("(%s, %p)\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    if (!apt)
    {
        if (!(apt = apartment_find_multi_threaded()))
        {
            ERR("apartment not initialised\n");
            return CO_E_NOTINITIALIZED;
        }
        apartment_release(apt);
    }

    context = HeapAlloc(GetProcessHeap(), 0, sizeof(*context));
    if (!context)
        return E_OUTOFMEMORY;

    context->IComThreadingInfo_iface.lpVtbl = &Context_Threading_Vtbl;
    context->IContextCallback_iface.lpVtbl = &Context_Callback_Vtbl;
    context->IObjContext_iface.lpVtbl = &Context_Object_Vtbl;
    context->refs = 1;
    if (apt->multi_threaded)
        context->apttype = APTTYPE_MTA;
    else if (apt->main)
        context->apttype = APTTYPE_MAINSTA;
    else
        context->apttype = APTTYPE_STA;

    hr = IComThreadingInfo_QueryInterface(&context->IComThreadingInfo_iface, riid, ppv);
    IComThreadingInfo_Release(&context->IComThreadingInfo_iface);

    return hr;
}


/***********************************************************************
 *           CoGetContextToken [OLE32.@]
 */
HRESULT WINAPI CoGetContextToken( ULONG_PTR *token )
{
    struct oletls *info = COM_CurrentInfo();

    TRACE("(%p)\n", token);

    if (!info)
        return E_OUTOFMEMORY;

    if (!info->apt)
    {
        APARTMENT *apt;
        if (!(apt = apartment_find_multi_threaded()))
        {
            ERR("apartment not initialised\n");
            return CO_E_NOTINITIALIZED;
        }
        apartment_release(apt);
    }

    if (!token)
        return E_POINTER;

    if (!info->context_token)
    {
        HRESULT hr;
        IObjContext *ctx;

        hr = CoGetObjectContext(&IID_IObjContext, (void **)&ctx);
        if (FAILED(hr)) return hr;
        info->context_token = ctx;
    }

    *token = (ULONG_PTR)info->context_token;
    TRACE("apt->context_token=%p\n", info->context_token);

    return S_OK;
}

/***********************************************************************
 *           CoGetDefaultContext [OLE32.@]
 */
HRESULT WINAPI CoGetDefaultContext(APTTYPE type, REFIID riid, LPVOID *ppv)
{
    FIXME("%d %s %p stub\n", type, debugstr_guid(riid), ppv);
    return E_NOINTERFACE;
}

HRESULT Handler_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    static const WCHAR wszInprocHandler32[] = {'I','n','p','r','o','c','H','a','n','d','l','e','r','3','2',0};
    HKEY hkey;
    HRESULT hres;

    hres = COM_OpenKeyForCLSID(rclsid, wszInprocHandler32, KEY_READ, &hkey);
    if (SUCCEEDED(hres))
    {
        struct class_reg_data regdata;
        WCHAR dllpath[MAX_PATH+1];

        regdata.u.hkey = hkey;
        regdata.hkey = TRUE;

        if (COM_RegReadPath(&regdata, dllpath, ARRAYSIZE(dllpath)) == ERROR_SUCCESS)
        {
            static const WCHAR wszOle32[] = {'o','l','e','3','2','.','d','l','l',0};
            if (!strcmpiW(dllpath, wszOle32))
            {
                RegCloseKey(hkey);
                return HandlerCF_Create(rclsid, riid, ppv);
            }
        }
        else
            WARN("not creating object for inproc handler path %s\n", debugstr_w(dllpath));
        RegCloseKey(hkey);
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

/***********************************************************************
 *		DllMain (OLE32.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID reserved)
{
    TRACE("%p 0x%x %p\n", hinstDLL, fdwReason, reserved);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        hProxyDll = hinstDLL;
	break;

    case DLL_PROCESS_DETACH:
        if (reserved) break;
        release_std_git();
        UnregisterClassW( wszAptWinClass, hProxyDll );
        RPC_UnregisterAllChannelHooks();
        COMPOBJ_DllList_Free();
        DeleteCriticalSection(&csRegisteredClassList);
        DeleteCriticalSection(&csApartment);
	break;

    case DLL_THREAD_DETACH:
        COM_TlsDestroy();
        break;
    }
    return TRUE;
}

/***********************************************************************
 *		DllRegisterServer (OLE32.@)
 */
HRESULT WINAPI DllRegisterServer(void)
{
    return OLE32_DllRegisterServer();
}

/***********************************************************************
 *		DllUnregisterServer (OLE32.@)
 */
HRESULT WINAPI DllUnregisterServer(void)
{
    return OLE32_DllUnregisterServer();
}
