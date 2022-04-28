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

#include "volpropsheet.h"
#include "resource.h"
#ifndef __REACTOS__
#include "mountmgr.h"
#else
#include "mountmgr_local.h"
#endif

#ifndef __REACTOS__
static const NTSTATUS STATUS_OBJECT_NAME_NOT_FOUND = 0xC0000034;
#endif

HRESULT __stdcall BtrfsVolPropSheet::QueryInterface(REFIID riid, void **ppObj) {
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

HRESULT __stdcall BtrfsVolPropSheet::Initialize(PCIDLIST_ABSOLUTE pidlFolder, IDataObject* pdtobj, HKEY hkeyProgID) {
    ULONG num_files;
    FORMATETC format = { CF_HDROP, nullptr, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
    HDROP hdrop;
    WCHAR fnbuf[MAX_PATH];

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

    num_files = DragQueryFileW((HDROP)stgm.hGlobal, 0xFFFFFFFF, nullptr, 0);

    if (num_files > 1) {
        GlobalUnlock(hdrop);
        return E_FAIL;
    }

    if (DragQueryFileW((HDROP)stgm.hGlobal, 0, fnbuf, sizeof(fnbuf) / sizeof(WCHAR))) {
        fn = fnbuf;

        win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h != INVALID_HANDLE_VALUE) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;
            ULONG devsize, i;

            i = 0;
            devsize = 1024;

            devices = (btrfs_device*)malloc(devsize);

            while (true) {
                Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_DEVICES, nullptr, 0, devices, devsize);
                if (Status == STATUS_BUFFER_OVERFLOW) {
                    if (i < 8) {
                        devsize += 1024;

                        free(devices);
                        devices = (btrfs_device*)malloc(devsize);

                        i++;
                    } else {
                        GlobalUnlock(hdrop);
                        return E_FAIL;
                    }
                } else
                    break;
            }

            if (!NT_SUCCESS(Status)) {
                GlobalUnlock(hdrop);
                return E_FAIL;
            }

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_UUID, nullptr, 0, &uuid, sizeof(BTRFS_UUID));
            uuid_set = NT_SUCCESS(Status);

            ignore = false;
            balance = new BtrfsBalance(fn);
        } else {
            GlobalUnlock(hdrop);
            return E_FAIL;
        }
    } else {
        GlobalUnlock(hdrop);
        return E_FAIL;
    }

    GlobalUnlock(hdrop);

    return S_OK;
}

typedef struct {
    uint64_t dev_id;
    wstring name;
    uint64_t alloc;
    uint64_t size;
} dev;

