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
#include <atlpath.h>

WINE_DEFAULT_DEBUG_CHANNEL (fprop);

// rundll32.exe shell32.dll,Options_RunDLL 0

/////////////////////////////////////////////////////////////////////////////

EXTERN_C BOOL PathIsExeW(LPCWSTR lpszPath);

#define FTA_MODIFYMASK (FTA_OpenIsSafe) // Bits modified by EditTypeDlg
#define NOASSOCRESID IDI_SHELL_DOCUMENT
#define SUPPORT_EXTENSIONWITHOUTPROGID 1 // NT5 does not support these but NT6 does

#define ASSOC_CCHMAX (32 + 1) // Extension or protocol (INTERNET_MAX_SCHEME_LENGTH)
#define TYPENAME_CCHMAX max(100, RTL_FIELD_SIZE(SHFILEINFOA, szTypeName))
#define ICONLOCATION_CCHMAX (MAX_PATH + 1 + 11)

typedef struct _FILE_TYPE_ENTRY
{
    WCHAR FileExtension[ASSOC_CCHMAX];
    WCHAR FileDescription[TYPENAME_CCHMAX];
    WCHAR ClassKey[MAX_PATH];
    DWORD EditFlags;
    WCHAR AppName[64];
    HICON hIconSmall;
    WCHAR ProgramPath[MAX_PATH];
    WCHAR IconPath[MAX_PATH];
    INT nIconIndex;

    bool IsExtension() const
    {
        return FileExtension[0] == '.';
    }
    LPCWSTR GetAssocForDisplay() const
    {
        return FileExtension + (FileExtension[0] == '.');
    }
    void InvalidateTypeName()
    {
        FileDescription[0] = FileDescription[1] = UNICODE_NULL;
    }
    void InvalidateDefaultApp()
    {
        ProgramPath[0] = ProgramPath[1] = AppName[0] = AppName[1] = UNICODE_NULL;
    }
    void Initialize()
    {
        ClassKey[0] = UNICODE_NULL;
        IconPath[0] = UNICODE_NULL;
        nIconIndex = 0;
        InvalidateTypeName();
        InvalidateDefaultApp();
    }
    void DestroyIcons()
    {
        if (hIconSmall)
            DestroyIcon(hIconSmall);
        hIconSmall = NULL;
    }
} FILE_TYPE_ENTRY, *PFILE_TYPE_ENTRY;

typedef struct _FILE_TYPE_GLOBALS
{
    HIMAGELIST himlSmall;
    UINT IconSize;
    HICON hDefExtIconSmall;
    HBITMAP hOpenWithImage;
    HANDLE hHeap;
    WCHAR NoneString[42];
    INT8 SortCol, SortReverse;
    UINT Restricted;
} FILE_TYPE_GLOBALS, *PFILE_TYPE_GLOBALS;

static DWORD
GetRegDWORD(HKEY hKey, LPCWSTR Name, DWORD &Value, DWORD DefaultValue = 0, BOOL Strict = FALSE)
{
    DWORD cb = sizeof(DWORD), type;
    LRESULT ec = RegQueryValueExW(hKey, Name, 0, &type, (BYTE*)&Value, &cb);
    if (ec == ERROR_SUCCESS)
    {
        if ((type == REG_DWORD && cb == sizeof(DWORD)) ||
            (!Strict && type == REG_BINARY && (cb && cb <= sizeof(DWORD))))
        {
            Value &= (0xffffffffUL >> (32 - cb * 8));
            return ec;
        }
    }
    Value = DefaultValue;
    return ec ? ec : ERROR_BAD_FORMAT;
}

static DWORD
GetRegDWORD(HKEY hKey, LPCWSTR Name, DWORD DefaultValue = 0)
{
    GetRegDWORD(hKey, Name, DefaultValue, DefaultValue, FALSE);
    return DefaultValue;
}

static HRESULT
GetClassKey(const FILE_TYPE_ENTRY &FTE, LPCWSTR &SubKey)
{
    HRESULT hr = S_OK;
    LPCWSTR path = FTE.IsExtension() ? FTE.ClassKey : FTE.FileExtension;
#if SUPPORT_EXTENSIONWITHOUTPROGID
    if (!*path && FTE.IsExtension())
    {
        path = FTE.FileExtension;
        hr = S_FALSE;
    }
#endif
    ASSERT(*path);
    SubKey = path;
    return hr;
}

static void
QuoteAppPathForCommand(CStringW &path)
{
    if (path.Find(' ') >= 0 && path.Find('\"') < 0)
        path = CStringW(L"\"") + path + L"\"";
}

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
    WCHAR ProgId[MAX_PATH] = { 0 };
    DWORD cb = sizeof(ProgId);
    RegQueryValueExW(hKey, NULL, NULL, NULL, LPBYTE(ProgId), &cb);
    RegCloseKey(hKey);

    // FIXME: Should verify that no other extensions are using this ProgId
    // delete "extfile" key (if any)
    if (ProgId[0])
        SHDeleteKeyW(HKEY_CLASSES_ROOT, ProgId);

    // delete ".ext" key
    BOOL ret = (SHDeleteKeyW(HKEY_CLASSES_ROOT, pszExt) == ERROR_SUCCESS);

    // notify
    SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_FLUSHNOWAIT, NULL, NULL);

    return ret;
}

static inline HICON
DoExtractIcon(LPCWSTR IconPath, INT iIndex, UINT cx, UINT cy)
{
    return SHELL32_SHExtractIcon(IconPath, iIndex, cx, cy);
}

static HICON
DoExtractIcon(LPCWSTR IconPath, INT iIndex = 0, BOOL bSmall = FALSE)
{
    UINT cx = GetSystemMetrics(bSmall ? SM_CXSMICON : SM_CXICON);
    UINT cy = GetSystemMetrics(bSmall ? SM_CYSMICON : SM_CYICON);
    return DoExtractIcon(IconPath, iIndex, cx, cy);
}

