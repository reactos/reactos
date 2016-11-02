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

#define ISOLATION_AWARE_ENABLED 1
#define STRSAFE_NO_DEPRECATE

#ifndef __REACTOS__
#include <windows.h>
#include <strsafe.h>
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
#include <uxtheme.h>

#include "propsheet.h"
#include "resource.h"

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

#define BTRFS_TYPE_FILE      1
#define BTRFS_TYPE_DIRECTORY 2
#define BTRFS_TYPE_CHARDEV   3
#define BTRFS_TYPE_BLOCKDEV  4
#define BTRFS_TYPE_FIFO      5
#define BTRFS_TYPE_SOCKET    6
#define BTRFS_TYPE_SYMLINK   7

extern HMODULE module;

extern void ShowNtStatusError(HWND hwnd, NTSTATUS Status);
static void format_size(UINT64 size, WCHAR* s, ULONG len);
static void ShowError(HWND hwnd, ULONG err);

HRESULT __stdcall BtrfsPropSheet::QueryInterface(REFIID riid, void **ppObj) {
    if (riid == IID_IUnknown || riid == IID_IShellPropSheetExt) {
        *ppObj = static_cast<IShellPropSheetExt*>(this); 
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

void BtrfsPropSheet::add_to_search_list(WCHAR* fn) {
    WCHAR* s;
    
    s = (WCHAR*)malloc((wcslen(fn) + 1) * sizeof(WCHAR));
    if (!s)
        return;
    
    memcpy(s, fn, (wcslen(fn) + 1) * sizeof(WCHAR));
    
    search_list.push_back(s);
}

void BtrfsPropSheet::do_search(WCHAR* fn) {
    HANDLE h;
    WCHAR* ss;
    WIN32_FIND_DATAW ffd;
    
    ss = (WCHAR*)malloc((wcslen(fn) + 3) * sizeof(WCHAR));
    if (!ss)
        return;
    
    memcpy(ss, fn, (wcslen(fn) + 1) * sizeof(WCHAR));
    wcscat(ss, L"\\*");
    
    h = FindFirstFileW(ss, &ffd);
    if (h == INVALID_HANDLE_VALUE)
        return;
    
    do {
        if (ffd.cFileName[0] != '.' || ((ffd.cFileName[1] != 0) && (ffd.cFileName[1] != '.' || ffd.cFileName[2] != 0))) {
            WCHAR* fn2 = (WCHAR*)malloc((wcslen(fn) + 1 + wcslen(ffd.cFileName) + 1) * sizeof(WCHAR));
                
            memcpy(fn2, fn, (wcslen(fn) + 1) * sizeof(WCHAR));
            wcscat(fn2, L"\\");
            wcscat(fn2, ffd.cFileName);
            
            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                add_to_search_list(fn2);
            } else {
                HANDLE fh;
                
                fh = CreateFileW(fn2, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                                 OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
                
                if (fh != INVALID_HANDLE_VALUE) {
                    NTSTATUS Status;
                    IO_STATUS_BLOCK iosb;
                    btrfs_inode_info bii2;
                    
                    Status = NtFsControlFile(fh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii2, sizeof(btrfs_inode_info));
                
                    if (Status == STATUS_SUCCESS) {
                        sizes[0] += bii2.inline_length;
                        sizes[1] += bii2.disk_size[0];
                        sizes[2] += bii2.disk_size[1];
                        sizes[3] += bii2.disk_size[2];
                        totalsize += bii2.inline_length + bii2.disk_size[0] + bii2.disk_size[1] + bii2.disk_size[2];
                    }
                    
                    CloseHandle(fh);
                }
                
                free(fn2);
            }
        }
    } while (FindNextFileW(h, &ffd));
    
    FindClose(h);
}

DWORD BtrfsPropSheet::search_list_thread() {
    while (!search_list.empty()) {
        WCHAR* fn = search_list.front();
        
        do_search(fn);
        
        search_list.pop_front();
        free(fn);
    }
    
    thread = NULL;
    
    return 0;
}

static DWORD WINAPI global_search_list_thread(LPVOID lpParameter) {
    BtrfsPropSheet* bps = (BtrfsPropSheet*)lpParameter;
    
    return bps->search_list_thread();
}

HRESULT __stdcall BtrfsPropSheet::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    UINT num_files, i;
    WCHAR fn[MAX_PATH];
    HDROP hdrop;
    
    if (pidlFolder)
        return E_FAIL;
        
    
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
    
    min_mode = 0;
    max_mode = 0;
    min_flags = 0;
    max_flags = 0;
    various_subvols = various_inodes = various_types = various_uids = various_gids = FALSE;
    
    can_change_perms = TRUE;
    can_change_owner = TRUE;
    can_change_nocow = TRUE;
    
    for (i = 0; i < num_files; i++) {
        if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
            h = CreateFileW(fn, FILE_TRAVERSE | FILE_READ_ATTRIBUTES | WRITE_DAC, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
            
            if (h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
            else
                can_change_perms = FALSE;
            
            h = CreateFileW(fn, FILE_TRAVERSE | FILE_READ_ATTRIBUTES | WRITE_OWNER, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
            
            if (h != INVALID_HANDLE_VALUE)
                CloseHandle(h);
            else
                can_change_owner = FALSE;
            
            h = CreateFileW(fn, FILE_TRAVERSE | FILE_READ_ATTRIBUTES | FILE_WRITE_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

            if (h == INVALID_HANDLE_VALUE && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == ERROR_WRITE_PROTECT)) {
                h = CreateFileW(fn, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
                                FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
                
                readonly = TRUE;
            }
            
            if (h != INVALID_HANDLE_VALUE) {
                BY_HANDLE_FILE_INFORMATION bhfi;
                btrfs_inode_info bii2;
                
                if (GetFileInformationByHandle(h, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                    add_to_search_list(fn);
                
                Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii2, sizeof(btrfs_inode_info));
                    
                if (Status == STATUS_SUCCESS && !bii2.top) {
                    int j;
                    
                    LARGE_INTEGER filesize;
                    
                    if (i == 0) {
                        subvol = bii2.subvol;
                        inode = bii2.inode;
                        type = bii2.type;
                        uid = bii2.st_uid;
                        gid = bii2.st_gid;
                        rdev = bii2.st_rdev;
                    } else {
                        if (subvol != bii2.subvol)
                            various_subvols = TRUE;
                        
                        if (inode != bii2.inode)
                            various_inodes = TRUE;
                        
                        if (type != bii2.type)
                            various_types = TRUE;
                        
                        if (uid != bii2.st_uid)
                            various_uids = TRUE;

                        if (gid != bii2.st_gid)
                            various_gids = TRUE;
                    }

                    if (bii2.inline_length > 0) {
                        totalsize += bii2.inline_length;
                        sizes[0] += bii2.inline_length;
                    }
                    
                    for (j = 0; j < 3; j++) {
                        if (bii2.disk_size[j] > 0) {
                            totalsize += bii2.disk_size[j];
                            sizes[j + 1] += bii2.disk_size[j];
                        }
                    }
                    
                    min_mode |= ~bii2.st_mode;
                    max_mode |= bii2.st_mode;
                    min_flags |= ~bii2.flags;
                    max_flags |= bii2.flags;
                    
                    ignore = FALSE;
                    
                    if (bii2.type != BTRFS_TYPE_DIRECTORY && GetFileSizeEx(h, &filesize)) {
                        if (filesize.QuadPart != 0)
                            can_change_nocow = FALSE;
                    }
                    
                    CloseHandle(h);
                } else {
                    CloseHandle(h);
                    return E_FAIL;
                }
            } else
                return E_FAIL;
        } else
            return E_FAIL;
    }
    
    min_mode = ~min_mode;
    min_flags = ~min_flags;
    
    mode = min_mode;
    mode_set = ~(min_mode ^ max_mode);
    
    flags = min_flags;
    flags_set = ~(min_flags ^ max_flags);
    
    if (search_list.size() > 0) {
        thread = CreateThread(NULL, 0, global_search_list_thread, this, 0, NULL);
        
        if (!thread)
            ShowError(NULL, GetLastError());
    }

    return S_OK;
}

static ULONG inode_type_to_string_ref(UINT8 type) {
    switch (type) {    
        case BTRFS_TYPE_FILE:
            return IDS_INODE_FILE;
        
        case BTRFS_TYPE_DIRECTORY:
            return IDS_INODE_DIR;
            
        case BTRFS_TYPE_CHARDEV:
            return IDS_INODE_CHAR;
            
        case BTRFS_TYPE_BLOCKDEV:
            return IDS_INODE_BLOCK;
            
        case BTRFS_TYPE_FIFO:
            return IDS_INODE_FIFO;
            
        case BTRFS_TYPE_SOCKET:
            return IDS_INODE_SOCKET;
            
        case BTRFS_TYPE_SYMLINK:
            return IDS_INODE_SYMLINK;
            
        default:
            return IDS_INODE_UNKNOWN;
    }
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

void BtrfsPropSheet::change_inode_flag(HWND hDlg, UINT64 flag, UINT state) {
    if (flag & BTRFS_INODE_NODATACOW)
        flag |= BTRFS_INODE_NODATASUM;
    
    if (state == BST_CHECKED) {
        flags |= flag;
        flags_set |= flag;
    } else if (state == BST_UNCHECKED) {
        flags &= ~flag;
        flags_set |= flag;
    } else if (state == BST_INDETERMINATE) {
        flags_set = ~flag;
    }
    
    flags_changed = TRUE;
    
    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
}

void BtrfsPropSheet::apply_changes(HWND hDlg) {
    UINT num_files, i;
    WCHAR fn[MAX_PATH]; // FIXME - is this long enough?
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    btrfs_set_inode_info bsii;
    
    if (various_uids)
        uid_changed = FALSE;
    
    if (various_gids)
        gid_changed = FALSE;
    
    if (!flags_changed && !perms_changed && !uid_changed && !gid_changed)
        return;

    num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);
    
    for (i = 0; i < num_files; i++) {
        if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
            ULONG perms = FILE_TRAVERSE | FILE_READ_ATTRIBUTES;
            
            if (flags_changed)
                perms |= FILE_WRITE_ATTRIBUTES;
            
            if (perms_changed)
                perms |= WRITE_DAC;
            
            if (uid_changed || gid_changed)
                perms |= WRITE_OWNER;
            
            h = CreateFileW(fn, perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

            if (h == INVALID_HANDLE_VALUE) {
                ShowError(hDlg, GetLastError());
                return;
            }
            
            ZeroMemory(&bsii, sizeof(btrfs_set_inode_info));
            
            btrfs_inode_info bii2;
                    
            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii2, sizeof(btrfs_inode_info));
        
            if (Status != STATUS_SUCCESS) {
                ShowNtStatusError(hDlg, Status);
                CloseHandle(h);
                return;
            }
            
            if (flags_changed) {
                bsii.flags_changed = TRUE;
                bsii.flags = (bii2.flags & ~flags_set) | (flags & flags_set);
            }
            
            if (perms_changed) {
                bsii.mode_changed = TRUE;
                bsii.st_mode = (bii2.st_mode & ~mode_set) | (mode & mode_set);
            }
            
            if (uid_changed) {
                bsii.uid_changed = TRUE;
                bsii.st_uid = uid;
            }
            
            if (gid_changed) {
                bsii.gid_changed = TRUE;
                bsii.st_gid = gid;
            }
            
            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, NULL, 0, &bsii, sizeof(btrfs_set_inode_info));
            CloseHandle(h);

            if (Status != STATUS_SUCCESS) {
                WCHAR s[255], t[255];
                
                if (!LoadStringW(module, IDS_SET_INODE_INFO_ERROR, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hDlg, GetLastError());
                    return;
                }
                
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, Status) == STRSAFE_E_INSUFFICIENT_BUFFER) {
                    ShowError(hDlg, ERROR_INSUFFICIENT_BUFFER);
                    return;
                }
                
                MessageBoxW(hDlg, s, L"Error", MB_ICONERROR);
            }
        }
    }
    
    flags_changed = FALSE;
    perms_changed = FALSE;
    uid_changed = FALSE;
    gid_changed = FALSE;
}

void BtrfsPropSheet::set_size_on_disk(HWND hwndDlg) {
    WCHAR size_on_disk[1024], s[1024], old_text[1024];
    
    format_size(totalsize, size_on_disk, sizeof(size_on_disk) / sizeof(WCHAR));
    
    if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), size_format, size_on_disk) == STRSAFE_E_INSUFFICIENT_BUFFER) {
        ShowError(hwndDlg, ERROR_INSUFFICIENT_BUFFER);
        return;
    }
    
    GetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, old_text, sizeof(old_text) / sizeof(WCHAR));
    
    if (wcscmp(s, old_text))
        SetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, s);
}

