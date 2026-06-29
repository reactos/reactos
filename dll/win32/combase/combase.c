/*
 *    Copyright 2005 Juan Lang
 *    Copyright 2005-2006 Robert Shearman (for CodeWeavers)
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

#define COBJMACROS
#include "ntstatus.h"
#define WIN32_NO_STATUS
#define USE_COM_CONTEXT_DEF
#include "objbase.h"
#include "ctxtcall.h"
#include "oleauto.h"
#include "dde.h"
#include "winternl.h"

#include "combase_private.h"

#include "wine/debug.h"

#ifndef RTL_CONSTANT_STRING
#define RTL_CONSTANT_STRING(s)  { sizeof(s)-sizeof((s)[0]), sizeof(s), s }
#endif

WINE_DEFAULT_DEBUG_CHANNEL(ole);

HINSTANCE hProxyDll;

static ULONG_PTR global_options[COMGLB_PROPERTIES_RESERVED3 + 1];

/* Ole32 exports */
extern void WINAPI DestroyRunningObjectTable(void);
extern HRESULT WINAPI Ole32DllGetClassObject(REFCLSID rclsid, REFIID riid, void **obj);

/*
 * Number of times CoInitialize is called. It is decreased every time CoUninitialize is called. When it hits 0, the COM libraries are freed
 */
static LONG com_lockcount;

static LONG com_server_process_refcount;

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

struct init_spy
{
    struct list entry;
    IInitializeSpy *spy;
    unsigned int id;
};

struct registered_ps
{
    struct list entry;
    IID iid;
    CLSID clsid;
};

static struct list registered_proxystubs = LIST_INIT(registered_proxystubs);

static CRITICAL_SECTION cs_registered_ps;
static CRITICAL_SECTION_DEBUG psclsid_cs_debug =
{
    0, 0, &cs_registered_ps,
    { &psclsid_cs_debug.ProcessLocksList, &psclsid_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": cs_registered_psclsid_list") }
};
static CRITICAL_SECTION cs_registered_ps = { &psclsid_cs_debug, -1, 0, 0, 0, 0 };

struct registered_class
{
    struct list entry;
    CLSID clsid;
    OXID apartment_id;
    IUnknown *object;
    DWORD clscontext;
    DWORD flags;
    unsigned int cookie;
    unsigned int rpcss_cookie;
};

static struct list registered_classes = LIST_INIT(registered_classes);

static CRITICAL_SECTION registered_classes_cs;
static CRITICAL_SECTION_DEBUG registered_classes_cs_debug =
{
    0, 0, &registered_classes_cs,
    { &registered_classes_cs_debug.ProcessLocksList, &registered_classes_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": registered_classes_cs") }
};
static CRITICAL_SECTION registered_classes_cs = { &registered_classes_cs_debug, -1, 0, 0, 0, 0 };

IUnknown * com_get_registered_class_object(const struct apartment *apt, REFCLSID rclsid, DWORD clscontext)
{
    struct registered_class *cur;
    IUnknown *object = NULL;

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY(cur, &registered_classes, struct registered_class, entry)
    {
        if ((apt->oxid == cur->apartment_id) &&
            (clscontext & cur->clscontext) &&
            IsEqualGUID(&cur->clsid, rclsid))
        {
            object = cur->object;
            IUnknown_AddRef(cur->object);
            break;
        }
    }

    LeaveCriticalSection(&registered_classes_cs);

    return object;
}

static struct init_spy *get_spy_entry(struct tlsdata *tlsdata, unsigned int id)
{
    struct init_spy *spy;

    LIST_FOR_EACH_ENTRY(spy, &tlsdata->spies, struct init_spy, entry)
    {
        if (id == spy->id && spy->spy)
            return spy;
    }

    return NULL;
}

static NTSTATUS create_key(HKEY *retkey, ACCESS_MASK access, const OBJECT_ATTRIBUTES *attr)
{
    NTSTATUS status = NtCreateKey((HANDLE *)retkey, access, attr, 0, NULL, 0, NULL);

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
            status = NtCreateKey(&subkey, access, &attr2, 0, NULL, 0, NULL);
            if (attr2.RootDirectory != attr->RootDirectory) NtClose(attr2.RootDirectory);
            if (status) return status;
            attr2.RootDirectory = subkey;
            while (i < len && buffer[i] == '\\') i++;
            pos = i;
            while (i < len && buffer[i] != '\\') i++;
        }
        str.Buffer = buffer + pos;
        str.Length = (i - pos) * sizeof(WCHAR);
        status = NtCreateKey((HANDLE *)retkey, access, &attr2, 0, NULL, 0, NULL);
        if (attr2.RootDirectory != attr->RootDirectory) NtClose(attr2.RootDirectory);
    }
    return status;
}

static HKEY classes_root_hkey;

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

static HKEY get_classes_root_hkey(HKEY hkey, REGSAM access);

static LSTATUS create_classes_key(HKEY hkey, const WCHAR *name, REGSAM access, HKEY *retkey)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (!(hkey = get_classes_root_hkey(hkey, access)))
        return ERROR_INVALID_HANDLE;

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

    return RtlNtStatusToDosError(create_key(retkey, access, &attr));
}

static HKEY get_classes_root_hkey(HKEY hkey, REGSAM access)
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

static LSTATUS open_classes_key(HKEY hkey, const WCHAR *name, REGSAM access, HKEY *retkey)
{
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING nameW;

    if (!(hkey = get_classes_root_hkey(hkey, access)))
        return ERROR_INVALID_HANDLE;

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

    return RtlNtStatusToDosError(NtOpenKey((HANDLE *)retkey, access, &attr));
}

HRESULT open_key_for_clsid(REFCLSID clsid, const WCHAR *keyname, REGSAM access, HKEY *subkey)
{
    static const WCHAR clsidW[] = L"CLSID\\";
    WCHAR path[CHARS_IN_GUID + ARRAY_SIZE(clsidW) - 1];
    LONG res;
    HKEY key;

    lstrcpyW(path, clsidW);
    StringFromGUID2(clsid, path + lstrlenW(clsidW), CHARS_IN_GUID);
    res = open_classes_key(HKEY_CLASSES_ROOT, path, access, &key);

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
HRESULT open_appidkey_from_clsid(REFCLSID clsid, REGSAM access, HKEY *subkey)
{
    static const WCHAR appidkeyW[] = L"AppId\\";
    DWORD res;
    WCHAR buf[CHARS_IN_GUID];
    WCHAR keyname[ARRAY_SIZE(appidkeyW) + CHARS_IN_GUID];
    DWORD size;
    HKEY hkey;
    DWORD type;
    HRESULT hr;

    /* read the AppID value under the class's key */
    hr = open_key_for_clsid(clsid, NULL, access, &hkey);
    if (FAILED(hr))
        return hr;

    size = sizeof(buf);
    res = RegQueryValueExW(hkey, L"AppId", NULL, &type, (LPBYTE)buf, &size);
    RegCloseKey(hkey);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS || type!=REG_SZ)
        return REGDB_E_READREGDB;

    lstrcpyW(keyname, appidkeyW);
    lstrcatW(keyname, buf);
    res = open_classes_key(HKEY_CLASSES_ROOT, keyname, access, subkey);
    if (res == ERROR_FILE_NOT_FOUND)
        return REGDB_E_KEYMISSING;
    else if (res != ERROR_SUCCESS)
        return REGDB_E_READREGDB;

    return S_OK;
}

/***********************************************************************
 *           InternalIsProcessInitialized  (combase.@)
 */
BOOL WINAPI InternalIsProcessInitialized(void)
{
    struct apartment *apt;

    if (!(apt = apartment_get_current_or_mta()))
        return FALSE;
    apartment_release(apt);

    return TRUE;
}

/***********************************************************************
 *           InternalTlsAllocData    (combase.@)
 */
HRESULT WINAPI InternalTlsAllocData(struct tlsdata **data)
{
    if (!(*data = calloc(1, sizeof(**data))))
        return E_OUTOFMEMORY;

    list_init(&(*data)->spies);
    NtCurrentTeb()->ReservedForOle = *data;

    return S_OK;
}

static void com_cleanup_tlsdata(void)
{
    struct tlsdata *tlsdata = NtCurrentTeb()->ReservedForOle;
    struct init_spy *cursor, *cursor2;

    if (!tlsdata)
        return;

    if (tlsdata->apt)
        apartment_release(tlsdata->apt);
    if (tlsdata->implicit_mta_cookie)
        apartment_decrement_mta_usage(tlsdata->implicit_mta_cookie);

    if (tlsdata->errorinfo)
        IErrorInfo_Release(tlsdata->errorinfo);
    if (tlsdata->state)
        IUnknown_Release(tlsdata->state);

    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &tlsdata->spies, struct init_spy, entry)
    {
        list_remove(&cursor->entry);
        if (cursor->spy)
            IInitializeSpy_Release(cursor->spy);
        free(cursor);
    }

    if (tlsdata->context_token)
        IObjContext_Release(tlsdata->context_token);

    free(tlsdata);
    NtCurrentTeb()->ReservedForOle = NULL;
}

struct global_options
{
    IGlobalOptions IGlobalOptions_iface;
    LONG refcount;
};

static inline struct global_options *impl_from_IGlobalOptions(IGlobalOptions *iface)
{
    return CONTAINING_RECORD(iface, struct global_options, IGlobalOptions_iface);
}

static HRESULT WINAPI global_options_QueryInterface(IGlobalOptions *iface, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualGUID(&IID_IGlobalOptions, riid) || IsEqualGUID(&IID_IUnknown, riid))
    {
        *ppv = iface;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }

    IUnknown_AddRef((IUnknown *)*ppv);
    return S_OK;
}

