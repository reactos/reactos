/* Copyright (c) Mark Harmstone 2016-17
 *
 * This file is part of WinBtrfs.
 *
 * WinBtrfs is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public Licence as published by
 * the Free Software Foundation, either version 3 of the Licence, or
 * (at your option) any later version.
 *
 * WinBtrfs is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public Licence for more details.
 *
 * You should have received a copy of the GNU Lesser General Public Licence
 * along with WinBtrfs.  If not, see <http://www.gnu.org/licenses/>. */

#include "shellext.h"
#include <windows.h>
#include <commctrl.h>
#include <strsafe.h>
#include <stddef.h>
#include "factory.h"
#include "resource.h"

static const GUID CLSID_ShellBtrfsIconHandler = { 0x2690b74f, 0xf353, 0x422d, { 0xbb, 0x12, 0x40, 0x15, 0x81, 0xee, 0xf8, 0xf0 } };
static const GUID CLSID_ShellBtrfsContextMenu = { 0x2690b74f, 0xf353, 0x422d, { 0xbb, 0x12, 0x40, 0x15, 0x81, 0xee, 0xf8, 0xf1 } };
static const GUID CLSID_ShellBtrfsPropSheet = { 0x2690b74f, 0xf353, 0x422d, { 0xbb, 0x12, 0x40, 0x15, 0x81, 0xee, 0xf8, 0xf2 } };
static const GUID CLSID_ShellBtrfsVolPropSheet = { 0x2690b74f, 0xf353, 0x422d, { 0xbb, 0x12, 0x40, 0x15, 0x81, 0xee, 0xf8, 0xf3 } };

#define COM_DESCRIPTION_ICON_HANDLER L"WinBtrfs shell extension (icon handler)"
#define COM_DESCRIPTION_CONTEXT_MENU L"WinBtrfs shell extension (context menu)"
#define COM_DESCRIPTION_PROP_SHEET L"WinBtrfs shell extension (property sheet)"
#define COM_DESCRIPTION_VOL_PROP_SHEET L"WinBtrfs shell extension (volume property sheet)"
#define ICON_OVERLAY_NAME L"WinBtrfs"

typedef enum _PROCESS_DPI_AWARENESS {
    PROCESS_DPI_UNAWARE,
    PROCESS_SYSTEM_DPI_AWARE,
    PROCESS_PER_MONITOR_DPI_AWARE
} PROCESS_DPI_AWARENESS;

typedef ULONG (WINAPI *_RtlNtStatusToDosError)(NTSTATUS Status);
typedef HRESULT (WINAPI *_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS value);

HMODULE module;
LONG objs_loaded = 0;

void ShowError(HWND hwnd, ULONG err) {
    WCHAR* buf;

    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                       err, 0, (WCHAR*)&buf, 0, NULL) == 0) {
        MessageBoxW(hwnd, L"FormatMessage failed", L"Error", MB_ICONERROR);
        return;
    }

    MessageBoxW(hwnd, buf, L"Error", MB_ICONERROR);

    LocalFree(buf);
}

void ShowStringError(HWND hwndDlg, int num, ...) {
    WCHAR title[255], s[1024], t[1024];
    va_list ap;

    if (!LoadStringW(module, IDS_ERROR, title, sizeof(title) / sizeof(WCHAR))) {
        ShowError(hwndDlg, GetLastError());
        return;
    }

    if (!LoadStringW(module, num, s, sizeof(s) / sizeof(WCHAR))) {
        ShowError(hwndDlg, GetLastError());
        return;
    }

    va_start(ap, num);
#ifndef __REACTOS__
    vswprintf(t, sizeof(t) / sizeof(WCHAR), s, ap);
#else
    vsnwprintf(t, sizeof(t) / sizeof(WCHAR), s, ap);
#endif

    MessageBoxW(hwndDlg, t, title, MB_ICONERROR);

    va_end(ap);
}

