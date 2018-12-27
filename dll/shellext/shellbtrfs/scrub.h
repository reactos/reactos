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

#pragma once

#include <windows.h>
#ifndef __REACTOS__
#include "../btrfs.h"
#include "../btrfsioctl.h"
#else
#include "btrfs.h"
#include "btrfsioctl.h"
#endif

class BtrfsScrub {
public:
    BtrfsScrub(const wstring& drive) {
        fn = drive;
    }

    INT_PTR CALLBACK ScrubDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam);

private:
    void RefreshScrubDlg(HWND hwndDlg, bool first_time);
    void UpdateTextBox(HWND hwndDlg, btrfs_query_scrub* bqs);
    void StartScrub(HWND hwndDlg);
    void PauseScrub(HWND hwndDlg);
    void StopScrub(HWND hwndDlg);

    wstring fn;
    uint32_t status;
    uint64_t chunks_left;
    uint32_t num_errors;
};
