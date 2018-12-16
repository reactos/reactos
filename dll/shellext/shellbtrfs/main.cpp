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
#include <stdexcept>
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

void format_size(uint64_t size, wstring& s, bool show_bytes) {
    wstring t, bytes, kb, nb;
    WCHAR nb2[255];
    ULONG sr;
    float f;
    NUMBERFMTW fmt;
    WCHAR dec[2], thou[4], grouping[64], *c;
#ifdef __REACTOS__
    WCHAR buffer[64];
#endif

#ifndef __REACTOS__
    nb = to_wstring(size);
#else
    swprintf(buffer, L"%I64d", size);
    nb = wstring(buffer);
#endif

    GetLocaleInfoW(LOCALE_USER_DEFAULT, LOCALE_STHOUSAND, thou, sizeof(thou) / sizeof(WCHAR));

    dec[0] = '.'; dec[1] = 0; // not used, but silences gcc warning

    fmt.NumDigits = 0;
    fmt.LeadingZero = 1;
    fmt.lpDecimalSep = dec;
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

    GetNumberFormatW(LOCALE_USER_DEFAULT, 0, nb.c_str(), &fmt, nb2, sizeof(nb2) / sizeof(WCHAR));

    if (size < 1024) {
        if (!load_string(module, size == 1 ? IDS_SIZE_BYTE : IDS_SIZE_BYTES, t))
            throw last_error(GetLastError());

        wstring_sprintf(s, t, nb2);
        return;
    }

    if (show_bytes) {
        if (!load_string(module, IDS_SIZE_BYTES, t))
            throw last_error(GetLastError());

        wstring_sprintf(bytes, t, nb2);
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

    if (!load_string(module, sr, t))
        throw last_error(GetLastError());

    if (show_bytes) {
        wstring_sprintf(kb, t, f);

        if (!load_string(module, IDS_SIZE_LARGE, t))
            throw last_error(GetLastError());

        wstring_sprintf(s, t, kb.c_str(), bytes.c_str());
    } else
        wstring_sprintf(s, t, f);
}

wstring format_message(ULONG last_error) {
    WCHAR* buf;
    wstring s;

    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        last_error, 0, (WCHAR*)&buf, 0, nullptr) == 0) {
        return L"(error retrieving message)";
    }

    s = buf;

    LocalFree(buf);

    // remove trailing newline
    while (s.length() > 0 && (s.substr(s.length() - 1, 1) == L"\r" || s.substr(s.length() - 1, 1) == L"\n"))
        s = s.substr(0, s.length() - 1);

    return s;
}

