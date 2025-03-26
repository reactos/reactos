// FontSub by Katayama Hirofumi MZ
//
// To the extent possible under law, the person who associated CC0 with
// FontSub has waived all copyright and related or neighboring rights
// to FontSub.
//
// You should have received a copy of the CC0 legalcode along with this
// work.  If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.


#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <tchar.h>
#include <vector>       // for std::vector
#include <set>          // for std::set
#include <string>       // for std::basic_string
#include <algorithm>    // for std::sort
#include <cstdio>
#include <cstring>
#include <cassert>
#include "resource.h"


#define NAME_COLUMN_WIDTH   250
#define SUB_COLUMN_WIDTH    250
#define MAX_STRING          120

#ifndef _countof
    #define _countof(array)     (sizeof(array) / sizeof(array[0]))
#endif

typedef std::wstring        STRING;

struct ITEM
{
    STRING  m_Name, m_Substitute;
    BYTE    m_CharSet1, m_CharSet2;
    ITEM(const STRING& Name, const STRING& Substitute,
         BYTE CharSet1, BYTE CharSet2)
        : m_Name(Name), m_Substitute(Substitute),
          m_CharSet1(CharSet1), m_CharSet2(CharSet2) { }
};

/* global variables */
HINSTANCE   g_hInstance = NULL;
HWND        g_hMainWnd = NULL;
HICON       g_hIcon = NULL;
HWND        g_hListView = NULL;
BOOL        g_bModified = FALSE;
BOOL        g_bNeedsReboot = FALSE;
INT         g_iItem = 0;

LPCWSTR     g_pszClassName = L"ReactOS Font Substitutes Editor";
LPCWSTR     g_pszFileHeader = L"Windows Registry Editor Version 5.00";

WCHAR       g_szTitle[MAX_STRING];
WCHAR       g_szNameHead[MAX_STRING];
WCHAR       g_szSubstituteHead[MAX_STRING];

INT         g_iSortColumn = 0;
BOOL        g_bSortAscendant = TRUE;

LPCWSTR     g_pszKey =
    L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes";

typedef std::set<STRING>    FONTNAMESET;
typedef std::vector<ITEM>   ITEMVECTOR;

FONTNAMESET g_Names;
ITEMVECTOR  g_Items;
STRING      g_strFontName;
STRING      g_strSubstitute;
BYTE        g_CharSet1 = DEFAULT_CHARSET;
BYTE        g_CharSet2 = DEFAULT_CHARSET;

typedef struct CHARSET_ENTRY
{
    BYTE        CharSet;
    LPCWSTR     DisplayName;
} CHARSET_ENTRY;

CHARSET_ENTRY g_CharSetList[] =
{
    { DEFAULT_CHARSET,      L"DEFAULT_CHARSET (1)" },
    { ANSI_CHARSET,         L"ANSI_CHARSET (0)" },
    { SYMBOL_CHARSET,       L"SYMBOL_CHARSET (2)" },
    { SHIFTJIS_CHARSET,     L"SHIFTJIS_CHARSET (128)" },
    { HANGUL_CHARSET,       L"HANGUL_CHARSET (129)" },
    { GB2312_CHARSET,       L"GB2312_CHARSET (134)" },
    { CHINESEBIG5_CHARSET,  L"CHINESEBIG5_CHARSET (136)" },
    { OEM_CHARSET,          L"OEM_CHARSET (255)" },
    { JOHAB_CHARSET,        L"JOHAB_CHARSET (130)" },
    { HEBREW_CHARSET,       L"HEBREW_CHARSET (177)" },
    { ARABIC_CHARSET,       L"ARABIC_CHARSET (178)" },
    { GREEK_CHARSET,        L"GREEK_CHARSET (161)" },
    { TURKISH_CHARSET,      L"TURKISH_CHARSET (162)" },
    { VIETNAMESE_CHARSET,   L"VIETNAMESE_CHARSET (163)" },
    { THAI_CHARSET,         L"THAI_CHARSET (222)" },
    { EASTEUROPE_CHARSET,   L"EASTEUROPE_CHARSET (238)" },
    { RUSSIAN_CHARSET,      L"RUSSIAN_CHARSET (204)" },
    { MAC_CHARSET,          L"MAC_CHARSET (77)" },
    { BALTIC_CHARSET,       L"BALTIC_CHARSET (186)" }
};
const WCHAR g_LongestName[] = L"CHINESEBIG5_CHARSET (136)";

static void trim(STRING& str)
{
    static const WCHAR Spaces[] = L" \t\r\n";
    size_t i = str.find_first_not_of(Spaces);
    size_t j = str.find_last_not_of(Spaces);
    if (i == STRING::npos || j == STRING::npos)
    {
        str.clear();
    }
    else
    {
        str = str.substr(i, j - i + 1);
    }
}

static int CALLBACK
EnumFontFamExProc(const ENUMLOGFONTW *pelf,
                  const NEWTEXTMETRICW *pntm,
                  int FontType,
                  LPARAM lParam)
{
    switch (pelf->elfFullName[0])
    {
        case UNICODE_NULL: case L'@':
            break;
        default:
            g_Names.insert((const WCHAR *)pelf->elfFullName);
    }
    switch (pelf->elfLogFont.lfFaceName[0])
    {
        case UNICODE_NULL: case L'@':
            break;
        default:
            g_Names.insert(pelf->elfLogFont.lfFaceName);
    }
    return 1;
}

BOOL DoLoadNames(void)
{
    g_Names.clear();

    LOGFONTW lf;
    ZeroMemory(&lf, sizeof(lf));
    lf.lfCharSet = DEFAULT_CHARSET;

    HDC hDC = CreateCompatibleDC(NULL);
    EnumFontFamiliesExW(hDC, &lf, (FONTENUMPROCW)EnumFontFamExProc, 0, 0);
    DeleteDC(hDC);

    return !g_Names.empty();
}

inline bool ItemCompareByNameAscend(const ITEM& Item1, const ITEM& Item2)
{
    return Item1.m_Name < Item2.m_Name;
}

inline bool ItemCompareByNameDescend(const ITEM& Item1, const ITEM& Item2)
{
    return Item1.m_Name > Item2.m_Name;
}

inline bool ItemCompareBySubAscend(const ITEM& Item1, const ITEM& Item2)
{
    return Item1.m_Substitute < Item2.m_Substitute;
}

inline bool ItemCompareBySubDescend(const ITEM& Item1, const ITEM& Item2)
{
    return Item1.m_Substitute > Item2.m_Substitute;
}

