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

#include "mscoree_private.h"

#include <stdio.h>
#include <assert.h>

#include <wine/library.h>

#include <fusion.h>

static const WCHAR net_11_subdir[] = {'1','.','0',0};
static const WCHAR net_20_subdir[] = {'2','.','0',0};
static const WCHAR net_40_subdir[] = {'4','.','0',0};

static const struct ICLRRuntimeInfoVtbl CLRRuntimeInfoVtbl;

#define NUM_RUNTIMES 3

static struct CLRRuntimeInfo runtimes[NUM_RUNTIMES] = {
    {{&CLRRuntimeInfoVtbl}, net_11_subdir, 1, 1, 4322, 0},
    {{&CLRRuntimeInfoVtbl}, net_20_subdir, 2, 0, 50727, 0},
    {{&CLRRuntimeInfoVtbl}, net_40_subdir, 4, 0, 30319, 0}
};

static int runtimes_initialized;

static CRITICAL_SECTION runtime_list_cs;
static CRITICAL_SECTION_DEBUG runtime_list_cs_debug =
{
    0, 0, &runtime_list_cs,
    { &runtime_list_cs_debug.ProcessLocksList,
      &runtime_list_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": runtime_list_cs") }
};
static CRITICAL_SECTION runtime_list_cs = { &runtime_list_cs_debug, -1, 0, 0, 0, 0 };

#define NUM_ABI_VERSIONS 2

static loaded_mono loaded_monos[NUM_ABI_VERSIONS];

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path, int abi_version);

static MonoAssembly* mono_assembly_search_hook_fn(MonoAssemblyName *aname, char **assemblies_path, void *user_data);

static void mono_shutdown_callback_fn(MonoProfiler *prof);

static void set_environment(LPCWSTR bin_path)
{
    WCHAR path_env[MAX_PATH];
    int len;

    static const WCHAR pathW[] = {'P','A','T','H',0};

    /* We have to modify PATH as Mono loads other DLLs from this directory. */
    GetEnvironmentVariableW(pathW, path_env, sizeof(path_env)/sizeof(WCHAR));
    len = strlenW(path_env);
    path_env[len++] = ';';
    strcpyW(path_env+len, bin_path);
    SetEnvironmentVariableW(pathW, path_env);
}

static void CDECL do_nothing(void)
{
}

static void missing_runtime_message(const CLRRuntimeInfo *This)
{
    if (This->major == 1)
        MESSAGE("wine: Install Mono 2.6 for Windows to run .NET 1.1 applications.\n");
    else if (This->major == 2)
        MESSAGE("wine: Install Mono for Windows to run .NET 2.0 applications.\n");
    else if (This->major == 4)
        MESSAGE("wine: Install Mono 2.8 or greater for Windows to run .NET 4.0 applications.\n");
}

