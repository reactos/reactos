/*
 * Implementation of mscoree.dll
 * Microsoft Component Object Runtime Execution Engine
 *
 * Copyright 2006 Paul Chitescu
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

#include <stdarg.h>

#include "wine/unicode.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "shellapi.h"

#include "initguid.h"
#include "cor.h"
#include "mscoree.h"
#include "mscoree_private.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );

static BOOL get_mono_path(LPWSTR path)
{
    static const WCHAR mono_key[] = {'S','o','f','t','w','a','r','e','\\','N','o','v','e','l','l','\\','M','o','n','o',0};
    static const WCHAR defaul_clr[] = {'D','e','f','a','u','l','t','C','L','R',0};
    static const WCHAR install_root[] = {'S','d','k','I','n','s','t','a','l','l','R','o','o','t',0};
    static const WCHAR slash[] = {'\\',0};

    WCHAR version[64], version_key[MAX_PATH];
    DWORD len;
    HKEY key;

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

    return TRUE;
}

static CRITICAL_SECTION mono_lib_cs;
static CRITICAL_SECTION_DEBUG mono_lib_cs_debug =
{
    0, 0, &mono_lib_cs,
    { &mono_lib_cs_debug.ProcessLocksList,
      &mono_lib_cs_debug.ProcessLocksList },
      0, 0, { (DWORD_PTR)(__FILE__ ": mono_lib_cs") }
};
static CRITICAL_SECTION mono_lib_cs = { &mono_lib_cs_debug, -1, 0, 0, 0, 0 };

HMODULE mono_handle;

void (*mono_config_parse)(const char *filename);
MonoAssembly* (*mono_domain_assembly_open) (MonoDomain *domain, const char *name);
void (*mono_jit_cleanup)(MonoDomain *domain);
int (*mono_jit_exec)(MonoDomain *domain, MonoAssembly *assembly, int argc, char *argv[]);
MonoDomain* (*mono_jit_init)(const char *file);
int (*mono_jit_set_trace_options)(const char* options);
void (*mono_set_dirs)(const char *assembly_dir, const char *config_dir);

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

static HMODULE load_mono(void)
{
    static const WCHAR mono_dll[] = {'\\','b','i','n','\\','m','o','n','o','.','d','l','l',0};
    static const WCHAR libmono_dll[] = {'\\','b','i','n','\\','l','i','b','m','o','n','o','.','d','l','l',0};
    static const WCHAR bin[] = {'\\','b','i','n',0};
    static const WCHAR lib[] = {'\\','l','i','b',0};
    static const WCHAR etc[] = {'\\','e','t','c',0};
    HMODULE result;
    WCHAR mono_path[MAX_PATH], mono_dll_path[MAX_PATH+16], mono_bin_path[MAX_PATH+4];
    WCHAR mono_lib_path[MAX_PATH+4], mono_etc_path[MAX_PATH+4];
    char mono_lib_path_a[MAX_PATH], mono_etc_path_a[MAX_PATH];

    EnterCriticalSection(&mono_lib_cs);

    if (!mono_handle)
    {
        if (!get_mono_path(mono_path)) goto end;

        strcpyW(mono_bin_path, mono_path);
        strcatW(mono_bin_path, bin);
        set_environment(mono_bin_path);

        strcpyW(mono_lib_path, mono_path);
        strcatW(mono_lib_path, lib);
        WideCharToMultiByte(CP_UTF8, 0, mono_lib_path, -1, mono_lib_path_a, MAX_PATH, NULL, NULL);

        strcpyW(mono_etc_path, mono_path);
        strcatW(mono_etc_path, etc);
        WideCharToMultiByte(CP_UTF8, 0, mono_etc_path, -1, mono_etc_path_a, MAX_PATH, NULL, NULL);

        strcpyW(mono_dll_path, mono_path);
        strcatW(mono_dll_path, mono_dll);
        mono_handle = LoadLibraryW(mono_dll_path);

        if (!mono_handle)
        {
            strcpyW(mono_dll_path, mono_path);
            strcatW(mono_dll_path, libmono_dll);
            mono_handle = LoadLibraryW(mono_dll_path);
        }

        if (!mono_handle) goto end;

#define LOAD_MONO_FUNCTION(x) do { \
    x = (void*)GetProcAddress(mono_handle, #x); \
    if (!x) { \
        mono_handle = NULL; \
        goto end; \
    } \
} while (0);

        LOAD_MONO_FUNCTION(mono_config_parse);
        LOAD_MONO_FUNCTION(mono_domain_assembly_open);
        LOAD_MONO_FUNCTION(mono_jit_cleanup);
        LOAD_MONO_FUNCTION(mono_jit_exec);
        LOAD_MONO_FUNCTION(mono_jit_init);
        LOAD_MONO_FUNCTION(mono_jit_set_trace_options);
        LOAD_MONO_FUNCTION(mono_set_dirs);

#undef LOAD_MONO_FUNCTION

        mono_set_dirs(mono_lib_path_a, mono_etc_path_a);

        mono_config_parse(NULL);
    }

end:
    result = mono_handle;

    LeaveCriticalSection(&mono_lib_cs);

    if (!result)
        MESSAGE("wine: Install the Windows version of Mono to run .NET executables\n");

    return result;
}

HRESULT WINAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor,
                                    LPCWSTR pwszHostConfigFile, VOID *pReserved,
                                    DWORD startupFlags, REFCLSID rclsid,
                                    REFIID riid, LPVOID *ppv)
{
    FIXME("(%s, %s, %s, %p, %d, %s, %s, %p): semi-stub!\n", debugstr_w(pwszVersion),
          debugstr_w(pwszBuildFlavor), debugstr_w(pwszHostConfigFile), pReserved,
          startupFlags, debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if (!get_mono_path(NULL))
    {
        MESSAGE("wine: Install the Windows version of Mono to run .NET executables\n");
        return E_FAIL;
    }

    return S_OK;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %d, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_WINE_PREATTACH:
        return FALSE;  /* prefer native version */
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

