/* Copyright (c) Mark Harmstone 2016
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

#ifndef __REACTOS__
#include <windows.h>
#include <winternl.h>
#else
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <strsafe.h>
#include <ndk/iofuncs.h>
#endif

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>

#include "contextmenu.h"
#include "resource.h"
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "../../drivers/filesystems/btrfs/btrfsioctl.h"
#endif

#define NEW_SUBVOL_VERBA "newsubvol"
#define NEW_SUBVOL_VERBW L"newsubvol"
#define SNAPSHOT_VERBA "snapshot"
#define SNAPSHOT_VERBW L"snapshot"

#ifndef __REACTOS__
// FIXME - is there a way to link to the proper header files without breaking everything?
#ifdef __cplusplus
extern "C" {
#endif
NTSYSCALLAPI NTSTATUS NTAPI NtFsControlFile(HANDLE FileHandle, HANDLE Event, PIO_APC_ROUTINE ApcRoutine, PVOID ApcContext, PIO_STATUS_BLOCK IoStatusBlock, ULONG FsControlCode, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength);
#ifdef __cplusplus
}
#endif

#define STATUS_SUCCESS          (NTSTATUS)0x00000000
#endif

typedef struct _KEY_NAME_INFORMATION {
    ULONG NameLength;
    WCHAR Name[1];
} KEY_NAME_INFORMATION;

typedef ULONG (WINAPI *_RtlNtStatusToDosError)(NTSTATUS Status);

extern HMODULE module;

// FIXME - don't assume subvol's top inode is 0x100

HRESULT __stdcall BtrfsContextMenu::QueryInterface(REFIID riid, void **ppObj) {
    if (riid == IID_IUnknown || riid == IID_IContextMenu) {
        *ppObj = static_cast<IContextMenu*>(this); 
        AddRef();
        return S_OK;
    } else if (riid == IID_IShellExtInit) {
        *ppObj = static_cast<IShellExtInit*>(this); 
        AddRef();
        return S_OK;
    }

    *ppObj = NULL;
    return E_NOINTERFACE;
}

HRESULT __stdcall BtrfsContextMenu::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;
    NTSTATUS Status;
    
    if (!pidlFolder) {
        FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        UINT num_files, i;
        WCHAR fn[MAX_PATH];
        HDROP hdrop;
        
        if (!pdtobj)
            return E_FAIL;
        
        stgm.tymed = TYMED_HGLOBAL;
        
        if (FAILED(pdtobj->GetData(&format, &stgm)))
            return E_INVALIDARG;
        
        stgm_set = TRUE;
        
        hdrop = (HDROP)GlobalLock(stgm.hGlobal);
        
        if (!hdrop) {
            ReleaseStgMedium(&stgm);
            stgm_set = FALSE;
            return E_INVALIDARG;
        }
         
        num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);
        
        for (i = 0; i < num_files; i++) {
            if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
                h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    
                if (h != INVALID_HANDLE_VALUE) {
                    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));
                        
                    if (Status == STATUS_SUCCESS && bgfi.inode == 0x100 && !bgfi.top) {
                        WCHAR parpath[MAX_PATH];
                        HANDLE h2;
                        
                        StringCchCopyW(parpath, sizeof(parpath) / sizeof(WCHAR), fn);
                        
                        PathRemoveFileSpecW(parpath);
                        
                        h2 = CreateFileW(parpath, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    
                        if (h2 == INVALID_HANDLE_VALUE) {
                            CloseHandle(h);
                            return E_FAIL;
                        }
                        
                        ignore = FALSE;
                        bg = FALSE;
                        
                        CloseHandle(h2);
                        CloseHandle(h);
                        return S_OK;
                    }
                    
                    CloseHandle(h);
                }
            }
        }
        
        return S_OK;
    }

    if (!SHGetPathFromIDListW(pidlFolder, path))
        return E_FAIL;
    
    // check we have permissions to create new subdirectory
    
    h = CreateFileW(path, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    
    if (h == INVALID_HANDLE_VALUE)
        return E_FAIL;
    
    // check is Btrfs volume
    
    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));
    
    if (Status != STATUS_SUCCESS) {
        CloseHandle(h);
        return E_FAIL;
    }
    
    CloseHandle(h);
    
    ignore = FALSE;
    bg = TRUE;
    
    return S_OK;
}

HRESULT __stdcall BtrfsContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags) {
    WCHAR str[256];
    
    if (ignore)
        return E_INVALIDARG;
    
    if (uFlags & CMF_DEFAULTONLY)
        return S_OK;
    
    if (!bg) {
        if (LoadStringW(module, IDS_CREATE_SNAPSHOT, str, sizeof(str) / sizeof(WCHAR)) == 0)
            return E_FAIL;

        if (!InsertMenuW(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst, str))
            return E_FAIL;

        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
    }
    
    if (LoadStringW(module, IDS_NEW_SUBVOL, str, sizeof(str) / sizeof(WCHAR)) == 0)
        return E_FAIL;

    if (!InsertMenuW(hmenu, indexMenu, MF_BYPOSITION, idCmdFirst, str))
        return E_FAIL;

    return MAKE_HRESULT(SEVERITY_SUCCESS, 0, 1);
}

static void ShowError(HWND hwnd, ULONG err) {
    WCHAR* buf;
    
    if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL,
                       err, 0, (WCHAR*)&buf, 0, NULL) == 0) {
        MessageBoxW(hwnd, L"FormatMessage failed", L"Error", MB_ICONERROR);
        return;
    }
    
    MessageBoxW(hwnd, buf, L"Error", MB_ICONERROR);
    
    LocalFree(buf);
}

void ShowNtStatusError(HWND hwnd, NTSTATUS Status) {
    _RtlNtStatusToDosError RtlNtStatusToDosError;
    HMODULE ntdll = LoadLibraryW(L"ntdll.dll");
    
    if (!ntdll) {
        MessageBoxW(hwnd, L"Error loading ntdll.dll", L"Error", MB_ICONERROR);
        return;
    }
    
    RtlNtStatusToDosError = (_RtlNtStatusToDosError)GetProcAddress(ntdll, "RtlNtStatusToDosError");
    
    if (!ntdll) {
        MessageBoxW(hwnd, L"Error loading RtlNtStatusToDosError in ntdll.dll", L"Error", MB_ICONERROR);
        FreeLibrary(ntdll);
        return;
    }
    
    ShowError(hwnd, RtlNtStatusToDosError(Status));
    
    FreeLibrary(ntdll);
}

static void create_snapshot(HWND hwnd, WCHAR* fn) {
    HANDLE h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;
    
    h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    
    if (h != INVALID_HANDLE_VALUE) {
        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));
            
        if (Status == STATUS_SUCCESS && bgfi.inode == 0x100 && !bgfi.top) {
            WCHAR parpath[MAX_PATH], subvolname[MAX_PATH], templ[MAX_PATH], name[MAX_PATH], searchpath[MAX_PATH];
            HANDLE h2, fff;
            btrfs_create_snapshot* bcs;
            ULONG namelen, pathend;
            WIN32_FIND_DATAW wfd;
            SYSTEMTIME time;
            
            StringCchCopyW(parpath, sizeof(parpath) / sizeof(WCHAR), fn);
            PathRemoveFileSpecW(parpath);
            
            StringCchCopyW(subvolname, sizeof(subvolname) / sizeof(WCHAR), fn);
            PathStripPathW(subvolname);
            
            h2 = CreateFileW(parpath, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);

            if (h2 == INVALID_HANDLE_VALUE) {
                ShowError(hwnd, GetLastError());
                CloseHandle(h);
                return;
            }
            
            if (!LoadStringW(module, IDS_SNAPSHOT_FILENAME, templ, MAX_PATH)) {
                ShowError(hwnd, GetLastError());
                CloseHandle(h);
                CloseHandle(h2);
                return;
            }
            
            GetLocalTime(&time);
            
            if (StringCchPrintfW(name, sizeof(name) / sizeof(WCHAR), templ, subvolname, time.wYear, time.wMonth, time.wDay) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                MessageBoxW(hwnd, L"Filename too long.\n", L"Error", MB_ICONERROR);
                CloseHandle(h);
                CloseHandle(h2);
                return;
            }
            
            StringCchCopyW(searchpath, sizeof(searchpath) / sizeof(WCHAR), parpath);
            StringCchCatW(searchpath, sizeof(searchpath) / sizeof(WCHAR), L"\\");
            pathend = wcslen(searchpath);
            
            StringCchCatW(searchpath, sizeof(searchpath) / sizeof(WCHAR), name);
            
            fff = FindFirstFileW(searchpath, &wfd);
            
            if (fff != INVALID_HANDLE_VALUE) {
                ULONG i = wcslen(searchpath), num = 2;
                
                do {
                    FindClose(fff);
                    
                    searchpath[i] = 0;
                    if (StringCchPrintfW(searchpath, sizeof(searchpath) / sizeof(WCHAR), L"%s (%u)", searchpath, num) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                        MessageBoxW(hwnd, L"Filename too long.\n", L"Error", MB_ICONERROR);
                        CloseHandle(h);
                        CloseHandle(h2);
                        return;
                    }

                    fff = FindFirstFileW(searchpath, &wfd);
                    num++;
                } while (fff != INVALID_HANDLE_VALUE);
            }
            
            namelen = wcslen(&searchpath[pathend]) * sizeof(WCHAR);
                        
            bcs = (btrfs_create_snapshot*)malloc(sizeof(btrfs_create_snapshot) - 1 + namelen);
            bcs->subvol = h;
            bcs->namelen = namelen;
            memcpy(bcs->name, &searchpath[pathend], namelen);
            
            Status = NtFsControlFile(h2, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SNAPSHOT, NULL, 0, bcs, sizeof(btrfs_create_snapshot) - 1 + namelen);
        
            if (Status != STATUS_SUCCESS)
                ShowNtStatusError(hwnd, Status);
            
            CloseHandle(h2);
        }
        
        CloseHandle(h);
    } else
        ShowError(hwnd, GetLastError());
}

HRESULT __stdcall BtrfsContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO pici) {
    if (ignore)
        return E_INVALIDARG;
    
    if (!bg) {
        if ((IS_INTRESOURCE(pici->lpVerb) && pici->lpVerb == 0) || !strcmp(pici->lpVerb, SNAPSHOT_VERBA)) {
            UINT num_files, i;
            WCHAR fn[MAX_PATH];
            
            if (!stgm_set)
                return E_FAIL;
            
            num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);
            
            if (num_files == 0)
                return E_FAIL;
            
            for (i = 0; i < num_files; i++) {
                if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
                    create_snapshot(pici->hwnd, fn);
                }
            }

            return S_OK;
        }
        
        return E_FAIL;
    }
    
    if ((IS_INTRESOURCE(pici->lpVerb) && pici->lpVerb == 0) || !strcmp(pici->lpVerb, NEW_SUBVOL_VERBA)) {
        HANDLE h;
        IO_STATUS_BLOCK iosb;
        NTSTATUS Status;
        ULONG pathlen, searchpathlen, pathend;
        WCHAR name[MAX_PATH], *searchpath;
        HANDLE fff;
        WIN32_FIND_DATAW wfd;
        
        if (!LoadStringW(module, IDS_NEW_SUBVOL_FILENAME, name, MAX_PATH)) {
            ShowError(pici->hwnd, GetLastError());
            return E_FAIL;
        }
        
        h = CreateFileW(path, FILE_ADD_SUBDIRECTORY, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    
        if (h == INVALID_HANDLE_VALUE) {
            ShowError(pici->hwnd, GetLastError());
            return E_FAIL;
        }
        
        pathlen = wcslen(path);
        
        searchpathlen = pathlen + wcslen(name) + 10;
        searchpath = (WCHAR*)malloc(searchpathlen * sizeof(WCHAR));
        
        StringCchCopyW(searchpath, searchpathlen, path);
        StringCchCatW(searchpath, searchpathlen, L"\\");
        pathend = wcslen(searchpath);
        
        StringCchCatW(searchpath, searchpathlen, name);
        
        fff = FindFirstFileW(searchpath, &wfd);
        
        if (fff != INVALID_HANDLE_VALUE) {
            ULONG i = wcslen(searchpath), num = 2;
            
            do {
                FindClose(fff);
                
                searchpath[i] = 0;
                if (StringCchPrintfW(searchpath, searchpathlen, L"%s (%u)", searchpath, num) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                    MessageBoxW(pici->hwnd, L"Filename too long.\n", L"Error", MB_ICONERROR);
                    CloseHandle(h);
                    return E_FAIL;
                }

                fff = FindFirstFileW(searchpath, &wfd);
                num++;
            } while (fff != INVALID_HANDLE_VALUE);
        }
        
        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_CREATE_SUBVOL, NULL, 0, &searchpath[pathend], wcslen(&searchpath[pathend]) * sizeof(WCHAR));
        
        free(searchpath);
        
        if (Status != STATUS_SUCCESS) {
            CloseHandle(h);
            ShowNtStatusError(pici->hwnd, Status);
            return E_FAIL;
        }
        
        CloseHandle(h);
        
        return S_OK;
    }
    
    return E_FAIL;
}

HRESULT __stdcall BtrfsContextMenu::GetCommandString(UINT_PTR idCmd, UINT uFlags, UINT* pwReserved, LPSTR pszName, UINT cchMax) {
    if (ignore)
        return E_INVALIDARG;
    
    if (idCmd != 0)
        return E_INVALIDARG;
    
    switch (uFlags) {
        case GCS_HELPTEXTA:
            if (LoadStringA(module, bg ? IDS_NEW_SUBVOL_HELP_TEXT : IDS_CREATE_SNAPSHOT_HELP_TEXT, pszName, cchMax))
                return S_OK;
            else
                return E_FAIL;
            
        case GCS_HELPTEXTW:
            if (LoadStringW(module, bg ? IDS_NEW_SUBVOL_HELP_TEXT : IDS_CREATE_SNAPSHOT_HELP_TEXT, (LPWSTR)pszName, cchMax))
                return S_OK;
            else
                return E_FAIL;
            
        case GCS_VALIDATEA:
        case GCS_VALIDATEW:
            return S_OK;
            
        case GCS_VERBA:
            return StringCchCopyA(pszName, cchMax, bg ? NEW_SUBVOL_VERBA : SNAPSHOT_VERBA);
            
        case GCS_VERBW:
            return StringCchCopyW((STRSAFE_LPWSTR)pszName, cchMax, bg ? NEW_SUBVOL_VERBW : SNAPSHOT_VERBW);
            
        default:
            return E_INVALIDARG;
    }
}