static HRESULT load_mono(CLRRuntimeInfo *This, loaded_mono **result)
{
    static const WCHAR bin[] = {'\\','b','i','n',0};
    static const WCHAR lib[] = {'\\','l','i','b',0};
    static const WCHAR etc[] = {'\\','e','t','c',0};
    static const WCHAR glibdll[] = {'l','i','b','g','l','i','b','-','2','.','0','-','0','.','d','l','l',0};
    WCHAR mono_dll_path[MAX_PATH+16], mono_bin_path[MAX_PATH+4];
    WCHAR mono_lib_path[MAX_PATH+4], mono_etc_path[MAX_PATH+4];
    char mono_lib_path_a[MAX_PATH], mono_etc_path_a[MAX_PATH];
    int trace_size;
    char trace_setting[256];

    if (This->mono_abi_version <= 0 || This->mono_abi_version > NUM_ABI_VERSIONS)
    {
        missing_runtime_message(This);
        return E_FAIL;
    }

    *result = &loaded_monos[This->mono_abi_version-1];

    if ((*result)->is_shutdown)
    {
        ERR("Cannot load Mono after it has been shut down.\n");
        *result = NULL;
        return E_FAIL;
    }

    if (!(*result)->mono_handle)
    {
        strcpyW(mono_bin_path, This->mono_path);
        strcatW(mono_bin_path, bin);
        set_environment(mono_bin_path);

        strcpyW(mono_lib_path, This->mono_path);
        strcatW(mono_lib_path, lib);
        WideCharToMultiByte(CP_UTF8, 0, mono_lib_path, -1, mono_lib_path_a, MAX_PATH, NULL, NULL);

        strcpyW(mono_etc_path, This->mono_path);
        strcatW(mono_etc_path, etc);
        WideCharToMultiByte(CP_UTF8, 0, mono_etc_path, -1, mono_etc_path_a, MAX_PATH, NULL, NULL);

        if (!find_mono_dll(This->mono_path, mono_dll_path, This->mono_abi_version)) goto fail;

        (*result)->mono_handle = LoadLibraryW(mono_dll_path);

        if (!(*result)->mono_handle) goto fail;

#define LOAD_MONO_FUNCTION(x) do { \
    (*result)->x = (void*)GetProcAddress((*result)->mono_handle, #x); \
    if (!(*result)->x) { \
        goto fail; \
    } \
} while (0);

        LOAD_MONO_FUNCTION(mono_assembly_get_image);
        LOAD_MONO_FUNCTION(mono_assembly_load_from);
        LOAD_MONO_FUNCTION(mono_assembly_open);
        LOAD_MONO_FUNCTION(mono_config_parse);
        LOAD_MONO_FUNCTION(mono_class_from_mono_type);
        LOAD_MONO_FUNCTION(mono_class_from_name);
        LOAD_MONO_FUNCTION(mono_class_get_method_from_name);
        LOAD_MONO_FUNCTION(mono_domain_assembly_open);
        LOAD_MONO_FUNCTION(mono_image_open_from_module_handle);
        LOAD_MONO_FUNCTION(mono_install_assembly_preload_hook);
        LOAD_MONO_FUNCTION(mono_jit_exec);
        LOAD_MONO_FUNCTION(mono_jit_init);
        LOAD_MONO_FUNCTION(mono_jit_set_trace_options);
        LOAD_MONO_FUNCTION(mono_marshal_get_vtfixup_ftnptr);
        LOAD_MONO_FUNCTION(mono_object_get_domain);
        LOAD_MONO_FUNCTION(mono_object_new);
        LOAD_MONO_FUNCTION(mono_object_unbox);
        LOAD_MONO_FUNCTION(mono_profiler_install);
        LOAD_MONO_FUNCTION(mono_reflection_type_from_name);
        LOAD_MONO_FUNCTION(mono_runtime_invoke);
        LOAD_MONO_FUNCTION(mono_runtime_object_init);
        LOAD_MONO_FUNCTION(mono_runtime_quit);
        LOAD_MONO_FUNCTION(mono_set_dirs);
        LOAD_MONO_FUNCTION(mono_stringify_assembly_name);
        LOAD_MONO_FUNCTION(mono_string_new);
        LOAD_MONO_FUNCTION(mono_thread_attach);

        /* GLib imports obsoleted by the 2.0 ABI */
        if (This->mono_abi_version == 1)
        {
            (*result)->glib_handle = LoadLibraryW(glibdll);
            if (!(*result)->glib_handle) goto fail;

            (*result)->mono_free = (void*)GetProcAddress((*result)->glib_handle, "g_free");
            if (!(*result)->mono_free) goto fail;
        }
        else
        {
            LOAD_MONO_FUNCTION(mono_free);
        }

#undef LOAD_MONO_FUNCTION

#define LOAD_OPT_VOID_MONO_FUNCTION(x) do { \
    (*result)->x = (void*)GetProcAddress((*result)->mono_handle, #x); \
    if (!(*result)->x) { \
        (*result)->x = do_nothing; \
    } \
} while (0);

        LOAD_OPT_VOID_MONO_FUNCTION(mono_runtime_set_shutting_down);
        LOAD_OPT_VOID_MONO_FUNCTION(mono_thread_pool_cleanup);
        LOAD_OPT_VOID_MONO_FUNCTION(mono_thread_suspend_all_other_threads);
        LOAD_OPT_VOID_MONO_FUNCTION(mono_threads_set_shutting_down);

#undef LOAD_OPT_VOID_MONO_FUNCTION

        (*result)->mono_profiler_install((MonoProfiler*)*result, mono_shutdown_callback_fn);

        (*result)->mono_set_dirs(mono_lib_path_a, mono_etc_path_a);

        (*result)->mono_config_parse(NULL);

        (*result)->mono_install_assembly_preload_hook(mono_assembly_search_hook_fn, *result);

        trace_size = GetEnvironmentVariableA("WINE_MONO_TRACE", trace_setting, sizeof(trace_setting));

        if (trace_size)
        {
            (*result)->mono_jit_set_trace_options(trace_setting);
        }
    }

    return S_OK;

fail:
    ERR("Could not load Mono into this process\n");
    FreeLibrary((*result)->mono_handle);
    FreeLibrary((*result)->glib_handle);
    (*result)->mono_handle = NULL;
    (*result)->glib_handle = NULL;
    return E_FAIL;
}

