/*
 *  Copyright (c) 1992-1997 Microsoft Corporation
 *  hotplug routines
 *
 *  09-May-1997 Jonle , created
 *
 */
#include "stdafx.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include "systray.h"
#include <setupapi.h>
#include <cfgmgr32.h>
#include <dbt.h>
#include <initguid.h>
#include <devguid.h>
#include <ks.h>
#include <ksmedia.h>
#include <ntddstor.h>


#define HPLUG_EJECT_EVENT       TEXT("HPlugEjectEvent")


//
// setupapi private exports
//
DWORD
pSetupGuidFromString(
  PWCHAR GuidString,
  LPGUID Guid
  );


typedef struct _HotPlugDevices {
     struct _HotPlugDevices *Next;
     DEVINST DevInst;
     DWORD   DevNodeStatus;
     DWORD   Capabilities;
     WORD    EjectMenuIndex;
     BOOLEAN PendingEvent;
     GUID    ClassGuid;
     PTCHAR  DevName;
     TCHAR   ClassGuidString[MAX_GUID_STRING_LEN];
     TCHAR   DevInstanceId[1];
} HOTPLUGDEVICES, *PHOTPLUGDEVICES;

CONFIGRET InitialConfigRet = CR_SUCCESS;
BOOL HotPlugInitialized = FALSE;
BOOL ShowShellIcon = FALSE;
HICON HotPlugIcon = NULL;
BOOL ServiceEnabled = FALSE;
HANDLE hEjectEvent = NULL;          // Event to if we are in the process of ejecting a device


extern HINSTANCE g_hInstance;       //  Global instance handle 4 this application.

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

