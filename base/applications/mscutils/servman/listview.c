/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/listview.c
 * PURPOSE:     service listview manipulation functions
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"

typedef struct _COLUMN_LIST
{
    int  iSubItem;
    int  cx;
    UINT idsText;
} COLUMN_LIST;

static const COLUMN_LIST Columns[] =
{
    /* name */
    { LVNAME,    150, IDS_FIRSTCOLUMN  },
    /* description */
    { LVDESC,    240, IDS_SECONDCOLUMN },
    /* status */
    { LVSTATUS,   55, IDS_THIRDCOLUMN  },
    /* startup type */
    { LVSTARTUP,  80, IDS_FOURTHCOLUMN },
    /* logon as */
    { LVLOGONAS, 100, IDS_FITHCOLUMN   },
};

VOID
SetListViewStyle(HWND hListView,
                 DWORD View)
{
    DWORD Style = GetWindowLongPtr(hListView, GWL_STYLE);

    if ((Style & LVS_TYPEMASK) != View)
    {
        SetWindowLongPtr(hListView,
                         GWL_STYLE,
                         (Style & ~LVS_TYPEMASK) | View);
    }
}

VOID
ListViewSelectionChanged(PMAIN_WND_INFO Info,
                         LPNMLISTVIEW pnmv)
{
    HMENU hMainMenu;

    /* get handle to menu */
    hMainMenu = GetMenu(Info->hMainWnd);

    /* activate properties menu item, if not already */
    if (GetMenuState(hMainMenu,
                     ID_PROP,
                     MF_BYCOMMAND) != MF_ENABLED)
    {
        EnableMenuItem(hMainMenu,
                       ID_PROP,
                       MF_ENABLED);
        EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0),
                       ID_PROP,
                       MF_ENABLED);
        SetMenuDefaultItem(GetSubMenu(Info->hShortcutMenu, 0),
                           ID_PROP,
                           MF_BYCOMMAND);
    }

    /* activate delete menu item, if not already */
    if (GetMenuState(hMainMenu,
                     ID_DELETE,
                     MF_BYCOMMAND) != MF_ENABLED)
    {
        EnableMenuItem(hMainMenu,
                       ID_DELETE,
                       MF_ENABLED);
        EnableMenuItem(GetSubMenu(Info->hShortcutMenu, 0),
                       ID_DELETE,
                       MF_ENABLED);
    }

    /* set selected service */
    Info->SelectedItem = pnmv->iItem;

    /* get pointer to selected service */
    Info->pCurrentService = GetSelectedService(Info);

    /* set current selected service in the status bar */
    SendMessage(Info->hStatus,
                SB_SETTEXT,
                1,
                (LPARAM)Info->pCurrentService->lpDisplayName);

    /* show the properties button */
    SendMessage(Info->hTool,
                TB_SETSTATE,
                ID_PROP,
                (LPARAM)MAKELONG(TBSTATE_ENABLED, 0));
}

