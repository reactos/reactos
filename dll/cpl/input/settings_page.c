/*
 * PROJECT:         input.dll
 * FILE:            dll/cpl/input/settings_page.c
 * PURPOSE:         input.dll
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 *                  Katayama Hirofumi MZ (katayama.hirofumi.mz@gmail.com)
 */

#include "input.h"
#include "layout_list.h"
#include "locale_list.h"
#include "input_list.h"
#include <shellapi.h>

static INT s_nLeavesCount = 0;
static INT s_iKeyboardImage = -1;
static INT s_iDotImage = -1;

static HICON
CreateLayoutIcon(LANGID LangID)
{
    WCHAR szBuf[4];
    HDC hdc;
    HBITMAP hbmColor, hbmMono, hBmpOld;
    RECT rect;
    HFONT hFontOld, hFont;
    ICONINFO IconInfo;
    HICON hIcon;
    LOGFONTW lf;
    BITMAPINFO bmi;
    INT cxIcon = GetSystemMetrics(SM_CXSMICON);
    INT cyIcon = GetSystemMetrics(SM_CYSMICON);

    /* Getting "EN", "FR", etc. from English, French, ... */
    if (!GetLocaleInfoW(LangID, LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                        szBuf, ARRAYSIZE(szBuf)))
    {
        StringCchCopyW(szBuf, ARRAYSIZE(szBuf), L"??");
    }
    szBuf[2] = UNICODE_NULL; /* Truncate the identifiers to two characters: "ENG" --> "EN" etc. */

    /* Prepare for DIB (device-independent bitmap) */
    ZeroMemory(&bmi, sizeof(bmi));
    bmi.bmiHeader.biSize = sizeof(bmi.bmiHeader);
    bmi.bmiHeader.biWidth = cxIcon;
    bmi.bmiHeader.biHeight = cyIcon;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 24;

    /* Create hdc, hbmColor and hbmMono */
    hdc = CreateCompatibleDC(NULL);
    hbmColor = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, NULL, NULL, 0);
    hbmMono = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL);

    /* Create a font */
    if (SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
        hFont = CreateFontIndirectW(&lf);
    else
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

    /* Checking NULL */
    if (!hdc || !hbmColor || !hbmMono || !hFont)
    {
        if (hdc)
            DeleteDC(hdc);
        if (hbmColor)
            DeleteObject(hbmColor);
        if (hbmMono)
            DeleteObject(hbmMono);
        if (hFont)
            DeleteObject(hFont);
        return NULL;
    }

    SetRect(&rect, 0, 0, cxIcon, cyIcon);

    /* Draw hbmColor */
    hBmpOld = SelectObject(hdc, hbmColor);
    SetDCBrushColor(hdc, GetSysColor(COLOR_HIGHLIGHT));
    FillRect(hdc, &rect, (HBRUSH)GetStockObject(DC_BRUSH));
    hFontOld = SelectObject(hdc, hFont);
    SetTextColor(hdc, GetSysColor(COLOR_HIGHLIGHTTEXT));
    SetBkMode(hdc, TRANSPARENT);
    DrawTextW(hdc, szBuf, 2, &rect, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
    SelectObject(hdc, hFontOld);
    SelectObject(hdc, hBmpOld);

    /* Fill hbmMono by black */
    hBmpOld = SelectObject(hdc, hbmMono);
    PatBlt(hdc, 0, 0, cxIcon, cyIcon, BLACKNESS);
    SelectObject(hdc, hBmpOld);

    /* Create an icon from hbmColor and hbmMono */
    IconInfo.hbmColor = hbmColor;
    IconInfo.hbmMask = hbmMono;
    IconInfo.fIcon = TRUE;
    hIcon = CreateIconIndirect(&IconInfo);

    /* Clean up */
    DeleteObject(hbmColor);
    DeleteObject(hbmMono);
    DeleteObject(hFont);
    DeleteDC(hdc);

    return hIcon;
}

static VOID InitDefaultLangComboBox(HWND hwndCombo)
{
    WCHAR szText[256];
    INPUT_LIST_NODE *pNode;
    INT iIndex, iDefault = (INT)SendMessageW(hwndCombo, CB_GETCURSEL, 0, 0);

    SendMessageW(hwndCombo, CB_RESETCONTENT, 0, 0);

    for (pNode = InputList_GetFirst(); pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        StringCchPrintfW(szText, _countof(szText), L"%s - %s",
                         pNode->pLocale->pszName, pNode->pLayout->pszName);
        iIndex = (INT)SendMessageW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)szText);
        SendMessageW(hwndCombo, CB_SETITEMDATA, iIndex, (LPARAM)pNode);

        if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT)
            iDefault = iIndex;
    }

    if (iDefault == (INT)SendMessageW(hwndCombo, CB_GETCOUNT, 0, 0))
        SendMessageW(hwndCombo, CB_SETCURSEL, iDefault - 1, 0);
    else
        SendMessageW(hwndCombo, CB_SETCURSEL, iDefault, 0);
}