void ShowNtStatusError(HWND hwnd, NTSTATUS Status) {
    _RtlNtStatusToDosError RtlNtStatusToDosError;
    HMODULE ntdll = LoadLibraryW(L"ntdll.dll");

    if (!ntdll) {
        MessageBoxW(hwnd, L"Error loading ntdll.dll", L"Error", MB_ICONERROR);
        return;
    }

    RtlNtStatusToDosError = (_RtlNtStatusToDosError)GetProcAddress(ntdll, "RtlNtStatusToDosError");

    if (!RtlNtStatusToDosError) {
        MessageBoxW(hwnd, L"Error loading RtlNtStatusToDosError in ntdll.dll", L"Error", MB_ICONERROR);
        FreeLibrary(ntdll);
        return;
    }

    ShowError(hwnd, RtlNtStatusToDosError(Status));

    FreeLibrary(ntdll);
}

void set_dpi_aware() {
    _SetProcessDpiAwareness SetProcessDpiAwareness;
    HMODULE shcore = LoadLibraryW(L"shcore.dll");

    if (!shcore)
        return;

    SetProcessDpiAwareness = (_SetProcessDpiAwareness)GetProcAddress(shcore, "SetProcessDpiAwareness");

    if (!SetProcessDpiAwareness)
        return;

    SetProcessDpiAwareness(PROCESS_PER_MONITOR_DPI_AWARE);
}

void format_size(UINT64 size, WCHAR* s, ULONG len, BOOL show_bytes) {
    WCHAR nb[255], nb2[255], t[255], bytes[255];
    WCHAR kb[255];
    ULONG sr;
    float f;
    NUMBERFMTW fmt;
    WCHAR thou[4], grouping[64], *c;

    _i64tow(size, nb, 10);

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thou, sizeof(thou) / sizeof(WCHAR));

    fmt.NumDigits = 0;
    fmt.LeadingZero = 1;
    fmt.lpDecimalSep = (LPWSTR)L"."; // not used
    fmt.lpThousandSep = thou;
    fmt.NegativeOrder = 0;

    // Grouping code copied from dlls/shlwapi/string.c in Wine - thank you

    fmt.Grouping = 0;
    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_SGROUPING, grouping, sizeof(grouping) / sizeof(WCHAR));

    c = grouping;
    while (*c) {
        if (*c >= '0' && *c < '9') {
            fmt.Grouping *= 10;
            fmt.Grouping += *c - '0';
        }

        c++;
    }

    if (fmt.Grouping % 10 == 0)
        fmt.Grouping /= 10;
    else
        fmt.Grouping *= 10;

    GetNumberFormatW(LOCALE_USER_DEFAULT, 0, nb, &fmt, nb2, sizeof(nb2) / sizeof(WCHAR));

    if (size < 1024) {
        if (!LoadStringW(module, size == 1 ? IDS_SIZE_BYTE : IDS_SIZE_BYTES, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(NULL, GetLastError());
            return;
        }

        if (StringCchPrintfW(s, len, t, nb2) == STRSAFE_E_INSUFFICIENT_BUFFER) {
            ShowError(NULL, ERROR_INSUFFICIENT_BUFFER);
            return;
        }

        return;
    }

    if (show_bytes) {
        if (!LoadStringW(module, IDS_SIZE_BYTES, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(NULL, GetLastError());
            return;
        }

        if (StringCchPrintfW(bytes, sizeof(bytes) / sizeof(WCHAR), t, nb2) == STRSAFE_E_INSUFFICIENT_BUFFER) {
            ShowError(NULL, ERROR_INSUFFICIENT_BUFFER);
            return;
        }
    }

    if (size >= 1152921504606846976) {
        sr = IDS_SIZE_EB;
        f = (float)size / 1152921504606846976.0f;
    } else if (size >= 1125899906842624) {
        sr = IDS_SIZE_PB;
        f = (float)size / 1125899906842624.0f;
    } else if (size >= 1099511627776) {
        sr = IDS_SIZE_TB;
        f = (float)size / 1099511627776.0f;
    } else if (size >= 1073741824) {
        sr = IDS_SIZE_GB;
        f = (float)size / 1073741824.0f;
    } else if (size >= 1048576) {
        sr = IDS_SIZE_MB;
        f = (float)size / 1048576.0f;
    } else {
        sr = IDS_SIZE_KB;
        f = (float)size / 1024.0f;
    }

    if (!LoadStringW(module, sr, t, sizeof(t) / sizeof(WCHAR))) {
        ShowError(NULL, GetLastError());
        return;
    }

    if (show_bytes) {
        if (StringCchPrintfW(kb, sizeof(kb) / sizeof(WCHAR), t, f) == STRSAFE_E_INSUFFICIENT_BUFFER) {
            ShowError(NULL, ERROR_INSUFFICIENT_BUFFER);
            return;
        }

        if (!LoadStringW(module, IDS_SIZE_LARGE, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(NULL, GetLastError());
            return;
        }

        if (StringCchPrintfW(s, len, t, kb, bytes) == STRSAFE_E_INSUFFICIENT_BUFFER) {
            ShowError(NULL, ERROR_INSUFFICIENT_BUFFER);
            return;
        }
    } else {
        if (StringCchPrintfW(s, len, t, f) == STRSAFE_E_INSUFFICIENT_BUFFER) {
            ShowError(NULL, ERROR_INSUFFICIENT_BUFFER);
            return;
        }
    }
}

std::wstring format_message(ULONG last_error) {
    WCHAR* buf;
    std::wstring s;

    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
        last_error, 0, (WCHAR*)&buf, 0, NULL) == 0) {
        return L"(error retrieving message)";
    }

    s = buf;

    LocalFree(buf);

    // remove trailing newline
    while (s.length() > 0 && (s.substr(s.length() - 1, 1) == L"\r" || s.substr(s.length() - 1, 1) == L"\n"))
        s = s.substr(0, s.length() - 1);

    return s;
}