void BtrfsVolPropSheet::FormatUsage(HWND hwndDlg, wstring& s, btrfs_usage* usage) {
    uint8_t i, j;
    uint64_t num_devs, dev_size, dev_alloc, data_size, data_alloc, metadata_size, metadata_alloc;
    btrfs_device* bd;
    vector<dev> devs;
    btrfs_usage* bue;
    wstring t, u, v;

    static const uint64_t types[] = { BLOCK_FLAG_DATA, BLOCK_FLAG_DATA | BLOCK_FLAG_METADATA, BLOCK_FLAG_METADATA, BLOCK_FLAG_SYSTEM };
    static const ULONG typestrings[] = { IDS_USAGE_DATA, IDS_USAGE_MIXED, IDS_USAGE_METADATA, IDS_USAGE_SYSTEM };
    static const uint64_t duptypes[] = { 0, BLOCK_FLAG_DUPLICATE, BLOCK_FLAG_RAID0, BLOCK_FLAG_RAID1, BLOCK_FLAG_RAID10, BLOCK_FLAG_RAID5,
                                         BLOCK_FLAG_RAID6, BLOCK_FLAG_RAID1C3, BLOCK_FLAG_RAID1C4 };
    static const ULONG dupstrings[] = { IDS_SINGLE, IDS_DUP, IDS_RAID0, IDS_RAID1, IDS_RAID10, IDS_RAID5, IDS_RAID6, IDS_RAID1C3, IDS_RAID1C4 };

    static const uint64_t raid_types = BLOCK_FLAG_DUPLICATE | BLOCK_FLAG_RAID0 | BLOCK_FLAG_RAID1 | BLOCK_FLAG_RAID10 | BLOCK_FLAG_RAID5 |
                                       BLOCK_FLAG_RAID6 | BLOCK_FLAG_RAID1C3 | BLOCK_FLAG_RAID1C4;

    s = L"";

    num_devs = 0;
    bd = devices;

    while (true) {
        num_devs++;

        if (bd->next_entry > 0)
            bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
        else
            break;
    }

    bd = devices;

    dev_size = 0;

    while (true) {
        dev d;

        if (bd->missing) {
            if (!load_string(module, IDS_MISSING, d.name))
                throw last_error(GetLastError());
        } else if (bd->device_number == 0xffffffff)
            d.name = wstring(bd->name, bd->namelen / sizeof(WCHAR));
        else if (bd->partition_number == 0) {
            if (!load_string(module, IDS_DISK_NUM, u))
                throw last_error(GetLastError());

            wstring_sprintf(d.name, u, bd->device_number);
        } else {
            if (!load_string(module, IDS_DISK_PART_NUM, u))
                throw last_error(GetLastError());

            wstring_sprintf(d.name, u, bd->device_number, bd->partition_number);
        }

        d.dev_id = bd->dev_id;
        d.alloc = 0;
        d.size = bd->size;

        devs.push_back(d);

        dev_size += bd->size;

        if (bd->next_entry > 0)
            bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
        else
            break;
    }

    dev_alloc = 0;
    data_size = data_alloc = 0;
    metadata_size = metadata_alloc = 0;

    bue = usage;
    while (true) {
        for (uint64_t k = 0; k < bue->num_devices; k++) {
            dev_alloc += bue->devices[k].alloc;

            if (bue->type & BLOCK_FLAG_DATA) {
                data_alloc += bue->devices[k].alloc;
            }

            if (bue->type & BLOCK_FLAG_METADATA) {
                metadata_alloc += bue->devices[k].alloc;
            }
        }

        if (bue->type & BLOCK_FLAG_DATA)
            data_size += bue->size;

        if (bue->type & BLOCK_FLAG_METADATA)
            metadata_size += bue->size;

        if (bue->next_entry > 0)
            bue = (btrfs_usage*)((uint8_t*)bue + bue->next_entry);
        else
            break;
    }

    // device size

    if (!load_string(module, IDS_USAGE_DEV_SIZE, u))
        throw last_error(GetLastError());

    format_size(dev_size, v, false);

    wstring_sprintf(t, u, v.c_str());

    s += t + L"\r\n";

    // device allocated

    if (!load_string(module, IDS_USAGE_DEV_ALLOC, u))
        throw last_error(GetLastError());

    format_size(dev_alloc, v, false);

    wstring_sprintf(t, u, v.c_str());

#ifndef __REACTOS__
    s += t + L"\r\n"s;
#else
    s += t + L"\r\n";
#endif

    // device unallocated

    if (!load_string(module, IDS_USAGE_DEV_UNALLOC, u))
        throw last_error(GetLastError());

    format_size(dev_size - dev_alloc, v, false);

    wstring_sprintf(t, u, v.c_str());

#ifndef __REACTOS__
    s += t + L"\r\n"s;
#else
    s += t + L"\r\n";
#endif

    // data ratio

    if (data_alloc > 0) {
        if (!load_string(module, IDS_USAGE_DATA_RATIO, u))
            throw last_error(GetLastError());

        wstring_sprintf(t, u, (float)data_alloc / (float)data_size);

#ifndef __REACTOS__
        s += t + L"\r\n"s;
#else
        s += t + L"\r\n";
#endif
    }

    // metadata ratio

    if (!load_string(module, IDS_USAGE_METADATA_RATIO, u))
        throw last_error(GetLastError());

    wstring_sprintf(t, u, (float)metadata_alloc / (float)metadata_size);

    s += t + L"\r\n\r\n";

    for (i = 0; i < sizeof(types) / sizeof(types[0]); i++) {
        for (j = 0; j < sizeof(duptypes) / sizeof(duptypes[0]); j++) {
            bue = usage;

            while (true) {
                if ((bue->type & types[i]) == types[i] && ((duptypes[j] == 0 && (bue->type & raid_types) == 0) || bue->type & duptypes[j])) {
                    wstring sizestring, usedstring, typestring, dupstring;

                    if (bue->type & BLOCK_FLAG_DATA && bue->type & BLOCK_FLAG_METADATA && (types[i] == BLOCK_FLAG_DATA || types[i] == BLOCK_FLAG_METADATA))
                        break;

                    if (!load_string(module, typestrings[i], typestring))
                        throw last_error(GetLastError());

                    if (!load_string(module, dupstrings[j], dupstring))
                        throw last_error(GetLastError());

                    format_size(bue->size, sizestring, false);
                    format_size(bue->used, usedstring, false);

                    wstring_sprintf(t, typestring, dupstring.c_str(), sizestring.c_str(), usedstring.c_str());

                    s += t + L"\r\n";

                    for (uint64_t k = 0; k < bue->num_devices; k++) {
                        bool found = false;

                        format_size(bue->devices[k].alloc, sizestring, false);

                        for (size_t l = 0; l < min((uint64_t)SIZE_MAX, num_devs); l++) {
                            if (devs[l].dev_id == bue->devices[k].dev_id) {
                                s += devs[l].name + L"\t" + sizestring + L"\r\n";

                                devs[l].alloc += bue->devices[k].alloc;

                                found = true;
                                break;
                            }
                        }

                        if (!found) {
                            if (!load_string(module, IDS_UNKNOWN_DEVICE, typestring))
                                throw last_error(GetLastError());

                            wstring_sprintf(t, typestring, bue->devices[k].dev_id);

#ifndef __REACTOS__
                            s += t + L"\t"s + sizestring + L"\r\n"s;
#else
                            s += t + L"\t" + sizestring + L"\r\n";
#endif
                        }
                    }

                    s += L"\r\n";

                    break;
                }

                if (bue->next_entry > 0)
                    bue = (btrfs_usage*)((uint8_t*)bue + bue->next_entry);
                else
                    break;
            }
        }
    }

    if (!load_string(module, IDS_USAGE_UNALLOC, t))
        throw last_error(GetLastError());

#ifndef __REACTOS__
    s += t + L"\r\n"s;
#else
    s += t + L"\r\n";
#endif

    for (size_t k = 0; k < min((uint64_t)SIZE_MAX, num_devs); k++) {
        wstring sizestring;

        format_size(devs[k].size - devs[k].alloc, sizestring, false);

        s += devs[k].name + L"\t" + sizestring + L"\r\n";
    }
}

