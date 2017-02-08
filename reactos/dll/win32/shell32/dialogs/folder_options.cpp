/*
 *    Open With  Context Menu extension
 *
 * Copyright 2007 Johannes Anderwald <johannes.anderwald@reactos.org>
 * Copyright 2016-2017 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
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

#define MAX_PROPERTY_SHEET_PAGE (32)

/// Folder Options:
/// CLASSKEY = HKEY_CLASSES_ROOT\CLSID\{6DFD7C5C-2451-11d3-A299-00C04F8EF6AF}
/// DefaultIcon = %SystemRoot%\system32\SHELL32.dll,-210
/// Verbs: Open / RunAs
///       Cmd: rundll32.exe shell32.dll,Options_RunDLL 0

/// ShellFolder Attributes: 0x0

typedef struct
{
    WCHAR FileExtension[30];
    WCHAR FileDescription[100];
    WCHAR ClassKey[MAX_PATH];
    DWORD EditFlags;
} FOLDER_FILE_TYPE_ENTRY, *PFOLDER_FILE_TYPE_ENTRY;

// uniquely-defined icon entry for Advanced Settings
typedef struct ADVANCED_ICON
{
    WCHAR   szPath[MAX_PATH];
    UINT    nIconIndex;
} ADVANCED_ICON;

// predefined icon IDs (See CreateTreeImageList function below)
#define I_CHECKED                   0
#define I_UNCHECKED                 1
#define I_CHECKED_DISABLED          2
#define I_UNCHECKED_DISABLED        3
#define I_RADIO_CHECKED             4
#define I_RADIO_UNCHECKED           5
#define I_RADIO_CHECKED_DISABLED    6
#define I_RADIO_UNCHECKED_DISABLED  7

#define PREDEFINED_ICON_COUNT       8

// definition of icon stock
static ADVANCED_ICON *  s_AdvancedIcons         = NULL;
static INT              s_AdvancedIconCount     = 0;
static HIMAGELIST       s_hImageList            = NULL;

static INT
Advanced_FindIcon(LPCWSTR pszPath, UINT nIconIndex)
{
    for (INT i = PREDEFINED_ICON_COUNT; i < s_AdvancedIconCount; ++i)
    {
        ADVANCED_ICON *pIcon = &s_AdvancedIcons[i];
        if (pIcon->nIconIndex == nIconIndex &&
            lstrcmpiW(pIcon->szPath, pszPath) == 0)
        {
            return i;   // icon ID
        }
    }
    return -1;  // not found
}

static INT
Advanced_AddIcon(LPCWSTR pszPath, UINT nIconIndex)
{
    ADVANCED_ICON *pAllocated;

    // return the ID if already existed
    INT nIconID = Advanced_FindIcon(pszPath, nIconIndex);
    if (nIconID != -1)
        return nIconID;     // already exists

    // extract a small icon
    HICON hIconSmall = NULL;
    ExtractIconExW(pszPath, nIconIndex, NULL, &hIconSmall, 1);
    if (hIconSmall == NULL)
        return -1;      // failure

    // resize s_AdvancedIcons
    size_t Size = (s_AdvancedIconCount + 1) * sizeof(ADVANCED_ICON);
    pAllocated = (ADVANCED_ICON *)realloc(s_AdvancedIcons, Size);
    if (pAllocated == NULL)
        return -1;      // failure
    else
        s_AdvancedIcons = pAllocated;

    // save icon information
    ADVANCED_ICON *pIcon = &s_AdvancedIcons[s_AdvancedIconCount];
    lstrcpynW(pIcon->szPath, pszPath, _countof(pIcon->szPath));
    pIcon->nIconIndex = nIconIndex;

    // add the icon to the image list
    ImageList_AddIcon(s_hImageList, hIconSmall);

    // increment the counter
    nIconID = s_AdvancedIconCount;
    ++s_AdvancedIconCount;

    DestroyIcon(hIconSmall);

    return nIconID;     // newly-added icon ID
}

// types of Advanced Setting entry
typedef enum ADVANCED_ENTRY_TYPE
{
    AETYPE_GROUP,
    AETYPE_CHECKBOX,
    AETYPE_RADIO,
} ADVANCED_ENTRY_TYPE;

// an entry info of Advanced Settings
typedef struct ADVANCED_ENTRY
{
    DWORD   dwID;                   // entry ID
    DWORD   dwParentID;             // parent entry ID
    DWORD   dwResourceID;           // resource ID
    WCHAR   szKeyName[64];          // entry key name
    DWORD   dwType;                 // ADVANCED_ENTRY_TYPE
    DWORD   dwOrdinal;              // ordinal number
    WCHAR   szText[MAX_PATH];       // text
    INT     nIconID;                // icon ID (See ADVANCED_ICON)

    HKEY    hkeyRoot;               // registry root key
    WCHAR   szRegPath[MAX_PATH];    // registry path
    WCHAR   szValueName[64];        // registry value name

    DWORD   dwCheckedValue;         // checked value
    DWORD   dwUncheckedValue;       // unchecked value
    DWORD   dwDefaultValue;         // defalut value
    BOOL    bHasUncheckedValue;     // If FALSE, UncheckedValue is invalid

    HTREEITEM   hItem;              // for TreeView
    BOOL        bGrayed;            // disabled?
    BOOL        bChecked;           // checked?
} ADVANCED_ENTRY, *PADVANCED_ENTRY;

// definition of advanced entries
static ADVANCED_ENTRY *     s_Advanced = NULL;
static INT                  s_AdvancedCount = 0;

static HBITMAP
Create24BppBitmap(HDC hDC, INT cx, INT cy)
{
    BITMAPINFO bi;
    LPVOID pvBits;

    ZeroMemory(&bi, sizeof(bi));
    bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bi.bmiHeader.biWidth = cx;
    bi.bmiHeader.biHeight = cy;
    bi.bmiHeader.biPlanes = 1;
    bi.bmiHeader.biBitCount = 24;
    bi.bmiHeader.biCompression = BI_RGB;

    HBITMAP hbm = CreateDIBSection(hDC, &bi, DIB_RGB_COLORS, &pvBits, NULL, 0);
    return hbm;
}

static HBITMAP
CreateCheckImage(HDC hDC, BOOL bCheck, BOOL bEnabled = TRUE)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = Create24BppBitmap(hDC, cxSmallIcon, cySmallIcon);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        UINT uState = DFCS_BUTTONCHECK | DFCS_FLAT | DFCS_MONO;
        if (bCheck)
            uState |= DFCS_CHECKED;
        if (!bEnabled)
            uState |= DFCS_INACTIVE;
        DrawFrameControl(hDC, &BoxRect, DFC_BUTTON, uState);
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

static HBITMAP
CreateCheckMask(HDC hDC)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = CreateBitmap(cxSmallIcon, cySmallIcon, 1, 1, NULL);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        FillRect(hDC, &Rect, HBRUSH(GetStockObject(WHITE_BRUSH)));
        FillRect(hDC, &BoxRect, HBRUSH(GetStockObject(BLACK_BRUSH)));
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

static HBITMAP
CreateRadioImage(HDC hDC, BOOL bCheck, BOOL bEnabled = TRUE)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = Create24BppBitmap(hDC, cxSmallIcon, cySmallIcon);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        UINT uState = DFCS_BUTTONRADIOIMAGE | DFCS_FLAT | DFCS_MONO;
        if (bCheck)
            uState |= DFCS_CHECKED;
        if (!bEnabled)
            uState |= DFCS_INACTIVE;
        DrawFrameControl(hDC, &BoxRect, DFC_BUTTON, uState);
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

static HBITMAP
CreateRadioMask(HDC hDC)
{
    INT cxSmallIcon = GetSystemMetrics(SM_CXSMICON);
    INT cySmallIcon = GetSystemMetrics(SM_CYSMICON);

    HBITMAP hbm = CreateBitmap(cxSmallIcon, cySmallIcon, 1, 1, NULL);
    if (hbm == NULL)
        return NULL;    // failure

    RECT Rect, BoxRect;
    SetRect(&Rect, 0, 0, cxSmallIcon, cySmallIcon);
    BoxRect = Rect;
    InflateRect(&BoxRect, -1, -1);

    HGDIOBJ hbmOld = SelectObject(hDC, hbm);
    {
        FillRect(hDC, &Rect, HBRUSH(GetStockObject(WHITE_BRUSH)));
        UINT uState = DFCS_BUTTONRADIOMASK | DFCS_FLAT | DFCS_MONO;
        DrawFrameControl(hDC, &BoxRect, DFC_BUTTON, uState);
    }
    SelectObject(hDC, hbmOld);

    return hbm;     // success
}

static HIMAGELIST
CreateTreeImageList(VOID)
{
    HIMAGELIST hImageList;
    hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 9, 1);
    if (hImageList == NULL)
        return NULL;    // failure

    // free if existed
    if (s_AdvancedIcons)
    {
        free(s_AdvancedIcons);
        s_AdvancedIcons = NULL;
    }
    s_AdvancedIconCount = 0;

    // allocate now
    ADVANCED_ICON *pAllocated;
    size_t Size = PREDEFINED_ICON_COUNT * sizeof(ADVANCED_ICON);
    pAllocated = (ADVANCED_ICON *)calloc(1, Size);
    if (pAllocated == NULL)
        return NULL;    // failure

    s_AdvancedIconCount = PREDEFINED_ICON_COUNT;
    s_AdvancedIcons = pAllocated;

    // add the predefined icons

    HDC hDC = CreateCompatibleDC(NULL);
    HBITMAP hbmMask = CreateCheckMask(hDC);

    HBITMAP hbmChecked, hbmUnchecked;

    hbmChecked = CreateCheckImage(hDC, TRUE);
    ImageList_Add(hImageList, hbmChecked, hbmMask);
    DeleteObject(hbmChecked);

    hbmUnchecked = CreateCheckImage(hDC, FALSE);
    ImageList_Add(hImageList, hbmUnchecked, hbmMask);
    DeleteObject(hbmUnchecked);

    hbmChecked = CreateCheckImage(hDC, TRUE, FALSE);
    ImageList_Add(hImageList, hbmChecked, hbmMask);
    DeleteObject(hbmChecked);

    hbmUnchecked = CreateCheckImage(hDC, FALSE, FALSE);
    ImageList_Add(hImageList, hbmUnchecked, hbmMask);
    DeleteObject(hbmUnchecked);

    DeleteObject(hbmMask);
    hbmMask = CreateRadioMask(hDC);

    hbmChecked = CreateRadioImage(hDC, TRUE);
    ImageList_Add(hImageList, hbmChecked, hbmMask);
    DeleteObject(hbmChecked);

    hbmUnchecked = CreateRadioImage(hDC, FALSE);
    ImageList_Add(hImageList, hbmUnchecked, hbmMask);
    DeleteObject(hbmUnchecked);

    hbmChecked = CreateRadioImage(hDC, TRUE, FALSE);
    ImageList_Add(hImageList, hbmChecked, hbmMask);
    DeleteObject(hbmChecked);

    hbmUnchecked = CreateRadioImage(hDC, FALSE, FALSE);
    ImageList_Add(hImageList, hbmUnchecked, hbmMask);
    DeleteObject(hbmUnchecked);

    DeleteObject(hbmMask);

    return hImageList;
}

static ADVANCED_ENTRY *
Advanced_GetItem(DWORD dwID)
{
    for (INT i = 0; i < s_AdvancedCount; ++i)
    {
        ADVANCED_ENTRY *pEntry = &s_Advanced[i];
        if (pEntry->dwID == dwID)
            return pEntry;
    }
    return NULL;    // failure
}

static INT
Advanced_GetImage(ADVANCED_ENTRY *pEntry)
{
    switch (pEntry->dwType)
    {
        case AETYPE_GROUP:
            return pEntry->nIconID;

        case AETYPE_CHECKBOX:
            if (pEntry->bGrayed)
            {
                if (pEntry->bChecked)
                    return I_CHECKED_DISABLED;
                else
                    return I_UNCHECKED_DISABLED;
            }
            else
            {
                if (pEntry->bChecked)
                    return I_CHECKED;
                else
                    return I_UNCHECKED;
            }

        case AETYPE_RADIO:
            if (pEntry->bGrayed)
            {
                if (pEntry->bChecked)
                    return I_RADIO_CHECKED_DISABLED;
                else
                    return I_RADIO_UNCHECKED_DISABLED;
            }
            else
            {
                if (pEntry->bChecked)
                    return I_RADIO_CHECKED;
                else
                    return I_RADIO_UNCHECKED;
            }
    }
    return -1;  // failure
}

static VOID
Advanced_InsertEntry(HWND hwndTreeView, ADVANCED_ENTRY *pEntry)
{
    ADVANCED_ENTRY *pParent = Advanced_GetItem(pEntry->dwParentID);
    HTREEITEM hParent = TVI_ROOT;
    if (pParent != NULL)
        hParent = pParent->hItem;

    TV_INSERTSTRUCT Insertion;
    ZeroMemory(&Insertion, sizeof(Insertion));
    Insertion.hParent = hParent;
    Insertion.hInsertAfter = TVI_LAST;
    Insertion.item.mask =
        TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    Insertion.item.pszText = pEntry->szText;

    INT iImage = Advanced_GetImage(pEntry);
    Insertion.item.iImage = Insertion.item.iSelectedImage = iImage;
    Insertion.item.lParam = pEntry->dwID;
    pEntry->hItem = TreeView_InsertItem(hwndTreeView, &Insertion);
}

static VOID
Advanced_InsertAll(HWND hwndTreeView)
{
    TreeView_DeleteAllItems(hwndTreeView);

    // insert the entries
    ADVANCED_ENTRY *pEntry;
    for (INT i = 0; i < s_AdvancedCount; ++i)
    {
        pEntry = &s_Advanced[i];
        Advanced_InsertEntry(hwndTreeView, pEntry);
    }

    // expand all
    for (INT i = 0; i < s_AdvancedCount; ++i)
    {
        pEntry = &s_Advanced[i];
        if (pEntry->dwType == AETYPE_GROUP)
        {
            TreeView_Expand(hwndTreeView, pEntry->hItem, TVE_EXPAND);
        }
    }
}

static BOOL
Advanced_LoadTree(HKEY hKey, LPCWSTR pszKeyName, DWORD dwParentID)
{
    DWORD dwIndex;
    WCHAR szKeyName[64], szText[MAX_PATH], *pch;
    DWORD Size, Value;
    ADVANCED_ENTRY *pAllocated;

    // resize s_Advanced
    Size = (s_AdvancedCount + 1) * sizeof(ADVANCED_ENTRY);
    pAllocated = (ADVANCED_ENTRY *)realloc(s_Advanced, Size);
    if (pAllocated == NULL)
        return FALSE;   // failure
    else
        s_Advanced = pAllocated;

    ADVANCED_ENTRY *pEntry = &s_Advanced[s_AdvancedCount];

    // dwID, dwParentID, szKeyName
    pEntry->dwID = s_AdvancedCount;
    pEntry->dwParentID = dwParentID;
    lstrcpynW(pEntry->szKeyName, pszKeyName, _countof(pEntry->szKeyName));

    // Text, ResourceID
    pEntry->szText[0] = 0;
    pEntry->dwResourceID = 0;
    szText[0] = 0;
    Size = sizeof(szText);
    RegQueryValueExW(hKey, L"Text", NULL, NULL, LPBYTE(szText), &Size);
    if (szText[0] == L'@')
    {
        pch = wcsrchr(szText, L',');
        if (pch)
        {
            *pch = 0;
            dwIndex = abs(_wtoi(pch + 1));
            pEntry->dwResourceID = dwIndex;
        }
        HINSTANCE hInst = LoadLibraryW(&szText[1]);
        LoadStringW(hInst, dwIndex, szText, _countof(szText));
        FreeLibrary(hInst);
    }
    else
    {
        pEntry->dwResourceID = DWORD(-1);
    }
    lstrcpynW(pEntry->szText, szText, _countof(pEntry->szText));

    // Type
    szText[0] = 0;
    RegQueryValueExW(hKey, L"Type", NULL, NULL, LPBYTE(szText), &Size);
    if (lstrcmpiW(szText, L"checkbox") == 0)
        pEntry->dwType = AETYPE_CHECKBOX;
    else if (lstrcmpiW(szText, L"radio") == 0)
        pEntry->dwType = AETYPE_RADIO;
    else if (lstrcmpiW(szText, L"group") == 0)
        pEntry->dwType = AETYPE_GROUP;
    else
        return FALSE;   // failure

    pEntry->nIconID = -1;
    if (pEntry->dwType == AETYPE_GROUP)
    {
        // Bitmap (Icon)
        UINT nIconIndex = 0;
        Size = sizeof(szText);
        szText[0] = 0;
        RegQueryValueExW(hKey, L"Bitmap", NULL, NULL, LPBYTE(szText), &Size);

        WCHAR szExpanded[MAX_PATH];
        ExpandEnvironmentStringsW(szText, szExpanded, _countof(szExpanded));
        pch = wcsrchr(szExpanded, L',');
        if (pch)
        {
            *pch = 0;
            nIconIndex = abs(_wtoi(pch + 1));
        }
        pEntry->nIconID = Advanced_AddIcon(szExpanded, nIconIndex);
    }

    // Ordinal (ReactOS extension)
    Size = sizeof(Value);
    Value = DWORD(-1);
    RegQueryValueExW(hKey, L"Ordinal", NULL, NULL, LPBYTE(&Value), &Size);
    pEntry->dwOrdinal = Value;

    if (pEntry->dwType == AETYPE_GROUP)
    {
        pEntry->hkeyRoot = NULL;
        pEntry->szRegPath[0] = 0;
        pEntry->szValueName[0] = 0;
        pEntry->dwCheckedValue = 0;
        pEntry->bHasUncheckedValue = FALSE;
        pEntry->dwUncheckedValue = 0;
        pEntry->dwDefaultValue = 0;
        pEntry->hItem = NULL;
        pEntry->bGrayed = FALSE;
        pEntry->bChecked = FALSE;
    }
    else
    {
        // HKeyRoot
        Value = DWORD(HKEY_CURRENT_USER);
        Size = sizeof(Value);
        RegQueryValueExW(hKey, L"HKeyRoot", NULL, NULL, LPBYTE(&Value), &Size);
        pEntry->hkeyRoot = HKEY(Value);

        // RegPath
        pEntry->szRegPath[0] = 0;
        Size = sizeof(szText);
        RegQueryValueExW(hKey, L"RegPath", NULL, NULL, LPBYTE(szText), &Size);
        lstrcpynW(pEntry->szRegPath, szText, _countof(pEntry->szRegPath));

        // ValueName
        pEntry->szValueName[0] = 0;
        Size = sizeof(szText);
        RegQueryValueExW(hKey, L"ValueName", NULL, NULL, LPBYTE(szText), &Size);
        lstrcpynW(pEntry->szValueName, szText, _countof(pEntry->szValueName));

        // CheckedValue
        Size = sizeof(Value);
        Value = 0x00000001;
        RegQueryValueExW(hKey, L"CheckedValue", NULL, NULL, LPBYTE(&Value), &Size);
        pEntry->dwCheckedValue = Value;

        // UncheckedValue
        Size = sizeof(Value);
        Value = 0x00000000;
        pEntry->bHasUncheckedValue = TRUE;
        if (RegQueryValueExW(hKey, L"UncheckedValue", NULL,
                             NULL, LPBYTE(&Value), &Size) != ERROR_SUCCESS)
        {
            pEntry->bHasUncheckedValue = FALSE;
        }
        pEntry->dwUncheckedValue = Value;

        // DefaultValue
        Size = sizeof(Value);
        Value = 0x00000001;
        RegQueryValueExW(hKey, L"DefaultValue", NULL, NULL, LPBYTE(&Value), &Size);
        pEntry->dwDefaultValue = Value;

        // hItem
        pEntry->hItem = NULL;

        // bGrayed, bChecked
        HKEY hkeyTarget;
        Value = pEntry->dwDefaultValue;
        pEntry->bGrayed = TRUE;
        if (RegOpenKeyExW(HKEY(pEntry->hkeyRoot), pEntry->szRegPath, 0,
                          KEY_READ, &hkeyTarget) == ERROR_SUCCESS)
        {
            Size = sizeof(Value);
            if (RegQueryValueExW(hkeyTarget, pEntry->szValueName, NULL, NULL,
                                 LPBYTE(&Value), &Size) == ERROR_SUCCESS)
            {
                pEntry->bGrayed = FALSE;
            }
            RegCloseKey(hkeyTarget);
        }
        pEntry->bChecked = (Value == pEntry->dwCheckedValue);
    }

    // Grayed (ReactOS extension)
    Size = sizeof(Value);
    Value = FALSE;
    RegQueryValueExW(hKey, L"Grayed", NULL, NULL, LPBYTE(&Value), &Size);
    if (!pEntry->bGrayed)
        pEntry->bGrayed = Value;

    BOOL bIsGroup = (pEntry->dwType == AETYPE_GROUP);
    dwParentID = pEntry->dwID;
    ++s_AdvancedCount;

    if (!bIsGroup)
        return TRUE;    // success

    // load the children
    dwIndex = 0;
    while (RegEnumKeyW(hKey, dwIndex, szKeyName,
                       _countof(szKeyName)) == ERROR_SUCCESS)
    {
        HKEY hkeyChild;
        if (RegOpenKeyExW(hKey, szKeyName, 0, KEY_READ,
                          &hkeyChild) != ERROR_SUCCESS)
        {
            ++dwIndex;
            continue;   // failure
        }

        Advanced_LoadTree(hkeyChild, szKeyName, dwParentID);
        RegCloseKey(hkeyChild);

        ++dwIndex;
    }

    return TRUE;    // success
}

static BOOL
Advanced_LoadAll(VOID)
{
    static const WCHAR s_szAdvanced[] =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced";

    // free if already existed
    if (s_Advanced)
    {
        free(s_Advanced);
        s_Advanced = NULL;
    }
    s_AdvancedCount = 0;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, s_szAdvanced, 0,
                      KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return FALSE;   // failure
    }

    // load the children
    WCHAR szKeyName[64];
    DWORD dwIndex = 0;
    while (RegEnumKeyW(hKey, dwIndex, szKeyName,
                       _countof(szKeyName)) == ERROR_SUCCESS)
    {
        HKEY hkeyChild;
        if (RegOpenKeyExW(hKey, szKeyName, 0, KEY_READ,
                          &hkeyChild) != ERROR_SUCCESS)
        {
            ++dwIndex;
            continue;   // failure
        }

        Advanced_LoadTree(hkeyChild, szKeyName, DWORD(-1));
        RegCloseKey(hkeyChild);

        ++dwIndex;
    }

    RegCloseKey(hKey);

    return TRUE;    // success
}

static int
Advanced_Compare(const void *x, const void *y)
{
    ADVANCED_ENTRY *pEntry1 = (ADVANCED_ENTRY *)x;
    ADVANCED_ENTRY *pEntry2 = (ADVANCED_ENTRY *)y;
    if (pEntry1->dwOrdinal < pEntry2->dwOrdinal)
        return -1;
    if (pEntry1->dwOrdinal > pEntry2->dwOrdinal)
        return 1;
    return 0;
}

static VOID
Advanced_SortAll(VOID)
{
    qsort(s_Advanced, s_AdvancedCount, sizeof(ADVANCED_ENTRY), Advanced_Compare);
}

EXTERN_C HPSXA WINAPI SHCreatePropSheetExtArrayEx(HKEY hKey, LPCWSTR pszSubKey, UINT max_iface, IDataObject *pDataObj);

static VOID
UpdateGeneralIcons(HWND hDlg)
{
    HWND hwndTaskIcon, hwndFolderIcon, hwndClickIcon;
    HICON hTaskIcon = NULL, hFolderIcon = NULL, hClickIcon = NULL;
    LPTSTR lpTaskIconName = NULL, lpFolderIconName = NULL, lpClickIconName = NULL;

    // show task setting icon
    if(IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_COMMONTASKS) == BST_CHECKED)
        lpTaskIconName = MAKEINTRESOURCE(IDI_SHELL_SHOW_COMMON_TASKS);
    else if(IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_CLASSICFOLDERS) == BST_CHECKED)
        lpTaskIconName = MAKEINTRESOURCE(IDI_SHELL_CLASSIC_FOLDERS);

    if (lpTaskIconName)
    {
        hTaskIcon = (HICON)LoadImage(shell32_hInstance,
                                              lpTaskIconName,
                                              IMAGE_ICON,
                                              0,
                                              0,
                                              LR_DEFAULTCOLOR);
        if (hTaskIcon)
        {
            hwndTaskIcon = GetDlgItem(hDlg,
                                    IDC_FOLDER_OPTIONS_TASKICON);
            if (hwndTaskIcon)
            {
                SendMessage(hwndTaskIcon,
                            STM_SETIMAGE,
                            IMAGE_ICON,
                            (LPARAM)hTaskIcon);
            }
        }
    }
    
    // show Folder setting icons
    if(IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_SAMEWINDOW) == BST_CHECKED)
        lpFolderIconName = MAKEINTRESOURCE(IDI_SHELL_OPEN_IN_SOME_WINDOW);
    else if(IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_OWNWINDOW) == BST_CHECKED)
        lpFolderIconName = MAKEINTRESOURCE(IDI_SHELL_OPEN_IN_NEW_WINDOW);
    
    if (lpFolderIconName)
    {
        hFolderIcon = (HICON)LoadImage(shell32_hInstance,
                                              lpFolderIconName,
                                              IMAGE_ICON,
                                              0,
                                              0,
                                              LR_DEFAULTCOLOR);
        if (hFolderIcon)
        {
            hwndFolderIcon = GetDlgItem(hDlg,
                                    IDC_FOLDER_OPTIONS_FOLDERICON);
            if (hwndFolderIcon)
            {
                SendMessage(hwndFolderIcon,
                            STM_SETIMAGE,
                            IMAGE_ICON,
                            (LPARAM)hFolderIcon);
            }
        }
    }

    // Show click setting icon
    if(IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_SINGLECLICK) == BST_CHECKED)
        lpClickIconName = MAKEINTRESOURCE(IDI_SHELL_SINGLE_CLICK_TO_OPEN);
    else if(IsDlgButtonChecked(hDlg, IDC_FOLDER_OPTIONS_DOUBLECLICK) == BST_CHECKED)
        lpClickIconName = MAKEINTRESOURCE(IDI_SHELL_DOUBLE_CLICK_TO_OPEN);

    if (lpClickIconName)
    {
        hClickIcon = (HICON)LoadImage(shell32_hInstance,
                                              lpClickIconName,
                                              IMAGE_ICON,
                                              0,
                                              0,
                                              LR_DEFAULTCOLOR);
        if (hClickIcon)
        {
            hwndClickIcon = GetDlgItem(hDlg,
                                    IDC_FOLDER_OPTIONS_CLICKICON);
            if (hwndClickIcon)
            {
                SendMessage(hwndClickIcon,
                            STM_SETIMAGE,
                            IMAGE_ICON,
                            (LPARAM)hClickIcon);
            }
        }
    }

    // Clean up
    if(hTaskIcon)
        DeleteObject(hTaskIcon);
    if(hFolderIcon)
        DeleteObject(hFolderIcon);
    if(hClickIcon)
        DeleteObject(hClickIcon);
    
    return;
}

INT_PTR
CALLBACK
FolderOptionsGeneralDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
)
{
    switch(uMsg)
    {
        case WM_INITDIALOG:
            // FIXME
            break;
            
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_FOLDER_OPTIONS_COMMONTASKS:
                case IDC_FOLDER_OPTIONS_CLASSICFOLDERS:
                case IDC_FOLDER_OPTIONS_SAMEWINDOW:
                case IDC_FOLDER_OPTIONS_OWNWINDOW:
                case IDC_FOLDER_OPTIONS_SINGLECLICK:
                case IDC_FOLDER_OPTIONS_DOUBLECLICK:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        UpdateGeneralIcons(hwndDlg);

                        /* Enable the 'Apply' button */
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
        {
            LPNMHDR pnmh = (LPNMHDR)lParam;

            switch (pnmh->code)
            {
                case PSN_SETACTIVE:
                    break;

                case PSN_APPLY:
                    break;
            }
            break;
        }
        
        case WM_DESTROY:
            break;
         
         default: 
             return FALSE;
    }
    return FALSE;
}