void DoSort(INT iColumn, BOOL bAscendant = TRUE)
{
    LV_COLUMN Column;
    ZeroMemory(&Column, sizeof(Column));
    Column.mask = LVCF_IMAGE | LVCF_SUBITEM;
    Column.iImage = 2;
    Column.iSubItem = 0;
    ListView_SetColumn(g_hListView, 0, &Column);
    Column.iSubItem = 1;
    ListView_SetColumn(g_hListView, 1, &Column);

    switch (iColumn)
    {
        case 0:
            Column.iSubItem = 0;
            if (bAscendant)
            {
                std::sort(g_Items.begin(), g_Items.end(),
                          ItemCompareByNameAscend);
                Column.iImage = 0;
                ListView_SetColumn(g_hListView, 0, &Column);
            }
            else
            {
                std::sort(g_Items.begin(), g_Items.end(),
                          ItemCompareByNameDescend);
                Column.iImage = 1;
                ListView_SetColumn(g_hListView, 0, &Column);
            }
            break;
        case 1:
            Column.iSubItem = 1;
            if (bAscendant)
            {
                std::sort(g_Items.begin(), g_Items.end(),
                          ItemCompareBySubAscend);
                Column.iImage = 0;
                ListView_SetColumn(g_hListView, 1, &Column);
            }
            else
            {
                std::sort(g_Items.begin(), g_Items.end(),
                          ItemCompareBySubDescend);
                Column.iImage = 1;
                ListView_SetColumn(g_hListView, 1, &Column);
            }
            break;
    }
    g_iSortColumn = iColumn;
    g_bSortAscendant = bAscendant;
    InvalidateRect(g_hListView, NULL, TRUE);
}

void LV_AddItems(HWND hwnd)
{
    ListView_DeleteAllItems(hwnd);

    LV_ITEM Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = LVIF_PARAM;

    const INT Count = INT(g_Items.size());
    for (INT i = 0; i < Count; ++i)
    {
        Item.iItem = i;
        Item.iSubItem = 0;
        Item.lParam = i;
        ListView_InsertItem(hwnd, &Item);

        Item.iItem = i;
        Item.iSubItem = 1;
        Item.lParam = i;
        ListView_InsertItem(hwnd, &Item);
    }
}

BOOL DoLoadItems(void)
{
    ITEMVECTOR Items;

    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, g_pszKey, 0, KEY_READ, &hKey);
    if (hKey == NULL)
        return FALSE;

    WCHAR szName[MAX_STRING], szValue[MAX_STRING];
    DWORD cbName, cbValue;
    for (DWORD dwIndex = 0; ; ++dwIndex)
    {
        cbName = sizeof(szName);
        cbValue = sizeof(szValue);
        LONG Error = RegEnumValueW(hKey, dwIndex, szName, &cbName,
            NULL, NULL, (LPBYTE)szValue, &cbValue);
        if (Error != ERROR_SUCCESS)
            break;

        BYTE CharSet1 = DEFAULT_CHARSET, CharSet2 = DEFAULT_CHARSET;
        LPWSTR pch;

        pch = wcsrchr(szName, L',');
        if (pch)
        {
            *pch = 0;
            CharSet1 = (BYTE)_wtoi(pch + 1);
        }

        pch = wcsrchr(szValue, L',');
        if (pch)
        {
            *pch = 0;
            CharSet2 = (BYTE)_wtoi(pch + 1);
        }

        ITEM Item(szName, szValue, CharSet1, CharSet2);
        trim(Item.m_Name);
        trim(Item.m_Substitute);
        Items.push_back(Item);
    }

    RegCloseKey(hKey);

    g_Items = Items;
    LV_AddItems(g_hListView);
    DoSort(0, TRUE);
    g_bModified = FALSE;
    g_bNeedsReboot = FALSE;

    return !g_Items.empty();
}

BOOL DoLoad(void)
{
    return DoLoadNames() && DoLoadItems();
}

void LV_InvalidateRow(HWND hwnd, INT iRow = -1)
{
    if (iRow == -1)
        iRow = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
    if (iRow == -1)
        return;

    RECT Rect;
    LPRECT GccIsWhining = &Rect;
    ListView_GetItemRect(hwnd, iRow, GccIsWhining, LVIR_BOUNDS);
    InvalidateRect(hwnd, &Rect, FALSE);
}

BOOL LV_Init(HWND hwnd)
{
    ListView_SetExtendedListViewStyle(hwnd,
        LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

    HIMAGELIST hImageList;
    hImageList = ImageList_Create(12, 12, ILC_COLOR8 | ILC_MASK, 2, 2);

    HBITMAP hbm;
    hbm = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(2), IMAGE_BITMAP,
                             12, 12, LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS);
    assert(hbm);
    ImageList_AddMasked(hImageList, hbm, RGB(192, 192, 192));
    DeleteObject(hbm);

    hbm = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(3), IMAGE_BITMAP,
                             12, 12, LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS);
    assert(hbm);
    ImageList_AddMasked(hImageList, hbm, RGB(192, 192, 192));
    DeleteObject(hbm);

    hbm = (HBITMAP)LoadImage(g_hInstance, MAKEINTRESOURCE(4), IMAGE_BITMAP,
                             12, 12, LR_CREATEDIBSECTION | LR_LOADMAP3DCOLORS);
    assert(hbm);
    ImageList_AddMasked(hImageList, hbm, RGB(192, 192, 192));
    DeleteObject(hbm);

    ListView_SetImageList(hwnd, hImageList, LVSIL_SMALL);

    LV_COLUMNW Column;
    ZeroMemory(&Column, sizeof(Column));
    Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH | LVCF_IMAGE;
    Column.fmt = LVCFMT_LEFT;

    Column.cx = NAME_COLUMN_WIDTH;
    Column.pszText = g_szNameHead;
    Column.iSubItem = 0;
    Column.iImage = 0;
    ListView_InsertColumn(hwnd, 0, &Column);

    Column.cx = SUB_COLUMN_WIDTH;
    Column.pszText = g_szSubstituteHead;
    Column.iSubItem = 1;
    Column.iImage = 2;
    ListView_InsertColumn(hwnd, 1, &Column);

    UINT State = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(hwnd, 0, State, State);

    return TRUE;
}