static ULONG WINAPI global_options_AddRef(IGlobalOptions *iface)
{
    struct global_options *options = impl_from_IGlobalOptions(iface);
    LONG refcount = InterlockedIncrement(&options->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    return refcount;
}

static ULONG WINAPI global_options_Release(IGlobalOptions *iface)
{
    struct global_options *options = impl_from_IGlobalOptions(iface);
    LONG refcount = InterlockedDecrement(&options->refcount);

    TRACE("%p, refcount %ld.\n", iface, refcount);

    if (!refcount)
        free(options);

    return refcount;
}

static HRESULT WINAPI global_options_Set(IGlobalOptions *iface, GLOBALOPT_PROPERTIES property, ULONG_PTR value)
{
    FIXME("%p, %u, %Ix.\n", iface, property, value);

    return S_OK;
}

static HRESULT WINAPI global_options_Query(IGlobalOptions *iface, GLOBALOPT_PROPERTIES property, ULONG_PTR *value)
{
    TRACE("%p, %u, %p.\n", iface, property, value);

    if (property < COMGLB_EXCEPTION_HANDLING || property > COMGLB_PROPERTIES_RESERVED3)
        return E_INVALIDARG;

    *value = global_options[property];

    return S_OK;
}

static const IGlobalOptionsVtbl global_options_vtbl =
{
    global_options_QueryInterface,
    global_options_AddRef,
    global_options_Release,
    global_options_Set,
    global_options_Query
};

static HRESULT WINAPI class_factory_QueryInterface(IClassFactory *iface, REFIID riid, void **ppv)
{
    TRACE("%p, %s, %p.\n", iface, debugstr_guid(riid), ppv);

    if (IsEqualIID(riid, &IID_IUnknown) || IsEqualIID(riid, &IID_IClassFactory))
    {
        *ppv = iface;
        return S_OK;
    }

    *ppv = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI class_factory_AddRef(IClassFactory *iface)
{
    return 2;
}

static ULONG WINAPI class_factory_Release(IClassFactory *iface)
{
    return 1;
}

static HRESULT WINAPI class_factory_LockServer(IClassFactory *iface, BOOL fLock)
{
    TRACE("%d\n", fLock);

    return S_OK;
}

static HRESULT WINAPI global_options_CreateInstance(IClassFactory *iface, IUnknown *outer, REFIID riid, void **ppv)
{
    struct global_options *object;
    HRESULT hr;

    TRACE("%p, %s, %p.\n", outer, debugstr_guid(riid), ppv);

    if (outer)
        return E_INVALIDARG;

    if (!(object = malloc(sizeof(*object))))
        return E_OUTOFMEMORY;
    object->IGlobalOptions_iface.lpVtbl = &global_options_vtbl;
    object->refcount = 1;

    hr = IGlobalOptions_QueryInterface(&object->IGlobalOptions_iface, riid, ppv);
    IGlobalOptions_Release(&object->IGlobalOptions_iface);
    return hr;
}

static const IClassFactoryVtbl global_options_factory_vtbl =
{
    class_factory_QueryInterface,
    class_factory_AddRef,
    class_factory_Release,
    global_options_CreateInstance,
    class_factory_LockServer
};

static IClassFactory global_options_factory = { &global_options_factory_vtbl };

static HRESULT get_builtin_class_factory(REFCLSID rclsid, REFIID riid, void **obj)
{
    if (IsEqualCLSID(rclsid, &CLSID_GlobalOptions))
        return IClassFactory_QueryInterface(&global_options_factory, riid, obj);
    return E_UNEXPECTED;
}

/***********************************************************************
 *           FreePropVariantArray    (combase.@)
 */
HRESULT WINAPI FreePropVariantArray(ULONG count, PROPVARIANT *rgvars)
{
    ULONG i;

    TRACE("%lu, %p.\n", count, rgvars);

    if (!rgvars)
        return E_INVALIDARG;

    for (i = 0; i < count; ++i)
        PropVariantClear(&rgvars[i]);

    return S_OK;
}

static HRESULT propvar_validatetype(VARTYPE vt)
{
    switch (vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_BSTR:
    case VT_ERROR:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_LPSTR:
    case VT_LPWSTR:
    case VT_FILETIME:
    case VT_BLOB:
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_STREAM:
    case VT_STORAGE:
    case VT_STREAMED_OBJECT:
    case VT_STORED_OBJECT:
    case VT_BLOB_OBJECT:
    case VT_CF:
    case VT_CLSID:
    case VT_I1|VT_VECTOR:
    case VT_I2|VT_VECTOR:
    case VT_I4|VT_VECTOR:
    case VT_I8|VT_VECTOR:
    case VT_R4|VT_VECTOR:
    case VT_R8|VT_VECTOR:
    case VT_CY|VT_VECTOR:
    case VT_DATE|VT_VECTOR:
    case VT_BSTR|VT_VECTOR:
    case VT_ERROR|VT_VECTOR:
    case VT_BOOL|VT_VECTOR:
    case VT_VARIANT|VT_VECTOR:
    case VT_UI1|VT_VECTOR:
    case VT_UI2|VT_VECTOR:
    case VT_UI4|VT_VECTOR:
    case VT_UI8|VT_VECTOR:
    case VT_LPSTR|VT_VECTOR:
    case VT_LPWSTR|VT_VECTOR:
    case VT_FILETIME|VT_VECTOR:
    case VT_CF|VT_VECTOR:
    case VT_CLSID|VT_VECTOR:
    case VT_ARRAY|VT_I1:
    case VT_ARRAY|VT_UI1:
    case VT_ARRAY|VT_I2:
    case VT_ARRAY|VT_UI2:
    case VT_ARRAY|VT_I4:
    case VT_ARRAY|VT_UI4:
    case VT_ARRAY|VT_INT:
    case VT_ARRAY|VT_UINT:
    case VT_ARRAY|VT_R4:
    case VT_ARRAY|VT_R8:
    case VT_ARRAY|VT_CY:
    case VT_ARRAY|VT_DATE:
    case VT_ARRAY|VT_BSTR:
    case VT_ARRAY|VT_BOOL:
    case VT_ARRAY|VT_DECIMAL:
    case VT_ARRAY|VT_DISPATCH:
    case VT_ARRAY|VT_UNKNOWN:
    case VT_ARRAY|VT_ERROR:
    case VT_ARRAY|VT_VARIANT:
        return S_OK;
    }
    WARN("Bad type %d\n", vt);
    return STG_E_INVALIDPARAMETER;
}

static void propvar_free_cf_array(ULONG count, CLIPDATA *data)
{
    ULONG i;
    for (i = 0; i < count; ++i)
        CoTaskMemFree(data[i].pClipData);
}

/***********************************************************************
 *           PropVariantClear    (combase.@)
 */
HRESULT WINAPI PropVariantClear(PROPVARIANT *pvar)
{
    HRESULT hr;

    TRACE("%p.\n", pvar);

    if (!pvar)
        return S_OK;

    hr = propvar_validatetype(pvar->vt);
    if (FAILED(hr))
    {
        memset(pvar, 0, sizeof(*pvar));
        return hr;
    }

    switch (pvar->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_I2:
    case VT_I4:
    case VT_I8:
    case VT_R4:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_ERROR:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_UI1:
    case VT_UI2:
    case VT_UI4:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_FILETIME:
        break;
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        if (pvar->pStream)
            IStream_Release(pvar->pStream);
        break;
    case VT_CLSID:
    case VT_LPSTR:
    case VT_LPWSTR:
        /* pick an arbitrary typed pointer - we don't care about the type
         * as we are just freeing it */
        CoTaskMemFree(pvar->puuid);
        break;
    case VT_BLOB:
    case VT_BLOB_OBJECT:
        CoTaskMemFree(pvar->blob.pBlobData);
        break;
    case VT_BSTR:
        SysFreeString(pvar->bstrVal);
        break;
    case VT_CF:
        if (pvar->pclipdata)
        {
            propvar_free_cf_array(1, pvar->pclipdata);
            CoTaskMemFree(pvar->pclipdata);
        }
        break;
    default:
        if (pvar->vt & VT_VECTOR)
        {
            ULONG i;

            switch (pvar->vt & ~VT_VECTOR)
            {
            case VT_VARIANT:
                FreePropVariantArray(pvar->capropvar.cElems, pvar->capropvar.pElems);
                break;
            case VT_CF:
                propvar_free_cf_array(pvar->caclipdata.cElems, pvar->caclipdata.pElems);
                break;
            case VT_BSTR:
                for (i = 0; i < pvar->cabstr.cElems; i++)
                    SysFreeString(pvar->cabstr.pElems[i]);
                break;
            case VT_LPSTR:
                for (i = 0; i < pvar->calpstr.cElems; i++)
                    CoTaskMemFree(pvar->calpstr.pElems[i]);
                break;
            case VT_LPWSTR:
                for (i = 0; i < pvar->calpwstr.cElems; i++)
                    CoTaskMemFree(pvar->calpwstr.pElems[i]);
                break;
            }
            if (pvar->vt & ~VT_VECTOR)
            {
                /* pick an arbitrary VT_VECTOR structure - they all have the same
                 * memory layout */
                CoTaskMemFree(pvar->capropvar.pElems);
            }
        }
        else if (pvar->vt & VT_ARRAY)
            hr = SafeArrayDestroy(pvar->parray);
        else
        {
            WARN("Invalid/unsupported type %d\n", pvar->vt);
            hr = STG_E_INVALIDPARAMETER;
        }
    }

    memset(pvar, 0, sizeof(*pvar));
    return hr;
}

/***********************************************************************
 *           PropVariantCopy        (combase.@)
 */
HRESULT WINAPI PropVariantCopy(PROPVARIANT *pvarDest, const PROPVARIANT *pvarSrc)
{
    ULONG len;
    HRESULT hr;

    TRACE("%p, %p vt %04x.\n", pvarDest, pvarSrc, pvarSrc->vt);

    hr = propvar_validatetype(pvarSrc->vt);
    if (FAILED(hr))
        return DISP_E_BADVARTYPE;

    /* this will deal with most cases */
    *pvarDest = *pvarSrc;

    switch (pvarSrc->vt)
    {
    case VT_EMPTY:
    case VT_NULL:
    case VT_I1:
    case VT_UI1:
    case VT_I2:
    case VT_UI2:
    case VT_BOOL:
    case VT_DECIMAL:
    case VT_I4:
    case VT_UI4:
    case VT_R4:
    case VT_ERROR:
    case VT_I8:
    case VT_UI8:
    case VT_INT:
    case VT_UINT:
    case VT_R8:
    case VT_CY:
    case VT_DATE:
    case VT_FILETIME:
        break;
    case VT_DISPATCH:
    case VT_UNKNOWN:
    case VT_STREAM:
    case VT_STREAMED_OBJECT:
    case VT_STORAGE:
    case VT_STORED_OBJECT:
        if (pvarDest->pStream)
            IStream_AddRef(pvarDest->pStream);
        break;
    case VT_CLSID:
        pvarDest->puuid = CoTaskMemAlloc(sizeof(CLSID));
        *pvarDest->puuid = *pvarSrc->puuid;
        break;
    case VT_LPSTR:
        if (pvarSrc->pszVal)
        {
            len = strlen(pvarSrc->pszVal);
            pvarDest->pszVal = CoTaskMemAlloc((len+1)*sizeof(CHAR));
            CopyMemory(pvarDest->pszVal, pvarSrc->pszVal, (len+1)*sizeof(CHAR));
        }
        break;
    case VT_LPWSTR:
        if (pvarSrc->pwszVal)
        {
            len = lstrlenW(pvarSrc->pwszVal);
            pvarDest->pwszVal = CoTaskMemAlloc((len+1)*sizeof(WCHAR));
            CopyMemory(pvarDest->pwszVal, pvarSrc->pwszVal, (len+1)*sizeof(WCHAR));
        }
        break;
    case VT_BLOB:
    case VT_BLOB_OBJECT:
        if (pvarSrc->blob.pBlobData)
        {
            len = pvarSrc->blob.cbSize;
            pvarDest->blob.pBlobData = CoTaskMemAlloc(len);
            CopyMemory(pvarDest->blob.pBlobData, pvarSrc->blob.pBlobData, len);
        }
        break;
    case VT_BSTR:
        pvarDest->bstrVal = SysAllocString(pvarSrc->bstrVal);
        break;
    case VT_CF:
        if (pvarSrc->pclipdata)
        {
            len = pvarSrc->pclipdata->cbSize - sizeof(pvarSrc->pclipdata->ulClipFmt);
            pvarDest->pclipdata = CoTaskMemAlloc(sizeof (CLIPDATA));
            pvarDest->pclipdata->cbSize = pvarSrc->pclipdata->cbSize;
            pvarDest->pclipdata->ulClipFmt = pvarSrc->pclipdata->ulClipFmt;
            pvarDest->pclipdata->pClipData = CoTaskMemAlloc(len);
            CopyMemory(pvarDest->pclipdata->pClipData, pvarSrc->pclipdata->pClipData, len);
        }
        break;
    default:
        if (pvarSrc->vt & VT_VECTOR)
        {
            int elemSize;
            ULONG i;

            switch (pvarSrc->vt & ~VT_VECTOR)
            {
            case VT_I1:       elemSize = sizeof(pvarSrc->cVal); break;
            case VT_UI1:      elemSize = sizeof(pvarSrc->bVal); break;
            case VT_I2:       elemSize = sizeof(pvarSrc->iVal); break;
            case VT_UI2:      elemSize = sizeof(pvarSrc->uiVal); break;
            case VT_BOOL:     elemSize = sizeof(pvarSrc->boolVal); break;
            case VT_I4:       elemSize = sizeof(pvarSrc->lVal); break;
            case VT_UI4:      elemSize = sizeof(pvarSrc->ulVal); break;
            case VT_R4:       elemSize = sizeof(pvarSrc->fltVal); break;
            case VT_R8:       elemSize = sizeof(pvarSrc->dblVal); break;
            case VT_ERROR:    elemSize = sizeof(pvarSrc->scode); break;
            case VT_I8:       elemSize = sizeof(pvarSrc->hVal); break;
            case VT_UI8:      elemSize = sizeof(pvarSrc->uhVal); break;
            case VT_CY:       elemSize = sizeof(pvarSrc->cyVal); break;
            case VT_DATE:     elemSize = sizeof(pvarSrc->date); break;
            case VT_FILETIME: elemSize = sizeof(pvarSrc->filetime); break;
            case VT_CLSID:    elemSize = sizeof(*pvarSrc->puuid); break;
            case VT_CF:       elemSize = sizeof(*pvarSrc->pclipdata); break;
            case VT_BSTR:     elemSize = sizeof(pvarSrc->bstrVal); break;
            case VT_LPSTR:    elemSize = sizeof(pvarSrc->pszVal); break;
            case VT_LPWSTR:   elemSize = sizeof(pvarSrc->pwszVal); break;
            case VT_VARIANT:  elemSize = sizeof(*pvarSrc->pvarVal); break;

            default:
                FIXME("Invalid element type: %ul\n", pvarSrc->vt & ~VT_VECTOR);
                return E_INVALIDARG;
            }
            len = pvarSrc->capropvar.cElems;
            pvarDest->capropvar.pElems = len ? CoTaskMemAlloc(len * elemSize) : NULL;
            if (pvarSrc->vt == (VT_VECTOR | VT_VARIANT))
            {
                for (i = 0; i < len; i++)
                    PropVariantCopy(&pvarDest->capropvar.pElems[i], &pvarSrc->capropvar.pElems[i]);
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_CF))
            {
                FIXME("Copy clipformats\n");
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_BSTR))
            {
                for (i = 0; i < len; i++)
                    pvarDest->cabstr.pElems[i] = SysAllocString(pvarSrc->cabstr.pElems[i]);
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_LPSTR))
            {
                size_t strLen;
                for (i = 0; i < len; i++)
                {
                    strLen = lstrlenA(pvarSrc->calpstr.pElems[i]) + 1;
                    pvarDest->calpstr.pElems[i] = CoTaskMemAlloc(strLen);
                    memcpy(pvarDest->calpstr.pElems[i],
                     pvarSrc->calpstr.pElems[i], strLen);
                }
            }
            else if (pvarSrc->vt == (VT_VECTOR | VT_LPWSTR))
            {
                size_t strLen;
                for (i = 0; i < len; i++)
                {
                    strLen = (lstrlenW(pvarSrc->calpwstr.pElems[i]) + 1) *
                     sizeof(WCHAR);
                    pvarDest->calpstr.pElems[i] = CoTaskMemAlloc(strLen);
                    memcpy(pvarDest->calpstr.pElems[i],
                     pvarSrc->calpstr.pElems[i], strLen);
                }
            }
            else
                CopyMemory(pvarDest->capropvar.pElems, pvarSrc->capropvar.pElems, len * elemSize);
        }
        else if (pvarSrc->vt & VT_ARRAY)
        {
            pvarDest->uhVal.QuadPart = 0;
            return SafeArrayCopy(pvarSrc->parray, &pvarDest->parray);
        }
        else
            WARN("Invalid/unsupported type %d\n", pvarSrc->vt);
    }

    return S_OK;
}

