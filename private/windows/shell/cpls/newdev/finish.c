//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       finish.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include "regstr.h"

#include <help.h>


typedef
UINT
(*PDEVICEPROBLEMTEXT)(
    HMACHINE hMachine,
    DEVNODE DevNode,
    ULONG ProblemNumber,
    LPTSTR Buffer,
    UINT   BufferSize
    );

BOOL
IsNullDriverInstalled(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine determines whether a null driver, or no driver at all, is
    installed for this device instance.  Currently the test is that I know
    a null driver was installed if the "Driver" value entry doesn't exist.

Arguments:

   hDevInfo         - Device Information handle

   DeviceInfoData   - Device Information Data structure


Return Value:

   Returns TRUE if a null driver was installed for this device, otherwise
   returns FALSE.

--*/

{
    TCHAR Buffer[256];
    PVOID pvBuffer=Buffer;
    
    if (SetupDiGetDeviceRegistryProperty(hDeviceInfo,
                                         DeviceInfoData,
                                         SPDRP_DRIVER,
                                         NULL,
                                         pvBuffer,
                                         sizeof(Buffer),
                                         NULL
                                         )) {

        return FALSE;

    } else {

        return TRUE;

    }
}

PTCHAR
DeviceProblemText(
   HMACHINE hMachine,
   DEVNODE DevNode,
   ULONG ProblemNumber
   )
{
   UINT LenChars, ReqLenChars;
   HMODULE hDevMgr=NULL;
   PTCHAR Buffer=NULL;
   PDEVICEPROBLEMTEXT pDeviceProblemText = NULL;

   hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
   if (hDevMgr) 
   {
       pDeviceProblemText = (PVOID) GetProcAddress(hDevMgr, "DeviceProblemTextW");
   }

   if (pDeviceProblemText) 
   {
       LenChars = (pDeviceProblemText)(hMachine,
                                       DevNode,
                                       ProblemNumber,
                                       Buffer,
                                       0
                                       );
       if (!LenChars) 
       {
           goto DPTExitCleanup;
       }

       LenChars++;  // one extra for terminating NULL

       Buffer = LocalAlloc(LPTR, LenChars*sizeof(TCHAR));
       if (!Buffer) 
       {
           goto DPTExitCleanup;
       }

       ReqLenChars = (pDeviceProblemText)(hMachine,
                                          DevNode,
                                          ProblemNumber,
                                          Buffer,
                                          LenChars
                                          );
       if (!ReqLenChars || ReqLenChars >= LenChars) 
       {
           LocalFree(Buffer);
           Buffer = NULL;
       }
   }

DPTExitCleanup:

   if (hDevMgr) 
   {
       FreeLibrary(hDevMgr);
   }

   return Buffer;
}

BOOL
DeviceHasResources(
   DEVINST DeviceInst
   )
{
   CONFIGRET ConfigRet;
   ULONG lcType = NUM_LOG_CONF;

   while (lcType--) 
   {
       ConfigRet = CM_Get_First_Log_Conf_Ex(NULL, DeviceInst, lcType, NULL);
       if (ConfigRet == CR_SUCCESS) 
       {
           return TRUE;
       }
   }

   return FALSE;
}

LONG
CreateCDMBackupKey(
    PNEWDEVWIZ NewDevWiz,
    LPCWSTR DisplayName,
    LPCWSTR BackupPath
    )
/*++

Routine Description:
   Creates the CDM reinstall backup registry key upon successfull copying of the
   files during the install. An install may have failed, but the files copied
   in which case the backup registry key is created. The following is created:

   HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\Reinstall
      USB_#VID_045E&PID_0009    ; PNP hardwareId.
          DisplayName     "USB Mouse Test"         ; friendly UI name
          HardwareID      "USB_#VID_045E&PID_0009" ; PNP hardwareId
          ReinstallString "C:\WINDOWS\SYSTEM\REINSTALLBACKUPS\USB#VID_045E&PID_0009"

Arguments:


Return Value:
   winerror code (ERROR_SUCCESS for success).


--*/
{
    LONG Error, SavedError;
    PTCHAR ptchSrc, ptchDst;
    HKEY hKeyReinstall, hKeyDeviceInstance;
    DWORD RegCreated;
    CONFIGRET ConfigRet;
    ULONG Len;
    DWORD Size;
    SP_DRVINFO_DATA DriverInfoData;
    PSP_DRVINFO_DETAIL_DATA pDriverInfoDetailData = NULL;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    TCHAR StringId[MAX_PATH];

    try {
    
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (!SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            SavedError = GetLastError();
            goto clean0;
        }
    
        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DriverInfoData
                                      )) {

            SavedError = GetLastError();
            goto clean0;
        }
    
        //
        // We need to get the SP_DRVINFO_DETAIL_DATA so we can get the hardware ID for
        // this device.
        //
        Size = 0;
        SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   &DriverInfoData,
                                   NULL,
                                   0,
                                   &Size
                                   );
    
        if (GetLastError() != ERROR_INSUFFICIENT_BUFFER ||
            Size == 0) {
    
            SavedError = GetLastError();
            goto clean0;
        }
    
        pDriverInfoDetailData = (PSP_DRVINFO_DETAIL_DATA)malloc(Size);                               
    
        if (!pDriverInfoDetailData) {
    
            SavedError = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }
    
        pDriverInfoDetailData->cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                        &NewDevWiz->DeviceInfoData,
                                        &DriverInfoData,
                                        pDriverInfoDetailData,
                                        Size,
                                        &Size
                                        )) {
    
            SavedError = GetLastError();
            goto clean0;
        }
    
        Error = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                               TEXT("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Reinstall"),
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hKeyReinstall,
                               &RegCreated
                               );
    
        if (Error != ERROR_SUCCESS) 
        {
            SavedError = Error;
            goto clean0;
        }
    
    
    
        //
        // Substitute the backslashes with '#' so we get a single key
        // (not a bunch of subkeys).
        //
    
        ptchDst = StringId;
        ptchSrc = (PTCHAR)pDriverInfoDetailData->HardwareID;
        while (*ptchSrc) 
        {
            if (*ptchSrc == TEXT('\\')) 
            {
                *ptchDst++ = TEXT('#');
                ptchSrc++;
            }
            else 
            {
                *ptchDst++ = *ptchSrc++;
            }
        }

        *ptchDst = TEXT('\0');
    
        Error = RegCreateKeyEx(hKeyReinstall,
                               StringId,
                               0,
                               NULL,
                               REG_OPTION_NON_VOLATILE,
                               KEY_WRITE,
                               NULL,
                               &hKeyDeviceInstance,
                               &RegCreated
                               );
    
        RegCloseKey(hKeyReinstall);
        if (Error != ERROR_SUCCESS) 
        {
            SavedError = Error;
            goto clean0;
        }
    
        //
        // Add the display name to the registry
        //
        SavedError = ERROR_SUCCESS;
        Error = RegSetValueEx(hKeyDeviceInstance,
                              TEXT("DisplayName"),
                              0,
                              REG_SZ,
                              (PVOID)DisplayName,
                              lstrlen(DisplayName)*sizeof(TCHAR)+sizeof(TCHAR)
                              );
    
        if (SavedError == ERROR_SUCCESS) 
        {
            SavedError = Error;
        }
    
        //
        // Add the Hardware ID to the registry
        //
        Error = RegSetValueEx(hKeyDeviceInstance,
                              TEXT("StringId"),
                              0,
                              REG_SZ,
                              (PVOID)pDriverInfoDetailData->HardwareID,
                              lstrlen(pDriverInfoDetailData->HardwareID)*sizeof(TCHAR)+sizeof(TCHAR)
                              );
    
        if (SavedError == ERROR_SUCCESS) 
        {
            SavedError = Error;
        }
    
    
        Error = RegSetValueEx(hKeyDeviceInstance,
                              TEXT("ReinstallString"),
                              0,
                              REG_SZ,
                              (PVOID)BackupPath,
                              lstrlen(BackupPath)*sizeof(TCHAR)+sizeof(TCHAR)
                              );
    
        if (SavedError == ERROR_SUCCESS) 
        {
            SavedError = Error;
        }
    
        RegCloseKey(hKeyDeviceInstance);

    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
        
        Error = RtlNtStatusToDosError(GetExceptionCode());
    }