VOID
ChangeListViewText(PMAIN_WND_INFO Info,
                   ENUM_SERVICE_STATUS_PROCESS* pService,
                   UINT Column)
{
    LVFINDINFO lvfi;
    LVITEM lvItem;
    INT index;

    lvfi.flags = LVFI_PARAM;
    lvfi.lParam = (LPARAM)pService;
    index = ListView_FindItem(Info->hListView,
                              -1,
                              &lvfi);
    if (index != -1)
    {
        lvItem.iItem = index;
        lvItem.iSubItem = Column;

        switch (Column)
        {
            case LVNAME:
            {
                LPQUERY_SERVICE_CONFIG lpServiceConfig;

                lpServiceConfig = GetServiceConfig(pService->lpServiceName);
                if (lpServiceConfig)
                {
                    lvItem.pszText = lpServiceConfig->lpDisplayName;
                    SendMessage(Info->hListView,
                                LVM_SETITEMTEXT,
                                lvItem.iItem,
                                (LPARAM)&lvItem);

                    HeapFree(ProcessHeap,
                             0,
                             lpServiceConfig);
                }
            }
            break;

            case LVDESC:
            {
                LPWSTR lpDescription;

                lpDescription = GetServiceDescription(pService->lpServiceName);

                lvItem.pszText = lpDescription;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXTW,
                            lvItem.iItem,
                            (LPARAM) &lvItem);

                HeapFree(ProcessHeap,
                         0,
                         lpDescription);
            }
            break;

            case LVSTATUS:
            {
                WCHAR szStatus[64];

                if (pService->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
                {
                    LoadStringW(hInstance,
                                IDS_SERVICES_STARTED,
                                szStatus,
                                sizeof(szStatus) / sizeof(WCHAR));
                }
                else
                {
                    szStatus[0] = 0;
                }

                lvItem.pszText = szStatus;
                SendMessageW(Info->hListView,
                             LVM_SETITEMTEXT,
                             lvItem.iItem,
                             (LPARAM) &lvItem);
            }
            break;

            case LVSTARTUP:
            {
                LPQUERY_SERVICE_CONFIGW lpServiceConfig;
                LPWSTR lpStartup = NULL;
                DWORD StringId = 0;

                lpServiceConfig = GetServiceConfig(pService->lpServiceName);

                if (lpServiceConfig)
                {
                    switch (lpServiceConfig->dwStartType)
                    {
                        case 2: StringId = IDS_SERVICES_AUTO; break;
                        case 3: StringId = IDS_SERVICES_MAN; break;
                        case 4: StringId = IDS_SERVICES_DIS; break;
                    }
                }

                if (StringId)
                    AllocAndLoadString(&lpStartup,
                                       hInstance,
                                       StringId);

                lvItem.pszText = lpStartup;
                SendMessageW(Info->hListView,
                             LVM_SETITEMTEXTW,
                             lvItem.iItem,
                             (LPARAM)&lvItem);

                LocalFree(lpStartup);
                HeapFree(ProcessHeap,
                         0,
                         lpServiceConfig);
            }
            break;

            case LVLOGONAS:
            {
                LPQUERY_SERVICE_CONFIG lpServiceConfig;

                lpServiceConfig = GetServiceConfig(pService->lpServiceName);
                if (lpServiceConfig)
                {
                    lvItem.pszText = lpServiceConfig->lpServiceStartName;
                    SendMessageW(Info->hListView,
                                 LVM_SETITEMTEXT,
                                 lvItem.iItem,
                                 (LPARAM)&lvItem);

                    HeapFree(ProcessHeap,
                             0,
                             lpServiceConfig);
                }
            }
            break;
        }
    }
}

BOOL
RefreshServiceList(PMAIN_WND_INFO Info)
{
    ENUM_SERVICE_STATUS_PROCESS *pService;
    LVITEMW lvItem;
    DWORD Index;

    SendMessage (Info->hListView,
                 WM_SETREDRAW,
                 FALSE,
                 0);

    (void)ListView_DeleteAllItems(Info->hListView);

    if (GetServiceList(Info))
    {
        for (Index = 0; Index < Info->NumServices; Index++)
        {
            INT i;

            pService = &Info->pAllServices[Index];

            /* set the display name */
            ZeroMemory(&lvItem, sizeof(LVITEMW));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.pszText = pService->lpDisplayName;

            /* Add the service pointer */
            lvItem.lParam = (LPARAM)pService;

            /* add it to the listview */
            lvItem.iItem = ListView_InsertItem(Info->hListView, &lvItem);

            /* fill out all the column data */
            for (i = LVDESC; i <= LVLOGONAS; i++)
            {
                ChangeListViewText(Info, pService, i);
            }
        }

        UpdateServiceCount(Info);
    }

    /* turn redraw flag on. */
    SendMessageW(Info->hListView,
                 WM_SETREDRAW,
                 TRUE,
                 0);

    return TRUE;
}

