/*
 * SetupAPI dialog functions
 *
 * Copyright 2009 Ricardo Filipe
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

#include "setupapi_private.h"

struct promptdisk_params {
    PCWSTR DialogTitle;
    PCWSTR DiskName;
    PCWSTR PathToSource;
    PCWSTR FileSought;
    PCWSTR TagFile;
    DWORD DiskPromptStyle;
    PWSTR PathBuffer;
    DWORD PathBufferSize;
    PDWORD PathRequiredSize;
};

/* initiates the fields of the SetupPromptForDisk dialog according to the parameters
*/
static void promptdisk_init(HWND hwnd, struct promptdisk_params *params)
{
    SetWindowLongPtrW(hwnd, DWLP_USER, (LONG_PTR)params);

    if(params->DialogTitle)
        SetWindowTextW(hwnd, params->DialogTitle);
    if(params->PathToSource)
        SetDlgItemTextW(hwnd, IDC_PATH, params->PathToSource);

    if(!(params->DiskPromptStyle & IDF_OEMDISK))
    {
        WCHAR message[256+2*MAX_PATH];
        WCHAR format[256];
        WCHAR unknown[256];
        DWORD_PTR args[2];
        LoadStringW(SETUPAPI_hInstance, IDS_PROMPTDISK, format, ARRAY_SIZE(format));

        args[0] = (DWORD_PTR)params->FileSought;
        if(params->DiskName)
            args[1] = (DWORD_PTR)params->DiskName;
        else
        {
            LoadStringW(SETUPAPI_hInstance, IDS_UNKNOWN, unknown, ARRAY_SIZE(unknown));
            args[1] = (DWORD_PTR)unknown;
        }
        FormatMessageW(FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                       format, 0, 0, message, ARRAY_SIZE(message), (__ms_va_list*)args);
        SetDlgItemTextW(hwnd, IDC_FILENEEDED, message);

        LoadStringW(SETUPAPI_hInstance, IDS_INFO, message, ARRAY_SIZE(message));
        SetDlgItemTextW(hwnd, IDC_INFO, message);
        LoadStringW(SETUPAPI_hInstance, IDS_COPYFROM, message, ARRAY_SIZE(message));
        SetDlgItemTextW(hwnd, IDC_COPYFROM, message);
    }
    if(params->DiskPromptStyle & IDF_NOBROWSE)
        ShowWindow(GetDlgItem(hwnd, IDC_RUNDLG_BROWSE), SW_HIDE);
}

/* When the user clicks in the Ok button in SetupPromptForDisk dialog
 * if the parameters are good it copies the path from the dialog to the output buffer
 * saves the required size for the buffer if PathRequiredSize is given
 * returns NO_ERROR if there is no PathBuffer to copy too
 * returns DPROMPT_BUFFERTOOSMALL if the path is too big to fit in PathBuffer
 */
static void promptdisk_ok(HWND hwnd, struct promptdisk_params *params)
{
    int requiredSize;
    WCHAR aux[MAX_PATH];
    GetWindowTextW(GetDlgItem(hwnd, IDC_PATH), aux, MAX_PATH);
    requiredSize = strlenW(aux)+1;

    if(params->PathRequiredSize)
    {
        *params->PathRequiredSize = requiredSize;
        TRACE("returning PathRequiredSize=%d\n",*params->PathRequiredSize);
    }
    if(!params->PathBuffer)
    {
        EndDialog(hwnd, NO_ERROR);
        return;
    }
    if(requiredSize > params->PathBufferSize)
    {
        EndDialog(hwnd, DPROMPT_BUFFERTOOSMALL);
        return;
    }
    strcpyW(params->PathBuffer, aux);
    TRACE("returning PathBuffer=%s\n", debugstr_w(params->PathBuffer));
    EndDialog(hwnd, DPROMPT_SUCCESS);
}

/* When the user clicks the browse button in SetupPromptForDisk dialog
 * it copies the path of the selected file to the dialog path field
 */
static void promptdisk_browse(HWND hwnd, struct promptdisk_params *params)
{
    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
    ofn.hwndOwner = hwnd;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFile = HeapAlloc(GetProcessHeap(), 0, MAX_PATH*sizeof(WCHAR));
    strcpyW(ofn.lpstrFile, params->FileSought);

    if(GetOpenFileNameW(&ofn))
    {
        WCHAR* last_slash = strrchrW(ofn.lpstrFile, '\\');
        if (last_slash) *last_slash = 0;
        SetDlgItemTextW(hwnd, IDC_PATH, ofn.lpstrFile);
    }
    HeapFree(GetProcessHeap(), 0, ofn.lpstrFile);
}

