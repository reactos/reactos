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
#ifndef __REACTOS__
#include <algorithm>
#include "../btrfs.h"
#include "mountmgr.h"
#else
#include <ntddstor.h>
#include <ndk/rtlfuncs.h>
#include <ndk/obfuncs.h>
#include "btrfs.h"
#include "mountmgr_local.h"
#endif

DEFINE_GUID(GUID_DEVINTERFACE_HIDDEN_VOLUME, 0x7f108a28L, 0x9833, 0x4b3b, 0xb7, 0x80, 0x2c, 0x6b, 0x5f, 0xa5, 0xc0, 0x62);

static wstring get_mountdev_name(const nt_handle& h ) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    MOUNTDEV_NAME mdn, *mdn2;
    wstring name;

    Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                   nullptr, 0, &mdn, sizeof(MOUNTDEV_NAME));
    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
        return L"";

    size_t mdnsize = offsetof(MOUNTDEV_NAME, Name[0]) + mdn.NameLength;

    mdn2 = (MOUNTDEV_NAME*)malloc(mdnsize);

    Status = NtDeviceIoControlFile(h, nullptr, nullptr, nullptr, &iosb, IOCTL_MOUNTDEV_QUERY_DEVICE_NAME,
                                   nullptr, 0, mdn2, (ULONG)mdnsize);
    if (!NT_SUCCESS(Status)) {
        free(mdn2);
        return L"";
    }

    name = wstring(mdn2->Name, mdn2->NameLength / sizeof(WCHAR));

    free(mdn2);

    return name;
}

