/*
 * ICLRMetaHost - discovery and management of available .NET runtimes
 *
 * Copyright 2010 Vincent Povirk for CodeWeavers
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

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define COBJMACROS

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winternl.h"
#include "ole2.h"
#include "shlwapi.h"

#include "corerror.h"
#include "cor.h"
#include "mscoree.h"
#include "corhdr.h"
#include "cordebug.h"
#include "metahost.h"
#include "fusion.h"
#include "wine/list.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

static const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl;

#define NUM_RUNTIMES 4

static struct CLRRuntimeInfo runtimes[NUM_RUNTIMES] = {
    {{&CLRRuntimeInfoVtbl}, 1, 0, 3705, 0},
    {{&CLRRuntimeInfoVtbl}, 1, 1, 4322, 0},
    {{&CLRRuntimeInfoVtbl}, 2, 0, 50727, 0},
    {{&CLRRuntimeInfoVtbl}, 4, 0, 30319, 0}
};

static CRITICAL_SECTION runtime_list_cs;
static CRITICAL_SECTION_DEBUG runtime_list_cs_debug =
{
    0, 0, &runtime_list_cs,
    { &runtime_list_cs_debug.ProcessLocksList,
      &runtime_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": runtime_list_cs") }
};
static CRITICAL_SECTION runtime_list_cs = { &runtime_list_cs_debug, -1, 0, 0, 0, 0 };

struct CLRMetaHost
{
    ICLRMetaHost ICLRMetaHost_iface;

    RuntimeLoadedCallbackFnPtr callback;
};

static struct CLRMetaHost GlobalCLRMetaHost;

static HMODULE mono_handle;

BOOL is_mono_started;
static BOOL is_mono_shutdown;

typedef struct _MonoProfilerDesc *MonoProfilerHandle;

typedef void (CDECL *MonoProfilerRuntimeShutdownBeginCallback) (MonoProfiler *prof);

MonoImage* (CDECL *mono_assembly_get_image)(MonoAssembly *assembly);
MonoAssembly* (CDECL *mono_assembly_load_from)(MonoImage *image, const char *fname, MonoImageOpenStatus *status);
const char* (CDECL *mono_assembly_name_get_name)(MonoAssemblyName *aname);
const char* (CDECL *mono_assembly_name_get_culture)(MonoAssemblyName *aname);
WORD (CDECL *mono_assembly_name_get_version)(MonoAssemblyName *aname, WORD *minor, WORD *build, WORD *revision);
MonoAssembly* (CDECL *mono_assembly_open)(const char *filename, MonoImageOpenStatus *status);
void (CDECL *mono_callspec_set_assembly)(MonoAssembly *assembly);
MonoClass* (CDECL *mono_class_from_mono_type)(MonoType *type);
MonoClass* (CDECL *mono_class_from_name)(MonoImage *image, const char* name_space, const char *name);
MonoMethod* (CDECL *mono_class_get_method_from_name)(MonoClass *klass, const char *name, int param_count);
static void (CDECL *mono_config_parse)(const char *filename);
MonoDomain* (CDECL *mono_domain_get)(void);
MonoDomain* (CDECL *mono_domain_get_by_id)(int id);
BOOL (CDECL *mono_domain_set)(MonoDomain *domain,BOOL force);
void (CDECL *mono_domain_set_config)(MonoDomain *domain,const char *base_dir,const char *config_file_name);
static void (CDECL *mono_free)(void *);
MonoImage* (CDECL *mono_get_corlib)(void);
static MonoImage* (CDECL *mono_image_open)(const char *fname, MonoImageOpenStatus *status);
MonoImage* (CDECL *mono_image_open_from_module_handle)(HMODULE module_handle, char* fname, UINT has_entry_point, MonoImageOpenStatus* status);
static void (CDECL *mono_install_assembly_preload_hook)(MonoAssemblyPreLoadFunc func, void *user_data);
int (CDECL *mono_jit_exec)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]);
MonoDomain* (CDECL *mono_jit_init_version)(const char *domain_name, const char *runtime_version);
static void (CDECL *mono_jit_set_aot_mode)(MonoAotMode mode);
static int (CDECL *mono_jit_set_trace_options)(const char* options);
void* (CDECL *mono_marshal_get_vtfixup_ftnptr)(MonoImage *image, DWORD token, WORD type);
MonoDomain* (CDECL *mono_object_get_domain)(MonoObject *obj);
MonoMethod* (CDECL *mono_object_get_virtual_method)(MonoObject *obj, MonoMethod *method);
MonoObject* (CDECL *mono_object_new)(MonoDomain *domain, MonoClass *klass);
void* (CDECL *mono_object_unbox)(MonoObject *obj);
static MonoProfilerHandle (CDECL *mono_profiler_create)(MonoProfiler *prof);
static void (CDECL *mono_profiler_install)(MonoProfiler *prof, MonoProfileFunc shutdown_callback);
static void (CDECL *mono_profiler_set_runtime_shutdown_begin_callback)(MonoProfilerHandle handle, MonoProfilerRuntimeShutdownBeginCallback cb);
MonoType* (CDECL *mono_reflection_type_from_name)(char *name, MonoImage *image);
MonoObject* (CDECL *mono_runtime_invoke)(MonoMethod *method, void *obj, void **params, MonoObject **exc);
void (CDECL *mono_runtime_object_init)(MonoObject *this_obj);
void (CDECL *mono_runtime_quit)(void);
static void (CDECL *mono_set_crash_chaining)(BOOL chain_signals);
static void (CDECL *mono_set_dirs)(const char *assembly_dir, const char *config_dir);
static void (CDECL *mono_set_verbose_level)(DWORD level);
MonoString* (CDECL *mono_string_new)(MonoDomain *domain, const char *str);
static char* (CDECL *mono_stringify_assembly_name)(MonoAssemblyName *aname);
MonoThread* (CDECL *mono_thread_attach)(MonoDomain *domain);
void (CDECL *mono_thread_manage)(void);
void (CDECL *mono_trace_set_print_handler)(MonoPrintCallback callback);
void (CDECL *mono_trace_set_printerr_handler)(MonoPrintCallback callback);
void (CDECL *mono_trace_set_log_handler)(MonoLogCallback callback, void *user_data);
static MonoAssembly* (CDECL *wine_mono_assembly_load_from_gac)(MonoAssemblyName *aname, MonoImageOpenStatus *status, int refonly);
static void (CDECL *wine_mono_install_assembly_preload_hook)(WineMonoAssemblyPreLoadFunc func, void *user_data);
static void (CDECL *wine_mono_install_assembly_preload_hook_v2)(WineMonoAssemblyPreLoadFunc func, void *user_data);

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path);

static MonoAssembly* CDECL mono_assembly_preload_hook_fn(MonoAssemblyName *aname, char **assemblies_path, void *user_data);
static MonoAssembly* CDECL wine_mono_assembly_preload_hook_fn(MonoAssemblyName *aname, char **assemblies_path, int *search_path, void *user_data);
static MonoAssembly* CDECL wine_mono_assembly_preload_hook_v2_fn(MonoAssemblyName *aname, char **assemblies_path, int *flags, void *user_data);

static void CDECL mono_shutdown_callback_fn(MonoProfiler *prof);

static MonoImage* CDECL image_open_module_handle_dummy(HMODULE module_handle,
    char* fname, UINT has_entry_point, MonoImageOpenStatus* status)
{
    return mono_image_open(fname, status);
}

static void CDECL set_crash_chaining_dummy(BOOL crash_chaining)
{
}

static void CDECL set_print_handler_dummy(MonoPrintCallback callback)
{
}

static void CDECL set_log_handler_dummy (MonoLogCallback callback, void *user_data)
{
}

static HRESULT load_mono(LPCWSTR mono_path)
{
    static const WCHAR lib[] = {'\\','l','i','b',0};
    static const WCHAR etc[] = {'\\','e','t','c',0};
    WCHAR mono_dll_path[MAX_PATH+16];
    WCHAR mono_lib_path[MAX_PATH+4], mono_etc_path[MAX_PATH+4];
    char mono_lib_path_a[MAX_PATH], mono_etc_path_a[MAX_PATH];
    int aot_size;
    char aot_setting[256];
    int trace_size;
    char trace_setting[256];
    int verbose_size;
    char verbose_setting[256];

    if (is_mono_shutdown)
    {
        ERR("Cannot load Mono after it has been shut down.\n");
        return E_FAIL;
    }

    if (!mono_handle)
    {
        lstrcpyW(mono_lib_path, mono_path);
        lstrcatW(mono_lib_path, lib);
        WideCharToMultiByte(CP_UTF8, 0, mono_lib_path, -1, mono_lib_path_a, MAX_PATH, NULL, NULL);

        lstrcpyW(mono_etc_path, mono_path);
        lstrcatW(mono_etc_path, etc);
        WideCharToMultiByte(CP_UTF8, 0, mono_etc_path, -1, mono_etc_path_a, MAX_PATH, NULL, NULL);

        if (!find_mono_dll(mono_path, mono_dll_path)) goto fail;

        mono_handle = LoadLibraryW(mono_dll_path);

        if (!mono_handle) goto fail;

#define LOAD_MONO_FUNCTION(x) do { \
    x = (void*)GetProcAddress(mono_handle, #x); \
    if (!x) { \
        goto fail; \
    } \
} while (0);

        LOAD_MONO_FUNCTION(mono_assembly_get_image);
        LOAD_MONO_FUNCTION(mono_assembly_load_from);
        LOAD_MONO_FUNCTION(mono_assembly_name_get_name);
        LOAD_MONO_FUNCTION(mono_assembly_name_get_culture);
        LOAD_MONO_FUNCTION(mono_assembly_name_get_version);
        LOAD_MONO_FUNCTION(mono_assembly_open);
        LOAD_MONO_FUNCTION(mono_config_parse);
        LOAD_MONO_FUNCTION(mono_class_from_mono_type);
        LOAD_MONO_FUNCTION(mono_class_from_name);
        LOAD_MONO_FUNCTION(mono_class_get_method_from_name);
        LOAD_MONO_FUNCTION(mono_domain_get);
        LOAD_MONO_FUNCTION(mono_domain_get_by_id);
        LOAD_MONO_FUNCTION(mono_domain_set);
        LOAD_MONO_FUNCTION(mono_domain_set_config);
        LOAD_MONO_FUNCTION(mono_free);
        LOAD_MONO_FUNCTION(mono_get_corlib);
        LOAD_MONO_FUNCTION(mono_image_open);
        LOAD_MONO_FUNCTION(mono_install_assembly_preload_hook);
        LOAD_MONO_FUNCTION(mono_jit_exec);
        LOAD_MONO_FUNCTION(mono_jit_init_version);
        LOAD_MONO_FUNCTION(mono_jit_set_trace_options);
        LOAD_MONO_FUNCTION(mono_marshal_get_vtfixup_ftnptr);
        LOAD_MONO_FUNCTION(mono_object_get_domain);
        LOAD_MONO_FUNCTION(mono_object_get_virtual_method);
        LOAD_MONO_FUNCTION(mono_object_new);
        LOAD_MONO_FUNCTION(mono_object_unbox);
        LOAD_MONO_FUNCTION(mono_reflection_type_from_name);
        LOAD_MONO_FUNCTION(mono_runtime_invoke);
        LOAD_MONO_FUNCTION(mono_runtime_object_init);
        LOAD_MONO_FUNCTION(mono_runtime_quit);
        LOAD_MONO_FUNCTION(mono_set_dirs);
        LOAD_MONO_FUNCTION(mono_set_verbose_level);
        LOAD_MONO_FUNCTION(mono_stringify_assembly_name);
        LOAD_MONO_FUNCTION(mono_string_new);
        LOAD_MONO_FUNCTION(mono_thread_attach);
        LOAD_MONO_FUNCTION(mono_thread_manage);

#undef LOAD_MONO_FUNCTION

#define LOAD_OPT_MONO_FUNCTION(x, default) do { \
    x = (void*)GetProcAddress(mono_handle, #x); \
    if (!x) { \
        x = default; \
    } \
} while (0);

        LOAD_OPT_MONO_FUNCTION(mono_callspec_set_assembly, NULL);
        LOAD_OPT_MONO_FUNCTION(mono_image_open_from_module_handle, image_open_module_handle_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_jit_set_aot_mode, NULL);
        LOAD_OPT_MONO_FUNCTION(mono_profiler_create, NULL);
        LOAD_OPT_MONO_FUNCTION(mono_profiler_install, NULL);
        LOAD_OPT_MONO_FUNCTION(mono_profiler_set_runtime_shutdown_begin_callback, NULL);
        LOAD_OPT_MONO_FUNCTION(mono_set_crash_chaining, set_crash_chaining_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_trace_set_print_handler, set_print_handler_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_trace_set_printerr_handler, set_print_handler_dummy);
        LOAD_OPT_MONO_FUNCTION(mono_trace_set_log_handler, set_log_handler_dummy);
        LOAD_OPT_MONO_FUNCTION(wine_mono_assembly_load_from_gac, NULL);
        LOAD_OPT_MONO_FUNCTION(wine_mono_install_assembly_preload_hook, NULL);
        LOAD_OPT_MONO_FUNCTION(wine_mono_install_assembly_preload_hook_v2, NULL);

#undef LOAD_OPT_MONO_FUNCTION

        if (mono_callspec_set_assembly == NULL)
        {
            mono_callspec_set_assembly = (void*)GetProcAddress(mono_handle, "mono_trace_set_assembly");
            if (!mono_callspec_set_assembly) goto fail;
        }

        if (mono_profiler_create != NULL)
        {
            /* Profiler API v2 */
            MonoProfilerHandle handle = mono_profiler_create(NULL);
            mono_profiler_set_runtime_shutdown_begin_callback(handle, mono_shutdown_callback_fn);
        }
        else if (mono_profiler_install != NULL)
        {
            /* Profiler API v1 */
            mono_profiler_install(NULL, mono_shutdown_callback_fn);
        }

        mono_set_crash_chaining(TRUE);

        mono_trace_set_print_handler(mono_print_handler_fn);
        mono_trace_set_printerr_handler(mono_print_handler_fn);
        mono_trace_set_log_handler(mono_log_handler_fn, NULL);

        mono_set_dirs(mono_lib_path_a, mono_etc_path_a);

        mono_config_parse(NULL);

        if (wine_mono_install_assembly_preload_hook_v2)
            wine_mono_install_assembly_preload_hook_v2(wine_mono_assembly_preload_hook_v2_fn, NULL);
        else if (wine_mono_install_assembly_preload_hook)
            wine_mono_install_assembly_preload_hook(wine_mono_assembly_preload_hook_fn, NULL);
        else
            mono_install_assembly_preload_hook(mono_assembly_preload_hook_fn, NULL);

        aot_size = GetEnvironmentVariableA("WINE_MONO_AOT", aot_setting, sizeof(aot_setting));

        if (aot_size)
        {
            MonoAotMode mode;
            if (strcmp(aot_setting, "interp") == 0)
                mode = MONO_AOT_MODE_INTERP_ONLY;
            else if (strcmp(aot_setting, "none") == 0)
                mode = MONO_AOT_MODE_NONE;
            else
            {
                ERR("unknown WINE_MONO_AOT setting, valid settings are interp and none\n");
                mode = MONO_AOT_MODE_NONE;
            }
            if (mono_jit_set_aot_mode != NULL)
            {
                mono_jit_set_aot_mode(mode);
            }
            else
            {
                ERR("mono_jit_set_aot_mode export not found\n");
            }
        }

        trace_size = GetEnvironmentVariableA("WINE_MONO_TRACE", trace_setting, sizeof(trace_setting));

        if (trace_size)
        {
            mono_jit_set_trace_options(trace_setting);
        }

        verbose_size = GetEnvironmentVariableA("WINE_MONO_VERBOSE", verbose_setting, sizeof(verbose_setting));

        if (verbose_size)
        {
            mono_set_verbose_level(verbose_setting[0] - '0');
        }
    }

    return S_OK;

