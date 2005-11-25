/*
 * ReactOS Device Manager Applet
 * Copyright (C) 2004 - 2005 ReactOS Team
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
/* $Id$
 *
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/hwpage.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */
#include <precomp.h>

#define NDEBUG
#include <debug.h>

typedef VOID (WINAPI *PINITCOMMONCONTROLS)(VOID);

typedef enum
{
    HWPD_STANDARDLIST = 0,
    HWPD_LARGELIST,
    HWPD_MAX = HWPD_LARGELIST
} HWPAGE_DISPLAYMODE, *PHWPAGE_DISPLAYMODE;

typedef struct _HWDEVINFO
{
    struct _HWCLASSDEVINFO *ClassDevInfo;
    SP_DEVINFO_DATA DevInfoData;
} HWDEVINFO, *PHWDEVINFO;

typedef struct _HWCLASSDEVINFO
{
    GUID Guid;
    HDEVINFO hDevInfo;
    INT ItemCount;
    PHWDEVINFO HwDevInfo;
} HWCLASSDEVINFO, *PHWCLASSDEVINFO;

typedef struct _HARDWARE_PAGE_DATA
{
    HWND hWnd;
    HWND hWndDevList;
    HINSTANCE hComCtl32; /* only save this to keep track of the references */
    INT DevListViewHeight;
    SP_CLASSIMAGELIST_DATA ClassImageListData;
    HWPAGE_DISPLAYMODE DisplayMode;

    /* parent window subclass info */
    WNDPROC ParentOldWndProc;
    HWND hWndParent;

    UINT NumberOfGuids;
    HWCLASSDEVINFO ClassDevInfo[1];
    /* struct may be dynamically expanded here! */
} HARDWARE_PAGE_DATA, *PHARDWARE_PAGE_DATA;

#define CX_TYPECOLUMN_WIDTH 80

static VOID
InitializeDevicesList(IN PHARDWARE_PAGE_DATA hpd)
{
    LVCOLUMN lvc;
    RECT rcClient;
    WCHAR szColName[255];
    int iCol = 0;

    /* set the list view style */
    ListView_SetExtendedListViewStyle(hpd->hWndDevList,
                                      LVS_EX_FULLROWSELECT);

    /* set the list view image list */
    if (hpd->ClassImageListData.ImageList != NULL)
    {
        ListView_SetImageList(hpd->hWndDevList,
                              hpd->ClassImageListData.ImageList,
                              LVSIL_SMALL);
    }

    GetClientRect(hpd->hWndDevList,
                  &rcClient);

    /* add the list view columns */
    lvc.mask = LVCF_TEXT | LVCF_WIDTH;
    lvc.fmt = LVCFMT_LEFT;
    lvc.pszText = szColName;

    if (LoadString(hDllInstance,
                   IDS_NAME,
                   szColName,
                   sizeof(szColName) / sizeof(szColName[0])))
    {
        lvc.cx = rcClient.right - CX_TYPECOLUMN_WIDTH -
                 GetSystemMetrics(SM_CXVSCROLL);
        ListView_InsertColumn(hpd->hWndDevList,
                              iCol++,
                              &lvc);
    }
    if (LoadString(hDllInstance,
                   IDS_TYPE,
                   szColName,
                   sizeof(szColName) / sizeof(szColName[0])))
    {
        lvc.cx = CX_TYPECOLUMN_WIDTH;
        ListView_InsertColumn(hpd->hWndDevList,
                              iCol++,
                              &lvc);
    }
}


static VOID
UpdateControlStates(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWDEVINFO HwDevInfo;

    HwDevInfo = (PHWDEVINFO)ListViewGetSelectedItemData(hpd->hWndDevList);
    if (HwDevInfo != NULL)
    {
        /* FIXME - update static controls and enable buttons */
    }
    else
    {
        /* FIXME - clear static controls and disable buttons */
    }
}