clean0:

    if (pDriverInfoDetailData) {

        free(pDriverInfoDetailData);
    }

    return SavedError;
}

BOOL
GetClassGuidForInf(
    PTSTR InfFileName,
    LPGUID ClassGuid
    )
{
    TCHAR ClassName[MAX_CLASS_NAME_LEN];
    DWORD NumGuids;

    if(!SetupDiGetINFClass(InfFileName,
                           ClassGuid,
                           ClassName,
                           sizeof(ClassName)/sizeof(TCHAR),
                           NULL))
    {
       return FALSE;
    }

    if (IsEqualGUID(ClassGuid, &GUID_NULL)) 
    {
        //
        // Then we need to retrieve the GUID associated with the INF's class name.
        // (If this class name isn't installed (i.e., has no corresponding GUID),
        // or if it matches with multiple GUIDs, then we abort.
        //
        if(!SetupDiClassGuidsFromName(ClassName, ClassGuid, 1, &NumGuids) || !NumGuids) 
        {
            return FALSE;
        }
    }

    return TRUE;
}

BOOL
IsInternetDriver(
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData
    )
{
    BOOL InternetDriver = FALSE;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(SetupDiGetSelectedDriver(hDeviceInfo, DeviceInfoData, &DriverInfoData)) {

        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (SetupDiGetDriverInstallParams(hDeviceInfo,
                                           DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInstallParams
                                           ) 
            &&
            (DriverInstallParams.Flags & DNF_INET_DRIVER))
        {
            InternetDriver = TRUE;
        }
    }

    return InternetDriver;
}

