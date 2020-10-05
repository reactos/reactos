/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Dialog Procs
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "precomp.h"

BOOL SystemDrive;
DLG_HANDLE DialogHandle;
WCHAR DriveLetter[ARR_MAX_SIZE];
UINT CleanmgrWindowMsg;

INT_PTR CALLBACK StartDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HBITMAP BitmapDrive = NULL;
    
    switch (message)
    {    
        case WM_INITDIALOG:
        {
            BitmapDrive = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_DRIVE));

            if (BitmapDrive == NULL)
            {
                DPRINT("LoadBitmapW(): Failed to open the bitmap!\n");
                EndDialog(hwnd, IDCANCEL);
                return FALSE;
            }

            InitStartDlg(hwnd, BitmapDrive);
            return TRUE;
        }

        case WM_NOTIFY:
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_MEASUREITEM:
        {
            LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

            if (lpmis->itemHeight < 18 + 2)
                lpmis->itemHeight = 18 + 2;

            break;
        }

        case WM_DRAWITEM:
        {
            if (!DrawItemCombobox(lParam))
            {
                DPRINT("DrawItemCombobox(): Failed to initialize the ComboBox!\n");
                EndDialog(hwnd, IDCANCEL);
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                HWND hComboCtrl = GetDlgItem(hwnd, IDC_DRIVE);
                int ItemIndex = SendMessageW(hComboCtrl, CB_GETCURSEL, 0, 0);
                StringCbCopyW(DriveLetter, sizeof(DriveLetter), GetProperDriveLetter(hComboCtrl, ItemIndex));
            }

            switch(LOWORD(wParam))
            {
                case IDOK:
                    DeleteObject(BitmapDrive);
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;  

        case WM_DESTROY:
            DeleteObject(BitmapDrive);
            break;

        default:
        {
            if (message == CleanmgrWindowMsg)
            {
                SetForegroundWindow(hwnd);
            }
            return FALSE;
        }
    }
    return TRUE;
}

INT_PTR CALLBACK ProgressDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{   
    switch(message)
    {
        case WM_INITDIALOG:
        {
            WCHAR FullText[ARR_MAX_SIZE] = { 0 };
            WCHAR SysDrive[ARR_MAX_SIZE] = { 0 };
            WCHAR TempText[ARR_MAX_SIZE] = { 0 };
            HANDLE ThreadOBJ = NULL;

            SystemDrive = FALSE;
            LoadStringW(GetModuleHandleW(NULL), IDS_SCAN, TempText, _countof(TempText));
            StringCchPrintfW(FullText, sizeof(FullText), TempText, DriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_SCAN, FullText);
            GetEnvironmentVariableW(L"SystemDrive", SysDrive, sizeof(SysDrive));
            if (wcscmp(SysDrive, DriveLetter) == 0)
            {
                SystemDrive = TRUE;
            }
            ThreadOBJ = CreateThread(NULL, 0, &GetRemovableDirSize, (LPVOID)hwnd, 0, NULL);
            CloseHandle(ThreadOBJ);
            SetForegroundWindow(hwnd);
            return TRUE;
        }

        case WM_NOTIFY:
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;

        default:
        {
            if (message == CleanmgrWindowMsg)
            {
                SetForegroundWindow(hwnd);
            }
            return FALSE;
        }
    }
    return TRUE;
}

INT_PTR CALLBACK ChoiceDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempText[ARR_MAX_SIZE] = { 0 };

    switch (message)
    {
        case WM_INITDIALOG:
        {
            WCHAR FullText[ARR_MAX_SIZE] = { 0 };
            HICON hbmIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_CLEANMGR));

            LoadStringW(GetModuleHandleW(NULL), IDS_CHOICE_DLG_TITLE, TempText, _countof(TempText));
            StringCchPrintfW(FullText, sizeof(FullText), TempText, DriveLetter);
            SetWindowTextW(hwnd, FullText);
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hbmIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hbmIcon);
            SetForegroundWindow(hwnd);
            return InitTabControl(hwnd);
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if ((pnmh->hwndFrom == DialogHandle.hTab) &&
                (pnmh->idFrom == IDC_TAB) &&
                (pnmh->code == TCN_SELCHANGE))
            {
                TabControlSelChange();
                break;
            }
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);
        }

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    int MesgBox = MessageBoxW(hwnd, TempText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                    switch (MesgBox)
                    {
                        case IDYES:
                            EndDialog(hwnd, IDOK);
                            break;

                        case IDNO:
                            break;
                    }
                    break;
                }
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;

        case WM_DESTROY:
            if (DialogHandle.hChoicePage)
            {
                DestroyWindow(DialogHandle.hChoicePage);
            }
            else if (DialogHandle.hOptionsPage)
            {
                DestroyWindow(DialogHandle.hOptionsPage);
            }
            break;

        default:
        {
            if (message == CleanmgrWindowMsg)
            {
                SetForegroundWindow(hwnd);
            }
            return FALSE;
        }
    }
    return TRUE;
}

INT_PTR CALLBACK ProgressEndDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            WCHAR FullText[ARR_MAX_SIZE] = { 0 };
            WCHAR TempText[ARR_MAX_SIZE] = { 0 };
            HANDLE ThreadOBJ = NULL;

            LoadStringW(GetModuleHandleW(NULL), IDS_REMOVAL, TempText, _countof(TempText));
            StringCchPrintfW(FullText, sizeof(FullText), TempText, DriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_REMOVAL, FullText);
            ThreadOBJ = CreateThread(NULL, 0, &FolderRemoval, (LPVOID)hwnd, 0, NULL);
            CloseHandle(ThreadOBJ);
            SetForegroundWindow(hwnd);
            return TRUE;
        }

        case WM_NOTIFY:
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCANCEL:
                    EndDialog(hwnd, 0);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, 0);
            break;

        default:
        {
            if (message == CleanmgrWindowMsg)
            {
                SetForegroundWindow(hwnd);
            }
            return FALSE;
        }
    }
    return TRUE;
}

INT_PTR CALLBACK SetStageFlagDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempText[ARR_MAX_SIZE] = { 0 };

    switch (message)
    {
        case WM_INITDIALOG:
            return InitStageFlagTabControl(hwnd);

        case WM_NOTIFY:
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                {
                    int MesgBox = MessageBoxW(hwnd, TempText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                    switch (MesgBox)
                    {
                        case IDYES:
                            EndDialog(hwnd, IDOK);
                            break;

                        case IDNO:
                            break;
                    }
                    break;
                }
                case IDCANCEL:
                    EndDialog(hwnd, IDCANCEL);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;

        case WM_DESTROY:
            DestroyWindow(DialogHandle.hSagesetPage);
            break;

        default:
        {
            if (message == CleanmgrWindowMsg)
            {
                SetForegroundWindow(hwnd);
            }
            return FALSE;
        }
    }
    return TRUE;
}
