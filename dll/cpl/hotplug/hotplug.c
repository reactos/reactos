/*
* PROJECT:     Safely Remove Hardware Applet
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/cpl/hotplug/hotplug.c
* PURPOSE:     applet initialization
* PROGRAMMERS: Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "hotplug.h"

#include <initguid.h>
#include <devguid.h>

#define NDEBUG
#include <debug.h>


typedef struct _HOTPLUG_DATA
{
    SP_CLASSIMAGELIST_DATA ImageListData;
    HMENU hPopupMenu;
    DWORD dwFlags;
} HOTPLUG_DATA, *PHOTPLUG_DATA;


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
HTREEITEM
InsertDeviceTreeItem(
    _In_ HWND hwndDeviceTree,
    _In_ HTREEITEM hParent,
    _In_ DWORD DevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    WCHAR szDisplayName[40];
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    TVINSERTSTRUCTW tvItem;
    GUID ClassGuid;
    INT nClassImage;
    DWORD dwSize;
    CONFIGRET cr;

    /* Get the device description */
    dwSize = sizeof(szDisplayName);
    cr = CM_Get_DevNode_Registry_Property(DevInst,
                                          CM_DRP_DEVICEDESC,
                                          NULL,
                                          szDisplayName,
                                          &dwSize,
                                          0);
    if (cr != CR_SUCCESS)
        wcscpy(szDisplayName, L"Unknown Device");

    /* Get the class GUID */
    dwSize = sizeof(szGuidString);
    cr = CM_Get_DevNode_Registry_Property(DevInst,
                                          CM_DRP_CLASSGUID,
                                          NULL,
                                          szGuidString,
                                          &dwSize,
                                          0);
    if (cr == CR_SUCCESS)
    {
        pSetupGuidFromString(szGuidString, &ClassGuid);
    }
    else
    {
        memcpy(&ClassGuid, &GUID_DEVCLASS_UNKNOWN, sizeof(GUID));
    }

    /* Get the image for the class this device is in */
    SetupDiGetClassImageIndex(&pHotplugData->ImageListData,
                              &ClassGuid,
                              &nClassImage);

    /* Add it to the device tree */
    ZeroMemory(&tvItem, sizeof(tvItem));
    tvItem.hParent = hParent;
    tvItem.hInsertAfter = TVI_LAST;

    tvItem.item.mask = TVIF_STATE | TVIF_TEXT /*| TVIF_PARAM*/ | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
    tvItem.item.state = TVIS_EXPANDED;
    tvItem.item.stateMask = TVIS_EXPANDED;
    tvItem.item.pszText = szDisplayName;
    tvItem.item.iImage = nClassImage;
    tvItem.item.iSelectedImage = nClassImage;
    tvItem.item.lParam = (LPARAM)NULL;

    return TreeView_InsertItem(hwndDeviceTree, &tvItem);
}


static
VOID
RecursiveInsertSubDevices(
    _In_ HWND hwndDeviceTree,
    _In_ HTREEITEM hParentItem,
    _In_ DWORD ParentDevInst,
    _In_ PHOTPLUG_DATA pHotplugData)
{
    HTREEITEM hTreeItem;
    DEVINST ChildDevInst;
    CONFIGRET cr;

    DPRINT("RecursiveInsertSubDevices()\n");

    cr = CM_Get_Child(&ChildDevInst, ParentDevInst, 0);
    if (cr != CR_SUCCESS)
    {
        DPRINT("No child! %lu\n", cr);
        return;
    }

    hTreeItem = InsertDeviceTreeItem(hwndDeviceTree,
                                     hParentItem,
                                     ChildDevInst,
                                     pHotplugData);
    if (hTreeItem != NULL)
    {
        RecursiveInsertSubDevices(hwndDeviceTree,
                                  hTreeItem,
                                  ChildDevInst,
                                  pHotplugData);
    }

    for (;;)
    {
        cr = CM_Get_Sibling(&ChildDevInst, ChildDevInst, 0);
        if (cr != CR_SUCCESS)
        {
            DPRINT("No sibling! %lu\n", cr);
            return;
        }

        hTreeItem = InsertDeviceTreeItem(hwndDeviceTree,
                                         hParentItem,
                                         ChildDevInst,
                                         pHotplugData);
        if (hTreeItem != NULL)
        {
            RecursiveInsertSubDevices(hwndDeviceTree,
                                      hTreeItem,
                                      ChildDevInst,
                                      pHotplugData);
        }
    }
}