static VOID
FillDevicesList(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWCLASSDEVINFO DevInfo, LastDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    WCHAR szBuffer[255];
    INT ItemCount = 0;

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    DevInfo = hpd->ClassDevInfo;
    LastDevInfo = DevInfo + hpd->NumberOfGuids;

    while (DevInfo != LastDevInfo)
    {
        INT ImageIndex = -1;

        DevInfo->hDevInfo = SetupDiGetClassDevs(&DevInfo->Guid,
                                                NULL,
                                                hpd->hWnd,
                                                DIGCF_PRESENT);
        if (DevInfo->hDevInfo != INVALID_HANDLE_VALUE)
        {
            LVITEM li;
            DWORD MemberIndex = 0;

            SetupDiGetClassImageIndex(&hpd->ClassImageListData,
                                      &DevInfo->Guid,
                                      &ImageIndex);

            while (SetupDiEnumDeviceInfo(DevInfo->hDevInfo,
                                         MemberIndex++,
                                         &DevInfoData))
            {
                DWORD RegDataType;
                INT iItem;

                if (DevInfo->HwDevInfo != NULL)
                {
                    PHWDEVINFO HwNewDevInfo = HeapReAlloc(GetProcessHeap(),
                                                          0,
                                                          DevInfo->HwDevInfo,
                                                          (DevInfo->ItemCount + 1) *
                                                              sizeof(HWDEVINFO));
                    if (HwNewDevInfo != NULL)
                    {
                        DevInfo->HwDevInfo = HwNewDevInfo;
                    }
                    else
                    {
                        DPRINT1("Unable to allocate memory for %d SP_DEVINFO_DATA structures!\n",
                                DevInfo->ItemCount + 1);
                        break;
                    }
                }
                else
                {
                    DevInfo->HwDevInfo = HeapAlloc(GetProcessHeap(),
                                                   0,
                                                   sizeof(HWDEVINFO));
                    if (DevInfo->HwDevInfo == NULL)
                    {
                        DPRINT1("Unable to allocate memory for a SP_DEVINFO_DATA structures!\n");
                        break;
                    }
                }

                DevInfo->HwDevInfo[DevInfo->ItemCount].ClassDevInfo = DevInfo;
                DevInfo->HwDevInfo[DevInfo->ItemCount++].DevInfoData = DevInfoData;

                if ((SetupDiGetDeviceRegistryProperty(DevInfo->hDevInfo,
                                                      &DevInfoData,
                                                      SPDRP_FRIENDLYNAME,
                                                      &RegDataType,
                                                      (PBYTE)szBuffer,
                                                      sizeof(szBuffer),
                                                      NULL) ||
                     SetupDiGetDeviceRegistryProperty(DevInfo->hDevInfo,
                                                      &DevInfoData,
                                                      SPDRP_DEVICEDESC,
                                                      &RegDataType,
                                                      (PBYTE)szBuffer,
                                                      sizeof(szBuffer),
                                                      NULL)) &&
                    RegDataType == REG_SZ)
                {
                    /* FIXME - check string for NULL termination! */

                    li.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT | LVIF_IMAGE;
                    li.iItem = ItemCount;
                    li.iSubItem = 0;
                    li.state = (ItemCount == 0 ? LVIS_SELECTED : 0);
                    li.stateMask = LVIS_SELECTED;
                    li.pszText = szBuffer;
                    li.iImage = ImageIndex;
                    li.lParam = (LPARAM)&DevInfo->HwDevInfo[DevInfo->ItemCount - 1];

                    iItem = ListView_InsertItem(hpd->hWndDevList,
                                                &li);
                    if (iItem != -1)
                    {
                        ItemCount++;

                        if (SetupDiGetClassDescription(&DevInfo->Guid,
                                                       szBuffer,
                                                       sizeof(szBuffer) / sizeof(szBuffer[0]),
                                                       NULL))
                        {
                            li.mask = LVIF_TEXT;
                            li.iItem = iItem;
                            li.iSubItem = 1;

                            ListView_SetItem(hpd->hWndDevList,
                                             &li);                  
                        }
                    }
                }
            }
        }

        DevInfo++;
    }

    /* update the controls */
    UpdateControlStates(hpd);
}


