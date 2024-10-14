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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * PROJECT:         ReactOS devmgr.dll
 * FILE:            lib/devmgr/hwpage.c
 * PURPOSE:         ReactOS Device Manager
 * PROGRAMMER:      Thomas Weidenmueller <w3seek@reactos.com>
 * UPDATE HISTORY:
 *      04-04-2004  Created
 */

#include "precomp.h"
#include "properties.h"
#include "resource.h"


typedef struct _HWDEVINFO
{
    struct _HWCLASSDEVINFO *ClassDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    BOOL HideDevice;
} HWDEVINFO, *PHWDEVINFO;

typedef struct _HWCLASSDEVINFO
{
    GUID Guid;
    HDEVINFO hDevInfo;
    INT ImageIndex;
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
    (void)ListView_SetExtendedListViewStyle(hpd->hWndDevList,
                                            LVS_EX_FULLROWSELECT);

    /* set the list view image list */
    if (hpd->ClassImageListData.ImageList != NULL)
    {
        (void)ListView_SetImageList(hpd->hWndDevList,
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
        (void)ListView_InsertColumn(hpd->hWndDevList,
                                    iCol++,
                                    &lvc);
    }
    if (LoadString(hDllInstance,
                   IDS_TYPE,
                   szColName,
                   sizeof(szColName) / sizeof(szColName[0])))
    {
        lvc.cx = CX_TYPECOLUMN_WIDTH;
        (void)ListView_InsertColumn(hpd->hWndDevList,
                                    iCol++,
                                    &lvc);
    }
}


static BOOL
DisplaySelectedDeviceProperties(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWDEVINFO HwDevInfo;
    SP_DEVINFO_DATA DevInfoData;
    BOOL Ret = FALSE;

    HwDevInfo = (PHWDEVINFO)ListViewGetSelectedItemData(hpd->hWndDevList);
    if (HwDevInfo != NULL)
    {
        /* make a copy of the SP_DEVINFO_DATA structure on the stack, it may
           become invalid in case the devices are updated */
        DevInfoData = HwDevInfo->DevInfoData;

        /* display the advanced properties */
        Ret = DisplayDeviceAdvancedProperties(hpd->hWnd,
                                              NULL,
                                              HwDevInfo->ClassDevInfo->hDevInfo,
                                              &DevInfoData,
                                              hpd->hComCtl32,
                                              NULL,
                                              0) != -1;
    }

    return Ret;
}


static VOID
UpdateControlStates(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWDEVINFO HwDevInfo;
    HWND hBtnTroubleShoot, hBtnProperties;

    hBtnTroubleShoot = GetDlgItem(hpd->hWnd,
                                  IDC_TROUBLESHOOT);
    hBtnProperties = GetDlgItem(hpd->hWnd,
                                IDC_PROPERTIES);

    HwDevInfo = (PHWDEVINFO)ListViewGetSelectedItemData(hpd->hWndDevList);
    if (HwDevInfo != NULL)
    {
        /* update static controls */
        WCHAR szBuffer[256];
        LPWSTR szFormatted = NULL;

        /* get the manufacturer string */
        if (GetDeviceManufacturerString(HwDevInfo->ClassDevInfo->hDevInfo,
                                        &HwDevInfo->DevInfoData,
                                        szBuffer,
                                        sizeof(szBuffer) / sizeof(szBuffer[0])) &&
            LoadAndFormatString(hDllInstance,
                                IDS_MANUFACTURER,
                                &szFormatted,
                                szBuffer) != 0)
        {
            SetDlgItemText(hpd->hWnd,
                           IDC_MANUFACTURER,
                           szFormatted);
            LocalFree((HLOCAL)szFormatted);
        }

        /* get the location string */
        if (GetDeviceLocationString(HwDevInfo->ClassDevInfo->hDevInfo,
                                    &HwDevInfo->DevInfoData,
                                    0,
                                    szBuffer,
                                    sizeof(szBuffer) / sizeof(szBuffer[0])) &&
            LoadAndFormatString(hDllInstance,
                                IDS_LOCATION,
                                &szFormatted,
                                szBuffer) != 0)
        {
            SetDlgItemText(hpd->hWnd,
                           IDC_LOCATION,
                           szFormatted);
            LocalFree((HLOCAL)szFormatted);
        }

        if (GetDeviceStatusString(HwDevInfo->DevInfoData.DevInst,
                                  NULL,
                                  szBuffer,
                                  sizeof(szBuffer) / sizeof(szBuffer[0])) &&
            LoadAndFormatString(hDllInstance,
                                IDS_STATUS,
                                &szFormatted,
                                szBuffer) != 0)
        {
            SetDlgItemText(hpd->hWnd,
                           IDC_STATUS,
                           szFormatted);
            LocalFree((HLOCAL)szFormatted);
        }
    }
    else
    {
        /* clear static controls */
        SetDlgItemText(hpd->hWnd,
                       IDC_MANUFACTURER,
                       NULL);
        SetDlgItemText(hpd->hWnd,
                       IDC_LOCATION,
                       NULL);
        SetDlgItemText(hpd->hWnd,
                       IDC_STATUS,
                       NULL);
    }

    EnableWindow(hBtnTroubleShoot,
                 HwDevInfo != NULL);
    EnableWindow(hBtnProperties,
                 HwDevInfo != NULL);
}


static VOID
FreeDevicesList(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWCLASSDEVINFO ClassDevInfo, LastClassDevInfo;

    ClassDevInfo = hpd->ClassDevInfo;
    LastClassDevInfo = ClassDevInfo + hpd->NumberOfGuids;

    /* free the device info set handles and structures */
    while (ClassDevInfo != LastClassDevInfo)
    {
        if (ClassDevInfo->hDevInfo != INVALID_HANDLE_VALUE)
        {
            SetupDiDestroyDeviceInfoList(ClassDevInfo->hDevInfo);
            ClassDevInfo->hDevInfo = INVALID_HANDLE_VALUE;
        }

        ClassDevInfo->ItemCount = 0;
        ClassDevInfo->ImageIndex = 0;

        if (ClassDevInfo->HwDevInfo != NULL)
        {
            HeapFree(GetProcessHeap(),
                     0,
                     ClassDevInfo->HwDevInfo);
            ClassDevInfo->HwDevInfo = NULL;
        }

        ClassDevInfo++;
    }
}


static VOID
BuildDevicesList(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWCLASSDEVINFO ClassDevInfo, LastClassDevInfo;
    SP_DEVINFO_DATA DevInfoData;

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

    ClassDevInfo = hpd->ClassDevInfo;
    LastClassDevInfo = ClassDevInfo + hpd->NumberOfGuids;

    while (ClassDevInfo != LastClassDevInfo)
    {
        ClassDevInfo->ImageIndex = -1;

        /* open a class device handle for the GUID we're processing */
        ClassDevInfo->hDevInfo = SetupDiGetClassDevs(&ClassDevInfo->Guid,
                                                     NULL,
                                                     hpd->hWnd,
                                                     DIGCF_PRESENT | DIGCF_PROFILE);
        if (ClassDevInfo->hDevInfo != INVALID_HANDLE_VALUE)
        {
            DWORD MemberIndex = 0;

            SetupDiGetClassImageIndex(&hpd->ClassImageListData,
                                      &ClassDevInfo->Guid,
                                      &ClassDevInfo->ImageIndex);

            /* enumerate all devices in the class */
            while (SetupDiEnumDeviceInfo(ClassDevInfo->hDevInfo,
                                         MemberIndex++,
                                         &DevInfoData))
            {
                BOOL HideDevice = FALSE;

                if (ClassDevInfo->HwDevInfo != NULL)
                {
                    PHWDEVINFO HwNewDevInfo = (PHWDEVINFO)HeapReAlloc(GetProcessHeap(),
                                                                      0,
                                                                      ClassDevInfo->HwDevInfo,
                                                                      (ClassDevInfo->ItemCount + 1) *
                                                                          sizeof(HWDEVINFO));
                    if (HwNewDevInfo != NULL)
                    {
                        ClassDevInfo->HwDevInfo = HwNewDevInfo;
                    }
                    else
                    {
                        ERR("Unable to allocate memory for %d SP_DEVINFO_DATA structures!\n",
                            ClassDevInfo->ItemCount + 1);
                        break;
                    }
                }
                else
                {
                    ClassDevInfo->HwDevInfo = (PHWDEVINFO)HeapAlloc(GetProcessHeap(),
                                                                    0,
                                                                    sizeof(HWDEVINFO));
                    if (ClassDevInfo->HwDevInfo == NULL)
                    {
                        ERR("Unable to allocate memory for a SP_DEVINFO_DATA structures!\n");
                        break;
                    }
                }

                /* Find out if the device should be hidden by default */
                IsDeviceHidden(DevInfoData.DevInst,
                               NULL,
                               &HideDevice);

                /* save all information for the current device */
                ClassDevInfo->HwDevInfo[ClassDevInfo->ItemCount].ClassDevInfo = ClassDevInfo;
                ClassDevInfo->HwDevInfo[ClassDevInfo->ItemCount].DevInfoData = DevInfoData;
                ClassDevInfo->HwDevInfo[ClassDevInfo->ItemCount++].HideDevice = HideDevice;
            }
        }

        ClassDevInfo++;
    }
}


static BOOL
DeviceIdMatch(IN HDEVINFO DeviceInfoSet,
              IN PSP_DEVINFO_DATA DeviceInfoData,
              IN LPCWSTR lpDeviceId)
{
    DWORD DevIdLen;
    LPWSTR lpQueriedDeviceId;
    BOOL Ret = FALSE;

    if (!SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                    DeviceInfoData,
                                    NULL,
                                    0,
                                    &DevIdLen) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        if (DevIdLen == wcslen(lpDeviceId) + 1)
        {
            lpQueriedDeviceId = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                                  0,
                                                  DevIdLen * sizeof(WCHAR));
            if (lpQueriedDeviceId != NULL)
            {
                if (SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                               DeviceInfoData,
                                               lpQueriedDeviceId,
                                               DevIdLen,
                                               NULL))
                {
                    Ret = (wcscmp(lpDeviceId,
                                  lpQueriedDeviceId) == 0);
                }

                HeapFree(GetProcessHeap(),
                         0,
                         lpQueriedDeviceId);
            }
        }
    }

    return Ret;
}


