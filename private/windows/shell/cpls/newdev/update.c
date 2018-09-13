//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       update.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include "regstr.h"

#define INSTALL_UI_TIMERID  1423

PROCESS_DEVICEMAP_INFORMATION ProcessDeviceMapInfo={0};

void
InstallSilentChilds(
   HWND hwdnParent,
   PNEWDEVWIZ NewDevWiz
   );



BOOL
RegistryDeviceName(
    DEVINST DevInst,
    PTCHAR  Buffer,
    DWORD   cbBuffer
    )
{
    ULONG ulSize;
    CONFIGRET ConfigRet;

    //
    // Try the registry for FRIENDLYNAME
    //

    ulSize = cbBuffer;
    ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                 CM_DRP_FRIENDLYNAME,
                                                 NULL,
                                                 Buffer,
                                                 &ulSize,
                                                 0);

    if (ConfigRet == CR_SUCCESS && *Buffer) {

        return TRUE;
    }


    //
    // Try the registry for DEVICEDESC
    //

    ulSize = cbBuffer;
    ConfigRet = CM_Get_DevNode_Registry_Property(DevInst,
                                                 CM_DRP_DEVICEDESC,
                                                 NULL,
                                                 Buffer,
                                                 &ulSize,
                                                 0);

    if (ConfigRet == CR_SUCCESS && *Buffer) {

        return TRUE;
    }

    return FALSE;
}




//
// returns TRUE if we were able to find a reasonable name
// (something besides unknown device).
//

void
SetDriverDescription(
    HWND hDlg,
    int iControl,
    PNEWDEVWIZ NewDevWiz
    )
{
    CONFIGRET ConfigRet;
    ULONG  ulSize;
    PTCHAR FriendlyName;
    PTCHAR Location;
    SP_DRVINFO_DATA DriverInfoData;


    //
    // If there is a selected driver use its driver description,
    // since this is what the user is going to install.
    //

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )
        &&
        *DriverInfoData.Description) {

        wcscpy(NewDevWiz->DriverDescription, DriverInfoData.Description);
        SetDlgItemText(hDlg, iControl, NewDevWiz->DriverDescription);
        return;
    }


    FriendlyName = BuildFriendlyName(NewDevWiz->DeviceInfoData.DevInst, FALSE, NULL);
    if (FriendlyName) {

        SetDlgItemText(hDlg, iControl, FriendlyName);
        LocalFree(FriendlyName);
        return;
    }

    SetDlgItemText(hDlg, iControl, szUnknown);

    return;
}





/*
 *  Intializes\Updates the global ProcessDeviceMapInfo which is used
 *  by GetNextDriveByType().
 *
 *  WARNING: NOT multithread safe!
 */
BOOL
IntializeDeviceMapInfo(
   void
   )
{
    NTSTATUS Status;

    Status = NtQueryInformationProcess(NtCurrentProcess(),
                                       ProcessDeviceMap,
                                       &ProcessDeviceMapInfo.Query,
                                       sizeof(ProcessDeviceMapInfo.Query),
                                       NULL
                                       );
    if (!NT_SUCCESS(Status)) {

        RtlZeroMemory(&ProcessDeviceMapInfo, sizeof(ProcessDeviceMapInfo));
        return FALSE;
    }

    return TRUE;
}



UINT
GetNextDriveByType(
    UINT DriveType,
    UINT DriveNumber
    )
/*++

Routine Description:

   Inspects each drive starting from DriveNumber in ascending order to find the
   first drive of the specified DriveType from the global ProcessDeviceMapInfo.
   The ProcessDeviceMapInfo must have been intialized and may need refreshing before
   invoking this function. Invoke IntializeDeviceMapInfo to initialize or update
   the DeviceMapInfo.

Arguments:

   DriveType - DriveType as defined in winbase, GetDriveType().

   DriveNumber - Starting DriveNumber, 1 based.

Return Value:

   DriveNumber - if nonzero Drive found, 1 based.

--*/
{

    //
    // OneBased DriveNumber to ZeroBased.
    //
    DriveNumber--;
    while (DriveNumber < 26) {

        if ((ProcessDeviceMapInfo.Query.DriveMap & (1<< DriveNumber)) &&
             ProcessDeviceMapInfo.Query.DriveType[DriveNumber] == DriveType) {

            return DriveNumber+1; // return 1 based DriveNumber found.
        }

        DriveNumber++;
    }

    return 0;
}



//
// This function takes a fully qualified path name and returns a pointer to
// the begining of the path portion.
//
// e.g. \\server\pathpart
//      d:\pathpart
//
// This function will always return a valid pointer, provided
// a valid pointer was passed in. If the caller passes in a badly
// formed pathname or an unknown format, where path ends up pointing is
// unknown, except that it will be someplace within the string.
//

