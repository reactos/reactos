/*
 * Copyright 2006-2007 Jacek Caban for CodeWeavers
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

#include "config.h"

#include <stdarg.h>
#include <fcntl.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#define COBJMACROS
#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winreg.h"
#include "ole2.h"
#include "commctrl.h"
#include "advpub.h"
#include "wininet.h"
#include "shellapi.h"

#include "wine/debug.h"
#include "wine/unicode.h"
#include "wine/library.h"

#include "mshtml_private.h"
#include "resource.h"

WINE_DEFAULT_DEBUG_CHANNEL(mshtml);

#define GECKO_FILE_NAME "wine_gecko-" GECKO_VERSION ".cab"

static const WCHAR mshtml_keyW[] =
    {'S','o','f','t','w','a','r','e',
     '\\','W','i','n','e',
     '\\','M','S','H','T','M','L',0};

static const CHAR mshtml_keyA[] =
    {'S','o','f','t','w','a','r','e',
    '\\','W','i','n','e',
    '\\','M','S','H','T','M','L',0};

static HWND install_dialog = NULL;
static LPWSTR tmp_file_name = NULL;
static HANDLE tmp_file = INVALID_HANDLE_VALUE;
static LPWSTR url = NULL;

static void clean_up(void)
{
    if(tmp_file != INVALID_HANDLE_VALUE)
        CloseHandle(tmp_file);

    if(tmp_file_name) {
        DeleteFileW(tmp_file_name);
        heap_free(tmp_file_name);
        tmp_file_name = NULL;
    }

    if(tmp_file != INVALID_HANDLE_VALUE) {
        CloseHandle(tmp_file);
        tmp_file = INVALID_HANDLE_VALUE;
    }

    if(install_dialog)
        EndDialog(install_dialog, 0);
}

static void set_status(DWORD id)
{
    HWND status = GetDlgItem(install_dialog, ID_DWL_STATUS);
    WCHAR buf[64];

    LoadStringW(hInst, id, buf, sizeof(buf)/sizeof(WCHAR));
    SendMessageW(status, WM_SETTEXT, 0, (LPARAM)buf);
}

static void set_registry(LPCSTR install_dir)
{
    WCHAR mshtml_key[100];
    LPWSTR gecko_path;
    HKEY hkey;
    DWORD res, len;

    static const WCHAR wszGeckoPath[] = {'G','e','c','k','o','P','a','t','h',0};
    static const WCHAR wszWineGecko[] = {'w','i','n','e','_','g','e','c','k','o',0};

    memcpy(mshtml_key, mshtml_keyW, sizeof(mshtml_keyW));
    mshtml_key[sizeof(mshtml_keyW)/sizeof(WCHAR)-1] = '\\';
    MultiByteToWideChar(CP_ACP, 0, GECKO_VERSION, sizeof(GECKO_VERSION),
            mshtml_key+sizeof(mshtml_keyW)/sizeof(WCHAR),
            (sizeof(mshtml_key)-sizeof(mshtml_keyW))/sizeof(WCHAR));

    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML\<version> */
    res = RegCreateKeyW(HKEY_CURRENT_USER, mshtml_key, &hkey);
    if(res != ERROR_SUCCESS) {
        ERR("Faild to create MSHTML key: %d\n", res);
        return;
    }

    len = MultiByteToWideChar(CP_ACP, 0, install_dir, -1, NULL, 0)-1;
    gecko_path = heap_alloc((len+1)*sizeof(WCHAR)+sizeof(wszWineGecko));
    MultiByteToWideChar(CP_ACP, 0, install_dir, -1, gecko_path, len+1);

    if (len && gecko_path[len-1] != '\\')
        gecko_path[len++] = '\\';

    memcpy(gecko_path+len, wszWineGecko, sizeof(wszWineGecko));

    res = RegSetValueExW(hkey, wszGeckoPath, 0, REG_SZ, (LPVOID)gecko_path,
                       len*sizeof(WCHAR)+sizeof(wszWineGecko));
    heap_free(gecko_path);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS)
        ERR("Failed to set GeckoPath value: %08x\n", res);
}