LPTSTR
DevNodeToDriveLetter(
    DEVINST DevInst
    )
{
    BOOL Result = FALSE;
    ULONG ulSize;
    TCHAR DeviceID[MAX_DEVICE_ID_LEN];
    LPTSTR DriveName = NULL;
    LPTSTR DeviceInterface = NULL;

    if (CM_Get_Device_ID_Ex(DevInst,
                            DeviceID,
                            sizeof(DeviceID)/sizeof(TCHAR),
                            0,
                            NULL
                            ) != CR_SUCCESS) {

        return FALSE;
    }
    
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

                        DriveName = LocalAlloc(LPTR, (lstrlen(driveName) + 1) * sizeof(TCHAR));
                        
                        if (DriveName) {

                            lstrcpy(DriveName, driveName);
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
    
    return DriveName;
}

int
CollectRelationDriveLetters(
    DEVINST DevInst,
    LPTSTR ListOfDrives,
    HMACHINE hMachine
    )
/*++

    This function looks at the removal relations of the specified DevInst and adds any drive
    letters associated with these removal relations to the ListOfDrives.
    
Return:
    Number of drive letters added to the list.    

--*/
{
    int NumberOfDrives = 0;
    LPTSTR SingleDrive = NULL;
    TCHAR szSeparator[32];
    DEVINST RelationDevInst;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    ULONG Len;
    PTCHAR DeviceIdRelations, CurrDevId;

    if (CM_Get_Device_ID_Ex(DevInst,
                            DeviceInstanceId,
                            sizeof(DeviceInstanceId) * sizeof(TCHAR),
                            0,
                            hMachine) == CR_SUCCESS) {

        Len = 0;
        if ((CM_Get_Device_ID_List_Size_Ex(&Len,
                                           DeviceInstanceId,
                                           CM_GETIDLIST_FILTER_REMOVALRELATIONS,
                                           hMachine) == CR_SUCCESS) &&
            (Len)) {

            DeviceIdRelations = LocalAlloc(LPTR, Len*sizeof(TCHAR));
            *DeviceIdRelations = TEXT('\0');

            if (DeviceIdRelations) {

                if ((CM_Get_Device_ID_List_Ex(DeviceInstanceId,
                                              DeviceIdRelations,
                                              Len,
                                              CM_GETIDLIST_FILTER_REMOVALRELATIONS,
                                              hMachine) == CR_SUCCESS) &&
                    (*DeviceIdRelations)) {

                    for (CurrDevId = DeviceIdRelations; *CurrDevId; CurrDevId += lstrlen(CurrDevId) + 1) {

                        if (CM_Locate_DevNode_Ex(&RelationDevInst, CurrDevId, 0, hMachine) == CR_SUCCESS) {

                            SingleDrive = DevNodeToDriveLetter(RelationDevInst);

                            if (SingleDrive) {

                                NumberOfDrives++;

                                //
                                // If this is not the first drive the add a comma space separator
                                //
                                if (ListOfDrives[0] != TEXT('\0')) {

                                    LoadString(g_hInstance, IDS_SEPARATOR, szSeparator, sizeof(szSeparator)/sizeof(TCHAR));

                                    lstrcat(ListOfDrives, szSeparator);
                                } 

                                lstrcat(ListOfDrives, SingleDrive);

                                LocalFree(SingleDrive);
                            }
                        }
                    }
                }

                LocalFree(DeviceIdRelations);
            }
        }
    }

    return NumberOfDrives;
}

int
CollectDriveLettersForDevNodeWorker(
    DEVINST DevInst,
    LPTSTR ListOfDrives,
    HMACHINE hMachine
    )
{
    DEVINST ChildDevInst;
    DEVINST SiblingDevInst;
    int NumberOfDrives = 0;
    LPTSTR SingleDrive = NULL;
    TCHAR szSeparator[32];

    //
    // Enumerate through all of the siblings and children of this devnode
    //
    do {

        ChildDevInst = 0;
        SiblingDevInst = 0;

        CM_Get_Child_Ex(&ChildDevInst, DevInst, 0, hMachine);
        CM_Get_Sibling_Ex(&SiblingDevInst, DevInst, 0, hMachine);

        //
        // Only get the drive letter for this device if it is NOT a hotplug
        // device.  If it is a hotplug device then it will have it's own
        // subtree that contains it's drive letters.
        //
        if (!IsHotPlugDevice(DevInst, hMachine)) {
        
            SingleDrive = DevNodeToDriveLetter(DevInst);
    
            if (SingleDrive) {
    
                NumberOfDrives++;
                
                //
                // If this is not the first drive the add a comma space separator
                //
                if (ListOfDrives[0] != TEXT('\0')) {

                    LoadString(g_hInstance, IDS_SEPARATOR, szSeparator, sizeof(szSeparator)/sizeof(TCHAR));
    
                    lstrcat(ListOfDrives, szSeparator);
                } 
    
                lstrcat(ListOfDrives, SingleDrive);
    
                LocalFree(SingleDrive);
            }
    
            //
            // Get the drive letters for any children of this devnode
            //
            if (ChildDevInst) {
    
                NumberOfDrives += CollectDriveLettersForDevNodeWorker(ChildDevInst, ListOfDrives, hMachine);
            }

            //
            // Add the drive letters for any removal relations of this devnode
            //
            NumberOfDrives += CollectRelationDriveLetters(DevInst, ListOfDrives, hMachine);
        }

    } while ((DevInst = SiblingDevInst) != 0);

    return NumberOfDrives;
}

LPTSTR
CollectDriveLettersForDevNode(
    DEVINST DevInst,
    HMACHINE hMachine
    )
{
    TCHAR Format[MAX_PATH];
    TCHAR ListOfDrives[MAX_PATH];
    DEVINST ChildDevInst;
    int NumberOfDrives = 0;
    LPTSTR SingleDrive = NULL;
    LPTSTR FinalDriveString = NULL;

    ListOfDrives[0] = TEXT('\0');

    //
    //First get any drive letter associated with this devnode
    //
    SingleDrive = DevNodeToDriveLetter(DevInst);
    
    if (SingleDrive) {

        NumberOfDrives++;

        lstrcat(ListOfDrives, SingleDrive);

        LocalFree(SingleDrive);
    }

    //
    // Next add on any drive letters associated with the children
    // of this devnode
    //
    ChildDevInst = 0;
    CM_Get_Child_Ex(&ChildDevInst, DevInst, 0, hMachine);

    if (ChildDevInst) {
    
        NumberOfDrives += CollectDriveLettersForDevNodeWorker(ChildDevInst, ListOfDrives, hMachine);
    }

    //
    // Finally add on any drive letters associated with the removal relations
    // of this devnode
    //
    NumberOfDrives += CollectRelationDriveLetters(DevInst, ListOfDrives, hMachine);
    
    if (ListOfDrives[0] != TEXT('\0')) {

        LoadString(g_hInstance, 
                   (NumberOfDrives > 1) ? IDS_DISKDRIVES : IDS_DISKDRIVE,
                   Format,
                   sizeof(Format)/sizeof(TCHAR)
                   );


        FinalDriveString = LocalAlloc(LPTR, (lstrlen(ListOfDrives) + lstrlen(Format) + 1) * sizeof(TCHAR));

        if (FinalDriveString) {
        
            wsprintf(FinalDriveString, Format, ListOfDrives);
        }
    }

    return FinalDriveString;
}

DWORD
GetHotPlugFlags(
   void
   )
{
    HKEY hKey;
    LONG Error;
    DWORD HotPlugFlags, cbHotPlugFlags;

    Error = RegCreateKey(HKEY_CURRENT_USER, REGSTR_PATH_SYSTRAY, &hKey);
    
    if (Error == ERROR_SUCCESS) {

        cbHotPlugFlags = sizeof(HotPlugFlags);

        Error = RegQueryValueEx(hKey,
                                TEXT("HotPlugFlags"),
                                NULL,
                                NULL,
                                (PVOID)&HotPlugFlags,
                                &cbHotPlugFlags
                                );
        RegCloseKey(hKey);
        if (Error == ERROR_SUCCESS) {
            
            return HotPlugFlags;
        }
    }

    return 0;
}




ULONG
RegistryDeviceName(
    DEVINST DevInst,
    PTCHAR  Buffer,
    DWORD   cbBuffer
    )
{
    ULONG ulSize = 0;
    CONFIGRET ConfigRet;
    LPTSTR ListOfDrives = NULL;

    //
    // Get the list of drives
    //
    ListOfDrives = CollectDriveLettersForDevNode(DevInst, NULL);

    //
    // Try the registry for FRIENDLYNAME
    //

    ulSize = cbBuffer;
    *Buffer = TEXT('\0');
    ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                 CM_DRP_FRIENDLYNAME,
                                                 NULL,
                                                 Buffer,
                                                 &ulSize,
                                                 0);

    if (ConfigRet != CR_SUCCESS || !(*Buffer)) {
        
        //
        // Try the registry for DEVICEDESC
        //
    
        ulSize = cbBuffer;
        *Buffer = TEXT('\0');
        ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                     CM_DRP_DEVICEDESC,
                                                     NULL,
                                                     Buffer,
                                                     &ulSize,
                                                     0);
    
        if (ConfigRet != CR_SUCCESS || !(*Buffer)) {
            
        
            //
            // try classname
            //
        
            ulSize = cbBuffer;
            *Buffer = TEXT('\0');
            ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                         CM_DRP_CLASS,
                                                         NULL,
                                                         Buffer,
                                                         &ulSize,
                                                         0);
        
            if (ConfigRet != CR_SUCCESS || !(*Buffer)) {
                
                ulSize = 0;;
            }
        }
    }

    //
    // Concatonate on the list of drive letters if this device has drive 
    // letters and there is enough space
    //
    if (ListOfDrives) {

        if ((ulSize + (lstrlen(ListOfDrives) * sizeof(TCHAR))) < cbBuffer) {

            lstrcat(Buffer, ListOfDrives);

            ulSize += (lstrlen(ListOfDrives) * sizeof(TCHAR));
        }

        LocalFree(ListOfDrives);
    } 
            
    return ulSize;
}

