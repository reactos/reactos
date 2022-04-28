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
#include <iostream>

#define SEND_BUFFER_LEN 1048576

DWORD BtrfsSend::Thread() {
    try {
        NTSTATUS Status;
        IO_STATUS_BLOCK iosb;
        btrfs_send_subvol* bss;
        btrfs_send_header header;
        btrfs_send_command end;
        ULONG i;

        buf = (char*)malloc(SEND_BUFFER_LEN);

        try {
            dirh = CreateFileW(subvol.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
            if (dirh == INVALID_HANDLE_VALUE)
                throw string_error(IDS_SEND_CANT_OPEN_DIR, subvol.c_str(), GetLastError(), format_message(GetLastError()).c_str());

            try {
                size_t bss_size = offsetof(btrfs_send_subvol, clones[0]) + (clones.size() * sizeof(HANDLE));
                bss = (btrfs_send_subvol*)malloc(bss_size);
                memset(bss, 0, bss_size);

                if (incremental) {
                    WCHAR parent[MAX_PATH];
                    HANDLE parenth;

                    parent[0] = 0;

                    GetDlgItemTextW(hwnd, IDC_PARENT_SUBVOL, parent, sizeof(parent) / sizeof(WCHAR));

                    parenth = CreateFileW(parent, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                    if (parenth == INVALID_HANDLE_VALUE)
                        throw string_error(IDS_SEND_CANT_OPEN_DIR, parent, GetLastError(), format_message(GetLastError()).c_str());

                    bss->parent = parenth;
                } else
                    bss->parent = nullptr;

                bss->num_clones = (ULONG)clones.size();

                for (i = 0; i < bss->num_clones; i++) {
                    HANDLE h;

                    h = CreateFileW(clones[i].c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                    if (h == INVALID_HANDLE_VALUE) {
                        auto le = GetLastError();
                        ULONG j;

                        for (j = 0; j < i; j++) {
                            CloseHandle(bss->clones[j]);
                        }

                        if (bss->parent) CloseHandle(bss->parent);

                        throw string_error(IDS_SEND_CANT_OPEN_DIR, clones[i].c_str(), le, format_message(le).c_str());
                    }

                    bss->clones[i] = h;
                }

                Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SEND_SUBVOL, bss, (ULONG)bss_size, nullptr, 0);

                for (i = 0; i < bss->num_clones; i++) {
                    CloseHandle(bss->clones[i]);
                }

                if (!NT_SUCCESS(Status)) {
                    if (Status == (NTSTATUS)STATUS_INVALID_PARAMETER) {
                        BY_HANDLE_FILE_INFORMATION fileinfo;
                        if (!GetFileInformationByHandle(dirh, &fileinfo)) {
                            auto le = GetLastError();
                            if (bss->parent) CloseHandle(bss->parent);
                            throw string_error(IDS_SEND_GET_FILE_INFO_FAILED, le, format_message(le).c_str());
                        }

                        if (!(fileinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                            if (bss->parent) CloseHandle(bss->parent);
                            throw string_error(IDS_SEND_NOT_READONLY);
                        }

                        if (bss->parent) {
                            if (!GetFileInformationByHandle(bss->parent, &fileinfo)) {
                                auto le = GetLastError();
                                CloseHandle(bss->parent);
                                throw string_error(IDS_SEND_GET_FILE_INFO_FAILED, le, format_message(le).c_str());
                            }

                            if (!(fileinfo.dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                                CloseHandle(bss->parent);
                                throw string_error(IDS_SEND_PARENT_NOT_READONLY);
                            }
                        }
                    }

                    if (bss->parent) CloseHandle(bss->parent);
                    throw string_error(IDS_SEND_FSCTL_BTRFS_SEND_SUBVOL_FAILED, Status, format_ntstatus(Status).c_str());
                }

                if (bss->parent) CloseHandle(bss->parent);

                stream = CreateFileW(file, FILE_WRITE_DATA | DELETE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
                if (stream == INVALID_HANDLE_VALUE)
                    throw string_error(IDS_SEND_CANT_OPEN_FILE, file, GetLastError(), format_message(GetLastError()).c_str());

                try {
                    memcpy(header.magic, BTRFS_SEND_MAGIC, sizeof(header.magic));
                    header.version = 1;

                    if (!WriteFile(stream, &header, sizeof(header), nullptr, nullptr))
                        throw string_error(IDS_SEND_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());

                    do {
                        Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_READ_SEND_BUFFER, nullptr, 0, buf, SEND_BUFFER_LEN);

                        if (NT_SUCCESS(Status)) {
                            if (!WriteFile(stream, buf, (DWORD)iosb.Information, nullptr, nullptr))
                                throw string_error(IDS_SEND_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());
                        }
                    } while (NT_SUCCESS(Status));

                    if (Status != STATUS_END_OF_FILE)
                        throw string_error(IDS_SEND_FSCTL_BTRFS_READ_SEND_BUFFER_FAILED, Status, format_ntstatus(Status).c_str());

                    end.length = 0;
                    end.cmd = BTRFS_SEND_CMD_END;
                    end.csum = 0x9dc96c50;

                    if (!WriteFile(stream, &end, sizeof(end), nullptr, nullptr))
                        throw string_error(IDS_SEND_WRITEFILE_FAILED, GetLastError(), format_message(GetLastError()).c_str());

                    SetEndOfFile(stream);
                } catch (...) {
                    FILE_DISPOSITION_INFO fdi;

                    fdi.DeleteFile = true;

                    Status = NtSetInformationFile(stream, &iosb, &fdi, sizeof(FILE_DISPOSITION_INFO), FileDispositionInformation);

                    CloseHandle(stream);
                    stream = INVALID_HANDLE_VALUE;

                    if (!NT_SUCCESS(Status))
                        throw ntstatus_error(Status);

                    throw;
                }

                CloseHandle(stream);
                stream = INVALID_HANDLE_VALUE;
            } catch (...) {
                CloseHandle(dirh);
                dirh = INVALID_HANDLE_VALUE;

                throw;
            }

            CloseHandle(dirh);
            dirh = INVALID_HANDLE_VALUE;
        } catch (...) {
            free(buf);
            buf = nullptr;

            started = false;

            SetDlgItemTextW(hwnd, IDCANCEL, closetext);
            EnableWindow(GetDlgItem(hwnd, IDOK), true);
            EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), true);
            EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), true);

            throw;
        }
    } catch (const exception& e) {
        auto msg = utf8_to_utf16(e.what());

        SetDlgItemTextW(hwnd, IDC_SEND_STATUS, msg.c_str());
        return 0;
    }


    free(buf);
    buf = nullptr;

    started = false;

    SetDlgItemTextW(hwnd, IDCANCEL, closetext);
    EnableWindow(GetDlgItem(hwnd, IDOK), true);
    EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), true);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), true);

    wstring success;

    load_string(module, IDS_SEND_SUCCESS, success);

    SetDlgItemTextW(hwnd, IDC_SEND_STATUS, success.c_str());

    return 0;
}

