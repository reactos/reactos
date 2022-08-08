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

typedef struct _FILE_STANDARD_INFORMATION {
    LARGE_INTEGER AllocationSize;
    LARGE_INTEGER EndOfFile;
    ULONG         NumberOfLinks;
    BOOLEAN       DeletePending;
    BOOLEAN       Directory;
} FILE_STANDARD_INFORMATION, *PFILE_STANDARD_INFORMATION;

#define FileStandardInformation (FILE_INFORMATION_CLASS)5

typedef struct _FILE_FS_SIZE_INFORMATION {
    LARGE_INTEGER TotalAllocationUnits;
    LARGE_INTEGER AvailableAllocationUnits;
    ULONG SectorsPerAllocationUnit;
    ULONG BytesPerSector;
} FILE_FS_SIZE_INFORMATION, *PFILE_FS_SIZE_INFORMATION;

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

    *ppObj = nullptr;
    return E_NOINTERFACE;
}

void BtrfsPropSheet::do_search(const wstring& fn) {
    wstring ss;
    WIN32_FIND_DATAW ffd;

#ifndef __REACTOS__
    ss = fn + L"\\*"s;
#else
    ss = fn + wstring(L"\\*");
#endif

    fff_handle h = FindFirstFileW(ss.c_str(), &ffd);
    if (h == INVALID_HANDLE_VALUE)
        return;

    do {
        if (ffd.cFileName[0] != '.' || ((ffd.cFileName[1] != 0) && (ffd.cFileName[1] != '.' || ffd.cFileName[2] != 0))) {
            wstring fn2;

#ifndef __REACTOS__
            fn2 = fn + L"\\"s + ffd.cFileName;
#else
            fn2 = fn + wstring(L"\\") + ffd.cFileName;
#endif

            if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
                search_list.push_back(fn2);
            else {
                win_handle fh = CreateFileW(fn2.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

                if (fh != INVALID_HANDLE_VALUE) {
                    NTSTATUS Status;
                    IO_STATUS_BLOCK iosb;
                    btrfs_inode_info bii2;

                    memset(&bii2, 0, sizeof(bii2));

                    Status = NtFsControlFile(fh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_INODE_INFO, nullptr, 0, &bii2, sizeof(btrfs_inode_info));

                    if (NT_SUCCESS(Status)) {
                        sizes[0] += bii2.inline_length;
                        sizes[1] += bii2.disk_size_uncompressed;
                        sizes[2] += bii2.disk_size_zlib;
                        sizes[3] += bii2.disk_size_lzo;
                        sizes[4] += bii2.disk_size_zstd;
                        totalsize += bii2.inline_length + bii2.disk_size_uncompressed + bii2.disk_size_zlib + bii2.disk_size_lzo + bii2.disk_size_zstd;
                        sparsesize += bii2.sparse_size;
                        num_extents += bii2.num_extents == 0 ? 0 : (bii2.num_extents - 1);
                    }

                    FILE_STANDARD_INFORMATION fsi;

                    Status = NtQueryInformationFile(fh, &iosb, &fsi, sizeof(fsi), FileStandardInformation);

                    if (NT_SUCCESS(Status)) {
                        if (bii2.inline_length > 0)
                            allocsize += fsi.EndOfFile.QuadPart;
                        else
                            allocsize += fsi.AllocationSize.QuadPart;
                    }
                }
            }
        }
    } while (FindNextFileW(h, &ffd));
}

DWORD BtrfsPropSheet::search_list_thread() {
    while (!search_list.empty()) {
        do_search(search_list.front());

        search_list.pop_front();
    }

    thread = nullptr;

    return 0;
}

static DWORD WINAPI global_search_list_thread(LPVOID lpParameter) {
    BtrfsPropSheet* bps = (BtrfsPropSheet*)lpParameter;

    return bps->search_list_thread();
}

HRESULT BtrfsPropSheet::check_file(const wstring& fn, UINT i, UINT num_files, UINT* sv) {
    win_handle h;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    FILE_ACCESS_INFORMATION fai;
    BY_HANDLE_FILE_INFORMATION bhfi;
    btrfs_inode_info bii2;

    h = CreateFileW(fn.c_str(), MAXIMUM_ALLOWED, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        return E_FAIL;

    Status = NtQueryInformationFile(h, &iosb, &fai, sizeof(FILE_ACCESS_INFORMATION), FileAccessInformation);
    if (!NT_SUCCESS(Status))
        return E_FAIL;

    if (fai.AccessFlags & FILE_READ_ATTRIBUTES)
        can_change_perms = fai.AccessFlags & WRITE_DAC;

    readonly = !(fai.AccessFlags & FILE_WRITE_ATTRIBUTES);

    if (!readonly && num_files == 1 && !can_change_perms)
        show_admin_button = true;

    if (GetFileInformationByHandle(h, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        search_list.push_back(fn);

    memset(&bii2, 0, sizeof(bii2));

    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_INODE_INFO, nullptr, 0, &bii2, sizeof(btrfs_inode_info));

    if (NT_SUCCESS(Status) && !bii2.top) {
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
                various_subvols = true;

            if (inode != bii2.inode)
                various_inodes = true;

            if (type != bii2.type)
                various_types = true;

            if (uid != bii2.st_uid)
                various_uids = true;

            if (gid != bii2.st_gid)
                various_gids = true;
        }

        if (bii2.inline_length > 0) {
            totalsize += bii2.inline_length;
            sizes[0] += bii2.inline_length;
        }

        if (bii2.disk_size_uncompressed > 0) {
            totalsize += bii2.disk_size_uncompressed;
            sizes[1] += bii2.disk_size_uncompressed;
        }

        if (bii2.disk_size_zlib > 0) {
            totalsize += bii2.disk_size_zlib;
            sizes[2] += bii2.disk_size_zlib;
        }

        if (bii2.disk_size_lzo > 0) {
            totalsize += bii2.disk_size_lzo;
            sizes[3] += bii2.disk_size_lzo;
        }

        if (bii2.disk_size_zstd > 0) {
            totalsize += bii2.disk_size_zstd;
            sizes[4] += bii2.disk_size_zstd;
        }

        sparsesize += bii2.sparse_size;
        num_extents += bii2.num_extents == 0 ? 0 : (bii2.num_extents - 1);

        FILE_STANDARD_INFORMATION fsi;

        Status = NtQueryInformationFile(h, &iosb, &fsi, sizeof(fsi), FileStandardInformation);

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        if (bii2.inline_length > 0)
            allocsize += fsi.EndOfFile.QuadPart;
        else
            allocsize += fsi.AllocationSize.QuadPart;

        min_mode |= ~bii2.st_mode;
        max_mode |= bii2.st_mode;
        min_flags |= ~bii2.flags;
        max_flags |= bii2.flags;
        min_compression_type = bii2.compression_type < min_compression_type ? bii2.compression_type : min_compression_type;
        max_compression_type = bii2.compression_type > max_compression_type ? bii2.compression_type : max_compression_type;

        if (bii2.inode == SUBVOL_ROOT_INODE) {
            bool ro = bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY;

            has_subvols = true;

            if (*sv == 0)
                ro_subvol = ro;
            else {
                if (ro_subvol != ro)
                    various_ro = true;
            }

            (*sv)++;
        }

        ignore = false;

        if (bii2.type != BTRFS_TYPE_DIRECTORY && GetFileSizeEx(h, &filesize)) {
            if (filesize.QuadPart != 0)
                can_change_nocow = false;
        }

        {
            FILE_FS_SIZE_INFORMATION ffsi;

            Status = NtQueryVolumeInformationFile(h, &iosb, &ffsi, sizeof(ffsi), FileFsSizeInformation);

            if (NT_SUCCESS(Status))
                sector_size = ffsi.BytesPerSector;

            if (sector_size == 0)
                sector_size = 4096;
        }
    } else
        return E_FAIL;

    return S_OK;
}

HRESULT BtrfsPropSheet::load_file_list() {
    UINT num_files, i, sv = 0;
    WCHAR fn[MAX_PATH];

    num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

    min_mode = 0;
    max_mode = 0;
    min_flags = 0;
    max_flags = 0;
    min_compression_type = 0xff;
    max_compression_type = 0;
    various_subvols = various_inodes = various_types = various_uids = various_gids = various_ro = false;

    can_change_perms = true;
    can_change_nocow = true;

    sizes[0] = sizes[1] = sizes[2] = sizes[3] = sizes[4] = 0;
    totalsize = allocsize = sparsesize = 0;

    for (i = 0; i < num_files; i++) {
        if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
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
    try {
        FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
        HDROP hdrop;
        HRESULT hr;

        if (pidlFolder)
            return E_FAIL;

        if (!pdtobj)
            return E_FAIL;

        stgm.tymed = TYMED_HGLOBAL;

        if (FAILED(pdtobj->GetData(&format, &stgm)))
            return E_INVALIDARG;

        stgm_set = true;

        hdrop = (HDROP)GlobalLock(stgm.hGlobal);

        if (!hdrop) {
            ReleaseStgMedium(&stgm);
            stgm_set = false;
            return E_INVALIDARG;
        }

        try {
            hr = load_file_list();
            if (FAILED(hr))
                return hr;

            if (search_list.size() > 0) {
                thread = CreateThread(nullptr, 0, global_search_list_thread, this, 0, nullptr);

                if (!thread)
                    throw last_error(GetLastError());
            }
        } catch (...) {
            GlobalUnlock(hdrop);
            throw;
        }

        GlobalUnlock(hdrop);
    } catch (const exception& e) {
        error_message(nullptr, e.what());

        return E_FAIL;
    }

    return S_OK;
}

void BtrfsPropSheet::set_cmdline(const wstring& cmdline) {
    win_handle h;
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
    various_subvols = various_inodes = various_types = various_uids = various_gids = various_ro = false;

    can_change_perms = true;
    can_change_nocow = true;

    h = CreateFileW(cmdline.c_str(), MAXIMUM_ALLOWED, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        throw last_error(GetLastError());

    Status = NtQueryInformationFile(h, &iosb, &fai, sizeof(FILE_ACCESS_INFORMATION), FileAccessInformation);
    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    if (fai.AccessFlags & FILE_READ_ATTRIBUTES)
        can_change_perms = fai.AccessFlags & WRITE_DAC;

    readonly = !(fai.AccessFlags & FILE_WRITE_ATTRIBUTES);

    if (GetFileInformationByHandle(h, &bhfi) && bhfi.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        search_list.push_back(cmdline);

    memset(&bii2, 0, sizeof(bii2));

    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_INODE_INFO, nullptr, 0, &bii2, sizeof(btrfs_inode_info));

    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    if (!bii2.top) {
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

        if (bii2.disk_size_uncompressed > 0) {
            totalsize += bii2.disk_size_uncompressed;
            sizes[1] += bii2.disk_size_uncompressed;
        }

        if (bii2.disk_size_zlib > 0) {
            totalsize += bii2.disk_size_zlib;
            sizes[2] += bii2.disk_size_zlib;
        }

        if (bii2.disk_size_lzo > 0) {
            totalsize += bii2.disk_size_lzo;
            sizes[3] += bii2.disk_size_lzo;
        }

        if (bii2.disk_size_zstd > 0) {
            totalsize += bii2.disk_size_zstd;
            sizes[4] += bii2.disk_size_zstd;
        }

        sparsesize += bii2.sparse_size;

        FILE_STANDARD_INFORMATION fsi;

        Status = NtQueryInformationFile(h, &iosb, &fsi, sizeof(fsi), FileStandardInformation);

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        if (bii2.inline_length > 0)
            allocsize += fsi.EndOfFile.QuadPart;
        else
            allocsize += fsi.AllocationSize.QuadPart;

        min_mode |= ~bii2.st_mode;
        max_mode |= bii2.st_mode;
        min_flags |= ~bii2.flags;
        max_flags |= bii2.flags;
        min_compression_type = bii2.compression_type < min_compression_type ? bii2.compression_type : min_compression_type;
        max_compression_type = bii2.compression_type > max_compression_type ? bii2.compression_type : max_compression_type;

        if (bii2.inode == SUBVOL_ROOT_INODE) {
            bool ro = bhfi.dwFileAttributes & FILE_ATTRIBUTE_READONLY;

            has_subvols = true;

            if (sv == 0)
                ro_subvol = ro;
            else {
                if (ro_subvol != ro)
                    various_ro = true;
            }

            sv++;
        }

        ignore = false;

        if (bii2.type != BTRFS_TYPE_DIRECTORY && GetFileSizeEx(h, &filesize)) {
            if (filesize.QuadPart != 0)
                can_change_nocow = false;
        }
    } else
        return;

    min_mode = ~min_mode;
    min_flags = ~min_flags;

    mode = min_mode;
    mode_set = ~(min_mode ^ max_mode);

    flags = min_flags;
    flags_set = ~(min_flags ^ max_flags);

    if (search_list.size() > 0) {
        thread = CreateThread(nullptr, 0, global_search_list_thread, this, 0, nullptr);

        if (!thread)
            throw last_error(GetLastError());
    }

    this->filename = cmdline;
}

static ULONG inode_type_to_string_ref(uint8_t type) {
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

void BtrfsPropSheet::change_inode_flag(HWND hDlg, uint64_t flag, UINT state) {
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

    flags_changed = true;

    SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
}

void BtrfsPropSheet::apply_changes_file(HWND hDlg, const wstring& fn) {
    win_handle h;
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

    h = CreateFileW(fn.c_str(), perms, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        throw last_error(GetLastError());

    ZeroMemory(&bsii, sizeof(btrfs_set_inode_info));

    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_INODE_INFO, nullptr, 0, &bii2, sizeof(btrfs_inode_info));

    if (!NT_SUCCESS(Status))
        throw ntstatus_error(Status);

    if (bii2.inode == SUBVOL_ROOT_INODE && ro_changed) {
        BY_HANDLE_FILE_INFORMATION bhfi;
        FILE_BASIC_INFO fbi;

        if (!GetFileInformationByHandle(h, &bhfi))
            throw last_error(GetLastError());

        memset(&fbi, 0, sizeof(fbi));
        fbi.FileAttributes = bhfi.dwFileAttributes;

        if (ro_subvol)
            fbi.FileAttributes |= FILE_ATTRIBUTE_READONLY;
        else
            fbi.FileAttributes &= ~FILE_ATTRIBUTE_READONLY;

        Status = NtSetInformationFile(h, &iosb, &fbi, sizeof(FILE_BASIC_INFO), FileBasicInformation);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    }

    if (flags_changed || perms_changed || uid_changed || gid_changed || compress_type_changed) {
        if (flags_changed) {
            bsii.flags_changed = true;
            bsii.flags = (bii2.flags & ~flags_set) | (flags & flags_set);
        }

        if (perms_changed) {
            bsii.mode_changed = true;
            bsii.st_mode = (bii2.st_mode & ~mode_set) | (mode & mode_set);
        }

        if (uid_changed) {
            bsii.uid_changed = true;
            bsii.st_uid = uid;
        }

        if (gid_changed) {
            bsii.gid_changed = true;
            bsii.st_gid = gid;
        }

        if (compress_type_changed) {
            bsii.compression_type_changed = true;
            bsii.compression_type = compress_type;
        }

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SET_INODE_INFO, &bsii, sizeof(btrfs_set_inode_info), nullptr, 0);

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    }
}

void BtrfsPropSheet::apply_changes(HWND hDlg) {
    UINT num_files, i;
    WCHAR fn[MAX_PATH]; // FIXME - is this long enough?

    if (various_uids)
        uid_changed = false;

    if (various_gids)
        gid_changed = false;

    if (!flags_changed && !perms_changed && !uid_changed && !gid_changed && !compress_type_changed && !ro_changed)
        return;

    if (filename[0] != 0)
        apply_changes_file(hDlg, filename);
    else {
        num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

        for (i = 0; i < num_files; i++) {
            if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
                apply_changes_file(hDlg, fn);
            }
        }
    }

    flags_changed = false;
    perms_changed = false;
    uid_changed = false;
    gid_changed = false;
    ro_changed = false;
}

void BtrfsPropSheet::set_size_on_disk(HWND hwndDlg) {
    wstring s, size_on_disk, cr, frag;
    WCHAR old_text[1024];
    float ratio;

    format_size(totalsize, size_on_disk, true);

    wstring_sprintf(s, size_format, size_on_disk.c_str());

    if (allocsize == sparsesize || totalsize == 0)
        ratio = 0.0f;
    else
        ratio = 100.0f * (1.0f - ((float)totalsize / (float)(allocsize - sparsesize)));

    wstring_sprintf(cr, cr_format, ratio);

    GetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, old_text, sizeof(old_text) / sizeof(WCHAR));

    if (s != old_text)
        SetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, s.c_str());

    GetDlgItemTextW(hwndDlg, IDC_COMPRESSION_RATIO, old_text, sizeof(old_text) / sizeof(WCHAR));

    if (cr != old_text)
        SetDlgItemTextW(hwndDlg, IDC_COMPRESSION_RATIO, cr.c_str());

    uint64_t extent_size = (allocsize - sparsesize - sizes[0]) / (sector_size == 0 ? 4096 : sector_size);

    if (num_extents == 0 || extent_size <= 1)
        ratio = 0.0f;
    else
        ratio = 100.0f * ((float)num_extents / (float)(extent_size - 1));

    wstring_sprintf(frag, frag_format, ratio);

    GetDlgItemTextW(hwndDlg, IDC_FRAGMENTATION, old_text, sizeof(old_text) / sizeof(WCHAR));

    if (frag != old_text)
        SetDlgItemTextW(hwndDlg, IDC_FRAGMENTATION, frag.c_str());
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

    perms_changed = true;

    SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
}

void BtrfsPropSheet::change_uid(HWND hDlg, uint32_t uid) {
    if (this->uid != uid) {
        this->uid = uid;
        uid_changed = true;

        SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
    }
}

void BtrfsPropSheet::change_gid(HWND hDlg, uint32_t gid) {
    if (this->gid != gid) {
        this->gid = gid;
        gid_changed = true;

        SendMessageW(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0);
    }
}

void BtrfsPropSheet::update_size_details_dialog(HWND hDlg) {
    wstring size;
    WCHAR old_text[1024];
    int i;
    ULONG items[] = { IDC_SIZE_INLINE, IDC_SIZE_UNCOMPRESSED, IDC_SIZE_ZLIB, IDC_SIZE_LZO, IDC_SIZE_ZSTD };

    for (i = 0; i < 5; i++) {
        format_size(sizes[i], size, true);

        GetDlgItemTextW(hDlg, items[i], old_text, sizeof(old_text) / sizeof(WCHAR));

        if (size != old_text)
            SetDlgItemTextW(hDlg, items[i], size.c_str());
    }
}

static INT_PTR CALLBACK SizeDetailsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                BtrfsPropSheet* bps = (BtrfsPropSheet*)lParam;

                SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)bps);

                bps->update_size_details_dialog(hwndDlg);

                if (bps->thread)
                    SetTimer(hwndDlg, 1, 250, nullptr);

                return true;
            }

            case WM_COMMAND:
                if (HIWORD(wParam) == BN_CLICKED && (LOWORD(wParam) == IDOK || LOWORD(wParam) == IDCANCEL)) {
                    EndDialog(hwndDlg, 0);
                    return true;
                }
            break;

            case WM_TIMER:
            {
                BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

                if (bps) {
                    bps->update_size_details_dialog(hwndDlg);

                    if (!bps->thread)
                        KillTimer(hwndDlg, 1);
                }

                break;
            }
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static void set_check_box(HWND hwndDlg, ULONG id, uint64_t min, uint64_t max) {
    if (min && max) {
        SendDlgItemMessageW(hwndDlg, id, BM_SETCHECK, BST_CHECKED, 0);
    } else if (!min && !max) {
        SendDlgItemMessageW(hwndDlg, id, BM_SETCHECK, BST_UNCHECKED, 0);
    } else {
        LONG_PTR style;

        style = GetWindowLongPtrW(GetDlgItem(hwndDlg, id), GWL_STYLE);
        style &= ~BS_AUTOCHECKBOX;
        style |= BS_AUTO3STATE;
        SetWindowLongPtrW(GetDlgItem(hwndDlg, id), GWL_STYLE, style);

        SendDlgItemMessageW(hwndDlg, id, BM_SETCHECK, BST_INDETERMINATE, 0);
    }
}

