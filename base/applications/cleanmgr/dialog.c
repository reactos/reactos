/*
 * PROJECT:         ReactOS Disk Cleanup
 * LICENSE:         GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:         Dialog Procs
 * COPYRIGHT:       Copyright 2020 Arnav Bhatt (arnavbhatt288 at gmail dot com)
 */
 
#include "resource.h"
#include "precomp.h"

#define CX_BITMAP 20
#define CY_BITMAP 20

void InitStartDlg(HWND hwnd, HBITMAP hBitmap);

INT_PTR CALLBACK StartDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempText[MAX_PATH] = { 0 };
    HBITMAP BitmapDrive = NULL;
    switch(message)
    {
        case WM_INITDIALOG:
        {
            BitmapDrive = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_DRIVE));
 
            if(BitmapDrive == NULL)
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
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                int ItemIndex = SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETCURSEL, 0, 0);
                if (ItemIndex == CB_ERR)
                {
                    MessageBoxW(NULL, L"SendMessageW failed!", L"Error", MB_OK | MB_ICONERROR);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                }
                SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETLBTEXT, (WPARAM)ItemIndex, (LPARAM)wcv.DriveLetter);
            }

            switch(LOWORD(wParam))
            {
                case IDOK:
                    if (wcslen(wcv.DriveLetter) == 0)
                    {
                        ZeroMemory(&TempText, sizeof(TempText));
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

void InitStartDlg(HWND hwnd, HBITMAP hBitmap)
{
    DWORD DwIndex = 0;
    DWORD NumOfDrives = 0;
    HWND hComboCtrl = NULL;
    WCHAR LogicalDrives[MAX_PATH] = { 0 };

    hComboCtrl = GetDlgItem(hwnd, IDC_DRIVE);

    if(hComboCtrl == NULL)
    {
        MessageBoxW(NULL, L"GetDlgItem() failed!", L"Error", MB_OK | MB_ICONERROR);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }
  
    NumOfDrives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);
    if (NumOfDrives == 0)
    {
        MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK | MB_ICONERROR);
        PostMessage(hwnd, WM_CLOSE, 0, 0);
        return;
    }

    if (NumOfDrives <= MAX_PATH)
    {
        WCHAR* SingleDrive = LogicalDrives;
        WCHAR RealDrive[MAX_PATH] = { 0 };
        while (*SingleDrive)
        {
            if (GetDriveTypeW(SingleDrive) == ONLY_PHYSICAL_DRIVE)
            {
                StringCchCopyW(RealDrive, MAX_PATH, SingleDrive);
                RealDrive[wcslen(RealDrive) - 1] = '\0';
                DwIndex = SendMessageW(hComboCtrl, CB_ADDSTRING, 0, (LPARAM)RealDrive);
                if (SendMessageW(hComboCtrl, CB_SETITEMDATA, DwIndex, (LPARAM)hBitmap) == CB_ERR)
                {
                    MessageBoxW(NULL, L"SendMessageW() failed!", L"Error", MB_OK | MB_ICONERROR);
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    return;
                }
                memset(RealDrive, 0, sizeof RealDrive);
            }
            SingleDrive += wcslen(SingleDrive) + 1;
        }
    }
   
    ComboBox_SetCurSel(hComboCtrl, 0);
    SendMessageW(hComboCtrl, CB_GETLBTEXT, (WPARAM)0, (LPARAM)wcv.DriveLetter);
}