static void find_devices(HWND hwnd, const GUID* guid, const mountmgr& mm, vector<device>& device_list) {
    HDEVINFO h;

    static const wstring dosdevices = L"\\DosDevices\\";

    h = SetupDiGetClassDevsW(guid, nullptr, 0, DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);

    if (h != INVALID_HANDLE_VALUE) {
        DWORD index = 0;
        SP_DEVICE_INTERFACE_DATA did;

        did.cbSize = sizeof(did);

        if (!SetupDiEnumDeviceInterfaces(h, nullptr, guid, index, &did))
            return;

        do {
            SP_DEVINFO_DATA dd;
            SP_DEVICE_INTERFACE_DETAIL_DATA_W* detail;
            DWORD size;

            dd.cbSize = sizeof(dd);

            SetupDiGetDeviceInterfaceDetailW(h, &did, nullptr, 0, &size, nullptr);

            detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_W*)malloc(size);
            memset(detail, 0, size);

            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W);

            if (SetupDiGetDeviceInterfaceDetailW(h, &did, detail, size, &size, &dd)) {
                NTSTATUS Status;
                nt_handle file;
                device dev;
                STORAGE_DEVICE_NUMBER sdn;
                IO_STATUS_BLOCK iosb;
                UNICODE_STRING path;
                OBJECT_ATTRIBUTES attr;
                GET_LENGTH_INFORMATION gli;
                ULONG i;
                uint8_t sb[4096];

                path.Buffer = detail->DevicePath;
                path.Length = path.MaximumLength = (uint16_t)(wcslen(detail->DevicePath) * sizeof(WCHAR));

                if (path.Length > 4 * sizeof(WCHAR) && path.Buffer[0] == '\\' && path.Buffer[1] == '\\'  && path.Buffer[2] == '?'  && path.Buffer[3] == '\\')
                    path.Buffer[1] = '?';

                InitializeObjectAttributes(&attr, &path, 0, nullptr, nullptr);

                Status = NtOpenFile(&file, FILE_GENERIC_READ, &attr, &iosb, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_SYNCHRONOUS_IO_ALERT);

                if (!NT_SUCCESS(Status)) {
                    free(detail);
                    index++;
                    continue;
                }

                dev.pnp_name = detail->DevicePath;

                Status = NtDeviceIoControlFile(file, nullptr, nullptr, nullptr, &iosb, IOCTL_DISK_GET_LENGTH_INFO, nullptr, 0, &gli, sizeof(GET_LENGTH_INFORMATION));
                if (!NT_SUCCESS(Status)) {
                    free(detail);
                    index++;
                    continue;
                }

                dev.size = gli.Length.QuadPart;

                Status = NtDeviceIoControlFile(file, nullptr, nullptr, nullptr, &iosb, IOCTL_STORAGE_GET_DEVICE_NUMBER, nullptr, 0, &sdn, sizeof(STORAGE_DEVICE_NUMBER));
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
                dev.has_parts = false;
                dev.ignore = false;
                dev.multi_device = false;

                dev.is_disk = RtlCompareMemory(guid, &GUID_DEVINTERFACE_DISK, sizeof(GUID)) == sizeof(GUID);

                if (dev.is_disk) {
                    STORAGE_PROPERTY_QUERY spq;
                    STORAGE_DEVICE_DESCRIPTOR sdd, *sdd2;
                    ULONG dlisize;
                    DRIVE_LAYOUT_INFORMATION_EX* dli;

                    spq.PropertyId = StorageDeviceProperty;
                    spq.QueryType = PropertyStandardQuery;
                    spq.AdditionalParameters[0] = 0;

                    Status = NtDeviceIoControlFile(file, nullptr, nullptr, nullptr, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
                                                   &spq, sizeof(STORAGE_PROPERTY_QUERY), &sdd, sizeof(STORAGE_DEVICE_DESCRIPTOR));

                    if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW) {
                        sdd2 = (STORAGE_DEVICE_DESCRIPTOR*)malloc(sdd.Size);

                        Status = NtDeviceIoControlFile(file, nullptr, nullptr, nullptr, &iosb, IOCTL_STORAGE_QUERY_PROPERTY,
                                                       &spq, sizeof(STORAGE_PROPERTY_QUERY), sdd2, sdd.Size);
                        if (NT_SUCCESS(Status)) {
                            string desc2;

                            desc2 = "";

                            if (sdd2->VendorIdOffset != 0) {
                                desc2 += (char*)((uint8_t*)sdd2 + sdd2->VendorIdOffset);

                                while (desc2.length() > 0 && desc2[desc2.length() - 1] == ' ')
                                    desc2 = desc2.substr(0, desc2.length() - 1);
                            }

                            if (sdd2->ProductIdOffset != 0) {
                                if (sdd2->VendorIdOffset != 0 && desc2.length() != 0 && desc2[desc2.length() - 1] != ' ')
                                    desc2 += " ";

                                desc2 += (char*)((uint8_t*)sdd2 + sdd2->ProductIdOffset);

                                while (desc2.length() > 0 && desc2[desc2.length() - 1] == ' ')
                                    desc2 = desc2.substr(0, desc2.length() - 1);
                            }

                            if (sdd2->VendorIdOffset != 0 || sdd2->ProductIdOffset != 0) {
                                ULONG ss;

                                ss = MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, desc2.c_str(), -1, nullptr, 0);

                                if (ss > 0) {
                                    WCHAR* desc3 = (WCHAR*)malloc(ss * sizeof(WCHAR));

                                    if (MultiByteToWideChar(CP_OEMCP, MB_PRECOMPOSED, desc2.c_str(), -1, desc3, ss))
                                        dev.friendly_name = desc3;

                                    free(desc3);
                                }
                            }
                        }

                        free(sdd2);
                    }

                    dlisize = 0;
                    dli = nullptr;

                    do {
                        dlisize += 1024;

                        if (dli)
                            free(dli);

                        dli = (DRIVE_LAYOUT_INFORMATION_EX*)malloc(dlisize);

                        Status = NtDeviceIoControlFile(file, nullptr, nullptr, nullptr, &iosb, IOCTL_DISK_GET_DRIVE_LAYOUT_EX,
                                                       nullptr, 0, dli, dlisize);
                    } while (Status == STATUS_BUFFER_TOO_SMALL);

                    if (NT_SUCCESS(Status) && dli->PartitionCount > 0)
                        dev.has_parts = true;

                    free(dli);
                } else {
                    try {
                        auto v = mm.query_points(L"", L"", wstring_view(path.Buffer, path.Length / sizeof(WCHAR)));

#ifndef __REACTOS__
                        for (const auto& p : v) {
                            if (p.symlink.length() == 14 && p.symlink.substr(0, dosdevices.length()) == dosdevices && p.symlink[13] == ':') {
#else
                        for(auto p = v.begin(); p != v.end(); ++p) {
                            if ((*p).symlink.length() == 14 && (*p).symlink.substr(0, dosdevices.length()) == dosdevices && (*p).symlink[13] == ':') {
#endif
                                WCHAR dr[3];

#ifndef __REACTOS__
                                dr[0] = p.symlink[12];
#else
                                dr[0] = (*p).symlink[12];
#endif
                                dr[1] = ':';
                                dr[2] = 0;

                                dev.drive = dr;
                                break;
                            }
                        }
                    } catch (...) { // don't fail entirely if mountmgr refuses to co-operate
                    }
                }

                if (!dev.is_disk || !dev.has_parts) {
                    i = 0;
                    while (fs_ident[i].name) {
                        if (i == 0 || fs_ident[i].kboff != fs_ident[i-1].kboff) {
                            LARGE_INTEGER off;

                            off.QuadPart = fs_ident[i].kboff * 1024;
                            Status = NtReadFile(file, nullptr, nullptr, nullptr, &iosb, sb, sizeof(sb), &off, nullptr);
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
                        wstring name;
                        wstring pref = L"\\Device\\Btrfs{";

                        name = get_mountdev_name(file);

                        if (name.length() > pref.length() && RtlCompareMemory(name.c_str(), pref.c_str(), pref.length() * sizeof(WCHAR)) == pref.length() * sizeof(WCHAR))
                            dev.ignore = true;
                    }
                }

                device_list.push_back(dev);
            }

            free(detail);

            index++;
        } while (SetupDiEnumDeviceInterfaces(h, nullptr, guid, index, &did));

        SetupDiDestroyDeviceInfoList(h);
    } else
        throw last_error(GetLastError());
}

