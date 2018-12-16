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

#ifndef __REACTOS__
#include <windows.h>
#include <winternl.h>
#else
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <ndk/iofuncs.h>
#include <ndk/iotypes.h>
#endif
#include <shlobj.h>
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "btrfsioctl.h"
#endif

typedef struct {
    wstring pnp_name;
    wstring friendly_name;
    wstring drive;
    wstring fstype;
    ULONG disk_num;
    ULONG part_num;
    uint64_t size;
    bool has_parts;
    BTRFS_UUID fs_uuid;
    BTRFS_UUID dev_uuid;
    bool ignore;
    bool multi_device;
    bool is_disk;
} device;

typedef struct {
    const WCHAR* name;
    const char* magic;
    ULONG magiclen;
    uint32_t sboff;
    uint32_t kboff;
} fs_identifier;

// This list is compiled from information in libblkid, part of util-linux
// and likewise under the LGPL. Thanks!

const static fs_identifier fs_ident[] = {
    { L"BeFS", "BFS1", 4, 0x20, 0 },
    { L"BeFS", "1SFB", 4, 0x20, 0 },
    { L"BeFS", "BFS1", 4, 0x220, 0 },
    { L"BeFS", "1SFB", 4, 0x220, 0 },
    { L"BFS", "\xce\xfa\xad\x1b", 4, 0, 0 },
    { L"RomFS", "-rom1fs-", 8, 0, 0 },
    { L"SquashFS", "hsqs", 4, 0, 0 },
    { L"SquashFS", "sqsh", 4, 0, 0 },
    { L"SquashFS", "hsqs", 4, 0, 0 },
    { L"UBIFS", "\x31\x18\x10\x06", 4, 0, 0 },
    { L"XFS", "XFSB", 4, 0, 0 },
    { L"ext2", "\123\357", 2, 0x38, 1 },
    { L"F2FS", "\x10\x20\xF5\xF2", 4, 0, 1 },
    { L"HFS", "BD", 2, 0, 1 },
    { L"HFS", "BD", 2, 0, 1 },
    { L"HFS", "H+", 2, 0, 1 },
    { L"HFS", "HX", 2, 0, 1 },
    { L"Minix", "\177\023", 2, 0x10, 1 },
    { L"Minix", "\217\023", 2, 0x10, 1 },
    { L"Minix", "\023\177", 2, 0x10, 1 },
    { L"Minix", "\023\217", 2, 0x10, 1 },
    { L"Minix", "\150\044", 2, 0x10, 1 },
    { L"Minix", "\170\044", 2, 0x10, 1 },
    { L"Minix", "\044\150", 2, 0x10, 1 },
    { L"Minix", "\044\170", 2, 0x10, 1 },
    { L"Minix", "\132\115", 2, 0x10, 1 },
    { L"Minix", "\115\132", 2, 0x10, 1 },
    { L"OCFS", "OCFSV2", 6, 0, 1 },
    { L"SysV", "\x2b\x55\x44", 3, 0x400, 1 },
    { L"SysV", "\x44\x55\x2b", 3, 0x400, 1 },
    { L"VXFS", "\365\374\001\245", 4, 0, 1 },
    { L"OCFS", "OCFSV2", 6, 0, 2 },
    { L"ExFAT", "EXFAT   ", 8, 3 },
    { L"NTFS", "NTFS    ", 8, 3 },
    { L"NetWare", "SPB5", 4, 0, 4 },
    { L"OCFS", "OCFSV2", 6, 0, 4 },
    { L"HPFS", "\x49\xe8\x95\xf9", 4, 0, 8 },
    { L"OCFS", "OracleCFS", 9, 0, 8 },
    { L"OCFS", "OCFSV2", 6, 0, 8 },
    { L"ReFS", "\000\000\000ReFS\000", 8 },
    { L"ReiserFS", "ReIsErFs",  8, 0x34, 8 },
    { L"ReiserFS", "ReIsErFs",  8, 20,  8 },
    { L"ISO9660", "CD001", 5, 1, 32, },
    { L"ISO9660", "CDROM", 5, 9, 32, },
    { L"JFS", "JFS1", 4, 0, 32 },
    { L"OCFS", "ORCLDISK", 8, 32 },
    { L"UDF", "BEA01", 5, 1, 32 },
    { L"UDF", "BOOT2", 5, 1, 32 },
    { L"UDF", "CD001", 5, 1, 32 },
    { L"UDF", "CDW02", 5, 1, 32 },
    { L"UDF", "NSR02", 5, 1, 32 },
    { L"UDF", "NSR03", 5, 1, 32 },
    { L"UDF", "TEA01", 5, 1, 32 },
    { L"Btrfs", "_BHRfS_M", 8, 0x40, 64 },
    { L"GFS", "\x01\x16\x19\x70", 4, 0, 64 },
    { L"GFS", "\x01\x16\x19\x70", 4, 0, 64 },
    { L"ReiserFS", "ReIsEr2Fs", 9, 0x34, 64 },
    { L"ReiserFS", "ReIsEr3Fs", 9, 0x34, 64 },
    { L"ReiserFS", "ReIsErFs",  8, 0x34, 64 },
    { L"ReiserFS", "ReIsEr4", 7, 0, 64 },
    { L"VMFS", "\x0d\xd0\x01\xc0", 4, 0, 1024 },
    { L"VMFS", "\x5e\xf1\xab\x2f", 4, 0, 2048 },
    { L"FAT", "MSWIN",    5, 0x52, 0 },
    { L"FAT", "FAT32   ", 8, 0x52, 0 },
    { L"FAT", "MSDOS",    5, 0x36, 0 },
    { L"FAT", "FAT16   ", 8, 0x36, 0 },
    { L"FAT", "FAT12   ", 8, 0x36, 0 },
    { L"FAT", "FAT     ", 8, 0x36, 0 },
    { L"FAT", "\353",     1, 0, 0 },
    { L"FAT", "\351",     1, 0, 0},
    { L"FAT", "\125\252", 2, 0x1fe, 0 },
    { nullptr, 0, 0, 0 }
};

class BtrfsDeviceAdd {
public:
    INT_PTR CALLBACK DeviceAddDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ShowDialog();
    void AddDevice(HWND hwndDlg);
    BtrfsDeviceAdd(HINSTANCE hinst, HWND hwnd, WCHAR* cmdline);

private:
    void populate_device_tree(HWND tree);

    HINSTANCE hinst;
    HWND hwnd;
    WCHAR* cmdline;
    device* sel;
    vector<device> device_list;
};

class BtrfsDeviceResize {
public:
    INT_PTR CALLBACK DeviceResizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    void ShowDialog(HWND hwnd, const wstring& fn, uint64_t dev_id);

private:
    void do_resize(HWND hwndDlg);

    uint64_t dev_id, new_size;
    wstring fn;
    WCHAR new_size_text[255];
    btrfs_device dev_info;
};
