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

#define COBJMACROS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winnls.h"
#include "winreg.h"
#include "ole2.h"
#include "ocidl.h"
#include "shellapi.h"
#include "strongname.h"

#include "initguid.h"
#include "msxml2.h"
#include "corerror.h"
#include "cor.h"
#include "mscoree.h"
#include "corhdr.h"
#include "cordebug.h"
#include "metahost.h"
#include "fusion.h"
#include "wine/list.h"
#include "mscoree_private.h"
#include "rpcproxy.h"

#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL( mscoree );
WINE_DECLARE_DEBUG_CHANNEL(winediag);

struct print_handler_tls
{
    int length;
    char buffer[1018];
};

static DWORD print_tls_index = TLS_OUT_OF_INDEXES;

typedef HRESULT (*fnCreateInstance)(REFIID riid, LPVOID *ppObj);

char *WtoA(LPCWSTR wstr)
{
    int length;
    char *result;

    length = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);

    result = malloc(length);

    if (result)
        WideCharToMultiByte(CP_UTF8, 0, wstr, -1, result, length, NULL, NULL);

    return result;
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

typedef struct mscorecf
{
    IClassFactory    IClassFactory_iface;
    LONG ref;

    fnCreateInstance pfnCreateInstance;

    CLSID clsid;
} mscorecf;

static inline mscorecf *impl_from_IClassFactory( IClassFactory *iface )
{
    return CONTAINING_RECORD(iface, mscorecf, IClassFactory_iface);
}

static HRESULT WINAPI mscorecf_QueryInterface(IClassFactory *iface, REFIID riid, LPVOID *ppobj )
{
    TRACE("%s %p\n", debugstr_guid(riid), ppobj);

    if (IsEqualGUID(riid, &IID_IUnknown) ||
        IsEqualGUID(riid, &IID_IClassFactory))
    {
        IClassFactory_AddRef( iface );
        *ppobj = iface;
        return S_OK;
    }

    ERR("interface %s not implemented\n", debugstr_guid(riid));
    return E_NOINTERFACE;
}

static ULONG WINAPI mscorecf_AddRef(IClassFactory *iface )
{
    mscorecf *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedIncrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    return ref;
}

static ULONG WINAPI mscorecf_Release(IClassFactory *iface )
{
    mscorecf *This = impl_from_IClassFactory(iface);
    ULONG ref = InterlockedDecrement(&This->ref);

    TRACE("%p ref=%lu\n", This, ref);

    if (ref == 0)
    {
        free(This);
    }

    return ref;
}

static HRESULT WINAPI mscorecf_CreateInstance(IClassFactory *iface,LPUNKNOWN pOuter,
                            REFIID riid, LPVOID *ppobj )
{
    mscorecf *This = impl_from_IClassFactory( iface );
    HRESULT hr;
    IUnknown *punk;

    TRACE("%p %s %p\n", pOuter, debugstr_guid(riid), ppobj );

    *ppobj = NULL;

    if (pOuter)
        return CLASS_E_NOAGGREGATION;

    hr = This->pfnCreateInstance( &This->clsid, (LPVOID*) &punk );
    if (SUCCEEDED(hr))
    {
        hr = IUnknown_QueryInterface( punk, riid, ppobj );

        IUnknown_Release( punk );
    }
    else
    {
        WARN("Cannot create an instance object. 0x%08lx\n", hr);
    }
    return hr;
}

static HRESULT WINAPI mscorecf_LockServer(IClassFactory *iface, BOOL dolock)
{
    FIXME("(%p)->(%d),stub!\n",iface,dolock);
    return S_OK;
}

static const struct IClassFactoryVtbl mscorecf_vtbl =
{
    mscorecf_QueryInterface,
    mscorecf_AddRef,
    mscorecf_Release,
    mscorecf_CreateInstance,
    mscorecf_LockServer
};