std::wstring format_ntstatus(NTSTATUS Status) {
    _RtlNtStatusToDosError RtlNtStatusToDosError;
    std::wstring s;
    HMODULE ntdll = LoadLibraryW(L"ntdll.dll");

    if (!ntdll)
        return L"(error loading ntdll.dll)";

    RtlNtStatusToDosError = (_RtlNtStatusToDosError)GetProcAddress(ntdll, "RtlNtStatusToDosError");

    if (!RtlNtStatusToDosError) {
        FreeLibrary(ntdll);
        return L"(error loading RtlNtStatusToDosError)";
    }

    s = format_message(RtlNtStatusToDosError(Status));

    FreeLibrary(ntdll);

    return s;
}

#ifdef __cplusplus
extern "C" {
#endif

STDAPI DllCanUnloadNow(void) {
    return objs_loaded == 0 ? S_OK : S_FALSE;
}

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
    if (rclsid == CLSID_ShellBtrfsIconHandler) {
        Factory* fact = new Factory;
        if (!fact)
            return E_OUTOFMEMORY;
        else {
            fact->type = FactoryIconHandler;

            return fact->QueryInterface(riid, ppv);
        }
    } else if (rclsid == CLSID_ShellBtrfsContextMenu) {
        Factory* fact = new Factory;
        if (!fact)
            return E_OUTOFMEMORY;
        else {
            fact->type = FactoryContextMenu;

            return fact->QueryInterface(riid, ppv);
        }
    } else if (rclsid == CLSID_ShellBtrfsPropSheet) {
        Factory* fact = new Factory;
        if (!fact)
            return E_OUTOFMEMORY;
        else {
            fact->type = FactoryPropSheet;

            return fact->QueryInterface(riid, ppv);
        }
    } else if (rclsid == CLSID_ShellBtrfsVolPropSheet) {
        Factory* fact = new Factory;
        if (!fact)
            return E_OUTOFMEMORY;
        else {
            fact->type = FactoryVolPropSheet;

            return fact->QueryInterface(riid, ppv);
        }
    }

    return CLASS_E_CLASSNOTAVAILABLE;
}