static BOOL
GetFileTypeIconsEx(PFILE_TYPE_ENTRY Entry, LPCWSTR IconLocation, UINT IconSize)
{
    Entry->hIconSmall = NULL;
    if (lstrcmpW(IconLocation, L"%1") == 0)
    {
        LPCWSTR ext = Entry->FileExtension;
        if (!lstrcmpiW(ext, L".exe") || !lstrcmpiW(ext, L".scr"))
        {
            Entry->hIconSmall = HICON(LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(IDI_SHELL_EXE),
                                                 IMAGE_ICON, IconSize, IconSize, 0));
            Entry->nIconIndex = -IDI_SHELL_EXE;
        }
        // Set the icon path to %1 on purpose so PickIconDlg will issue a warning
        StringCbCopyW(Entry->IconPath, sizeof(Entry->IconPath), IconLocation);
    }
    else
    {
        // Expand the REG_EXPAND_SZ string by environment variables
        if (ExpandEnvironmentStringsW(IconLocation, Entry->IconPath, _countof(Entry->IconPath)))
        {
            Entry->nIconIndex = PathParseIconLocationW(Entry->IconPath);
            Entry->hIconSmall = DoExtractIcon(Entry->IconPath, Entry->nIconIndex, IconSize, IconSize);
        }
    }
    return Entry->hIconSmall != NULL;
}

static BOOL
GetFileTypeIconsByKey(HKEY hKey, PFILE_TYPE_ENTRY Entry, UINT IconSize)
{
    Entry->hIconSmall = NULL;

    HKEY hDefIconKey;
    LONG nResult = RegOpenKeyExW(hKey, L"DefaultIcon", 0, KEY_READ, &hDefIconKey);
    if (nResult != ERROR_SUCCESS)
        return FALSE;

    // Get the icon location
    WCHAR szLocation[ICONLOCATION_CCHMAX];
    DWORD dwSize = sizeof(szLocation);
    nResult = RegQueryValueExW(hDefIconKey, NULL, NULL, NULL, LPBYTE(szLocation), &dwSize);
    szLocation[_countof(szLocation) - 1] = UNICODE_NULL;

    RegCloseKey(hDefIconKey);
    if (nResult != ERROR_SUCCESS || !szLocation[0])
        return FALSE;

    return GetFileTypeIconsEx(Entry, szLocation, IconSize);
}

static LPCWSTR
GetProgramPath(PFILE_TYPE_ENTRY Entry)
{
    if (!Entry->ProgramPath[1] && !Entry->ProgramPath[0])
    {
        DWORD cch = _countof(Entry->ProgramPath);
        if (FAILED(AssocQueryStringW(ASSOCF_INIT_IGNOREUNKNOWN, ASSOCSTR_EXECUTABLE,
                                     Entry->FileExtension, NULL, Entry->ProgramPath, &cch)))
        {
            Entry->ProgramPath[0] = UNICODE_NULL;
            Entry->ProgramPath[1] = TRUE;
        }
    }
    return Entry->ProgramPath;
}

static BOOL
QueryFileDescription(LPCWSTR ProgramPath, LPWSTR pszName, INT cchName)
{
    SHFILEINFOW fi;
    fi.szDisplayName[0] = UNICODE_NULL;
    if (SHGetFileInfoW(ProgramPath, 0, &fi, sizeof(fi), SHGFI_DISPLAYNAME))
    {
        StringCchCopyW(pszName, cchName, fi.szDisplayName);
        return TRUE;
    }
    return !!GetFileTitleW(ProgramPath, pszName, cchName);
}

static LPCWSTR
GetAppName(PFILE_TYPE_ENTRY Entry)
{
    if (!Entry->AppName[1] && !Entry->AppName[0])
    {
        LPCWSTR exe = GetProgramPath(Entry);
        if (!*exe || !QueryFileDescription(exe, Entry->AppName, _countof(Entry->AppName)))
        {
            Entry->AppName[0] = UNICODE_NULL;
            Entry->AppName[1] = TRUE;
        }
    }
    return Entry->AppName;
}

static LPWSTR
GetTypeName(PFILE_TYPE_ENTRY Entry, PFILE_TYPE_GLOBALS pG)
{
    if (!Entry->FileDescription[1] && !Entry->FileDescription[0])
    {
        Entry->FileDescription[1] = TRUE;
        if (Entry->IsExtension())
        {
            SHFILEINFOW fi;
            if (SHGetFileInfoW(Entry->FileExtension, 0, &fi, sizeof(fi), SHGFI_TYPENAME |
                               SHGFI_USEFILEATTRIBUTES) && *fi.szTypeName)
            {
                StringCchCopyW(Entry->FileDescription, _countof(Entry->FileDescription), fi.szTypeName);
            }
        }
        else
        {
            // FIXME: Fix and use ASSOCSTR_FRIENDLYDOCNAME
            DWORD cb = sizeof(Entry->FileDescription), Fallback = TRUE;
            LPCWSTR ClassKey;
            HRESULT hr = GetClassKey(*Entry, ClassKey);
            HKEY hKey;
            if (SUCCEEDED(hr) && !RegOpenKeyExW(HKEY_CLASSES_ROOT, ClassKey, 0, KEY_READ, &hKey))
            {
                Fallback = RegQueryValueExW(hKey, NULL, 0, NULL, (BYTE*)Entry->FileDescription, &cb) != ERROR_SUCCESS ||
                           !*Entry->FileDescription;
                RegCloseKey(hKey);
            }
            if (Fallback)
            {
                StringCchCopyW(Entry->FileDescription, _countof(Entry->FileDescription), Entry->FileExtension);
            }
        }
    }
    return Entry->FileDescription;
}

static void
InitializeDefaultIcons(PFILE_TYPE_GLOBALS pG)
{
    const INT ResId = NOASSOCRESID;
    if (!pG->hDefExtIconSmall)
    {
        pG->hDefExtIconSmall = HICON(LoadImageW(shell32_hInstance, MAKEINTRESOURCEW(ResId),
                                                IMAGE_ICON, pG->IconSize, pG->IconSize, 0));
    }

    if (!ImageList_GetImageCount(pG->himlSmall))
    {
        int idx = ImageList_AddIcon(pG->himlSmall, pG->hDefExtIconSmall);
        ASSERT(idx == 0);
    }
}

