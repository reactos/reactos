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

#include "shellext.h"
#include "balance.h"
#include "resource.h"
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "btrfsioctl.h"
#endif
#include <shlobj.h>
#include <uxtheme.h>
#include <stdio.h>
#ifndef __REACTOS__
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

static uint64_t convtypes2[] = { BLOCK_FLAG_SINGLE, BLOCK_FLAG_DUPLICATE, BLOCK_FLAG_RAID0, BLOCK_FLAG_RAID1,
                                 BLOCK_FLAG_RAID5, BLOCK_FLAG_RAID1C3, BLOCK_FLAG_RAID6, BLOCK_FLAG_RAID10,
                                 BLOCK_FLAG_RAID1C4 };

static WCHAR hex_digit(uint8_t u) {
    if (u >= 0xa && u <= 0xf)
        return (uint8_t)(u - 0xa + 'a');
    else
        return (uint8_t)(u + '0');
}

static void serialize(void* data, ULONG len, WCHAR* s) {
    uint8_t* d;

    d = (uint8_t*)data;

    while (true) {
        *s = hex_digit((uint8_t)(*d >> 4)); s++;
        *s = hex_digit(*d & 0xf); s++;

        d++;
        len--;

        if (len == 0) {
            *s = 0;
            return;
        }
    }
}

void BtrfsBalance::StartBalance(HWND hwndDlg) {
    wstring t;
    WCHAR modfn[MAX_PATH], u[600];
    SHELLEXECUTEINFOW sei;
    btrfs_start_balance bsb;

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
    t = L"\""s + modfn + L"\",StartBalance "s + fn + L" "s;
#else
    t = wstring(L"\"") + modfn + wstring(L"\",StartBalance ") + fn + wstring(L" ");
#endif

    RtlCopyMemory(&bsb.opts[0], &data_opts, sizeof(btrfs_balance_opts));
    RtlCopyMemory(&bsb.opts[1], &metadata_opts, sizeof(btrfs_balance_opts));
    RtlCopyMemory(&bsb.opts[2], &system_opts, sizeof(btrfs_balance_opts));

    if (IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED)
        bsb.opts[0].flags |= BTRFS_BALANCE_OPTS_ENABLED;
    else
        bsb.opts[0].flags &= ~BTRFS_BALANCE_OPTS_ENABLED;

    if (IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED)
        bsb.opts[1].flags |= BTRFS_BALANCE_OPTS_ENABLED;
    else
        bsb.opts[1].flags &= ~BTRFS_BALANCE_OPTS_ENABLED;

    if (IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED)
        bsb.opts[2].flags |= BTRFS_BALANCE_OPTS_ENABLED;
    else
        bsb.opts[2].flags &= ~BTRFS_BALANCE_OPTS_ENABLED;

    serialize(&bsb, sizeof(btrfs_start_balance), u);

    t += u;

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

    cancelling = false;
    removing = false;
    shrinking = false;
    balance_status = BTRFS_BALANCE_RUNNING;

    EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_BALANCE), true);
    EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_BALANCE), true);
    EnableWindow(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), true);
    EnableWindow(GetDlgItem(hwndDlg, IDC_DATA), false);
    EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA), false);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM), false);
    EnableWindow(GetDlgItem(hwndDlg, IDC_DATA_OPTIONS), data_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? true : false);
    EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA_OPTIONS), metadata_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? true : false);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM_OPTIONS), system_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? true : false);

    EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), false);

    WaitForSingleObject(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);
}

