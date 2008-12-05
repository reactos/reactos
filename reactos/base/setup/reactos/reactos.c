/*
 *  ReactOS applications
 *  Copyright (C) 2004-2008 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS GUI first stage setup application
 * FILE:        subsys/system/reactos/reactos.c
 * PROGRAMMERS: Eric Kohl
 *              Matthias Kupfer
 *              Dmitry Chapyshev (dmitry@reactos.org)
 */

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>
#include <setupapi.h>
#include <initguid.h>
#include <devguid.h>
#include <wine/unicode.h>

#include "resource.h"

/* GLOBALS ******************************************************************/

HFONT hTitleFont;

typedef struct _LANG
{
    TCHAR LangId[9];
    TCHAR LangName[128];
} LANG, *PLANG;

typedef struct _KBLAYOUT
{
    TCHAR LayoutId[9];
    TCHAR LayoutName[128];
    TCHAR DllName[128];
} KBLAYOUT, *PKBLAYOUT;


// generic entries with simple 1:1 mapping
typedef struct _GENENTRY
{
    TCHAR Id[24];
    TCHAR Value[128];
} GENENTRY, *PGENENTRY;

struct
{
    // Settings
    LONG DestDiskNumber; // physical disk
    LONG DestPartNumber; // partition on disk
    LONG DestPartSize; // if partition doesn't exist, size of partition
    LONG FSType; // file system type on partition 
    LONG MBRInstallType; // install bootloader
    LONG FormatPart; // type of format the partition
    LONG SelectedLangId; // selected language (table index)
    LONG SelectedKBLayout; // selected keyboard layout (table index)
    TCHAR InstallDir[MAX_PATH]; // installation directory on hdd
    LONG SelectedComputer; // selected computer type (table index)
    LONG SelectedDisplay; // selected display type (table index)
    LONG SelectedKeyboard; // selected keyboard type (table index)
    BOOLEAN RepairUpdateFlag; // flag for update/repair an installed reactos
    // txtsetup.sif data
    LONG DefaultLang; // default language (table index)
    PLANG pLanguages;
    LONG LangCount;
    LONG DefaultKBLayout; // default keyboard layout (table index)
    PKBLAYOUT pKbLayouts;
    LONG KbLayoutCount;
    PGENENTRY pComputers;
    LONG CompCount;
    PGENENTRY pDisplays;
    LONG DispCount;
    PGENENTRY pKeyboards;
    LONG KeybCount;
} SetupData;

typedef struct _IMGINFO
{
    HBITMAP hBitmap;
    INT cxSource;
    INT cySource;
} IMGINFO, *PIMGINFO;

TCHAR abort_msg[512], abort_title[64];
HINSTANCE hInstance;
BOOL isUnattend;

/* FUNCTIONS ****************************************************************/

static VOID
CenterWindow(HWND hWnd)
{
    HWND hWndParent;
    RECT rcParent;
    RECT rcWindow;

    hWndParent = GetParent(hWnd);
    if (hWndParent == NULL)
        hWndParent = GetDesktopWindow();

    GetWindowRect(hWndParent, &rcParent);
    GetWindowRect(hWnd, &rcWindow);

    SetWindowPos(hWnd,
                 HWND_TOP,
                 ((rcParent.right - rcParent.left) - (rcWindow.right - rcWindow.left)) / 2,
                 ((rcParent.bottom - rcParent.top) - (rcWindow.bottom - rcWindow.top)) / 2,
                 0,
                 0,
                 SWP_NOSIZE);
}

static HFONT
CreateTitleFont(VOID)
{
    NONCLIENTMETRICS ncm;
    LOGFONT LogFont;
    HDC hdc;
    INT FontSize;
    HFONT hFont;

    ncm.cbSize = sizeof(NONCLIENTMETRICS);
    SystemParametersInfo(SPI_GETNONCLIENTMETRICS, 0, &ncm, 0);

    LogFont = ncm.lfMessageFont;
    LogFont.lfWeight = FW_BOLD;
    _tcscpy(LogFont.lfFaceName, _T("MS Shell Dlg"));

    hdc = GetDC(NULL);
    FontSize = 12;
    LogFont.lfHeight = 0 - GetDeviceCaps (hdc, LOGPIXELSY) * FontSize / 72;
    hFont = CreateFontIndirect(&LogFont);
    ReleaseDC(NULL, hdc);

    return hFont;
}

