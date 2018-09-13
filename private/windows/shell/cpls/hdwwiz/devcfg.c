//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       devcfg.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <initguid.h>
#include <devguid.h>


//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

TCHAR szUnknownDevice[64];
USHORT LenUnknownDevice;

TCHAR szUnknown[64];
USHORT LenUnknown;

PTCHAR
BuildFriendlyName(
   DEVINST DevInst,
   HMACHINE hMachine
   )
{
    PTCHAR Location;
    PTCHAR FriendlyName;
    CONFIGRET ConfigRet;
    ULONG ulSize;
    TCHAR szBuffer[MAX_PATH];

    //
    // Try the registry for FRIENDLYNAME
    //

    ulSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                    CM_DRP_FRIENDLYNAME,
                                                    NULL,
                                                    szBuffer,
                                                    &ulSize,
                                                    0,
                                                    hMachine
                                                    );
    if (ConfigRet != CR_SUCCESS || !*szBuffer) {
        //
        // Try the registry for DEVICEDESC
        //

        ulSize = sizeof(szBuffer);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                        CM_DRP_DEVICEDESC,
                                                        NULL,
                                                        szBuffer,
                                                        &ulSize,
                                                        0,
                                                        hMachine
                                                        );
        if (ConfigRet != CR_SUCCESS || !*szBuffer) {
            GUID ClassGuid;

            //
            // Try the registry for CLASSNAME
            //

            ulSize = sizeof(szBuffer);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                            CM_DRP_CLASSGUID,
                                                            NULL,
                                                            szBuffer,
                                                            &ulSize,
                                                            0,
                                                            hMachine
                                                            );


            if (ConfigRet == CR_SUCCESS) {
                pSetupGuidFromString(szBuffer, &ClassGuid);
                }


            if (!IsEqualGUID(&ClassGuid, &GUID_NULL) &&
                !IsEqualGUID(&ClassGuid, &GUID_DEVCLASS_UNKNOWN))
              {
                ulSize = sizeof(szBuffer);
                ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                                CM_DRP_CLASS,
                                                                NULL,
                                                                szBuffer,
                                                                &ulSize,
                                                                0,
                                                                hMachine
                                                                );
                }
            else {
                ConfigRet = ~CR_SUCCESS;
                }


            }
        }


    if (ConfigRet == CR_SUCCESS && *szBuffer) {
        FriendlyName = LocalAlloc(LPTR, ulSize);
        if (FriendlyName) {
            memcpy(FriendlyName, szBuffer, ulSize);
            }
        }
    else {
        FriendlyName = NULL;
        }


    return FriendlyName;
}

void
AddItemToListView(
    PHARDWAREWIZ HardwareWiz,
    HWND hwndListView,
    DEVINST DevInst,
    DWORD Problem,
    BOOL HiddenDevice,
    DEVINST SelectedDevInst
    )
{
    INT Index;
    LV_ITEM lviItem;
    PTCHAR FriendlyName;
    PTCHAR LocationInfo;
    GUID ClassGuid;
    ULONG ulSize;
    CONFIGRET ConfigRet;
    TCHAR szBuffer[MAX_PATH];


    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iSubItem = 0;
    lviItem.lParam = DevInst;

    //
    // Devices with problems need to go at the top of the list
    //
    if (Problem) {
    
        lviItem.iItem = 0;

    } else {

        lviItem.iItem = ListView_GetItemCount(hwndListView);
    }
    
    //
    // fetch a name for this device
    //

    FriendlyName = BuildFriendlyName(DevInst, HardwareWiz->hMachine);
    if (FriendlyName) {
    
        lviItem.pszText = FriendlyName;

    } else {
    
        lviItem.pszText = szUnknown;
    }

    //
    // Fetch the class icon for this device.
    //

    ulSize = sizeof(szBuffer);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                                    CM_DRP_CLASSGUID,
                                                    NULL,
                                                    szBuffer,
                                                    &ulSize,
                                                    0,
                                                    HardwareWiz->hMachine
                                                    );


    if (ConfigRet == CR_SUCCESS) {
    
        pSetupGuidFromString(szBuffer, &ClassGuid);

    } else {
    
        ClassGuid = GUID_DEVCLASS_UNKNOWN;
    }

    if (SetupDiGetClassImageIndex(&HardwareWiz->ClassImageList,
                                  &ClassGuid,
                                  &lviItem.iImage
                                  ))
    {
        lviItem.mask |= (LVIF_IMAGE | LVIF_STATE);

        if (Problem) {
        
            lviItem.state = (Problem == CM_PROB_DISABLED) ?
                            INDEXTOOVERLAYMASK(IDI_DISABLED_OVL - IDI_CLASSICON_OVERLAYFIRST + 1) :
                            INDEXTOOVERLAYMASK(IDI_PROBLEM_OVL - IDI_CLASSICON_OVERLAYFIRST + 1);

        } else {

            lviItem.state = INDEXTOOVERLAYMASK(0);
        }

        lviItem.stateMask = LVIS_OVERLAYMASK;

        if (HiddenDevice) {

            lviItem.state |= LVIS_CUT;
            lviItem.stateMask |= LVIS_CUT;
        }
    }

    Index = ListView_InsertItem(hwndListView, &lviItem);

    if ((Index != -1) && (SelectedDevInst == DevInst)) {

        ListView_SetItemState(hwndListView,
                              Index,
                              LVIS_SELECTED|LVIS_FOCUSED,
                              LVIS_SELECTED|LVIS_FOCUSED
                              );
    }


    if (FriendlyName) {
    
        LocalFree(FriendlyName);
    }

    return;
}