static DWORD WINAPI send_thread(LPVOID lpParameter) {
    BtrfsSend* bs = (BtrfsSend*)lpParameter;

    return bs->Thread();
}

void BtrfsSend::StartSend(HWND hwnd) {
    wstring s;
    HWND cl;

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

    started = true;

    wstring writing;

    load_string(module, IDS_SEND_WRITING, writing);

    SetDlgItemTextW(hwnd, IDC_SEND_STATUS, writing.c_str());

    load_string(module, IDS_SEND_CANCEL, s);
    SetDlgItemTextW(hwnd, IDCANCEL, s.c_str());

    EnableWindow(GetDlgItem(hwnd, IDOK), false);
    EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), false);
    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), false);

    clones.clear();

    cl = GetDlgItem(hwnd, IDC_CLONE_LIST);
    auto num_clones = SendMessageW(cl, LB_GETCOUNT, 0, 0);

    if (num_clones != LB_ERR) {
        for (unsigned int i = 0; i < (unsigned int)num_clones; i++) {
            WCHAR* t;

            auto len = SendMessageW(cl, LB_GETTEXTLEN, i, 0);
            t = (WCHAR*)malloc((len + 1) * sizeof(WCHAR));

            SendMessageW(cl, LB_GETTEXT, i, (LPARAM)t);

            clones.push_back(t);

            free(t);
        }
    }

    thread = CreateThread(nullptr, 0, send_thread, this, 0, nullptr);

    if (!thread)
        throw last_error(GetLastError());
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
    ofn.Flags = OFN_EXPLORER;

    if (!GetSaveFileNameW(&ofn))
        return;

    SetDlgItemTextW(hwnd, IDC_STREAM_DEST, file);
}