#ifndef __REACTOS__ // Disabled because building with our <algorithm> seems complex right now...
static bool sort_devices(device i, device j) {
    if (i.disk_num < j.disk_num)
        return true;

    if (i.disk_num == j.disk_num && i.part_num < j.part_num)
        return true;

    return false;
}
#endif

void BtrfsDeviceAdd::populate_device_tree(HWND tree) {
    HWND hwnd = GetParent(tree);
    unsigned int i;
    ULONG last_disk_num = 0xffffffff;
    HTREEITEM diskitem;
    NTSTATUS Status;
    OBJECT_ATTRIBUTES attr;
    UNICODE_STRING us;
    IO_STATUS_BLOCK iosb;
    btrfs_filesystem* bfs = nullptr;

    static WCHAR btrfs[] = L"\\Btrfs";

    device_list.clear();

    {
        mountmgr mm;

        {
            nt_handle btrfsh;

            us.Length = us.MaximumLength = (uint16_t)(wcslen(btrfs) * sizeof(WCHAR));
            us.Buffer = btrfs;

            InitializeObjectAttributes(&attr, &us, 0, nullptr, nullptr);

            Status = NtOpenFile(&btrfsh, SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &iosb,
                                FILE_SHARE_READ | FILE_SHARE_WRITE, FILE_SYNCHRONOUS_IO_ALERT);
            if (NT_SUCCESS(Status)) {
                ULONG bfssize = 0;

                do {
                    bfssize += 1024;

                    if (bfs) free(bfs);
                    bfs = (btrfs_filesystem*)malloc(bfssize);

                    Status = NtDeviceIoControlFile(btrfsh, nullptr, nullptr, nullptr, &iosb, IOCTL_BTRFS_QUERY_FILESYSTEMS, nullptr, 0, bfs, bfssize);
                    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
                        free(bfs);
                        bfs = nullptr;
                        break;
                    }
                } while (Status == STATUS_BUFFER_OVERFLOW);

                if (bfs && bfs->num_devices == 0) { // no mounted filesystems found
                    free(bfs);
                    bfs = nullptr;
                }
            }
        }

        find_devices(hwnd, &GUID_DEVINTERFACE_DISK, mm, device_list);
        find_devices(hwnd, &GUID_DEVINTERFACE_VOLUME, mm, device_list);
        find_devices(hwnd, &GUID_DEVINTERFACE_HIDDEN_VOLUME, mm, device_list);
    }

#ifndef __REACTOS__ // Disabled because building with our <algorithm> seems complex right now...
    sort(device_list.begin(), device_list.end(), sort_devices);