fail:
    ERR("Could not load Mono into this process\n");
    FreeLibrary(mono_handle);
    mono_handle = NULL;
    return E_FAIL;
}

static char* get_exe_basename_utf8(void)
{
    WCHAR filenameW[MAX_PATH], *basenameW;

    GetModuleFileNameW(NULL, filenameW, MAX_PATH);

    basenameW = wcsrchr(filenameW, '\\');
    if (basenameW)
        basenameW += 1;
    else
        basenameW = filenameW;

    return WtoA(basenameW);
}

MonoDomain* get_root_domain(void)
{
    static MonoDomain* root_domain;

    if (root_domain != NULL)
        return root_domain;

    EnterCriticalSection(&runtime_list_cs);

    if (root_domain == NULL)
    {
        char *exe_basename;

        exe_basename = get_exe_basename_utf8();

        root_domain = mono_jit_init_version(exe_basename, "v4.0.30319");

        free(exe_basename);

        is_mono_started = TRUE;
    }

    LeaveCriticalSection(&runtime_list_cs);

    return root_domain;
}

static void CDECL mono_shutdown_callback_fn(MonoProfiler *prof)
{
    is_mono_shutdown = TRUE;
}

static HRESULT WINAPI thread_set_fn(void)
{
    WARN("stub\n");
    return S_OK;
}