LONG
ClassInstallerInstalls(
    HWND hwndParent,
    PNEWDEVWIZ NewDevWiz,
    HDEVINFO hDeviceInfo,
    PSP_DEVINFO_DATA DeviceInfoData,
    BOOL InstallFilesOnly,
    BOOL BackupOldDrivers
    )
{
    LONG Error;
    HSPFILEQ FileQueue;
    PVOID MessageHandlerContext = NULL;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    SP_DRVINFO_DATA       DriverInfoData;
    SP_DRVINSTALL_PARAMS  DriverInstallParams;
    SP_BACKUP_QUEUE_PARAMS BackupQueueParams;
    TCHAR DisplayName[MAX_PATH];
    TCHAR BackupPath[MAX_PATH];

    //
    // verify with class installer, and class-specific coinstallers
    // that the driver is not blacklisted
    //
    if (!SetupDiCallClassInstaller(DIF_ALLOW_INSTALL,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   )) 
    {
        if (GetLastError() != ERROR_DI_DO_DEFAULT)  
        {
            return GetLastError();
        }
    }

    if (BackupOldDrivers) {

        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (!SetupDiGetDeviceInstallParams(hDeviceInfo,
                                           DeviceInfoData,
                                           &DeviceInstallParams
                                           ))
        {
            return GetLastError();
        }

        FileQueue = DeviceInstallParams.FileQueue;

        //
        // Get the display name for this device that will be stored in the registry as the backup
        // description.  If one was passed in from CDM.DLL then use that, otherwize just use the
        // devices name.
        //
        if (NewDevWiz->UpdateDriverInfo) {

            wcscpy(DisplayName, NewDevWiz->UpdateDriverInfo->DisplayName);

        } else {

            PTCHAR FriendlyName = NULL;
            
            FriendlyName = BuildFriendlyName(DeviceInfoData->DevInst, FALSE, NULL);

            if (FriendlyName) {
            
                wcscpy(DisplayName, FriendlyName);
                LocalFree(FriendlyName);
            
            
            } else {

                wcscpy(DisplayName, szUnknown);
            }
        }
    }

    //
    // If we're installing from a legacy INF, then we have to do it all in one
    // shot via DIF_INSTALLDEVICE.  Otherwise, we split the file copy
    // operations out into a pre-installation step, so that all files are
    // available for subsequent operations.
    //
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if(SetupDiGetSelectedDriver(hDeviceInfo, DeviceInfoData, &DriverInfoData)) {

        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (!SetupDiGetDriverInstallParams(hDeviceInfo,
                                           DeviceInfoData,
                                           &DriverInfoData,
                                           &DriverInstallParams
                                           ))
        {
            return GetLastError();
        }

        if (DriverInstallParams.Flags & DNF_LEGACYINF) {

            if (SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                          hDeviceInfo,
                                          DeviceInfoData
                                          ))
            {
                if (BackupOldDrivers) 
                {
                    Error = ERROR_SUCCESS;
                    
                    MessageHandlerContext = SetupInitDefaultQueueCallbackEx(
                                                hwndParent,
                                                (DeviceInstallParams.Flags & DI_QUIETINSTALL)
                                                    ? INVALID_HANDLE_VALUE : NULL,
                                                0,
                                                0,
                                                NULL
                                                );

                    if (MessageHandlerContext) 
                    {
                        BackupQueueParams.cbSize = sizeof(SP_BACKUP_QUEUE_PARAMS);
                        if (SetupGetBackupInformation(FileQueue, &BackupQueueParams)) 
                        {
                            lstrcpy(BackupPath, BackupQueueParams.FullInfPath);

                        } else {

                            BackupPath[0] = TEXT('\0');
                        }

                        if (!SetupCommitFileQueue(hwndParent,
                                                  FileQueue,
                                                  SetupDefaultQueueCallback,
                                                  MessageHandlerContext
                                                  )) {

                            Error = GetLastError();
                        }
                    
                        if ((Error == ERROR_SUCCESS) &&
                            (BackupPath[0] != TEXT('\0'))) {
                    
                            CreateCDMBackupKey(NewDevWiz, DisplayName, BackupPath);
                        }

                        SetupTermDefaultQueueCallback(MessageHandlerContext);
                    }
                }

                return Error;
            }

            else 
            {
                return GetLastError();
            }
        }
    }

    //
    // Install the files first in one shot.
    // This allows new coinstallers to run during the install.
    //

    if (!SetupDiCallClassInstaller(DIF_INSTALLDEVICEFILES,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   ))
    {
        return GetLastError();
    }

    if (BackupOldDrivers) 
    {
        Error = ERROR_SUCCESS;

        BackupQueueParams.cbSize = sizeof(SP_BACKUP_QUEUE_PARAMS);
        if (SetupGetBackupInformation(FileQueue, &BackupQueueParams)) 
        {
            lstrcpy(BackupPath, BackupQueueParams.FullInfPath);

        } else {

            BackupPath[0] = TEXT('\0');
        }
        
        MessageHandlerContext = SetupInitDefaultQueueCallbackEx(
                                    hwndParent,
                                    (DeviceInstallParams.Flags & DI_QUIETINSTALL)
                                        ? INVALID_HANDLE_VALUE : NULL,
                                    0,
                                    0,
                                    NULL
                                    );

        if (MessageHandlerContext) 
        {
            if (!SetupCommitFileQueue(hwndParent,
                                      FileQueue,
                                      SetupDefaultQueueCallback,
                                      MessageHandlerContext
                                      )) {

                Error = GetLastError();
            }

            if ((Error == ERROR_SUCCESS) &&
                (BackupPath[0] != TEXT('\0'))) {

                CreateCDMBackupKey(NewDevWiz, DisplayName, BackupPath);
            }

            SetupTermDefaultQueueCallback(MessageHandlerContext);
        }

        if (Error != ERROR_SUCCESS) 
        {
            return Error;
        }
    }

    if (InstallFilesOnly) 
    {
        return ERROR_SUCCESS;
    }

    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(hDeviceInfo,
                                      DeviceInfoData,
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.Flags |= DI_NOFILECOPY;
        SetupDiSetDeviceInstallParams(hDeviceInfo,
                                      DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }


    //
    // Register any device-specific co-installers for this device,
    //

    if (!SetupDiCallClassInstaller(DIF_REGISTER_COINSTALLERS,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   ))
    {
        return GetLastError();
    }



    //
    // install any INF/class installer-specified interfaces.
    // and then finally the real "InstallDevice"!
    //

    if (!SetupDiCallClassInstaller(DIF_INSTALLINTERFACES,
                                  hDeviceInfo,
                                  DeviceInfoData
                                  )
        ||
        !SetupDiCallClassInstaller(DIF_INSTALLDEVICE,
                                   hDeviceInfo,
                                   DeviceInfoData
                                   ))
    {
        return GetLastError();
    }

    return ERROR_SUCCESS;
}

