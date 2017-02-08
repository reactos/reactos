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

#include <shlobj.h>
#include <deque>
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "../../drivers/filesystems/btrfs/btrfsioctl.h"
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
        ignore = TRUE;
        stgm_set = FALSE;
        readonly = FALSE;
        flags_changed = FALSE;
        perms_changed = FALSE;
        uid_changed = FALSE;
        gid_changed = FALSE;
        can_change_perms = FALSE;
        can_change_owner = FALSE;
        thread = NULL;
        mode = mode_set = 0;
        flags = flags_set = 0;
        
        sizes[0] = sizes[1] = sizes[2] = sizes[3] = 0;
        totalsize = 0;
        
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
    
    void change_inode_flag(HWND hDlg, UINT64 flag, UINT state);
    void change_perm_flag(HWND hDlg, ULONG perm, UINT state);
    void change_uid(HWND hDlg, UINT32 uid);
    void change_gid(HWND hDlg, UINT32 gid);
    void apply_changes(HWND hDlg);
    void set_size_on_disk(HWND hwndDlg);
    void add_to_search_list(WCHAR* fn);
    DWORD search_list_thread();
    void do_search(WCHAR* fn);
    void update_size_details_dialog(HWND hDlg);
 
    BOOL readonly;
    BOOL can_change_perms;
    BOOL can_change_owner;
    BOOL can_change_nocow;
    WCHAR size_format[255];
    HANDLE thread;
    UINT32 min_mode, max_mode, mode, mode_set;
    UINT64 min_flags, max_flags, flags, flags_set;
    UINT64 subvol, inode, rdev;
    UINT8 type;
    UINT32 uid, gid;
    BOOL various_subvols, various_inodes, various_types, various_uids, various_gids;
    
private:
    LONG refcount;
    BOOL ignore;
    STGMEDIUM stgm;
    BOOL stgm_set;
    BOOL flags_changed, perms_changed, uid_changed, gid_changed;
    UINT64 sizes[4], totalsize;
    std::deque<WCHAR*> search_list;
};