static HRESULT WINAPI thread_unset_fn(void)
{
    WARN("stub\n");
    return S_OK;
}

static HRESULT CLRRuntimeInfo_GetRuntimeHost(CLRRuntimeInfo *This, RuntimeHost **result)
{
    HRESULT hr = S_OK;
    WCHAR mono_path[MAX_PATH];

    if (This->loaded_runtime)
    {
        *result = This->loaded_runtime;
        return hr;
    }

    if (!get_mono_path(mono_path, FALSE))
    {
        ERR("Wine Mono is not installed\n");
        return CLR_E_SHIM_RUNTIME;
    }

    EnterCriticalSection(&runtime_list_cs);

    if (This->loaded_runtime)
    {
        *result = This->loaded_runtime;
        LeaveCriticalSection(&runtime_list_cs);
        return hr;
    }

    if (GlobalCLRMetaHost.callback)
    {
        GlobalCLRMetaHost.callback(&This->ICLRRuntimeInfo_iface, thread_set_fn, thread_unset_fn);
    }

    hr = load_mono(mono_path);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_Construct(This, &This->loaded_runtime);

    LeaveCriticalSection(&runtime_list_cs);

    if (SUCCEEDED(hr))
        *result = This->loaded_runtime;

    return hr;
}

void expect_no_runtimes(void)
{
    if (mono_handle && is_mono_started && !is_mono_shutdown)
    {
        ERR("Process exited with a Mono runtime loaded.\n");
        return;
    }
}

static inline CLRRuntimeInfo *impl_from_ICLRRuntimeInfo(ICLRRuntimeInfo *iface)
{
    return CONTAINING_RECORD(iface, CLRRuntimeInfo, ICLRRuntimeInfo_iface);
}

