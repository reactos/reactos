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

#include <assert.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "shellapi.h"
#include "shlwapi.h"

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

static HANDLE dll_fixup_heap; /* using a separate heap so we can have execute permission */

static CRITICAL_SECTION fixup_list_cs;
static CRITICAL_SECTION_DEBUG fixup_list_cs_debug =
{
    0, 0, &fixup_list_cs,
    { &fixup_list_cs_debug.ProcessLocksList,
      &fixup_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": fixup_list_cs") }
};
static CRITICAL_SECTION fixup_list_cs = { &fixup_list_cs_debug, -1, 0, 0, 0, 0 };

static struct list dll_fixups;

WCHAR **private_path = NULL;

struct dll_fixup
{
    struct list entry;
    BOOL done;
    HMODULE dll;
    void *thunk_code; /* pointer into dll_fixup_heap */
    VTableFixup *fixup;
    void *vtable;
    void *tokens; /* pointer into process heap */
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

struct clrclass_data
{
    ULONG size;
    DWORD res[2];
    ULONG module_len;
    ULONG module_offset;
    ULONG name_len;
    ULONG name_offset;
    ULONG version_len;
    ULONG version_offset;
    DWORD res2[2];
};

static MonoDomain* domain_attach(MonoDomain *domain)
{
    MonoDomain *prev_domain = mono_domain_get();

    if (prev_domain == domain)
        /* Do not set or restore domain. */
        return NULL;

    mono_thread_attach(domain);

    return prev_domain;
}

static void domain_restore(MonoDomain *prev_domain)
{
    if (prev_domain != NULL)
        mono_domain_set(prev_domain, FALSE);
}

static HRESULT RuntimeHost_GetDefaultDomain(RuntimeHost *This, const WCHAR *config_path, MonoDomain **result)
{
    WCHAR exe_config[MAX_PATH];
    WCHAR base_dir[MAX_PATH];
    char *base_dirA, *config_pathA, *slash;
    HRESULT res=S_OK;
    static BOOL configured_domain;

    *result = get_root_domain();

    EnterCriticalSection(&This->lock);

    if (configured_domain) goto end;

    if (!config_path)
    {
        GetModuleFileNameW(NULL, exe_config, MAX_PATH);
        lstrcatW(exe_config, L".config");

        config_path = exe_config;
    }

    config_pathA = WtoA(config_path);
    if (!config_pathA)
    {
        res = E_OUTOFMEMORY;
        goto end;
    }

    GetModuleFileNameW(NULL, base_dir, ARRAY_SIZE(base_dir));
    base_dirA = WtoA(base_dir);
    if (!base_dirA)
    {
        free(config_pathA);
        res = E_OUTOFMEMORY;
        goto end;
    }

    slash = strrchr(base_dirA, '\\');
    if (slash)
        *(slash + 1) = 0;

    TRACE("setting base_dir: %s, config_path: %s\n", base_dirA, config_pathA);
    mono_domain_set_config(*result, base_dirA, config_pathA);

    free(config_pathA);
    free(base_dirA);

end:

    configured_domain = TRUE;

    LeaveCriticalSection(&This->lock);

    return res;
}

static BOOL RuntimeHost_GetMethod(MonoDomain *domain, const char *assemblyname,
    const char *namespace, const char *typename, const char *methodname, int arg_count,
    MonoMethod **method)
{
    MonoAssembly *assembly;
    MonoImage *image;
    MonoClass *klass;

    if (!assemblyname)
    {
        image = mono_get_corlib();
    }
    else
    {
        MonoImageOpenStatus status;
        assembly = mono_assembly_open(assemblyname, &status);
        if (!assembly)
        {
            ERR("Cannot load assembly %s, status=%i\n", assemblyname, status);
            return FALSE;
        }

        image = mono_assembly_get_image(assembly);
        if (!image)
        {
            ERR("Couldn't get assembly image for %s\n", assemblyname);
            return FALSE;
        }
    }

    klass = mono_class_from_name(image, namespace, typename);
    if (!klass)
    {
        ERR("Couldn't get class %s.%s from image\n", namespace, typename);
        return FALSE;
    }

    *method = mono_class_get_method_from_name(klass, methodname, arg_count);
    if (!*method)
    {
        ERR("Couldn't get method %s from class %s.%s\n", methodname, namespace, typename);
        return FALSE;
    }

    return TRUE;
}

static HRESULT RuntimeHost_Invoke(RuntimeHost *This, MonoDomain *domain,
    const char *assemblyname, const char *namespace, const char *typename, const char *methodname,
    MonoObject *obj, void **args, int arg_count, MonoObject **result);

static HRESULT RuntimeHost_DoInvoke(RuntimeHost *This, MonoDomain *domain,
    const char *methodname, MonoMethod *method, MonoObject *obj, void **args, MonoObject **result)
{
    MonoObject *exc;
    static const char *get_hresult = "get_HResult";

    *result = mono_runtime_invoke(method, obj, args, &exc);
    if (exc)
    {
        HRESULT hr;
        MonoObject *hr_object;

        if (methodname != get_hresult)
        {
            /* Map the exception to an HRESULT. */
            hr = RuntimeHost_Invoke(This, domain, NULL, "System", "Exception", get_hresult,
                exc, NULL, 0, &hr_object);
            if (SUCCEEDED(hr))
                hr = *(HRESULT*)mono_object_unbox(hr_object);
            if (SUCCEEDED(hr))
                hr = E_FAIL;
        }
        else
            hr = E_FAIL;
        *result = NULL;
        return hr;
    }

    return S_OK;
}

static HRESULT RuntimeHost_Invoke(RuntimeHost *This, MonoDomain *domain,
    const char *assemblyname, const char *namespace, const char *typename, const char *methodname,
    MonoObject *obj, void **args, int arg_count, MonoObject **result)
{
    MonoMethod *method;
    MonoDomain *prev_domain;
    HRESULT hr;

    *result = NULL;

    prev_domain = domain_attach(domain);

    if (!RuntimeHost_GetMethod(domain, assemblyname, namespace, typename, methodname,
            arg_count, &method))
    {
        domain_restore(prev_domain);
        return E_FAIL;
    }

    hr = RuntimeHost_DoInvoke(This, domain, methodname, method, obj, args, result);
    if (FAILED(hr))
    {
        ERR("Method %s.%s:%s raised an exception, hr=%lx\n", namespace, typename, methodname, hr);
    }

    domain_restore(prev_domain);

    return hr;
}

static HRESULT RuntimeHost_VirtualInvoke(RuntimeHost *This, MonoDomain *domain,
    const char *assemblyname, const char *namespace, const char *typename, const char *methodname,
    MonoObject *obj, void **args, int arg_count, MonoObject **result)
{
    MonoMethod *method;
    MonoDomain *prev_domain;
    HRESULT hr;

    *result = NULL;

    if (!obj)
    {
        ERR("\"this\" object cannot be null\n");
        return E_POINTER;
    }

    prev_domain = domain_attach(domain);

    if (!RuntimeHost_GetMethod(domain, assemblyname, namespace, typename, methodname,
            arg_count, &method))
    {
        domain_restore(prev_domain);
        return E_FAIL;
    }

    method = mono_object_get_virtual_method(obj, method);
    if (!method)
    {
        ERR("Object %p does not support method %s.%s:%s\n", obj, namespace, typename, methodname);
        domain_restore(prev_domain);
        return E_FAIL;
    }

    hr = RuntimeHost_DoInvoke(This, domain, methodname, method, obj, args, result);
    if (FAILED(hr))
    {
        ERR("Method %s.%s:%s raised an exception, hr=%lx\n", namespace, typename, methodname, hr);
    }

    domain_restore(prev_domain);

    return hr;
}

static HRESULT RuntimeHost_GetObjectForIUnknown(RuntimeHost *This, MonoDomain *domain,
    IUnknown *unk, MonoObject **obj)
{
    HRESULT hr;
    void *args[1];
    MonoObject *result;

    args[0] = &unk;
    hr = RuntimeHost_Invoke(This, domain, NULL, "System.Runtime.InteropServices", "Marshal", "GetObjectForIUnknown",
        NULL, args, 1, &result);

    if (SUCCEEDED(hr))
    {
        *obj = result;
    }
    return hr;
}

static HRESULT RuntimeHost_AddDomain(RuntimeHost *This, const WCHAR *name, IUnknown *setup,
    IUnknown *evidence, MonoDomain **result)
{
    HRESULT res;
    char *nameA;
    MonoDomain *domain;
    void *args[3];
    MonoObject *new_domain, *id;

    res = RuntimeHost_GetDefaultDomain(This, NULL, &domain);
    if (FAILED(res))
    {
        return res;
    }

    nameA = WtoA(name);
    if (!nameA)
    {
        return E_OUTOFMEMORY;
    }

    args[0] = mono_string_new(domain, nameA);
    free(nameA);

    if (!args[0])
    {
        return E_OUTOFMEMORY;
    }

    if (evidence)
    {
        res = RuntimeHost_GetObjectForIUnknown(This, domain, evidence, (MonoObject **)&args[1]);
        if (FAILED(res))
        {
            return res;
        }
    }
    else
    {
        args[1] = NULL;
    }

    if (setup)
    {
        res = RuntimeHost_GetObjectForIUnknown(This, domain, setup, (MonoObject **)&args[2]);
        if (FAILED(res))
        {
            return res;
        }
    }
    else
    {
        args[2] = NULL;
    }

    res = RuntimeHost_Invoke(This, domain, NULL, "System", "AppDomain", "CreateDomain",
        NULL, args, 3, &new_domain);

    if (FAILED(res))
    {
        return res;
    }

    /* new_domain is not the AppDomain itself, but a transparent proxy.
     * So, we'll retrieve its ID, and use that to get the real domain object.
     * We can't do a regular invoke, because that will bypass the proxy.
     * Instead, do a vcall.
     */

    res = RuntimeHost_VirtualInvoke(This, domain, NULL, "System", "AppDomain", "get_Id",
        new_domain, NULL, 0, &id);

    if (FAILED(res))
    {
        return res;
    }

    TRACE("returning domain id %d\n", *(int *)mono_object_unbox(id));

    *result = mono_domain_get_by_id(*(int *)mono_object_unbox(id));

    return S_OK;
}

static HRESULT RuntimeHost_GetIUnknownForDomain(RuntimeHost *This, MonoDomain *domain, IUnknown **punk)
{
    HRESULT hr;
    MonoObject *appdomain_object;
    IUnknown *unk;

    hr = RuntimeHost_Invoke(This, domain, NULL, "System", "AppDomain", "get_CurrentDomain",
        NULL, NULL, 0, &appdomain_object);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_GetIUnknownForObject(This, appdomain_object, &unk);

    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface(unk, &IID__AppDomain, (void**)punk);

        IUnknown_Release(unk);
    }