static BOOL
ViewDlg_OnInitDialog(HWND hwndDlg)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, 14003);

    s_hImageList = CreateTreeImageList();
    TreeView_SetImageList(hwndTreeView, s_hImageList, TVSIL_NORMAL);

    Advanced_LoadAll();
    Advanced_SortAll();
    Advanced_InsertAll(hwndTreeView);

    return TRUE;    // set focus
}

static BOOL
ViewDlg_ToggleCheckItem(HWND hwndDlg, HTREEITEM hItem)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, 14003);

    // get the item
    TV_ITEM Item;
    INT i;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_PARAM;
    Item.hItem = hItem;
    if (!TreeView_GetItem(hwndTreeView, &Item))
        return FALSE;       // no such item

    ADVANCED_ENTRY *pEntry = Advanced_GetItem(Item.lParam);
    if (pEntry == NULL)
        return FALSE;       // no such item
    if (pEntry->bGrayed)
        return FALSE;       // disabled

    // toggle check mark
    Item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    switch (pEntry->dwType)
    {
        case AETYPE_CHECKBOX:
            pEntry->bChecked = !pEntry->bChecked;
            break;
        case AETYPE_RADIO:
            // reset all the entries of the same parent
            for (i = 0; i < s_AdvancedCount; ++i)
            {
                ADVANCED_ENTRY *pEntry2 = &s_Advanced[i];
                if (pEntry->dwParentID == pEntry2->dwParentID)
                {
                    pEntry2->bChecked = FALSE;

                    Item.hItem = pEntry2->hItem;
                    INT iImage = Advanced_GetImage(pEntry2);
                    Item.iImage = Item.iSelectedImage = iImage;
                    TreeView_SetItem(hwndTreeView, &Item);
                }
            }
            pEntry->bChecked = TRUE;
            break;
        default:
            return FALSE;   // failure
    }
    Item.iImage = Item.iSelectedImage = Advanced_GetImage(pEntry);
    Item.hItem = hItem;
    TreeView_SetItem(hwndTreeView, &Item);

    // redraw the item
    RECT rcItem;
    TreeView_GetItemRect(hwndTreeView, hItem, &rcItem, FALSE);
    InvalidateRect(hwndTreeView, &rcItem, TRUE);
    return TRUE;    // success
}