BOOL EditDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    COMBOBOXEXITEMW Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.mask = CBEIF_TEXT;

    FONTNAMESET::iterator it, end = g_Names.end();
    for (it = g_Names.begin(); it != end; ++it)
    {
        Item.pszText = const_cast<LPWSTR>(it->c_str());
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb2));
        SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEM, 0, (LPARAM)&Item);
    }
    SetDlgItemTextW(hwnd, edt1, g_strFontName.c_str());
    SetDlgItemTextW(hwnd, cmb2, g_strSubstitute.c_str());

    const INT Count = _countof(g_CharSetList);
    for (INT i = 0; i < Count; ++i)
    {
        Item.pszText = const_cast<LPWSTR>(g_CharSetList[i].DisplayName);
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb3));
        SendDlgItemMessageW(hwnd, cmb3, CBEM_INSERTITEM, 0, (LPARAM)&Item);
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb4));
        SendDlgItemMessageW(hwnd, cmb4, CBEM_INSERTITEM, 0, (LPARAM)&Item);
    }

    SendDlgItemMessageW(hwnd, cmb3, CB_SETCURSEL, 0, 0);
    SendDlgItemMessageW(hwnd, cmb4, CB_SETCURSEL, 0, 0);
    for (INT i = 0; i < Count; ++i)
    {
        if (g_CharSet1 == g_CharSetList[i].CharSet)
        {
            SendDlgItemMessageW(hwnd, cmb3, CB_SETCURSEL, i, 0);
        }
        if (g_CharSet2 == g_CharSetList[i].CharSet)
        {
            SendDlgItemMessageW(hwnd, cmb4, CB_SETCURSEL, i, 0);
        }
    }

    SIZE siz;
    HDC hDC = CreateCompatibleDC(NULL);
    SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
    GetTextExtentPoint32W(hDC, g_LongestName, lstrlenW(g_LongestName), &siz);
    DeleteDC(hDC);

    SendDlgItemMessageW(hwnd, cmb3, CB_SETHORIZONTALEXTENT, siz.cx + 16, 0);
    SendDlgItemMessageW(hwnd, cmb4, CB_SETHORIZONTALEXTENT, siz.cx + 16, 0);

    EnableWindow(GetDlgItem(hwnd, cmb3), FALSE);

    return TRUE;
}

void LV_OnDelete(HWND hwnd, INT iRow = -1)
{
    if (iRow == -1)
        iRow = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
    if (iRow == -1)
        return;

    UINT State = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(g_hListView, iRow, State, State);

    WCHAR sz[MAX_STRING];
    LoadStringW(g_hInstance, IDS_QUERYDELETE, sz, _countof(sz));
    if (IDYES != MessageBoxW(g_hMainWnd, sz, g_szTitle,
                             MB_ICONINFORMATION | MB_YESNO))
    {
        return;
    }

    ListView_DeleteItem(hwnd, iRow);
    g_Items.erase(g_Items.begin() + iRow);
    g_bModified = TRUE;

    ListView_SetItemState(g_hListView, iRow, State, State);

    InvalidateRect(hwnd, NULL, TRUE);
}

void EditDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    WCHAR szValue[MAX_STRING];
    STRING str;
    INT i;

    switch (id)
    {
        case IDOK:
            GetDlgItemTextW(hwnd, cmb2, szValue, _countof(szValue));
            str = szValue;
            trim(str);
            if (str.empty())
            {
                WCHAR sz[MAX_STRING];
                SendDlgItemMessageW(hwnd, cmb2, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                SetFocus(GetDlgItem(hwnd, cmb2));
                LoadStringW(g_hInstance, IDS_ENTERNAME2, sz, _countof(sz));
                MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
                return;
            }

            g_Items[g_iItem].m_CharSet2 = DEFAULT_CHARSET;
            i = SendDlgItemMessageW(hwnd, cmb4, CB_GETCURSEL, 0, 0);
            if (i != CB_ERR)
            {
                g_Items[g_iItem].m_CharSet2 = g_CharSetList[i].CharSet;
            }
            g_Items[g_iItem].m_Substitute = str;

            g_bModified = TRUE;
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
        case psh1:
            LV_OnDelete(g_hListView, g_iItem);
            EndDialog(hwnd, psh1);
            break;
    }
}

INT_PTR CALLBACK
EditDlg_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, EditDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, EditDlg_OnCommand);
    }
    return 0;
}

void LV_OnDblClk(HWND hwnd)
{
    g_iItem = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
    if (g_iItem == -1)
        return;

    g_strFontName = g_Items[g_iItem].m_Name;
    g_strSubstitute = g_Items[g_iItem].m_Substitute;
    g_CharSet1 = g_Items[g_iItem].m_CharSet1;
    g_CharSet2 = g_Items[g_iItem].m_CharSet2;

    DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_EDIT), g_hMainWnd,
              EditDlg_DlgProc);
    InvalidateRect(g_hListView, NULL, TRUE);
}

BOOL MainWnd_OnCreate(HWND hwnd, LPCREATESTRUCT lpCreateStruct)
{
    DWORD dwStyle = WS_CHILD | WS_VISIBLE | WS_VSCROLL |
                    LVS_SINGLESEL | LVS_REPORT | LVS_OWNERDRAWFIXED;
    DWORD dwExStyle = WS_EX_CLIENTEDGE;
    g_hListView = CreateWindowEx(dwExStyle, WC_LISTVIEW, NULL, dwStyle,
                                 0, 0, 0, 0,
                                 hwnd, (HMENU)1, g_hInstance, NULL);
    if (g_hListView == NULL)
        return FALSE;

    if (!LV_Init(g_hListView))
        return FALSE;

    if (!DoLoad())
        return FALSE;

    UINT State = LVIS_SELECTED | LVIS_FOCUSED;
    ListView_SetItemState(g_hListView, 0, State, State);
    SetFocus(g_hListView);
    return TRUE;
}

BOOL AddDlg_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    COMBOBOXEXITEMW Item;
    ZeroMemory(&Item, sizeof(Item));
    Item.iItem = -1;
    Item.mask = CBEIF_TEXT;

    FONTNAMESET::iterator it, end = g_Names.end();
    for (it = g_Names.begin(); it != end; ++it)
    {
        Item.pszText = const_cast<LPWSTR>(it->c_str());
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb1));
        SendDlgItemMessageW(hwnd, cmb1, CBEM_INSERTITEM, 0, (LPARAM)&Item);
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb2));
        SendDlgItemMessageW(hwnd, cmb2, CBEM_INSERTITEM, 0, (LPARAM)&Item);
    }
    WCHAR szEnterName[MAX_STRING];
    LoadStringW(g_hInstance, IDS_ENTERNAME, szEnterName, _countof(szEnterName));
    SetDlgItemTextW(hwnd, cmb1, szEnterName);
    SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));

    const INT Count = _countof(g_CharSetList);
    for (INT i = 0; i < Count; ++i)
    {
        Item.pszText = const_cast<LPWSTR>(g_CharSetList[i].DisplayName);
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb3));
        SendDlgItemMessageW(hwnd, cmb3, CBEM_INSERTITEM, 0, (LPARAM)&Item);
        Item.iItem = ComboBox_GetCount(GetDlgItem(hwnd, cmb4));
        SendDlgItemMessageW(hwnd, cmb4, CBEM_INSERTITEM, 0, (LPARAM)&Item);
    }

    SendDlgItemMessageW(hwnd, cmb3, CB_SETCURSEL, 0, 0);
    SendDlgItemMessageW(hwnd, cmb4, CB_SETCURSEL, 0, 0);
    for (INT i = 0; i < Count; ++i)
    {
        if (g_CharSet1 == g_CharSetList[i].CharSet)
        {
            SendDlgItemMessageW(hwnd, cmb3, CB_SETCURSEL, i, 0);
        }
        if (g_CharSet2 == g_CharSetList[i].CharSet)
        {
            SendDlgItemMessageW(hwnd, cmb4, CB_SETCURSEL, i, 0);
        }
    }

    SIZE siz;
    HDC hDC = CreateCompatibleDC(NULL);
    SelectObject(hDC, GetStockObject(DEFAULT_GUI_FONT));
    GetTextExtentPoint32W(hDC, g_LongestName, lstrlenW(g_LongestName), &siz);
    DeleteDC(hDC);

    SendDlgItemMessageW(hwnd, cmb3, CB_SETHORIZONTALEXTENT, siz.cx + 16, 0);
    SendDlgItemMessageW(hwnd, cmb4, CB_SETHORIZONTALEXTENT, siz.cx + 16, 0);

    return TRUE;
}