wstring format_ntstatus(NTSTATUS Status) {
    _RtlNtStatusToDosError RtlNtStatusToDosError;
    wstring s;
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

bool load_string(HMODULE module, UINT id, wstring& s) {
    int len;
    LPWSTR retstr = nullptr;

    len = LoadStringW(module, id, (LPWSTR)&retstr, 0);

    if (len == 0)
        return false;

    s = wstring(retstr, len);

    return true;
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

void wstring_sprintf(wstring& s, wstring fmt, ...) {
    int len;
    va_list args;

    va_start(args, fmt);
    len = _vsnwprintf(nullptr, 0, fmt.c_str(), args);

    if (len == 0)
        s = L"";
    else {
        s.resize(len);
        _vsnwprintf((wchar_t*)s.c_str(), len, fmt.c_str(), args);
    }

    va_end(args);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

extern "C" STDAPI DllCanUnloadNow(void) {
    return objs_loaded == 0 ? S_OK : S_FALSE;
}

extern "C" STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv) {
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

static void write_reg_key(HKEY root, const wstring& keyname, const WCHAR* val, const wstring& data) {
    LONG l;
    HKEY hk;
    DWORD dispos;

    l = RegCreateKeyExW(root, keyname.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &hk, &dispos);
    if (l != ERROR_SUCCESS)
        throw string_error(IDS_REGCREATEKEY_FAILED, l);

    l = RegSetValueExW(hk, val, 0, REG_SZ, (const BYTE*)data.c_str(), (data.length() + 1) * sizeof(WCHAR));
    if (l != ERROR_SUCCESS)
        throw string_error(IDS_REGSETVALUEEX_FAILED, l);

    l = RegCloseKey(hk);
    if (l != ERROR_SUCCESS)
        throw string_error(IDS_REGCLOSEKEY_FAILED, l);
}

static void register_clsid(const GUID clsid, const WCHAR* description) {
    WCHAR* clsidstring;
    wstring inproc, progid, clsidkeyname;
    WCHAR dllpath[MAX_PATH];

    StringFromCLSID(clsid, &clsidstring);

    try {
#ifndef __REACTOS__
        inproc = L"CLSID\\"s + clsidstring + L"\\InprocServer32"s;
        progid = L"CLSID\\"s + clsidstring + L"\\ProgId"s;
        clsidkeyname = L"CLSID\\"s + clsidstring;
#else
        inproc = wstring(L"CLSID\\") + clsidstring + wstring(L"\\InprocServer32");
        progid = wstring(L"CLSID\\") + clsidstring + wstring(L"\\ProgId");
        clsidkeyname = wstring(L"CLSID\\") + clsidstring;
#endif

        write_reg_key(HKEY_CLASSES_ROOT, clsidkeyname, nullptr, description);

        GetModuleFileNameW(module, dllpath, sizeof(dllpath));

        write_reg_key(HKEY_CLASSES_ROOT, inproc, nullptr, dllpath);

        write_reg_key(HKEY_CLASSES_ROOT, inproc, L"ThreadingModel", L"Apartment");
    } catch (...) {
        CoTaskMemFree(clsidstring);
        throw;
    }

    CoTaskMemFree(clsidstring);
}

static void unregister_clsid(const GUID clsid) {
    WCHAR* clsidstring;

    StringFromCLSID(clsid, &clsidstring);

    try {
        WCHAR clsidkeyname[MAX_PATH];

        wsprintfW(clsidkeyname, L"CLSID\\%s", clsidstring);

        LONG l = RegDeleteTreeW(HKEY_CLASSES_ROOT, clsidkeyname);

        if (l != ERROR_SUCCESS)
            throw string_error(IDS_REGDELETETREE_FAILED, l);
    } catch (...) {
        CoTaskMemFree(clsidstring);
        throw;
    }

    CoTaskMemFree(clsidstring);
}

static void reg_icon_overlay(const GUID clsid, const wstring& name) {
    WCHAR* clsidstring;

    StringFromCLSID(clsid, &clsidstring);

    try {
#ifndef __REACTOS__
        wstring path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\"s + name;
#else
        wstring path = wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\") + name;
#endif

        write_reg_key(HKEY_LOCAL_MACHINE, path, nullptr, clsidstring);
    } catch (...) {
        CoTaskMemFree(clsidstring);
        throw;
    }

    CoTaskMemFree(clsidstring);
}

static void unreg_icon_overlay(const wstring& name) {
#ifndef __REACTOS__
    wstring path = L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\"s + name;
#else
    wstring path = wstring(L"Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\ShellIconOverlayIdentifiers\\") + name;
#endif

    LONG l = RegDeleteTreeW(HKEY_LOCAL_MACHINE, path.c_str());

    if (l != ERROR_SUCCESS)
        throw string_error(IDS_REGDELETETREE_FAILED, l);
}

static void reg_context_menu_handler(const GUID clsid, const wstring& filetype, const wstring& name) {
    WCHAR* clsidstring;

    StringFromCLSID(clsid, &clsidstring);

    try {
#ifndef __REACTOS__
        wstring path = filetype + L"\\ShellEx\\ContextMenuHandlers\\"s + name;
#else
        wstring path = filetype + wstring(L"\\ShellEx\\ContextMenuHandlers\\") + name;
#endif

        write_reg_key(HKEY_CLASSES_ROOT, path, nullptr, clsidstring);
    } catch (...) {
        CoTaskMemFree(clsidstring);
        throw;
    }
}

static void unreg_context_menu_handler(const wstring& filetype, const wstring& name) {
#ifndef __REACTOS__
    wstring path = filetype + L"\\ShellEx\\ContextMenuHandlers\\"s + name;
#else
    wstring path = filetype + wstring(L"\\ShellEx\\ContextMenuHandlers\\") + name;
#endif

    LONG l = RegDeleteTreeW(HKEY_CLASSES_ROOT, path.c_str());

    if (l != ERROR_SUCCESS)
        throw string_error(IDS_REGDELETETREE_FAILED, l);
}

static void reg_prop_sheet_handler(const GUID clsid, const wstring& filetype, const wstring& name) {
    WCHAR* clsidstring;

    StringFromCLSID(clsid, &clsidstring);

    try {
#ifndef __REACTOS__
        wstring path = filetype + L"\\ShellEx\\PropertySheetHandlers\\"s + name;
#else
        wstring path = filetype + wstring(L"\\ShellEx\\PropertySheetHandlers\\") + name;
#endif

        write_reg_key(HKEY_CLASSES_ROOT, path, nullptr, clsidstring);
    } catch (...) {
        CoTaskMemFree(clsidstring);
        throw;
    }
}

static void unreg_prop_sheet_handler(const wstring& filetype, const wstring& name) {
#ifndef __REACTOS__
    wstring path = filetype + L"\\ShellEx\\PropertySheetHandlers\\"s + name;
#else
    wstring path = filetype + wstring(L"\\ShellEx\\PropertySheetHandlers\\") + name;
#endif

    LONG l = RegDeleteTreeW(HKEY_CLASSES_ROOT, path.c_str());

    if (l != ERROR_SUCCESS)
        throw string_error(IDS_REGDELETETREE_FAILED, l);
}

extern "C" STDAPI DllRegisterServer(void) {
    try {
        register_clsid(CLSID_ShellBtrfsIconHandler, COM_DESCRIPTION_ICON_HANDLER);
        register_clsid(CLSID_ShellBtrfsContextMenu, COM_DESCRIPTION_CONTEXT_MENU);
        register_clsid(CLSID_ShellBtrfsPropSheet, COM_DESCRIPTION_PROP_SHEET);
        register_clsid(CLSID_ShellBtrfsVolPropSheet, COM_DESCRIPTION_VOL_PROP_SHEET);

        reg_icon_overlay(CLSID_ShellBtrfsIconHandler, ICON_OVERLAY_NAME);

        reg_context_menu_handler(CLSID_ShellBtrfsContextMenu, L"Directory\\Background", ICON_OVERLAY_NAME);
        reg_context_menu_handler(CLSID_ShellBtrfsContextMenu, L"Folder", ICON_OVERLAY_NAME);

        reg_prop_sheet_handler(CLSID_ShellBtrfsPropSheet, L"Folder", ICON_OVERLAY_NAME);
        reg_prop_sheet_handler(CLSID_ShellBtrfsPropSheet, L"*", ICON_OVERLAY_NAME);
        reg_prop_sheet_handler(CLSID_ShellBtrfsVolPropSheet, L"Drive", ICON_OVERLAY_NAME);
    } catch (const exception& e) {
        error_message(nullptr, e.what());
        return E_FAIL;
    }

    return S_OK;
}

extern "C" STDAPI DllUnregisterServer(void) {
    try {
        unreg_prop_sheet_handler(L"Folder", ICON_OVERLAY_NAME);
        unreg_prop_sheet_handler(L"*", ICON_OVERLAY_NAME);
        unreg_prop_sheet_handler(L"Drive", ICON_OVERLAY_NAME);
        unreg_context_menu_handler(L"Folder", ICON_OVERLAY_NAME);
        unreg_context_menu_handler(L"Directory\\Background", ICON_OVERLAY_NAME);
        unreg_icon_overlay(ICON_OVERLAY_NAME);

        unregister_clsid(CLSID_ShellBtrfsVolPropSheet);
        unregister_clsid(CLSID_ShellBtrfsPropSheet);
        unregister_clsid(CLSID_ShellBtrfsContextMenu);
        unregister_clsid(CLSID_ShellBtrfsIconHandler);
    } catch (const exception& e) {
        error_message(nullptr, e.what());
        return E_FAIL;
    }

    return S_OK;
}

extern "C" STDAPI DllInstall(BOOL bInstall, LPCWSTR pszCmdLine) {
    if (bInstall)
        return DllRegisterServer();
    else
        return DllUnregisterServer();
}

extern "C" BOOL APIENTRY DllMain(HANDLE hModule, DWORD dwReason, void* lpReserved) {
    if (dwReason == DLL_PROCESS_ATTACH)
        module = (HMODULE)hModule;

    return true;
}

static void create_subvol(const wstring& fn) {
    size_t found = fn.rfind(L"\\");
    wstring path, file;
    win_handle h;
    ULONG bcslen;
    btrfs_create_subvol* bcs;
    IO_STATUS_BLOCK iosb;

    if (found == wstring::npos) {
        path = L"";
        file = fn;
    } else {
        path = fn.substr(0, found);
        file = fn.substr(found + 1);
    }
    path += L"\\";

    h = CreateFileW(path.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return;

    bcslen = offsetof(btrfs_create_subvol, name[0]) + (file.length() * sizeof(WCHAR));
    bcs = (btrfs_create_subvol*)malloc(bcslen);

    bcs->readonly = false;
    bcs->posix = false;
    bcs->namelen = (uint16_t)(file.length() * sizeof(WCHAR));
    memcpy(bcs->name, file.c_str(), bcs->namelen);

    NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, bcs, bcslen, nullptr, 0);
}

extern "C" void CALLBACK CreateSubvolW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    vector<wstring> args;

    command_line_to_args(lpszCmdLine, args);

    if (args.size() >= 1)
        create_subvol(args[0]);
}

static void create_snapshot2(const wstring& source, const wstring& fn) {
    size_t found = fn.rfind(L"\\");
    wstring path, file;
    win_handle h, src;
    ULONG bcslen;
    btrfs_create_snapshot* bcs;
    IO_STATUS_BLOCK iosb;

    if (found == wstring::npos) {
        path = L"";
        file = fn;
    } else {
        path = fn.substr(0, found);
        file = fn.substr(found + 1);
    }
    path += L"\\";

    src = CreateFileW(source.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
    if (src == INVALID_HANDLE_VALUE)
        return;

    h = CreateFileW(path.c_str(), FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return;

    bcslen = offsetof(btrfs_create_snapshot, name[0]) + (file.length() * sizeof(WCHAR));
    bcs = (btrfs_create_snapshot*)malloc(bcslen);

    bcs->readonly = false;
    bcs->posix = false;
    bcs->namelen = (uint16_t)(file.length() * sizeof(WCHAR));
    memcpy(bcs->name, file.c_str(), bcs->namelen);
    bcs->subvol = src;

    NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, bcs, bcslen, nullptr, 0);
}

extern "C" void CALLBACK CreateSnapshotW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    vector<wstring> args;

    command_line_to_args(lpszCmdLine, args);

    if (args.size() >= 2)
        create_snapshot2(args[0], args[1]);
}

void command_line_to_args(LPWSTR cmdline, vector<wstring> args) {
    LPWSTR* l;
    int num_args;

    args.clear();

    l = CommandLineToArgvW(cmdline, &num_args);

    if (!l)
        return;

    try {
        args.reserve(num_args);

        for (unsigned int i = 0; i < (unsigned int)num_args; i++) {
            args.push_back(l[i]);
        }
    } catch (...) {
        LocalFree(l);
        throw;
    }

    LocalFree(l);
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4996)
#endif

string_error::string_error(int resno, ...) {
    wstring fmt, s;
    int len;
    va_list args;

    if (!load_string(module, resno, fmt))
        throw runtime_error("LoadString failed."); // FIXME

    va_start(args, resno);
    len = _vsnwprintf(nullptr, 0, fmt.c_str(), args);

    if (len == 0)
        s = L"";
    else {
        s.resize(len);
        _vsnwprintf((wchar_t*)s.c_str(), len, fmt.c_str(), args);
    }

    va_end(args);

    utf16_to_utf8(s, msg);
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

void utf8_to_utf16(const string& utf8, wstring& utf16) {
    NTSTATUS Status;
    ULONG utf16len;
    WCHAR* buf;

    Status = RtlUTF8ToUnicodeN(nullptr, 0, &utf16len, utf8.c_str(), utf8.length());
    if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_RTLUTF8TOUNICODEN_FAILED, Status, format_ntstatus(Status).c_str());

    buf = (WCHAR*)malloc(utf16len + sizeof(WCHAR));

    if (!buf)
        throw string_error(IDS_OUT_OF_MEMORY);

    Status = RtlUTF8ToUnicodeN(buf, utf16len, &utf16len, utf8.c_str(), utf8.length());
    if (!NT_SUCCESS(Status)) {
        free(buf);
        throw string_error(IDS_RECV_RTLUTF8TOUNICODEN_FAILED, Status, format_ntstatus(Status).c_str());
    }

    buf[utf16len / sizeof(WCHAR)] = 0;

    utf16 = buf;

    free(buf);
}

void utf16_to_utf8(const wstring& utf16, string& utf8) {
    NTSTATUS Status;
    ULONG utf8len;
    char* buf;

    Status = RtlUnicodeToUTF8N(nullptr, 0, &utf8len, utf16.c_str(), utf16.length() * sizeof(WCHAR));
    if (!NT_SUCCESS(Status))
        throw string_error(IDS_RECV_RTLUNICODETOUTF8N_FAILED, Status, format_ntstatus(Status).c_str());

    buf = (char*)malloc(utf8len + sizeof(char));

    if (!buf)
        throw string_error(IDS_OUT_OF_MEMORY);

    Status = RtlUnicodeToUTF8N(buf, utf8len, &utf8len, utf16.c_str(), utf16.length() * sizeof(WCHAR));
    if (!NT_SUCCESS(Status)) {
        free(buf);
        throw string_error(IDS_RECV_RTLUNICODETOUTF8N_FAILED, Status, format_ntstatus(Status).c_str());
    }

    buf[utf8len] = 0;

    utf8 = buf;

    free(buf);
}

last_error::last_error(DWORD errnum) {
    WCHAR* buf;

    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
        errnum, 0, (WCHAR*)&buf, 0, nullptr) == 0)
        throw runtime_error("FormatMessage failed");

    try {
        utf16_to_utf8(buf, msg);
    } catch (...) {
        LocalFree(buf);
        throw;
    }

    LocalFree(buf);
}