WCHAR *
GetPathPart(
    WCHAR *Path
    )
{
    //
    // We assume that we are being passed a fully qualified path name
    //

    //
    // check for UNC path.
    //

    if (*Path == L'\\') {

        Path += 2;                           // skip double backslash
    }


    //
    // if UNC Path points to beg of server name
    // if drive letter format, points to the drive letter
    //

    while (*Path && *Path++ != L'\\') {

        ;
    }

    return Path;
}


//
//  Gets information about the specified file/drive root and sets the
//  icon and description fields in the dialog.
//
void
UpdateFileInfo(
    HWND        hDlg,
    LPWSTR      FileName,
    int         IconType,
    int         PathType
    )
{
    HICON hicon;
    DWORD Len;
    WCHAR *pwch;
    WCHAR *FilePart;
    WCHAR *PathPart;
    WCHAR *DisplayName;
    SHFILEINFOW ShFileInfo;
    WCHAR  DefaultPath[MAX_PATH];
    WCHAR  NameFormat[MAX_PATH];
    WCHAR  NameBuffer[MAX_PATH];


    switch (IconType) {

       case DRVUPD_INTERNETICON:
            hicon = LoadIcon(hNewDev, MAKEINTRESOURCE(IDI_INTERNET));

            LoadString(hNewDev, IDS_INTERNET_HOST, NameFormat, sizeof(NameFormat)/sizeof(WCHAR));

            if (!*FileName) {

                LoadString(hNewDev, IDS_DEFAULT_INTERNET_HOST, DefaultPath, sizeof(DefaultPath)/sizeof(WCHAR));
                wsprintf(NameBuffer, NameFormat, DefaultPath);

            } else {

                wsprintf(NameBuffer, NameFormat, FileName);
            }
            
            DisplayName = NameBuffer;
            break;

       case DRVUPD_SHELLICON:
            if (!*FileName) {

                hicon = NULL;
                DisplayName = FileName;
                break;
            }


            //
            // The file name may be a list of locations delimited by semicolons
            // for the icon, we use the first name in the list.
            //
            pwch = wcschr(FileName, L';');
            if (pwch) {

                *pwch = L'\0';
            }

            Len = GetFullPathNameW(FileName,
                                   sizeof(NameBuffer)/sizeof(WCHAR),
                                   NameBuffer,
                                   &FilePart
                                   );

            if (pwch) {

                *pwch = L';';
            }


            if (Len && Len < sizeof(NameBuffer)) {

                if (PathType == DRVUPD_PATHPART) {

                    PathPart = GetPathPart(NameBuffer);
                    *PathPart = '\0';
                    DisplayName = FileName;

                } else {

                    DisplayName = NameBuffer;
                }

                SHGetFileInfoW(NameBuffer,
                               0,
                               &ShFileInfo,
                               sizeof(ShFileInfo),
                               SHGFI_ICON | SHGFI_DISPLAYNAME | SHGFI_LARGEICON
                               );

                hicon = ShFileInfo.hIcon;

                break;
            }

            // in case of failure fall thru to do folder icon

        case DRVUPD_FOLDERICON:
        default:
            DisplayName = FileName;
            hicon = LoadIcon(hNewDev, MAKEINTRESOURCE(IDI_FOLDER));


    }

    hicon = (HICON)SendDlgItemMessage(hDlg,
                                      IDC_SEARCHICON,
                                      STM_SETICON,
                                      (WPARAM)hicon,
                                      0L
                                      );
    if (hicon) {

        DestroyIcon(hicon);
    }

    SetDlgItemText(hDlg, IDC_SEARCHNAME, DisplayName);

    return;
}

BOOL
pVerifyUpdateDriverInfoPath(
    PNEWDEVWIZ NewDevWiz
    )

/*++

    This API will verify that the selected driver node lives in the path
    specified in UpdateDriverInfo->InfPathName.
    
Return Value:
    This API will return TRUE in all cases except where we have a valid
    UpdateDriverInfo structure and a valid InfPathName field and that
    path does not match the path where the selected driver lives.
 
--*/

