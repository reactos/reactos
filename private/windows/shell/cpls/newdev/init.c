//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       init.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include "regstr.h"


HMODULE hNewDev=NULL;
DWORD dwRestartFlags= 0;
DWORD dwSetupFlags=0;
UINT wHelpMessage;


TCHAR *DevicePath = NULL;
TCHAR DefaultInf[128];

HANDLE UpdateDeviceClassUiEvent = NULL;
HANDLE TerminateUiEvent = NULL;
HANDLE hDlgUI = NULL;

LRESULT CALLBACK
DevInstallDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   );



void
BuildDevicePath(
   VOID
   )
{
    TCHAR ValueBuffer[MAX_PATH*2];
    TCHAR *ValueDevicePath;
    DWORD Len, cbData, Type;
    LONG Error;
    HKEY hKey;


    if (DevicePath) {
       
        free(DevicePath);
        DevicePath = NULL;
    }

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, REGSTR_PATH_SETUP, 0, KEY_READ, &hKey);
    if (Error != ERROR_SUCCESS) {

        return;
    }

    ValueDevicePath = ValueBuffer;
    cbData = sizeof(ValueBuffer);
    Error = RegQueryValueEx(hKey,
                            REGSTR_VAL_DEVICEPATH,
                            NULL,
                            &Type,
                            (LPBYTE)ValueDevicePath,
                            &cbData
                            );

    if (Error == ERROR_MORE_DATA) {
       
        if (ValueDevicePath = malloc(cbData)) {
           
            Error = RegQueryValueEx(hKey,
                                    REGSTR_VAL_DEVICEPATH,
                                    NULL,
                                    &Type,
                                    (LPBYTE)ValueDevicePath,
                                    &cbData
                                    );
        }
    }

    RegCloseKey(hKey);

    if (Error != ERROR_SUCCESS) {

        goto BDPExit;
    }


    DevicePath = malloc(cbData*2);
    if (!DevicePath) {

        goto BDPExit;
    }
    
    memset(DevicePath, 0, cbData*2);


    Len = ExpandEnvironmentStrings(ValueDevicePath,
                                   DevicePath,
                                   cbData*2/sizeof(TCHAR)
                                   );

    if (Len > cbData*2/sizeof(TCHAR)) {

        free(DevicePath);
        DevicePath = malloc(Len*sizeof(TCHAR));
        
        if (DevicePath) {

            memset(DevicePath, 0, Len*sizeof(TCHAR));
            ExpandEnvironmentStrings(ValueDevicePath,
                                     DevicePath,
                                     Len
                                     );
        }
    }


BDPExit:

    if (ValueDevicePath && ValueDevicePath != ValueBuffer) {

        free(ValueDevicePath);
    }

    return;
}





BOOL
DllInitialize(
    IN PVOID hmod,
    IN ULONG ulReason,
    IN PCONTEXT pctx OPTIONAL
    )
{
    hNewDev = hmod;

    if (ulReason == DLL_PROCESS_ATTACH) {
        
        DisableThreadLibraryCalls(hmod);

        wHelpMessage   = RegisterWindowMessage(TEXT("ShellHelp"));
        BuildDevicePath();
        LoadString(hNewDev, IDS_DEFAULTINF, DefaultInf, sizeof(DefaultInf)/sizeof(TCHAR));
        IntializeDeviceMapInfo();

        LoadString(hNewDev,
                   IDS_UNKNOWN,
                   (PTCHAR)szUnknown,
                   SIZECHARS(szUnknown)
                   );

        LenUnknown = lstrlen(szUnknown) * sizeof(TCHAR) + sizeof(TCHAR);


        LoadString(hNewDev,
                   IDS_UNKNOWNDEVICE,
                   (PTCHAR)szUnknownDevice,
                   SIZECHARS(szUnknownDevice)
                   );

        LenUnknownDevice = lstrlen(szUnknownDevice) * sizeof(TCHAR) + sizeof(TCHAR);

    }
    
    else if (ulReason == DLL_PROCESS_DETACH) {
        
        free(DevicePath);
    }

    return TRUE;
}