static void mono_shutdown_callback_fn(MonoProfiler *prof)
{
    loaded_mono *mono = (loaded_mono*)prof;

    mono->is_shutdown = TRUE;
}

static HRESULT CLRRuntimeInfo_GetRuntimeHost(CLRRuntimeInfo *This, RuntimeHost **result)
{
    HRESULT hr = S_OK;
    loaded_mono *ploaded_mono;

    if (This->loaded_runtime)
    {
        *result = This->loaded_runtime;
        return hr;
    }

    EnterCriticalSection(&runtime_list_cs);

    hr = load_mono(This, &ploaded_mono);

    if (SUCCEEDED(hr))
        hr = RuntimeHost_Construct(This, ploaded_mono, &This->loaded_runtime);

    LeaveCriticalSection(&runtime_list_cs);

    if (SUCCEEDED(hr))
        *result = This->loaded_runtime;

    return hr;
}

void unload_all_runtimes(void)
{
    int i;

    for (i=0; i<NUM_ABI_VERSIONS; i++)
    {
        loaded_mono *mono = &loaded_monos[i];
        if (mono->mono_handle && mono->is_started && !mono->is_shutdown)
        {
            /* Copied from Mono's ves_icall_System_Environment_Exit */
	    mono->mono_threads_set_shutting_down();
	    mono->mono_runtime_set_shutting_down();
	    mono->mono_thread_pool_cleanup();
	    mono->mono_thread_suspend_all_other_threads();
	    mono->mono_runtime_quit();
        }
    }

    for (i=0; i<NUM_RUNTIMES; i++)
        if (runtimes[i].loaded_runtime)
            RuntimeHost_Destroy(runtimes[i].loaded_runtime);
}

void expect_no_runtimes(void)
{
    int i;

    for (i=0; i<NUM_ABI_VERSIONS; i++)
    {
        loaded_mono *mono = &loaded_monos[i];
        if (mono->mono_handle && mono->is_started && !mono->is_shutdown)
        {
            ERR("Process exited with a Mono runtime loaded.\n");
            return;
        }
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

    size = snprintf(version, sizeof(version), "v%u.%u.%u", This->major, This->minor, This->build);

    assert(size <= sizeof(version));

    *pcchBuffer = MultiByteToWideChar(CP_UTF8, 0, version, -1, NULL, 0);

    if (pwzBuffer)
    {
        if (buffer_size >= *pcchBuffer)
            MultiByteToWideChar(CP_UTF8, 0, version, -1, pwzBuffer, buffer_size);
        else
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    }

    return hr;
}

static BOOL get_install_root(LPWSTR install_dir)
{
    const WCHAR dotnet_key[] = {'S','O','F','T','W','A','R','E','\\','M','i','c','r','o','s','o','f','t','\\','.','N','E','T','F','r','a','m','e','w','o','r','k','\\',0};
    const WCHAR install_root[] = {'I','n','s','t','a','l','l','R','o','o','t',0};

    DWORD len;
    HKEY key;

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, dotnet_key, 0, KEY_READ, &key))
        return FALSE;

    len = MAX_PATH;
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
            strcpyW(pwzBuffer, system_dir);
        else
            hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
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
    FIXME("%p %u %p %p %x\n", iface, iResourceID, pwzBuffer, pcchBuffer, iLocaleid);

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
    FIXME("%p %x %s\n", iface, dwStartupFlags, debugstr_w(pwzHostConfigFile));

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