static BOOL
Normalize(PFILE_TYPE_ENTRY Entry)
{
    // We don't need this information until somebody tries to edit the entry
    if (!Entry->IconPath[0])
    {
        StringCbCopyW(Entry->IconPath, sizeof(Entry->IconPath), g_pszShell32);
        Entry->nIconIndex = NOASSOCRESID > 1 ? -NOASSOCRESID : 0;
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// EditTypeDlg

#define LISTBOX_MARGIN  2

enum EDITTYPEFLAGS { ETF_ALWAYSEXT = 1 << 0, ETF_BROWSESAME = 1 << 1 };

typedef struct EDITTYPE_DIALOG
{
    HWND hwndLV;
    PFILE_TYPE_GLOBALS pG;
    PFILE_TYPE_ENTRY pEntry;
    CSimpleMap<CStringW, CStringW> CommandLineMap;
    CAtlList<CStringW> ModifiedVerbs;
    WCHAR szIconPath[MAX_PATH];
    INT nIconIndex;
    bool ChangedIcon;
    WCHAR szDefaultVerb[VERBKEY_CCHMAX];
    WCHAR TypeName[TYPENAME_CCHMAX];
} EDITTYPE_DIALOG, *PEDITTYPE_DIALOG;

static void
EditTypeDlg_OnChangeIcon(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    WCHAR szPath[MAX_PATH];
    INT IconIndex = pEditType->nIconIndex;
    ExpandEnvironmentStringsW(pEditType->szIconPath, szPath, _countof(szPath));
    if (PickIconDlg(hwndDlg, szPath, _countof(szPath), &IconIndex))
    {
        HICON hIconLarge = DoExtractIcon(szPath, IconIndex, FALSE);

        // replace Windows directory with "%SystemRoot%" (for portability)
        WCHAR szWinDir[MAX_PATH];
        UINT lenWinDir = GetWindowsDirectoryW(szWinDir, _countof(szWinDir));
        if (StrStrIW(szPath, szWinDir) == szPath)
        {
            CPathW str(L"%SystemRoot%");
            str.Append(&szPath[lenWinDir]);
            StringCbCopyW(szPath, sizeof(szPath), str);
        }

        // update EDITTYPE_DIALOG
        StringCbCopyW(pEditType->szIconPath, sizeof(pEditType->szIconPath), szPath);
        pEditType->nIconIndex = IconIndex;
        pEditType->ChangedIcon = true;

        // set icon to dialog
        HICON hOld = (HICON)SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_ICON, STM_SETICON, (WPARAM)hIconLarge, 0);
        if (hOld)
            DestroyIcon(hOld);
    }
}

static BOOL
EditTypeDlg_OnDrawItem(HWND hwndDlg, LPDRAWITEMSTRUCT pDraw, PEDITTYPE_DIALOG pEditType)
{
    WCHAR szText[MAX_PATH];
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

    HWND hClassCombo = GetDlgItem(hwndDlg, IDC_NEWEXT_COMBOBOX);
    if (pNewExt->bAdvanced)
    {
        rc1.top += pNewExt->dy;
        rc1.bottom += pNewExt->dy;

        rc2.top += pNewExt->dy;
        rc2.bottom += pNewExt->dy;

        ShowWindow(GetDlgItem(hwndDlg, IDC_NEWEXT_ASSOC), SW_SHOWNOACTIVATE);
        ShowWindow(hClassCombo, SW_SHOWNOACTIVATE);

        CStringW strLeft(MAKEINTRESOURCEW(IDS_NEWEXT_ADVANCED_LEFT));
        SetDlgItemTextW(hwndDlg, IDC_NEWEXT_ADVANCED, strLeft);

        SetFocus(hClassCombo);
    }
    else
    {
        rc1.top -= pNewExt->dy;
        rc1.bottom -= pNewExt->dy;

        rc2.top -= pNewExt->dy;
        rc2.bottom -= pNewExt->dy;

        ShowWindow(GetDlgItem(hwndDlg, IDC_NEWEXT_ASSOC), SW_HIDE);
        ShowWindow(hClassCombo, SW_HIDE);

        CStringW strRight(MAKEINTRESOURCEW(IDS_NEWEXT_ADVANCED_RIGHT));
        SetDlgItemTextW(hwndDlg, IDC_NEWEXT_ADVANCED, strRight);

        rc.bottom -= pNewExt->dy;

        SendMessageW(hClassCombo, CB_SETCURSEL, 0, 0); // Reset the combo to the "new class" mode
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

#if 0
    // FIXME: Implement the "choose existing class" mode
    GetDlgItemTextW(hwndDlg, IDC_NEWEXT_COMBOBOX, pNewExt->szFileType, _countof(pNewExt->szFileType));
    StrTrimW(pNewExt->szFileType, g_pszSpace);
#endif
    pNewExt->szFileType[0] = UNICODE_NULL; // "new class" mode

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
        WCHAR szFileType[TYPENAME_CCHMAX];
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
        if (DeleteExt(hwndDlg, strExt))
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

static PFILE_TYPE_ENTRY
FileTypesDlg_InsertToLV(HWND hListView, LPCWSTR Assoc, INT iItem, PFILE_TYPE_GLOBALS pG)
{
    PFILE_TYPE_ENTRY Entry;
    HKEY hKey, hTemp;
    LVITEMW lvItem;
    DWORD dwSize;

    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, Assoc, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return NULL;
    }
    if (Assoc[0] != L'.' && !RegValueExists(hKey, L"URL Protocol"))
    {
        RegCloseKey(hKey);
        return NULL;
    }

    Entry = (PFILE_TYPE_ENTRY)HeapAlloc(pG->hHeap, 0, sizeof(FILE_TYPE_ENTRY));
    if (!Entry)
    {
        RegCloseKey(hKey);
        return NULL;
    }
    Entry->Initialize();

    if (Assoc[0] == L'.')
    {
        if (PathIsExeW(Assoc))
        {
exclude:
            HeapFree(pG->hHeap, 0, Entry);
            RegCloseKey(hKey);
            return NULL;
        }

        dwSize = sizeof(Entry->ClassKey);
        if (RegQueryValueExW(hKey, NULL, NULL, NULL, LPBYTE(Entry->ClassKey), &dwSize))
        {
            Entry->ClassKey[0] = UNICODE_NULL; // No ProgId
        }
#if SUPPORT_EXTENSIONWITHOUTPROGID
        if (Entry->ClassKey[0] && !RegOpenKeyExW(HKEY_CLASSES_ROOT, Entry->ClassKey, 0, KEY_READ, &hTemp))
        {
            RegCloseKey(hKey);
            hKey = hTemp;
        }
#else
        if (!Entry->ClassKey[0])
            goto exclude;
#endif
    }

    Entry->EditFlags = GetRegDWORD(hKey, L"EditFlags", 0);
    if (Entry->EditFlags & FTA_Exclude)
        goto exclude;

    wcscpy(Entry->FileExtension, Assoc);

    // get icon
    Entry->IconPath[0] = UNICODE_NULL;
    BOOL defaultIcon = !GetFileTypeIconsByKey(hKey, Entry, pG->IconSize);

    RegCloseKey(hKey);

    // add icon to imagelist
    INT iSmallImage = 0;
    if (!defaultIcon && Entry->hIconSmall)
    {
        iSmallImage = ImageList_AddIcon(pG->himlSmall, Entry->hIconSmall);
    }

    lvItem.mask = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 0;
    lvItem.lParam = (LPARAM)Entry;
    lvItem.iImage = iSmallImage;
    lvItem.pszText = Assoc[0] == L'.' ? _wcsupr(&Entry->FileExtension[1]) : pG->NoneString;
    SendMessageW(hListView, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);

    lvItem.mask = LVIF_TEXT;
    lvItem.iItem = iItem;
    lvItem.iSubItem = 1;
    lvItem.pszText = LPSTR_TEXTCALLBACK;
    ListView_SetItem(hListView, &lvItem);

    return Entry;
}

static BOOL
FileTypesDlg_AddExt(HWND hwndDlg, LPCWSTR pszExt, LPCWSTR pszProgId, PFILE_TYPE_GLOBALS pG)
{
    DWORD dwValue = 1;
    HKEY hKey;
    WCHAR szKey[13];    // max. "ft4294967295" + "\0"
    LONG nResult;

    if (!*pszProgId)
    {
        pszProgId = szKey;
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
    }

    // Create the ".ext" key
    WCHAR szExt[ASSOC_CCHMAX];
    if (*pszExt == L'.') // The user is allowed to type the extension with or without the . in the new dialog!
        ++pszExt;
    StringCbPrintfW(szExt, sizeof(szExt), L".%s", pszExt);
    _wcslwr(szExt);
    nResult = RegCreateKeyEx(HKEY_CLASSES_ROOT, szExt, 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    _wcsupr(szExt);
    if (ERROR_SUCCESS != nResult)
        return FALSE;

    // Set the default value of ".ext" to "ft%06u"
    RegSetString(hKey, NULL, pszProgId, REG_SZ);

    RegCloseKey(hKey);

    // Insert an item to the listview
    HWND hListView = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
    INT iItem = ListView_GetItemCount(hListView);
    if (!FileTypesDlg_InsertToLV(hListView, szExt, iItem, pG))
        return FALSE;

    ListView_SetItemState(hListView, iItem, -1, LVIS_FOCUSED | LVIS_SELECTED);
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

    WCHAR szExt[MAX_PATH];
    szExt[0] = L'.';
    ListView_GetItemText(hListView, iItem, 0, &szExt[1], _countof(szExt) - 1);
    _wcslwr(szExt);

    if (DeleteExt(hwndDlg, szExt))
    {
        ListView_DeleteItem(hListView, iItem);
    }
    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// common code of NewActionDlg and EditActionDlg

typedef struct ACTION_DIALOG
{
    HWND hwndLB;
    PFILE_TYPE_GLOBALS pG;
    PFILE_TYPE_ENTRY pEntry;
    WCHAR szAction[VERBKEY_CCHMAX];
    WCHAR szApp[MAX_PATH];
    BOOL bUseDDE;
} ACTION_DIALOG, *PACTION_DIALOG;

static void
ActionDlg_OnBrowse(HWND hwndDlg, PACTION_DIALOG pNewAct, BOOL bEdit = FALSE)
{
    WCHAR szFile[MAX_PATH];
    szFile[0] = UNICODE_NULL;

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
    ofn.Flags = OFN_EXPLORER | OFN_ENABLESIZING | OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = L"exe";
    if (GetOpenFileNameW(&ofn))
    {
        if (bEdit)
        {
            CStringW str = szFile;
            QuoteAppPathForCommand(str);
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
                str += GetTypeName(s_pEditAct->pEntry, s_pEditAct->pG);
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

static void
EditTypeDlg_Restrict(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;
    static const WORD map[] = {
        FTA_NoEditIcon, IDC_EDITTYPE_CHANGE_ICON,
        FTA_NoEditDesc, IDC_EDITTYPE_TEXT,
        FTA_NoNewVerb, IDC_EDITTYPE_NEW,
        FTA_NoEditVerb, IDC_EDITTYPE_EDIT_BUTTON,
        FTA_NoRemoveVerb, IDC_EDITTYPE_REMOVE,
        FTA_NoEditDflt, IDC_EDITTYPE_SET_DEFAULT
    };
    for (SIZE_T i = 0; i < _countof(map); i += 2)
    {
        if (pEntry->EditFlags & map[i + 0])
            EnableWindow(GetDlgItem(hwndDlg, map[i + 1]), FALSE);
    }
}

static BOOL
EditTypeDlg_UpdateEntryIcon(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;
    WCHAR buf[MAX_PATH];

    pEntry->IconPath[0] = UNICODE_NULL; // I want the default icon
    Normalize(pEntry);
    pEntry->DestroyIcons();
    pEntry->hIconSmall = DoExtractIcon(pEntry->IconPath, pEntry->nIconIndex, TRUE);
    if (ExpandEnvironmentStringsW(pEditType->szIconPath, buf, _countof(buf)) && buf[0])
    {
        HICON hIco = DoExtractIcon(buf, pEditType->nIconIndex, TRUE);
        if (hIco)
        {
            pEntry->DestroyIcons();
            pEntry->hIconSmall = hIco;
        }
    }
    StringCbCopyW(pEntry->IconPath, sizeof(pEntry->IconPath), pEditType->szIconPath);
    pEntry->nIconIndex = pEditType->nIconIndex;

    HWND hListView = pEditType->hwndLV;
    InitializeDefaultIcons(pEditType->pG);
    HIMAGELIST himlSmall = pEditType->pG->himlSmall;
    INT iSmallImage = ImageList_AddIcon(himlSmall, pEntry->hIconSmall);

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
                       LPCWSTR TypeName, EDITTYPEFLAGS Etf)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;
    LPCWSTR ClassKey;
    HRESULT hr = GetClassKey(*pEntry, ClassKey);
    BOOL OnlyExt = hr != S_OK;
    HKEY hClassKey;
    if (FAILED(hr) || RegCreateKeyExW(HKEY_CLASSES_ROOT, ClassKey, 0, NULL, 0,
                                      KEY_QUERY_VALUE | KEY_WRITE, NULL,
                                      &hClassKey, NULL) != ERROR_SUCCESS)
    {
        return FALSE;
    }

    // Refresh the EditFlags
    DWORD dw;
    if (GetRegDWORD(hClassKey, L"EditFlags", dw, 0, FALSE) == ERROR_SUCCESS)
        pEntry->EditFlags = (dw & ~FTA_MODIFYMASK) | (pEntry->EditFlags & FTA_MODIFYMASK);

    if (!OnlyExt)
    {
        // Set class properties
        RegSetOrDelete(hClassKey, L"EditFlags", REG_DWORD, pEntry->EditFlags ? &pEntry->EditFlags : NULL, 4);
        if (pEntry->IsExtension())
        {
            RegSetOrDelete(hClassKey, L"AlwaysShowExt", REG_SZ, (Etf & ETF_ALWAYSEXT) ? L"" : NULL, 0);
        }
        if (RegKeyExists(hClassKey, L"DocObject"))
        {
            LRESULT ec = GetRegDWORD(hClassKey, L"BrowserFlags", dw, 0, TRUE);
            if (ec == ERROR_SUCCESS || ec == ERROR_FILE_NOT_FOUND)
            {
                dw = (dw & ~8) | ((Etf & ETF_BROWSESAME) ? 0 : 8); // Note: 8 means NOT
                RegSetOrDelete(hClassKey, L"BrowserFlags", REG_DWORD, dw ? &dw : NULL, sizeof(dw));
            }
        }
    }
    if (!(pEntry->EditFlags & FTA_NoEditDesc))
    {
        if (!OnlyExt)
            RegDeleteValueW(hClassKey, NULL); // Legacy name (in ProgId only)

        // Deleting this value is always the correct thing to do but Windows does not do this.
        // This means the user cannot override the translated known type names set by the OS.
        if (OnlyExt)
            RegDeleteValueW(hClassKey, L"FriendlyTypeName"); // MUI name (extension or ProgId)

        if (TypeName[0])
            RegSetString(hClassKey, OnlyExt ? L"FriendlyTypeName" : NULL, TypeName, REG_SZ);
        pEntry->InvalidateTypeName();
    }

    if (pEntry->IconPath[0] && !(pEntry->EditFlags & FTA_NoEditIcon) && pEditType->ChangedIcon)
    {
        HKEY hDefaultIconKey;
        if (RegCreateKeyExW(hClassKey, L"DefaultIcon", 0, NULL, 0, KEY_WRITE,
                            NULL, &hDefaultIconKey, NULL) == ERROR_SUCCESS)
        {
            DWORD type = REG_SZ;
            WCHAR buf[ICONLOCATION_CCHMAX];
            LPCWSTR fmt = L"%s,%d";
            if (!lstrcmpW(pEntry->IconPath, L"%1"))
            {
                fmt = L"%s"; // No icon index for "%1"
            }
            else if (StrChrW(pEntry->IconPath, L'%'))
            {
                type = REG_EXPAND_SZ;
            }
            StringCbPrintfW(buf, sizeof(buf), fmt, pEntry->IconPath, pEntry->nIconIndex);

            RegSetString(hDefaultIconKey, NULL, buf, type);
            RegCloseKey(hDefaultIconKey);
        }
    }

    HKEY hShellKey;
    if (RegCreateKeyExW(hClassKey, L"shell", 0, NULL, 0, KEY_READ | KEY_WRITE, NULL,
                        &hShellKey, NULL) != ERROR_SUCCESS)
    {
        RegCloseKey(hClassKey);
        return FALSE;
    }

    // set default action
    if (!(pEntry->EditFlags & FTA_NoEditDflt))
    {
        if (pEditType->szDefaultVerb[0])
            RegSetString(hShellKey, NULL, pEditType->szDefaultVerb, REG_SZ);
        else
            RegDeleteValueW(hShellKey, NULL);
    }

    // delete shell commands
    WCHAR szVerbName[VERBKEY_CCHMAX];
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

    // write shell commands
    const INT nCount = pEditType->CommandLineMap.GetSize();
    for (INT i = 0; i < nCount; ++i)
    {
        const CStringW& key = pEditType->CommandLineMap.GetKeyAt(i);
        const CStringW& cmd = pEditType->CommandLineMap.GetValueAt(i);
        if (!pEditType->ModifiedVerbs.Find(key))
        {
            ASSERT(RegKeyExists(hShellKey, key));
            continue;
        }

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
                DWORD dwSize = (cmd.GetLength() + 1) * sizeof(WCHAR);
                DWORD dwType = REG_SZ;
                int exp;
                if ((exp = cmd.Find('%', 0)) >= 0 && cmd.Find('%', exp + 1) >= 0)
                    dwType = REG_EXPAND_SZ;
                RegSetValueExW(hCommandKey, NULL, 0, dwType, LPBYTE(LPCWSTR(cmd)), dwSize);
                RegCloseKey(hCommandKey);
            }

            RegCloseKey(hVerbKey);
        }
    }

    RegCloseKey(hShellKey);
    RegCloseKey(hClassKey);
    return TRUE;
}

static BOOL
EditTypeDlg_ReadClass(HWND hwndDlg, PEDITTYPE_DIALOG pEditType, EDITTYPEFLAGS &Etf)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;
    LPCWSTR ClassKey;
    HRESULT hr = GetClassKey(*pEntry, ClassKey);
    HKEY hClassKey;
    if (FAILED(hr) || RegOpenKeyExW(HKEY_CLASSES_ROOT, ClassKey, 0, KEY_READ, &hClassKey))
        return FALSE;

    UINT etfbits = (RegValueExists(hClassKey, L"AlwaysShowExt")) ? ETF_ALWAYSEXT : 0;

    // 8 in BrowserFlags listed in KB 162059 seems to be our bit
    BOOL docobj = RegKeyExists(hClassKey, L"DocObject");
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SAME_WINDOW), docobj);
    etfbits |= (docobj && (GetRegDWORD(hClassKey, L"BrowserFlags") & 8)) ? 0 : ETF_BROWSESAME;

    Etf = EDITTYPEFLAGS(etfbits);

    // open "shell" key
    HKEY hShellKey;
    if (RegOpenKeyExW(hClassKey, L"shell", 0, KEY_READ, &hShellKey) != ERROR_SUCCESS)
    {
        RegCloseKey(hClassKey);
        return FALSE;
    }

    WCHAR DefaultVerb[VERBKEY_CCHMAX];
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
    WCHAR szVerbName[VERBKEY_CCHMAX];
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
                WCHAR szValue[MAX_PATH * 2];
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

    WCHAR TypeName[TYPENAME_CCHMAX];
    dwSize = sizeof(TypeName);
    if (!RegQueryValueExW(hClassKey, NULL, NULL, NULL, LPBYTE(TypeName), &dwSize))
    {
        TypeName[_countof(TypeName) - 1] = UNICODE_NULL; // Terminate
        SetDlgItemTextW(hwndDlg, IDC_EDITTYPE_TEXT, TypeName);
    }

    RegCloseKey(hShellKey);
    RegCloseKey(hClassKey);
    return TRUE;
}

static void
EditTypeDlg_OnOK(HWND hwndDlg, PEDITTYPE_DIALOG pEditType)
{
    PFILE_TYPE_ENTRY pEntry = pEditType->pEntry;

    WCHAR TypeName[TYPENAME_CCHMAX];
    GetDlgItemTextW(hwndDlg, IDC_EDITTYPE_TEXT, TypeName, _countof(TypeName));
    StrTrimW(TypeName, g_pszSpace);

    UINT etf = 0;
    pEntry->EditFlags &= ~(FTA_MODIFYMASK);
    if (!SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_CONFIRM_OPEN, BM_GETCHECK, 0, 0))
        pEntry->EditFlags |= FTA_OpenIsSafe;
    if (SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_SHOW_EXT, BM_GETCHECK, 0, 0))
        etf |= ETF_ALWAYSEXT;
    if (SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_SAME_WINDOW, BM_GETCHECK, 0, 0))
        etf |= ETF_BROWSESAME;

    // update entry icon
    EditTypeDlg_UpdateEntryIcon(hwndDlg, pEditType);

    // write registry
    EditTypeDlg_WriteClass(hwndDlg, pEditType, TypeName, (EDITTYPEFLAGS)etf);

    pEntry->InvalidateDefaultApp();

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
    WCHAR szText[VERBKEY_CCHMAX];
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
            // open 'New Action' dialog
            action.bUseDDE = FALSE;
            action.hwndLB = GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX);
            action.pEntry = pEditType->pEntry;
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
                    QuoteAppPathForCommand(strCommandLine);
                    strCommandLine += L" \"%1\"";
                    pEditType->CommandLineMap.SetAt(action.szAction, strCommandLine);
                    pEditType->ModifiedVerbs.AddHead(action.szAction);
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
                EditTypeDlg_Restrict(hwndDlg, pEditType);
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
            action.pG = pEditType->pG;
            action.pEntry = pEditType->pEntry;
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
                pEditType->ModifiedVerbs.AddHead(action.szAction);
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
    EDITTYPEFLAGS Etf;
    ExpandEnvironmentStringsW(pEntry->IconPath, pEditType->szIconPath, _countof(pEditType->szIconPath));
    pEditType->nIconIndex = pEntry->nIconIndex;
    StringCbCopyW(pEditType->szDefaultVerb, sizeof(pEditType->szDefaultVerb), L"open");
    pEditType->ChangedIcon = false;

    // set info
    HICON hIco = DoExtractIcon(pEditType->szIconPath, pEditType->nIconIndex);
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_ICON, STM_SETICON, (WPARAM)hIco, 0);
    EditTypeDlg_ReadClass(hwndDlg, pEditType, Etf);
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_CONFIRM_OPEN, BM_SETCHECK, !(pEntry->EditFlags & FTA_OpenIsSafe), 0);
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_SHOW_EXT, BM_SETCHECK, !!(Etf & ETF_ALWAYSEXT), 0);
    EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SHOW_EXT), pEntry->IsExtension());
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_SAME_WINDOW, BM_SETCHECK, !!(Etf & ETF_BROWSESAME), 0);
    InvalidateRect(GetDlgItem(hwndDlg, IDC_EDITTYPE_LISTBOX), NULL, TRUE);

    // select first item
    SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_SETCURSEL, 0, 0);
    // is listbox empty?
    if (SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_LISTBOX, LB_GETCOUNT, 0, 0) == 0)
    {
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_EDIT_BUTTON), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_REMOVE), FALSE);
        EnableWindow(GetDlgItem(hwndDlg, IDC_EDITTYPE_SET_DEFAULT), FALSE);
    }
    EditTypeDlg_Restrict(hwndDlg, pEditType);
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

        case WM_DESTROY:
        {
            HICON hOld = (HICON)SendDlgItemMessageW(hwndDlg, IDC_EDITTYPE_ICON, STM_GETICON, 0, 0);
            if (hOld)
                DestroyIcon(hOld);
            break;
        }

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
    PFILE_TYPE_GLOBALS pG = (PFILE_TYPE_GLOBALS)lParamSort;
    PFILE_TYPE_ENTRY entry1 = (PFILE_TYPE_ENTRY)lParam1, entry2 = (PFILE_TYPE_ENTRY)lParam2;
    int x = 0;
    if (pG->SortCol == 1)
        x = wcsicmp(GetTypeName(entry1, pG), GetTypeName(entry2, pG));
    if (!x && !(x = entry1->IsExtension() - entry2->IsExtension()))
        x = wcsicmp(entry1->FileExtension, entry2->FileExtension);
    return x * pG->SortReverse;
}

