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
#include <string>

#define NO_SHLWAPI_STRFCNS
#include <shlwapi.h>
#include <uxtheme.h>

void BtrfsScrub::UpdateTextBox(HWND hwndDlg, btrfs_query_scrub* bqs) {
    btrfs_query_scrub* bqs2 = NULL;
    BOOL alloc_bqs2 = FALSE;
    NTSTATUS Status;
    std::wstring s;
    WCHAR t[255], u[255], dt[255], tm[255];
    FILETIME filetime;
    SYSTEMTIME systime;
    UINT64 recoverable_errors = 0, unrecoverable_errors = 0;

    if (bqs->num_errors > 0) {
        HANDLE h;
        IO_STATUS_BLOCK iosb;
        ULONG len;

        h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ShowError(hwndDlg, GetLastError());
            return;
        }

        len = 0;

        do {
            len += 1024;

            if (bqs2)
                free(bqs2);

            bqs2 = (btrfs_query_scrub*)malloc(len);

            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_QUERY_SCRUB, NULL, 0, bqs2, len);

            if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
                ShowNtStatusError(hwndDlg, Status);
                CloseHandle(h);
                free(bqs2);
                return;
            }
        } while (Status == STATUS_BUFFER_OVERFLOW);

        alloc_bqs2 = TRUE;

        CloseHandle(h);
    } else
        bqs2 = bqs;

    s[0] = 0;

    // "scrub started"
    if (bqs2->start_time.QuadPart > 0) {
        filetime.dwLowDateTime = bqs2->start_time.LowPart;
        filetime.dwHighDateTime = bqs2->start_time.HighPart;

        if (!FileTimeToSystemTime(&filetime, &systime)) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!SystemTimeToTzSpecificLocalTime(NULL, &systime, &systime)) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!LoadStringW(module, IDS_SCRUB_MSG_STARTED, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime, NULL, dt, sizeof(dt) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systime, NULL, tm, sizeof(tm) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, dt, tm) == STRSAFE_E_INSUFFICIENT_BUFFER)
            goto end;

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
                if (!LoadStringW(module, IDS_SCRUB_MSG_RECOVERABLE_PARITY, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    goto end;
                }

                if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    goto end;
            } else if (bse->is_metadata) {
                int message;

                if (bse->recovered)
                    message = IDS_SCRUB_MSG_RECOVERABLE_METADATA;
                else if (bse->metadata.firstitem.obj_id == 0 && bse->metadata.firstitem.obj_type == 0 && bse->metadata.firstitem.offset == 0)
                    message = IDS_SCRUB_MSG_UNRECOVERABLE_METADATA;
                else
                    message = IDS_SCRUB_MSG_UNRECOVERABLE_METADATA_FIRSTITEM;

                if (!LoadStringW(module, message, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    goto end;
                }

                if (bse->recovered) {
                    if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device) == STRSAFE_E_INSUFFICIENT_BUFFER)
                        goto end;
                } else if (bse->metadata.firstitem.obj_id == 0 && bse->metadata.firstitem.obj_type == 0 && bse->metadata.firstitem.offset == 0) {
                    if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device,
                        bse->metadata.root, bse->metadata.level) == STRSAFE_E_INSUFFICIENT_BUFFER)
                        goto end;
                } else {
                    if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device,
                        bse->metadata.root, bse->metadata.level, bse->metadata.firstitem.obj_id, bse->metadata.firstitem.obj_type,
                        bse->metadata.firstitem.offset) == STRSAFE_E_INSUFFICIENT_BUFFER)
                        goto end;
                }
            } else {
                int message;

                if (bse->recovered)
                    message = IDS_SCRUB_MSG_RECOVERABLE_DATA;
                else if (bse->data.subvol != 0)
                    message = IDS_SCRUB_MSG_UNRECOVERABLE_DATA_SUBVOL;
                else
                    message = IDS_SCRUB_MSG_UNRECOVERABLE_DATA;

                if (!LoadStringW(module, message, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    goto end;
                }

                if (bse->recovered) {
                    if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device) == STRSAFE_E_INSUFFICIENT_BUFFER)
                        goto end;
                } else if (bse->data.subvol != 0) {
                    if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device, bse->data.subvol,
                        bse->data.filename_length / sizeof(WCHAR), bse->data.filename, bse->data.offset) == STRSAFE_E_INSUFFICIENT_BUFFER)
                        goto end;
                } else {
                    if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, bse->address, bse->device, bse->data.filename_length / sizeof(WCHAR),
                        bse->data.filename, bse->data.offset) == STRSAFE_E_INSUFFICIENT_BUFFER)
                        goto end;
                }
            }

            s += u;
            s += L"\r\n";

            if (bse->next_entry == 0)
                break;
            else
                bse = (btrfs_scrub_error*)((UINT8*)bse + bse->next_entry);
        } while (TRUE);
    }

    if (bqs2->finish_time.QuadPart > 0) {
        WCHAR d1[255], d2[255];
        float speed;

        // "scrub finished"

        filetime.dwLowDateTime = bqs2->finish_time.LowPart;
        filetime.dwHighDateTime = bqs2->finish_time.HighPart;

        if (!FileTimeToSystemTime(&filetime, &systime)) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!SystemTimeToTzSpecificLocalTime(NULL, &systime, &systime)) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!LoadStringW(module, IDS_SCRUB_MSG_FINISHED, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!GetDateFormatW(LOCALE_USER_DEFAULT, DATE_SHORTDATE, &systime, NULL, dt, sizeof(dt) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (!GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &systime, NULL, tm, sizeof(tm) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, dt, tm) == STRSAFE_E_INSUFFICIENT_BUFFER)
            goto end;

        s += u;
        s += L"\r\n";

        // summary

        if (!LoadStringW(module, IDS_SCRUB_MSG_SUMMARY, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        format_size(bqs2->data_scrubbed, d1, sizeof(d1) / sizeof(WCHAR), FALSE);

        speed = (float)bqs2->data_scrubbed / ((float)bqs2->duration / 10000000.0f);

        format_size((UINT64)speed, d2, sizeof(d2) / sizeof(WCHAR), FALSE);

        if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, d1, bqs2->duration / 10000000, d2) == STRSAFE_E_INSUFFICIENT_BUFFER)
            goto end;

        s += u;
        s += L"\r\n";

        // recoverable errors

        if (!LoadStringW(module, IDS_SCRUB_MSG_SUMMARY_ERRORS_RECOVERABLE, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, recoverable_errors) == STRSAFE_E_INSUFFICIENT_BUFFER)
            goto end;

        s += u;
        s += L"\r\n";

        // unrecoverable errors

        if (!LoadStringW(module, IDS_SCRUB_MSG_SUMMARY_ERRORS_UNRECOVERABLE, t, sizeof(t) / sizeof(WCHAR))) {
            ShowError(hwndDlg, GetLastError());
            goto end;
        }

        if (StringCchPrintfW(u, sizeof(u) / sizeof(WCHAR), t, unrecoverable_errors) == STRSAFE_E_INSUFFICIENT_BUFFER)
            goto end;

        s += u;
        s += L"\r\n";
    }

    SetWindowTextW(GetDlgItem(hwndDlg, IDC_SCRUB_INFO), s.c_str());