static VOID
SetControlsState(HWND hwndDlg)
{
    HWND hwndList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);
    HWND hwndCombo = GetDlgItem(hwndDlg, IDC_DEFAULT_LANGUAGE);
    BOOL bIsLeaf, bCanRemove, bCanProp;
    HTREEITEM hSelected = TreeView_GetSelection(hwndList);
    TV_ITEM item = { TVIF_PARAM };
    item.hItem = hSelected;

    bIsLeaf = (hSelected && TreeView_GetItem(hwndList, &item) && HIWORD(item.lParam));

    bCanRemove = bIsLeaf && (s_nLeavesCount > 1);
    bCanProp = bIsLeaf;

    EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE_BUTTON), bCanRemove);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PROP_BUTTON), bCanProp);

    InitDefaultLangComboBox(hwndCombo);
}

static HICON LoadIMEIcon(LPCTSTR pszImeFile)
{
    WCHAR szSysDir[MAX_PATH], szPath[MAX_PATH];
    HICON hIconSm = NULL;

    GetSystemDirectoryW(szSysDir, _countof(szSysDir));
    StringCchPrintfW(szPath, _countof(szPath), L"%s\\%s", szSysDir, pszImeFile);

    ExtractIconExW(szPath, 0, NULL, &hIconSm, 1);
    return hIconSm;
}

HTREEITEM FindLanguageInList(HWND hwndList, LPCTSTR pszLangName)
{
    TV_ITEM item;
    TCHAR szText[128];
    HTREEITEM hItem;

    hItem = TreeView_GetRoot(hwndList);
    while (hItem)
    {
        szText[0] = 0;
        item.mask       = TVIF_TEXT | TVIF_HANDLE;
        item.pszText    = szText;
        item.cchTextMax = _countof(szText);
        item.hItem      = hItem;
        TreeView_GetItem(hwndList, &item);
        if (lstrcmpi(szText, pszLangName) == 0)
            return hItem;

        hItem = TreeView_GetNextSibling(hwndList, hItem);
    }

    return NULL;
}