void AddDlg_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    WCHAR szKey[MAX_STRING], szValue[MAX_STRING], sz[MAX_STRING];
    INT i, iCharSet1, iCharSet2;
    BYTE CharSet1, CharSet2;
    STRING key, value;
    switch (id)
    {
        case IDOK:
            GetDlgItemTextW(hwnd, cmb1, szKey, _countof(szKey));
            key = szKey;
            trim(key);
            LoadStringW(g_hInstance, IDS_ENTERNAME, sz, _countof(sz));
            if (key.empty() || key == sz)
            {
                SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                SetFocus(GetDlgItem(hwnd, cmb1));
                LoadStringW(g_hInstance, IDS_ENTERNAME2, sz, _countof(sz));
                MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
                return;
            }

            GetDlgItemTextW(hwnd, cmb2, szValue, _countof(szValue));
            value = szValue;
            trim(value);
            if (value.empty())
            {
                SendDlgItemMessageW(hwnd, cmb2, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                SetFocus(GetDlgItem(hwnd, cmb2));
                LoadStringW(g_hInstance, IDS_ENTERNAME2, sz, _countof(sz));
                MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
                return;
            }

            iCharSet1 = SendDlgItemMessageW(hwnd, cmb3, CB_GETCURSEL, 0, 0);
            if (iCharSet1 == CB_ERR)
                iCharSet1 = 0;
            iCharSet2 = SendDlgItemMessageW(hwnd, cmb4, CB_GETCURSEL, 0, 0);
            if (iCharSet2 == CB_ERR)
                iCharSet2 = 0;

            CharSet1 = g_CharSetList[iCharSet1].CharSet;
            CharSet2 = g_CharSetList[iCharSet2].CharSet;

            for (i = 0; i < (INT)g_Items.size(); ++i)
            {
                if (g_Items[i].m_Name == key &&
                    g_Items[i].m_CharSet1 == CharSet1)
                {
                    WCHAR sz[MAX_STRING];
                    SendDlgItemMessageW(hwnd, cmb1, CB_SETEDITSEL, 0, MAKELPARAM(0, -1));
                    SetFocus(GetDlgItem(hwnd, cmb1));
                    LoadStringW(g_hInstance, IDS_ALREADYEXISTS, sz, _countof(sz));
                    MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
                    return;
                }
            }
            {
                ITEM Item(key, value, CharSet1, CharSet2);
                g_Items.push_back(Item);
                g_bModified = TRUE;

                i = (INT)g_Items.size();
                LV_ITEM LvItem;
                ZeroMemory(&LvItem, sizeof(LvItem));
                LvItem.mask = LVIF_PARAM;
                LvItem.iItem = i;
                LvItem.lParam = i;

                LvItem.iSubItem = 0;
                ListView_InsertItem(g_hListView, &LvItem);

                LvItem.iSubItem = 1;
                ListView_InsertItem(g_hListView, &LvItem);
            }
            g_bModified = TRUE;
            EndDialog(hwnd, IDOK);
            break;
        case IDCANCEL:
            EndDialog(hwnd, IDCANCEL);
            break;
    }
}

INT_PTR CALLBACK
AddDlg_DlgProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_INITDIALOG, AddDlg_OnInitDialog);
        HANDLE_MSG(hwnd, WM_COMMAND, AddDlg_OnCommand);
    }
    return 0;
}

void MainWnd_OnNew(HWND hwnd)
{
    g_iItem = ListView_GetNextItem(hwnd, -1, LVNI_SELECTED);
    if (g_iItem == -1)
        return;

    g_strFontName = g_Items[g_iItem].m_Name;
    g_strSubstitute = g_Items[g_iItem].m_Substitute;
    g_CharSet1 = g_Items[g_iItem].m_CharSet1;
    g_CharSet2 = g_Items[g_iItem].m_CharSet2;

    if (IDOK == DialogBox(g_hInstance, MAKEINTRESOURCE(IDD_ADD), g_hMainWnd,
                          AddDlg_DlgProc))
    {
        INT i = ListView_GetItemCount(g_hListView) - 1;
        UINT State = LVIS_SELECTED | LVIS_FOCUSED;
        ListView_SetItemState(g_hListView, i, State, State);
        ListView_EnsureVisible(g_hListView, i, FALSE);
    }
}

BOOL MainWnd_OnUpdateRegistry(HWND hwnd)
{
    // open the key
    HKEY hKey = NULL;
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, g_pszKey, 0, KEY_ALL_ACCESS, &hKey);
    if (hKey == NULL)
        return FALSE;

    // clear all values
    WCHAR szName[MAX_STRING], szValue[MAX_STRING];
    DWORD cbName, cbValue;
    for (;;)
    {
        cbName = sizeof(szName);
        cbValue = sizeof(szValue);
        LONG Error = RegEnumValueW(hKey, 0, szName, &cbName,
                                   NULL, NULL, (LPBYTE)szValue, &cbValue);
        if (Error != ERROR_SUCCESS)
            break;

        RegDeleteValueW(hKey, szName);
    }

    // set values
    size_t Count = g_Items.size();
    for (size_t i = 0; i < Count; ++i)
    {
        DWORD cbData = (g_Items[i].m_Substitute.size() + 1) * sizeof(WCHAR);
        RegSetValueExW(hKey, g_Items[i].m_Name.c_str(), 0,
            REG_SZ, (LPBYTE)g_Items[i].m_Substitute.c_str(), cbData);
    }

    // close now
    RegCloseKey(hKey);

    g_bModified = FALSE;
    g_bNeedsReboot = TRUE;
    return TRUE;
}