/***********************************************************************
 *           CoFileTimeNow        (combase.@)
 */
HRESULT WINAPI CoFileTimeNow(FILETIME *filetime)
{
    GetSystemTimeAsFileTime(filetime);
    return S_OK;
}

/******************************************************************************
 *            CoCreateGuid        (combase.@)
 */
HRESULT WINAPI CoCreateGuid(GUID *guid)
{
    RPC_STATUS status;

    if (!guid) return E_INVALIDARG;

    status = UuidCreate(guid);
    if (status == RPC_S_OK || status == RPC_S_UUID_LOCAL_ONLY) return S_OK;
    return HRESULT_FROM_WIN32(status);
}

/******************************************************************************
 *            CoQueryProxyBlanket        (combase.@)
 */
HRESULT WINAPI CoQueryProxyBlanket(IUnknown *proxy, DWORD *authn_service,
        DWORD *authz_service, OLECHAR **servername, DWORD *authn_level,
        DWORD *imp_level, void **auth_info, DWORD *capabilities)
{
    IClientSecurity *client_security;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %p, %p, %p, %p.\n", proxy, authn_service, authz_service, servername, authn_level, imp_level,
            auth_info, capabilities);

    hr = IUnknown_QueryInterface(proxy, &IID_IClientSecurity, (void **)&client_security);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_QueryBlanket(client_security, proxy, authn_service, authz_service, servername,
                authn_level, imp_level, auth_info, capabilities);
        IClientSecurity_Release(client_security);
    }

    if (FAILED(hr)) ERR("-- failed with %#lx.\n", hr);
    return hr;
}

/******************************************************************************
 *            CoSetProxyBlanket        (combase.@)
 */
HRESULT WINAPI CoSetProxyBlanket(IUnknown *proxy, DWORD authn_service, DWORD authz_service,
        OLECHAR *servername, DWORD authn_level, DWORD imp_level, void *auth_info, DWORD capabilities)
{
    IClientSecurity *client_security;
    HRESULT hr;

    TRACE("%p, %lu, %lu, %p, %lu, %lu, %p, %#lx.\n", proxy, authn_service, authz_service, servername,
            authn_level, imp_level, auth_info, capabilities);

    hr = IUnknown_QueryInterface(proxy, &IID_IClientSecurity, (void **)&client_security);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_SetBlanket(client_security, proxy, authn_service, authz_service, servername, authn_level,
                imp_level, auth_info, capabilities);
        IClientSecurity_Release(client_security);
    }

    if (FAILED(hr)) ERR("-- failed with %#lx.\n", hr);
    return hr;
}

/***********************************************************************
 *           CoCopyProxy        (combase.@)
 */
HRESULT WINAPI CoCopyProxy(IUnknown *proxy, IUnknown **proxy_copy)
{
    IClientSecurity *client_security;
    HRESULT hr;

    TRACE("%p, %p.\n", proxy, proxy_copy);

    hr = IUnknown_QueryInterface(proxy, &IID_IClientSecurity, (void **)&client_security);
    if (SUCCEEDED(hr))
    {
        hr = IClientSecurity_CopyProxy(client_security, proxy, proxy_copy);
        IClientSecurity_Release(client_security);
    }

    if (FAILED(hr)) ERR("-- failed with %#lx.\n", hr);
    return hr;
}

/***********************************************************************
 *           CoQueryClientBlanket        (combase.@)
 */
HRESULT WINAPI CoQueryClientBlanket(DWORD *authn_service, DWORD *authz_service, OLECHAR **servername,
        DWORD *authn_level, DWORD *imp_level, RPC_AUTHZ_HANDLE *privs, DWORD *capabilities)
{
    IServerSecurity *server_security;
    HRESULT hr;

    TRACE("%p, %p, %p, %p, %p, %p, %p.\n", authn_service, authz_service, servername, authn_level, imp_level,
            privs, capabilities);

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&server_security);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_QueryBlanket(server_security, authn_service, authz_service, servername, authn_level,
                imp_level, privs, capabilities);
        IServerSecurity_Release(server_security);
    }

    return hr;
}

/***********************************************************************
 *           CoImpersonateClient        (combase.@)
 */
HRESULT WINAPI CoImpersonateClient(void)
{
    IServerSecurity *server_security;
    HRESULT hr;

    TRACE("\n");

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&server_security);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_ImpersonateClient(server_security);
        IServerSecurity_Release(server_security);
    }

    return hr;
}

/***********************************************************************
 *           CoRevertToSelf        (combase.@)
 */
HRESULT WINAPI CoRevertToSelf(void)
{
    IServerSecurity *server_security;
    HRESULT hr;

    TRACE("\n");

    hr = CoGetCallContext(&IID_IServerSecurity, (void **)&server_security);
    if (SUCCEEDED(hr))
    {
        hr = IServerSecurity_RevertToSelf(server_security);
        IServerSecurity_Release(server_security);
    }

    return hr;
}

/***********************************************************************
 *           CoInitializeSecurity    (combase.@)
 */
HRESULT WINAPI CoInitializeSecurity(PSECURITY_DESCRIPTOR sd, LONG cAuthSvc,
        SOLE_AUTHENTICATION_SERVICE *asAuthSvc, void *reserved1, DWORD authn_level,
        DWORD imp_level, void *reserved2, DWORD capabilities, void *reserved3)
{
    FIXME("%p, %ld, %p, %p, %ld, %ld, %p, %ld, %p stub\n", sd, cAuthSvc, asAuthSvc, reserved1, authn_level,
            imp_level, reserved2, capabilities, reserved3);

    return S_OK;
}

/***********************************************************************
 *           CoGetObjectContext    (combase.@)
 */
HRESULT WINAPI CoGetObjectContext(REFIID riid, void **ppv)
{
    IObjContext *context;
    HRESULT hr;

    TRACE("%s, %p.\n", debugstr_guid(riid), ppv);

    *ppv = NULL;
    hr = CoGetContextToken((ULONG_PTR *)&context);
    if (FAILED(hr))
        return hr;

    return IObjContext_QueryInterface(context, riid, ppv);
}

/***********************************************************************
 *           CoGetDefaultContext    (combase.@)
 */
HRESULT WINAPI CoGetDefaultContext(APTTYPE type, REFIID riid, void **obj)
{
    FIXME("%d, %s, %p stub\n", type, debugstr_guid(riid), obj);

    return E_NOINTERFACE;
}

/***********************************************************************
 *          CoGetCallState        (combase.@)
 */
HRESULT WINAPI CoGetCallState(int arg1, ULONG *arg2)
{
    FIXME("%d, %p.\n", arg1, arg2);

    return E_NOTIMPL;
}

/***********************************************************************
 *          CoGetActivationState    (combase.@)
 */
HRESULT WINAPI CoGetActivationState(GUID guid, DWORD arg2, DWORD *arg3)
{
    FIXME("%s, %lx, %p.\n", debugstr_guid(&guid), arg2, arg3);

    return E_NOTIMPL;
}

/******************************************************************************
 *          CoGetTreatAsClass       (combase.@)
 */
HRESULT WINAPI CoGetTreatAsClass(REFCLSID clsidOld, CLSID *clsidNew)
{
    WCHAR buffW[CHARS_IN_GUID];
    LONG len = sizeof(buffW);
    HRESULT hr = S_OK;
    HKEY hkey = NULL;

    TRACE("%s, %p.\n", debugstr_guid(clsidOld), clsidNew);

    if (!clsidOld || !clsidNew)
        return E_INVALIDARG;

    *clsidNew = *clsidOld;

    hr = open_key_for_clsid(clsidOld, L"TreatAs", KEY_READ, &hkey);
    if (FAILED(hr))
    {
        hr = S_FALSE;
        goto done;
    }

    if (RegQueryValueW(hkey, NULL, buffW, &len))
    {
        hr = S_FALSE;
        goto done;
    }

    hr = CLSIDFromString(buffW, clsidNew);
    if (FAILED(hr))
        ERR("Failed to get CLSID from string %s, hr %#lx.\n", debugstr_w(buffW), hr);
done:
    if (hkey) RegCloseKey(hkey);
    return hr;
}