static LRESULT
CALLBACK
ParentSubWndProc(IN HWND hwnd,
                 IN UINT uMsg,
                 IN WPARAM wParam,
                 IN LPARAM lParam)
{
    PHARDWARE_PAGE_DATA hpd;

    hpd = (PHARDWARE_PAGE_DATA)GetProp(hwnd,
                                       L"DevMgrSubClassInfo");
    if (hpd != NULL)
    {
        if (uMsg == WM_SIZE)
        {
            /* resize the hardware page */
            SetWindowPos(hpd->hWnd,
                         NULL,
                         0,
                         0,
                         LOWORD(lParam),
                         HIWORD(lParam),
                         SWP_NOZORDER);
        }

        /* pass the message the the old window proc */
        return CallWindowProc(hpd->ParentOldWndProc,
                              hwnd,
                              uMsg,
                              wParam,
                              lParam);
    }
    else
    {
        /* this is not a good idea if the subclassed window was an ansi
           window, but we failed finding out the previous window proc
           so we can't use CallWindowProc. This should rarely - if ever -
           happen. */

        return DefWindowProc(hwnd,
                             uMsg,
                             wParam,
                             lParam);
    }
}


static VOID
HardwareDlgResize(IN PHARDWARE_PAGE_DATA hpd,
                  IN INT cx,
                  IN INT cy)
{
    HDWP dwp;
    HWND hControl, hButton;
    INT Width, x, y;
    RECT rc, rcButton;
    POINT pt = {0};
    POINT ptMargin = {0};
    POINT ptMarginGroup = {0};

    /* use left margin of the IDC_DEVICES label as the right
       margin of all controls outside the group box */
    hControl = GetDlgItem(hpd->hWnd,
                          IDC_DEVICES);
    GetWindowRect(hControl,
                  &rc);
    MapWindowPoints(hControl,
                    hpd->hWnd,
                    &ptMargin,
                    1);

    Width = cx - (2 * ptMargin.x);

    if ((dwp = BeginDeferWindowPos(8)))
    {
        /* rc already has the window rect of IDC_DEVICES! */
        if (!(dwp = DeferWindowPos(dwp,
                                   hControl,
                                   NULL,
                                   0,
                                   0,
                                   Width,
                                   rc.bottom - rc.top,
                                   SWP_NOMOVE | SWP_NOZORDER)))
        {
            return;
        }

        /* resize the devices list view control */
        GetWindowRect(hpd->hWndDevList,
                      &rc);
        MapWindowPoints(hpd->hWndDevList,
                        hpd->hWnd,
                        &pt,
                        1);
        y = pt.y + hpd->DevListViewHeight + ptMargin.y;
        if (!(dwp = DeferWindowPos(dwp,
                                   hpd->hWndDevList,
                                   NULL,
                                   0,
                                   0,
                                   Width,
                                   hpd->DevListViewHeight,
                                   SWP_NOMOVE | SWP_NOZORDER)))
        {
            return;
        }

        /* resize the group box control */
        hControl = GetDlgItem(hpd->hWnd,
                              IDC_PROPERTIESGROUP);
        GetWindowRect(hControl,
                      &rc);
        if (!(dwp = DeferWindowPos(dwp,
                                   hControl,
                                   NULL,
                                   ptMargin.x,
                                   y,
                                   Width,
                                   cy - y - ptMargin.y,
                                   SWP_NOZORDER)))
        {
            return;
        }

        /* use left margin of the IDC_MANUFACTURER label as the right
           margin of all controls inside the group box */
        hControl = GetDlgItem(hpd->hWnd,
                              IDC_MANUFACTURER);
        GetWindowRect(hControl,
                      &rc);
        MapWindowPoints(hControl,
                        hpd->hWnd,
                        &ptMarginGroup,
                        1);

        ptMarginGroup.y = ptMargin.y * 2;
        Width = cx - (2 * ptMarginGroup.x);
        y += ptMarginGroup.y;
        if (!(dwp = DeferWindowPos(dwp,
                                   hControl,
                                   NULL,
                                   ptMarginGroup.x,
                                   y,
                                   Width,
                                   rc.bottom - rc.top,
                                   SWP_NOZORDER)))
        {
            return;
        }
        y += rc.bottom - rc.top + (ptMargin.y / 2);

        /* resize the IDC_LOCATION label */
        hControl = GetDlgItem(hpd->hWnd,
                              IDC_LOCATION);
        GetWindowRect(hControl,
                      &rc);
        if (!(dwp = DeferWindowPos(dwp,
                                   hControl,
                                   NULL,
                                   ptMarginGroup.x,
                                   y,
                                   Width,
                                   rc.bottom - rc.top,
                                   SWP_NOZORDER)))
        {
            return;
        }
        y += rc.bottom - rc.top + (ptMargin.y / 2);

        /* measure the size of the buttons */
        hButton = GetDlgItem(hpd->hWnd,
                             IDC_PROPERTIES);
        GetWindowRect(hButton,
                      &rcButton);

        /* resize the IDC_STATUS label */
        hControl = GetDlgItem(hpd->hWnd,
                              IDC_STATUS);
        GetWindowRect(hControl,
                      &rc);
        if (!(dwp = DeferWindowPos(dwp,
                                   hControl,
                                   NULL,
                                   ptMarginGroup.x,
                                   y,
                                   Width,
                                   cy - y - (3 * ptMargin.y) -
                                       (rcButton.bottom - rcButton.top),
                                   SWP_NOZORDER)))
        {
            return;
        }

        /* move the IDC_PROPERTIES button */
        y = cy - (2 * ptMargin.y) - (rcButton.bottom - rcButton.top);
        x = cx - ptMarginGroup.x - (rcButton.right - rcButton.left);
        if (!(dwp = DeferWindowPos(dwp,
                                   hButton,
                                   NULL,
                                   x,
                                   y,
                                   0,
                                   0,
                                   SWP_NOSIZE | SWP_NOZORDER)))
        {
            return;
        }

        /* move the IDC_TROUBLESHOOT button */
        hButton = GetDlgItem(hpd->hWnd,
                             IDC_TROUBLESHOOT);
        GetWindowRect(hButton,
                      &rcButton);
        x -= (ptMargin.x / 2) + (rcButton.right - rcButton.left);
        if (!(dwp = DeferWindowPos(dwp,
                                   hButton,
                                   NULL,
                                   x,
                                   y,
                                   0,
                                   0,
                                   SWP_NOSIZE | SWP_NOZORDER)))
        {
            return;
        }

        EndDeferWindowPos(dwp);
    }
}


