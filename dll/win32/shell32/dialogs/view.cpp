/*
 *     'View' tab property sheet of Folder Options
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

/////////////////////////////////////////////////////////////////////////////
// View Tree

// predefined icon IDs (See ViewDlg_CreateTreeImageList function below)
#define I_CHECKED                   0
#define I_UNCHECKED                 1
#define I_CHECKED_DISABLED          2
#define I_UNCHECKED_DISABLED        3
#define I_RADIO_CHECKED             4
#define I_RADIO_UNCHECKED           5
#define I_RADIO_CHECKED_DISABLED    6
#define I_RADIO_UNCHECKED_DISABLED  7
#define PREDEFINED_ICON_COUNT       8

// uniquely-defined icon entry for View Advanced Settings
typedef struct VIEWTREE_ICON
{
    WCHAR   szPath[MAX_PATH];
    UINT    nIconIndex;
} VIEWTREE_ICON, *PVIEWTREE_ICON;

// types of View Advanced Setting entry
typedef enum VIEWTREE_ENTRY_TYPE
{
    AETYPE_GROUP,
    AETYPE_CHECKBOX,
    AETYPE_RADIO,
} VIEWTREE_ENTRY_TYPE, *PVIEWTREE_ENTRY_TYPE;

// an entry info of View Advanced Settings
typedef struct VIEWTREE_ENTRY
{
    DWORD   dwID;                   // entry ID
    DWORD   dwParentID;             // parent entry ID
    DWORD   dwResourceID;           // resource ID
    WCHAR   szKeyName[64];          // entry key name
    DWORD   dwType;                 // VIEWTREE_ENTRY_TYPE
    WCHAR   szText[MAX_PATH];       // text
    INT     nIconID;                // icon ID (See VIEWTREE_ICON)

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
} VIEWTREE_ENTRY, *PVIEWTREE_ENTRY;

// definition of view advanced entries
static PVIEWTREE_ENTRY  s_ViewTreeEntries       = NULL;
static INT              s_ViewTreeEntryCount    = 0;

// definition of icon stock
static PVIEWTREE_ICON   s_ViewTreeIcons         = NULL;
static INT              s_ViewTreeIconCount     = 0;
static HIMAGELIST       s_hTreeImageList        = NULL;

static INT
ViewTree_FindIcon(LPCWSTR pszPath, UINT nIconIndex)
{
    for (INT i = PREDEFINED_ICON_COUNT; i < s_ViewTreeIconCount; ++i)
    {
        PVIEWTREE_ICON pIcon = &s_ViewTreeIcons[i];
        if (pIcon->nIconIndex == nIconIndex &&
            lstrcmpiW(pIcon->szPath, pszPath) == 0)
        {
            return i;   // icon ID
        }
    }
    return -1;  // not found
}

static INT
ViewTree_AddIcon(LPCWSTR pszPath, UINT nIconIndex)
{
    PVIEWTREE_ICON pAllocated;

    // return the ID if already existed
    INT nIconID = ViewTree_FindIcon(pszPath, nIconIndex);
    if (nIconID != -1)
        return nIconID;     // already exists

    // extract a small icon
    HICON hIconSmall = NULL;
    ExtractIconExW(pszPath, nIconIndex, NULL, &hIconSmall, 1);
    if (hIconSmall == NULL)
        return -1;      // failure

    // resize s_ViewTreeIcons
    size_t Size = (s_ViewTreeIconCount + 1) * sizeof(VIEWTREE_ICON);
    pAllocated = (PVIEWTREE_ICON)realloc(s_ViewTreeIcons, Size);
    if (pAllocated == NULL)
        return -1;      // failure
    else
        s_ViewTreeIcons = pAllocated;

    // save icon information
    PVIEWTREE_ICON pIcon = &s_ViewTreeIcons[s_ViewTreeIconCount];
    lstrcpynW(pIcon->szPath, pszPath, _countof(pIcon->szPath));
    pIcon->nIconIndex = nIconIndex;

    // add the icon to the image list
    ImageList_AddIcon(s_hTreeImageList, hIconSmall);

    // increment the counter
    nIconID = s_ViewTreeIconCount;
    ++s_ViewTreeIconCount;

    DestroyIcon(hIconSmall);

    return nIconID;     // newly-added icon ID
}

static PVIEWTREE_ENTRY
ViewTree_GetItem(DWORD dwID)
{
    if (dwID == DWORD(-1))
        return NULL;

    for (INT i = 0; i < s_ViewTreeEntryCount; ++i)
    {
        PVIEWTREE_ENTRY pEntry = &s_ViewTreeEntries[i];
        if (pEntry->dwID == dwID)
            return pEntry;
    }
    return NULL;    // failure
}

static INT
ViewTree_GetImage(PVIEWTREE_ENTRY pEntry)
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
ViewTree_InsertEntry(HWND hwndTreeView, PVIEWTREE_ENTRY pEntry)
{
    PVIEWTREE_ENTRY pParent = ViewTree_GetItem(pEntry->dwParentID);
    HTREEITEM hParent = TVI_ROOT;
    if (pParent)
        hParent = pParent->hItem;

    TV_INSERTSTRUCT Insertion;
    ZeroMemory(&Insertion, sizeof(Insertion));
    Insertion.hParent = hParent;
    Insertion.hInsertAfter = TVI_LAST;
    Insertion.item.mask =
        TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
    Insertion.item.pszText = pEntry->szText;

    INT iImage = ViewTree_GetImage(pEntry);
    Insertion.item.iImage = Insertion.item.iSelectedImage = iImage;
    Insertion.item.lParam = pEntry->dwID;
    pEntry->hItem = TreeView_InsertItem(hwndTreeView, &Insertion);
}

static VOID
ViewTree_InsertAll(HWND hwndTreeView)
{
    TreeView_DeleteAllItems(hwndTreeView);

    // insert the entries
    PVIEWTREE_ENTRY pEntry;
    for (INT i = 0; i < s_ViewTreeEntryCount; ++i)
    {
        pEntry = &s_ViewTreeEntries[i];
        ViewTree_InsertEntry(hwndTreeView, pEntry);
    }

    // expand all
    for (INT i = 0; i < s_ViewTreeEntryCount; ++i)
    {
        pEntry = &s_ViewTreeEntries[i];
        if (pEntry->dwType == AETYPE_GROUP)
        {
            TreeView_Expand(hwndTreeView, pEntry->hItem, TVE_EXPAND);
        }
    }
}

static BOOL
ViewTree_LoadTree(HKEY hKey, LPCWSTR pszKeyName, DWORD dwParentID)
{
    DWORD dwIndex;
    WCHAR szKeyName[64], szText[MAX_PATH], *pch;
    DWORD Size, Value;
    PVIEWTREE_ENTRY pAllocated;

    // resize s_ViewTreeEntries
    Size = (s_ViewTreeEntryCount + 1) * sizeof(VIEWTREE_ENTRY);
    pAllocated = (PVIEWTREE_ENTRY)realloc(s_ViewTreeEntries, Size);
    if (pAllocated == NULL)
        return FALSE;   // failure
    else
        s_ViewTreeEntries = pAllocated;

    PVIEWTREE_ENTRY pEntry = &s_ViewTreeEntries[s_ViewTreeEntryCount];

    // dwID, dwParentID, szKeyName
    pEntry->dwID = s_ViewTreeEntryCount;
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
        pEntry->nIconID = ViewTree_AddIcon(szExpanded, nIconIndex);
    }

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
        HKEY HKeyRoot = HKEY_CURRENT_USER;
        Size = sizeof(HKeyRoot);
        RegQueryValueExW(hKey, L"HKeyRoot", NULL, NULL, LPBYTE(&HKeyRoot), &Size);
        pEntry->hkeyRoot = HKeyRoot;

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
    ++s_ViewTreeEntryCount;

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

        ViewTree_LoadTree(hkeyChild, szKeyName, dwParentID);
        RegCloseKey(hkeyChild);

        ++dwIndex;
    }

    return TRUE;    // success
}

static BOOL ViewTree_LoadAll(VOID)
{
    // free if already existed
    if (s_ViewTreeEntries)
    {
        free(s_ViewTreeEntries);
        s_ViewTreeEntries = NULL;
    }
    s_ViewTreeEntryCount = 0;

    HKEY hKey;
    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE,
                      L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced",
                      0, KEY_READ, &hKey) != ERROR_SUCCESS)
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

        ViewTree_LoadTree(hkeyChild, szKeyName, DWORD(-1));
        RegCloseKey(hkeyChild);

        ++dwIndex;
    }

    RegCloseKey(hKey);

    return TRUE;    // success
}

static int ViewTree_Compare(const void *x, const void *y)
{
    PVIEWTREE_ENTRY pEntry1 = (PVIEWTREE_ENTRY)x;
    PVIEWTREE_ENTRY pEntry2 = (PVIEWTREE_ENTRY)y;

    DWORD dwParentID1 = pEntry1->dwParentID;
    DWORD dwParentID2 = pEntry2->dwParentID;

    if (dwParentID1 == dwParentID2)
        return lstrcmpi(pEntry1->szText, pEntry2->szText);

    DWORD i, m, n;
    const UINT MAX_DEPTH = 32;
    PVIEWTREE_ENTRY pArray1[MAX_DEPTH];
    PVIEWTREE_ENTRY pArray2[MAX_DEPTH];

    // Make ancestor lists
    for (i = m = n = 0; i < MAX_DEPTH; ++i)
    {
        PVIEWTREE_ENTRY pParent1 = ViewTree_GetItem(dwParentID1);
        PVIEWTREE_ENTRY pParent2 = ViewTree_GetItem(dwParentID2);
        if (!pParent1 && !pParent2)
            break;

        if (pParent1)
        {
            pArray1[m++] = pParent1;
            dwParentID1 = pParent1->dwParentID;
        }
        if (pParent2)
        {
            pArray2[n++] = pParent2;
            dwParentID2 = pParent2->dwParentID;
        }
    }

    UINT k = min(m, n);
    for (i = 0; i < k; ++i)
    {
        INT nCompare = lstrcmpi(pArray1[m - i - 1]->szText, pArray2[n - i - 1]->szText);
        if (nCompare < 0)
            return -1;
        if (nCompare > 0)
            return 1;
    }

    if (m < n)
        return -1;
    if (m > n)
        return 1;
    return lstrcmpi(pEntry1->szText, pEntry2->szText);
}

static VOID
ViewTree_SortAll(VOID)
{
    qsort(s_ViewTreeEntries, s_ViewTreeEntryCount, sizeof(VIEWTREE_ENTRY), ViewTree_Compare);
}

/////////////////////////////////////////////////////////////////////////////
// ViewDlg

static HIMAGELIST
ViewDlg_CreateTreeImageList(VOID)
{
    HIMAGELIST hImageList;
    hImageList = ImageList_Create(16, 16, ILC_COLOR24 | ILC_MASK, 9, 1);
    if (hImageList == NULL)
        return NULL;    // failure

    // free if existed
    if (s_ViewTreeIcons)
    {
        free(s_ViewTreeIcons);
        s_ViewTreeIcons = NULL;
    }
    s_ViewTreeIconCount = 0;

    // allocate now
    PVIEWTREE_ICON pAllocated;
    size_t Size = PREDEFINED_ICON_COUNT * sizeof(VIEWTREE_ICON);
    pAllocated = (PVIEWTREE_ICON)calloc(1, Size);
    if (pAllocated == NULL)
        return NULL;    // failure

    s_ViewTreeIconCount = PREDEFINED_ICON_COUNT;
    s_ViewTreeIcons = pAllocated;

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

static BOOL
ViewDlg_OnInitDialog(HWND hwndDlg)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

    s_hTreeImageList = ViewDlg_CreateTreeImageList();
    TreeView_SetImageList(hwndTreeView, s_hTreeImageList, TVSIL_NORMAL);

    ViewTree_LoadAll();
    ViewTree_SortAll();
    ViewTree_InsertAll(hwndTreeView);

    return TRUE;    // set focus
}

static BOOL
ViewDlg_ToggleCheckItem(HWND hwndDlg, HTREEITEM hItem)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

    // get the item
    TV_ITEM Item;
    INT i;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_PARAM;
    Item.hItem = hItem;
    if (!TreeView_GetItem(hwndTreeView, &Item))
        return FALSE;       // no such item

    VIEWTREE_ENTRY *pEntry = ViewTree_GetItem(Item.lParam);
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
            for (i = 0; i < s_ViewTreeEntryCount; ++i)
            {
                VIEWTREE_ENTRY *pEntry2 = &s_ViewTreeEntries[i];
                if (pEntry->dwParentID == pEntry2->dwParentID)
                {
                    pEntry2->bChecked = FALSE;

                    Item.hItem = pEntry2->hItem;
                    INT iImage = ViewTree_GetImage(pEntry2);
                    Item.iImage = Item.iSelectedImage = iImage;
                    TreeView_SetItem(hwndTreeView, &Item);
                }
            }
            pEntry->bChecked = TRUE;
            break;

        default:
            return FALSE;   // failure
    }
    Item.iImage = Item.iSelectedImage = ViewTree_GetImage(pEntry);
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
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

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
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

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
                VIEWTREE_ENTRY *pEntry = ViewTree_GetItem(lParam);
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
ViewDlg_RestoreDefaults(HWND hwndDlg)
{
    HWND hwndTreeView = GetDlgItem(hwndDlg, IDC_VIEW_TREEVIEW);

    for (INT i = 0; i < s_ViewTreeEntryCount; ++i)
    {
        // ignore if the type is group
        VIEWTREE_ENTRY *pEntry = &s_ViewTreeEntries[i];
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
                       LPBYTE(&pEntry->dwDefaultValue), sizeof(DWORD));
        RegCloseKey(hKey);

        // update check status
        pEntry->bChecked = (pEntry->dwCheckedValue == pEntry->dwDefaultValue);

        // update the image
        TV_ITEM Item;
        ZeroMemory(&Item, sizeof(Item));
        Item.mask = TVIF_HANDLE | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
        Item.hItem = pEntry->hItem;
        Item.iImage = Item.iSelectedImage = ViewTree_GetImage(pEntry);
        TreeView_SetItem(hwndTreeView, &Item);
    }

    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
}

static VOID
ScanAdvancedSettings(SHELLSTATE *pSS, DWORD *pdwMask)
{
    for (INT i = 0; i < s_ViewTreeEntryCount; ++i)
    {
        const VIEWTREE_ENTRY *pEntry = &s_ViewTreeEntries[i];
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

static BOOL CALLBACK
RefreshBrowsersCallback(HWND hWnd, LPARAM msg)
{
    WCHAR ClassName[100];
    if (GetClassNameW(hWnd, ClassName, _countof(ClassName)))
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
    for (INT i = 0; i < s_ViewTreeEntryCount; ++i)
    {
        // ignore the entry if the type is group or the entry is grayed
        VIEWTREE_ENTRY *pEntry = &s_ViewTreeEntries[i];
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

// IDD_FOLDER_OPTIONS_VIEW
INT_PTR CALLBACK
FolderOptionsViewDlg(
    HWND    hwndDlg,
    UINT    uMsg,
    WPARAM  wParam,
    LPARAM  lParam)
{
    INT_PTR Result;
    NMTVCUSTOMDRAW *Draw;

    switch (uMsg)
    {
        case WM_INITDIALOG:
            return ViewDlg_OnInitDialog(hwndDlg);

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDC_VIEW_RESTORE_DEFAULTS: // Restore Defaults
                    ViewDlg_RestoreDefaults(hwndDlg);
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
                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, Result);
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