#endif

    for (i = 0; i < device_list.size(); i++) {
        if (!device_list[i].ignore) {
            TVINSERTSTRUCTW tis;
            HTREEITEM item;
            wstring name, size;

            if (device_list[i].disk_num != 0xffffffff && device_list[i].disk_num == last_disk_num)
                tis.hParent = diskitem;
            else
                tis.hParent = TVI_ROOT;

            tis.hInsertAfter = TVI_LAST;
            tis.itemex.mask = TVIF_TEXT | TVIF_STATE | TVIF_PARAM;
            tis.itemex.state = TVIS_EXPANDED;
            tis.itemex.stateMask = TVIS_EXPANDED;

            if (device_list[i].disk_num != 0xffffffff) {
                wstring t;

                if (!load_string(module, device_list[i].part_num != 0 ? IDS_PARTITION : IDS_DISK_NUM, t))
                    throw last_error(GetLastError());

                wstring_sprintf(name, t, device_list[i].part_num != 0 ? device_list[i].part_num : device_list[i].disk_num);
            } else
                name = device_list[i].pnp_name;

            // match child Btrfs devices to their parent
            if (bfs && device_list[i].drive == L"" && device_list[i].fstype == L"Btrfs") {
                btrfs_filesystem* bfs2 = bfs;

                while (true) {
                    if (RtlCompareMemory(&bfs2->uuid, &device_list[i].fs_uuid, sizeof(BTRFS_UUID)) == sizeof(BTRFS_UUID)) {
                        ULONG j, k;
                        btrfs_filesystem_device* dev;

                        for (j = 0; j < bfs2->num_devices; j++) {
                            if (j == 0)
                                dev = &bfs2->device;
                            else
                                dev = (btrfs_filesystem_device*)((uint8_t*)dev + offsetof(btrfs_filesystem_device, name[0]) + dev->name_length);

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
                        bfs2 = (btrfs_filesystem*)((uint8_t*)bfs2 + bfs2->next_entry);
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

            format_size(device_list[i].size, size, false);
            name += size;

            name += L")";

            tis.itemex.pszText = (WCHAR*)name.c_str();
            tis.itemex.cchTextMax = (int)name.length();
            tis.itemex.lParam = (LPARAM)&device_list[i];

            item = (HTREEITEM)SendMessageW(tree, TVM_INSERTITEMW, 0, (LPARAM)&tis);
            if (!item)
                throw string_error(IDS_TVM_INSERTITEM_FAILED);

            if (device_list[i].part_num == 0) {
                diskitem = item;
                last_disk_num = device_list[i].disk_num;
            }
        }
    }
}

void BtrfsDeviceAdd::AddDevice(HWND hwndDlg) {
    wstring mess, title;
    NTSTATUS Status;
    UNICODE_STRING vn;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;

    if (!sel) {
        EndDialog(hwndDlg, 0);
        return;
    }

    if (sel->fstype != L"") {
        wstring s;

        if (!load_string(module, IDS_ADD_DEVICE_CONFIRMATION_FS, s))
            throw last_error(GetLastError());

        wstring_sprintf(mess, s, sel->fstype.c_str());
    } else {
        if (!load_string(module, IDS_ADD_DEVICE_CONFIRMATION, mess))
            throw last_error(GetLastError());
    }

    if (!load_string(module, IDS_CONFIRMATION_TITLE, title))
        throw last_error(GetLastError());

    if (MessageBoxW(hwndDlg, mess.c_str(), title.c_str(), MB_YESNO) != IDYES)
        return;

    win_handle h = CreateFileW(cmdline, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h == INVALID_HANDLE_VALUE)
        throw last_error(GetLastError());

    {
        nt_handle h2;

        vn.Length = vn.MaximumLength = (uint16_t)(sel->pnp_name.length() * sizeof(WCHAR));
        vn.Buffer = (WCHAR*)sel->pnp_name.c_str();

        InitializeObjectAttributes(&attr, &vn, OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE, nullptr, nullptr);

        Status = NtOpenFile(&h2, FILE_GENERIC_READ | FILE_GENERIC_WRITE, &attr, &iosb, FILE_SHARE_READ, FILE_SYNCHRONOUS_IO_ALERT);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        if (!sel->is_disk) {
            Status = NtFsControlFile(h2, nullptr, nullptr, nullptr, &iosb, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0);
            if (!NT_SUCCESS(Status))
                throw string_error(IDS_LOCK_FAILED, Status);
        }

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_ADD_DEVICE, &h2, sizeof(HANDLE), nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        if (!sel->is_disk) {
            Status = NtFsControlFile(h2, nullptr, nullptr, nullptr, &iosb, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0);
            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            Status = NtFsControlFile(h2, nullptr, nullptr, nullptr, &iosb, FSCTL_UNLOCK_VOLUME, nullptr, 0, nullptr, 0);
            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        }
    }

    EndDialog(hwndDlg, 0);
}

INT_PTR CALLBACK BtrfsDeviceAdd::DeviceAddDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);
                populate_device_tree(GetDlgItem(hwndDlg, IDC_DEVICE_TREE));
                EnableWindow(GetDlgItem(hwndDlg, IDOK), false);
                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                                AddDevice(hwndDlg);
                            return true;

                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                            return true;
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
                        bool enable = false;

                        RtlZeroMemory(&tvi, sizeof(TVITEMW));
                        tvi.hItem = nmtv->itemNew.hItem;
                        tvi.mask = TVIF_PARAM | TVIF_HANDLE;

                        if (SendMessageW(GetDlgItem(hwndDlg, IDC_DEVICE_TREE), TVM_GETITEMW, 0, (LPARAM)&tvi))
                            sel = tvi.lParam == 0 ? nullptr : (device*)tvi.lParam;
                        else
                            sel = nullptr;

                        if (sel)
                            enable = (!sel->is_disk || !sel->has_parts) && !sel->multi_device;

                        EnableWindow(GetDlgItem(hwndDlg, IDOK), enable);
                        break;
                    }
                }
            break;
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_DeviceAddDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsDeviceAdd* bda;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bda = (BtrfsDeviceAdd*)lParam;
    } else {
        bda = (BtrfsDeviceAdd*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    }

    if (bda)
        return bda->DeviceAddDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsDeviceAdd::ShowDialog() {
    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_DEVICE_ADD), hwnd, stub_DeviceAddDlgProc, (LPARAM)this);
}