void BtrfsBalance::PauseBalance(HWND hwndDlg) {
    WCHAR modfn[MAX_PATH];
    wstring t;
    SHELLEXECUTEINFOW sei;

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
    t = L"\""s + modfn + L"\",PauseBalance " + fn;
#else
    t = wstring(L"\"") + modfn + wstring(L"\",PauseBalance ") + fn;
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

void BtrfsBalance::StopBalance(HWND hwndDlg) {
    WCHAR modfn[MAX_PATH];
    wstring t;
    SHELLEXECUTEINFOW sei;

    GetModuleFileNameW(module, modfn, sizeof(modfn) / sizeof(WCHAR));

#ifndef __REACTOS__
    t = L"\""s + modfn + L"\",StopBalance " + fn;
#else
    t = wstring(L"\"") + modfn + wstring(L"\",StopBalance ") + fn;
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

    cancelling = true;

    WaitForSingleObject(sei.hProcess, INFINITE);
    CloseHandle(sei.hProcess);
}

void BtrfsBalance::RefreshBalanceDlg(HWND hwndDlg, bool first) {
    bool balancing = false;
    wstring s, t;

    {
        win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h != INVALID_HANDLE_VALUE) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_BALANCE, nullptr, 0, &bqb, sizeof(btrfs_query_balance));

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        } else
            throw last_error(GetLastError());
    }

    if (cancelling)
        bqb.status = BTRFS_BALANCE_STOPPED;

    balancing = bqb.status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED);

    if (!balancing) {
        if (first || balance_status != BTRFS_BALANCE_STOPPED) {
            int resid;

            EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_BALANCE), false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_BALANCE), false);
            SendMessageW(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), PBM_SETSTATE, PBST_NORMAL, 0);
            EnableWindow(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_DATA), true);
            EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA), true);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM), true);

            if (balance_status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED)) {
                CheckDlgButton(hwndDlg, IDC_DATA, BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_METADATA, BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SYSTEM, BST_UNCHECKED);

                SendMessageW(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), PBM_SETPOS, 0, 0);
            }

            EnableWindow(GetDlgItem(hwndDlg, IDC_DATA_OPTIONS), IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED ? true : false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA_OPTIONS), IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED ? true : false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM_OPTIONS), IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED ? true : false);

            if (bqb.status & BTRFS_BALANCE_ERROR) {
                if (removing)
                    resid = IDS_BALANCE_FAILED_REMOVAL;
                else if (shrinking)
                    resid = IDS_BALANCE_FAILED_SHRINK;
                else
                    resid = IDS_BALANCE_FAILED;

                if (!load_string(module, resid, s))
                    throw last_error(GetLastError());

                wstring_sprintf(t, s, bqb.error, format_ntstatus(bqb.error).c_str());

                SetDlgItemTextW(hwndDlg, IDC_BALANCE_STATUS, t.c_str());
            } else {
                if (cancelling)
                    resid = removing ? IDS_BALANCE_CANCELLED_REMOVAL : (shrinking ? IDS_BALANCE_CANCELLED_SHRINK : IDS_BALANCE_CANCELLED);
                else if (balance_status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED))
                    resid = removing ? IDS_BALANCE_COMPLETE_REMOVAL : (shrinking ? IDS_BALANCE_COMPLETE_SHRINK : IDS_BALANCE_COMPLETE);
                else
                    resid = IDS_NO_BALANCE;

                if (!load_string(module, resid, s))
                    throw last_error(GetLastError());

                SetDlgItemTextW(hwndDlg, IDC_BALANCE_STATUS, s.c_str());
            }

            EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), !readonly && (IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED ||
                         IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED || IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED) ? true: false);

            balance_status = bqb.status;
            cancelling = false;
        }

        return;
    }

    if (first || !(balance_status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED))) {
        EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_BALANCE), true);
        EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_BALANCE), true);
        EnableWindow(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), true);
        EnableWindow(GetDlgItem(hwndDlg, IDC_DATA), false);
        EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA), false);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM), false);

        CheckDlgButton(hwndDlg, IDC_DATA, bqb.data_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_METADATA, bqb.metadata_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? BST_CHECKED : BST_UNCHECKED);
        CheckDlgButton(hwndDlg, IDC_SYSTEM, bqb.system_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? BST_CHECKED : BST_UNCHECKED);

        EnableWindow(GetDlgItem(hwndDlg, IDC_DATA_OPTIONS), bqb.data_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? true : false);
        EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA_OPTIONS), bqb.metadata_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? true : false);
        EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM_OPTIONS), bqb.system_opts.flags & BTRFS_BALANCE_OPTS_ENABLED ? true : false);

        EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), false);
    }

    SendMessageW(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), PBM_SETRANGE32, 0, (LPARAM)bqb.total_chunks);
    SendMessageW(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), PBM_SETPOS, (WPARAM)(bqb.total_chunks - bqb.chunks_left), 0);

    if (bqb.status & BTRFS_BALANCE_PAUSED && balance_status != bqb.status)
        SendMessageW(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), PBM_SETSTATE, PBST_PAUSED, 0);
    else if (!(bqb.status & BTRFS_BALANCE_PAUSED) && balance_status & BTRFS_BALANCE_PAUSED)
        SendMessageW(GetDlgItem(hwndDlg, IDC_BALANCE_PROGRESS), PBM_SETSTATE, PBST_NORMAL, 0);

    balance_status = bqb.status;

    if (bqb.status & BTRFS_BALANCE_REMOVAL) {
        if (!load_string(module, balance_status & BTRFS_BALANCE_PAUSED ? IDS_BALANCE_PAUSED_REMOVAL : IDS_BALANCE_RUNNING_REMOVAL, s))
            throw last_error(GetLastError());

        wstring_sprintf(t, s, bqb.data_opts.devid, bqb.total_chunks - bqb.chunks_left, bqb.total_chunks,
                        (float)(bqb.total_chunks - bqb.chunks_left) * 100.0f / (float)bqb.total_chunks);

        removing = true;
        shrinking = false;
    } else if (bqb.status & BTRFS_BALANCE_SHRINKING) {
        if (!load_string(module, balance_status & BTRFS_BALANCE_PAUSED ? IDS_BALANCE_PAUSED_SHRINK : IDS_BALANCE_RUNNING_SHRINK, s))
            throw last_error(GetLastError());

        wstring_sprintf(t, s, bqb.data_opts.devid, bqb.total_chunks - bqb.chunks_left, bqb.total_chunks,
                        (float)(bqb.total_chunks - bqb.chunks_left) * 100.0f / (float)bqb.total_chunks);

        removing = false;
        shrinking = true;
    } else {
        if (!load_string(module, balance_status & BTRFS_BALANCE_PAUSED ? IDS_BALANCE_PAUSED : IDS_BALANCE_RUNNING, s))
            throw last_error(GetLastError());

        wstring_sprintf(t, s, bqb.total_chunks - bqb.chunks_left, bqb.total_chunks,
                        (float)(bqb.total_chunks - bqb.chunks_left) * 100.0f / (float)bqb.total_chunks);

        removing = false;
        shrinking = false;
    }

    SetDlgItemTextW(hwndDlg, IDC_BALANCE_STATUS, t.c_str());
}

