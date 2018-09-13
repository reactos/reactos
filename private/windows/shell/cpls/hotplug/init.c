//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       init.c
//
//--------------------------------------------------------------------------

#include "hotplug.h"

HMODULE hHotPlug;



void
HotPlugDeviceTree(
   HWND hwndParent,
   PTCHAR MachineName,
   BOOLEAN HotPlugTree
   )
{
    CONFIGRET ConfigRet;
    DEVICETREE DeviceTree;
    PWIZPAGE_OBJECT WizPageObject = NULL;

    PROPSHEETPAGE Page;

    memset(&DeviceTree, 0, sizeof(DeviceTree));

    if (WizPageObject = malloc(sizeof(WIZPAGE_OBJECT))) {

        WizPageObject->RefCount = 0;
        WizPageObject->DeviceTree = &DeviceTree;

    } else {

        return;
    }

    if (MachineName) {

        lstrcpy(DeviceTree.MachineName, MachineName);
        ConfigRet = CM_Connect_Machine(MachineName, &DeviceTree.hMachine);
        if (ConfigRet != CR_SUCCESS) {

            return;
        }
    }

    DeviceTree.HotPlugTree = HotPlugTree;
    InitializeListHead(&DeviceTree.ChildSiblingList);
    DeviceTree.IsDialog = TRUE;
    DeviceTree.HideUI = FALSE;

    //
    // Since we're using the same code as the Add New Device Wizard, we
    // have to supply a LPROPSHEETPAGE as the lParam to the DialogProc.
    // (All we care about is the lParam field, and the DWORD at the end
    // of the buffer.)
    //
    Page.lParam = (LPARAM)WizPageObject;

    DialogBoxParam(hHotPlug,
                   MAKEINTRESOURCE(DLG_DEVTREE),
                   hwndParent,
                   DevTreeDlgProc,
                   (LPARAM)&Page
                   );

    if (DeviceTree.hMachine) {

        CM_Disconnect_Machine(DeviceTree.hMachine);
    }

    free(WizPageObject);

    return;
}




BOOL
HotPlugEjectDevice(
   HWND hwndParent,
   PTCHAR DeviceInstanceId
   )