static void
FileTypesDlg_Sort(PFILE_TYPE_GLOBALS pG, HWND hListView, INT Column = -1)
{
    pG->SortReverse = pG->SortCol == Column ? pG->SortReverse * -1 : 1;
    pG->SortCol = Column < 0 ? 0 : (INT8) Column;
    ListView_SortItems(hListView, FileTypesDlg_CompareItems, (LPARAM)pG);
}

static VOID
FileTypesDlg_InitListView(HWND hwndDlg, HWND hListView)
{
    RECT clientRect;
    LPCWSTR columnName;
    WCHAR szBuf[50];

    LVCOLUMNW col;
    col.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    col.fmt = 0;

    GetClientRect(hListView, &clientRect);
    INT column0Size = (clientRect.right - clientRect.left) / 4;

    columnName = L"Extensions"; // Default to English
    if (LoadStringW(shell32_hInstance, IDS_COLUMN_EXTENSION, szBuf, _countof(szBuf)))
        columnName = szBuf;
    col.pszText     = const_cast<LPWSTR>(columnName);
    col.iSubItem    = 0;
    col.cx          = column0Size;
    SendMessageW(hListView, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    columnName = L"File Types"; // Default to English
    if (LoadStringW(shell32_hInstance, IDS_FILE_TYPES, szBuf, _countof(szBuf)))
    {
        columnName = szBuf;
    }
    else
    {
        ERR("Failed to load localized string!\n");
    }
    col.pszText     = const_cast<LPWSTR>(columnName);
    col.iSubItem    = 1;
    col.cx          = clientRect.right - clientRect.left - column0Size - GetSystemMetrics(SM_CYVSCROLL);
    SendMessageW(hListView, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    const UINT lvexstyle = LVS_EX_FULLROWSELECT | LVS_EX_LABELTIP;
    ListView_SetExtendedListViewStyleEx(hListView, lvexstyle, lvexstyle);
}

static void
FileTypesDlg_SetGroupboxText(HWND hwndDlg, LPCWSTR Assoc)
{
    CStringW buf;
    buf.Format(IDS_FILE_DETAILS, Assoc);
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_DETAILS_GROUPBOX, buf.GetString());
}

static void
FileTypesDlg_Refresh(HWND hwndDlg, HWND hListView, PFILE_TYPE_GLOBALS pG)
{
    ListView_DeleteAllItems(hListView);
    ImageList_RemoveAll(pG->himlSmall);
    InitializeDefaultIcons(pG);
    FileTypesDlg_SetGroupboxText(hwndDlg, L"");
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_DESCRIPTION, L"");
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_APPNAME, L"");
    EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_DELETE), FALSE);
    RedrawWindow(hwndDlg, NULL, NULL, RDW_ALLCHILDREN | RDW_INVALIDATE | RDW_UPDATENOW);