static VOID
ViewDlg_OnTreeViewClick(HWND hwndDlg)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, 14003);

    // do hit test to get the clicked item
    TV_HITTESTINFO HitTest;
    ZeroMemory(&HitTest, sizeof(HitTest));
    DWORD dwPos = GetMessagePos();
    HitTest.pt.x = LOWORD(dwPos);
    HitTest.pt.y = HIWORD(dwPos);
    ScreenToClient(hwndTreeView, &HitTest.pt);
    HTREEITEM hItem = TreeView_HitTest(hwndTreeView, &HitTest);

    // toggle the check mark if possible
    if (ViewDlg_ToggleCheckItem(hwndDlg, hItem))
    {
        // property sheet was changed
        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
    }
}

static void
ViewDlg_OnTreeViewKeyDown(HWND hwndDlg, TV_KEYDOWN *KeyDown)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, 14003);

    if (KeyDown->wVKey == VK_SPACE)
    {
        // [Space] key was pressed
        HTREEITEM hItem = TreeView_GetSelection(hwndTreeView);
        if (ViewDlg_ToggleCheckItem(hwndDlg, hItem))
        {
            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
        }
    }
}

static INT_PTR
ViewDlg_OnTreeCustomDraw(HWND hwndDlg, NMTVCUSTOMDRAW *Draw)
{
    NMCUSTOMDRAW& nmcd = Draw->nmcd;
    switch (nmcd.dwDrawStage)
    {
        case CDDS_PREPAINT:
            return CDRF_NOTIFYITEMDRAW;     // for CDDS_ITEMPREPAINT

        case CDDS_ITEMPREPAINT:
            if (!(nmcd.uItemState & CDIS_SELECTED)) // not selected
            {
                LPARAM lParam = nmcd.lItemlParam;
                ADVANCED_ENTRY *pEntry = Advanced_GetItem(lParam);
                if (pEntry && pEntry->bGrayed) // disabled
                {
                    // draw as grayed
                    Draw->clrText = GetSysColor(COLOR_GRAYTEXT);
                    Draw->clrTextBk = GetSysColor(COLOR_WINDOW);
                    return CDRF_NEWFONT;
                }
            }
            break;

        default:
            break;
    }
    return CDRF_DODEFAULT;
}

