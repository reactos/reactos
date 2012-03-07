/*
 *
 * Copyright 2008 Alistair Leslie-Hughes
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

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "shellapi.h"

#include "cor.h"
#include "mscoree.h"
#include "metahost.h"
#include "corhdr.h"
#include "cordebug.h"
#include "wine/list.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

#include "initguid.h"

DEFINE_GUID(IID__AppDomain, 0x05f696dc,0x2b29,0x3663,0xad,0x8b,0xc4,0x38,0x9c,0xf2,0xa7,0x13);

struct DomainEntry
{
    struct list entry;
    MonoDomain *domain;
};

static HRESULT RuntimeHost_AddDomain(RuntimeHost *This, MonoDomain **result)
{
    struct DomainEntry *entry;
    char *mscorlib_path;
    HRESULT res=S_OK;

    EnterCriticalSection(&This->lock);

    entry = HeapAlloc(GetProcessHeap(), 0, sizeof(*entry));
    if (!entry)
    {
        res = E_OUTOFMEMORY;
        goto end;
    }

    mscorlib_path = WtoA(This->version->mscorlib_path);
    if (!mscorlib_path)
    {
        HeapFree(GetProcessHeap(), 0, entry);
        res = E_OUTOFMEMORY;
        goto end;
    }

    entry->domain = This->mono->mono_jit_init(mscorlib_path);

    HeapFree(GetProcessHeap(), 0, mscorlib_path);

    if (!entry->domain)
    {
        HeapFree(GetProcessHeap(), 0, entry);
        res = E_FAIL;
        goto end;
    }

    This->mono->is_started = TRUE;

    list_add_tail(&This->domains, &entry->entry);

    *result = entry->domain;

end:
    LeaveCriticalSection(&This->lock);

    return res;
}

static HRESULT RuntimeHost_GetDefaultDomain(RuntimeHost *This, MonoDomain **result)
{
    HRESULT res=S_OK;

    EnterCriticalSection(&This->lock);

    if (This->default_domain) goto end;

    res = RuntimeHost_AddDomain(This, &This->default_domain);

end:
    *result = This->default_domain;

    LeaveCriticalSection(&This->lock);

    return res;
}

static void RuntimeHost_DeleteDomain(RuntimeHost *This, MonoDomain *domain)
{
    struct DomainEntry *entry;

    EnterCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY(entry, &This->domains, struct DomainEntry, entry)
    {
        if (entry->domain == domain)
        {
            list_remove(&entry->entry);
            if (This->default_domain == domain)
                This->default_domain = NULL;
            HeapFree(GetProcessHeap(), 0, entry);
            break;
        }
    }

    LeaveCriticalSection(&This->lock);
}

static HRESULT RuntimeHost_GetIUnknownForDomain(RuntimeHost *This, MonoDomain *domain, IUnknown **punk)
{
    HRESULT hr;
    void *args[1];
    MonoAssembly *assembly;
    MonoImage *image;
    MonoClass *klass;
    MonoMethod *method;
    MonoObject *appdomain_object;
    IUnknown *unk;

    assembly = This->mono->mono_domain_assembly_open(domain, "mscorlib");
    if (!assembly)
    {
        ERR("Cannot load mscorlib\n");
        return E_FAIL;
    }

    image = This->mono->mono_assembly_get_image(assembly);
    if (!image)
    {
        ERR("Couldn't get assembly image\n");
        return E_FAIL;
    }

    klass = This->mono->mono_class_from_name(image, "System", "AppDomain");
    if (!klass)
    {
        ERR("Couldn't get class from image\n");
        return E_FAIL;
    }

    method = This->mono->mono_class_get_method_from_name(klass, "get_CurrentDomain", 0);
    if (!method)
    {
        ERR("Couldn't get method from class\n");
        return E_FAIL;
    }

    args[0] = NULL;
    appdomain_object = This->mono->mono_runtime_invoke(method, NULL, args, NULL);
    if (!appdomain_object)
    {
        ERR("Couldn't get result pointer\n");
        return E_FAIL;
    }

    hr = RuntimeHost_GetIUnknownForObject(This, appdomain_object, &unk);

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(unk, &IID__AppDomain, (void**)punk);

        IUnknown_Release(unk);
    }

    return hr;
}

static inline RuntimeHost *impl_from_ICLRRuntimeHost( ICLRRuntimeHost *iface )
{
    return CONTAINING_RECORD(iface, RuntimeHost, ICLRRuntimeHost_iface);
}

static inline RuntimeHost *impl_from_ICorRuntimeHost( ICorRuntimeHost *iface )
{
    return CONTAINING_RECORD(iface, RuntimeHost, ICorRuntimeHost_iface);
}

/*** IUnknown methods ***/
static HRESULT WINAPI corruntimehost_QueryInterface(ICorRuntimeHost* iface,
        REFIID riid,
        void **ppvObject)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICorRuntimeHost ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICorRuntimeHost_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI corruntimehost_AddRef(ICorRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );

    return InterlockedIncrement( &This->ref );
}