void error_message(HWND hwnd, const char* msg) {
    wstring title, wmsg;

    load_string(module, IDS_ERROR, title);

    utf8_to_utf16(msg, wmsg);

    MessageBoxW(hwnd, wmsg.c_str(), title.c_str(), MB_ICONERROR);
}

ntstatus_error::ntstatus_error(NTSTATUS Status) {
    _RtlNtStatusToDosError RtlNtStatusToDosError;
    HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
    WCHAR* buf;

    if (!ntdll)
        throw runtime_error("Error loading ntdll.dll.");

    try {
        RtlNtStatusToDosError = (_RtlNtStatusToDosError)GetProcAddress(ntdll, "RtlNtStatusToDosError");

        if (!RtlNtStatusToDosError)
            throw runtime_error("Error loading RtlNtStatusToDosError in ntdll.dll.");

        if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr,
            RtlNtStatusToDosError(Status), 0, (WCHAR*)&buf, 0, nullptr) == 0)
            throw runtime_error("FormatMessage failed");

        try {
            utf16_to_utf8(buf, msg);
        } catch (...) {
            LocalFree(buf);
            throw;
        }

        LocalFree(buf);
    } catch (...) {
        FreeLibrary(ntdll);
        throw;
    }

    FreeLibrary(ntdll);
}