/******************************************************************************
 *               ProgIDFromCLSID        (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH ProgIDFromCLSID(REFCLSID clsid, LPOLESTR *progid)
{
    ACTCTX_SECTION_KEYED_DATA data;
    LONG progidlen = 0;
    HKEY hkey;
    REGSAM opposite = (sizeof(void *) > sizeof(int)) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    BOOL is_wow64;
    HRESULT hr;

    if (!progid)
        return E_INVALIDARG;

    *progid = NULL;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION,
            clsid, &data))
    {
        struct comclassredirect_data *comclass = (struct comclassredirect_data *)data.lpData;
        if (comclass->progid_len)
        {
            WCHAR *ptrW;

            *progid = CoTaskMemAlloc(comclass->progid_len + sizeof(WCHAR));
            if (!*progid) return E_OUTOFMEMORY;

            ptrW = (WCHAR *)((BYTE *)comclass + comclass->progid_offset);
            memcpy(*progid, ptrW, comclass->progid_len + sizeof(WCHAR));
            return S_OK;
        }
        else
            return REGDB_E_CLASSNOTREG;
    }

    hr = open_key_for_clsid(clsid, L"ProgID", KEY_READ, &hkey);
    if (FAILED(hr) && (opposite == KEY_WOW64_32KEY || (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
    {
        hr = open_key_for_clsid(clsid, L"ProgID", opposite | KEY_READ, &hkey);
        if (FAILED(hr))
            return hr;
    }

    if (RegQueryValueW(hkey, NULL, NULL, &progidlen))
        hr = REGDB_E_CLASSNOTREG;

    if (hr == S_OK)
    {
        *progid = CoTaskMemAlloc(progidlen * sizeof(WCHAR));
        if (*progid)
        {
            if (RegQueryValueW(hkey, NULL, *progid, &progidlen))
            {
                hr = REGDB_E_CLASSNOTREG;
                CoTaskMemFree(*progid);
                *progid = NULL;
            }
        }
        else
            hr = E_OUTOFMEMORY;
    }

    RegCloseKey(hkey);
    return hr;
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

static BOOL guid_from_string(LPCWSTR s, GUID *id)
{
    int i;

    if (!s || s[0] != '{')
    {
        memset(id, 0, sizeof(*id));
        if (!s) return TRUE;
        return FALSE;
    }

    TRACE("%s -> %p\n", debugstr_w(s), id);

    /* In form {XXXXXXXX-XXXX-XXXX-XXXX-XXXXXXXXXXXX} */

    id->Data1 = 0;
    for (i = 1; i < 9; ++i)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data1 = (id->Data1 << 4) | guid_conv_table[s[i]];
    }
    if (s[9] != '-') return FALSE;

    id->Data2 = 0;
    for (i = 10; i < 14; ++i)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data2 = (id->Data2 << 4) | guid_conv_table[s[i]];
    }
    if (s[14] != '-') return FALSE;

    id->Data3 = 0;
    for (i = 15; i < 19; ++i)
    {
        if (!is_valid_hex(s[i])) return FALSE;
        id->Data3 = (id->Data3 << 4) | guid_conv_table[s[i]];
    }
    if (s[19] != '-') return FALSE;

    for (i = 20; i < 37; i += 2)
    {
        if (i == 24)
        {
            if (s[i] != '-') return FALSE;
            i++;
        }
        if (!is_valid_hex(s[i]) || !is_valid_hex(s[i + 1])) return FALSE;
        id->Data4[(i - 20) / 2] = guid_conv_table[s[i]] << 4 | guid_conv_table[s[i + 1]];
    }

    if (s[37] == '}' && s[38] == '\0')
        return TRUE;

    return FALSE;
}

static HRESULT clsid_from_string_reg(LPCOLESTR progid, CLSID *clsid)
{
    WCHAR buf2[CHARS_IN_GUID];
    LONG buf2len = sizeof(buf2);
    HKEY xhkey;
    WCHAR *buf;

    memset(clsid, 0, sizeof(*clsid));
    buf = malloc((lstrlenW(progid) + 8) * sizeof(WCHAR));
    if (!buf) return E_OUTOFMEMORY;

    lstrcpyW(buf, progid);
    lstrcatW(buf, L"\\CLSID");
    if (open_classes_key(HKEY_CLASSES_ROOT, buf, MAXIMUM_ALLOWED, &xhkey))
    {
        free(buf);
        WARN("couldn't open key for ProgID %s\n", debugstr_w(progid));
        return CO_E_CLASSSTRING;
    }
    free(buf);

    if (RegQueryValueW(xhkey, NULL, buf2, &buf2len))
    {
        RegCloseKey(xhkey);
        WARN("couldn't query clsid value for ProgID %s\n", debugstr_w(progid));
        return CO_E_CLASSSTRING;
    }
    RegCloseKey(xhkey);
    return guid_from_string(buf2, clsid) ? S_OK : CO_E_CLASSSTRING;
}

/******************************************************************************
 *                CLSIDFromProgID        (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CLSIDFromProgID(LPCOLESTR progid, CLSID *clsid)
{
    ACTCTX_SECTION_KEYED_DATA data;

    if (!progid || !clsid)
        return E_INVALIDARG;

    data.cbSize = sizeof(data);
    if (FindActCtxSectionStringW(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_PROGID_REDIRECTION,
            progid, &data))
    {
        struct progidredirect_data *progiddata = (struct progidredirect_data *)data.lpData;
        CLSID *alias = (CLSID *)((BYTE *)data.lpSectionBase + progiddata->clsid_offset);
        *clsid = *alias;
        return S_OK;
    }

    return clsid_from_string_reg(progid, clsid);
}

/******************************************************************************
 *              CLSIDFromProgIDEx        (combase.@)
 */
HRESULT WINAPI CLSIDFromProgIDEx(LPCOLESTR progid, CLSID *clsid)
{
    FIXME("%s, %p: semi-stub\n", debugstr_w(progid), clsid);

    return CLSIDFromProgID(progid, clsid);
}

/******************************************************************************
 *                CLSIDFromString        (combase.@)
 */
HRESULT WINAPI CLSIDFromString(LPCOLESTR str, LPCLSID clsid)
{
    CLSID tmp_id;
    HRESULT hr;

    if (!clsid)
        return E_INVALIDARG;

    if (guid_from_string(str, clsid))
        return S_OK;

    /* It appears a ProgID is also valid */
    hr = clsid_from_string_reg(str, &tmp_id);
    if (SUCCEEDED(hr))
        *clsid = tmp_id;

    return hr;
}

/******************************************************************************
 *                IIDFromString        (combase.@)
 */
HRESULT WINAPI IIDFromString(LPCOLESTR str, IID *iid)
{
    TRACE("%s, %p\n", debugstr_w(str), iid);

    if (!str)
    {
        memset(iid, 0, sizeof(*iid));
        return S_OK;
    }

    /* length mismatch is a special case */
    if (lstrlenW(str) + 1 != CHARS_IN_GUID)
        return E_INVALIDARG;

    if (str[0] != '{')
        return CO_E_IIDSTRING;

    return guid_from_string(str, iid) ? S_OK : CO_E_IIDSTRING;
}

/******************************************************************************
 *            StringFromCLSID        (combase.@)
 */
HRESULT WINAPI StringFromCLSID(REFCLSID clsid, LPOLESTR *str)
{
    if (!(*str = CoTaskMemAlloc(CHARS_IN_GUID * sizeof(WCHAR)))) return E_OUTOFMEMORY;
    StringFromGUID2(clsid, *str, CHARS_IN_GUID);
    return S_OK;
}

/******************************************************************************
 *            StringFromGUID2        (combase.@)
 */
INT WINAPI StringFromGUID2(REFGUID guid, LPOLESTR str, INT cmax)
{
    if (!guid || cmax < CHARS_IN_GUID) return 0;
    swprintf(str, CHARS_IN_GUID, L"{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", guid->Data1,
            guid->Data2, guid->Data3, guid->Data4[0], guid->Data4[1], guid->Data4[2], guid->Data4[3],
            guid->Data4[4], guid->Data4[5], guid->Data4[6], guid->Data4[7]);
    return CHARS_IN_GUID;
}

