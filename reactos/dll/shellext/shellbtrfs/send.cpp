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
#include "send.h"
#include "resource.h"
#include <stddef.h>
#include <shlobj.h>
#ifdef __REACTOS__
#undef DeleteFile
#endif

#define SEND_BUFFER_LEN 1048576

void BtrfsSend::ShowSendError(UINT msg, ...) {
    WCHAR s[1024], t[1024];
    va_list ap;

    if (!LoadStringW(module, msg, s, sizeof(s) / sizeof(WCHAR))) {
        ShowError(hwnd, GetLastError());
        return;
    }

    va_start(ap, msg);
#ifndef __REACTOS__
    vswprintf(t, sizeof(t) / sizeof(WCHAR), s, ap);
#else
    vsnwprintf(t, sizeof(t) / sizeof(WCHAR), s, ap);
#endif

    SetDlgItemTextW(hwnd, IDC_SEND_STATUS, t);

    va_end(ap);
}

DWORD BtrfsSend::Thread() {
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_send_subvol* bss;
    btrfs_send_header header;
    btrfs_send_command end;
    BOOL success = FALSE;
    ULONG bss_size, i;

    buf = (char*)malloc(SEND_BUFFER_LEN);

    dirh = CreateFileW(subvol.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (dirh == INVALID_HANDLE_VALUE) {
        ShowSendError(IDS_SEND_CANT_OPEN_DIR, subvol.c_str(), GetLastError(), format_message(GetLastError()).c_str());
        goto end3;
    }

    bss_size = offsetof(btrfs_send_subvol, clones[0]) + (clones.size() * sizeof(HANDLE));
    bss = (btrfs_send_subvol*)malloc(bss_size);
    memset(bss, 0, bss_size);

    if (incremental) {
        WCHAR parent[MAX_PATH];
        HANDLE parenth;

        parent[0] = 0;

        GetDlgItemTextW(hwnd, IDC_PARENT_SUBVOL, parent, sizeof(parent) / sizeof(WCHAR));

        parenth = CreateFileW(parent, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (parenth == INVALID_HANDLE_VALUE) {
            ShowSendError(IDS_SEND_CANT_OPEN_DIR, parent, GetLastError(), format_message(GetLastError()).c_str());
            goto end2;
        }

        bss->parent = parenth;
    } else
        bss->parent = NULL;

    bss->num_clones = clones.size();

    for (i = 0; i < bss->num_clones; i++) {
        HANDLE h;

        h = CreateFileW(clones[i].c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ULONG j;

            ShowSendError(IDS_SEND_CANT_OPEN_DIR, clones[i].c_str(), GetLastError(), format_message(GetLastError()).c_str());

            for (j = 0; j < i; j++) {
                CloseHandle(bss->clones[j]);
            }

            if (bss->parent) CloseHandle(bss->parent);
            goto end2;
        }

        bss->clones[i] = h;
    }

    Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SEND_SUBVOL, bss, bss_size, NULL, 0);

    for (i = 0; i < bss->num_clones; i++) {
        CloseHandle(bss->clones[i]);
    }

    if (!NT_SUCCESS(Status)) {
        if (Status == (NTSTATUS)STATUS_INVALID_PARAMETER) {
            BY_HANDLE_FILE_INFORMATION fileinfo;
            if (!GetFileInformationByHandle(dirh, &fileinfo)) {
                ShowSendError(IDS_SEND_GET_FILE_INFO_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                if (bss->parent) CloseHandle(bss->parent);
                goto end2;
            }

            if (!(fileinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                ShowSendError(IDS_SEND_NOT_READONLY);
                if (bss->parent) CloseHandle(bss->parent);
                goto end2;
            }

            if (bss->parent) {
                if (!GetFileInformationByHandle(bss->parent, &fileinfo)) {
                    ShowSendError(IDS_SEND_GET_FILE_INFO_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                    CloseHandle(bss->parent);
                    goto end2;
                }

                if (!(fileinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                    ShowSendError(IDS_SEND_PARENT_NOT_READONLY);
                    CloseHandle(bss->parent);
                    goto end2;
                }
            }
        }

        ShowSendError(IDS_SEND_FSCTL_BTRFS_SEND_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
        if (bss->parent) CloseHandle(bss->parent);
        goto end2;
    }

    if (bss->parent) CloseHandle(bss->parent);

    stream = CreateFileW(file, FILE_WRITE_DATA | DELETE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (stream == INVALID_HANDLE_VALUE) {
        ShowSendError(IDS_SEND_CANT_OPEN_FILE, file, GetLastError(), format_message(GetLastError()).c_str());
        goto end2;
    }

    memcpy(header.magic, BTRFS_SEND_MAGIC, sizeof(BTRFS_SEND_MAGIC));
    header.version = 1;

    if (!WriteFile(stream, &header, sizeof(header), NULL, NULL)) {
        ShowSendError(IDS_SEND_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        goto end;
    }

    do {
        Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_READ_SEND_BUFFER, NULL, 0, buf, SEND_BUFFER_LEN);

        if (NT_SUCCESS(Status)) {
            if (!WriteFile(stream, buf, iosb.Information, NULL, NULL))
                ShowSendError(IDS_SEND_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        }
    } while (NT_SUCCESS(Status));

    if (Status != STATUS_END_OF_FILE) {
        ShowSendError(IDS_SEND_FSCTL_BTRFS_READ_SEND_BUFFER_FAILED, Status, format_ntstatus(Status).c_str());
        goto end;
    }

    end.length = 0;
    end.cmd = BTRFS_SEND_CMD_END;
    end.csum = 0x9dc96c50;

    if (!WriteFile(stream, &end, sizeof(end), NULL, NULL)) {
        ShowSendError(IDS_SEND_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        goto end;
    }

    SetEndOfFile(stream);

    ShowSendError(IDS_SEND_SUCCESS);
    success = TRUE;

end:
    if (!success) {
        FILE_DISPOSITION_INFO fdi;

        fdi.DeleteFile = TRUE;

        SetFileInformationByHandle(stream, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO));
    }

    CloseHandle(stream);
    stream = INVALID_HANDLE_VALUE;

end2:
    CloseHandle(dirh);
    dirh = INVALID_HANDLE_VALUE;

end3:
    free(buf);
    buf = NULL;

    started = FALSE;

    SetDlgItemTextW(hwnd, IDCANCEL, closetext);
    EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), TRUE);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);

    return 0;
}

static DWORD WINAPI send_thread(LPVOID lpParameter) {
    BtrfsSend* bs = (BtrfsSend*)lpParameter;

    return bs->Thread();
}

void BtrfsSend::StartSend(HWND hwnd) {
    WCHAR s[255];
    HWND cl;
    ULONG num_clones;

    if (started)
        return;

    GetDlgItemTextW(hwnd, IDC_STREAM_DEST, file, sizeof(file) / sizeof(WCHAR));

    if (file[0] == 0)
        return;

    if (incremental) {
        WCHAR parent[MAX_PATH];

        GetDlgItemTextW(hwnd, IDC_PARENT_SUBVOL, parent, sizeof(parent) / sizeof(WCHAR));

        if (parent[0] == 0)
            return;
    }

    started = TRUE;
    ShowSendError(IDS_SEND_WRITING);

    LoadStringW(module, IDS_SEND_CANCEL, s, sizeof(s) / sizeof(WCHAR));
    SetDlgItemTextW(hwnd, IDCANCEL, s);

    EnableWindow(GetDlgItem(hwnd, IDOK), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), FALSE);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), FALSE);

    clones.clear();

    cl = GetDlgItem(hwnd, IDC_CLONE_LIST);
    num_clones = SendMessageW(cl, LB_GETCOUNT, 0, 0);

    if ((LRESULT)num_clones != LB_ERR) {
        ULONG i;

        for (i = 0; i < num_clones; i++) {
            WCHAR* s;
            ULONG len;

            len = SendMessageW(cl, LB_GETTEXTLEN, i, 0);
            s = (WCHAR*)malloc((len + 1) * sizeof(WCHAR));

            SendMessageW(cl, LB_GETTEXT, i, (LPARAM)s);

            clones.push_back(s);

            free(s);
        }
    }

    thread = CreateThread(NULL, 0, send_thread, this, 0, NULL);

    if (!thread)
        ShowError(NULL, GetLastError());
}

void BtrfsSend::Browse(HWND hwnd) {
    OPENFILENAMEW ofn;

    file[0] = 0;

    memset(&ofn, 0, sizeof(OPENFILENAMEW));
    ofn.lStructSize = sizeof(OPENFILENAMEW);
    ofn.hwndOwner = hwnd;
    ofn.hInstance = module;
    ofn.lpstrFile = file;
    ofn.nMaxFile = sizeof(file) / sizeof(WCHAR);

    if (!GetSaveFileNameW(&ofn))
        return;

    SetDlgItemTextW(hwnd, IDC_STREAM_DEST, file);
}

void BtrfsSend::BrowseParent(HWND hwnd) {
    BROWSEINFOW bi;
    PIDLIST_ABSOLUTE root, pidl;
    HRESULT hr;
    WCHAR parent[MAX_PATH], volpathw[MAX_PATH];
    HANDLE h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    if (!GetVolumePathNameW(subvol.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1)) {
        ShowStringError(hwnd, IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return;
    }

    hr = SHParseDisplayName(volpathw, 0, &root, 0, 0);
    if (FAILED(hr)) {
        ShowStringError(hwnd, IDS_SHPARSEDISPLAYNAME_FAILED);
        return;
    }

    memset(&bi, 0, sizeof(BROWSEINFOW));

    bi.hwndOwner = hwnd;
    bi.pidlRoot = root;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;

    pidl = SHBrowseForFolderW(&bi);

    if (!pidl)
        return;

    if (!SHGetPathFromIDListW(pidl, parent)) {
        ShowStringError(hwnd, IDS_SHGETPATHFROMIDLIST_FAILED);
        return;
    }

    h = CreateFileW(parent, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ShowStringError(hwnd, IDS_SEND_CANT_OPEN_DIR, parent, GetLastError(), format_message(GetLastError()).c_str());
        return;
    }

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));
    if (!NT_SUCCESS(Status)) {
        ShowStringError(hwnd, IDS_GET_FILE_IDS_FAILED, Status, format_ntstatus(Status).c_str());
        CloseHandle(h);
        return;
    }

    CloseHandle(h);

    if (bgfi.inode != 0x100 || bgfi.top) {
        ShowStringError(hwnd, IDS_NOT_SUBVOL);
        return;
    }

    SetDlgItemTextW(hwnd, IDC_PARENT_SUBVOL, parent);
}

void BtrfsSend::AddClone(HWND hwnd) {
    BROWSEINFOW bi;
    PIDLIST_ABSOLUTE root, pidl;
    HRESULT hr;
    WCHAR path[MAX_PATH], volpathw[MAX_PATH];
    HANDLE h;
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    if (!GetVolumePathNameW(subvol.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1)) {
        ShowStringError(hwnd, IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());
        return;
    }

    hr = SHParseDisplayName(volpathw, 0, &root, 0, 0);
    if (FAILED(hr)) {
        ShowStringError(hwnd, IDS_SHPARSEDISPLAYNAME_FAILED);
        return;
    }

    memset(&bi, 0, sizeof(BROWSEINFOW));

    bi.hwndOwner = hwnd;
    bi.pidlRoot = root;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;

    pidl = SHBrowseForFolderW(&bi);

    if (!pidl)
        return;

    if (!SHGetPathFromIDListW(pidl, path)) {
        ShowStringError(hwnd, IDS_SHGETPATHFROMIDLIST_FAILED);
        return;
    }

    h = CreateFileW(path, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (h == INVALID_HANDLE_VALUE) {
        ShowStringError(hwnd, IDS_SEND_CANT_OPEN_DIR, path, GetLastError(), format_message(GetLastError()).c_str());
        return;
    }

    Status = NtFsControlFile(h, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_GET_FILE_IDS, NULL, 0, &bgfi, sizeof(btrfs_get_file_ids));
    if (!NT_SUCCESS(Status)) {
        ShowStringError(hwnd, IDS_GET_FILE_IDS_FAILED, Status, format_ntstatus(Status).c_str());
        CloseHandle(h);
        return;
    }

    CloseHandle(h);

    if (bgfi.inode != 0x100 || bgfi.top) {
        ShowStringError(hwnd, IDS_NOT_SUBVOL);
        return;
    }

    SendMessageW(GetDlgItem(hwnd, IDC_CLONE_LIST), LB_ADDSTRING, 0, (LPARAM)path);
}

void BtrfsSend::RemoveClone(HWND hwnd) {
    LRESULT sel;
    HWND cl = GetDlgItem(hwnd, IDC_CLONE_LIST);

    sel = SendMessageW(cl, LB_GETCURSEL, 0, 0);

    if (sel == LB_ERR)
        return;

    SendMessageW(cl, LB_DELETESTRING, sel, 0);

    if (SendMessageW(cl, LB_GETCURSEL, 0, 0) == LB_ERR)
        EnableWindow(GetDlgItem(hwnd, IDC_CLONE_REMOVE), FALSE);
}

INT_PTR BtrfsSend::SendDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_INITDIALOG:
            this->hwnd = hwndDlg;

            GetDlgItemTextW(hwndDlg, IDCANCEL, closetext, sizeof(closetext) / sizeof(WCHAR));
        break;

        case WM_COMMAND:
            switch (HIWORD(wParam)) {
                case BN_CLICKED:
                    switch (LOWORD(wParam)) {
                        case IDOK:
                            StartSend(hwndDlg);
                        return TRUE;

                        case IDCANCEL:
                            if (started) {
                                TerminateThread(thread, 0);

                                if (stream != INVALID_HANDLE_VALUE) {
                                    FILE_DISPOSITION_INFO fdi;

                                    fdi.DeleteFile = TRUE;

                                    SetFileInformationByHandle(stream, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO));
                                    CloseHandle(stream);
                                }

                                if (dirh != INVALID_HANDLE_VALUE)
                                    CloseHandle(dirh);

                                started = FALSE;

                                SetDlgItemTextW(hwndDlg, IDCANCEL, closetext);

                                EnableWindow(GetDlgItem(hwnd, IDOK), TRUE);
                                EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), TRUE);
                                EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), TRUE);
                            } else
                                EndDialog(hwndDlg, 1);
                        return TRUE;

                        case IDC_BROWSE:
                            Browse(hwndDlg);
                        return TRUE;

                        case IDC_INCREMENTAL:
                            incremental = IsDlgButtonChecked(hwndDlg, LOWORD(wParam));

                            EnableWindow(GetDlgItem(hwnd, IDC_PARENT_SUBVOL), incremental);
                            EnableWindow(GetDlgItem(hwnd, IDC_PARENT_BROWSE), incremental);
                        return TRUE;

                        case IDC_PARENT_BROWSE:
                            BrowseParent(hwndDlg);
                        return TRUE;

                        case IDC_CLONE_ADD:
                            AddClone(hwndDlg);
                        return TRUE;

                        case IDC_CLONE_REMOVE:
                            RemoveClone(hwndDlg);
                        return TRUE;
                    }
                break;

                case LBN_SELCHANGE:
                    switch (LOWORD(wParam)) {
                        case IDC_CLONE_LIST:
                            EnableWindow(GetDlgItem(hwnd, IDC_CLONE_REMOVE), TRUE);
                        return TRUE;
                    }
                break;
            }
        break;
    }

    return FALSE;
}