static BOOL install_cab(LPCWSTR file_name)
{
    HMODULE advpack;
    char install_dir[MAX_PATH];
    HRESULT (WINAPI *pExtractFilesA)(LPCSTR,LPCSTR,DWORD,LPCSTR,LPVOID,DWORD);
    LPSTR file_name_a;
    DWORD res;
    HRESULT hres;

    static const WCHAR wszAdvpack[] = {'a','d','v','p','a','c','k','.','d','l','l',0};

    TRACE("(%s)\n", debugstr_w(file_name));

    GetWindowsDirectoryA(install_dir, sizeof(install_dir));
    strcat(install_dir, "\\gecko\\");
    res = CreateDirectoryA(install_dir, NULL);
    if(!res && GetLastError() != ERROR_ALREADY_EXISTS) {
        ERR("Could not create directory: %08u\n", GetLastError());
        return FALSE;
    }

    strcat(install_dir, GECKO_VERSION);
    res = CreateDirectoryA(install_dir, NULL);
    if(!res && GetLastError() != ERROR_ALREADY_EXISTS) {
        ERR("Could not create directory: %08u\n", GetLastError());
        return FALSE;
    }

    advpack = LoadLibraryW(wszAdvpack);
    pExtractFilesA = (void *)GetProcAddress(advpack, "ExtractFiles");

    /* FIXME: Use unicode version (not yet implemented) */
    file_name_a = heap_strdupWtoA(file_name);
    hres = pExtractFilesA(file_name_a, install_dir, 0, NULL, NULL, 0);
    FreeLibrary(advpack);
    heap_free(file_name_a);
    if(FAILED(hres)) {
        ERR("Could not extract package: %08x\n", hres);
        clean_up();
        return FALSE;
    }

    set_registry(install_dir);
    clean_up();

    return TRUE;
}

static BOOL install_from_unix_file(const char *file_name)
{
    LPWSTR dos_file_name;
    int fd;
    BOOL ret;

    static WCHAR *(*wine_get_dos_file_name)(const char*);
    static const WCHAR kernel32W[] = {'k','e','r','n','e','l','3','2','.','d','l','l',0};

    fd = open(file_name, O_RDONLY);
    if(fd == -1) {
        TRACE("%s not found\n", debugstr_a(file_name));
        return FALSE;
    }

    close(fd);

    if(!wine_get_dos_file_name)
        wine_get_dos_file_name = (void*)GetProcAddress(GetModuleHandleW(kernel32W), "wine_get_dos_file_name");

    if(wine_get_dos_file_name) { /* Wine UNIX mode */
	dos_file_name = wine_get_dos_file_name(file_name);
	if(!dos_file_name) {
	    ERR("Could not get dos file name of %s\n", debugstr_a(file_name));
	    return FALSE;
	}
    } else { /* Windows mode */
	UINT res;
	WARN("Could not get wine_get_dos_file_name function, calling install_cab directly.\n");
	res = MultiByteToWideChar( CP_ACP, 0, file_name, -1, 0, 0);
	dos_file_name = heap_alloc (res*sizeof(WCHAR));
	MultiByteToWideChar( CP_ACP, 0, file_name, -1, dos_file_name, res);
    }

    ret = install_cab(dos_file_name);

    heap_free(dos_file_name);
    return ret;
}

static BOOL install_from_registered_dir(void)
{
    char *file_name;
    DWORD res, type, size = MAX_PATH;
    BOOL ret;

    file_name = heap_alloc(size+sizeof(GECKO_FILE_NAME));
    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML */
    res = RegGetValueA(HKEY_CURRENT_USER, mshtml_keyA, "GeckoCabDir", RRF_RT_ANY, &type, (PBYTE)file_name, &size);
    if(res == ERROR_MORE_DATA) {
        file_name = heap_realloc(file_name, size+sizeof(GECKO_FILE_NAME));
        res = RegGetValueA(HKEY_CURRENT_USER, mshtml_keyA, "GeckoCabDir", RRF_RT_ANY, &type, (PBYTE)file_name, &size);
    }
    
    if(res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        heap_free(file_name);
        return FALSE;
    }

    strcat(file_name, GECKO_FILE_NAME);

    TRACE("Trying %s\n", debugstr_a(file_name));

    ret = install_from_unix_file(file_name);

    heap_free(file_name);
    return ret;
}