    return hr;
}

void RuntimeHost_ExitProcess(RuntimeHost *This, INT exitcode)
{
    HRESULT hr;
    void *args[2];
    MonoDomain *domain;
    MonoObject *dummy;

    hr = RuntimeHost_GetDefaultDomain(This, NULL, &domain);
    if (FAILED(hr))
    {
        ERR("Cannot get domain, hr=%lx\n", hr);
        return;
    }

    args[0] = &exitcode;
    args[1] = NULL;
    RuntimeHost_Invoke(This, domain, NULL, "System", "Environment", "Exit",
        NULL, args, 1, &dummy);

    ERR("Process should have exited\n");
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
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    MonoDomain *dummy;

    TRACE("%p\n", This);

    return RuntimeHost_GetDefaultDomain(This, NULL, &dummy);
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
    return ICorRuntimeHost_CreateDomainEx(iface, friendlyName, NULL, NULL, appDomain);
}

static HRESULT WINAPI corruntimehost_GetDefaultDomain(
    ICorRuntimeHost* iface,
    IUnknown **pAppDomain)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    HRESULT hr;
    MonoDomain *domain;

    TRACE("(%p)\n", iface);

    hr = RuntimeHost_GetDefaultDomain(This, NULL, &domain);

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
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    HRESULT hr;
    MonoDomain *domain;

    if (!friendlyName || !appDomain)
    {
        return E_POINTER;
    }
    if (!is_mono_started)
    {
        return E_FAIL;
    }

    TRACE("(%p)\n", iface);

    hr = RuntimeHost_AddDomain(This, friendlyName, setup, evidence, &domain);

    if (SUCCEEDED(hr))
    {
        hr = RuntimeHost_GetIUnknownForDomain(This, domain, appDomain);
    }

    return hr;
}