BOOL
BuildDeviceListView(
    PHARDWAREWIZ HardwareWiz,
    HWND hwndListView,
    BOOL ShowHiddenDevices,
    DEVINST SelectedDevInst,
    DWORD *DevicesDetected,
    ADDDEVNODETOLIST_CALLBACK AddDevNodeToListCallBack
    )
{
    HDEVINFO hDeviceInfo;
    DWORD Index;
    ULONG DevNodeStatus, DevNodeProblem;
    SP_DEVINFO_DATA DevInfoData;
    BOOL HiddenDevice;

    *DevicesDetected = 0;

    hDeviceInfo = SetupDiGetClassDevsEx(NULL,   // classguid
                                        NULL,   // enumerator
                                        NULL,   // hdwnParent
                                        ShowHiddenDevices ? DIGCF_ALLCLASSES : DIGCF_ALLCLASSES | DIGCF_PRESENT,
                                        NULL,   // existing HDEVINFO set
                                        HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL,
                                        0
                                        );
                                        
    if (hDeviceInfo == INVALID_HANDLE_VALUE) {
    
        return FALSE;
    }

    DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    Index = 0;
    while (SetupDiEnumDeviceInfo(hDeviceInfo, Index++, &DevInfoData)) {

        if (CM_Get_DevNode_Status_Ex(&DevNodeStatus,
                                     &DevNodeProblem,
                                     DevInfoData.DevInst,
                                     0,
                                     HardwareWiz->hMachine
                                     ) != CR_SUCCESS) {
        
            DevNodeProblem = 0;
        }

        HiddenDevice = IsDeviceHidden(HardwareWiz, &DevInfoData);

        //
        // Only call AddItemToListView if the device is not a hidden device.
        //
        // If ProblemDevices is TRUE then only call the callback if this device
        // has a problem.  If ProblemDevices is FALSE then only call the callback
        // if this device does NOT have a problem.  
        //
        if (ShowHiddenDevices || !HiddenDevice) {
        
            //
            // Check the callback to see if we should add this devnode to the list.
            //
            if (!AddDevNodeToListCallBack || AddDevNodeToListCallBack(HardwareWiz, &DevInfoData)) {
                
                *DevicesDetected += 1;

                //
                // Add the item to the ListView
                //
                AddItemToListView(HardwareWiz,
                                  hwndListView,
                                  DevInfoData.DevInst,
                                  DevNodeProblem,
                                  HiddenDevice,
                                  SelectedDevInst);
            }
        }

        DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    }
    
    SetupDiDestroyDeviceInfoList(hDeviceInfo);
    
    return TRUE;
}
