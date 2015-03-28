/*
 * Copyright 2006-2010 Jacek Caban for CodeWeavers
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

#include "appwiz.h"

#include <stdio.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <msi.h>

#define GECKO_VERSION "2.34"

#ifdef __i386__
#define ARCH_STRING "x86"
#define GECKO_SHA "330100fecc9c12e98760fd5abfbf3e3d0cccf052"
#else
#define ARCH_STRING ""
#define GECKO_SHA "???"
#endif

typedef struct {
    const char *version;
    const char *file_name;
    const char *sha;
    const char *config_key;
    const char *dir_config_key;
    LPCWSTR dialog_template;
} addon_info_t;

static const addon_info_t addons_info[] = {
    {
        GECKO_VERSION,
        "wine_gecko-" GECKO_VERSION "-" ARCH_STRING ".msi",
        GECKO_SHA,
        "MSHTML",
        "GeckoCabDir",
        MAKEINTRESOURCEW(ID_DWL_GECKO_DIALOG)
    }
};

static const addon_info_t *addon;

static HWND install_dialog = NULL;

static WCHAR GeckoUrl[] = L"http://svn.reactos.org/amine/wine_gecko-2.34-x86.msi";

/* SHA definitions are copied from advapi32. They aren't available in headers. */

typedef struct {
   ULONG Unknown[6];
   ULONG State[5];
   ULONG Count[2];
   UCHAR Buffer[64];
} SHA_CTX, *PSHA_CTX;

void WINAPI A_SHAInit(PSHA_CTX);
void WINAPI A_SHAUpdate(PSHA_CTX,const unsigned char*,UINT);
void WINAPI A_SHAFinal(PSHA_CTX,PULONG);

