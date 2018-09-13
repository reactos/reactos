//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       miscutil.c
//
//--------------------------------------------------------------------------

#include "HotPlug.h"
#include <regstr.h>
#include <initguid.h>
#include <ntddstor.h>
#include <wdmguid.h>

LPTSTR
FormatString(
    LPCTSTR format,
    ...
    )
{
    LPTSTR str = NULL;
    va_list arglist;
    va_start(arglist, format);
    
    if (FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ALLOCATE_BUFFER,
                      format, 
                      0, 
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                      (LPTSTR)&str, 
                      0, 
                      &arglist
                      ) == 0) {
        str = NULL;
    }
    
    va_end(arglist);

    return str;
}

PTCHAR
BuildLocationInformation(
    DEVINST DevInst,
    HMACHINE hMachine
    )
{
    CONFIGRET ConfigRet;
    ULONG ulSize;
    DWORD UINumber;
    PTCHAR Location = NULL;
    PTCHAR ParentName;
    DEVINST DevInstParent;
    int BusLocationStringId;
    TCHAR szBuffer[MAX_PATH];
    TCHAR UINumberDescFormat[MAX_PATH];
    TCHAR szFormat[MAX_PATH];      

    szBuffer[0] = TEXT('\0');

    //
    // Get the LocationInformation for this device.
    //
    ulSize = sizeof(szBuffer);
    CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                        CM_DRP_LOCATION_INFORMATION,
                                        NULL,
                                        szBuffer,
                                        &ulSize,
                                        0,
                                        hMachine
                                        );

    //
    // UINumber has precedence over all other location information so check if this
    // device has a UINumber
    //
    ulSize = sizeof(UINumber);
    if ((CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                             CM_DRP_UI_NUMBER,
                                             NULL,
                                             &UINumber,
                                             &ulSize,
                                             0,
                                             hMachine
                                             ) == CR_SUCCESS) &&
        (ulSize > 0)) {

        UINumberDescFormat[0] = TEXT('\0');
        ulSize = sizeof(UINumberDescFormat);

        //
        // Get the UINumber description format string from the device's parent,
        // if there is one, otherwise default to 'Location %1'
        if ((CM_Get_Parent_Ex(&DevInstParent, DevInst, 0, hMachine) == CR_SUCCESS) &&
            (CM_Get_DevNode_Registry_Property_Ex(DevInstParent,
                                                 CM_DRP_UI_NUMBER_DESC_FORMAT,
                                                 NULL,
                                                 UINumberDescFormat,
                                                 &ulSize,
                                                 0,
                                                 hMachine) == CR_SUCCESS) &&
            *UINumberDescFormat) {

        } else {
            LoadString(hHotPlug, IDS_UI_NUMBER_DESC_FORMAT, UINumberDescFormat, sizeof(UINumberDescFormat)/sizeof(TCHAR));
        }

        //
        // Prepend "at " to the begining of the UINumber string.
        //
        LoadString(hHotPlug, IDS_AT, szFormat, sizeof(szFormat)/sizeof(TCHAR));
        lstrcat(szFormat, UINumberDescFormat);

        //
        // Fill in the UINumber string
        //
        Location = FormatString(szFormat, UINumber);
    }

    //
    // We don't have a UINumber but we do have LocationInformation
    //
    else if (*szBuffer) {
        LoadString(hHotPlug, IDS_LOCATION, szFormat, sizeof(szFormat)/sizeof(TCHAR));
        Location = LocalAlloc(LPTR, lstrlen(szBuffer)*sizeof(TCHAR) + sizeof(szFormat) + sizeof(TCHAR));
        if (Location) {
            wsprintf(Location, szFormat, szBuffer);
        }
    }

    //
    // We don't have a UINumber or LocationInformation so we need to get a description
    // of the parent of this device.
    //
    else {

        ConfigRet = CM_Get_Parent_Ex(&DevInstParent, DevInst, 0, hMachine);
        if (ConfigRet == CR_SUCCESS) {

            //
            // Try the registry for FRIENDLYNAME
            //

            ulSize = sizeof(szBuffer);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInstParent,
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
                ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInstParent,
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
                    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInstParent,
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
                        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DevInstParent,
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
        }

        if (*szBuffer) {
            LoadString(hHotPlug, IDS_LOCATION_NOUINUMBER, szFormat, sizeof(szFormat)/sizeof(TCHAR));
            Location = LocalAlloc(LPTR, lstrlen(szBuffer)*sizeof(TCHAR) + sizeof(szFormat) + sizeof(TCHAR));
            if (Location) {
                wsprintf(Location, szFormat, szBuffer);
            }
        }
    }

    return Location;
}

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