static BOOL install_from_default_dir(void)
{
    const char *data_dir, *subdir;
    char *file_name;
    int len, len2;
    BOOL ret;

    if((data_dir = wine_get_data_dir()))
        subdir = "/gecko/";
    else if((data_dir = wine_get_build_dir()))
        subdir = "/../gecko/";
    else
        return FALSE;

    len = strlen(data_dir);
    len2 = strlen(subdir);

    file_name = heap_alloc(len+len2+sizeof(GECKO_FILE_NAME));
    memcpy(file_name, data_dir, len);
    memcpy(file_name+len, subdir, len2);
    memcpy(file_name+len+len2, GECKO_FILE_NAME, sizeof(GECKO_FILE_NAME));

    ret = install_from_unix_file(file_name);

    heap_free(file_name);
    return ret;
}

static HRESULT WINAPI InstallCallback_QueryInterface(IBindStatusCallback *iface,
                                                     REFIID riid, void **ppv)
{
    if(IsEqualGUID(&IID_IUnknown, riid) || IsEqualGUID(&IID_IBindStatusCallback, riid)) {
        *ppv = iface;
        return S_OK;
    }

    return E_INVALIDARG;
}

static ULONG WINAPI InstallCallback_AddRef(IBindStatusCallback *iface)
{
    return 2;
}

static ULONG WINAPI InstallCallback_Release(IBindStatusCallback *iface)
{
    return 1;
}