static INT_PTR CALLBACK stub_SendDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsSend* bs;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bs = (BtrfsSend*)lParam;
    } else
        bs = (BtrfsSend*)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

    if (bs)
        return bs->SendDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return FALSE;
}

void BtrfsSend::Open(HWND hwnd, LPWSTR path) {
    subvol = path;

    if (DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_SEND_SUBVOL), hwnd, stub_SendDlgProc, (LPARAM)this) <= 0)
        ShowError(hwnd, GetLastError());
}

void CALLBACK SendSubvolGUIW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    HANDLE token;
    TOKEN_PRIVILEGES tp;
    LUID luid;
    BtrfsSend* bs;

    set_dpi_aware();

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    if (!LookupPrivilegeValueW(NULL, L"SeManageVolumePrivilege", &luid)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    if (!AdjustTokenPrivileges(token, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL)) {
        ShowError(hwnd, GetLastError());
        return;
    }

    bs = new BtrfsSend;

    bs->Open(hwnd, lpszCmdLine);

    delete bs;

    CloseHandle(token);
}

static void send_subvol(std::wstring subvol, std::wstring file, std::wstring parent, std::vector<std::wstring> clones) {
    char* buf;
    HANDLE dirh, stream;
    ULONG bss_size, i;
    btrfs_send_subvol* bss;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    btrfs_send_header header;
    btrfs_send_command end;
    BOOL success = FALSE;

    buf = (char*)malloc(SEND_BUFFER_LEN);

    dirh = CreateFileW(subvol.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
    if (dirh == INVALID_HANDLE_VALUE)
        goto end3;

    stream = CreateFileW(file.c_str(), FILE_WRITE_DATA | DELETE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (stream == INVALID_HANDLE_VALUE) {
        CloseHandle(dirh);
        goto end3;
    }

    bss_size = offsetof(btrfs_send_subvol, clones[0]) + (clones.size() * sizeof(HANDLE));
    bss = (btrfs_send_subvol*)malloc(bss_size);
    memset(bss, 0, bss_size);

    if (parent != L"") {
        HANDLE parenth;

        parenth = CreateFileW(parent.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (parenth == INVALID_HANDLE_VALUE)
            goto end2;

        bss->parent = parenth;
    } else
        bss->parent = NULL;

    bss->num_clones = clones.size();

    for (i = 0; i < bss->num_clones; i++) {
        HANDLE h;

        h = CreateFileW(clones[i].c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
        if (h == INVALID_HANDLE_VALUE) {
            ULONG j;

            for (j = 0; j < i; j++) {
                CloseHandle(bss->clones[j]);
            }

            if (bss->parent) CloseHandle(bss->parent);
            goto end2;
        }

        bss->clones[i] = h;
    }

    Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_SEND_SUBVOL, bss, bss_size, NULL, 0);

    for (i = 0; i < bss->num_clones; i++) {
        CloseHandle(bss->clones[i]);
    }

    if (bss->parent) CloseHandle(bss->parent);

    if (!NT_SUCCESS(Status))
        goto end2;

    memcpy(header.magic, BTRFS_SEND_MAGIC, sizeof(BTRFS_SEND_MAGIC));
    header.version = 1;

    if (!WriteFile(stream, &header, sizeof(header), NULL, NULL))
        goto end2;

    do {
        Status = NtFsControlFile(dirh, NULL, NULL, NULL, &iosb, FSCTL_BTRFS_READ_SEND_BUFFER, NULL, 0, buf, SEND_BUFFER_LEN);

        if (NT_SUCCESS(Status))
            WriteFile(stream, buf, iosb.Information, NULL, NULL);
    } while (NT_SUCCESS(Status));

    if (Status != STATUS_END_OF_FILE)
        goto end2;

    end.length = 0;
    end.cmd = BTRFS_SEND_CMD_END;
    end.csum = 0x9dc96c50;

    if (!WriteFile(stream, &end, sizeof(end), NULL, NULL))
        goto end2;

    SetEndOfFile(stream);

    success = TRUE;

end2:
    if (!success) {
        FILE_DISPOSITION_INFO fdi;

        fdi.DeleteFile = TRUE;

        SetFileInformationByHandle(stream, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO));
    }

    CloseHandle(dirh);
    CloseHandle(stream);

end3:
    free(buf);
}

void CALLBACK SendSubvolW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    LPWSTR* args;
    int num_args;
    std::wstring subvol = L"", parent = L"", file = L"";
    std::vector<std::wstring> clones;

    args = CommandLineToArgvW(lpszCmdLine, &num_args);

    if (!args)
        return;

    if (num_args >= 2) {
        HANDLE token;
        TOKEN_PRIVILEGES tp;
        LUID luid;
        int i;

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

        for (i = 0; i < num_args; i++) {
            if (args[i][0] == '-') {
                if (args[i][2] == 0 && i < num_args - 1) {
                    if (args[i][1] == 'p') {
                        parent = args[i+1];
                        i++;
                    } else if (args[i][1] == 'c') {
                        clones.push_back(args[i+1]);
                        i++;
                    }
                }
            } else {
                if (subvol == L"")
                    subvol = args[i];
                else if (file == L"")
                    file = args[i];
            }
        }

        if (subvol != L"" && file != L"")
            send_subvol(subvol, file, parent, clones);
    }

end:
    LocalFree(args);
}