HRESULT WINAPI CorBindToRuntimeHost(LPCWSTR pwszVersion, LPCWSTR pwszBuildFlavor,
                                    LPCWSTR pwszHostConfigFile, VOID *pReserved,
                                    DWORD startupFlags, REFCLSID rclsid,
                                    REFIID riid, LPVOID *ppv)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;

    TRACE("(%s, %s, %s, %p, %ld, %s, %s, %p)\n", debugstr_w(pwszVersion),
          debugstr_w(pwszBuildFlavor), debugstr_w(pwszHostConfigFile), pReserved,
          startupFlags, debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    *ppv = NULL;

    ret = get_runtime_info(NULL, pwszVersion, pwszHostConfigFile, NULL, startupFlags, 0, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        ret = ICLRRuntimeInfo_GetInterface(info, rclsid, riid, ppv);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

void CDECL mono_print_handler_fn(const char *string, INT is_stdout)
{
    struct print_handler_tls *tls = TlsGetValue(print_tls_index);

    if (!tls)
    {
        tls = malloc(sizeof(*tls));
        tls->length = 0;
        TlsSetValue(print_tls_index, tls);
    }

    while (*string)
    {
        int remaining_buffer = sizeof(tls->buffer) - tls->length;
        int length = strlen(string);
        const char *newline = memchr(string, '\n', min(length, remaining_buffer));

        if (newline)
        {
            length = newline - string + 1;
            wine_dbg_printf("%.*s%.*s", tls->length, tls->buffer, length, string);
            tls->length = 0;
            string += length;
        }
        else if (length > remaining_buffer)
        {
            /* this would overflow Wine's debug buffer */
            wine_dbg_printf("%.*s%.*s\n", tls->length, tls->buffer, remaining_buffer, string);
            tls->length = 0;
            string += remaining_buffer;
        }
        else
        {
            memcpy(tls->buffer + tls->length, string, length);
            tls->length += length;
            break;
        }
    }
}

void CDECL mono_log_handler_fn(const char *log_domain, const char *log_level, const char *message, INT fatal, void *user_data)
{
    SIZE_T len = (log_domain ? strlen(log_domain) + 2 : 0) + strlen(message) + strlen("\n") + 1;
    char *msg = calloc(len, sizeof(char));

    if (msg)
    {
        sprintf(msg, "%s%s%s\n", log_domain ? log_domain : "", log_domain ? ": " : "", message);
        mono_print_handler_fn(msg, 0);
    }

    free(msg);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    TRACE("(%p, %ld, %p)\n", hinstDLL, fdwReason, lpvReserved);

    switch (fdwReason)
    {
    case DLL_PROCESS_ATTACH:
        runtimehost_init();

        print_tls_index = TlsAlloc();

        if (print_tls_index == TLS_OUT_OF_INDEXES)
            return FALSE;

        break;
    case DLL_THREAD_DETACH:
        if (print_tls_index != TLS_OUT_OF_INDEXES)
            free(TlsGetValue(print_tls_index));
        break;
    case DLL_PROCESS_DETACH:
        expect_no_runtimes();
        if (lpvReserved) break; /* process is terminating */
        runtimehost_uninit();
        if (print_tls_index != TLS_OUT_OF_INDEXES)
        {
            free(TlsGetValue(print_tls_index));
            TlsFree(print_tls_index);
        }
        break;
    }
    return TRUE;
}

__int32 WINAPI _CorExeMain2(PBYTE ptrMemory, DWORD cntMemory, LPWSTR imageName, LPWSTR loaderName, LPWSTR cmdLine)
{
    TRACE("(%p, %lu, %s, %s, %s)\n", ptrMemory, cntMemory, debugstr_w(imageName), debugstr_w(loaderName), debugstr_w(cmdLine));
    FIXME("Directly running .NET applications not supported.\n");
    return -1;
}

void WINAPI CorExitProcess(int exitCode)
{
    TRACE("(%x)\n", exitCode);
    CLRMetaHost_ExitProcess(0, exitCode);
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
    ICLRRuntimeInfo *info;
    HRESULT ret;

    TRACE("(%p, %ld, %p)!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength || !pbuffer)
        return E_POINTER;

    ret = get_runtime_info(NULL, NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        *dwLength = cchBuffer;
        ret = ICLRRuntimeInfo_GetRuntimeDirectory(info, pbuffer, dwLength);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI GetCORVersion(LPWSTR pbuffer, DWORD cchBuffer, DWORD *dwLength)
{
    ICLRRuntimeInfo *info;
    HRESULT ret;

    TRACE("(%p, %ld, %p)!\n", pbuffer, cchBuffer, dwLength);

    if (!dwLength || !pbuffer)
        return E_POINTER;

    ret = get_runtime_info(NULL, NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        *dwLength = cchBuffer;
        ret = ICLRRuntimeInfo_GetVersionString(info, pbuffer, dwLength);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI CorIsLatestSvc(int *unk1, int *unk2)
{
    ERR_(winediag)("If this function is called, it is likely the result of a broken .NET installation\n");

    if (!unk1 || !unk2)
        return E_POINTER;

    return S_OK;
}

HRESULT WINAPI CorGetSvc(void *unk)
{
    ERR_(winediag)("If this function is called, it is likely the result of a broken .NET installation\n");

    return E_NOTIMPL;
}

HRESULT WINAPI GetRequestedRuntimeInfo(LPCWSTR pExe, LPCWSTR pwszVersion, LPCWSTR pConfigurationFile,
    DWORD startupFlags, DWORD runtimeInfoFlags, LPWSTR pDirectory, DWORD dwDirectory, DWORD *dwDirectoryLength,
    LPWSTR pVersion, DWORD cchBuffer, DWORD *dwlength)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;
    DWORD length_dummy;

    TRACE("(%s, %s, %s, 0x%08lx, 0x%08lx, %p, 0x%08lx, %p, %p, 0x%08lx, %p)\n", debugstr_w(pExe),
          debugstr_w(pwszVersion), debugstr_w(pConfigurationFile), startupFlags, runtimeInfoFlags, pDirectory,
          dwDirectory, dwDirectoryLength, pVersion, cchBuffer, dwlength);

    if (!dwDirectoryLength) dwDirectoryLength = &length_dummy;

    if (!dwlength) dwlength = &length_dummy;

    ret = get_runtime_info(pExe, pwszVersion, pConfigurationFile, NULL, startupFlags, runtimeInfoFlags, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        *dwlength = cchBuffer;
        ret = ICLRRuntimeInfo_GetVersionString(info, pVersion, dwlength);

        if (SUCCEEDED(ret))
        {
            if(pwszVersion)
                pVersion[0] = pwszVersion[0];

            *dwDirectoryLength = dwDirectory;
            ret = ICLRRuntimeInfo_GetRuntimeDirectory(info, pDirectory, dwDirectoryLength);
        }

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI GetRequestedRuntimeVersion(LPWSTR pExe, LPWSTR pVersion, DWORD cchBuffer, DWORD *dwlength)
{
    TRACE("(%s, %p, %ld, %p)\n", debugstr_w(pExe), pVersion, cchBuffer, dwlength);

    if(!dwlength)
        return E_POINTER;

    return GetRequestedRuntimeInfo(pExe, NULL, NULL, 0, 0, NULL, 0, NULL, pVersion, cchBuffer, dwlength);
}

HRESULT WINAPI GetRealProcAddress(LPCSTR procname, void **ppv)
{
    FIXME("(%s, %p)\n", debugstr_a(procname), ppv);
    return CLR_E_SHIM_RUNTIMEEXPORT;
}

HRESULT WINAPI GetFileVersion(LPCWSTR szFilename, LPWSTR szBuffer, DWORD cchBuffer, DWORD *dwLength)
{
    TRACE("(%s, %p, %ld, %p)\n", debugstr_w(szFilename), szBuffer, cchBuffer, dwLength);

    if (!szFilename || !dwLength)
        return E_POINTER;

    *dwLength = cchBuffer;
    return CLRMetaHost_GetVersionFromFile(0, szFilename, szBuffer, dwLength);
}

HRESULT WINAPI LoadLibraryShim( LPCWSTR szDllName, LPCWSTR szVersion, LPVOID pvReserved, HMODULE * phModDll)
{
    HRESULT ret=S_OK;
    WCHAR dll_filename[MAX_PATH];
    WCHAR version[MAX_PATH];
    static const WCHAR default_version[] = {'v','1','.','1','.','4','3','2','2',0};
    static const WCHAR slash[] = {'\\',0};
    DWORD dummy;

    TRACE("(%p %s, %p, %p, %p)\n", szDllName, debugstr_w(szDllName), szVersion, pvReserved, phModDll);

    if (!szDllName || !phModDll)
        return E_POINTER;

    if (!get_install_root(dll_filename))
    {
        ERR("error reading registry key for installroot\n");
        dll_filename[0] = 0;
    }
    else
    {
        if (!szVersion)
        {
            ret = GetCORVersion(version, MAX_PATH, &dummy);
            if (SUCCEEDED(ret))
                szVersion = version;
            else
                szVersion = default_version;
        }
        lstrcatW(dll_filename, szVersion);
        lstrcatW(dll_filename, slash);
    }

    lstrcatW(dll_filename, szDllName);

    *phModDll = LoadLibraryW(dll_filename);

    return *phModDll ? S_OK : E_HANDLE;
}

HRESULT WINAPI LockClrVersion(FLockClrVersionCallback hostCallback, FLockClrVersionCallback *pBeginHostSetup, FLockClrVersionCallback *pEndHostSetup)
{
    FIXME("(%p %p %p): stub\n", hostCallback, pBeginHostSetup, pEndHostSetup);
    return S_OK;
}

HRESULT WINAPI CoInitializeCor(DWORD fFlags)
{
    FIXME("(0x%08lx): stub\n", fFlags);
    return S_OK;
}

HRESULT WINAPI GetAssemblyMDImport(LPCWSTR szFileName, REFIID riid, IUnknown **ppIUnk)
{
    FIXME("(%p %s, %s, %p): stub\n", szFileName, debugstr_w(szFileName), debugstr_guid(riid), *ppIUnk);
    return ERROR_CALL_NOT_IMPLEMENTED;
}

HRESULT WINAPI GetVersionFromProcess(HANDLE hProcess, LPWSTR pVersion, DWORD cchBuffer, DWORD *dwLength)
{
    FIXME("(%p, %p, %ld, %p): stub\n", hProcess, pVersion, cchBuffer, dwLength);
    return E_NOTIMPL;
}

HRESULT WINAPI LoadStringRCEx(LCID culture, UINT resId, LPWSTR pBuffer, int iBufLen, int bQuiet, int* pBufLen)
{
    HRESULT res = S_OK;
    if ((iBufLen <= 0) || !pBuffer)
        return E_INVALIDARG;
    pBuffer[0] = 0;
    if (resId) {
        FIXME("(%ld, %x, %p, %d, %d, %p): semi-stub\n", culture, resId, pBuffer, iBufLen, bQuiet, pBufLen);
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
    HRESULT ret;
    ICLRRuntimeInfo *info;

    TRACE("%s %s %ld %s %s %p\n", debugstr_w(szVersion), debugstr_w(szBuildFlavor), nflags, debugstr_guid( rslsid ),
          debugstr_guid( riid ), ppv);

    *ppv = NULL;

    ret = get_runtime_info(NULL, szVersion, NULL, NULL, nflags, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        ret = ICLRRuntimeInfo_GetInterface(info, rslsid, riid, ppv);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

HRESULT WINAPI CorBindToCurrentRuntime(LPCWSTR filename, REFCLSID rclsid, REFIID riid, LPVOID *ppv)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;

    TRACE("(%s, %s, %s, %p)\n", debugstr_w(filename), debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    *ppv = NULL;

    ret = get_runtime_info(NULL, NULL, filename, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        ret = ICLRRuntimeInfo_GetInterface(info, rclsid, riid, ppv);

        ICLRRuntimeInfo_Release(info);
    }

    return ret;
}

STDAPI ClrCreateManagedInstance(LPCWSTR pTypeName, REFIID riid, void **ppObject)
{
    HRESULT ret;
    ICLRRuntimeInfo *info;
    RuntimeHost *host;
    MonoObject *obj;
    IUnknown *unk;

    TRACE("(%s,%s,%p)\n", debugstr_w(pTypeName), debugstr_guid(riid), ppObject);

    /* FIXME: How to determine which runtime version to use? */
    ret = get_runtime_info(NULL, NULL, NULL, NULL, 0, RUNTIME_INFO_UPGRADE_VERSION, TRUE, &info);

    if (SUCCEEDED(ret))
    {
        ret = ICLRRuntimeInfo_GetRuntimeHost(info, &host);

        ICLRRuntimeInfo_Release(info);
    }

    if (SUCCEEDED(ret))
        ret = RuntimeHost_CreateManagedInstance(host, pTypeName, NULL, &obj);

    if (SUCCEEDED(ret))
        ret = RuntimeHost_GetIUnknownForObject(host, obj, &unk);

    if (SUCCEEDED(ret))
    {
        ret = IUnknown_QueryInterface(unk, riid, ppObject);
        IUnknown_Release(unk);
    }

    return ret;
}

BOOLEAN WINAPI StrongNameSignatureVerification(LPCWSTR filename, DWORD inFlags, DWORD *pOutFlags)
{
    FIXME("(%s, 0x%lX, %p): stub\n", debugstr_w(filename), inFlags, pOutFlags);
    return FALSE;
}

BOOLEAN WINAPI StrongNameSignatureVerificationEx(LPCWSTR filename, BOOLEAN forceVerification, BOOLEAN *pVerified)
{
    FIXME("(%s, %u, %p): stub\n", debugstr_w(filename), forceVerification, pVerified);
    *pVerified = TRUE;
    return TRUE;
}

BOOLEAN WINAPI StrongNameTokenFromAssembly(LPCWSTR path, BYTE **token, ULONG *size)
{
    FIXME("(%s, %p, %p): stub\n", debugstr_w(path), token, size);
    return FALSE;
}

HRESULT WINAPI CreateDebuggingInterfaceFromVersion(int nDebugVersion, LPCWSTR version, IUnknown **ppv)
{
    static const WCHAR v2_0[] = {'v','2','.','0','.','5','0','7','2','7',0};
    HRESULT hr = E_FAIL;
    ICLRRuntimeInfo *runtimeinfo;

    if(nDebugVersion < 1 || nDebugVersion > 4)
        return E_INVALIDARG;

    TRACE("(%d %s, %p): stub\n", nDebugVersion, debugstr_w(version), ppv);

    if(!ppv)
        return E_INVALIDARG;

    *ppv = NULL;

    if(wcscmp(version, v2_0) != 0)
    {
        FIXME("Currently .NET Version '%s' not support.\n", debugstr_w(version));
        return E_INVALIDARG;
    }

    if(nDebugVersion != 3)
        return E_INVALIDARG;

    hr = CLRMetaHost_GetRuntime(0, version, &IID_ICLRRuntimeInfo, (void**)&runtimeinfo);
    if(hr == S_OK)
    {
        hr = ICLRRuntimeInfo_GetInterface(runtimeinfo, &CLSID_CLRDebuggingLegacy, &IID_ICorDebug, (void**)ppv);

        ICLRRuntimeInfo_Release(runtimeinfo);
    }

    if(!*ppv)
        return E_FAIL;

    return hr;
}

HRESULT WINAPI CLRCreateInstance(REFCLSID clsid, REFIID riid, LPVOID *ppInterface)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(clsid), debugstr_guid(riid), ppInterface);

    if (IsEqualGUID(clsid, &CLSID_CLRMetaHost))
        return CLRMetaHost_CreateInstance(riid, ppInterface);
    if (IsEqualGUID(clsid, &CLSID_CLRMetaHostPolicy))
        return CLRMetaHostPolicy_CreateInstance(riid, ppInterface);

    FIXME("not implemented for class %s\n", debugstr_guid(clsid));

    return CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT WINAPI CreateInterface(REFCLSID clsid, REFIID riid, LPVOID *ppInterface)
{
    TRACE("(%s,%s,%p)\n", debugstr_guid(clsid), debugstr_guid(riid), ppInterface);

    return CLRCreateInstance(clsid, riid, ppInterface);
}

HRESULT WINAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
    mscorecf *This;
    HRESULT hr;

    TRACE("(%s, %s, %p): stub\n", debugstr_guid(rclsid), debugstr_guid(riid), ppv);

    if(!ppv)
        return E_INVALIDARG;

    This = malloc(sizeof(mscorecf));

    This->IClassFactory_iface.lpVtbl = &mscorecf_vtbl;
    This->pfnCreateInstance = create_monodata;
    This->ref = 1;
    This->clsid = *rclsid;

    hr = IClassFactory_QueryInterface( &This->IClassFactory_iface, riid, ppv );
    IClassFactory_Release(&This->IClassFactory_iface);

    return hr;
}

static void parse_msi_version_string(const char *version, int *parts)
{
    const char *minor_start, *build_start;

    parts[0] = atoi(version);

    parts[1] = parts[2] = 0;

    minor_start = strchr(version, '.');
    if (minor_start)
    {
        minor_start++;
        parts[1] = atoi(minor_start);

        build_start = strchr(minor_start, '.');
        if (build_start)
            parts[2] = atoi(build_start+1);
    }
}

static int compare_versions(const char *a, const char *b)
{
    int a_parts[3], b_parts[3], i;

    parse_msi_version_string(a, a_parts);
    parse_msi_version_string(b, b_parts);

    for (i=0; i<3; i++)
        if (a_parts[i] != b_parts[i])
            return a_parts[i] - b_parts[i];

    return 0;
}

static BOOL invoke_appwiz(void)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    WCHAR app[MAX_PATH];
    WCHAR *args;
    LONG len;
    BOOL ret;

    static const WCHAR controlW[] = {'\\','c','o','n','t','r','o','l','.','e','x','e',0};
    static const WCHAR argsW[] =
        {' ','a','p','p','w','i','z','.','c','p','l',' ','i','n','s','t','a','l','l','_','m','o','n','o',0};

    len = GetSystemDirectoryW(app, MAX_PATH - ARRAY_SIZE(controlW));
    memcpy(app+len, controlW, sizeof(controlW));

    args = malloc(len * sizeof(WCHAR) + sizeof(controlW) + sizeof(argsW));
    if(!args)
        return FALSE;

    memcpy(args, app, len*sizeof(WCHAR) + sizeof(controlW));
    memcpy(args + len + ARRAY_SIZE(controlW) - 1, argsW, sizeof(argsW));

    TRACE("starting %s\n", debugstr_w(args));

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessW(app, args, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    free(args);
    if (ret) {
        CloseHandle(pi.hThread);
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
    }

    return ret;
}

static BOOL get_support_msi(LPCWSTR mono_path, LPWSTR msi_path)
{
    static const WCHAR support_msi_relative[] = {'\\','s','u','p','p','o','r','t','\\','w','i','n','e','m','o','n','o','-','s','u','p','p','o','r','t','.','m','s','i',0};
    UINT (WINAPI *pMsiOpenPackageW)(LPCWSTR,ULONG*);
    UINT (WINAPI *pMsiGetProductPropertyA)(ULONG,LPCSTR,LPSTR,LPDWORD);
    UINT (WINAPI *pMsiCloseHandle)(ULONG);
    HMODULE hmsi = NULL;
    char versionstringbuf[15];
    UINT res;
    DWORD buffer_size;
    ULONG msiproduct;
    BOOL ret=FALSE;

    hmsi = GetModuleHandleA("msi");

    lstrcpyW(msi_path, mono_path);
    lstrcatW(msi_path, support_msi_relative);

    pMsiOpenPackageW = (void*)GetProcAddress(hmsi, "MsiOpenPackageW");

    res = pMsiOpenPackageW(msi_path, &msiproduct);

    if (res == ERROR_SUCCESS)
    {
        buffer_size = sizeof(versionstringbuf);

        pMsiGetProductPropertyA = (void*)GetProcAddress(hmsi, "MsiGetProductPropertyA");

        res = pMsiGetProductPropertyA(msiproduct, "ProductVersion", versionstringbuf, &buffer_size);

        pMsiCloseHandle = (void*)GetProcAddress(hmsi, "MsiCloseHandle");

        pMsiCloseHandle(msiproduct);
    }

    if (res == ERROR_SUCCESS) {
        TRACE("found support msi version %s at %s\n", versionstringbuf, debugstr_w(msi_path));

        if (compare_versions(WINE_MONO_VERSION, versionstringbuf) <= 0)
        {
            ret = TRUE;
        }
    }

    return ret;
}

static BOOL install_wine_mono(void)
{
    BOOL is_wow64 = FALSE;
    HMODULE hmsi = NULL;
    HRESULT initresult = E_FAIL;
    UINT (WINAPI *pMsiEnumRelatedProductsA)(LPCSTR,DWORD,DWORD,LPSTR);
    UINT (WINAPI *pMsiGetProductInfoA)(LPCSTR,LPCSTR,LPSTR,DWORD*);
    UINT (WINAPI *pMsiInstallProductW)(LPCWSTR,LPCWSTR);
    char versionstringbuf[15];
    char productcodebuf[39];
    UINT res;
    DWORD buffer_size;
    BOOL ret;
    WCHAR mono_path[MAX_PATH];
    WCHAR support_msi_path[MAX_PATH];

    static const char* mono_upgrade_code = "{DE624609-C6B5-486A-9274-EF0B854F6BC5}";

    IsWow64Process(GetCurrentProcess(), &is_wow64);

    if (is_wow64)
    {
        TRACE("not installing mono in wow64 process\n");
        return TRUE;
    }

    TRACE("searching for mono runtime\n");

    if (!get_mono_path(mono_path, FALSE))
    {
        TRACE("mono runtime not found\n");
        return invoke_appwiz();
    }

    TRACE("mono runtime is at %s\n", debugstr_w(mono_path));

    hmsi = LoadLibraryA("msi");

    if (!hmsi)
    {
        ERR("couldn't load msi.dll\n");
        return FALSE;
    }

    pMsiEnumRelatedProductsA = (void*)GetProcAddress(hmsi, "MsiEnumRelatedProductsA");

    res = pMsiEnumRelatedProductsA(mono_upgrade_code, 0, 0, productcodebuf);

    if (res == ERROR_SUCCESS)
    {
        pMsiGetProductInfoA = (void*)GetProcAddress(hmsi, "MsiGetProductInfoA");

        buffer_size = sizeof(versionstringbuf);

        res = pMsiGetProductInfoA(productcodebuf, "VersionString", versionstringbuf, &buffer_size);
    }
    else if (res != ERROR_NO_MORE_ITEMS)
    {
        ERR("MsiEnumRelatedProducts failed, err=%u\n", res);
    }

    if (res == ERROR_SUCCESS)
    {
        TRACE("found installed support package %s\n", versionstringbuf);

        if (compare_versions(WINE_MONO_VERSION, versionstringbuf) <= 0)
        {
            TRACE("support package is at least %s, quitting\n", WINE_MONO_VERSION);
            ret = TRUE;
            goto end;
        }
    }

    initresult = CoInitialize(NULL);

    ret = get_support_msi(mono_path, support_msi_path);
    if (!ret)
    {
        /* Try looking outside c:\windows\mono */
        ret = (get_mono_path(mono_path, TRUE) &&
            get_support_msi(mono_path, support_msi_path));
    }

    if (ret)
    {
        TRACE("installing support msi\n");

        pMsiInstallProductW = (void*)GetProcAddress(hmsi, "MsiInstallProductW");

        res = pMsiInstallProductW(support_msi_path, NULL);

        if (res == ERROR_SUCCESS)
        {
            ret = TRUE;
            goto end;
        }
        else
            ERR("MsiInstallProduct failed, err=%i\n", res);
    }

    ret = invoke_appwiz();

end:
    if (hmsi)
        FreeLibrary(hmsi);
    if (SUCCEEDED(initresult))
        CoUninitialize();
    return ret;
}

HRESULT WINAPI DllRegisterServer(void)
{
    install_wine_mono();

    return __wine_register_resources();
}

HRESULT WINAPI DllUnregisterServer(void)
{
    return __wine_unregister_resources();
}

void WINAPI CoEEShutDownCOM(void)
{
    FIXME("stub.\n");
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