static HRESULT WINAPI CLRRuntimeInfo_QueryInterface(ICLRRuntimeInfo* iface,
        REFIID riid,
        void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICLRRuntimeInfo ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICLRRuntimeInfo_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CLRRuntimeInfo_AddRef(ICLRRuntimeInfo* iface)
{
    return 2;
}

static ULONG WINAPI CLRRuntimeInfo_Release(ICLRRuntimeInfo* iface)
{
    return 1;
}

static HRESULT WINAPI CLRRuntimeInfo_GetVersionString(ICLRRuntimeInfo* iface,
    LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    struct CLRRuntimeInfo *This = impl_from_ICLRRuntimeInfo(iface);
    DWORD buffer_size = *pcchBuffer;
    HRESULT hr = S_OK;
    char version[11];
    DWORD size;

    TRACE("%p %p %p\n", iface, pwzBuffer, pcchBuffer);

    size = snprintf(version, sizeof(version), "v%lu.%lu.%lu", This->major, This->minor, This->build);

    assert(size <= sizeof(version));

    *pcchBuffer = MultiByteToWideChar(CP_UTF8, 0, version, -1, NULL, 0);

    if (pwzBuffer)
    {
        if (buffer_size >= *pcchBuffer)
            MultiByteToWideChar(CP_UTF8, 0, version, -1, pwzBuffer, buffer_size);
        else
            hr = E_NOT_SUFFICIENT_BUFFER;
    }

    return hr;
}

static BOOL get_install_root(LPWSTR install_dir)
{
    static const WCHAR dotnet_key[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','.','N','E','T','F','r','a','m','e','w','o','r','k','\\',0};
    static const WCHAR install_root[] = {'I','n','s','t','a','l','l','R','o','o','t',0};

    DWORD len;
    HKEY key;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, dotnet_key, 0, KEY_READ, &key))
        return FALSE;

    len = MAX_PATH * sizeof(WCHAR);
    if (RegQueryValueExW(key, install_root, 0, NULL, (LPBYTE)install_dir, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    return TRUE;
}

static HRESULT WINAPI CLRRuntimeInfo_GetRuntimeDirectory(ICLRRuntimeInfo* iface,
    LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    static const WCHAR slash[] = {'\\',0};
    DWORD buffer_size = *pcchBuffer;
    WCHAR system_dir[MAX_PATH];
    WCHAR version[MAX_PATH];
    DWORD version_size, size;
    HRESULT hr = S_OK;

    TRACE("%p %p %p\n", iface, pwzBuffer, pcchBuffer);

    if (!get_install_root(system_dir))
    {
        ERR("error reading registry key for installroot\n");
        return E_FAIL;
    }
    else
    {
        version_size = MAX_PATH;
        ICLRRuntimeInfo_GetVersionString(iface, version, &version_size);
        lstrcatW(system_dir, version);
        lstrcatW(system_dir, slash);
        size = lstrlenW(system_dir) + 1;
    }

    *pcchBuffer = size;

    if (pwzBuffer)
    {
        if (buffer_size >= size)
            lstrcpyW(pwzBuffer, system_dir);
        else
            hr = E_NOT_SUFFICIENT_BUFFER;
    }

    return hr;
}

static HRESULT WINAPI CLRRuntimeInfo_IsLoaded(ICLRRuntimeInfo* iface,
    HANDLE hndProcess, BOOL *pbLoaded)
{
    FIXME("%p %p %p\n", iface, hndProcess, pbLoaded);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_LoadErrorString(ICLRRuntimeInfo* iface,
    UINT iResourceID, LPWSTR pwzBuffer, DWORD *pcchBuffer, LONG iLocaleid)
{
    FIXME("%p %u %p %p %lx\n", iface, iResourceID, pwzBuffer, pcchBuffer, iLocaleid);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_LoadLibrary(ICLRRuntimeInfo* iface,
    LPCWSTR pwzDllName, HMODULE *phndModule)
{
    WCHAR version[MAX_PATH];
    HRESULT hr;
    DWORD cchBuffer;

    TRACE("%p %s %p\n", iface, debugstr_w(pwzDllName), phndModule);

    cchBuffer = MAX_PATH;
    hr = ICLRRuntimeInfo_GetVersionString(iface, version, &cchBuffer);
    if (FAILED(hr)) return hr;

    return LoadLibraryShim(pwzDllName, version, NULL, phndModule);
}

static HRESULT WINAPI CLRRuntimeInfo_GetProcAddress(ICLRRuntimeInfo* iface,
    LPCSTR pszProcName, LPVOID *ppProc)
{
    FIXME("%p %s %p\n", iface, debugstr_a(pszProcName), ppProc);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_GetInterface(ICLRRuntimeInfo* iface,
    REFCLSID rclsid, REFIID riid, LPVOID *ppUnk)
{
    struct CLRRuntimeInfo *This = impl_from_ICLRRuntimeInfo(iface);
    RuntimeHost *host;
    HRESULT hr;

    TRACE("%p %s %s %p\n", iface, debugstr_guid(rclsid), debugstr_guid(riid), ppUnk);

    hr = CLRRuntimeInfo_GetRuntimeHost(This, &host);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_GetInterface(host, rclsid, riid, ppUnk);

    return hr;
}

static HRESULT WINAPI CLRRuntimeInfo_IsLoadable(ICLRRuntimeInfo* iface,
    BOOL *pbLoadable)
{
    FIXME("%p %p\n", iface, pbLoadable);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_SetDefaultStartupFlags(ICLRRuntimeInfo* iface,
    DWORD dwStartupFlags, LPCWSTR pwzHostConfigFile)
{
    FIXME("%p %lx %s\n", iface, dwStartupFlags, debugstr_w(pwzHostConfigFile));

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_GetDefaultStartupFlags(ICLRRuntimeInfo* iface,
    DWORD *pdwStartupFlags, LPWSTR pwzHostConfigFile, DWORD *pcchHostConfigFile)
{
    FIXME("%p %p %p %p\n", iface, pdwStartupFlags, pwzHostConfigFile, pcchHostConfigFile);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_BindAsLegacyV2Runtime(ICLRRuntimeInfo* iface)
{
    FIXME("%p\n", iface);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRRuntimeInfo_IsStarted(ICLRRuntimeInfo* iface,
    BOOL *pbStarted, DWORD *pdwStartupFlags)
{
    FIXME("%p %p %p\n", iface, pbStarted, pdwStartupFlags);

    return E_NOTIMPL;
}

static const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl = {
    CLRRuntimeInfo_QueryInterface,
    CLRRuntimeInfo_AddRef,
    CLRRuntimeInfo_Release,
    CLRRuntimeInfo_GetVersionString,
    CLRRuntimeInfo_GetRuntimeDirectory,
    CLRRuntimeInfo_IsLoaded,
    CLRRuntimeInfo_LoadErrorString,
    CLRRuntimeInfo_LoadLibrary,
    CLRRuntimeInfo_GetProcAddress,
    CLRRuntimeInfo_GetInterface,
    CLRRuntimeInfo_IsLoadable,
    CLRRuntimeInfo_SetDefaultStartupFlags,
    CLRRuntimeInfo_GetDefaultStartupFlags,
    CLRRuntimeInfo_BindAsLegacyV2Runtime,
    CLRRuntimeInfo_IsStarted
};

HRESULT ICLRRuntimeInfo_GetRuntimeHost(ICLRRuntimeInfo *iface, RuntimeHost **result)
{
    struct CLRRuntimeInfo *This = impl_from_ICLRRuntimeInfo(iface);

    assert(This->ICLRRuntimeInfo_iface.lpVtbl == &CLRRuntimeInfoVtbl);

    return CLRRuntimeInfo_GetRuntimeHost(This, result);
}

#ifdef __i386__
static const WCHAR libmono2_arch_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','-','x','8','6','.','d','l','l',0};
#elif defined(__x86_64__)
static const WCHAR libmono2_arch_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','-','x','8','6','_','6','4','.','d','l','l',0};
#else
static const WCHAR libmono2_arch_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','.','d','l','l',0};
#endif

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path)
{
    static const WCHAR mono2_dll[] = {'\\','b','i','n','\\','m','o','n','o','-','2','.','0','.','d','l','l',0};
    static const WCHAR libmono2_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','.','d','l','l',0};
    DWORD attributes=INVALID_FILE_ATTRIBUTES;

    lstrcpyW(dll_path, path);
    lstrcatW(dll_path, libmono2_arch_dll);
    attributes = GetFileAttributesW(dll_path);

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        lstrcpyW(dll_path, path);
        lstrcatW(dll_path, mono2_dll);
        attributes = GetFileAttributesW(dll_path);
    }

    if (attributes == INVALID_FILE_ATTRIBUTES)
    {
        lstrcpyW(dll_path, path);
        lstrcatW(dll_path, libmono2_dll);
        attributes = GetFileAttributesW(dll_path);
    }

    return (attributes != INVALID_FILE_ATTRIBUTES);
}

static BOOL get_mono_path_local(LPWSTR path)
{
    static const WCHAR subdir_mono[] = {'\\','m','o','n','o','\\','m','o','n','o','-','2','.','0', 0};
    WCHAR base_path[MAX_PATH], mono_dll_path[MAX_PATH];

    /* c:\windows\mono\mono-2.0 */
    GetWindowsDirectoryW(base_path, MAX_PATH);
    lstrcatW(base_path, subdir_mono);

    if (find_mono_dll(base_path, mono_dll_path))
    {
        lstrcpyW(path, base_path);
        return TRUE;
    }

    return FALSE;
}

static BOOL get_mono_path_registry(LPWSTR path)
{
    static const WCHAR keyname[] = {'S','o','f','t','w','a','r','e','\\','W','i','n','e','\\','M','o','n','o',0};
    static const WCHAR valuename[] = {'R','u','n','t','i','m','e','P','a','t','h',0};
    WCHAR base_path[MAX_PATH], mono_dll_path[MAX_PATH];
    HKEY hkey;
    DWORD res, valuesize;
    BOOL ret=FALSE;

    /* @@ Wine registry key: HKCU\Software\Wine\Mono */
    res = RegOpenKeyW(HKEY_CURRENT_USER, keyname, &hkey);
    if (res != ERROR_SUCCESS)
        return FALSE;

    valuesize = sizeof(base_path);
    res = RegGetValueW(hkey, NULL, valuename, RRF_RT_REG_SZ, NULL, base_path, &valuesize);
    if (res == ERROR_SUCCESS && find_mono_dll(base_path, mono_dll_path))
    {
        lstrcpyW(path, base_path);
        ret = TRUE;
    }

    RegCloseKey(hkey);

    return ret;
}

static BOOL get_mono_path_dos(const WCHAR *dir, LPWSTR path)
{
    static const WCHAR unix_prefix[] = {'\\','\\','?','\\','u','n','i','x','\\'};
    static const WCHAR basedir[] = L"\\wine-mono-" WINE_MONO_VERSION;
    LPWSTR dos_dir;
    WCHAR mono_dll_path[MAX_PATH];
    DWORD len;
    BOOL ret;

    if (memcmp(dir, unix_prefix, sizeof(unix_prefix)) == 0)
        return FALSE;  /* No drive letter for this directory */

    len = lstrlenW( dir ) + lstrlenW( basedir ) + 1;
    if (!(dos_dir = malloc( len * sizeof(WCHAR) ))) return FALSE;
    lstrcpyW( dos_dir, dir );
    lstrcatW( dos_dir, basedir );

    ret = find_mono_dll(dos_dir, mono_dll_path);
    if (ret)
        lstrcpyW(path, dos_dir);

    free(dos_dir);

    return ret;
}

#ifndef __REACTOS__
static BOOL get_mono_path_unix(const char *unix_dir, LPWSTR path)
{
    static WCHAR * (CDECL *p_wine_get_dos_file_name)(const char*);
    LPWSTR dos_dir;
    BOOL ret;

    if (!p_wine_get_dos_file_name)
    {
        p_wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleA("kernel32"), "wine_get_dos_file_name");
        if (!p_wine_get_dos_file_name)
            return FALSE;
    }

    dos_dir = p_wine_get_dos_file_name(unix_dir);
    if (!dos_dir)
        return FALSE;

    ret = get_mono_path_dos( dos_dir, path);

    HeapFree(GetProcessHeap(), 0, dos_dir);
    return ret;
}
#endif

static BOOL get_mono_path_datadir(LPWSTR path)
{
    static const WCHAR winedatadirW[] = {'W','I','N','E','D','A','T','A','D','I','R',0};
    static const WCHAR winebuilddirW[] = {'W','I','N','E','B','U','I','L','D','D','I','R',0};
    static const WCHAR unix_prefix[] = {'\\','?','?','\\','u','n','i','x','\\'};
    static const WCHAR monoW[] = {'\\','m','o','n','o',0};
    static const WCHAR dotdotmonoW[] = {'\\','.','.','\\','m','o','n','o',0};
    const WCHAR *data_dir, *suffix;
    WCHAR *package_dir;
    BOOL ret;

    if ((data_dir = _wgetenv( winedatadirW )))
        suffix = monoW;
    else if ((data_dir = _wgetenv( winebuilddirW )))
        suffix = dotdotmonoW;
    else
        return FALSE;

    if (!wcsncmp( data_dir, unix_prefix, wcslen(unix_prefix) )) return FALSE;
    data_dir += 4;  /* skip \??\ prefix */
    package_dir = malloc((wcslen(data_dir) + wcslen(suffix) + 1) * sizeof(WCHAR));
    lstrcpyW( package_dir, data_dir );
    lstrcatW( package_dir, suffix );

    ret = get_mono_path_dos(package_dir, path);

    free(package_dir);

    return ret;
}

BOOL get_mono_path(LPWSTR path, BOOL skip_local)
{
#ifdef __REACTOS__
    return (!skip_local && get_mono_path_local(path)) ||
        get_mono_path_registry(path) ||
        get_mono_path_datadir(path);
#else
    return (!skip_local && get_mono_path_local(path)) ||
        get_mono_path_registry(path) ||
        get_mono_path_datadir(path) ||
        get_mono_path_unix(INSTALL_DATADIR "/wine/mono", path) ||
        (strcmp(INSTALL_DATADIR, "/usr/share") &&
         get_mono_path_unix("/usr/share/wine/mono", path)) ||
        get_mono_path_unix("/opt/wine/mono", path);
#endif
}

struct InstalledRuntimeEnum
{
    IEnumUnknown IEnumUnknown_iface;
    LONG ref;
    ULONG pos;
};

static const struct IEnumUnknownVtbl InstalledRuntimeEnum_Vtbl;

static inline struct InstalledRuntimeEnum *impl_from_IEnumUnknown(IEnumUnknown *iface)
{
    return CONTAINING_RECORD(iface, struct InstalledRuntimeEnum, IEnumUnknown_iface);
}

static HRESULT WINAPI InstalledRuntimeEnum_QueryInterface(IEnumUnknown* iface, REFIID riid,
        void **ppvObject)
{
    TRACE("%p %s %p\n", iface, debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_IEnumUnknown ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    IEnumUnknown_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI InstalledRuntimeEnum_AddRef(IEnumUnknown* iface)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    return ref;
}

static ULONG WINAPI InstalledRuntimeEnum_Release(IEnumUnknown* iface)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%lu\n", iface, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI InstalledRuntimeEnum_Next(IEnumUnknown *iface, ULONG celt,
    IUnknown **rgelt, ULONG *pceltFetched)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG num_fetched = 0;
    HRESULT hr=S_OK;
    IUnknown *item;

    TRACE("(%p,%lu,%p,%p)\n", iface, celt, rgelt, pceltFetched);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        item = (IUnknown*)&runtimes[This->pos].ICLRRuntimeInfo_iface;
        IUnknown_AddRef(item);
        rgelt[num_fetched] = item;
        num_fetched++;
        This->pos++;
    }

    if (pceltFetched)
        *pceltFetched = num_fetched;

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Skip(IEnumUnknown *iface, ULONG celt)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG num_fetched = 0;
    HRESULT hr=S_OK;

    TRACE("(%p,%lu)\n", iface, celt);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        num_fetched++;
        This->pos++;
    }

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Reset(IEnumUnknown *iface)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);

    TRACE("(%p)\n", iface);

    This->pos = 0;

    return S_OK;
}

static HRESULT WINAPI InstalledRuntimeEnum_Clone(IEnumUnknown *iface, IEnumUnknown **ppenum)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    struct InstalledRuntimeEnum *new_enum;

    TRACE("(%p)\n", iface);

    new_enum = malloc(sizeof(*new_enum));
    if (!new_enum)
        return E_OUTOFMEMORY;

    new_enum->IEnumUnknown_iface.lpVtbl = &InstalledRuntimeEnum_Vtbl;
    new_enum->ref = 1;
    new_enum->pos = This->pos;

    *ppenum = &new_enum->IEnumUnknown_iface;

    return S_OK;
}

static const struct IEnumUnknownVtbl InstalledRuntimeEnum_Vtbl = {
    InstalledRuntimeEnum_QueryInterface,
    InstalledRuntimeEnum_AddRef,
    InstalledRuntimeEnum_Release,
    InstalledRuntimeEnum_Next,
    InstalledRuntimeEnum_Skip,
    InstalledRuntimeEnum_Reset,
    InstalledRuntimeEnum_Clone
};

static HRESULT WINAPI CLRMetaHost_QueryInterface(ICLRMetaHost* iface,
        REFIID riid,
        void **ppvObject)
{
    TRACE("%s %p\n", debugstr_guid(riid), ppvObject);

    if ( IsEqualGUID( riid, &IID_ICLRMetaHost ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        *ppvObject = iface;
    }
    else
    {
        FIXME("Unsupported interface %s\n", debugstr_guid(riid));
        return E_NOINTERFACE;
    }

    ICLRMetaHost_AddRef( iface );

    return S_OK;
}

static ULONG WINAPI CLRMetaHost_AddRef(ICLRMetaHost* iface)
{
    return 2;
}

static ULONG WINAPI CLRMetaHost_Release(ICLRMetaHost* iface)
{
    return 1;
}

static inline BOOL isDigit(WCHAR c)
{
    return c >= '0' && c <= '9';
}

static BOOL parse_runtime_version(LPCWSTR version, DWORD *major, DWORD *minor, DWORD *build)
{
    *major = 0;
    *minor = 0;
    *build = 0;

    if (version[0] == 'v' || version[0] == 'V')
    {
        version++;
        if (!isDigit(*version))
            return FALSE;

        while (isDigit(*version))
            *major = *major * 10 + (*version++ - '0');

        if (*version == 0)
            return TRUE;

        if (*version++ != '.' || !isDigit(*version))
            return FALSE;

        while (isDigit(*version))
            *minor = *minor * 10 + (*version++ - '0');

        if (*version == 0)
            return TRUE;

        if (*version++ != '.' || !isDigit(*version))
            return FALSE;

        while (isDigit(*version))
            *build = *build * 10 + (*version++ - '0');

        return *version == 0;
    }
    else
        return FALSE;
}

static HRESULT get_runtime(LPCWSTR pwzVersion, BOOL allow_short,
    REFIID iid, LPVOID *ppRuntime)
{
    int i;
    DWORD major, minor, build;

    if (!pwzVersion)
        return E_POINTER;

    if (!parse_runtime_version(pwzVersion, &major, &minor, &build))
    {
        ERR("Cannot parse %s\n", debugstr_w(pwzVersion));
        return CLR_E_SHIM_RUNTIME;
    }

    for (i=0; i<NUM_RUNTIMES; i++)
    {
        if (runtimes[i].major == major && runtimes[i].minor == minor &&
            (runtimes[i].build == build || (allow_short && major >= 4 && build == 0)))
        {
            return ICLRRuntimeInfo_QueryInterface(&runtimes[i].ICLRRuntimeInfo_iface, iid,
                    ppRuntime);
        }
    }

    FIXME("Unrecognized version %s\n", debugstr_w(pwzVersion));
    return CLR_E_SHIM_RUNTIME;
}

HRESULT WINAPI CLRMetaHost_GetRuntime(ICLRMetaHost* iface,
    LPCWSTR pwzVersion, REFIID iid, LPVOID *ppRuntime)
{
    TRACE("%s %s %p\n", debugstr_w(pwzVersion), debugstr_guid(iid), ppRuntime);

    return get_runtime(pwzVersion, FALSE, iid, ppRuntime);
}

HRESULT WINAPI CLRMetaHost_GetVersionFromFile(ICLRMetaHost* iface,
    LPCWSTR pwzFilePath, LPWSTR pwzBuffer, DWORD *pcchBuffer)
{
    ASSEMBLY *assembly;
    HRESULT hr;
    LPSTR version;
    ULONG buffer_size=*pcchBuffer;

    TRACE("%s %p %p\n", debugstr_w(pwzFilePath), pwzBuffer, pcchBuffer);

    hr = assembly_create(&assembly, pwzFilePath);

    if (SUCCEEDED(hr))
    {
        hr = assembly_get_runtime_version(assembly, &version);

        if (SUCCEEDED(hr))
        {
            *pcchBuffer = MultiByteToWideChar(CP_UTF8, 0, version, -1, NULL, 0);

            if (pwzBuffer)
            {
                if (buffer_size >= *pcchBuffer)
                    MultiByteToWideChar(CP_UTF8, 0, version, -1, pwzBuffer, buffer_size);
                else
                    hr = E_NOT_SUFFICIENT_BUFFER;
            }
        }

        assembly_release(assembly);
    }

    return hr;
}

static HRESULT WINAPI CLRMetaHost_EnumerateInstalledRuntimes(ICLRMetaHost* iface,
    IEnumUnknown **ppEnumerator)
{
    struct InstalledRuntimeEnum *new_enum;

    TRACE("%p\n", ppEnumerator);

    new_enum = malloc(sizeof(*new_enum));
    if (!new_enum)
        return E_OUTOFMEMORY;

    new_enum->IEnumUnknown_iface.lpVtbl = &InstalledRuntimeEnum_Vtbl;
    new_enum->ref = 1;
    new_enum->pos = 0;

    *ppEnumerator = &new_enum->IEnumUnknown_iface;

    return S_OK;
}

static HRESULT WINAPI CLRMetaHost_EnumerateLoadedRuntimes(ICLRMetaHost* iface,
    HANDLE hndProcess, IEnumUnknown **ppEnumerator)
{
    FIXME("%p %p\n", hndProcess, ppEnumerator);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_RequestRuntimeLoadedNotification(ICLRMetaHost* iface,
    RuntimeLoadedCallbackFnPtr pCallbackFunction)
{
    TRACE("%p\n", pCallbackFunction);

    if(!pCallbackFunction)
        return E_POINTER;

    if (GlobalCLRMetaHost.callback)
        return HOST_E_INVALIDOPERATION;

    GlobalCLRMetaHost.callback = pCallbackFunction;

    return S_OK;
}

static HRESULT WINAPI CLRMetaHost_QueryLegacyV2RuntimeBinding(ICLRMetaHost* iface,
    REFIID riid, LPVOID *ppUnk)
{
    FIXME("%s %p\n", debugstr_guid(riid), ppUnk);

    return E_NOTIMPL;
}

HRESULT WINAPI CLRMetaHost_ExitProcess(ICLRMetaHost* iface, INT32 iExitCode)
{
    TRACE("%i\n", iExitCode);

    EnterCriticalSection(&runtime_list_cs);

    if (is_mono_started && !is_mono_shutdown)
    {
        /* search for a runtime and call System.Environment.Exit() */
        int i;

        for (i=0; i<NUM_RUNTIMES; i++)
            if (runtimes[i].loaded_runtime)
                RuntimeHost_ExitProcess(runtimes[i].loaded_runtime, iExitCode);
    }

    ExitProcess(iExitCode);
}

static const struct ICLRMetaHostVtbl CLRMetaHost_vtbl =
{
    CLRMetaHost_QueryInterface,
    CLRMetaHost_AddRef,
    CLRMetaHost_Release,
    CLRMetaHost_GetRuntime,
    CLRMetaHost_GetVersionFromFile,
    CLRMetaHost_EnumerateInstalledRuntimes,
    CLRMetaHost_EnumerateLoadedRuntimes,
    CLRMetaHost_RequestRuntimeLoadedNotification,
    CLRMetaHost_QueryLegacyV2RuntimeBinding,
    CLRMetaHost_ExitProcess
};

static struct CLRMetaHost GlobalCLRMetaHost = {
    { &CLRMetaHost_vtbl }
};

HRESULT CLRMetaHost_CreateInstance(REFIID riid, void **ppobj)
{
    return ICLRMetaHost_QueryInterface(&GlobalCLRMetaHost.ICLRMetaHost_iface, riid, ppobj);
}

struct CLRMetaHostPolicy
{
    ICLRMetaHostPolicy ICLRMetaHostPolicy_iface;
};

static struct CLRMetaHostPolicy GlobalCLRMetaHostPolicy;

static HRESULT WINAPI metahostpolicy_QueryInterface(ICLRMetaHostPolicy *iface, REFIID riid, void **obj)
{
    TRACE("%s %p\n", debugstr_guid(riid), obj);

    if ( IsEqualGUID( riid, &IID_ICLRMetaHostPolicy ) ||
         IsEqualGUID( riid, &IID_IUnknown ) )
    {
        ICLRMetaHostPolicy_AddRef( iface );
        *obj = iface;
        return S_OK;
    }

    FIXME("Unsupported interface %s\n", debugstr_guid(riid));

    *obj = NULL;
    return E_NOINTERFACE;
}

static ULONG WINAPI metahostpolicy_AddRef(ICLRMetaHostPolicy *iface)
{
    return 2;
}

static ULONG WINAPI metahostpolicy_Release(ICLRMetaHostPolicy *iface)
{
    return 1;
}

static HRESULT WINAPI metahostpolicy_GetRequestedRuntime(ICLRMetaHostPolicy *iface, METAHOST_POLICY_FLAGS dwPolicyFlags,
    LPCWSTR pwzBinary, IStream *pCfgStream, LPWSTR pwzVersion, DWORD *pcchVersion,
    LPWSTR pwzImageVersion, DWORD *pcchImageVersion, DWORD *pdwConfigFlags, REFIID riid,
    LPVOID *ppRuntime)
{
    ICLRRuntimeInfo *result;
    HRESULT hr;
    WCHAR filename[MAX_PATH];
    const WCHAR *path = NULL;
    int flags = 0;

    TRACE("%d %p %p %p %p %p %p %p %s %p\n", dwPolicyFlags, pwzBinary, pCfgStream,
        pwzVersion, pcchVersion, pwzImageVersion, pcchImageVersion, pdwConfigFlags,
        debugstr_guid(riid), ppRuntime);

    if (pdwConfigFlags) {
        FIXME("ignoring config flags\n");
        *pdwConfigFlags = 0;
    }

    if(dwPolicyFlags & METAHOST_POLICY_USE_PROCESS_IMAGE_PATH)
    {
        GetModuleFileNameW(0, filename, MAX_PATH);
        path = filename;
    }
    else if(pwzBinary)
    {
        path = pwzBinary;
    }

    if(dwPolicyFlags & METAHOST_POLICY_APPLY_UPGRADE_POLICY)
        flags |= RUNTIME_INFO_UPGRADE_VERSION;

    hr = get_runtime_info(path, pwzImageVersion, NULL, pCfgStream, 0, flags, FALSE, &result);
    if (SUCCEEDED(hr))
    {
        if (pwzImageVersion)
        {
            /* Ignoring errors on purpose */
            ICLRRuntimeInfo_GetVersionString(result, pwzImageVersion, pcchImageVersion);
        }

        hr = ICLRRuntimeInfo_QueryInterface(result, riid, ppRuntime);

        ICLRRuntimeInfo_Release(result);
    }

    TRACE("<- 0x%08lx\n", hr);

    return hr;
}

static const struct ICLRMetaHostPolicyVtbl CLRMetaHostPolicy_vtbl =
{
    metahostpolicy_QueryInterface,
    metahostpolicy_AddRef,
    metahostpolicy_Release,
    metahostpolicy_GetRequestedRuntime
};

static struct CLRMetaHostPolicy GlobalCLRMetaHostPolicy = {
    { &CLRMetaHostPolicy_vtbl }
};

HRESULT CLRMetaHostPolicy_CreateInstance(REFIID riid, void **ppobj)
{
    return ICLRMetaHostPolicy_QueryInterface(&GlobalCLRMetaHostPolicy.ICLRMetaHostPolicy_iface, riid, ppobj);
}

/*
 * Assembly search override settings:
 *
 * WINE_MONO_OVERRIDES=*,Gac=n
 *  Never search the Windows GAC for libraries.
 *
 * WINE_MONO_OVERRIDES=*,MonoGac=n
 *  Never search the Mono GAC for libraries.
 *
 * WINE_MONO_OVERRIDES=*,PrivatePath=n
 *  Never search the AppDomain search path for libraries.
 *
 * WINE_MONO_OVERRIDES=Microsoft.Xna.Framework,Gac=n
 *  Never search the Windows GAC for Microsoft.Xna.Framework
 *
 * WINE_MONO_OVERRIDES=Microsoft.Xna.Framework.*,Gac=n;Microsoft.Xna.Framework.GamerServices,Gac=y
 *  Never search the Windows GAC for Microsoft.Xna.Framework, or any library starting
 *  with Microsoft.Xna.Framework, except for Microsoft.Xna.Framework.GamerServices
 */

/* assembly search override flags */
#define ASSEMBLY_SEARCH_GAC 1
#define ASSEMBLY_SEARCH_UNDEFINED 2
#define ASSEMBLY_SEARCH_PRIVATEPATH 4
#define ASSEMBLY_SEARCH_MONOGAC 8
#define ASSEMBLY_SEARCH_DEFAULT (ASSEMBLY_SEARCH_GAC|ASSEMBLY_SEARCH_PRIVATEPATH|ASSEMBLY_SEARCH_MONOGAC)

typedef struct override_entry {
    char *name;
    DWORD flags;
    struct list entry;
} override_entry;

static struct list env_overrides = LIST_INIT(env_overrides);

#define IS_OPTION_TRUE(ch) \
    ((ch) == 'y' || (ch) == 'Y' || (ch) == 't' || (ch) == 'T' || (ch) == '1')
#define IS_OPTION_FALSE(ch) \
    ((ch) == 'n' || (ch) == 'N' || (ch) == 'f' || (ch) == 'F' || (ch) == '0')

static void parse_override_entry(override_entry *entry, const char *string, int string_len)
{
    const char *next_key, *equals, *value;
    UINT kvp_len, key_len;

    entry->flags = ASSEMBLY_SEARCH_DEFAULT;

    while (string && string_len > 0)
    {
        next_key = memchr(string, ',', string_len);

        if (next_key)
        {
            kvp_len = next_key - string;
            next_key++;
        }
        else
            kvp_len = string_len;

        equals = memchr(string, '=', kvp_len);

        if (equals)
        {
            key_len = equals - string;
            value = equals + 1;
            switch (key_len) {
            case 3:
                if (!_strnicmp(string, "gac", 3)) {
                    if (IS_OPTION_TRUE(*value))
                        entry->flags |= ASSEMBLY_SEARCH_GAC;
                    else if (IS_OPTION_FALSE(*value))
                        entry->flags &= ~ASSEMBLY_SEARCH_GAC;
                }
                break;
            case 7:
                if (!_strnicmp(string, "monogac", 7)) {
                    if (IS_OPTION_TRUE(*value))
                        entry->flags |= ASSEMBLY_SEARCH_MONOGAC;
                    else if (IS_OPTION_FALSE(*value))
                        entry->flags &= ~ASSEMBLY_SEARCH_MONOGAC;
                }
                break;
            case 11:
                if (!_strnicmp(string, "privatepath", 11)) {
                    if (IS_OPTION_TRUE(*value))
                        entry->flags |= ASSEMBLY_SEARCH_PRIVATEPATH;
                    else if (IS_OPTION_FALSE(*value))
                        entry->flags &= ~ASSEMBLY_SEARCH_PRIVATEPATH;
                }
                break;
            default:
                break;
            }
        }

        string = next_key;
        string_len -= kvp_len + 1;
    }
}

static BOOL WINAPI parse_env_overrides(INIT_ONCE *once, void *param, void **context)
{
    const char *override_string = getenv("WINE_MONO_OVERRIDES");
    struct override_entry *entry;

    if (override_string)
    {
        const char *entry_start;

        entry_start = override_string;

        while (entry_start && *entry_start)
        {
            const char *next_entry, *basename_end;
            UINT entry_len;

            next_entry = strchr(entry_start, ';');

            if (next_entry)
            {
                entry_len = next_entry - entry_start;
                next_entry++;
            }
            else
                entry_len = strlen(entry_start);

            basename_end = memchr(entry_start, ',', entry_len);

            if (!basename_end)
            {
                entry_start = next_entry;
                continue;
            }

            entry = calloc(1, sizeof(*entry));
            if (!entry)
            {
                ERR("out of memory\n");
                break;
            }

            entry->name = calloc(1, basename_end - entry_start + 1);
            if (!entry->name)
            {
                ERR("out of memory\n");
                free(entry);
                break;
            }

            memcpy(entry->name, entry_start, basename_end - entry_start);

            entry_len -= basename_end - entry_start + 1;
            entry_start = basename_end + 1;

            parse_override_entry(entry, entry_start, entry_len);

            list_add_tail(&env_overrides, &entry->entry);

            entry_start = next_entry;
        }
    }

    return TRUE;
}

static DWORD get_basename_search_flags(const char *basename, MonoAssemblyName *aname, HKEY userkey, HKEY appkey)
{
    struct override_entry *entry;
    char buffer[256];
    DWORD buffer_size;
    static INIT_ONCE init_once = INIT_ONCE_STATIC_INIT;

    InitOnceExecuteOnce(&init_once, parse_env_overrides, NULL, NULL);

    LIST_FOR_EACH_ENTRY(entry, &env_overrides, override_entry, entry)
    {
        if (strcmp(basename, entry->name) == 0)
        {
            return entry->flags;
        }
    }

    buffer_size = sizeof(buffer);
    if (appkey && !RegQueryValueExA(appkey, basename, 0, NULL, (LPBYTE)buffer, &buffer_size))
    {
        override_entry reg_entry;

        memset(&reg_entry, 0, sizeof(reg_entry));

        parse_override_entry(&reg_entry, buffer, strlen(buffer));

        return reg_entry.flags;
    }

    buffer_size = sizeof(buffer);
    if (userkey && !RegQueryValueExA(userkey, basename, 0, NULL, (LPBYTE)buffer, &buffer_size))
    {
        override_entry reg_entry;

        memset(&reg_entry, 0, sizeof(reg_entry));

        parse_override_entry(&reg_entry, buffer, strlen(buffer));

        return reg_entry.flags;
    }

    if (strcmp(basename, "Microsoft.Xna.Framework.*") == 0 &&
        mono_assembly_name_get_version(aname, NULL, NULL, NULL) == 4)
        /* Use FNA as a replacement for XNA4. */
        return ASSEMBLY_SEARCH_MONOGAC;

    return ASSEMBLY_SEARCH_UNDEFINED;
}

static HKEY get_app_overrides_key(void)
{
    static const WCHAR subkeyW[] = {'\\','M','o','n','o','\\','A','s','m','O','v','e','r','r','i','d','e','s',0};
    WCHAR bufferW[MAX_PATH+18];
    HKEY appkey = 0;
    DWORD len;

    len = (GetModuleFileNameW( 0, bufferW, MAX_PATH ));
    if (len && len < MAX_PATH)
    {
        HKEY tmpkey;
        WCHAR *p, *appname = bufferW;
        if ((p = wcsrchr( appname, '/' ))) appname = p + 1;
        if ((p = wcsrchr( appname, '\\' ))) appname = p + 1;
        lstrcatW( appname, subkeyW );
        /* @@ Wine registry key: HKCU\Software\Wine\AppDefaults\app.exe\Mono\AsmOverrides */
        if (!RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\AppDefaults", &tmpkey ))
        {
            if (RegOpenKeyW( tmpkey, appname, &appkey )) appkey = 0;
            RegCloseKey( tmpkey );
        }
    }

    return appkey;
}

static DWORD get_assembly_search_flags(MonoAssemblyName *aname)
{
    const char *name = mono_assembly_name_get_name(aname);
    char *name_copy, *name_end;
    DWORD result;
    HKEY appkey = 0, userkey;

    /* @@ Wine registry key: HKCU\Software\Wine\Mono\AsmOverrides */
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Mono\\AsmOverrides", &userkey )) userkey = 0;

    appkey = get_app_overrides_key();

    result = get_basename_search_flags(name, aname, userkey, appkey);
    if (result != ASSEMBLY_SEARCH_UNDEFINED)
    {
        if (userkey) RegCloseKey(userkey);
        if (appkey) RegCloseKey(appkey);
        return result;
    }

    name_copy = malloc(strlen(name) + 3);
    if (!name_copy)
    {
        ERR("out of memory\n");
        if (userkey) RegCloseKey(userkey);
        if (appkey) RegCloseKey(appkey);
        return ASSEMBLY_SEARCH_DEFAULT;
    }

    strcpy(name_copy, name);
    name_end = name_copy + strlen(name);

    do
    {
        strcpy(name_end, ".*");
        result = get_basename_search_flags(name_copy, aname, userkey, appkey);
        if (result != ASSEMBLY_SEARCH_UNDEFINED) break;

        *name_end = 0;
        name_end = strrchr(name_copy, '.');
    } while (name_end != NULL);

    /* default flags */
    if (result == ASSEMBLY_SEARCH_UNDEFINED)
    {
        result = get_basename_search_flags("*", aname, userkey, appkey);
        if (result == ASSEMBLY_SEARCH_UNDEFINED)
            result = ASSEMBLY_SEARCH_DEFAULT;
    }

    free(name_copy);
    if (appkey) RegCloseKey(appkey);
    if (userkey) RegCloseKey(userkey);

    return result;
}

HRESULT get_file_from_strongname(WCHAR* stringnameW, WCHAR* assemblies_path, int path_length)
{
    HRESULT hr=S_OK;
    IAssemblyCache *asmcache;
    ASSEMBLY_INFO info;
    static const WCHAR fusiondll[] = {'f','u','s','i','o','n',0};
    HMODULE hfusion=NULL;
    static HRESULT (WINAPI *pCreateAssemblyCache)(IAssemblyCache**,DWORD);

    if (!pCreateAssemblyCache)
    {
        hr = LoadLibraryShim(fusiondll, NULL, NULL, &hfusion);

        if (SUCCEEDED(hr))
        {
            pCreateAssemblyCache = (void*)GetProcAddress(hfusion, "CreateAssemblyCache");
            if (!pCreateAssemblyCache)
                hr = E_FAIL;
        }
    }

    if (SUCCEEDED(hr))
        hr = pCreateAssemblyCache(&asmcache, 0);

    if (SUCCEEDED(hr))
    {
        info.cbAssemblyInfo = sizeof(info);
        info.pszCurrentAssemblyPathBuf = assemblies_path;
        info.cchBuf = path_length;
        assemblies_path[0] = 0;

        hr = IAssemblyCache_QueryAssemblyInfo(asmcache, 0, stringnameW, &info);

        IAssemblyCache_Release(asmcache);
    }

    return hr;
}

static MonoAssembly* mono_assembly_try_load(WCHAR *path)
{
    MonoAssembly *result = NULL;
    MonoImageOpenStatus stat;
    char *pathA;

    if (!(pathA = WtoA(path))) return NULL;

    result = mono_assembly_open(pathA, &stat);
    free(pathA);

    if (result) TRACE("found: %s\n", debugstr_w(path));
    return result;
}

static MonoAssembly* CDECL mono_assembly_preload_hook_fn(MonoAssemblyName *aname, char **assemblies_path, void *user_data)
{
    int flags = 0;
    return wine_mono_assembly_preload_hook_v2_fn(aname, assemblies_path, &flags, user_data);
}

static MonoAssembly* CDECL wine_mono_assembly_preload_hook_fn(MonoAssemblyName *aname, char **assemblies_path, int *halt_search, void *user_data)
{
    int flags = 0;
    MonoAssembly* result = wine_mono_assembly_preload_hook_v2_fn(aname, assemblies_path, &flags, user_data);
    if (flags & WINE_PRELOAD_SKIP_PRIVATE_PATH)
        *halt_search = 1;
    return result;
}

static MonoAssembly* CDECL wine_mono_assembly_preload_hook_v2_fn(MonoAssemblyName *aname, char **assemblies_path, int *flags, void *user_data)
{
    HRESULT hr;
    MonoAssembly *result=NULL;
    char *stringname=NULL;
    const char *assemblyname;
    const char *culture;
    LPWSTR stringnameW, cultureW;
    int stringnameW_size, cultureW_size;
    WCHAR path[MAX_PATH];
    char *pathA;
    MonoImageOpenStatus stat;
    DWORD search_flags;
    int i;
    static const WCHAR dotdllW[] = {'.','d','l','l',0};
    static const WCHAR dotexeW[] = {'.','e','x','e',0};

    stringname = mono_stringify_assembly_name(aname);
    assemblyname = mono_assembly_name_get_name(aname);
    culture = mono_assembly_name_get_culture(aname);
    if (culture)
    {
        cultureW_size = MultiByteToWideChar(CP_UTF8, 0, culture, -1, NULL, 0);
        cultureW = malloc(cultureW_size * sizeof(WCHAR));
        if (cultureW) MultiByteToWideChar(CP_UTF8, 0, culture, -1, cultureW, cultureW_size);
    }
    else cultureW = NULL;

    TRACE("%s\n", debugstr_a(stringname));

    if (!stringname || !assemblyname) return NULL;

    search_flags = get_assembly_search_flags(aname);
    if (private_path && (search_flags & ASSEMBLY_SEARCH_PRIVATEPATH) != 0)
    {
        stringnameW_size = MultiByteToWideChar(CP_UTF8, 0, assemblyname, -1, NULL, 0);
        stringnameW = malloc(stringnameW_size * sizeof(WCHAR));
        if (stringnameW)
        {
            MultiByteToWideChar(CP_UTF8, 0, assemblyname, -1, stringnameW, stringnameW_size);
            for (i = 0; private_path[i] != NULL; i++)
            {
                /* This is the lookup order used in Mono */
                /* 1st try: [culture]/[name].dll (culture may be empty) */
                wcscpy(path, private_path[i]);
                if (cultureW) PathAppendW(path, cultureW);
                PathAppendW(path, stringnameW);
                wcscat(path, dotdllW);
                result = mono_assembly_try_load(path);
                if (result) break;

                /* 2nd try: [culture]/[name].exe (culture may be empty) */
                wcscpy(path + wcslen(path) - wcslen(dotdllW), dotexeW);
                result = mono_assembly_try_load(path);
                if (result) break;

                /* 3rd try: [culture]/[name]/[name].dll (culture may be empty) */
                path[wcslen(path) - wcslen(dotexeW)] = 0;
                PathAppendW(path, stringnameW);
                wcscat(path, dotdllW);
                result = mono_assembly_try_load(path);
                if (result) break;

                /* 4th try: [culture]/[name]/[name].exe (culture may be empty) */
                wcscpy(path + wcslen(path) - wcslen(dotdllW), dotexeW);
                result = mono_assembly_try_load(path);
                if (result) break;
            }
            free(stringnameW);
            if (result) goto done;
        }
    }

    if ((search_flags & ASSEMBLY_SEARCH_GAC) != 0)
    {
        stringnameW_size = MultiByteToWideChar(CP_UTF8, 0, stringname, -1, NULL, 0);

        stringnameW = malloc(stringnameW_size * sizeof(WCHAR));
        if (stringnameW)
        {
            MultiByteToWideChar(CP_UTF8, 0, stringname, -1, stringnameW, stringnameW_size);

            hr = get_file_from_strongname(stringnameW, path, MAX_PATH);

            free(stringnameW);
        }
        else
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
        {
            TRACE("found: %s\n", debugstr_w(path));

            pathA = WtoA(path);

            if (pathA)
            {
                result = mono_assembly_open(pathA, &stat);

                if (!result)
                    ERR("Failed to load %s, status=%u\n", debugstr_w(path), stat);

                free(pathA);

                if (result)
                {
                    *flags |= WINE_PRELOAD_SET_GAC;
                    goto done;
                }
            }
        }
    }
    else
        TRACE("skipping Windows GAC search due to override setting\n");

    if (wine_mono_assembly_load_from_gac)
    {
        if (search_flags & ASSEMBLY_SEARCH_MONOGAC)
        {
            result = wine_mono_assembly_load_from_gac (aname, &stat, FALSE);

            if (result)
            {
                TRACE("found in Mono GAC\n");
                *flags |= WINE_PRELOAD_SET_GAC;
                goto done;
            }
        }
        else
        {
            *flags |= WINE_PRELOAD_SKIP_GAC;
            TRACE("skipping Mono GAC search due to override setting\n");
        }
    }

    if ((search_flags & ASSEMBLY_SEARCH_PRIVATEPATH) == 0)
    {
        TRACE("skipping AppDomain search path due to override setting\n");
        *flags |= WINE_PRELOAD_SKIP_PRIVATE_PATH;
    }

done:
    free(cultureW);
    mono_free(stringname);

    return result;
}

HRESULT get_runtime_info(LPCWSTR exefile, LPCWSTR version, LPCWSTR config_file,
    IStream *config_stream, DWORD startup_flags, DWORD runtimeinfo_flags,
    BOOL legacy, ICLRRuntimeInfo **result)
{
    static const WCHAR dotconfig[] = {'.','c','o','n','f','i','g',0};
    static const DWORD supported_startup_flags = 0;
    static const DWORD supported_runtime_flags = RUNTIME_INFO_UPGRADE_VERSION;
    int i;
    WCHAR local_version[MAX_PATH];
    ULONG local_version_size = MAX_PATH;
    WCHAR local_config_file[MAX_PATH];
    HRESULT hr;
    parsed_config_file parsed_config;

    if (startup_flags & ~supported_startup_flags)
        FIXME("unsupported startup flags %lx\n", startup_flags & ~supported_startup_flags);

    if (runtimeinfo_flags & ~supported_runtime_flags)
        FIXME("unsupported runtimeinfo flags %lx\n", runtimeinfo_flags & ~supported_runtime_flags);

    if (exefile && !exefile[0])
        exefile = NULL;

    if (exefile && !config_file && !config_stream)
    {
        lstrcpyW(local_config_file, exefile);
        lstrcatW(local_config_file, dotconfig);

        config_file = local_config_file;
    }

    if (config_file || config_stream)
    {
        BOOL found = FALSE;
        if (config_file)
            hr = parse_config_file(config_file, &parsed_config);
        else
            hr = parse_config_stream(config_stream, &parsed_config);

        if (SUCCEEDED(hr))
        {
            supported_runtime *entry;
            LIST_FOR_EACH_ENTRY(entry, &parsed_config.supported_runtimes, supported_runtime, entry)
            {
                hr = get_runtime(entry->version, TRUE, &IID_ICLRRuntimeInfo, (void**)result);
                if (SUCCEEDED(hr))
                {
                    found = TRUE;
                    break;
                }
            }
        }
        else
        {
            WARN("failed to parse config file %s, hr=%lx\n", debugstr_w(config_file), hr);
        }

        free_parsed_config_file(&parsed_config);

        if (found)
            return S_OK;
    }

    if (exefile && !version)
    {
        DWORD major, minor, build;

        hr = CLRMetaHost_GetVersionFromFile(0, exefile, local_version, &local_version_size);

        version = local_version;

        if (FAILED(hr)) return hr;

        /* When running an executable, specifically when getting the version number from
         * the exe, native accepts a matching major.minor with build <= expected build. */
        if (!parse_runtime_version(version, &major, &minor, &build))
        {
            ERR("Cannot parse %s\n", debugstr_w(version));
            return CLR_E_SHIM_RUNTIME;
        }

        if (legacy)
            i = 3;
        else
            i = NUM_RUNTIMES;

        while (i--)
        {
            if (runtimes[i].major == major && runtimes[i].minor == minor && runtimes[i].build >= build)
            {
                return ICLRRuntimeInfo_QueryInterface(&runtimes[i].ICLRRuntimeInfo_iface,
                        &IID_ICLRRuntimeInfo, (void **)result);
            }
        }
    }

    if (version)
    {
        hr = CLRMetaHost_GetRuntime(0, version, &IID_ICLRRuntimeInfo, (void**)result);
        if(SUCCEEDED(hr))
            return hr;
    }

    if (runtimeinfo_flags & RUNTIME_INFO_UPGRADE_VERSION)
    {
        DWORD major, minor, build;

        if (version && !parse_runtime_version(version, &major, &minor, &build))
        {
            ERR("Cannot parse %s\n", debugstr_w(version));
            return CLR_E_SHIM_RUNTIME;
        }

        if (legacy)
            i = 3;
        else
            i = NUM_RUNTIMES;

        while (i--)
        {
            /* Must be greater or equal to the version passed in. */
            if (!version || ((runtimes[i].major >= major && runtimes[i].minor >= minor && runtimes[i].build >= build) ||
                 (runtimes[i].major >= major && runtimes[i].minor > minor) ||
                 (runtimes[i].major > major)))
            {
                return ICLRRuntimeInfo_QueryInterface(&runtimes[i].ICLRRuntimeInfo_iface,
                        &IID_ICLRRuntimeInfo, (void **)result);
            }
        }

        return CLR_E_SHIM_RUNTIME;
    }

    return CLR_E_SHIM_RUNTIME;
}