BtrfsDeviceAdd::BtrfsDeviceAdd(HINSTANCE hinst, HWND hwnd, WCHAR* cmdline) {
    this->hinst = hinst;
    this->hwnd = hwnd;
    this->cmdline = cmdline;

    sel = nullptr;
}

void BtrfsDeviceResize::do_resize(HWND hwndDlg) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_resize br;

    {
        win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        br.device = dev_id;
        br.size = new_size;

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RESIZE, &br, sizeof(btrfs_resize), nullptr, 0);

        if (Status != STATUS_MORE_PROCESSING_REQUIRED && !NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    }

    if (Status != STATUS_MORE_PROCESSING_REQUIRED) {
        wstring s, t, u;

        load_string(module, IDS_RESIZE_SUCCESSFUL, s);
        format_size(new_size, u, true);
        wstring_sprintf(t, s, dev_id, u.c_str());
        MessageBoxW(hwndDlg, t.c_str(), L"", MB_OK);

        EndDialog(hwndDlg, 0);
    } else {
        HWND par;

        par = GetParent(hwndDlg);
        EndDialog(hwndDlg, 0);

        BtrfsBalance bb(fn, false, true);
        bb.ShowBalance(par);
    }
}

INT_PTR CALLBACK BtrfsDeviceResize::DeviceResizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                win_handle h;
                WCHAR s[255];
                wstring t, u;

                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                GetDlgItemTextW(hwndDlg, IDC_RESIZE_DEVICE_ID, s, sizeof(s) / sizeof(WCHAR));
                wstring_sprintf(t, s, dev_id);
                SetDlgItemTextW(hwndDlg, IDC_RESIZE_DEVICE_ID, t.c_str());

                h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

                if (h != INVALID_HANDLE_VALUE) {
                    NTSTATUS Status;
                    IO_STATUS_BLOCK iosb;
                    btrfs_device *devices, *bd;
                    ULONG devsize;
                    bool found = false;
                    HWND slider;

                    devsize = 1024;
                    devices = (btrfs_device*)malloc(devsize);

                    while (true) {
                        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_DEVICES, nullptr, 0, devices, devsize);
                        if (Status == STATUS_BUFFER_OVERFLOW) {
                            devsize += 1024;

                            free(devices);
                            devices = (btrfs_device*)malloc(devsize);
                        } else
                            break;
                    }

                    if (!NT_SUCCESS(Status)) {
                        free(devices);
                        return false;
                    }

                    bd = devices;

                    while (true) {
                        if (bd->dev_id == dev_id) {
                            memcpy(&dev_info, bd, sizeof(btrfs_device));
                            found = true;
                            break;
                        }

                        if (bd->next_entry > 0)
                            bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
                        else
                            break;
                    }

                    if (!found) {
                        free(devices);
                        return false;
                    }

                    free(devices);

                    GetDlgItemTextW(hwndDlg, IDC_RESIZE_CURSIZE, s, sizeof(s) / sizeof(WCHAR));
                    format_size(dev_info.size, u, true);
                    wstring_sprintf(t, s, u.c_str());
                    SetDlgItemTextW(hwndDlg, IDC_RESIZE_CURSIZE, t.c_str());

                    new_size = dev_info.size;

                    GetDlgItemTextW(hwndDlg, IDC_RESIZE_NEWSIZE, new_size_text, sizeof(new_size_text) / sizeof(WCHAR));
                    wstring_sprintf(t, new_size_text, u.c_str());
                    SetDlgItemTextW(hwndDlg, IDC_RESIZE_NEWSIZE, t.c_str());

                    slider = GetDlgItem(hwndDlg, IDC_RESIZE_SLIDER);
                    SendMessageW(slider, TBM_SETRANGEMIN, false, 0);
                    SendMessageW(slider, TBM_SETRANGEMAX, false, (LPARAM)(dev_info.max_size / 1048576));
                    SendMessageW(slider, TBM_SETPOS, true, (LPARAM)(new_size / 1048576));
                } else
                    return false;

                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                                do_resize(hwndDlg);
                                return true;

                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                                return true;
                        }
                    break;
                }
            break;

            case WM_HSCROLL:
            {
                wstring t, u;

                new_size = UInt32x32To64(SendMessageW(GetDlgItem(hwndDlg, IDC_RESIZE_SLIDER), TBM_GETPOS, 0, 0), 1048576);

                format_size(new_size, u, true);
                wstring_sprintf(t, new_size_text, u.c_str());
                SetDlgItemTextW(hwndDlg, IDC_RESIZE_NEWSIZE, t.c_str());

                EnableWindow(GetDlgItem(hwndDlg, IDOK), new_size > 0 ? true : false);

                break;
            }
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_DeviceResizeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsDeviceResize* bdr;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bdr = (BtrfsDeviceResize*)lParam;
    } else
        bdr = (BtrfsDeviceResize*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    if (bdr)
        return bdr->DeviceResizeDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsDeviceResize::ShowDialog(HWND hwnd, const wstring& fn, uint64_t dev_id) {
    this->dev_id = dev_id;
    this->fn = fn;

    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_RESIZE), hwnd, stub_DeviceResizeDlgProc, (LPARAM)this);
}

