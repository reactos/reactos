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

#pragma once

#include <shlobj.h>
#include <deque>
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "btrfsioctl.h"
#endif

#ifndef S_IRUSR
#define S_IRUSR 0000400
#endif

#ifndef S_IWUSR
#define S_IWUSR 0000200
#endif

#ifndef S_IXUSR
#define S_IXUSR 0000100
#endif

#ifndef S_IRGRP
#define S_IRGRP (S_IRUSR >> 3)
#endif

#ifndef S_IWGRP
#define S_IWGRP (S_IWUSR >> 3)
#endif

#ifndef S_IXGRP
#define S_IXGRP (S_IXUSR >> 3)
#endif

#ifndef S_IROTH
#define S_IROTH (S_IRGRP >> 3)
#endif

#ifndef S_IWOTH
#define S_IWOTH (S_IWGRP >> 3)
#endif

#ifndef S_IXOTH
#define S_IXOTH (S_IXGRP >> 3)
#endif

#ifndef S_ISUID
#define S_ISUID 0004000
#endif

#ifndef S_ISGID
#define S_ISGID 0002000
#endif

#ifndef S_ISVTX
#define S_ISVTX 0001000
#endif

#define BTRFS_INODE_NODATASUM   0x001
#define BTRFS_INODE_NODATACOW   0x002
#define BTRFS_INODE_READONLY    0x004
#define BTRFS_INODE_NOCOMPRESS  0x008
#define BTRFS_INODE_PREALLOC    0x010
#define BTRFS_INODE_SYNC        0x020
#define BTRFS_INODE_IMMUTABLE   0x040
#define BTRFS_INODE_APPEND      0x080
#define BTRFS_INODE_NODUMP      0x100
#define BTRFS_INODE_NOATIME     0x200
#define BTRFS_INODE_DIRSYNC     0x400
#define BTRFS_INODE_COMPRESS    0x800

extern LONG objs_loaded;

class BtrfsPropSheet : public IShellExtInit, IShellPropSheetExt {
public:
    BtrfsPropSheet() {
        refcount = 0;
        ignore = true;
        stgm_set = false;
        readonly = false;
        flags_changed = false;
        perms_changed = false;
        uid_changed = false;
        gid_changed = false;
        compress_type_changed = false;
        ro_changed = false;
        can_change_perms = false;
        show_admin_button = false;
        thread = nullptr;
        mode = mode_set = 0;
        flags = flags_set = 0;
        has_subvols = false;
        filename = L"";

        sizes[0] = sizes[1] = sizes[2] = sizes[3] = sizes[4] = 0;
        totalsize = allocsize = sparsesize = 0;
        num_extents = 0;
        sector_size = 0;
        size_format[0] = 0;
        cr_format[0] = 0;
        frag_format[0] = 0;

        InterlockedIncrement(&objs_loaded);
    }

    virtual ~BtrfsPropSheet() {
        if (stgm_set) {
            GlobalUnlock(stgm.hGlobal);
            ReleaseStgMedium(&stgm);
        }

        InterlockedDecrement(&objs_loaded);
    }

    // IUnknown

    HRESULT __stdcall QueryInterface(REFIID riid, void **ppObj);

    ULONG __stdcall AddRef() {
        return InterlockedIncrement(&refcount);
    }

    ULONG __stdcall Release() {
        LONG rc = InterlockedDecrement(&refcount);

        if (rc == 0)
            delete this;

        return rc;
    }

    // IShellExtInit

    virtual HRESULT __stdcall Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID);

    // IShellPropSheetExt

    virtual HRESULT __stdcall AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam);
    virtual HRESULT __stdcall ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam);

    void init_propsheet(HWND hwndDlg);
    void change_inode_flag(HWND hDlg, uint64_t flag, UINT state);
    void change_perm_flag(HWND hDlg, ULONG perm, UINT state);
    void change_uid(HWND hDlg, uint32_t uid);
    void change_gid(HWND hDlg, uint32_t gid);
    void apply_changes(HWND hDlg);
    void set_size_on_disk(HWND hwndDlg);
    DWORD search_list_thread();
    void do_search(const wstring& fn);
    void update_size_details_dialog(HWND hDlg);
    void open_as_admin(HWND hwndDlg);
    void set_cmdline(const wstring& cmdline);

    bool readonly;
    bool can_change_perms;
    bool can_change_nocow;
    WCHAR size_format[255], cr_format[255], frag_format[255];
    HANDLE thread;
    uint32_t min_mode, max_mode, mode, mode_set;
    uint64_t min_flags, max_flags, flags, flags_set;
    uint64_t subvol, inode, rdev;
    uint8_t type, min_compression_type, max_compression_type, compress_type;
    uint32_t uid, gid;
    bool various_subvols, various_inodes, various_types, various_uids, various_gids, compress_type_changed, has_subvols,
         ro_subvol, various_ro, ro_changed, show_admin_button;

private:
    LONG refcount;
    bool ignore;
    STGMEDIUM stgm;
    bool stgm_set;
    bool flags_changed, perms_changed, uid_changed, gid_changed;
    uint64_t sizes[5], totalsize, allocsize, sparsesize, num_extents;
    deque<wstring> search_list;
    wstring filename;
    uint32_t sector_size;

    void apply_changes_file(HWND hDlg, const wstring& fn);
    HRESULT check_file(const wstring& fn, UINT i, UINT num_files, UINT* sv);
    HRESULT load_file_list();
};
