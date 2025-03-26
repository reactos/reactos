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

static INT s_nAliveLeafCount = 0;
static INT s_nRootCount = 0;
static INT s_iKeyboardImage = -1;
static INT s_iDotImage = -1;

static HICON
CreateLayoutIcon(LANGID LangID)
{
    WCHAR szBuf[4];
    HDC hdcScreen, hdc;
    HBITMAP hbmColor, hbmMono, hBmpOld;
    HFONT hFont, hFontOld;
    LOGFONTW lf;
    RECT rect;
    ICONINFO IconInfo;
    HICON hIcon;
    INT cxIcon = GetSystemMetrics(SM_CXSMICON);
    INT cyIcon = GetSystemMetrics(SM_CYSMICON);

    /* Getting "EN", "FR", etc. from English, French, ... */
    if (GetLocaleInfoW(LangID,
                       LOCALE_SABBREVLANGNAME | LOCALE_NOUSEROVERRIDE,
                       szBuf,
                       ARRAYSIZE(szBuf)) == 0)
    {
        szBuf[0] = szBuf[1] = L'?';
    }
    szBuf[2] = UNICODE_NULL; /* Truncate the identifier to two characters: "ENG" --> "EN" etc. */

    /* Create hdc, hbmColor and hbmMono */
    hdcScreen = GetDC(NULL);
    hdc = CreateCompatibleDC(hdcScreen);
    hbmColor = CreateCompatibleBitmap(hdcScreen, cxIcon, cyIcon);
    ReleaseDC(NULL, hdcScreen);
    hbmMono = CreateBitmap(cxIcon, cyIcon, 1, 1, NULL);

    /* Checking NULL */
    if (!hdc || !hbmColor || !hbmMono)
    {
        if (hbmMono)
            DeleteObject(hbmMono);
        if (hbmColor)
            DeleteObject(hbmColor);
        if (hdc)
            DeleteDC(hdc);
        return NULL;
    }

    /* Create a font */
    hFont = NULL;
    if (SystemParametersInfoW(SPI_GETICONTITLELOGFONT, sizeof(lf), &lf, 0))
    {
        /* Override the current size with something manageable */
        lf.lfHeight = -11;
        lf.lfWidth = 0;
        hFont = CreateFontIndirectW(&lf);
    }
    if (!hFont)
        hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);

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

    /* Fill hbmMono with black */
    SelectObject(hdc, hbmMono);
    PatBlt(hdc, 0, 0, cxIcon, cyIcon, BLACKNESS);
    SelectObject(hdc, hBmpOld);

    /* Create an icon from hbmColor and hbmMono */
    IconInfo.fIcon = TRUE;
    IconInfo.xHotspot = IconInfo.yHotspot = 0;
    IconInfo.hbmColor = hbmColor;
    IconInfo.hbmMask = hbmMono;
    hIcon = CreateIconIndirect(&IconInfo);

    /* Clean up */
    DeleteObject(hFont);
    DeleteObject(hbmMono);
    DeleteObject(hbmColor);
    DeleteDC(hdc);

    return hIcon;
}

static VOID InitDefaultLangComboBox(HWND hwndCombo)
{
    WCHAR szText[256];
    INPUT_LIST_NODE *pNode;
    INT iIndex, nCount, iDefault = (INT)SendMessageW(hwndCombo, CB_GETCURSEL, 0, 0);

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

    nCount = (INT)SendMessageW(hwndCombo, CB_GETCOUNT, 0, 0);
    if (iDefault >= nCount)
        SendMessageW(hwndCombo, CB_SETCURSEL, nCount - 1, 0);
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
    TV_ITEM item = { TVIF_PARAM | TVIF_HANDLE };
    item.hItem = hSelected;

    bIsLeaf = (hSelected && TreeView_GetItem(hwndList, &item) && HIWORD(item.lParam));

    bCanRemove = (bIsLeaf && (s_nAliveLeafCount > 1)) || (s_nRootCount > 1);
    bCanProp = bIsLeaf;

    EnableWindow(GetDlgItem(hwndDlg, IDC_REMOVE_BUTTON), bCanRemove);
    EnableWindow(GetDlgItem(hwndDlg, IDC_PROP_BUTTON), bCanProp);

    InitDefaultLangComboBox(hwndCombo);
}