static VOID
FillDevicesListViewControl(IN PHARDWARE_PAGE_DATA hpd,
                           IN LPCWSTR lpSelectDeviceId  OPTIONAL,
                           IN GUID *SelectedClassGuid  OPTIONAL)
{
    PHWCLASSDEVINFO ClassDevInfo, LastClassDevInfo;
    PHWDEVINFO HwDevInfo, LastHwDevInfo;
    WCHAR szBuffer[255];
    BOOL SelectedInClass;
    INT ItemCount = 0;

    BuildDevicesList(hpd);

    ClassDevInfo = hpd->ClassDevInfo;
    LastClassDevInfo = ClassDevInfo + hpd->NumberOfGuids;

    while (ClassDevInfo != LastClassDevInfo)
    {
        if (ClassDevInfo->HwDevInfo != NULL)
        {
            HwDevInfo = ClassDevInfo->HwDevInfo;
            LastHwDevInfo = HwDevInfo + ClassDevInfo->ItemCount;

            SelectedInClass = (SelectedClassGuid != NULL &&
                               IsEqualGUID(*SelectedClassGuid,
                                           ClassDevInfo->Guid));
            while (HwDevInfo != LastHwDevInfo)
            {
                INT iItem;
                LVITEM li = {0};

                /* get the device name */
                if (!HwDevInfo->HideDevice &&
                    GetDeviceDescriptionString(ClassDevInfo->hDevInfo,
                                               &HwDevInfo->DevInfoData,
                                               szBuffer,
                                               sizeof(szBuffer) / sizeof(szBuffer[0])))
                {
                    li.mask = LVIF_PARAM | LVIF_STATE | LVIF_TEXT | LVIF_IMAGE;
                    li.iItem = ItemCount;
                    if ((ItemCount == 0 && lpSelectDeviceId == NULL) ||
                        (SelectedInClass &&
                         DeviceIdMatch(ClassDevInfo->hDevInfo,
                                       &HwDevInfo->DevInfoData,
                                       lpSelectDeviceId)))
                    {
                        li.state = LVIS_SELECTED;
                    }
                    li.stateMask = LVIS_SELECTED;
                    li.pszText = szBuffer;
                    li.iImage = ClassDevInfo->ImageIndex;
                    li.lParam = (LPARAM)HwDevInfo;

                    iItem = ListView_InsertItem(hpd->hWndDevList,
                                                &li);
                    if (iItem != -1)
                    {
                        ItemCount++;

                        /* get the device type for the second column */
                        if (GetDeviceTypeString(&HwDevInfo->DevInfoData,
                                                szBuffer,
                                                sizeof(szBuffer) / sizeof(szBuffer[0])))
                        {
                            li.mask = LVIF_TEXT;
                            li.iItem = iItem;
                            li.iSubItem = 1;

                            (void)ListView_SetItem(hpd->hWndDevList,
                                                   &li);
                        }
                    }
                }

                HwDevInfo++;
            }
        }

        ClassDevInfo++;
    }

    /* update the controls */
    UpdateControlStates(hpd);
}