static void init_multi_qi(DWORD count, MULTI_QI *mqi, HRESULT hr)
{
    ULONG i;

    for (i = 0; i < count; i++)
    {
        mqi[i].pItf = NULL;
        mqi[i].hr = hr;
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
        mqi[index].hr = IUnknown_QueryInterface(unk, mqi[index].pIID, (void **)&mqi[index].pItf);
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
 *          CoGetInstanceFromFile    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoGetInstanceFromFile(COSERVERINFO *server_info, CLSID *rclsid,
        IUnknown *outer, DWORD cls_context, DWORD grfmode, OLECHAR *filename, DWORD count,
        MULTI_QI *results)
{
    IPersistFile *pf = NULL;
    IUnknown *obj = NULL;
    CLSID clsid;
    HRESULT hr;

    if (!count || !results)
        return E_INVALIDARG;

    if (server_info)
        FIXME("() non-NULL server_info not supported\n");

    init_multi_qi(count, results, E_NOINTERFACE);

    if (!rclsid)
    {
        hr = GetClassFile(filename, &clsid);
        if (FAILED(hr))
        {
            ERR("Failed to get CLSID from a file.\n");
            return hr;
        }

        rclsid = &clsid;
    }

    hr = CoCreateInstance(rclsid, outer, cls_context, &IID_IUnknown, (void **)&obj);
    if (hr != S_OK)
    {
        init_multi_qi(count, results, hr);
        return hr;
    }

    /* Init from file */
    hr = IUnknown_QueryInterface(obj, &IID_IPersistFile, (void **)&pf);
    if (FAILED(hr))
    {
        init_multi_qi(count, results, hr);
        IUnknown_Release(obj);
        return hr;
    }

    hr = IPersistFile_Load(pf, filename, grfmode);
    IPersistFile_Release(pf);
    if (SUCCEEDED(hr))
        return return_multi_qi(obj, count, results, FALSE);
    else
    {
        init_multi_qi(count, results, hr);
        IUnknown_Release(obj);
        return hr;
    }
}

/***********************************************************************
 *           CoGetInstanceFromIStorage        (combase.@)
 */
HRESULT WINAPI CoGetInstanceFromIStorage(COSERVERINFO *server_info, CLSID *rclsid,
        IUnknown *outer, DWORD cls_context, IStorage *storage, DWORD count, MULTI_QI *results)
{
    IPersistStorage *ps = NULL;
    IUnknown *obj = NULL;
    STATSTG stat;
    HRESULT hr;

    if (!count || !results || !storage)
        return E_INVALIDARG;

    if (server_info)
        FIXME("() non-NULL server_info not supported\n");

    init_multi_qi(count, results, E_NOINTERFACE);

    if (!rclsid)
    {
        memset(&stat.clsid, 0, sizeof(stat.clsid));
        hr = IStorage_Stat(storage, &stat, STATFLAG_NONAME);
        if (FAILED(hr))
        {
            ERR("Failed to get CLSID from a storage.\n");
            return hr;
        }

        rclsid = &stat.clsid;
    }

    hr = CoCreateInstance(rclsid, outer, cls_context, &IID_IUnknown, (void **)&obj);
    if (hr != S_OK)
        return hr;

    /* Init from IStorage */
    hr = IUnknown_QueryInterface(obj, &IID_IPersistStorage, (void **)&ps);
    if (FAILED(hr))
        ERR("failed to get IPersistStorage\n");

    if (ps)
    {
        IPersistStorage_Load(ps, storage);
        IPersistStorage_Release(ps);
    }

    return return_multi_qi(obj, count, results, FALSE);
}

/***********************************************************************
 *           CoCreateInstance        (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoCreateInstance(REFCLSID rclsid, IUnknown *outer, DWORD cls_context,
        REFIID riid, void **obj)
{
    MULTI_QI multi_qi = { .pIID = riid };
    HRESULT hr;

    TRACE("%s, %p, %#lx, %s, %p.\n", debugstr_guid(rclsid), outer, cls_context, debugstr_guid(riid), obj);

    if (!obj)
        return E_POINTER;

    hr = CoCreateInstanceEx(rclsid, outer, cls_context, NULL, 1, &multi_qi);
    *obj = multi_qi.pItf;
    return hr;
}

/***********************************************************************
 *           CoCreateInstanceFromApp    (combase.@)
 */
HRESULT WINAPI CoCreateInstanceFromApp(REFCLSID rclsid, IUnknown *outer, DWORD cls_context,
        void *server_info, ULONG count, MULTI_QI *results)
{
    TRACE("%s, %p, %#lx, %p, %lu, %p\n", debugstr_guid(rclsid), outer, cls_context, server_info,
            count, results);

    return CoCreateInstanceEx(rclsid, outer, cls_context | CLSCTX_APPCONTAINER, server_info,
            count, results);
}

static HRESULT com_get_class_object(REFCLSID rclsid, DWORD clscontext,
        COSERVERINFO *server_info, REFIID riid, void **obj)
{
    struct class_reg_data clsreg = { 0 };
    HRESULT hr = E_UNEXPECTED;
    IUnknown *registered_obj;
    struct apartment *apt;

    if (!obj)
        return E_INVALIDARG;

    *obj = NULL;

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (server_info)
        FIXME("server_info name %s, authinfo %p\n", debugstr_w(server_info->pwszName), server_info->pAuthInfo);

    if (clscontext & CLSCTX_INPROC_SERVER)
    {
        if (IsEqualCLSID(rclsid, &CLSID_InProcFreeMarshaler) ||
                IsEqualCLSID(rclsid, &CLSID_GlobalOptions) ||
                (!(clscontext & CLSCTX_APPCONTAINER) && IsEqualCLSID(rclsid, &CLSID_ManualResetEvent)) ||
                IsEqualCLSID(rclsid, &CLSID_StdGlobalInterfaceTable))
        {
            apartment_release(apt);

            if (IsEqualCLSID(rclsid, &CLSID_GlobalOptions))
                return get_builtin_class_factory(rclsid, riid, obj);
            else
                return Ole32DllGetClassObject(rclsid, riid, obj);
        }
    }

    if (clscontext & CLSCTX_INPROC)
    {
        ACTCTX_SECTION_KEYED_DATA data;

        data.cbSize = sizeof(data);
        /* search activation context first */
        if (FindActCtxSectionGuid(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, NULL,
                ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, rclsid, &data))
        {
            struct comclassredirect_data *comclass = (struct comclassredirect_data *)data.lpData;

            clsreg.u.actctx.module_name = (WCHAR *)((BYTE *)data.lpSectionBase + comclass->name_offset);
            clsreg.u.actctx.hactctx = data.hActCtx;
            clsreg.u.actctx.threading_model = comclass->model;
            clsreg.origin = CLASS_REG_ACTCTX;

            hr = apartment_get_inproc_class_object(apt, &clsreg, &comclass->clsid, riid, clscontext, obj);
            ReleaseActCtx(data.hActCtx);
            apartment_release(apt);
            return hr;
        }
    }

    /*
     * First, try and see if we can't match the class ID with one of the
     * registered classes.
     */
    if (!(clscontext & CLSCTX_APPCONTAINER) && (registered_obj = com_get_registered_class_object(apt, rclsid, clscontext)))
    {
        hr = IUnknown_QueryInterface(registered_obj, riid, obj);
        IUnknown_Release(registered_obj);
        apartment_release(apt);
        return hr;
    }

    /* First try in-process server */
    if (clscontext & CLSCTX_INPROC_SERVER)
    {
        HKEY hkey;

        hr = open_key_for_clsid(rclsid, L"InprocServer32", KEY_READ, &hkey);
        if (FAILED(hr))
        {
            if (hr == REGDB_E_CLASSNOTREG)
                ERR("class %s not registered\n", debugstr_guid(rclsid));
            else if (hr == REGDB_E_KEYMISSING)
            {
                WARN("class %s not registered as in-proc server\n", debugstr_guid(rclsid));
                hr = REGDB_E_CLASSNOTREG;
            }
        }

        if (SUCCEEDED(hr))
        {
            clsreg.u.hkey = hkey;
            clsreg.origin = CLASS_REG_REGISTRY;

            hr = apartment_get_inproc_class_object(apt, &clsreg, rclsid, riid, clscontext, obj);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hr))
        {
            apartment_release(apt);
            return hr;
        }
    }

    /* Next try in-process handler */
    if (clscontext & CLSCTX_INPROC_HANDLER)
    {
        HKEY hkey;

        hr = open_key_for_clsid(rclsid, L"InprocHandler32", KEY_READ, &hkey);
        if (FAILED(hr))
        {
            if (hr == REGDB_E_CLASSNOTREG)
                ERR("class %s not registered\n", debugstr_guid(rclsid));
            else if (hr == REGDB_E_KEYMISSING)
            {
                WARN("class %s not registered in-proc handler\n", debugstr_guid(rclsid));
                hr = REGDB_E_CLASSNOTREG;
            }
        }

        if (SUCCEEDED(hr))
        {
            clsreg.u.hkey = hkey;
            clsreg.origin = CLASS_REG_REGISTRY;

            hr = apartment_get_inproc_class_object(apt, &clsreg, rclsid, riid, clscontext, obj);
            RegCloseKey(hkey);
        }

        /* return if we got a class, otherwise fall through to one of the
         * other types */
        if (SUCCEEDED(hr))
        {
            apartment_release(apt);
            return hr;
        }
    }
    apartment_release(apt);

    /* Next try out of process */
    if (clscontext & CLSCTX_LOCAL_SERVER)
    {
        hr = rpc_get_local_class_object(rclsid, riid, obj);
        if (SUCCEEDED(hr))
            return hr;
    }

    /* Finally try remote: this requires networked DCOM (a lot of work) */
    if (clscontext & CLSCTX_REMOTE_SERVER)
    {
        FIXME ("CLSCTX_REMOTE_SERVER not supported\n");
        hr = REGDB_E_CLASSNOTREG;
    }

    if (FAILED(hr))
        ERR("no class object %s could be created for context %#lx\n", debugstr_guid(rclsid), clscontext);

    return hr;
}

/***********************************************************************
 *           CoCreateInstanceEx    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoCreateInstanceEx(REFCLSID rclsid, IUnknown *outer, DWORD cls_context,
        COSERVERINFO *server_info, ULONG count, MULTI_QI *results)
{
    IClassFactory *factory;
    IUnknown *unk = NULL;
    CLSID clsid;
    HRESULT hr;

    TRACE("%s, %p, %#lx, %p, %lu, %p\n", debugstr_guid(rclsid), outer, cls_context, server_info, count, results);

    if (!count || !results)
        return E_INVALIDARG;

    if (server_info)
        FIXME("Server info is not supported.\n");

    init_multi_qi(count, results, E_NOINTERFACE);

    clsid = *rclsid;
    if (!(cls_context & CLSCTX_APPCONTAINER))
        CoGetTreatAsClass(rclsid, &clsid);

    if (FAILED(hr = com_get_class_object(&clsid, cls_context, NULL, &IID_IClassFactory, (void **)&factory)))
        return hr;

    hr = IClassFactory_CreateInstance(factory, outer, results[0].pIID, (void **)&unk);
    IClassFactory_Release(factory);
    if (FAILED(hr))
    {
        if (hr == CLASS_E_NOAGGREGATION && outer)
            FIXME("Class %s does not support aggregation\n", debugstr_guid(&clsid));
        else
            FIXME("no instance created for interface %s of class %s, hr %#lx.\n",
                    debugstr_guid(results[0].pIID), debugstr_guid(&clsid), hr);
        return hr;
    }

    return return_multi_qi(unk, count, results, TRUE);
}

/***********************************************************************
 *           CoGetClassObject    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoGetClassObject(REFCLSID rclsid, DWORD clscontext,
        COSERVERINFO *server_info, REFIID riid, void **obj)
{
    TRACE("%s, %#lx, %s\n", debugstr_guid(rclsid), clscontext, debugstr_guid(riid));

    return com_get_class_object(rclsid, clscontext, server_info, riid, obj);
}

/***********************************************************************
 *           CoFreeUnusedLibraries    (combase.@)
 */
void WINAPI DECLSPEC_HOTPATCH CoFreeUnusedLibraries(void)
{
    CoFreeUnusedLibrariesEx(INFINITE, 0);
}

/***********************************************************************
 *           CoGetCallContext        (combase.@)
 */
HRESULT WINAPI CoGetCallContext(REFIID riid, void **obj)
{
    struct tlsdata *tlsdata;
    HRESULT hr;

    TRACE("%s, %p\n", debugstr_guid(riid), obj);

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (!tlsdata->call_state)
        return RPC_E_CALL_COMPLETE;

    return IUnknown_QueryInterface(tlsdata->call_state, riid, obj);
}

/***********************************************************************
 *           CoSwitchCallContext    (combase.@)
 */
HRESULT WINAPI CoSwitchCallContext(IUnknown *context, IUnknown **old_context)
{
    struct tlsdata *tlsdata;
    HRESULT hr;

    TRACE("%p, %p\n", context, old_context);

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    /* Reference counts are not touched. */
    *old_context = tlsdata->call_state;
    tlsdata->call_state = context;

    return S_OK;
}

/******************************************************************************
 *          CoRegisterInitializeSpy    (combase.@)
 */
HRESULT WINAPI CoRegisterInitializeSpy(IInitializeSpy *spy, ULARGE_INTEGER *cookie)
{
    struct tlsdata *tlsdata;
    struct init_spy *entry;
    unsigned int id;
    HRESULT hr;

    TRACE("%p, %p\n", spy, cookie);

    if (!spy || !cookie)
        return E_INVALIDARG;

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    hr = IInitializeSpy_QueryInterface(spy, &IID_IInitializeSpy, (void **)&spy);
    if (FAILED(hr))
        return hr;

    entry = malloc(sizeof(*entry));
    if (!entry)
    {
        IInitializeSpy_Release(spy);
        return E_OUTOFMEMORY;
    }

    entry->spy = spy;

    id = 0;
    while (get_spy_entry(tlsdata, id) != NULL)
    {
        id++;
    }

    entry->id = id;
    list_add_head(&tlsdata->spies, &entry->entry);

    cookie->u.HighPart = GetCurrentThreadId();
    cookie->u.LowPart = entry->id;

    return S_OK;
}

/******************************************************************************
 *          CoRevokeInitializeSpy    (combase.@)
 */
HRESULT WINAPI CoRevokeInitializeSpy(ULARGE_INTEGER cookie)
{
    struct tlsdata *tlsdata;
    struct init_spy *spy;
    HRESULT hr;

    TRACE("%s\n", wine_dbgstr_longlong(cookie.QuadPart));

    if (cookie.u.HighPart != GetCurrentThreadId())
        return E_INVALIDARG;

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (!(spy = get_spy_entry(tlsdata, cookie.u.LowPart))) return E_INVALIDARG;

    IInitializeSpy_Release(spy->spy);
    spy->spy = NULL;
    if (!tlsdata->spies_lock)
    {
        list_remove(&spy->entry);
        free(spy);
    }
    return S_OK;
}

static BOOL com_peek_message(struct apartment *apt, MSG *msg)
{
    /* First try to retrieve messages for incoming COM calls to the apartment window */
    return (apt->win && PeekMessageW(msg, apt->win, 0, 0, PM_REMOVE | PM_NOYIELD)) ||
            /* Next retrieve other messages necessary for the app to remain responsive */
            PeekMessageW(msg, NULL, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE | PM_NOYIELD) ||
            PeekMessageW(msg, NULL, 0, 0, PM_QS_PAINT | PM_QS_SENDMESSAGE | PM_REMOVE | PM_NOYIELD);
}

/***********************************************************************
 *           CoWaitForMultipleHandles    (combase.@)
 */
HRESULT WINAPI CoWaitForMultipleHandles(DWORD flags, DWORD timeout, ULONG handle_count, HANDLE *handles,
        DWORD *index)
{
    BOOL check_apc = !!(flags & COWAIT_ALERTABLE), message_loop;
    struct { BOOL post; UINT code; } quit = { .post = FALSE };
    DWORD start_time, wait_flags = 0;
    struct tlsdata *tlsdata;
    struct apartment *apt;
    HRESULT hr;

    TRACE("%#lx, %#lx, %lu, %p, %p\n", flags, timeout, handle_count, handles, index);

    if (!index)
        return E_INVALIDARG;

    *index = 0;

    if (!handles)
        return E_INVALIDARG;

    if (!handle_count)
        return RPC_E_NO_SYNC;

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    apt = com_get_current_apt();
    message_loop = apt && !apt->multi_threaded;

    if (flags & COWAIT_WAITALL)
        wait_flags |= MWMO_WAITALL;
    if (flags & COWAIT_ALERTABLE)
        wait_flags |= MWMO_ALERTABLE;

    start_time = GetTickCount();

    while (TRUE)
    {
        DWORD now = GetTickCount(), res;

        if (now - start_time > timeout)
        {
            hr = RPC_S_CALLPENDING;
            break;
        }

        if (message_loop)
        {
            TRACE("waiting for rpc completion or window message\n");

            res = WAIT_TIMEOUT;

            if (check_apc)
            {
                res = WaitForMultipleObjectsEx(handle_count, handles, !!(flags & COWAIT_WAITALL), 0, TRUE);
                check_apc = FALSE;
            }

            if (res == WAIT_TIMEOUT)
                res = MsgWaitForMultipleObjectsEx(handle_count, handles,
                        timeout == INFINITE ? INFINITE : start_time + timeout - now,
                        QS_SENDMESSAGE | QS_ALLPOSTMESSAGE | QS_PAINT, wait_flags);

            if (res == WAIT_OBJECT_0 + handle_count)  /* messages available */
            {
                int msg_count = 0;
                MSG msg;

                /* call message filter */

                if (apt->filter)
                {
                    PENDINGTYPE pendingtype = tlsdata->pending_call_count_server ? PENDINGTYPE_NESTED : PENDINGTYPE_TOPLEVEL;
                    DWORD be_handled = IMessageFilter_MessagePending(apt->filter, 0 /* FIXME */, now - start_time, pendingtype);

                    TRACE("IMessageFilter_MessagePending returned %ld\n", be_handled);

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

                if (!apt->win)
                {
                    /* If window is NULL on apartment, peek at messages so that it will not trigger
                     * MsgWaitForMultipleObjects next time. */
                    PeekMessageW(NULL, NULL, 0, 0, PM_QS_POSTMESSAGE | PM_NOREMOVE | PM_NOYIELD);
                }

                /* Some apps (e.g. Visio 2010) don't handle WM_PAINT properly and loop forever,
                 * so after processing 100 messages we go back to checking the wait handles */
                while (msg_count++ < 100 && com_peek_message(apt, &msg))
                {
                    if (msg.message == WM_QUIT)
                    {
                        TRACE("Received WM_QUIT message\n");
                        quit.post = TRUE;
                        quit.code = msg.wParam;
                    }
                    else
                    {
                        TRACE("Received message whilst waiting for RPC: 0x%04x\n", msg.message);
                        TranslateMessage(&msg);
                        DispatchMessageW(&msg);
                    }
                }
                continue;
            }
        }
        else
        {
            TRACE("Waiting for rpc completion\n");

            res = WaitForMultipleObjectsEx(handle_count, handles, !!(flags & COWAIT_WAITALL),
                    (timeout == INFINITE) ? INFINITE : start_time + timeout - now, !!(flags & COWAIT_ALERTABLE));
        }

        switch (res)
        {
        case WAIT_TIMEOUT:
            hr = RPC_S_CALLPENDING;
            break;
        case WAIT_FAILED:
            hr = HRESULT_FROM_WIN32(GetLastError());
            break;
        default:
            *index = res;
            break;
        }
        break;
    }
    if (quit.post) PostQuitMessage(quit.code);

    TRACE("-- %#lx\n", hr);

    return hr;
}

/******************************************************************************
 *            CoRegisterMessageFilter        (combase.@)
 */
HRESULT WINAPI CoRegisterMessageFilter(IMessageFilter *filter, IMessageFilter **ret_filter)
{
    IMessageFilter *old_filter;
    struct apartment *apt;

    TRACE("%p, %p\n", filter, ret_filter);

    apt = com_get_current_apt();

    /* Can't set a message filter in a multi-threaded apartment */
    if (!apt || apt->multi_threaded)
    {
        WARN("Can't set message filter in MTA or uninitialized apt\n");
        return CO_E_NOT_SUPPORTED;
    }

    if (filter)
        IMessageFilter_AddRef(filter);

    EnterCriticalSection(&apt->cs);

    old_filter = apt->filter;
    apt->filter = filter;

    LeaveCriticalSection(&apt->cs);

    if (ret_filter)
        *ret_filter = old_filter;
    else if (old_filter)
        IMessageFilter_Release(old_filter);

    return S_OK;
}

static void com_revoke_all_ps_clsids(void)
{
    struct registered_ps *cur, *cur2;

    EnterCriticalSection(&cs_registered_ps);

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &registered_proxystubs, struct registered_ps, entry)
    {
        list_remove(&cur->entry);
        free(cur);
    }

    LeaveCriticalSection(&cs_registered_ps);
}

static HRESULT get_ps_clsid_from_registry(const WCHAR* path, REGSAM access, CLSID *pclsid)
{
    WCHAR value[CHARS_IN_GUID];
    HKEY hkey;
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
 *             CoGetPSClsid        (combase.@)
 */
HRESULT WINAPI CoGetPSClsid(REFIID riid, CLSID *pclsid)
{
    static const WCHAR interfaceW[] = L"Interface\\";
    static const WCHAR psW[] = L"\\ProxyStubClsid32";
    WCHAR path[ARRAY_SIZE(interfaceW) - 1 + CHARS_IN_GUID - 1 + ARRAY_SIZE(psW)];
    ACTCTX_SECTION_KEYED_DATA data;
    struct registered_ps *cur;
    REGSAM opposite = (sizeof(void*) > sizeof(int)) ? KEY_WOW64_32KEY : KEY_WOW64_64KEY;
    BOOL is_wow64;
    HRESULT hr;

    TRACE("%s, %p\n", debugstr_guid(riid), pclsid);

    if (!InternalIsProcessInitialized())
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (!pclsid)
        return E_INVALIDARG;

    EnterCriticalSection(&cs_registered_ps);

    LIST_FOR_EACH_ENTRY(cur, &registered_proxystubs, struct registered_ps, entry)
    {
        if (IsEqualIID(&cur->iid, riid))
        {
            *pclsid = cur->clsid;
            LeaveCriticalSection(&cs_registered_ps);
            return S_OK;
        }
    }

    LeaveCriticalSection(&cs_registered_ps);

    data.cbSize = sizeof(data);
    if (FindActCtxSectionGuid(0, NULL, ACTIVATION_CONTEXT_SECTION_COM_INTERFACE_REDIRECTION,
            riid, &data))
    {
        struct ifacepsredirect_data *ifaceps = (struct ifacepsredirect_data *)data.lpData;
        *pclsid = ifaceps->iid;
        return S_OK;
    }

    /* Interface\\{string form of riid}\\ProxyStubClsid32 */
    lstrcpyW(path, interfaceW);
    StringFromGUID2(riid, path + ARRAY_SIZE(interfaceW) - 1, CHARS_IN_GUID);
    lstrcpyW(path + ARRAY_SIZE(interfaceW) - 1 + CHARS_IN_GUID - 1, psW);

    hr = get_ps_clsid_from_registry(path, KEY_READ, pclsid);
    if (FAILED(hr) && (opposite == KEY_WOW64_32KEY || (IsWow64Process(GetCurrentProcess(), &is_wow64) && is_wow64)))
        hr = get_ps_clsid_from_registry(path, opposite | KEY_READ, pclsid);

    if (hr == S_OK)
        TRACE("() Returning CLSID %s\n", debugstr_guid(pclsid));
    else
        WARN("No PSFactoryBuffer object is registered for IID %s\n", debugstr_guid(riid));

    return hr;
}

/*****************************************************************************
 *             CoRegisterPSClsid    (combase.@)
 */
HRESULT WINAPI CoRegisterPSClsid(REFIID riid, REFCLSID rclsid)
{
    struct registered_ps *cur;

    TRACE("%s, %s\n", debugstr_guid(riid), debugstr_guid(rclsid));

    if (!InternalIsProcessInitialized())
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    EnterCriticalSection(&cs_registered_ps);

    LIST_FOR_EACH_ENTRY(cur, &registered_proxystubs, struct registered_ps, entry)
    {
        if (IsEqualIID(&cur->iid, riid))
        {
            cur->clsid = *rclsid;
            LeaveCriticalSection(&cs_registered_ps);
            return S_OK;
        }
    }

    cur = malloc(sizeof(*cur));
    if (!cur)
    {
        LeaveCriticalSection(&cs_registered_ps);
        return E_OUTOFMEMORY;
    }

    cur->iid = *riid;
    cur->clsid = *rclsid;
    list_add_head(&registered_proxystubs, &cur->entry);

    LeaveCriticalSection(&cs_registered_ps);

    return S_OK;
}

struct thread_context
{
    IComThreadingInfo IComThreadingInfo_iface;
    IContextCallback IContextCallback_iface;
    IObjContext IObjContext_iface;
    LONG refcount;
};

static inline struct thread_context *impl_from_IComThreadingInfo(IComThreadingInfo *iface)
{
    return CONTAINING_RECORD(iface, struct thread_context, IComThreadingInfo_iface);
}

static inline struct thread_context *impl_from_IContextCallback(IContextCallback *iface)
{
    return CONTAINING_RECORD(iface, struct thread_context, IContextCallback_iface);
}

static inline struct thread_context *impl_from_IObjContext(IObjContext *iface)
{
    return CONTAINING_RECORD(iface, struct thread_context, IObjContext_iface);
}

static HRESULT WINAPI thread_context_info_QueryInterface(IComThreadingInfo *iface, REFIID riid, void **obj)
{
    struct thread_context *context = impl_from_IComThreadingInfo(iface);

    *obj = NULL;

    if (IsEqualIID(riid, &IID_IComThreadingInfo) ||
        IsEqualIID(riid, &IID_IUnknown))
    {
        *obj = &context->IComThreadingInfo_iface;
    }
    else if (IsEqualIID(riid, &IID_IContextCallback))
    {
        *obj = &context->IContextCallback_iface;
    }
    else if (IsEqualIID(riid, &IID_IObjContext))
    {
        *obj = &context->IObjContext_iface;
    }

    if (*obj)
    {
        IUnknown_AddRef((IUnknown *)*obj);
        return S_OK;
    }

    FIXME("interface not implemented %s\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI thread_context_info_AddRef(IComThreadingInfo *iface)
{
    struct thread_context *context = impl_from_IComThreadingInfo(iface);
    return InterlockedIncrement(&context->refcount);
}

static ULONG WINAPI thread_context_info_Release(IComThreadingInfo *iface)
{
    struct thread_context *context = impl_from_IComThreadingInfo(iface);

    /* Context instance is initially created with CoGetContextToken() with refcount set to 0,
       releasing context while refcount is at 0 destroys it. */
    if (!context->refcount)
    {
        free(context);
        return 0;
    }

    return InterlockedDecrement(&context->refcount);
}

static HRESULT WINAPI thread_context_info_GetCurrentApartmentType(IComThreadingInfo *iface, APTTYPE *apttype)
{
    APTTYPEQUALIFIER qualifier;

    TRACE("%p\n", apttype);

    return CoGetApartmentType(apttype, &qualifier);
}

static HRESULT WINAPI thread_context_info_GetCurrentThreadType(IComThreadingInfo *iface, THDTYPE *thdtype)
{
    APTTYPEQUALIFIER qualifier;
    APTTYPE apttype;
    HRESULT hr;

    hr = CoGetApartmentType(&apttype, &qualifier);
    if (FAILED(hr))
        return hr;

    TRACE("%p\n", thdtype);

    switch (apttype)
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

static HRESULT WINAPI thread_context_info_GetCurrentLogicalThreadId(IComThreadingInfo *iface, GUID *logical_thread_id)
{
    TRACE("%p\n", logical_thread_id);

    return CoGetCurrentLogicalThreadId(logical_thread_id);
}

static HRESULT WINAPI thread_context_info_SetCurrentLogicalThreadId(IComThreadingInfo *iface, REFGUID logical_thread_id)
{
    FIXME("%s stub\n", debugstr_guid(logical_thread_id));

    return E_NOTIMPL;
}

static const IComThreadingInfoVtbl thread_context_info_vtbl =
{
    thread_context_info_QueryInterface,
    thread_context_info_AddRef,
    thread_context_info_Release,
    thread_context_info_GetCurrentApartmentType,
    thread_context_info_GetCurrentThreadType,
    thread_context_info_GetCurrentLogicalThreadId,
    thread_context_info_SetCurrentLogicalThreadId
};

static HRESULT WINAPI thread_context_callback_QueryInterface(IContextCallback *iface, REFIID riid, void **obj)
{
    struct thread_context *context = impl_from_IContextCallback(iface);
    return IComThreadingInfo_QueryInterface(&context->IComThreadingInfo_iface, riid, obj);
}

static ULONG WINAPI thread_context_callback_AddRef(IContextCallback *iface)
{
    struct thread_context *context = impl_from_IContextCallback(iface);
    return IComThreadingInfo_AddRef(&context->IComThreadingInfo_iface);
}

static ULONG WINAPI thread_context_callback_Release(IContextCallback *iface)
{
    struct thread_context *context = impl_from_IContextCallback(iface);
    return IComThreadingInfo_Release(&context->IComThreadingInfo_iface);
}

static HRESULT WINAPI thread_context_callback_ContextCallback(IContextCallback *iface,
        PFNCONTEXTCALL callback, ComCallData *param, REFIID riid, int method, IUnknown *punk)
{
    FIXME("%p, %p, %p, %s, %d, %p\n", iface, callback, param, debugstr_guid(riid), method, punk);

    return E_NOTIMPL;
}

static const IContextCallbackVtbl thread_context_callback_vtbl =
{
    thread_context_callback_QueryInterface,
    thread_context_callback_AddRef,
    thread_context_callback_Release,
    thread_context_callback_ContextCallback
};

static HRESULT WINAPI thread_object_context_QueryInterface(IObjContext *iface, REFIID riid, void **obj)
{
    struct thread_context *context = impl_from_IObjContext(iface);
    return IComThreadingInfo_QueryInterface(&context->IComThreadingInfo_iface, riid, obj);
}

static ULONG WINAPI thread_object_context_AddRef(IObjContext *iface)
{
    struct thread_context *context = impl_from_IObjContext(iface);
    return IComThreadingInfo_AddRef(&context->IComThreadingInfo_iface);
}

static ULONG WINAPI thread_object_context_Release(IObjContext *iface)
{
    struct thread_context *context = impl_from_IObjContext(iface);
    return IComThreadingInfo_Release(&context->IComThreadingInfo_iface);
}

static HRESULT WINAPI thread_object_context_SetProperty(IObjContext *iface, REFGUID propid, CPFLAGS flags, IUnknown *punk)
{
    FIXME("%p, %s, %lx, %p\n", iface, debugstr_guid(propid), flags, punk);

    return E_NOTIMPL;
}

static HRESULT WINAPI thread_object_context_RemoveProperty(IObjContext *iface, REFGUID propid)
{
    FIXME("%p, %s\n", iface, debugstr_guid(propid));

    return E_NOTIMPL;
}

static HRESULT WINAPI thread_object_context_GetProperty(IObjContext *iface, REFGUID propid, CPFLAGS *flags, IUnknown **punk)
{
    FIXME("%p, %s, %p, %p\n", iface, debugstr_guid(propid), flags, punk);

    return E_NOTIMPL;
}

static HRESULT WINAPI thread_object_context_EnumContextProps(IObjContext *iface, IEnumContextProps **props)
{
    FIXME("%p, %p\n", iface, props);

    return E_NOTIMPL;
}

static void WINAPI thread_object_context_Reserved1(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static void WINAPI thread_object_context_Reserved2(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static void WINAPI thread_object_context_Reserved3(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static void WINAPI thread_object_context_Reserved4(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static void WINAPI thread_object_context_Reserved5(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static void WINAPI thread_object_context_Reserved6(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static void WINAPI thread_object_context_Reserved7(IObjContext *iface)
{
    FIXME("%p\n", iface);
}

static const IObjContextVtbl thread_object_context_vtbl =
{
    thread_object_context_QueryInterface,
    thread_object_context_AddRef,
    thread_object_context_Release,
    thread_object_context_SetProperty,
    thread_object_context_RemoveProperty,
    thread_object_context_GetProperty,
    thread_object_context_EnumContextProps,
    thread_object_context_Reserved1,
    thread_object_context_Reserved2,
    thread_object_context_Reserved3,
    thread_object_context_Reserved4,
    thread_object_context_Reserved5,
    thread_object_context_Reserved6,
    thread_object_context_Reserved7
};

/***********************************************************************
 *           CoGetContextToken    (combase.@)
 */
HRESULT WINAPI CoGetContextToken(ULONG_PTR *token)
{
    struct tlsdata *tlsdata;
    HRESULT hr;

    TRACE("%p\n", token);

    if (!InternalIsProcessInitialized())
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (!token)
        return E_POINTER;

    if (!tlsdata->context_token)
    {
        struct thread_context *context;

        context = calloc(1, sizeof(*context));
        if (!context)
            return E_OUTOFMEMORY;

        context->IComThreadingInfo_iface.lpVtbl = &thread_context_info_vtbl;
        context->IContextCallback_iface.lpVtbl = &thread_context_callback_vtbl;
        context->IObjContext_iface.lpVtbl = &thread_object_context_vtbl;
        /* Context token does not take a reference, it's always zero until the
           interface is explicitly requested with CoGetObjectContext(). */
        context->refcount = 0;

        tlsdata->context_token = &context->IObjContext_iface;
    }

    *token = (ULONG_PTR)tlsdata->context_token;
    TRACE("context_token %p\n", tlsdata->context_token);

    return S_OK;
}

/***********************************************************************
 *              CoGetCurrentLogicalThreadId    (combase.@)
 */
HRESULT WINAPI CoGetCurrentLogicalThreadId(GUID *id)
{
    struct tlsdata *tlsdata;
    HRESULT hr;

    if (!id)
        return E_INVALIDARG;

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (IsEqualGUID(&tlsdata->causality_id, &GUID_NULL))
    {
        CoCreateGuid(&tlsdata->causality_id);
        tlsdata->flags |= OLETLS_UUIDINITIALIZED;
    }

    *id = tlsdata->causality_id;

    return S_OK;
}

/******************************************************************************
 *              CoGetCurrentProcess        (combase.@)
 */
DWORD WINAPI CoGetCurrentProcess(void)
{
    struct tlsdata *tlsdata;

    if (FAILED(com_get_tlsdata(&tlsdata)))
        return 0;

    if (!tlsdata->thread_seqid)
        rpcss_get_next_seqid(&tlsdata->thread_seqid);

    return tlsdata->thread_seqid;
}

/***********************************************************************
 *           CoFreeUnusedLibrariesEx    (combase.@)
 */
void WINAPI DECLSPEC_HOTPATCH CoFreeUnusedLibrariesEx(DWORD unload_delay, DWORD reserved)
{
    struct apartment *apt = com_get_current_apt();
    if (!apt)
    {
        ERR("apartment not initialised\n");
        return;
    }

    apartment_freeunusedlibraries(apt, unload_delay);
}

/*
 * When locked, don't modify list (unless we add a new head), so that it's
 * safe to iterate it. Freeing of list entries is delayed and done on unlock.
 */
static inline void lock_init_spies(struct tlsdata *tlsdata)
{
    tlsdata->spies_lock++;
}

static void unlock_init_spies(struct tlsdata *tlsdata)
{
    struct init_spy *spy, *next;

    if (--tlsdata->spies_lock) return;

    LIST_FOR_EACH_ENTRY_SAFE(spy, next, &tlsdata->spies, struct init_spy, entry)
    {
        if (spy->spy) continue;
        list_remove(&spy->entry);
        free(spy);
    }
}

/******************************************************************************
 *           CoInitializeWOW    (combase.@)
 */
HRESULT WINAPI CoInitializeWOW(DWORD arg1, DWORD arg2)
{
    FIXME("%#lx, %#lx\n", arg1, arg2);

    return S_OK;
}

/******************************************************************************
 *                    CoInitializeEx    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoInitializeEx(void *reserved, DWORD model)
{
    struct tlsdata *tlsdata;
    struct init_spy *cursor;
    HRESULT hr;

    TRACE("%p, %#lx\n", reserved, model);

    if (reserved)
        WARN("Unexpected reserved argument %p\n", reserved);

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (InterlockedExchangeAdd(&com_lockcount, 1) == 0)
        TRACE("Initializing the COM libraries\n");

    lock_init_spies(tlsdata);
    LIST_FOR_EACH_ENTRY(cursor, &tlsdata->spies, struct init_spy, entry)
    {
        if (cursor->spy) IInitializeSpy_PreInitialize(cursor->spy, model, tlsdata->inits);
    }
    unlock_init_spies(tlsdata);

    hr = enter_apartment(tlsdata, model);

    lock_init_spies(tlsdata);
    LIST_FOR_EACH_ENTRY(cursor, &tlsdata->spies, struct init_spy, entry)
    {
        if (cursor->spy) hr = IInitializeSpy_PostInitialize(cursor->spy, hr, model, tlsdata->inits);
    }
    unlock_init_spies(tlsdata);

    return hr;
}

/***********************************************************************
 *           CoUninitialize    (combase.@)
 */
void WINAPI DECLSPEC_HOTPATCH CoUninitialize(void)
{
    struct tlsdata *tlsdata;
    struct init_spy *cursor, *next;
    LONG lockcount;

    TRACE("\n");

    if (FAILED(com_get_tlsdata(&tlsdata)))
        return;

    lock_init_spies(tlsdata);
    LIST_FOR_EACH_ENTRY_SAFE(cursor, next, &tlsdata->spies, struct init_spy, entry)
    {
        if (cursor->spy) IInitializeSpy_PreUninitialize(cursor->spy, tlsdata->inits);
    }
    unlock_init_spies(tlsdata);

    /* sanity check */
    if (!tlsdata->inits)
    {
        ERR("Mismatched CoUninitialize\n");

        lock_init_spies(tlsdata);
        LIST_FOR_EACH_ENTRY_SAFE(cursor, next, &tlsdata->spies, struct init_spy, entry)
        {
            if (cursor->spy) IInitializeSpy_PostUninitialize(cursor->spy, tlsdata->inits);
        }
        unlock_init_spies(tlsdata);

        return;
    }

    leave_apartment(tlsdata);

    /*
     * Decrease the reference count.
     * If we are back to 0 locks on the COM library, make sure we free
     * all the associated data structures.
     */
    lockcount = InterlockedExchangeAdd(&com_lockcount, -1);
    if (lockcount == 1)
    {
        TRACE("Releasing the COM libraries\n");

        com_revoke_all_ps_clsids();
        DestroyRunningObjectTable();
    }
    else if (lockcount < 1)
    {
        ERR("Unbalanced lock count %ld\n", lockcount);
        InterlockedExchangeAdd(&com_lockcount, 1);
    }

    lock_init_spies(tlsdata);
    LIST_FOR_EACH_ENTRY(cursor, &tlsdata->spies, struct init_spy, entry)
    {
        if (cursor->spy) IInitializeSpy_PostUninitialize(cursor->spy, tlsdata->inits);
    }
    unlock_init_spies(tlsdata);
}

/***********************************************************************
 *           CoIncrementMTAUsage    (combase.@)
 */
HRESULT WINAPI CoIncrementMTAUsage(CO_MTA_USAGE_COOKIE *cookie)
{
    TRACE("%p\n", cookie);

    return apartment_increment_mta_usage(cookie);
}

/***********************************************************************
 *           CoDecrementMTAUsage    (combase.@)
 */
HRESULT WINAPI CoDecrementMTAUsage(CO_MTA_USAGE_COOKIE cookie)
{
    TRACE("%p\n", cookie);

    apartment_decrement_mta_usage(cookie);
    return S_OK;
}

/***********************************************************************
 *           CoGetApartmentType    (combase.@)
 */
HRESULT WINAPI CoGetApartmentType(APTTYPE *type, APTTYPEQUALIFIER *qualifier)
{
    struct tlsdata *tlsdata;
    struct apartment *apt;
    HRESULT hr;

    TRACE("%p, %p\n", type, qualifier);

    if (!type || !qualifier)
        return E_INVALIDARG;

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (!tlsdata->apt)
        *type = APTTYPE_CURRENT;
    else if (tlsdata->apt->multi_threaded)
        *type = APTTYPE_MTA;
    else if (tlsdata->apt->main)
        *type = APTTYPE_MAINSTA;
    else
        *type = APTTYPE_STA;

    *qualifier = APTTYPEQUALIFIER_NONE;

    if (!tlsdata->apt && (apt = apartment_get_mta()))
    {
        apartment_release(apt);
        *type = APTTYPE_MTA;
        *qualifier = APTTYPEQUALIFIER_IMPLICIT_MTA;
        return S_OK;
    }

    return tlsdata->apt ? S_OK : CO_E_NOTINITIALIZED;
}

/******************************************************************************
 *        CoRegisterClassObject    (combase.@)
 * BUGS
 *  MSDN claims that multiple interface registrations are legal, but we
 *  can't do that with our current implementation.
 */
HRESULT WINAPI CoRegisterClassObject(REFCLSID rclsid, IUnknown *object, DWORD clscontext,
        DWORD flags, DWORD *cookie)
{
    static LONG next_cookie;

    struct registered_class *newclass;
    IUnknown *found_object;
    struct apartment *apt;
    HRESULT hr = S_OK;

    TRACE("%s, %p, %#lx, %#lx, %p\n", debugstr_guid(rclsid), object, clscontext, flags, cookie);

    if (!cookie || !object)
        return E_INVALIDARG;

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("COM was not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    *cookie = 0;

    /* REGCLS_MULTIPLEUSE implies registering as inproc server. This is what
     * differentiates the flag from REGCLS_MULTI_SEPARATE. */
    if (flags & REGCLS_MULTIPLEUSE)
        clscontext |= CLSCTX_INPROC_SERVER;

    /*
     * First, check if the class is already registered.
     * If it is, this should cause an error.
     */
    if ((found_object = com_get_registered_class_object(apt, rclsid, clscontext)))
    {
        if (flags & REGCLS_MULTIPLEUSE)
        {
            if (clscontext & CLSCTX_LOCAL_SERVER)
                hr = CoLockObjectExternal(found_object, TRUE, FALSE);
            IUnknown_Release(found_object);
            apartment_release(apt);
            return hr;
        }

        IUnknown_Release(found_object);
        ERR("object already registered for class %s\n", debugstr_guid(rclsid));
        apartment_release(apt);
        return CO_E_OBJISREG;
    }

    newclass = calloc(1, sizeof(*newclass));
    if (!newclass)
    {
        apartment_release(apt);
        return E_OUTOFMEMORY;
    }

    newclass->clsid = *rclsid;
    newclass->apartment_id = apt->oxid;
    newclass->clscontext = clscontext;
    newclass->flags = flags;

    if (!(newclass->cookie = InterlockedIncrement(&next_cookie)))
        newclass->cookie = InterlockedIncrement(&next_cookie);

    newclass->object = object;
    IUnknown_AddRef(newclass->object);

    EnterCriticalSection(&registered_classes_cs);
    list_add_tail(&registered_classes, &newclass->entry);
    LeaveCriticalSection(&registered_classes_cs);

    *cookie = newclass->cookie;

    if (clscontext & CLSCTX_LOCAL_SERVER)
    {
        IStream *marshal_stream;

        hr = apartment_get_local_server_stream(apt, &marshal_stream);
        if(FAILED(hr))
        {
            apartment_release(apt);
            return hr;
        }

        hr = rpc_register_local_server(&newclass->clsid, marshal_stream, flags, &newclass->rpcss_cookie);
        IStream_Release(marshal_stream);
    }

    apartment_release(apt);
    return S_OK;
}

static void com_revoke_class_object(struct registered_class *entry)
{
    list_remove(&entry->entry);

    if (entry->clscontext & CLSCTX_LOCAL_SERVER)
        rpc_revoke_local_server(entry->rpcss_cookie);

    IUnknown_Release(entry->object);
    free(entry);
}

/* Cleans up rpcss registry */
static void com_revoke_local_servers(void)
{
    struct registered_class *cur, *cur2;

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &registered_classes, struct registered_class, entry)
    {
        if (cur->clscontext & CLSCTX_LOCAL_SERVER)
            com_revoke_class_object(cur);
    }

    LeaveCriticalSection(&registered_classes_cs);
}

void apartment_revoke_all_classes(const struct apartment *apt)
{
    struct registered_class *cur, *cur2;

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY_SAFE(cur, cur2, &registered_classes, struct registered_class, entry)
    {
        if (cur->apartment_id == apt->oxid)
            com_revoke_class_object(cur);
    }

    LeaveCriticalSection(&registered_classes_cs);
}

/***********************************************************************
 *           CoRevokeClassObject    (combase.@)
 */
HRESULT WINAPI DECLSPEC_HOTPATCH CoRevokeClassObject(DWORD cookie)
{
    HRESULT hr = E_INVALIDARG;
    struct registered_class *cur;
    struct apartment *apt;

    TRACE("%#lx\n", cookie);

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("COM was not initialized\n");
        return CO_E_NOTINITIALIZED;
    }

    EnterCriticalSection(&registered_classes_cs);

    LIST_FOR_EACH_ENTRY(cur, &registered_classes, struct registered_class, entry)
    {
        if (cur->cookie != cookie)
            continue;

        if (cur->apartment_id == apt->oxid)
        {
            com_revoke_class_object(cur);
            hr = S_OK;
        }
        else
        {
            ERR("called from wrong apartment, should be called from %s\n", wine_dbgstr_longlong(cur->apartment_id));
            hr = RPC_E_WRONG_THREAD;
        }

        break;
    }

    LeaveCriticalSection(&registered_classes_cs);
    apartment_release(apt);

    return hr;
}

/***********************************************************************
 *           CoAddRefServerProcess    (combase.@)
 */
ULONG WINAPI CoAddRefServerProcess(void)
{
    ULONG refs;

    TRACE("\n");

    EnterCriticalSection(&registered_classes_cs);
    refs = ++com_server_process_refcount;
    LeaveCriticalSection(&registered_classes_cs);

    TRACE("refs before: %ld\n", refs - 1);

    return refs;
}

/***********************************************************************
 *           CoReleaseServerProcess [OLE32.@]
 */
ULONG WINAPI CoReleaseServerProcess(void)
{
    ULONG refs;

    TRACE("\n");

    EnterCriticalSection(&registered_classes_cs);

    refs = --com_server_process_refcount;
    /* FIXME: suspend objects */

    LeaveCriticalSection(&registered_classes_cs);

    TRACE("refs after: %ld\n", refs);

    return refs;
}

/******************************************************************************
 *            CoDisconnectObject    (combase.@)
 */
HRESULT WINAPI CoDisconnectObject(IUnknown *object, DWORD reserved)
{
    struct stub_manager *manager;
    struct apartment *apt;
    IMarshal *marshal;
    HRESULT hr;

    TRACE("%p, %#lx\n", object, reserved);

    if (!object)
        return E_INVALIDARG;

    hr = IUnknown_QueryInterface(object, &IID_IMarshal, (void **)&marshal);
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

    manager = get_stub_manager_from_object(apt, object, FALSE);
    if (manager)
    {
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

/******************************************************************************
 *            CoLockObjectExternal    (combase.@)
 */
HRESULT WINAPI CoLockObjectExternal(IUnknown *object, BOOL lock, BOOL last_unlock_releases)
{
    struct stub_manager *stubmgr;
    struct apartment *apt;

    TRACE("%p, %d, %d\n", object, lock, last_unlock_releases);

    if (!(apt = apartment_get_current_or_mta()))
    {
        ERR("apartment not initialised\n");
        return CO_E_NOTINITIALIZED;
    }

    stubmgr = get_stub_manager_from_object(apt, object, lock);
    if (!stubmgr)
    {
        WARN("stub object not found %p\n", object);
        /* Note: native is pretty broken here because it just silently
         * fails, without returning an appropriate error code, making apps
         * think that the object was disconnected, when it actually wasn't */
        apartment_release(apt);
        return S_OK;
    }

    if (lock)
        stub_manager_ext_addref(stubmgr, 1, FALSE);
    else
        stub_manager_ext_release(stubmgr, 1, FALSE, last_unlock_releases);

    stub_manager_int_release(stubmgr);
    apartment_release(apt);
    return S_OK;
}

/***********************************************************************
 *           CoRegisterChannelHook    (combase.@)
 */
HRESULT WINAPI CoRegisterChannelHook(REFGUID guidExtension, IChannelHook *channel_hook)
{
    TRACE("%s, %p\n", debugstr_guid(guidExtension), channel_hook);

    return rpc_register_channel_hook(guidExtension, channel_hook);
}

/***********************************************************************
 *           CoDisableCallCancellation    (combase.@)
 */
HRESULT WINAPI CoDisableCallCancellation(void *reserved)
{
    struct tlsdata *tlsdata;
    HRESULT hr;

    TRACE("%p\n", reserved);

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    if (!tlsdata->cancelcount)
        return CO_E_CANCEL_DISABLED;

    tlsdata->cancelcount--;

    return S_OK;
}

/***********************************************************************
 *           CoEnableCallCancellation    (combase.@)
 */
HRESULT WINAPI CoEnableCallCancellation(void *reserved)
{
    struct tlsdata *tlsdata;
    HRESULT hr;

    TRACE("%p\n", reserved);

    if (FAILED(hr = com_get_tlsdata(&tlsdata)))
        return hr;

    tlsdata->cancelcount++;

    return S_OK;
}

/***********************************************************************
 *           CoGetCallerTID    (combase.@)
 */
HRESULT WINAPI CoGetCallerTID(DWORD *tid)
{
    FIXME("stub!\n");
    return E_NOTIMPL;
}

/***********************************************************************
 *           CoIsHandlerConnected    (combase.@)
 */
BOOL WINAPI CoIsHandlerConnected(IUnknown *object)
{
    FIXME("%p\n", object);

    return TRUE;
}

/***********************************************************************
 *           CoSuspendClassObjects   (combase.@)
 */
HRESULT WINAPI CoSuspendClassObjects(void)
{
    FIXME("\n");

    return S_OK;
}

/***********************************************************************
 *           CoResumeClassObjects    (combase.@)
 */
HRESULT WINAPI CoResumeClassObjects(void)
{
    FIXME("stub\n");

    return S_OK;
}

/***********************************************************************
 *           CoRegisterSurrogate    (combase.@)
 */
HRESULT WINAPI CoRegisterSurrogate(ISurrogate *surrogate)
{
    FIXME("%p stub\n", surrogate);

    return E_NOTIMPL;
}

/***********************************************************************
 *           CoRegisterSurrogateEx  (combase.@)
 */
HRESULT WINAPI CoRegisterSurrogateEx(REFGUID guid, void *reserved)
{
    FIXME("%s, %p stub\n", debugstr_guid(guid), reserved);

    return E_NOTIMPL;
}

/***********************************************************************
 *            DllMain     (combase.@)
 */
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD reason, LPVOID reserved)
{
    TRACE("%p, %#lx, %p\n", hinstDLL, reason, reserved);

    switch (reason)
    {
    case DLL_PROCESS_ATTACH:
        hProxyDll = hinstDLL;
        break;
    case DLL_PROCESS_DETACH:
        com_revoke_local_servers();
        if (reserved) break;
        apartment_global_cleanup();
        DeleteCriticalSection(&registered_classes_cs);
        rpc_unregister_channel_hooks();
        break;
    case DLL_THREAD_DETACH:
        com_cleanup_tlsdata();
        break;
    }

    return TRUE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void **obj)
{
    TRACE("%s, %s, %p.\n", debugstr_guid(rclsid), debugstr_guid(riid), obj);

    *obj = NULL;

    if (IsEqualCLSID(rclsid, &CLSID_GlobalOptions))
        return IClassFactory_QueryInterface(&global_options_factory, riid, obj);

    return CLASS_E_CLASSNOTAVAILABLE;
}