static BOOL write_reg_key(HKEY root, const WCHAR* keyname, const WCHAR* val, DWORD type, const BYTE* data, DWORD datasize) {
    LONG l;
    HKEY hk;
    DWORD dispos;

    l = RegCreateKeyExW(root, keyname, 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hk, &dispos);
    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegCreateKey returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        return FALSE;
    }

    l = RegSetValueExW(hk, val, 0, type, data, datasize);
    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegSetValueEx returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        return FALSE;
    }

    l = RegCloseKey(hk);
    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegCloseKey returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        return FALSE;
    }

    return TRUE;
}

static BOOL register_clsid(const GUID clsid, const WCHAR* description) {
    WCHAR* clsidstring;
    WCHAR inproc[MAX_PATH], progid[MAX_PATH], clsidkeyname[MAX_PATH], dllpath[MAX_PATH];
    BOOL ret = FALSE;

    StringFromCLSID(clsid, &clsidstring);

    wsprintfW(inproc, L"CLSID\\%s\\InprocServer32", clsidstring);
    wsprintfW(progid, L"CLSID\\%s\\ProgId", clsidstring);
    wsprintfW(clsidkeyname, L"CLSID\\%s", clsidstring);

    if (!write_reg_key(HKEY_CLASSES_ROOT, clsidkeyname, NULL, REG_SZ, (BYTE*)description, (wcslen(description) + 1) * sizeof(WCHAR)))
        goto end;

    GetModuleFileNameW(module, dllpath, sizeof(dllpath));

    if (!write_reg_key(HKEY_CLASSES_ROOT, inproc, NULL, REG_SZ, (BYTE*)dllpath, (wcslen(dllpath) + 1) * sizeof(WCHAR)))
        goto end;

    if (!write_reg_key(HKEY_CLASSES_ROOT, inproc, L"ThreadingModel", REG_SZ, (BYTE*)L"Apartment", (wcslen(L"Apartment") + 1) * sizeof(WCHAR)))
        goto end;

    ret = TRUE;

end:
    CoTaskMemFree(clsidstring);

    return ret;
}

static BOOL unregister_clsid(const GUID clsid) {
    WCHAR* clsidstring;
    WCHAR clsidkeyname[MAX_PATH];
    BOOL ret = FALSE;
    LONG l;

    StringFromCLSID(clsid, &clsidstring);
    wsprintfW(clsidkeyname, L"CLSID\\%s", clsidstring);

    l = RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidkeyname);

    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegDeleteTree returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        ret = FALSE;
    } else
        ret = TRUE;

    CoTaskMemFree(clsidstring);

    return ret;
}

static BOOL reg_icon_overlay(const GUID clsid, const WCHAR* name) {
    WCHAR path[MAX_PATH];
    WCHAR* clsidstring;
    BOOL ret = FALSE;

    StringFromCLSID(clsid, &clsidstring);

    wcscpy(path, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\");
    wcscat(path, name);

    if (!write_reg_key(HKEY_LOCAL_MACHINE, path, NULL, REG_SZ, (BYTE*)clsidstring, (wcslen(clsidstring) + 1) * sizeof(WCHAR)))
        goto end;

    ret = TRUE;

end:
    CoTaskMemFree(clsidstring);

    return ret;
}

static BOOL unreg_icon_overlay(const WCHAR* name) {
    WCHAR path[MAX_PATH];
    LONG l;

    wcscpy(path, L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\");
    wcscat(path, name);

    l = RegDeleteTreeW(HKEY_LOCAL_MACHINE, path);

    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegDeleteTree returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        return FALSE;
    } else
        return TRUE;
}

static BOOL reg_context_menu_handler(const GUID clsid, const WCHAR* filetype, const WCHAR* name) {
    WCHAR path[MAX_PATH];
    WCHAR* clsidstring;
    BOOL ret = FALSE;

    StringFromCLSID(clsid, &clsidstring);

    wcscpy(path, filetype);
    wcscat(path, L"\\ShellEx\\ContextMenuHandlers\\");
    wcscat(path, name);

    if (!write_reg_key(HKEY_CLASSES_ROOT, path, NULL, REG_SZ, (BYTE*)clsidstring, (wcslen(clsidstring) + 1) * sizeof(WCHAR)))
        goto end;

    ret = TRUE;

end:
    CoTaskMemFree(clsidstring);

    return ret;
}

static BOOL unreg_context_menu_handler(const WCHAR* filetype, const WCHAR* name) {
    WCHAR path[MAX_PATH];
    LONG l;

    wcscpy(path, filetype);
    wcscat(path, L"\\ShellEx\\ContextMenuHandlers\\");
    wcscat(path, name);

    l = RegDeleteTreeW(HKEY_CLASSES_ROOT, path);

    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegDeleteTree returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        return FALSE;
    } else
        return TRUE;
}

