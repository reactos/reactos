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

#include <windows.h>
#ifndef __REACTOS__
#include "../btrfsioctl.h"
#else
#include "btrfsioctl.h"
#endif

class BtrfsBalance {
public:
    BtrfsBalance(const wstring& drive, bool RemoveDevice = false, bool ShrinkDevice = false) {
        removing = false;
        devices = nullptr;
        called_from_RemoveDevice = RemoveDevice;
        called_from_ShrinkDevice = ShrinkDevice;
        fn = drive;
    }

    void ShowBalance(HWND hwndDlg);
    INT_PTR CALLBACK BalanceDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);
    INT_PTR CALLBACK BalanceOptsDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void ShowBalanceOptions(HWND hwndDlg, uint8_t type);
    void SaveBalanceOpts(HWND hwndDlg);
    void StartBalance(HWND hwndDlg);
    void RefreshBalanceDlg(HWND hwndDlg, bool first);
    void PauseBalance(HWND hwndDlg);
    void StopBalance(HWND hwndDlg);

    uint32_t balance_status;
    btrfs_balance_opts data_opts, metadata_opts, system_opts;
    uint8_t opts_type;
    btrfs_query_balance bqb;
    bool cancelling;
    bool removing;
    bool shrinking;
    wstring fn;
    btrfs_device* devices;
    bool readonly;
    bool called_from_RemoveDevice, called_from_ShrinkDevice;
};
