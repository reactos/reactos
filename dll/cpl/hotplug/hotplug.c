/*
 * PROJECT:     Safely Remove Hardware Applet
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Applet initialization
 * COPYRIGHT:   Copyright 2013 Johannes Anderwald <johannes.anderwald@reactos.org>
 *              Copyright 2020 Eric Kohl <eric.kohl@reactos.org>
 */

#include "hotplug.h"

#define NDEBUG
#include <debug.h>

// globals
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_HOTPLUG, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};

static
DWORD
GetHotPlugFlags(VOID)
{
    HKEY hKey = NULL;
    DWORD dwFlags = 0;
    DWORD dwSize, dwError;

    dwError = RegOpenKeyExW(HKEY_CURRENT_USER,
                            REGSTR_PATH_SYSTRAY,
                            0,
                            KEY_READ,
                            &hKey);
    if (dwError != ERROR_SUCCESS)
        goto done;

    dwSize = sizeof(dwFlags);
    dwError = RegQueryValueExW(hKey,
                               L"HotPlugFlags",
                               NULL,
                               NULL,
                               (LPBYTE)&dwFlags,
                               &dwSize);
    if (dwError != ERROR_SUCCESS)
        dwFlags = 0;

done:
    if (hKey != NULL)
        RegCloseKey(hKey);

    return dwFlags;
}

static
DWORD
SetHotPlugFlags(
    _In_ DWORD dwFlags)
{
    HKEY hKey = NULL;
    DWORD dwError;

    dwError = RegCreateKeyExW(HKEY_CURRENT_USER,
                              REGSTR_PATH_SYSTRAY,
                              0,
                              NULL,
                              REG_OPTION_NON_VOLATILE,
                              KEY_WRITE,
                              NULL,
                              &hKey,
                              NULL);
    if (dwError != ERROR_SUCCESS)
        goto done;

    dwError = RegSetValueExW(hKey,
                             L"HotPlugFlags",
                             0,
                             REG_DWORD,
                             (LPBYTE)&dwFlags,
                             sizeof(dwFlags));

done:
    if (hKey != NULL)
        RegCloseKey(hKey);

    return dwError;
}

static
VOID
UpdateDialog(
    _In_ HWND hwndDlg)
{
    HWND hwndDeviceTree;
    BOOL bHasItem;

    hwndDeviceTree = GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE);

    bHasItem = (TreeView_GetCount(hwndDeviceTree) != 0);
    if (bHasItem)
        TreeView_SelectItem(hwndDeviceTree, TreeView_GetFirstVisible(hwndDeviceTree));

    EnableWindow(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_PROPERTIES), bHasItem);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_STOP), bHasItem);
}

static
VOID
ShowContextMenu(
    HWND hwndDlg,
    HWND hwndTreeView,
    PHOTPLUG_DATA pHotplugData)
{
    HTREEITEM hTreeItem;
    RECT rc;
    POINT pt;

    hTreeItem = TreeView_GetSelection(hwndTreeView);
    if (hTreeItem == NULL)
        return;

    TreeView_GetItemRect(hwndTreeView, hTreeItem, &rc, TRUE);

    pt.x = (rc.left + rc.right) / 2;
    pt.y = (rc.top + rc.bottom) / 2;
    ClientToScreen(hwndTreeView, &pt);
    TrackPopupMenu(GetSubMenu(pHotplugData->hPopupMenu, 0),
                   TPM_LEFTALIGN | TPM_TOPALIGN,
                   pt.x,
                   pt.y,
                   0,
                   hwndDlg,
                   NULL);
}

static
DEVINST
GetSelectedDeviceInst(
    _In_ PHOTPLUG_DATA pHotplugData)
{
    HTREEITEM hTreeItem;
    TVITEMW item;

    hTreeItem = TreeView_GetSelection(pHotplugData->hwndDeviceTree);
    if (hTreeItem == NULL)
        return 0;

    ZeroMemory(&item, sizeof(item));
    item.mask = TVIF_PARAM;
    item.hItem = hTreeItem;

    TreeView_GetItem(pHotplugData->hwndDeviceTree, &item);

    return item.lParam;
}

