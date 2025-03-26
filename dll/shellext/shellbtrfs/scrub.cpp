/* Copyright (c) Mark Harmstone 2017
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
#include "scrub.h"
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

void BtrfsScrub::UpdateTextBox(HWND hwndDlg, btrfs_query_scrub* bqs) {
    btrfs_query_scrub* bqs2 = nullptr;
    bool alloc_bqs2 = false;
    NTSTATUS Status;
    wstring s, t, u;
    WCHAR dt[255], tm[255];
    FILETIME filetime;
    SYSTEMTIME systime;
    uint64_t recoverable_errors = 0, unrecoverable_errors = 0;

    try {
        if (bqs->num_errors > 0) {
            win_handle h;
            IO_STATUS_BLOCK iosb;
            ULONG len;

            h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
            if (h == INVALID_HANDLE_VALUE)
                throw last_error(GetLastError());

            len = 0;

            try {
                do {
                    len += 1024;

                    if (bqs2) {
                        free(bqs2);
                        bqs2 = nullptr;
                    }

                    bqs2 = (btrfs_query_scrub*)malloc(len);

                    Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_SCRUB, nullptr, 0, bqs2, len);

                    if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
                        throw ntstatus_error(Status);
                } while (Status == STATUS_BUFFER_OVERFLOW);
            } catch (...) {
                if (bqs2)
                    free(bqs2);

                throw;
            }

            alloc_bqs2 = true;
        } else
            bqs2 = bqs;

        // "scrub started"
        if (bqs2->start_time.QuadPart > 0) {
            filetime.dwLowDateTime = bqs2->start_time.LowPart;
            filetime.dwHighDateTime = bqs2->start_time.HighPart;

            if (!FileTimeToSystemTime(&filetime, &systime))
                throw last_error(GetLastError());

            if (!SystemTimeToTzSpecificLocalTime(nullptr, &systime, &systime))
                throw last_error(GetLastError());

            if (!load_string(module, IDS_SCRUB_MSG_STARTED, t))
                throw last_error(GetLastError());

            if (!GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime, nullptr, dt, sizeof(dt) / sizeof(WCHAR)))
                throw last_error(GetLastError());

            if (!GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systime, nullptr, tm, sizeof(tm) / sizeof(WCHAR)))
                throw last_error(GetLastError());

            wstring_sprintf(u, t, dt, tm);

            s += u;
            s += L"\r\n";
        }

        // errors
        if (bqs2->num_errors > 0) {
            btrfs_scrub_error* bse = &bqs2->errors;

            do {
                if (bse->recovered)
                    recoverable_errors++;
                else
                    unrecoverable_errors++;

                if (bse->parity) {
                    if (!load_string(module, IDS_SCRUB_MSG_RECOVERABLE_PARITY, t))
                        throw last_error(GetLastError());

                    wstring_sprintf(u, t, bse->address, bse->device);
                } else if (bse->is_metadata) {
                    int message;

                    if (bse->recovered)
                        message = IDS_SCRUB_MSG_RECOVERABLE_METADATA;
                    else if (bse->metadata.firstitem.obj_id == 0 && bse->metadata.firstitem.obj_type == 0 && bse->metadata.firstitem.offset == 0)
                        message = IDS_SCRUB_MSG_UNRECOVERABLE_METADATA;
                    else
                        message = IDS_SCRUB_MSG_UNRECOVERABLE_METADATA_FIRSTITEM;

                    if (!load_string(module, message, t))
                        throw last_error(GetLastError());

                    if (bse->recovered)
                        wstring_sprintf(u, t, bse->address, bse->device);
                    else if (bse->metadata.firstitem.obj_id == 0 && bse->metadata.firstitem.obj_type == 0 && bse->metadata.firstitem.offset == 0)
                        wstring_sprintf(u, t, bse->address, bse->device, bse->metadata.root, bse->metadata.level);
                    else
                        wstring_sprintf(u, t, bse->address, bse->device, bse->metadata.root, bse->metadata.level, bse->metadata.firstitem.obj_id,
                                        bse->metadata.firstitem.obj_type, bse->metadata.firstitem.offset);
                } else {
                    int message;

                    if (bse->recovered)
                        message = IDS_SCRUB_MSG_RECOVERABLE_DATA;
                    else if (bse->data.subvol != 0)
                        message = IDS_SCRUB_MSG_UNRECOVERABLE_DATA_SUBVOL;
                    else
                        message = IDS_SCRUB_MSG_UNRECOVERABLE_DATA;

                    if (!load_string(module, message, t))
                        throw last_error(GetLastError());

                    if (bse->recovered)
                        wstring_sprintf(u, t, bse->address, bse->device);
                    else if (bse->data.subvol != 0)
                        wstring_sprintf(u, t, bse->address, bse->device, bse->data.subvol,
                            bse->data.filename_length / sizeof(WCHAR), bse->data.filename, bse->data.offset);
                    else
                        wstring_sprintf(u, t, bse->address, bse->device, bse->data.filename_length / sizeof(WCHAR),
                            bse->data.filename, bse->data.offset);
                }

                s += u;
                s += L"\r\n";

                if (bse->next_entry == 0)
                    break;
                else
                    bse = (btrfs_scrub_error*)((uint8_t*)bse + bse->next_entry);
            } while (true);
        }

        if (bqs2->finish_time.QuadPart > 0) {
            wstring d1, d2;
            float speed;

            // "scrub finished"

            filetime.dwLowDateTime = bqs2->finish_time.LowPart;
            filetime.dwHighDateTime = bqs2->finish_time.HighPart;

            if (!FileTimeToSystemTime(&filetime, &systime))
                throw last_error(GetLastError());

            if (!SystemTimeToTzSpecificLocalTime(nullptr, &systime, &systime))
                throw last_error(GetLastError());

            if (!load_string(module, IDS_SCRUB_MSG_FINISHED, t))
                throw last_error(GetLastError());

            if (!GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime, nullptr, dt, sizeof(dt) / sizeof(WCHAR)))
                throw last_error(GetLastError());

            if (!GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systime, nullptr, tm, sizeof(tm) / sizeof(WCHAR)))
                throw last_error(GetLastError());

            wstring_sprintf(u, t, dt, tm);

            s += u;
            s += L"\r\n";

            // summary

            if (!load_string(module, IDS_SCRUB_MSG_SUMMARY, t))
                throw last_error(GetLastError());

            format_size(bqs2->data_scrubbed, d1, false);

            speed = (float)bqs2->data_scrubbed / ((float)bqs2->duration / 10000000.0f);

            format_size((uint64_t)speed, d2, false);

            wstring_sprintf(u, t, d1.c_str(), bqs2->duration / 10000000, d2.c_str());

            s += u;
            s += L"\r\n";

            // recoverable errors

            if (!load_string(module, IDS_SCRUB_MSG_SUMMARY_ERRORS_RECOVERABLE, t))
                throw last_error(GetLastError());

            wstring_sprintf(u, t, recoverable_errors);

            s += u;
            s += L"\r\n";

            // unrecoverable errors

            if (!load_string(module, IDS_SCRUB_MSG_SUMMARY_ERRORS_UNRECOVERABLE, t))
                throw last_error(GetLastError());

            wstring_sprintf(u, t, unrecoverable_errors);

            s += u;
            s += L"\r\n";
        }

        SetWindowTextW(GetDlgItem(hwndDlg, IDC_SCRUB_INFO), s.c_str());
    } catch (...) {
        if (alloc_bqs2)
            free(bqs2);

        throw;
    }

    if (alloc_bqs2)
        free(bqs2);
}

void BtrfsScrub::RefreshScrubDlg(HWND hwndDlg, bool first_time) {
    btrfs_query_scrub bqs;

    {
        win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            NTSTATUS Status;
            IO_STATUS_BLOCK iosb;

            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_SCRUB, nullptr, 0, &bqs, sizeof(btrfs_query_scrub));

            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
                throw ntstatus_error(Status);
        } else
            throw last_error(GetLastError());
    }

    if (first_time || status != bqs.status || chunks_left != bqs.chunks_left) {
        wstring s;

        if (bqs.status == BTRFS_SCRUB_STOPPED) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_START_SCRUB), true);
            EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_SCRUB), false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_SCRUB), false);

            if (bqs.error != STATUS_SUCCESS) {
                wstring t;

                if (!load_string(module, IDS_SCRUB_FAILED, t))
                    throw last_error(GetLastError());

                wstring_sprintf(s, t, bqs.error);
            } else {
                if (!load_string(module, bqs.total_chunks == 0 ? IDS_NO_SCRUB : IDS_SCRUB_FINISHED, s))
                    throw last_error(GetLastError());
            }
        } else {
            wstring t;
            float pc;

            EnableWindow(GetDlgItem(hwndDlg, IDC_START_SCRUB), false);
            EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_SCRUB), true);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_SCRUB), true);

            if (!load_string(module, bqs.status == BTRFS_SCRUB_PAUSED ? IDS_SCRUB_PAUSED : IDS_SCRUB_RUNNING, t))
                throw last_error(GetLastError());

            pc = ((float)(bqs.total_chunks - bqs.chunks_left) / (float)bqs.total_chunks) * 100.0f;

            wstring_sprintf(s, t, bqs.total_chunks - bqs.chunks_left, bqs.total_chunks, pc);
        }

        SetDlgItemTextW(hwndDlg, IDC_SCRUB_STATUS, s.c_str());

        if (first_time || status != bqs.status) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), bqs.status != BTRFS_SCRUB_STOPPED);

            if (bqs.status != BTRFS_SCRUB_STOPPED) {
                SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETRANGE32, 0, (LPARAM)bqs.total_chunks);
                SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETPOS, (WPARAM)(bqs.total_chunks - bqs.chunks_left), 0);

                if (bqs.status == BTRFS_SCRUB_PAUSED)
                    SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETSTATE, PBST_PAUSED, 0);
                else
                    SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETSTATE, PBST_NORMAL, 0);
            } else {
                SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETRANGE32, 0, 0);
                SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETPOS, 0, 0);
            }

            chunks_left = bqs.chunks_left;
        }
    }

    if (bqs.status != BTRFS_SCRUB_STOPPED && chunks_left != bqs.chunks_left) {
        SendMessageW(GetDlgItem(hwndDlg, IDC_SCRUB_PROGRESS), PBM_SETPOS, (WPARAM)(bqs.total_chunks - bqs.chunks_left), 0);
        chunks_left = bqs.chunks_left;
    }

    if (first_time || status != bqs.status || num_errors != bqs.num_errors) {
        UpdateTextBox(hwndDlg, &bqs);

        num_errors = bqs.num_errors;
    }

    status = bqs.status;
}

void BtrfsScrub::StartScrub(HWND hwndDlg) {
    win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_START_SCRUB, nullptr, 0, nullptr, 0);

        if (Status == STATUS_DEVICE_NOT_READY) {
            btrfs_query_balance bqb;
            NTSTATUS Status2;

            Status2 = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_BALANCE, nullptr, 0, &bqb, sizeof(btrfs_query_balance));

            if (NT_SUCCESS(Status2) && bqb.status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED))
                throw string_error(IDS_SCRUB_BALANCE_RUNNING);
        }

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);

        RefreshScrubDlg(hwndDlg, true);
    } else
        throw last_error(GetLastError());
}

void BtrfsScrub::PauseScrub(HWND hwndDlg) {
    btrfs_query_scrub bqs;

    win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_QUERY_SCRUB, nullptr, 0, &bqs, sizeof(btrfs_query_scrub));

        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
            throw ntstatus_error(Status);

        if (bqs.status == BTRFS_SCRUB_PAUSED)
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_RESUME_SCRUB, nullptr, 0, nullptr, 0);
        else
            Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_PAUSE_SCRUB, nullptr, 0, nullptr, 0);

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    } else
        throw last_error(GetLastError());
}

void BtrfsScrub::StopScrub(HWND hwndDlg) {
    win_handle h = CreateFileW(fn.c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                               OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);

    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_STOP_SCRUB, nullptr, 0, nullptr, 0);

        if (!NT_SUCCESS(Status))
            throw ntstatus_error(Status);
    } else
        throw last_error(GetLastError());
}

INT_PTR CALLBACK BtrfsScrub::ScrubDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
        switch (uMsg) {
            case WM_INITDIALOG:
                RefreshScrubDlg(hwndDlg, true);
                SetTimer(hwndDlg, 1, 1000, nullptr);
            break;

            case WM_COMMAND:
                switch (HIWORD(wParam)) {
                    case BN_CLICKED:
                        switch (LOWORD(wParam)) {
                            case IDOK:
                            case IDCANCEL:
                                EndDialog(hwndDlg, 0);
                            return true;

                            case IDC_START_SCRUB:
                                StartScrub(hwndDlg);
                            return true;

                            case IDC_PAUSE_SCRUB:
                                PauseScrub(hwndDlg);
                            return true;

                            case IDC_CANCEL_SCRUB:
                                StopScrub(hwndDlg);
                            return true;
                        }
                    break;
                }
            break;

            case WM_TIMER:
                RefreshScrubDlg(hwndDlg, false);
            break;
        }
    } catch (const exception& e) {
        error_message(hwndDlg, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_ScrubDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsScrub* bs;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bs = (BtrfsScrub*)lParam;
    } else {
        bs = (BtrfsScrub*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);
    }

    if (bs)
        return bs->ScrubDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

extern "C" void CALLBACK ShowScrubW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    try {
        win_handle token;
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

        set_dpi_aware();

        BtrfsScrub scrub(lpszCmdLine);

        DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_SCRUB), hwnd, stub_ScrubDlgProc, (LPARAM)&scrub);
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

extern "C" void CALLBACK StartScrubW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    vector<wstring> args;

    command_line_to_args(lpszCmdLine, args);

    if (args.size() >= 1) {
        LUID luid;
        TOKEN_PRIVILEGES tp;

        {
            win_handle token;

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
                return;

            if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
                return;

            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
                return;
        }

        win_handle h = CreateFileW(args[0].c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            IO_STATUS_BLOCK iosb;

            NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_START_SCRUB, nullptr, 0, nullptr, 0);
        }
    }
}

extern "C" void CALLBACK StopScrubW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    vector<wstring> args;

    command_line_to_args(lpszCmdLine, args);

    if (args.size() >= 1) {
        LUID luid;
        TOKEN_PRIVILEGES tp;

        {
            win_handle token;

            if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
                return;

            if (!LookupPrivilegeValueW(nullptr, L"SeManageVolumePrivilege", &luid))
                return;

            tp.PrivilegeCount = 1;
            tp.Privileges[0].Luid = luid;
            tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

            if (!AdjustTokenPrivileges(token, false, &tp, sizeof(TOKEN_PRIVILEGES), nullptr, nullptr))
                return;
        }

        win_handle h = CreateFileW(args[0].c_str(), FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr,
                                   OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr);
        if (h != INVALID_HANDLE_VALUE) {
            IO_STATUS_BLOCK iosb;

            NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_STOP_SCRUB, nullptr, 0, nullptr, 0);
        }
    }
}