static VOID
Advanced_RestoreDefaults(HWND hwndDlg)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, 14003);

    for (INT i = 0; i < s_AdvancedCount; ++i)
    {
        // ignore if the type is group
        ADVANCED_ENTRY *pEntry = &s_Advanced[i];
        if (pEntry->dwType == AETYPE_GROUP)
            continue;

        // set default value on registry
        HKEY hKey;
        if (RegOpenKeyExW(HKEY(pEntry->hkeyRoot), pEntry->szRegPath,
                          0, KEY_WRITE, &hKey) != ERROR_SUCCESS)
        {
            continue;
        }
        RegSetValueExW(hKey, pEntry->szValueName, 0, REG_DWORD,
                       LPBYTE(pEntry->dwDefaultValue), sizeof(DWORD));
        RegCloseKey(hKey);

        // update check status
        pEntry->bChecked = (pEntry->dwCheckedValue == pEntry->dwDefaultValue);

        // update the image
        TV_ITEM Item;
        ZeroMemory(&Item, sizeof(Item));
        Item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        Item.hItem = pEntry->hItem;
        Item.iImage = Item.iSelectedImage = Advanced_GetImage(pEntry);
        TreeView_SetItem(hwndTreeView, &Item);
    }

    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
}

/* FIXME: These macros should not be defined here */
#ifndef SSF_SHOWSUPERHIDDEN
    #define SSF_SHOWSUPERHIDDEN     0x00040000