static VOID
InitImageInfo(PIMGINFO ImgInfo)
{
    BITMAP bitmap;

    ZeroMemory(ImgInfo, sizeof(*ImgInfo));

    ImgInfo->hBitmap = LoadImage(hInstance,
                                 MAKEINTRESOURCE(IDB_ROSLOGO),
                                 IMAGE_BITMAP,
                                 0,
                                 0,
                                 LR_DEFAULTCOLOR);

    if (ImgInfo->hBitmap != NULL)
    {
        GetObject(ImgInfo->hBitmap, sizeof(BITMAP), &bitmap);

        ImgInfo->cxSource = bitmap.bmWidth;
        ImgInfo->cySource = bitmap.bmHeight;
    }
}

static INT_PTR CALLBACK
StartDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            /* Center the wizard window */
            CenterWindow (hwndControl);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            /* Hide and disable the 'Cancel' button at the moment,
             * we use this button to cancel the setup process
             * like F3 in usetup
             */
            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow (hwndControl, SW_HIDE);
            EnableWindow (hwndControl, FALSE);
            
            /* Set title font */
            SendDlgItemMessage(hwndDlg,
                               IDC_STARTTITLE,
                               WM_SETFONT,
                               (WPARAM)hTitleFont,
                               (LPARAM)TRUE);
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                break;
                default:
                break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

static INT_PTR CALLBACK
LangSelDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    PIMGINFO pImgInfo;
    LONG i;
    LRESULT tindex;
    HWND hList;

    pImgInfo = (PIMGINFO)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
            
            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow (hwndControl, SW_SHOW);
            EnableWindow (hwndControl, TRUE);

            pImgInfo = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IMGINFO));
            if (pImgInfo == NULL)
            {
                EndDialog(hwndDlg, 0);
                return FALSE;
            }

            SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pImgInfo);

            InitImageInfo(pImgInfo);

            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/

            hList = GetDlgItem(hwndDlg, IDC_LANGUAGES);

            for (i=0; i < SetupData.LangCount; i++)
            {
                tindex = SendMessage(hList, CB_ADDSTRING, (WPARAM) 0, (LPARAM) SetupData.pLanguages[i].LangName);
                SendMessage(hList, CB_SETITEMDATA, tindex, i);
                if (SetupData.DefaultLang == i)
                SendMessage(hList, CB_SETCURSEL, (WPARAM) tindex,(LPARAM) 0);
            }

            hList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT);

            for (i=0; i < SetupData.KbLayoutCount; i++)
            {
                tindex = SendMessage(hList, CB_ADDSTRING, (WPARAM) 0, (LPARAM)SetupData.pKbLayouts[i].LayoutName);
                SendMessage(hList, CB_SETITEMDATA, tindex, i);
                if (SetupData.DefaultKBLayout == i)
                SendMessage(hList,CB_SETCURSEL,(WPARAM)tindex,(LPARAM)0);
            }
        }
        break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDrawItem;
            lpDrawItem = (LPDRAWITEMSTRUCT) lParam;

            if (lpDrawItem->CtlID == IDB_ROSLOGO)
            {
                HDC hdcMem;
                LONG left;

                /* position image in centre of dialog */
                left = (lpDrawItem->rcItem.right - pImgInfo->cxSource) / 2;

                hdcMem = CreateCompatibleDC(lpDrawItem->hDC);
                if (hdcMem != NULL)
                {
                    SelectObject(hdcMem, pImgInfo->hBitmap);
                    BitBlt(lpDrawItem->hDC,
                           left,
                           lpDrawItem->rcItem.top,
                           lpDrawItem->rcItem.right - lpDrawItem->rcItem.left,
                           lpDrawItem->rcItem.bottom - lpDrawItem->rcItem.top,
                           hdcMem,
                           0,
                           0,
                           SRCCOPY);
                    DeleteDC(hdcMem);
                }
            }
            return TRUE;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLong(hwndDlg,
                                  DWL_MSGRESULT,
                                  MessageBox(GetParent(hwndDlg),
                                             abort_msg,
                                             abort_title,
                                             MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: // set the selected data
                {
                    hList =GetDlgItem(hwndDlg, IDC_LANGUAGES); 
                    tindex = SendMessage(hList,CB_GETCURSEL, (WPARAM)0, (LPARAM)0);

                    if (tindex != CB_ERR)
                    {
                        SetupData.SelectedLangId = SendMessage(hList, CB_GETITEMDATA, (WPARAM) tindex, (LPARAM) 0);
                        WORD LangID = _tcstol(SetupData.pLanguages[SetupData.SelectedLangId].LangId, NULL, 16);
                        SetThreadLocale(MAKELCID(LangID, SORT_DEFAULT));
                        // FIXME: need to reload all resource to force
                        // the new language setting
                    }

                    hList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT); 
                    tindex = SendMessage(hList,CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
                    if (tindex != CB_ERR)
                    {
                        SetupData.SelectedKBLayout = SendMessage(hList, CB_GETITEMDATA, (WPARAM) tindex, (LPARAM) 0);
                    }
                    return TRUE;
                }

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }
    return FALSE;
}