static HRESULT WINAPI corruntimehost_CreateDomainSetup(
    ICorRuntimeHost* iface,
    IUnknown **appDomainSetup)
{
    RuntimeHost *This = impl_from_ICorRuntimeHost( iface );
    HRESULT hr;
    MonoDomain *domain;
    MonoObject *obj;
    static const WCHAR classnameW[] = {'S','y','s','t','e','m','.','A','p','p','D','o','m','a','i','n','S','e','t','u','p',',','m','s','c','o','r','l','i','b',0};

    TRACE("(%p)\n", iface);

    hr = RuntimeHost_GetDefaultDomain(This, NULL, &domain);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_CreateManagedInstance(This, classnameW, domain, &obj);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_GetIUnknownForObject(This, obj, appDomainSetup);

    return hr;
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
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    MonoDomain *dummy;

    TRACE("%p\n", This);

    return RuntimeHost_GetDefaultDomain(This, NULL, &dummy);
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
    FIXME("(%p,%lu,%i)\n", iface, dwAppDomainId, fWaitUntilDone);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteInAppDomain(ICLRRuntimeHost* iface,
    DWORD dwAppDomainId, FExecuteInAppDomainCallback pCallback, void *cookie)
{
    FIXME("(%p,%lu,%p,%p)\n", iface, dwAppDomainId, pCallback, cookie);
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
    FIXME("(%p,%s,%lu,%lu)\n", iface, debugstr_w(pwzAppFullName), dwManifestPaths, dwActivationData);
    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeHost_ExecuteInDefaultAppDomain(ICLRRuntimeHost* iface,
    LPCWSTR pwzAssemblyPath, LPCWSTR pwzTypeName, LPCWSTR pwzMethodName,
    LPCWSTR pwzArgument, DWORD *pReturnValue)
{
    RuntimeHost *This = impl_from_ICLRRuntimeHost( iface );
    HRESULT hr;
    MonoDomain *domain, *prev_domain;
    MonoObject *result;
    MonoString *str;
    char *filenameA = NULL, *classA = NULL, *methodA = NULL;
    char *argsA = NULL, *ns;

    TRACE("(%p,%s,%s,%s,%s)\n", iface, debugstr_w(pwzAssemblyPath),
        debugstr_w(pwzTypeName), debugstr_w(pwzMethodName), debugstr_w(pwzArgument));

    hr = RuntimeHost_GetDefaultDomain(This, NULL, &domain);

    if (FAILED(hr))
        return hr;

    prev_domain = domain_attach(domain);

    if (SUCCEEDED(hr))
    {
        filenameA = WtoA(pwzAssemblyPath);
        if (!filenameA) hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        classA = WtoA(pwzTypeName);
        if (!classA) hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        ns = strrchr(classA, '.');
        if (ns)
            *ns = '\0';
        else
            hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        methodA = WtoA(pwzMethodName);
        if (!methodA) hr = E_OUTOFMEMORY;
    }

    /* The .NET function we are calling has the following declaration
     *   public static int functionName(String param)
     */
    if (SUCCEEDED(hr))
    {
        argsA = WtoA(pwzArgument);
        if (!argsA) hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        str = mono_string_new(domain, argsA);
        if (!str) hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        hr = RuntimeHost_Invoke(This, domain, filenameA, classA, ns+1, methodA,
            NULL, (void**)&str, 1, &result);
    }

    if (SUCCEEDED(hr))
        *pReturnValue = *(DWORD*)mono_object_unbox(result);

    domain_restore(prev_domain);

    free(filenameA);
    free(classA);
    free(argsA);
    free(methodA);

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
    MonoDomain *prev_domain;

    if (!domain)
        hr = RuntimeHost_GetDefaultDomain(This, NULL, &domain);

    if (FAILED(hr))
        return hr;

    prev_domain = domain_attach(domain);

    if (SUCCEEDED(hr))
    {
        nameA = WtoA(name);
        if (!nameA)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr))
    {
        type = mono_reflection_type_from_name(nameA, NULL);
        if (!type)
        {
            ERR("Cannot find type %s\n", debugstr_w(name));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        klass = mono_class_from_mono_type(type);
        if (!klass)
        {
            ERR("Cannot convert type %s to a class\n", debugstr_w(name));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        obj = mono_object_new(domain, klass);
        if (!obj)
        {
            ERR("Cannot allocate object of type %s\n", debugstr_w(name));
            hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
    {
        /* FIXME: Detect exceptions from the constructor? */
        mono_runtime_object_init(obj);
        *result = obj;
    }

    domain_restore(prev_domain);

    free(nameA);

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
    MonoObject *result;
    HRESULT hr;

    domain = mono_object_get_domain(obj);

    hr = RuntimeHost_Invoke(This, domain, NULL, "System.Runtime.InteropServices", "Marshal", "GetIUnknownForObject",
        NULL, (void**)&obj, 1, &result);

    if (SUCCEEDED(hr))
        *ppUnk = *(IUnknown**)mono_object_unbox(result);
    else
        *ppUnk = NULL;

    return hr;
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

    *argv = malloc(size);
    current_arg = (char*)(*argv + *argc + 1);

    for (i=0; i<*argc; i++)
    {
        (*argv)[i] = current_arg;
        current_arg += WideCharToMultiByte(CP_UTF8, 0, argvw[i], -1, current_arg, size, NULL, NULL);
    }

    (*argv)[*argc] = NULL;

    LocalFree(argvw);
}

#if __i386__

# define CAN_FIXUP_VTABLE 1

#include "pshpack1.h"

struct vtable_fixup_thunk
{
    /* push %ecx */
    BYTE i7;
    /* sub $0x4,%esp */
    BYTE i1[3];
    /* mov fixup,(%esp) */
    BYTE i2[3];
    struct dll_fixup *fixup;
    /* mov function,%eax */
    BYTE i3;
    void (CDECL *function)(struct dll_fixup *);
    /* call *%eax */
    BYTE i4[2];
    /* pop %eax */
    BYTE i5;
    /* pop %ecx */
    BYTE i8;
    /* jmp *vtable_entry */
    BYTE i6[2];
    void *vtable_entry;
};

static const struct vtable_fixup_thunk thunk_template = {
    0x51,
    {0x83,0xec,0x04},
    {0xc7,0x04,0x24},
    NULL,
    0xb8,
    NULL,
    {0xff,0xd0},
    0x58,
    0x59,
    {0xff,0x25},
    NULL
};

#include "poppack.h"

#elif __x86_64__ /* !__i386__ */

# define CAN_FIXUP_VTABLE 1

#include "pshpack1.h"

struct vtable_fixup_thunk
{
    /* push %rbp;
       mov %rsp, %rbp
       sub $0x80, %rsp ; 0x8*4 + 0x10*4 + 0x20
    */
    BYTE i1[11];
    /*
        mov %rcx, 0x60(%rsp); mov %rdx, 0x68(%rsp); mov %r8, 0x70(%rsp); mov %r9, 0x78(%rsp);
        movaps %xmm0,0x20(%rsp); ...; movaps %xmm3,0x50(%esp)
    */
    BYTE i2[40];
    /* mov function,%rax */
    BYTE i3[2];
    void (CDECL *function)(struct dll_fixup *);
    /* mov fixup,%rcx */
    BYTE i4[2];
    struct dll_fixup *fixup;
    /* call *%rax */
    BYTE i5[2];
    /*
        mov 0x60(%rsp),%rcx; mov 0x68(%rsp),%rdx; mov 0x70(%rsp),%r8; mov 0x78(%rsp),%r9;
        movaps 0x20(%rsp),xmm0; ...; movaps 0x50(%esp),xmm3
    */
    BYTE i6[40];
    /* mov %rbp, %rsp
       pop %rbp
    */
    BYTE i7[4];
    /* mov vtable_entry, %rax */
    BYTE i8[2];
    void *vtable_entry;
    /* mov [%rax],%rax
       jmp %rax */
    BYTE i9[5];
};

static const struct vtable_fixup_thunk thunk_template = {
    {0x55,0x48,0x89,0xE5,  0x48,0x81,0xEC,0x80,0x00,0x00,0x00},
    {0x48,0x89,0x4C,0x24,0x60, 0x48,0x89,0x54,0x24,0x68,
     0x4C,0x89,0x44,0x24,0x70, 0x4C,0x89,0x4C,0x24,0x78,
     0x0F,0x29,0x44,0x24,0x20, 0x0F,0x29,0x4C,0x24,0x30,
     0x0F,0x29,0x54,0x24,0x40, 0x0F,0x29,0x5C,0x24,0x50,
    },
    {0x48,0xB8},
    NULL,
    {0x48,0xB9},
    NULL,
    {0xFF,0xD0},
    {0x48,0x8B,0x4C,0x24,0x60, 0x48,0x8B,0x54,0x24,0x68,
     0x4C,0x8B,0x44,0x24,0x70, 0x4C,0x8B,0x4C,0x24,0x78,
     0x0F,0x28,0x44,0x24,0x20, 0x0F,0x28,0x4C,0x24,0x30,
     0x0F,0x28,0x54,0x24,0x40, 0x0F,0x28,0x5C,0x24,0x50,
     },
    {0x48,0x89,0xEC, 0x5D},
    {0x48,0xB8},
    NULL,
    {0x48,0x8B,0x00,0xFF,0xE0}
};

#include "poppack.h"

#else /* !__i386__ && !__x86_64__ */

# define CAN_FIXUP_VTABLE 0

struct vtable_fixup_thunk
{
    struct dll_fixup *fixup;
    void (CDECL *function)(struct dll_fixup *fixup);
    void *vtable_entry;
};

static const struct vtable_fixup_thunk thunk_template = {0};

#endif

DWORD WINAPI GetTokenForVTableEntry(HINSTANCE hinst, BYTE **ppVTEntry)
{
    struct dll_fixup *fixup;
    DWORD result = 0;
    DWORD rva;
    int i;

    TRACE("%p,%p\n", hinst, ppVTEntry);

    rva = (BYTE*)ppVTEntry - (BYTE*)hinst;

    EnterCriticalSection(&fixup_list_cs);
    LIST_FOR_EACH_ENTRY(fixup, &dll_fixups, struct dll_fixup, entry)
    {
        if (fixup->dll != hinst)
            continue;
        if (rva < fixup->fixup->rva || (rva - fixup->fixup->rva >= fixup->fixup->count * sizeof(ULONG_PTR)))
            continue;
        i = (rva - fixup->fixup->rva) / sizeof(ULONG_PTR);
        result = ((ULONG_PTR*)fixup->tokens)[i];
        break;
    }
    LeaveCriticalSection(&fixup_list_cs);

    TRACE("<-- %lx\n", result);
    return result;
}

static void CDECL ReallyFixupVTable(struct dll_fixup *fixup)
{
    HRESULT hr=S_OK;
    WCHAR filename[MAX_PATH];
    ICLRRuntimeInfo *info=NULL;
    RuntimeHost *host;
    char *filenameA;
    MonoImage *image=NULL;
    MonoAssembly *assembly=NULL;
    MonoImageOpenStatus status=0;
    MonoDomain *domain;

    if (fixup->done) return;

    /* It's possible we'll have two threads doing this at once. This is
     * considered preferable to the potential deadlock if we use a mutex. */

    GetModuleFileNameW(fixup->dll, filename, MAX_PATH);

    TRACE("%p,%p,%s\n", fixup, fixup->dll, debugstr_w(filename));

    filenameA = WtoA(filename);
    if (!filenameA)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
        hr = get_runtime_info(filename, NULL, NULL, NULL, 0, 0, FALSE, &info);

    if (SUCCEEDED(hr))
        hr = ICLRRuntimeInfo_GetRuntimeHost(info, &host);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_GetDefaultDomain(host, NULL, &domain);

    if (SUCCEEDED(hr))
    {
        MonoDomain *prev_domain;

        prev_domain = domain_attach(domain);

        assembly = mono_assembly_open(filenameA, &status);

        if (assembly)
        {
            int i;

            /* Mono needs an image that belongs to an assembly. */
            image = mono_assembly_get_image(assembly);

#if __x86_64__
            if (fixup->fixup->type & COR_VTABLE_64BIT)
#else
            if (fixup->fixup->type & COR_VTABLE_32BIT)
#endif
            {
                void **vtable = fixup->vtable;
                ULONG_PTR *tokens = fixup->tokens;
                for (i=0; i<fixup->fixup->count; i++)
                {
                    vtable[i] = mono_marshal_get_vtfixup_ftnptr(
                        image, tokens[i], fixup->fixup->type);
                }
            }

            fixup->done = TRUE;
        }

        domain_restore(prev_domain);
    }

    if (info != NULL)
        ICLRRuntimeInfo_Release(info);

    free(filenameA);

    if (!fixup->done)
    {
        ERR("unable to fixup vtable, hr=%lx, status=%d\n", hr, status);
        /* If we returned now, we'd get an infinite loop. */
        assert(0);
    }
}

static void FixupVTableEntry(HMODULE hmodule, VTableFixup *vtable_fixup)
{
    /* We can't actually generate code for the functions without loading mono,
     * and loading mono inside DllMain is a terrible idea. So we make thunks
     * that call ReallyFixupVTable, which will load the runtime and fill in the
     * vtable, then do an indirect jump using the (now filled in) vtable. Note
     * that we have to keep the thunks around forever, as one of them may get
     * called while we're filling in the table, and we can never be sure all
     * threads are clear. */
    struct dll_fixup *fixup;

    fixup = malloc(sizeof(*fixup));

    fixup->dll = hmodule;
    fixup->thunk_code = HeapAlloc(dll_fixup_heap, 0, sizeof(struct vtable_fixup_thunk) * vtable_fixup->count);
    fixup->fixup = vtable_fixup;
    fixup->vtable = (BYTE*)hmodule + vtable_fixup->rva;
    fixup->done = FALSE;

#if __x86_64__
    if (vtable_fixup->type & COR_VTABLE_64BIT)
#else
    if (vtable_fixup->type & COR_VTABLE_32BIT)
#endif
    {
        void **vtable = fixup->vtable;
        ULONG_PTR *tokens;
        int i;
        struct vtable_fixup_thunk *thunks = fixup->thunk_code;

        tokens = fixup->tokens = malloc(sizeof(*tokens) * vtable_fixup->count);
        memcpy(tokens, vtable, sizeof(*tokens) * vtable_fixup->count);
        for (i=0; i<vtable_fixup->count; i++)
        {
            thunks[i] = thunk_template;
            thunks[i].fixup = fixup;
            thunks[i].function = ReallyFixupVTable;
            thunks[i].vtable_entry = &vtable[i];
            vtable[i] = &thunks[i];
        }
    }
    else
    {
        ERR("unsupported vtable fixup flags %x\n", vtable_fixup->type);
        HeapFree(dll_fixup_heap, 0, fixup->thunk_code);
        free(fixup);
        return;
    }

    EnterCriticalSection(&fixup_list_cs);
    list_add_tail(&dll_fixups, &fixup->entry);
    LeaveCriticalSection(&fixup_list_cs);
}

static void FixupVTable_Assembly(HMODULE hmodule, ASSEMBLY *assembly)
{
    VTableFixup *vtable_fixups;
    ULONG vtable_fixup_count, i;

    assembly_get_vtable_fixups(assembly, &vtable_fixups, &vtable_fixup_count);
    if (CAN_FIXUP_VTABLE)
        for (i=0; i<vtable_fixup_count; i++)
            FixupVTableEntry(hmodule, &vtable_fixups[i]);
    else if (vtable_fixup_count)
        FIXME("cannot fixup vtable; expect a crash\n");
}

static void FixupVTable(HMODULE hmodule)
{
    ASSEMBLY *assembly;
    HRESULT hr;

    hr = assembly_from_hmodule(&assembly, hmodule);
    if (SUCCEEDED(hr))
    {
        FixupVTable_Assembly(hmodule, assembly);
        assembly_release(assembly);
    }
    else
        ERR("failed to read CLR headers, hr=%lx\n", hr);
}

__int32 WINAPI _CorExeMain(void)
{
    static const WCHAR dotconfig[] = {'.','c','o','n','f','i','g',0};
    static const WCHAR scW[] = {';',0};
    int exit_code;
    int argc;
    char **argv;
    MonoDomain *domain=NULL;
    MonoImage *image;
    MonoImageOpenStatus status;
    MonoAssembly *assembly=NULL;
    WCHAR filename[MAX_PATH], config_file[MAX_PATH], *temp, **priv_path;
    SIZE_T config_file_dir_size;
    char *filenameA;
    ICLRRuntimeInfo *info;
    RuntimeHost *host;
    parsed_config_file parsed_config;
    HRESULT hr;
    int i, number_of_private_paths = 0;

    get_utf8_args(&argc, &argv);

    GetModuleFileNameW(NULL, filename, MAX_PATH);

    TRACE("%s argc=%i\n", debugstr_w(filename), argc);

    filenameA = WtoA(filename);
    if (!filenameA)
    {
        free(argv);
        return -1;
    }

    FixupVTable(GetModuleHandleW(NULL));

    wcscpy(config_file, filename);
    wcscat(config_file, dotconfig);

    hr = parse_config_file(config_file, &parsed_config);
    if (SUCCEEDED(hr) && parsed_config.private_path && parsed_config.private_path[0])
    {
#ifndef __REACTOS__
        WCHAR *save;
#endif
        for(i = 0; parsed_config.private_path[i] != 0; i++)
            if (parsed_config.private_path[i] == ';') number_of_private_paths++;
        if (parsed_config.private_path[wcslen(parsed_config.private_path) - 1] != ';') number_of_private_paths++;
        config_file_dir_size = (wcsrchr(config_file, '\\') - config_file) + 1;
        priv_path = malloc((number_of_private_paths + 1) * sizeof(WCHAR *));
        /* wcstok ignores trailing semicolons */
#ifdef __REACTOS__
        temp = wcstok(parsed_config.private_path, scW);
#else
        temp = wcstok_s(parsed_config.private_path, scW, &save);
#endif
        for (i = 0; i < number_of_private_paths; i++)
        {
            priv_path[i] = malloc((config_file_dir_size + wcslen(temp) + 1) * sizeof(WCHAR));
            memcpy(priv_path[i], config_file, config_file_dir_size * sizeof(WCHAR));
            wcscpy(priv_path[i] + config_file_dir_size, temp);
#ifdef __REACTOS__
            temp = wcstok(NULL, scW);
#else
            temp = wcstok_s(NULL, scW, &save);
#endif
        }
        priv_path[number_of_private_paths] = NULL;
        if (InterlockedCompareExchangePointer((void **)&private_path, priv_path, NULL))
            ERR("private_path was already set\n");
    }

    free_parsed_config_file(&parsed_config);

    hr = get_runtime_info(filename, NULL, NULL, NULL, 0, 0, FALSE, &info);

    if (SUCCEEDED(hr))
    {
        hr = ICLRRuntimeInfo_GetRuntimeHost(info, &host);

        if (SUCCEEDED(hr))
            hr = RuntimeHost_GetDefaultDomain(host, config_file, &domain);

        if (SUCCEEDED(hr))
        {
            image = mono_image_open_from_module_handle(GetModuleHandleW(NULL),
                filenameA, 1, &status);

            if (image)
                assembly = mono_assembly_load_from(image, filenameA, &status);

            if (assembly)
            {
                mono_callspec_set_assembly(assembly);

                exit_code = mono_jit_exec(domain, assembly, argc, argv);
            }
            else
            {
                ERR("couldn't load %s, status=%d\n", debugstr_w(filename), status);
                exit_code = -1;
            }
        }
        else
            exit_code = -1;

        ICLRRuntimeInfo_Release(info);
    }
    else
        exit_code = -1;

    free(argv);

    if (domain)
    {
        mono_thread_manage();
        mono_runtime_quit();
    }

    ExitProcess(exit_code);

    return exit_code;
}

BOOL WINAPI _CorDllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    ASSEMBLY *assembly=NULL;
    HRESULT hr;

    TRACE("(%p, %ld, %p)\n", hinstDLL, fdwReason, lpvReserved);

    hr = assembly_from_hmodule(&assembly, hinstDLL);
    if (SUCCEEDED(hr))
    {
        NativeEntryPointFunc NativeEntryPoint=NULL;

        assembly_get_native_entrypoint(assembly, &NativeEntryPoint);
        if (fdwReason == DLL_PROCESS_ATTACH)
        {
            if (!NativeEntryPoint)
                DisableThreadLibraryCalls(hinstDLL);
            FixupVTable_Assembly(hinstDLL,assembly);
        }
        assembly_release(assembly);
        /* FIXME: clean up the vtables on DLL_PROCESS_DETACH */
        if (NativeEntryPoint)
            return NativeEntryPoint(hinstDLL, fdwReason, lpvReserved);
    }
    else
        ERR("failed to read CLR headers, hr=%lx\n", hr);

    return TRUE;
}

/* called from DLL_PROCESS_ATTACH */
void runtimehost_init(void)
{
    dll_fixup_heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
    list_init(&dll_fixups);
}

/* called from DLL_PROCESS_DETACH */
void runtimehost_uninit(void)
{
    struct dll_fixup *fixup, *fixup2;

    HeapDestroy(dll_fixup_heap);
    LIST_FOR_EACH_ENTRY_SAFE(fixup, fixup2, &dll_fixups, struct dll_fixup, entry)
    {
        free(fixup->tokens);
        free(fixup);
    }
}

HRESULT RuntimeHost_Construct(CLRRuntimeInfo *runtime_version, RuntimeHost** result)
{
    RuntimeHost *This;

    This = malloc(sizeof *This);
    if ( !This )
        return E_OUTOFMEMORY;

    This->ICorRuntimeHost_iface.lpVtbl = &corruntimehost_vtbl;
    This->ICLRRuntimeHost_iface.lpVtbl = &CLRHostVtbl;

    This->ref = 1;
    This->version = runtime_version;
    InitializeCriticalSectionEx(&This->lock, 0, RTL_CRITICAL_SECTION_FLAG_FORCE_DEBUG_INFO);
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

static BOOL try_create_registration_free_com(REFIID clsid, WCHAR *classname, UINT classname_size, WCHAR *filename, UINT filename_size)
{
    ACTCTX_SECTION_KEYED_DATA guid_info = { sizeof(ACTCTX_SECTION_KEYED_DATA) };
    ACTIVATION_CONTEXT_ASSEMBLY_DETAILED_INFORMATION *assembly_info = NULL;
    SIZE_T bytes_assembly_info;
    struct comclassredirect_data *redirect_data;
    struct clrclass_data *class_data;
    void *ptr_name;
    const WCHAR *ptr_path_start, *ptr_path_end;
    WCHAR path[MAX_PATH] = {0};
    WCHAR str_dll[] = {'.','d','l','l',0};
    BOOL ret = FALSE;

    if (!FindActCtxSectionGuid(FIND_ACTCTX_SECTION_KEY_RETURN_HACTCTX, 0, ACTIVATION_CONTEXT_SECTION_COM_SERVER_REDIRECTION, clsid, &guid_info))
    {
        DWORD error = GetLastError();
        if (error != ERROR_SXS_KEY_NOT_FOUND)
            ERR("Failed to find guid: %ld\n", error);
        goto end;
    }

    QueryActCtxW(0, guid_info.hActCtx, &guid_info.ulAssemblyRosterIndex, AssemblyDetailedInformationInActivationContext, NULL, 0, &bytes_assembly_info);
    assembly_info = malloc(bytes_assembly_info);
    if (!QueryActCtxW(0, guid_info.hActCtx, &guid_info.ulAssemblyRosterIndex,
            AssemblyDetailedInformationInActivationContext, assembly_info, bytes_assembly_info, &bytes_assembly_info))
    {
        ERR("QueryActCtxW failed: %ld!\n", GetLastError());
        goto end;
    }

    redirect_data = guid_info.lpData;
    class_data = (void *)((char *)redirect_data + redirect_data->clrdata_offset);

    ptr_name = (char *)class_data + class_data->name_offset;
    if (lstrlenW(ptr_name) + 1 > classname_size) /* Include null-terminator */
    {
        ERR("Buffer is too small\n");
        goto end;
    }
    lstrcpyW(classname, ptr_name);

    ptr_path_start = assembly_info->lpAssemblyEncodedAssemblyIdentity;
    ptr_path_end = wcschr(ptr_path_start, ',');
    memcpy(path, ptr_path_start, (char*)ptr_path_end - (char*)ptr_path_start);

    GetModuleFileNameW(NULL, filename, filename_size);
    PathRemoveFileSpecW(filename);

    if (lstrlenW(filename) + lstrlenW(path) + ARRAY_SIZE(str_dll) + 1 > filename_size) /* Include blackslash */
    {
        ERR("Buffer is too small\n");
        goto end;
    }

    PathAppendW(filename, path);
    lstrcatW(filename, str_dll);

    ret = TRUE;

end:
    free(assembly_info);

    if (guid_info.hActCtx)
        ReleaseActCtx(guid_info.hActCtx);

    return ret;
}

#define CHARS_IN_GUID 39

HRESULT create_monodata(REFCLSID clsid, LPVOID *ppObj)
{
    static const WCHAR wszFileSlash[] = L"file:///";
    static const WCHAR wszCLSIDSlash[] = L"CLSID\\";
    static const WCHAR wszInprocServer32[] = L"\\InprocServer32";
    WCHAR path[CHARS_IN_GUID + ARRAY_SIZE(wszCLSIDSlash) + ARRAY_SIZE(wszInprocServer32) - 1];
    MonoDomain *domain;
    MonoAssembly *assembly;
    ICLRRuntimeInfo *info = NULL;
    RuntimeHost *host;
    HRESULT hr;
    HKEY key, subkey;
    LONG res;
    int offset = 0;
    HANDLE file = INVALID_HANDLE_VALUE;
    DWORD numKeys, keyLength;
    WCHAR codebase[MAX_PATH + 8];
    WCHAR classname[350], subkeyName[256];
    WCHAR filename[MAX_PATH];
    DWORD dwBufLen;

    lstrcpyW(path, wszCLSIDSlash);
    StringFromGUID2(clsid, path + lstrlenW(wszCLSIDSlash), CHARS_IN_GUID);
    lstrcatW(path, wszInprocServer32);

    TRACE("Registry key: %s\n", debugstr_w(path));

    res = RegOpenKeyExW(HKEY_CLASSES_ROOT, path, 0, KEY_READ, &key);
    if (res != ERROR_FILE_NOT_FOUND)
    {
        res = RegOpenKeyExW( key, L"Server", 0, KEY_READ, &subkey );
        if (res == ERROR_SUCCESS)
        {
            /* Not a managed class, just chain through LoadLibraryShim */
            HMODULE module;
            HRESULT (WINAPI *pDllGetClassObject)(REFCLSID,REFIID,LPVOID*);
            IClassFactory *classfactory;

            dwBufLen = sizeof( filename );
            res = RegGetValueW( subkey, NULL, NULL, RRF_RT_REG_SZ, NULL, filename, &dwBufLen );

            RegCloseKey( subkey );

            if (res != ERROR_SUCCESS)
            {
                WARN("Can't read default value from Server subkey.\n");
                hr = CLASS_E_CLASSNOTAVAILABLE;
                goto cleanup;
            }

            hr = LoadLibraryShim( filename, L"v4.0.30319", NULL, &module);
            if (FAILED(hr))
            {
                WARN("Can't load %s.\n", debugstr_w(filename));
                goto cleanup;
            }

            pDllGetClassObject = (void*)GetProcAddress( module, "DllGetClassObject" );
            if (!pDllGetClassObject)
            {
                WARN("Can't get DllGetClassObject from %s.\n", debugstr_w(filename));
                hr = CLASS_E_CLASSNOTAVAILABLE;
                goto cleanup;
            }

            hr = pDllGetClassObject( clsid, &IID_IClassFactory, (void**)&classfactory );
            if (SUCCEEDED(hr))
            {
                hr = IClassFactory_CreateInstance( classfactory, NULL, &IID_IUnknown, ppObj );

                IClassFactory_Release( classfactory );
            }

            goto cleanup;
        }

        dwBufLen = sizeof( classname );
        res = RegGetValueW( key, NULL, L"Class", RRF_RT_REG_SZ, NULL, classname, &dwBufLen);
        if(res != ERROR_SUCCESS)
        {
            WARN("Class value cannot be found.\n");
            hr = CLASS_E_CLASSNOTAVAILABLE;
            goto cleanup;
        }

        TRACE("classname (%s)\n", debugstr_w(classname));

        dwBufLen = sizeof( codebase );
        res = RegGetValueW( key, NULL, L"CodeBase", RRF_RT_REG_SZ, NULL, codebase, &dwBufLen);
        if(res == ERROR_SUCCESS)
        {
            /* Strip file:/// */
            if(wcsncmp(codebase, wszFileSlash, lstrlenW(wszFileSlash)) == 0)
                offset = lstrlenW(wszFileSlash);

            lstrcpyW(filename, codebase + offset);

            file = CreateFileW(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
        }

        if (file != INVALID_HANDLE_VALUE)
            CloseHandle(file);
        else
        {
            WCHAR assemblyname[MAX_PATH + 8];

            hr = CLASS_E_CLASSNOTAVAILABLE;
            WARN("CodeBase value cannot be found, trying Assembly.\n");
            /* get the last subkey of InprocServer32 */
            res = RegQueryInfoKeyW(key, 0, 0, 0, &numKeys, 0, 0, 0, 0, 0, 0, 0);
            if (res != ERROR_SUCCESS)
                goto cleanup;
            if (numKeys > 0)
            {
                numKeys--;
                keyLength = ARRAY_SIZE(subkeyName);
                res = RegEnumKeyExW(key, numKeys, subkeyName, &keyLength, 0, 0, 0, 0);
                if (res != ERROR_SUCCESS)
                    goto cleanup;
                res = RegOpenKeyExW(key, subkeyName, 0, KEY_READ, &subkey);
                if (res != ERROR_SUCCESS)
                    goto cleanup;
                dwBufLen = sizeof( assemblyname );
                res = RegGetValueW(subkey, NULL, L"Assembly", RRF_RT_REG_SZ, NULL, assemblyname, &dwBufLen);
                RegCloseKey(subkey);
                if (res != ERROR_SUCCESS)
                    goto cleanup;
            }
            else
            {
                dwBufLen = sizeof( assemblyname );
                res = RegGetValueW(key, NULL, L"Assembly", RRF_RT_REG_SZ, NULL, assemblyname, &dwBufLen);
                if (res != ERROR_SUCCESS)
                    goto cleanup;
            }

            hr = get_file_from_strongname(assemblyname, filename, MAX_PATH);
            if (FAILED(hr))
            {
                /*
                 * The registry doesn't have a CodeBase entry or the file isn't there, and it's not in the GAC.
                 *
                 * Use the Assembly Key to retrieve the filename.
                 *    Assembly : REG_SZ : AssemblyName, Version=X.X.X.X, Culture=neutral, PublicKeyToken=null
                 */
                WCHAR *ns;

                WARN("Attempt to load from the application directory.\n");
                GetModuleFileNameW(NULL, filename, MAX_PATH);
                ns = wcsrchr(filename, '\\');
                *(ns+1) = '\0';

                ns = wcschr(assemblyname, ',');
                *(ns) = '\0';
                lstrcatW(filename, assemblyname);
                *(ns) = '.';
                lstrcatW(filename, L".dll");
            }
        }
    }
    else
    {
        if (!try_create_registration_free_com(clsid, classname, ARRAY_SIZE(classname), filename, ARRAY_SIZE(filename)))
            return CLASS_E_CLASSNOTAVAILABLE;

        TRACE("classname (%s)\n", debugstr_w(classname));
    }

    TRACE("filename (%s)\n", debugstr_w(filename));

    *ppObj = NULL;


    hr = get_runtime_info(filename, NULL, NULL, NULL, 0, 0, FALSE, &info);
    if (SUCCEEDED(hr))
    {
        hr = ICLRRuntimeInfo_GetRuntimeHost(info, &host);

        if (SUCCEEDED(hr))
            hr = RuntimeHost_GetDefaultDomain(host, NULL, &domain);

        if (SUCCEEDED(hr))
        {
            MonoImage *image;
            MonoClass *klass;
            MonoObject *result;
            MonoDomain *prev_domain;
            MonoImageOpenStatus status;
            IUnknown *unk = NULL;
            char *filenameA, *ns;
            char *classA;

            hr = CLASS_E_CLASSNOTAVAILABLE;

            prev_domain = domain_attach(domain);

            filenameA = WtoA(filename);
            assembly = mono_assembly_open(filenameA, &status);
            free(filenameA);
            if (!assembly)
            {
                ERR("Cannot open assembly %s, status=%i\n", debugstr_w(filename), status);
                domain_restore(prev_domain);
                goto cleanup;
            }

            image = mono_assembly_get_image(assembly);
            if (!image)
            {
                ERR("Couldn't get assembly image\n");
                domain_restore(prev_domain);
                goto cleanup;
            }

            classA = WtoA(classname);
            ns = strrchr(classA, '.');
            *ns = '\0';

            klass = mono_class_from_name(image, classA, ns+1);
            free(classA);
            if (!klass)
            {
                ERR("Couldn't get class from image\n");
                domain_restore(prev_domain);
                goto cleanup;
            }

            /*
             * Use the default constructor for the .NET class.
             */
            result = mono_object_new(domain, klass);
            mono_runtime_object_init(result);

            hr = RuntimeHost_GetIUnknownForObject(host, result, &unk);
            if (SUCCEEDED(hr))
            {
                hr = IUnknown_QueryInterface(unk, &IID_IUnknown, ppObj);

                IUnknown_Release(unk);
            }
            else
                hr = CLASS_E_CLASSNOTAVAILABLE;

            domain_restore(prev_domain);
        }
        else
            hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    else
        hr = CLASS_E_CLASSNOTAVAILABLE;

cleanup:
    if(info)
        ICLRRuntimeInfo_Release(info);

    RegCloseKey(key);

    return hr;
}