BOOL
_AnyHotPlugDevices(
    DEVINST      DeviceInstance
    )
{
    ULONG      Len;
    DWORD      Capabilities, Status, Problem;
    CONFIGRET  ConfigRet;
    DEVINST    ChildDeviceInstance;


    do {
        //
        // Device capabilities
        //

        Len = sizeof(Capabilities);
        ConfigRet = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                                     CM_DRP_CAPABILITIES,
                                                     NULL,
                                                     (PVOID)&Capabilities,
                                                     &Len,
                                                     0
                                                     );
        if (ConfigRet != CR_SUCCESS) {
            
            Capabilities = 0;
        }

        ConfigRet = CM_Get_DevNode_Status(&Status,
                                          &Problem,
                                          DeviceInstance,
                                          0
                                          );

        if (ConfigRet != CR_SUCCESS) {

            Status = 0;
            Problem = 0;
        }

        //
        // Only show devices that:
        //  -> do NOT have problem CM_PROB_DEVICE_NOT_THERE
        //  -> are removable
        //  -> are NOT surprise removable
        //  -> are NOT dock devices
        //
        if ((Problem != CM_PROB_DEVICE_NOT_THERE) &&
            (Capabilities & CM_DEVCAP_REMOVABLE) &&
            !(Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) &&
            !(Capabilities & CM_DEVCAP_DOCKDEVICE)) {

            //
            // As soon as we find one device then we need to show the icon, so no need
            // to continue the search.
            //
            return TRUE;
        }

        //
        // If this devinst has children, then recurse to fill in its
        // child sibling list.
        //

        ConfigRet = CM_Get_Child(&ChildDeviceInstance,
                                 DeviceInstance,
                                 0
                                 );

        if (ConfigRet == CR_SUCCESS) {
            
            if (_AnyHotPlugDevices(ChildDeviceInstance)) {
                
                return TRUE;
            }
        }

        //
        // Next sibling ...
        //

        ConfigRet = CM_Get_Sibling(&DeviceInstance,
                                      DeviceInstance,
                                      0
                                      );

    } while (ConfigRet == CR_SUCCESS);


    return FALSE;
}