{
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;

    //
    // If we don't have a UpdateDriverInfo structure or a valid InfPathName field
    // in that structure then just return TRUE now.
    //
    if (!NewDevWiz->UpdateDriverInfo || !NewDevWiz->UpdateDriverInfo->InfPathName) {

        return TRUE;
    }

    //
    // Get the selected driver's path
    //
    ZeroMemory(&DriverInfoData, sizeof(DriverInfoData));
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 )) {
        //
        // There is no selected driver so just return TRUE
        //
        return TRUE;
    }

    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                    &NewDevWiz->DeviceInfoData,
                                    &DriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(DriverInfoDetailData),
                                    NULL
                                    )
        &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        
        //
        // We should never hit this case, but if we have a selected driver and
        // we can't get the SP_DRVINFO_DETAIL_DATA that contains the InfFileName
        // the return FALSE.
        //
        return FALSE;
    }

    if (lstrlen(NewDevWiz->UpdateDriverInfo->InfPathName) == 
        lstrlen(DriverInfoDetailData.InfFileName)) {

        //
        // If the two paths are the same size then we will just compare them
        //
        return (!lstrcmpi(NewDevWiz->UpdateDriverInfo->InfPathName,
                          DriverInfoDetailData.InfFileName));

    } else {

        //
        // The two paths are different lengths so we'll tack a trailing backslash
        // onto the UpdateDriverInfo->InfPathName and then do a _strnicmp
        // NOTE that we only tack on a trailing backslash if the length of the
        // path is greater than two since it isn't needed on the driver letter
        // followed by a colon case (A:).
        //
        // The reason we do this is we don't want the following case to match
        // c:\winnt\in
        // c:\winnt\inf\foo.inf
        //
        TCHAR TempPath[MAX_PATH];

        lstrcpy(TempPath, NewDevWiz->UpdateDriverInfo->InfPathName);

        if (lstrlen(NewDevWiz->UpdateDriverInfo->InfPathName) > 2) {
        
            lstrcat(TempPath, TEXT("\\"));
        }

        return (!_tcsnicmp(TempPath,
                           DriverInfoDetailData.InfFileName,
                           lstrlen(TempPath)));
    }
}

INT_PTR
InitDevInstallDlgProc(
      HWND hDlg,
      PNEWDEVWIZ NewDevWiz
      )
{
    PSEARCHTHREAD SearchThread;
    CONFIGRET ConfigRet;
    TCHAR  szString[MAX_PATH];
    ULONG  ulSize;
    USHORT Options;
    PUPDATEDRIVERINFO UpdateDriverInfo;
    HICON hicon;
    TCHAR Searching[MAX_PATH];


    //
    // Set the description field
    //

    ulSize = sizeof(szString);
    ConfigRet = CM_Get_DevNode_Registry_Property(NewDevWiz->DeviceInfoData.DevInst,
                                                 CM_DRP_DEVICEDESC,
                                                 NULL,
                                                 szString,
                                                 &ulSize,
                                                 0);

    if (ConfigRet == CR_SUCCESS && *szString) {

        SetDlgItemText(hDlg, IDC_NDW_DESCRIPTION, szString);
    
    } else {

        LoadString(hNewDev,
                   IDS_SEARCHING,
                   (PTCHAR)Searching,
                   SIZECHARS(Searching)
                   );

        SetDlgItemText(hDlg, IDC_NDW_DESCRIPTION, Searching);
    }

    //
    // Set the class Icon
    //
    if (!IsEqualGUID(&NewDevWiz->DeviceInfoData.ClassGuid, &GUID_NULL)) {

        if (SetupDiLoadClassIcon(&NewDevWiz->DeviceInfoData.ClassGuid, &hicon, NULL)) {
    
            hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            if (hicon) {
    
                DestroyIcon(hicon);
            }
        }
    }
    
    //
    // If this is UpdateDriver, set the dialog title
    //

    if (NewDevWiz->WizardType == NDWTYPE_UPDATE) {

        SetDlgText(hDlg, 0, IDS_UPDATEDEVICE,IDS_UPDATEDEVICE);
    }

    SetDlgText(hDlg, IDC_NDW_INSTRUCTIONS, IDS_DRVUPD_WAIT, IDS_DRVUPD_WAIT);


    //
    // Fetch the device capabilities
    //

    ulSize = sizeof(NewDevWiz->Capabilities);
    ConfigRet = CM_Get_DevNode_Registry_Property_Ex(NewDevWiz->DeviceInfoData.DevInst,
                                                    CM_DRP_CAPABILITIES,
                                                    NULL,
                                                    (PVOID)&NewDevWiz->Capabilities,
                                                    &ulSize,
                                                    0,
                                                    NULL
                                                    );
    if (ConfigRet != CR_SUCCESS) {

        NewDevWiz->Capabilities = 0;
    }


    NewDevWiz->ExitDetect = FALSE;
    NewDevWiz->CurrCursor = NewDevWiz->IdcAppStarting;
    SetCursor(NewDevWiz->CurrCursor);

    //
    // For UPDATE, search all drivers, including old internet drivers
    //
    if (NewDevWiz->WizardType == NDWTYPE_UPDATE) {

        Options = SEARCH_DEFAULT;

    //
    // For FOUNDNEW, don't include old Internet drivers in the list
    // of drivers.
    //
    } else {

        Options = SEARCH_DEFAULT_EXCLUDE_OLD_INET;
    }


    UpdateDriverInfo = NewDevWiz->UpdateDriverInfo;

    if (UpdateDriverInfo) {

        //
        // Drivers are from the Internet (newdev API called from WU)
        //
        if (UpdateDriverInfo->FromInternet) {
        
            wcscpy(NewDevWiz->BrowsePath, UpdateDriverInfo->InfPathName);
    
            if (UpdateDriverInfo->Force) {
    
                Options = SEARCH_WINDOWSUPDATE;
    
            } else {
    
                Options |= SEARCH_WINDOWSUPDATE;
            }
        }

        //
        // Normal app just telling us to update this device using the specified INF
        //
        else {

            wcscpy(NewDevWiz->SingleInfPath, UpdateDriverInfo->InfPathName);
    
            if (UpdateDriverInfo->Force) {
    
                Options = SEARCH_SINGLEINF;
    
            } else {
    
                Options |= SEARCH_SINGLEINF;
            }
        }
    }

    //
    // If this is a new device then do an initial search.  This is not needed for the
    // update driver case since the wizard will always be displayed.  Unless this is
    // the Windows Update UPDATE case...we don't want any UI in that case either.
    //
    if (!UpdateDriverInfo && NewDevWiz->WizardType == NDWTYPE_UPDATE) {
    
        //
        // Jump directly into the wizard
        //
        PostMessage(hDlg, WUM_SEARCHDRIVERS, 0, 0);

    } else {

        if (!SearchThreadRequest(NewDevWiz->SearchThread,
                                 hDlg,
                                 SEARCH_DRIVERS,
                                 Options,
                                 0
                                 )) {

            NewDevWiz->LastError = GetLastError();
            return FALSE;
        }
    }

    return TRUE;
}