static INT_PTR CALLBACK
TypeDlgProc(HWND hwndDlg,
            UINT uMsg,
            WPARAM wParam,
            LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            CheckDlgButton(hwndDlg, IDC_INSTALL, BST_CHECKED);
            
            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                break;

                case PSN_QUERYCANCEL:
                    SetWindowLong(hwndDlg,
                                  DWL_MSGRESULT,
                                  MessageBox(GetParent(hwndDlg),
                                             abort_msg,
                                             abort_title,
                                             MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: // set the selected data
                    SetupData.RepairUpdateFlag = !(SendMessage(GetDlgItem(hwndDlg, IDC_INSTALL),
                                                               BM_GETCHECK,
                                                               (WPARAM) 0,
                                                               (LPARAM) 0) == BST_CHECKED);
                    return TRUE;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }
    return FALSE;
}

static INT_PTR CALLBACK
DeviceDlgProc(HWND hwndDlg,
              UINT uMsg,
              WPARAM wParam,
              LPARAM lParam)
{
    LONG i;
    LRESULT tindex;
    HWND hList;

    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/

            hList = GetDlgItem(hwndDlg, IDC_COMPUTER);

            for (i=0; i < SetupData.CompCount; i++)
            {
                tindex = SendMessage(hList, CB_ADDSTRING, (WPARAM) 0, (LPARAM) SetupData.pComputers[i].Value);
                SendMessage(hList, CB_SETITEMDATA, tindex, i);
            }
            SendMessage(hList, CB_SETCURSEL, 0, 0); // set first as default

            hList = GetDlgItem(hwndDlg, IDC_DISPLAY);

            for (i=0; i < SetupData.DispCount; i++)
            {
                tindex = SendMessage(hList, CB_ADDSTRING, (WPARAM) 0, (LPARAM) SetupData.pDisplays[i].Value);
                SendMessage(hList, CB_SETITEMDATA, tindex, i);
            }
            SendMessage(hList, CB_SETCURSEL, 0, 0); // set first as default

            hList = GetDlgItem(hwndDlg, IDC_KEYBOARD);

            for (i=0; i < SetupData.KeybCount; i++)
            {
                tindex = SendMessage(hList,CB_ADDSTRING,(WPARAM)0,(LPARAM)SetupData.pKeyboards[i].Value);
                SendMessage(hList,CB_SETITEMDATA,tindex,i);
            }
            SendMessage(hList,CB_SETCURSEL,0,0); // set first as default
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLong(hwndDlg,
                                  DWL_MSGRESULT,
                                  MessageBox(GetParent(hwndDlg),
                                             abort_msg,
                                             abort_title,
                                             MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                case PSN_WIZNEXT: // set the selected data
                {
                    hList = GetDlgItem(hwndDlg, IDC_COMPUTER); 

                    tindex = SendMessage(hList, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (tindex != CB_ERR)
                    {
                        SetupData.SelectedComputer = SendMessage(hList,
                                                                 CB_GETITEMDATA,
                                                                 (WPARAM) tindex,
                                                                 (LPARAM) 0);
                    }

                    hList = GetDlgItem(hwndDlg, IDC_DISPLAY);

                    tindex = SendMessage(hList, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (tindex != CB_ERR)
                    {
                        SetupData.SelectedDisplay = SendMessage(hList,
                                                                CB_GETITEMDATA,
                                                                (WPARAM) tindex,
                                                                (LPARAM) 0);
                    }

                    hList =GetDlgItem(hwndDlg, IDC_KEYBOARD);

                    tindex = SendMessage(hList, CB_GETCURSEL, (WPARAM) 0, (LPARAM) 0);
                    if (tindex != CB_ERR)
                    {
                        SetupData.SelectedKeyboard = SendMessage(hList,
                                                                 CB_GETITEMDATA,
                                                                 (WPARAM) tindex,
                                                                 (LPARAM) 0);
                    }
                    return TRUE;
                }

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }
    return FALSE;
}

static INT_PTR CALLBACK
MoreOptDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            CheckDlgButton(hwndDlg, IDC_INSTFREELDR, BST_CHECKED);
            SendMessage(GetDlgItem(hwndDlg, IDC_PATH),
                        WM_SETTEXT,
                        (WPARAM) 0,
                        (LPARAM) SetupData.InstallDir);
        }
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                {
                    SendMessage(GetDlgItem(hwndDlg, IDC_PATH),
                                WM_GETTEXT,
                                (WPARAM) sizeof(SetupData.InstallDir) / sizeof(TCHAR),
                                (LPARAM) SetupData.InstallDir);
                    
                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                }

                case IDCANCEL:
                {
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
                }
            }
        }
    }

    return FALSE;
}