static ULONG WINAPI corruntimehost_Release(ICorRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    ULONG ref;

    ref = InterlockedDecrement( &This->ref );

    return ref;
}

/*** ICorRuntimeHost methods ***/
static HRESULT WINAPI corruntimehost_CreateLogicalThreadState(
                    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_DeleteLogicalThreadState(
                    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_SwitchInLogicalThreadState(
                    ICorRuntimeHost* iface,
                    DWORD *fiberCookie)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_SwitchOutLogicalThreadState(
                    ICorRuntimeHost* iface,
                    DWORD **fiberCookie)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_LocksHeldByLogicalThread(
                    ICorRuntimeHost* iface,
                    DWORD *pCount)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_MapFile(
    ICorRuntimeHost* iface,
    HANDLE hFile,
    HMODULE *mapAddress)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_GetConfiguration(
    ICorRuntimeHost* iface,
    ICorConfiguration **pConfiguration)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_Start(
    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return S_OK;
}

static HRESULT WINAPI corruntimehost_Stop(
    ICorRuntimeHost* iface)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateDomain(
    ICorRuntimeHost* iface,
    LPCWSTR friendlyName,
    IUnknown *identityArray,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_GetDefaultDomain(
    ICorRuntimeHost* iface,
    IUnknown **pAppDomain)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    HRESULT hr;
    MonoDomain *domain;

    TRACE("(%p)\n", iface);

    hr = RuntimeHost_GetDefaultDomain(This, &domain);

    if (SUCCEEDED(hr))
    {
        hr = RuntimeHost_GetIUnknownForDomain(This, domain, pAppDomain);
    }

    return hr;
}

static HRESULT WINAPI corruntimehost_EnumDomains(
    ICorRuntimeHost* iface,
    HDOMAINENUM *hEnum)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_NextDomain(
    ICorRuntimeHost* iface,
    HDOMAINENUM hEnum,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CloseEnum(
    ICorRuntimeHost* iface,
    HDOMAINENUM hEnum)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateDomainEx(
    ICorRuntimeHost* iface,
    LPCWSTR friendlyName,
    IUnknown *setup,
    IUnknown *evidence,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateDomainSetup(
    ICorRuntimeHost* iface,
    IUnknown **appDomainSetup)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CreateEvidence(
    ICorRuntimeHost* iface,
    IUnknown **evidence)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_UnloadDomain(
    ICorRuntimeHost* iface,
    IUnknown *appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI corruntimehost_CurrentDomain(
    ICorRuntimeHost* iface,
    IUnknown **appDomain)
{
    FIXME("stub %p\n", iface);
    return E_NOTIMPL;
}

static const struct ICorRuntimeHostVtbl corruntimehost_vtbl =
{
    corruntimehost_QueryInterface,
    corruntimehost_AddRef,
    corruntimehost_Release,
    corruntimehost_CreateLogicalThreadState,
    corruntimehost_DeleteLogicalThreadState,
    corruntimehost_SwitchInLogicalThreadState,
    corruntimehost_SwitchOutLogicalThreadState,
    corruntimehost_LocksHeldByLogicalThread,
    corruntimehost_MapFile,
    corruntimehost_GetConfiguration,
    corruntimehost_Start,
    corruntimehost_Stop,
    corruntimehost_CreateDomain,
    corruntimehost_GetDefaultDomain,
    corruntimehost_EnumDomains,
    corruntimehost_NextDomain,
    corruntimehost_CloseEnum,
    corruntimehost_CreateDomainEx,
    corruntimehost_CreateDomainSetup,
    corruntimehost_CreateEvidence,
    corruntimehost_UnloadDomain,
    corruntimehost_CurrentDomain
};

static HRESULT WINAPI CLRRuntimeHost_QueryInterface(ICLRRuntimeHost* iface,
        REFIID riid,
        void **ppvObject)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    TRACE("%p %s %p\n", This, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICLRRuntimeHost ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICLRRuntimeHost_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CLRRuntimeHost_AddRef(ICLRRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    return ICorRuntimeHost_AddRef(&This->ICorRuntimeHost_iface);
}

static ULONG WINAPI CLRRuntimeHost_Release(ICLRRuntimeHost* iface)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    return ICorRuntimeHost_Release(&This->ICorRuntimeHost_iface);
}

static HRESULT WINAPI CLRRuntimeHost_Start(ICLRRuntimeHost* iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_Stop(ICLRRuntimeHost* iface)
{
    FIXME("(%p)\n", iface);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_SetHostControl(ICLRRuntimeHost* iface,
    IHostControl *pHostControl)
{
    FIXME("(%p,%p)\n", iface, pHostControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_GetCLRControl(ICLRRuntimeHost* iface,
    ICLRControl **pCLRControl)
{
    FIXME("(%p,%p)\n", iface, pCLRControl);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_UnloadAppDomain(ICLRRuntimeHost* iface,
    DWORD dwAppDomainId, BOOL fWaitUntilDone)
{
    FIXME("(%p,%u,%i)\n", iface, dwAppDomainId, fWaitUntilDone);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteInAppDomain(ICLRRuntimeHost* iface,
    DWORD dwAppDomainId, FExecuteInAppDomainCallback pCallback, void *cookie)
{
    FIXME("(%p,%u,%p,%p)\n", iface, dwAppDomainId, pCallback, cookie);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_GetCurrentAppDomainId(ICLRRuntimeHost* iface,
    DWORD *pdwAppDomainId)
{
    FIXME("(%p,%p)\n", iface, pdwAppDomainId);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteApplication(ICLRRuntimeHost* iface,
    LPCWSTR pwzAppFullName, DWORD dwManifestPaths, LPCWSTR *ppwzManifestPaths,
    DWORD dwActivationData, LPCWSTR *ppwzActivationData, int *pReturnValue)
{
    FIXME("(%p,%s,%u,%u)\n", iface, debugstr_w(pwzAppFullName), dwManifestPaths, dwActivationData);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteInDefaultAppDomain(ICLRRuntimeHost* iface,
    LPCWSTR pwzAssemblyPath, LPCWSTR pwzTypeName, LPCWSTR pwzMethodName,
    LPCWSTR pwzArgument, DWORD *pReturnValue)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    HRESULT hr;
    MonoDomain *domain;
    MonoAssembly *assembly;
    MonoImage *image;
    MonoClass *klass;
    MonoMethod *method;
    MonoObject *result;
    MonoString *str;
    void *args[2];
    char *filenameA = NULL, *classA = NULL, *methodA = NULL;
    char *argsA = NULL, *ns;

    TRACE("(%p,%s,%s,%s,%s)\n", iface, debugstr_w(pwzAssemblyPath),
        debugstr_w(pwzTypeName), debugstr_w(pwzMethodName), debugstr_w(pwzArgument));

    hr = RuntimeHost_GetDefaultDomain(This, &domain);
    if(hr != S_OK)
    {
        ERR("Couldn't get Default Domain\n");
        return hr;
    }

    hr = E_FAIL;

    filenameA = WtoA(pwzAssemblyPath);
    assembly = This->mono->mono_domain_assembly_open(domain, filenameA);
    if (!assembly)
    {
        ERR("Cannot open assembly %s\n", filenameA);
        goto cleanup;
    }

    image = This->mono->mono_assembly_get_image(assembly);
    if (!image)
    {
        ERR("Couldn't get assembly image\n");
        goto cleanup;
    }

    classA = WtoA(pwzTypeName);
    ns = strrchr(classA, '.');
    *ns = '\0';
    klass = This->mono->mono_class_from_name(image, classA, ns+1);
    if (!klass)
    {
        ERR("Couldn't get class from image\n");
        goto cleanup;
    }

    methodA = WtoA(pwzMethodName);
    method = This->mono->mono_class_get_method_from_name(klass, methodA, 1);
    if (!method)
    {
        ERR("Couldn't get method from class\n");
        goto cleanup;
    }

    /* The .NET function we are calling has the following declaration
     *   public static int functionName(String param)
     */
    argsA = WtoA(pwzArgument);
    str = This->mono->mono_string_new(domain, argsA);
    args[0] = str;
    args[1] = NULL;
    result = This->mono->mono_runtime_invoke(method, NULL, args, NULL);
    if (!result)
        ERR("Couldn't get result pointer\n");
    else
    {
        *pReturnValue = *(DWORD*)This->mono->mono_object_unbox(result);
        hr = S_OK;
    }

cleanup:
    if(filenameA)
        HeapFree(GetProcessHeap(), 0, filenameA);
    if(classA)
        HeapFree(GetProcessHeap(), 0, classA);
    if(argsA)
        HeapFree(GetProcessHeap(), 0, argsA);
    if(methodA)
        HeapFree(GetProcessHeap(), 0, methodA);

    return hr;
}

static const struct ICLRRuntimeHostVtbl CLRHostVtbl =
{
    CLRRuntimeHost_QueryInterface,
    CLRRuntimeHost_AddRef,
    CLRRuntimeHost_Release,
    CLRRuntimeHost_Start,
    CLRRuntimeHost_Stop,
    CLRRuntimeHost_SetHostControl,
    CLRRuntimeHost_GetCLRControl,
    CLRRuntimeHost_UnloadAppDomain,
    CLRRuntimeHost_ExecuteInAppDomain,
    CLRRuntimeHost_GetCurrentAppDomainId,
    CLRRuntimeHost_ExecuteApplication,
    CLRRuntimeHost_ExecuteInDefaultAppDomain
};

/* Create an instance of a type given its name, by calling its constructor with
 * no arguments. Note that result MUST be in the stack, or the garbage
 * collector may free it prematurely. */
HRESULT RuntimeHost_CreateManagedInstance(RuntimeHost *This, LPCWSTR name,
    MonoDomain *domain, MonoObject **result)
{
    HRESULT hr=S_OK;
    char *nameA=NULL;
    MonoType *type;
    MonoClass *klass;
    MonoObject *obj;

    if (!domain)
        hr = RuntimeHost_GetDefaultDomain(This, &domain);

    if (SUCCEEDED(hr))
    {
        nameA = WtoA(name);
        if (!nameA)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        type = This->mono->mono_reflection_type_from_name(nameA, NULL);
        if (!type)
        {
            ERR("Cannot find type %s\n", debugstr_w(name));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        klass = This->mono->mono_class_from_mono_type(type);
        if (!klass)
        {
            ERR("Cannot convert type %s to a class\n", debugstr_w(name));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        obj = This->mono->mono_object_new(domain, klass);
        if (!obj)
        {
            ERR("Cannot allocate object of type %s\n", debugstr_w(name));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        /* FIXME: Detect exceptions from the constructor? */
        This->mono->mono_runtime_object_init(obj);
        *result = obj;
    }

    HeapFree(GetProcessHeap(), 0, nameA);

    return hr;
}

/* Get an IUnknown pointer for a Mono object.
 *
 * This is just a "light" wrapper around
 * System.Runtime.InteropServices.Marshal:GetIUnknownForObject
 *
 * NOTE: The IUnknown* is created with a reference to the object.
 * Until they have a reference, objects must be in the stack to prevent the
 * garbage collector from freeing them. */
HRESULT RuntimeHost_GetIUnknownForObject(RuntimeHost *This, MonoObject *obj,
    IUnknown **ppUnk)
{
    MonoDomain *domain;
    MonoAssembly *assembly;
    MonoImage *image;
    MonoClass *klass;
    MonoMethod *method;
    MonoObject *result;
    void *args[2];

    domain = This->mono->mono_object_get_domain(obj);

    assembly = This->mono->mono_domain_assembly_open(domain, "mscorlib");
    if (!assembly)
    {
        ERR("Cannot load mscorlib\n");
        return E_FAIL;
    }

    image = This->mono->mono_assembly_get_image(assembly);
    if (!image)
    {
        ERR("Couldn't get assembly image\n");
        return E_FAIL;
    }

    klass = This->mono->mono_class_from_name(image, "System.Runtime.InteropServices", "Marshal");
    if (!klass)
    {
        ERR("Couldn't get class from image\n");
        return E_FAIL;
    }

    method = This->mono->mono_class_get_method_from_name(klass, "GetIUnknownForObject", 1);
    if (!method)
    {
        ERR("Couldn't get method from class\n");
        return E_FAIL;
    }

    args[0] = obj;
    args[1] = NULL;
    result = This->mono->mono_runtime_invoke(method, NULL, args, NULL);
    if (!result)
    {
        ERR("Couldn't get result pointer\n");
        return E_FAIL;
    }

    *ppUnk = *(IUnknown**)This->mono->mono_object_unbox(result);
    if (!*ppUnk)
    {
        ERR("GetIUnknownForObject returned 0\n");
        return E_FAIL;
    }

    return S_OK;
}

static void get_utf8_args(int *argc, char ***argv)
{
    WCHAR **argvw;
    int size=0, i;
    char *current_arg;

    argvw = CommandLineToArgvW(GetCommandLineW(), argc);

    for (i=0; i<*argc; i++)
    {
        size += sizeof(char*);
        size += WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, NULL, 0, NULL, NULL);
    }
    size += sizeof(char*);

    *argv = HeapAlloc(GetProcessHeap(), 0, size);
    current_arg = (char*)(*argv + *argc + 1);

    for (i=0; i<*argc; i++)
    {
        (*argv)[i] = current_arg;
        current_arg += WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, current_arg, size, NULL, NULL);
    }

    (*argv)[*argc] = NULL;

    HeapFree(GetProcessHeap(), 0, argvw);
}

__int32 WINAPI _CorExeMain(void)
{
    int exit_code;
    int argc;
    char **argv;
    MonoDomain *domain;
    MonoAssembly *assembly;
    WCHAR filename[MAX_PATH];
    char *filenameA;
    ICLRRuntimeInfo *info;
    RuntimeHost *host;
    HRESULT hr;
    int i;

    get_utf8_args(&argc, &argv);

    GetModuleFileNameW(NULL, filename, MAX_PATH);

    TRACE("%s", debugstr_w(filename));
    for (i=0; i<argc; i++)
        TRACE(" %s", debugstr_a(argv[i]));
    TRACE("\n");

    filenameA = WtoA(filename);
    if (!filenameA)
        return -1;

    hr = get_runtime_info(filename, NULL, NULL, 0, 0, FALSE, &info);

    if (SUCCEEDED(hr))
    {
        hr = ICLRRuntimeInfo_GetRuntimeHost(info, &host);

        if (SUCCEEDED(hr))
            hr = RuntimeHost_GetDefaultDomain(host, &domain);

        if (SUCCEEDED(hr))
        {
            assembly = host->mono->mono_domain_assembly_open(domain, filenameA);

            exit_code = host->mono->mono_jit_exec(domain, assembly, argc, argv);

            RuntimeHost_DeleteDomain(host, domain);
        }
        else
            exit_code = -1;

        ICLRRuntimeInfo_Release(info);
    }
    else
        exit_code = -1;

    HeapFree(GetProcessHeap(), 0, argv);

    unload_all_runtimes();

    return exit_code;
}

HRESULT RuntimeHost_Construct(const CLRRuntimeInfo *runtime_version,
    loaded_mono *loaded_mono, RuntimeHost** result)
{
    RuntimeHost *This;

    This = HeapAlloc( GetProcessHeap(), 0, sizeof *This );
    if ( !This )
        return E_OUTOFMEMORY;

    This->ICorRuntimeHost_iface.lpVtbl = &corruntimehost_vtbl;
    This->ICLRRuntimeHost_iface.lpVtbl = &CLRHostVtbl;

    This->ref = 1;
    This->version = runtime_version;
    This->mono = loaded_mono;
    list_init(&This->domains);
    This->default_domain = NULL;
    InitializeCriticalSection(&This->lock);
    This->lock.DebugInfo->Spare[0] = (DWORD_PTR)(__FILE__ ": RuntimeHost.lock");

    *result = This;

    return S_OK;
}

HRESULT RuntimeHost_GetInterface(RuntimeHost *This, REFCLSID clsid, REFIID riid, void **ppv)
{
    IUnknown *unk;
    HRESULT hr;

    if (IsEqualGUID(clsid, &CLSID_CorRuntimeHost))
    {
        unk = (IUnknown*)&This->ICorRuntimeHost_iface;
        IUnknown_AddRef(unk);
    }
    else if (IsEqualGUID(clsid, &CLSID_CLRRuntimeHost))
    {
        unk = (IUnknown*)&This->ICLRRuntimeHost_iface;
        IUnknown_AddRef(unk);
    }
    else if (IsEqualGUID(clsid, &CLSID_CorMetaDataDispenser) ||
             IsEqualGUID(clsid, &CLSID_CorMetaDataDispenserRuntime))
    {
        hr = MetaDataDispenser_CreateInstance(&unk);
        if (FAILED(hr))
            return hr;
    }
    else if (IsEqualGUID(clsid, &CLSID_CLRDebuggingLegacy))
    {
        hr = CorDebug_Create(&This->ICLRRuntimeHost_iface, &unk);
        if (FAILED(hr))
            return hr;
    }
    else
        unk = NULL;

    if (unk)
    {
        hr = IUnknown_QueryInterface(unk, riid, ppv);

        IUnknown_Release(unk);

        return hr;
    }
    else
        FIXME("not implemented for class %s\n", debugstr_guid(clsid));

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT RuntimeHost_Destroy(RuntimeHost *This)
{
    struct DomainEntry *cursor, *cursor2;

    This->lock.DebugInfo->Spare[0] = 0;
    DeleteCriticalSection(&This->lock);

    LIST_FOR_EACH_ENTRY_SAFE(cursor, cursor2, &This->domains, struct DomainEntry, entry)
    {
        list_remove(&cursor->entry);
        HeapFree(GetProcessHeap(), 0, cursor);
    }

    HeapFree( GetProcessHeap(), 0, This );
    return S_OK;
}