void BtrfsPropSheet::change_perm_flag(HWND hDlg, ULONG flag, UINT state) {
    if (state == BST_CHECKED) {
        mode |= flag;
        mode_set |= flag;
    } else if (state == BST_UNCHECKED) {
        mode &= ~flag;
        mode_set |= flag;
    } else if (state == BST_INDETERMINATE) {
        mode_set = ~flag;
    }
    
    perms_changed = TRUE;
    
    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
}

void BtrfsPropSheet::change_uid(HWND hDlg, UINT32 uid) {
    this->uid = uid;
    
    uid_changed = TRUE;
    
    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
}

void BtrfsPropSheet::change_gid(HWND hDlg, UINT32 gid) {
    this->gid = gid;
    
    gid_changed = TRUE;
    
    SendMessageW(GetParent(hDlg), PSM_CHANGED, 0, 0);
}

void BtrfsPropSheet::update_size_details_dialog(HWND hDlg) {
    WCHAR size[1024], old_text[1024];
    int i;
    ULONG items[] = { IDC_SIZE_INLINE, IDC_SIZE_UNCOMPRESSED, IDC_SIZE_ZLIB, IDC_SIZE_LZO };
    
    for (i = 0; i < 4; i++) {
        format_size(sizes[i], size, sizeof(size) / sizeof(WCHAR));
        
        GetDlgItemTextW(hDlg, items[i], old_text, sizeof(old_text) / sizeof(WCHAR));
        
        if (wcscmp(size, old_text))
            SetDlgItemTextW(hDlg, items[i], size);
    }
}