static BOOL reg_prop_sheet_handler(const GUID clsid, const WCHAR* filetype, const WCHAR* name) {
    WCHAR path[MAX_PATH];
    WCHAR* clsidstring;
    BOOL ret = FALSE;

    StringFromCLSID(clsid, &clsidstring);

    wcscpy(path, filetype);
    wcscat(path, L"\\ShellEx\\PropertySheetHandlers\\");
    wcscat(path, name);

    if (!write_reg_key(HKEY_CLASSES_ROOT, path, NULL, REG_SZ, (BYTE*)clsidstring, (wcslen(clsidstring) + 1) * sizeof(WCHAR)))
        goto end;

    ret = TRUE;

end:
    CoTaskMemFree(clsidstring);

    return ret;
}

static BOOL unreg_prop_sheet_handler(const WCHAR* filetype, const WCHAR* name) {
    WCHAR path[MAX_PATH];
    LONG l;

    wcscpy(path, filetype);
    wcscat(path, L"\\ShellEx\\PropertySheetHandlers\\");
    wcscat(path, name);

    l = RegDeleteTreeW(HKEY_CLASSES_ROOT, path);

    if (l != ERROR_SUCCESS) {
        WCHAR s[255];
        wsprintfW(s, L"RegDeleteTree returned %08x", l);
        MessageBoxW(0, s, NULL, MB_ICONERROR);

        return FALSE;
    } else
        return TRUE;
}

STDAPI DllRegisterServer(void) {
    if (!register_clsid(CLSID_ShellBtrfsIconHandler, COM_DESCRIPTION_ICON_HANDLER))
        return E_FAIL;

    if (!register_clsid(CLSID_ShellBtrfsContextMenu, COM_DESCRIPTION_CONTEXT_MENU))
        return E_FAIL;

    if (!register_clsid(CLSID_ShellBtrfsPropSheet, COM_DESCRIPTION_PROP_SHEET))
        return E_FAIL;

    if (!register_clsid(CLSID_ShellBtrfsVolPropSheet, COM_DESCRIPTION_VOL_PROP_SHEET))
        return E_FAIL;

    if (!reg_icon_overlay(CLSID_ShellBtrfsIconHandler, ICON_OVERLAY_NAME)) {
        MessageBoxW(0, L"Failed to register icon overlay.", NULL, MB_ICONERROR);
        return E_FAIL;
    }

    if (!reg_context_menu_handler(CLSID_ShellBtrfsContextMenu, L"Directory\\Background", ICON_OVERLAY_NAME)) {
        MessageBoxW(0, L"Failed to register context menu handler.", NULL, MB_ICONERROR);
        return E_FAIL;
    }

    if (!reg_context_menu_handler(CLSID_ShellBtrfsContextMenu, L"Folder", ICON_OVERLAY_NAME)) {
        MessageBoxW(0, L"Failed to register context menu handler.", NULL, MB_ICONERROR);
        return E_FAIL;
    }

    if (!reg_prop_sheet_handler(CLSID_ShellBtrfsPropSheet, L"Folder", ICON_OVERLAY_NAME)) {
        MessageBoxW(0, L"Failed to register property sheet handler.", NULL, MB_ICONERROR);
        return E_FAIL;
    }

    if (!reg_prop_sheet_handler(CLSID_ShellBtrfsPropSheet, L"*", ICON_OVERLAY_NAME)) {
        MessageBoxW(0, L"Failed to register property sheet handler.", NULL, MB_ICONERROR);
        return E_FAIL;
    }

    if (!reg_prop_sheet_handler(CLSID_ShellBtrfsVolPropSheet, L"Drive", ICON_OVERLAY_NAME)) {
        MessageBoxW(0, L"Failed to register volume property sheet handler.", NULL, MB_ICONERROR);
        return E_FAIL;
    }

    return S_OK;
}