static BOOL CALLBACK
EnumResNameProc(HMODULE hModule, LPCWSTR lpszType, LPWSTR lpszName, LONG_PTR lParam)
{
    HICON* phIconSm = (HICON*)lParam;
    if (*phIconSm)
        return FALSE;

    *phIconSm = (HICON)LoadImageW(hModule, lpszName, IMAGE_ICON,
                                  GetSystemMetrics(SM_CXSMICON),
                                  GetSystemMetrics(SM_CYSMICON),
                                  0);
    return TRUE;
}

static HICON LoadIMEIcon(LPCTSTR pszImeFile)
{
    WCHAR szSysDir[MAX_PATH], szPath[MAX_PATH];
    HINSTANCE hImeInst;
    HICON hIconSm = NULL;

    GetSystemDirectoryW(szSysDir, _countof(szSysDir));
    StringCchPrintfW(szPath, _countof(szPath), L"%s\\%s", szSysDir, pszImeFile);

    hImeInst = LoadLibraryExW(szPath, NULL, DONT_RESOLVE_DLL_REFERENCES);
    if (hImeInst == NULL)
        return NULL;

    EnumResourceNamesW(hImeInst, RT_GROUP_ICON, EnumResNameProc, (LPARAM)&hIconSm);
    FreeLibrary(hImeInst);

    return hIconSm;
}