static INT_PTR CALLBACK SizeDetailsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            BtrfsPropSheet* bps = (BtrfsPropSheet*)lParam;
            
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)bps);
            
            bps->update_size_details_dialog(hwndDlg);
            
            if (bps->thread)
                SetTimer(hwndDlg, 1, 250, NULL);
            
            return TRUE;
        }
            
        case WM_COMMAND:
            if (HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
                EndDialog(hwndDlg, 0);
                return TRUE;
            }
        break;
        
        case WM_TIMER:
        {
            BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
            
            if (bps) {
                bps->update_size_details_dialog(hwndDlg);
                
                if (!bps->thread)
                    KillTimer(hwndDlg, 1);
            }
            
            break;
        }
    }
    
    return FALSE;
}

static void set_check_box(HWND hwndDlg, ULONG id, UINT64 min, UINT64 max) {
    if (min && max) {
        SendDlgItemMessage(hwndDlg, id, BM_SETCHECK, BST_CHECKED, 0);
    } else if (!min && !max) {
        SendDlgItemMessage(hwndDlg, id, BM_SETCHECK, BST_UNCHECKED, 0);
    } else {
        LONG_PTR style;
        
        style = GetWindowLongPtr(GetDlgItem(hwndDlg, id), GWL_STYLE);
        style &= ~BS_AUTOCHECKBOX;
        style |= BS_AUTO3STATE;
        SetWindowLongPtr(GetDlgItem(hwndDlg, id), GWL_STYLE, style);
        
        SendDlgItemMessage(hwndDlg, id, BM_SETCHECK, BST_INDETERMINATE, 0);
    }
}

