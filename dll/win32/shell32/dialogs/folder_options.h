/*
 *    Folder Options
 *
 * Copyright 2007 Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2016-2018 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

// IDD_FOLDER_OPTIONS_GENERAL
INT_PTR
CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

// IDD_FOLDER_OPTIONS_VIEW
INT_PTR CALLBACK
FolderOptionsViewDlg(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam);

// IDD_FOLDER_OPTIONS_FILETYPES
INT_PTR CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam);

HBITMAP Create24BppBitmap(HDC hDC, INT cx, INT cy);
HBITMAP BitmapFromIcon(HICON hIcon, INT cx, INT cy);
HBITMAP CreateCheckImage(HDC hDC, BOOL bCheck, BOOL bEnabled = TRUE);
HBITMAP CreateCheckMask(HDC hDC);
HBITMAP CreateRadioImage(HDC hDC, BOOL bCheck, BOOL bEnabled = TRUE);
HBITMAP CreateRadioMask(HDC hDC);

extern LPCWSTR g_pszShell32;
extern LPCWSTR g_pszSpace;