BOOL
AnyHotPlugDevices(
    void
    )
{
    CONFIGRET ConfigRet;
    DEVNODE DeviceInstance, ChildDeviceInstance;

    ConfigRet = CM_Locate_DevNode(&DeviceInstance,
                                 NULL,
                                 CM_LOCATE_DEVNODE_NORMAL
                                 );
    
    InitialConfigRet = ConfigRet;
    
    if (ConfigRet != CR_SUCCESS) {
        
        return FALSE;
    }

    ConfigRet = CM_Get_Child(&ChildDeviceInstance,
                             DeviceInstance,
                             0
                             );

    if (ConfigRet != CR_SUCCESS) {
        
        return FALSE;
    }

    return(_AnyHotPlugDevices(ChildDeviceInstance));
}

BOOL
_AddHotPlugDevices(
    DEVINST      DeviceInstance,
    PHOTPLUGDEVICES *HotPlugDevicesList
    )
{
    PHOTPLUGDEVICES HotPlugDevice;
    DWORD      Len, LenDevName, LenDevInstanceId;
    DWORD      Capabilities;
    CONFIGRET  ConfigRet;
    ULONG      Problem, DevNodeStatus;
    DEVINST    ChildDeviceInstance;
    TCHAR      DevInstanceId[MAX_DEVICE_ID_LEN];
    TCHAR      DevName[MAX_PATH];


    do {
        //
        // Device capabilities
        //

        Len = sizeof(Capabilities);
        ConfigRet = CM_Get_DevNode_Registry_Property(DeviceInstance,
                                                     CM_DRP_CAPABILITIES,
                                                     NULL,
                                                     (PVOID)&Capabilities,
                                                     &Len,
                                                     0
                                                     );
        if (ConfigRet != CR_SUCCESS) {
            
            Capabilities = 0;
        }

        ConfigRet = CM_Get_DevNode_Status(&DevNodeStatus,
                                          &Problem,
                                          DeviceInstance,
                                          0
                                          );

        if (ConfigRet != CR_SUCCESS) {
            
            DevNodeStatus = 0;
            Problem = 0;
        }



        //
        // Only show devices that:
        //  -> do NOT have problem CM_PROB_DEVICE_NOT_THERE
        //  -> are removable
        //  -> are NOT surprise removable
        //  -> are NOT dock devices
        //
        if ((Problem != CM_PROB_DEVICE_NOT_THERE) &&
            (Capabilities & CM_DEVCAP_REMOVABLE) &&
            !(Capabilities & CM_DEVCAP_SURPRISEREMOVALOK) &&
            !(Capabilities & CM_DEVCAP_DOCKDEVICE)) {

            *DevInstanceId = TEXT('\0');
            LenDevInstanceId = sizeof(DevInstanceId);
            ConfigRet = CM_Get_Device_ID(DeviceInstance,
                                         (PVOID)DevInstanceId,
                                         LenDevInstanceId,
                                         0
                                         );

            if (ConfigRet != CR_SUCCESS || !*DevInstanceId) {
                
                *DevInstanceId = TEXT('\0');
                LenDevInstanceId = 0;
            }


            Len = sizeof(HOTPLUGDEVICES) + LenDevInstanceId;
            HotPlugDevice = LocalAlloc(LPTR, Len);
            
            if (!HotPlugDevice) {
                
                return FALSE;
            }

            //
            // link it in
            //
            HotPlugDevice->Next = *HotPlugDevicesList;
            *HotPlugDevicesList = HotPlugDevice;
            HotPlugDevice->DevInst = DeviceInstance;
            HotPlugDevice->DevNodeStatus = DevNodeStatus;
            HotPlugDevice->Capabilities = Capabilities;

            //
            // Fetch the ClassGuid
            //

            Len = sizeof(HotPlugDevice->ClassGuidString);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_CLASSGUID,
                                                            NULL,
                                                            HotPlugDevice->ClassGuidString,
                                                            &Len,
                                                            0,
                                                            0
                                                            );


            if (ConfigRet != CR_SUCCESS) {
                
                *HotPlugDevice->ClassGuidString = TEXT('\0');
            }

            //
            // copy in the names
            //

            memcpy(HotPlugDevice->DevInstanceId, DevInstanceId, LenDevInstanceId);

            LenDevName = RegistryDeviceName(DeviceInstance, DevName, sizeof(DevName));
            HotPlugDevice->DevName = LocalAlloc(LPTR, LenDevName + sizeof(TCHAR));
            
            if (HotPlugDevice->DevName) {
                
                memcpy(HotPlugDevice->DevName, DevName, LenDevName);
            }
        }


        //
        // If this devinst has children, then recurse to fill in its
        // child sibling list.
        //

        ConfigRet = CM_Get_Child(&ChildDeviceInstance,
                                 DeviceInstance,
                                 0
                                 );

        if (ConfigRet == CR_SUCCESS) {
            
            if (!_AddHotPlugDevices(ChildDeviceInstance, HotPlugDevicesList)) {
                
                return FALSE;
            }
        }

        //
        // Next sibling ...
        //

        ConfigRet = CM_Get_Sibling(&DeviceInstance,
                                      DeviceInstance,
                                      0
                                      );

    } while (ConfigRet == CR_SUCCESS);


    return TRUE;
}