//
// invokable only from finish page!
//
DWORD
InstallDev(
    HWND       hwndParent,
    PNEWDEVWIZ NewDevWiz
    )
{
    HSPFILEQ FileQueue = NULL;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;
    TCHAR ClassGuidString[MAX_GUID_STRING_LEN];
    GUID ClassGuidInf;
    LPGUID ClassGuid;
    int   ClassGuidNum;
    DWORD Error = ERROR_SUCCESS;
    BOOL IgnoreRebootFlags = FALSE;
    TCHAR Buffer[MAX_PATH*2];
    PVOID pvBuffer = Buffer;
    ULONG DevNodeStatus = 0, Problem = 0;
    DWORD ClassGuidListSize, i;
    BOOL Backup = FALSE;


    if (!NewDevWiz->ClassGuidSelected) 
    {
        NewDevWiz->ClassGuidSelected = (LPGUID)&GUID_NULL;
    }


    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 ))
    {
        //
        // Get details on this driver node, so that we can examine the INF that this
        // node came from.
        //
        DriverInfoDetailData.cbSize = sizeof(SP_DRVINFO_DETAIL_DATA);
        if(!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                       &NewDevWiz->DeviceInfoData,
                                       &DriverInfoData,
                                       &DriverInfoDetailData,
                                       sizeof(DriverInfoDetailData),
                                       NULL
                                       ))
        {
            Error = GetLastError();
            if (Error != ERROR_INSUFFICIENT_BUFFER) 
            {
                goto clean0;
            }
        }


        //
        // Verif that the class is installed, if its not then
        // attempt to install it.
        //

        NdwBuildClassInfoList(NewDevWiz, 0);


        //
        // fetch classguid from inf, (It may be different than what we already
        // have in class guid selected).
        //
        if (!GetClassGuidForInf(DriverInfoDetailData.InfFileName, &ClassGuidInf)) 
        {
            ClassGuidInf = *NewDevWiz->ClassGuidSelected;
        }

        if (IsEqualGUID(&ClassGuidInf, &GUID_NULL)) 
        {
            ClassGuidInf = GUID_DEVCLASS_UNKNOWN;
        }


        //
        // if the ClassGuidInf wasn't found then this class hasn't been installed yet.
        // -install the class installer now.
        //

        ClassGuid = NewDevWiz->ClassGuidList;
        ClassGuidNum = NewDevWiz->ClassGuidNum;
        while (ClassGuidNum--) 
        {
            if (IsEqualGUID(ClassGuid, &ClassGuidInf)) 
            {
                break;
            }

            ClassGuid++;
        }

        if (ClassGuidNum < 0 &&
            !SetupDiInstallClass(hwndParent,
                                 DriverInfoDetailData.InfFileName,
                                 NewDevWiz->SilentMode ? DI_QUIETINSTALL : 0,
                                 NULL
                                 ))
        {
            Error = GetLastError();
            goto clean0;
        }


        //
        // Now make sure that the class of this device is the same as the class
        // of the selected driver node.
        //
        if (!IsEqualGUID(&ClassGuidInf, NewDevWiz->ClassGuidSelected)) 
        {
            pSetupStringFromGuid(&ClassGuidInf,
                                 ClassGuidString,
                                 sizeof(ClassGuidString)/sizeof(TCHAR)
                                 );

            SetupDiSetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                             &NewDevWiz->DeviceInfoData,
                                             SPDRP_CLASSGUID,
                                             (PBYTE)ClassGuidString,
                                             sizeof(ClassGuidString)
                                             );
        }
    }


    //
    // No selected driver, and no associated class--use "Unknown" class.
    //
    else 
    {

        //
        // If the devnode is currently running 'raw', then remember this
        // fact so that we don't require a reboot later (NULL driver installation
        // isn't going to change anything).
        //
        if (CM_Get_DevNode_Status(&DevNodeStatus,
                                  &Problem,
                                  NewDevWiz->DeviceInfoData.DevInst,
                                  0) == CR_SUCCESS)
        {
            if (!SetupDiGetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  SPDRP_SERVICE,
                                                  NULL,     // regdatatype
                                                  pvBuffer,
                                                  sizeof(Buffer),
                                                  NULL
                                                  ))
            {
                *Buffer = TEXT('\0');
            }

            if((DevNodeStatus & DN_STARTED) && (*Buffer == TEXT('\0')))
            {
                IgnoreRebootFlags = TRUE;
            }
        }

        if (IsEqualGUID(NewDevWiz->ClassGuidSelected, &GUID_NULL)) 
        {

            pSetupStringFromGuid(&GUID_DEVCLASS_UNKNOWN,
                                 ClassGuidString,
                                 sizeof(ClassGuidString)/sizeof(TCHAR)
                                 );


            SetupDiSetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                             &NewDevWiz->DeviceInfoData,
                                             SPDRP_CLASSGUID,
                                             (PBYTE)ClassGuidString,
                                             sizeof(ClassGuidString)
                                             );
        }

        ClassGuidInf = *NewDevWiz->ClassGuidSelected;
    }

    //
    // See if we need to backup the current drivers.
    //
    // Once again we need to special case printers because not only
    // can printer drivers not be backed-up, they also don't install
    // correctly if DI_NOVCP is specified.  One can only hope that
    // they will behave like all other devices one of these days!
    //
    if (IsEqualGUID(&ClassGuidInf, &GUID_DEVCLASS_PRINTER)) {

        Backup = FALSE;
    
    } else {
    
        if (NewDevWiz->UpdateDriverInfo) {
    
            //
            // Windows Update case
            //
            Backup = NewDevWiz->UpdateDriverInfo->Backup;
        
        } else {
    
            //
            // CDM case
            //
            Backup = IsInternetDriver(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData);
        }
    }

    if (Backup) {

        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (!SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                           &NewDevWiz->DeviceInfoData,
                                           &DeviceInstallParams
                                           ))
        {
            Error = GetLastError();
            goto clean0;
        }

        FileQueue = SetupOpenFileQueue();

        if (!FileQueue) {
        
           Error =ERROR_NOT_ENOUGH_MEMORY;
           goto clean0;
        }

        DeviceInstallParams.Flags |= DI_NOVCP;
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_PREINSTALLBACKUP;
        DeviceInstallParams.FileQueue = FileQueue;

        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DeviceInstallParams
                                      );
    }

    Error = ClassInstallerInstalls(hwndParent,
                                   NewDevWiz,
                                   NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   FALSE,
                                   Backup
                                   );

    //
    // If this is a WU/CDM install and it was successful then set
    // the DriverWasUpgraded to TRUE
    //
    if (NewDevWiz->UpdateDriverInfo && (Error == ERROR_SUCCESS)) {

        NewDevWiz->UpdateDriverInfo->DriverWasUpgraded = TRUE;
    }

    //
    // If this is a new device (currently no drivers are installed) and we encounter
    // an error that is not ERROR_CANCELLED then we will install the NULL driver for
    // this device and set the FAILED INSTALL flag.
    //
    if ((Error != ERROR_SUCCESS) &&
        (Error != ERROR_CANCELLED))
    {
        if (IsNullDriverInstalled(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData)) {
        
            if (SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                         &NewDevWiz->DeviceInfoData,
                                         NULL
                                         ))
            {
                DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

                if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DeviceInstallParams
                                                  ))
                {
                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
                    SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  &DeviceInstallParams
                                                  );
                }

                SetupDiInstallDevice(NewDevWiz->hDeviceInfo, &NewDevWiz->DeviceInfoData);
            }
        }

        goto clean0;
    }


    //
    // Fetch the latest DeviceInstallParams for the restart bits.
    //

    if(!IgnoreRebootFlags) 
    {
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
        {
            if (DeviceInstallParams.Flags & (DI_NEEDRESTART | DI_NEEDREBOOT)) 
            {
                NewDevWiz->Reboot |= DI_NEEDREBOOT;
            }
        }
    }