/* ----------------------------------------------------------------------
 * SetDlgText - Set Dialog Text Field
 *
 * Concatenates a number of string resources and does a SetWindowText()
 * for a dialog text control.
 *
 * Parameters:
 *
 *  hDlg         - Dialog handle
 *  iControl     - Dialog control ID to receive text
 *  nStartString - ID of first string resource to concatenate
 *  nEndString   - ID of last string resource to concatenate
 *
 *  Note: the string IDs must be consecutive.
 */

void
SetDlgText(HWND hDlg, int iControl, int nStartString, int nEndString)
{
    int     iX;
    TCHAR   szText[MAX_PATH];

    szText[0] = '\0';
    for (iX = nStartString; iX<= nEndString; iX++) {
         LoadString(hHotPlug,
                    iX,
                    szText + lstrlen(szText),
                    sizeof(szText)/sizeof(TCHAR) - lstrlen(szText)
                    );
        }

    if (iControl) {
        SetDlgItemText(hDlg, iControl, szText);
        }
    else {
        SetWindowText(hDlg, szText);
        }

}


VOID
HotPlugPropagateMessage(
    HWND hWnd,
    UINT uMessage,
    WPARAM wParam,
    LPARAM lParam
    )
{
    while ((hWnd = GetWindow(hWnd, GW_CHILD))) {
        SendMessage(hWnd, uMessage, wParam, lParam);
        }
}

BOOL
RemovalPermission(
   void
   )
{
    return TRUE;
}


int
HPMessageBox(
   HWND hWnd,
   int  IdText,
   int  IdCaption,
   UINT Type
   )
{
   TCHAR szText[MAX_PATH];
   TCHAR szCaption[MAX_PATH];

   if (LoadString(hHotPlug, IdText, szText, MAX_PATH) &&
       LoadString(hHotPlug, IdCaption, szCaption, MAX_PATH))
     {
       return MessageBox(hWnd, szText, szCaption, Type);
       }

   return IDIGNORE;
}



void
InvalidateTreeItemRect(
    HWND hwndTree,
    HTREEITEM  hTreeItem
    )
{
    RECT rect;

    if (hTreeItem && TreeView_GetItemRect(hwndTree, hTreeItem, &rect, FALSE)) {
        
        InvalidateRect(hwndTree, &rect, FALSE);
    }
}




DWORD
GetHotPlugFlags(
    PHKEY phKey
    )
{
    HKEY hKey;
    LONG Error;
    DWORD HotPlugFlags, cbHotPlugFlags;

    Error = RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SYSTRAY, &hKey);
    if (Error == ERROR_SUCCESS) {

        cbHotPlugFlags = sizeof(HotPlugFlags);

        Error = RegQueryValueEx(hKey,
                                szHotPlugFlags,
                                NULL,
                                NULL,
                                (PVOID)&HotPlugFlags,
                                &cbHotPlugFlags
                                );

        if (phKey) {
            *phKey = hKey;
        }
        else {
            RegCloseKey(hKey);
        }
    }

    if (Error != ERROR_SUCCESS) {
        HotPlugFlags = 0;
    }

    return HotPlugFlags;
}