BOOL
AddHotPlugDevices(
    PHOTPLUGDEVICES *HotPlugDevicesList
    )
{
    CONFIGRET ConfigRet;
    DEVNODE DeviceInstance, ChildDeviceInstance;

    *HotPlugDevicesList = NULL;

    ConfigRet = CM_Locate_DevNode(&DeviceInstance,
                                 NULL,
                                 CM_LOCATE_DEVNODE_NORMAL
                                 );
    
    InitialConfigRet = ConfigRet;
    
    if (ConfigRet != CR_SUCCESS) {
        
        return FALSE;
    }

    ConfigRet = CM_Get_Child(&ChildDeviceInstance,
                             DeviceInstance,
                             0
                             );

    if (ConfigRet != CR_SUCCESS) {
        
        return FALSE;
    }

    return (_AddHotPlugDevices(ChildDeviceInstance, HotPlugDevicesList));
}


void
FreeHotPlugDevicesList(
    PHOTPLUGDEVICES *HotPlugDevicesList
    )
{
    PHOTPLUGDEVICES HotPlugDevices, HotPlugDevicesFree;

    HotPlugDevices = *HotPlugDevicesList;
    *HotPlugDevicesList = NULL;

    while (HotPlugDevices) {

        HotPlugDevicesFree = HotPlugDevices;
        HotPlugDevices = HotPlugDevicesFree->Next;
        
        if (HotPlugDevicesFree->DevName) {
           
           LocalFree(HotPlugDevicesFree->DevName);
           HotPlugDevicesFree->DevName = NULL;
        }

        LocalFree(HotPlugDevicesFree);
    }
}