clean0:

    if (FileQueue) {

        SetupCloseFileQueue(FileQueue);
    }


    return Error;
}

DWORD
InstallNullDriver(
    HWND  hDlg,
    PNEWDEVWIZ NewDevWiz,
    BOOL FailedInstall
    )
{
    SP_DEVINSTALL_PARAMS    DevInstallParams;
    DWORD  Status = ERROR_SUCCESS;

    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (FailedInstall) 
    {
        if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DevInstallParams
                                          ))
        {
            DevInstallParams.FlagsEx |= DI_FLAGSEX_SETFAILEDINSTALL;
            SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DevInstallParams
                                          );
        }
    }


    if (SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 NULL
                                 ))
    {
        Status = InstallDev(hDlg, NewDevWiz);
    }


    return Status;

} // InstallNullDriver


BOOL
CALLBACK
AddPropSheetPageProc(
    IN HPROPSHEETPAGE hpage,
    IN LPARAM lParam
   )
{
    *((HPROPSHEETPAGE *)lParam) = hpage;
    return TRUE;
}

void
DisplayResource(
     PNEWDEVWIZ NewDevWiz,
     HWND hWndParent
     )
{
    HINSTANCE hLib;
    PROPSHEETHEADER psh;
    HPROPSHEETPAGE  hpsPages[1];
    SP_PROPSHEETPAGE_REQUEST PropPageRequest;
    LPFNADDPROPSHEETPAGES ExtensionPropSheetPage;
    LPTSTR Title;
    SP_DEVINSTALL_PARAMS    DevInstallParams;

    //
    // Now get the resource selection page from setupapi.dll
    //

    hLib = GetModuleHandle(TEXT("setupapi.dll"));
    if (hLib) 
    {
        ExtensionPropSheetPage = (PVOID)GetProcAddress(hLib, "ExtensionPropSheetPageProc");
    }

    if (!ExtensionPropSheetPage) 
    {
        return;
    }

    PropPageRequest.cbSize = sizeof(SP_PROPSHEETPAGE_REQUEST);
    PropPageRequest.PageRequested  = SPPSR_SELECT_DEVICE_RESOURCES;
    PropPageRequest.DeviceInfoSet  = NewDevWiz->hDeviceInfo;
    PropPageRequest.DeviceInfoData = &NewDevWiz->DeviceInfoData;

    if (!ExtensionPropSheetPage(&PropPageRequest,
                                AddPropSheetPageProc,
                                (LONG_PTR)hpsPages
                                ))
    {
        // warning ?
        return;
    }

    //
    // create the property sheet
    //

    psh.dwSize      = sizeof(PROPSHEETHEADER);
    psh.dwFlags     = PSH_PROPTITLE | PSH_NOAPPLYNOW;
    psh.hwndParent  = hWndParent;
    psh.hInstance   = hNewDev;
    psh.pszIcon     = NULL;

    switch (NewDevWiz->WizardType) {
        
        case NDWTYPE_FOUNDNEW:
            Title = (LPTSTR)IDS_FOUNDDEVICE;
            break;

        case NDWTYPE_UPDATE:
            Title = (LPTSTR)IDS_UPDATEDEVICE;
            break;

        default:
            Title = TEXT(""); // unknown
        }

    psh.pszCaption  = Title;

    psh.nPages      = 1;
    psh.phpage      = hpsPages;
    psh.nStartPage  = 0;
    psh.pfnCallback = NULL;


    //
    // Clear the Propchange pending bit in the DeviceInstall params.
    //

    DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DevInstallParams
                                      ))
    {
        DevInstallParams.FlagsEx &= ~DI_FLAGSEX_PROPCHANGE_PENDING;
        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DevInstallParams
                                      );
    }

    if (PropertySheet(&psh) == -1) 
    {
        DestroyPropertySheetPage(hpsPages[0]);
    }


    //
    // If a PropChange occurred invoke the DIF_PROPERTYCHANGE
    //

    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      &DevInstallParams
                                      ))
    {
        if (DevInstallParams.FlagsEx & DI_FLAGSEX_PROPCHANGE_PENDING) 
        {
            SP_PROPCHANGE_PARAMS PropChangeParams;

            PropChangeParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);
            PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;
            PropChangeParams.Scope = DICS_FLAG_GLOBAL;
            PropChangeParams.HwProfile = 0;

            if (SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                             &NewDevWiz->DeviceInfoData,
                                             (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                             sizeof(PropChangeParams)
                                             ))
            {
                SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                          NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData
                                          );
            }

            //
            // Clear the class install parameters.
            //

            SetupDiSetClassInstallParams(NewDevWiz->hDeviceInfo,
                                         &NewDevWiz->DeviceInfoData,
                                         NULL,
                                         0
                                         );
        }
    }


    return;
}