static INT_PTR CALLBACK
PartitionDlgProc(HWND hwndDlg,
                 UINT uMsg,
                 WPARAM wParam,
                 LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            break;
        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDOK:
                    EndDialog(hwndDlg, IDOK);
                    return TRUE;
                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    return TRUE;
            }
        }
    }
    return FALSE;
}

static INT_PTR CALLBACK
DriveDlgProc(HWND hwndDlg,
             UINT uMsg,
             WPARAM wParam,
             LPARAM lParam)
{
#if 0
    HDEVINFO h;
    HWND hList;
    SP_DEVINFO_DATA DevInfoData;
    DWORD i;
#endif
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
#if 0
            h = SetupDiGetClassDevs(&GUID_DEVCLASS_DISKDRIVE, NULL, NULL, DIGCF_PRESENT);
            if (h != INVALID_HANDLE_VALUE)
            {
                hList =GetDlgItem(hwndDlg, IDC_PARTITION); 
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                for (i=0; SetupDiEnumDeviceInfo(h, i, &DevInfoData); i++)
                {
                    DWORD DataT;
                    LPTSTR buffer = NULL;
                    DWORD buffersize = 0;

                    while (!SetupDiGetDeviceRegistryProperty(h,
                                                             &DevInfoData,
                                                             SPDRP_DEVICEDESC,
                                                             &DataT,
                                                             (PBYTE)buffer,
                                                             buffersize,
                                                             &buffersize))
                    {
                        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
                        {
                            if (buffer) LocalFree(buffer);
                            buffer = LocalAlloc(LPTR, buffersize * 2);
                        }
                        else
                            break;
                    }
                    if (buffer)
                    {
                        SendMessage(hList, LB_ADDSTRING, (WPARAM) 0, (LPARAM) buffer);
                        LocalFree(buffer);
                    }
                }
                SetupDiDestroyDeviceInfoList(h);
            }
#endif
        }
        break;

        case WM_COMMAND:
        {
            switch(LOWORD(wParam))
            {
                case IDC_PARTMOREOPTS:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_BOOTOPTIONS),
                              hwndDlg,
                              (DLGPROC) MoreOptDlgProc);
                    break;
                case IDC_PARTCREATE:
                    DialogBox(hInstance,
                              MAKEINTRESOURCE(IDD_PARTITION),
                              hwndDlg,
                              (DLGPROC) PartitionDlgProc);
                    break;
                case IDC_PARTDELETE:
                    break;
            }
            break;
        }

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE:
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                case PSN_QUERYCANCEL:
                    SetWindowLong(hwndDlg,
                                  DWL_MSGRESULT,
                                  MessageBox(GetParent(hwndDlg),
                                             abort_msg,
                                             abort_title,
                                             MB_YESNO | MB_ICONQUESTION) != IDYES);
                    return TRUE;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

