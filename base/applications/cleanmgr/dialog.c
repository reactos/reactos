/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Dialog Procs
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "precomp.h"

INT_PTR CALLBACK StartDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    HBITMAP BitmapDrive = NULL;
    switch(message)
    {
        case WM_INITDIALOG:
        {
            BitmapDrive = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_DRIVE));
 
            if (BitmapDrive == NULL)
            {
                MessageBoxW(NULL, L"LoadBitmapW() failed!", L"Error", MB_OK | MB_ICONERROR);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
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
            if(!DrawItemCombobox(lParam))
            {
                MessageBoxW(NULL, L"DrawItemCombobox() failed!", L"Error", MB_OK | MB_ICONERROR);
                PostMessage(hwnd, WM_CLOSE, 0, 0);
            }
            break;
        }

        case WM_COMMAND:
            if (HIWORD(wParam) == CBN_SELCHANGE)
            {
                int ItemIndex = SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETCURSEL, 0, 0);
                WCHAR StringComboBox[ARR_MAX_SIZE] = { 0 };
                WCHAR *GatheredDriveLatter = NULL;
                if (ItemIndex == CB_ERR)
                {
                    MessageBoxW(NULL, L"SendMessageW failed!", L"Error", MB_OK | MB_ICONERROR);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
                SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)StringComboBox);
                StringComboBox[wcslen(StringComboBox) - 1] = L'\0';
                GatheredDriveLatter = wcsrchr(StringComboBox, L':');
                GatheredDriveLatter--;
                StringCbCopyW(wcv.DriveLetter, _countof(wcv.DriveLetter), GatheredDriveLatter);
            }

            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (wcv.DriveLetter == NULL)
                    {
                        WCHAR TempText[ARR_MAX_SIZE] = { 0 };
                        LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_DRIVE, TempText, _countof(TempText));
                        MessageBoxW(hwnd, TempText, L"Warning", MB_OK | MB_ICONWARNING);
                        break;
                    }
                    DeleteObject(BitmapDrive);
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;

        case WM_CLOSE:
            DeleteObject(BitmapDrive);
            EndDialog(hwnd, IDCANCEL);
            break;  

        default:
            return FALSE;
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

            bv.SysDrive = FALSE;
            LoadStringW(GetModuleHandleW(NULL), IDS_SCAN, TempText, _countof(TempText));
            StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_SCAN, FullText);
            GetEnvironmentVariableW(L"SystemDrive", SysDrive, _countof(SysDrive));
            if (wcscmp(SysDrive, wcv.DriveLetter) == 0)
            {
                bv.SysDrive = TRUE;
            }
            ThreadOBJ = CreateThread(NULL, 0, &SizeCheck, (LPVOID)hwnd, 0, NULL);
            CloseHandle(ThreadOBJ);
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
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;
        case WM_DESTROY:
            EndDialog(hwnd, 0);
            break;
        default:
            return FALSE;
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
            HICON hbmIcon = LoadIconW(GetModuleHandleW(NULL), MAKEINTRESOURCE(IDI_DRIVE));
            LoadStringW(GetModuleHandleW(NULL), IDS_CHOICE_DLG_TITLE, TempText, _countof(TempText));
            StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
            SetWindowTextW(hwnd, FullText);
            SendMessageW(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hbmIcon);
            SendMessageW(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hbmIcon);
            return OnCreate(hwnd);
        }

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;
            if ((pnmh->hwndFrom == dv.hTab) &&
                (pnmh->idFrom == IDC_TAB) &&
                (pnmh->code == TCN_SELCHANGE))
            {
                OnTabWndSelChange();
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
                    HWND hList = GetDlgItem(dv.hChoicePage, IDC_CHOICE_LIST);
                    int NumOfItems = ListView_GetItemCount(hList);
                    BOOL ItemChecked = FALSE;
                    for (int i = 0; i < NumOfItems; i++)
                    {
                        if (ListView_GetCheckState(hList, i))
                        {
                            ItemChecked = TRUE;
                            break;
                        }
                    }
                    
                    if (ItemChecked == FALSE)
                    {
                        LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_OPTION, TempText, _countof(TempText));
                        MessageBoxW(hwnd, TempText, L"Warning", MB_OK | MB_ICONWARNING);
                        break;
                    }

                    LoadStringW(GetModuleHandleW(NULL), IDS_CONFIRM_DELETION, TempText, _countof(TempText));
                    int MesgBox = MessageBoxW(hwnd, TempText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                    switch (MesgBox)
                    {
                        case IDYES:
                            if (dv.hChoicePage)
                            {
                                DestroyWindow(dv.hChoicePage);
                            }

                            if (dv.hOptionsPage)
                            {
                                DestroyWindow(dv.hOptionsPage);
                            }

                            EndDialog(hwnd, IDOK);
                            break;

                        case IDNO:
                            break;
                    }
                    break;
                }
                case IDCANCEL:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
    
        case WM_CLOSE:
            if (dv.hChoicePage)
            {
                DestroyWindow(dv.hChoicePage);
            }

            if (dv.hOptionsPage)
            {
                DestroyWindow(dv.hOptionsPage);
            }

            EndDialog(hwnd, IDCANCEL);
            break;
        
        default:
            return FALSE;
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
        StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
        SetDlgItemTextW(hwnd, IDC_STATIC_REMOVAL, FullText);
        ThreadOBJ = CreateThread(NULL, 0, &FolderRemoval, (LPVOID)hwnd, 0, NULL);
        CloseHandle(ThreadOBJ);
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
                PostMessage(hwnd, WM_CLOSE, 0, 0);
                break;
        }
        break;

    case WM_CLOSE:
        EndDialog(hwnd, 0);
        break;

    case WM_DESTROY:
        EndDialog(hwnd, 0);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK SagesetDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempText[ARR_MAX_SIZE] = { 0 };

    switch (message)
    {
        case WM_INITDIALOG:
            return OnCreateSageset(hwnd);

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
                    HWND hList = GetDlgItem(dv.hSagesetPage, IDC_SAGESET_LIST);
                    int NumOfItems = ListView_GetItemCount(hList);
                    BOOL ItemChecked = FALSE;
                    for (int i = 0; i < NumOfItems; i++)
                    {
                        if (ListView_GetCheckState(hList, i))
                        {
                            ItemChecked = TRUE;
                            break;
                        }
                    }

                    if (ItemChecked == FALSE)
                    {
                        LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_OPTION, TempText, _countof(TempText));
                        MessageBoxW(hwnd, TempText, L"Warning", MB_OK | MB_ICONWARNING);
                        break;
                    }

                    LoadStringW(GetModuleHandleW(NULL), IDS_CONFIRM_CONFIG, TempText, _countof(TempText));
                    int MesgBox = MessageBoxW(hwnd, TempText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                    switch (MesgBox)
                    {
                        case IDYES:
                            DestroyWindow(dv.hSagesetPage);
                            EndDialog(hwnd, IDOK);
                            break;

                        case IDNO:
                            break;
                    }
                    break;
                }
                case IDCANCEL:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;
    
        case WM_CLOSE:
            DestroyWindow(dv.hSagesetPage);
            EndDialog(hwnd, IDCANCEL);
            break;
        
        default:
            return FALSE;
    }
    return TRUE;
}