void BtrfsPropSheet::open_as_admin(HWND hwndDlg) {
    ULONG num_files, i;
    WCHAR fn[MAX_PATH], modfn[MAX_PATH];

    num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

    if (num_files == 0)
        return;

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

    for (i = 0; i < num_files; i++) {
        if (DragQueryFileW((HDROP)stgm.hGlobal, i, fn, sizeof(fn) / sizeof(WCHAR))) {
            wstring t;
            SHELLEXECUTEINFOW sei;

#ifndef __REACTOS__
            t = L"\""s + modfn + L"\",ShowPropSheet "s + fn;
#else
            t = wstring(L"\"") + modfn + wstring(L"\",ShowPropSheet ") + fn;
#endif

            RtlZeroMemory(&sei, sizeof(sei));

            sei.cbSize = sizeof(sei);
            sei.hwnd = hwndDlg;
            sei.lpVerb = L"runas";
            sei.lpFile = L"rundll32.exe";
            sei.lpParameters = t.c_str();
            sei.nShow = SW_SHOW;
            sei.fMask = SEE_MASK_NOCLOSEPROCESS;

            if (!ShellExecuteExW(&sei))
                throw last_error(GetLastError());

            WaitForSingleObject(sei.hProcess, INFINITE);
            CloseHandle(sei.hProcess);

            load_file_list();
            init_propsheet(hwndDlg);
        }
    }
}

