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
#include "devices.h"
#include "resource.h"
#include "balance.h"
#include <stddef.h>
#include <uxtheme.h>
#include <setupapi.h>
#include <strsafe.h>
#include <mountmgr.h>
#include <algorithm>
#ifndef __REACTOS__
#include "../btrfs.h"
#else
#include <ntddstor.h>
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>
#include "btrfs.h"
#endif

DEFINE_GUID(GUID_DEVINTERFACE_HIDDEN_VOLUME, 0x7f108a28L, 0x9833, 0x4b3b, 0xb7, 0x80, 0x2c, 0x6b, 0x5f, 0xa5, 0xc0, 0x62);

static std::wstring get_mountdev_name(HANDLE h) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    MOUNTDEV_NAME mdn, *mdn2;
    ULONG mdnsize;
    std::wstring name;

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                   NULL, 0, &mdn, sizeof(MOUNTDEV_NAME));
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        return L"";

    mdnsize = offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;

    mdn2 = (MOUNTDEV_NAME*)malloc(mdnsize);

    Status = NtDeviceIoControlFile(h, NULL, NULL, NULL, &iosb, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                   NULL, 0, mdn2, mdnsize);
    if (!NT_SUCCESS(Status)) {
        free(mdn2);
        return L"";
    }

    name = std::wstring(mdn2->Name, mdn2->NameLength / sizeof(WCHAR));

    free(mdn2);

    return name;
}