INT_PTR CALLBACK
NDW_InstallDevDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);
    LONG Error;
    ULONG DevNodeStatus, Problem;

    switch (wMsg) {
    
        case WM_INITDIALOG: {
        
            LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
            NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
            SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

            break;
        }

        case WM_DESTROY:
            break;

        case WUM_DOINSTALL:

            // do the Install
            NewDevWiz->LastError = InstallDev(hDlg, NewDevWiz);
            NewDevWiz->InstallPending = FALSE;
            NewDevWiz->CurrCursor = NULL;
            PropSheet_PressButton(hwndParentDlg, PSBTN_NEXT);
            break;


        case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {
        
            case PSN_SETACTIVE: {
            
                HICON hicon;
                SP_DRVINFO_DATA DriverInfoData;

                NewDevWiz->PrevPage = IDD_NEWDEVWIZ_INSTALLDEV;

                //
                // This is an intermediary status page, no buttons needed.
                // Set the device description
                // Set the class Icon
                //
                PropSheet_SetWizButtons(hwndParentDlg, 0);
                EnableWindow(GetDlgItem(GetParent(hDlg),  IDCANCEL), FALSE);

                SetDriverDescription(hDlg, IDC_NDW_DESCRIPTION, NewDevWiz);

                if (SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) 
                {
                    hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                    if (hicon) 
                    {
                        DestroyIcon(hicon);
                    }
                }

                NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
                SetCursor(NewDevWiz->CurrCursor);


                //
                // Post ourselves a msg, to do the actual install, this allows this
                // page to show itself while the install is actually occuring.
                //
                NewDevWiz->InstallPending = TRUE;

                if (NewDevWiz->SilentMode) 
                {
                    //
                    // do the Install immediately and move to the next page
                    // to prevent any UI from showing.
                    //

                    NewDevWiz->LastError =InstallDev(hDlg, NewDevWiz);
                    NewDevWiz->InstallPending = FALSE;
                    NewDevWiz->CurrCursor = NULL;

                   
                    //
                    // Add the FinishInstall Page and jump to it if the install was successful
                    //

                    if (NewDevWiz->LastError == ERROR_SUCCESS) {             
                    
                        NewDevWiz->WizExtFinishInstall.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL,
                                                                                     WizExtFinishInstallDlgProc,
                                                                                     NewDevWiz
                                                                                     );

                        if (NewDevWiz->WizExtFinishInstall.hPropSheet) 
                        {
                            PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtFinishInstall.hPropSheet);
                        }

                        SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_FINISHINSTALL);

                    } else {

                        //
                        // There was an error during the install so just jump to our finish page
                        //
                        SetDlgMsgResult(hDlg, wMsg, -1);
                    }
                }

                else 
                {
                    PostMessage(hDlg, WUM_DOINSTALL, 0, 0);
                }

                break;
            }


            case PSN_WIZNEXT:

                //
                // Add the FinishInstall Page and jump to it if the installation succeded.
                //

                if (NewDevWiz->LastError == ERROR_SUCCESS) {             
                
                    NewDevWiz->WizExtFinishInstall.hPropSheet = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL,
                                                                                 WizExtFinishInstallDlgProc,
                                                                                 NewDevWiz
                                                                                 );

                    if (NewDevWiz->WizExtFinishInstall.hPropSheet) 
                    {
                        PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtFinishInstall.hPropSheet);
                    }

                    SetDlgMsgResult(hDlg, wMsg, IDD_WIZARDEXT_FINISHINSTALL);

                } else {

                    //
                    // There was an error during the install so just jump to our finish page
                    //
                    SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_FINISH);
                }
                break;


        }
        break;


        case WM_SETCURSOR:
            if (NewDevWiz->CurrCursor) 
            {
                SetCursor(NewDevWiz->CurrCursor);
                break;
            }

            // fall thru to return(FALSE);

        default:
            return(FALSE);
        }

    return(TRUE);
}

