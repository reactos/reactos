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

#define ISOLATION_AWARE_ENABLED 1
#define STRSAFE_NO_DEPRECATE

#include "shellext.h"
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
#include <ndk/iotypes.h>
#endif

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include <uxtheme.h>

#include "propsheet.h"
#include "resource.h"

#define SUBVOL_ROOT_INODE 0x100

#ifndef __REACTOS__
#ifndef __MINGW32__ // in winternl.h in mingw

typedef struct _FILE_ACCESS_INFORMATION {
    ACCESS_MASK AccessFlags;
} FILE_ACCESS_INFORMATION, *PFILE_ACCESS_INFORMATION;

#define FileAccessInformation (FILE_INFORMATION_CLASS)8

#endif
#endif

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

                    if (NT_SUCCESS(Status)) {
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

HRESULT BtrfsPropSheet::check_file(std::wstring fn, UINT i, UINT num_files, UINT* sv) {
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    FILE_ACCESS_INFORMATION fai;
    BY_HANDLE_FILE_INFORMATION bhfi;
    btrfs_inode_info bii2;

    h = CreateFileW(fn.c_str(), MAXIMUM_ALLOWED, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

    if (h == INVALID_HANDLE_VALUE)
        return E_FAIL;

    Status = NtQueryInformationFile(h, &iosb, &fai, sizeof(FILE_ACCESS_INFORMATION), FileAccessInformation);
    if (!NT_SUCCESS(Status)) {
        CloseHandle(h);
        return E_FAIL;
    }

    if (fai.AccessFlags & FILE_READ_ATTRIBUTES)
        can_change_perms = fai.AccessFlags & WRITE_DAC;

    readonly = !(fai.AccessFlags & FILE_WRITE_ATTRIBUTES);

    if (!readonly && num_files == 1 && !can_change_perms)
        show_admin_button = TRUE;

    if (GetFileInformationByHandle(h, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        add_to_search_list((WCHAR*)fn.c_str());

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii2, sizeof(btrfs_inode_info));

    if (NT_SUCCESS(Status) && !bii2.top) {
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
        min_compression_type = bii2.compression_type < min_compression_type ? bii2.compression_type : min_compression_type;
        max_compression_type = bii2.compression_type > max_compression_type ? bii2.compression_type : max_compression_type;

        if (bii2.inode == SUBVOL_ROOT_INODE) {
            BOOL ro = bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY;

            has_subvols = TRUE;

            if (*sv == 0)
                ro_subvol = ro;
            else {
                if (ro_subvol != ro)
                    various_ro = TRUE;
            }

            (*sv)++;
        }

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

    return S_OK;
}

HRESULT BtrfsPropSheet::load_file_list() {
    UINT num_files, i, sv = 0;
    WCHAR fn[MAX_PATH];

    num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);

    min_mode = 0;
    max_mode = 0;
    min_flags = 0;
    max_flags = 0;
    min_compression_type = 0xff;
    max_compression_type = 0;
    various_subvols = various_inodes = various_types = various_uids = various_gids = various_ro = FALSE;

    can_change_perms = TRUE;
    can_change_nocow = TRUE;

    sizes[0] = sizes[1] = sizes[2] = sizes[3] = 0;

    for (i = 0; i < num_files; i++) {
        if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
            HRESULT hr;

            hr = check_file(fn, i, num_files, &sv);
            if (FAILED(hr))
                return hr;
        } else
            return E_FAIL;
    }

    min_mode = ~min_mode;
    min_flags = ~min_flags;

    mode = min_mode;
    mode_set = ~(min_mode ^ max_mode);

    flags = min_flags;
    flags_set = ~(min_flags ^ max_flags);

    return S_OK;
}

HRESULT __stdcall BtrfsPropSheet::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    FORMATETC format = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HDROP hdrop;
    HRESULT hr;

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

    hr = load_file_list();
    if (FAILED(hr))
        return hr;

    if (search_list.size() > 0) {
        thread = CreateThread(NULL, 0, global_search_list_thread, this, 0, NULL);

        if (!thread)
            ShowError(NULL, GetLastError());
    }

    GlobalUnlock(hdrop);

    return S_OK;
}

void BtrfsPropSheet::set_cmdline(std::wstring cmdline) {
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    UINT sv = 0;
    BY_HANDLE_FILE_INFORMATION bhfi;
    btrfs_inode_info bii2;
    FILE_ACCESS_INFORMATION fai;

    min_mode = 0;
    max_mode = 0;
    min_flags = 0;
    max_flags = 0;
    min_compression_type = 0xff;
    max_compression_type = 0;
    various_subvols = various_inodes = various_types = various_uids = various_gids = various_ro = FALSE;

    can_change_perms = TRUE;
    can_change_nocow = TRUE;

    h = CreateFileW(cmdline.c_str(), MAXIMUM_ALLOWED, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

    if (h == INVALID_HANDLE_VALUE)
        return;

    Status = NtQueryInformationFile(h, &iosb, &fai, sizeof(FILE_ACCESS_INFORMATION), FileAccessInformation);
    if (!NT_SUCCESS(Status)) {
        CloseHandle(h);
        return;
    }

    if (fai.AccessFlags & FILE_READ_ATTRIBUTES)
        can_change_perms = fai.AccessFlags & WRITE_DAC;

    readonly = !(fai.AccessFlags & FILE_WRITE_ATTRIBUTES);

    if (GetFileInformationByHandle(h, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        add_to_search_list((WCHAR*)cmdline.c_str());

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii2, sizeof(btrfs_inode_info));

    if (NT_SUCCESS(Status) && !bii2.top) {
        int j;

        LARGE_INTEGER filesize;

        subvol = bii2.subvol;
        inode = bii2.inode;
        type = bii2.type;
        uid = bii2.st_uid;
        gid = bii2.st_gid;
        rdev = bii2.st_rdev;

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
        min_compression_type = bii2.compression_type < min_compression_type ? bii2.compression_type : min_compression_type;
        max_compression_type = bii2.compression_type > max_compression_type ? bii2.compression_type : max_compression_type;

        if (bii2.inode == SUBVOL_ROOT_INODE) {
            BOOL ro = bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY;

            has_subvols = TRUE;

            if (sv == 0)
                ro_subvol = ro;
            else {
                if (ro_subvol != ro)
                    various_ro = TRUE;
            }

            sv++;
        }

        ignore = FALSE;

        if (bii2.type != BTRFS_TYPE_DIRECTORY && GetFileSizeEx(h, &filesize)) {
            if (filesize.QuadPart != 0)
                can_change_nocow = FALSE;
        }

        CloseHandle(h);
    } else {
        CloseHandle(h);
        return;
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

    this->filename = cmdline;
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

    SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
}

void BtrfsPropSheet::apply_changes_file(HWND hDlg, std::wstring fn) {
    HANDLE h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    btrfs_set_inode_info bsii;
    btrfs_inode_info bii2;
    ULONG perms = FILE_TRAVERSE | FILE_READ_ATTRIBUTES;

    if (flags_changed || ro_changed)
        perms |= FILE_WRITE_ATTRIBUTES;

    if (perms_changed || gid_changed || uid_changed)
        perms |= WRITE_DAC;

    if (mode_set & S_ISUID && (((min_mode & S_ISUID) != (max_mode & S_ISUID)) || ((min_mode & S_ISUID) != (mode & S_ISUID))))
        perms |= WRITE_OWNER;

    h = CreateFileW(fn.c_str(), perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        ShowError(hDlg, GetLastError());
        return;
    }

    ZeroMemory(&bsii, sizeof(btrfs_set_inode_info));

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_INODE_INFO, NULL, 0, &bii2, sizeof(btrfs_inode_info));

    if (!NT_SUCCESS(Status)) {
        ShowNtStatusError(hDlg, Status);
        CloseHandle(h);
        return;
    }

    if (bii2.inode == SUBVOL_ROOT_INODE && ro_changed) {
        BY_HANDLE_FILE_INFORMATION bhfi;
        FILE_BASIC_INFO fbi;

        if (!GetFileInformationByHandle(h, &bhfi)) {
            ShowError(hDlg, GetLastError());
            return;
        }

        memset(&fbi, 0, sizeof(fbi));
        fbi.FileAttributes = bhfi.dwFileAttributes;

        if (ro_subvol)
            fbi.FileAttributes |= FILE_ATTRIBUTE_READONLY;
        else
            fbi.FileAttributes &= ~FILE_ATTRIBUTE_READONLY;

        if (!SetFileInformationByHandle(h, FileBasicInfo, &fbi, sizeof(fbi))) {
            CloseHandle(h);
            ShowError(hDlg, GetLastError());
            return;
        }
    }

    if (flags_changed || perms_changed || uid_changed || gid_changed || compress_type_changed) {
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

        if (compress_type_changed) {
            bsii.compression_type_changed = TRUE;
            bsii.compression_type = compress_type;
        }

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), NULL, 0);

        if (!NT_SUCCESS(Status)) {
            ShowNtStatusError(hDlg, Status);
            CloseHandle(h);
            return;
        }
    }

    CloseHandle(h);
}

void BtrfsPropSheet::apply_changes(HWND hDlg) {
    UINT num_files, i;
    WCHAR fn[MAX_PATH]; // FIXME - is this long enough?

    if (various_uids)
        uid_changed = FALSE;

    if (various_gids)
        gid_changed = FALSE;

    if (!flags_changed && !perms_changed && !uid_changed && !gid_changed && !compress_type_changed && !ro_changed)
        return;

    if (filename[0] != 0)
        apply_changes_file(hDlg, filename);
    else {
        num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);

        for (i = 0; i < num_files; i++) {
            if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
                apply_changes_file(hDlg, fn);
            }
        }
    }

    flags_changed = FALSE;
    perms_changed = FALSE;
    uid_changed = FALSE;
    gid_changed = FALSE;
    ro_changed = FALSE;
}