STDAPI DllUnregisterServer(void) {
    unreg_prop_sheet_handler(L"Folder", ICON_OVERLAY_NAME);
    unreg_prop_sheet_handler(L"*", ICON_OVERLAY_NAME);
    unreg_prop_sheet_handler(L"Drive", ICON_OVERLAY_NAME);
    unreg_context_menu_handler(L"Folder", ICON_OVERLAY_NAME);
    unreg_context_menu_handler(L"Directory\\Background", ICON_OVERLAY_NAME);
    unreg_icon_overlay(ICON_OVERLAY_NAME);

    if (!unregister_clsid(CLSID_ShellBtrfsVolPropSheet))
        return E_FAIL;

    if (!unregister_clsid(CLSID_ShellBtrfsPropSheet))
        return E_FAIL;

    if (!unregister_clsid(CLSID_ShellBtrfsContextMenu))
        return E_FAIL;

    if (!unregister_clsid(CLSID_ShellBtrfsIconHandler))
        return E_FAIL;

    return S_OK;
}

STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine) {
    if (bInstall)
        return DllRegisterServer();
    else
        return DllUnregisterServer();
}

BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH)
        module = (HMODULE)hModule;

    return TRUE;
}

static void create_subvol(std::wstring fn) {
    size_t found = fn.rfind(L"\\");
    std::wstring path, file;
    HANDLE h;
    ULONG bcslen;
    btrfs_create_subvol* bcs;
    IO_STATUS_BLOCK iosb;

    if (found == std::wstring::npos) {
        path = L"";
        file = fn;
    } else {
        path = fn.substr(0, found);
        file = fn.substr(found + 1);
    }
    path += L"\\";

    h = CreateFileW(path.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (h == INVALID_HANDLE_VALUE)
        return;

    bcslen = offsetof(btrfs_create_subvol, name[0]) + (file.length() * sizeof(WCHAR));
    bcs = (btrfs_create_subvol*)malloc(bcslen);

    bcs->readonly = FALSE;
    bcs->posix = FALSE;
    bcs->namelen = file.length() * sizeof(WCHAR);
    memcpy(bcs->name, file.c_str(), bcs->namelen);

    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, bcs, bcslen, NULL, 0);

    CloseHandle(h);
}

void CALLBACK CreateSubvolW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 1)
        create_subvol(args[0]);

    LocalFree(args);
}

static void create_snapshot2(std::wstring source, std::wstring fn) {
    size_t found = fn.rfind(L"\\");
    std::wstring path, file;
    HANDLE h, src;
    ULONG bcslen;
    btrfs_create_snapshot* bcs;
    IO_STATUS_BLOCK iosb;

    if (found == std::wstring::npos) {
        path = L"";
        file = fn;
    } else {
        path = fn.substr(0, found);
        file = fn.substr(found + 1);
    }
    path += L"\\";

    src = CreateFileW(source.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (src == INVALID_HANDLE_VALUE)
        return;

    h = CreateFileW(path.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        CloseHandle(src);
        return;
    }

    bcslen = offsetof(btrfs_create_snapshot, name[0]) + (file.length() * sizeof(WCHAR));
    bcs = (btrfs_create_snapshot*)malloc(bcslen);

    bcs->readonly = FALSE;
    bcs->posix = FALSE;
    bcs->namelen = file.length() * sizeof(WCHAR);
    memcpy(bcs->name, file.c_str(), bcs->namelen);
    bcs->subvol = src;

    NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, bcslen, NULL, 0);

    CloseHandle(h);
    CloseHandle(src);
}

void CALLBACK CreateSnapshotW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 2)
        create_snapshot2(args[0], args[1]);

    LocalFree(args);
}

#ifdef __cplusplus
}
#endif