void BtrfsSend::BrowseParent(HWND hwnd) {
    BROWSEINFOW bi;
    PIDLIST_ABSOLUTE root, pidl;
    HRESULT hr;
    WCHAR parent[MAX_PATH], volpathw[MAX_PATH];
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    if (!GetVolumePathNameW(subvol.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1))
        throw string_error(IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    hr = SHParseDisplayName(volpathw, 0, &root, 0, 0);
    if (FAILED(hr))
        throw string_error(IDS_SHPARSEDISPLAYNAME_FAILED);

    memset(&bi, 0, sizeof(BROWSEINFOW));

    bi.hwndOwner = hwnd;
    bi.pidlRoot = root;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;

    pidl = SHBrowseForFolderW(&bi);

    if (!pidl)
        return;

    if (!SHGetPathFromIDListW(pidl, parent))
        throw string_error(IDS_SHGETPATHFROMIDLIST_FAILED);

    {
        win_handle h = CreateFileW(parent, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_SEND_CANT_OPEN_DIR, parent, GetLastError(), format_message(GetLastError()).c_str());

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_FILE_IDS, nullptr, 0, &bgfi, sizeof(btrfs_get_file_ids));
        if (!NT_SUCCESS(Status))
            throw string_error(IDS_GET_FILE_IDS_FAILED, Status, format_ntstatus(Status).c_str());
    }

    if (bgfi.inode != 0x100 || bgfi.top)
        throw string_error(IDS_NOT_SUBVOL);

    SetDlgItemTextW(hwnd, IDC_PARENT_SUBVOL, parent);
}