#endif
#ifndef SSF_SEPPROCESS
    #define SSF_SEPPROCESS          0x00080000
#endif

static VOID
ScanAdvancedSettings(SHELLSTATE *pSS, DWORD *pdwMask)
{
    for (INT i = 0; i < s_AdvancedCount; ++i)
    {
        const ADVANCED_ENTRY *pEntry = &s_Advanced[i];
        if (pEntry->dwType == AETYPE_GROUP || pEntry->bGrayed)
            continue;

        BOOL bChecked = pEntry->bChecked;

        // FIXME: Add more items
        if (lstrcmpiW(pEntry->szKeyName, L"SuperHidden") == 0)
        {
            pSS->fShowSuperHidden = !bChecked ? 1 : 0;
            *pdwMask |= SSF_SHOWSUPERHIDDEN;
            continue;
        }
        if (lstrcmpiW(pEntry->szKeyName, L"DesktopProcess") == 0)
        {
            pSS->fSepProcess = bChecked ? 1 : 0;
            *pdwMask |= SSF_SEPPROCESS;
            continue;
        }
        if (lstrcmpiW(pEntry->szKeyName, L"SHOWALL") == 0)
        {
            pSS->fShowAllObjects = !bChecked ? 1 : 0;
            *pdwMask |= SSF_SHOWALLOBJECTS;
            continue;
        }
        if (lstrcmpiW(pEntry->szKeyName, L"HideFileExt") == 0)
        {
            pSS->fShowExtensions = !bChecked ? 1 : 0;
            *pdwMask |= SSF_SHOWEXTENSIONS;
            continue;
        }
        if (lstrcmpiW(pEntry->szKeyName, L"ShowCompColor") == 0)
        {
            pSS->fShowCompColor = bChecked ? 1 : 0;
            *pdwMask |= SSF_SHOWCOMPCOLOR;
            continue;
        }
        if (lstrcmpiW(pEntry->szKeyName, L"ShowInfoTip") == 0)
        {
            pSS->fShowInfoTip = bChecked ? 1 : 0;
            *pdwMask |= SSF_SHOWINFOTIP;
            continue;
        }
    }
}

