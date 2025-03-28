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

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS
#define NONAMELESSUNION

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winreg.h"
#include "winuser.h"
#define USE_COM_CONTEXT_DEF
#include "objbase.h"
#include "ole2.h"
#include "ole2ver.h"
#include "ctxtcall.h"
#include "dde.h"
#include "servprov.h"

#ifndef __REACTOS__
#include "initguid.h"
#endif
#include "compobj_private.h"
#include "moniker.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(ole);

/****************************************************************************
 * This section defines variables internal to the COM module.
 */

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
    ULONG flags;
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

enum class_reg_data_origin
{
    CLASS_REG_ACTCTX,
    CLASS_REG_REGISTRY,
};

struct class_reg_data
{
    enum class_reg_data_origin origin;
    union
    {
        struct
        {
            const WCHAR *module_name;
            DWORD threading_model;
            HANDLE hactctx;
        } actctx;
        HKEY hkey;
    } u;
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

extern void WINAPI InternalRevokeAllPSClsids(void);

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
        ULONG miscmask = (comclass->flags >> 8) & 0xff;

        if (!(miscmask & misc))
        {
            if (!(miscmask & MiscStatus))
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
    {'\\','R','e','g','i','s','t','r','y','\\','M','a','c','h','i','n','e',
     '\\','S','o','f','t','w','a','r','e','\\','C','l','a','s','s','e','s',0};
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

static void COM_RevokeRegisteredClassObject(RegisteredClass *curClass)
{
    list_remove(&curClass->entry);

    if (curClass->runContext & CLSCTX_LOCAL_SERVER)
        RPC_StopLocalServer(curClass->RpcRegistration);

    IUnknown_Release(curClass->classObject);
    HeapFree(GetProcessHeap(), 0, curClass);
}

void COM_RevokeAllClasses(const struct apartment *apt)
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

HRESULT WINAPI ManualResetEvent_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID iid, void **ppv)
{
    MREImpl *This = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MREImpl));
    HRESULT hr;

    if (outer)
        FIXME("Aggregation not implemented.\n");

    This->ref = 1;
    This->ISynchronize_iface.lpVtbl = &vt_ISynchronize;
    This->ISynchronizeHandle_iface.lpVtbl = &SynchronizeHandleVtbl;
    This->event = CreateEventW(NULL, TRUE, FALSE, NULL);

    hr = ISynchronize_QueryInterface(&This->ISynchronize_iface, iid, ppv);
    ISynchronize_Release(&This->ISynchronize_iface);
    return hr;
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
  struct apartment *apt;

  TRACE("(%08x)\n",dwRegister);

  if (!(apt = apartment_get_current_or_mta()))
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
  apartment_release(apt);
  return hr;
}

