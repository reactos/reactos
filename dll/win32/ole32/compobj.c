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
 *   - Implement the OXID resolver so we don't need magic endpoint names for
 *     clients and servers to meet up
 *
 */

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#define COBJMACROS
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
static NTSTATUS create_key( HKEY *retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr )
{
    NTSTATUS status = NtCreateKey( (HANDLE *)retkey, access, attr, 0, NULL, 0, NULL );

    if (status == STATUS_OBJECT_NAME_NOT_FOUND)
    {
        HANDLE subkey;
        WCHAR *buffer = attr->ObjectName->Buffer;
        DWORD pos = 0, i = 0, len = attr->ObjectName->Length / sizeof(WCHAR);
        UNICODE_STRING str;
        OBJECT_ATTRIBUTES attr2 = *attr;

        while (i < len && buffer[i] != '\\') i++;
        if (i == len) return status;

        attr2.ObjectName = &str;

        while (i < len)
        {
            str.Buffer = buffer + pos;
            str.Length = (i - pos) * sizeof(WCHAR);
            status = NtCreateKey( &subkey, access, &attr2, 0, NULL, 0, NULL );
            if (attr2.RootDirectory != attr->RootDirectory) NtClose( attr2.RootDirectory );
            if (status) return status;
            attr2.RootDirectory = subkey;
            while (i < len && buffer[i] == '\\') i++;
            pos = i;
            while (i < len && buffer[i] != '\\') i++;
        }
        str.Buffer = buffer + pos;
        str.Length = (i - pos) * sizeof(WCHAR);
        status = NtCreateKey( (PHANDLE)retkey, access, &attr2, 0, NULL, 0, NULL );
        if (attr2.RootDirectory != attr->RootDirectory) NtClose( attr2.RootDirectory );
    }
    return status;
}

static HKEY classes_root_hkey;

#ifndef RTL_CONSTANT_STRING
#define RTL_CONSTANT_STRING(s)  { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#endif

/* create the special HKEY_CLASSES_ROOT key */
static HKEY create_classes_root_hkey(DWORD access)
{
    HKEY hkey, ret = 0;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Registry\\Machine\\Software\\Classes");

    attr.Length = sizeof(attr);
    attr.RootDirectory = 0;
    attr.ObjectName = &name;
#ifdef __REACTOS__
    attr.Attributes = OBJ_CASE_INSENSITIVE;
#else
    attr.Attributes = 0;
#endif
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
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
        access &= ~KEY_WOW64_32KEY;
        if (create_classes_key(classes_root_hkey, L"Wow6432Node", access, &hkey))
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
#ifdef __REACTOS__
    attr.Attributes = OBJ_CASE_INSENSITIVE;
#else
    attr.Attributes = 0;
#endif
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
#ifdef __REACTOS__
    attr.Attributes = OBJ_CASE_INSENSITIVE;
#else
    attr.Attributes = 0;
#endif
    attr.SecurityDescriptor = NULL;
    attr.SecurityQualityOfService = NULL;
    RtlInitUnicodeString( &nameW, name );

    return RtlNtStatusToDosError( NtOpenKey( (HANDLE *)retkey, access, &attr ) );
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
    TRACE("%p, refcount %ld.\n", iface, ref);

    return ref;
}

static ULONG WINAPI ISynchronize_fnRelease(ISynchronize *iface)
{
    MREImpl *This = impl_from_ISynchronize(iface);
    LONG ref = InterlockedDecrement(&This->ref);
    TRACE("%p, refcount %ld.\n", iface, ref);

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
    DWORD index;
    TRACE("%p, %#lx, %#lx.\n", iface, dwFlags, dwMilliseconds);
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

/* open HKCR\\CLSID\\{string form of clsid}\\{keyname} key */
HRESULT COM_OpenKeyForCLSID(REFCLSID clsid, LPCWSTR keyname, REGSAM access, HKEY *subkey)
{
    WCHAR path[CHARS_IN_GUID + ARRAY_SIZE(L"CLSID\\") - 1];
    LONG res;
    HKEY key;

    lstrcpyW(path, L"CLSID\\");
    StringFromGUID2(clsid, path + lstrlenW(L"CLSID\\"), CHARS_IN_GUID);
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
       if (!RegQueryValueW(hkey, L"AutoTreatAs", auto_treat_as, &auto_treat_as_size) &&
           CLSIDFromString(auto_treat_as, &id) == S_OK)
       {
           if (RegSetValueW(hkey, L"TreatAs", REG_SZ, auto_treat_as, sizeof(auto_treat_as)))
           {
               res = REGDB_E_WRITEREGDB;
               goto done;
           }
       }
       else
       {
           if (RegDeleteKeyW(hkey, L"TreatAs"))
               res = REGDB_E_WRITEREGDB;
           goto done;
       }
    }
    else
    {
        if(IsEqualGUID(clsidNew, &CLSID_NULL)){
           RegDeleteKeyW(hkey, L"TreatAs");
        }else{
            if(!StringFromGUID2(clsidNew, szClsidNew, ARRAY_SIZE(szClsidNew))){
                WARN("StringFromGUID2 failed\n");
                res = E_FAIL;
                goto done;
            }

            if (RegSetValueW(hkey, L"TreatAs", REG_SZ, szClsidNew, sizeof(szClsidNew)) != ERROR_SUCCESS){
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
        ULONG_PTR cookie;

        *dst = 0;
        ActivateActCtx(regdata->u.actctx.hactctx, &cookie);
        ret = SearchPathW(NULL, regdata->u.actctx.module_name, L".dll", dstlen, dst, NULL);
        DeactivateActCtx(0, cookie);
        return *dst != 0;
    }
}

HRESULT Handler_DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HKEY hkey;
    HRESULT hres;

    hres = COM_OpenKeyForCLSID(rclsid, L"InprocHandler32", KEY_READ, &hkey);
    if (SUCCEEDED(hres))
    {
        struct class_reg_data regdata;
        WCHAR dllpath[MAX_PATH+1];

        regdata.u.hkey = hkey;
        regdata.origin = CLASS_REG_REGISTRY;

        if (get_object_dll_path(&regdata, dllpath, ARRAY_SIZE(dllpath)))
        {
            if (!wcsicmp(dllpath, L"ole32.dll"))
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
    TRACE("%p, %#lx, %p.\n", hinstDLL, fdwReason, reserved);

    switch(fdwReason) {
    case DLL_PROCESS_ATTACH:
        hProxyDll = hinstDLL;
	break;

    case DLL_PROCESS_DETACH:
        clipbrd_destroy();
        if (reserved) break;
        release_std_git();
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