static INT_PTR CALLBACK PropSheetDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
            BtrfsPropSheet* bps = (BtrfsPropSheet*)psp->lParam;
            WCHAR s[255];
            ULONG sr;
            int i;
            
            static ULONG perm_controls[] = { IDC_USERR, IDC_USERW, IDC_USERX, IDC_GROUPR, IDC_GROUPW, IDC_GROUPX, IDC_OTHERR, IDC_OTHERW, IDC_OTHERX, 0 };
            static ULONG perms[] = { S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH, 0 };
            
            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
            
            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)bps);
            
            if (bps->various_subvols) {
                if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
            } else {
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%llx", bps->subvol) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return FALSE;
            }
            
            SetDlgItemTextW(hwndDlg, IDC_SUBVOL, s);
            
            if (bps->various_inodes) {
                if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
            } else {
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%llx", bps->inode) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return FALSE;
            }
            
            SetDlgItemTextW(hwndDlg, IDC_INODE, s);
            
            if (bps->various_types)
                sr = IDS_VARIOUS;
            else
                sr = inode_type_to_string_ref(bps->type);
            
            if (bps->various_inodes) {
                if (sr == IDS_INODE_CHAR)
                    sr = IDS_INODE_CHAR_SIMPLE;
                else if (sr == IDS_INODE_BLOCK)
                    sr = IDS_INODE_BLOCK_SIMPLE;
            }
            
            if (sr == IDS_INODE_UNKNOWN) {
                WCHAR t[255];
                
                if (!LoadStringW(module, sr, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
                
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, bps->type) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return FALSE;
            } else if (sr == IDS_INODE_CHAR || sr == IDS_INODE_BLOCK) {
                WCHAR t[255];
                
                if (!LoadStringW(module, sr, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
                
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, (UINT64)((bps->rdev & 0xFFFFFFFFFFF) >> 20), (UINT32)(bps->rdev & 0xFFFFF)) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return FALSE;
            } else {
                if (!LoadStringW(module, sr, s, sizeof(s) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
            }
            
            SetDlgItemTextW(hwndDlg, IDC_TYPE, s);
            
            GetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, bps->size_format, sizeof(bps->size_format) / sizeof(WCHAR));
            bps->set_size_on_disk(hwndDlg);
            
            if (bps->thread)
                SetTimer(hwndDlg, 1, 250, NULL);
            
            set_check_box(hwndDlg, IDC_NODATACOW, bps->min_flags & BTRFS_INODE_NODATACOW, bps->max_flags & BTRFS_INODE_NODATACOW);
            set_check_box(hwndDlg, IDC_COMPRESS, bps->min_flags & BTRFS_INODE_COMPRESS, bps->max_flags & BTRFS_INODE_COMPRESS);
            
            i = 0;
            while (perm_controls[i] != 0) {
                set_check_box(hwndDlg, perm_controls[i], bps->min_mode & perms[i], bps->max_mode & perms[i]);
                i++;
            }
            
            if (bps->various_uids) {
                if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
                
                EnableWindow(GetDlgItem(hwndDlg, IDC_UID), 0);
            } else {
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%u", bps->uid) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return FALSE;
            }
            
            SetDlgItemTextW(hwndDlg, IDC_UID, s);
            
            if (bps->various_gids) {
                if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return FALSE;
                }
                
                EnableWindow(GetDlgItem(hwndDlg, IDC_GID), 0);
            } else {
                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%u", bps->gid) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return FALSE;
            }
            
            SetDlgItemTextW(hwndDlg, IDC_GID, s);
            
            if (!bps->can_change_nocow)
                EnableWindow(GetDlgItem(hwndDlg, IDC_NODATACOW), 0);
            
            if (!bps->can_change_owner) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_UID), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_GID), 0);
            }
            
            if (!bps->can_change_perms) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_USERR), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USERW), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USERX), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_GROUPR), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_GROUPW), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_GROUPX), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OTHERR), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OTHERW), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_OTHERX), 0);
            }
            
            if (bps->readonly) {
                EnableWindow(GetDlgItem(hwndDlg, IDC_NODATACOW), 0);
                EnableWindow(GetDlgItem(hwndDlg, IDC_COMPRESS), 0);
            }
            
            return FALSE;
        }
        
        case WM_COMMAND:
        {
            BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
            
            if (bps && !bps->readonly) {
                switch (HIWORD(wParam)) {
                    case BN_CLICKED: {
                        switch (LOWORD(wParam)) {
                            case IDC_NODATACOW:
                                bps->change_inode_flag(hwndDlg, BTRFS_INODE_NODATACOW, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_COMPRESS:
                                bps->change_inode_flag(hwndDlg, BTRFS_INODE_COMPRESS, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_USERR:
                                bps->change_perm_flag(hwndDlg, S_IRUSR, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_USERW:
                                bps->change_perm_flag(hwndDlg, S_IWUSR, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_USERX:
                                bps->change_perm_flag(hwndDlg, S_IXUSR, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_GROUPR:
                                bps->change_perm_flag(hwndDlg, S_IRGRP, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_GROUPW:
                                bps->change_perm_flag(hwndDlg, S_IWGRP, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_GROUPX:
                                bps->change_perm_flag(hwndDlg, S_IXGRP, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_OTHERR:
                                bps->change_perm_flag(hwndDlg, S_IROTH, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_OTHERW:
                                bps->change_perm_flag(hwndDlg, S_IWOTH, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                            
                            case IDC_OTHERX:
                                bps->change_perm_flag(hwndDlg, S_IXOTH, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) == BST_CHECKED);
                            break;
                        }
                    }
                    
                    case EN_CHANGE: {
                        switch (LOWORD(wParam)) {
                            case IDC_UID: {
                                WCHAR s[255];
                                
                                GetDlgItemTextW(hwndDlg, LOWORD(wParam), s, sizeof(s) / sizeof(WCHAR));
                                
                                bps->change_uid(hwndDlg, _wtoi(s));
                                break;
                            }
                            
                            case IDC_GID: {
                                WCHAR s[255];
                                
                                GetDlgItemTextW(hwndDlg, LOWORD(wParam), s, sizeof(s) / sizeof(WCHAR));
                                
                                bps->change_gid(hwndDlg, _wtoi(s));
                                break;
                            }
                        }
                    }
                }
            }
            
            break;
        }
        
        case WM_NOTIFY:
        {
            switch (((LPNMHDR)lParam)->code) {
                case PSN_KILLACTIVE:
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, FALSE);
                break;
                    
                case PSN_APPLY: {
                    BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
                    
                    bps->apply_changes(hwndDlg);
                    SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                    break;
                }
                
                case NM_CLICK:
                case NM_RETURN: {
                    if (((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwndDlg, IDC_SIZE_ON_DISK)) {
                        PNMLINK pNMLink = (PNMLINK)lParam;
                        
                        if (pNMLink->item.iLink == 0)
                            DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_SIZE_DETAILS), hwndDlg, SizeDetailsDlgProc, GetWindowLongPtr(hwndDlg, GWLP_USERDATA));
                    }
                    break;
                }
            }
        }
        
        case WM_TIMER:
        {
            BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
            
            if (bps) {
                bps->set_size_on_disk(hwndDlg);
                
                if (!bps->thread)
                    KillTimer(hwndDlg, 1);
            }
            
            break;
        }
    }
    
    return FALSE;
}

static void format_size(UINT64 size, WCHAR* s, ULONG len) {
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
    
    if (!LoadStringW(module, IDS_SIZE_BYTES, t, sizeof(t) / sizeof(WCHAR))) {
        ShowError(NULL, GetLastError());
        return;
    }
    
    if (StringCchPrintfW(bytes, sizeof(bytes) / sizeof(WCHAR), t, nb2) == STRSAFE_E_INSUFFICIENT_BUFFER) {
        ShowError(NULL, ERROR_INSUFFICIENT_BUFFER);
        return;
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
}

HRESULT __stdcall BtrfsPropSheet::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) {
    PROPSHEETPAGE psp;
    HPROPSHEETPAGE hPage;
    INITCOMMONCONTROLSEX icex;
    
    if (ignore)
        return S_OK;
    
    icex.dwSize = sizeof(icex);
    icex.dwICC = ICC_LINK_CLASS;
    
    if (!InitCommonControlsEx(&icex)) {
        MessageBoxW(NULL, L"InitCommonControlsEx failed", L"Error", MB_ICONERROR);
    }
    
    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE;
    psp.hInstance = module;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROP_SHEET);
    psp.hIcon = 0;
    psp.pszTitle = MAKEINTRESOURCE(IDS_PROP_SHEET_TITLE);
    psp.pfnDlgProc = (DLGPROC)PropSheetDlgProc;
    psp.pcRefParent = (UINT*)&objs_loaded;
    psp.pfnCallback = NULL;
    psp.lParam = (LPARAM)this;

    hPage = CreatePropertySheetPage(&psp);
            
    if (hPage) {
        if (pfnAddPage(hPage, lParam)) {
            this->AddRef();
            return S_OK;
        } else
            DestroyPropertySheetPage(hPage);
    } else
        return E_OUTOFMEMORY;
    
    return E_FAIL;
}

HRESULT __stdcall BtrfsPropSheet::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam) {
    return S_OK;
}