void BtrfsSend::AddClone(HWND hwnd) {
    BROWSEINFOW bi;
    PIDLIST_ABSOLUTE root, pidl;
    HRESULT hr;
    WCHAR path[MAX_PATH], volpathw[MAX_PATH];
    NTSTATUS Status;
    IO_STATUS_BLOCK iosb;
    btrfs_get_file_ids bgfi;

    if (!GetVolumePathNameW(subvol.c_str(), volpathw, (sizeof(volpathw) / sizeof(WCHAR)) - 1))
        throw string_error(IDS_RECV_GETVOLUMEPATHNAME_FAILED, GetLastError(), format_message(GetLastError()).c_str());

    hr = SHParseDisplayName(volpathw, 0, &root, 0, 0);
    if (FAILED(hr))
        throw string_error(IDS_SHPARSEDISPLAYNAME_FAILED);

    memset(&bi, 0, sizeof(BROWSEINFOW));

    bi.hwndOwner = hwnd;
    bi.pidlRoot = root;
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_USENEWUI | BIF_NONEWFOLDERBUTTON;

    pidl = SHBrowseForFolderW(&bi);

    if (!pidl)
        return;

    if (!SHGetPathFromIDListW(pidl, path))
        throw string_error(IDS_SHGETPATHFROMIDLIST_FAILED);

    {
        win_handle h = CreateFileW(path, FILE_TRAVERSE, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (h == INVALID_HANDLE_VALUE)
            throw string_error(IDS_SEND_CANT_OPEN_DIR, path, GetLastError(), format_message(GetLastError()).c_str());

        Status = NtFsControlFile(h, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_GET_FILE_IDS, nullptr, 0, &bgfi, sizeof(btrfs_get_file_ids));
        if (!NT_SUCCESS(Status))
            throw string_error(IDS_GET_FILE_IDS_FAILED, Status, format_ntstatus(Status).c_str());
    }

    if (bgfi.inode != 0x100 || bgfi.top)
        throw string_error(IDS_NOT_SUBVOL);

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
        EnableWindow(GetDlgItem(hwnd, IDC_CLONE_REMOVE), false);
}

INT_PTR BtrfsSend::SendDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    try {
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
                            return true;

                            case IDCANCEL:
                                if (started) {
                                    TerminateThread(thread, 0);

                                    if (stream != INVALID_HANDLE_VALUE) {
                                        NTSTATUS Status;
                                        FILE_DISPOSITION_INFO fdi;
                                        IO_STATUS_BLOCK iosb;

                                        fdi.DeleteFile = true;

                                        Status = NtSetInformationFile(stream, &iosb, &fdi, sizeof(FILE_DISPOSITION_INFO), FileDispositionInformation);

                                        CloseHandle(stream);

                                        if (!NT_SUCCESS(Status))
                                            throw ntstatus_error(Status);
                                    }

                                    if (dirh != INVALID_HANDLE_VALUE)
                                        CloseHandle(dirh);

                                    started = false;

                                    SetDlgItemTextW(hwndDlg, IDCANCEL, closetext);

                                    EnableWindow(GetDlgItem(hwnd, IDOK), true);
                                    EnableWindow(GetDlgItem(hwnd, IDC_STREAM_DEST), true);
                                    EnableWindow(GetDlgItem(hwnd, IDC_BROWSE), true);
                                } else
                                    EndDialog(hwndDlg, 1);
                            return true;

                            case IDC_BROWSE:
                                Browse(hwndDlg);
                            return true;

                            case IDC_INCREMENTAL:
                                incremental = IsDlgButtonChecked(hwndDlg, LOWORD(wParam));

                                EnableWindow(GetDlgItem(hwnd, IDC_PARENT_SUBVOL), incremental);
                                EnableWindow(GetDlgItem(hwnd, IDC_PARENT_BROWSE), incremental);
                            return true;

                            case IDC_PARENT_BROWSE:
                                BrowseParent(hwndDlg);
                            return true;

                            case IDC_CLONE_ADD:
                                AddClone(hwndDlg);
                            return true;

                            case IDC_CLONE_REMOVE:
                                RemoveClone(hwndDlg);
                            return true;
                        }
                    break;

                    case LBN_SELCHANGE:
                        switch (LOWORD(wParam)) {
                            case IDC_CLONE_LIST:
                                EnableWindow(GetDlgItem(hwnd, IDC_CLONE_REMOVE), true);
                            return true;
                        }
                    break;
                }
            break;
        }
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }

    return false;
}

static INT_PTR CALLBACK stub_SendDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    BtrfsSend* bs;

    if (uMsg == WM_INITDIALOG) {
        SetWindowLongPtrW(hwndDlg, GWLP_USERDATA, (LONG_PTR)lParam);
        bs = (BtrfsSend*)lParam;
    } else
        bs = (BtrfsSend*)GetWindowLongPtrW(hwndDlg, GWLP_USERDATA);

    if (bs)
        return bs->SendDlgProc(hwndDlg, uMsg, wParam, lParam);
    else
        return false;
}

void BtrfsSend::Open(HWND hwnd, LPWSTR path) {
    subvol = path;

    if (DialogBoxParamW(module, MAKEINTRESOURCEW(IDD_SEND_SUBVOL), hwnd, stub_SendDlgProc, (LPARAM)this) <= 0)
        throw last_error(GetLastError());
}

extern "C" void CALLBACK SendSubvolGUIW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
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

        BtrfsSend bs;

        bs.Open(hwnd, lpszCmdLine);
    } catch (const exception& e) {
        error_message(hwnd, e.what());
    }
}

