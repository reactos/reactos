/*
 * PROJECT:         ReactOS Applications Manager
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            base/applications/rapps/toolbar.c
 * PURPOSE:         ToolBar functions
 * PROGRAMMERS:     Dmitry Chapyshev (dmitry@reactos.org)
 */

#include "rapps.h"

#define TOOLBAR_HEIGHT 24

HWND hToolBar;
HWND hSearchBar;

static WCHAR szInstallBtn[MAX_STR_LEN];
static WCHAR szUninstallBtn[MAX_STR_LEN];
static WCHAR szModifyBtn[MAX_STR_LEN];

/* Toolbar buttons */
static const TBBUTTON Buttons[] =
{   /* iBitmap, idCommand, fsState, fsStyle, bReserved[2], dwData, iString */
    { 0, ID_INSTALL,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)szInstallBtn},
    { 1, ID_UNINSTALL, TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)szUninstallBtn},
    { 2, ID_MODIFY,    TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, (INT_PTR)szModifyBtn},
    { 5, 0,            TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
    { 3, ID_REFRESH,   TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, 0},
    { 5, 0,            TBSTATE_ENABLED, BTNS_SEP, {0}, 0, 0},
    { 4, ID_SETTINGS,  TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, 0},
    { 5, ID_EXIT,      TBSTATE_ENABLED, BTNS_BUTTON | BTNS_AUTOSIZE, {0}, 0, 0}
};


VOID
ToolBarOnGetDispInfo(LPTOOLTIPTEXT lpttt)
{
    UINT idButton = (UINT)lpttt->hdr.idFrom;

    switch (idButton)
    {
        case ID_EXIT:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_EXIT);
            break;

        case ID_INSTALL:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_INSTALL);
            break;

        case ID_UNINSTALL:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_UNINSTALL);
            break;

        case ID_MODIFY:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_MODIFY);
            break;

        case ID_SETTINGS:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_SETTINGS);
            break;

        case ID_REFRESH:
            lpttt->lpszText = MAKEINTRESOURCE(IDS_TOOLTIP_REFRESH);
            break;
    }
}

VOID
AddImageToImageList(HIMAGELIST hImageList, UINT ImageIndex)
{
    HANDLE hImage;

    if (!(hImage = LoadImage(hInst,
                             MAKEINTRESOURCE(ImageIndex),
                             IMAGE_ICON,
                             TOOLBAR_HEIGHT,
                             TOOLBAR_HEIGHT,
                             0)))
    {
        /* TODO: Error message */
    }

    ImageList_AddIcon(hImageList, hImage);
    DeleteObject(hImage);
}

HIMAGELIST
InitImageList(VOID)
{
    HIMAGELIST hImageList;

    /* Create the toolbar icon image list */
    hImageList = ImageList_Create(TOOLBAR_HEIGHT,//GetSystemMetrics(SM_CXSMICON),
                                  TOOLBAR_HEIGHT,//GetSystemMetrics(SM_CYSMICON),
                                  ILC_MASK | GetSystemColorDepth(),
                                  1,
                                  1);
    if (!hImageList)
    {
        /* TODO: Error message */
        return NULL;
    }

    AddImageToImageList(hImageList, IDI_INSTALL);
    AddImageToImageList(hImageList, IDI_UNINSTALL);
    AddImageToImageList(hImageList, IDI_MODIFY);
    AddImageToImageList(hImageList, IDI_REFRESH);
    AddImageToImageList(hImageList, IDI_SETTINGS);
    AddImageToImageList(hImageList, IDI_EXIT);

    return hImageList;
}

static
BOOL
CreateSearchBar(VOID)
{
    WCHAR szBuf[MAX_STR_LEN];

    hSearchBar = CreateWindowExW(WS_EX_CLIENTEDGE,
                                 L"Edit",
                                 NULL,
                                 WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL, 
                                 0,
                                 5,
                                 200,
                                 22,
                                 hToolBar,
                                 (HMENU)0,
                                 hInst,
                                 0);

    SendMessageW(hSearchBar, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), 0);

    LoadStringW(hInst, IDS_SEARCH_TEXT, szBuf, sizeof(szBuf) / sizeof(WCHAR));
    SetWindowTextW(hSearchBar, szBuf);

    SetParent(hSearchBar, hToolBar); 

    return TRUE;
}

BOOL
CreateToolBar(HWND hwnd)
{
    INT NumButtons = sizeof(Buttons) / sizeof(Buttons[0]);
    HIMAGELIST hImageList;

    LoadStringW(hInst, IDS_INSTALL, szInstallBtn, sizeof(szInstallBtn) / sizeof(WCHAR));
    LoadStringW(hInst, IDS_UNINSTALL, szUninstallBtn, sizeof(szUninstallBtn) / sizeof(WCHAR));
    LoadStringW(hInst, IDS_MODIFY, szModifyBtn, sizeof(szModifyBtn) / sizeof(WCHAR));

    hToolBar = CreateWindowExW(0,
                               TOOLBARCLASSNAMEW,
                               NULL,
                               WS_CHILD | WS_VISIBLE | TBSTYLE_FLAT | TBSTYLE_TOOLTIPS | TBSTYLE_LIST,
                               0, 0, 0, 0,
                               hwnd,
                               0,
                               hInst,
                               NULL);

    if (!hToolBar)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    SendMessageW(hToolBar, TB_SETEXTENDEDSTYLE, 0, TBSTYLE_EX_HIDECLIPPEDBUTTONS);
    SendMessageW(hToolBar, TB_BUTTONSTRUCTSIZE, sizeof(Buttons[0]), 0);

    hImageList = InitImageList();

    if (!hImageList)
    {
        /* TODO: Show error message */
        return FALSE;
    }

    ImageList_Destroy((HIMAGELIST)SendMessageW(hToolBar,
                                               TB_SETIMAGELIST,
                                               0,
                                               (LPARAM)hImageList));

    SendMessageW(hToolBar, TB_ADDBUTTONS, NumButtons, (LPARAM)Buttons);

    CreateSearchBar();

    return TRUE;
}