static INT_PTR CALLBACK
SummaryDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow(hwndControl, SW_HIDE);
            EnableWindow(hwndControl, FALSE);

            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE: 
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT | PSWIZB_BACK);
                    break;

                default:
                    break;
            }
        }
        break;

        default:
            break;
    }

    return FALSE;
}

static INT_PTR CALLBACK
ProcessDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            hwndControl = GetDlgItem(GetParent(hwndDlg), IDCANCEL);
            ShowWindow(hwndControl, SW_HIDE);
            EnableWindow(hwndControl, FALSE);

            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
        }
        break;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE: 
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
                   // disable all buttons during installation process
                   // PropSheet_SetWizButtons(GetParent(hwndDlg), 0 );
                   break;

                default:
                   break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

static INT_PTR CALLBACK
RestartDlgProc(HWND hwndDlg,
               UINT uMsg,
               WPARAM wParam,
               LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
        {
            HWND hwndControl;
            DWORD dwStyle;

            hwndControl = GetParent(hwndDlg);

            dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
            SetWindowLong(hwndControl, GWL_STYLE, dwStyle & ~WS_SYSMENU);
        
            /* Set title font */
            /*SendDlgItemMessage(hwndDlg,
                                 IDC_STARTTITLE,
                                 WM_SETFONT,
                                 (WPARAM)hTitleFont,
                                 (LPARAM)TRUE);*/
        }
        break;

        case WM_TIMER:
        {
            INT Position;
            HWND hWndProgress;

            hWndProgress = GetDlgItem(hwndDlg, IDC_RESTART_PROGRESS);
            Position = SendMessage(hWndProgress, PBM_GETPOS, 0, 0);
            if (Position == 300)
            {
                KillTimer(hwndDlg, 1);
                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_FINISH);
            }
            else
            {
                SendMessage(hWndProgress, PBM_SETPOS, Position + 1, 0);
            }
            return TRUE;
        }

        case WM_DESTROY:
            return TRUE;

        case WM_NOTIFY:
        {
            LPNMHDR lpnm = (LPNMHDR)lParam;

            switch (lpnm->code)
            {        
                case PSN_SETACTIVE: // Only "Finish" for closing the App
                {
                    PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_FINISH);
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETRANGE, 0, MAKELPARAM(0, 300));
                    SendDlgItemMessage(hwndDlg, IDC_RESTART_PROGRESS, PBM_SETPOS, 0, 0);
                    SetTimer(hwndDlg, 1, 50, NULL);
                }
                break;

                default:
                    break;
            }
        }
        break;

        default:
            break;

    }

    return FALSE;
}