static VOID
AddToInputListView(HWND hwndList, INPUT_LIST_NODE *pInputNode)
{
    TV_ITEM item;
    TV_INSERTSTRUCT insert;
    HIMAGELIST hImageList = TreeView_GetImageList(hwndList, TVSIL_NORMAL);
    WCHAR szKeyboard[64];
    HTREEITEM hItem;
    BOOL bBold = !!(pInputNode->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT);

    if (s_iKeyboardImage == -1)
    {
        // Keyboard icon
        HICON hKeyboardIcon = (HICON)LoadImageW(hApplet,
                                                MAKEINTRESOURCEW(IDI_KEYBOARD),
                                                IMAGE_ICON,
                                                GetSystemMetrics(SM_CXSMICON),
                                                GetSystemMetrics(SM_CYSMICON),
                                                0);
        if (hKeyboardIcon)
        {
            s_iKeyboardImage = ImageList_AddIcon(hImageList, hKeyboardIcon);
            DestroyIcon(hKeyboardIcon);
        }
    }

    if (s_iDotImage == -1)
    {
        // Dot icon
        HICON hDotIcon = (HICON)LoadImageW(hApplet,
                                           MAKEINTRESOURCEW(IDI_DOT),
                                           IMAGE_ICON,
                                           GetSystemMetrics(SM_CXSMICON),
                                           GetSystemMetrics(SM_CYSMICON),
                                           0);
        if (hDotIcon)
        {
            s_iDotImage = ImageList_AddIcon(hImageList, hDotIcon);
            DestroyIcon(hDotIcon);
        }
    }

    hItem = FindLanguageInList(hwndList, pInputNode->pLocale->pszName);
    if (hItem == NULL)
    {
        // Language icon
        INT LangImageIndex = -1;
        HICON hLangIcon = CreateLayoutIcon(LOWORD(pInputNode->pLocale->dwId));
        if (hLangIcon)
        {
            LangImageIndex = ImageList_AddIcon(hImageList, hLangIcon);
            DestroyIcon(hLangIcon);
        }

        // Language
        ZeroMemory(&item, sizeof(item));
        item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
        item.pszText        = pInputNode->pLocale->pszName;
        item.iImage         = LangImageIndex;
        item.iSelectedImage = LangImageIndex;
        item.lParam         = LOWORD(pInputNode->pLocale->dwId);
        if (bBold)
        {
            item.state = item.stateMask = TVIS_BOLD;
        }
        insert.hParent      = TVI_ROOT;
        insert.hInsertAfter = TVI_LAST;
        insert.item         = item;
        hItem = TreeView_InsertItem(hwndList, &insert);

        // Keyboard
        LoadStringW(hApplet, IDS_KEYBOARD, szKeyboard, _countof(szKeyboard));
        ZeroMemory(&item, sizeof(item));
        item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
        item.pszText        = szKeyboard;
        item.iImage         = s_iKeyboardImage;
        item.iSelectedImage = s_iKeyboardImage;
        item.lParam         = 0xFFFF;
        if (bBold)
        {
            item.state = item.stateMask = TVIS_BOLD;
        }
        insert.hParent      = hItem;
        insert.hInsertAfter = TVI_LAST;
        insert.item         = item;
        hItem = TreeView_InsertItem(hwndList, &insert);
    }
    else
    {
        // Language
        ZeroMemory(&item, sizeof(item));
        item.mask           = TVIF_STATE | TVIF_HANDLE;
        item.hItem          = hItem;
        item.stateMask      = TVIS_BOLD;
        if (TreeView_GetItem(hwndList, &item) && bBold && !(item.state & TVIS_BOLD))
        {
            // Make the item bold
            item.mask = TVIF_STATE | TVIF_HANDLE;
            item.hItem = hItem;
            item.state = item.stateMask = TVIS_BOLD;
            TreeView_SetItem(hwndList, &item);
        }

        // Keyboard
        hItem = TreeView_GetChild(hwndList, hItem);
        ZeroMemory(&item, sizeof(item));
        item.mask           = TVIF_STATE | TVIF_HANDLE;
        item.hItem          = hItem;
        item.stateMask      = TVIS_BOLD;
        if (TreeView_GetItem(hwndList, &item) && bBold && !(item.state & TVIS_BOLD))
        {
            // Make the item bold
            item.mask = TVIF_STATE | TVIF_HANDLE;
            item.hItem = hItem;
            item.state = item.stateMask = TVIS_BOLD;
            TreeView_SetItem(hwndList, &item);
        }
    }

    // Input method
    if (hItem)
    {
        INT ImeImageIndex = s_iDotImage;
        if (IS_IME_HKL(pInputNode->hkl) && pInputNode->pLayout->pszImeFile)
        {
            HICON hImeIcon = LoadIMEIcon(pInputNode->pLayout->pszImeFile);
            if (hImeIcon)
            {
                ImeImageIndex = ImageList_AddIcon(hImageList, hImeIcon);
                DestroyIcon(hImeIcon);
            }
        }

        ZeroMemory(&item, sizeof(item));
        item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE | TVIF_STATE;
        item.pszText        = pInputNode->pLayout->pszName;
        item.iImage         = ImeImageIndex;
        item.iSelectedImage = ImeImageIndex;
        item.lParam         = (LPARAM)pInputNode;
        if (bBold)
        {
            item.state = item.stateMask = TVIS_BOLD;
        }
        insert.hParent      = hItem;
        insert.hInsertAfter = TVI_LAST;
        insert.item         = item;
        TreeView_InsertItem(hwndList, &insert);
    }
}