#ifdef __REACTOS__
NTSTATUS NTAPI RtlUnicodeToUTF8N(CHAR *utf8_dest, ULONG utf8_bytes_max,
                                 ULONG *utf8_bytes_written,
                                 const WCHAR *uni_src, ULONG uni_bytes)
{
    NTSTATUS status;
    ULONG i;
    ULONG written;
    ULONG ch;
    BYTE utf8_ch[4];
    ULONG utf8_ch_len;

    if (!uni_src)
        return STATUS_INVALID_PARAMETER_4;
    if (!utf8_bytes_written)
        return STATUS_INVALID_PARAMETER;
    if (utf8_dest && uni_bytes % sizeof(WCHAR))
        return STATUS_INVALID_PARAMETER_5;

    written = 0;
    status = STATUS_SUCCESS;

    for (i = 0; i < uni_bytes / sizeof(WCHAR); i++)
    {
        /* decode UTF-16 into ch */
        ch = uni_src[i];
        if (ch >= 0xdc00 && ch <= 0xdfff)
        {
            ch = 0xfffd;
            status = STATUS_SOME_NOT_MAPPED;
        }
        else if (ch >= 0xd800 && ch <= 0xdbff)
        {
            if (i + 1 < uni_bytes / sizeof(WCHAR))
            {
                ch -= 0xd800;
                ch <<= 10;
                if (uni_src[i + 1] >= 0xdc00 && uni_src[i + 1] <= 0xdfff)
                {
                    ch |= uni_src[i + 1] - 0xdc00;
                    ch += 0x010000;
                    i++;
                }
                else
                {
                    ch = 0xfffd;
                    status = STATUS_SOME_NOT_MAPPED;
                }
            }
            else
            {
                ch = 0xfffd;
                status = STATUS_SOME_NOT_MAPPED;
            }
        }

        /* encode ch as UTF-8 */
        if (ch < 0x80)
        {
            utf8_ch[0] = ch & 0x7f;
            utf8_ch_len = 1;
        }
        else if (ch < 0x800)
        {
            utf8_ch[0] = 0xc0 | (ch >>  6 & 0x1f);
            utf8_ch[1] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 2;
        }
        else if (ch < 0x10000)
        {
            utf8_ch[0] = 0xe0 | (ch >> 12 & 0x0f);
            utf8_ch[1] = 0x80 | (ch >>  6 & 0x3f);
            utf8_ch[2] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 3;
        }
        else if (ch < 0x200000)
        {
            utf8_ch[0] = 0xf0 | (ch >> 18 & 0x07);
            utf8_ch[1] = 0x80 | (ch >> 12 & 0x3f);
            utf8_ch[2] = 0x80 | (ch >>  6 & 0x3f);
            utf8_ch[3] = 0x80 | (ch >>  0 & 0x3f);
            utf8_ch_len = 4;
        }

        if (!utf8_dest)
        {
            written += utf8_ch_len;
            continue;
        }

        if (utf8_bytes_max >= utf8_ch_len)
        {
            memcpy(utf8_dest, utf8_ch, utf8_ch_len);
            utf8_dest += utf8_ch_len;
            utf8_bytes_max -= utf8_ch_len;
            written += utf8_ch_len;
        }
        else
        {
            utf8_bytes_max = 0;
            status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    *utf8_bytes_written = written;
    return status;
}
#endif
