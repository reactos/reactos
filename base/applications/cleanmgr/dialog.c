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

DIRSIZE sz;
DLG_VAR dv;
WCHAR_VAR wcv;
BOOL_VAR bv;

BOOL CALLBACK StartDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    WCHAR LogicalDrives[MAX_PATH] = { 0 };
    WCHAR TempText[MAX_PATH] = { 0 };
    int ItemIndex = 0;
    static HBITMAP HbmDrive = NULL;
    static HBITMAP HbmMask = NULL;
    DWORD DwIndex = 0;
    DWORD NumOfDrives = 0;
    static HWND HComboCtrl = NULL;

    switch(Message)
    {
        case WM_INITDIALOG:
            HComboCtrl = GetDlgItem(hwnd, IDC_DRIVE);
            
            if(HComboCtrl == NULL)
            {
                MessageBoxW(NULL, L"GetDlgItem() failed!", L"Error", MB_OK | MB_ICONERROR);
                return FALSE;
            }
            
            NumOfDrives = GetLogicalDriveStringsW(MAX_PATH, LogicalDrives);
            if (NumOfDrives == 0)
            {
                MessageBoxW(NULL, L"GetLogicalDriveStringsW() failed!", L"Error", MB_OK | MB_ICONERROR);
                return FALSE;
            }
            HbmDrive = LoadBitmapW(dv.hInst, MAKEINTRESOURCE(IDB_DRIVE));
            HbmMask = LoadBitmapW(dv.hInst, MAKEINTRESOURCE(IDB_MASK));
            
            if(HbmDrive == NULL || HbmMask == NULL)
            {
                MessageBoxW(NULL, L"LoadBitmapW() failed!", L"Error", MB_OK | MB_ICONERROR);
                return FALSE;
            }

            if (NumOfDrives <= MAX_PATH)
            {
                WCHAR* SingleDrive = LogicalDrives;
                WCHAR RealDrive[MAX_PATH] = { 0 };
                while (*SingleDrive)
                {
                    if (GetDriveTypeW(SingleDrive) == 3)
                    {
                        StringCchCopyW(RealDrive, MAX_PATH, SingleDrive);
                        RealDrive[wcslen(RealDrive) - 1] = '\0';
                        DwIndex = SendMessageW(HComboCtrl, CB_ADDSTRING, 0, (LPARAM)RealDrive);
                        if (SendMessageW(HComboCtrl, CB_SETITEMDATA, DwIndex, (LPARAM)HbmDrive) == CB_ERR)
                        {
                            return FALSE;
                        }
                        memset(RealDrive, 0, sizeof RealDrive);
                    }
                    SingleDrive += wcslen(SingleDrive) + 1;
                }
            }
            
            ComboBox_SetCurSel(HComboCtrl, 0);
            SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETLBTEXT, (WPARAM)0, (LPARAM)wcv.DriveLetter);
            return TRUE;

        case WM_NOTIFY:
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;
        
        case WM_MEASUREITEM:
        {
            // Set the height of the items in the food groups combo box.
            LPMEASUREITEMSTRUCT lpmis = (LPMEASUREITEMSTRUCT)lParam;

            if (lpmis->itemHeight < 18 + 2)
                lpmis->itemHeight = 18 + 2;

            break;
        }

        case WM_DRAWITEM:
        {
            COLORREF ClrBackground, ClrForeground;
            TEXTMETRIC tm;
            int x, y;
            size_t cch;
            HBITMAP HbmIcon;
            WCHAR achTemp[256] = { 0 };

            LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

            if (lpdis->itemID == -1)
                break;

            HbmIcon = (HBITMAP)lpdis->itemData;

            if (HbmIcon == NULL)
            {
                MessageBoxW(NULL, L"WM_DRAWITEM failed!", L"Error", MB_OK | MB_ICONERROR);
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
                break;

            SelectObject(hdc, HbmMask);
            BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
                CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCAND);

            SelectObject(hdc, HbmIcon);
            BitBlt(lpdis->hDC, x, lpdis->rcItem.top + 1,
                CX_BITMAP, CY_BITMAP, hdc, 0, 0, SRCPAINT);

            DeleteDC(hdc);

            if (lpdis->itemState & ODS_FOCUS)
                DrawFocusRect(lpdis->hDC, &lpdis->rcItem);

            break;
        }

        case WM_COMMAND:
            if(HIWORD(wParam) == CBN_SELCHANGE)
            {
                ItemIndex = SendMessageW(GetDlgItem(hwnd, IDC_DRIVE), CB_GETCURSEL, 0, 0);
                if (ItemIndex == CB_ERR)
                {
                    MessageBoxW(NULL, L"SendMessageW failed!", L"Error", MB_OK | MB_ICONERROR);
                    return FALSE;
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
                    DeleteObject(HbmDrive);
                    DeleteObject(HbmMask);
                    EndDialog(hwnd, IDOK);
                    break;
                case IDCANCEL:
                    PostMessage(hwnd, WM_CLOSE, 0, 0);
                    break;
            }
            break;

        case WM_CLOSE:
            EndDialog(hwnd, IDCANCEL);
            break;  

        default:
            return FALSE;
    }
    return TRUE;
}

