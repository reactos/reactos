/*
 * PROJECT:     ReactOS Applications Manager
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * FILE:        base/applications/rapps/gui.cpp
 * PURPOSE:     GUI classes for RAPPS
 * COPYRIGHT:   Copyright 2015 David Quintana           (gigaherz@gmail.com)
 *              Copyright 2017 Alexander Shaposhnikov   (sanchaez@reactos.org)
 */
#include "rapps.h"

#include "rapps.h"
#include "rosui.h"
#include "crichedit.h"

#include <shlobj_undoc.h>
#include <shlguid_undoc.h>

#include <atlbase.h>
#include <atlcom.h>
#include <atlwin.h>
#include <wininet.h>
#include <shellutils.h>
#include <rosctrls.h>

#define SEARCH_TIMER_ID 'SR'
#define LISTVIEW_ICON_SIZE 24
#define TREEVIEW_ICON_SIZE 24

HWND hListView = NULL;

INT GetSystemColorDepth()
{
    DEVMODEW pDevMode;
    INT ColorDepth;

    pDevMode.dmSize = sizeof(pDevMode);
    pDevMode.dmDriverExtra = 0;

    if (!EnumDisplaySettingsW(NULL, ENUM_CURRENT_SETTINGS, &pDevMode))
    {
        /* TODO: Error message */
        return ILC_COLOR;
    }

    switch (pDevMode.dmBitsPerPel)
    {
    case 32: ColorDepth = ILC_COLOR32; break;
    case 24: ColorDepth = ILC_COLOR24; break;
    case 16: ColorDepth = ILC_COLOR16; break;
    case  8: ColorDepth = ILC_COLOR8;  break;
    case  4: ColorDepth = ILC_COLOR4;  break;
    default: ColorDepth = ILC_COLOR;   break;
    }

    return ColorDepth;
}

class CAvailableAppView
{
    static inline VOID InsertTextAfterLoaded_RichEdit(UINT uStringID,
                                                      const ATL::CStringW& szText,
                                                      DWORD StringFlags,
                                                      DWORD TextFlags)
    {
        ATL::CStringW szLoadedText;
        if (!szText.IsEmpty() && szLoadedText.LoadStringW(uStringID))
        {
            InsertRichEditText(szLoadedText, StringFlags);
            InsertRichEditText(szText, TextFlags);
        }
    }

    static inline VOID InsertLoadedTextNewl_RichEdit(UINT uStringID,
                                                     DWORD StringFlags)
    {
        ATL::CStringW szLoadedText;
        if (szLoadedText.LoadStringW(uStringID))
        {
            InsertRichEditText(L"\n", 0);
            InsertRichEditText(szLoadedText, StringFlags);
            InsertRichEditText(L"\n", 0);
        }
    }

    static VOID InsertVersionInfo_RichEdit(CAvailableApplicationInfo* Info)
    {
        if (Info->IsInstalled())
        {
            if (Info->HasInstalledVersion())
            {
                if (Info->HasUpdate())
                    InsertLoadedTextNewl_RichEdit(IDS_STATUS_UPDATE_AVAILABLE, CFE_ITALIC);
                else
                    InsertLoadedTextNewl_RichEdit(IDS_STATUS_INSTALLED, CFE_ITALIC);

                InsertTextAfterLoaded_RichEdit(IDS_AINFO_VERSION, Info->m_szInstalledVersion, CFE_BOLD, 0);
            }
            else
            {
                InsertLoadedTextNewl_RichEdit(IDS_STATUS_INSTALLED, CFE_ITALIC);
            }
        }
        else
        {
            InsertLoadedTextNewl_RichEdit(IDS_STATUS_NOTINSTALLED, CFE_ITALIC);
        }

        InsertTextAfterLoaded_RichEdit(IDS_AINFO_AVAILABLEVERSION, Info->m_szVersion, CFE_BOLD, 0);
    }

    static VOID InsertLicenseInfo_RichEdit(CAvailableApplicationInfo* Info)
    {
        ATL::CStringW szLicense;
        switch (Info->m_LicenseType)
        {
        case LICENSE_OPENSOURCE:
            szLicense.LoadStringW(IDS_LICENSE_OPENSOURCE);
            break;
        case LICENSE_FREEWARE:
            szLicense.LoadStringW(IDS_LICENSE_FREEWARE);
            break;
        case LICENSE_TRIAL:
            szLicense.LoadStringW(IDS_LICENSE_TRIAL);
            break;
        default:
            InsertTextAfterLoaded_RichEdit(IDS_AINFO_LICENSE, Info->m_szLicense, CFE_BOLD, 0);
            return;
        }

        szLicense += L" (" + Info->m_szLicense + L")";
        InsertTextAfterLoaded_RichEdit(IDS_AINFO_LICENSE, szLicense, CFE_BOLD, 0);
    }

    static VOID InsertLanguageInfo_RichEdit(CAvailableApplicationInfo* Info)
    {
        if (!Info->HasLanguageInfo())
        {
            return;
        }

        const INT nTranslations = Info->m_LanguageLCIDs.GetSize();
        ATL::CStringW szLangInfo;
        ATL::CStringW szLoadedTextAvailability;
        ATL::CStringW szLoadedAInfoText;

        szLoadedAInfoText.LoadStringW(IDS_AINFO_LANGUAGES);

        if (Info->HasNativeLanguage())
        {
            szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_AVAILABLE_TRANSLATION);
            if (nTranslations > 1)
            {
                ATL::CStringW buf;
                buf.LoadStringW(IDS_LANGUAGE_MORE_PLACEHOLDER);
                szLangInfo.Format(buf, nTranslations - 1);
            }
            else
            {
                szLangInfo.LoadStringW(IDS_LANGUAGE_SINGLE);
                szLangInfo = L" (" + szLangInfo + L")";
            }
        }
        else if (Info->HasEnglishLanguage())
        {
            szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_ENGLISH_TRANSLATION);
            if (nTranslations > 1)
            {
                ATL::CStringW buf;
                buf.LoadStringW(IDS_LANGUAGE_AVAILABLE_PLACEHOLDER);
                szLangInfo.Format(buf, nTranslations - 1);
            }
            else
            {
                szLangInfo.LoadStringW(IDS_LANGUAGE_SINGLE);
                szLangInfo = L" (" + szLangInfo + L")";
            }
        }
        else
        {
            szLoadedTextAvailability.LoadStringW(IDS_LANGUAGE_NO_TRANSLATION);
        }

        InsertRichEditText(szLoadedAInfoText, CFE_BOLD);
        InsertRichEditText(szLoadedTextAvailability, NULL);
        InsertRichEditText(szLangInfo, CFE_ITALIC);
    }

public:
    static BOOL ShowAvailableAppInfo(INT Index)
    {
        CAvailableApplicationInfo* Info = (CAvailableApplicationInfo*) ListViewGetlParam(Index);
        if (!Info) return FALSE;

        NewRichEditText(Info->m_szName, CFE_BOLD);
        InsertVersionInfo_RichEdit(Info);
        InsertLicenseInfo_RichEdit(Info);
        InsertLanguageInfo_RichEdit(Info);

        InsertTextAfterLoaded_RichEdit(IDS_AINFO_SIZE, Info->m_szSize, CFE_BOLD, 0);
        InsertTextAfterLoaded_RichEdit(IDS_AINFO_URLSITE, Info->m_szUrlSite, CFE_BOLD, CFE_LINK);
        InsertTextAfterLoaded_RichEdit(IDS_AINFO_DESCRIPTION, Info->m_szDesc, CFE_BOLD, 0);
        InsertTextAfterLoaded_RichEdit(IDS_AINFO_URLDOWNLOAD, Info->m_szUrlDownload, CFE_BOLD, CFE_LINK);

        return TRUE;
    }
};