LRESULT CALLBACK
DevInstallDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
   )
{
    PNEWDEVWIZ NewDevWiz=NULL;
    BOOL Status = TRUE;


    if (message == WM_INITDIALOG) {

        NewDevWiz = (PNEWDEVWIZ)lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        if (!InitDevInstallDlgProc(hDlg, NewDevWiz)) {

            DestroyWindow(hDlg);
        }

        return TRUE;
    }

    //
    // retrieve private data from window long (stored there during WM_INITDIALOG)
    //
    NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);


    switch (message) {

        case WM_DESTROY: {

            HICON hicon;

            CancelSearchRequest(NewDevWiz);

            hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_CLASSICON,STM_GETICON,0,0));
            if (hicon) {

                DestroyIcon(hicon);
            }

            hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_SEARCHICON,STM_GETICON,0,0));
            if (hicon) {

                DestroyIcon(hicon);
            }


            break;
        }

        case WM_CLOSE:
            SendMessage (hDlg, WM_COMMAND, IDCANCEL, 0L);
            break;

        case WM_COMMAND: {

            HCURSOR hOldCursor;

            switch(wParam) {

                case IDCANCEL: {

                    NewDevWiz->ExitDetect = TRUE;


                    NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
                    SetCursor(NewDevWiz->CurrCursor);
                    CancelSearchRequest(NewDevWiz);
                    NewDevWiz->CurrCursor = NULL;
                    NewDevWiz->LastError = ERROR_CANCELLED;

                    DestroyWindow(hDlg);
                }
            }
        }
        break;

        case WM_SETCURSOR:
            if (NewDevWiz->CurrCursor) {

                SetCursor(NewDevWiz->CurrCursor);
                break;
            }

            return FALSE;
            // break;


        case WUM_SEARCHDRIVERS: {

            SP_DRVINFO_DATA DriverInfoData;
            SP_DRVINSTALL_PARAMS DriverInstallParams;
            SP_DEVINSTALL_PARAMS  DeviceInstallParams;
            CONFIGRET ConfigRet;
            HCURSOR hCursor;

            NewDevWiz->CurrCursor = NULL;
            SetCursor(NewDevWiz->IdcArrow);

            if (NewDevWiz->ExitDetect) {

                break;
            }

            //
            // We are done searching for compatible drivers from the default
            // windows inf path (LOCAL_MACHINE\SOFTWARE\Windows\CurrentPath\DevicePath).
            // Fill in known information about the device.
            //
            // If this is found new and we have a HardwareID rank match go straight to install
            // else if update driver, or no HardwareID rank match, keep the dialog up for
            // a few seconds, before continuing.
            //

            SetDlgText(hDlg, IDC_NDW_INSTRUCTIONS, IDS_DRVUPD_INSTALLING, IDS_DRVUPD_INSTALLING);
            DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
            if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                         &NewDevWiz->DeviceInfoData,
                                         &DriverInfoData
                                         )) {

                wcscpy(NewDevWiz->DriverDescription, DriverInfoData.Description);

                //
                // fetch rank of driver found.
                //

                DriverInstallParams.cbSize = sizeof(DriverInstallParams);
                if (!SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                                   &NewDevWiz->DeviceInfoData,
                                                   &DriverInfoData,
                                                   &DriverInstallParams
                                                   )) {

                    DriverInstallParams.Rank = (DWORD)-1;
                }

            } else {

                *NewDevWiz->DriverDescription = L'\0';
                DriverInstallParams.Rank = (DWORD)-1;
                DriverInstallParams.Flags = 0;
            }

            //
            // If we know the class, set the class icon.
            //

            if (!IsEqualGUID(&NewDevWiz->DeviceInfoData.ClassGuid, &GUID_NULL)) {

                HICON hicon;


                //
                // Set the class Icon
                //

                NewDevWiz->ClassGuidSelected = &NewDevWiz->DeviceInfoData.ClassGuid;
                if (SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) {

                    hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                    if (hicon) {

                        DestroyIcon(hicon);
                    }
                }

                if (!SetupDiClassNameFromGuid(NewDevWiz->ClassGuidSelected,
                                              NewDevWiz->ClassName,
                                              sizeof(NewDevWiz->ClassName),
                                              NULL
                                              )) {

                    NewDevWiz->ClassGuidSelected = NULL;
                    *NewDevWiz->ClassName = TEXT('\0');
                }
            }

            SetDriverDescription(hDlg, IDC_NDW_DESCRIPTION, NewDevWiz);


            //
            //
            // If Windows Update force install commence with the install silently
            // unless there is an error no further UI is necessary.
            //
            // For "Found New Hardware"
            //    with a HardwareID Rank Match driver
            //    OR
            //    with a Compatible driver and a silentinstall\rawdeviceok device
            //
            // jump into the finish page of the common newdev wizard to finish
            // the install. Otherwise invoke the wizard to step thru inf search and
            // install.
            //

            if (NewDevWiz->UpdateDriverInfo) {
                
                //
                // If we have UpdateDriverInfo and the caller specified a specfic InfPathName (whether
                // a full path to an INF or just the path where INFs live) then we want to verify
                // that the selected driver's INF lives in that specified path.  If it does not then
                // do not automatically install it since that is not what the caller intended.
                //
                if (pVerifyUpdateDriverInfoPath(NewDevWiz)) {
                
                    NewDevWiz->SilentMode = TRUE;

                    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                      &NewDevWiz->DeviceInfoData,
                                                      &DeviceInstallParams
                                                      )) {

                        DeviceInstallParams.Flags |= DI_QUIETINSTALL;
                        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                      &NewDevWiz->DeviceInfoData,
                                                      &DeviceInstallParams
                                                      );
                    }


                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    
                    InstallDeviceWizard(hDlg, NewDevWiz);

                    SetCursor(hCursor);

                    InstallSilentChilds(hDlg, NewDevWiz);

                    DestroyWindow(hDlg);
                    break;
                }

                //
                // Update Driver didn't give us a Rank zero driver to install.
                // just destroy the Dialog and return to the caller.
                //
                DestroyWindow(hDlg);

                break;
            }

            else if (NewDevWiz->WizardType == NDWTYPE_FOUNDNEW) {
                
                if (DriverInstallParams.Rank <= DRIVER_HARDWAREID_RANK) {
                    
                    NewDevWiz->SilentMode = TRUE;

                    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
                    if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                      &NewDevWiz->DeviceInfoData,
                                                      &DeviceInstallParams
                                                      )) {

                        DeviceInstallParams.Flags |= DI_QUIETINSTALL;
                        SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                                      &NewDevWiz->DeviceInfoData,
                                                      &DeviceInstallParams
                                                      );
                    }

                    hCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
                    
                    InstallDeviceWizard(hDlg, NewDevWiz);

                    SetCursor(hCursor);

                    if (!(NewDevWiz->Capabilities & CM_DEVCAP_SILENTINSTALL)) {

                        InstallSilentChilds(hDlg, NewDevWiz);
                    }

                    DestroyWindow(hDlg);
                    
                    break;
                }

                //
                // Devices that don't have a hardware ID match should cause UI
                // to be presented, regardless of their SilentInstall or RawDeviceOK 
                // capabilities.
                //
                else if ((NewDevWiz->Capabilities & CM_DEVCAP_RAWDEVICEOK) &&
                        (NewDevWiz->Capabilities & CM_DEVCAP_SILENTINSTALL)) {
                    //
                    // If the device is both raw and silent, and has a
                    // non-hardware-id match, then we want to just install the
                    // null driver.
                    //

                    ULONG DevNodeStatus, Problem;


                    //
                    // RawDevices don't have have to have a driver provided
                    // the devnode is marked as started.
                    //

                    ConfigRet = CM_Get_DevNode_Status(&DevNodeStatus,
                                                      &Problem,
                                                      NewDevWiz->DeviceInfoData.DevInst,
                                                      0
                                                      );

                    if (ConfigRet == CR_SUCCESS && (DevNodeStatus & DN_STARTED)) {

                        InstallNullDriver(hDlg, NewDevWiz, FALSE);
                        DestroyWindow(hDlg);
                        break;
                    }
                }
            }


            //
            // Enter the common wizard entry for found new and update driver.
            //


            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              )) {

                ULONG ConfigFlag, Len;


                //
                // if not manually installed, allow excluded drivers.
                //

                Len = sizeof(ConfigFlag);
                ConfigRet = CM_Get_DevNode_Registry_Property_Ex(
                                   NewDevWiz->DeviceInfoData.DevInst,
                                   CM_DRP_CONFIGFLAGS,
                                   NULL,
                                   (PVOID)&ConfigFlag,
                                   &Len,
                                   0,
                                   NULL
                                   );

                if (ConfigRet == CR_SUCCESS && !(ConfigFlag & CONFIGFLAG_MANUAL_INSTALL)) {

                    DeviceInstallParams.FlagsEx |= DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
                }


                //
                // Shouldn't be quietinstall, since we can't find a HardwareID rank driver.
                // and user needs to choose or search for non-HardwareID rank driver.
                //

                DeviceInstallParams.Flags &= ~DI_QUIETINSTALL;

                SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                              &NewDevWiz->DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }

            UpdateDeviceWizard(GetParent(hDlg), NewDevWiz);

            if (NewDevWiz->WizardType == NDWTYPE_FOUNDNEW &&
               !(NewDevWiz->Capabilities & CM_DEVCAP_SILENTINSTALL)) {

                InstallSilentChilds(hDlg, NewDevWiz);
            }

            DestroyWindow(hDlg);

            break;
        }

        default:
            return FALSE;

    }


    return TRUE;

}  // DevInstallDlgProc