static
VOID
ShowDeviceProperties(
    _In_ HWND hwndParent,
    _In_ DEVINST DevInst)
{
    ULONG ulSize;
    CONFIGRET cr;
    LPWSTR pszDevId;

    cr = CM_Get_Device_ID_Size(&ulSize, DevInst, 0);
    if (cr != CR_SUCCESS || ulSize == 0)
        return;

    /* Take the terminating NULL into account */
    ulSize++;

    pszDevId = HeapAlloc(GetProcessHeap(), 0, ulSize * sizeof(WCHAR));
    if (pszDevId == NULL)
        return;

    cr = CM_Get_Device_IDW(DevInst, pszDevId, ulSize, 0);
    if (cr == CR_SUCCESS)
    {
        typedef int (WINAPI *PFDEVICEPROPERTIESW)(HWND, LPCWSTR, LPCWSTR, BOOL);
        HMODULE hDevMgrDll;
        PFDEVICEPROPERTIESW pDevicePropertiesW;

        hDevMgrDll = LoadLibraryW(L"devmgr.dll");
        if (hDevMgrDll != NULL)
        {
            pDevicePropertiesW = (PFDEVICEPROPERTIESW)GetProcAddress(hDevMgrDll, "DevicePropertiesW");
            if (pDevicePropertiesW != NULL)
                pDevicePropertiesW(hwndParent, NULL, pszDevId, FALSE);

            FreeLibrary(hDevMgrDll);
        }
    }

    HeapFree(GetProcessHeap(), 0, pszDevId);
}

INT_PTR
CALLBACK
SafeRemovalDlgProc(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    PHOTPLUG_DATA pHotplugData;

    pHotplugData = (PHOTPLUG_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pHotplugData = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(HOTPLUG_DATA));
            if (pHotplugData != NULL)
            {
                WCHAR szWindowTitle[MAX_PATH];

                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pHotplugData);

                if (LoadStringW(hApplet,
                                IDS_CPLNAME,
                                szWindowTitle,
                                ARRAYSIZE(szWindowTitle)))
                {
                    SetWindowTextW(hwndDlg, szWindowTitle);
                }

                pHotplugData->hIcon = (HICON)LoadImageW(hApplet,
                                                        MAKEINTRESOURCEW(IDI_HOTPLUG),
                                                        IMAGE_ICON,
                                                        GetSystemMetrics(SM_CXICON),
                                                        GetSystemMetrics(SM_CYICON),
                                                        LR_DEFAULTCOLOR);
                pHotplugData->hIconSm = (HICON)LoadImageW(hApplet,
                                                          MAKEINTRESOURCEW(IDI_HOTPLUG),
                                                          IMAGE_ICON,
                                                          GetSystemMetrics(SM_CXSMICON),
                                                          GetSystemMetrics(SM_CYSMICON),
                                                          LR_DEFAULTCOLOR);
                SendMessageW(hwndDlg, WM_SETICON, ICON_BIG, (LPARAM)pHotplugData->hIcon);
                SendMessageW(hwndDlg, WM_SETICON, ICON_SMALL, (LPARAM)pHotplugData->hIconSm);

                pHotplugData->ImageListData.cbSize = sizeof(pHotplugData->ImageListData);
                SetupDiGetClassImageList(&pHotplugData->ImageListData);

                pHotplugData->hPopupMenu = LoadMenu(hApplet, MAKEINTRESOURCE(IDM_POPUP_DEVICE_TREE));
                pHotplugData->hwndDeviceTree = GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE);
                pHotplugData->dwFlags = GetHotPlugFlags();

                if (pHotplugData->dwFlags & HOTPLUG_DISPLAY_DEVICE_COMPONENTS)
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DISPLAY_COMPONENTS), BST_CHECKED);

                TreeView_SetImageList(pHotplugData->hwndDeviceTree,
                                      pHotplugData->ImageListData.ImageList,
                                      TVSIL_NORMAL);

                EnumHotpluggedDevices(pHotplugData);
                UpdateDialog(hwndDlg);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCLOSE:
                    KillTimer(hwndDlg, 1);
                    EndDialog(hwndDlg, TRUE);
                    break;

                case IDC_SAFE_REMOVE_DISPLAY_COMPONENTS:
                    if (HIWORD(wParam) == BN_CLICKED)
                    {
                        if (pHotplugData != NULL)
                        {
                            if (Button_GetCheck(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DISPLAY_COMPONENTS)) == BST_CHECKED)
                                pHotplugData->dwFlags |= HOTPLUG_DISPLAY_DEVICE_COMPONENTS;
                            else
                                pHotplugData->dwFlags &= ~HOTPLUG_DISPLAY_DEVICE_COMPONENTS;

                            SetHotPlugFlags(pHotplugData->dwFlags);

                            EnumHotpluggedDevices(pHotplugData);
                            UpdateDialog(hwndDlg);
                        }
                    }
                    break;

                case IDC_SAFE_REMOVE_PROPERTIES:
                case IDM_PROPERTIES:
                    ShowDeviceProperties(hwndDlg, GetSelectedDeviceInst(pHotplugData));
                    break;

                case IDC_SAFE_REMOVE_STOP:
                case IDM_STOP:
                {
                    if (pHotplugData != NULL)
                    {
                        DialogBoxParamW(hApplet,
                                        MAKEINTRESOURCEW(IDD_CONFIRM_STOP_HARDWARE_DIALOG),
                                        hwndDlg,
                                        ConfirmRemovalDlgProc,
                                        (LPARAM)pHotplugData);
                    }

                    break;
                }
            }
            break;

        case WM_DEVICECHANGE:
            switch (wParam)
            {
                case DBT_DEVNODES_CHANGED:
                    SetTimer(hwndDlg, 1, 500, NULL);
                    break;
            }
            break;

        case WM_TIMER:
            if (wParam == 1)
            {
                KillTimer(hwndDlg, 1);

                if (pHotplugData != NULL)
                {
                    EnumHotpluggedDevices(pHotplugData);
                    UpdateDialog(hwndDlg);
                }
            }
            break;

        case WM_NOTIFY:
            if (((LPNMHDR)lParam)->idFrom == IDC_SAFE_REMOVE_DEVICE_TREE)
            {
                if (((LPNMHDR)lParam)->code == NM_RCLICK)
                {
                    if (pHotplugData != NULL)
                    {
                        ShowContextMenu(hwndDlg,
                                        ((LPNMHDR)lParam)->hwndFrom,
                                        pHotplugData);
                        return TRUE;
                    }
                }
            }
            break;

        case WM_CLOSE:
            KillTimer(hwndDlg, 1);
            EndDialog(hwndDlg, TRUE);
            break;

        case WM_DESTROY:
            if (pHotplugData != NULL)
            {
                if (pHotplugData->hPopupMenu != NULL)
                    DestroyMenu(pHotplugData->hPopupMenu);

                SetupDiDestroyClassImageList(&pHotplugData->ImageListData);

                if (pHotplugData->hIconSm)
                    DestroyIcon(pHotplugData->hIconSm);

                if (pHotplugData->hIcon)
                    DestroyIcon(pHotplugData->hIcon);

                HeapFree(GetProcessHeap(), 0, pHotplugData);
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)NULL);
            }
            break;
    }

    return FALSE;
}