extern "C"
VOID WINAPI SHGetSetSettings(LPSHELLSTATE lpss, DWORD dwMask, BOOL bSet);

static BOOL CALLBACK RefreshBrowsersCallback (HWND hWnd, LPARAM msg)
{
    WCHAR ClassName[100];
    if (GetClassName(hWnd, ClassName, 100))
    {
        if (!wcscmp(ClassName, L"Progman") || 
            !wcscmp(ClassName, L"CabinetWClass") ||
            !wcscmp(ClassName, L"ExploreWClass"))
        {
            PostMessage(hWnd, WM_COMMAND, FCIDM_DESKBROWSER_REFRESH, 0);
        }
    }
    return TRUE;
}

static VOID
ViewDlg_Apply(HWND hwndDlg)
{
    for (INT i = 0; i < s_AdvancedCount; ++i)
    {
        // ignore the entry if the type is group or the entry is grayed
        ADVANCED_ENTRY *pEntry = &s_Advanced[i];
        if (pEntry->dwType == AETYPE_GROUP || pEntry->bGrayed)
            continue;

        // open the registry key
        HKEY hkeyTarget;
        if (RegOpenKeyExW(HKEY(pEntry->hkeyRoot), pEntry->szRegPath, 0,
                          KEY_WRITE, &hkeyTarget) != ERROR_SUCCESS)
        {
            continue;
        }

        // checked or unchecked?
        DWORD dwValue, dwSize;
        if (pEntry->bChecked)
        {
            dwValue = pEntry->dwCheckedValue;
        }
        else
        {
            if (pEntry->bHasUncheckedValue)
            {
                dwValue = pEntry->dwUncheckedValue;
            }
            else
            {
                // there is no unchecked value
                RegCloseKey(hkeyTarget);
                continue;   // ignore
            }
        }

        // set the value
        dwSize = sizeof(dwValue);
        RegSetValueExW(hkeyTarget, pEntry->szValueName, 0, REG_DWORD,
                       LPBYTE(&dwValue), dwSize);

        // close now
        RegCloseKey(hkeyTarget);
    }

    // scan advanced settings for user's settings
    DWORD dwMask = 0;
    SHELLSTATE ShellState;
    ZeroMemory(&ShellState, sizeof(ShellState));
    ScanAdvancedSettings(&ShellState, &dwMask);

    // update user's settings
    SHGetSetSettings(&ShellState, dwMask, TRUE);

    // notify all
    SendMessage(HWND_BROADCAST, WM_WININICHANGE, 0, 0);
    
    EnumWindows(RefreshBrowsersCallback, NULL);
}

INT_PTR CALLBACK
FolderOptionsViewDlg(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    INT_PTR Result;
    NMTVCUSTOMDRAW *Draw;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            return ViewDlg_OnInitDialog(hwndDlg);
        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case 14004: // Restore Defaults
                    Advanced_RestoreDefaults(hwndDlg);
                    break;
            }
            break;
        case WM_NOTIFY:
            switch (LPNMHDR(lParam)->code)
            {
                case NM_CLICK:  // clicked on treeview
                    ViewDlg_OnTreeViewClick(hwndDlg);
                    break;
                case NM_CUSTOMDRAW:     // custom draw (for graying)
                    Draw = (NMTVCUSTOMDRAW *)lParam;
                    Result = ViewDlg_OnTreeCustomDraw(hwndDlg, Draw);
                    SetWindowLongPtr(hwndDlg, DWL_MSGRESULT, Result);
                    return Result;
                case TVN_KEYDOWN:       // key is down
                    ViewDlg_OnTreeViewKeyDown(hwndDlg, (TV_KEYDOWN *)lParam);
                    break;
                case PSN_APPLY:         // [Apply] is clicked
                    ViewDlg_Apply(hwndDlg);
                    break;
                default:
                    break;
            }
            break;
    }

    return FALSE;
}

static
VOID
InitializeFileTypesListCtrlColumns(HWND hDlgCtrl)
{
    RECT clientRect;
    LVCOLUMNW col;
    WCHAR szName[50];
    DWORD dwStyle;
    int columnSize = 140;


    if (!LoadStringW(shell32_hInstance, IDS_COLUMN_EXTENSION, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        /* default to english */
        wcscpy(szName, L"Extensions");
    }

    /* make sure its null terminated */
    szName[(sizeof(szName)/sizeof(WCHAR))-1] = 0;

    GetClientRect(hDlgCtrl, &clientRect);
    ZeroMemory(&col, sizeof(LV_COLUMN));
    columnSize = 140; //FIXME
    col.iSubItem   = 0;
    col.mask      = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM | LVCF_FMT;
    col.fmt = LVCFMT_FIXED_WIDTH;
    col.cx         = columnSize | LVCFMT_LEFT;
    col.cchTextMax = wcslen(szName);
    col.pszText    = szName;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 0, (LPARAM)&col);

    if (!LoadStringW(shell32_hInstance, IDS_FILE_TYPES, szName, sizeof(szName) / sizeof(WCHAR)))
    {
        /* default to english */
        wcscpy(szName, L"File Types");
        ERR("Failed to load localized string!\n");
    }

    col.iSubItem   = 1;
    col.cx         = clientRect.right - clientRect.left - columnSize;
    col.cchTextMax = wcslen(szName);
    col.pszText    = szName;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTCOLUMNW, 1, (LPARAM)&col);

    /* set full select style */
    dwStyle = (DWORD) SendMessage(hDlgCtrl, LVM_GETEXTENDEDLISTVIEWSTYLE, 0, 0);
    dwStyle = dwStyle | LVS_EX_FULLROWSELECT;
    SendMessage(hDlgCtrl, LVM_SETEXTENDEDLISTVIEWSTYLE, 0, dwStyle);
}