void BtrfsBalance::SaveBalanceOpts(HWND hwndDlg) {
    btrfs_balance_opts* opts;

    switch (opts_type) {
        case 1:
            opts = &data_opts;
        break;

        case 2:
            opts = &metadata_opts;
        break;

        case 3:
            opts = &system_opts;
        break;

        default:
            return;
    }

    RtlZeroMemory(opts, sizeof(btrfs_balance_opts));

    if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES) == BST_CHECKED) {
        opts->flags |= BTRFS_BALANCE_OPTS_PROFILES;

        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_SINGLE) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_SINGLE;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_DUP) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_DUPLICATE;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID0) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID0;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID1) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID1;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID10) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID10;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID5) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID5;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID6) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID6;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID1C3) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID1C3;
        if (IsDlgButtonChecked(hwndDlg, IDC_PROFILES_RAID1C4) == BST_CHECKED) opts->profiles |= BLOCK_FLAG_RAID1C4;
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_DEVID) == BST_CHECKED) {
        opts->flags |= BTRFS_BALANCE_OPTS_DEVID;

        auto sel = SendMessageW(GetDlgItem(hwndDlg, IDC_DEVID_COMBO), CB_GETCURSEL, 0, 0);

        if (sel == CB_ERR)
            opts->flags &= ~BTRFS_BALANCE_OPTS_DEVID;
        else {
            btrfs_device* bd = devices;
            int i = 0;

            while (true) {
                if (i == sel) {
                    opts->devid = bd->dev_id;
                    break;
                }

                i++;

                if (bd->next_entry > 0)
                    bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
                else
                    break;
            }

            if (opts->devid == 0)
                opts->flags &= ~BTRFS_BALANCE_OPTS_DEVID;
        }
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_DRANGE) == BST_CHECKED) {
        WCHAR s[255];

        opts->flags |= BTRFS_BALANCE_OPTS_DRANGE;

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_DRANGE_START), s, sizeof(s) / sizeof(WCHAR));
        opts->drange_start = _wtoi64(s);

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_DRANGE_END), s, sizeof(s) / sizeof(WCHAR));
        opts->drange_end = _wtoi64(s);

        if (opts->drange_end < opts->drange_start)
            throw string_error(IDS_DRANGE_END_BEFORE_START);
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_VRANGE) == BST_CHECKED) {
        WCHAR s[255];

        opts->flags |= BTRFS_BALANCE_OPTS_VRANGE;

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_VRANGE_START), s, sizeof(s) / sizeof(WCHAR));
        opts->vrange_start = _wtoi64(s);

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_VRANGE_END), s, sizeof(s) / sizeof(WCHAR));
        opts->vrange_end = _wtoi64(s);

        if (opts->vrange_end < opts->vrange_start)
            throw string_error(IDS_VRANGE_END_BEFORE_START);
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_LIMIT) == BST_CHECKED) {
        WCHAR s[255];

        opts->flags |= BTRFS_BALANCE_OPTS_LIMIT;

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_LIMIT_START), s, sizeof(s) / sizeof(WCHAR));
        opts->limit_start = _wtoi64(s);

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_LIMIT_END), s, sizeof(s) / sizeof(WCHAR));
        opts->limit_end = _wtoi64(s);

        if (opts->limit_end < opts->limit_start)
            throw string_error(IDS_LIMIT_END_BEFORE_START);
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_STRIPES) == BST_CHECKED) {
        WCHAR s[255];

        opts->flags |= BTRFS_BALANCE_OPTS_STRIPES;

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_STRIPES_START), s, sizeof(s) / sizeof(WCHAR));
        opts->stripes_start = (uint8_t)_wtoi(s);

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_STRIPES_END), s, sizeof(s) / sizeof(WCHAR));
        opts->stripes_end = (uint8_t)_wtoi(s);

        if (opts->stripes_end < opts->stripes_start)
            throw string_error(IDS_STRIPES_END_BEFORE_START);
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_USAGE) == BST_CHECKED) {
        WCHAR s[255];

        opts->flags |= BTRFS_BALANCE_OPTS_USAGE;

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_USAGE_START), s, sizeof(s) / sizeof(WCHAR));
        opts->usage_start = (uint8_t)_wtoi(s);

        GetWindowTextW(GetDlgItem(hwndDlg, IDC_USAGE_END), s, sizeof(s) / sizeof(WCHAR));
        opts->usage_end = (uint8_t)_wtoi(s);

        if (opts->usage_end < opts->usage_start)
            throw string_error(IDS_USAGE_END_BEFORE_START);
    }

    if (IsDlgButtonChecked(hwndDlg, IDC_CONVERT) == BST_CHECKED) {
        opts->flags |= BTRFS_BALANCE_OPTS_CONVERT;

        auto sel = SendMessageW(GetDlgItem(hwndDlg, IDC_CONVERT_COMBO), CB_GETCURSEL, 0, 0);

        if (sel == CB_ERR || (unsigned int)sel >= sizeof(convtypes2) / sizeof(convtypes2[0]))
            opts->flags &= ~BTRFS_BALANCE_OPTS_CONVERT;
        else {
            opts->convert = convtypes2[sel];

            if (IsDlgButtonChecked(hwndDlg, IDC_SOFT) == BST_CHECKED) opts->flags |= BTRFS_BALANCE_OPTS_SOFT;
        }
    }

    EndDialog(hwndDlg, 0);
}