void BtrfsPropSheet::set_size_on_disk(HWND hwndDlg) {
    WCHAR size_on_disk[1024], s[1024], old_text[1024];

    format_size(totalsize, size_on_disk, sizeof(size_on_disk) / sizeof(WCHAR), TRUE);

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

    SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
}

void BtrfsPropSheet::change_uid(HWND hDlg, UINT32 uid) {
    if (this->uid != uid) {
        this->uid = uid;
        uid_changed = TRUE;

        SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
    }
}

void BtrfsPropSheet::change_gid(HWND hDlg, UINT32 gid) {
    if (this->gid != gid) {
        this->gid = gid;
        gid_changed = TRUE;

        SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
    }
}

void BtrfsPropSheet::update_size_details_dialog(HWND hDlg) {
    WCHAR size[1024], old_text[1024];
    int i;
    ULONG items[] = { IDC_SIZE_INLINE, IDC_SIZE_UNCOMPRESSED, IDC_SIZE_ZLIB, IDC_SIZE_LZO };

    for (i = 0; i < 4; i++) {
        format_size(sizes[i], size, sizeof(size) / sizeof(WCHAR), TRUE);

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

void BtrfsPropSheet::open_as_admin(HWND hwndDlg) {
    ULONG num_files, i;
    WCHAR fn[MAX_PATH];

    num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, NULL, 0);

    for (i = 0; i < num_files; i++) {
        if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(MAX_PATH))) {
            WCHAR t[MAX_PATH + 100];
            SHELLEXECUTEINFOW sei;

            t[0] = '"';
            GetModuleFileNameW(module, t + 1, (sizeof(t) / sizeof(WCHAR)) - 1);
            wcscat(t, L"\",ShowPropSheet ");
            wcscat(t, fn);

            RtlZeroMemory(&sei, sizeof(sei));

            sei.cbSize = sizeof(sei);
            sei.hwnd = hwndDlg;
            sei.lpVerb = L"runas";
            sei.lpFile = L"rundll32.exe";
            sei.lpParameters = t;
            sei.nShow = SW_SHOW;
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;

            if (!ShellExecuteExW(&sei)) {
                ShowError(hwndDlg, GetLastError());
                return;
            }

            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);

            load_file_list();
            init_propsheet(hwndDlg);
        }
    }
}