void LoadSetupData()
{
    WCHAR szPath[MAX_PATH];
    TCHAR tmp[10];
    WCHAR *ch;
    HINF hTxtsetupSif;
    INFCONTEXT InfContext;
    //TCHAR szValue[MAX_PATH];
    DWORD LineLength;
    LONG Count;

    GetModuleFileNameW(NULL,szPath,MAX_PATH);
    ch = strrchrW(szPath,L'\\');
    if (ch != NULL)
        *ch = L'\0';

    wcscat(szPath, L"\\txtsetup.sif");
    hTxtsetupSif = SetupOpenInfFileW(szPath, NULL, INF_STYLE_OLDNT, NULL);
    if (hTxtsetupSif != INVALID_HANDLE_VALUE)
    {
        // get language list
        Count = SetupGetLineCount(hTxtsetupSif, _T("Language"));
        if (Count > 0)
        {
            SetupData.pLanguages = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(LANG) * Count);
            if (SetupData.pLanguages != NULL)
            {
                SetupData.LangCount = Count;
                Count = 0;
                if (SetupFindFirstLine(hTxtsetupSif, _T("Language"), NULL, &InfContext))
                {
                    do
                    {
                        SetupGetStringField(&InfContext,
                                            0,
                                            SetupData.pLanguages[Count].LangId,
                                            sizeof(SetupData.pLanguages[Count].LangId) / sizeof(TCHAR),
                                            &LineLength);

                        SetupGetStringField(&InfContext,
                                            1,
                                            SetupData.pLanguages[Count].LangName,
                                            sizeof(SetupData.pLanguages[Count].LangName) / sizeof(TCHAR),
                                            &LineLength);
                        ++Count;
                    }
                while (SetupFindNextLine(&InfContext, &InfContext) && Count < SetupData.LangCount);
                }
            }
        }

        // get keyboard layout list
        Count = SetupGetLineCount(hTxtsetupSif, _T("KeyboardLayout"));
        if (Count > 0)
        {
            SetupData.pKbLayouts = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(KBLAYOUT) * Count);
            if (SetupData.pKbLayouts != NULL)
            {
                SetupData.KbLayoutCount = Count;
                Count = 0;
                if (SetupFindFirstLine(hTxtsetupSif, _T("KeyboardLayout"), NULL, &InfContext))
                {
                    do
                    {
                        SetupGetStringField(&InfContext,
                                            0,
                                            SetupData.pKbLayouts[Count].LayoutId,
                                            sizeof(SetupData.pKbLayouts[Count].LayoutId) / sizeof(TCHAR),
                                            &LineLength);

                        SetupGetStringField(&InfContext,
                                            1,
                                            SetupData.pKbLayouts[Count].LayoutName,
                                            sizeof(SetupData.pKbLayouts[Count].LayoutName) / sizeof(TCHAR),
                                            &LineLength);
                        ++Count;
                    }
                    while (SetupFindNextLine(&InfContext, &InfContext) && Count < SetupData.LangCount);
                }
            }
        }

        // get default for keyboard and language
        SetupData.DefaultKBLayout = -1;
        SetupData.DefaultLang = -1;

        // TODO: get defaults from underlaying running system
        if (SetupFindFirstLine(hTxtsetupSif, _T("NLS"), _T("DefaultLayout"), &InfContext))
        {
            SetupGetStringField(&InfContext, 1, tmp, sizeof(tmp) / sizeof(TCHAR), &LineLength);
            for (Count = 0; Count < SetupData.KbLayoutCount; Count++)
                if (_tcscmp(tmp, SetupData.pKbLayouts[Count].LayoutId) == 0)
                {
                    SetupData.DefaultKBLayout = Count;
                    break;
                }
        }

        if (SetupFindFirstLine(hTxtsetupSif, _T("NLS"), _T("DefaultLanguage"), &InfContext))
        {
            SetupGetStringField(&InfContext, 1, tmp, sizeof(tmp) / sizeof(TCHAR), &LineLength);
            for (Count = 0; Count < SetupData.LangCount; Count++)
                if (_tcscmp(tmp, SetupData.pLanguages[Count].LangId) == 0)
                {
                    SetupData.DefaultLang = Count;
                    break;
                }
        }

        // get computers list
        Count = SetupGetLineCount(hTxtsetupSif, _T("Computer"));
        if (Count > 0)
        {
            SetupData.pComputers = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GENENTRY) * Count);
            if (SetupData.pComputers != NULL)
            {
                SetupData.CompCount = Count;
                Count = 0;
                if (SetupFindFirstLine(hTxtsetupSif, _T("Computer"), NULL, &InfContext))
                {
                    do
                    {
                        SetupGetStringField(&InfContext,
                                            0,
                                            SetupData.pComputers[Count].Id,
                                            sizeof(SetupData.pComputers[Count].Id) / sizeof(TCHAR),
                                            &LineLength);

                        SetupGetStringField(&InfContext,
                                            1,
                                            SetupData.pComputers[Count].Value,
                                            sizeof(SetupData.pComputers[Count].Value) / sizeof(TCHAR),
                                            &LineLength);
                        ++Count;
                    }
                    while (SetupFindNextLine(&InfContext, &InfContext) && Count < SetupData.CompCount);
                }
            }
        }

        // get display list
        Count = SetupGetLineCount(hTxtsetupSif, _T("Display"));
        if (Count > 0)
        {
            SetupData.pDisplays = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GENENTRY) * Count);
            if (SetupData.pDisplays != NULL)
            {
                SetupData.DispCount = Count;
                Count = 0;

                if (SetupFindFirstLine(hTxtsetupSif, _T("Display"), NULL, &InfContext))
                {
                    do
                    {
                        SetupGetStringField(&InfContext,
                                            0,
                                            SetupData.pDisplays[Count].Id,
                                            sizeof(SetupData.pDisplays[Count].Id) / sizeof(TCHAR),
                                            &LineLength);

                        SetupGetStringField(&InfContext,
                                            1,
                                            SetupData.pDisplays[Count].Value,
                                            sizeof(SetupData.pDisplays[Count].Value) / sizeof(TCHAR),
                                            &LineLength);
                        ++Count;
                    }
                    while (SetupFindNextLine(&InfContext, &InfContext) && Count < SetupData.DispCount);
                }
            }
        }

        // get keyboard list
        Count = SetupGetLineCount(hTxtsetupSif, _T("Keyboard"));
        if (Count > 0)
        {
            SetupData.pKeyboards = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(GENENTRY) * Count);
            if (SetupData.pKeyboards != NULL)
            {
                SetupData.KeybCount = Count;
                Count = 0;

                if (SetupFindFirstLine(hTxtsetupSif, _T("Keyboard"), NULL, &InfContext))
                {
                    do
                    {
                        SetupGetStringField(&InfContext,
                                            0,
                                            SetupData.pKeyboards[Count].Id,
                                            sizeof(SetupData.pKeyboards[Count].Id) / sizeof(TCHAR),
                                            &LineLength);

                        SetupGetStringField(&InfContext,
                                            1,
                                            SetupData.pKeyboards[Count].Value,
                                            sizeof(SetupData.pKeyboards[Count].Value) / sizeof(TCHAR),
                                            &LineLength);
                        ++Count;
                    }
                    while (SetupFindNextLine(&InfContext, &InfContext) && Count < SetupData.KeybCount);
                }
            }
        }

        // get install directory
        if (SetupFindFirstLine(hTxtsetupSif, _T("SetupData"), _T("DefaultPath"), &InfContext))
        {
            SetupGetStringField(&InfContext,
                                1,
                                SetupData.InstallDir,
                                sizeof(SetupData.InstallDir) / sizeof(TCHAR),
                                &LineLength);
        }
        SetupCloseInfFile(hTxtsetupSif);
    }
}