#if DBG
    DWORD statTickStart = GetTickCount();
#endif

    INT iItem = 0;
    WCHAR szName[ASSOC_CCHMAX];
    DWORD dwName = _countof(szName);
    DWORD dwIndex = 0;
    SendMessage(hListView, WM_SETREDRAW, FALSE, 0);
    while (RegEnumKeyExW(HKEY_CLASSES_ROOT, dwIndex++, szName, &dwName,
                         NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        if (FileTypesDlg_InsertToLV(hListView, szName, iItem, pG))
            ++iItem;
        dwName = _countof(szName);
    }
    FileTypesDlg_Sort(pG, hListView);
    SendMessage(hListView, WM_SETREDRAW, TRUE, 0);
    RedrawWindow(hListView, NULL, NULL, RDW_ALLCHILDREN | RDW_ERASE | RDW_FRAME | RDW_INVALIDATE);

#if DBG
    DbgPrint("FT loaded %u (%ums)\n", iItem, GetTickCount() - statTickStart);
#endif
    // select first item
    ListView_SetItemState(hListView, 0, -1, LVIS_FOCUSED | LVIS_SELECTED);
}

static PFILE_TYPE_GLOBALS
FileTypesDlg_Initialize(HWND hwndDlg)
{
    HWND hListView = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
    PFILE_TYPE_GLOBALS pG = (PFILE_TYPE_GLOBALS)SHAlloc(sizeof(*pG));
    if (!pG)
        return pG;

    pG->SortReverse = 1;
    pG->hDefExtIconSmall = NULL;
    pG->hOpenWithImage = NULL;
    pG->IconSize = GetSystemMetrics(SM_CXSMICON); // Shell icons are always square
    pG->himlSmall = ImageList_Create(pG->IconSize, pG->IconSize, ILC_COLOR32 | ILC_MASK, 256, 20);
    pG->hHeap = GetProcessHeap();

    pG->NoneString[0] = UNICODE_NULL;
    LoadStringW(shell32_hInstance, IDS_NONE, pG->NoneString, _countof(pG->NoneString));

    if (!(pG->Restricted = SHRestricted(REST_NOFILEASSOCIATE)))
    {
        HKEY hKey;
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE, L"Software\\Classes", 0, NULL, 0, KEY_WRITE, NULL, &hKey, NULL))
            pG->Restricted = TRUE;
        else
            RegCloseKey(hKey);
    }

    FileTypesDlg_InitListView(hwndDlg, hListView);
    ListView_SetImageList(hListView, pG->himlSmall, LVSIL_SMALL);
    EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_NEW), !pG->Restricted);

    // Delay loading the items so the propertysheet has time to finalize the UI
    PostMessage(hListView, WM_KEYDOWN, VK_F5, 0);
    return pG;
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

        // Select first item (Win2k3 does it)
        HWND hListView = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
        ListView_SetItemState(hListView, 0, -1, LVIS_FOCUSED | LVIS_SELECTED);
    }
}