class CMainToolbar :
    public CUiWindow< CToolbar<> >
{
    const INT m_iToolbarHeight;
    DWORD m_dButtonsWidthMax;

    WCHAR szInstallBtn[MAX_STR_LEN];
    WCHAR szUninstallBtn[MAX_STR_LEN];
    WCHAR szModifyBtn[MAX_STR_LEN];
    WCHAR szSelectAll[MAX_STR_LEN];

    VOID AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex)
    {
        HICON hImage;

        if (!(hImage = (HICON) LoadImageW(hInst,
                                          MAKEINTRESOURCE(ImageIndex),
                                          IMAGE_ICON,
                                          m_iToolbarHeight,
                                          m_iToolbarHeight,
                                          0)))
        {
            /* TODO: Error message */
        }

        ImageList_AddIcon(hImageList, hImage);
        DeleteObject(hImage);
    }

    HIMAGELIST InitImageList()
    {
        HIMAGELIST hImageList;

        /* Create the toolbar icon image list */
        hImageList = ImageList_Create(m_iToolbarHeight,//GetSystemMetrics(SM_CXSMICON),
                                      m_iToolbarHeight,//GetSystemMetrics(SM_CYSMICON),
                                      ILC_MASK | GetSystemColorDepth(),
                                      1, 1);
        if (!hImageList)
        {
            /* TODO: Error message */
            return NULL;
        }

        AddImageToImageList(hImageList, IDI_INSTALL);
        AddImageToImageList(hImageList, IDI_UNINSTALL);
        AddImageToImageList(hImageList, IDI_MODIFY);
        AddImageToImageList(hImageList, IDI_CHECK_ALL);
        AddImageToImageList(hImageList, IDI_REFRESH);
        AddImageToImageList(hImageList, IDI_UPDATE_DB);
        AddImageToImageList(hImageList, IDI_SETTINGS);
        AddImageToImageList(hImageList, IDI_EXIT);

        return hImageList;
    }