BOOL
InstallDeviceInstance(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot,
   PUPDATEDRIVERINFO UpdateDriverInfo,
   BOOL SilentInstall
   )
{
    PSP_INSTALLWIZARD_DATA  InstallWizard;
    CONFIGRET ConfigRet;
    ULONG ConfigFlag, Len;
    NEWDEVWIZ NewDevWiz;
    SEARCHTHREAD SearchThread;
    SP_DRVINFO_DATA DriverInfoData;
    HWND hDlg;
    MSG Msg;
    HICON hIcon;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;

    //
    // ensure we have a device instance, and that we are Admin.
    //

    if (!DeviceInstanceId  || !*DeviceInstanceId) {
       
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (NoPrivilegeWarning(hwndParent)) {
       
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    memset(&NewDevWiz, 0, sizeof(NewDevWiz));

    NewDevWiz.PromptForReboot = pReboot == NULL;
    NewDevWiz.WizardType = UpdateDriver ? NDWTYPE_UPDATE : NDWTYPE_FOUNDNEW;
    NewDevWiz.SilentMode = FALSE;
    NewDevWiz.UpdateDriverInfo = UpdateDriverInfo;

    NewDevWiz.hDeviceInfo = SetupDiCreateDeviceInfoList(NULL, hwndParent);
   
    if (NewDevWiz.hDeviceInfo == INVALID_HANDLE_VALUE) {
       
        return FALSE;
    }


    NewDevWiz.LastError = ERROR_SUCCESS;

    try {

        NewDevWiz.DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
   
        if (!SetupDiOpenDeviceInfo(NewDevWiz.hDeviceInfo,
                                   DeviceInstanceId,
                                   hwndParent,
                                   0,
                                   &NewDevWiz.DeviceInfoData
                                   ))
        {
            NewDevWiz.LastError = GetLastError();
            goto IDIExit;
        }

#if DBG
        DbgPrint("InstallDevInst %x <%ws>\n", NewDevWiz.DeviceInfoData.DevInst, DeviceInstanceId);
#endif



        ConfigRet = CM_Get_Device_ID(NewDevWiz.DeviceInfoData.DevInst,
                                NewDevWiz.InstallDeviceInstanceId,
                                sizeof(NewDevWiz.InstallDeviceInstanceId)/sizeof(TCHAR),
                                0
                                );

        if (ConfigRet != CR_SUCCESS) {
       
            NewDevWiz.LastError = ConfigRet;
            goto IDIExit;
        }

        SetupDiSetSelectedDevice(NewDevWiz.hDeviceInfo, &NewDevWiz.DeviceInfoData);


        Len = sizeof(ConfigFlag);
        ConfigRet = CM_Get_DevNode_Registry_Property_Ex(NewDevWiz.DeviceInfoData.DevInst,
                                                   CM_DRP_CONFIGFLAGS,
                                                   NULL,
                                                   (PVOID)&ConfigFlag,
                                                   &Len,
                                                   0,
                                                   NULL
                                                   );

        if (ConfigRet == CR_SUCCESS && (ConfigFlag & CONFIGFLAG_MANUAL_INSTALL)) {
       
            NewDevWiz.ManualInstall = TRUE;
        }


        //
        // initialize DeviceInstallParams
        //

        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        
        if (SetupDiGetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {

            DeviceInstallParams.Flags |= DI_SHOWOEM;
            DeviceInstallParams.hwndParent = hwndParent;

            SetupDiSetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }
   
        else {
            
            NewDevWiz.LastError = GetLastError();
            goto IDIExit;
        }


        //
        // Create the Search thread to look for compatible drivers.
        // This thread will sit around waiting for requests until
        // told to go away.
        //
        memset(&SearchThread, 0, sizeof(SEARCHTHREAD));
        NewDevWiz.SearchThread = &SearchThread;

        NewDevWiz.LastError = CreateSearchThread(&NewDevWiz);
   
        if (NewDevWiz.LastError != ERROR_SUCCESS) {
       
            goto IDIExit;
        }


        hDlg = CreateDialogParam(hNewDev,
                            MAKEINTRESOURCE(DLG_DEVINSTALL),
                            hwndParent,
                            DevInstallDlgProc,
                            (LPARAM)&NewDevWiz
                            );
        
        //
        // If we don't have a parent window, center.
        //
        if (!hDlg) {
       
            NewDevWiz.LastError = ERROR_GEN_FAILURE;
            goto IDIExit;
        }

        hIcon = NULL;
           
        hIcon = LoadIcon(hNewDev,MAKEINTRESOURCE(IDI_NEWDEVICEICON));
   
        if (hIcon) {
       
            SendMessage(hDlg, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
            SendMessage(hDlg, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
        }

        //
        // If drivers say this is a silent install, then hide this dialog
        //
        if (!UpdateDriver && 
            (!(NewDevWiz.Capabilities & CM_DEVCAP_SILENTINSTALL) && 
            !SilentInstall)) {
       
            ShowWindow(hDlg, SW_SHOW);
        }


        while (IsWindow(hDlg)) {
      
            if (GetMessage(&Msg, NULL, 0, 0) && !IsDialogMessage(hDlg,&Msg)) {
          
                TranslateMessage(&Msg);
                DispatchMessage(&Msg);
            }
        }

        if (hIcon) {
            
            DestroyIcon(hIcon);
        }

        //
        // copy out the reboot flags for the caller
        // or put up the restart dialog if caller didn't ask for the reboot flag
        //

        if (pReboot) {
        
            *pReboot = NewDevWiz.Reboot;
        }
    
        else if (NewDevWiz.Reboot) {
        
             RestartDialog(hwndParent, NULL, EWX_REBOOT);
        }

IDIExit:
   ;

    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
          NewDevWiz.LastError = RtlNtStatusToDosError(GetExceptionCode());
    }

    if (NewDevWiz.SearchThread) {
        
        DestroySearchThread(NewDevWiz.SearchThread);
    }

    if (NewDevWiz.hDeviceInfo && NewDevWiz.hDeviceInfo != INVALID_HANDLE_VALUE) {
        
        SetupDiDestroyDeviceInfoList(NewDevWiz.hDeviceInfo);
        NewDevWiz.hDeviceInfo = NULL;
    }

    SetLastError(NewDevWiz.LastError);

    return NewDevWiz.LastError == ERROR_SUCCESS;
}


BOOL
InstallDevInstEx(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot,
   BOOL SilentInstall
   )
/*++

Routine Description:

   Exported Entry point from newdev.dll. Installs an existing Device Instance,
   and is invoked by Device Mgr to update a driver, or by Config mgr when a new
   device was found. In both cases the Device Instance exists in the registry.


Arguments:

    hwndParent - Window handle of the top-level window to use for any UI related
                 to installing the device.

    DeviceInstanceId - Supplies the ID of the device instance.  This is the registry
                       path (relative to the Enum branch) of the device instance key.

    UpdateDriver      - TRUE only newer or higher rank drivers are installed.

    pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

    SilentInstall - TRUE means the "New Hardware Found" dialog will not be displayed


Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/
{
   return InstallDeviceInstance(hwndParent, DeviceInstanceId, UpdateDriver, pReboot, NULL, SilentInstall);
}

BOOL
InstallDevInst(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   BOOL UpdateDriver,
   PDWORD pReboot
   )
{
    return InstallDevInstEx(hwndParent,
                            DeviceInstanceId,
                            UpdateDriver,
                            pReboot,
                            FALSE);
}

BOOL
EnumAndUpgradeDevices(
    HWND hwndParent,
    LPCWSTR HardwareId,
    PUPDATEDRIVERINFO UpdateDriverInfo,
    PDWORD pReboot
    )
{
    HDEVINFO hDevInfo;
    SP_DEVINFO_DATA DeviceInfoData;
    DWORD Index;
    DWORD Size;
    LPWSTR HardwareIdList = NULL;
    LPWSTR SingleHardwareId;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];
    BOOL Match;
    BOOL Result = TRUE;
    BOOL NoSuchDevNode = TRUE;
    DWORD SingleNeedsReboot;
    DWORD TotalNeedsReboot = 0;
    DWORD LastError = ERROR_SUCCESS;

    if (pReboot) {
        *pReboot = 0;
    }

    hDevInfo = SetupDiGetClassDevs(NULL,
                                   NULL,
                                   hwndParent,
                                   DIGCF_ALLCLASSES | DIGCF_PRESENT
                                   );

    if (INVALID_HANDLE_VALUE == hDevInfo) {

        return FALSE;
    }

    ZeroMemory(&DeviceInfoData, sizeof(SP_DEVINFO_DATA));
    DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
    Index = 0;

    //
    // Enumerate through all of the devices until we hit an installation error
    // or we run out of devices
    //
    while (Result &&
           SetupDiEnumDeviceInfo(hDevInfo,
                                 Index++,
                                 &DeviceInfoData
                                 )) {
        Match = FALSE;
        Size = 0;

        //
        // Get the list of HardwareIds for this device
        //
        SetupDiGetDeviceRegistryProperty(hDevInfo,
                                         &DeviceInfoData,
                                         SPDRP_HARDWAREID,
                                         NULL,
                                         NULL,
                                         0,
                                         &Size
                                         );

        HardwareIdList = malloc(Size + 2);

        if (HardwareIdList) {

            if (SetupDiGetDeviceRegistryProperty(hDevInfo,
                                                 &DeviceInfoData,
                                                 SPDRP_HARDWAREID,
                                                 NULL,
                                                 (PBYTE)HardwareIdList,
                                                 Size,
                                                 &Size
                                                 )) {

                //
                // If any of the devices HardwareIds match the given ID then
                // we have a match and need to upgrade the drivers on this device.
                //
                for (SingleHardwareId = HardwareIdList;
                     *SingleHardwareId;
                     SingleHardwareId += lstrlen(SingleHardwareId) + 1) {

                    if (_wcsicmp(SingleHardwareId, HardwareId) == 0) {

                        Match = TRUE;
                        NoSuchDevNode = FALSE;

                        if (HardwareIdList) {

                            free(HardwareIdList);
                            HardwareIdList = NULL;
                        }

                        break;
                    }
                }
            }
        }


        if (HardwareIdList) {

            free(HardwareIdList);
            HardwareIdList = NULL;
        }

        //
        // If we have a match then install the drivers on this device instance
        //
        if (Match) {

            if (SetupDiGetDeviceInstanceId(hDevInfo,
                                           &DeviceInfoData,
                                           DeviceInstanceId,
                                           SIZECHARS(DeviceInstanceId),
                                           &Size
                                           )) {

                SingleNeedsReboot = 0;
                Result = InstallDeviceInstance(hwndParent, 
                                               DeviceInstanceId, 
                                               TRUE, 
                                               &SingleNeedsReboot, 
                                               UpdateDriverInfo, 
                                               FALSE
                                               );

                TotalNeedsReboot |= SingleNeedsReboot;

                //
                // We only want to backup the first device we install...not every one.
                //
                UpdateDriverInfo->Backup = FALSE;
            }
        }
    }

    //
    // We need to preserve the LastError information around the setupapi call 
    // SetupDiDesrtoyDeviceInfoList otherwise it will get set to ERROR_SUCCESS
    // and we will loose whatever error information we had before.
    //
    LastError = GetLastError();

    SetupDiDestroyDeviceInfoList(hDevInfo);

    SetLastError(LastError);

    //
    // If the caller wants to handle the reboot themselves then pass the information
    // back to them.
    //
    if (pReboot) {
        *pReboot = TotalNeedsReboot;
    }

    //
    // The caller did not specify a pointer to a Reboot DWORD so we will handle the
    // rebooting ourselves if necessary
    else {
        if (TotalNeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            RestartDialog(hwndParent, NULL, EWX_REBOOT);
        }
    }

    //
    // If NoSuchDevNode is TRUE then we were unable to match the specified Hardware ID against
    // any of the devices on the system.  In this case we will set the last error to
    // ERROR_NO_SUCH_DEVINST.
    //
    if (NoSuchDevNode) {
        SetLastError(ERROR_NO_SUCH_DEVINST);
    }

    return Result;
}

BOOL
InstallWindowsUpdateDriver(
    HWND hwndParent,
    LPCWSTR HardwareId,
    LPCWSTR InfPathName,
    LPCWSTR DisplayName,
    BOOL Force,
    BOOL Backup,
    PDWORD pReboot
    )
/*++

Routine Description:

   Exported Entry point from newdev.dll. It is invoked by Windows Update to update a driver.
   This function will scan through all of the devices on the machine and attempt to install
   these drivers on any devices that match the given HardwareId.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   HardwareId - Supplies the Hardware ID to match agaist existing devices on the
                system.                                                     
                                                     
   InfPathName - Inf Pathname and associated driver files.

   DisplayName - Friendly UI name which is stored in CDM's reinstall backup registry key
                 "DisplayName" Value.

   Force - if TRUE this API will only look for infs in the directory specified by InfLocation.

   Backup - if TRUE this API will backup the existing drivers before installing the drivers
            from Windows Update.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

Return Value:

   BOOL TRUE if a device was upgraded to a CDM driver.
        FALSE if no devices were upgraded to a CDM driver.  GetLastError()
            will be ERROR_SUCCESS if nothing went wrong, this driver
            simply wasn't for any devices on the machine or wasn't
            better than the current driver.  If GetLastError() returns
            any other error then there was an error during the installation
            of this driver.                                        
                                     
--*/
{
    UPDATEDRIVERINFO UpdateDriverInfo;

    UpdateDriverInfo.InfPathName = InfPathName;
    UpdateDriverInfo.DisplayName = DisplayName;
    UpdateDriverInfo.Force = Force;
    UpdateDriverInfo.Backup = Backup;
    UpdateDriverInfo.FromInternet = TRUE;

    //
    // Assume that the upgrade will fail
    //
    UpdateDriverInfo.DriverWasUpgraded = FALSE;

    //
    // Call EnumAndUpgradeDevices which will enumerate through all the devices on the machine
    // and for any that match the given hardware ID it will attempt to upgrade to the specified
    // drivers.
    //
    EnumAndUpgradeDevices(hwndParent, HardwareId, &UpdateDriverInfo, pReboot);

    return UpdateDriverInfo.DriverWasUpgraded;
}

BOOL
WINAPI
UpdateDriverForPlugAndPlayDevicesW(
    HWND hwndParent,
    LPCWSTR HardwareId,
    LPCWSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired OPTIONAL
    )
/*++

Routine Description:

   This function will scan through all of the devices on the machine and attempt to install
   the drivers in FullInfPath on any devices that match the given HardwareId. The default 
   behavior is to only install the specified drivers if the are better then the currently
   installed driver.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   HardwareId - Supplies the Hardware ID to match agaist existing devices on the
                system.                                                     
                                                     
   FullInfPath - Full path to an Inf and associated driver files.
   
   InstallFlags - INSTALLFLAG_FORCE - If this flag is specified then newdev will not compare the
                    specified INF file with the current driver.  The specified INF file and drivers
                    will always be installed unless an error occurs.

   pReboot - Optional address of BOOL to determine if a reboot is required or not.
             If pReboot is NULL then newdev.dll will prompt for a reboot if one is needed. If
             pReboot is a valid BOOL pointer then the reboot status is passed back to the
             caller and it is the callers responsibility to prompt for a reboot if one is
             needed.

Return Value:

   BOOL TRUE if a device was upgraded to the specified driver.
        FALSE if no devices were upgraded to the specified driver.  GetLastError()
            will be ERROR_SUCCESS if nothing went wrong, this driver
            wasn't better than the current driver.  If GetLastError() returns
            any other error then there was an error during the installation
            of this driver.                                        

--*/
{
    UPDATEDRIVERINFO UpdateDriverInfo;
    DWORD NeedsReboot = 0;

    //
    // First verify that the process has sufficient SE_LOAD_DRIVER_NAME privileges.
    //
    if (!DoesUserHavePrivilege((PCTSTR)SE_LOAD_DRIVER_NAME)) {

        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Verify the parameters
    //
    if ((!HardwareId || (HardwareId[0] == TEXT('\0'))) ||
        (!FullInfPath || (FullInfPath[0] == TEXT('\0')))) {

        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (InstallFlags &~ INSTALLFLAG_BITS) {
        SetLastError(ERROR_INVALID_FLAGS);
        return FALSE;
    }

    if (!FileExists(FullInfPath, NULL)) {
        
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
    
    UpdateDriverInfo.InfPathName = FullInfPath;
    UpdateDriverInfo.Force = (InstallFlags & INSTALLFLAG_FORCE);
    UpdateDriverInfo.Backup = FALSE;
    UpdateDriverInfo.FromInternet = FALSE;

    
    //
    // Assume that the upgrade will fail
    //
    UpdateDriverInfo.DriverWasUpgraded = FALSE;

    //
    // Call EnumAndUpgradeDevices which will enumerate through all the devices on the machine
    // and for any that match the given hardware ID it will attempt to upgrade to the specified
    // drivers.
    //
    EnumAndUpgradeDevices(hwndParent, HardwareId, &UpdateDriverInfo, &NeedsReboot);

    if (bRebootRequired) {
        if (NeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            *bRebootRequired = TRUE;
        } else {
            *bRebootRequired = FALSE;
        }
    } else {
        if (NeedsReboot & (DI_NEEDRESTART | DI_NEEDREBOOT)) {
            RestartDialog(hwndParent, NULL, EWX_REBOOT);
        }
    }

    return UpdateDriverInfo.DriverWasUpgraded;
}

BOOL
WINAPI
UpdateDriverForPlugAndPlayDevicesA(
    HWND hwndParent,
    LPCSTR HardwareId,
    LPCSTR FullInfPath,
    DWORD InstallFlags,
    PBOOL bRebootRequired OPTIONAL
    )
{
    WCHAR   UnicodeHardwareId[MAX_DEVICE_ID_LEN];
    WCHAR   UnicodeFullInfPath[MAX_PATH];
    
    //
    // Convert the HardwareId and FullInfPath to UNICODE and call
    // InstallDriverForPlugAndPlayDevicesW
    //
    UnicodeHardwareId[0] = TEXT('\0');
    UnicodeFullInfPath[0] = TEXT('\0');
    MultiByteToWideChar(CP_ACP, 0, HardwareId, -1, UnicodeHardwareId, SIZECHARS(UnicodeHardwareId));
    MultiByteToWideChar(CP_ACP, 0, FullInfPath, -1, UnicodeFullInfPath, SIZECHARS(UnicodeFullInfPath));

    return UpdateDriverForPlugAndPlayDevicesW(hwndParent,
                                              UnicodeHardwareId,
                                              UnicodeFullInfPath,
                                              InstallFlags,
                                              bRebootRequired
                                              );
}

BOOL
InstallCDMDriver(
   HWND hwndParent,
   LPCWSTR DeviceInstanceId,
   LPCWSTR InfPathName,
   LPCWSTR DisplayName,
   BOOL Force,
   PDWORD pReboot
   )
/*++

Routine Description:

   Exported Entry point from newdev.dll. Installs an existing Device Instance
   following CDM semantics. It is invoked by Code Donload Manager to update a driver.


Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   DeviceInstanceId - Supplies the ID of the device instance.  This is the registry
                      path (relative to the Enum branch) of the device instance key.

   InfPathName - Inf Pathname and associated driver files.

   DisplayName - Friendly UI name which is stored in CDM's reinstall backup registry key
                 "DisplayName" Value.

   Force - if TRUE this API will only look for infs in the directory specified by InfLocation.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)

Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/
{
    UPDATEDRIVERINFO UpdateDriverInfo;

    UpdateDriverInfo.InfPathName = InfPathName;
    UpdateDriverInfo.DisplayName = DisplayName;
    UpdateDriverInfo.Force = Force == TRUE;
    UpdateDriverInfo.FromInternet = TRUE;

    //
    // Always perform a backup using this broken API
    //
    UpdateDriverInfo.Backup = TRUE;

    return InstallDeviceInstance(hwndParent, DeviceInstanceId, TRUE, pReboot, &UpdateDriverInfo, FALSE);
}




DWORD
WINAPI 
DevInstallW(
    HWND hwnd, 
    HINSTANCE hInst, 
    LPWSTR szCmd, 
    int nShow)
{
    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);

    //
    // This is a rundll32 entrypt that just calls directly into the
    // InstallDevInst exported entrypt.
    //
    if (!InstallDevInst(NULL, szCmd, FALSE, NULL)) {
    
        return GetLastError();
    }

    return 0;
}

DWORD
DevInstallUiOnlyThread(
    HANDLE hPipeRead        
    )
{
    DWORD DeviceInstanceIdLength, BytesRead;
    TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];

    //
    // Continue reading DeviceInstanceIdLength's from the pipe until
    // the other end is closed.
    //
    while(ReadFile(hPipeRead,
                   (LPVOID)&DeviceInstanceIdLength,
                   sizeof(DWORD),
                   &BytesRead,
                   NULL)) {

        //
        // Read the DeviceInstanceId from the pipe if the DeviceInstanceIdLength
        // is not 0.  A 0 length DeviceInstanceIdLenght means that we should hide
        // the UI.
        //
        if (DeviceInstanceIdLength) {

            if (!ReadFile(hPipeRead,
                          (LPVOID)DeviceInstanceId,
                          DeviceInstanceIdLength,
                          &BytesRead,
                          NULL)) {

                //
                // If this read fails then just close the UI and close the process.
                //
                goto clean0;
            }

            //
            // Post a message to the UI so that it can update the string and class
            // icon.
            //
            PostMessage(hDlgUI, WUM_UPDATEUI, 0, (LPARAM)DeviceInstanceId);
            ShowWindow(hDlgUI, SW_SHOW);
        
        } else {

            //
            // If the DeviceInstanceIdLength is 0 then this means we should hide the UI.
            // Send in a NULL DeviceInstanceId to the Ui to do this.
            //
            ShowWindow(hDlgUI, SW_HIDE);
        }

    }

clean0:

    //
    // Tell the UI to go away because we are done
    //
    PostMessage(hDlgUI, WUM_EXIT, 0, 0);

    return 0;
}

DWORD
WINAPI 
DevInstallUiOnlyW(
    HWND hwnd, 
    HINSTANCE hInst, 
    LPWSTR szCmd, 
    int nShow)
{
    HANDLE hThread;
    DWORD ThreadId;
    HANDLE hPipeRead;
    HICON hIcon;
    MSG Msg;

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(nShow);

    if(!szCmd || !*szCmd) {
        goto clean0;
    }

    //
    // Get the read handle fro the anonymous named pipe
    //
    hPipeRead = (HANDLE)StrToInt(szCmd);

    hDlgUI = CreateDialog(hNewDev,
                          MAKEINTRESOURCE(DLG_DEVINSTALL),
                          hwnd,
                          DevInstallUiOnlyDlgProc
                          );

    if (!hDlgUI) {

        goto clean0;
    }

    hThread = CreateThread(NULL,
                           0,
                           DevInstallUiOnlyThread,
                           (PVOID)hPipeRead,
                           0,
                           &ThreadId
                           );

    if (!hThread) {
        
        DestroyWindow(hDlgUI);
        goto clean0;
    }

    hIcon = NULL;

    hIcon = LoadIcon(hNewDev,MAKEINTRESOURCE(IDI_NEWDEVICEICON));

    if (hIcon) {

        SendMessage(hDlgUI, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
        SendMessage(hDlgUI, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    }

    ShowWindow(hDlgUI, SW_SHOW);

    while (IsWindow(hDlgUI)) {

        if (GetMessage(&Msg, NULL, 0, 0) && !IsDialogMessage(hDlgUI, &Msg)) {

            TranslateMessage(&Msg);
            DispatchMessage(&Msg);
        }
    }


clean0:
    return 0;
}


DWORD
WINAPI 
DevInstallRebootPromptW(
    HWND hwnd, 
    HINSTANCE hInst, 
    LPWSTR szCmd, 
    int nShow)
{
    TCHAR RebootText[MAX_PATH];

    UNREFERENCED_PARAMETER(hInst);
    UNREFERENCED_PARAMETER(szCmd);
    UNREFERENCED_PARAMETER(nShow);

    LoadString(hNewDev, IDS_NEWDEVICE_REBOOT, RebootText, SIZECHARS(RebootText));

    RestartDialog(hwnd, RebootText, EWX_REBOOT);
    return 0;
}