static void find_devices(HWND hwnd, const GUID* guid, HANDLE mountmgr, std::vector<device>* device_list) {
    HDEVINFO h;

    static WCHAR dosdevices[] = L"\\DosDevices\\";

    h = SetupDiGetClassDevs(guid, NULL, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (h != INVALID_HANDLE_VALUE) {
        DWORD index = 0;
        SP_DEVICE_INTERFACE_DATA did;

        did.cbSize = sizeof(did);

        if (!SetupDiEnumDeviceInterfaces(h, NULL, guid, index, &did))
            return;

        do {
            SP_DEVINFO_DATA dd;
            SP_DEVICE_INTERFACE_DETAIL_DATA_W* detail;
            DWORD size;

            dd.cbSize = sizeof(dd);

            SetupDiGetDeviceInterfaceDetailW(h, &did, NULL, 0, &size, NULL);

            detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)malloc(size);
            memset(detail, 0, size);

            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

            if (SetupDiGetDeviceInterfaceDetailW(h, &did, detail, size, &size, &dd)) {
                NTSTATUS Status;
                HANDLE file;
                device dev;
                STORAGE_DEVICE_NUMBER sdn;
                IO_STATUS_BLOCK iosb;
                UNICODE_STRING path;
                OBJECT_ATTRIBUTES attr;
                GET_LENGTH_INFORMATION gli;
                ULONG i;
                UINT8 sb[4096];

                path.Buffer = detail->DevicePath;
                path.Length = path.MaximumLength = wcslen(detail->DevicePath) * sizeof(WCHAR);

                if (path.Length > 4 * sizeof(WCHAR) && path.Buffer[0] == '\\' && path.Buffer[1] == '\\'  && path.Buffer[2] == '?'  && path.Buffer[3] == '\\')
                    path.Buffer[1] = '?';

                InitializeObjectAttributes(&attr, &path, 0, NULL, NULL);

                Status = NtOpenFile(&file, FILE_GENERIC_READ, &attr, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_ALERT);

                if (!NT_SUCCESS(Status))
                    goto nextitem2;

                dev.pnp_name = detail->DevicePath;

                Status = NtDeviceIoControlFile(file, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_LENGTH_INFO, NULL, 0, &gli, sizeof(GET_LENGTH_INFORMATION));
                if (!NT_SUCCESS(Status))
                    goto nextitem;

                dev.size = gli.Length.QuadPart;

                Status = NtDeviceIoControlFile(file, NULL, NULL, NULL, &iosb, IOCTL_STORAGE_GET_DEVICE_NUMBER, NULL, 0, &sdn, sizeof(STORAGE_DEVICE_NUMBER));
                if (!NT_SUCCESS(Status)) {
                    dev.disk_num = 0xffffffff;
                    dev.part_num = 0xffffffff;
                } else {
                    dev.disk_num = sdn.DeviceNumber;
                    dev.part_num = sdn.PartitionNumber;
                }

                dev.friendly_name = L"";
                dev.drive = L"";
                dev.fstype = L"";
                dev.has_parts = FALSE;
                dev.ignore = FALSE;
                dev.multi_device = FALSE;

                dev.is_disk = RtlCompareMemory(guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) == sizeof(GUID);

                if (dev.is_disk) {
                    STORAGE_PROPERTY_QUERY spq;
                    STORAGE_DEVICE_DESCRIPTOR sdd, *sdd2;
                    ULONG dlisize;
                    DRIVE_LAYOUT_INFORMATION_EX* dli;

                    spq.PropertyId = StorageDeviceProperty;
                    spq.QueryType = PropertyStandardQuery;
                    spq.AdditionalParameters[0] = 0;

                    Status = NtDeviceIoControlFile(file, NULL, NULL, NULL, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
                                                   &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR));

                    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) {
                        sdd2 = (STORAGE_DEVICE_DESCRIPTOR*)malloc(sdd.Size);

                        Status = NtDeviceIoControlFile(file, NULL, NULL, NULL, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
                                                    &spq, sizeof(STORAGE_PROPERTY_QUERY), sdd2, sdd.Size);
                        if (NT_SUCCESS(Status)) {
                            std::string desc2;

                            desc2 = "";

                            if (sdd2->VendorIdOffset != 0) {
                                desc2 += (char*)((UINT8*)sdd2 + sdd2->VendorIdOffset);

                                while (desc2.length() > 0 && desc2[desc2.length() - 1] == ' ')
                                    desc2 = desc2.substr(0, desc2.length() - 1);
                            }

                            if (sdd2->ProductIdOffset != 0) {
                                if (sdd2->VendorIdOffset != 0 && desc2.length() != 0 && desc2[desc2.length() - 1] != ' ')
                                    desc2 += " ";

                                desc2 += (char*)((UINT8*)sdd2 + sdd2->ProductIdOffset);

                                while (desc2.length() > 0 && desc2[desc2.length() - 1] == ' ')
                                    desc2 = desc2.substr(0, desc2.length() - 1);
                            }

                            if (sdd2->VendorIdOffset != 0 || sdd2->ProductIdOffset != 0) {
                                ULONG ss;

                                ss = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, desc2.c_str(), -1, NULL, 0);

                                if (ss > 0) {
                                    WCHAR* desc3 = (WCHAR*)malloc(ss * sizeof(WCHAR));

                                    if (MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, desc2.c_str(), -1, desc3, ss * sizeof(WCHAR)))
                                        dev.friendly_name = desc3;

                                    free(desc3);
                                }
                            }
                        }

                        free(sdd2);
                    }

                    dlisize = 0;
                    dli = NULL;

                    do {
                        dlisize += 1024;

                        if (dli)
                            free(dli);

                        dli = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(dlisize);

                        Status = NtDeviceIoControlFile(file, NULL, NULL, NULL, &iosb, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                                       NULL, 0, dli, dlisize);
                    } while (Status == STATUS_BUFFER_TOO_SMALL);

                    if (NT_SUCCESS(Status) && dli->PartitionCount > 0)
                        dev.has_parts = TRUE;

                    free(dli);
                } else {
                    ULONG mmpsize;
                    MOUNTMGR_MOUNT_POINT* mmp;
                    MOUNTMGR_MOUNT_POINTS mmps;

                    mmpsize = sizeof(MOUNTMGR_MOUNT_POINT) + path.Length;

                    mmp = (MOUNTMGR_MOUNT_POINT*)malloc(mmpsize);

                    RtlZeroMemory(mmp, sizeof(MOUNTMGR_MOUNT_POINT));
                    mmp->DeviceNameOffset = sizeof(MOUNTMGR_MOUNT_POINT);
                    mmp->DeviceNameLength = path.Length;
                    RtlCopyMemory(&mmp[1], path.Buffer, path.Length);

                    Status = NtDeviceIoControlFile(mountmgr, NULL, NULL, NULL, &iosb, IOCTL_MOUNTMGR_QUERY_POINTS,
                                                   mmp, mmpsize, &mmps, sizeof(MOUNTMGR_MOUNT_POINTS));
                    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) {
                        MOUNTMGR_MOUNT_POINTS* mmps2;

                        mmps2 = (MOUNTMGR_MOUNT_POINTS*)malloc(mmps.Size);

                        Status = NtDeviceIoControlFile(mountmgr, NULL, NULL, NULL, &iosb, IOCTL_MOUNTMGR_QUERY_POINTS,
                                                    mmp, mmpsize, mmps2, mmps.Size);

                        if (NT_SUCCESS(Status)) {
                            ULONG i;

                            for (i = 0; i < mmps2->NumberOfMountPoints; i++) {
                                WCHAR* symlink = (WCHAR*)((UINT8*)mmps2 + mmps2->MountPoints[i].SymbolicLinkNameOffset);

                                if (mmps2->MountPoints[i].SymbolicLinkNameLength == 0x1c &&
                                    RtlCompareMemory(symlink, dosdevices, wcslen(dosdevices) * sizeof(WCHAR)) == wcslen(dosdevices) * sizeof(WCHAR) &&
                                    symlink[13] == ':'
                                ) {
                                    WCHAR dr[3];

                                    dr[0] = symlink[12];
                                    dr[1] = ':';
                                    dr[2] = 0;

                                    dev.drive = dr;
                                    break;
                                }
                            }
                        }
                    }

                    free(mmp);
                }

                if (!dev.is_disk || !dev.has_parts) {
                    i = 0;
                    while (fs_ident[i].name) {
                        if (i == 0 || fs_ident[i].kboff != fs_ident[i-1].kboff) {
                            LARGE_INTEGER off;

                            off.QuadPart = fs_ident[i].kboff * 1024;
                            Status = NtReadFile(file, NULL, NULL, NULL, &iosb, sb, sizeof(sb), &off, NULL);
                        }

                        if (NT_SUCCESS(Status)) {
                            if (RtlCompareMemory(sb + fs_ident[i].sboff, fs_ident[i].magic, fs_ident[i].magiclen) == fs_ident[i].magiclen) {
                                dev.fstype = fs_ident[i].name;

                                if (dev.fstype == L"Btrfs") {
                                    superblock* bsb = (superblock*)sb;

                                    RtlCopyMemory(&dev.fs_uuid, &bsb->uuid, sizeof(BTRFS_UUID));
                                    RtlCopyMemory(&dev.dev_uuid, &bsb->dev_item.device_uuid, sizeof(BTRFS_UUID));
                                }

                                break;
                            }
                        }

                        i++;
                    }

                    if (dev.fstype == L"Btrfs" && RtlCompareMemory(guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) != sizeof(GUID)) {
                        std::wstring name;
                        std::wstring pref = L"\\Device\\Btrfs{";

                        name = get_mountdev_name(file);

                        if (name.length() > pref.length() && RtlCompareMemory(name.c_str(), pref.c_str(), pref.length() * sizeof(WCHAR)) == pref.length() * sizeof(WCHAR))
                            dev.ignore = TRUE;
                    }
                }

                device_list->push_back(dev);