/* Handles the messages sent to the SetupPromptForDisk dialog
*/
static INT_PTR CALLBACK promptdisk_proc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_INITDIALOG:
            promptdisk_init(hwnd, (struct promptdisk_params *)lParam);
            return TRUE;
        case WM_COMMAND:
            switch(wParam)
            {
                case IDOK:
                {
                    struct promptdisk_params *params =
                        (struct promptdisk_params *)GetWindowLongPtrW(hwnd, DWLP_USER);
                    promptdisk_ok(hwnd, params);
                    return TRUE;
                }
                case IDCANCEL:
                    EndDialog(hwnd, DPROMPT_CANCEL);
                    return TRUE;
                case IDC_RUNDLG_BROWSE:
                {
                    struct promptdisk_params *params =
                        (struct promptdisk_params *)GetWindowLongPtrW(hwnd, DWLP_USER);
                    promptdisk_browse(hwnd, params);
                    return TRUE;
                }
            }
    }
    return FALSE;
}

/***********************************************************************
 *      SetupPromptForDiskA (SETUPAPI.@)
 */
UINT WINAPI SetupPromptForDiskA(HWND hwndParent, PCSTR DialogTitle, PCSTR DiskName,
        PCSTR PathToSource, PCSTR FileSought, PCSTR TagFile, DWORD DiskPromptStyle,
        PSTR PathBuffer, DWORD PathBufferSize, PDWORD PathRequiredSize)
{
    WCHAR *DialogTitleW, *DiskNameW, *PathToSourceW;
    WCHAR *FileSoughtW, *TagFileW, PathBufferW[MAX_PATH];
    UINT ret, length;

    TRACE("%p, %s, %s, %s, %s, %s, 0x%08x, %p, %d, %p\n", hwndParent, debugstr_a(DialogTitle),
          debugstr_a(DiskName), debugstr_a(PathToSource), debugstr_a(FileSought),
          debugstr_a(TagFile), DiskPromptStyle, PathBuffer, PathBufferSize,
          PathRequiredSize);

    DialogTitleW = strdupAtoW(DialogTitle);
    DiskNameW = strdupAtoW(DiskName);
    PathToSourceW = strdupAtoW(PathToSource);
    FileSoughtW = strdupAtoW(FileSought);
    TagFileW = strdupAtoW(TagFile);

    ret = SetupPromptForDiskW(hwndParent, DialogTitleW, DiskNameW, PathToSourceW,
            FileSoughtW, TagFileW, DiskPromptStyle, PathBufferW, MAX_PATH, PathRequiredSize);

    HeapFree(GetProcessHeap(), 0, DialogTitleW);
    HeapFree(GetProcessHeap(), 0, DiskNameW);
    HeapFree(GetProcessHeap(), 0, PathToSourceW);
    HeapFree(GetProcessHeap(), 0, FileSoughtW);
    HeapFree(GetProcessHeap(), 0, TagFileW);

    if(ret == DPROMPT_SUCCESS)
    {
        length = WideCharToMultiByte(CP_ACP, 0, PathBufferW, -1, NULL, 0, NULL, NULL);
        if(PathRequiredSize) *PathRequiredSize = length;
        if(PathBuffer)
        {
            if(length > PathBufferSize)
                return DPROMPT_BUFFERTOOSMALL;
            WideCharToMultiByte(CP_ACP, 0, PathBufferW, -1, PathBuffer, length, NULL, NULL);
        }
    }
    return ret;
}

/***********************************************************************
 *      SetupPromptForDiskW (SETUPAPI.@)
 */
UINT WINAPI SetupPromptForDiskW(HWND hwndParent, PCWSTR DialogTitle, PCWSTR DiskName,
        PCWSTR PathToSource, PCWSTR FileSought, PCWSTR TagFile, DWORD DiskPromptStyle,
        PWSTR PathBuffer, DWORD PathBufferSize, PDWORD PathRequiredSize)
{
    struct promptdisk_params params;
    UINT ret;

    TRACE("%p, %s, %s, %s, %s, %s, 0x%08x, %p, %d, %p\n", hwndParent, debugstr_w(DialogTitle),
          debugstr_w(DiskName), debugstr_w(PathToSource), debugstr_w(FileSought),
          debugstr_w(TagFile), DiskPromptStyle, PathBuffer, PathBufferSize,
          PathRequiredSize);

    if(!FileSought)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return DPROMPT_CANCEL;
    }
    params.DialogTitle = DialogTitle;
    params.DiskName = DiskName;
    params.PathToSource = PathToSource;
    params.FileSought = FileSought;
    params.TagFile = TagFile;
    params.DiskPromptStyle = DiskPromptStyle;
    params.PathBuffer = PathBuffer;
    params.PathBufferSize = PathBufferSize;
    params.PathRequiredSize = PathRequiredSize;

    ret = DialogBoxParamW(SETUPAPI_hInstance, MAKEINTRESOURCEW(IDPROMPTFORDISK),
        hwndParent, promptdisk_proc, (LPARAM)&params);

    if(ret == DPROMPT_CANCEL)
        SetLastError(ERROR_CANCELLED);
    return ret;
}