/*
 *  Shows or deletes the shell notify icon and tip
 */

void
HotPlugShowNotifyIcon(
    HWND hWnd,
    BOOL bShowIcon
    )
{
    TCHAR HotPlugTip[64];

    ShowShellIcon = bShowIcon;

    if (bShowIcon) {
        
        LoadString(g_hInstance,
                   IDS_HOTPLUGTIP,
                   HotPlugTip,
                   sizeof(HotPlugTip)/sizeof(TCHAR)
                   );

        HotPlugIcon = LoadImage(g_hInstance,
                                MAKEINTRESOURCE(IDI_HOTPLUG),
                                IMAGE_ICON,
                                16,
                                16,
                                0
                                );

        SysTray_NotifyIcon(hWnd, STWM_NOTIFYHOTPLUG, NIM_ADD, HotPlugIcon, HotPlugTip);

    }
    
    else {
        
        SysTray_NotifyIcon(hWnd, STWM_NOTIFYHOTPLUG, NIM_DELETE, NULL, NULL);
        
        if (HotPlugIcon) {
            
            DestroyIcon(HotPlugIcon);
        }
    }
}



//
// first time intialization of Hotplug module.
//



BOOL
HotPlugInit(
    HWND hWnd
    )
{
    DEV_BROADCAST_DEVICEINTERFACE dbi;
    CONFIGRET ConfigRet;
    INT WaitStatus;
    HINSTANCE hUsbWatch;
    FARPROC proc;
    
    //
    // If we are already initialized then just return whether there are any HotPlug
    // devices or not.
    //
    if (HotPlugInitialized) {
        
        return (AnyHotPlugDevices());
    }
    
    //
    // Load the Usb Watch utility which pops up whenever there's an error on the 
    // Universal Serial Bus
    //
    hUsbWatch = LoadLibrary(TEXT("usbuidll.dll"));
    
    if (hUsbWatch) {
        
        proc = GetProcAddress( hUsbWatch, "USBErrorMessagesInit" );
        
        if (proc) {
            proc ();
        }
    }

    hEjectEvent = CreateEvent(NULL, TRUE, TRUE, HPLUG_EJECT_EVENT);
    
    HotPlugInitialized = TRUE;

    return (AnyHotPlugDevices());
}




BOOL
HotPlug_CheckEnable(
    HWND hWnd,
    BOOL bSvcEnabled
    )
/*++

Routine Description:

   Called at init time and whenever services are enabled/disabled.
   Hotplug is always alive to receive device change notifications.

   The shell notify icon is enabled\disabled depending on:

   - systray registry setting for services,
        AND
   - availability of removable devices.


Arguments:

   hwnd - Our Window handle

   bSvcEnabled - TRUE Service is being enabled.


Return Value:

   BOOL Returns TRUE if active.


--*/

{
    BOOL EnableShellIcon;

    //
    // If we are being enabled and we are already enabled, or we
    // are being disabled and we are already disabled then just
    // return since we have nothing to do.
    //
    if (ServiceEnabled == bSvcEnabled) {

        return ServiceEnabled;
    }

    ServiceEnabled = bSvcEnabled;

    EnableShellIcon = bSvcEnabled && HotPlugInit(hWnd);

    HotPlugShowNotifyIcon(hWnd, EnableShellIcon);

    return EnableShellIcon;
}

DWORD
HotPlugEjectDevice_Thread(
   PTCHAR DeviceInstanceId
   )
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

    //
    // Set the hEjectEvent so that the right-click popup menu will work again now that we are finished
    // ejecting/stopping the device.
    //
    SetEvent(hEjectEvent);

    SetLastError(ConfigRet);
    return (ConfigRet == CR_SUCCESS);
}