nextitem:
                NtClose(file);
            }

nextitem2:
            free(detail);

            index++;
        } while (SetupDiEnumDeviceInterfaces(h, NULL, guid, index, &did));

        SetupDiDestroyDeviceInfoList(h);
    } else {
        ShowError(hwnd, GetLastError());
        return;
    }
}

static bool sort_devices(device i, device j) {
    if (i.disk_num < j.disk_num)
        return true;

    if (i.disk_num == j.disk_num && i.part_num < j.part_num)
        return true;

    return false;
}

void BtrfsDeviceAdd::populate_device_tree(HWND tree) {
    HWND hwnd = GetParent(tree);
    unsigned int i;
    ULONG last_disk_num = 0xffffffff;
    HTREEITEM diskitem;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING us;
    IO_STATUS_BLOCK iosb;
    HANDLE mountmgr, btrfsh;
    btrfs_filesystem* bfs = NULL;

    static WCHAR btrfs[] = L"\\Btrfs";

    device_list.clear();

    RtlInitUnicodeString(&us, MOUNTMGR_DEVICE_NAME);
    InitializeObjectAttributes(&attr, &us, 0, NULL, NULL);

    Status = NtOpenFile(&mountmgr, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb,
                        FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status)) {
        MessageBoxW(hwnd, L"Could not get handle to mount manager.", L"Error", MB_ICONERROR);
        return;
    }

    us.Length = us.MaximumLength = wcslen(btrfs) * sizeof(WCHAR);
    us.Buffer = btrfs;

    InitializeObjectAttributes(&attr, &us, 0, NULL, NULL);

    Status = NtOpenFile(&btrfsh, SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &iosb,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
    if (NT_SUCCESS(Status)) {
        ULONG bfssize = 0;

        do {
            bfssize += 1024;

            if (bfs) free(bfs);
            bfs = (btrfs_filesystem*)malloc(bfssize);

            Status = NtDeviceIoControlFile(btrfsh, NULL, NULL, NULL, &iosb, IOCTL_BTRFS_QUERY_FILESYSTEMS, NULL, 0, bfs, bfssize);
            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
                free(bfs);
                bfs = NULL;
                break;
            }
        } while (Status == STATUS_BUFFER_OVERFLOW);

        if (bfs && bfs->num_devices == 0) { // no mounted filesystems found
            free(bfs);
            bfs = NULL;
        }
    }
    NtClose(btrfsh);

    find_devices(hwnd, &GUID_DEVINTERFACE_DISK, mountmgr, &device_list);
    find_devices(hwnd, &GUID_DEVINTERFACE_VOLUME, mountmgr, &device_list);
    find_devices(hwnd, &GUID_DEVINTERFACE_HIDDEN_VOLUME, mountmgr, &device_list);

    NtClose(mountmgr);

    std::sort(device_list.begin(), device_list.end(), sort_devices);

    for (i = 0; i < device_list.size(); i++) {
        if (!device_list[i].ignore) {
            TVINSERTSTRUCTW tis;
            HTREEITEM item;
            std::wstring name;
            WCHAR size[255];

            if (device_list[i].disk_num != 0xffffffff && device_list[i].disk_num == last_disk_num)
                tis.hParent = diskitem;
            else
                tis.hParent = TVI_ROOT;

            tis.hInsertAfter = TVI_LAST;
            tis.itemex.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
            tis.itemex.state = TVIS_EXPANDED;
            tis.itemex.stateMask = TVIS_EXPANDED;

            if (device_list[i].disk_num != 0xffffffff) {
                WCHAR t[255], u[255];

                if (!LoadStringW(module, device_list[i].part_num != 0 ? IDS_PARTITION : IDS_DISK_NUM, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwnd, GetLastError());
                    return;
                }

                if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, device_list[i].part_num != 0 ? device_list[i].part_num : device_list[i].disk_num) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return;

                name = u;
            } else
                name = device_list[i].pnp_name;

            // match child Btrfs devices to their parent
            if (bfs && device_list[i].drive == L"" && device_list[i].fstype == L"Btrfs") {
                btrfs_filesystem* bfs2 = bfs;

                while (TRUE) {
                    if (RtlCompareMemory(&bfs2->uuid, &device_list[i].fs_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                        ULONG j, k;
                        btrfs_filesystem_device* dev;

                        for (j = 0; j < bfs2->num_devices; j++) {
                            if (j == 0)
                                dev = &bfs2->device;
                            else
                                dev = (btrfs_filesystem_device*)((UINT8*)dev + offsetof(btrfs_filesystem_device, name[0]) + dev->name_length);

                            if (RtlCompareMemory(&device_list[i].dev_uuid, &device_list[i].dev_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                                for (k = 0; k < device_list.size(); k++) {
                                    if (k != i && device_list[k].fstype == L"Btrfs" && device_list[k].drive != L"" &&
                                        RtlCompareMemory(&device_list[k].fs_uuid, &device_list[i].fs_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                                        device_list[i].drive = device_list[k].drive;
                                        break;
                                    }
                                }

                                device_list[i].multi_device = bfs2->num_devices > 1;

                                break;
                            }
                        }

                        break;
                    }

                    if (bfs2->next_entry != 0)
                        bfs2 = (btrfs_filesystem*)((UINT8*)bfs2 + bfs2->next_entry);
                    else
                        break;
                }
            }

            name += L" (";

            if (device_list[i].friendly_name != L"") {
                name += device_list[i].friendly_name;
                name += L", ";
            }

            if (device_list[i].drive != L"") {
                name += device_list[i].drive;
                name += L", ";
            }

            if (device_list[i].fstype != L"") {
                name += device_list[i].fstype;
                name += L", ";
            }

            format_size(device_list[i].size, size, sizeof(size) / sizeof(WCHAR), FALSE);
            name += size;

            name += L")";

            tis.itemex.pszText = (WCHAR*)name.c_str();
            tis.itemex.cchTextMax = name.length();
            tis.itemex.lParam = (LPARAM)&device_list[i];

            item = (HTREEITEM)SendMessageW(tree, TVM_INSERTITEMW, 0, (LPARAM)&tis);
            if (!item) {
                MessageBoxW(hwnd, L"TVM_INSERTITEM failed", L"Error", MB_ICONERROR);
                return;
            }

            if (device_list[i].part_num == 0) {
                diskitem = item;
                last_disk_num = device_list[i].disk_num;
            }
        }
    }
}

void BtrfsDeviceAdd::AddDevice(HWND hwndDlg) {
    WCHAR mess[255], title[255];
    NTSTATUS Status;
    UNICODE_STRING vn;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    HANDLE h, h2;

    if (!sel) {
        EndDialog(hwndDlg, 0);
        return;
    }

    if (sel->fstype != L"") {
        WCHAR s[255];

        if (!LoadStringW(module, IDS_ADD_DEVICE_CONFIRMATION_FS, s, sizeof(s) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        if (StringCchPrintfW(mess, sizeof(mess) / sizeof(WCHAR), s, sel->fstype.c_str()) == STRSAFE_E_INSUFFICIENT_BUFFER)
            return;
    } else {
        if (!LoadStringW(module, IDS_ADD_DEVICE_CONFIRMATION, mess, sizeof(mess) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            return;
        }
    }

    if (!LoadStringW(module, IDS_CONFIRMATION_TITLE, title, sizeof(title) / sizeof(WCHAR))) {
        ShowError(hwndDlg, GetLastError());
        return;
    }

    if (MessageBoxW(hwndDlg, mess, title, MB_YESNO) != IDYES)
        return;

    h = CreateFileW(cmdline, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        ShowError(hwndDlg, GetLastError());
        return;
    }

    vn.Length = vn.MaximumLength = sel->pnp_name.length() * sizeof(WCHAR);
    vn.Buffer = (WCHAR*)sel->pnp_name.c_str();

    InitializeObjectAttributes(&attr, &vn, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, NULL, NULL);

    Status = NtOpenFile(&h2, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);
    if (!NT_SUCCESS(Status)) {
        ShowNtStatusError(hwndDlg, Status);
        CloseHandle(h);
        return;
    }

    if (!sel->is_disk) {
        Status = NtFsControlFile(h2, NULL, NULL, NULL, &iosb, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0);
        if (!NT_SUCCESS(Status)) {
            WCHAR t[255], u[255];

            if (!LoadStringW(module, IDS_LOCK_FAILED, t, sizeof(t) / sizeof(WCHAR))) {
                ShowError(hwnd, GetLastError());
                return;
            }

            if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, Status) == STRSAFE_E_INSUFFICIENT_BUFFER)
                return;

            if (!LoadStringW(module, IDS_ERROR, title, sizeof(title) / sizeof(WCHAR))) {
                ShowError(hwndDlg, GetLastError());
                return;
            }

            MessageBoxW(hwndDlg, u, title, MB_ICONERROR);

            NtClose(h2);
            CloseHandle(h);
            return;
        }
    }

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_ADD_DEVICE, &h2, sizeof(HANDLE), NULL, 0);
    if (!NT_SUCCESS(Status)) {
        ShowNtStatusError(hwndDlg, Status);
        NtClose(h2);
        CloseHandle(h);
        return;
    }

    if (!sel->is_disk) {
        Status = NtFsControlFile(h2, NULL, NULL, NULL, &iosb, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0);
        if (!NT_SUCCESS(Status))
            ShowNtStatusError(hwndDlg, Status);

        Status = NtFsControlFile(h2, NULL, NULL, NULL, &iosb, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0);
        if (!NT_SUCCESS(Status))
            ShowNtStatusError(hwndDlg, Status);
    }

    NtClose(h2);
    CloseHandle(h);

    EndDialog(hwndDlg, 0);
}

INT_PTR CALLBACK BtrfsDeviceAdd::DeviceAddDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
            populate_device_tree(GetDlgItem(hwndDlg, IDC_DEVICE_TREE));
            EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
            break;
        }

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDOK:
                            AddDevice(hwndDlg);
                        return TRUE;

                        case IDCANCEL:
                            EndDialog(hwndDlg, 0);
                        return TRUE;
                    }
                break;
            }
        break;

        case WM_NOTIFY:
            switch (((LPNMHDR)lParam)->code) {
                case TVN_SELCHANGEDW:
                {
                    NMTREEVIEWW* nmtv = (NMTREEVIEWW*)lParam;
                    TVITEMW tvi;
                    BOOL enable = FALSE;

                    RtlZeroMemory(&tvi, sizeof(TVITEMW));
                    tvi.hItem = nmtv->itemNew.hItem;
                    tvi.mask = TVIF_PARAM | TVIF_HANDLE;

                    if (SendMessageW(GetDlgItem(hwndDlg, IDC_DEVICE_TREE), TVM_GETITEMW, 0, (LPARAM)&tvi))
                        sel = tvi.lParam == 0 ? NULL : (device*)tvi.lParam;
                    else
                        sel = NULL;

                    if (sel)
                        enable = (!sel->is_disk || !sel->has_parts) && !sel->multi_device;

                    EnableWindow(GetDlgItem(hwndDlg, IDOK), enable);
                    break;
                }
            }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK stub_DeviceAddDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsDeviceAdd* bda;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bda = (BtrfsDeviceAdd*)lParam;
    } else {
        bda = (BtrfsDeviceAdd*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    }

    if (bda)
        return bda->DeviceAddDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return FALSE;
}

