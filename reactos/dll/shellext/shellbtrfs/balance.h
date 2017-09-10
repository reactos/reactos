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

#include <windows.h>
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "btrfsioctl.h"
#endif

class BtrfsBalance {
public:
    BtrfsBalance(WCHAR* drive, BOOL RemoveDevice = FALSE, BOOL ShrinkDevice = FALSE) {
        removing = FALSE;
        devices = NULL;
        called_from_RemoveDevice = RemoveDevice;
        called_from_ShrinkDevice = ShrinkDevice;
        wcscpy(fn, drive);
    }

    void ShowBalance(HWND hwndDlg);
    INT_PTR CALLBACK BalanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK BalanceOptsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void ShowBalanceOptions(HWND hwndDlg, UINT8 type);
    void SaveBalanceOpts(HWND hwndDlg);
    void StartBalance(HWND hwndDlg);
    void RefreshBalanceDlg(HWND hwndDlg, BOOL first);
    void PauseBalance(HWND hwndDlg);
    void StopBalance(HWND hwndDlg);

    UINT32 balance_status;
    btrfs_balance_opts data_opts, metadata_opts, system_opts;
    UINT8 opts_type;
    btrfs_query_balance bqb;
    BOOL cancelling;
    BOOL removing;
    BOOL shrinking;
    WCHAR fn[MAX_PATH];
    btrfs_device* devices;
    BOOL readonly;
    BOOL called_from_RemoveDevice, called_from_ShrinkDevice;
};