static VOID
UpdateDevicesListViewControl(IN PHARDWARE_PAGE_DATA hpd)
{
    PHWDEVINFO HwDevInfo;
    GUID SelectedClassGuid = {0};
    LPWSTR lpDeviceId = NULL;

    /* if a device currently is selected, remember the device id so we can
       select the device after the update if still present */
    HwDevInfo = (PHWDEVINFO)ListViewGetSelectedItemData(hpd->hWndDevList);
    if (HwDevInfo != NULL)
    {
        DWORD DevIdLen;
        if (!SetupDiGetDeviceInstanceId(HwDevInfo->ClassDevInfo->hDevInfo,
                                        &HwDevInfo->DevInfoData,
                                        NULL,
                                        0,
                                        &DevIdLen) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            SelectedClassGuid = HwDevInfo->DevInfoData.ClassGuid;
            lpDeviceId = (LPWSTR)HeapAlloc(GetProcessHeap(),
                                           0,
                                           DevIdLen * sizeof(WCHAR));
            if (lpDeviceId != NULL &&
                !SetupDiGetDeviceInstanceId(HwDevInfo->ClassDevInfo->hDevInfo,
                                            &HwDevInfo->DevInfoData,
                                            lpDeviceId,
                                            DevIdLen,
                                            NULL))
            {
                HeapFree(GetProcessHeap(),
                         0,
                         lpDeviceId);
                lpDeviceId = NULL;
            }
        }
    }

    /* clear the devices list view control */
    (void)ListView_DeleteAllItems(hpd->hWndDevList);

    /* free the device list */
    FreeDevicesList(hpd);

    /* build rebuild the device list and fill the list box again */
    FillDevicesListViewControl(hpd,
                               lpDeviceId,
                               (lpDeviceId != NULL ?
                                    &SelectedClassGuid :
                                    NULL));

    if (lpDeviceId != NULL)
    {
        HeapFree(GetProcessHeap(),
                 0,
                 lpDeviceId);
    }
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
        else if (uMsg == WM_DEVICECHANGE && IsWindowVisible(hpd->hWnd))
        {
            /* forward a WM_DEVICECHANGE message to the hardware
               page which wouldn't get the message itself as it is
               a child window */
            SendMessage(hpd->hWnd,
                        WM_DEVICECHANGE,
                        wParam,
                        lParam);
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
    INT_PTR Ret = FALSE;

    hpd = (PHARDWARE_PAGE_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

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

                        case NM_DBLCLK:
                        {
                            DisplaySelectedDeviceProperties(hpd);
                            break;
                        }
                    }
                }
                break;
            }

            case WM_COMMAND:
            {
                switch (LOWORD(wParam))
                {
                    case IDC_TROUBLESHOOT:
                    {
                        /* FIXME - start the help using the command in the window text */
                        break;
                    }

                    case IDC_PROPERTIES:
                    {
                        DisplaySelectedDeviceProperties(hpd);
                        break;
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

            case WM_DEVICECHANGE:
            {
                /* FIXME - don't call UpdateDevicesListViewControl for all events */
                UpdateDevicesListViewControl(hpd);
                Ret = TRUE;
                break;
            }

            case WM_INITDIALOG:
            {
                hpd = (PHARDWARE_PAGE_DATA)lParam;
                if (hpd != NULL)
                {
                    HWND hWndParent;

                    hpd->hWnd = hwndDlg;
                    SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR)hpd);

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
                    FillDevicesListViewControl(hpd,
                                               NULL,
                                               NULL);

                    /* decide whether to show or hide the troubleshoot button */
                    EnableTroubleShoot(hpd,
                                       GetWindowTextLength(hwndDlg) != 0);
                }
                Ret = TRUE;
                break;
            }

            case WM_DESTROY:
            {
                /* zero hpd pointer in window data, because it can be used later (WM_DESTROY has not to be last message) */
                SetWindowLongPtr(hwndDlg, DWLP_USER, (DWORD_PTR)NULL);

                /* free devices list */
                FreeDevicesList(hpd);

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

    return Ret;
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
 *   DisplayMode:    Sets the size of the device list view control
 *
 * RETURN VALUE
 *   Returns the handle of the hardware page window that has been created or
 *   NULL if it failed.
 *
 * @implemented
 */
HWND
WINAPI
DeviceCreateHardwarePageEx(IN HWND hWndParent,
                           IN LPGUID lpGuids,
                           IN UINT uNumberOfGuids,
                           IN HWPAGE_DISPLAYMODE DisplayMode)
{
    PHARDWARE_PAGE_DATA hpd;

    /* allocate the HARDWARE_PAGE_DATA structure. Make sure it is
       zeroed because the initialization code assumes that in
       failure cases! */
    hpd = (PHARDWARE_PAGE_DATA)HeapAlloc(GetProcessHeap(),
                                         HEAP_ZERO_MEMORY,
                                         FIELD_OFFSET(HARDWARE_PAGE_DATA,
                                                      ClassDevInfo[uNumberOfGuids]));
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
            hpd->ClassDevInfo[i].hDevInfo = INVALID_HANDLE_VALUE;
            hpd->ClassDevInfo[i].Guid = lpGuids[i];
        }

        /* load comctl32.dll dynamically */
        hpd->hComCtl32 = LoadAndInitComctl32();
        if (hpd->hComCtl32 == NULL)
        {
            goto Cleanup;
        }

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
 * @implemented
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