static void
FileTypesDlg_OnItemChanging(HWND hwndDlg, PFILE_TYPE_ENTRY pEntry, PFILE_TYPE_GLOBALS pG)
{
    HBITMAP &hbmProgram = pG->hOpenWithImage;
    LPCWSTR DispAssoc = pEntry->GetAssocForDisplay();
    LPCWSTR TypeName = GetTypeName(pEntry, pG);
    CStringW buf;

    // format buffer and set description
    FileTypesDlg_SetGroupboxText(hwndDlg, DispAssoc);
    if (pEntry->IsExtension())
        buf.Format(IDS_FILE_DETAILSADV, DispAssoc, TypeName, TypeName);
    else
        buf = L"";
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_DESCRIPTION, buf.GetString());

    // delete previous program image
    if (hbmProgram)
    {
        DeleteObject(hbmProgram);
        hbmProgram = NULL;
    }

    // set program name
    LPCWSTR appname = GetAppName(pEntry);
    SetDlgItemTextW(hwndDlg, IDC_FILETYPES_APPNAME, appname);

    // set program image
    HICON hIconSm = NULL;
    LPCWSTR exe = GetProgramPath(pEntry);
    if (*exe)
    {
        ExtractIconExW(exe, 0, NULL, &hIconSm, 1);
    }
    hbmProgram = BitmapFromIcon(hIconSm, 16, 16);
    DestroyIcon(hIconSm);
    SendDlgItemMessageW(hwndDlg, IDC_FILETYPES_ICON, STM_SETIMAGE, IMAGE_BITMAP, LPARAM(hbmProgram));

    // Enable/Disable the buttons
    EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_CHANGE),
                 !pG->Restricted && pEntry->IsExtension());
    EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_ADVANCED),
                 !(pEntry->EditFlags & FTA_NoEdit) && !pG->Restricted);
    EnableWindow(GetDlgItem(hwndDlg, IDC_FILETYPES_DELETE),
                 !(pEntry->EditFlags & FTA_NoRemove) && !pG->Restricted && pEntry->IsExtension());
}

