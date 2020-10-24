/*
 * PROJECT:     ReactOS Disk Cleanup
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Dialog Functions
 * COPYRIGHT:   Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "precomp.h"

BOOL IsSystemDrive;
DLGHANDLE DialogHandle;
HBITMAP BitmapMask;
WCHAR SelectedDriveLetter[3];
UINT CleanmgrWindowMsg;
static HBITMAP BitmapDrive = NULL;

INT_PTR CALLBACK StartDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {    
        case WM_INITDIALOG:
        {
            /* Load the bitmaps for combobox control */
            BitmapDrive = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_DRIVE));
            if (BitmapDrive == NULL)
            {
                DPRINT("LoadBitmapW(): Failed to open the bitmap!\n");
                EndDialog(hwnd, IDCANCEL);
                return FALSE;
            }

            BitmapMask = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_MASK));
            if (BitmapMask == NULL)
            {
                DPRINT("LoadBitmapW(): Failed to load the mask bitmap!\n");
                EndDialog(hwnd, IDCANCEL);
                return FALSE;
            }

            /* Start the initialization of the combobox control */
            InitStartDlgComboBox(hwnd, BitmapDrive);
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
            {
                lpmis->itemHeight = 18 + 2;
            }
            break;
        }

        case WM_DRAWITEM:
        {
            /* Draw the bitmaps for the combobox control */
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
                StringCbCopyW(SelectedDriveLetter, sizeof(SelectedDriveLetter), GetProperDriveLetter(hComboCtrl, ItemIndex));
            }

            switch (LOWORD(wParam))
            {
                case IDOK:
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
            DeleteObject(BitmapMask);
            break;

        default:
        {
            /* If the dialog box recieves the registered Window message call SetForegroundWindow() function */
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
    switch (message)
    {
        case WM_INITDIALOG:
        {
            HANDLE ThreadObj = NULL;
            WCHAR FullText[ARR_MAX_SIZE] = { 0 };
            WCHAR TempText[ARR_MAX_SIZE] = { 0 };
            WCHAR *TempPtr = NULL;

            IsSystemDrive = FALSE;
            LoadStringW(GetModuleHandleW(NULL), IDS_SCAN, TempText, _countof(TempText));
            StringCchPrintfW(FullText, sizeof(FullText), TempText, SelectedDriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_SCAN, FullText);

            /* In order to get the root drive, GetWindowsDirectoryW() function is getting
               called and then the drive letter is extracted from the retrieved path */
            GetWindowsDirectoryW(TempText, MAX_PATH);
            TempPtr = wcschr(TempText, L'\\');
            TempText[wcslen(TempText) - wcslen(TempPtr)] = L'\0';

            /* Checking if the drive letter specified by the user is of the root drive */
            if (wcscmp(TempText, SelectedDriveLetter) == 0)
            {
                IsSystemDrive = TRUE;
            }

            /* Create a thread for getting the size of removable directories. */
            ThreadObj = CreateThread(NULL, 0, &GetRemovableDirSize, (LPVOID)hwnd, 0, NULL);
            CloseHandle(ThreadObj);
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

INT_PTR CALLBACK TabParentDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_INITDIALOG:
        {
            /* Load and set the icon for the dialog box */
            HICON hbmIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_CLEANMGR));
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hbmIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hbmIcon);

            /* If the lParam value specified by DialogBoxParamW is false, set a title string with drive letter */
            if (!lParam)
            {
                WCHAR FullText[ARR_MAX_SIZE] = { 0 };
                WCHAR TempText[ARR_MAX_SIZE] = { 0 };

                LoadStringW(GetModuleHandleW(NULL), IDS_CHOICE_DLG_TITLE, TempText, _countof(TempText));
                StringCchPrintfW(FullText, sizeof(FullText), TempText, SelectedDriveLetter);
                SetWindowTextW(hwnd, FullText);
            }

            /* Start the initialization of the tab control */
            InitTabControl(hwnd, lParam);
            return TRUE;
        }

        case WM_NOTIFY:
        {
            /* Detect if the user selects different tab and then call TabControlSelChange() function */
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
                    WCHAR WarningText[ARR_MAX_SIZE] = { 0 };

                    LoadStringW(GetModuleHandleW(NULL), IDS_CONFIRMATION, WarningText, _countof(WarningText));
                    int MesgBox = MessageBoxW(hwnd, WarningText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
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
            if (DialogHandle.hChoicePage || DialogHandle.hOptionsPage)
            {
                DestroyWindow(DialogHandle.hChoicePage);
                DestroyWindow(DialogHandle.hOptionsPage);
            }
            else if (DialogHandle.hSagesetPage)
            {
                DestroyWindow(DialogHandle.hSagesetPage);
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
            HANDLE ThreadObj = NULL;

            /* Set the required title of the dialog box */
            LoadStringW(GetModuleHandleW(NULL), IDS_REMOVAL, TempText, _countof(TempText));
            StringCchPrintfW(FullText, sizeof(FullText), TempText, SelectedDriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_REMOVAL, FullText);

            /* Create a thread for removing required directories */
            ThreadObj = CreateThread(NULL, 0, &RemoveRequiredFolder, (LPVOID)hwnd, 0, NULL);
            CloseHandle(ThreadObj);
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