static BOOL find_mono_dll(LPCWSTR path, LPWSTR dll_path, int abi_version)
{
    static const WCHAR mono_dll[] = {'\\','b','i','n','\\','m','o','n','o','.','d','l','l',0};
    static const WCHAR libmono_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','.','d','l','l',0};
    static const WCHAR mono2_dll[] = {'\\','b','i','n','\\','m','o','n','o','-','2','.','0','.','d','l','l',0};
    static const WCHAR libmono2_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','-','2','.','0','.','d','l','l',0};
    DWORD attributes=INVALID_FILE_ATTRIBUTES;

    if (abi_version == 1)
    {
        strcpyW(dll_path, path);
        strcatW(dll_path, mono_dll);
        attributes = GetFileAttributesW(dll_path);

        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            strcpyW(dll_path, path);
            strcatW(dll_path, libmono_dll);
            attributes = GetFileAttributesW(dll_path);
        }
    }
    else if (abi_version == 2)
    {
        strcpyW(dll_path, path);
        strcatW(dll_path, libmono2_arch_dll);
        attributes = GetFileAttributesW(dll_path);

        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            strcpyW(dll_path, path);
            strcatW(dll_path, mono2_dll);
            attributes = GetFileAttributesW(dll_path);
        }

        if (attributes == INVALID_FILE_ATTRIBUTES)
        {
            strcpyW(dll_path, path);
            strcatW(dll_path, libmono2_dll);
            attributes = GetFileAttributesW(dll_path);
        }
    }

    return (attributes != INVALID_FILE_ATTRIBUTES);
}

static BOOL get_mono_path_from_registry(LPWSTR path, int abi_version)
{
    static const WCHAR mono_key[] = {'S','o','f','t','w','a','r','e','\\','N','o','v','e','l','l','\\','M','o','n','o',0};
    static const WCHAR defaul_clr[] = {'D','e','f','a','u','l','t','C','L','R',0};
    static const WCHAR install_root[] = {'S','d','k','I','n','s','t','a','l','l','R','o','o','t',0};
    static const WCHAR slash[] = {'\\',0};

    WCHAR version[64], version_key[MAX_PATH];
    DWORD len;
    HKEY key;
    WCHAR dll_path[MAX_PATH];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, mono_key, 0, KEY_READ, &key))
        return FALSE;

    len = sizeof(version);
    if (RegQueryValueExW(key, defaul_clr, 0, NULL, (LPBYTE)version, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    lstrcpyW(version_key, mono_key);
    lstrcatW(version_key, slash);
    lstrcatW(version_key, version);

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, version_key, 0, KEY_READ, &key))
        return FALSE;

    len = sizeof(WCHAR) * MAX_PATH;
    if (RegQueryValueExW(key, install_root, 0, NULL, (LPBYTE)path, &len))
    {
        RegCloseKey(key);
        return FALSE;
    }
    RegCloseKey(key);

    return find_mono_dll(path, dll_path, abi_version);
}

static BOOL get_mono_path_from_folder(LPCWSTR folder, LPWSTR mono_path, int abi_version)
{
    static const WCHAR mono_one_dot_zero[] = {'\\','m','o','n','o','-','1','.','0', 0};
    static const WCHAR mono_two_dot_zero[] = {'\\','m','o','n','o','-','2','.','0', 0};
    WCHAR mono_dll_path[MAX_PATH];
    BOOL found = FALSE;

    strcpyW(mono_path, folder);

    if (abi_version == 1)
        strcatW(mono_path, mono_one_dot_zero);
    else if (abi_version == 2)
        strcatW(mono_path, mono_two_dot_zero);

    found = find_mono_dll(mono_path, mono_dll_path, abi_version);

    return found;
}