// based on functions in sys/sysmacros.h
#define major(rdev) ((((rdev) >> 8) & 0xFFF) | ((UINT32)((rdev) >> 32) & ~0xFFF))
#define minor(rdev) (((rdev) & 0xFF) | ((UINT32)((rdev) >> 12) & ~0xFF))

void BtrfsPropSheet::init_propsheet(HWND hwndDlg) {
    WCHAR s[255];
    ULONG sr;
    int i;
    HWND comptype;

    static ULONG perm_controls[] = { IDC_USERR, IDC_USERW, IDC_USERX, IDC_GROUPR, IDC_GROUPW, IDC_GROUPX, IDC_OTHERR, IDC_OTHERW, IDC_OTHERX,
                                     IDC_SETUID, IDC_SETGID, IDC_STICKY, 0 };
    static ULONG perms[] = { S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH, S_ISUID, S_ISGID, S_ISVTX, 0 };
    static ULONG comp_types[] = { IDS_COMPRESS_ANY, IDS_COMPRESS_ZLIB, IDS_COMPRESS_LZO, 0 };

    if (various_subvols) {
        if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }
    } else {
        if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%llx", subvol) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    }

    SetDlgItemTextW(hwndDlg, IDC_SUBVOL, s);

    if (various_inodes) {
        if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }
    } else {
        if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%llx", inode) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    }

    SetDlgItemTextW(hwndDlg, IDC_INODE, s);

    if (various_types)
        sr = IDS_VARIOUS;
    else
        sr = inode_type_to_string_ref(type);

    if (various_inodes) {
        if (sr == IDS_INODE_CHAR)
            sr = IDS_INODE_CHAR_SIMPLE;
        else if (sr == IDS_INODE_BLOCK)
            sr = IDS_INODE_BLOCK_SIMPLE;
    }

    if (sr == IDS_INODE_UNKNOWN) {
        WCHAR t[255];

        if (!LoadStringW(module, sr, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, type) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    } else if (sr == IDS_INODE_CHAR || sr == IDS_INODE_BLOCK) {
        WCHAR t[255];

        if (!LoadStringW(module, sr, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, major(rdev), minor(rdev)) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    } else {
        if (!LoadStringW(module, sr, s, sizeof(s) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }
    }

    SetDlgItemTextW(hwndDlg, IDC_TYPE, s);

    GetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, size_format, sizeof(size_format) / sizeof(WCHAR));
    set_size_on_disk(hwndDlg);

    if (thread)
        SetTimer(hwndDlg, 1, 250, NULL);

    set_check_box(hwndDlg, IDC_NODATACOW, min_flags & BTRFS_INODE_NODATACOW, max_flags & BTRFS_INODE_NODATACOW);
    set_check_box(hwndDlg, IDC_COMPRESS, min_flags & BTRFS_INODE_COMPRESS, max_flags & BTRFS_INODE_COMPRESS);

    comptype = GetDlgItem(hwndDlg, IDC_COMPRESS_TYPE);

    if (min_compression_type != max_compression_type) {
        SendMessage(comptype, CB_ADDSTRING, NULL, (LPARAM)L"");
        SendMessage(comptype, CB_SETCURSEL, 0, 0);
    }

    i = 0;
    while (comp_types[i] != 0) {
        WCHAR t[255];

        if (!LoadStringW(module, comp_types[i], t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        SendMessage(comptype, CB_ADDSTRING, NULL, (LPARAM)t);

        i++;
    }

    if (min_compression_type == max_compression_type) {
        SendMessage(comptype, CB_SETCURSEL, min_compression_type, 0);
        compress_type = min_compression_type;
    }

    EnableWindow(comptype, max_flags & BTRFS_INODE_COMPRESS);

    i = 0;
    while (perm_controls[i] != 0) {
        set_check_box(hwndDlg, perm_controls[i], min_mode & perms[i], max_mode & perms[i]);
        i++;
    }

    if (various_uids) {
        if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        EnableWindow(GetDlgItem(hwndDlg, IDC_UID), 0);
    } else {
        if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%u", uid) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    }

    SetDlgItemTextW(hwndDlg, IDC_UID, s);

    if (various_gids) {
        if (!LoadStringW(module, IDS_VARIOUS, s, sizeof(s) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        EnableWindow(GetDlgItem(hwndDlg, IDC_GID), 0);
    } else {
        if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), L"%u", gid) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    }

    SetDlgItemTextW(hwndDlg, IDC_GID, s);

    ShowWindow(GetDlgItem(hwndDlg, IDC_SUBVOL_RO), has_subvols);

    if (has_subvols)
        set_check_box(hwndDlg, IDC_SUBVOL_RO, ro_subvol, various_ro ? (!ro_subvol) : ro_subvol);

    if (!can_change_nocow)
        EnableWindow(GetDlgItem(hwndDlg, IDC_NODATACOW), 0);

    if (!can_change_perms) {
        i = 0;
        while (perm_controls[i] != 0) {
            EnableWindow(GetDlgItem(hwndDlg, perm_controls[i]), 0);
            i++;
        }

        EnableWindow(GetDlgItem(hwndDlg, IDC_UID), 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_GID), 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SETUID), 0);
    }

    if (readonly) {
        EnableWindow(GetDlgItem(hwndDlg, IDC_NODATACOW), 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_COMPRESS), 0);
        EnableWindow(GetDlgItem(hwndDlg, IDC_COMPRESS_TYPE), 0);
    }

    if (show_admin_button) {
        SendMessageW(GetDlgItem(hwndDlg, IDC_OPEN_ADMIN), BCM_SETSHIELD, 0, TRUE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_OPEN_ADMIN), SW_SHOW);
    } else
        ShowWindow(GetDlgItem(hwndDlg, IDC_OPEN_ADMIN), SW_HIDE);
}

static INT_PTR CALLBACK PropSheetDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            PROPSHEETPAGE* psp = (PROPSHEETPAGE*)lParam;
            BtrfsPropSheet* bps = (BtrfsPropSheet*)psp->lParam;

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

            SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)bps);

            bps->init_propsheet(hwndDlg);

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
                                bps->change_inode_flag(hwndDlg, BTRFS_INODE_NODATACOW, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_COMPRESS:
                                bps->change_inode_flag(hwndDlg, BTRFS_INODE_COMPRESS, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));

                                EnableWindow(GetDlgItem(hwndDlg, IDC_COMPRESS_TYPE), IsDlgButtonChecked(hwndDlg, LOWORD(wParam)) != BST_UNCHECKED);
                            break;

                            case IDC_USERR:
                                bps->change_perm_flag(hwndDlg, S_IRUSR, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_USERW:
                                bps->change_perm_flag(hwndDlg, S_IWUSR, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_USERX:
                                bps->change_perm_flag(hwndDlg, S_IXUSR, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_GROUPR:
                                bps->change_perm_flag(hwndDlg, S_IRGRP, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_GROUPW:
                                bps->change_perm_flag(hwndDlg, S_IWGRP, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_GROUPX:
                                bps->change_perm_flag(hwndDlg, S_IXGRP, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_OTHERR:
                                bps->change_perm_flag(hwndDlg, S_IROTH, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_OTHERW:
                                bps->change_perm_flag(hwndDlg, S_IWOTH, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_OTHERX:
                                bps->change_perm_flag(hwndDlg, S_IXOTH, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_SETUID:
                                bps->change_perm_flag(hwndDlg, S_ISUID, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_SETGID:
                                bps->change_perm_flag(hwndDlg, S_ISGID, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_STICKY:
                                bps->change_perm_flag(hwndDlg, S_ISVTX, IsDlgButtonChecked(hwndDlg, LOWORD(wParam)));
                            break;

                            case IDC_SUBVOL_RO:
                                switch (IsDlgButtonChecked(hwndDlg, LOWORD(wParam))) {
                                    case BST_CHECKED:
                                        bps->ro_subvol = TRUE;
                                        bps->ro_changed = TRUE;
                                    break;

                                    case BST_UNCHECKED:
                                        bps->ro_subvol = FALSE;
                                        bps->ro_changed = TRUE;
                                    break;

                                    case BST_INDETERMINATE:
                                        bps->ro_changed = FALSE;
                                    break;
                                }

                                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);
                            break;

                            case IDC_OPEN_ADMIN:
                                bps->open_as_admin(hwndDlg);
                            break;
                        }

                        break;
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

                        break;
                    }

                    case CBN_SELCHANGE: {
                        switch (LOWORD(wParam)) {
                            case IDC_COMPRESS_TYPE: {
                                int sel = SendMessageW(GetDlgItem(hwndDlg, LOWORD(wParam)), CB_GETCURSEL, 0, 0);

                                if (bps->min_compression_type != bps->max_compression_type) {
                                    if (sel == 0)
                                        bps->compress_type_changed = FALSE;
                                    else {
                                        bps->compress_type = sel - 1;
                                        bps->compress_type_changed = TRUE;
                                    }
                                } else {
                                    bps->compress_type = sel;
                                    bps->compress_type_changed = TRUE;
                                }

                                SendMessageW(GetParent(hwndDlg), PSM_CHANGED, (WPARAM)hwndDlg, 0);

                                break;
                            }
                        }

                        break;
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

#ifdef __cplusplus
extern "C" {
#endif

void CALLBACK ShowPropSheetW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    BtrfsPropSheet* bps;
    PROPSHEETPAGEW psp;
    PROPSHEETHEADERW psh;
    WCHAR title[255];

    set_dpi_aware();

    LoadStringW(module, IDS_STANDALONE_PROPSHEET_TITLE, title, sizeof(title) / sizeof(WCHAR));

    bps = new BtrfsPropSheet;
    bps->set_cmdline(lpszCmdLine);

    psp.dwSize = sizeof(psp);
    psp.dwFlags = PSP_USETITLE;
    psp.hInstance = module;
    psp.pszTemplate = MAKEINTRESOURCEW(IDD_PROP_SHEET);
    psp.hIcon = 0;
    psp.pszTitle = MAKEINTRESOURCEW(IDS_PROP_SHEET_TITLE);
    psp.pfnDlgProc = (DLGPROC)PropSheetDlgProc;
    psp.pfnCallback = NULL;
    psp.lParam = (LPARAM)bps;

    memset(&psh, 0, sizeof(PROPSHEETHEADERW));

    psh.dwSize = sizeof(PROPSHEETHEADERW);
    psh.dwFlags = PSH_PROPSHEETPAGE;
    psh.hwndParent = hwnd;
    psh.hInstance = psp.hInstance;
    psh.pszCaption = title;
    psh.nPages = 1;
    psh.ppsp = &psp;

    PropertySheetW(&psh);
}

#ifdef __cplusplus
}
#endif