// based on functions in sys/sysmacros.h
#define major(rdev) ((((rdev) >> 8) & 0xFFF) | ((uint32_t)((rdev) >> 32) & ~0xFFF))
#define minor(rdev) (((rdev) & 0xFF) | ((uint32_t)((rdev) >> 12) & ~0xFF))

void BtrfsPropSheet::init_propsheet(HWND hwndDlg) {
    wstring s;
    ULONG sr;
    int i;
    HWND comptype;

    static ULONG perm_controls[] = { IDC_USERR, IDC_USERW, IDC_USERX, IDC_GROUPR, IDC_GROUPW, IDC_GROUPX, IDC_OTHERR, IDC_OTHERW, IDC_OTHERX,
                                     IDC_SETUID, IDC_SETGID, IDC_STICKY, 0 };
    static ULONG perms[] = { S_IRUSR, S_IWUSR, S_IXUSR, S_IRGRP, S_IWGRP, S_IXGRP, S_IROTH, S_IWOTH, S_IXOTH, S_ISUID, S_ISGID, S_ISVTX, 0 };
    static ULONG comp_types[] = { IDS_COMPRESS_ANY, IDS_COMPRESS_ZLIB, IDS_COMPRESS_LZO, IDS_COMPRESS_ZSTD, 0 };

    if (various_subvols) {
        if (!load_string(module, IDS_VARIOUS, s))
            throw last_error(GetLastError());
    } else
        wstring_sprintf(s, L"%llx", subvol);

    SetDlgItemTextW(hwndDlg, IDC_SUBVOL, s.c_str());

    if (various_inodes) {
        if (!load_string(module, IDS_VARIOUS, s))
            throw last_error(GetLastError());
    } else
        wstring_sprintf(s, L"%llx", inode);

    SetDlgItemTextW(hwndDlg, IDC_INODE, s.c_str());

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
        wstring t;

        if (!load_string(module, sr, t))
            throw last_error(GetLastError());

        wstring_sprintf(s, t, type);
    } else if (sr == IDS_INODE_CHAR || sr == IDS_INODE_BLOCK) {
        wstring t;

        if (!load_string(module, sr, t))
            throw last_error(GetLastError());

        wstring_sprintf(s, t, major(rdev), minor(rdev));
    } else {
        if (!load_string(module, sr, s))
            throw last_error(GetLastError());
    }

    SetDlgItemTextW(hwndDlg, IDC_TYPE, s.c_str());

    if (size_format[0] == 0)
        GetDlgItemTextW(hwndDlg, IDC_SIZE_ON_DISK, size_format, sizeof(size_format) / sizeof(WCHAR));

    if (cr_format[0] == 0)
        GetDlgItemTextW(hwndDlg, IDC_COMPRESSION_RATIO, cr_format, sizeof(cr_format) / sizeof(WCHAR));

    if (frag_format[0] == 0)
        GetDlgItemTextW(hwndDlg, IDC_FRAGMENTATION, frag_format, sizeof(frag_format) / sizeof(WCHAR));

    set_size_on_disk(hwndDlg);

    if (thread)
        SetTimer(hwndDlg, 1, 250, nullptr);

    set_check_box(hwndDlg, IDC_NODATACOW, min_flags & BTRFS_INODE_NODATACOW, max_flags & BTRFS_INODE_NODATACOW);
    set_check_box(hwndDlg, IDC_COMPRESS, min_flags & BTRFS_INODE_COMPRESS, max_flags & BTRFS_INODE_COMPRESS);

    comptype = GetDlgItem(hwndDlg, IDC_COMPRESS_TYPE);

    while (SendMessageW(comptype, CB_GETCOUNT, 0, 0) > 0) {
        SendMessageW(comptype, CB_DELETESTRING, 0, 0);
    }

    if (min_compression_type != max_compression_type) {
        SendMessageW(comptype, CB_ADDSTRING, 0, (LPARAM)L"");
        SendMessageW(comptype, CB_SETCURSEL, 0, 0);
    }

    i = 0;
    while (comp_types[i] != 0) {
        wstring t;

        if (!load_string(module, comp_types[i], t))
            throw last_error(GetLastError());

        SendMessageW(comptype, CB_ADDSTRING, 0, (LPARAM)t.c_str());

        i++;
    }

    if (min_compression_type == max_compression_type) {
        SendMessageW(comptype, CB_SETCURSEL, min_compression_type, 0);
        compress_type = min_compression_type;
    }

    EnableWindow(comptype, max_flags & BTRFS_INODE_COMPRESS);

    i = 0;
    while (perm_controls[i] != 0) {
        set_check_box(hwndDlg, perm_controls[i], min_mode & perms[i], max_mode & perms[i]);
        i++;
    }

    if (various_uids) {
        if (!load_string(module, IDS_VARIOUS, s))
            throw last_error(GetLastError());

        EnableWindow(GetDlgItem(hwndDlg, IDC_UID), 0);
    } else
        s = to_wstring(uid);

    SetDlgItemTextW(hwndDlg, IDC_UID, s.c_str());

    if (various_gids) {
        if (!load_string(module, IDS_VARIOUS, s))
            throw last_error(GetLastError());

        EnableWindow(GetDlgItem(hwndDlg, IDC_GID), 0);
    } else
        s = to_wstring(gid);

    SetDlgItemTextW(hwndDlg, IDC_GID, s.c_str());

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
        SendMessageW(GetDlgItem(hwndDlg, IDC_OPEN_ADMIN), BCM_SETSHIELD, 0, true);
        ShowWindow(GetDlgItem(hwndDlg, IDC_OPEN_ADMIN), SW_SHOW);
    } else
        ShowWindow(GetDlgItem(hwndDlg, IDC_OPEN_ADMIN), SW_HIDE);
}