void BtrfsVolPropSheet::RefreshUsage(HWND hwndDlg) {
    wstring s;
    btrfs_usage* usage;

    win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;
        ULONG devsize, usagesize, i;

        i = 0;
        devsize = 1024;

        devices = (btrfs_device*)malloc(devsize);

        while (true) {
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_DEVICES, nullptr, 0, devices, devsize);
            if (Status == STATUS_BUFFER_OVERFLOW) {
                if (i < 8) {
                    devsize += 1024;

                    free(devices);
                    devices = (btrfs_device*)malloc(devsize);

                    i++;
                } else
                    return;
            } else
                break;
        }

        if (!NT_SUCCESS(Status))
            return;

        i = 0;
        usagesize = 1024;

        usage = (btrfs_usage*)malloc(usagesize);

        while (true) {
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_USAGE, nullptr, 0, usage, usagesize);
            if (Status == STATUS_BUFFER_OVERFLOW) {
                if (i < 8) {
                    usagesize += 1024;

                    free(usage);
                    usage = (btrfs_usage*)malloc(usagesize);

                    i++;
                } else
                    return;
            } else
                break;
        }

        if (!NT_SUCCESS(Status)) {
            free(usage);
            return;
        }

        ignore = false;
    } else
        return;

    FormatUsage(hwndDlg, s, usage);

    SetDlgItemTextW(hwndDlg, IDC_USAGE_BOX, s.c_str());

    free(usage);
}

INT_PTR CALLBACK BtrfsVolPropSheet::UsageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                wstring s;
                int i;
                ULONG usagesize;
                NTSTATUS Status;
                IO_STATUS_BLOCK iosb;

                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

                if (h != INVALID_HANDLE_VALUE) {
                    btrfs_usage* usage;

                    i = 0;
                    usagesize = 1024;

                    usage = (btrfs_usage*)malloc(usagesize);

                    while (true) {
                        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_USAGE, nullptr, 0, usage, usagesize);
                        if (Status == STATUS_BUFFER_OVERFLOW) {
                            if (i < 8) {
                                usagesize += 1024;

                                free(usage);
                                usage = (btrfs_usage*)malloc(usagesize);

                                i++;
                            } else
                                break;
                        } else
                            break;
                    }

                    if (!NT_SUCCESS(Status)) {
                        free(usage);
                        break;
                    }

                    FormatUsage(hwndDlg, s, usage);

                    SetDlgItemTextW(hwndDlg, IDC_USAGE_BOX, s.c_str());

                    free(usage);
                }

                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                            return true;

                            case IDC_USAGE_REFRESH:
                                RefreshUsage(hwndDlg);
                            return true;
                        }
                    break;
                }
            break;
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_UsageDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsVolPropSheet* bvps;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bvps = (BtrfsVolPropSheet*)lParam;
    } else {
        bvps = (BtrfsVolPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    }

    if (bvps)
        return bvps->UsageDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsVolPropSheet::ShowUsage(HWND hwndDlg) {
   DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_VOL_USAGE), hwndDlg, stub_UsageDlgProc, (LPARAM)this);
}

static void add_lv_column(HWND list, int string, int cx) {
    LVCOLUMNW lvc;
    wstring s;

    if (!load_string(module, string, s))
        throw last_error(GetLastError());

    lvc.mask = LVCF_TEXT|LVCF_WIDTH;
    lvc.pszText = (WCHAR*)s.c_str();
    lvc.cx = cx;
    SendMessageW(list, LVM_INSERTCOLUMNW, 0, (LPARAM)&lvc);
}

static int CALLBACK lv_sort(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort) {
    if (lParam1 < lParam2)
        return -1;
    else if (lParam1 > lParam2)
        return 1;
    else
        return 0;
}

static uint64_t find_dev_alloc(uint64_t dev_id, btrfs_usage* usage) {
    btrfs_usage* bue;
    uint64_t alloc;

    alloc = 0;

    bue = usage;
    while (true) {
        uint64_t k;

        for (k = 0; k < bue->num_devices; k++) {
            if (bue->devices[k].dev_id == dev_id)
                alloc += bue->devices[k].alloc;
        }

        if (bue->next_entry > 0)
            bue = (btrfs_usage*)((uint8_t*)bue + bue->next_entry);
        else
            break;
    }

    return alloc;
}