void
UpdateInstallUiOnly(
    HWND hDlg,
    PTSTR DeviceId
    )
{
    DEVINST DevInst;
    HICON hIcon = NULL;
    GUID ClassGuid;
    TCHAR szBuffer[MAX_PATH];
    ULONG ulSize;
    PTCHAR FriendlyName = NULL;

    if (DeviceId && CM_Locate_DevNode(&DevInst,
                                      DeviceId,
                                      CM_LOCATE_DEVNODE_NORMAL
                                      ) == CR_SUCCESS) {
        
        //
        // Set the class Icon
        //
        ulSize = sizeof(szBuffer);
        if (CM_Get_DevNode_Registry_Property(DevInst,
                                             CM_DRP_CLASSGUID,
                                             NULL,
                                             szBuffer,
                                             &ulSize,
                                             0
                                             ) == CR_SUCCESS) {
            
            pSetupGuidFromString(szBuffer, &ClassGuid);
    
            if (!IsEqualGUID(&ClassGuid, &GUID_NULL)) {
    
                SetupDiLoadClassIcon(&ClassGuid, &hIcon, NULL);
            }
        }

        //
        // Get the device friendly name
        //
        FriendlyName = BuildFriendlyName(DevInst, TRUE, NULL);
    }

    //
    // Set the class icon
    //
    if (!hIcon) {

        //
        // Either no device was specified or we could not get a class icon. So just
        // Grab the generic icon
        //
        hIcon = LoadIcon(hNewDev, MAKEINTRESOURCE(IDI_NEWDEVICEICON));
    }

    hIcon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hIcon, 0L);

    if (hIcon) {

        DestroyIcon(hIcon);
    }

    //
    // Set the device description
    //
    if (FriendlyName) {

        SetDlgItemText(hDlg, IDC_NDW_DESCRIPTION, FriendlyName);
        LocalFree(FriendlyName);
    
    } else {

        //
        // We could not get a friendly name for the device or not device was specified
        // so just display the Searching... text.
        //
        LoadString(hNewDev, IDS_NEWSEARCH, szBuffer, SIZECHARS(szBuffer));
        SetDlgItemText(hDlg, IDC_NDW_DESCRIPTION, szBuffer);
    }
}