static VOID ExpandTreeItem(HWND hwndTree, HTREEITEM hItem)
{
    TreeView_Expand(hwndTree, hItem, TVE_EXPAND);
    hItem = TreeView_GetChild(hwndTree, hItem);
    while (hItem)
    {
        ExpandTreeItem(hwndTree, hItem);
        hItem = TreeView_GetNextSibling(hwndTree, hItem);
    }
}

static VOID
UpdateInputListView(HWND hwndList)
{
    INPUT_LIST_NODE *pNode;
    HIMAGELIST hImageList = TreeView_GetImageList(hwndList, TVSIL_NORMAL);
    HTREEITEM hItem;

    if (hImageList)
        ImageList_RemoveAll(hImageList);

    TreeView_DeleteAllItems(hwndList);
    s_nLeavesCount = 0;
    s_iDotImage = s_iKeyboardImage = -1;

    InputList_Sort();

    for (pNode = InputList_GetFirst(); pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        AddToInputListView(hwndList, pNode);
        ++s_nLeavesCount;
    }

    hItem = TreeView_GetRoot(hwndList);
    while (hItem)
    {
        ExpandTreeItem(hwndList, hItem);
        hItem = TreeView_GetNextSibling(hwndList, hItem);
    }

    InvalidateRect(hwndList, NULL, TRUE);
}


static VOID
OnInitSettingsPage(HWND hwndDlg)
{
    HWND hwndInputList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);
    HIMAGELIST hLayoutImageList, hOldImageList;

    LayoutList_Create();
    LocaleList_Create();
    InputList_Create();

    EnableWindow(GetDlgItem(hwndDlg, IDC_LANGUAGE_BAR), FALSE);

    if (hwndInputList != NULL)
    {
        hLayoutImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                            GetSystemMetrics(SM_CYSMICON),
                                            ILC_COLOR8 | ILC_MASK, 0, 0);
        if (hLayoutImageList != NULL)
        {
            hOldImageList = TreeView_SetImageList(hwndInputList, hLayoutImageList, TVSIL_NORMAL);
            ImageList_Destroy(hOldImageList);
        }

        UpdateInputListView(hwndInputList);
    }

    SetControlsState(hwndDlg);
}


static VOID
OnDestroySettingsPage(HWND hwndDlg)
{
    LayoutList_Destroy();
    LocaleList_Destroy();
    InputList_Destroy();
}


VOID
OnCommandSettingsPage(HWND hwndDlg, WPARAM wParam)
{
    switch (LOWORD(wParam))
    {
        case IDC_ADD_BUTTON:
        {
            if (DialogBoxW(hApplet,
                           MAKEINTRESOURCEW(IDD_ADD),
                           hwndDlg,
                           AddDialogProc) == IDOK)
            {
                UpdateInputListView(GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST));
                SetControlsState(hwndDlg);
                PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
            }
        }
        break;

        case IDC_REMOVE_BUTTON:
        {
            HWND hwndList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);
            if (hwndList)
            {
                HTREEITEM hItem = TreeView_GetSelection(hwndList);
                TV_ITEM item = { TVIF_HANDLE | TVIF_PARAM };
                item.hItem = hItem;

                if (hItem && TreeView_GetItem(hwndList, &item))
                {
                    InputList_Remove((INPUT_LIST_NODE*) item.lParam);
                    UpdateInputListView(hwndList);
                    SetControlsState(hwndDlg);
                    PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                }
            }
        }
        break;

        case IDC_PROP_BUTTON:
        {
            HWND hwndList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);
            if (hwndList)
            {
                HTREEITEM hItem = TreeView_GetSelection(hwndList);
                TV_ITEM item = { TVIF_HANDLE | TVIF_PARAM };
                item.hItem = hItem;

                if (hItem && TreeView_GetItem(hwndList, &item))
                {
                    if (DialogBoxParamW(hApplet,
                                        MAKEINTRESOURCEW(IDD_INPUT_LANG_PROP),
                                        hwndDlg,
                                        EditDialogProc,
                                        item.lParam) == IDOK)
                    {
                        UpdateInputListView(hwndList);
                        SetControlsState(hwndDlg);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                }
            }
        }
        break;

        case IDC_KEY_SET_BTN:
        {
            DialogBoxW(hApplet,
                       MAKEINTRESOURCEW(IDD_KEYSETTINGS),
                       hwndDlg,
                       KeySettingsDialogProc);
        }
        break;

        case IDC_LANGUAGE_BAR:
        {
            // FIXME
            break;
        }

        case IDC_DEFAULT_LANGUAGE:
        {
            if (HIWORD(wParam) == CBN_SELENDOK)
            {
                HWND hwndList = GetDlgItem(hwndDlg, IDC_KEYLAYOUT_LIST);
                HWND hwndCombo = GetDlgItem(hwndDlg, IDC_DEFAULT_LANGUAGE);
                INT iSelected = (INT)SendMessageW(hwndCombo, CB_GETCURSEL, 0, 0);
                if (iSelected != CB_ERR)
                {
                    LPARAM lParam = SendMessageW(hwndCombo, CB_GETITEMDATA, iSelected, 0);
                    if (lParam)
                    {
                        InputList_SetDefault((INPUT_LIST_NODE*)lParam);
                        UpdateInputListView(hwndList);
                        SetControlsState(hwndDlg);
                        PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                    }
                }
            }
        }
    }
}

