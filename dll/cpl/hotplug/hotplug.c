/*
* PROJECT:     Safely Remove Hardware Applet
* LICENSE:     GPL - See COPYING in the top level directory
* FILE:        dll/cpl/hotplug/hotplug.c
* PURPOSE:     applet initialization
* PROGRAMMERS: Johannes Anderwald (johannes.anderwald@reactos.org)
*/

#include "hotplug.h"

#define NDEBUG
#include <debug.h>

typedef struct _HOTPLUG_DATA
{
    SP_CLASSIMAGELIST_DATA ImageListData;
} HOTPLUG_DATA, *PHOTPLUG_DATA;


// globals
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_HOTPLUG, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};


static
VOID
EnumHotpluggedDevices(
    HWND hwndDeviceTree,
    PHOTPLUG_DATA pHotplugData)
{
    WCHAR szDisplayName[40];
    SP_DEVINFO_DATA did = { 0 };
    HDEVINFO hdev;
    int idev;
    DWORD dwCapabilities, dwSize;
    ULONG ulStatus, ulProblem;
    TVINSERTSTRUCTW tvItem;
//    HTREEITEM hTreeItem;
    WCHAR szGuidString[MAX_GUID_STRING_LEN];
    GUID ClassGuid;
    INT nClassImage;
    CONFIGRET cr;

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
            /* Get the device description */
            dwSize = sizeof(szDisplayName);
            cr = CM_Get_DevNode_Registry_Property(did.DevInst,
                                                  CM_DRP_DEVICEDESC,
                                                  NULL,
                                                  szDisplayName,
                                                  &dwSize,
                                                  0);
            if (cr != CR_SUCCESS)
                wcscpy(szDisplayName, L"Unknown Device");

            /* Get the class GUID */
            dwSize = sizeof(szGuidString);
            cr = CM_Get_DevNode_Registry_Property(did.DevInst,
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
//                memcpy(&ClassGuid, GUID_DEVCLASS_UNKNOWN, sizeof(GUID));
            }

            /* Get the image for the class this device is in */
            SetupDiGetClassImageIndex(&pHotplugData->ImageListData,
                                      &ClassGuid,
                                      &nClassImage);

            /* Add it to the device tree */
            ZeroMemory(&tvItem, sizeof(tvItem));
            tvItem.hParent = TVI_ROOT;
            tvItem.hInsertAfter = TVI_FIRST;

            tvItem.item.mask = TVIF_STATE | TVIF_TEXT /*| TVIF_PARAM*/ | TVIF_IMAGE | TVIF_SELECTEDIMAGE;
            tvItem.item.state = TVIS_EXPANDED;
            tvItem.item.stateMask = TVIS_EXPANDED;
            tvItem.item.pszText = szDisplayName;
            tvItem.item.iImage = nClassImage;
            tvItem.item.iSelectedImage = nClassImage;
            tvItem.item.lParam = (LPARAM)NULL;

            /*hTreeItem =*/ TreeView_InsertItem(hwndDeviceTree, &tvItem);



        }
    }

    SetupDiDestroyDeviceInfoList(hdev);
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

    UNREFERENCED_PARAMETER(lParam);


    pHotplugData = (PHOTPLUG_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            pHotplugData = HeapAlloc(GetProcessHeap(), 0, sizeof(HOTPLUG_DATA));
            if (pHotplugData != NULL)
            {
                SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)pHotplugData);

                pHotplugData->ImageListData.cbSize = sizeof(pHotplugData->ImageListData);
                SetupDiGetClassImageList(&pHotplugData->ImageListData);

                TreeView_SetImageList(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE),
                                      pHotplugData->ImageListData.ImageList,
                                      TVSIL_NORMAL);

                EnumHotpluggedDevices(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE),
                                      pHotplugData);
            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam))
            {
                case IDCLOSE:
                    KillTimer(hwndDlg, 1);
                    EndDialog(hwndDlg, TRUE);
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
    DPRINT1("InitApplet()\n");

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