void BtrfsDeviceAdd::ShowDialog() {
    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_DEVICE_ADD), hwnd, stub_DeviceAddDlgProc, (LPARAM)this);
}

BtrfsDeviceAdd::BtrfsDeviceAdd(HINSTANCE hinst, HWND hwnd, WCHAR* cmdline) {
    this->hinst = hinst;
    this->hwnd = hwnd;
    this->cmdline = cmdline;

    sel = NULL;
}

void BtrfsDeviceResize::do_resize(HWND hwndDlg) {
    HANDLE h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_resize br;

    h = CreateFileW(fn, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        ShowError(hwndDlg, GetLastError());
        return;
    }

    br.device = dev_id;
    br.size = new_size;

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_RESIZE, &br, sizeof(btrfs_resize), NULL, 0);

    if (Status != STATUS_MORE_PROCESSING_REQUIRED && !NT_SUCCESS(Status)) {
        ShowNtStatusError(hwndDlg, Status);
        CloseHandle(h);
        return;
    }

    CloseHandle(h);

    if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
        WCHAR s[255], t[255], u[255];

        LoadStringW(module, IDS_RESIZE_SUCCESSFUL, s, sizeof(s) / sizeof(WCHAR));
        format_size(new_size, u, sizeof(u) / sizeof(WCHAR), TRUE);
        StringCchPrintfW(t, sizeof(t) / sizeof(WCHAR), s, dev_id, u);
        MessageBoxW(hwndDlg, t, L"", MB_OK);

        EndDialog(hwndDlg, 0);
    } else {
        BtrfsBalance* bb;
        HWND par;

        par = GetParent(hwndDlg);
        EndDialog(hwndDlg, 0);

        bb = new BtrfsBalance(fn, FALSE, TRUE);

        bb->ShowBalance(par);

        delete bb;
    }
}