static VOID
InitListViewImage(PMAIN_WND_INFO Info)
{
    HICON hSmIconItem, hLgIconItem;
    HIMAGELIST hSmall, hLarge;

    hSmall = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                              GetSystemMetrics(SM_CYSMICON),
                              ILC_MASK | ILC_COLOR32,
                              1,
                              1);
    if (hSmall)
    {
        hSmIconItem = LoadImageW(hInstance,
                                 MAKEINTRESOURCE(IDI_SM_ICON),
                                 IMAGE_ICON,
                                 16,
                                 16,
                                 0);
        if (hSmIconItem)
        {
            ImageList_AddIcon(hSmall,
                              hSmIconItem);
            (void)ListView_SetImageList(Info->hListView,
                                        hSmall,
                                        LVSIL_SMALL);

            DestroyIcon(hSmIconItem);
        }
    }

    hLarge = ImageList_Create(GetSystemMetrics(SM_CXICON),
                              GetSystemMetrics(SM_CYICON),
                              ILC_MASK | ILC_COLOR32,
                              1,
                              1);
    if (hLarge)
    {
        hLgIconItem = LoadImageW(hInstance,
                                 MAKEINTRESOURCE(IDI_SM_ICON),
                                 IMAGE_ICON,
                                 32,
                                 32,
                                 0);
        if (hLgIconItem)
        {
            ImageList_AddIcon(hLarge,
                              hLgIconItem);
            (void)ListView_SetImageList(Info->hListView,
                                        hLarge,
                                        LVSIL_NORMAL);
            DestroyIcon(hLgIconItem);
        }
    }
}

BOOL
CreateListView(PMAIN_WND_INFO Info)
{
    LVCOLUMNW lvc = { 0 };
    WCHAR szTemp[256];
    HDITEM hdi;
    int i, n;

    Info->hListView = CreateWindowExW(WS_EX_CLIENTEDGE,
                                      WC_LISTVIEWW,
                                      NULL,
                                      WS_CHILD | WS_VISIBLE | LVS_REPORT | WS_BORDER |
                                         LBS_NOTIFY | LVS_SORTASCENDING | LBS_NOREDRAW,
                                      0, 0, 0, 0,
                                      Info->hMainWnd,
                                      (HMENU) IDC_SERVLIST,
                                      hInstance,
                                      NULL);
    if (Info->hListView == NULL)
    {
        MessageBoxW(Info->hMainWnd,
                    L"Could not create List View.",
                    L"Error",
                    MB_OK | MB_ICONERROR);
        return FALSE;
    }

    Info->hHeader = ListView_GetHeader(Info->hListView);

    (void)ListView_SetExtendedListViewStyle(Info->hListView,
                                            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);/*LVS_EX_GRIDLINES |*/

    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
    lvc.fmt  = LVCFMT_LEFT;
    lvc.pszText = szTemp;

    /* Add columns to the list-view */
    for (n = 0; n < sizeof(Columns) / sizeof(Columns[0]); n++)
    {
        lvc.iSubItem = Columns[n].iSubItem;
        lvc.cx = Columns[n].cx;

        LoadStringW(hInstance,
                    Columns[n].idsText,
                    szTemp,
                    sizeof(szTemp) / sizeof(szTemp[0]));

        i = ListView_InsertColumn(Info->hListView, Columns[n].iSubItem, &lvc);

        hdi.mask = HDI_LPARAM;
        hdi.lParam = ORD_ASCENDING;
        (void)Header_SetItem(Info->hHeader, i, &hdi);
    }

    InitListViewImage(Info);

    /* check the details view menu item */
    CheckMenuRadioItem(GetMenu(Info->hMainWnd),
                       ID_VIEW_LARGE,
                       ID_VIEW_DETAILS,
                       ID_VIEW_DETAILS,
                       MF_BYCOMMAND);

    return TRUE;
}