INT_PTR CALLBACK BtrfsBalance::BalanceOptsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                HWND devcb, convcb;
                btrfs_device* bd;
                btrfs_balance_opts* opts;
                static int convtypes[] = { IDS_SINGLE2, IDS_DUP, IDS_RAID0, IDS_RAID1, IDS_RAID5, IDS_RAID1C3, IDS_RAID6, IDS_RAID10, IDS_RAID1C4, 0 };
                int i, num_devices = 0, num_writeable_devices = 0;
                wstring s, u;
                bool balance_started = balance_status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED);

                switch (opts_type) {
                    case 1:
                        opts = balance_started ? &bqb.data_opts : &data_opts;
                    break;

                    case 2:
                        opts = balance_started ? &bqb.metadata_opts : &metadata_opts;
                    break;

                    case 3:
                        opts = balance_started ? &bqb.system_opts : &system_opts;
                    break;

                    default:
                        return true;
                }

                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                devcb = GetDlgItem(hwndDlg, IDC_DEVID_COMBO);

                if (!load_string(module, IDS_DEVID_LIST, u))
                    throw last_error(GetLastError());

                bd = devices;
                while (true) {
                    wstring t, v;

                    if (bd->device_number == 0xffffffff)
                        s = wstring(bd->name, bd->namelen);
                    else if (bd->partition_number == 0) {
                        if (!load_string(module, IDS_DISK_NUM, v))
                            throw last_error(GetLastError());

                        wstring_sprintf(s, v, bd->device_number);
                    } else {
                        if (!load_string(module, IDS_DISK_PART_NUM, v))
                            throw last_error(GetLastError());

                        wstring_sprintf(s, v, bd->device_number, bd->partition_number);
                    }

                    wstring_sprintf(t, u, bd->dev_id, s.c_str());

                    SendMessageW(devcb, CB_ADDSTRING, 0, (LPARAM)t.c_str());

                    if (opts->devid == bd->dev_id)
                        SendMessageW(devcb, CB_SETCURSEL, num_devices, 0);

                    num_devices++;

                    if (!bd->readonly)
                        num_writeable_devices++;

                    if (bd->next_entry > 0)
                        bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
                    else
                        break;
                }

                convcb = GetDlgItem(hwndDlg, IDC_CONVERT_COMBO);

                if (num_writeable_devices == 0)
                    num_writeable_devices = num_devices;

                i = 0;
                while (convtypes[i] != 0) {
                    if (!load_string(module, convtypes[i], s))
                        throw last_error(GetLastError());

                    SendMessageW(convcb, CB_ADDSTRING, 0, (LPARAM)s.c_str());

                    if (opts->convert == convtypes2[i])
                        SendMessageW(convcb, CB_SETCURSEL, i, 0);

                    i++;

                    if (num_writeable_devices < 2 && i == 2)
                        break;
                    else if (num_writeable_devices < 3 && i == 4)
                        break;
                    else if (num_writeable_devices < 4 && i == 6)
                        break;
                }

                // profiles

                CheckDlgButton(hwndDlg, IDC_PROFILES, opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_SINGLE, opts->profiles & BLOCK_FLAG_SINGLE ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_DUP, opts->profiles & BLOCK_FLAG_DUPLICATE ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID0, opts->profiles & BLOCK_FLAG_RAID0 ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID1, opts->profiles & BLOCK_FLAG_RAID1 ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID10, opts->profiles & BLOCK_FLAG_RAID10 ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID5, opts->profiles & BLOCK_FLAG_RAID5 ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID6, opts->profiles & BLOCK_FLAG_RAID6 ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID1C3, opts->profiles & BLOCK_FLAG_RAID1C3 ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_PROFILES_RAID1C4, opts->profiles & BLOCK_FLAG_RAID1C4 ? BST_CHECKED : BST_UNCHECKED);

                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_SINGLE), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_DUP), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID0), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID1), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID10), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID5), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID6), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID1C3), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID1C4), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_PROFILES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES), balance_started ? false : true);

                // usage

                CheckDlgButton(hwndDlg, IDC_USAGE, opts->flags & BTRFS_BALANCE_OPTS_USAGE ? BST_CHECKED : BST_UNCHECKED);

                s = to_wstring(opts->usage_start);
                SetDlgItemTextW(hwndDlg, IDC_USAGE_START, s.c_str());
                SendMessageW(GetDlgItem(hwndDlg, IDC_USAGE_START_SPINNER), UDM_SETRANGE32, 0, 100);

                s = to_wstring(opts->usage_end);
                SetDlgItemTextW(hwndDlg, IDC_USAGE_END, s.c_str());
                SendMessageW(GetDlgItem(hwndDlg, IDC_USAGE_END_SPINNER), UDM_SETRANGE32, 0, 100);

                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_START), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_USAGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_START_SPINNER), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_USAGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_END), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_USAGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_END_SPINNER), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_USAGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE), balance_started ? false : true);

                // devid

                if (num_devices < 2 || balance_started)
                    EnableWindow(GetDlgItem(hwndDlg, IDC_DEVID), false);

                CheckDlgButton(hwndDlg, IDC_DEVID, opts->flags & BTRFS_BALANCE_OPTS_DEVID ? BST_CHECKED : BST_UNCHECKED);
                EnableWindow(devcb, (opts->flags & BTRFS_BALANCE_OPTS_DEVID && num_devices >= 2 && !balance_started) ? true : false);

                // drange

                CheckDlgButton(hwndDlg, IDC_DRANGE, opts->flags & BTRFS_BALANCE_OPTS_DRANGE ? BST_CHECKED : BST_UNCHECKED);

                s = to_wstring(opts->drange_start);
                SetDlgItemTextW(hwndDlg, IDC_DRANGE_START, s.c_str());

                s = to_wstring(opts->drange_end);
                SetDlgItemTextW(hwndDlg, IDC_DRANGE_END, s.c_str());

                EnableWindow(GetDlgItem(hwndDlg, IDC_DRANGE_START), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_DRANGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DRANGE_END), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_DRANGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_DRANGE), balance_started ? false : true);

                // vrange

                CheckDlgButton(hwndDlg, IDC_VRANGE, opts->flags & BTRFS_BALANCE_OPTS_VRANGE ? BST_CHECKED : BST_UNCHECKED);

                s = to_wstring(opts->vrange_start);
                SetDlgItemTextW(hwndDlg, IDC_VRANGE_START, s.c_str());

                s = to_wstring(opts->vrange_end);
                SetDlgItemTextW(hwndDlg, IDC_VRANGE_END, s.c_str());

                EnableWindow(GetDlgItem(hwndDlg, IDC_VRANGE_START), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_VRANGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VRANGE_END), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_VRANGE ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_VRANGE), balance_started ? false : true);

                // limit

                CheckDlgButton(hwndDlg, IDC_LIMIT, opts->flags & BTRFS_BALANCE_OPTS_LIMIT ? BST_CHECKED : BST_UNCHECKED);

                s = to_wstring(opts->limit_start);
                SetDlgItemTextW(hwndDlg, IDC_LIMIT_START, s.c_str());
                SendMessageW(GetDlgItem(hwndDlg, IDC_LIMIT_START_SPINNER), UDM_SETRANGE32, 0, 0x7fffffff);

                s = to_wstring(opts->limit_end);
                SetDlgItemTextW(hwndDlg, IDC_LIMIT_END, s.c_str());
                SendMessageW(GetDlgItem(hwndDlg, IDC_LIMIT_END_SPINNER), UDM_SETRANGE32, 0, 0x7fffffff);

                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_START), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_LIMIT ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_START_SPINNER), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_LIMIT ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_END), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_LIMIT ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_END_SPINNER), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_LIMIT ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT), balance_started ? false : true);

                // stripes

                CheckDlgButton(hwndDlg, IDC_STRIPES, opts->flags & BTRFS_BALANCE_OPTS_STRIPES ? BST_CHECKED : BST_UNCHECKED);

                s = to_wstring(opts->stripes_start);
                SetDlgItemTextW(hwndDlg, IDC_STRIPES_START, s.c_str());
                SendMessageW(GetDlgItem(hwndDlg, IDC_STRIPES_START_SPINNER), UDM_SETRANGE32, 0, 0xffff);

                s = to_wstring(opts->stripes_end);
                SetDlgItemTextW(hwndDlg, IDC_STRIPES_END, s.c_str());
                SendMessageW(GetDlgItem(hwndDlg, IDC_STRIPES_END_SPINNER), UDM_SETRANGE32, 0, 0xffff);

                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_START), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_STRIPES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_START_SPINNER), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_STRIPES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_END), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_STRIPES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_END_SPINNER), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_STRIPES ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES), balance_started ? false : true);

                // convert

                CheckDlgButton(hwndDlg, IDC_CONVERT, opts->flags & BTRFS_BALANCE_OPTS_CONVERT ? BST_CHECKED : BST_UNCHECKED);
                CheckDlgButton(hwndDlg, IDC_SOFT, opts->flags & BTRFS_BALANCE_OPTS_SOFT ? BST_CHECKED : BST_UNCHECKED);

                EnableWindow(GetDlgItem(hwndDlg, IDC_SOFT), !balance_started && opts->flags & BTRFS_BALANCE_OPTS_CONVERT ? true : false);
                EnableWindow(convcb, !balance_started && opts->flags & BTRFS_BALANCE_OPTS_CONVERT ? true : false);
                EnableWindow(GetDlgItem(hwndDlg, IDC_CONVERT), balance_started ? false : true);

                break;
            }

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                                if (balance_status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED))
                                    EndDialog(hwndDlg, 0);
                                else
                                    SaveBalanceOpts(hwndDlg);
                            return true;

                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                            return true;

                            case IDC_PROFILES: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_PROFILES) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_SINGLE), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_DUP), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID0), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID1), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID10), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID5), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID6), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID1C3), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_PROFILES_RAID1C4), enabled);
                                break;
                            }

                            case IDC_USAGE: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_USAGE) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_START), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_START_SPINNER), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_END), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_USAGE_END_SPINNER), enabled);
                                break;
                            }

                            case IDC_DEVID: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_DEVID) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_DEVID_COMBO), enabled);
                                break;
                            }

                            case IDC_DRANGE: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_DRANGE) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_DRANGE_START), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_DRANGE_END), enabled);
                                break;
                            }

                            case IDC_VRANGE: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_VRANGE) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_VRANGE_START), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_VRANGE_END), enabled);
                                break;
                            }

                            case IDC_LIMIT: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_LIMIT) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_START), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_START_SPINNER), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_END), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_LIMIT_END_SPINNER), enabled);
                                break;
                            }

                            case IDC_STRIPES: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_STRIPES) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_START), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_START_SPINNER), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_END), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_STRIPES_END_SPINNER), enabled);
                                break;
                            }

                            case IDC_CONVERT: {
                                bool enabled = IsDlgButtonChecked(hwndDlg, IDC_CONVERT) == BST_CHECKED ? true : false;

                                EnableWindow(GetDlgItem(hwndDlg, IDC_CONVERT_COMBO), enabled);
                                EnableWindow(GetDlgItem(hwndDlg, IDC_SOFT), enabled);
                                break;
                            }
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