end:
    if (alloc_bqs2)
        free(bqs2);
}

void BtrfsScrub::RefreshScrubDlg(HWND hwndDlg, BOOL first_time) {
    HANDLE h;
    btrfs_query_scrub bqs;

    h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_QUERY_SCRUB, NULL, 0, &bqs, sizeof(btrfs_query_scrub));

        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
            ShowNtStatusError(hwndDlg, Status);
            CloseHandle(h);
            return;
        }

        CloseHandle(h);
    } else {
        ShowError(hwndDlg, GetLastError());
        return;
    }

    if (first_time || status != bqs.status || chunks_left != bqs.chunks_left) {
        WCHAR s[255];

        if (bqs.status == BTRFS_SCRUB_STOPPED) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_START_SCRUB), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_SCRUB), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_SCRUB), FALSE);

            if (bqs.error != STATUS_SUCCESS) {
                WCHAR t[255];

                if (!LoadStringW(module, IDS_SCRUB_FAILED, t, sizeof(t) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return;
                }

                if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, bqs.error) == STRSAFE_E_INSUFFICIENT_BUFFER)
                    return;
            } else {
                if (!LoadStringW(module, bqs.total_chunks == 0 ? IDS_NO_SCRUB : IDS_SCRUB_FINISHED, s, sizeof(s) / sizeof(WCHAR))) {
                    ShowError(hwndDlg, GetLastError());
                    return;
                }
            }
        } else {
            WCHAR t[255];
            float pc;

            EnableWindow(GetDlgItem(hwndDlg, IDC_START_SCRUB), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_PAUSE_SCRUB), TRUE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_CANCEL_SCRUB), TRUE);

            if (!LoadStringW(module, bqs.status == BTRFS_SCRUB_PAUSED ? IDS_SCRUB_PAUSED : IDS_SCRUB_RUNNING, t, sizeof(t) / sizeof(WCHAR))) {
                ShowError(hwndDlg, GetLastError());
                return;
            }

            pc = ((float)(bqs.total_chunks - bqs.chunks_left) / (float)bqs.total_chunks) * 100.0f;

            if (StringCchPrintfW(s, sizeof(s) / sizeof(WCHAR), t, bqs.total_chunks - bqs.chunks_left, bqs.total_chunks, pc) == STRSAFE_E_INSUFFICIENT_BUFFER)
                return;
        }

        SetDlgItemTextW(hwndDlg, IDC_SCRUB_STATUS, s);

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
    HANDLE h;

    h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_START_SCRUB, NULL, 0, NULL, 0);

        if (Status == STATUS_DEVICE_NOT_READY) {
            btrfs_query_balance bqb;
            NTSTATUS Status2;

            Status2 = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_QUERY_BALANCE, NULL, 0, &bqb, sizeof(btrfs_query_balance));

            if (NT_SUCCESS(Status2) && bqb.status & (BTRFS_BALANCE_RUNNING | BTRFS_BALANCE_PAUSED)) {
                ShowStringError(hwndDlg, IDS_SCRUB_BALANCE_RUNNING);
                CloseHandle(h);
                return;
            }
        }

        if (!NT_SUCCESS(Status)) {
            ShowNtStatusError(hwndDlg, Status);
            CloseHandle(h);
            return;
        }

        RefreshScrubDlg(hwndDlg, TRUE);

        CloseHandle(h);
    } else {
        ShowError(hwndDlg, GetLastError());
        return;
    }
}