void BtrfsVolPropSheet::RefreshDevList(HWND devlist) {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    ULONG usagesize, devsize;
    btrfs_usage* usage;
    btrfs_device* bd;
    int i;
    uint64_t num_rw_devices;
    {
        win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        i = 0;
        devsize = 1024;

        if (devices)
            free(devices);

        devices = (btrfs_device*)malloc(devsize);

        while (true) {
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_DEVICES, nullptr, 0, devices, devsize);
            if (Status == STATUS_BUFFER_OVERFLOW) {
                if (i < 8) {
                    devsize += 1024;

                    free(devices);
                    devices = (btrfs_device*)malloc(devsize);

                    i++;
                } else
                    return;
            } else
                break;
        }

        if (!NT_SUCCESS(Status))
            return;

        bd = devices;

        i = 0;
        usagesize = 1024;

        usage = (btrfs_usage*)malloc(usagesize);

        while (true) {
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_USAGE, nullptr, 0, usage, usagesize);
            if (Status == STATUS_BUFFER_OVERFLOW) {
                if (i < 8) {
                    usagesize += 1024;

                    free(usage);
                    usage = (btrfs_usage*)malloc(usagesize);

                    i++;
                } else {
                    free(usage);
                    return;
                }
            } else
                break;
        }

        if (!NT_SUCCESS(Status)) {
            free(usage);
            return;
        }
    }

    SendMessageW(devlist, LVM_DELETEALLITEMS, 0, 0);

    num_rw_devices = 0;

    i = 0;
    while (true) {
        LVITEMW lvi;
        wstring s, u;
        uint64_t alloc;

        // ID

        RtlZeroMemory(&lvi, sizeof(LVITEMW));
        lvi.mask = LVIF_TEXT | LVIF_PARAM;
        lvi.iItem = (int)SendMessageW(devlist, LVM_GETITEMCOUNT, 0, 0);
        lvi.lParam = (LPARAM)bd->dev_id;

        s = to_wstring(bd->dev_id);
        lvi.pszText = (LPWSTR)s.c_str();

        SendMessageW(devlist, LVM_INSERTITEMW, 0, (LPARAM)&lvi);

        // description

        lvi.mask = LVIF_TEXT;
        lvi.iSubItem = 1;

        if (bd->missing) {
            if (!load_string(module, IDS_MISSING, s))
                throw last_error(GetLastError());
        } else if (bd->device_number == 0xffffffff)
            s = wstring(bd->name, bd->namelen / sizeof(WCHAR));
        else if (bd->partition_number == 0) {
            if (!load_string(module, IDS_DISK_NUM, u))
                throw last_error(GetLastError());

            wstring_sprintf(s, u, bd->device_number);
        } else {
            if (!load_string(module, IDS_DISK_PART_NUM, u))
                throw last_error(GetLastError());

            wstring_sprintf(s, u, bd->device_number, bd->partition_number);
        }

        lvi.pszText = (LPWSTR)s.c_str();

        SendMessageW(devlist, LVM_SETITEMW, 0, (LPARAM)&lvi);

        // readonly

        lvi.iSubItem = 2;
        load_string(module, bd->readonly ? IDS_DEVLIST_READONLY_YES : IDS_DEVLIST_READONLY_NO, s);
        lvi.pszText = (LPWSTR)s.c_str();
        SendMessageW(devlist, LVM_SETITEMW, 0, (LPARAM)&lvi);

        if (!bd->readonly)
            num_rw_devices++;

        // size

        lvi.iSubItem = 3;
        format_size(bd->size, s, false);
        lvi.pszText = (LPWSTR)s.c_str();
        SendMessageW(devlist, LVM_SETITEMW, 0, (LPARAM)&lvi);

        // alloc

        alloc = find_dev_alloc(bd->dev_id, usage);

        lvi.iSubItem = 4;
        format_size(alloc, s, false);
        lvi.pszText = (LPWSTR)s.c_str();
        SendMessageW(devlist, LVM_SETITEMW, 0, (LPARAM)&lvi);

        // alloc %

        wstring_sprintf(s, L"%1.1f%%", (float)alloc * 100.0f / (float)bd->size);
        lvi.iSubItem = 5;
        lvi.pszText = (LPWSTR)s.c_str();
        SendMessageW(devlist, LVM_SETITEMW, 0, (LPARAM)&lvi);

        i++;

        if (bd->next_entry > 0)
            bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
        else
            break;
    }

    free(usage);

    SendMessageW(devlist, LVM_SORTITEMS, 0, (LPARAM)lv_sort);

    EnableWindow(GetDlgItem(GetParent(devlist), IDC_DEVICE_ADD), num_rw_devices > 0);
    EnableWindow(GetDlgItem(GetParent(devlist), IDC_DEVICE_REMOVE), num_rw_devices > 1);
}