LPWSTR SkipSpace(LPCWSTR pch)
{
    while (*pch && wcschr(L" \t\r\n", *pch) != NULL)
    {
        ++pch;
    }
    return const_cast<LPWSTR>(pch);
}

LPWSTR SkipQuoted(LPWSTR pch)
{
    ++pch;  // L'"'
    while (*pch)
    {
        if (*pch == L'"')
        {
            ++pch;
            break;
        }
        if (*pch == L'\\')
        {
            ++pch;
        }
        ++pch;
    }
    return pch;
}

void UnescapeHex(const STRING& str, size_t& i, STRING& Ret, BOOL Unicode)
{
    STRING Num;

    // hexadecimal
    if (iswxdigit(str[i]))
    {
        Num += str[i];
        ++i;
        if (iswxdigit(str[i]))
        {
            Num += str[i];
            ++i;
            if (Unicode)
            {
                if (iswxdigit(str[i]))
                {
                    Num += str[i];
                    ++i;
                    if (iswxdigit(str[i]))
                    {
                        Num += str[i];
                        ++i;
                    }
                }
            }
        }
    }
    if (!Num.empty())
    {
        Ret += (WCHAR)wcstoul(&Num[0], NULL, 16);
    }
}

void UnescapeOther(const STRING& str, size_t& i, STRING& Ret)
{
    STRING Num;

    // check octal
    if (L'0' <= str[i] && str[i] < L'8')
    {
        Num += str[i];
        ++i;
        if (L'0' <= str[i] && str[i] < L'8')
        {
            Num += str[i];
            ++i;
            if (L'0' <= str[i] && str[i] < L'8')
            {
                Num += str[i];
                ++i;
            }
        }
    }
    if (Num.empty())
    {
        Ret += str[i];
        ++i;
    }
    else
    {
        // octal
        Ret += (WCHAR)wcstoul(&Num[0], NULL, 8);
    }
}

// process escape sequence
void UnescapeChar(const STRING& str, size_t& i, STRING& Ret)
{
    if (str[i] != L'\\')
    {
        Ret += str[i];
        ++i;
        return;
    }

    ++i;
    switch (str[i])
    {
        case L'a': Ret += L'\a'; ++i; break;
        case L'b': Ret += L'\b'; ++i; break;
        case L'f': Ret += L'\f'; ++i; break;
        case L'n': Ret += L'\n'; ++i; break;
        case L'r': Ret += L'\r'; ++i; break;
        case L't': Ret += L'\t'; ++i; break;
        case L'v': Ret += L'\v'; ++i; break;
        case L'x':
            // hexidemical
            ++i;
            UnescapeHex(str, i, Ret, FALSE);
            break;
        case L'u':
            // Unicode hexidemical
            ++i;
            UnescapeHex(str, i, Ret, TRUE);
            break;
        default:
            // other case
            UnescapeOther(str, i, Ret);
            break;
    }
}

STRING Unquote(const STRING& str)
{
    if (str[0] != L'"')
        return str;

    STRING Ret;
    size_t i = 1;
    while (i < str.size())
    {
        if (str[i] == L'"' || str[i] == UNICODE_NULL)
            break;

        UnescapeChar(str, i, Ret);
    }
    return Ret;
}

BOOL DoParseFile(LPVOID pvContents, DWORD dwSize)
{
    ITEMVECTOR  Items;

    LPWSTR pch, pchSep, pchStart = (LPWSTR)pvContents;

    pchStart[dwSize / sizeof(WCHAR)] = UNICODE_NULL;

    // check header
    const DWORD cbHeader = lstrlenW(g_pszFileHeader) * sizeof(WCHAR);
    if (memcmp(pchStart, g_pszFileHeader, cbHeader) != 0)
        return FALSE;

    pchStart += cbHeader / sizeof(WCHAR);

    // find the key
    WCHAR szKey[MAX_STRING];
    wsprintfW(szKey, L"[HKEY_LOCAL_MACHINE\\%s]", g_pszKey);
    pch = wcsstr(pchStart, szKey);
    if (pch == NULL)
        return FALSE;

    pchStart = pch + lstrlenW(szKey);

    for (;;)
    {
        pchStart = SkipSpace(pchStart);
        if (*pchStart == UNICODE_NULL || *pchStart == L'[')
            break;

        pch = wcschr(pchStart, L'\n');
        if (pch)
            *pch = UNICODE_NULL;

        pchSep = SkipQuoted(pchStart);
        if (*pchSep == L'=')
        {
            *pchSep = UNICODE_NULL;

            STRING key = pchStart;
            trim(key);
            key = Unquote(key);

            STRING value = pchSep + 1;
            trim(value);
            value = Unquote(value);

            BYTE CharSet1 = DEFAULT_CHARSET, CharSet2 = DEFAULT_CHARSET;

            size_t pos;
            pos = key.find(L',');
            if (pos != STRING::npos)
            {
                CharSet1 = (BYTE)_wtoi(&key[pos + 1]);
                key.resize(pos);
                trim(key);
            }
            pos = value.find(L',');
            if (pos != STRING::npos)
            {
                CharSet2 = (BYTE)_wtoi(&value[pos + 1]);
                value.resize(pos);
                trim(value);
            }

            ITEM Item(key, value, CharSet1, CharSet2);
            Items.push_back(Item);
        }

        if (pch == NULL)
            break;

        pchStart = pch + 1;
    }

    g_Items = Items;
    g_bModified = TRUE;

    LV_AddItems(g_hListView);
    return TRUE;
}