public:
    CMainToolbar() : m_iToolbarHeight(24)
    {
    }

    VOID OnGetDispInfo(LPTOOLTIPTEXT lpttt)
    {
        UINT idButton = (UINT) lpttt->hdr.idFrom;

        switch (idButton)
        {
        case ID_EXIT:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_EXIT);
            break;

        case ID_INSTALL:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_INSTALL);
            break;

        case ID_UNINSTALL:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UNINSTALL);
            break;

        case ID_MODIFY:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_MODIFY);
            break;

        case ID_SETTINGS:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_SETTINGS);
            break;

        case ID_REFRESH:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_REFRESH);
            break;

        case ID_RESETDB:
            lpttt->lpszText = MAKEINTRESOURCEW(IDS_TOOLTIP_UPDATE_DB);
            break;
        }
    }

    HWND Create(HWND hwndParent)
    {
        /* Create buttons */
        TBBUTTON Buttons[] =
        {   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
            {  0, ID_INSTALL,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szInstallBtn      },
            {  1, ID_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szUninstallBtn    },
            {  2, ID_MODIFY,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szModifyBtn       },
            {  3, ID_CHECK_ALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, (INT_PTR) szSelectAll       },
            { -1, 0,            TBSTATE_ENABLED, BTNS_SEP,                    { 0 }, 0, 0                           },
            {  4, ID_REFRESH,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
            {  5, ID_RESETDB,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
            { -1, 0,            TBSTATE_ENABLED, BTNS_SEP,                    { 0 }, 0, 0                           },
            {  6, ID_SETTINGS,  TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
            {  7, ID_EXIT,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, { 0 }, 0, 0                           },
        };

        LoadStringW(hInst, IDS_INSTALL, szInstallBtn, _countof(szInstallBtn));
        LoadStringW(hInst, IDS_UNINSTALL, szUninstallBtn, _countof(szUninstallBtn));
        LoadStringW(hInst, IDS_MODIFY, szModifyBtn, _countof(szModifyBtn));
        LoadStringW(hInst, IDS_SELECT_ALL, szSelectAll, _countof(szSelectAll));

        m_hWnd = CreateWindowExW(0, TOOLBARCLASSNAMEW, NULL,
                                 WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST,
                                 0, 0, 0, 0,
                                 hwndParent,
                                 0, hInst, NULL);

        if (!m_hWnd)
        {
            /* TODO: Show error message */
            return FALSE;
        }

        SendMessageW(TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
        SetButtonStructSize();

        /* Set image list */
        HIMAGELIST hImageList = InitImageList();

        if (!hImageList)
        {
            /* TODO: Show error message */
            return FALSE;
        }

        ImageList_Destroy(SetImageList(hImageList));

        AddButtons(_countof(Buttons), Buttons);

        /* Remember ideal width to use as a max width of buttons */
        SIZE size;
        GetIdealSize(FALSE, &size);
        m_dButtonsWidthMax = size.cx;

        return m_hWnd;
    }

    VOID HideButtonCaption()
    {
        DWORD dCurrentExStyle = (DWORD) SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0);
        SendMessageW(TB_SETEXTENDEDSTYLE, 0, dCurrentExStyle | TBSTYLE_EX_MIXEDBUTTONS);
    }

    VOID ShowButtonCaption()
    {
        DWORD dCurrentExStyle = (DWORD) SendMessageW(TB_GETEXTENDEDSTYLE, 0, 0);
        SendMessageW(TB_SETEXTENDEDSTYLE, 0, dCurrentExStyle & ~TBSTYLE_EX_MIXEDBUTTONS);
    }

    DWORD GetMaxButtonsWidth() const
    {
        return m_dButtonsWidthMax;
    }
};

class CAppsListView :
    public CUiWindow<CListView>
{
    struct SortContext
    {
        CAppsListView * lvw;
        INT iSubItem;
    };

    BOOL bHasAllChecked;
    BOOL bIsAscending;
    BOOL bHasCheckboxes;

    INT nLastHeaderID;

public:
    CAppsListView() :
        bHasAllChecked(FALSE),
        bIsAscending(TRUE),
        bHasCheckboxes(FALSE),
        nLastHeaderID(-1)
    {
    }

    VOID SetCheckboxesVisible(BOOL bIsVisible)
    {
        if (bIsVisible)
        {
            SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
        }
        else
        {
            SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
        }

        bHasCheckboxes = bIsVisible;
    }

    VOID ColumnClick(LPNMLISTVIEW pnmv)
    {
        HWND hHeader;
        HDITEMW hColumn;
        INT nHeaderID = pnmv->iSubItem;

        if ((GetWindowLongPtr(GWL_STYLE) & ~LVS_NOSORTHEADER) == 0)
            return;

        hHeader = (HWND) SendMessage(LVM_GETHEADER, 0, 0);
        ZeroMemory(&hColumn, sizeof(hColumn));

        /* If the sorting column changed, remove the sorting style from the old column */
        if ((nLastHeaderID != -1) && (nLastHeaderID != nHeaderID))
        {
            hColumn.mask = HDI_FORMAT;
            Header_GetItem(hHeader, nLastHeaderID, &hColumn);
            hColumn.fmt &= ~(HDF_SORTUP | HDF_SORTDOWN);
            Header_SetItem(hHeader, nLastHeaderID, &hColumn);
        }

        /* Set the sorting style to the new column */
        hColumn.mask = HDI_FORMAT;
        Header_GetItem(hHeader, nHeaderID, &hColumn);

        hColumn.fmt &= (bIsAscending ? ~HDF_SORTDOWN : ~HDF_SORTUP);
        hColumn.fmt |= (bIsAscending ? HDF_SORTUP : HDF_SORTDOWN);
        Header_SetItem(hHeader, nHeaderID, &hColumn);

        /* Sort the list, using the current values of nHeaderID and bIsAscending */
        SortContext ctx = {this, nHeaderID};
        SortItems(s_CompareFunc, &ctx);

        /* Save new values */
        nLastHeaderID = nHeaderID;
        bIsAscending = !bIsAscending;
    }

    PVOID GetLParam(INT Index)
    {
        INT ItemIndex;
        LVITEMW Item;

        if (Index == -1)
        {
            ItemIndex = (INT) SendMessage(LVM_GETNEXTITEM, -1, LVNI_FOCUSED);
            if (ItemIndex == -1)
                return NULL;
        }
        else
        {
            ItemIndex = Index;
        }

        ZeroMemory(&Item, sizeof(Item));

        Item.mask = LVIF_PARAM;
        Item.iItem = ItemIndex;
        if (!GetItem(&Item))
            return NULL;

        return (PVOID) Item.lParam;
    }

    BOOL AddColumn(INT Index, ATL::CStringW& Text, INT Width, INT Format)
    {
        return AddColumn(Index, const_cast<LPWSTR>(Text.GetString()), Width, Format);
    }

    BOOL AddColumn(INT Index, LPWSTR lpText, INT Width, INT Format)
    {
        LVCOLUMNW Column;

        ZeroMemory(&Column, sizeof(Column));

        Column.mask = LVCF_FMT | LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;
        Column.iSubItem = Index;
        Column.pszText = lpText;
        Column.cx = Width;
        Column.fmt = Format;

        return (InsertColumn(Index, &Column) == -1) ? FALSE : TRUE;
    }

    INT AddItem(INT ItemIndex, INT IconIndex, LPWSTR lpText, LPARAM lParam)
    {
        LVITEMW Item;

        ZeroMemory(&Item, sizeof(Item));

        Item.mask = LVIF_TEXT | LVIF_PARAM | LVIF_STATE | LVIF_IMAGE;
        Item.pszText = lpText;
        Item.lParam = lParam;
        Item.iItem = ItemIndex;
        Item.iImage = IconIndex;

        return InsertItem(&Item);
    }

    static INT CALLBACK s_CompareFunc(LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort)
    {
        SortContext * ctx = ((SortContext*) lParamSort);
        return ctx->lvw->CompareFunc(lParam1, lParam2, ctx->iSubItem);
    }

    INT CompareFunc(LPARAM lParam1, LPARAM lParam2, INT iSubItem)
    {
        ATL::CStringW Item1, Item2;
        LVFINDINFOW IndexInfo;
        INT Index;

        IndexInfo.flags = LVFI_PARAM;

        IndexInfo.lParam = lParam1;
        Index = FindItem(-1, &IndexInfo);
        GetItemText(Index, iSubItem, Item1.GetBuffer(MAX_STR_LEN), MAX_STR_LEN);
        Item1.ReleaseBuffer();

        IndexInfo.lParam = lParam2;
        Index = FindItem(-1, &IndexInfo);
        GetItemText(Index, iSubItem, Item2.GetBuffer(MAX_STR_LEN), MAX_STR_LEN);
        Item2.ReleaseBuffer();

        return bIsAscending ? Item1.Compare(Item2) : Item2.Compare(Item1);
    }

    HWND Create(HWND hwndParent)
    {
        RECT r = {205, 28, 465, 250};
        DWORD style = WS_CHILD | WS_VISIBLE | LVS_SORTASCENDING | LVS_REPORT | LVS_SINGLESEL | LVS_SHOWSELALWAYS;
        HMENU menu = GetSubMenu(LoadMenuW(hInst, MAKEINTRESOURCEW(IDR_APPLICATIONMENU)), 0);

        HWND hwnd = CListView::Create(hwndParent, r, NULL, style, WS_EX_CLIENTEDGE, menu);

        if (hwnd)
        {
            SetCheckboxesVisible(FALSE);
        }

        return hwnd;
    }

    BOOL GetCheckState(INT item)
    {
        return (BOOL) (GetItemState(item, LVIS_STATEIMAGEMASK) >> 12) - 1;
    }

    VOID SetCheckState(INT item, BOOL fCheck)
    {
        if (bHasCheckboxes)
        {
            SetItemState(item, INDEXTOSTATEIMAGEMASK((fCheck) ? 2 : 1), LVIS_STATEIMAGEMASK);
            SetSelected(item, fCheck);
        }
    }

    VOID SetSelected(INT item, BOOL value)
    {
        if (item < 0)
        {
            for (INT i = 0; i >= 0; i = GetNextItem(i, LVNI_ALL))
            {
                CAvailableApplicationInfo* pAppInfo = (CAvailableApplicationInfo*) GetItemData(i);
                if (pAppInfo)
                {
                    pAppInfo->m_IsSelected = value;
                }
            }
        }
        else
        {
            CAvailableApplicationInfo* pAppInfo = (CAvailableApplicationInfo*) GetItemData(item);
            if (pAppInfo)
            {
                pAppInfo->m_IsSelected = value;
            }
        }
    }

    VOID CheckAll()
    {
        if (bHasCheckboxes)
        {
            bHasAllChecked = !bHasAllChecked;
            SetCheckState(-1, bHasAllChecked);
        }
    }

    ATL::CSimpleArray<CAvailableApplicationInfo> GetCheckedItems()
    {
        if (!bHasCheckboxes)
        {
            return ATL::CSimpleArray<CAvailableApplicationInfo>();
        }

        ATL::CSimpleArray<CAvailableApplicationInfo> list;
        for (INT i = 0; i >= 0; i = GetNextItem(i, LVNI_ALL))
        {
            if (GetCheckState(i) != FALSE)
            {
                CAvailableApplicationInfo* pAppInfo = (CAvailableApplicationInfo*) GetItemData(i);
                list.Add(*pAppInfo);
            }
        }
        return list;
    }

    CAvailableApplicationInfo* GetSelectedData()
    {
        INT item = GetSelectionMark();
        return (CAvailableApplicationInfo*) GetItemData(item);
    }
};

class CSideTreeView :
    public CUiWindow<CTreeView>
{
    HIMAGELIST hImageTreeView;

public:
    CSideTreeView() :
        CUiWindow(),
        hImageTreeView(ImageList_Create(TREEVIEW_ICON_SIZE, TREEVIEW_ICON_SIZE,
                                        GetSystemColorDepth() | ILC_MASK,
                                        0, 1))
    {
    }

    HTREEITEM AddItem(HTREEITEM hParent, ATL::CStringW &Text, INT Image, INT SelectedImage, LPARAM lParam)
    {
        return CUiWindow<CTreeView>::AddItem(hParent, const_cast<LPWSTR>(Text.GetString()), Image, SelectedImage, lParam);
    }

    HTREEITEM AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
    {
        ATL::CStringW szText;
        INT Index;
        HICON hIcon;

        hIcon = (HICON) LoadImageW(hInst,
                                   MAKEINTRESOURCE(IconIndex),
                                   IMAGE_ICON,
                                   TREEVIEW_ICON_SIZE,
                                   TREEVIEW_ICON_SIZE,
                                   LR_CREATEDIBSECTION);
        if (hIcon)
        {
            Index = ImageList_AddIcon(hImageTreeView, hIcon);
            DestroyIcon(hIcon);
        }

        szText.LoadStringW(TextIndex);
        return AddItem(hRootItem, szText, Index, Index, TextIndex);
    }

    HIMAGELIST SetImageList()
    {
        return CUiWindow<CTreeView>::SetImageList(hImageTreeView, TVSIL_NORMAL);
    }

    VOID DestroyImageList()
    {
        if (hImageTreeView)
            ImageList_Destroy(hImageTreeView);
    }

    ~CSideTreeView()
    {
        DestroyImageList();
    }
};

class CSearchBar :
    public CWindow
{
public:
    const INT m_Width;
    const INT m_Height;

    CSearchBar() : m_Width(200), m_Height(22)
    {
    }

    VOID SetText(LPCWSTR lpszText)
    {
        SendMessageW(SB_SETTEXT, SBT_NOBORDERS, (LPARAM) lpszText);
    }

    HWND Create(HWND hwndParent)
    {
        ATL::CStringW szBuf;
        m_hWnd = CreateWindowExW(WS_EX_CLIENTEDGE, L"Edit", NULL,
                                 WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL,
                                 0, 0, m_Width, m_Height,
                                 hwndParent, (HMENU) NULL,
                                 hInst, 0);

        SendMessageW(WM_SETFONT, (WPARAM) GetStockObject(DEFAULT_GUI_FONT), 0);
        szBuf.LoadStringW(IDS_SEARCH_TEXT);
        SetWindowTextW(szBuf);
        return m_hWnd;
    }

};

class CMainWindow :
    public CWindowImpl<CMainWindow, CWindow, CFrameWinTraits>
{
    CUiPanel* m_ClientPanel;
    CUiSplitPanel* m_VSplitter;
    CUiSplitPanel* m_HSplitter;

    CMainToolbar* m_Toolbar;
    CAppsListView* m_ListView;

    CSideTreeView* m_TreeView;
    CUiWindow<CStatusBar>* m_StatusBar;
    CUiWindow<CRichEdit>* m_RichEdit;

    CUiWindow<CSearchBar>* m_SearchBar;
    CAvailableApps m_AvailableApps;

    LPWSTR pLink;

    INT nSelectedApps;

    BOOL bSearchEnabled;
    BOOL bUpdating;

public:
    CMainWindow() :
        m_ClientPanel(NULL),
        pLink(NULL),
        bSearchEnabled(FALSE)
    {
    }

private:
    VOID InitApplicationsList()
    {
        ATL::CStringW szText;

        /* Add columns to ListView */
        szText.LoadStringW(IDS_APP_NAME);
        m_ListView->AddColumn(0, szText, 250, LVCFMT_LEFT);

        szText.LoadStringW(IDS_APP_INST_VERSION);
        m_ListView->AddColumn(1, szText, 90, LVCFMT_RIGHT);

        szText.LoadStringW(IDS_APP_DESCRIPTION);
        m_ListView->AddColumn(3, szText, 300, LVCFMT_LEFT);

        // Unnesesary since the list updates on every TreeView selection
        // UpdateApplicationsList(ENUM_ALL_COMPONENTS);
    }

    HTREEITEM AddCategory(HTREEITEM hRootItem, UINT TextIndex, UINT IconIndex)
    {
        return m_TreeView->AddCategory(hRootItem, TextIndex, IconIndex);
    }

    VOID InitCategoriesList()
    {
        HTREEITEM hRootItemInstalled, hRootItemAvailable;

        hRootItemInstalled = AddCategory(TVI_ROOT, IDS_INSTALLED, IDI_CATEGORY);
        AddCategory(hRootItemInstalled, IDS_APPLICATIONS, IDI_APPS);
        AddCategory(hRootItemInstalled, IDS_UPDATES, IDI_APPUPD);

        AddCategory(TVI_ROOT, IDS_SELECTEDFORINST, IDI_SELECTEDFORINST);

        hRootItemAvailable = AddCategory(TVI_ROOT, IDS_AVAILABLEFORINST, IDI_CATEGORY);
        AddCategory(hRootItemAvailable, IDS_CAT_AUDIO, IDI_CAT_AUDIO);
        AddCategory(hRootItemAvailable, IDS_CAT_VIDEO, IDI_CAT_VIDEO);
        AddCategory(hRootItemAvailable, IDS_CAT_GRAPHICS, IDI_CAT_GRAPHICS);
        AddCategory(hRootItemAvailable, IDS_CAT_GAMES, IDI_CAT_GAMES);
        AddCategory(hRootItemAvailable, IDS_CAT_INTERNET, IDI_CAT_INTERNET);
        AddCategory(hRootItemAvailable, IDS_CAT_OFFICE, IDI_CAT_OFFICE);
        AddCategory(hRootItemAvailable, IDS_CAT_DEVEL, IDI_CAT_DEVEL);
        AddCategory(hRootItemAvailable, IDS_CAT_EDU, IDI_CAT_EDU);
        AddCategory(hRootItemAvailable, IDS_CAT_ENGINEER, IDI_CAT_ENGINEER);
        AddCategory(hRootItemAvailable, IDS_CAT_FINANCE, IDI_CAT_FINANCE);
        AddCategory(hRootItemAvailable, IDS_CAT_SCIENCE, IDI_CAT_SCIENCE);
        AddCategory(hRootItemAvailable, IDS_CAT_TOOLS, IDI_CAT_TOOLS);
        AddCategory(hRootItemAvailable, IDS_CAT_DRIVERS, IDI_CAT_DRIVERS);
        AddCategory(hRootItemAvailable, IDS_CAT_LIBS, IDI_CAT_LIBS);
        AddCategory(hRootItemAvailable, IDS_CAT_THEMES, IDI_CAT_THEMES);
        AddCategory(hRootItemAvailable, IDS_CAT_OTHER, IDI_CAT_OTHER);

        m_TreeView->SetImageList();
        m_TreeView->Expand(hRootItemInstalled, TVE_EXPAND);
        m_TreeView->Expand(hRootItemAvailable, TVE_EXPAND);
        m_TreeView->SelectItem(hRootItemAvailable);
    }

    BOOL CreateStatusBar()
    {
        m_StatusBar = new CUiWindow<CStatusBar>();
        m_StatusBar->m_VerticalAlignment = UiAlign_RightBtm;
        m_StatusBar->m_HorizontalAlignment = UiAlign_Stretch;
        m_ClientPanel->Children().Append(m_StatusBar);

        return m_StatusBar->Create(m_hWnd, (HMENU) IDC_STATUSBAR) != NULL;
    }

    BOOL CreateToolbar()
    {
        m_Toolbar = new CMainToolbar();
        m_Toolbar->m_VerticalAlignment = UiAlign_LeftTop;
        m_Toolbar->m_HorizontalAlignment = UiAlign_Stretch;
        m_ClientPanel->Children().Append(m_Toolbar);

        return m_Toolbar->Create(m_hWnd) != NULL;
    }

    BOOL CreateTreeView()
    {
        m_TreeView = new CSideTreeView();
        m_TreeView->m_VerticalAlignment = UiAlign_Stretch;
        m_TreeView->m_HorizontalAlignment = UiAlign_Stretch;
        m_VSplitter->First().Append(m_TreeView);

        return m_TreeView->Create(m_hWnd) != NULL;
    }

    BOOL CreateListView()
    {
        m_ListView = new CAppsListView();
        m_ListView->m_VerticalAlignment = UiAlign_Stretch;
        m_ListView->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->First().Append(m_ListView);

        hListView = m_ListView->Create(m_hWnd);
        return hListView != NULL;
    }

    BOOL CreateRichEdit()
    {
        m_RichEdit = new CUiWindow<CRichEdit>();
        m_RichEdit->m_VerticalAlignment = UiAlign_Stretch;
        m_RichEdit->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->Second().Append(m_RichEdit);

        return m_RichEdit->Create(m_hWnd) != NULL;
    }

    BOOL CreateVSplitter()
    {
        m_VSplitter = new CUiSplitPanel();
        m_VSplitter->m_VerticalAlignment = UiAlign_Stretch;
        m_VSplitter->m_HorizontalAlignment = UiAlign_Stretch;
        m_VSplitter->m_DynamicFirst = FALSE;
        m_VSplitter->m_Horizontal = FALSE;
        m_VSplitter->m_MinFirst = 0;
        m_VSplitter->m_MinSecond = 320;
        m_VSplitter->m_Pos = 240;
        m_ClientPanel->Children().Append(m_VSplitter);

        return m_VSplitter->Create(m_hWnd) != NULL;
    }

    BOOL CreateHSplitter()
    {
        m_HSplitter = new CUiSplitPanel();
        m_HSplitter->m_VerticalAlignment = UiAlign_Stretch;
        m_HSplitter->m_HorizontalAlignment = UiAlign_Stretch;
        m_HSplitter->m_DynamicFirst = TRUE;
        m_HSplitter->m_Horizontal = TRUE;
        m_HSplitter->m_Pos = INT_MAX; //set INT_MAX to use lowest possible position (m_MinSecond)
        m_HSplitter->m_MinFirst = 10;
        m_HSplitter->m_MinSecond = 140;
        m_VSplitter->Second().Append(m_HSplitter);

        return m_HSplitter->Create(m_hWnd) != NULL;
    }

    BOOL CreateSearchBar()
    {
        m_SearchBar = new CUiWindow<CSearchBar>();
        m_SearchBar->m_VerticalAlignment = UiAlign_LeftTop;
        m_SearchBar->m_HorizontalAlignment = UiAlign_RightBtm;
        m_SearchBar->m_Margin.top = 4;
        m_SearchBar->m_Margin.right = 6;

        return m_SearchBar->Create(m_Toolbar->m_hWnd) != NULL;
    }

    BOOL CreateLayout()
    {
        BOOL b = TRUE;
        bUpdating = TRUE;

        m_ClientPanel = new CUiPanel();
        m_ClientPanel->m_VerticalAlignment = UiAlign_Stretch;
        m_ClientPanel->m_HorizontalAlignment = UiAlign_Stretch;

        // Top level
        b = b && CreateStatusBar();
        b = b && CreateToolbar();
        b = b && CreateSearchBar();
        b = b && CreateVSplitter();

        // Inside V Splitter
        b = b && CreateHSplitter();
        b = b && CreateTreeView();

        // Inside H Splitter
        b = b && CreateListView();
        b = b && CreateRichEdit();

        if (b)
        {
            RECT rTop;
            RECT rBottom;

            /* Size status bar */
            m_StatusBar->SendMessageW(WM_SIZE, 0, 0);

            /* Size tool bar */
            m_Toolbar->AutoSize();

            ::GetWindowRect(m_Toolbar->m_hWnd, &rTop);
            ::GetWindowRect(m_StatusBar->m_hWnd, &rBottom);

            m_VSplitter->m_Margin.top = rTop.bottom - rTop.top;
            m_VSplitter->m_Margin.bottom = rBottom.bottom - rBottom.top;
        }

        bUpdating = FALSE;
        return b;
    }

    BOOL InitControls()
    {
        if (CreateLayout())
        {

            InitApplicationsList();
            InitCategoriesList();

            nSelectedApps = 0;
            UpdateStatusBarText();

            return TRUE;
        }

        return FALSE;
    }

    VOID OnSize(HWND hwnd, WPARAM wParam, LPARAM lParam)
    {
        if (wParam == SIZE_MINIMIZED)
            return;

        /* Size status bar */
        m_StatusBar->SendMessage(WM_SIZE, 0, 0);

        /* Size tool bar */
        m_Toolbar->AutoSize();

        /* Automatically hide captions */
        DWORD dToolbarTreshold = m_Toolbar->GetMaxButtonsWidth();
        DWORD dSearchbarMargin = (LOWORD(lParam) - m_SearchBar->m_Width);

        if (dSearchbarMargin > dToolbarTreshold)
        {
            m_Toolbar->ShowButtonCaption();
        }
        else if (dSearchbarMargin < dToolbarTreshold)
        {
            m_Toolbar->HideButtonCaption();
        }

        RECT r = {0, 0, LOWORD(lParam), HIWORD(lParam)};
        HDWP hdwp = NULL;
        INT count = m_ClientPanel->CountSizableChildren();

        hdwp = BeginDeferWindowPos(count);
        if (hdwp)
        {
            hdwp = m_ClientPanel->OnParentSize(r, hdwp);
            if (hdwp)
            {
                EndDeferWindowPos(hdwp);
            }

        }

        // TODO: Sub-layouts for children of children
        count = m_SearchBar->CountSizableChildren();
        hdwp = BeginDeferWindowPos(count);
        if (hdwp)
        {
            hdwp = m_SearchBar->OnParentSize(r, hdwp);
            if (hdwp)
            {
                EndDeferWindowPos(hdwp);
            }
        }

    }

    BOOL ProcessWindowMessage(HWND hwnd, UINT Msg, WPARAM wParam, LPARAM lParam, LRESULT& theResult, DWORD dwMapId)
    {
        theResult = 0;
        switch (Msg)
        {
        case WM_CREATE:
            if (!InitControls())
                ::PostMessageW(hwnd, WM_CLOSE, 0, 0);
            break;

        case WM_DESTROY:
        {
            ShowWindow(SW_HIDE);
            SaveSettings(hwnd);

            FreeLogs();
            m_AvailableApps.FreeCachedEntries();

            if (IsInstalledEnum(SelectedEnumType))
                FreeInstalledAppList();

            delete m_ClientPanel;

            PostQuitMessage(0);
            return 0;
        }

        case WM_COMMAND:
            OnCommand(wParam, lParam);
            break;

        case WM_NOTIFY:
        {
            LPNMHDR data = (LPNMHDR) lParam;

            switch (data->code)
            {
            case TVN_SELCHANGED:
            {
                if (data->hwndFrom == m_TreeView->m_hWnd)
                {
                    switch (((LPNMTREEVIEW) lParam)->itemNew.lParam)
                    {
                    case IDS_INSTALLED:
                        UpdateApplicationsList(ENUM_ALL_INSTALLED);
                        break;

                    case IDS_APPLICATIONS:
                        UpdateApplicationsList(ENUM_INSTALLED_APPLICATIONS);
                        break;

                    case IDS_UPDATES:
                        UpdateApplicationsList(ENUM_UPDATES);
                        break;

                    case IDS_AVAILABLEFORINST:
                        UpdateApplicationsList(ENUM_ALL_AVAILABLE);
                        break;

                    case IDS_CAT_AUDIO:
                        UpdateApplicationsList(ENUM_CAT_AUDIO);
                        break;

                    case IDS_CAT_DEVEL:
                        UpdateApplicationsList(ENUM_CAT_DEVEL);
                        break;

                    case IDS_CAT_DRIVERS:
                        UpdateApplicationsList(ENUM_CAT_DRIVERS);
                        break;

                    case IDS_CAT_EDU:
                        UpdateApplicationsList(ENUM_CAT_EDU);
                        break;

                    case IDS_CAT_ENGINEER:
                        UpdateApplicationsList(ENUM_CAT_ENGINEER);
                        break;

                    case IDS_CAT_FINANCE:
                        UpdateApplicationsList(ENUM_CAT_FINANCE);
                        break;

                    case IDS_CAT_GAMES:
                        UpdateApplicationsList(ENUM_CAT_GAMES);
                        break;

                    case IDS_CAT_GRAPHICS:
                        UpdateApplicationsList(ENUM_CAT_GRAPHICS);
                        break;

                    case IDS_CAT_INTERNET:
                        UpdateApplicationsList(ENUM_CAT_INTERNET);
                        break;

                    case IDS_CAT_LIBS:
                        UpdateApplicationsList(ENUM_CAT_LIBS);
                        break;

                    case IDS_CAT_OFFICE:
                        UpdateApplicationsList(ENUM_CAT_OFFICE);
                        break;

                    case IDS_CAT_OTHER:
                        UpdateApplicationsList(ENUM_CAT_OTHER);
                        break;

                    case IDS_CAT_SCIENCE:
                        UpdateApplicationsList(ENUM_CAT_SCIENCE);
                        break;

                    case IDS_CAT_TOOLS:
                        UpdateApplicationsList(ENUM_CAT_TOOLS);
                        break;

                    case IDS_CAT_VIDEO:
                        UpdateApplicationsList(ENUM_CAT_VIDEO);
                        break;

                    case IDS_CAT_THEMES:
                        UpdateApplicationsList(ENUM_CAT_THEMES);
                        break;

                    case IDS_SELECTEDFORINST:
                        UpdateApplicationsList(ENUM_CAT_SELECTED);
                        break;
                    }
                }

                HMENU mainMenu = ::GetMenu(hwnd);
                HMENU lvwMenu = ::GetMenu(m_ListView->m_hWnd);

                /* Disable/enable items based on treeview selection */
                if (IsSelectedNodeInstalled())
                {
                    EnableMenuItem(mainMenu, ID_REGREMOVE, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_INSTALL, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_UNINSTALL, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_MODIFY, MF_ENABLED);

                    EnableMenuItem(lvwMenu, ID_REGREMOVE, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_INSTALL, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_UNINSTALL, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_MODIFY, MF_ENABLED);

                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_REGREMOVE, TRUE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_INSTALL, FALSE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_UNINSTALL, TRUE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_MODIFY, TRUE);
                }
                else
                {
                    EnableMenuItem(mainMenu, ID_REGREMOVE, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_INSTALL, MF_ENABLED);
                    EnableMenuItem(mainMenu, ID_UNINSTALL, MF_GRAYED);
                    EnableMenuItem(mainMenu, ID_MODIFY, MF_GRAYED);

                    EnableMenuItem(lvwMenu, ID_REGREMOVE, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_INSTALL, MF_ENABLED);
                    EnableMenuItem(lvwMenu, ID_UNINSTALL, MF_GRAYED);
                    EnableMenuItem(lvwMenu, ID_MODIFY, MF_GRAYED);

                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_REGREMOVE, FALSE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_INSTALL, TRUE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_UNINSTALL, FALSE);
                    m_Toolbar->SendMessageW(TB_ENABLEBUTTON, ID_MODIFY, FALSE);
                }
            }
            break;

            case LVN_ITEMCHANGED:
            {
                LPNMLISTVIEW pnic = (LPNMLISTVIEW) lParam;

                if (pnic->hdr.hwndFrom == m_ListView->m_hWnd)
                {
                    /* Check if this is a valid item
                    * (technically, it can be also an unselect) */
                    INT ItemIndex = pnic->iItem;
                    if (ItemIndex == -1 ||
                        ItemIndex >= ListView_GetItemCount(pnic->hdr.hwndFrom))
                    {
                        break;
                    }

                    /* Check if the focus has been moved to another item */
                    if ((pnic->uChanged & LVIF_STATE) &&
                        (pnic->uNewState & LVIS_FOCUSED) &&
                        !(pnic->uOldState & LVIS_FOCUSED))
                    {
                        if (IsInstalledEnum(SelectedEnumType))
                            ShowInstalledAppInfo(ItemIndex);
                        if (IsAvailableEnum(SelectedEnumType))
                            CAvailableAppView::ShowAvailableAppInfo(ItemIndex);
                    }
                    /* Check if the item is checked */
                    if ((pnic->uNewState & LVIS_STATEIMAGEMASK) && !bUpdating)
                    {
                        BOOL checked = m_ListView->GetCheckState(pnic->iItem);
                        /* FIXME: HAX!
                        - preventing decremention below zero as a safeguard for ReactOS
                          In ReactOS this action is triggered whenever user changes *selection*, but should be only when *checkbox* state toggled
                          Maybe LVIS_STATEIMAGEMASK is set incorrectly
                        */
                        nSelectedApps +=
                            (checked)
                            ? 1
                            : ((nSelectedApps > 0)
                               ? -1
                               : 0);

                        /* Update item's selection status */
                        m_ListView->SetSelected(pnic->iItem, checked);

                        UpdateStatusBarText();
                    }
                }
            }
            break;

            case LVN_COLUMNCLICK:
            {
                LPNMLISTVIEW pnmv = (LPNMLISTVIEW) lParam;

                m_ListView->ColumnClick(pnmv);
            }
            break;

            case NM_CLICK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    if (IsInstalledEnum(SelectedEnumType))
                        ShowInstalledAppInfo(-1);
                    if (IsAvailableEnum(SelectedEnumType))
                        CAvailableAppView::ShowAvailableAppInfo(-1);
                }
            }
            break;

            case NM_DBLCLK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    /* this won't do anything if the program is already installed */
                    SendMessageW(hwnd, WM_COMMAND, ID_INSTALL, 0);
                }
            }
            break;

            case NM_RCLICK:
            {
                if (data->hwndFrom == m_ListView->m_hWnd && ((LPNMLISTVIEW) lParam)->iItem != -1)
                {
                    ShowPopupMenu(m_ListView->m_hWnd, 0, ID_INSTALL);
                }
            }
            break;

            case EN_LINK:
                OnLink((ENLINK*) lParam);
                break;

            case TTN_GETDISPINFO:
                m_Toolbar->OnGetDispInfo((LPTOOLTIPTEXT) lParam);
                break;
            }
        }
        break;

        case WM_SIZE:
            OnSize(hwnd, wParam, lParam);
            break;

        case WM_SIZING:
        {
            LPRECT pRect = (LPRECT) lParam;

            if (pRect->right - pRect->left < 565)
                pRect->right = pRect->left + 565;

            if (pRect->bottom - pRect->top < 300)
                pRect->bottom = pRect->top + 300;

            return TRUE;
        }

        case WM_SYSCOLORCHANGE:
        {
            /* Forward WM_SYSCOLORCHANGE to common controls */
            m_ListView->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
            m_TreeView->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
            m_Toolbar->SendMessageW(WM_SYSCOLORCHANGE, 0, 0);
            m_ListView->SendMessageW(EM_SETBKGNDCOLOR, 0, GetSysColor(COLOR_BTNFACE));
        }
        break;

        case WM_TIMER:
            if (wParam == SEARCH_TIMER_ID)
            {
                ::KillTimer(hwnd, SEARCH_TIMER_ID);
                if (bSearchEnabled)
                    UpdateApplicationsList(-1);
            }
            break;
        }

        return FALSE;
    }

    virtual VOID OnLink(ENLINK *Link)
    {
        switch (Link->msg)
        {
        case WM_LBUTTONUP:
        case WM_RBUTTONUP:
        {
            if (pLink) HeapFree(GetProcessHeap(), 0, pLink);

            pLink = (LPWSTR) HeapAlloc(GetProcessHeap(), 0,
                (max(Link->chrg.cpMin, Link->chrg.cpMax) -
                 min(Link->chrg.cpMin, Link->chrg.cpMax) + 1) * sizeof(WCHAR));
            if (!pLink)
            {
                /* TODO: Error message */
                return;
            }

            m_RichEdit->SendMessageW(EM_SETSEL, Link->chrg.cpMin, Link->chrg.cpMax);
            m_RichEdit->SendMessageW(EM_GETSELTEXT, 0, (LPARAM) pLink);

            ShowPopupMenu(m_RichEdit->m_hWnd, IDR_LINKMENU, -1);
        }
        break;
        }
    }

    BOOL IsSelectedNodeInstalled()
    {
        HTREEITEM hSelectedItem = m_TreeView->GetSelection();
        TV_ITEM tItem;

        tItem.mask = TVIF_PARAM | TVIF_HANDLE;
        tItem.hItem = hSelectedItem;
        m_TreeView->GetItem(&tItem);
        switch (tItem.lParam)
        {
        case IDS_INSTALLED:
        case IDS_APPLICATIONS:
        case IDS_UPDATES:
            return TRUE;
        default:
            return FALSE;
        }
    }

    VOID OnCommand(WPARAM wParam, LPARAM lParam)
    {
        WORD wCommand = LOWORD(wParam);

        if (lParam == (LPARAM) m_SearchBar->m_hWnd)
        {
            ATL::CStringW szBuf;

            switch (HIWORD(wParam))
            {
            case EN_SETFOCUS:
            {
                ATL::CStringW szWndText;

                szBuf.LoadStringW(IDS_SEARCH_TEXT);
                m_SearchBar->GetWindowTextW(szWndText);
                if (szBuf == szWndText)
                {
                    bSearchEnabled = FALSE;
                    m_SearchBar->SetWindowTextW(L"");
                }
            }
            break;

            case EN_KILLFOCUS:
            {
                m_SearchBar->GetWindowTextW(szBuf);
                if (szBuf.IsEmpty())
                {
                    szBuf.LoadStringW(IDS_SEARCH_TEXT);
                    bSearchEnabled = FALSE;
                    m_SearchBar->SetWindowTextW(szBuf.GetString());
                }
            }
            break;

            case EN_CHANGE:
            {
                ATL::CStringW szWndText;

                if (!bSearchEnabled)
                {
                    bSearchEnabled = TRUE;
                    break;
                }

                szBuf.LoadStringW(IDS_SEARCH_TEXT);
                m_SearchBar->GetWindowTextW(szWndText);
                if (szBuf == szWndText)
                {
                    szSearchPattern.Empty();
                }
                else
                {
                    szSearchPattern = szWndText;
                }

                DWORD dwDelay;
                SystemParametersInfoW(SPI_GETMENUSHOWDELAY, 0, &dwDelay, 0);
                SetTimer(SEARCH_TIMER_ID, dwDelay);
            }
            break;
            }

            return;
        }

        switch (wCommand)
        {
        case ID_OPEN_LINK:
            ShellExecuteW(m_hWnd, L"open", pLink, NULL, NULL, SW_SHOWNOACTIVATE);
            HeapFree(GetProcessHeap(), 0, pLink);
            break;

        case ID_COPY_LINK:
            CopyTextToClipboard(pLink);
            HeapFree(GetProcessHeap(), 0, pLink);
            break;

        case ID_SETTINGS:
            CreateSettingsDlg(m_hWnd);
            break;

        case ID_EXIT:
            PostMessageW(WM_CLOSE, 0, 0);
            break;

        case ID_SEARCH:
            ::SetFocus(m_SearchBar->m_hWnd);
            break;

        case ID_INSTALL:
            if (IsAvailableEnum(SelectedEnumType))
            {
                if (nSelectedApps > 0)
                {
                    CDownloadManager::DownloadListOfApplications(m_AvailableApps.GetSelected());
                    UpdateApplicationsList(-1);
                    m_ListView->SetSelected(-1, FALSE);
                }
                else if (CDownloadManager::DownloadApplication(m_ListView->GetSelectedData()))
                {
                    UpdateApplicationsList(-1);
                }

            }
            break;

        case ID_UNINSTALL:
            if (UninstallApplication(-1, FALSE))
                UpdateApplicationsList(-1);
            break;

        case ID_MODIFY:
            if (UninstallApplication(-1, TRUE))
                UpdateApplicationsList(-1);
            break;

        case ID_REGREMOVE:
            RemoveAppFromRegistry(-1);
            break;

        case ID_REFRESH:
            UpdateApplicationsList(-1);
            break;

        case ID_RESETDB:
            CAvailableApps::ForceUpdateAppsDB();
            UpdateApplicationsList(-1);
            break;

        case ID_HELP:
            MessageBoxW(L"Help not implemented yet", NULL, MB_OK);
            break;

        case ID_ABOUT:
            ShowAboutDialog();
            break;

        case ID_CHECK_ALL:
            m_ListView->CheckAll();
            break;
        }
    }

    VOID FreeInstalledAppList()
    {
        INT Count = m_ListView->GetItemCount() - 1;
        PINSTALLED_INFO Info;

        while (Count >= 0)
        {
            Info = (PINSTALLED_INFO) ListViewGetlParam(Count);
            if (Info)
            {
                RegCloseKey(Info->hSubKey);
                delete Info;
            }
            Count--;
        }
    }

    static BOOL SearchPatternMatch(LPCWSTR szHaystack, LPCWSTR szNeedle)
    {
        if (!*szNeedle)
            return TRUE;
        /* TODO: Improve pattern search beyond a simple case-insensitive substring search. */
        return StrStrIW(szHaystack, szNeedle) != NULL;
    }

    static BOOL CALLBACK s_EnumInstalledAppProc(INT ItemIndex, ATL::CStringW &m_szName, PINSTALLED_INFO Info)
    {
        PINSTALLED_INFO ItemInfo;
        ATL::CStringW szText;
        INT Index;

        if (!SearchPatternMatch(m_szName.GetString(), szSearchPattern))
        {
            RegCloseKey(Info->hSubKey);
            return TRUE;
        }

        ItemInfo = new INSTALLED_INFO(*Info);
        if (!ItemInfo)
        {
            RegCloseKey(Info->hSubKey);
            return FALSE;
        }

        Index = ListViewAddItem(ItemIndex, 0, m_szName, (LPARAM) ItemInfo);

        /* Get version info */
        GetApplicationString(ItemInfo->hSubKey, L"DisplayVersion", szText);
        ListView_SetItemText(hListView, Index, 1, const_cast<LPWSTR>(szText.GetString()));

        /* Get comments */
        GetApplicationString(ItemInfo->hSubKey, L"Comments", szText);
        ListView_SetItemText(hListView, Index, 2, const_cast<LPWSTR>(szText.GetString()));

        return TRUE;
    }

    static BOOL CALLBACK s_EnumAvailableAppProc(CAvailableApplicationInfo* Info, LPCWSTR szFolderPath)
    {
        INT Index;
        HICON hIcon = NULL;

        HIMAGELIST hImageListView = ListView_GetImageList(hListView, LVSIL_SMALL);

        if (!SearchPatternMatch(Info->m_szName.GetString(), szSearchPattern) &&
            !SearchPatternMatch(Info->m_szDesc.GetString(), szSearchPattern))
        {
            return TRUE;
        }

        /* Load icon from file */
        ATL::CStringW szIconPath;
        szIconPath.Format(L"%lsicons\\%ls.ico", szFolderPath, Info->m_szName.GetString());
        hIcon = (HICON) LoadImageW(NULL,
                                   szIconPath.GetString(),
                                   IMAGE_ICON,
                                   LISTVIEW_ICON_SIZE,
                                   LISTVIEW_ICON_SIZE,
                                   LR_LOADFROMFILE);

        if (!hIcon || GetLastError() != ERROR_SUCCESS)
        {
            /* Load default icon */
            hIcon = (HICON) LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
        }

        Index = ImageList_AddIcon(hImageListView, hIcon);
        DestroyIcon(hIcon);

        Index = ListViewAddItem(Info->m_Category, Index, Info->m_szName.GetString(), (LPARAM) Info);
        ListView_SetImageList(hListView, hImageListView, LVSIL_SMALL);

        ListView_SetItemText(hListView, Index, 1, const_cast<LPWSTR>(Info->m_szVersion.GetString()));
        ListView_SetItemText(hListView, Index, 2, const_cast<LPWSTR>(Info->m_szDesc.GetString()));
        ListView_SetCheckState(hListView, Index, Info->m_IsSelected);

        return TRUE;
    }

    VOID UpdateStatusBarText()
    {
        if (m_StatusBar)
        {
            ATL::CStringW szBuffer;

            szBuffer.Format(IDS_APPS_COUNT, m_ListView->GetItemCount(), nSelectedApps);
            m_StatusBar->SetText(szBuffer);
        }
    }

    VOID UpdateApplicationsList(INT EnumType)
    {
        ATL::CStringW szBuffer1, szBuffer2;
        HIMAGELIST hImageListView;
        BOOL bWasInInstalled = IsInstalledEnum(SelectedEnumType);

        bUpdating = TRUE;
        m_ListView->SetRedraw(FALSE);

        if (EnumType < 0)
        {
            EnumType = SelectedEnumType;
        }

        //if previous one was INSTALLED purge the list
        //TODO: make the Installed category a separate class to avoid doing this
        if (bWasInInstalled)
        {
            FreeInstalledAppList();
        }

        m_ListView->DeleteAllItems();

        // Create new ImageList
        hImageListView = ImageList_Create(LISTVIEW_ICON_SIZE,
                                          LISTVIEW_ICON_SIZE,
                                          GetSystemColorDepth() | ILC_MASK,
                                          0, 1);
        HIMAGELIST hImageListBuf = m_ListView->SetImageList(hImageListView, LVSIL_SMALL);
        if (hImageListBuf)
        {
            ImageList_Destroy(hImageListBuf);
        }

        if (IsInstalledEnum(EnumType))
        {
            if (!bWasInInstalled)
            {
                m_ListView->SetCheckboxesVisible(FALSE);
            }

            HICON hIcon = (HICON) LoadIconW(hInst, MAKEINTRESOURCEW(IDI_MAIN));
            ImageList_AddIcon(hImageListView, hIcon);
            DestroyIcon(hIcon);

            // Enum installed applications and updates
            EnumInstalledApplications(EnumType, TRUE, s_EnumInstalledAppProc);
            EnumInstalledApplications(EnumType, FALSE, s_EnumInstalledAppProc);
        }
        else if (IsAvailableEnum(EnumType))
        {
            if (bWasInInstalled)
            {
                m_ListView->SetCheckboxesVisible(TRUE);
            }

            // Enum available applications
            m_AvailableApps.Enum(EnumType, s_EnumAvailableAppProc);
        }

        SelectedEnumType = EnumType;
        UpdateStatusBarText();
        SetWelcomeText();

        // Set automatic column width for program names if the list is not empty
        if (m_ListView->GetItemCount() > 0)
        {
            ListView_SetColumnWidth(m_ListView->GetWindow(), 0, LVSCW_AUTOSIZE);
        }

        bUpdating = FALSE;
        m_ListView->SetRedraw(TRUE);
    }

