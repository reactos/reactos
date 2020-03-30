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

// globals
HINSTANCE hApplet = 0;

/* Applets */
APPLET Applets[NUM_APPLETS] =
{
    {IDI_HOTPLUG, IDS_CPLNAME, IDS_CPLDESCRIPTION, InitApplet}
};


static
VOID
EnumHotpluggedDevices(HWND hwndDeviceTree)
{
    WCHAR szDisplayName[40];
    SP_DEVINFO_DATA did = { 0 };
    HDEVINFO hdev;
    int idev;
    DWORD dwCapabilities, dwSize;
    ULONG ulStatus, ulProblem;
    TVINSERTSTRUCTW tvItem;
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

            /* Add it to the device tree */
            ZeroMemory(&tvItem, sizeof(tvItem));
            tvItem.hParent = TVI_ROOT;
            tvItem.hInsertAfter = TVI_FIRST;

            tvItem.item.mask = TVIF_STATE | TVIF_TEXT /*| TVIF_PARAM | TVIF_IMAGE | TVIF_SELECTEDIMAGE */;
            tvItem.item.state = TVIS_EXPANDED;
            tvItem.item.stateMask = TVIS_EXPANDED;
            tvItem.item.pszText = szDisplayName;
//            tvItem.item.iImage = IMAGE_SOUND_SECTION;
//            tvItem.item.iSelectedImage = IMAGE_SOUND_SECTION;
            tvItem.item.lParam = (LPARAM)NULL;

            /*hTreeItem = */ TreeView_InsertItem(hwndDeviceTree, &tvItem);



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
    UNREFERENCED_PARAMETER(lParam);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            EnumHotpluggedDevices(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE));
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
                EnumHotpluggedDevices(GetDlgItem(hwndDlg, IDC_SAFE_REMOVE_DEVICE_TREE));
            }
            break;

        case WM_CLOSE:
            KillTimer(hwndDlg, 1);
            EndDialog(hwndDlg, TRUE);
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