BOOL DoImport(HWND hwnd, LPCWSTR pszFile)
{
    HANDLE hFile = CreateFileW(pszFile, GENERIC_READ,
        FILE_SHARE_READ | FILE_SHARE_DELETE, NULL,
        OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bSuccess = FALSE;
    DWORD dwSize = GetFileSize(hFile, NULL);
    if (dwSize != 0xFFFFFFFF)
    {
        std::vector<BYTE> Contents(dwSize + 2);
        DWORD cbRead;
        if (ReadFile(hFile, &Contents[0], dwSize, &cbRead, NULL) &&
            cbRead == dwSize)
        {
            /* check BOM */
            if (memcmp(&Contents[0], "\xFF\xFE", 2) == 0)
            {
                bSuccess = DoParseFile(&Contents[2], dwSize - 2);
            }
            else
            {
                bSuccess = DoParseFile(&Contents[0], dwSize);
            }
        }
    }
    CloseHandle(hFile);

    return bSuccess;
}

STRING Escape(const STRING& str)
{
    STRING Ret;
    for (size_t i = 0; i < str.size(); ++i)
    {
        switch (str[i])
        {
            case L'"': case L'\\':
                Ret += L'\\';
                Ret += str[i];
                break;
            default:
                Ret += str[i];
        }
    }
    return Ret;
}

BOOL DoExport(HWND hwnd, LPCWSTR pszFile)
{
    HANDLE hFile = CreateFileW(pszFile, GENERIC_WRITE, FILE_SHARE_READ,
        NULL, CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL | FILE_FLAG_WRITE_THROUGH, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
        return FALSE;

    BOOL bSuccess;
    DWORD dwSize, cbWritten;
    WCHAR szCharSet1[MAX_STRING], szCharSet2[MAX_STRING];
    WCHAR szLine[MAX_STRING * 2 + 4];

    /* write header */
    dwSize = lstrlenW(g_pszFileHeader) * sizeof(WCHAR);
    bSuccess =
        WriteFile(hFile, "\xFF\xFE", 2, &cbWritten, NULL) &&
        WriteFile(hFile, g_pszFileHeader, dwSize, &cbWritten, NULL);
    if (bSuccess)
    {
        wsprintfW(szLine, L"\r\n\r\n[HKEY_LOCAL_MACHINE\\%s]\r\n", g_pszKey);
        dwSize = lstrlenW(szLine) * sizeof(WCHAR);
        bSuccess = WriteFile(hFile, szLine, dwSize, &cbWritten, NULL);
    }
    if (bSuccess)
    {
        size_t i, Count = g_Items.size();
        for (i = 0; i < Count; ++i)
        {
            if (g_Items[i].m_CharSet1 != DEFAULT_CHARSET)
                wsprintfW(szCharSet1, L",%u", g_Items[i].m_CharSet1);
            else
                szCharSet1[0] = UNICODE_NULL;

            if (g_Items[i].m_CharSet2 != DEFAULT_CHARSET)
                wsprintfW(szCharSet2, L",%u", g_Items[i].m_CharSet2);
            else
                szCharSet2[0] = UNICODE_NULL;

            STRING Name = Escape(g_Items[i].m_Name);
            STRING Substitute = Escape(g_Items[i].m_Substitute);
            wsprintfW(szLine, L"\"%s%s\"=\"%s%s\"\r\n",
                      Name.c_str(), szCharSet1,
                      Substitute.c_str(), szCharSet2);

            dwSize = lstrlenW(szLine) * sizeof(WCHAR);
            if (!WriteFile(hFile, szLine, dwSize, &cbWritten, NULL))
            {
                bSuccess = FALSE;
                break;
            }
        }
        WriteFile(hFile, L"\r\n", 2 * sizeof(WCHAR), &cbWritten, NULL);
    }
    CloseHandle(hFile);

    if (!bSuccess)
    {
        DeleteFileW(pszFile);
    }

    return bSuccess;
}

void MakeFilter(LPWSTR pszFilter)
{
    while (*pszFilter)
    {
        if (*pszFilter == L'|')
            *pszFilter = 0;

        ++pszFilter;
    }
}

void MainWnd_OnImport(HWND hwnd)
{
    OPENFILENAMEW ofn = {0};
    WCHAR szFile[MAX_PATH] = L"";
    WCHAR szImportTitle[MAX_STRING];
    WCHAR szCannotImport[MAX_STRING];
    WCHAR szImportFilter[MAX_STRING];
    LoadStringW(g_hInstance, IDS_IMPORT, szImportTitle, _countof(szImportTitle));
    LoadStringW(g_hInstance, IDS_CANTIMPORT, szCannotImport, _countof(szCannotImport));
    LoadStringW(g_hInstance, IDS_INPFILTER, szImportFilter, _countof(szImportFilter));
    MakeFilter(szImportFilter);

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szImportFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = szImportTitle;
    ofn.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING |
                OFN_EXPLORER | OFN_FILEMUSTEXIST |
                OFN_HIDEREADONLY | OFN_LONGNAMES |
                OFN_PATHMUSTEXIST;
    ofn.lpstrDefExt = L"reg";
    if (GetOpenFileNameW(&ofn))
    {
        if (!DoImport(hwnd, szFile))
        {
            MessageBoxW(hwnd, szCannotImport, g_szTitle, MB_ICONERROR);
        }
    }
}

void MainWnd_OnExport(HWND hwnd)
{
    OPENFILENAMEW ofn = {0};
    WCHAR szFile[MAX_PATH] = L"";
    WCHAR szExportTitle[MAX_STRING];
    WCHAR szCannotExport[MAX_STRING];
    WCHAR szExportFilter[MAX_STRING];
    LoadStringW(g_hInstance, IDS_EXPORT, szExportTitle, _countof(szExportTitle));
    LoadStringW(g_hInstance, IDS_CANTEXPORT, szCannotExport, _countof(szCannotExport));
    LoadStringW(g_hInstance, IDS_OUTFILTER, szExportFilter, _countof(szExportFilter));
    MakeFilter(szExportFilter);

    ofn.lStructSize = OPENFILENAME_SIZE_VERSION_400;
    ofn.hwndOwner = hwnd;
    ofn.lpstrFilter = szExportFilter;
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = _countof(szFile);
    ofn.lpstrTitle = szExportTitle;
    ofn.Flags = OFN_DONTADDTORECENT | OFN_ENABLESIZING |
                OFN_EXPLORER | OFN_HIDEREADONLY | OFN_LONGNAMES |
                OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;
    ofn.lpstrDefExt = L"reg";
    if (GetSaveFileNameW(&ofn))
    {
        if (!DoExport(hwnd, szFile))
        {
            MessageBoxW(hwnd, szCannotExport, g_szTitle, MB_ICONERROR);
        }
    }
}

void MainWnd_OnReload(HWND hwnd)
{
    DoLoad();
}

void MainWnd_OnEdit(HWND hwnd)
{
    LV_OnDblClk(g_hListView);
}

void MainWnd_OnDelete(HWND hwnd)
{
    LV_OnDelete(g_hListView);
}

void MainWnd_OnOpenRegKey(HWND hwnd)
{
    static const WCHAR s_szRegeditKey[] =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Applets\\Regedit";
    WCHAR sz[MAX_STRING];

    // open regedit key
    HKEY hKey = NULL;
    LSTATUS Result = RegCreateKeyExW(HKEY_CURRENT_USER, s_szRegeditKey, 0,
                                     NULL, 0, KEY_WRITE, NULL, &hKey, NULL);
    if (Result != ERROR_SUCCESS)
    {
        LoadStringW(g_hInstance, IDS_CANTOPENKEY, sz, _countof(sz));
        MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
        return;
    }

    // set LastKey value
    wsprintfW(sz, L"HKEY_LOCAL_MACHINE\\%s", g_pszKey);
    DWORD dwSize = sizeof(sz);
    Result = RegSetValueExW(hKey, L"LastKey", 0, REG_SZ,
                                 (LPBYTE)sz, dwSize);

    // close now
    RegCloseKey(hKey);

    if (Result != ERROR_SUCCESS)
    {
        LoadStringW(g_hInstance, IDS_CANTOPENKEY, sz, _countof(sz));
        MessageBoxW(hwnd, sz, NULL, MB_ICONERROR);
        return;
    }

    // open by regedit
    ShellExecuteW(hwnd, NULL, L"regedit.exe", NULL, NULL, SW_SHOWNORMAL);
}

void MainWnd_OnAbout(HWND hwnd)
{
    WCHAR szAbout[MAX_PATH];
    LoadStringW(g_hInstance, IDS_ABOUT, szAbout, _countof(szAbout));

    MSGBOXPARAMS Params;
    ZeroMemory(&Params, sizeof(Params));
    Params.cbSize = sizeof(Params);
    Params.hwndOwner = hwnd;
    Params.hInstance = g_hInstance;
    Params.lpszText = szAbout;
    Params.lpszCaption = g_szTitle;
    Params.dwStyle = MB_OK | MB_USERICON;
    Params.lpszIcon = MAKEINTRESOURCEW(1);
    Params.dwLanguageId = LANG_USER_DEFAULT;
    MessageBoxIndirectW(&Params);
}

void MainWnd_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    switch (id)
    {
        case ID_NEW:
            MainWnd_OnNew(hwnd);
            break;
        case ID_EDIT:
            MainWnd_OnEdit(hwnd);
            break;
        case ID_EXIT:
            PostMessage(hwnd, WM_CLOSE, 0, 0);
            break;
        case ID_RELOAD:
            MainWnd_OnReload(hwnd);
            break;
        case ID_UPDATE_REGISTRY:
            MainWnd_OnUpdateRegistry(hwnd);
            break;
        case ID_DELETE:
            MainWnd_OnDelete(hwnd);
            break;
        case ID_IMPORT:
            MainWnd_OnImport(hwnd);
            break;
        case ID_EXPORT:
            MainWnd_OnExport(hwnd);
            break;
        case ID_OPEN_REGKEY:
            MainWnd_OnOpenRegKey(hwnd);
            break;
        case ID_ABOUT:
            MainWnd_OnAbout(hwnd);
            break;
    }
}

void MainWnd_OnDestroy(HWND hwnd)
{
    PostQuitMessage(0);
}

void MainWnd_OnSize(HWND hwnd, UINT state, int cx, int cy)
{
    MoveWindow(g_hListView, 0, 0, cx, cy, TRUE);
}

void MainWnd_OnDrawItem(HWND hwnd, const DRAWITEMSTRUCT *lpDrawItem)
{
    if (lpDrawItem->CtlType != ODT_LISTVIEW)
        return;

    HDC hDC = lpDrawItem->hDC;
    SetBkMode(hDC, TRANSPARENT);

    INT iColumn = 0, x, cx;
    RECT rcItem, rcSubItem, rcText;
    STRING Str;

    x = -GetScrollPos(g_hListView, SB_HORZ);

    rcItem = lpDrawItem->rcItem;
    if (lpDrawItem->itemState & ODS_SELECTED)
    {
        FillRect(hDC, &rcItem, (HBRUSH)(COLOR_HIGHLIGHT + 1));
        SetTextColor(hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
    }
    else
    {
        FillRect(hDC, &rcItem, (HBRUSH)(COLOR_WINDOW + 1));
        SetTextColor(hDC, GetSysColor(COLOR_WINDOWTEXT));
    }

    cx = ListView_GetColumnWidth(g_hListView, iColumn);
    rcSubItem = rcItem;
    rcSubItem.left = x;
    rcSubItem.right = x + cx;

    WCHAR sz[MAX_STRING];

    rcText = rcSubItem;
    InflateRect(&rcText, -1, -1);
    Str = g_Items[lpDrawItem->itemID].m_Name;
    BYTE CharSet1 = g_Items[lpDrawItem->itemID].m_CharSet1;
    if (CharSet1 != DEFAULT_CHARSET)
        wsprintfW(sz, L"%s,%u", Str.c_str(), CharSet1);
    else
        wsprintfW(sz, L"%s", Str.c_str());

    DrawTextW(hDC, sz, lstrlenW(sz), &rcText,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS |
              DT_NOPREFIX);

    x += cx;
    ++iColumn;

    cx = ListView_GetColumnWidth(g_hListView, iColumn);
    rcSubItem = rcItem;
    rcSubItem.left = x;
    rcSubItem.right = x + cx;

    rcText = rcSubItem;
    InflateRect(&rcText, -1, -1);
    Str = g_Items[lpDrawItem->itemID].m_Substitute;
    BYTE CharSet2 = g_Items[lpDrawItem->itemID].m_CharSet2;
    if (CharSet2 != DEFAULT_CHARSET)
        wsprintfW(sz, L"%s,%u", Str.c_str(), CharSet2);
    else
        wsprintfW(sz, L"%s", Str.c_str());

    DrawTextW(hDC, sz, lstrlenW(sz), &rcText,
              DT_SINGLELINE | DT_LEFT | DT_VCENTER | DT_END_ELLIPSIS |
              DT_NOPREFIX);
}

void MainWnd_OnMeasureItem(HWND hwnd, MEASUREITEMSTRUCT *lpMeasureItem)
{
    if (lpMeasureItem->CtlType != ODT_LISTVIEW)
        return;

    TEXTMETRIC tm;
    HDC hDC = GetDC(hwnd);
    GetTextMetrics(hDC, &tm);
    ReleaseDC(hwnd, hDC);

    lpMeasureItem->itemHeight = tm.tmHeight * 4 / 3;
}

LRESULT MainWnd_OnNotify(HWND hwnd, int idFrom, NMHDR *pnmhdr)
{
    NM_LISTVIEW *pNMLV = (NM_LISTVIEW *)pnmhdr;
    LV_KEYDOWN *pLVKD = (LV_KEYDOWN *)pnmhdr;

    switch (pnmhdr->code)
    {
        case LVN_COLUMNCLICK:
            if (pNMLV->iSubItem == g_iSortColumn)
                DoSort(pNMLV->iSubItem, !g_bSortAscendant);
            else
                DoSort(pNMLV->iSubItem, TRUE);
            break;
        case NM_DBLCLK:
            LV_OnDblClk(g_hListView);
            break;
        case LVN_KEYDOWN:
            if (pLVKD->wVKey == VK_RETURN)  // [Enter] key
            {
                LV_OnDblClk(g_hListView);
            }
            if (pLVKD->wVKey == VK_DELETE)  // [Del] key
            {
                LV_OnDelete(g_hListView);
            }
            break;
    }
    return 0;
}

LRESULT MainWnd_OnContextMenu(HWND hwnd, HWND hwndContext, UINT xPos, UINT yPos)
{
    POINT pt = {(INT)xPos, (INT)yPos};
    ScreenToClient(g_hListView, &pt);
    SendMessageW(g_hListView, WM_LBUTTONDOWN, 0, MAKELPARAM(pt.x, pt.y));

    HMENU hMenu = LoadMenu(g_hInstance, MAKEINTRESOURCE(2));
    if (hMenu == NULL)
        return 0;

    HMENU hSubMenu = GetSubMenu(hMenu, 0);
    if (hSubMenu == NULL)
        return 0;

    SetForegroundWindow(hwnd);
    TrackPopupMenu(hSubMenu, TPM_LEFTALIGN | TPM_RIGHTBUTTON,
                   xPos, yPos, 0, g_hMainWnd, NULL);
    PostMessage(g_hMainWnd, WM_NULL, 0, 0);
    return 0;
}

void MainWnd_OnActivate(HWND hwnd, UINT state, HWND hwndActDeact, BOOL fMinimized)
{
    if (state != WA_INACTIVE)
    {
        SetFocus(g_hListView);
    }
}

BOOL EnableProcessPrivileges(LPCWSTR lpPrivilegeName, BOOL bEnable = TRUE)
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tokenPrivileges;
    BOOL Ret;

    Ret = ::OpenProcessToken(::GetCurrentProcess(),
                             TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                             &hToken);
    if (!Ret)
        return Ret;     // failure

    Ret = ::LookupPrivilegeValueW(NULL, lpPrivilegeName, &luid);
    if (Ret)
    {
        tokenPrivileges.PrivilegeCount = 1;
        tokenPrivileges.Privileges[0].Luid = luid;
        tokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

        Ret = ::AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, 0, 0, 0);
    }

    ::CloseHandle(hToken);
    return Ret;
}