INT_PTR CALLBACK BtrfsDeviceResize::DeviceResizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
        {
            HANDLE h;
            WCHAR s[255], t[255], u[255];

            EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

            GetDlgItemTextW(hwndDlg, IDC_RESIZE_DEVICE_ID, s, sizeof(s) / sizeof(WCHAR));
            StringCchPrintfW(t, sizeof(t) / sizeof(WCHAR), s, dev_id);
            SetDlgItemTextW(hwndDlg, IDC_RESIZE_DEVICE_ID, t);

            h = CreateFileW(fn, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

            if (h != INVALID_HANDLE_VALUE) {
                NTSTATUS Status;
                IO_STATUS_BLOCK iosb;
                btrfs_device *devices, *bd;
                ULONG devsize;
                BOOL found = FALSE;
                HWND slider;

                devsize = 1024;
                devices = (btrfs_device*)malloc(devsize);

                while (TRUE) {
                    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_DEVICES, NULL, 0, devices, devsize);
                    if (Status == STATUS_BUFFER_OVERFLOW) {
                        devsize += 1024;

                        free(devices);
                        devices = (btrfs_device*)malloc(devsize);
                    } else
                        break;
                }

                if (!NT_SUCCESS(Status)) {
                    free(devices);
                    CloseHandle(h);
                    return FALSE;
                }

                bd = devices;

                while (TRUE) {
                    if (bd->dev_id == dev_id) {
                        memcpy(&dev_info, bd, sizeof(btrfs_device));
                        found = TRUE;
                        break;
                    }

                    if (bd->next_entry > 0)
                        bd = (btrfs_device*)((UINT8*)bd + bd->next_entry);
                    else
                        break;
                }

                if (!found) {
                    free(devices);
                    CloseHandle(h);
                    return FALSE;
                }

                free(devices);
                CloseHandle(h);

                GetDlgItemTextW(hwndDlg, IDC_RESIZE_CURSIZE, s, sizeof(s) / sizeof(WCHAR));
                format_size(dev_info.size, u, sizeof(u) / sizeof(WCHAR), TRUE);
                StringCchPrintfW(t, sizeof(t) / sizeof(WCHAR), s, u);
                SetDlgItemTextW(hwndDlg, IDC_RESIZE_CURSIZE, t);

                new_size = dev_info.size;

                GetDlgItemTextW(hwndDlg, IDC_RESIZE_NEWSIZE, new_size_text, sizeof(new_size_text) / sizeof(WCHAR));
                StringCchPrintfW(t, sizeof(t) / sizeof(WCHAR), new_size_text, u);
                SetDlgItemTextW(hwndDlg, IDC_RESIZE_NEWSIZE, t);

                slider = GetDlgItem(hwndDlg, IDC_RESIZE_SLIDER);
                SendMessageW(slider, TBM_SETRANGEMIN, FALSE, 0);
                SendMessageW(slider, TBM_SETRANGEMAX, FALSE, dev_info.max_size / 1048576);
                SendMessageW(slider, TBM_SETPOS, TRUE, new_size / 1048576);
            } else
                return FALSE;

            break;
        }

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDOK:
                            do_resize(hwndDlg);
                            return TRUE;

                        case IDCANCEL:
                            EndDialog(hwndDlg, 0);
                            return TRUE;
                    }
                break;
            }
        break;

        case WM_HSCROLL:
        {
            WCHAR t[255], u[255];

            new_size = UInt32x32To64(SendMessageW(GetDlgItem(hwndDlg, IDC_RESIZE_SLIDER), TBM_GETPOS, 0, 0), 1048576);

            format_size(new_size, u, sizeof(u) / sizeof(WCHAR), TRUE);
            StringCchPrintfW(t, sizeof(t) / sizeof(WCHAR), new_size_text, u);
            SetDlgItemTextW(hwndDlg, IDC_RESIZE_NEWSIZE, t);

            EnableWindow(GetDlgItem(hwndDlg, IDOK), new_size > 0 ? TRUE : FALSE);

            break;
        }
    }

    return FALSE;
}