static
VOID
EnumHotpluggedDevices(
    HWND hwndDeviceTree,
    PHOTPLUG_DATA pHotplugData)
{
    SP_DEVINFO_DATA did = { 0 };
    HDEVINFO hdev;
    int idev;
    DWORD dwCapabilities, dwSize;
    ULONG ulStatus, ulProblem;
    HTREEITEM hTreeItem;
    CONFIGRET cr;

    DPRINT1("EnumHotpluggedDevices()\n");

    TreeView_DeleteAllItems(hwndDeviceTree);

    hdev = SetupDiGetClassDevs(NULL, NULL, 0, DIGCF_ALLCLASSES | DIGCF_PRESENT);
    if (hdev == INVALID_HANDLE_VALUE)
        return;

    did.cbSize = sizeof(did);

    /* Enumerate all the attached devices */
    for (idev = 0; SetupDiEnumDeviceInfo(hdev, idev, &did); idev++)
    {
        ulStatus = 0;
        ulProblem = 0;

        cr = CM_Get_DevNode_Status(&ulStatus,
                                   &ulProblem,
                                   did.DevInst,
                                   0);
        if (cr != CR_SUCCESS)
            continue;

        dwCapabilities = 0,
        dwSize = sizeof(dwCapabilities);
        cr = CM_Get_DevNode_Registry_Property(did.DevInst,
                                              CM_DRP_CAPABILITIES,
                                              NULL,
                                              &dwCapabilities,
                                              &dwSize,
                                              0);
        if (cr != CR_SUCCESS)
            continue;

        /* Add devices that require safe removal to the device tree */
        if ( (dwCapabilities & CM_DEVCAP_REMOVABLE) &&
            !(dwCapabilities & CM_DEVCAP_DOCKDEVICE) &&
            !(dwCapabilities & CM_DEVCAP_SURPRISEREMOVALOK) &&
            ((dwCapabilities & CM_DEVCAP_EJECTSUPPORTED) || (ulStatus & DN_DISABLEABLE)) &&
            ulProblem == 0)
        {
            hTreeItem = InsertDeviceTreeItem(hwndDeviceTree,
                                             TVI_ROOT,
                                             did.DevInst,
                                             pHotplugData);

            if ((hTreeItem != NULL) && (pHotplugData->dwFlags & HOTPLUG_DISPLAY_DEVICE_COMPONENTS))
            {
                RecursiveInsertSubDevices(hwndDeviceTree,
                                          hTreeItem,
                                          did.DevInst,
                                          pHotplugData);
            }
        }
    }

    SetupDiDestroyDeviceInfoList(hdev);
}


static
VOID
UpdateButtons(
    HWND hwndDlg)
{
    BOOL bEnabled;

    bEnabled = (TreeView_GetCount(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE)) != 0);

    EnableWindow(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_PROPERTIES), bEnabled);
    EnableWindow(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_STOP), bEnabled);
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
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pHotplugData);

                pHotplugData->ImageListData.cbSize = sizeof(pHotplugData->ImageListData);
                SetupDiGetClassImageList(&pHotplugData->ImageListData);

                pHotplugData->hPopupMenu = LoadMenu(hApplet, MAKEINTRESOURCE(IDM_POPUP_DEVICE_TREE));

                pHotplugData->dwFlags = GetHotPlugFlags();

                if (pHotplugData->dwFlags & HOTPLUG_DISPLAY_DEVICE_COMPONENTS)
                    Button_SetCheck(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DISPLAY_COMPONENTS), BST_CHECKED);

                TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE),
                                      pHotplugData->ImageListData.ImageList,
                                      TVSIL_NORMAL);

                EnumHotpluggedDevices(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE),
                                      pHotplugData);
                UpdateButtons(hwndDlg);
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

                            EnumHotpluggedDevices(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE),
                                                  pHotplugData);
                        }
                    }
                    break;
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
                    EnumHotpluggedDevices(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE),
                                          pHotplugData);
                    UpdateButtons(hwndDlg);
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
            {
                CPLINFO *CPlInfo = (CPLINFO*)lParam2;
                CPlInfo->lData = 0;
                CPlInfo->idIcon = Applets[i].idIcon;
                CPlInfo->idName = Applets[i].idName;
                CPlInfo->idInfo = Applets[i].idDescription;
            }
            break;

        case CPL_DBLCLK:
            Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
            break;

        case CPL_STARTWPARMSW:
            return Applets[i].AppletProc(hwndCPl, uMsg, lParam1, lParam2);
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