void
HotPlugEjectDevice(
    HWND hwnd,
    PTCHAR DeviceInstanceId
    )
{
    DWORD ThreadId;

    //
    // Reset the hEjectEvent so that the user can't bring up the right-click popup menu when
    // we are in the process of ejecting/stopping a device.
    //
    ResetEvent(hEjectEvent);

    //
    // We need to have stobject.dll eject/stop the device on a separate thread because if
    // we remove a device that stobject.dll listens for (battery, sound, ect.) we will cause
    // a large delay and the eject/stop could end up getting vetoed because the stobject.dll
    // code could not be processed and release it's handles because we were locking up the main
    // thread.
    //
    CreateThread(NULL,
                 0,
                 (LPTHREAD_START_ROUTINE)HotPlugEjectDevice_Thread,
                 (LPVOID)DeviceInstanceId,
                 0,
                 &ThreadId
                 );
}

void
HotPlug_Timer(
   HWND hwnd
   )
/*++

Routine Description:

   Hotplug Timer msg handler, used to invoke EjectMenu for single Left click

Arguments:

   hDlg - Our Window handle


Return Value:

   BOOL Returns TRUE if active.


--*/


{
    POINT pt;
    HMENU EjectMenu;
    UINT MenuIndex;
    PHOTPLUGDEVICES HotPlugDevicesList;
    PHOTPLUGDEVICES SingleHotPlugDevice;
    TCHAR  MenuDeviceName[MAX_PATH+64];
    TCHAR  Format[64];


    KillTimer(hwnd, HOTPLUG_TIMER_ID);

    if (!HotPlugInitialized) {

        PostMessage(hwnd, STWM_ENABLESERVICE, 0, TRUE);
        return;
    }

    //
    // We only want to create the popup menu if the hEjectEvent is signaled.  If it is not
    // signaled then we are in the middle of ejecting/stopping a device on a separate thread
    // and don't want to allow the user to bring up the menu until we are finished with that 
    // device.
    //
    if (!hEjectEvent ||
        WaitForSingleObject(hEjectEvent, 0) == WAIT_OBJECT_0) {
    
        //
        // We are not in the middle of ejecting/stopping a device so we should display the popup
        // menu.
        //
        EjectMenu = CreatePopupMenu();
       
        if (!EjectMenu) {
    
            return;
        }
    
        SetForegroundWindow(hwnd);
        GetCursorPos(&pt);
    
    
        //
        // Add each of the removable devices in the list to the menu.
        //
    
        if (!AddHotPlugDevices(&HotPlugDevicesList)) {
    
            return;
        }
    
        SingleHotPlugDevice = HotPlugDevicesList;
    
    
        //
        // Add a title and separator at the top of the menu.
        //
    
        LoadString(g_hInstance,
                   IDS_HPLUGMENU_REMOVE,
                   Format,
                   sizeof(Format)/sizeof(TCHAR)
                   );
    
    
        MenuIndex = 1;
       
        while (SingleHotPlugDevice) {
           
            wsprintf(MenuDeviceName, Format, SingleHotPlugDevice->DevName);
            AppendMenu(EjectMenu, MF_STRING, MenuIndex, MenuDeviceName);
            SingleHotPlugDevice->EjectMenuIndex = MenuIndex++;
            SingleHotPlugDevice = SingleHotPlugDevice->Next;
        }
    
    
        MenuIndex = TrackPopupMenu(EjectMenu,
                                   TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                                   pt.x,
                                   pt.y,
                                   0,
                                   hwnd,
                                   NULL
                                   );
    
    
        // now do an eject!
    
        SingleHotPlugDevice = HotPlugDevicesList;
        
        while (SingleHotPlugDevice) {
           
            if (MenuIndex == SingleHotPlugDevice->EjectMenuIndex) {
               
                HotPlugEjectDevice(hwnd, SingleHotPlugDevice->DevInstanceId);
                break;
            }
    
            SingleHotPlugDevice = SingleHotPlugDevice->Next;
        }
    
    
        DestroyMenu(EjectMenu);
    
        if (!SingleHotPlugDevice) {
    
            SetIconFocus(hwnd, STWM_NOTIFYHOTPLUG);
        }
    
        FreeHotPlugDevicesList(&HotPlugDevicesList);
    }
    
    return;
}