static INT_PTR CALLBACK stub_BalanceOptsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsBalance* bb;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bb = (BtrfsBalance*)lParam;
    } else {
        bb = (BtrfsBalance*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    }

    if (bb)
        return bb->BalanceOptsDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsBalance::ShowBalanceOptions(HWND hwndDlg, uint8_t type) {
    opts_type = type;
    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_BALANCE_OPTIONS), hwndDlg, stub_BalanceOptsDlgProc, (LPARAM)this);
}

INT_PTR CALLBACK BtrfsBalance::BalanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
            {
                EnableThemeDialogTexture(hwndDlg, ETDT_ENABLETAB);

                RtlZeroMemory(&data_opts, sizeof(btrfs_balance_opts));
                RtlZeroMemory(&metadata_opts, sizeof(btrfs_balance_opts));
                RtlZeroMemory(&system_opts, sizeof(btrfs_balance_opts));

                removing = called_from_RemoveDevice;
                shrinking = called_from_ShrinkDevice;
                balance_status = (removing || shrinking) ? BTRFS_BALANCE_RUNNING : BTRFS_BALANCE_STOPPED;
                cancelling = false;
                RefreshBalanceDlg(hwndDlg, true);

                if (readonly) {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_BALANCE), false);
                    EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_BALANCE), false);
                }

                SendMessageW(GetDlgItem(hwndDlg, IDC_START_BALANCE), BCM_SETSHIELD, 0, true);
                SendMessageW(GetDlgItem(hwndDlg, IDC_PAUSE_BALANCE), BCM_SETSHIELD, 0, true);
                SendMessageW(GetDlgItem(hwndDlg, IDC_CANCEL_BALANCE), BCM_SETSHIELD, 0, true);

                SetTimer(hwndDlg, 1, 1000, nullptr);

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

                            case IDC_DATA:
                                EnableWindow(GetDlgItem(hwndDlg, IDC_DATA_OPTIONS), IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED ? true : false);

                                EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), !readonly && (IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED ||
                                IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED || IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED) ? true: false);
                            return true;

                            case IDC_METADATA:
                                EnableWindow(GetDlgItem(hwndDlg, IDC_METADATA_OPTIONS), IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED ? true : false);

                                EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), !readonly && (IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED ||
                                IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED || IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED) ? true: false);
                            return true;

                            case IDC_SYSTEM:
                                EnableWindow(GetDlgItem(hwndDlg, IDC_SYSTEM_OPTIONS), IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED ? true : false);

                                EnableWindow(GetDlgItem(hwndDlg, IDC_START_BALANCE), !readonly && (IsDlgButtonChecked(hwndDlg, IDC_DATA) == BST_CHECKED ||
                                IsDlgButtonChecked(hwndDlg, IDC_METADATA) == BST_CHECKED || IsDlgButtonChecked(hwndDlg, IDC_SYSTEM) == BST_CHECKED) ? true: false);
                            return true;

                            case IDC_DATA_OPTIONS:
                                ShowBalanceOptions(hwndDlg, 1);
                            return true;

                            case IDC_METADATA_OPTIONS:
                                ShowBalanceOptions(hwndDlg, 2);
                            return true;

                            case IDC_SYSTEM_OPTIONS:
                                ShowBalanceOptions(hwndDlg, 3);
                            return true;

                            case IDC_START_BALANCE:
                                StartBalance(hwndDlg);
                            return true;

                            case IDC_PAUSE_BALANCE:
                                PauseBalance(hwndDlg);
                                RefreshBalanceDlg(hwndDlg, false);
                            return true;

                            case IDC_CANCEL_BALANCE:
                                StopBalance(hwndDlg);
                                RefreshBalanceDlg(hwndDlg, false);
                            return true;
                        }
                    break;
                }
            break;

            case WM_TIMER:
                RefreshBalanceDlg(hwndDlg, false);
                break;
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_BalanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsBalance* bb;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bb = (BtrfsBalance*)lParam;
    } else {
        bb = (BtrfsBalance*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);
    }

    if (bb)
        return bb->BalanceDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsBalance::ShowBalance(HWND hwndDlg) {
    btrfs_device* bd;

    if (devices) {
        free(devices);
        devices = nullptr;
    }

    {
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
                    } else
                        return;
                } else
                    break;
            }

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        } else
            throw last_error(GetLastError());
    }

    readonly = true;
    bd = devices;

    while (true) {
        if (!bd->readonly) {
            readonly = false;
            break;
        }

        if (bd->next_entry > 0)
            bd = (btrfs_device*)((uint8_t*)bd + bd->next_entry);
        else
            break;
    }

    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_BALANCE), hwndDlg, stub_BalanceDlgProc, (LPARAM)this);
}