static BOOL sha_check(const WCHAR *file_name)
{
    const unsigned char *file_map;
    HANDLE file, map;
    ULONG sha[5];
    char buf[2*sizeof(sha)+1];
    SHA_CTX ctx;
    DWORD size, i;

    file = CreateFileW(file_name, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(file == INVALID_HANDLE_VALUE)
        return FALSE;

    size = GetFileSize(file, NULL);

    map = CreateFileMappingW(file, NULL, PAGE_READONLY, 0, 0, NULL);
    CloseHandle(file);
    if(!map)
        return FALSE;

    file_map = MapViewOfFile(map, FILE_MAP_READ, 0, 0, 0);
    CloseHandle(map);
    if(!file_map)
        return FALSE;

    A_SHAInit(&ctx);
    A_SHAUpdate(&ctx, file_map, size);
    A_SHAFinal(&ctx, sha);

    UnmapViewOfFile(file_map);

    for(i=0; i < sizeof(sha); i++)
        sprintf(buf + i*2, "%02x", *((unsigned char*)sha+i));

    if(strcmp(buf, addon->sha)) {
        WARN("Got %s, expected %s\n", buf, addon->sha);
        return FALSE;
    }

    return TRUE;
}

static void set_status(DWORD id)
{
    HWND status = GetDlgItem(install_dialog, ID_DWL_STATUS);
    WCHAR buf[64];

    LoadStringW(hApplet, id, buf, sizeof(buf)/sizeof(WCHAR));
    SendMessageW(status, WM_SETTEXT, 0, (LPARAM)buf);
}

enum install_res {
    INSTALL_OK = 0,
    INSTALL_FAILED,
    INSTALL_NEXT,
};

static enum install_res install_file(const WCHAR *file_name)
{
    ULONG res;

    res = MsiInstallProductW(file_name, NULL);
    if(res != ERROR_SUCCESS) {
        ERR("MsiInstallProduct failed: %u\n", res);
        return INSTALL_FAILED;
    }

    return INSTALL_OK;
}

static enum install_res install_from_unix_file(const char *dir, const char *subdir, const char *file_name)
{
    LPWSTR dos_file_name;
    char *file_path;
    int fd, len;
    enum install_res ret;
    UINT res;

    len = strlen(dir);
    file_path = heap_alloc(len+strlen(subdir)+strlen(file_name)+3);
    if(!file_path)
        return INSTALL_FAILED;

    memcpy(file_path, dir, len);
    if(len && file_path[len-1] != '/' && file_path[len-1] != '\\')
        file_path[len++] = '/';
    if(*subdir) {
        strcpy(file_path+len, subdir);
        len += strlen(subdir);
        file_path[len++] = '/';
    }
    strcpy(file_path+len, file_name);

    fd = _open(file_path, O_RDONLY);
    if(fd == -1) {
        TRACE("%s not found\n", debugstr_a(file_path));
        heap_free(file_path);
        return INSTALL_NEXT;
    }

    _close(fd);

    WARN("Could not get wine_get_dos_file_name function, calling install_cab directly.\n");
    res = MultiByteToWideChar( CP_ACP, 0, file_path, -1, 0, 0);
    dos_file_name = heap_alloc (res*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, file_path, -1, dos_file_name, res);

    heap_free(file_path);

    ret = install_file(dos_file_name);

    heap_free(dos_file_name);
    return ret;
}

static const CHAR mshtml_keyA[] =
    {'S','o','f','t','w','a','r','e',
    '\\','W','i','n','e',
    '\\','M','S','H','T','M','L',0};

static enum install_res install_from_registered_dir(void)
{
    char *package_dir;
    DWORD res, type, size = MAX_PATH;
    enum install_res ret;

    package_dir = heap_alloc(size + sizeof(addon->file_name));

    res = RegGetValueA(HKEY_CURRENT_USER, mshtml_keyA, "GeckoCabDir", RRF_RT_ANY, &type, (PBYTE)package_dir, &size);
    if(res == ERROR_MORE_DATA) {
        package_dir = heap_realloc(package_dir, size + sizeof(addon->file_name));
        res = RegGetValueA(HKEY_CURRENT_USER, mshtml_keyA, "GeckoCabDir", RRF_RT_ANY, &type, (PBYTE)package_dir, &size);
    }

    if(res != ERROR_SUCCESS || (type != REG_SZ && type != REG_EXPAND_SZ)) {
        heap_free(package_dir);
        return INSTALL_FAILED;
    }

    if (type == REG_EXPAND_SZ)
    {
        size = ExpandEnvironmentStringsA(package_dir, NULL, 0);
        if (size)
        {
            char* buf = heap_alloc(size + sizeof(addon->file_name));
            ExpandEnvironmentStringsA(package_dir, buf, size);
            heap_free(package_dir);
            package_dir = buf;
        }
    }

    TRACE("Trying %s/%s\n", debugstr_a(package_dir), debugstr_a(addon->file_name));

    ret = install_from_unix_file(package_dir, "", addon->file_name);

    heap_free(package_dir);
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
    set_status(IDS_DOWNLOADING);
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
        return S_OK;
    }

    set_status(IDS_INSTALLING);
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
    ERR("\n");
    return E_NOTIMPL;
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

static DWORD WINAPI download_proc(PVOID arg)
{
    WCHAR tmp_dir[MAX_PATH], tmp_file[MAX_PATH];
    HRESULT hres;

    GetTempPathW(sizeof(tmp_dir)/sizeof(WCHAR), tmp_dir);
    GetTempFileNameW(tmp_dir, NULL, 0, tmp_file);

    TRACE("using temp file %s\n", debugstr_w(tmp_file));

    hres = URLDownloadToFileW(NULL, GeckoUrl, tmp_file, 0, &InstallCallback);
    if(FAILED(hres)) {
        ERR("URLDownloadToFile failed: %08x\n", hres);
        return 0;
    }

    if(sha_check(tmp_file)) {
        install_file(tmp_file);
    }else {
        WCHAR message[256];

        if(LoadStringW(hApplet, IDS_INVALID_SHA, message, sizeof(message)/sizeof(WCHAR))) {
            MessageBoxW(NULL, message, NULL, MB_ICONERROR);
        }
    }

    DeleteFileW(tmp_file);
    EndDialog(install_dialog, 0);
    return 0;
}

static INT_PTR CALLBACK installer_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg) {
    case WM_INITDIALOG:
        ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_HIDE);
        install_dialog = hwnd;
        return TRUE;

    case WM_NOTIFY:
        break;

    case WM_COMMAND:
        switch(wParam) {
        case IDCANCEL:
            EndDialog(hwnd, 0);
            return FALSE;

        case ID_DWL_INSTALL:
            ShowWindow(GetDlgItem(hwnd, ID_DWL_PROGRESS), SW_SHOW);
            EnableWindow(GetDlgItem(hwnd, ID_DWL_INSTALL), 0);
            EnableWindow(GetDlgItem(hwnd, IDCANCEL), 0); /* FIXME */
            CloseHandle( CreateThread(NULL, 0, download_proc, NULL, 0, NULL));
            return FALSE;
        }
    }

    return FALSE;
}

BOOL install_addon(addon_t addon_type, HWND hwnd_parent)
{

    if(!*ARCH_STRING)
        return FALSE;

    addon = addons_info + addon_type;

    /*
     * Try to find addon .msi file in following order:
     * - directory stored in $dir_config_key value of HKCU/Wine/Software/$config_key key
     * - download the package
     */
    if (install_from_registered_dir() == INSTALL_NEXT)
        DialogBoxW(hApplet, addon->dialog_template, hwnd_parent, installer_proc);

    return TRUE;
}