BOOL isUnattendSetup()
{
    WCHAR szPath[MAX_PATH];
    WCHAR *ch;
    HINF hUnattendedInf;
    INFCONTEXT InfContext;
    TCHAR szValue[MAX_PATH];
    DWORD LineLength;
    //HKEY hKey;
    BOOL result = 0;

    GetModuleFileNameW(NULL, szPath, MAX_PATH);
    ch = strrchrW(szPath, L'\\');
    if (ch != NULL)
        *ch = L'\0';

    wcscat(szPath, L"\\unattend.inf");
    hUnattendedInf = SetupOpenInfFileW(szPath, NULL, INF_STYLE_OLDNT, NULL);

    if (hUnattendedInf != INVALID_HANDLE_VALUE)
    {
        if (SetupFindFirstLine(hUnattendedInf, _T("Unattend"), _T("UnattendSetupEnabled"),&InfContext))
        {
            if (SetupGetStringField(&InfContext,
                                    1,
                                    szValue,
                                    sizeof(szValue) / sizeof(TCHAR),
                                    &LineLength) && (_tcsicmp(szValue, _T("yes")) == 0))
            {
                result = 1; // unattendSetup enabled
                // read values and store in SetupData
            }
        }
            SetupCloseInfFile(hUnattendedInf);
    }

    return result;
}