static void send_subvol(const wstring& subvol, const wstring& file, const wstring& parent, const vector<wstring>& clones) {
    char* buf;
    win_handle dirh, stream;
    ULONG i;
    btrfs_send_subvol* bss;
    IO_STATUS_BLOCK iosb;
    NTSTATUS Status;
    btrfs_send_header header;
    btrfs_send_command end;

    buf = (char*)malloc(SEND_BUFFER_LEN);

    try {
        dirh = CreateFileW(subvol.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
        if (dirh == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        stream = CreateFileW(file.c_str(), FILE_WRITE_DATA | DELETE, 0, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (stream == INVALID_HANDLE_VALUE)
            throw last_error(GetLastError());

        try {
            size_t bss_size = offsetof(btrfs_send_subvol, clones[0]) + (clones.size() * sizeof(HANDLE));
            bss = (btrfs_send_subvol*)malloc(bss_size);
            memset(bss, 0, bss_size);

            if (parent != L"") {
                HANDLE parenth;

                parenth = CreateFileW(parent.c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                if (parenth == INVALID_HANDLE_VALUE)
                    throw last_error(GetLastError());

                bss->parent = parenth;
            } else
                bss->parent = nullptr;

            bss->num_clones = (ULONG)clones.size();

            for (i = 0; i < bss->num_clones; i++) {
                HANDLE h;

                h = CreateFileW(clones[i].c_str(), FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
                if (h == INVALID_HANDLE_VALUE) {
                    auto le = GetLastError();
                    ULONG j;

                    for (j = 0; j < i; j++) {
                        CloseHandle(bss->clones[j]);
                    }

                    if (bss->parent) CloseHandle(bss->parent);

                    throw last_error(le);
                }

                bss->clones[i] = h;
            }

            Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_SEND_SUBVOL, bss, (ULONG)bss_size, nullptr, 0);

            for (i = 0; i < bss->num_clones; i++) {
                CloseHandle(bss->clones[i]);
            }

            if (bss->parent) CloseHandle(bss->parent);

            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            memcpy(header.magic, BTRFS_SEND_MAGIC, sizeof(header.magic));
            header.version = 1;

            if (!WriteFile(stream, &header, sizeof(header), nullptr, nullptr))
                throw last_error(GetLastError());

            do {
                Status = NtFsControlFile(dirh, nullptr, nullptr, nullptr, &iosb, FSCTL_BTRFS_READ_SEND_BUFFER, nullptr, 0, buf, SEND_BUFFER_LEN);

                if (NT_SUCCESS(Status))
                    WriteFile(stream, buf, (DWORD)iosb.Information, nullptr, nullptr);
            } while (NT_SUCCESS(Status));

            if (Status != STATUS_END_OF_FILE)
                throw ntstatus_error(Status);

            end.length = 0;
            end.cmd = BTRFS_SEND_CMD_END;
            end.csum = 0x9dc96c50;

            if (!WriteFile(stream, &end, sizeof(end), nullptr, nullptr))
                throw last_error(GetLastError());

            SetEndOfFile(stream);
        } catch (...) {
            FILE_DISPOSITION_INFO fdi;

            fdi.DeleteFile = true;

            Status = NtSetInformationFile(stream, &iosb, &fdi, sizeof(FILE_DISPOSITION_INFO), FileDispositionInformation);
            if (!NT_SUCCESS(Status))
                throw ntstatus_error(Status);

            throw;
        }
    } catch (...) {
        free(buf);
        throw;
    }

    free(buf);
}

extern "C" void CALLBACK SendSubvolW(HWND hwnd, HINSTANCE hinst, LPWSTR lpszCmdLine, int nCmdShow) {
    vector<wstring> args;
    wstring subvol = L"", parent = L"", file = L"";
    vector<wstring> clones;

    command_line_to_args(lpszCmdLine, args);

    if (args.size() >= 2) {
        TOKEN_PRIVILEGES tp;
        LUID luid;

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

        for (unsigned int i = 0; i < args.size(); i++) {
            if (args[i][0] == '-') {
                if (args[i][2] == 0 && i < args.size() - 1) {
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

        if (subvol != L"" && file != L"") {
            try {
                send_subvol(subvol, file, parent, clones);
            } catch (const exception& e) {
                cerr << "Error: " << e.what() << endl;
            }
        }
    }
}