static INT_PTR CALLBACK stub_DeviceResizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsDeviceResize* bdr;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bdr = (BtrfsDeviceResize*)lParam;
    } else
        bdr = (BtrfsDeviceResize*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    if (bdr)
        return bdr->DeviceResizeDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return FALSE;
}

void BtrfsDeviceResize::ShowDialog(HWND hwnd, WCHAR* fn, UINT64 dev_id) {
    this->dev_id = dev_id;
    wcscpy(this->fn, fn);

    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_RESIZE), hwnd, stub_DeviceResizeDlgProc, (LPARAM)this);
}

#ifdef __cplusplus
extern "C" {
#endif

void CALLBACK AddDeviceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    BtrfsDeviceAdd* bda;

    set_dpi_aware();

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    bda = new BtrfsDeviceAdd(hinst, hwnd, lpszCmdLine);
    bda->ShowDialog();
    delete bda;

end:
    CloseHandle(token);
}

void CALLBACK RemoveDeviceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    WCHAR *s, *vol, *dev;
    UINT64 devid;
    HANDLE h, token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    BtrfsBalance* bb;

    set_dpi_aware();

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    s = wcsstr(lpszCmdLine, L"|");
    if (!s)
        goto end;

    s[0] = 0;

    vol = lpszCmdLine;
    dev = &s[1];

    devid = _wtoi(dev);
    if (devid == 0)
        goto end;

    h = CreateFileW(vol, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                    OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);

    if (h == INVALID_HANDLE_VALUE) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_REMOVE_DEVICE, &devid, sizeof(UINT64), NULL, 0);
    if (!NT_SUCCESS(Status)) {
        if (Status == STATUS_CANNOT_DELETE)
            ShowStringError(hwnd, IDS_CANNOT_REMOVE_RAID);
        else
            ShowNtStatusError(hwnd, Status);

        CloseHandle(h);
        goto end;
    }

    CloseHandle(h);

    bb = new BtrfsBalance(vol, TRUE);

    bb->ShowBalance(hwnd);

    delete bb;

end:
    CloseHandle(token);
}

void CALLBACK ResizeDeviceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    WCHAR *s, *vol, *dev;
    UINT64 devid;
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    BtrfsDeviceResize* bdr;

    set_dpi_aware();

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        ShowError(hwnd, GetLastError());
        goto end;
    }

    s = wcsstr(lpszCmdLine, L"|");
    if (!s)
        goto end;

    s[0] = 0;

    vol = lpszCmdLine;
    dev = &s[1];

    devid = _wtoi(dev);
    if (devid == 0)
        goto end;

    bdr = new BtrfsDeviceResize;
    bdr->ShowDialog(hwnd, vol, devid);
    delete bdr;

end:
    CloseHandle(token);
}

#ifdef __cplusplus
}
#endif