static HRESULT WINAPI InstallCallback_OnStartBinding(IBindStatusCallback *iface,
        DWORD dwReserved, IBinding *pib)
{
    WCHAR tmp_dir[MAX_PATH];

    set_status(IDS_DOWNLOADING);

    GetTempPathW(sizeof(tmp_dir)/sizeof(WCHAR), tmp_dir);

    tmp_file_name = heap_alloc(MAX_PATH*sizeof(WCHAR));
    GetTempFileNameW(tmp_dir, NULL, 0, tmp_file_name);

    TRACE("creating temp file %s\n", debugstr_w(tmp_file_name));

    tmp_file = CreateFileW(tmp_file_name, GENERIC_WRITE, 0, NULL, 
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    if(tmp_file == INVALID_HANDLE_VALUE) {
        ERR("Could not create file: %d\n", GetLastError());
        clean_up();
        return E_FAIL;
    }

    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetPriority(IBindStatusCallback *iface,
        LONG *pnPriority)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnLowResource(IBindStatusCallback *iface,
       DWORD dwReserved)
{
    return E_NOTIMPL;
}

static HRESULT WINAPI InstallCallback_OnProgress(IBindStatusCallback *iface, ULONG ulProgress,
        ULONG ulProgressMax, ULONG ulStatusCode, LPCWSTR szStatusText)
{
    HWND progress = GetDlgItem(install_dialog, ID_DWL_PROGRESS);

    if(ulProgressMax)
        SendMessageW(progress, PBM_SETRANGE32, 0, ulProgressMax);
    if(ulProgress)
        SendMessageW(progress, PBM_SETPOS, ulProgress, 0);

    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnStopBinding(IBindStatusCallback *iface,
        HRESULT hresult, LPCWSTR szError)
{
    if(FAILED(hresult)) {
        ERR("Binding failed %08x\n", hresult);
        clean_up();
        return S_OK;
    }

    CloseHandle(tmp_file);
    tmp_file = INVALID_HANDLE_VALUE;

    set_status(IDS_INSTALLING);

    install_cab(tmp_file_name);

    return S_OK;
}

static HRESULT WINAPI InstallCallback_GetBindInfo(IBindStatusCallback *iface,
        DWORD* grfBINDF, BINDINFO* pbindinfo)
{
    /* FIXME */
    *grfBINDF = 0;
    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnDataAvailable(IBindStatusCallback *iface, DWORD grfBSCF,
        DWORD dwSize, FORMATETC* pformatetc, STGMEDIUM* pstgmed)
{
    IStream *str = pstgmed->u.pstm;
    BYTE buf[1024];
    DWORD size;
    HRESULT hres;

    do {
        DWORD written;

        size = 0;
        hres = IStream_Read(str, buf, sizeof(buf), &size);
        if(size)
            WriteFile(tmp_file, buf, size, &written, NULL);
    }while(hres == S_OK);

    return S_OK;
}

static HRESULT WINAPI InstallCallback_OnObjectAvailable(IBindStatusCallback *iface,
        REFIID riid, IUnknown* punk)
{
    ERR("\n");
    return E_NOTIMPL;
}

static const IBindStatusCallbackVtbl InstallCallbackVtbl = {
    InstallCallback_QueryInterface,
    InstallCallback_AddRef,
    InstallCallback_Release,
    InstallCallback_OnStartBinding,
    InstallCallback_GetPriority,
    InstallCallback_OnLowResource,
    InstallCallback_OnProgress,
    InstallCallback_OnStopBinding,
    InstallCallback_GetBindInfo,
    InstallCallback_OnDataAvailable,
    InstallCallback_OnObjectAvailable
};

static IBindStatusCallback InstallCallback = { &InstallCallbackVtbl };

static LPWSTR get_url(void)
{
    HKEY hkey;
    DWORD res, type;
    DWORD size = INTERNET_MAX_URL_LENGTH*sizeof(WCHAR);
    DWORD returned_size;
    LPWSTR url;

    static const WCHAR wszGeckoUrl[] = {'G','e','c','k','o','U','r','l',0};
    static const WCHAR httpW[] = {'h','t','t','p'};
    static const WCHAR v_formatW[] = {'?','v','=',0};

    /* @@ Wine registry key: HKCU\Software\Wine\MSHTML */
    res = RegOpenKeyW(HKEY_CURRENT_USER, mshtml_keyW, &hkey);
    if(res != ERROR_SUCCESS)
        return NULL;

    url = heap_alloc(size);
    returned_size = size;

    res = RegQueryValueExW(hkey, wszGeckoUrl, NULL, &type, (LPBYTE)url, &returned_size);
    RegCloseKey(hkey);
    if(res != ERROR_SUCCESS || type != REG_SZ) {
        heap_free(url);
        return NULL;
    }

    if(returned_size > sizeof(httpW) && !memcmp(url, httpW, sizeof(httpW))) {
        strcatW(url, v_formatW);
        MultiByteToWideChar(CP_ACP, 0, GECKO_VERSION, -1, url+strlenW(url), size/sizeof(WCHAR)-strlenW(url));
    }

    TRACE("Got URL %s\n", debugstr_w(url));
    return url;
}

static DWORD WINAPI download_proc(PVOID arg)
{
    IMoniker *mon;
    IBindCtx *bctx;
    IStream *str = NULL;
    HRESULT hres;

    CreateURLMoniker(NULL, url, &mon);
    heap_free(url);
    url = NULL;

    CreateAsyncBindCtx(0, &InstallCallback, 0, &bctx);

    hres = IMoniker_BindToStorage(mon, bctx, NULL, &IID_IStream, (void**)&str);
    IBindCtx_Release(bctx);
    if(FAILED(hres)) {
        ERR("BindToStorage failed: %08x\n", hres);
        return 0;
    }

    if(str)
        IStream_Release(str);

    return 0;
}

static INT_PTR CALLBACK installer_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_INITDIALOG:
        ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_HIDE);
        install_dialog = hwnd;
        return TRUE;

    case WM_COMMAND:
        switch(wParam) {
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return FALSE;

        case ID_DWL_INSTALL:
            ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_SHOW);
            EnableWindow(GetDlgItem(hwnd, ID_DWL_INSTALL), 0);
            EnableWindow(GetDlgItem(hwnd, IDCANCEL), 0); /* FIXME */
            CreateThread(NULL, 0, download_proc, NULL, 0, NULL);
            return FALSE;
        }
    }

    return FALSE;
}

BOOL install_wine_gecko(BOOL silent)
{
    HANDLE hsem;

    SetLastError(ERROR_SUCCESS);
    hsem = CreateSemaphoreA( NULL, 0, 1, "mshtml_install_semaphore");

    if(GetLastError() == ERROR_ALREADY_EXISTS) {
        WaitForSingleObject(hsem, INFINITE);
    }else {
        /*
         * Try to find Gecko .cab file in following order:
         * - directory stored in GeckoCabDir value of HKCU/Software/MSHTML key
         * - $datadir/gecko
         * - download from URL stored in GeckoUrl value of HKCU/Software/MSHTML key
         */
        if(!install_from_registered_dir()
           && !install_from_default_dir()
           && !silent && (url = get_url()))
            DialogBoxW(hInst, MAKEINTRESOURCEW(ID_DWL_DIALOG), 0, installer_proc);
    }

    ReleaseSemaphore(hsem, 1, NULL);
    CloseHandle(hsem);

    return TRUE;
}
