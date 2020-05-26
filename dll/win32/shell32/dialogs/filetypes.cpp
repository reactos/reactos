/*
 *     'File Types' tab property sheet of Folder Options
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

#include "precomp.h"

WINE_DEFAULT_DEBUG_CHANNEL (fprop);

// DefaultIcon = %SystemRoot%\system32\SHELL32.dll,-210
// Verbs: Open / RunAs
//        Cmd: rundll32.exe shell32.dll,Options_RunDLL 0

/////////////////////////////////////////////////////////////////////////////

typedef struct FILE_TYPE_ENTRY
{
    WCHAR FileExtension[30];
    WCHAR FileDescription[100];
    WCHAR ClassKey[MAX_PATH];
    WCHAR ClassName[64];
    DWORD EditFlags;
    WCHAR AppName[64];
    HICON hIconLarge;
    HICON hIconSmall;
    WCHAR ProgramPath[MAX_PATH];
    WCHAR IconPath[MAX_PATH];
    INT nIconIndex;
} FILE_TYPE_ENTRY, *PFILE_TYPE_ENTRY;

static BOOL
DeleteExt(HWND hwndDlg, LPCWSTR pszExt)
{
    if (*pszExt != L'.')
        return FALSE;

    // open ".ext" key
    HKEY hKey;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, pszExt, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
        return FALSE;

    // query "extfile" key name
    WCHAR szValue[64] = { 0 };
    DWORD cbValue = sizeof(szValue);
    RegQueryValueExW(hKey, NULL, NULL, NULL, LPBYTE(szValue), &cbValue);
    RegCloseKey(hKey);

    // delete "extfile" key (if any)
    if (szValue[0])
        SHDeleteKeyW(HKEY_CLASSES_ROOT, szValue);

    // delete ".ext" key
    return SHDeleteKeyW(HKEY_CLASSES_ROOT, pszExt) == ERROR_SUCCESS;
}

static inline HICON
DoExtractIcon(PFILE_TYPE_ENTRY Entry, LPCWSTR IconPath,
              INT iIndex = 0, BOOL bSmall = FALSE)
{
    HICON hIcon = NULL;

    if (iIndex < 0)
    {
        // A negative value will be interpreted as a negated resource ID.
        iIndex = -iIndex;

        INT cx, cy;
        HINSTANCE hDLL = LoadLibraryExW(IconPath, NULL, LOAD_LIBRARY_AS_DATAFILE);
        if (bSmall)
        {
            cx = GetSystemMetrics(SM_CXSMICON);
            cy = GetSystemMetrics(SM_CYSMICON);
        }
        else
        {
            cx = GetSystemMetrics(SM_CXICON);
            cy = GetSystemMetrics(SM_CYICON);
        }
        hIcon = HICON(LoadImageW(hDLL, MAKEINTRESOURCEW(iIndex), IMAGE_ICON,
                                 cx, cy, 0));
        FreeLibrary(hDLL);
    }
    else
    {
        // A positive value is icon index.
        if (bSmall)
            ExtractIconExW(IconPath, iIndex, NULL, &hIcon, 1);
        else
            ExtractIconExW(IconPath, iIndex, &hIcon, NULL, 1);
    }
    return hIcon;
}

static void
DoFileTypeIconLocation(PFILE_TYPE_ENTRY Entry, LPCWSTR IconLocation)
{
    // Expand the REG_EXPAND_SZ string by environment variables
    WCHAR szLocation[MAX_PATH + 32];
    if (!ExpandEnvironmentStringsW(IconLocation, szLocation, _countof(szLocation)))
        return;

    Entry->nIconIndex = PathParseIconLocationW(szLocation);
    StringCbCopyW(Entry->IconPath, sizeof(Entry->IconPath), szLocation);
    Entry->hIconLarge = DoExtractIcon(Entry, szLocation, Entry->nIconIndex, FALSE);
    Entry->hIconSmall = DoExtractIcon(Entry, szLocation, Entry->nIconIndex, TRUE);
}

static BOOL
GetFileTypeIconsEx(PFILE_TYPE_ENTRY Entry, LPCWSTR IconLocation)
{
    Entry->hIconLarge = Entry->hIconSmall = NULL;

    if (lstrcmpiW(Entry->FileExtension, L".exe") == 0 ||
        lstrcmpiW(Entry->FileExtension, L".scr") == 0)
    {
        // It's an executable
        Entry->hIconLarge = LoadIconW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_EXE));
        INT cx = GetSystemMetrics(SM_CXSMICON);
        INT cy = GetSystemMetrics(SM_CYSMICON);
        Entry->hIconSmall = HICON(LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_EXE),
                                             IMAGE_ICON, cx, cy, 0));
        StringCbCopyW(Entry->IconPath, sizeof(Entry->IconPath), g_pszShell32);
        Entry->nIconIndex = -IDI_SHELL_EXE;
    }
    else if (lstrcmpW(IconLocation, L"%1") == 0)
    {
        return FALSE;   // self icon
    }
    else
    {
        DoFileTypeIconLocation(Entry, IconLocation);
    }

    return Entry->hIconLarge && Entry->hIconSmall;
}

static BOOL
GetFileTypeIconsByKey(HKEY hKey, PFILE_TYPE_ENTRY Entry)
{
    Entry->hIconLarge = Entry->hIconSmall = NULL;

    // Open the "DefaultIcon" registry key
    HKEY hDefIconKey;
    LONG nResult = RegOpenKeyExW(hKey, L"DefaultIcon", 0, KEY_READ, &hDefIconKey);
    if (nResult != ERROR_SUCCESS)
        return FALSE;

    // Get the icon location
    WCHAR szLocation[MAX_PATH + 32] = { 0 };
    DWORD dwSize = sizeof(szLocation);
    nResult = RegQueryValueExW(hDefIconKey, NULL, NULL, NULL, LPBYTE(szLocation), &dwSize);

    RegCloseKey(hDefIconKey);

    if (nResult != ERROR_SUCCESS || szLocation[0] == 0)
        return FALSE;

    return GetFileTypeIconsEx(Entry, szLocation);
}

static BOOL
QueryFileDescription(LPCWSTR ProgramPath, LPWSTR pszName, INT cchName)
{
    SHFILEINFOW FileInfo = { 0 };
    if (SHGetFileInfoW(ProgramPath, 0, &FileInfo, sizeof(FileInfo), SHGFI_DISPLAYNAME))
    {
        StringCchCopyW(pszName, cchName, FileInfo.szDisplayName);
        return TRUE;
    }

    return !!GetFileTitleW(ProgramPath, pszName, cchName);
}

static void
SetFileTypeEntryDefaultIcon(PFILE_TYPE_ENTRY Entry)
{
    Entry->hIconLarge = LoadIconW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_FOLDER_OPTIONS));
    INT cxSmall = GetSystemMetrics(SM_CXSMICON);
    INT cySmall = GetSystemMetrics(SM_CYSMICON);
    Entry->hIconSmall = HICON(LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_FOLDER_OPTIONS),
                                         IMAGE_ICON, cxSmall, cySmall, 0));
    StringCbCopyW(Entry->IconPath, sizeof(Entry->IconPath), g_pszShell32);
    Entry->nIconIndex = -IDI_SHELL_FOLDER_OPTIONS;
}

/////////////////////////////////////////////////////////////////////////////
// EditTypeDlg

#define LISTBOX_MARGIN  2

typedef struct EDITTYPE_DIALOG
{
    HWND hwndLV;
    PFILE_TYPE_ENTRY pEntry;
    CSimpleMap<CStringW, CStringW> CommandLineMap;
    WCHAR szIconPath[MAX_PATH];
    INT nIconIndex;
    WCHAR szDefaultVerb[64];
} EDITTYPE_DIALOG, *PEDITTYPE_DIALOG;

static void
EditTypeDlg_OnChangeIcon(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    WCHAR szPath[MAX_PATH];
    INT IconIndex;

    ExpandEnvironmentStringsW(pEditType->szIconPath, szPath, _countof(szPath));
    IconIndex = pEditType->nIconIndex;
    if (PickIconDlg(hwndDlg, szPath, _countof(szPath), &IconIndex))
    {
        // replace Windows directory with "%SystemRoot%" (for portability)
        WCHAR szWinDir[MAX_PATH];
        GetWindowsDirectoryW(szWinDir, _countof(szWinDir));
        if (wcsstr(szPath, szWinDir) == 0)
        {
            CStringW str(L"%SystemRoot%");
            str += &szPath[wcslen(szWinDir)];
            StringCbCopyW(szPath, sizeof(szPath), LPCWSTR(str));
        }

        // update FILE_TYPE_ENTRY
        PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;
        DestroyIcon(pEntry->hIconLarge);
        DestroyIcon(pEntry->hIconSmall);
        pEntry->hIconLarge = DoExtractIcon(pEntry, szPath, IconIndex, FALSE);
        pEntry->hIconSmall = DoExtractIcon(pEntry, szPath, IconIndex, TRUE);

        // update EDITTYPE_DIALOG
        StringCbCopyW(pEditType->szIconPath, sizeof(pEditType->szIconPath), szPath);
        pEditType->nIconIndex = IconIndex;

        // set icon to dialog
        SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_ICON, STM_SETICON, (WPARAM)pEntry->hIconLarge, 0);
    }
}

static BOOL
EditTypeDlg_OnDrawItem(HWND hwndDlg, LPDRAWITEMSTRUCT pDraw, PEDITTYPE_DIALOG pEditType)
{
    WCHAR szText[64];
    HFONT hFont, hFont2;

    if (!pDraw)
        return FALSE;

    // fill rect and set colors
    if (pDraw->itemState & ODS_SELECTED)
    {
        FillRect(pDraw->hDC, &pDraw->rcItem, HBRUSH(COLOR_HIGHLIGHT + 1));
        SetTextColor(pDraw->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
        SetBkColor(pDraw->hDC, GetSysColor(COLOR_HIGHLIGHT));
    }
    else
    {
        FillRect(pDraw->hDC, &pDraw->rcItem, HBRUSH(COLOR_WINDOW + 1));
        SetTextColor(pDraw->hDC, GetSysColor(COLOR_WINDOWTEXT));
        SetBkColor(pDraw->hDC, GetSysColor(COLOR_WINDOW));
    }

    // get listbox text
    HWND hwndListBox = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);
    SendMessageW(hwndListBox, LB_GETTEXT, pDraw->itemID, (LPARAM)szText);

    // is it default?
    hFont = (HFONT)SendMessageW(hwndListBox, WM_GETFONT, 0, 0);
    if (lstrcmpiW(pEditType->szDefaultVerb, szText) == 0)
    {
        // default. set bold
        LOGFONTW lf;
        GetObject(hFont, sizeof(lf), &lf);
        lf.lfWeight = FW_BOLD;
        hFont2 = CreateFontIndirectW(&lf);
        if (hFont2)
        {
            HGDIOBJ hFontOld = SelectObject(pDraw->hDC, hFont2);
            InflateRect(&pDraw->rcItem, -LISTBOX_MARGIN, -LISTBOX_MARGIN);
            DrawTextW(pDraw->hDC, szText, -1, &pDraw->rcItem,
                      DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
            InflateRect(&pDraw->rcItem, LISTBOX_MARGIN, LISTBOX_MARGIN);
            SelectObject(pDraw->hDC, hFontOld);
            DeleteObject(hFont2);
        }
    }
    else
    {
        // non-default
        InflateRect(&pDraw->rcItem, -LISTBOX_MARGIN, -LISTBOX_MARGIN);
        DrawTextW(pDraw->hDC, szText, -1, &pDraw->rcItem,
                  DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);
        InflateRect(&pDraw->rcItem, LISTBOX_MARGIN, LISTBOX_MARGIN);
    }

    // draw focus rect
    if (pDraw->itemState & ODS_FOCUS)
    {
        DrawFocusRect(pDraw->hDC, &pDraw->rcItem);
    }
    return TRUE;
}

static BOOL
EditTypeDlg_OnMeasureItem(HWND hwndDlg, LPMEASUREITEMSTRUCT pMeasure, PEDITTYPE_DIALOG pEditType)
{
    if (!pMeasure)
        return FALSE;

    HWND hwndLB = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);

    HDC hDC = GetDC(hwndLB);
    if (hDC)
    {
        TEXTMETRICW tm;
        GetTextMetricsW(hDC, &tm);
        pMeasure->itemHeight = tm.tmHeight + LISTBOX_MARGIN * 2;
        ReleaseDC(hwndLB, hDC);
        return TRUE;
    }
    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
// NewExtDlg

typedef struct NEWEXT_DIALOG
{
    HWND hwndLV;
    RECT rcDlg;
    BOOL bAdvanced;
    INT dy;
    WCHAR szExt[16];
    WCHAR szFileType[64];
} NEWEXT_DIALOG, *PNEWEXT_DIALOG;

static VOID
NewExtDlg_OnAdvanced(HWND hwndDlg, PNEWEXT_DIALOG pNewExt)
{
    // If "Advanced" button was clicked, then we shrink or expand the dialog.
    RECT rc, rc1, rc2;

    GetWindowRect(hwndDlg, &rc);
    rc.bottom = rc.top + (pNewExt->rcDlg.bottom - pNewExt->rcDlg.top);

    GetWindowRect(GetDlgItem(hwndDlg, IDOK), &rc1);
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc1, 2);

    GetWindowRect(GetDlgItem(hwndDlg, IDCANCEL), &rc2);
    MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rc2, 2);

    if (pNewExt->bAdvanced)
    {
        rc1.top += pNewExt->dy;
        rc1.bottom += pNewExt->dy;

        rc2.top += pNewExt->dy;
        rc2.bottom += pNewExt->dy;

        ShowWindow(GetDlgItem(hwndDlg, IDC_NEWEXT_ASSOC), SW_SHOWNOACTIVATE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_NEWEXT_COMBOBOX), SW_SHOWNOACTIVATE);

        CStringW strLeft(MAKEINTRESOURCEW(IDS_NEWEXT_ADVANCED_LEFT));
        SetDlgItemTextW(hwndDlg, IDC_NEWEXT_ADVANCED, strLeft);

        SetFocus(GetDlgItem(hwndDlg, IDC_NEWEXT_COMBOBOX));
    }
    else
    {
        rc1.top -= pNewExt->dy;
        rc1.bottom -= pNewExt->dy;

        rc2.top -= pNewExt->dy;
        rc2.bottom -= pNewExt->dy;

        ShowWindow(GetDlgItem(hwndDlg, IDC_NEWEXT_ASSOC), SW_HIDE);
        ShowWindow(GetDlgItem(hwndDlg, IDC_NEWEXT_COMBOBOX), SW_HIDE);

        CStringW strRight(MAKEINTRESOURCEW(IDS_NEWEXT_ADVANCED_RIGHT));
        SetDlgItemTextW(hwndDlg, IDC_NEWEXT_ADVANCED, strRight);

        rc.bottom -= pNewExt->dy;

        CStringW strText(MAKEINTRESOURCEW(IDS_NEWEXT_NEW));
        SetDlgItemTextW(hwndDlg, IDC_NEWEXT_COMBOBOX, strText);
    }

    HDWP hDWP = BeginDeferWindowPos(3);

    if (hDWP)
        hDWP = DeferWindowPos(hDWP, GetDlgItem(hwndDlg, IDOK), NULL,
                              rc1.left, rc1.top, rc1.right - rc1.left, rc1.bottom - rc1.top,
                              SWP_NOACTIVATE | SWP_NOZORDER);
    if (hDWP)
        hDWP = DeferWindowPos(hDWP, GetDlgItem(hwndDlg, IDCANCEL), NULL,
                              rc2.left, rc2.top, rc2.right - rc2.left, rc2.bottom - rc2.top,
                              SWP_NOACTIVATE | SWP_NOZORDER);
    if (hDWP)
        hDWP = DeferWindowPos(hDWP, hwndDlg, NULL,
                              rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top,
                              SWP_NOACTIVATE | SWP_NOZORDER);

    if (hDWP)
        EndDeferWindowPos(hDWP);
}

static BOOL
NewExtDlg_OnInitDialog(HWND hwndDlg, PNEWEXT_DIALOG pNewExt)
{
    pNewExt->bAdvanced = FALSE;

    // get window rectangle
    GetWindowRect(hwndDlg, &pNewExt->rcDlg);

    // get delta Y
    RECT rc1, rc2;
    GetWindowRect(GetDlgItem(hwndDlg, IDC_NEWEXT_EDIT), &rc1);
    GetWindowRect(GetDlgItem(hwndDlg, IDC_NEWEXT_COMBOBOX), &rc2);
    pNewExt->dy = rc2.top - rc1.top;

    // initialize
    CStringW strText(MAKEINTRESOURCEW(IDS_NEWEXT_NEW));
    SendDlgItemMessageW(hwndDlg, IDC_NEWEXT_COMBOBOX, CB_ADDSTRING, 0, (LPARAM)(LPCWSTR)strText);
    SendDlgItemMessageW(hwndDlg, IDC_NEWEXT_COMBOBOX, CB_SETCURSEL, 0, 0);
    SendDlgItemMessageW(hwndDlg, IDC_NEWEXT_EDIT, EM_SETLIMITTEXT, _countof(pNewExt->szExt) - 1, 0);

    // shrink it first time
    NewExtDlg_OnAdvanced(hwndDlg, pNewExt);

    return TRUE;
}

static BOOL
NewExtDlg_OnOK(HWND hwndDlg, PNEWEXT_DIALOG pNewExt)
{
    LV_FINDINFO find;
    INT iItem;

    GetDlgItemTextW(hwndDlg, IDC_NEWEXT_EDIT, pNewExt->szExt, _countof(pNewExt->szExt));
    StrTrimW(pNewExt->szExt, g_pszSpace);
    _wcsupr(pNewExt->szExt);

    GetDlgItemTextW(hwndDlg, IDC_NEWEXT_COMBOBOX, pNewExt->szFileType, _countof(pNewExt->szFileType));
    StrTrimW(pNewExt->szFileType, g_pszSpace);

    if (pNewExt->szExt[0] == 0)
    {
        CStringW strText(MAKEINTRESOURCEW(IDS_NEWEXT_SPECIFY_EXT));
        CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
        MessageBoxW(hwndDlg, strText, strTitle, MB_ICONERROR);
        return FALSE;
    }

    ZeroMemory(&find, sizeof(find));
    find.flags = LVFI_STRING;
    if (pNewExt->szExt[0] == L'.')
    {
        find.psz = &pNewExt->szExt[1];
    }
    else
    {
        find.psz = pNewExt->szExt;
    }

    iItem = ListView_FindItem(pNewExt->hwndLV, -1, &find);
    if (iItem >= 0)
    {
        // already exists

        // get file type
        WCHAR szFileType[64];
        LV_ITEM item;
        ZeroMemory(&item, sizeof(item));
        item.mask = LVIF_TEXT;
        item.pszText = szFileType;
        item.cchTextMax = _countof(szFileType);
        item.iItem = iItem;
        item.iSubItem = 1;
        ListView_GetItem(pNewExt->hwndLV, &item);

        // get text
        CStringW strText;
        strText.Format(IDS_NEWEXT_ALREADY_ASSOC, find.psz, szFileType, find.psz, szFileType);

        // get title
        CStringW strTitle;
        strTitle.LoadString(IDS_NEWEXT_EXT_IN_USE);

        if (MessageBoxW(hwndDlg, strText, strTitle, MB_ICONWARNING | MB_YESNO) == IDNO)
        {
            return FALSE;
        }

        // Delete the extension
        CStringW strExt(L".");
        strExt += find.psz;
        strExt.MakeLower();
        DeleteExt(hwndDlg, strExt);

        // Delete the item
        ListView_DeleteItem(pNewExt->hwndLV, iItem);
    }

    EndDialog(hwndDlg, IDOK);
    return TRUE;
}

// IDD_NEWEXTENSION
static INT_PTR CALLBACK
NewExtDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    static PNEWEXT_DIALOG s_pNewExt = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_pNewExt = (PNEWEXT_DIALOG)lParam;
            NewExtDlg_OnInitDialog(hwndDlg, s_pNewExt);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    NewExtDlg_OnOK(hwndDlg, s_pNewExt);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;

                case IDC_NEWEXT_ADVANCED:
                    s_pNewExt->bAdvanced = !s_pNewExt->bAdvanced;
                    NewExtDlg_OnAdvanced(hwndDlg, s_pNewExt);
                    break;
            }
            break;
    }
    return 0;
}

static BOOL
FileTypesDlg_InsertToLV(HWND hListView, LPCWSTR szName, INT iItem, LPCWSTR szFile)
{
    PFILE_TYPE_ENTRY Entry;
    HKEY hKey;
    LVITEMW lvItem;
    DWORD dwSize;
    DWORD dwType;

    if (szName[0] != L'.')
    {
        // FIXME handle URL protocol handlers
        return FALSE;
    }

    // get imagelists of listview
    HIMAGELIST himlLarge = ListView_GetImageList(hListView, LVSIL_NORMAL);
    HIMAGELIST himlSmall = ListView_GetImageList(hListView, LVSIL_SMALL);

    // allocate file type entry
    Entry = (PFILE_TYPE_ENTRY)HeapAlloc(GetProcessHeap(), 0, sizeof(FILE_TYPE_ENTRY));
    if (!Entry)
        return FALSE;

    // open key
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szName, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Entry);
        return FALSE;
    }

    // FIXME check for duplicates

    // query for the default key
    dwSize = sizeof(Entry->ClassKey);
    if (RegQueryValueExW(hKey, NULL, NULL, NULL, LPBYTE(Entry->ClassKey), &dwSize) != ERROR_SUCCESS)
    {
        // no link available
        Entry->ClassKey[0] = 0;
    }

    Entry->ClassName[0] = 0;
    if (Entry->ClassKey[0])
    {
        HKEY hTemp;
        // try open linked key
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, Entry->ClassKey, 0, KEY_READ, &hTemp) == ERROR_SUCCESS)
        {
            DWORD dwSize = sizeof(Entry->ClassName);
            RegQueryValueExW(hTemp, NULL, NULL, NULL, LPBYTE(Entry->ClassName), &dwSize);

            // use linked key
            RegCloseKey(hKey);
            hKey = hTemp;
        }
    }

    // read friendly type name
    if (RegLoadMUIStringW(hKey, L"FriendlyTypeName", Entry->FileDescription,
                          sizeof(Entry->FileDescription), NULL, 0, NULL) != ERROR_SUCCESS)
    {
        // read file description
        dwSize = sizeof(Entry->FileDescription);
        Entry->FileDescription[0] = 0;

        // read default key
        RegQueryValueExW(hKey, NULL, NULL, NULL, LPBYTE(Entry->FileDescription), &dwSize);
    }

    // Read the EditFlags value
    Entry->EditFlags = 0;
    if (!RegQueryValueExW(hKey, L"EditFlags", NULL, &dwType, NULL, &dwSize))
    {
        if ((dwType == REG_DWORD || dwType == REG_BINARY) && dwSize == sizeof(DWORD))
            RegQueryValueExW(hKey, L"EditFlags", NULL, NULL, (LPBYTE)&Entry->EditFlags, &dwSize);
    }

    // convert extension to upper case
    wcscpy(Entry->FileExtension, szName);
    _wcsupr(Entry->FileExtension);

    // get icon
    if (!GetFileTypeIconsByKey(hKey, Entry))
    {
        // set default icon
        SetFileTypeEntryDefaultIcon(Entry);
    }

    // close key
    RegCloseKey(hKey);

    // get program path and app name
    DWORD cch = _countof(Entry->ProgramPath);
    if (S_OK == AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_EXECUTABLE,
                                  Entry->FileExtension, NULL, Entry->ProgramPath, &cch))
    {
        QueryFileDescription(Entry->ProgramPath, Entry->AppName, _countof(Entry->AppName));
    }
    else
    {
        Entry->ProgramPath[0] = Entry->AppName[0] = 0;
    }

    // add icon to imagelist
    INT iLargeImage = -1, iSmallImage = -1;
    if (Entry->hIconLarge && Entry->hIconSmall)
    {
        iLargeImage = ImageList_AddIcon(himlLarge, Entry->hIconLarge);
        iSmallImage = ImageList_AddIcon(himlSmall, Entry->hIconSmall);
        ASSERT(iLargeImage == iSmallImage);
    }

    // Do not add excluded entries
    if (Entry->EditFlags & 0x00000001) //FTA_Exclude
    {
        DestroyIcon(Entry->hIconLarge);
        DestroyIcon(Entry->hIconSmall);
        HeapFree(GetProcessHeap(), 0, Entry);
        return FALSE;
    }

    if (!Entry->FileDescription[0])
    {
        // construct default 'FileExtensionFile' by formatting the uppercase extension
        // with IDS_FILE_EXT_TYPE, outputting something like a l18n 'INI File'

        StringCbPrintfW(Entry->FileDescription, sizeof(Entry->FileDescription),
                        szFile, &Entry->FileExtension[1]);
    }

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvItem.iSubItem = 0;
    lvItem.pszText = &Entry->FileExtension[1];
    lvItem.iItem = iItem;
    lvItem.lParam = (LPARAM)Entry;
    lvItem.iImage = iSmallImage;
    SendMessageW(hListView, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = Entry->FileDescription;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 1;
    ListView_SetItem(hListView, &lvItem);

    return TRUE;
}

static BOOL
FileTypesDlg_AddExt(HWND hwndDlg, LPCWSTR pszExt, LPCWSTR pszFileType)
{
    DWORD dwValue = 1;
    HKEY hKey;
    WCHAR szKey[13];    // max. "ft4294967295" + "\0"
    LONG nResult;

    // Search the next "ft%06u" key name
    do
    {
        StringCbPrintfW(szKey, sizeof(szKey), L"ft%06u", dwValue);

        nResult = RegOpenKeyEx(HKEY_CLASSES_ROOT, szKey, 0, KEY_READ, &hKey);
        if (nResult != ERROR_SUCCESS)
            break;

        RegCloseKey(hKey);
        ++dwValue;
    } while (dwValue != 0);

    RegCloseKey(hKey);

    if (dwValue == 0)
        return FALSE;

    // Create new "ft%06u" key
    nResult = RegCreateKeyEx(HKEY_CLASSES_ROOT, szKey, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (ERROR_SUCCESS != nResult)
        return FALSE;

    RegCloseKey(hKey);

    // Create the ".ext" key
    WCHAR szExt[16];
    if (*pszExt == L'.')
        ++pszExt;
    StringCbPrintfW(szExt, sizeof(szExt), L".%s", pszExt);
    _wcslwr(szExt);
    nResult = RegCreateKeyEx(HKEY_CLASSES_ROOT, szExt, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    _wcsupr(szExt);
    if (ERROR_SUCCESS != nResult)
        return FALSE;

    // Set the default value of ".ext" to "ft%06u"
    DWORD dwSize = (lstrlen(szKey) + 1) * sizeof(WCHAR);
    RegSetValueExW(hKey, NULL, 0, REG_SZ, (LPBYTE)szKey, dwSize);

    RegCloseKey(hKey);

    // Make up the file type name
    WCHAR szFile[100];
    CStringW strFormat(MAKEINTRESOURCEW(IDS_FILE_EXT_TYPE));
    StringCbPrintfW(szFile, sizeof(szFile), strFormat, &szExt[1]);

    // Insert an item to the listview
    HWND hListView = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
    INT iItem = ListView_GetItemCount(hListView);
    if (!FileTypesDlg_InsertToLV(hListView, szExt, iItem, szFile))
        return FALSE;

    LV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.mask = LVIF_STATE | LVIF_TEXT;
    item.iItem = iItem;
    item.state = LVIS_SELECTED | LVIS_FOCUSED;
    item.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
    item.pszText = &szExt[1];
    ListView_SetItem(hListView, &item);

    item.pszText = szFile;
    item.iSubItem = 1;
    ListView_SetItem(hListView, &item);

    ListView_EnsureVisible(hListView, iItem, FALSE);

    return TRUE;
}

static BOOL
FileTypesDlg_RemoveExt(HWND hwndDlg)
{
    HWND hListView = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);

    INT iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (iItem == -1)
        return FALSE;

    WCHAR szExt[20];
    szExt[0] = L'.';
    ListView_GetItemText(hListView, iItem, 0, &szExt[1], _countof(szExt) - 1);
    _wcslwr(szExt);

    DeleteExt(hwndDlg, szExt);
    ListView_DeleteItem(hListView, iItem);
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// common code of NewActionDlg and EditActionDlg

typedef struct ACTION_DIALOG
{
    HWND hwndLB;
    WCHAR ClassName[64];
    WCHAR szAction[64];
    WCHAR szApp[MAX_PATH];
    BOOL bUseDDE;
} ACTION_DIALOG, *PACTION_DIALOG;

static void
ActionDlg_OnBrowse(HWND hwndDlg, PACTION_DIALOG pNewAct, BOOL bEdit = FALSE)
{
    WCHAR szFile[MAX_PATH];
    szFile[0] = 0;

    WCHAR szFilter[MAX_PATH];
    LoadStringW(shell32_hInstance, IDS_EXE_FILTER, szFilter, _countof(szFilter));

    CStringW strTitle(MAKEINTRESOURCEW(IDS_OPEN_WITH));

    OPENFILENAMEW ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400W;
    ofn.hwndOwner = hwndDlg;
    ofn.lpstrFilter = szFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = strTitle;
    ofn.Flags = OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"exe";
    if (GetOpenFileNameW(&ofn))
    {
        if (bEdit)
        {
            CStringW str = szFile;
            str += L" \"%1\"";
            SetDlgItemTextW(hwndDlg, IDC_ACTION_APP, str);
        }
        else
        {
            SetDlgItemTextW(hwndDlg, IDC_ACTION_APP, szFile);
        }
    }
}

/////////////////////////////////////////////////////////////////////////////
// NewActionDlg

static void
NewActionDlg_OnOK(HWND hwndDlg, PACTION_DIALOG pNewAct)
{
    // check action
    GetDlgItemTextW(hwndDlg, IDC_ACTION_ACTION, pNewAct->szAction, _countof(pNewAct->szAction));
    StrTrimW(pNewAct->szAction, g_pszSpace);
    if (pNewAct->szAction[0] == 0)
    {
        // action was empty, error
        HWND hwndCtrl = GetDlgItem(hwndDlg, IDC_ACTION_ACTION);
        SendMessageW(hwndCtrl, EM_SETSEL, 0, -1);
        SetFocus(hwndCtrl);
        CStringW strText(MAKEINTRESOURCEW(IDS_SPECIFY_ACTION));
        CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
        MessageBoxW(hwndDlg, strText, strTitle, MB_ICONERROR);
        return;
    }

    // check app
    GetDlgItemTextW(hwndDlg, IDC_ACTION_APP, pNewAct->szApp, _countof(pNewAct->szApp));
    StrTrimW(pNewAct->szApp, g_pszSpace);
    if (pNewAct->szApp[0] == 0 ||
        GetFileAttributesW(pNewAct->szApp) == INVALID_FILE_ATTRIBUTES)
    {
        // app was empty or invalid
        HWND hwndCtrl = GetDlgItem(hwndDlg, IDC_ACTION_APP);
        SendMessageW(hwndCtrl, EM_SETSEL, 0, -1);
        SetFocus(hwndCtrl);
        CStringW strText(MAKEINTRESOURCEW(IDS_INVALID_PROGRAM));
        CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
        MessageBoxW(hwndDlg, strText, strTitle, MB_ICONERROR);
        return;
    }

    EndDialog(hwndDlg, IDOK);
}

// IDD_ACTION
static INT_PTR CALLBACK
NewActionDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PACTION_DIALOG s_pNewAct = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_pNewAct = (PACTION_DIALOG)lParam;
            s_pNewAct->bUseDDE = FALSE;
            EnableWindow(GetDlgItem(hwndDlg, IDC_ACTION_USE_DDE), FALSE);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    NewActionDlg_OnOK(hwndDlg, s_pNewAct);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;

                case IDC_ACTION_BROWSE:
                    ActionDlg_OnBrowse(hwndDlg, s_pNewAct, FALSE);
                    break;
            }
            break;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// EditActionDlg

static void
EditActionDlg_OnOK(HWND hwndDlg, PACTION_DIALOG pEditAct)
{
    // check action
    GetDlgItemTextW(hwndDlg, IDC_ACTION_ACTION, pEditAct->szAction, _countof(pEditAct->szAction));
    StrTrimW(pEditAct->szAction, g_pszSpace);
    if (pEditAct->szAction[0] == 0)
    {
        // action was empty. show error
        HWND hwndCtrl = GetDlgItem(hwndDlg, IDC_ACTION_ACTION);
        SendMessageW(hwndCtrl, EM_SETSEL, 0, -1);
        SetFocus(hwndCtrl);
        CStringW strText(MAKEINTRESOURCEW(IDS_SPECIFY_ACTION));
        CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
        MessageBoxW(hwndDlg, strText, strTitle, MB_ICONERROR);
    }

    // check app
    GetDlgItemTextW(hwndDlg, IDC_ACTION_APP, pEditAct->szApp, _countof(pEditAct->szApp));
    StrTrimW(pEditAct->szApp, g_pszSpace);
    if (pEditAct->szApp[0] == 0)
    {
        // app was empty. show error
        HWND hwndCtrl = GetDlgItem(hwndDlg, IDC_ACTION_APP);
        SendMessageW(hwndCtrl, EM_SETSEL, 0, -1);
        SetFocus(hwndCtrl);
        CStringW strText(MAKEINTRESOURCEW(IDS_INVALID_PROGRAM));
        CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
        MessageBoxW(hwndDlg, strText, strTitle, MB_ICONERROR);
    }

    EndDialog(hwndDlg, IDOK);
}

// IDD_ACTION
static INT_PTR CALLBACK
EditActionDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PACTION_DIALOG s_pEditAct = NULL;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_pEditAct = (PACTION_DIALOG)lParam;
            s_pEditAct->bUseDDE = FALSE;
            SetDlgItemTextW(hwndDlg, IDC_ACTION_ACTION, s_pEditAct->szAction);
            SetDlgItemTextW(hwndDlg, IDC_ACTION_APP, s_pEditAct->szApp);
            EnableWindow(GetDlgItem(hwndDlg, IDC_ACTION_USE_DDE), FALSE);
            EnableWindow(GetDlgItem(hwndDlg, IDC_ACTION_ACTION), FALSE);
            {
                // set title
                CStringW str(MAKEINTRESOURCEW(IDS_EDITING_ACTION));
                str += s_pEditAct->ClassName;
                SetWindowTextW(hwndDlg, str);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDOK:
                    EditActionDlg_OnOK(hwndDlg, s_pEditAct);
                    break;

                case IDCANCEL:
                    EndDialog(hwndDlg, IDCANCEL);
                    break;

                case IDC_ACTION_BROWSE:
                    ActionDlg_OnBrowse(hwndDlg, s_pEditAct, TRUE);
                    break;
            }
            break;
    }
    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// EditTypeDlg

static BOOL
EditTypeDlg_UpdateEntryIcon(HWND hwndDlg, PEDITTYPE_DIALOG pEditType,
                            LPCWSTR IconPath, INT IconIndex)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;

    BOOL bIconSet = FALSE;
    if (IconPath && IconPath[0])
    {
        DestroyIcon(pEntry->hIconLarge);
        DestroyIcon(pEntry->hIconSmall);
        pEntry->hIconLarge = DoExtractIcon(pEntry, IconPath, IconIndex, FALSE);
        pEntry->hIconSmall = DoExtractIcon(pEntry, IconPath, IconIndex, TRUE);

        bIconSet = (pEntry->hIconLarge && pEntry->hIconSmall);
    }
    if (bIconSet)
    {
        StringCbCopyW(pEntry->IconPath, sizeof(pEntry->IconPath), IconPath);
        pEntry->nIconIndex = IconIndex;
    }
    else
    {
        SetFileTypeEntryDefaultIcon(pEntry);
    }

    HWND hListView = pEditType->hwndLV;
    HIMAGELIST himlLarge = ListView_GetImageList(hListView, LVSIL_NORMAL);
    HIMAGELIST himlSmall = ListView_GetImageList(hListView, LVSIL_SMALL);

    INT iLargeImage = ImageList_AddIcon(himlLarge, pEntry->hIconLarge);
    INT iSmallImage = ImageList_AddIcon(himlSmall, pEntry->hIconSmall);
    ASSERT(iLargeImage == iSmallImage);

    INT iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
    if (iItem != -1)
    {
        LV_ITEMW Item = { LVIF_IMAGE, iItem };
        Item.iImage = iSmallImage;
        ListView_SetItem(hListView, &Item);
    }
    return TRUE;
}

static BOOL
EditTypeDlg_WriteClass(HWND hwndDlg, PEDITTYPE_DIALOG pEditType,
                       LPCWSTR ClassKey, LPCWSTR ClassName, INT cchName)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;

    if (ClassKey[0] == 0)
        return FALSE;

    // create or open class key
    HKEY hClassKey;
    if (RegCreateKeyExW(HKEY_CLASSES_ROOT, ClassKey, 0, NULL, 0, KEY_WRITE, NULL,
                        &hClassKey, NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // create "DefaultIcon" key
    if (pEntry->IconPath[0])
    {
        HKEY hDefaultIconKey;
        if (RegCreateKeyExW(hClassKey, L"DefaultIcon", 0, NULL, 0, KEY_WRITE, NULL,
                            &hDefaultIconKey, NULL) == ERROR_SUCCESS)
        {
            WCHAR szText[MAX_PATH];
            StringCbPrintfW(szText, sizeof(szText), L"%s,%d",
                             pEntry->IconPath, pEntry->nIconIndex);

            // set icon location
            DWORD dwSize = (lstrlenW(szText) + 1) * sizeof(WCHAR);
            RegSetValueExW(hDefaultIconKey, NULL, 0, REG_EXPAND_SZ, LPBYTE(szText), dwSize);

            RegCloseKey(hDefaultIconKey);
        }
    }

    // create "shell" key
    HKEY hShellKey;
    if (RegCreateKeyExW(hClassKey, L"shell", 0, NULL, 0, KEY_WRITE, NULL,
                        &hShellKey, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hClassKey);
        return FALSE;
    }

    // delete shell commands
    WCHAR szVerbName[64];
    DWORD dwIndex = 0;
    while (RegEnumKeyW(hShellKey, dwIndex, szVerbName, _countof(szVerbName)) == ERROR_SUCCESS)
    {
        if (pEditType->CommandLineMap.FindKey(szVerbName) == -1)
        {
            // doesn't exist in CommandLineMap, then delete it
            if (SHDeleteKeyW(hShellKey, szVerbName) == ERROR_SUCCESS)
            {
                --dwIndex;
            }
        }
        ++dwIndex;
    }

    // set default action
    RegSetValueExW(hShellKey, NULL, 0, REG_SZ,
                   LPBYTE(pEditType->szDefaultVerb), sizeof(pEditType->szDefaultVerb));

    // write shell commands
    const INT nCount = pEditType->CommandLineMap.GetSize();
    for (INT i = 0; i < nCount; ++i)
    {
        CStringW& key = pEditType->CommandLineMap.GetKeyAt(i);
        CStringW& value = pEditType->CommandLineMap.GetValueAt(i);

        // create verb key
        HKEY hVerbKey;
        if (RegCreateKeyExW(hShellKey, key, 0, NULL, 0, KEY_WRITE, NULL,
                            &hVerbKey, NULL) == ERROR_SUCCESS)
        {
            // create command key
            HKEY hCommandKey;
            if (RegCreateKeyExW(hVerbKey, L"command", 0, NULL, 0, KEY_WRITE, NULL,
                                &hCommandKey, NULL) == ERROR_SUCCESS)
            {
                // write the default value
                DWORD dwSize = (value.GetLength() + 1) * sizeof(WCHAR);
                RegSetValueExW(hCommandKey, NULL, 0, REG_EXPAND_SZ, LPBYTE(LPCWSTR(value)), dwSize);

                RegCloseKey(hCommandKey);
            }

            RegCloseKey(hVerbKey);
        }
    }

    // set class name to class key
    RegSetValueExW(hClassKey, NULL, 0, REG_SZ, LPBYTE(ClassName), cchName);

    RegCloseKey(hShellKey);
    RegCloseKey(hClassKey);

    return TRUE;
}

static BOOL
EditTypeDlg_ReadClass(HWND hwndDlg, PEDITTYPE_DIALOG pEditType, LPCWSTR ClassKey)
{
    // open class key
    HKEY hClassKey;
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, ClassKey, 0, KEY_READ, &hClassKey) != ERROR_SUCCESS)
        return FALSE;

    // open "shell" key
    HKEY hShellKey;
    if (RegOpenKeyExW(hClassKey, L"shell", 0, KEY_READ, &hShellKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hClassKey);
        return FALSE;
    }

    WCHAR DefaultVerb[64];
    DWORD dwSize = sizeof(DefaultVerb);
    if (RegQueryValueExW(hShellKey, NULL, NULL, NULL,
                         LPBYTE(DefaultVerb), &dwSize) == ERROR_SUCCESS)
    {
        StringCbCopyW(pEditType->szDefaultVerb, sizeof(pEditType->szDefaultVerb), DefaultVerb);
    }
    else
    {
        StringCbCopyW(pEditType->szDefaultVerb, sizeof(pEditType->szDefaultVerb), L"open");
    }

    // enumerate shell verbs
    WCHAR szVerbName[64];
    DWORD dwIndex = 0;
    while (RegEnumKeyW(hShellKey, dwIndex, szVerbName, _countof(szVerbName)) == ERROR_SUCCESS)
    {
        // open verb key
        HKEY hVerbKey;
        LONG nResult = RegOpenKeyExW(hShellKey, szVerbName, 0, KEY_READ, &hVerbKey);
        if (nResult == ERROR_SUCCESS)
        {
            // open command key
            HKEY hCommandKey;
            nResult = RegOpenKeyExW(hVerbKey, L"command", 0, KEY_READ, &hCommandKey);
            if (nResult == ERROR_SUCCESS)
            {
                // get command line
                WCHAR szValue[MAX_PATH + 32];
                dwSize = sizeof(szValue);
                nResult = RegQueryValueExW(hCommandKey, NULL, NULL, NULL, LPBYTE(szValue), &dwSize);
                if (nResult == ERROR_SUCCESS)
                {
                    pEditType->CommandLineMap.SetAt(szVerbName, szValue);
                }

                RegCloseKey(hCommandKey);
            }

            RegCloseKey(hVerbKey);
        }
        SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_ADDSTRING, 0, LPARAM(szVerbName));
        ++dwIndex;
    }

    RegCloseKey(hShellKey);
    RegCloseKey(hClassKey);

    return TRUE;
}

static void
EditTypeDlg_OnOK(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;

    // get class name
    GetDlgItemTextW(hwndDlg, IDC_EDITTYPE_TEXT, pEntry->ClassName, _countof(pEntry->ClassName));
    StrTrimW(pEntry->ClassName, g_pszSpace);

    // update entry icon
    EditTypeDlg_UpdateEntryIcon(hwndDlg, pEditType, pEditType->szIconPath, pEditType->nIconIndex);

    // write registry
    EditTypeDlg_WriteClass(hwndDlg, pEditType, pEntry->ClassKey, pEntry->ClassName,
                           _countof(pEntry->ClassName));

    // update the icon cache
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, NULL, NULL);

    EndDialog(hwndDlg, IDOK);
}

static BOOL
EditTypeDlg_OnRemove(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    // get current selection
    INT iItem = SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_GETCURSEL, 0, 0);
    if (iItem == LB_ERR)
        return FALSE;

    // ask user for removal
    CStringW strText(MAKEINTRESOURCEW(IDS_REMOVE_ACTION));
    CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
    if (MessageBoxW(hwndDlg, strText, strTitle, MB_ICONINFORMATION | MB_YESNO) == IDNO)
        return FALSE;

    // get text
    WCHAR szText[64];
    szText[0] = 0;
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_GETTEXT, iItem, (LPARAM)szText);
    StrTrimW(szText, g_pszSpace);

    // remove it
    pEditType->CommandLineMap.Remove(szText);
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_DELETESTRING, iItem, 0);
    return TRUE;
}

static void
EditTypeDlg_OnCommand(HWND hwndDlg, UINT id, UINT code, PEDITTYPE_DIALOG pEditType)
{
    INT iItem, iIndex;
    ACTION_DIALOG action;
    switch (id)
    {
        case IDOK:
            EditTypeDlg_OnOK(hwndDlg, pEditType);
            break;

        case IDCANCEL:
            EndDialog(hwndDlg, IDCANCEL);
            break;

        case IDC_EDITTYPE_CHANGE_ICON:
            EditTypeDlg_OnChangeIcon(hwndDlg, pEditType);
            break;

        case IDC_EDITTYPE_NEW:
            action.bUseDDE = FALSE;
            action.hwndLB = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);
            StringCbPrintfW(action.ClassName, sizeof(action.ClassName), pEditType->pEntry->ClassName);
            // open 'New Action' dialog
            if (IDOK == DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_ACTION), hwndDlg,
                                        NewActionDlgProc, LPARAM(&action)))
            {
                if (SendMessageW(action.hwndLB, LB_FINDSTRING, -1, (LPARAM)action.szAction) != LB_ERR)
                {
                    // already exists, error
                    HWND hwndCtrl = GetDlgItem(hwndDlg, IDC_ACTION_ACTION);
                    SendMessageW(hwndCtrl, EM_SETSEL, 0, -1);
                    SetFocus(hwndCtrl);

                    CStringW strText, strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
                    strText.Format(IDS_ACTION_EXISTS, action.szAction);
                    MessageBoxW(hwndDlg, strText, strTitle, MB_ICONERROR);
                }
                else
                {
                    // add it
                    CStringW strCommandLine = action.szApp;
                    strCommandLine += L" \"%1\"";
                    pEditType->CommandLineMap.SetAt(action.szAction, strCommandLine);
                    SendMessageW(action.hwndLB, LB_ADDSTRING, 0, LPARAM(action.szAction));
                    if (SendMessageW(action.hwndLB, LB_GETCOUNT, 0, 0) == 1)
                    {
                        // set default
                        StringCbCopyW(pEditType->szDefaultVerb, sizeof(pEditType->szDefaultVerb), action.szAction);
                        InvalidateRect(action.hwndLB, NULL, TRUE);
                    }
                }
            }
            break;

        case IDC_EDITTYPE_LISTBOX:
            if (code == LBN_SELCHANGE)
            {
                action.hwndLB = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);
                INT iItem = SendMessageW(action.hwndLB, LB_GETCURSEL, 0, 0);
                SendMessageW(action.hwndLB, LB_GETTEXT, iItem, LPARAM(action.szAction));
                if (lstrcmpiW(action.szAction, pEditType->szDefaultVerb) == 0)
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SET_DEFAULT), FALSE);
                }
                else
                {
                    EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SET_DEFAULT), TRUE);
                }
                break;
            }
            else if (code != LBN_DBLCLK)
            {
                break;
            }
            // FALL THROUGH

        case IDC_EDITTYPE_EDIT_BUTTON:
            action.bUseDDE = FALSE;
            action.hwndLB = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);
            StringCbPrintfW(action.ClassName, sizeof(action.ClassName), pEditType->pEntry->ClassName);
            iItem = SendMessageW(action.hwndLB, LB_GETCURSEL, 0, 0);
            if (iItem == LB_ERR)
                break;

            // get action
            SendMessageW(action.hwndLB, LB_GETTEXT, iItem, LPARAM(action.szAction));

            // get app
            {
                iIndex = pEditType->CommandLineMap.FindKey(action.szAction);
                CStringW str = pEditType->CommandLineMap.GetValueAt(iIndex);
                StringCbCopyW(action.szApp, sizeof(action.szApp), LPCWSTR(str));
            }

            // open dialog
            if (IDOK == DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_ACTION), hwndDlg,
                                        EditActionDlgProc, LPARAM(&action)))
            {
                SendMessageW(action.hwndLB, LB_DELETESTRING, iItem, 0);
                SendMessageW(action.hwndLB, LB_INSERTSTRING, iItem, LPARAM(action.szAction));
                pEditType->CommandLineMap.SetAt(action.szAction, action.szApp);
            }
            break;

        case IDC_EDITTYPE_REMOVE:
            EditTypeDlg_OnRemove(hwndDlg, pEditType);
            break;

        case IDC_EDITTYPE_SET_DEFAULT:
            action.hwndLB = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);
            iItem = SendMessageW(action.hwndLB, LB_GETCURSEL, 0, 0);
            if (iItem == LB_ERR)
                break;

            SendMessageW(action.hwndLB, LB_GETTEXT, iItem, LPARAM(action.szAction));

            // set default
            StringCbCopyW(pEditType->szDefaultVerb, sizeof(pEditType->szDefaultVerb), action.szAction);
            EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SET_DEFAULT), FALSE);
            InvalidateRect(action.hwndLB, NULL, TRUE);
            break;
    }
}

static BOOL
EditTypeDlg_OnInitDialog(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;
    StringCbCopyW(pEditType->szIconPath, sizeof(pEditType->szIconPath), pEntry->IconPath);
    pEditType->nIconIndex = pEntry->nIconIndex;
    StringCbCopyW(pEditType->szDefaultVerb, sizeof(pEditType->szDefaultVerb), L"open");

    // set info
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_ICON, STM_SETICON, (WPARAM)pEntry->hIconLarge, 0);
    SetDlgItemTextW(hwndDlg, IDC_EDITTYPE_TEXT, pEntry->ClassName);
    EditTypeDlg_ReadClass(hwndDlg, pEditType, pEntry->ClassKey);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX), NULL, TRUE);

    // is listbox empty?
    if (SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_GETCOUNT, 0, 0) == 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_EDIT_BUTTON), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SET_DEFAULT), FALSE);
    }
    else
    {
        // select first item
        SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_SETCURSEL, 0, 0);
    }

    EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SAME_WINDOW), FALSE);

    return TRUE;
}

// IDD_EDITTYPE
static INT_PTR CALLBACK
EditTypeDlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    static PEDITTYPE_DIALOG s_pEditType = NULL;
    LPDRAWITEMSTRUCT pDraw;
    LPMEASUREITEMSTRUCT pMeasure;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            s_pEditType = (PEDITTYPE_DIALOG)lParam;
            return EditTypeDlg_OnInitDialog(hwndDlg, s_pEditType);

        case WM_DRAWITEM:
            pDraw = LPDRAWITEMSTRUCT(lParam);
            return EditTypeDlg_OnDrawItem(hwndDlg, pDraw, s_pEditType);

        case WM_MEASUREITEM:
            pMeasure = LPMEASUREITEMSTRUCT(lParam);
            return EditTypeDlg_OnMeasureItem(hwndDlg, pMeasure, s_pEditType);

        case WM_COMMAND:
            EditTypeDlg_OnCommand(hwndDlg, LOWORD(wParam), HIWORD(wParam), s_pEditType);
            break;
    }

    return 0;
}

/////////////////////////////////////////////////////////////////////////////
// FileTypesDlg

static INT CALLBACK
FileTypesDlg_CompareItems(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PFILE_TYPE_ENTRY Entry1, Entry2;
    int x;

    Entry1 = (PFILE_TYPE_ENTRY)lParam1;
    Entry2 = (PFILE_TYPE_ENTRY)lParam2;

    x = wcsicmp(Entry1->FileExtension, Entry2->FileExtension);
    if (x != 0)
        return x;

    return wcsicmp(Entry1->FileDescription, Entry2->FileDescription);
}

static VOID
FileTypesDlg_InitListView(HWND hwndDlg, HWND hListView)
{
    RECT clientRect;
    LVCOLUMNW col;
    WCHAR szName[50];
    DWORD dwStyle;
    INT columnSize = 140;

    if (!LoadStringW(shell32_hInstance, IDS_COLUMN_EXTENSION, szName, _countof(szName)))
    {
        // default to english
        wcscpy(szName, L"Extensions");
    }

    // make sure its null terminated
    szName[_countof(szName) - 1] = 0;

    GetClientRect(hListView, &clientRect);
    ZeroMemory(&col, sizeof(LV_COLUMN));
    columnSize      = 120;
    col.iSubItem    = 0;
    col.mask        = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    col.fmt         = LVCFMT_FIXED_WIDTH;
    col.cx          = columnSize | LVCFMT_LEFT;
    col.cchTextMax  = wcslen(szName);
    col.pszText     = szName;
    SendMessageW(hListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    if (!LoadStringW(shell32_hInstance, IDS_FILE_TYPES, szName, _countof(szName)))
    {
        // default to english
        wcscpy(szName, L"File Types");
        ERR("Failed to load localized string!\n");
    }

    col.iSubItem    = 1;
    col.cx          = clientRect.right - clientRect.left - columnSize;
    col.cchTextMax  = wcslen(szName);
    col.pszText     = szName;
    SendMessageW(hListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    // set full select style
    dwStyle = (DWORD)SendMessage(hListView, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    dwStyle = dwStyle | LVS_EX_FULLROWSELECT;
    SendMessage(hListView, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
}

static PFILE_TYPE_ENTRY
FileTypesDlg_DoList(HWND hwndDlg)
{
    HWND hListView;
    DWORD dwIndex = 0;
    WCHAR szName[50];
    WCHAR szFile[100];
    DWORD dwName;
    LVITEMW lvItem;
    INT iItem = 0;
    HIMAGELIST himlLarge, himlSmall;

    // create imagelists
    himlLarge = ImageList_Create(GetSystemMetrics(SM_CXICON), GetSystemMetrics(SM_CYICON),
                                 ILC_COLOR32 | ILC_MASK, 256, 20);
    himlSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                 ILC_COLOR32 | ILC_MASK, 256, 20);

    // set imagelists to listview.
    hListView = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
    ListView_SetImageList(hListView, himlLarge, LVSIL_NORMAL);
    ListView_SetImageList(hListView, himlSmall, LVSIL_SMALL);

    FileTypesDlg_InitListView(hwndDlg, hListView);

    szFile[0] = 0;
    if (!LoadStringW(shell32_hInstance, IDS_FILE_EXT_TYPE, szFile, _countof(szFile)))
    {
        // default to english
        wcscpy(szFile, L"%s File");
    }
    szFile[(_countof(szFile)) - 1] = 0;

    dwName = _countof(szName);

    while (RegEnumKeyExW(HKEY_CLASSES_ROOT, dwIndex++, szName, &dwName,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        if (FileTypesDlg_InsertToLV(hListView, szName, iItem, szFile))
            ++iItem;
        dwName = _countof(szName);
    }

    // Leave if the list is empty
    if (iItem == 0)
        return NULL;

    // sort list
    ListView_SortItems(hListView, FileTypesDlg_CompareItems, NULL);

    // select first item
    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = (UINT)-1;
    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.iItem = 0;
    ListView_SetItem(hListView, &lvItem);

    lvItem.mask = LVIF_PARAM;
    ListView_GetItem(hListView, &lvItem);

    return (PFILE_TYPE_ENTRY)lvItem.lParam;
}

static inline PFILE_TYPE_ENTRY
FileTypesDlg_GetEntry(HWND hListView, INT iItem = -1)
{
    if (iItem == -1)
    {
        iItem = ListView_GetNextItem(hListView, -1, LVNI_SELECTED);
        if (iItem == -1)
            return NULL;
    }

    LV_ITEMW lvItem = { LVIF_PARAM, iItem };
    if (ListView_GetItem(hListView, &lvItem))
        return (PFILE_TYPE_ENTRY)lvItem.lParam;

    return NULL;
}

static void
FileTypesDlg_OnDelete(HWND hwndDlg)
{
    CStringW strRemoveExt(MAKEINTRESOURCEW(IDS_REMOVE_EXT));
    CStringW strTitle(MAKEINTRESOURCEW(IDS_FILE_TYPES));
    if (MessageBoxW(hwndDlg, strRemoveExt, strTitle, MB_ICONQUESTION | MB_YESNO) == IDYES)
    {
        FileTypesDlg_RemoveExt(hwndDlg);
    }
}

static void
FileTypesDlg_OnItemChanging(HWND hwndDlg, PFILE_TYPE_ENTRY pEntry)
{
    WCHAR Buffer[255];
    static HBITMAP s_hbmProgram = NULL;

    // format buffer and set groupbox text
    CStringW strFormat(MAKEINTRESOURCEW(IDS_FILE_DETAILS));
    StringCbPrintfW(Buffer, sizeof(Buffer), strFormat, &pEntry->FileExtension[1]);
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_DETAILS_GROUPBOX, Buffer);

    // format buffer and set description
    strFormat.LoadString(IDS_FILE_DETAILSADV);
    StringCbPrintfW(Buffer, sizeof(Buffer), strFormat,
                    &pEntry->FileExtension[1], pEntry->FileDescription,
                    pEntry->FileDescription);
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_DESCRIPTION, Buffer);

    // delete previous program image
    if (s_hbmProgram)
    {
        DeleteObject(s_hbmProgram);
        s_hbmProgram = NULL;
    }

    // set program image
    HICON hIconSm = NULL;
    ExtractIconExW(pEntry->ProgramPath, 0, NULL, &hIconSm, 1);
    s_hbmProgram = BitmapFromIcon(hIconSm, 16, 16);
    DestroyIcon(hIconSm);
    SendDlgItemMessageW(hwndDlg, IDC_FILETYPES_ICON, STM_SETIMAGE, IMAGE_BITMAP, LPARAM(s_hbmProgram));

    // set program name
    if (pEntry->AppName[0])
        SetDlgItemTextW(hwndDlg, IDC_FILETYPES_APPNAME, pEntry->AppName);
    else
        SetDlgItemTextW(hwndDlg, IDC_FILETYPES_APPNAME, L"ReactOS");

    // Enable the Delete button
    if (pEntry->EditFlags & 0x00000010) // FTA_NoRemove
        EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_DELETE), FALSE);
    else
        EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_DELETE), TRUE);
}

// IDD_FOLDER_OPTIONS_FILETYPES
INT_PTR CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMLISTVIEW lppl;
    PFILE_TYPE_ENTRY pEntry;
    OPENASINFO Info;
    NEWEXT_DIALOG newext;
    EDITTYPE_DIALOG edittype;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pEntry = FileTypesDlg_DoList(hwndDlg);

            // Disable the Delete button if the listview is empty
            // the selected item should not be deleted by the user
            if (pEntry == NULL || (pEntry->EditFlags & 0x00000010)) // FTA_NoRemove
                EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_DELETE), FALSE);
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FILETYPES_NEW:
                    newext.hwndLV = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
                    if (IDOK == DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_NEWEXTENSION),
                                                hwndDlg, NewExtDlgProc, (LPARAM)&newext))
                    {
                        FileTypesDlg_AddExt(hwndDlg, newext.szExt, newext.szFileType);
                    }
                    break;

                case IDC_FILETYPES_DELETE:
                    FileTypesDlg_OnDelete(hwndDlg);
                    break;

                case IDC_FILETYPES_CHANGE:
                    pEntry = FileTypesDlg_GetEntry(GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW));
                    if (pEntry)
                    {
                        ZeroMemory(&Info, sizeof(Info));
                        Info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT;
                        Info.pcszFile = pEntry->FileExtension;
                        Info.pcszClass = NULL;
                        SHOpenWithDialog(hwndDlg, &Info);
                    }
                    break;

                case IDC_FILETYPES_ADVANCED:
                    edittype.hwndLV = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
                    edittype.pEntry = FileTypesDlg_GetEntry(edittype.hwndLV);
                    DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_EDITTYPE),
                                    hwndDlg, EditTypeDlgProc, (LPARAM)&edittype);
                    break;
            }
            break;

        case WM_NOTIFY:
            lppl = (LPNMLISTVIEW) lParam;
            switch (lppl->hdr.code)
            {
                case LVN_KEYDOWN:
                {
                    LV_KEYDOWN *pKeyDown = (LV_KEYDOWN *)lParam;
                    if (pKeyDown->wVKey == VK_DELETE)
                    {
                        FileTypesDlg_OnDelete(hwndDlg);
                    }
                    break;
                }

                case NM_DBLCLK:
                    edittype.hwndLV = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
                    edittype.pEntry = FileTypesDlg_GetEntry(edittype.hwndLV);
                    DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_EDITTYPE),
                                    hwndDlg, EditTypeDlgProc, (LPARAM)&edittype);
                    break;

                case LVN_DELETEALLITEMS:
                    return FALSE;   // send LVN_DELETEITEM

                case LVN_DELETEITEM:
                    pEntry = FileTypesDlg_GetEntry(lppl->hdr.hwndFrom, lppl->iItem);
                    if (pEntry)
                    {
                        DestroyIcon(pEntry->hIconLarge);
                        DestroyIcon(pEntry->hIconSmall);
                        HeapFree(GetProcessHeap(), 0, pEntry);
                    }
                    return FALSE;

                case LVN_ITEMCHANGING:
                    pEntry = FileTypesDlg_GetEntry(lppl->hdr.hwndFrom, lppl->iItem);
                    if (!pEntry)
                    {
                        return TRUE;
                    }

                    if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                    {
                        FileTypesDlg_OnItemChanging(hwndDlg, pEntry);
                    }
                    break;

                case PSN_SETACTIVE:
                    // On page activation, set the focus to the listview
                    SetFocus(GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW));
                    break;
            }
            break;
    }

    return FALSE;
}