LRESULT CALLBACK
DevInstallUiOnlyDlgProc(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
   )
{
    static BOOL bCanExit;
    
    switch (message) {
    case WM_INITDIALOG:
        //
        // We want the Ui to be displayed for at least 3 seconds otherwise it flashes too 
        // quickly and a user can't see it.
        //
        bCanExit = FALSE;
        SetTimer(hDlg, INSTALL_UI_TIMERID, 3000, NULL);
        return TRUE;

    case WM_DESTROY: {

        HICON hicon;

        hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_CLASSICON,STM_GETICON,0,0));
        if (hicon) {

            DestroyIcon(hicon);
        }

        break;
    }

    case WM_TIMER:
        if (INSTALL_UI_TIMERID == wParam) {

            //
            // At this point the Ui has been displayed for at least 3 seconds so we can
            // exit whenever we are finished.  If bCanExit is already TRUE then we have
            // already been asked to exit so just do a DestroyWindow at this point, otherwise
            // set bCanExit to TRUE so we can exit when we are finished installing devices.
            //
            if (bCanExit) {
            
                DestroyWindow(hDlg);

            } else {
                
                KillTimer(hDlg, INSTALL_UI_TIMERID);
                bCanExit = TRUE;
            }
        }
        break;

    case WUM_UPDATEUI:
        UpdateInstallUiOnly(hDlg, (PTSTR)lParam);
        break;

    case WUM_EXIT:
        if (bCanExit) {
        
            DestroyWindow(hDlg);
        } else {

            ShowWindow(hDlg, SW_SHOW);
            bCanExit = TRUE;
        }
        break;

    default:
        return FALSE;

    }


    return TRUE;

}  // DevInstallUiOnlyDlgProc