void BtrfsScrub::PauseScrub(HWND hwndDlg) {
    HANDLE h;
    btrfs_query_scrub bqs;

    h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_QUERY_SCRUB, NULL, 0, &bqs, sizeof(btrfs_query_scrub));

        if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW) {
            ShowNtStatusError(hwndDlg, Status);
            CloseHandle(h);
            return;
        }

        if (bqs.status == BTRFS_SCRUB_PAUSED)
            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_RESUME_SCRUB, NULL, 0, NULL, 0);
        else
            Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_PAUSE_SCRUB, NULL, 0, NULL, 0);

        if (!NT_SUCCESS(Status)) {
            ShowNtStatusError(hwndDlg, Status);
            CloseHandle(h);
            return;
        }

        CloseHandle(h);
    } else {
        ShowError(hwndDlg, GetLastError());
        return;
    }
}

void BtrfsScrub::StopScrub(HWND hwndDlg) {
    HANDLE h;

    h = CreateFileW(fn, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
    if (h != INVALID_HANDLE_VALUE) {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;

        Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_STOP_SCRUB, NULL, 0, NULL, 0);

        if (!NT_SUCCESS(Status)) {
            ShowNtStatusError(hwndDlg, Status);
            CloseHandle(h);
            return;
        }

        CloseHandle(h);
    } else {
        ShowError(hwndDlg, GetLastError());
        return;
    }
}

INT_PTR CALLBACK BtrfsScrub::ScrubDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            RefreshScrubDlg(hwndDlg, TRUE);
            SetTimer(hwndDlg, 1, 1000, NULL);
        break;

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDOK:
                        case IDCANCEL:
                            EndDialog(hwndDlg, 0);
                        return TRUE;

                        case IDC_START_SCRUB:
                            StartScrub(hwndDlg);
                        return TRUE;

                        case IDC_PAUSE_SCRUB:
                            PauseScrub(hwndDlg);
                        return TRUE;

                        case IDC_CANCEL_SCRUB:
                            StopScrub(hwndDlg);
                        return TRUE;
                    }
                break;
            }
        break;

        case WM_TIMER:
            RefreshScrubDlg(hwndDlg, FALSE);
        break;
    }

    return FALSE;
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
        return FALSE;
}

void CALLBACK ShowScrubW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    BtrfsScrub* scrub;

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

    set_dpi_aware();

    scrub = new BtrfsScrub(lpszCmdLine);

    DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_SCRUB), hwnd, stub_ScrubDlgProc, (LPARAM)scrub);

    delete scrub;

end:
    CloseHandle(token);
}

void CALLBACK StartScrubW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 1) {
        HANDLE h, token;
        LUID luid;
        TOKEN_PRIVILEGES tp;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            goto end;

        if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid))
            goto end;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
            goto end;

        CloseHandle(token);

        h = CreateFileW(args[0], FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            IO_STATUS_BLOCK iosb;

            NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_START_SCRUB, NULL, 0, NULL, 0);

            CloseHandle(h);
        }
    }

end:
    LocalFree(args);
}

void CALLBACK StopScrubW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 1) {
        HANDLE h, token;
        LUID luid;
        TOKEN_PRIVILEGES tp;

        if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token))
            goto end;

        if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid))
            goto end;

        tp.PrivilegeCount = 1;
        tp.Privileges[0].Luid = luid;
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL))
            goto end;

        CloseHandle(token);

        h = CreateFileW(args[0], FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
                        OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL);
        if (h != INVALID_HANDLE_VALUE) {
            IO_STATUS_BLOCK iosb;

            NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_STOP_SCRUB, NULL, 0, NULL, 0);

            CloseHandle(h);
        }
    }

end:
    LocalFree(args);
}