#ifdef __cplusplus
extern "C" {
#endif

void CALLBACK AddDeviceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        win_handle token;
        TOKEN_PRIVILEGES tp;
        LUID luid;

        set_dpi_aware();

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        BtrfsDeviceAdd bda(hinst, hwnd, lpszCmdLine);
        bda.ShowDialog();
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

void CALLBACK RemoveDeviceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        WCHAR *s, *vol, *dev;
        uint64_t devid;
        win_handle h, token;
        TOKEN_PRIVILEGES tp;
        LUID luid;
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        set_dpi_aware();

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        s = wcsstr(lpszCmdLine, L"|");
        if (!s)
            return;

        s[0] = 0;

        vol = lpszCmdLine;
        dev = &s[1];

        devid = _wtoi(dev);
        if (devid == 0)
            return;

        h = CreateFileW(vol, FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_REMOVE_DEVICE, &devid, sizeof(uint64_t), nullptr, 0);
        if (!NT_SUCCESS(Status)) {
            if (Status == STATUS_CANNOT_DELETE)
                throw string_error(IDS_CANNOT_REMOVE_RAID);
            else
                throw ntstatus_error(Status);

            return;
        }

        BtrfsBalance bb(vol, true);
        bb.ShowBalance(hwnd);
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

void CALLBACK ResizeDeviceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        WCHAR *s, *vol, *dev;
        uint64_t devid;
        win_handle token;
        TOKEN_PRIVILEGES tp;
        LUID luid;

        set_dpi_aware();

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        s = wcsstr(lpszCmdLine, L"|");
        if (!s)
            return;

        s[0] = 0;

        vol = lpszCmdLine;
        dev = &s[1];

        devid = _wtoi(dev);
        if (devid == 0)
            return;

        BtrfsDeviceResize bdr;
        bdr.ShowDialog(hwnd, vol, devid);
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

#ifdef __cplusplus
}
#endif