void MainWnd_OnClose(HWND hwnd)
{
    if (!g_bNeedsReboot && !g_bModified)
    {
        DestroyWindow(hwnd);
        return;
    }

    if (g_bModified)
    {
        WCHAR szUpdateNow[MAX_STRING];
        LoadStringW(g_hInstance, IDS_QUERYUPDATE, szUpdateNow, _countof(szUpdateNow));
        INT id = MessageBoxW(hwnd, szUpdateNow, g_szTitle,
                             MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (id)
        {
        case IDYES:
            MainWnd_OnUpdateRegistry(hwnd);
            break;
        case IDNO:
            break;
        case IDCANCEL:
            return;
        }
    }

    if (g_bNeedsReboot)
    {
        WCHAR szRebootNow[MAX_STRING];
        LoadStringW(g_hInstance, IDS_REBOOTNOW, szRebootNow, _countof(szRebootNow));
        INT id = MessageBoxW(hwnd, szRebootNow, g_szTitle,
                             MB_ICONINFORMATION | MB_YESNOCANCEL);
        switch (id)
        {
        case IDYES:
            EnableProcessPrivileges(SE_SHUTDOWN_NAME, TRUE);
            ::ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
            break;
        case IDNO:
            break;
        case IDCANCEL:
            return;
        }
    }

    ::DestroyWindow(hwnd);
}

LRESULT CALLBACK
WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        HANDLE_MSG(hwnd, WM_CREATE, MainWnd_OnCreate);
        HANDLE_MSG(hwnd, WM_COMMAND, MainWnd_OnCommand);
        HANDLE_MSG(hwnd, WM_DESTROY, MainWnd_OnDestroy);
        HANDLE_MSG(hwnd, WM_SIZE, MainWnd_OnSize);
        HANDLE_MSG(hwnd, WM_DRAWITEM, MainWnd_OnDrawItem);
        HANDLE_MSG(hwnd, WM_MEASUREITEM, MainWnd_OnMeasureItem);
        HANDLE_MSG(hwnd, WM_NOTIFY, MainWnd_OnNotify);
        HANDLE_MSG(hwnd, WM_CONTEXTMENU, MainWnd_OnContextMenu);
        HANDLE_MSG(hwnd, WM_ACTIVATE, MainWnd_OnActivate);
        HANDLE_MSG(hwnd, WM_CLOSE, MainWnd_OnClose);
        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
}