LONG
APIENTRY
InitApplet(
    HWND hwnd,
    UINT uMsg,
    LPARAM wParam,
    LPARAM lParam)
{
    DPRINT("InitApplet()\n");

    DialogBox(hApplet,
              MAKEINTRESOURCE(IDD_SAFE_REMOVE_HARDWARE_DIALOG),
              hwnd,
              SafeRemovalDlgProc);

    // TODO
    return TRUE;
}

LONG
CALLBACK
CPlApplet(
    HWND hwndCPl,
    UINT uMsg,
    LPARAM lParam1,
    LPARAM lParam2)
{
    UINT i = (UINT)lParam1;

    switch(uMsg)
    {
        case CPL_INIT:
            return TRUE;

        case CPL_GETCOUNT:
            return NUM_APPLETS;

        case CPL_INQUIRE:
            if (i < NUM_APPLETS)
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            else
            {
                return TRUE;
            }
            break;

        case CPL_DBLCLK:
            if (i < NUM_APPLETS)
                Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            else
                return TRUE;
            break;

        case CPL_STARTWPARMSW:
            if (i < NUM_APPLETS)
                return Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;
    }
    return FALSE;
}

INT
WINAPI
DllMain(
    HINSTANCE hinstDLL,
    DWORD dwReason,
    LPVOID lpvReserved)
{
    UNREFERENCED_PARAMETER(lpvReserved);

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        case DLL_THREAD_ATTACH:
            hApplet = hinstDLL;
            break;
    }
    return TRUE;
}