int WINAPI
_tWinMain(HINSTANCE hInst,
          HINSTANCE hPrevInstance,
          LPTSTR lpszCmdLine,
          int nCmdShow)
{
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE ahpsp[8];
    PROPSHEETPAGE psp = {0};
    UINT nPages = 0;
    hInstance = hInst;
    isUnattend = isUnattendSetup();

    if (!isUnattend)
    {
        LoadString(hInst,IDS_ABORTSETUP, abort_msg, sizeof(abort_msg)/sizeof(TCHAR));
        LoadString(hInst,IDS_ABORTSETUP2, abort_title,sizeof(abort_title)/sizeof(TCHAR));

        LoadSetupData();

        /* Create the Start page, until setup is working */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_HIDEHEADER;
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = StartDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_STARTPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create language selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LANGTITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LANGSUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = LangSelDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_LANGSELPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create install type selection page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_TYPETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_TYPESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = TypeDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_TYPEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create device settings page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DEVICETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DEVICESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = DeviceDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DEVICEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create install device settings page / boot method / install directory */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRIVETITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRIVESUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = DriveDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DRIVEPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);

        /* Create summary page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_SUMMARYTITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_SUMMARYSUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = SummaryDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_SUMMARYPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);
    }

    /* Create installation progress page */
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_PROCESSTITLE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_PROCESSSUBTITLE);
    psp.hInstance = hInst;
    psp.lParam = 0;
    psp.pfnDlgProc = ProcessDlgProc;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_PROCESSPAGE);
    ahpsp[nPages++] = CreatePropertySheetPage(&psp);

    if (!isUnattend)
    {
        /* Create finish to reboot page */
        psp.dwSize = sizeof(PROPSHEETPAGE);
        psp.dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_RESTARTTITLE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_RESTARTSUBTITLE);
        psp.hInstance = hInst;
        psp.lParam = 0;
        psp.pfnDlgProc = RestartDlgProc;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_RESTARTPAGE);
        ahpsp[nPages++] = CreatePropertySheetPage(&psp);
    }

    /* Create the property sheet */
    psh.dwSize = sizeof(PROPSHEETHEADER);
    psh.dwFlags = PSH_WIZARD97 | PSH_WATERMARK | PSH_HEADER;
    psh.hInstance = hInst;
    psh.hwndParent = NULL;
    psh.nPages = nPages;
    psh.nStartPage = 0;
    psh.phpage = ahpsp;
    psh.pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    psh.pszbmHeader = MAKEINTRESOURCE(IDB_HEADER);

    /* Create title font */
    hTitleFont = CreateTitleFont();

    /* Display the wizard */
    PropertySheet(&psh);

    DeleteObject(hTitleFont);

    return 0;
}

/* EOF */