BOOL WINAPI _CorDllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    FIXME("(%p, %d, %p): stub\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDLL);
        break;
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
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
    int trace_size;
    char trace_setting[256];
    int argc;
    char **argv;
    MonoDomain *domain;
    MonoAssembly *assembly;
    char filename[MAX_PATH];

    if (!load_mono())
    {
        return -1;
    }

    get_utf8_args(&argc, &argv);

    trace_size = GetEnvironmentVariableA("WINE_MONO_TRACE", trace_setting, sizeof(trace_setting));

    if (trace_size)
    {
        mono_jit_set_trace_options(trace_setting);
    }

    GetModuleFileNameA(NULL, filename, MAX_PATH);

    domain = mono_jit_init(filename);

    assembly = mono_domain_assembly_open(domain, filename);

    exit_code = mono_jit_exec(domain, assembly, argc, argv);

    mono_jit_cleanup(domain);

    HeapFree(GetProcessHeap(), 0, argv);

    return exit_code;
}

__int32 WINAPI _CorExeMain2(PBYTE ptrMemory, DWORD cntMemory, LPWSTR imageName, LPWSTR loaderName, LPWSTR cmdLine)
{
    TRACE("(%p, %u, %s, %s, %s)\n", ptrMemory, cntMemory, debugstr_w(imageName), debugstr_w(loaderName), debugstr_w(cmdLine));
    FIXME("Directly running .NET applications not supported.\n");
    return -1;
}

void WINAPI CorExitProcess(int exitCode)
{
    FIXME("(%x) stub\n", exitCode);
    ExitProcess(exitCode);
}

VOID WINAPI _CorImageUnloading(PVOID imageBase)
{
    TRACE("(%p): stub\n", imageBase);
}

HRESULT WINAPI _CorValidateImage(PVOID* imageBase, LPCWSTR imageName)
{
    TRACE("(%p, %s): stub\n", imageBase, debugstr_w(imageName));
    return E_FAIL;
}