static BOOL IsRebootNeeded(VOID)
{
    INPUT_LIST_NODE *pNode;

    for (pNode = InputList_GetFirst(); pNode != NULL; pNode = pNode->pNext)
    {
        if (IS_IME_HKL(pNode->hkl)) /* IME? */
        {
            return TRUE;
        }
    }

    return FALSE;
}

BOOL EnableProcessPrivileges(LPCWSTR lpPrivilegeName, BOOL bEnable)
{
    HANDLE hToken;
    LUID luid;
    TOKEN_PRIVILEGES tokenPrivileges;
    BOOL Ret;

    Ret = OpenProcessToken(GetCurrentProcess(),
                           TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                           &hToken);
    if (!Ret)
        return Ret;     // failure

    Ret = LookupPrivilegeValueW(NULL, lpPrivilegeName, &luid);
    if (Ret)
    {
        tokenPrivileges.PrivilegeCount = 1;
        tokenPrivileges.Privileges[0].Luid = luid;
        tokenPrivileges.Privileges[0].Attributes = bEnable ? SE_PRIVILEGE_ENABLED : 0;

        Ret = AdjustTokenPrivileges(hToken, FALSE, &tokenPrivileges, 0, 0, 0);
    }

    CloseHandle(hToken);
    return Ret;
}

static INT_PTR
OnNotifySettingsPage(HWND hwndDlg, LPARAM lParam)
{
    LPNMHDR header = (LPNMHDR)lParam;

    switch (header->code)
    {
        case TVN_SELCHANGED:
        {
            SetControlsState(hwndDlg);
        }
        break;

        case TVN_ITEMEXPANDING:
        {
            // FIXME: Prevent collapse
        }
        break;

        case PSN_APPLY:
        {
            BOOL bRebootNeeded = IsRebootNeeded();

            /* Write Input Methods list to registry */
            if (InputList_Process() && bRebootNeeded)
            {
                /* Needs reboot */
                WCHAR szNeedsReboot[128], szLanguage[64];
                LoadStringW(hApplet, IDS_REBOOT_NOW, szNeedsReboot, _countof(szNeedsReboot));
                LoadStringW(hApplet, IDS_LANGUAGE, szLanguage, _countof(szLanguage));

                if (MessageBoxW(hwndDlg, szNeedsReboot, szLanguage,
                                MB_ICONINFORMATION | MB_YESNOCANCEL) == IDYES)
                {
                    EnableProcessPrivileges(SE_SHUTDOWN_NAME, TRUE);
                    ExitWindowsEx(EWX_REBOOT | EWX_FORCE, 0);
                }
            }
        }
        break;
    }

    return 0;
}

INT_PTR CALLBACK
SettingsPageProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            OnInitSettingsPage(hwndDlg);
            break;

        case WM_DESTROY:
            OnDestroySettingsPage(hwndDlg);
            break;

        case WM_COMMAND:
            OnCommandSettingsPage(hwndDlg, wParam);
            break;

        case WM_NOTIFY:
            return OnNotifySettingsPage(hwndDlg, lParam);
    }

    return FALSE;
}