static BOOL get_mono_path(LPWSTR path, int abi_version)
{
    static const WCHAR subdir_mono[] = {'\\','m','o','n','o',0};
    static const WCHAR sibling_mono[] = {'\\','.','.','\\','m','o','n','o',0};
    WCHAR base_path[MAX_PATH];
    const char *unix_data_dir;
    WCHAR *dos_data_dir;
    int build_tree=0;
    static WCHAR* (CDECL *wine_get_dos_file_name)(const char*);

    /* First try c:\windows\mono */
    GetWindowsDirectoryW(base_path, MAX_PATH);
    strcatW(base_path, subdir_mono);

    if (get_mono_path_from_folder(base_path, path, abi_version))
        return TRUE;

    /* Next: /usr/share/wine/mono */
    unix_data_dir = wine_get_data_dir();

    if (!unix_data_dir)
    {
        unix_data_dir = wine_get_build_dir();
        build_tree = 1;
    }

    if (unix_data_dir)
    {
        if (!wine_get_dos_file_name)
            wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleA("kernel32"), "wine_get_dos_file_name");

        if (wine_get_dos_file_name)
        {
            dos_data_dir = wine_get_dos_file_name(unix_data_dir);

            if (dos_data_dir)
            {
                strcpyW(base_path, dos_data_dir);
                strcatW(base_path, build_tree ? sibling_mono : subdir_mono);

                HeapFree(GetProcessHeap(), 0, dos_data_dir);

                if (get_mono_path_from_folder(base_path, path, abi_version))
                    return TRUE;
            }
        }
    }

    /* Last: the registry */
    return get_mono_path_from_registry(path, abi_version);
}

static void find_runtimes(void)
{
    int abi_version, i;
    static const WCHAR libmono[] = {'\\','l','i','b','\\','m','o','n','o','\\',0};
    static const WCHAR mscorlib[] = {'\\','m','s','c','o','r','l','i','b','.','d','l','l',0};
    WCHAR mono_path[MAX_PATH], lib_path[MAX_PATH];
    BOOL any_runtimes_found = FALSE;

    if (runtimes_initialized) return;

    EnterCriticalSection(&runtime_list_cs);

    if (runtimes_initialized) goto end;

    for (abi_version=NUM_ABI_VERSIONS; abi_version>0; abi_version--)
    {
        if (!get_mono_path(mono_path, abi_version))
            continue;

        for (i=0; i<NUM_RUNTIMES; i++)
        {
            if (runtimes[i].mono_abi_version == 0)
            {
                strcpyW(lib_path, mono_path);
                strcatW(lib_path, libmono);
                strcatW(lib_path, runtimes[i].mono_libdir);
                strcatW(lib_path, mscorlib);

                if (GetFileAttributesW(lib_path) != INVALID_FILE_ATTRIBUTES)
                {
                    runtimes[i].mono_abi_version = abi_version;

                    strcpyW(runtimes[i].mono_path, mono_path);
                    strcpyW(runtimes[i].mscorlib_path, lib_path);

                    any_runtimes_found = TRUE;
                }
            }
        }
    }

    if (!any_runtimes_found)
    {
        /* Report all runtimes are available if Mono isn't installed.
         * FIXME: Remove this when Mono is properly packaged. */
        for (i=0; i<NUM_RUNTIMES; i++)
            runtimes[i].mono_abi_version = -1;
    }

    runtimes_initialized = 1;

end:
    LeaveCriticalSection(&runtime_list_cs);
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

    TRACE("(%p) refcount=%u\n", iface, ref);

    return ref;
}

static ULONG WINAPI InstalledRuntimeEnum_Release(IEnumUnknown* iface)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("(%p) refcount=%u\n", iface, ref);

    if (ref == 0)
    {
        HeapFree(GetProcessHeap(), 0, This);
    }

    return ref;
}

static HRESULT WINAPI InstalledRuntimeEnum_Next(IEnumUnknown *iface, ULONG celt,
    IUnknown **rgelt, ULONG *pceltFetched)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    int num_fetched = 0;
    HRESULT hr=S_OK;
    IUnknown *item;

    TRACE("(%p,%u,%p,%p)\n", iface, celt, rgelt, pceltFetched);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        if (runtimes[This->pos].mono_abi_version)
        {
            item = (IUnknown*)&runtimes[This->pos].ICLRRuntimeInfo_iface;
            IUnknown_AddRef(item);
            rgelt[num_fetched] = item;
            num_fetched++;
        }
        This->pos++;
    }

    if (pceltFetched)
        *pceltFetched = num_fetched;

    return hr;
}