void BtrfsVolPropSheet::ResetStats(HWND hwndDlg) {
    wstring t, sel;
    WCHAR modfn[MAX_PATH];
    SHELLEXECUTEINFOW sei;

    sel = to_wstring(stats_dev);

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
    t = L"\""s + modfn + L"\",ResetStats " + fn + L"|" + sel;
#else
    t = wstring(L"\"") + modfn + wstring(L"\",ResetStats ") + fn + wstring(L"|") + sel;
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

    win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;
        ULONG devsize, i;

        i = 0;
        devsize = 1024;

        free(devices);
        devices = (btrfs_device*)malloc(devsize);

        while (true) {
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_DEVICES, nullptr, 0, devices, devsize);
            if (Status == STATUS_BUFFER_OVERFLOW) {
                if (i < 8) {
                    devsize += 1024;

                    free(devices);
                    devices = (btrfs_device*)malloc(devsize);

                    i++;
                } else
                    break;
            } else
                break;
        }
    }

    EndDialog(hwndDlg, 0);
}

INT_PTR CALLBACK BtrfsVolPropSheet::StatsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                WCHAR s[255];
                wstring t;
                btrfs_device *bd, *dev = nullptr;
                int i;

                static int stat_ids[] = { IDC_WRITE_ERRS, IDC_READ_ERRS, IDC_FLUSH_ERRS, IDC_CORRUPTION_ERRS, IDC_GENERATION_ERRS };

                bd = devices;

                while (true) {
                    if (bd->dev_id == stats_dev) {
                        dev = bd;
                        break;
                    }

                    if (bd->next_entry > 0)
                        bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
                    else
                        break;
                }

                if (!dev) {
                    EndDialog(hwndDlg, 0);
                    throw string_error(IDS_CANNOT_FIND_DEVICE);
                }

                GetDlgItemTextW(hwndDlg, IDC_DEVICE_ID, s, sizeof(s) / sizeof(WCHAR));

                wstring_sprintf(t, s, dev->dev_id);

                SetDlgItemTextW(hwndDlg, IDC_DEVICE_ID, t.c_str());

                for (i = 0; i < 5; i++) {
                    GetDlgItemTextW(hwndDlg, stat_ids[i], s, sizeof(s) / sizeof(WCHAR));

                    wstring_sprintf(t, s, dev->stats[i]);

                    SetDlgItemTextW(hwndDlg, stat_ids[i], t.c_str());
                }

                SendMessageW(GetDlgItem(hwndDlg, IDC_RESET_STATS), BCM_SETSHIELD, 0, true);
                EnableWindow(GetDlgItem(hwndDlg, IDC_RESET_STATS), !readonly);

                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                            return true;

                            case IDC_RESET_STATS:
                                ResetStats(hwndDlg);
                            return true;
                        }
                    break;
                }
            break;
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_StatsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsVolPropSheet* bvps;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bvps = (BtrfsVolPropSheet*)lParam;
    } else {
        bvps = (BtrfsVolPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    }

    if (bvps)
        return bvps->StatsDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsVolPropSheet::ShowStats(HWND hwndDlg, uint64_t devid) {
    stats_dev = devid;

    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_DEVICE_STATS), hwndDlg, stub_StatsDlgProc, (LPARAM)this);
}