INT
FindItem(HWND hDlgCtrl, WCHAR * ItemName)
{
    LVFINDINFOW findInfo;
    ZeroMemory(&findInfo, sizeof(LVFINDINFOW));

    findInfo.flags = LVFI_STRING;
    findInfo.psz = ItemName;
    return ListView_FindItem(hDlgCtrl, 0, &findInfo);
}

static
VOID
InsertFileType(HWND hDlgCtrl, WCHAR * szName, PINT iItem, WCHAR * szFile)
{
    PFOLDER_FILE_TYPE_ENTRY Entry;
    HKEY hKey;
    LVITEMW lvItem;
    DWORD dwSize;
    DWORD dwType;

    if (szName[0] != L'.')
    {
        /* FIXME handle URL protocol handlers */
        return;
    }

    /* allocate file type entry */
    Entry = (PFOLDER_FILE_TYPE_ENTRY)HeapAlloc(GetProcessHeap(), 0, sizeof(FOLDER_FILE_TYPE_ENTRY));

    if (!Entry)
        return;

    /* open key */
    if (RegOpenKeyExW(HKEY_CLASSES_ROOT, szName, 0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        HeapFree(GetProcessHeap(), 0, Entry);
        return;
    }

    /* FIXME check for duplicates */

    /* query for the default key */
    dwSize = sizeof(Entry->ClassKey);
    if (RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)Entry->ClassKey, &dwSize) != ERROR_SUCCESS)
    {
        /* no link available */
        Entry->ClassKey[0] = 0;
    }

    if (Entry->ClassKey[0])
    {
        HKEY hTemp;
        /* try open linked key */
        if (RegOpenKeyExW(HKEY_CLASSES_ROOT, Entry->ClassKey, 0, KEY_READ, &hTemp) == ERROR_SUCCESS)
        {
            /* use linked key */
            RegCloseKey(hKey);
            hKey = hTemp;
        }
    }

    /* read friendly type name */
    if (RegLoadMUIStringW(hKey, L"FriendlyTypeName", Entry->FileDescription, sizeof(Entry->FileDescription), NULL, 0, NULL) != ERROR_SUCCESS)
    {
        /* read file description */
        dwSize = sizeof(Entry->FileDescription);
        Entry->FileDescription[0] = 0;

        /* read default key */
        RegQueryValueExW(hKey, NULL, NULL, NULL, (LPBYTE)Entry->FileDescription, &dwSize);
    }

    /* Read the EditFlags value */
    Entry->EditFlags = 0;
    if (!RegQueryValueExW(hKey, L"EditFlags", NULL, &dwType, NULL, &dwSize))
    {
        if ((dwType == REG_DWORD || dwType == REG_BINARY) && dwSize == sizeof(DWORD))
            RegQueryValueExW(hKey, L"EditFlags", NULL, NULL, (LPBYTE)&Entry->EditFlags, &dwSize);
    }

    /* close key */
    RegCloseKey(hKey);

    /* Do not add excluded entries */
    if (Entry->EditFlags & 0x00000001) //FTA_Exclude
    {
        HeapFree(GetProcessHeap(), 0, Entry);
        return;
    }

    /* convert extension to upper case */
    wcscpy(Entry->FileExtension, szName);
    _wcsupr(Entry->FileExtension);

    if (!Entry->FileDescription[0])
    {
        /* construct default 'FileExtensionFile' by formatting the uppercase extension
           with IDS_FILE_EXT_TYPE, outputting something like a l18n 'INI File' */

        StringCchPrintf(Entry->FileDescription, _countof(Entry->FileDescription), szFile, &Entry->FileExtension[1]);
    }

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT | LVIF_PARAM;
    lvItem.iSubItem = 0;
    lvItem.pszText = &Entry->FileExtension[1];
    lvItem.iItem = *iItem;
    lvItem.lParam = (LPARAM)Entry;
    (void)SendMessageW(hDlgCtrl, LVM_INSERTITEMW, 0, (LPARAM)&lvItem);

    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_TEXT;
    lvItem.pszText = Entry->FileDescription;
    lvItem.iItem = *iItem;
    lvItem.iSubItem = 1;
    ListView_SetItem(hDlgCtrl, &lvItem);

    (*iItem)++;
}

static
int
CALLBACK
ListViewCompareProc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
{
    PFOLDER_FILE_TYPE_ENTRY Entry1, Entry2;
    int x;

    Entry1 = (PFOLDER_FILE_TYPE_ENTRY)lParam1;
    Entry2 = (PFOLDER_FILE_TYPE_ENTRY)lParam2;

    x = wcsicmp(Entry1->FileExtension, Entry2->FileExtension);
    if (x != 0)
        return x;

    return wcsicmp(Entry1->FileDescription, Entry2->FileDescription);
}