static HRESULT WINAPI InstalledRuntimeEnum_Skip(IEnumUnknown *iface, ULONG celt)
{
    struct InstalledRuntimeEnum *This = impl_from_IEnumUnknown(iface);
    int num_fetched = 0;
    HRESULT hr=S_OK;

    TRACE("(%p,%u)\n", iface, celt);

    while (num_fetched < celt)
    {
        if (This->pos >= NUM_RUNTIMES)
        {
            hr = S_FALSE;
            break;
        }
        if (runtimes[This->pos].mono_abi_version)
        {
            num_fetched++;
        }
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

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_enum));
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

struct CLRMetaHost
{
    ICLRMetaHost ICLRMetaHost_iface;
};

static struct CLRMetaHost GlobalCLRMetaHost;

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

static BOOL parse_runtime_version(LPCWSTR version, DWORD *major, DWORD *minor, DWORD *build)
{
    *major = 0;
    *minor = 0;
    *build = 0;

    if (version[0] == 'v' || version[0] == 'V')
    {
        version++;
        if (!isdigit(*version))
            return FALSE;

        while (isdigit(*version))
            *major = *major * 10 + (*version++ - '0');

        if (*version == 0)
            return TRUE;

        if (*version++ != '.' || !isdigit(*version))
            return FALSE;

        while (isdigit(*version))
            *minor = *minor * 10 + (*version++ - '0');

        if (*version == 0)
            return TRUE;

        if (*version++ != '.' || !isdigit(*version))
            return FALSE;

        while (isdigit(*version))
            *build = *build * 10 + (*version++ - '0');

        return *version == 0;
    }
    else
        return FALSE;
}