BOOL DrawItemCombobox(LPARAM lParam)
{
    COLORREF ClrBackground, ClrForeground;
    TEXTMETRIC tm;
    int x, y;
    size_t cch;
    HBITMAP BitmapIcon = NULL;
    WCHAR achTemp[256] = { 0 };
    HBITMAP BitmapMask = NULL;

    LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

    if (lpdis->itemID == -1)
        return FALSE;

    BitmapMask = LoadBitmapW(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDB_MASK));
 
   if(BitmapMask == NULL)
    {
        DPRINT("LoadBitmapW(): Failed to load the mask bitmap!\n");
        return FALSE;
    }
    
    BitmapIcon = (HBITMAP)lpdis->itemData;

    if (BitmapIcon == NULL)
    {
        DPRINT("LoadBitmapW(): Failed to load BitmapIcon bitmap!\n");
        return FALSE;
    }
            
    ClrForeground = SetTextColor(lpdis->hDC,
        GetSysColor(lpdis->itemState & ODS_SELECTED ?
            COLOR_HIGHLIGHTTEXT : COLOR_WINDOWTEXT));

    ClrBackground = SetBkColor(lpdis->hDC,
        GetSysColor(lpdis->itemState & ODS_SELECTED ?
            COLOR_HIGHLIGHT : COLOR_WINDOW));

    GetTextMetrics(lpdis->hDC, &tm);
    y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;
    x = LOWORD(GetDialogBaseUnits()) / 4;

    SendMessage(lpdis->hwndItem, CB_GETLBTEXT,
                lpdis->itemID, (LPARAM)achTemp);

    StringCchLength(achTemp, _countof(achTemp), &cch);

    ExtTextOut(lpdis->hDC, CX_BITMAP + 2 * x, y,
        ETO_CLIPPED | ETO_OPAQUE, &lpdis->rcItem,
        achTemp, (UINT)cch, NULL);

    SetTextColor(lpdis->hDC, ClrForeground);
    SetBkColor(lpdis->hDC, ClrBackground);

    HDC hdc = CreateCompatibleDC(lpdis->hDC);
    if (hdc == NULL)
    {
        DPRINT("CreateCompatibleDC(): Failed to create a valid handle!\n");
        return FALSE;
    }

    SelectObject(hdc, BitmapMask);
    BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
           CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCAND);

    SelectObject(hdc, BitmapIcon);
    BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
           CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCPAINT);

    DeleteDC(hdc);

    if (lpdis->itemState & ODS_FOCUS)
        DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

    return TRUE;
}

INT_PTR CALLBACK ProgressDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE ThreadOBJ = NULL;
    
    switch(message)
    {
        case WM_INITDIALOG:
        {
            WCHAR FullText[MAX_PATH] = { 0 };
            WCHAR SysDrive[MAX_PATH] = { 0 };
            WCHAR TempText[MAX_PATH] = { 0 };

            bv.SysDrive = FALSE;
            LoadStringW(GetModuleHandleW(NULL), IDS_SCAN, TempText, _countof(TempText));
            StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
            SetDlgItemTextW(hwnd, IDC_STATIC_SCAN, FullText);
            GetEnvironmentVariableW(L"SystemDrive", SysDrive, _countof(SysDrive));
            if(wcscmp(SysDrive, wcv.DriveLetter) == 0)
            {
                bv.SysDrive = TRUE;
            }
            ThreadOBJ = CreateThread(NULL, 0, &SizeCheck, (LPVOID)hwnd, 0, NULL);
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
            CloseHandle(ThreadOBJ);
            EndDialog(hwnd, IDCANCEL);
            break;
        case WM_DESTROY:
            CloseHandle(ThreadOBJ);
            EndDialog(hwnd, 0);
            break;
        default:
            return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK ChoiceDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempText[MAX_PATH] = { 0 };

    switch (message)
    {
        case WM_INITDIALOG:
        {
            WCHAR FullText[MAX_PATH] = { 0 };
            LoadStringW(GetModuleHandleW(NULL), IDS_TITLE, TempText, _countof(TempText));
            StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
            SetWindowTextW(hwnd, FullText);
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
                            if(dv.hChoicePage)
                                DestroyWindow(dv.hChoicePage);

                            if (dv.hOptionsPage)
                                DestroyWindow(dv.hOptionsPage);

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
            if(dv.hChoicePage)
                DestroyWindow(dv.hChoicePage);

            if (dv.hOptionsPage)
                DestroyWindow(dv.hOptionsPage);

            EndDialog(hwnd, IDCANCEL);
            break;
        
        default:
            return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK ProgressEndDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE ThreadOBJ = NULL;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        WCHAR FullText[MAX_PATH] = { 0 };
        WCHAR TempText[MAX_PATH] = { 0 };

        LoadStringW(GetModuleHandleW(NULL), IDS_REMOVAL, TempText, _countof(TempText));
        StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
        SetDlgItemTextW(hwnd, IDC_STATIC_REMOVAL, FullText);
        ThreadOBJ = CreateThread(NULL, 0, &FolderRemoval, (LPVOID)hwnd, 0, NULL);
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
        CloseHandle(ThreadOBJ);
        EndDialog(hwnd, 0);
        break;

    case WM_DESTROY:
        CloseHandle(ThreadOBJ);
        EndDialog(hwnd, 0);
        break;

    default:
        return FALSE;
    }
    return TRUE;
}

INT_PTR CALLBACK SagesetDlgProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    WCHAR TempText[MAX_PATH] = { 0 };

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