INT_PTR CALLBACK BtrfsVolPropSheet::DeviceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                HWND devlist;
                RECT rect;
                ULONG w;

                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                devlist = GetDlgItem(hwndDlg, IDC_DEVLIST);

                GetClientRect(devlist, &rect);
                w = rect.right - rect.left;

                add_lv_column(devlist, IDS_DEVLIST_ALLOC_PC, w * 5 / 44);
                add_lv_column(devlist, IDS_DEVLIST_ALLOC, w * 6 / 44);
                add_lv_column(devlist, IDS_DEVLIST_SIZE, w * 6 / 44);
                add_lv_column(devlist, IDS_DEVLIST_READONLY, w * 7 / 44);
                add_lv_column(devlist, IDS_DEVLIST_DESC, w * 16 / 44);
                add_lv_column(devlist, IDS_DEVLIST_ID, w * 4 / 44);

                SendMessageW(GetDlgItem(hwndDlg, IDC_DEVICE_ADD), BCM_SETSHIELD, 0, true);
                SendMessageW(GetDlgItem(hwndDlg, IDC_DEVICE_REMOVE), BCM_SETSHIELD, 0, true);
                SendMessageW(GetDlgItem(hwndDlg, IDC_DEVICE_RESIZE), BCM_SETSHIELD, 0, true);

                RefreshDevList(devlist);

                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                            case IDCANCEL:
                                KillTimer(hwndDlg, 1);
                                EndDialog(hwndDlg, 0);
                            return true;

                            case IDC_DEVICE_ADD:
                            {
                                wstring t;
                                WCHAR modfn[MAX_PATH];
                                SHELLEXECUTEINFOW sei;

                                GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
                                t = L"\""s + modfn + L"\",AddDevice "s + fn;
#else
                                t = wstring(L"\"") + modfn + wstring(L"\",AddDevice ") + fn;
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

                                RefreshDevList(GetDlgItem(hwndDlg, IDC_DEVLIST));

                                return true;
                            }

                            case IDC_DEVICE_REFRESH:
                                RefreshDevList(GetDlgItem(hwndDlg, IDC_DEVLIST));
                                return true;

                            case IDC_DEVICE_SHOW_STATS:
                            {
                                WCHAR sel[MAX_PATH];
                                HWND devlist;
                                LVITEMW lvi;

                                devlist = GetDlgItem(hwndDlg, IDC_DEVLIST);

                                auto index = SendMessageW(devlist, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

                                if (index == -1)
                                    return true;

                                RtlZeroMemory(&lvi, sizeof(LVITEMW));
                                lvi.mask = LVIF_TEXT;
                                lvi.iItem = (int)index;
                                lvi.iSubItem = 0;
                                lvi.pszText = sel;
                                lvi.cchTextMax = sizeof(sel) / sizeof(WCHAR);
                                SendMessageW(devlist, LVM_GETITEMW, 0, (LPARAM)&lvi);

                                ShowStats(hwndDlg, _wtoi(sel));
                                return true;
                            }

                            case IDC_DEVICE_REMOVE:
                            {
                                wstring t, mess, mess2, title;
                                WCHAR modfn[MAX_PATH], sel[MAX_PATH], sel2[MAX_PATH];
                                HWND devlist;
                                SHELLEXECUTEINFOW sei;
                                LVITEMW lvi;

                                devlist = GetDlgItem(hwndDlg, IDC_DEVLIST);

                                auto index = SendMessageW(devlist, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

                                if (index == -1)
                                    return true;

                                RtlZeroMemory(&lvi, sizeof(LVITEMW));
                                lvi.mask = LVIF_TEXT;
                                lvi.iItem = (int)index;
                                lvi.iSubItem = 0;
                                lvi.pszText = sel;
                                lvi.cchTextMax = sizeof(sel) / sizeof(WCHAR);
                                SendMessageW(devlist, LVM_GETITEMW, 0, (LPARAM)&lvi);

                                lvi.iSubItem = 1;
                                lvi.pszText = sel2;
                                lvi.cchTextMax = sizeof(sel2) / sizeof(WCHAR);
                                SendMessageW(devlist, LVM_GETITEMW, 0, (LPARAM)&lvi);

                                if (!load_string(module, IDS_REMOVE_DEVICE_CONFIRMATION, mess))
                                    throw last_error(GetLastError());

                                wstring_sprintf(mess2, mess, sel, sel2);

                                if (!load_string(module, IDS_CONFIRMATION_TITLE, title))
                                    throw last_error(GetLastError());

                                if (MessageBoxW(hwndDlg, mess2.c_str(), title.c_str(), MB_YESNO) != IDYES)
                                    return true;

                                GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
                                t = L"\""s + modfn + L"\",RemoveDevice "s + fn + L"|"s + sel;
#else
                                t = wstring(L"\"") + modfn + wstring(L"\",RemoveDevice ") + fn + wstring(L"|") + sel;
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

                                RefreshDevList(GetDlgItem(hwndDlg, IDC_DEVLIST));

                                return true;
                            }

                            case IDC_DEVICE_RESIZE:
                            {
                                HWND devlist;
                                LVITEMW lvi;
                                wstring t;
                                WCHAR modfn[MAX_PATH], sel[100];
                                SHELLEXECUTEINFOW sei;

                                devlist = GetDlgItem(hwndDlg, IDC_DEVLIST);

                                auto index = SendMessageW(devlist, LVM_GETNEXTITEM, -1, LVNI_SELECTED);

                                if (index == -1)
                                    return true;

                                RtlZeroMemory(&lvi, sizeof(LVITEMW));
                                lvi.mask = LVIF_TEXT;
                                lvi.iItem = (int)index;
                                lvi.iSubItem = 0;
                                lvi.pszText = sel;
                                lvi.cchTextMax = sizeof(sel) / sizeof(WCHAR);
                                SendMessageW(devlist, LVM_GETITEMW, 0, (LPARAM)&lvi);

                                GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
                                t = L"\""s + modfn + L"\",ResizeDevice "s + fn + L"|"s + sel;
#else
                                t = wstring(L"\"") + modfn + wstring(L"\",ResizeDevice ") + fn + wstring(L"|") + sel;
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

                                RefreshDevList(GetDlgItem(hwndDlg, IDC_DEVLIST));
                            }
                        }
                    break;
                }
            break;

            case WM_NOTIFY:
                switch (((LPNMHDR)lParam)->code) {
                    case LVN_ITEMCHANGED:
                    {
                        NMLISTVIEW* nmv = (NMLISTVIEW*)lParam;

                        EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_SHOW_STATS), nmv->uNewState & LVIS_SELECTED);

                        if (nmv->uNewState & LVIS_SELECTED && !readonly) {
                            HWND devlist;
                            btrfs_device* bd;
                            bool device_readonly = false;
                            LVITEMW lvi;
                            WCHAR sel[MAX_PATH];
                            uint64_t devid;

                            devlist = GetDlgItem(hwndDlg, IDC_DEVLIST);

                            RtlZeroMemory(&lvi, sizeof(LVITEMW));
                            lvi.mask = LVIF_TEXT;
                            lvi.iItem = nmv->iItem;
                            lvi.iSubItem = 0;
                            lvi.pszText = sel;
                            lvi.cchTextMax = sizeof(sel) / sizeof(WCHAR);
                            SendMessageW(devlist, LVM_GETITEMW, 0, (LPARAM)&lvi);
                            devid = _wtoi(sel);

                            bd = devices;

                            while (true) {
                                if (bd->dev_id == devid) {
                                    device_readonly = bd->readonly;
                                    break;
                                }

                                if (bd->next_entry > 0)
                                    bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
                                else
                                    break;
                            }

                            EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_RESIZE), !device_readonly);
                        } else
                            EnableWindow(GetDlgItem(hwndDlg, IDC_DEVICE_RESIZE), false);

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