void
InitDriverUpdateDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    if (NewDevWiz->WizardType == NDWTYPE_FOUNDNEW) {

        SetDlgText(hDlg, IDC_DRVUPD_DRVMSG1, IDS_NEWDRIVER_WELCOME, IDS_NEWDRIVER_WELCOME);
        SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_NEWDRIVER_WELCOME1, IDS_NEWDRIVER_WELCOME1);

    } else {

        SetDlgText(hDlg, IDC_DRVUPD_DRVMSG1, IDS_DRVUPD_WELCOME, IDS_DRVUPD_WELCOME);
        SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_DRVUPD_WELCOME1, IDS_DRVUPD_WELCOME1);
    }


    //
    // Set the Initial radio button state to do auto-search.
    //

    CheckRadioButton(hDlg,
                     IDC_DRVUPD_SEARCH,
                     IDC_DRVUPD_SELECTDRIVER,
                     IDC_DRVUPD_SEARCH
                     );


//
// temp debug code to dump pnp hdwids.
//
#if DBG
    {
       CONFIGRET ConfigRet;
       ULONG Len;
       TCHAR Buffer[MAX_PATH];

       Len = sizeof(Buffer);
       memset(Buffer, 0 , sizeof(Buffer));
       ConfigRet = CM_Get_DevNode_Registry_Property_Ex(NewDevWiz->DeviceInfoData.DevInst,
                                                       CM_DRP_HARDWAREID,
                                                       NULL,
                                                       (PVOID)Buffer,
                                                       &Len,
                                                       0,
                                                       NULL
                                                       );
       DbgPrint("InstallDev HARDWAREID(s): %ws\n",
                ConfigRet == CR_SUCCESS ? Buffer : L"?"
                );


       Len = sizeof(Buffer);
       memset(Buffer, 0 , sizeof(Buffer));
       ConfigRet = CM_Get_DevNode_Registry_Property_Ex(NewDevWiz->DeviceInfoData.DevInst,
                                                       CM_DRP_COMPATIBLEIDS,
                                                       NULL,
                                                       (PVOID)Buffer,
                                                       &Len,
                                                       0,
                                                       NULL
                                                       );
       DbgPrint("InstallDev COMPATBLEID(s): %ws\n",
                ConfigRet == CR_SUCCESS ? Buffer : L"?"
                );
       }
#endif

    return;
}




//
// "Update Device Driver Wizard" that is launched when the user clicks
// the "Update Driver" button, or when we get new plug and play hardware
// that we do not have a HardwareID rank match.
//

INT_PTR CALLBACK
DriverUpdateDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz;
    HICON hicon;


    if (message == WM_INITDIALOG) {

        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        InitDriverUpdateDlgProc(hDlg, NewDevWiz);

        return TRUE;
    }

    NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message) {

        case WM_NOTIFY:
            switch (((NMHDR FAR *)lParam)->code) {

                case PSN_SETACTIVE: {

                    NewDevWiz->PrevPage = IDD_DRVUPD;
                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                    //
                    // Set the Driver description field. We do this each time we are
                    // activated, since we may not have it if we haven't found
                    // a compatible driver yet.
                    //
                    SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);

                    hicon = NULL;
                    if (NewDevWiz->ClassGuidSelected &&
                        SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL)) {

                        hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);

                    } else {

                        SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                        SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
                    }

                    if (hicon) {

                        DestroyIcon(hicon);
                    }

                    break;
                }


                case PSN_WIZNEXT:
                    if (IsDlgButtonChecked(hDlg, IDC_DRVUPD_SEARCH)) {

                        SetDlgMsgResult(hDlg, message, IDD_DRVUPD_SEARCH);
                    }

                    else if (IsDlgButtonChecked(hDlg, IDC_DRVUPD_SELECTDRIVER)) {

                        ULONG DevNodeStatus;
                        ULONG Problem=0;
                        HDEVINFO hDeviceInfo;
                        SP_DRVINFO_DATA DriverInfoData;
                        HWND hwndParentDlg = GetParent(hDlg);


                        //
                        // If we have a selected driver,
                        // or we know the class and there wasn't a problem installing
                        // go into select device
                        //
                        // TBD: there may be a number of other cases in which we should
                        // go into the selclass page instead.
                        //

                        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
                        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                                  &NewDevWiz->DeviceInfoData,
                                                  SPDIT_COMPATDRIVER,
                                                  0,
                                                  &DriverInfoData
                                                  )
                            ||
                            (!IsEqualGUID(&NewDevWiz->DeviceInfoData.ClassGuid,
                                          &GUID_NULL
                                          )

                             &&
                             CM_Get_DevNode_Status(&DevNodeStatus,
                                                   &Problem,
                                                   NewDevWiz->DeviceInfoData.DevInst,
                                                   0
                                                   ) == CR_SUCCESS
                             &&
                             Problem != CM_PROB_FAILED_INSTALL
                             )) {

                            NewDevWiz->ClassGuidSelected = &NewDevWiz->DeviceInfoData.ClassGuid;
                            NewDevWiz->EnterInto = IDD_NEWDEVWIZ_SELECTDEVICE;
                            NewDevWiz->EnterFrom = IDD_DRVUPD;
                            SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_SELECTDEVICE);
                            break;
                        }

                        NewDevWiz->ClassGuidSelected = NULL;
                        NewDevWiz->EnterInto = IDD_NEWDEVWIZ_SELECTCLASS;
                        NewDevWiz->EnterFrom = IDD_DRVUPD;
                        SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_SELECTCLASS);

                    }
                    break;

                case PSN_WIZBACK:
                    SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_INTRO);
                    break;

                default:
                    return FALSE;
            }
            break;

        case WM_DESTROY:
        ;
        break;

        default:
            return FALSE;

    } // end of switch on message

    return TRUE;
}  // DriverUpdateDlgProc