static uint8_t from_hex_digit(WCHAR c) {
    if (c >= 'a' && c <= 'f')
        return (uint8_t)(c - 'a' + 0xa);
    else if (c >= 'A' && c <= 'F')
        return (uint8_t)(c - 'A' + 0xa);
    else
        return (uint8_t)(c - '0');
}

static void unserialize(void* data, ULONG len, WCHAR* s) {
    uint8_t* d;

    d = (uint8_t*)data;

    while (s[0] != 0 && s[1] != 0 && len > 0) {
        *d = (uint8_t)(from_hex_digit(s[0]) << 4) | from_hex_digit(s[1]);

        s += 2;
        d++;
        len--;
    }
}

extern "C" void CALLBACK StartBalanceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        WCHAR *s, *vol, *block;
        win_handle h, token;
        btrfs_start_balance bsb;
        TOKEN_PRIVILEGES tp;
        LUID luid;

        s = wcsstr(lpszCmdLine, L" ");
        if (!s)
            return;

        s[0] = 0;

        vol = lpszCmdLine;
        block = &s[1];

        RtlZeroMemory(&bsb, sizeof(btrfs_start_balance));
        unserialize(&bsb, sizeof(btrfs_start_balance), block);

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        h = CreateFileW(vol, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h != INVALID_HANDLE_VALUE) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_START_BALANCE, &bsb, sizeof(btrfs_start_balance), nullptr, 0);

            if (Status == STATUS_DEVICE_NOT_READY) {
                btrfs_query_scrub bqs;
                NTSTATUS Status2;

                Status2 = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_SCRUB, nullptr, 0, &bqs, sizeof(btrfs_query_scrub));

                if ((NT_SUCCESS(Status2) || Status2 == STATUS_BUFFER_OVERFLOW) && bqs.status != BTRFS_SCRUB_STOPPED)
                    throw string_error(IDS_BALANCE_SCRUB_RUNNING);
            }

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        } else
            throw last_error(GetLastError());
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