static INT_PTR CALLBACK PropSheetDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                PROPSHEETPAGEW* psp = (PROPSHEETPAGEW*)lParam;
                BtrfsPropSheet* bps = (BtrfsPropSheet*)psp->lParam;

                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)bps);

                bps->init_propsheet(hwndDlg);

                return false;
            }

            case WM_COMMAND:
            {
                BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

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
                                            bps->ro_subvol = true;
                                            bps->ro_changed = true;
                                        break;

                                        case BST_UNCHECKED:
                                            bps->ro_subvol = false;
                                            bps->ro_changed = true;
                                        break;

                                        case BST_INDETERMINATE:
                                            bps->ro_changed = false;
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
                                    auto sel = SendMessageW(GetDlgItem(hwndDlg, LOWORD(wParam)), CB_GETCURSEL, 0, 0);

                                    if (bps->min_compression_type != bps->max_compression_type) {
                                        if (sel == 0)
                                            bps->compress_type_changed = false;
                                        else {
                                            bps->compress_type = (uint8_t)(sel - 1);
                                            bps->compress_type_changed = true;
                                        }
                                    } else {
                                        bps->compress_type = (uint8_t)sel;
                                        bps->compress_type_changed = true;
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
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, false);
                    break;

                    case PSN_APPLY: {
                        BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

                        bps->apply_changes(hwndDlg);
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, PSNRET_NOERROR);
                        break;
                    }

                    case NM_CLICK:
                    case NM_RETURN: {
                        if (((LPNMHDR)lParam)->hwndFrom == GetDlgItem(hwndDlg, IDC_SIZE_ON_DISK)) {
                            PNMLINK pNMLink = (PNMLINK)lParam;

                            if (pNMLink->item.iLink == 0)
                                DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_SIZE_DETAILS), hwndDlg, SizeDetailsDlgProc, GetWindowLongPtrW(hwndDlg, GWLP_USERDATA));
                        }
                        break;
                    }
                }
            }

            case WM_TIMER:
            {
                BtrfsPropSheet* bps = (BtrfsPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

                if (bps) {
                    bps->set_size_on_disk(hwndDlg);

                    if (!bps->thread)
                        KillTimer(hwndDlg, 1);
                }

                break;
            }
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

HRESULT __stdcall BtrfsPropSheet::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) {
    try {
        PROPSHEETPAGEW psp;
        HPROPSHEETPAGE hPage;
        INITCOMMONCONTROLSEX icex;

        if (ignore)
            return S_OK;

        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_LINK_CLASS;

        if (!InitCommonControlsEx(&icex))
            throw string_error(IDS_INITCOMMONCONTROLSEX_FAILED);

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_USEREFPARENT | PSP_USETITLE;
        psp.hInstance = module;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_PROP_SHEET);
        psp.hIcon = 0;
        psp.pszTitle = MAKEINTRESOURCEW(IDS_PROP_SHEET_TITLE);
        psp.pfnDlgProc = (DLGPROC)PropSheetDlgProc;
        psp.pcRefParent = (UINT*)&objs_loaded;
        psp.pfnCallback = nullptr;
        psp.lParam = (LPARAM)this;

        hPage = CreatePropertySheetPageW(&psp);

        if (hPage) {
            if (pfnAddPage(hPage, lParam)) {
                this->AddRef();
                return S_OK;
            } else
                DestroyPropertySheetPage(hPage);
        } else
            return E_OUTOFMEMORY;
    } catch (const exception& e) {
        error_message(nullptr, e.what());
    }

    return E_FAIL;
}