void
ShowInstallSummary(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    LONG Error;
    CONFIGRET ConfigRet;
    ULONG Len, Problem, DevNodeStatus;
    BOOL HasResources;
    HWND hwndParentDlg = GetParent(hDlg);
    PTCHAR ErrorMsg, ProblemText;
    TCHAR TextBuffer[MAX_PATH*4];


    Problem = 0;
    *TextBuffer = TEXT('\0');

    Error = NewDevWiz->LastError;

    //
    // Installation was canceled
    //
    if (Error == ERROR_CANCELLED) 
    {
        if (NewDevWiz->SilentMode) 
        {
            HideWindowByMove(hwndParentDlg);
        }

        PropSheet_PressButton(hwndParentDlg, PSBTN_CANCEL);
        return;
    }

    //
    // On Windows Update installs we don't want to show any UI at all, even
    // if there was an error during the installation.  
    // We can tell a WU install from a CDM install because only a WU install
    // has a UpdateDriverInfo structure and is SilentMode
    //
    if (NewDevWiz->SilentMode && NewDevWiz->UpdateDriverInfo) 
    {
        HideWindowByMove(hwndParentDlg);
        PropSheet_PressButton(hwndParentDlg, PSBTN_FINISH);
        return;
    }


    if (NewDevWiz->hfontTextBigBold ) {
        
        SetWindowFont(GetDlgItem(hDlg, IDC_FINISH_MSG1), NewDevWiz->hfontTextBigBold, TRUE);
    }

    if (NDWTYPE_UPDATE == NewDevWiz->WizardType) {

        SetDlgText(hDlg, IDC_FINISH_MSG1, IDS_FINISH_MSG1_UPGRADE, IDS_FINISH_MSG1_UPGRADE);
    
    } else {

        SetDlgText(hDlg, IDC_FINISH_MSG1, IDS_FINISH_MSG1_NEW, IDS_FINISH_MSG1_NEW);
    }

    //
    // Installation failed
    //
    if (Error != ERROR_SUCCESS) 
    {
        NewDevWiz->Installed = FALSE;

        //
        // Display failure message for installation
        //
        LoadText(TextBuffer, sizeof(TextBuffer), IDS_NDW_ERRORFIN1_PNP, IDS_NDW_ERRORFIN1_PNP);

#if DBG
        DbgPrint("InstallDev Error =%x\n", Error);
#endif


        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                          NULL,
                          HRESULT_FROM_SETUPAPI(Error),
                          0,
                          (LPTSTR)&ErrorMsg,
                          0,
                          NULL
                          ))
        {
            lstrcat(TextBuffer, TEXT("\n\n"));
            lstrcat(TextBuffer, ErrorMsg);
            LocalFree(ErrorMsg);
        }

        SetDlgItemText(hDlg, IDC_NDW_TEXT, TextBuffer);

    }

    //
    // No errors installing the drivers for this device
    //
    else  
    {
        
        //
        // Check to see if the device itself has any problems
        //
        Error = CM_Get_DevNode_Status(&DevNodeStatus,
                                      &Problem,
                                      NewDevWiz->DeviceInfoData.DevInst,
                                      0
                                      );
        if(Error != CR_SUCCESS) 
        {

            //
            // For some reason, we couldn't retrieve the devnode's status.
            // Default status and problem values to zero.
            //
            DevNodeStatus = Problem = 0;

        }

        //
        // make sure the reboot flags\Problem are set correctly
        //
        if (NewDevWiz->Reboot || Problem == CM_PROB_NEED_RESTART) 
        {
            if (Problem != CM_PROB_PARTIAL_LOG_CONF) 
            {
                Problem = CM_PROB_NEED_RESTART;
            }
                
            NewDevWiz->Reboot |= DI_NEEDREBOOT;
        }


        NewDevWiz->Installed = TRUE;
        HasResources = DeviceHasResources(NewDevWiz->DeviceInfoData.DevInst);

        //
        // The device has a problem
        //
        if ((Error != CR_SUCCESS) || Problem) 
        {
            //
            // Show the resource button if the device has resources and it
            // has the problem CM_PROB_PARTIAL_LOG_CONF
            //
            if (HasResources && (Problem == CM_PROB_PARTIAL_LOG_CONF))
            {
                ShowWindow(GetDlgItem(hDlg, IDC_NDW_DISPLAYRESOURCE), SW_SHOW);
            }
        
            if (Problem == CM_PROB_NEED_RESTART) 
            {
                LoadText(TextBuffer, sizeof(TextBuffer), IDS_NDW_NORMALFINISH1, IDS_NDW_NORMALFINISH1);

                lstrcat(TextBuffer, TEXT("\n\n"));
                LoadText(TextBuffer, sizeof(TextBuffer), IDS_NEEDREBOOT, IDS_NEEDREBOOT);
            }
            
            else 
            {
                LoadText(TextBuffer, sizeof(TextBuffer), IDS_INSTALL_PROBLEM_PNP, IDS_INSTALL_PROBLEM_PNP);

                if (Problem) 
                {
                    ProblemText = DeviceProblemText(NULL,
                                                    NewDevWiz->DeviceInfoData.DevInst,
                                                    Problem
                                                    );

                    if (ProblemText) 
                    {
                        lstrcat(TextBuffer, TEXT("\n\n"));
                        lstrcat(TextBuffer, ProblemText);
                        LocalFree(ProblemText);
                    }
                }
            }

#if DBG
            DbgPrint("InstallDev CM_Get_DevNode_Status()=%x DevNodeStatus=%x Problem=%x\n",
                     Error,
                     DevNodeStatus,
                     Problem
                     );
#endif

        }

        //
        // Installation was sucessful and the device does not have any problems
        //
        else 
        {
            //
            // If this was a silent install (a Rank 0 match for example) then don't show the finish
            // page.
            //
            if (NewDevWiz->SilentMode) 
            {
                HideWindowByMove(hwndParentDlg);
                PropSheet_PressButton(hwndParentDlg, PSBTN_FINISH);
                return;
            }

            //
            // We just kept the current driver, we did not reinstall it.
            //
            if (NewDevWiz->DontReinstallCurrentDriver) {

                LoadText(TextBuffer, sizeof(TextBuffer), IDS_NDW_CURRENTFINISH, IDS_NDW_CURRENTFINISH);

            //
            // The installation (or reinstallation) was successful.
            //
            } else {

                LoadText(TextBuffer, sizeof(TextBuffer), IDS_NDW_NORMALFINISH1, IDS_NDW_NORMALFINISH1);
            }
        }

        SetDlgItemText(hDlg, IDC_NDW_TEXT, TextBuffer);
    }
}