HRESULT WINAPI CLRMetaHost_GetRuntime(ICLRMetaHost* iface,
    LPCWSTR pwzVersion, REFIID iid, LPVOID *ppRuntime)
{
    int i;
    DWORD major, minor, build;

    TRACE("%s %s %p\n", debugstr_w(pwzVersion), debugstr_guid(iid), ppRuntime);

    if (!pwzVersion)
        return E_POINTER;

    if (!parse_runtime_version(pwzVersion, &major, &minor, &build))
    {
        ERR("Cannot parse %s\n", debugstr_w(pwzVersion));
        return CLR_E_SHIM_RUNTIME;
    }

    find_runtimes();

    for (i=0; i<NUM_RUNTIMES; i++)
    {
        if (runtimes[i].major == major && runtimes[i].minor == minor &&
            runtimes[i].build == build)
        {
            if (runtimes[i].mono_abi_version)
                return ICLRRuntimeInfo_QueryInterface(&runtimes[i].ICLRRuntimeInfo_iface, iid,
                        ppRuntime);
            else
            {
                missing_runtime_message(&runtimes[i]);
                return CLR_E_SHIM_RUNTIME;
            }
        }
    }

    FIXME("Unrecognized version %s\n", debugstr_w(pwzVersion));
    return CLR_E_SHIM_RUNTIME;
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
                    hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
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

    find_runtimes();

    new_enum = HeapAlloc(GetProcessHeap(), 0, sizeof(*new_enum));
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
    FIXME("%p\n", pCallbackFunction);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_QueryLegacyV2RuntimeBinding(ICLRMetaHost* iface,
    REFIID riid, LPVOID *ppUnk)
{
    FIXME("%s %p\n", debugstr_guid(riid), ppUnk);

    return E_NOTIMPL;
}

static HRESULT WINAPI CLRMetaHost_ExitProcess(ICLRMetaHost* iface, INT32 iExitCode)
{
    FIXME("%i: stub\n", iExitCode);

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

static MonoAssembly* mono_assembly_search_hook_fn(MonoAssemblyName *aname, char **assemblies_path, void *user_data)
{
    loaded_mono *mono = user_data;
    HRESULT hr=S_OK;
    MonoAssembly *result=NULL;
    char *stringname=NULL;
    LPWSTR stringnameW;
    int stringnameW_size;
    IAssemblyCache *asmcache;
    ASSEMBLY_INFO info;
    WCHAR path[MAX_PATH];
    char *pathA;
    MonoImageOpenStatus stat;
    static WCHAR fusiondll[] = {'f','u','s','i','o','n',0};
    HMODULE hfusion=NULL;
    static HRESULT (WINAPI *pCreateAssemblyCache)(IAssemblyCache**,DWORD);

    stringname = mono->mono_stringify_assembly_name(aname);

    TRACE("%s\n", debugstr_a(stringname));

    if (!stringname) return NULL;

    /* FIXME: We should search the given paths before the GAC. */

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
        stringnameW_size = MultiByteToWideChar(CP_UTF8, 0, stringname, -1, NULL, 0);

        stringnameW = HeapAlloc(GetProcessHeap(), 0, stringnameW_size * sizeof(WCHAR));
        if (stringnameW)
            MultiByteToWideChar(CP_UTF8, 0, stringname, -1, stringnameW, stringnameW_size);
        else
            hr = E_OUTOFMEMORY;

        if (SUCCEEDED(hr))
        {
            info.cbAssemblyInfo = sizeof(info);
            info.pszCurrentAssemblyPathBuf = path;
            info.cchBuf = MAX_PATH;
            path[0] = 0;

            hr = IAssemblyCache_QueryAssemblyInfo(asmcache, 0, stringnameW, &info);
        }

        HeapFree(GetProcessHeap(), 0, stringnameW);

        IAssemblyCache_Release(asmcache);
    }

    if (SUCCEEDED(hr))
    {
        TRACE("found: %s\n", debugstr_w(path));

        pathA = WtoA(path);

        if (pathA)
        {
            result = mono->mono_assembly_open(pathA, &stat);

            if (!result)
                ERR("Failed to load %s, status=%u\n", debugstr_w(path), stat);

            HeapFree(GetProcessHeap(), 0, pathA);
        }
    }

    mono->mono_free(stringname);

    return result;
}

HRESULT get_runtime_info(LPCWSTR exefile, LPCWSTR version, LPCWSTR config_file,
    DWORD startup_flags, DWORD runtimeinfo_flags, BOOL legacy, ICLRRuntimeInfo **result)
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
        FIXME("unsupported startup flags %x\n", startup_flags & ~supported_startup_flags);

    if (runtimeinfo_flags & ~supported_runtime_flags)
        FIXME("unsupported runtimeinfo flags %x\n", runtimeinfo_flags & ~supported_runtime_flags);

    if (exefile && !config_file)
    {
        strcpyW(local_config_file, exefile);
        strcatW(local_config_file, dotconfig);

        config_file = local_config_file;
    }

    if (config_file)
    {
        int found=0;
        hr = parse_config_file(config_file, &parsed_config);

        if (SUCCEEDED(hr))
        {
            supported_runtime *entry;
            LIST_FOR_EACH_ENTRY(entry, &parsed_config.supported_runtimes, supported_runtime, entry)
            {
                hr = CLRMetaHost_GetRuntime(0, entry->version, &IID_ICLRRuntimeInfo, (void**)result);
                if (SUCCEEDED(hr))
                {
                    found = 1;
                    break;
                }
            }
        }
        else
        {
            WARN("failed to parse config file %s, hr=%x\n", debugstr_w(config_file), hr);
        }

        free_parsed_config_file(&parsed_config);

        if (found)
            return S_OK;
    }

    if (exefile && !version)
    {
        hr = CLRMetaHost_GetVersionFromFile(0, exefile, local_version, &local_version_size);

        version = local_version;

        if (FAILED(hr)) return hr;
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

        find_runtimes();

        if (legacy)
            i = 2;
        else
            i = NUM_RUNTIMES;

        while (i--)
        {
            if (runtimes[i].mono_abi_version)
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
        }

        if (legacy)
            missing_runtime_message(&runtimes[1]);
        else
            missing_runtime_message(&runtimes[NUM_RUNTIMES-1]);

        return CLR_E_SHIM_RUNTIME;
    }

    return CLR_E_SHIM_RUNTIME;
}