public:
    static ATL::CWndClassInfo& GetWndClassInfo()
    {
        DWORD csStyle = CS_VREDRAW | CS_HREDRAW;
        static ATL::CWndClassInfo wc =
        {
            {
                sizeof(WNDCLASSEX),
                csStyle,
                StartWindowProc,
                0,
                0,
                NULL,
                LoadIconW(_AtlBaseModule.GetModuleInstance(), MAKEINTRESOURCEW(IDI_MAIN)),
                LoadCursorW(NULL, IDC_ARROW),
                (HBRUSH) (COLOR_BTNFACE + 1),
                MAKEINTRESOURCEW(IDR_MAINMENU),
                L"RAppsWnd",
                NULL
            },
            NULL, NULL, IDC_ARROW, TRUE, 0, _T("")
        };
        return wc;
    }

    HWND Create()
    {
        ATL::CStringW szWindowName;
        szWindowName.LoadStringW(IDS_APPTITLE);

        RECT r = {
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Left : CW_USEDEFAULT),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Top : CW_USEDEFAULT),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Width : 680),
            (SettingsInfo.bSaveWndPos ? SettingsInfo.Height : 450)
        };
        r.right += r.left;
        r.bottom += r.top;

        return CWindowImpl::Create(NULL, r, szWindowName.GetString(), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_WINDOWEDGE);
    }

    CStatusBar * GetStatusBar()
    {
        return m_StatusBar;
    }

    CAppsListView * GetListView()
    {
        return m_ListView;
    }

    CRichEdit * GetRichEdit()
    {
        return m_RichEdit;
    }

    CAvailableApps * GetAvailableApps()
    {
        return &m_AvailableApps;
    }
};