HRESULT __stdcall BtrfsPropSheet::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam) {
    return S_OK;
}

#ifdef __cplusplus
extern "C" {
#endif

void CALLBACK ShowPropSheetW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        BtrfsPropSheet bps;
        PROPSHEETPAGEW psp;
        PROPSHEETHEADERW psh;
        INITCOMMONCONTROLSEX icex;
        wstring title;

        set_dpi_aware();

        load_string(module, IDS_STANDALONE_PROPSHEET_TITLE, title);

        bps.set_cmdline(lpszCmdLine);

        icex.dwSize = sizeof(icex);
        icex.dwICC = ICC_LINK_CLASS;

        if (!InitCommonControlsEx(&icex))
            throw string_error(IDS_INITCOMMONCONTROLSEX_FAILED);

        psp.dwSize = sizeof(psp);
        psp.dwFlags = PSP_USETITLE;
        psp.hInstance = module;
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_PROP_SHEET);
        psp.hIcon = 0;
        psp.pszTitle = MAKEINTRESOURCEW(IDS_PROP_SHEET_TITLE);
        psp.pfnDlgProc = (DLGPROC)PropSheetDlgProc;
        psp.pfnCallback = nullptr;
        psp.lParam = (LPARAM)&bps;

        memset(&psh, 0, sizeof(PROPSHEETHEADERW));

        psh.dwSize = sizeof(PROPSHEETHEADERW);
        psh.dwFlags = PSH_PROPSHEETPAGE;
        psh.hwndParent = hwnd;
        psh.hInstance = psp.hInstance;
        psh.pszCaption = title.c_str();
        psh.nPages = 1;
        psh.ppsp = &psp;

        if (PropertySheetW(&psh) < 0)
            throw last_error(GetLastError());
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

#ifdef __cplusplus
}
#endif