void
InstallSilentChildSiblings(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz,
   DEVINST DeviceInstance,
   BOOL ReinstallAll
   )
{
    CONFIGRET ConfigRet;
    DEVINST ChildDeviceInstance;
    ULONG Ulong, ulValue;
    BOOL NeedsInstall, IsSilent;

    do {

        //
        // If this device instance needs installing and is silent then install it,
        // and its children.
        //

        IsSilent = FALSE;
        if (!ReinstallAll) {

            Ulong = sizeof(ulValue);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_CAPABILITIES,
                                                            NULL,
                                                            (PVOID)&ulValue,
                                                            &Ulong,
                                                            0,
                                                            NULL
                                                            );

            if (ConfigRet == CR_SUCCESS && (ulValue & CM_DEVCAP_SILENTINSTALL)) {

                IsSilent = TRUE;
            }
        }

        if (IsSilent || ReinstallAll) {

            Ulong = sizeof(ulValue);
            ConfigRet = CM_Get_DevNode_Registry_Property_Ex(DeviceInstance,
                                                            CM_DRP_CONFIGFLAGS,
                                                            NULL,
                                                            (PVOID)&ulValue,
                                                            &Ulong,
                                                            0,
                                                            NULL
                                                            );

            if (ConfigRet == CR_SUCCESS && (ulValue & CONFIGFLAG_FINISH_INSTALL)) {

                NeedsInstall = TRUE;

            } else {

                ConfigRet = CM_Get_DevNode_Status(&Ulong,
                                                  &ulValue,
                                                  DeviceInstance,
                                                  0
                                                  );

                NeedsInstall = ConfigRet == CR_SUCCESS &&
                               (ulValue == CM_PROB_REINSTALL ||
                                ulValue == CM_PROB_NOT_CONFIGURED
                                );
            }


            if (NeedsInstall) {

                TCHAR DeviceInstanceId[MAX_DEVICE_ID_LEN];

                ConfigRet = CM_Get_Device_ID(DeviceInstance,
                                            DeviceInstanceId,
                                            sizeof(DeviceInstanceId)/sizeof(TCHAR),
                                            0
                                            );

                if (ConfigRet == CR_SUCCESS) {

                    if (InstallDevInst(hwndParent,
                                       DeviceInstanceId,
                                       FALSE,   // only for found new.
                                       &Ulong
                                       )) {

                        NewDevWiz->Reboot |= Ulong;
                    }


                    //
                    // If this devinst has children, then recurse to install them as well.
                    //

                    ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                                DeviceInstance,
                                                0,
                                                NULL
                                                );

                    if (ConfigRet == CR_SUCCESS) {

                        InstallSilentChildSiblings(hwndParent, NewDevWiz, ChildDeviceInstance, ReinstallAll);
                    }

                }
            }
        }


        //
        // Next sibling ...
        //

        ConfigRet = CM_Get_Sibling_Ex(&DeviceInstance,
                                      DeviceInstance,
                                      0,
                                      NULL
                                      );

    } while (ConfigRet == CR_SUCCESS);

}





void
InstallSilentChilds(
   HWND hwndParent,
   PNEWDEVWIZ NewDevWiz
   )
{
    CONFIGRET ConfigRet;
    DEVINST ChildDeviceInstance;

    ConfigRet = CM_Get_Child_Ex(&ChildDeviceInstance,
                                NewDevWiz->DeviceInfoData.DevInst,
                                0,
                                NULL
                                );

    if (ConfigRet == CR_SUCCESS) {

        InstallSilentChildSiblings(hwndParent, NewDevWiz, ChildDeviceInstance, FALSE);
    }
}