void
HotPlugContextMenu(
   HWND hwnd
   )
{
   POINT pt;
   HMENU ContextMenu;
   UINT MenuIndex;
   TCHAR Buffer[MAX_PATH];


   ContextMenu = CreatePopupMenu();
   if (!ContextMenu) {
       return;
       }

   SetForegroundWindow(hwnd);
   GetCursorPos(&pt);

   LoadString(g_hInstance, IDS_HPLUGMENU_PROPERTIES, Buffer, sizeof(Buffer)/sizeof(TCHAR));
   AppendMenu(ContextMenu, MF_STRING,IDS_HPLUGMENU_PROPERTIES, Buffer);

   SetMenuDefaultItem(ContextMenu, IDS_HPLUGMENU_PROPERTIES, FALSE);


   MenuIndex = TrackPopupMenu(ContextMenu,
                              TPM_RIGHTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY,
                              pt.x,
                              pt.y,
                              0,
                              hwnd,
                              NULL
                              );



   switch (MenuIndex) {
       case IDS_HPLUGMENU_PROPERTIES:
            SysTray_RunProperties(IDS_RUNHPLUGPROPERTIES);
            break;

       }

   DestroyMenu(ContextMenu);

   SetIconFocus(hwnd, STWM_NOTIFYHOTPLUG);

   return;
}



void
HotPlug_Notify(
   HWND hwnd,
   WPARAM wParam,
   LPARAM lParam
   )

{
    switch (lParam) {
       
    case WM_RBUTTONUP:
        HotPlugContextMenu(hwnd);
        break;

    case WM_LBUTTONDOWN:
        SetTimer(hwnd, HOTPLUG_TIMER_ID, GetDoubleClickTime()+100, NULL);
        break;

    case WM_LBUTTONDBLCLK:
        KillTimer(hwnd, HOTPLUG_TIMER_ID);
        SysTray_RunProperties(IDS_RUNHPLUGPROPERTIES);
        break;
    }

    return;
}


int
HotPlug_DeviceChangeTimer(
   HWND hDlg
   )
{
    BOOL bAnyHotPlugDevices;

    KillTimer(hDlg, HOTPLUG_DEVICECHANGE_TIMERID);

    //
    // Let's see if we have any hot plug devices, which means we
    // need to show the systray icon.
    //
    bAnyHotPlugDevices = AnyHotPlugDevices();

    //
    // If the service is not enabled then don't bother because the icon will
    // NOT be shown
    //
    if (ServiceEnabled) {

        //
        // If there are now some hot plug devices and the icon is not being shown,
        // then show it.
        //
        if (bAnyHotPlugDevices && !ShowShellIcon) {
        
            HotPlugShowNotifyIcon(hDlg, TRUE);
        }

        //
        // If there are NOT any hot plug devices and the icon is still being shown,
        // then hide it.
        //
        else if (!bAnyHotPlugDevices && ShowShellIcon) {

            HotPlugShowNotifyIcon(hDlg, FALSE);
        }
    }

    return 0;
}

void
HotPlug_DeviceChange(
   HWND hwnd,
   WPARAM wParam,
   LPARAM lParam
   )

/*++

Routine Description:

   To avoid deadlock with CM, a timer is started and the timer message handler does 
   the real work.

Arguments:

   hDlg        - Window handle of Dialog

   wParam  - DBT Event

   lParam  - DBT event notification type.

Return Value:

--*/

{
    if (wParam == DBT_DEVNODES_CHANGED) {

        SetTimer(hwnd, HOTPLUG_DEVICECHANGE_TIMERID, 1000, NULL);
    }

    return;
}

void
HotPlug_WmDestroy(
    HWND hWnd
    )
{
    if (hEjectEvent) {
        CloseHandle(hEjectEvent);
    }
}
