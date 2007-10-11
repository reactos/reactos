/*
 * PROJECT:     ReactOS Services
 * LICENSE:     GPL - See COPYING in the top level directory
 * FILE:        base/applications/mscutils/servman/listview.c
 * PURPOSE:     service listview manipulation functions
 * COPYRIGHT:   Copyright 2006-2007 Ged Murphy <gedmurphy@reactos.org>
 *
 */

#include "precomp.h"


VOID
SetListViewStyle(HWND hListView,
                 DWORD View)
{
    DWORD Style = GetWindowLong(hListView, GWL_STYLE);

    if ((Style & LVS_TYPEMASK) != View)
    {
        SetWindowLong(hListView,
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
                LPTSTR lpDescription;

                lpDescription = GetServiceDescription(pService->lpServiceName);

                lvItem.pszText = lpDescription;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM) &lvItem);

                HeapFree(ProcessHeap,
                         0,
                         lpDescription);
            }
            break;

            case LVSTATUS:
            {
                TCHAR szStatus[64];

                if (pService->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
                {
                    LoadString(hInstance,
                               IDS_SERVICES_STARTED,
                               szStatus,
                               sizeof(szStatus) / sizeof(TCHAR));
                }
                else
                {
                    szStatus[0] = 0;
                }

                lvItem.pszText = szStatus;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM) &lvItem);
            }
            break;

            case LVSTARTUP:
            {
                LPQUERY_SERVICE_CONFIG lpServiceConfig;
                LPTSTR lpStartup = NULL;
                DWORD StringId = 0;

                lpServiceConfig = GetServiceConfig(pService->lpServiceName);

                switch (lpServiceConfig->dwStartType)
                {
                    case 2: StringId = IDS_SERVICES_AUTO; break;
                    case 3: StringId = IDS_SERVICES_MAN; break;
                    case 4: StringId = IDS_SERVICES_DIS; break;
                }

                if (StringId)
                    AllocAndLoadString(&lpStartup,
                                       hInstance,
                                       StringId);

                lvItem.pszText = lpStartup;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);

                HeapFree(ProcessHeap,
                         0,
                         lpStartup);
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
        }
    }
}


BOOL
RefreshServiceList(PMAIN_WND_INFO Info)
{
    ENUM_SERVICE_STATUS_PROCESS *pService;
    LPTSTR lpDescription;
    LVITEM lvItem;
    TCHAR szStatus[64];
    DWORD NumServices;
    DWORD Index;

    SendMessage (Info->hListView,
                 WM_SETREDRAW,
                 FALSE,
                 0);

    (void)ListView_DeleteAllItems(Info->hListView);

    if (GetServiceList(Info, &NumServices))
    {
        for (Index = 0; Index < NumServices; Index++)
        {
            LPQUERY_SERVICE_CONFIG pServiceConfig;

            pService = &Info->pAllServices[Index];

            /* set the display name */
            ZeroMemory(&lvItem, sizeof(LVITEM));
            lvItem.mask = LVIF_TEXT | LVIF_PARAM;
            lvItem.pszText = pService->lpDisplayName;

            /* Add the service pointer */
            lvItem.lParam = (LPARAM)pService;

            /* add it to the listview */
            lvItem.iItem = ListView_InsertItem(Info->hListView, &lvItem);

            /* set the description */
            if ((lpDescription = GetServiceDescription(pService->lpServiceName)))
            {
                lvItem.pszText = lpDescription;
                lvItem.iSubItem = LVDESC;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);

                HeapFree(ProcessHeap,
                         0,
                         lpDescription);
            }

            /* set the status */
            if (pService->ServiceStatusProcess.dwCurrentState == SERVICE_RUNNING)
            {
                LoadString(hInstance,
                           IDS_SERVICES_STARTED,
                           szStatus,
                           sizeof(szStatus) / sizeof(TCHAR));
                lvItem.pszText = szStatus;
                lvItem.iSubItem = LVSTATUS;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);
            }

            pServiceConfig = GetServiceConfig(pService->lpServiceName);
            if (pServiceConfig)
            {
                DWORD StringId = 0;
                LPTSTR lpStartup;

                /* set the startup type */
                switch (pServiceConfig->dwStartType)
                {
                    case 2: StringId = IDS_SERVICES_AUTO; break;
                    case 3: StringId = IDS_SERVICES_MAN; break;
                    case 4: StringId = IDS_SERVICES_DIS; break;
                }

                if (StringId)
                    AllocAndLoadString(&lpStartup,
                                       hInstance,
                                       StringId);

                lvItem.pszText = lpStartup;
                lvItem.iSubItem = LVSTARTUP;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);

                LocalFree(lpStartup);

                /* set Log On As */
                lvItem.pszText = pServiceConfig->lpServiceStartName;
                lvItem.iSubItem = LVLOGONAS;
                SendMessage(Info->hListView,
                            LVM_SETITEMTEXT,
                            lvItem.iItem,
                            (LPARAM)&lvItem);

                HeapFree(ProcessHeap,
                         0,
                         pServiceConfig);
            }
        }

        UpdateServiceCount(Info);
    }

    /* turn redraw flag on. */
    SendMessage (Info->hListView,
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
        hSmIconItem = LoadImage(hInstance,
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
        hLgIconItem = LoadImage(hInstance,
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
    LVCOLUMN lvc = { 0 };
    TCHAR szTemp[256];

    Info->hListView = CreateWindowEx(WS_EX_CLIENTEDGE,
                                     WC_LISTVIEW,
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
        MessageBox(Info->hMainWnd,
                   _T("Could not create List View."),
                   _T("Error"),
                   MB_OK | MB_ICONERROR);
        return FALSE;
    }

    (void)ListView_SetExtendedListViewStyle(Info->hListView,
                                            LVS_EX_FULLROWSELECT | LVS_EX_HEADERDRAGDROP);/*LVS_EX_GRIDLINES |*/

    lvc.mask = LVCF_TEXT | LVCF_SUBITEM | LVCF_WIDTH  | LVCF_FMT;
    lvc.fmt  = LVCFMT_LEFT;

    /* Add columns to the list-view */
    /* name */
    lvc.iSubItem = LVNAME;
    lvc.cx       = 150;
    LoadString(hInstance,
               IDS_FIRSTCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                0,
                                &lvc);

    /* description */
    lvc.iSubItem = LVDESC;
    lvc.cx       = 240;
    LoadString(hInstance,
               IDS_SECONDCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                1,
                                &lvc);

    /* status */
    lvc.iSubItem = LVSTATUS;
    lvc.cx       = 55;
    LoadString(hInstance,
               IDS_THIRDCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                2,
                                &lvc);

    /* startup type */
    lvc.iSubItem = LVSTARTUP;
    lvc.cx       = 80;
    LoadString(hInstance,
               IDS_FOURTHCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                3,
                                &lvc);

    /* logon as */
    lvc.iSubItem = LVLOGONAS;
    lvc.cx       = 100;
    LoadString(hInstance,
               IDS_FITHCOLUMN,
               szTemp,
               sizeof(szTemp) / sizeof(TCHAR));
    lvc.pszText  = szTemp;
    (void)ListView_InsertColumn(Info->hListView,
                                4,
                                &lvc);

    InitListViewImage(Info);

    /* check the details view menu item */
    CheckMenuRadioItem(GetMenu(Info->hMainWnd),
                       ID_VIEW_LARGE,
                       ID_VIEW_DETAILS,
                       ID_VIEW_DETAILS,
                       MF_BYCOMMAND);

    return TRUE;
}