/*++

Routine Description:

   Exported Entry point from hotplug.cpl to eject a specific Device Instance.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   DeviceInstanceId - Supplies the ID of the device instance.  This is the registry
                      path (relative to the Enum branch) of the device instance key.

Return Value:

   BOOL TRUE for success (does not mean device was ejected or not),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/

{
    DEVNODE DevNode;
    CONFIGRET ConfigRet;

    if ((ConfigRet = CM_Locate_DevNode(&DevNode,
                                       DeviceInstanceId,
                                       0)) == CR_SUCCESS) {

        ConfigRet = CM_Request_Device_Eject_Ex(DevNode,
                                               NULL,
                                               NULL,
                                               0,
                                               0,
                                               NULL);
    }

    SetLastError(ConfigRet);
    return (ConfigRet == CR_SUCCESS);
}

DWORD
WINAPI
HotPlugSurpriseWarnW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead = INVALID_HANDLE_VALUE;
    HANDLE hEvent = NULL;
    DWORD BytesRead;
    DWORD NumDevices = 0;
    ULONG DeviceIdsLength = 0;
    PTSTR DeviceIds = NULL;
    PTSTR SingleDeviceId = NULL;
    SURPRISEWARNDATA SurpriseWarnData;
    PSURPRISEWARNDEVICES SurpriseWarnDevicesList = NULL;
    PSURPRISEWARNDEVICES SingleSurpriseWarnDevice;
    BOOLEAN bDockDeviceInList;

    //
    // Get the read handle for the anonymous named pipe
    //
    hPipeRead = (HANDLE)StrToInt(szCmd);

    if (!ReadFile(hPipeRead,
                  (LPVOID)&hEvent,
                  sizeof(HANDLE),
                  &BytesRead,
                  NULL) ||
        (hEvent == NULL)) {

        goto clean0;
    }

    //
    // Read the first ULONG from the pipe, this is the length of all the
    // Device Ids.
    //
    if (!ReadFile(hPipeRead,
                  (LPVOID)&DeviceIdsLength,
                  sizeof(ULONG),
                  &BytesRead,
                  NULL) ||
        (DeviceIdsLength == 0)) {

        goto clean0;
    }

    //
    // Allocate space to hold the DeviceIds
    //
    DeviceIds = LocalAlloc(LPTR, DeviceIdsLength);

    if (!DeviceIds) {

        goto clean0;
    }

    //
    // Read all of the DeviceIds from the pipe at once
    //
    if (!ReadFile(hPipeRead,
                  (LPVOID)DeviceIds,
                  DeviceIdsLength,
                  &BytesRead,
                  NULL)) {

        goto clean0;
    }

    //
    // We are finished reading from the pipe, so close the handle
    //
    if (hPipeRead != INVALID_HANDLE_VALUE) {

        CloseHandle(hPipeRead);
        hPipeRead = INVALID_HANDLE_VALUE;
    }

    //
    // Enumerate through the multi-sz list of Device Ids.
    //
    bDockDeviceInList = FALSE;
    for (SingleDeviceId = DeviceIds;
         *SingleDeviceId;
         SingleDeviceId += lstrlen(SingleDeviceId) + 1) {

        SingleSurpriseWarnDevice = (PSURPRISEWARNDEVICES)LocalAlloc(LPTR, sizeof(SURPRISEWARNDEVICES));

        if (SingleSurpriseWarnDevice) {

            //
            // We don't want to show devices that have CM_DEVCAP_SURPRISEREMOVALOK or a
            // device that is not started becasue they can be surprise removed.
            //
            DEVNODE DeviceInstance;
            DWORD Capabilities;
            DWORD Len;
            DWORD Status, Problem;

            Len = sizeof(Capabilities);
            Capabilities = 0;
            Status = 0;

            if ((CM_Locate_DevNode(&DeviceInstance,
                                   SingleDeviceId,
                                   0) == CR_SUCCESS) &&
                (CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                     CM_DRP_CAPABILITIES,
                                                     NULL,
                                                     (PVOID)&Capabilities,
                                                     &Len,
                                                     0,
                                                     NULL) == CR_SUCCESS)) {


                if (Capabilities & CM_DEVCAP_DOCKDEVICE) {

                    bDockDeviceInList = TRUE;
                }

                if ((CM_Get_DevNode_Status(&Status,
                                          &Problem,
                                          DeviceInstance,
                                          0) == CR_SUCCESS) &&

                    ((Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) ||
                    !(Status & DN_STARTED))) {

                    //
                    // This device can be surprise removed so skip it.
                    //
                    continue;
                }
            }

            //
            // This device should not be surprise removed, so add it to the list of
            // devices that will be displayed in the UI.
            //
            NumDevices++;

            lstrcpy(SingleSurpriseWarnDevice->DeviceInstanceId, SingleDeviceId);

            SingleSurpriseWarnDevice->Next = NULL;
            if (SurpriseWarnDevicesList) {
                SurpriseWarnDevicesList->Next = SingleSurpriseWarnDevice;
            } else {
                SurpriseWarnDevicesList = SingleSurpriseWarnDevice;
            }
        }
    }

    //
    // Signal umpnpmgr that it can continue.
    //
    if (hEvent) {

        SetEvent(hEvent);
    }

    //
    // If we have any devices then bring up the surprise removal dialog
    //
    if (NumDevices) {

        SurpriseWarnData.hMachine = NULL;
        SurpriseWarnData.DeviceList = SurpriseWarnDevicesList;
        SurpriseWarnData.NumDevices = NumDevices;
        SurpriseWarnData.DockInList = bDockDeviceInList;

        if (bDockDeviceInList) {

            DialogBoxParam(hHotPlug,
                           MAKEINTRESOURCE(DLG_SURPRISEUNDOCK),
                           NULL,
                           SurpriseWarnDlgProc,
                           (LPARAM)&SurpriseWarnData
                           );
        } else {

            DialogBoxParam(hHotPlug,
                           MAKEINTRESOURCE(DLG_SURPRISEWARN),
                           NULL,
                           SurpriseWarnDlgProc,
                           (LPARAM)&SurpriseWarnData
                           );
        }
    }

clean0:

    if (hPipeRead != INVALID_HANDLE_VALUE) {

        CloseHandle(hPipeRead);
    }

    if (hEvent) {

        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    if (DeviceIds) {

        LocalFree(DeviceIds);
    }

    while (SurpriseWarnDevicesList) {

        SingleSurpriseWarnDevice = SurpriseWarnDevicesList;
        SurpriseWarnDevicesList = SurpriseWarnDevicesList->Next;

        LocalFree(SingleSurpriseWarnDevice);
    }

    return 1;
}


DWORD
WINAPI
HotPlugRemovalVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_REMOVAL);
}

DWORD
WINAPI
HotPlugEjectVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_EJECT);
}

DWORD
WINAPI
HotPlugStandbyVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_STANDBY);
}

DWORD
WINAPI
HotPlugHibernateVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_HIBERNATE);
}

DWORD
WINAPI
HotPlugWarmEjectVetoedW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    return HandleVetoedOperation(szCmd, VETOED_WARM_EJECT);
}


DWORD
WINAPI
HandleVetoedOperation(
    LPWSTR              szCmd,
    VETOED_OPERATION    VetoedOperation
    )
{
    HANDLE hPipeRead = INVALID_HANDLE_VALUE;
    HANDLE hEvent = NULL;
    PNP_VETO_TYPE VetoType;
    DWORD BytesRead;
    DWORD NumDevices = 0;
    ULONG VetoNamesLength = 0;
    PTSTR SingleDeviceId = NULL;
    PTSTR DeviceIds = NULL;
    VETO_DEVICE_COLLECTION_DATA RemovalVetoData;
    PDEVICE_COLLECTION pRemovalVetoDeviceList = NULL;
    PDEVICE_COLLECTION pSingleRemovalVetoDevice;
    DWORD dwCapabilities, len;
    BOOLEAN bDockDeviceInList;
    DEVNODE cmDeviceNode;

    try {

        //
        // Get the read handle for the anonymous named pipe
        //
        hPipeRead = (HANDLE)StrToInt(szCmd);

        //
        // The first dword is the finish event.
        //
        if (!ReadFile(hPipeRead,
                      (LPVOID)&hEvent,
                      sizeof(HANDLE),
                      &BytesRead,
                      NULL) ||
            (hEvent == NULL)) {

            goto clean0;
        }

        //
        // The second DWORD is the VetoType
        //
        if (!ReadFile(hPipeRead,
                      (LPVOID)&VetoType,
                      sizeof(PNP_VETO_TYPE),
                      &BytesRead,
                      NULL)) {

            goto clean0;
        }

        //
        // Read the next ULONG from the pipe, this is the length of all the
        // Veto Names.
        //
        if (!ReadFile(hPipeRead,
                      (LPVOID)&VetoNamesLength,
                      sizeof(ULONG),
                      &BytesRead,
                      NULL) ||
            (VetoNamesLength == 0)) {

            goto clean0;
        }

        //
        // Allocate space to hold the VetoNames
        //
        DeviceIds = LocalAlloc(LPTR, VetoNamesLength);

        if (!DeviceIds) {

            goto clean0;
        }

        //
        // Read all of the VetoNames from the pipe at once
        //
        if (!ReadFile(hPipeRead,
                      (LPVOID) DeviceIds,
                      VetoNamesLength,
                      &BytesRead,
                      NULL)) {

            goto clean0;
        }

        //
        // We are finished reading from the pipe, so close the handle
        //
        if (hPipeRead != INVALID_HANDLE_VALUE) {

            CloseHandle(hPipeRead);
            hPipeRead = INVALID_HANDLE_VALUE;
        }

        //
        // Enumerate through the multi-sz list of Device Ids.
        //
        bDockDeviceInList = FALSE;
        for (SingleDeviceId = DeviceIds;
             *SingleDeviceId;
             SingleDeviceId += lstrlen(SingleDeviceId) + 1) {

            pSingleRemovalVetoDevice = (PDEVICE_COLLECTION)LocalAlloc(LPTR, sizeof(DEVICE_COLLECTION));

            if (pSingleRemovalVetoDevice) {

                len = sizeof(dwCapabilities);

                if (CM_Locate_DevNode(
                    &cmDeviceNode,
                    SingleDeviceId,
                    0) == CR_SUCCESS) {

                    if (CM_Get_DevNode_Registry_Property_Ex(
                        cmDeviceNode,
                        CM_DRP_CAPABILITIES,
                        NULL,
                        (PVOID)&dwCapabilities,
                        &len,
                        0,
                        NULL) == CR_SUCCESS) {

                        if (dwCapabilities & CM_DEVCAP_DOCKDEVICE) {

                            bDockDeviceInList = TRUE;
                        }
                    }
                }

                //
                // This device should not be surprise removed, so add it to the list of
                // devices that will be displayed in the UI.
                //
                NumDevices++;

                lstrcpy(pSingleRemovalVetoDevice->DeviceInstanceId, SingleDeviceId);

                pSingleRemovalVetoDevice->Next = NULL;
                if (pRemovalVetoDeviceList) {
                    pRemovalVetoDeviceList->Next = pSingleRemovalVetoDevice;
                } else {
                    pRemovalVetoDeviceList = pSingleRemovalVetoDevice;
                }
            }
        }

        //
        // There should always be one device as that is the device who's removal
        // was vetoed.
        //
        ASSERT(NumDevices);

        //
        // Invent the VetoedOperation "VETOED_UNDOCK" from an eject containing
        // another dock.
        //
        if (bDockDeviceInList) {

            if (VetoedOperation == VETOED_EJECT) {

                VetoedOperation = VETOED_UNDOCK;

            } else if (VetoedOperation == VETOED_WARM_EJECT) {

                VetoedOperation = VETOED_WARM_UNDOCK;
            }
        }

        RemovalVetoData.hMachine = NULL;
        RemovalVetoData.DeviceList = pRemovalVetoDeviceList;
        RemovalVetoData.NumDevices = NumDevices;
        RemovalVetoData.VetoType = VetoType;
        RemovalVetoData.VetoedOperation = VetoedOperation;

        DialogBoxParam(
            hHotPlug,
            MAKEINTRESOURCE(DLG_REMOVAL_VETOED),
            NULL,
            DeviceRemovalVetoedDlgProc,
            (LPARAM)&RemovalVetoData
            );

clean0:;

    } except(EXCEPTION_EXECUTE_HANDLER) {
       ;
    }


    if (hPipeRead != INVALID_HANDLE_VALUE) {

        CloseHandle(hPipeRead);
    }

    if (hEvent) {

        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    if (DeviceIds) {

        LocalFree(DeviceIds);
    }

    while (pRemovalVetoDeviceList) {

        pSingleRemovalVetoDevice = pRemovalVetoDeviceList;
        pRemovalVetoDeviceList = pRemovalVetoDeviceList->Next;

        LocalFree(pSingleRemovalVetoDevice);
    }

    return 1;
}

DWORD
WINAPI
HotPlugSafeRemovalNotificationW(
    HWND hwnd,
    HINSTANCE hInst,
    LPWSTR szCmd,
    int nShow
    )
{
    HANDLE hPipeRead = INVALID_HANDLE_VALUE;
    HANDLE hEvent = NULL;
    DWORD BytesRead;
    DWORD NumDevices = 0;
    ULONG DeviceIdsLength = 0;
    PTSTR DeviceIds = NULL;
    PTSTR SingleDeviceId = NULL;
    SAFE_REMOVAL_COLLECTION_DATA SafeRemovalData;
    PDEVICE_COLLECTION pSafeRemovalDeviceList = NULL;
    PDEVICE_COLLECTION pSingleSafeRemovalDevice;
    DEVNODE cmDeviceNode;
    DWORD dwCapabilities, len;
    BOOLEAN bDockDeviceInList;

    //
    // Get the read handle for the anonymous named pipe
    //
    hPipeRead = (HANDLE)StrToInt(szCmd);

    if (!ReadFile(hPipeRead,
                  (LPVOID)&hEvent,
                  sizeof(HANDLE),
                  &BytesRead,
                  NULL) ||
        (hEvent == NULL)) {

        goto clean0;
    }

    //
    // Read the first ULONG from the pipe, this is the length of all the
    // Device Ids.
    //
    if (!ReadFile(hPipeRead,
                  (LPVOID)&DeviceIdsLength,
                  sizeof(ULONG),
                  &BytesRead,
                  NULL) ||
        (DeviceIdsLength == 0)) {

        goto clean0;
    }

    //
    // Allocate space to hold the DeviceIds
    //
    DeviceIds = LocalAlloc(LPTR, DeviceIdsLength);

    if (!DeviceIds) {

        goto clean0;
    }

    //
    // Read all of the DeviceIds from the pipe at once
    //
    if (!ReadFile(hPipeRead,
                  (LPVOID)DeviceIds,
                  DeviceIdsLength,
                  &BytesRead,
                  NULL)) {

        goto clean0;
    }

    //
    // We are finished reading from the pipe, so close the handle
    //
    if (hPipeRead != INVALID_HANDLE_VALUE) {

        CloseHandle(hPipeRead);
        hPipeRead = INVALID_HANDLE_VALUE;
    }

    //
    // Enumerate through the multi-sz list of Device Ids.
    //
    bDockDeviceInList = FALSE;
    for (SingleDeviceId = DeviceIds;
         *SingleDeviceId;
         SingleDeviceId += lstrlen(SingleDeviceId) + 1) {

        pSingleSafeRemovalDevice = (PDEVICE_COLLECTION)LocalAlloc(LPTR, sizeof(DEVICE_COLLECTION));

        if (pSingleSafeRemovalDevice) {

            len = sizeof(dwCapabilities);

            if (CM_Locate_DevNode(
                &cmDeviceNode,
                SingleDeviceId,
                CM_LOCATE_DEVNODE_PHANTOM) == CR_SUCCESS) {

                if (CM_Get_DevNode_Registry_Property_Ex(
                    cmDeviceNode,
                    CM_DRP_CAPABILITIES,
                    NULL,
                    (PVOID)&dwCapabilities,
                    &len,
                    0,
                    NULL) == CR_SUCCESS) {

                    if (dwCapabilities & CM_DEVCAP_DOCKDEVICE) {

                        bDockDeviceInList = TRUE;
                    }
                }
            }

            //
            // This device should not be surprise removed, so add it to the list of
            // devices that will be displayed in the UI.
            //
            NumDevices++;

            lstrcpy(pSingleSafeRemovalDevice->DeviceInstanceId, SingleDeviceId);

            pSingleSafeRemovalDevice->Next = NULL;
            if (pSafeRemovalDeviceList) {
                pSafeRemovalDeviceList->Next = pSingleSafeRemovalDevice;
            } else {
                pSafeRemovalDeviceList = pSingleSafeRemovalDevice;
            }
        }
    }

    //
    // Signal umpnpmgr that it can continue.
    //
    if (hEvent) {

        SetEvent(hEvent);
    }

    //
    // If we have any devices then bring up the safe removal dialog
    //
    if (NumDevices) {

        SafeRemovalData.hMachine = NULL;
        SafeRemovalData.DeviceList = pSafeRemovalDeviceList;
        SafeRemovalData.NumDevices = NumDevices;
        SafeRemovalData.DockInList = bDockDeviceInList;

        DialogBoxParam(
            hHotPlug,
            MAKEINTRESOURCE(DLG_REMOVAL_COMPLETE),
            NULL,
            SafeRemovalDlgProc,
            (LPARAM)&SafeRemovalData
            );
    }

clean0:

    if (hPipeRead != INVALID_HANDLE_VALUE) {

        CloseHandle(hPipeRead);
    }

    if (hEvent) {

        SetEvent(hEvent);
        CloseHandle(hEvent);
    }

    if (DeviceIds) {

        LocalFree(DeviceIds);
    }

    while (pSafeRemovalDeviceList) {

        pSingleSafeRemovalDevice = pSafeRemovalDeviceList;
        pSafeRemovalDeviceList = pSafeRemovalDeviceList->Next;

        LocalFree(pSingleSafeRemovalDevice);
    }

    return 1;
}


LONG
CPlApplet(
    HWND  hWnd,
    WORD  uMsg,
    DWORD lParam1,
    LONG  lParam2
    )
{
    LPNEWCPLINFO lpCPlInfo;
    LPCPLINFO lpOldCPlInfo;


    switch (uMsg) {
       case CPL_INIT:
           return TRUE;

       case CPL_GETCOUNT:
           return 1;

       case CPL_INQUIRE:
           lpOldCPlInfo = (LPCPLINFO)(LPARAM)lParam2;
           lpOldCPlInfo->lData = 0L;
           lpOldCPlInfo->idIcon = IDI_HOTPLUGICON;
           lpOldCPlInfo->idName = IDS_HOTPLUGNAME;
           lpOldCPlInfo->idInfo = IDS_HOTPLUGINFO;
           return TRUE;

       case CPL_NEWINQUIRE:
           lpCPlInfo = (LPNEWCPLINFO)(LPARAM)lParam2;
           lpCPlInfo->hIcon = LoadIcon(hHotPlug, MAKEINTRESOURCE(IDI_HOTPLUGICON));
           LoadString(hHotPlug, IDS_HOTPLUGNAME, lpCPlInfo->szName, sizeof(lpCPlInfo->szName));
           LoadString(hHotPlug, IDS_HOTPLUGINFO, lpCPlInfo->szInfo, sizeof(lpCPlInfo->szInfo));
           lpCPlInfo->dwHelpContext = IDH_HOTPLUGAPPLET;
           lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
           lpCPlInfo->lData = 0;
           lpCPlInfo->szHelpFile[0] = '\0';
           return TRUE;

       case CPL_DBLCLK:
           HotPlugDeviceTree(hWnd, NULL, TRUE);
           break;

       case CPL_STARTWPARMS:
           //
           // what does this mean ?
           //

           break;

       case CPL_EXIT:


           // Free up any allocations of resources made.

           break;

       default:
           break;
       }

    return 0L;
}


BOOL DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL
    )
{
    hHotPlug = hmod;

    if (ulReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hmod);
        InitCommonControls();
        }


    return TRUE;
}