HRESULT WINAPI GetCORSystemDirectory(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    FIXME("(%p, %d, %p): stub!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength)
        return E_POINTER;

    *dwLength = 0;

    return S_OK;
}

HRESULT WINAPI GetCORVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    static const WCHAR version[] = {'v','1','.','1','.','4','3','2','2',0};

    FIXME("(%p, %d, %p): semi-stub!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength)
        return E_POINTER;

    *dwLength = lstrlenW(version);

    if (cchBuffer < *dwLength)
        return ERROR_INSUFFICIENT_BUFFER;

    if (pbuffer)
        lstrcpyW(pbuffer, version);

    return S_OK;
}

HRESULT WINAPI GetRequestedRuntimeInfo(LPCWSTR pExe, LPCWSTR pwszVersion, LPCWSTR pConfigurationFile,
    DWORD startupFlags, DWORD runtimeInfoFlags, LPWSTR pDirectory, DWORD dwDirectory, DWORD *dwDirectoryLength,
    LPWSTR pVersion, DWORD cchBuffer, DWORD *dwlength)
{
    FIXME("(%s, %s, %s, 0x%08x, 0x%08x, %p, 0x%08x, %p, %p, 0x%08x, %p) stub\n", debugstr_w(pExe),
          debugstr_w(pwszVersion), debugstr_w(pConfigurationFile), startupFlags, runtimeInfoFlags, pDirectory,
          dwDirectory, dwDirectoryLength, pVersion, cchBuffer, dwlength);
    return GetCORVersion(pVersion, cchBuffer, dwlength);
}

HRESULT WINAPI LoadLibraryShim( LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE * phModDll)
{
    FIXME("(%p %s, %p, %p, %p): semi-stub\n", szDllName, debugstr_w(szDllName), szVersion, pvReserved, phModDll);

    if (phModDll) *phModDll = LoadLibraryW(szDllName);
    return S_OK;
}

HRESULT WINAPI LockClrVersion(FLockClrVersionCallback hostCallback, FLockClrVersionCallback *pBeginHostSetup, FLockClrVersionCallback *pEndHostSetup)
{
    FIXME("(%p %p %p): stub\n", hostCallback, pBeginHostSetup, pEndHostSetup);
    return S_OK;
}

HRESULT WINAPI CoInitializeCor(DWORD fFlags)
{
    FIXME("(0x%08x): stub\n", fFlags);
    return S_OK;
}

HRESULT WINAPI GetAssemblyMDImport(LPCWSTR szFileName, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("(%p %s, %s, %p): stub\n", szFileName, debugstr_w(szFileName), debugstr_guid(riid), *ppIUnk);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI GetVersionFromProcess(HANDLE hProcess, LPWSTR pVersion, DWORD cchBuffer, DWORD *dwLength)
{
    FIXME("(%p, %p, %d, %p): stub\n", hProcess, pVersion, cchBuffer, dwLength);
    return E_NOTIMPL;
}

HRESULT WINAPI LoadStringRCEx(LCID culture, UINT resId, LPWSTR pBuffer, int iBufLen, int bQuiet, int* pBufLen)
{
    HRESULT res = S_OK;
    if ((iBufLen <= 0) || !pBuffer)
        return E_INVALIDARG;
    pBuffer[0] = 0;
    if (resId) {
        FIXME("(%d, %x, %p, %d, %d, %p): semi-stub\n", culture, resId, pBuffer, iBufLen, bQuiet, pBufLen);
        res = E_NOTIMPL;
    }
    else
        res = E_FAIL;
    if (pBufLen)
        *pBufLen = lstrlenW(pBuffer);
    return res;
}

HRESULT WINAPI LoadStringRC(UINT resId, LPWSTR pBuffer, int iBufLen, int bQuiet)
{
    return LoadStringRCEx(-1, resId, pBuffer, iBufLen, bQuiet, NULL);
}

HRESULT WINAPI CorBindToRuntimeEx(LPWSTR szVersion, LPWSTR szBuildFlavor, DWORD nflags, REFCLSID rslsid,
                                  REFIID riid, LPVOID *ppv)
{
    FIXME("%s %s %d %s %s %p\n", debugstr_w(szVersion), debugstr_w(szBuildFlavor), nflags, debugstr_guid( rslsid ),
          debugstr_guid( riid ), ppv);

    if(IsEqualGUID( riid, &IID_ICorRuntimeHost ))
    {
        *ppv = create_corruntimehost();
        return S_OK;
    }
    *ppv = NULL;
    return E_NOTIMPL;
}

HRESULT WINAPI CorBindToCurrentRuntime(LPCWSTR filename, REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    FIXME("(%s, %s, %s, %p): stub\n", debugstr_w(filename), debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    return E_NOTIMPL;
}

STDAPI ClrCreateManagedInstance(LPCWSTR pTypeName, REFIID riid, void **ppObject)
{
    FIXME("(%s,%s,%p)\n", debugstr_w(pTypeName), debugstr_guid(riid), ppObject);
    return E_NOTIMPL;
}

BOOL WINAPI StrongNameSignatureVerification(LPCWSTR filename, DWORD inFlags, DWORD* pOutFlags)
{
    FIXME("(%s, 0x%X, %p): stub\n", debugstr_w(filename), inFlags, pOutFlags);
    return FALSE;
}

BOOL WINAPI StrongNameSignatureVerificationEx(LPCWSTR filename, BOOL forceVerification, BOOL* pVerified)
{
    FIXME("(%s, %u, %p): stub\n", debugstr_w(filename), forceVerification, pVerified);
    return FALSE;
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    FIXME("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);
    if(!ppv)
        return E_INVALIDARG;

    return E_NOTIMPL;
}

HRESULT WINAPI DllRegisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllUnregisterServer(void)
{
    FIXME("\n");
    return S_OK;
}

HRESULT WINAPI DllCanUnloadNow(VOID)
{
    return S_OK;
}

INT WINAPI ND_RU1( const void *ptr, INT offset )
{
    return *((const BYTE *)ptr + offset);
}

INT WINAPI ND_RI2( const void *ptr, INT offset )
{
    return *(const SHORT *)((const BYTE *)ptr + offset);
}

INT WINAPI ND_RI4( const void *ptr, INT offset )
{
    return *(const INT *)((const BYTE *)ptr + offset);
}

INT64 WINAPI ND_RI8( const void *ptr, INT offset )
{
    return *(const INT64 *)((const BYTE *)ptr + offset);
}

void WINAPI ND_WU1( void *ptr, INT offset, BYTE val )
{
    *((BYTE *)ptr + offset) = val;
}

void WINAPI ND_WI2( void *ptr, INT offset, SHORT val )
{
    *(SHORT *)((BYTE *)ptr + offset) = val;
}

void WINAPI ND_WI4( void *ptr, INT offset, INT val )
{
    *(INT *)((BYTE *)ptr + offset) = val;
}

void WINAPI ND_WI8( void *ptr, INT offset, INT64 val )
{
    *(INT64 *)((BYTE *)ptr + offset) = val;
}

void WINAPI ND_CopyObjDst( const void *src, void *dst, INT offset, INT size )
{
    memcpy( (BYTE *)dst + offset, src, size );
}

void WINAPI ND_CopyObjSrc( const void *src, INT offset, void *dst, INT size )
{
    memcpy( dst, (const BYTE *)src + offset, size );
}