// IDD_FOLDER_OPTIONS_FILETYPES
INT_PTR CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PFILE_TYPE_GLOBALS pGlobals = (PFILE_TYPE_GLOBALS)GetWindowLongPtrW(hwndDlg, DWLP_USER);
    if (!pGlobals && uMsg != WM_INITDIALOG)
        return FALSE;
    LPNMLISTVIEW lppl;
    PFILE_TYPE_ENTRY pEntry;
    NEWEXT_DIALOG newext;
    EDITTYPE_DIALOG edittype;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pGlobals = FileTypesDlg_Initialize(hwndDlg);
            SetWindowLongPtrW(hwndDlg, DWLP_USER, (LONG_PTR)pGlobals);
            return TRUE;

        case WM_DESTROY:
            SetWindowLongPtrW(hwndDlg, DWLP_USER, 0);
            if (pGlobals)
            {
                DestroyIcon(pGlobals->hDefExtIconSmall);
                DeleteObject(pGlobals->hOpenWithImage);
                SHFree(pGlobals);
            }
            break;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FILETYPES_NEW:
                    newext.hwndLV = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
                    if (IDOK == DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_NEWEXTENSION),
                                                hwndDlg, NewExtDlgProc, (LPARAM)&newext))
                    {
                        FileTypesDlg_AddExt(hwndDlg, newext.szExt, newext.szFileType, pGlobals);
                    }
                    break;

                case IDC_FILETYPES_DELETE:
                    FileTypesDlg_OnDelete(hwndDlg);
                    break;

                case IDC_FILETYPES_CHANGE:
                    pEntry = FileTypesDlg_GetEntry(GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW));
                    if (pEntry)
                    {
                        OPENASINFO oai = { pEntry->FileExtension, 0, OAIF_FORCE_REGISTRATION | OAIF_REGISTER_EXT };
                        if (SUCCEEDED(SHOpenWithDialog(hwndDlg, &oai)))
                        {
                            pEntry->InvalidateDefaultApp();
                            FileTypesDlg_OnItemChanging(hwndDlg, pEntry, pGlobals);
                        }
                    }
                    break;

                case IDC_FILETYPES_ADVANCED:
                    edittype.hwndLV = GetDlgItem(hwndDlg, IDC_FILETYPES_LISTVIEW);
                    edittype.pG = pGlobals;
                    edittype.pEntry = FileTypesDlg_GetEntry(edittype.hwndLV);
                    if (Normalize(edittype.pEntry))
                    {
                        DialogBoxParamW(shell32_hInstance, MAKEINTRESOURCEW(IDD_EDITTYPE),
                                        hwndDlg, EditTypeDlgProc, (LPARAM)&edittype);
                        FileTypesDlg_OnItemChanging(hwndDlg, edittype.pEntry, pGlobals);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            lppl = (LPNMLISTVIEW) lParam;
            switch (lppl->hdr.code)
            {
                case LVN_GETDISPINFO:
                {
                    LPNMLVDISPINFOW pLVDI = (LPNMLVDISPINFOW)lParam;
                    PFILE_TYPE_ENTRY entry = (PFILE_TYPE_ENTRY)pLVDI->item.lParam;
                    if (entry && (pLVDI->item.mask & LVIF_TEXT))
                    {
                        if (pLVDI->item.iSubItem == 1)
                        {
                            pLVDI->item.pszText = GetTypeName(entry, pGlobals);
                            pLVDI->item.mask |= LVIF_DI_SETITEM;
                        }
                    }
                    break;
                }

                case LVN_KEYDOWN:
                {
                    LV_KEYDOWN *pKeyDown = (LV_KEYDOWN *)lParam;
                    switch (pKeyDown->wVKey)
                    {
                        case VK_DELETE:
                            FileTypesDlg_OnDelete(hwndDlg);
                            break;
                        case VK_F5:
                            FileTypesDlg_Refresh(hwndDlg, pKeyDown->hdr.hwndFrom, pGlobals);
                            break;
                    }
                    break;
                }

                case NM_DBLCLK:
                    SendMessage(hwndDlg, WM_COMMAND, IDC_FILETYPES_ADVANCED, 0);
                    break;

                case LVN_DELETEALLITEMS:
                    return FALSE;   // send LVN_DELETEITEM

                case LVN_DELETEITEM:
                    pEntry = FileTypesDlg_GetEntry(lppl->hdr.hwndFrom, lppl->iItem);
                    if (pEntry)
                    {
                        pEntry->DestroyIcons();
                        HeapFree(pGlobals->hHeap, 0, pEntry);
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
                        FileTypesDlg_OnItemChanging(hwndDlg, pEntry, pGlobals);
                    }
                    break;

                case LVN_COLUMNCLICK:
                    FileTypesDlg_Sort(pGlobals, lppl->hdr.hwndFrom, lppl->iSubItem);
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