static INT_PTR CALLBACK stub_DeviceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsVolPropSheet* bvps;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bvps = (BtrfsVolPropSheet*)lParam;
    } else {
        bvps = (BtrfsVolPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    }

    if (bvps)
        return bvps->DeviceDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsVolPropSheet::ShowDevices(HWND hwndDlg) {
   DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_DEVICES), hwndDlg, stub_DeviceDlgProc, (LPARAM)this);
}

void BtrfsVolPropSheet::ShowScrub(HWND hwndDlg) {
    wstring t;
    WCHAR modfn[MAX_PATH];
    SHELLEXECUTEINFOW sei;

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
    t = L"\""s + modfn + L"\",ShowScrub "s + fn;
#else
    t = wstring(L"\"") + modfn + wstring(L"\",ShowScrub ") + fn;
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
}

void BtrfsVolPropSheet::ShowChangeDriveLetter(HWND hwndDlg) {
    wstring t;
    WCHAR modfn[MAX_PATH];
    SHELLEXECUTEINFOW sei;

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
    t = L"\""s + modfn + L"\",ShowChangeDriveLetter "s + fn;
#else
    t = wstring(L"\"") + modfn + wstring(L"\",ShowChangeDriveLetter ") + fn;
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
}

static INT_PTR CALLBACK PropSheetDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                PROPSHEETPAGEW* psp = (PROPSHEETPAGEW*)lParam;
                BtrfsVolPropSheet* bps = (BtrfsVolPropSheet*)psp->lParam;
                btrfs_device* bd;

                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)bps);

                bps->readonly = true;
                bd = bps->devices;

                while (true) {
                    if (!bd->readonly) {
                        bps->readonly = false;
                        break;
                    }

                    if (bd->next_entry > 0)
                        bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
                    else
                        break;
                }

                if (bps->uuid_set) {
                    WCHAR s[255];
                    wstring t;

                    GetDlgItemTextW(hwndDlg, IDC_UUID, s, sizeof(s) / sizeof(WCHAR));

                    wstring_sprintf(t, s, bps->uuid.uuid[0], bps->uuid.uuid[1], bps->uuid.uuid[2], bps->uuid.uuid[3], bps->uuid.uuid[4], bps->uuid.uuid[5],
                                    bps->uuid.uuid[6], bps->uuid.uuid[7], bps->uuid.uuid[8], bps->uuid.uuid[9], bps->uuid.uuid[10], bps->uuid.uuid[11],
                                    bps->uuid.uuid[12], bps->uuid.uuid[13], bps->uuid.uuid[14], bps->uuid.uuid[15]);

                    SetDlgItemTextW(hwndDlg, IDC_UUID, t.c_str());
                } else
                    SetDlgItemTextW(hwndDlg, IDC_UUID, L"");

                SendMessageW(GetDlgItem(hwndDlg, IDC_VOL_SCRUB), BCM_SETSHIELD, 0, true);
                SendMessageW(GetDlgItem(hwndDlg, IDC_VOL_CHANGE_DRIVE_LETTER), BCM_SETSHIELD, 0, true);

                return false;
            }

            case WM_NOTIFY:
            {
                switch (((LPNMHDR)lParam)->code) {
                    case PSN_KILLACTIVE:
                        SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, false);
                    break;
                }
                break;
            }

            case WM_COMMAND:
            {
                BtrfsVolPropSheet* bps = (BtrfsVolPropSheet*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

                if (bps) {
                    switch (HIWORD(wParam)) {
                        case BN_CLICKED: {
                            switch (LOWORD(wParam)) {
                                case IDC_VOL_SHOW_USAGE:
                                    bps->ShowUsage(hwndDlg);
                                break;

                                case IDC_VOL_BALANCE:
                                    bps->balance->ShowBalance(hwndDlg);
                                break;

                                case IDC_VOL_DEVICES:
                                    bps->ShowDevices(hwndDlg);
                                break;

                                case IDC_VOL_SCRUB:
                                    bps->ShowScrub(hwndDlg);
                                break;

                                case IDC_VOL_CHANGE_DRIVE_LETTER:
                                    bps->ShowChangeDriveLetter(hwndDlg);
                                break;
                            }
                        }
                    }
                }

                break;
            }
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

HRESULT __stdcall BtrfsVolPropSheet::AddPages(LPFNADDPROPSHEETPAGE pfnAddPage, LPARAM lParam) {
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
        psp.pszTemplate = MAKEINTRESOURCEW(IDD_VOL_PROP_SHEET);
        psp.hIcon = 0;
        psp.pszTitle = MAKEINTRESOURCEW(IDS_VOL_PROP_SHEET_TITLE);
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

HRESULT __stdcall BtrfsVolPropSheet::ReplacePage(UINT uPageID, LPFNADDPROPSHEETPAGE pfnReplacePage, LPARAM lParam) {
    return S_OK;
}

void BtrfsChangeDriveLetter::do_change(HWND hwndDlg) {
    unsigned int sel = (unsigned int)SendDlgItemMessageW(hwndDlg, IDC_DRIVE_LETTER_COMBO, CB_GETCURSEL, 0, 0);

    if (sel < letters.size()) {
        wstring dd;

        if (fn.length() == 3 && fn[1] == L':' && fn[2] == L'\\') {
            dd = L"\\DosDevices\\?:";

            dd[12] = fn[0];
        } else
#ifndef __REACTOS__
            throw runtime_error("Volume path was not root of drive.");
#else
            error_message(nullptr, "Volume path was not root of drive.");
#endif

        mountmgr mm;
        wstring dev_name;

        {
            auto v = mm.query_points(dd);

            if (v.empty())
#ifndef __REACTOS__
                throw runtime_error("Error finding device name.");
#else
                error_message(nullptr, "Error finding device name.");
#endif

            dev_name = v[0].device_name;
        }

        wstring new_dd = L"\\DosDevices\\?:";
        new_dd[12] = letters[sel];

        mm.delete_points(dd);

        try {
            mm.create_point(new_dd, dev_name);
        } catch (...) {
            // if fails, try to recreate old symlink, so we're not left with no drive letter at all
            mm.create_point(dd, dev_name);
            throw;
        }
    }

    EndDialog(hwndDlg, 1);
}

INT_PTR BtrfsChangeDriveLetter::DlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                HWND cb = GetDlgItem(hwndDlg, IDC_DRIVE_LETTER_COMBO);

                SendMessageW(cb, CB_RESETCONTENT, 0, 0);

                mountmgr mm;
                wstring drv;

                drv = L"\\DosDevices\\?:";

                for (wchar_t l = 'A'; l <= 'Z'; l++) {
                    bool found = true;

                    drv[12] = l;

                    try {
                        auto v = mm.query_points(drv);

                        if (v.empty())
                            found = false;
                    } catch (const ntstatus_error& ntstatus) {
                        if (ntstatus.Status == STATUS_OBJECT_NAME_NOT_FOUND)
                            found = false;
                        else
                            throw;
                    }

                    if (!found) {
                        wstring str = L"?:";

                        str[0] = l;
                        letters.push_back(l);

                        SendMessageW(cb, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(str.c_str()));
                    }
                }

                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                                do_change(hwndDlg);
                                return true;

                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                                return true;
                        }
                        break;
                }
            break;
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR __stdcall dlg_proc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsChangeDriveLetter* bcdl;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bcdl = (BtrfsChangeDriveLetter*)lParam;
    } else
        bcdl = (BtrfsChangeDriveLetter*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    return bcdl->DlgProc(hwndDlg, uMsg, wParam, lParam);
}

void BtrfsChangeDriveLetter::show() {
    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_DRIVE_LETTER), hwnd, dlg_proc, (LPARAM)this);
}

#ifdef __cplusplus
extern "C" {
#endif

void CALLBACK ResetStatsW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        win_handle token;
        NTSTATUS Status;
        TOKEN_PRIVILEGES tp;
        LUID luid;
        uint64_t devid;
        wstring cmdline, vol, dev;
        size_t pipe;
        IO_STATUS_BLOCK iosb;

        set_dpi_aware();

        cmdline = lpszCmdLine;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        pipe = cmdline.find(L"|");

        if (pipe == string::npos)
            return;

        vol = cmdline.substr(0, pipe);
        dev = cmdline.substr(pipe + 1);

        devid = _wtoi(dev.c_str());
        if (devid == 0)
            return;

        win_handle h = CreateFileW(vol.c_str(), FILE_TRAVERSE | FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RESET_STATS, &devid, sizeof(uint64_t), nullptr, 0);
        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

void CALLBACK ShowChangeDriveLetterW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    BtrfsChangeDriveLetter bcdl(hwnd, lpszCmdLine);

    bcdl.show();
}

#ifdef __cplusplus
}
#endif