// global interface
CMainWindow * g_MainWindow;

HWND CreateMainWindow()
{
    g_MainWindow = new CMainWindow();
    return g_MainWindow->Create();
}

DWORD_PTR ListViewGetlParam(INT item)
{
    if (item < 0)
    {
        item = g_MainWindow->GetListView()->GetSelectionMark();
    }
    return g_MainWindow->GetListView()->GetItemData(item);
}

VOID SetStatusBarText(LPCWSTR szText)
{
    g_MainWindow->GetStatusBar()->SetText(szText);
}

INT ListViewAddItem(INT ItemIndex, INT IconIndex, LPWSTR lpName, LPARAM lParam)
{
    return g_MainWindow->GetListView()->AddItem(ItemIndex, IconIndex, lpName, lParam);
}

VOID NewRichEditText(LPCWSTR szText, DWORD flags)
{
    g_MainWindow->GetRichEdit()->SetText(szText, flags);
}

VOID InsertRichEditText(LPCWSTR szText, DWORD flags)
{
    g_MainWindow->GetRichEdit()->InsertText(szText, flags);
}

CAvailableApps* GetAvailableApps()
{
    return g_MainWindow->GetAvailableApps();
}

// ATL version of functions above
VOID SetStatusBarText(const ATL::CStringW& szText)
{
    SetStatusBarText(szText.GetString());
}

INT ListViewAddItem(INT ItemIndex, INT IconIndex, const ATL::CStringW& Name, LPARAM lParam)
{
    return ListViewAddItem(ItemIndex, IconIndex, const_cast<LPWSTR>(Name.GetString()), lParam);
}

VOID NewRichEditText(const ATL::CStringW& szText, DWORD flags)
{
    NewRichEditText(szText.GetString(), flags);
}

VOID InsertRichEditText(const ATL::CStringW& szText, DWORD flags)
{
    InsertRichEditText(szText.GetString(), flags);
}