static VOID
EnableTroubleShoot(PHARDWARE_PAGE_DATA hpd,
                   BOOL Enable)
{
    HWND hBtnTroubleShoot = GetDlgItem(hpd->hWnd,
                                       IDC_TROUBLESHOOT);

    EnableWindow(hBtnTroubleShoot,
                 Enable);
    ShowWindow(hBtnTroubleShoot,
               Enable ? SW_SHOW : SW_HIDE);
}


static INT_PTR
CALLBACK
HardwareDlgProc(IN HWND hwndDlg,
                IN UINT uMsg,
                IN WPARAM wParam,
                IN LPARAM lParam)
{
    PHARDWARE_PAGE_DATA hpd;

    hpd = (PHARDWARE_PAGE_DATA)GetWindowLongPtr(hwndDlg,
                                                DWL_USER);

    if (hpd != NULL || uMsg == WM_INITDIALOG)
    {
        switch (uMsg)
        {
            case WM_NOTIFY:
            {
                NMHDR *pnmh = (NMHDR*)lParam;
                if (pnmh->hwndFrom == hpd->hWndDevList)
                {
                    switch (pnmh->code)
                    {
                        case LVN_ITEMCHANGED:
                        {
                            LPNMLISTVIEW pnmv = (LPNMLISTVIEW)lParam;
                            
                            if ((pnmv->uChanged & LVIF_STATE) &&
                                ((pnmv->uOldState & (LVIS_FOCUSED | LVIS_SELECTED)) ||
                                 (pnmv->uNewState & (LVIS_FOCUSED | LVIS_SELECTED))))
                            {
                                UpdateControlStates(hpd);
                            }
                            break;
                        }
                    }
                }
                break;
            }

            case WM_SIZE:
                HardwareDlgResize(hpd,
                                  (INT)LOWORD(lParam),
                                  (INT)HIWORD(lParam));
                break;            

            case WM_SETTEXT:
            {
                LPCWSTR szWndText = (LPCWSTR)lParam;
                EnableTroubleShoot(hpd,
                                   (szWndText != NULL && szWndText[0] != L'\0'));
                break;
            }

            case WM_INITDIALOG:
            {
                hpd = (PHARDWARE_PAGE_DATA)lParam;
                if (hpd != NULL)
                {
                    HWND hWndParent;

                    hpd->hWnd = hwndDlg;
                    SetWindowLongPtr(hwndDlg,
                                     DWL_USER,
                                     (DWORD_PTR)hpd);

                    hpd->ClassImageListData.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);

                    SetupDiGetClassImageList(&hpd->ClassImageListData);

                    /* calculate the size of the devices list view control */
                    hpd->hWndDevList = GetDlgItem(hwndDlg,
                                                  IDC_LV_DEVICES);
                    if (hpd->hWndDevList != NULL)
                    {
                        RECT rcClient;
                        GetClientRect(hpd->hWndDevList,
                                      &rcClient);
                        hpd->DevListViewHeight = rcClient.bottom;

                        if (hpd->DisplayMode == HWPD_LARGELIST)
                        {
                            hpd->DevListViewHeight = (hpd->DevListViewHeight * 3) / 2;
                        }
                    }

                    /* subclass the parent window */
                    hWndParent = GetAncestor(hwndDlg,
                                             GA_PARENT);
                    if (hWndParent != NULL)
                    {
                        RECT rcClient;

                        if (GetClientRect(hWndParent,
                                          &rcClient) &&
                            SetWindowPos(hwndDlg,
                                         NULL,
                                         0,
                                         0,
                                         rcClient.right,
                                         rcClient.bottom,
                                         SWP_NOZORDER))
                        {
                            /* subclass the parent window. This is not safe
                               if the parent window belongs to another thread! */
                            hpd->ParentOldWndProc = (WNDPROC)SetWindowLongPtr(hWndParent,
                                                                              GWLP_WNDPROC,
                                                                              (LONG_PTR)ParentSubWndProc);

                            if (hpd->ParentOldWndProc != NULL &&
                                SetProp(hWndParent,
                                        L"DevMgrSubClassInfo",
                                        (HANDLE)hpd))
                            {
                                hpd->hWndParent = hWndParent;
                            }
                        }
                    }

                    /* initialize the devices list view control */
                    InitializeDevicesList(hpd);

                    /* fill the devices list view control */
                    FillDevicesList(hpd);

                    /* decide whether to show or hide the troubleshoot button */
                    EnableTroubleShoot(hpd,
                                       GetWindowTextLength(hwndDlg) != 0);
                }
                break;
            }

            case WM_DESTROY:
            {
                UINT i;

                /* free the device info set handles */
                for (i = 0;
                     i < hpd->NumberOfGuids;
                     i++)
                {
                    SetupDiDestroyDeviceInfoList(hpd->ClassDevInfo[i].hDevInfo);
                    if (hpd->ClassDevInfo[i].HwDevInfo != NULL)
                    {
                        HeapFree(GetProcessHeap(),
                                 0,
                                 hpd->ClassDevInfo[i].HwDevInfo);
                    }
                }

                /* restore the old window proc of the subclassed parent window */
                if (hpd->hWndParent != NULL && hpd->ParentOldWndProc != NULL)
                {
                    SetWindowLongPtr(hpd->hWndParent,
                                     GWLP_WNDPROC,
                                     (LONG_PTR)hpd->ParentOldWndProc);
                }

                if (hpd->ClassImageListData.ImageList != NULL)
                {
                    SetupDiDestroyClassImageList(&hpd->ClassImageListData);
                }

                /* free the reference to comctl32 */
                FreeLibrary(hpd->hComCtl32);
                hpd->hComCtl32 = NULL;

                /* free the allocated resources */
                HeapFree(GetProcessHeap(),
                         0,
                         hpd);
                break;
            }
        }
    }

    return FALSE;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceCreateHardwarePageEx
 *
 * DESCRIPTION
 *   Creates a hardware page
 *
 * ARGUMENTS
 *   hWndParent:     Handle to the parent window
 *   lpGuids:        An array of guids of devices that are to be listed
 *   uNumberOfGuids: Numbers of guids in the Guids array
 *   Unknown:        Unknown parameter, see NOTEs
 *
 * RETURN VALUE
 *   Returns the handle of the hardware page window that has been created or
 *   NULL if it failed.
 *
 * REVISIONS
 *   13-05-2005 first working version (Sebastian Gasiorek <zebasoftis@gmail.com>)
 *
 * TODO
 *   missing: device icon in list view, Troubleshoot button, device properties,
 *            status description,
 *            devices should be visible afer PSN_SETACTIVE message
 *
 */