static void COM_TlsDestroy(void)
{
    struct oletls *info = NtCurrentTeb()->ReservedForOle;
    if (info)
    {
        struct init_spy *cursor, *cursor2;

        if (info->apt) apartment_release(info->apt);
        if (info->errorinfo) IErrorInfo_Release(info->errorinfo);
        if (info->state) IUnknown_Release(info->state);

        LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &info->spies, struct init_spy, entry)
        {
            list_remove(&cursor->entry);
            if (cursor->spy) IInitializeSpy_Release(cursor->spy);
            heap_free(cursor);
        }

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

/*
 * When locked, don't modify list (unless we add a new head), so that it's
 * safe to iterate it. Freeing of list entries is delayed and done on unlock.
 */
static inline void lock_init_spies(struct oletls *info)
{
    info->spies_lock++;
}

static void unlock_init_spies(struct oletls *info)
{
    struct init_spy *spy, *next;

    if (--info->spies_lock) return;

    LIST_FOR_EACH_ENTRY_SAFE(spy, next, &info->spies, struct init_spy, entry)
    {
        if (spy->spy) continue;
        list_remove(&spy->entry);
        heap_free(spy);
    }
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
  struct init_spy *cursor;
  HRESULT hr;

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

  lock_init_spies(info);
  LIST_FOR_EACH_ENTRY(cursor, &info->spies, struct init_spy, entry)
  {
      if (cursor->spy) IInitializeSpy_PreInitialize(cursor->spy, dwCoInit, info->inits);
  }
  unlock_init_spies(info);

  hr = enter_apartment( info, dwCoInit );

  lock_init_spies(info);
  LIST_FOR_EACH_ENTRY(cursor, &info->spies, struct init_spy, entry)
  {
      if (cursor->spy) hr = IInitializeSpy_PostInitialize(cursor->spy, hr, dwCoInit, info->inits);
  }
  unlock_init_spies(info);

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
  struct init_spy *cursor, *next;
  LONG lCOMRefCnt;

  TRACE("()\n");

  /* will only happen on OOM */
  if (!info) return;

  lock_init_spies(info);
  LIST_FOR_EACH_ENTRY_SAFE(cursor, next, &info->spies, struct init_spy, entry)
  {
      if (cursor->spy) IInitializeSpy_PreUninitialize(cursor->spy, info->inits);
  }
  unlock_init_spies(info);

  /* sanity check */
  if (!info->inits)
  {
      ERR("Mismatched CoUninitialize\n");

      lock_init_spies(info);
      LIST_FOR_EACH_ENTRY_SAFE(cursor, next, &info->spies, struct init_spy, entry)
      {
          if (cursor->spy) IInitializeSpy_PostUninitialize(cursor->spy, info->inits);
      }
      unlock_init_spies(info);

      return;
  }

  leave_apartment( info );

  /*
   * Decrease the reference count.
   * If we are back to 0 locks on the COM library, make sure we free
   * all the associated data structures.
   */
  lCOMRefCnt = InterlockedExchangeAdd(&s_COMLockCount,-1);
  if (lCOMRefCnt==1)
  {
    TRACE("() - Releasing the COM libraries\n");

    InternalRevokeAllPSClsids();
    RunningObjectTableImpl_UnInitialize();
  }
  else if (lCOMRefCnt<1) {
    ERR( "CoUninitialize() - not CoInitialized.\n" );
    InterlockedExchangeAdd(&s_COMLockCount,1); /* restore the lock count. */
  }

  lock_init_spies(info);
  LIST_FOR_EACH_ENTRY(cursor, &info->spies, struct init_spy, entry)
  {
      if (cursor->spy) IInitializeSpy_PostUninitialize(cursor->spy, info->inits);
  }
  unlock_init_spies(info);
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
    struct apartment *apt;

    TRACE("(%p, 0x%08x)\n", lpUnk, reserved);

    if (!lpUnk) return E_INVALIDARG;

    hr = IUnknown_QueryInterface(lpUnk, &IID_IMarshal, (void **)&marshal);
    if (hr == S_OK)
    {
        hr = IMarshal_DisconnectObject(marshal, reserved);
        IMarshal_Release(marshal);
        return hr;
    }

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

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

    apartment_release(apt);
    return S_OK;
}

/* open HKCR\\CLSID\\{string form of clsid}\\{keyname} key */
HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *subkey)
{
    static const WCHAR wszCLSIDSlash[] = {'C','L','S','I','D','\\',0};
    WCHAR path[CHARS_IN_GUID + ARRAY_SIZE(wszCLSIDSlash) - 1];
    LONG res;
    HKEY key;

    lstrcpyW(path, wszCLSIDSlash);
    StringFromGUID2(clsid, path + lstrlenW(wszCLSIDSlash), CHARS_IN_GUID);
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
    WCHAR keyname[ARRAY_SIZE(szAppIdKey) + CHARS_IN_GUID];
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

    lstrcpyW(keyname, szAppIdKey);
    lstrcatW(keyname, buf);
    res = open_classes_key(HKEY_CLASSES_ROOT, keyname, access, subkey);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

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
HRESULT COM_GetRegisteredClassObject(const struct apartment *apt, REFCLSID rclsid,
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
  struct apartment *apt;

  TRACE("(%s,%p,0x%08x,0x%08x,%p)\n",
	debugstr_guid(rclsid),pUnk,dwClsContext,flags,lpdwRegister);

  if ( (lpdwRegister==0) || (pUnk==0) )
    return E_INVALIDARG;

  if (!(apt = apartment_get_current_or_mta()))
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
      apartment_release(apt);
      return hr;
    }
    IUnknown_Release(foundObject);
    ERR("object already registered for class %s\n", debugstr_guid(rclsid));
    apartment_release(apt);
    return CO_E_OBJISREG;
  }

  newClass = HeapAlloc(GetProcessHeap(), 0, sizeof(RegisteredClass));
  if ( newClass == NULL )
  {
    apartment_release(apt);
    return E_OUTOFMEMORY;
  }

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

      hr = apartment_get_local_server_stream(apt, &marshal_stream);
      if(FAILED(hr))
      {
          apartment_release(apt);
          return hr;
      }

      hr = RPC_StartLocalServer(&newClass->classIdentifier,
                                marshal_stream,
                                flags & (REGCLS_MULTIPLEUSE|REGCLS_MULTI_SEPARATE),
                                &newClass->RpcRegistration);
      IStream_Release(marshal_stream);
  }
  apartment_release(apt);
  return S_OK;
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
    struct class_reg_data clsreg = { 0 };
    IUnknown *regClassObject;
    HRESULT	hres = E_UNEXPECTED;
    struct apartment *apt;

    TRACE("CLSID: %s,IID: %s\n", debugstr_guid(rclsid), debugstr_guid(iid));

    if (!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (pServerInfo) {
	FIXME("pServerInfo->name=%s pAuthInfo=%p\n",
              debugstr_w(pServerInfo->pwszName), pServerInfo->pAuthInfo);
    }

    if (CLSCTX_INPROC_SERVER & dwClsContext)
    {
        if (IsEqualCLSID(rclsid, &CLSID_InProcFreeMarshaler) ||
                IsEqualCLSID(rclsid, &CLSID_GlobalOptions) ||
                IsEqualCLSID(rclsid, &CLSID_ManualResetEvent) ||
                IsEqualCLSID(rclsid, &CLSID_StdGlobalInterfaceTable))
        {
            apartment_release(apt);
            return Ole32DllGetClassObject(rclsid, iid, ppv);
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

            clsreg.u.actctx.module_name = (WCHAR *)((BYTE *)data.lpSectionBase + comclass->name_offset);
            clsreg.u.actctx.hactctx = data.hActCtx;
            clsreg.u.actctx.threading_model = comclass->model;
            clsreg.origin = CLASS_REG_ACTCTX;

            hres = apartment_get_inproc_class_object(apt, &clsreg, &comclass->clsid, iid, !(dwClsContext & WINE_CLSCTX_DONT_HOST), ppv);
            ReleaseActCtx(data.hActCtx);
            apartment_release(apt);
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
      apartment_release(apt);
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
            clsreg.origin = CLASS_REG_REGISTRY;

            hres = apartment_get_inproc_class_object(apt, &clsreg, rclsid, iid, !(dwClsContext & WINE_CLSCTX_DONT_HOST), ppv);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hres))
        {
            apartment_release(apt);
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
            clsreg.origin = CLASS_REG_REGISTRY;

            hres = apartment_get_inproc_class_object(apt, &clsreg, rclsid, iid, !(dwClsContext & WINE_CLSCTX_DONT_HOST), ppv);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hres))
        {
            apartment_release(apt);
            return hres;
        }
    }
    apartment_release(apt);

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

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    stubmgr = get_stub_manager_from_object(apt, pUnk, fLock);
    if (!stubmgr)
    {
        WARN("stub object not found %p\n", pUnk);
        /* Note: native is pretty broken here because it just silently
         * fails, without returning an appropriate error code, making apps
         * think that the object was disconnected, when it actually wasn't */
        apartment_release(apt);
        return S_OK;
    }

    if (fLock)
        stub_manager_ext_addref(stubmgr, 1, FALSE);
    else
        stub_manager_ext_release(stubmgr, 1, FALSE, fLastUnlockReleases);

    stub_manager_int_release(stubmgr);
    apartment_release(apt);
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
            if(!StringFromGUID2(clsidNew, szClsidNew, ARRAY_SIZE(szClsidNew))){
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
 *           CoGetObject [OLE32.@]
 *
 * Gets the object named by converting the name to a moniker and binding to it.
 *
 * PARAMS
 *  pszName      [I] String representing the object.
 *  pBindOptions [I] Parameters affecting the binding to the named object.
 *  riid         [I] Interface to bind to on the object.
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

/* Returns expanded dll path from the registry or activation context. */
static BOOL get_object_dll_path(const struct class_reg_data *regdata, WCHAR *dst, DWORD dstlen)
{
    DWORD ret;

    if (regdata->origin == CLASS_REG_REGISTRY)
    {
	DWORD keytype;
	WCHAR src[MAX_PATH];
	DWORD dwLength = dstlen * sizeof(WCHAR);

        if( (ret = RegQueryValueExW(regdata->u.hkey, NULL, NULL, &keytype, (BYTE*)src, &dwLength)) == ERROR_SUCCESS ) {
            if (keytype == REG_EXPAND_SZ) {
              if (dstlen <= ExpandEnvironmentStringsW(src, dst, dstlen)) ret = ERROR_MORE_DATA;
            } else {
              const WCHAR *quote_start;
              quote_start = wcschr(src, '\"');
              if (quote_start) {
                const WCHAR *quote_end = wcschr(quote_start + 1, '\"');
                if (quote_end) {
                  memmove(src, quote_start + 1,
                          (quote_end - quote_start - 1) * sizeof(WCHAR));
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
        static const WCHAR dllW[] = {'.','d','l','l',0};
        ULONG_PTR cookie;

        *dst = 0;
        ActivateActCtx(regdata->u.actctx.hactctx, &cookie);
        ret = SearchPathW(NULL, regdata->u.actctx.module_name, dllW, dstlen, dst, NULL);
        DeactivateActCtx(0, cookie);
        return *dst != 0;
    }
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
        regdata.origin = CLASS_REG_REGISTRY;

        if (get_object_dll_path(&regdata, dllpath, ARRAY_SIZE(dllpath)))
        {
            static const WCHAR wszOle32[] = {'o','l','e','3','2','.','d','l','l',0};
            if (!wcsicmp(dllpath, wszOle32))
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
 *           CoGetApartmentType [OLE32.@]
 */
HRESULT WINAPI CoGetApartmentType(APTTYPE *type, APTTYPEQUALIFIER *qualifier)
{
    struct oletls *info = COM_CurrentInfo();
    struct apartment *apt;

    TRACE("(%p, %p)\n", type, qualifier);

    if (!type || !qualifier)
        return E_INVALIDARG;

    if (!info)
        return E_OUTOFMEMORY;

    if (!info->apt)
        *type = APTTYPE_CURRENT;
    else if (info->apt->multi_threaded)
        *type = APTTYPE_MTA;
    else if (info->apt->main)
        *type = APTTYPE_MAINSTA;
    else
        *type = APTTYPE_STA;

    *qualifier = APTTYPEQUALIFIER_NONE;

    if (!info->apt && (apt = apartment_get_mta()))
    {
        apartment_release(apt);
        *type = APTTYPE_MTA;
        *qualifier = APTTYPEQUALIFIER_IMPLICIT_MTA;
        return S_OK;
    }

    return info->apt ? S_OK : CO_E_NOTINITIALIZED;
}

/***********************************************************************
 *           CoIncrementMTAUsage [OLE32.@]
 */
HRESULT WINAPI CoIncrementMTAUsage(CO_MTA_USAGE_COOKIE *cookie)
{
    TRACE("%p\n", cookie);

    return apartment_increment_mta_usage(cookie);
}

/***********************************************************************
 *           CoDecrementMTAUsage [OLE32.@]
 */
HRESULT WINAPI CoDecrementMTAUsage(CO_MTA_USAGE_COOKIE cookie)
{
    TRACE("%p\n", cookie);

    apartment_decrement_mta_usage(cookie);
    return S_OK;
}

/***********************************************************************
 *           CoDisableCallCancellation [OLE32.@]
 */
HRESULT WINAPI CoDisableCallCancellation(void *reserved)
{
    FIXME("(%p): stub\n", reserved);

    return E_NOTIMPL;
}

/***********************************************************************
 *           CoEnableCallCancellation [OLE32.@]
 */
HRESULT WINAPI CoEnableCallCancellation(void *reserved)
{
    FIXME("(%p): stub\n", reserved);

    return E_NOTIMPL;
}

/***********************************************************************
 *           CoRegisterSurrogate [OLE32.@]
 */
HRESULT WINAPI CoRegisterSurrogate(ISurrogate *surrogate)
{
    FIXME("(%p): stub\n", surrogate);

    return E_NOTIMPL;
}

/***********************************************************************
 *           CoRegisterSurrogateEx [OLE32.@]
 */
HRESULT WINAPI CoRegisterSurrogateEx(REFGUID guid, void *reserved)
{
    FIXME("(%s %p): stub\n", debugstr_guid(guid), reserved);

    return E_NOTIMPL;
}

BOOL WINAPI InternalIsInitialized(void)
{
    struct apartment *apt;

    if (!(apt = apartment_get_current_or_mta()))
        return FALSE;
    apartment_release(apt);

    return TRUE;
}

typedef struct {
    IGlobalOptions IGlobalOptions_iface;
    LONG ref;
} GlobalOptions;

static inline GlobalOptions *impl_from_IGlobalOptions(IGlobalOptions *iface)
{
    return CONTAINING_RECORD(iface, GlobalOptions, IGlobalOptions_iface);
}

static HRESULT WINAPI GlobalOptions_QueryInterface(IGlobalOptions *iface, REFIID riid, void **ppv)
{
    GlobalOptions *This = impl_from_IGlobalOptions(iface);

    TRACE("(%p)->(%s %p)\n", This, debugstr_guid(riid), ppv);

    if (IsEqualGUID(&IID_IGlobalOptions, riid) || IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown*)*ppv);
    return S_OK;
}

static ULONG WINAPI GlobalOptions_AddRef(IGlobalOptions *iface)
{
    GlobalOptions *This = impl_from_IGlobalOptions(iface);
    LONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    return ref;
}

static ULONG WINAPI GlobalOptions_Release(IGlobalOptions *iface)
{
    GlobalOptions *This = impl_from_IGlobalOptions(iface);
    LONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) ref=%d\n", This, ref);

    if (!ref)
        heap_free(This);

    return ref;
}

static HRESULT WINAPI GlobalOptions_Set(IGlobalOptions *iface, GLOBALOPT_PROPERTIES property, ULONG_PTR value)
{
    GlobalOptions *This = impl_from_IGlobalOptions(iface);
    FIXME("(%p)->(%u %lx)\n", This, property, value);
    return S_OK;
}

static HRESULT WINAPI GlobalOptions_Query(IGlobalOptions *iface, GLOBALOPT_PROPERTIES property, ULONG_PTR *value)
{
    GlobalOptions *This = impl_from_IGlobalOptions(iface);
    FIXME("(%p)->(%u %p)\n", This, property, value);
    return E_NOTIMPL;
}

static const IGlobalOptionsVtbl GlobalOptionsVtbl = {
    GlobalOptions_QueryInterface,
    GlobalOptions_AddRef,
    GlobalOptions_Release,
    GlobalOptions_Set,
    GlobalOptions_Query
};

HRESULT WINAPI GlobalOptions_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    GlobalOptions *global_options;
    HRESULT hres;

    TRACE("(%p %s %p)\n", outer, debugstr_guid(riid), ppv);

    if (outer)
        return E_INVALIDARG;

    global_options = heap_alloc(sizeof(*global_options));
    if (!global_options)
        return E_OUTOFMEMORY;
    global_options->IGlobalOptions_iface.lpVtbl = &GlobalOptionsVtbl;
    global_options->ref = 1;

    hres = IGlobalOptions_QueryInterface(&global_options->IGlobalOptions_iface, riid, ppv);
    IGlobalOptions_Release(&global_options->IGlobalOptions_iface);
    return hres;
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
        RPC_UnregisterAllChannelHooks();
        DeleteCriticalSection(&csRegisteredClassList);
        apartment_global_cleanup();
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