static
PFOLDER_FILE_TYPE_ENTRY
InitializeFileTypesListCtrl(HWND hwndDlg)
{
    HWND hDlgCtrl;
    DWORD dwIndex = 0;
    WCHAR szName[50];
    WCHAR szFile[100];
    DWORD dwName;
    LVITEMW lvItem;
    INT iItem = 0;

    hDlgCtrl = GetDlgItem(hwndDlg, 14000);
    InitializeFileTypesListCtrlColumns(hDlgCtrl);

    szFile[0] = 0;
    if (!LoadStringW(shell32_hInstance, IDS_FILE_EXT_TYPE, szFile, _countof(szFile)))
    {
        /* default to english */
        wcscpy(szFile, L"%s File");
    }
    szFile[(_countof(szFile)) - 1] = 0;

    dwName = _countof(szName);

    while (RegEnumKeyExW(HKEY_CLASSES_ROOT, dwIndex++, szName, &dwName, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
    {
        InsertFileType(hDlgCtrl, szName, &iItem, szFile);
        dwName = _countof(szName);
    }

    /* Leave if the list is empty */
    if (iItem == 0)
        return NULL;

    /* sort list */
    ListView_SortItems(hDlgCtrl, ListViewCompareProc, NULL);

    /* select first item */
    ZeroMemory(&lvItem, sizeof(LVITEMW));
    lvItem.mask = LVIF_STATE;
    lvItem.stateMask = (UINT)-1;
    lvItem.state = LVIS_FOCUSED | LVIS_SELECTED;
    lvItem.iItem = 0;
    ListView_SetItem(hDlgCtrl, &lvItem);

    lvItem.mask = LVIF_PARAM;
    ListView_GetItem(hDlgCtrl, &lvItem);

    return (PFOLDER_FILE_TYPE_ENTRY)lvItem.lParam;
}

static
PFOLDER_FILE_TYPE_ENTRY
FindSelectedItem(
    HWND hDlgCtrl)
{
    UINT Count, Index;
    LVITEMW lvItem;

    Count = ListView_GetItemCount(hDlgCtrl);

    for (Index = 0; Index < Count; Index++)
    {
        ZeroMemory(&lvItem, sizeof(LVITEM));
        lvItem.mask = LVIF_PARAM | LVIF_STATE;
        lvItem.iItem = Index;
        lvItem.stateMask = (UINT) - 1;

        if (ListView_GetItem(hDlgCtrl, &lvItem))
        {
            if (lvItem.state & LVIS_SELECTED)
                return (PFOLDER_FILE_TYPE_ENTRY)lvItem.lParam;
        }
    }

    return NULL;
}

INT_PTR
CALLBACK
FolderOptionsFileTypesDlg(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LPNMLISTVIEW lppl;
    LVITEMW lvItem;
    WCHAR Buffer[255], FormatBuffer[255];
    PFOLDER_FILE_TYPE_ENTRY pItem;
    OPENASINFO Info;

    switch(uMsg)
    {
        case WM_INITDIALOG:
            pItem = InitializeFileTypesListCtrl(hwndDlg);

            /* Disable the Delete button if the listview is empty or
               the selected item should not be deleted by the user */
            if (pItem == NULL || (pItem->EditFlags & 0x00000010)) // FTA_NoRemove
                EnableWindow(GetDlgItem(hwndDlg, 14002), FALSE);
            return TRUE;

        case WM_COMMAND:
            switch(LOWORD(wParam))
            {
                case 14006:
                    pItem = FindSelectedItem(GetDlgItem(hwndDlg, 14000));
                    if (pItem)
                    {
                        Info.oaifInFlags = OAIF_ALLOW_REGISTRATION | OAIF_REGISTER_EXT;
                        Info.pcszClass = pItem->FileExtension;
                        SHOpenWithDialog(hwndDlg, &Info);
                    }
                    break;
            }
            break;

        case WM_NOTIFY:
            lppl = (LPNMLISTVIEW) lParam;

            if (lppl->hdr.code == LVN_ITEMCHANGING)
            {
                ZeroMemory(&lvItem, sizeof(LVITEM));
                lvItem.mask = LVIF_PARAM;
                lvItem.iItem = lppl->iItem;
                if (!SendMessageW(lppl->hdr.hwndFrom, LVM_GETITEMW, 0, (LPARAM)&lvItem))
                    return TRUE;

                pItem = (PFOLDER_FILE_TYPE_ENTRY)lvItem.lParam;
                if (!pItem)
                    return TRUE;

                if (!(lppl->uOldState & LVIS_FOCUSED) && (lppl->uNewState & LVIS_FOCUSED))
                {
                    /* new focused item */
                    if (!LoadStringW(shell32_hInstance, IDS_FILE_DETAILS, FormatBuffer, sizeof(FormatBuffer) / sizeof(WCHAR)))
                    {
                        /* use default english format string */
                        wcscpy(FormatBuffer, L"Details for '%s' extension");
                    }

                    /* format buffer */
                    swprintf(Buffer, FormatBuffer, &pItem->FileExtension[1]);
                    /* update dialog */
                    SetDlgItemTextW(hwndDlg, 14003, Buffer);

                    if (!LoadStringW(shell32_hInstance, IDS_FILE_DETAILSADV, FormatBuffer, sizeof(FormatBuffer) / sizeof(WCHAR)))
                    {
                        /* use default english format string */
                        wcscpy(FormatBuffer, L"Files with extension '%s' are of type '%s'. To change settings that affect all '%s' files, click Advanced.");
                    }
                    /* format buffer */
                    swprintf(Buffer, FormatBuffer, &pItem->FileExtension[1], &pItem->FileDescription[0], &pItem->FileDescription[0]);
                    /* update dialog */
                    SetDlgItemTextW(hwndDlg, 14007, Buffer);

                    /* Enable the Delete button */
                    if (pItem->EditFlags & 0x00000010) // FTA_NoRemove
                        EnableWindow(GetDlgItem(hwndDlg, 14002), FALSE);
                    else
                        EnableWindow(GetDlgItem(hwndDlg, 14002), TRUE);
                }
            }
            else if (lppl->hdr.code == PSN_SETACTIVE)
            {
                /* On page activation, set the focus to the listview */
                SetFocus(GetDlgItem(hwndDlg, 14000));
            }
            break;
    }

    return FALSE;
}

static
VOID
ShowFolderOptionsDialog(HWND hWnd, HINSTANCE hInst)
{
    PROPSHEETHEADERW pinfo;
    HPROPSHEETPAGE hppages[3];
    HPROPSHEETPAGE hpage;
    UINT num_pages = 0;
    WCHAR szOptions[100];

    hpage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_GENERAL, FolderOptionsGeneralDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_VIEW, FolderOptionsViewDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    hpage = SH_CreatePropertySheetPage(IDD_FOLDER_OPTIONS_FILETYPES, FolderOptionsFileTypesDlg, 0, NULL);
    if (hpage)
        hppages[num_pages++] = hpage;

    szOptions[0] = L'\0';
    LoadStringW(shell32_hInstance, IDS_FOLDER_OPTIONS, szOptions, sizeof(szOptions) / sizeof(WCHAR));
    szOptions[(sizeof(szOptions)/sizeof(WCHAR))-1] = L'\0';

    memset(&pinfo, 0x0, sizeof(PROPSHEETHEADERW));
    pinfo.dwSize = sizeof(PROPSHEETHEADERW);
    pinfo.dwFlags = PSH_NOCONTEXTHELP;
    pinfo.nPages = num_pages;
    pinfo.phpage = hppages;
    pinfo.pszCaption = szOptions;

    PropertySheetW(&pinfo);
}

static
VOID
Options_RunDLLCommon(HWND hWnd, HINSTANCE hInst, int fOptions, DWORD nCmdShow)
{
    switch(fOptions)
    {
        case 0:
            ShowFolderOptionsDialog(hWnd, hInst);
            break;
        case 1:
            // show taskbar options dialog
            FIXME("notify explorer to show taskbar options dialog");
            //PostMessage(GetShellWindow(), WM_USER+22, fOptions, 0);
            break;
        default:
            FIXME("unrecognized options id %d\n", fOptions);
    }
}

/*************************************************************************
 *              Options_RunDLL (SHELL32.@)
 */
EXTERN_C VOID WINAPI Options_RunDLL(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}

/*************************************************************************
 *              Options_RunDLLA (SHELL32.@)
 */
EXTERN_C VOID WINAPI Options_RunDLLA(HWND hWnd, HINSTANCE hInst, LPCSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntA(cmd), nCmdShow);
}

/*************************************************************************
 *              Options_RunDLLW (SHELL32.@)
 */
EXTERN_C VOID WINAPI Options_RunDLLW(HWND hWnd, HINSTANCE hInst, LPCWSTR cmd, DWORD nCmdShow)
{
    Options_RunDLLCommon(hWnd, hInst, StrToIntW(cmd), nCmdShow);
}