//
// This function determines if the device is a boot storage device.
// We spit out a warning when users are trying to remove or disable
// a boot storage device(or a device contains a boot storage device).
//
// INPUT:
//  NONE
// OUTPUT:
//  TRUE  if the device is a boot device
//  FALSE if the device is not a boot device
LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    )
{
    ULONG ulSize;
    TCHAR szBuffer[MAX_PATH];
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
    PTSTR DriveString = NULL;
    PTSTR DeviceInterface = NULL;

    if (CM_Get_Device_ID_Ex(DevInst,
                            DeviceID,
                            sizeof(DeviceID)/sizeof(TCHAR),
                            0,
                            NULL
                            ) != CR_SUCCESS) {

        return FALSE;
    }

    // create a device info list contains all the interface classed
    // exposed by this device.
    ulSize = 0;

    if ((CM_Get_Device_Interface_List_Size(&ulSize,
                                           (LPGUID)&VolumeClassGuid,
                                           DeviceID,
                                           0)  == CR_SUCCESS) &&
        (ulSize > 1) &&
        ((DeviceInterface = LocalAlloc(LPTR, ulSize*sizeof(TCHAR))) != NULL) &&
        (CM_Get_Device_Interface_List((LPGUID)&VolumeClassGuid,
                                      DeviceID,
                                      DeviceInterface,
                                      ulSize,
                                      0
                                      )  == CR_SUCCESS) &&
        *DeviceInterface)
    {
        LPTSTR devicePath, p;
        TCHAR thisVolumeName[MAX_PATH];
        TCHAR enumVolumeName[MAX_PATH];
        TCHAR driveName[4];
        ULONG length;
        BOOL bResult;

        length = lstrlen(DeviceInterface);
        devicePath = LocalAlloc(LPTR, (length + 1) * sizeof(TCHAR) + sizeof(UNICODE_NULL));

        if (devicePath) {

            lstrcpyn(devicePath, DeviceInterface, length + 1);

            p = wcschr(&(devicePath[4]), TEXT('\\'));

            if (!p) {
                //
                // No refstring is present in the symbolic link; add a trailing
                // '\' char (as required by GetVolumeNameForVolumeMountPoint).
                //
                p = devicePath + length;
                *p = TEXT('\\');
            }

            p++;
            *p = UNICODE_NULL;

            thisVolumeName[0] = UNICODE_NULL;
            bResult = GetVolumeNameForVolumeMountPoint(devicePath,
                                                       thisVolumeName,
                                                       MAX_PATH
                                                       );
            LocalFree(devicePath);

            if (bResult && thisVolumeName[0]) {

                driveName[1] = TEXT(':');
                driveName[2] = TEXT('\\');
                driveName[3] = TEXT('\0');

                for (driveName[0] = TEXT('A'); driveName[0] <= TEXT('Z'); driveName[0]++) {

                    enumVolumeName[0] = TEXT('\0');

                    GetVolumeNameForVolumeMountPoint(driveName, enumVolumeName, MAX_PATH);

                    if (!lstrcmpi(thisVolumeName, enumVolumeName)) {

                        driveName[2] = TEXT('\0');

                        wsprintf(szBuffer, TEXT(" - (%s)"), driveName);

                        DriveString = LocalAlloc(LPTR, (lstrlen(szBuffer) + 1) * sizeof(TCHAR));
                        
                        if (DriveString) {

                            lstrcpy(DriveString, szBuffer);
                        }
                        
                        break;
                    }
                }
            }
        }
    }

    if (DeviceInterface) {

        LocalFree(DeviceInterface);
    }

    return DriveString;
}

BOOL
IsHotPlugDevice(
    DEVINST DevInst,
    HMACHINE hMachine
    )
/**+

    A device is considered a HotPlug device if the following are TRUE:
        - Does NOT have problem CM_PROB_DEVICE_NOT_THERE
        - has Capability CM_DEVCAP_REMOVABLE
        - does NOT have Capability CM_DEVCAP_SURPRISEREMOVALOK
        - does NOT have Capability CM_DEVCAP_DOCKDEVICE

Returns:
    TRUE if this is a HotPlug device
    FALSE if this is not a HotPlug device.

-**/
{
    DWORD Capabilities;
    DWORD Len;
    DWORD Status, Problem;

    Capabilities = Status = Problem = 0;

    Len = sizeof(Capabilities);

    if ((CM_Get_DevNode_Registry_Property_Ex(DevInst,
                                             CM_DRP_CAPABILITIES,
                                             NULL,
                                             (PVOID)&Capabilities,
                                             &Len,
                                             0,
                                             hMachine) == CR_SUCCESS) &&
        (CM_Get_DevNode_Status_Ex(&Status,
                                  &Problem,
                                  DevInst,
                                  0,
                                  hMachine) == CR_SUCCESS) &&

        ((CM_PROB_DEVICE_NOT_THERE != Problem) &&
         (Capabilities & CM_DEVCAP_REMOVABLE) &&
         !(Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) &&
         !(Capabilities & CM_DEVCAP_DOCKDEVICE))) {

        return TRUE;
    }

    return FALSE;
}