HWND
WINAPI
DeviceCreateHardwarePageEx(IN HWND hWndParent,
                           IN LPGUID lpGuids,
                           IN UINT uNumberOfGuids,
                           IN HWPAGE_DISPLAYMODE DisplayMode)
{
    PHARDWARE_PAGE_DATA hpd;
    PINITCOMMONCONTROLS pInitCommonControls;

    /* allocate the HARDWARE_PAGE_DATA structure. Make sure it is
       zeroed because the initialization code assumes that in
       failure cases! */
    hpd = HeapAlloc(GetProcessHeap(),
                    HEAP_ZERO_MEMORY,
                    FIELD_OFFSET(HARDWARE_PAGE_DATA,
                                 ClassDevInfo) +
                        (uNumberOfGuids * sizeof(HWCLASSDEVINFO)));
    if (hpd != NULL)
    {
        HWND hWnd;
        UINT i;

        hpd->DisplayMode = ((DisplayMode > HWPD_MAX) ? HWPD_STANDARDLIST : DisplayMode);

        /* initialize the HARDWARE_PAGE_DATA structure */
        hpd->NumberOfGuids = uNumberOfGuids;
        for (i = 0;
             i < uNumberOfGuids;
             i++)
        {
            hpd->ClassDevInfo[i].Guid = lpGuids[i];
        }

        /* load comctl32.dll dynamically */
        hpd->hComCtl32 = LoadLibrary(TEXT("comctl32.dll"));
        if (hpd->hComCtl32 == NULL)
        {
            goto Cleanup;
        }

        /* initialize the common controls */
        pInitCommonControls = (PINITCOMMONCONTROLS)GetProcAddress(hpd->hComCtl32,
                                                                  "InitCommonControls");
        if (pInitCommonControls == NULL)
        {
            goto Cleanup;
        }
        pInitCommonControls();

        /* create the dialog */
        hWnd = CreateDialogParam(hDllInstance,
                                 MAKEINTRESOURCE(IDD_HARDWARE), 
                                 hWndParent, 
                                 HardwareDlgProc, 
                                 (LPARAM)hpd);
        if (hWnd != NULL)
        {
            return hWnd;
        }
        else
        {
Cleanup:
            /* oops, something went wrong... */
            if (hpd->hComCtl32 != NULL)
            {
                FreeLibrary(hpd->hComCtl32);
            }

            HeapFree(GetProcessHeap(),
                     0,
                     hpd);
        }
    }

    return NULL;
}


/***************************************************************************
 * NAME                                                         EXPORTED
 *      DeviceCreateHardwarePage
 *
 * DESCRIPTION
 *   Creates a hardware page
 *
 * ARGUMENTS
 *   hWndParent: Handle to the parent window
 *   lpGuid:     Guid of the device
 *
 * RETURN VALUE
 *   Returns the handle of the hardware page window that has been created or
 *   NULL if it failed.
 *
 * REVISIONS
 *
 * NOTE
 *
 */
HWND
WINAPI
DeviceCreateHardwarePage(IN HWND hWndParent,
                         IN LPGUID lpGuid)
{
    return DeviceCreateHardwarePageEx(hWndParent,
                                      lpGuid,
                                      1,
                                      HWPD_LARGELIST);
}