INT WINAPI wWinMain(
    HINSTANCE   hInstance,
    HINSTANCE   hPrevInstance,
    LPWSTR       lpCmdLine,
    INT         nCmdShow)
{
    g_hInstance = hInstance;
    InitCommonControls();

    HACCEL hAccel = LoadAcceleratorsW(hInstance, MAKEINTRESOURCEW(1));

    LoadStringW(hInstance, IDS_TITLE, g_szTitle, _countof(g_szTitle));
    LoadStringW(hInstance, IDS_FONTNAME, g_szNameHead, _countof(g_szNameHead));
    LoadStringW(hInstance, IDS_SUBSTITUTE, g_szSubstituteHead, _countof(g_szSubstituteHead));

    WNDCLASSW wc = {0};
    wc.style = 0;
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    g_hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(1));
    wc.hIcon = g_hIcon;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_3DFACE + 1);
    wc.lpszMenuName = MAKEINTRESOURCEW(1);
    wc.lpszClassName = g_pszClassName;
    if (!RegisterClassW(&wc))
    {
        MessageBoxA(NULL, "ERROR: RegisterClass failed.", NULL, MB_ICONERROR);
        return 1;
    }

    const DWORD dwStyle = WS_OVERLAPPEDWINDOW;
    INT Width = NAME_COLUMN_WIDTH + SUB_COLUMN_WIDTH +
                GetSystemMetrics(SM_CXVSCROLL) +
                GetSystemMetrics(SM_CXSIZEFRAME);
    INT Height = 320;

    RECT Rect = { 0, 0, Width, Height };
    AdjustWindowRect(&Rect, dwStyle, TRUE);
    Width = Rect.right - Rect.left;
    Height = Rect.bottom - Rect.top;

    g_hMainWnd = CreateWindowW(g_pszClassName, g_szTitle, dwStyle,
        CW_USEDEFAULT, CW_USEDEFAULT, Width, Height,
        NULL, NULL, hInstance, NULL);
    if (g_hMainWnd == NULL)
    {
        MessageBoxA(NULL, "ERROR: CreateWindow failed.", NULL, MB_ICONERROR);
        return 2;
    }

    ShowWindow(g_hMainWnd, nCmdShow);
    UpdateWindow(g_hMainWnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (TranslateAccelerator(g_hMainWnd, hAccel, &msg))
            continue;

        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return (INT)msg.wParam;
}