BOOL CALLBACK ProgressDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE ThreadOBJ = NULL;
    WCHAR TempText[MAX_PATH] = { 0 };
    WCHAR FullText[MAX_PATH] = { 0 };
    WCHAR SysDrive[MAX_PATH] = { 0 };
    
    switch(Message)
    {
        case WM_INITDIALOG:
            bv.SysDrive = FALSE;
            LoadStringW(GetModuleHandleW(NULL), IDS_SCAN, TempText, _countof(TempText));
            StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
            dv.hwndDlg = hwnd;
            SetDlgItemTextW(hwnd, IDC_STATIC_SCAN, FullText);
            GetEnvironmentVariableW(L"SystemDrive", SysDrive, _countof(SysDrive));
            if(wcscmp(SysDrive, wcv.DriveLetter) == 0)
            {
                bv.SysDrive = TRUE;
            }
            ThreadOBJ = CreateThread(NULL, 0, &SizeCheck, NULL, 0, NULL);
            return TRUE;
        
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

BOOL CALLBACK ChoiceDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    LPNMHDR pnmh;
    int MesgBox;
    
    WCHAR TempText[MAX_PATH] = { 0 };
    WCHAR FullText[MAX_PATH] = { 0 };

    switch (Message)
    {
        case WM_INITDIALOG:
            LoadStringW(GetModuleHandleW(NULL), IDS_TITLE, TempText, _countof(TempText));
            StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
            SetWindowTextW(hwnd, FullText);
            return OnCreate(hwnd);

        case WM_NOTIFY:
            pnmh = (LPNMHDR)lParam;
            if ((pnmh->hwndFrom == dv.hTab) &&
                (pnmh->idFrom == IDC_TAB) &&
                (pnmh->code == TCN_SELCHANGE))
            {
                OnTabWndSelChange();
            }
            return ThemeHandler(hwnd, (LPNMCUSTOMDRAW)lParam);

        case WM_THEMECHANGED:
            InvalidateRect(hwnd, NULL, FALSE);
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    if (bv.ChkDskClean == FALSE && bv.RecycleClean == FALSE && bv.ChkDskClean == FALSE && bv.RappsClean == FALSE && bv.TempClean == FALSE)
                    {
                        ZeroMemory(&TempText, sizeof(TempText));
                        LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_OPTION, TempText, _countof(TempText));
                        MessageBoxW(hwnd, TempText, L"Warning", MB_OK | MB_ICONWARNING);
                        break;
                    }
            
                    ZeroMemory(&TempText, sizeof(TempText));
                    LoadStringW(GetModuleHandleW(NULL), IDS_CONFIRM_DELETION, TempText, _countof(TempText));
                    MesgBox = MessageBoxW(hwnd, TempText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                    switch (MesgBox)
                    {
                        case IDYES:
                            EndDialog(hwnd, IDOK);
                            break;

                        case IDNO:
                            break;
                    }
                    break;

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

BOOL CALLBACK ProgressEndDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    static HANDLE ThreadOBJ = NULL;
    WCHAR TempText[MAX_PATH] = { 0 };
    WCHAR FullText[MAX_PATH] = { 0 };

    switch (Message)
    {
    case WM_INITDIALOG:
        LoadStringW(GetModuleHandleW(NULL), IDS_REMOVAL, TempText, _countof(TempText));
        StringCchPrintfW(FullText, _countof(FullText), TempText, wcv.DriveLetter);
        dv.hwndDlg = hwnd;
        SetDlgItemTextW(hwnd, IDC_STATIC_REMOVAL, FullText);
        ThreadOBJ = CreateThread(NULL, 0, &FolderRemoval, NULL, 0, NULL);
        return TRUE;

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

BOOL CALLBACK SagesetDlgProc(HWND hwnd, UINT Message, WPARAM wParam, LPARAM lParam)
{
    int MesgBox;
    WCHAR TempText[MAX_PATH] = { 0 };

    switch (Message)
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
                    if (bv.ChkDskClean == FALSE && bv.RecycleClean == FALSE && bv.ChkDskClean == FALSE && bv.RappsClean == FALSE && bv.TempClean == FALSE)
                    {
                        ZeroMemory(&TempText, sizeof(TempText));
                        LoadStringW(GetModuleHandleW(NULL), IDS_WARNING_OPTION, TempText, _countof(TempText));
                        MessageBoxW(hwnd, TempText, L"Warning", MB_OK | MB_ICONWARNING);
                        break;
                    }
            
                    ZeroMemory(&TempText, sizeof(TempText));
                    LoadStringW(GetModuleHandleW(NULL), IDS_CONFIRM_CONFIG, TempText, _countof(TempText));
                    MesgBox = MessageBoxW(hwnd, TempText, L"Warning", MB_YESNO | MB_ICONWARNING | MB_DEFBUTTON2);
                    switch (MesgBox)
                    {
                        case IDYES:
                            EndDialog(hwnd, IDOK);
                            break;

                        case IDNO:
                            break;
                    }
                    break;

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