static HTREEITEM FindLanguageInList(HWND hwndList, LPCTSTR pszLangName)
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
        if (_wcsicmp(szText, pszLangName) == 0)
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
        item.lParam         = LOWORD(pInputNode->pLocale->dwId); // HIWORD(item.lParam) == 0
        if (bBold)
        {
            item.state = item.stateMask = TVIS_BOLD;
        }
        insert.hParent      = TVI_ROOT;
        insert.hInsertAfter = TVI_LAST;
        insert.item         = item;
        hItem = TreeView_InsertItem(hwndList, &insert);

        // The type of input method (currently keyboard only)
        LoadStringW(hApplet, IDS_KEYBOARD, szKeyboard, _countof(szKeyboard));
        ZeroMemory(&item, sizeof(item));
        item.mask           = TVIF_TEXT | TVIF_IMAGE | TVIF_PARAM | TVIF_SELECTEDIMAGE;
        item.pszText        = szKeyboard;
        item.iImage         = s_iKeyboardImage;
        item.iSelectedImage = s_iKeyboardImage;
        item.lParam         = 0;  // HIWORD(item.lParam) == 0
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

        // The type of input method (currently keyboard only)
        hItem = TreeView_GetChild(hwndList, hItem);
    }

    // Input method
    if (hItem)
    {
        INT ImeImageIndex = s_iDotImage;
        if (IS_IME_HKL(pInputNode->hkl) && pInputNode->pLayout->pszImeFile) // IME?
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
        item.lParam         = (LPARAM)pInputNode; // HIWORD(item.lParam) != 0
        if (bBold)
        {
            item.state = item.stateMask = TVIS_BOLD; // Make the item bold
        }
        insert.hParent      = hItem;
        insert.hInsertAfter = TVI_LAST;
        insert.item         = item;
        hItem = TreeView_InsertItem(hwndList, &insert);
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
    HICON hKeyboardIcon, hDotIcon;

    ImageList_RemoveAll(hImageList);
    TreeView_DeleteAllItems(hwndList);

    // Add keyboard icon
    s_iKeyboardImage = -1;
    hKeyboardIcon = (HICON)LoadImageW(hApplet, MAKEINTRESOURCEW(IDI_KEYBOARD), IMAGE_ICON,
                                      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                      0);
    if (hKeyboardIcon)
    {
        s_iKeyboardImage = ImageList_AddIcon(hImageList, hKeyboardIcon);
        DestroyIcon(hKeyboardIcon);
    }

    // Add dot icon
    s_iDotImage = -1;
    hDotIcon = (HICON)LoadImageW(hApplet, MAKEINTRESOURCEW(IDI_DOT), IMAGE_ICON,
                                 GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON),
                                 0);
    if (hDotIcon)
    {
        s_iDotImage = ImageList_AddIcon(hImageList, hDotIcon);
        DestroyIcon(hDotIcon);
    }

    InputList_Sort();

    s_nAliveLeafCount = InputList_GetAliveCount();

    // Add items to the list
    for (pNode = InputList_GetFirst(); pNode; pNode = pNode->pNext)
    {
        if (pNode->wFlags & INPUT_LIST_NODE_FLAG_DELETED)
            continue;

        AddToInputListView(hwndList, pNode);
    }

    // Expand all (with counting s_nRootCount)
    s_nRootCount = 0;
    hItem = TreeView_GetRoot(hwndList);
    while (hItem)
    {
        ++s_nRootCount;
        ExpandTreeItem(hwndList, hItem);
        hItem = TreeView_GetNextSibling(hwndList, hItem);
    }

    // Redraw
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

    hLayoutImageList = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                        GetSystemMetrics(SM_CYSMICON),
                                        ILC_COLOR8 | ILC_MASK, 0, 0);
    if (hLayoutImageList != NULL)
    {
        hOldImageList = TreeView_SetImageList(hwndInputList, hLayoutImageList, TVSIL_NORMAL);
        ImageList_Destroy(hOldImageList);
    }

    UpdateInputListView(hwndInputList);

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
                    if (item.lParam == 0) // Branch? (currently branch is keyboard only)
                    {
                        // Get root of branch
                        item.hItem = TreeView_GetParent(hwndList, hItem);
                        TreeView_GetItem(hwndList, &item);
                    }

                    if (HIWORD(item.lParam)) // Leaf?
                    {
                        if (InputList_Remove((INPUT_LIST_NODE*)item.lParam))
                            g_bRebootNeeded = TRUE;
                    }
                    else // Root?
                    {
                        if (InputList_RemoveByLang(LOWORD(item.lParam)))
                            g_bRebootNeeded = TRUE;
                    }

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

                if (hItem && TreeView_GetItem(hwndList, &item) && HIWORD(item.lParam))
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
                        INPUT_LIST_NODE* pNode = (INPUT_LIST_NODE*)lParam;
                        if (!(pNode->wFlags & INPUT_LIST_NODE_FLAG_DEFAULT))
                        {
                            g_bRebootNeeded = TRUE;
                            InputList_SetDefault(pNode);
                            UpdateInputListView(hwndList);
                            SetControlsState(hwndDlg);
                            PropSheet_Changed(GetParent(hwndDlg), hwndDlg);
                        }
                    }
                }
            }
        }
    }
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
            break;
        }

        case TVN_ITEMEXPANDING:
        {
            // FIXME: Prevent collapse (COMCTL32 is buggy)
            // https://bugs.winehq.org/show_bug.cgi?id=53727
            NM_TREEVIEW* pTreeView = (NM_TREEVIEW*)lParam;
            if ((pTreeView->action & TVE_TOGGLE) == TVE_COLLAPSE)
            {
                SetWindowLongPtrW(hwndDlg, DWLP_MSGRESULT, TRUE);
                return TRUE;
            }
            break;
        }

        case PSN_APPLY:
        {
            /* Write Input Methods list to registry */
            g_bRebootNeeded |= InputList_Process();
            break;
        }
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
            return TRUE;

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