INT_PTR CALLBACK
NDW_FinishDlgProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg) {
        case WM_INITDIALOG: 
        {
           LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
           NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
           SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

           break;
        }

       case WM_DESTROY:
           break;

       case WM_COMMAND:
           switch (wParam) 
           {
               case IDC_NDW_DISPLAYRESOURCE:
                   DisplayResource(NewDevWiz, GetParent(hDlg));
                   break;
           }

           break;


       case WM_NOTIFY:
       switch (((NMHDR FAR *)lParam)->code) {
           case PSN_SETACTIVE: {
               HICON hicon;
               SP_DRVINFO_DATA DriverInfoData;


               //
               // No back button since install is already done.
               // set the device description
               // Hide Resources button until we know if resources exist or not.
               // Set the class Icon
               //
               PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);

               EnableWindow(GetDlgItem(GetParent(hDlg),  IDCANCEL), FALSE);

               SetDriverDescription(hDlg, IDC_NDW_DESCRIPTION, NewDevWiz);

               ShowWindow(GetDlgItem(hDlg, IDC_NDW_CONFLICTHELP), SW_HIDE);
               ShowWindow(GetDlgItem(hDlg, IDC_NDW_DISPLAYRESOURCE), SW_HIDE);

               if (SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) 
               {
                   hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                   if (hicon) 
                   {
                       DestroyIcon(hicon);
                   }
               }

               ShowInstallSummary(hDlg, NewDevWiz);

               break;
               }


           case PSN_RESET:
               break;


            case PSN_WIZFINISH:
            {
                HKEY hKeyDeviceInstaller;
                DWORD RegCreated;
            
                //
                // Save away the search options in the registry
                //
                if (RegCreateKeyEx(HKEY_CURRENT_USER,
                                   REGSTR_PATH_DEVICEINSTALLER,
                                   0,
                                   NULL,
                                   REG_OPTION_NON_VOLATILE,
                                   KEY_WRITE,
                                   NULL,
                                   &hKeyDeviceInstaller,
                                   &RegCreated) == ERROR_SUCCESS) {

                    RegSetValueEx(hKeyDeviceInstaller,
                                  REGSTR_VAL_SEARCHOPTIONS,
                                  0,
                                  REG_DWORD,
                                  (PVOID)&(NewDevWiz->SearchOptions),
                                  sizeof(DWORD)
                                  );

                    RegCloseKey(hKeyDeviceInstaller);
                }                                   
            }
               break;

           }
        break;


        default:
            return(FALSE);
        }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtFinishInstallDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
       
    case WM_INITDIALOG: {
           
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
       
        case PSN_SETACTIVE:

            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_FINISHINSTALL;

            if (PrevPageId == IDD_NEWDEVWIZ_INSTALLDEV) 
            {
                //
                // Moving forward on first page
                //


                //
                // Add ClassWizard Extension pages for FinishInstall
                //

                AddClassWizExtPages(hwndParentDlg,
                                    NewDevWiz,
                                    &NewDevWiz->WizExtFinishInstall.DeviceWizardData,
                                    DIF_NEWDEVICEWIZARD_FINISHINSTALL
                                    );

                //
                // Add the end page, which is FinishInstall end
                //

                NewDevWiz->WizExtFinishInstall.hPropSheetEnd = CreateWizExtPage(IDD_WIZARDEXT_FINISHINSTALL_END,
                                                                                WizExtFinishInstallEndDlgProc,
                                                                                NewDevWiz
                                                                                );

                if (NewDevWiz->WizExtFinishInstall.hPropSheetEnd) 
                {
                    PropSheet_AddPage(hwndParentDlg, NewDevWiz->WizExtFinishInstall.hPropSheetEnd);
                }
            }


            //
            // We can't go backwards, so always go forward
            //

            SetDlgMsgResult(hDlg, wMsg, -1);
            break;

        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}

INT_PTR CALLBACK
WizExtFinishInstallEndDlgProc(
    HWND hDlg, 
    UINT wMsg, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    HWND hwndParentDlg = GetParent(hDlg);
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ )GetWindowLongPtr(hDlg, DWLP_USER);
    int PrevPageId;


    switch (wMsg) {
       
    case WM_INITDIALOG: {
           
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ )lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        break;
    }

    case WM_DESTROY:
        break;


    case WM_NOTIFY:
       
        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE:

            PrevPageId = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_WIZARDEXT_FINISHINSTALL_END;

           //
           // We can't go backwards, so always go forward
           //

           SetDlgMsgResult(hDlg, wMsg, IDD_NEWDEVWIZ_FINISH);
           break;

        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            SetDlgMsgResult(hDlg, wMsg, 0);
            break;
        }
        break;

    default:
        return(FALSE);
    }

    return(TRUE);
}