extern "C" void CALLBACK PauseBalanceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        win_handle h, token;
        TOKEN_PRIVILEGES tp;
        LUID luid;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        h = CreateFileW(lpszCmdLine, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h != INVALID_HANDLE_VALUE) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;
            btrfs_query_balance bqb2;

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_BALANCE, nullptr, 0, &bqb2, sizeof(btrfs_query_balance));
            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            if (bqb2.status & BTRFS_BALANCE_PAUSED)
                Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RESUME_BALANCE, nullptr, 0, nullptr, 0);
            else if (bqb2.status & BTRFS_BALANCE_RUNNING)
                Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_PAUSE_BALANCE, nullptr, 0, nullptr, 0);
            else
                return;

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        } else
            throw last_error(GetLastError());
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

extern "C" void CALLBACK StopBalanceW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        win_handle h, token;
        TOKEN_PRIVILEGES tp;
        LUID luid;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            throw last_error(GetLastError());

        if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
            throw last_error(GetLastError());

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
            throw last_error(GetLastError());

        h = CreateFileW(lpszCmdLine, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

        if (h != INVALID_HANDLE_VALUE) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;
            btrfs_query_balance bqb2;

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_BALANCE, nullptr, 0, &bqb2, sizeof(btrfs_query_balance));
            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            if (bqb2.status & BTRFS_BALANCE_PAUSED || bqb2.status & BTRFS_BALANCE_RUNNING)
                Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_STOP_BALANCE, nullptr, 0, nullptr, 0);
            else
                return;

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);
        } else
            throw last_error(GetLastError());
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}
