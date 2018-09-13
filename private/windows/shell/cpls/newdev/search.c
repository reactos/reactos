//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       search.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <regstr.h>

typedef struct _DirectoryNameList {
   struct _DirectoryNameList *Next;
   UNICODE_STRING DirectoryName;
   WCHAR NameBuffer[1];
} DIRNAMES, *PDIRNAMES;


// CDM exports (there is no public header)
typedef
BOOL
(*PFNCDMINTERNETAVAILABLE)(
    void
    );




WCHAR StarDotStar[]=L"*.*";


void
SetDriverPath(
   PNEWDEVWIZ NewDevWiz,
   TCHAR      *DriverPath,
   BOOL       InetSearch
   )
{
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    SP_DRVINFO_DATA DriverInfoData;
    TCHAR *PathUI;

    PathUI = DriverPath;

    //
    // If the Path is NULL then set it to the default INF search path,
    // unless this is an Internet search.  A NULL path for an Internet
    // search means to search Windows Update.
    //
    if (!PathUI) {
    
        if (InetSearch) {

            PathUI = TEXT("");
        
        } else {

            PathUI = DevicePath ? DevicePath : TEXT("");
        }
    }


    UpdateFileInfo(NewDevWiz->SearchThread->hDlg,
                   PathUI,
                   InetSearch ? DRVUPD_INTERNETICON: DRVUPD_SHELLICON,
                   DRVUPD_PATHPART
                   );

    DeviceInstallParams.cbSize = sizeof(DeviceInstallParams);
    SetupDiGetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  &DeviceInstallParams
                                  );

    wcscpy(DeviceInstallParams.DriverPath, DriverPath ? DriverPath : L"");

    SetupDiSetDeviceInstallParams(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  &DeviceInstallParams
                                  );
}



void
SearchDirectoryForDrivers(
    PNEWDEVWIZ NewDevWiz,
    WCHAR *Directory
    )
{
    HANDLE FindHandle;
    PDIRNAMES DirNamesHead=NULL;
    PDIRNAMES DirNames, Next;
    PWCHAR AppendSub;
    ULONG Len;
    WIN32_FIND_DATAW FindData;
    WCHAR SubDirName[MAX_PATH+sizeof(WCHAR)];

    if (NewDevWiz->SearchThread->CancelRequest) {

        return;
    }

    Len = wcslen(Directory);
    memcpy(SubDirName, Directory, Len*sizeof(WCHAR));
    AppendSub = SubDirName + Len;


    //
    // See if there are is anything (files, subdirs) in this dir.
    //
    *AppendSub = L'\\';
    memcpy(AppendSub+1, StarDotStar, sizeof(StarDotStar));
    FindHandle = FindFirstFileW(SubDirName, &FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {

        return;
    }

    //
    // There might be inf files so invoke setup to look.
    //
    *AppendSub = L'\0';
    SetDriverPath(NewDevWiz, Directory, FALSE);
    SetupDiBuildDriverInfoList(NewDevWiz->hDeviceInfo,
                                   &NewDevWiz->DeviceInfoData,
                                   SPDIT_COMPATDRIVER
                                   );




    //
    // find all of the subdirs, and save them in a temporary buffer,
    // so that we can close the find handle *before* going recursive.
    //

    do {

        if (NewDevWiz->SearchThread->CancelRequest) {

            break;
        }

        if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) &&
            wcscmp(FindData.cFileName, L".") &&
            wcscmp(FindData.cFileName, L".."))
        {
            USHORT Len;

            Len = (USHORT)wcslen(FindData.cFileName) * sizeof(WCHAR);
            DirNames = malloc(sizeof(DIRNAMES) + Len);
            if (!DirNames) {

                return;
            }

            DirNames->DirectoryName.Length = Len;
            DirNames->DirectoryName.MaximumLength = Len + sizeof(WCHAR);
            DirNames->DirectoryName.Buffer = DirNames->NameBuffer;
            memcpy(DirNames->NameBuffer, FindData.cFileName, Len + sizeof(WCHAR));

            DirNames->Next = DirNamesHead;
            DirNamesHead = DirNames;

        }

    } while (FindNextFileW(FindHandle, &FindData));

    FindClose(FindHandle);

    if (!DirNamesHead) {

        return;
    }


    *AppendSub++ = L'\\';


    Next = DirNamesHead;
    while (Next) {

         DirNames = Next;

         memcpy(AppendSub,
                DirNames->DirectoryName.Buffer,
                DirNames->DirectoryName.Length + sizeof(WCHAR)
                );

         Next= DirNames->Next;
         free(DirNames);
         SearchDirectoryForDrivers(NewDevWiz, SubDirName);
     }
}


void
SearchDriveForDrivers(
    PNEWDEVWIZ NewDevWiz,
    UINT DriveNumber
    )
{
    UINT PrevMode;
    TCHAR DriveRoot[]=TEXT("a:");

    DriveRoot[0] = DriveNumber - 1 + DriveRoot[0];

    PrevMode = SetErrorMode(0);
    SetErrorMode(PrevMode | SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    SearchDirectoryForDrivers(NewDevWiz,  DriveRoot);

    SetErrorMode(PrevMode);
}





void
InitDriverSearchDlgProc(
    HWND hDlg,
    PNEWDEVWIZ NewDevWiz
    )
{
    HICON hicon;
    HMODULE hModCDM = NULL;
    PFNCDMINTERNETAVAILABLE CdmInternetAvailable;

    //
    // Grey out or Check the appropriate search option check boxes
    //

    IntializeDeviceMapInfo();

    if (GetNextDriveByType(DRIVE_REMOVABLE, 1)) {

        //
        // This machine has a floppy drive
        //
        EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_FLOPPY), TRUE);

        CheckDlgButton(hDlg, IDC_SEARCHOPTION_FLOPPY, 
            (NewDevWiz->SearchOptions & SEARCH_FLOPPY) ? BST_CHECKED : BST_UNCHECKED);

    } else {

        //
        // This machine does not have a floppy drive
        //
        EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_FLOPPY), FALSE);
        CheckDlgButton(hDlg, IDC_SEARCHOPTION_FLOPPY, BST_UNCHECKED);
    }


    if (GetNextDriveByType(DRIVE_CDROM, 1)) {

        //
        // This machine has a CD-ROM or DVD drive
        //
        EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_CDROM), TRUE);

        CheckDlgButton(hDlg, IDC_SEARCHOPTION_CDROM, 
            (NewDevWiz->SearchOptions & SEARCH_CDROM) ? BST_CHECKED : BST_UNCHECKED);

    } else {

        // 
        // This machine does not have a CD_ROM or DVD drive
        //
        EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_CDROM), FALSE);
        CheckDlgButton(hDlg, IDC_SEARCHOPTION_CDROM, BST_UNCHECKED);
    }


    if (!NewDevWiz->UpdateDriverInfo
        &&
        (hModCDM = LoadLibrary(TEXT("cdm.dll")))
        &&
        (CdmInternetAvailable = (PVOID)GetProcAddress(hModCDM,
                                                      "DownloadIsInternetAvailable"
                                                      ))
        &&
        CdmInternetAvailable()) {

        //
        // This machine is capable of searching the Internet
        //
        EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_INTERNET), TRUE);

        CheckDlgButton(hDlg, IDC_SEARCHOPTION_INTERNET, 
            (NewDevWiz->SearchOptions & SEARCH_INET) ? BST_CHECKED : BST_UNCHECKED);

    } else {

        //
        // This machine is not capable of searching the Internet
        //
        EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_INTERNET), FALSE);
        CheckDlgButton(hDlg, IDC_SEARCHOPTION_INTERNET, BST_UNCHECKED);
    }

    if (hModCDM) {
    
        FreeLibrary(hModCDM);
    }

    //
    // Searching other locations
    //
    EnableWindow(GetDlgItem(hDlg, IDC_SEARCHOPTION_OTHER), TRUE );

    CheckDlgButton(hDlg, IDC_SEARCHOPTION_OTHER, 
        (NewDevWiz->SearchOptions & SEARCH_PATH) ? BST_CHECKED : BST_UNCHECKED);

}





INT_PTR CALLBACK
DriverSearchDlgProc(
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

        InitDriverSearchDlgProc(hDlg, NewDevWiz);

        return TRUE;
    }

    NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message) {

    case WM_NOTIFY:
        
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE:
            NewDevWiz->PrevPage = IDD_DRVUPD_SEARCH;
            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);

            hicon = NULL;
            if (NewDevWiz->ClassGuidSelected &&
                SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL))
            {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }
            
            else {
                
                SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {

                DestroyIcon(hicon);
            }

            break;


        case PSN_WIZNEXT:
            NewDevWiz->SearchOptions = 0;

            // fetch the current Search Options
            NewDevWiz->SearchOptions |= SEARCH_DEFAULT;

            if (IsDlgButtonChecked(hDlg, IDC_SEARCHOPTION_FLOPPY)) {

                NewDevWiz->SearchOptions |= SEARCH_FLOPPY;
            }
            
            if (IsDlgButtonChecked(hDlg, IDC_SEARCHOPTION_CDROM)) {
                
                NewDevWiz->SearchOptions |= SEARCH_CDROM;
            }
            
            if (IsDlgButtonChecked(hDlg, IDC_SEARCHOPTION_INTERNET)) {
                
                NewDevWiz->SearchOptions |= SEARCH_INET;
            }
            
            if (IsDlgButtonChecked(hDlg, IDC_SEARCHOPTION_OTHER)) {
                
                NewDevWiz->SearchOptions |= SEARCH_PATH;
            }


            SetDlgMsgResult(hDlg, message, IDD_DRVUPD_SEARCHING);
            break;

        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, message, IDD_DRVUPD);
            break;

        case PSN_RESET:
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
}


WCHAR szINFDIR[]=L"\\inf\\";

BOOL
IsInstalledDriver(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData
   )
/*++
    Determines if the currently selected driver is the
    currently installed driver. By comparing DriverInfoData
    and DriverInfoDetailData.
    
--*/
{
    BOOL bReturn;
    HKEY  hDevRegKey;
    DWORD cbData, Len;
    PWCHAR pwch;

    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    TCHAR Buffer[MAX_PATH*2];
    PVOID pvBuffer=Buffer;


    bReturn = FALSE;



    //
    // Open a reg key to the driver specific location
    //

    hDevRegKey = SetupDiOpenDevRegKey(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DRV,
                                      KEY_READ
                                      );

    if (hDevRegKey == INVALID_HANDLE_VALUE) {
    
        goto SIIDExit;
    }


    //
    // Compare Description, Manufacturer, and Provider Name.
    // These are the three unique "keys" within a single inf file.
    // Fetch the drvinfo, drvdetailinfo for the selected device.
    //

    //
    // If the Device Description isn't the same, its a different driver.
    //

    if (!SetupDiGetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          SPDRP_DEVICEDESC,
                                          NULL,                 // regdatatype
                                          pvBuffer,
                                          sizeof(Buffer),
                                          NULL
                                          )) {
                                          
        *Buffer = TEXT('\0');
    }

    if (wcscmp(DriverInfoData->Description, Buffer)) {
    
        goto SIIDExit;
    }


    //
    // If the Manufacturer Name isn't the same, its different
    //

    if (!SetupDiGetDeviceRegistryProperty(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          SPDRP_MFG,
                                          NULL, // regdatatype
                                          pvBuffer,
                                          sizeof(Buffer),
                                          NULL
                                          )) {
                                          
        *Buffer = TEXT('\0');
    }

    if (wcscmp(DriverInfoData->MfgName, Buffer)) {
    
        goto SIIDExit;
    }



    //
    // If the Provider Name isn't the same, its different
    //

    cbData = sizeof(Buffer);
    if (RegQueryValueEx(hDevRegKey,
                        REGSTR_VAL_PROVIDER_NAME,
                        NULL,
                        NULL,
                        pvBuffer,
                        &cbData
                        ) != ERROR_SUCCESS) {
                        
        *Buffer = TEXT('\0');
    }

    if (wcscmp(DriverInfoData->ProviderName, Buffer)) {
    
        goto SIIDExit;
    }



    //
    // Check the InfName, InfSection and DriverDesc
    // NOTE: the installed infName will not contain the path to the default windows
    // inf directory. If the same inf name has been found for the selected driver
    // from another location besides the default inf search path, then it will
    // contain a path, and is treated as a *different* driver.
    //


    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);
    if (!SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                    &NewDevWiz->DeviceInfoData,
                                    DriverInfoData,
                                    &DriverInfoDetailData,
                                    sizeof(DriverInfoDetailData),
                                    NULL
                                    )
        &&
        GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        
        goto SIIDExit;
    }



    Len = GetWindowsDirectory(Buffer, MAX_PATH);
    if (Len && Len < MAX_PATH) {
    
        pwch = Buffer + Len - 1;
        if (*pwch != L'\\') {
        
            pwch++;
        }

        wcscpy(pwch, szINFDIR);
        pwch += sizeof(szINFDIR)/sizeof(WCHAR) - 1;

        cbData = MAX_PATH*sizeof(WCHAR);
        if (RegQueryValueEx(hDevRegKey,
                            REGSTR_VAL_INFPATH,
                            NULL,
                            NULL,
                            (PVOID)pwch,
                            &cbData
                            ) != ERROR_SUCCESS )
        {
            *Buffer = TEXT('\0');
        }


        if (_wcsicmp( DriverInfoDetailData.InfFileName, Buffer)) {
        
            goto SIIDExit;
        }

    } else {
    
        goto SIIDExit;
    }



    cbData = sizeof(Buffer);
    if (RegQueryValueEx(hDevRegKey,
                        REGSTR_VAL_INFSECTION,
                        NULL,
                        NULL,
                        pvBuffer,
                        &cbData
                        ) != ERROR_SUCCESS ) {
                        
        *Buffer = TEXT('\0');
    }

    if (wcscmp(DriverInfoDetailData.SectionName, Buffer)) {
    
        goto SIIDExit;
    }


    cbData = sizeof(Buffer);
    if (RegQueryValueEx(hDevRegKey,
                        REGSTR_VAL_DRVDESC,
                        NULL,
                        NULL,
                        pvBuffer,
                        &cbData
                        ) != ERROR_SUCCESS ) {
                        
        *Buffer = TEXT('\0');
    }

    if (wcscmp(DriverInfoDetailData.DrvDescription, Buffer)) {
    
        goto SIIDExit;
    }


    bReturn = TRUE;



SIIDExit:

    if (hDevRegKey != INVALID_HANDLE_VALUE) {
    
        RegCloseKey(hDevRegKey);
    }


    return bReturn;
}

BOOL
IsOldInternetDriver(
   PNEWDEVWIZ NewDevWiz,
   PSP_DRVINFO_DATA DriverInfoData
   )
/*++
    Determins if the specified driver node is an Internet driver by checking
    if the SP_DRVINSTALL_PARAMS.Flags DNF_OLD_INET_DRIVER flag is set.
    
--*/
{
    SP_DRVINSTALL_PARAMS DriverInstallParams;

    //
    // Get the DriverInstallParams so we can see if we got this driver from the Internet
    //
    ZeroMemory(&DriverInstallParams, sizeof(SP_DRVINSTALL_PARAMS));
    DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);

    if (SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      DriverInfoData,
                                      &DriverInstallParams) &&
        (DriverInstallParams.Flags & DNF_OLD_INET_DRIVER))  {

        return TRUE;
    }

    return FALSE;
}

INT_PTR CALLBACK
DriverSearchingDlgProc(
    HWND hDlg, 
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz;

    if (message == WM_INITDIALOG) {
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);
        return TRUE;
        }

    NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch(message) {

    case WM_NOTIFY:
        switch (((NMHDR FAR *)lParam)->code) {

        case PSN_SETACTIVE: {
            HICON hicon;
            int PrevPage;

            SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);

            hicon = NULL;
            if (NewDevWiz->ClassGuidSelected &&
                SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL))
            {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }
            
            else {
                
                SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {
                
                DestroyIcon(hicon);
            }

            ShowWindow(GetDlgItem(hDlg, IDC_SEARCHICON), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_SEARCHNAME), SW_SHOW);
            ShowWindow(GetDlgItem(hDlg, IDC_CANCELINSTALL), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_FINISHINSTALL), SW_HIDE);

            PrevPage = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_DRVUPD_SEARCHING;
            NewDevWiz->ExitDetect = FALSE;


            //
            // if coming from DRVUPD_SEARCH page then begin driver search
            //
            if (PrevPage == IDD_DRVUPD_SEARCH) {
                
                CheckDlgButton(hDlg, IDC_LISTDRIVERS, BST_UNCHECKED);
                ShowWindow(GetDlgItem(hDlg, IDC_LISTDRIVERS), SW_HIDE);
                ShowWindow(GetDlgItem(hDlg, IDC_LISTDRIVERS_TEXT), SW_HIDE);


                SetDlgText(hDlg, IDC_DRVUPD_DRVMSG1, IDS_SEARCHING_WAIT, IDS_SEARCHING_WAIT);
                SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_DRVUPD_LOCATION, IDS_DRVUPD_LOCATION);
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);

                if (NewDevWiz->SearchOptions & SEARCH_PATH) {
                    
                    UINT    DiskPrompt;
                    TCHAR   Title[MAX_PATH];


                    LoadString(hNewDev,
                               NewDevWiz->WizardType == NDWTYPE_FOUNDNEW
                                  ? IDS_FOUNDDEVICE : IDS_UPDATEDEVICE,
                               Title,
                               sizeof(Title)/sizeof(TCHAR)
                               );

                    DiskPrompt = SetupPromptForDisk(
                                     hDlg,
                                     Title,
                                     NULL,   // DiskName
                                     *NewDevWiz->BrowsePath ? NewDevWiz->BrowsePath : NULL,
                                     L"*.inf",
                                     NULL,
                                     IDF_OEMDISK | IDF_NOCOMPRESSED | IDF_NOSKIP,
                                     NewDevWiz->BrowsePath,
                                     sizeof(NewDevWiz->BrowsePath)/sizeof(TCHAR),
                                     NULL
                                     );

                    if (DiskPrompt != DPROMPT_SUCCESS) {
                        
                        PropSheet_PressButton(GetParent(hDlg), PSBTN_BACK);
                        break;
                    }

                    UpdateFileInfo(hDlg,
                                   NewDevWiz->BrowsePath,
                                   DRVUPD_SHELLICON,
                                   DRVUPD_PATHPART
                                   );

                }

                NewDevWiz->CurrCursor = NewDevWiz->IdcAppStarting;
                SetCursor(NewDevWiz->CurrCursor);

                SearchThreadRequest(NewDevWiz->SearchThread,
                                    hDlg,
                                    SEARCH_DRIVERS,
                                    NewDevWiz->SearchOptions,
                                    0
                                    );
            }

            //
            // if coming back from DRVUPD_FINISH page, search is done,
            // so wait for instructions from user.
            //

            else {
                
                PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);
            }

        }
        break;

        case PSN_WIZNEXT:
            if (IsDlgButtonChecked(hDlg, IDC_LISTDRIVERS)) {
                
                SetDlgMsgResult(hDlg, message, IDD_LISTDRIVERS);
            }
             
            else if (NewDevWiz->DontReinstallCurrentDriver) {

                //
                // If the current driver is the best driver and it was from the
                // Internet then just jump directly to the finish page.
                //
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_FINISH);
            }

            else {
                
                //
                // wiznext only gets turned on if we have found an inf, so do the install.
                //
                NewDevWiz->EnterInto = IDD_NEWDEVWIZ_INSTALLDEV;
                NewDevWiz->EnterFrom = IDD_DRVUPD_SEARCHING;
                SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_INSTALLDEV);
            }

            break;


        case PSN_WIZBACK:
            if (NewDevWiz->ExitDetect) {
                
                SetDlgMsgResult(hDlg, message, -1);
                break;
            }

            NewDevWiz->ExitDetect = TRUE;
            NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
            SetCursor(NewDevWiz->CurrCursor);
            CancelSearchRequest(NewDevWiz);
            NewDevWiz->CurrCursor = NULL;

            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);
            SetDlgMsgResult(hDlg, message, IDD_DRVUPD_SEARCH);

            break;

        case PSN_QUERYCANCEL:
            if (NewDevWiz->ExitDetect) {
                
                SetDlgMsgResult(hDlg, message, TRUE);
                break;
            }

            NewDevWiz->ExitDetect = TRUE;
            NewDevWiz->CurrCursor = NewDevWiz->IdcWait;
            SetCursor(NewDevWiz->CurrCursor);
            CancelSearchRequest(NewDevWiz);
            NewDevWiz->CurrCursor = NULL;

            SetDlgMsgResult(hDlg, message, FALSE);

            break;

        case PSN_WIZFINISH:
            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), TRUE);

            //
            // Finish button only occurs if we couldn't find driver files,
            // and is an error condition. if its found new we install
            // the null driver, and mark the node as having a problem
            // (unless its raw device ok).
            //

            if (NewDevWiz->WizardType == NDWTYPE_FOUNDNEW)
            {
                if (IsDlgButtonChecked(hDlg, IDC_FINISHINSTALL))
                {
                    InstallNullDriver(hDlg,
                                      NewDevWiz,
                                      (NewDevWiz->Capabilities & CM_DEVCAP_RAWDEVICEOK)
                                         ? FALSE : TRUE
                                      );
                }

                else if (IsDlgButtonChecked(hDlg, IDC_CANCELINSTALL)) {
    
                    NewDevWiz->LastError = ERROR_CANCELLED;
                }
            }

            break;

        case PSN_RESET:
            break;

        default:
            return FALSE;
        }

        break;


    case WM_DESTROY: {
        
        HICON hicon;

        CancelSearchRequest(NewDevWiz);

        hicon = (HICON)LOWORD(SendDlgItemMessage(hDlg,IDC_SEARCHICON,STM_GETICON,0,0));
        
        if (hicon) {
            
            DestroyIcon(hicon);
        }
    }
    break;


    case WUM_SEARCHDRIVERS: {
    
        SP_DRVINFO_DATA DriverInfoData;

        NewDevWiz->CurrCursor = NULL;
        SetCursor(NewDevWiz->IdcArrow);

        if (NewDevWiz->ExitDetect) {
            
            break;
        }

        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        if (SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     &DriverInfoData
                                     ))
        {

            //
            // If this is update driver, we are supposed to determine what the
            // currently installed driver is and whether we have found a "better"
            // driver. The search thread will have selected the best driver.
            // If this is the same as the currently installed driver then we
            // haven't found a better driver.
            //

            SetDlgText(hDlg, IDC_DRVUPD_DRVMSG1, IDS_SEARCHING_RESULTS, IDS_SEARCHING_RESULTS);

            if (NewDevWiz->WizardType == NDWTYPE_UPDATE) {
                 
                //
                // Is this the currently installed driver?
                //
                if (IsInstalledDriver(NewDevWiz, &DriverInfoData)) {
            
                    //
                    // Is this an Old Internet driver?  We need to know this because we can NOT
                    // reinstall an Old Inetnet driver since we don't have the source files.
                    //
                    if (IsOldInternetDriver(NewDevWiz, &DriverInfoData)) {

                        NewDevWiz->DontReinstallCurrentDriver = TRUE;

                        SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_DRVUPD_CURRENT, IDS_DRVUPD_CURRENT);

                    } else {

                        SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_DRVUPD_BETTER, IDS_DRVUPD_BETTER);
                    }

                } else {
            
                    SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_DRVUPD_FOUND, IDS_DRVUPD_FOUND);
                }
            
            //
            // We must be in the Found New Hardware wizard
            //
            } else {

                SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_FOUNDNEW_FOUND, IDS_FOUNDNEW_FOUND);
            }


            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);


            //
            // If we found more then one driver, let the user choose from the list
            // by enabling the instll from list checkbox.
            //
            if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData,
                                      SPDIT_COMPATDRIVER,
                                      1,
                                      &DriverInfoData
                                      ))
            {
                ShowWindow(GetDlgItem(hDlg, IDC_LISTDRIVERS), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_LISTDRIVERS_TEXT), SW_SHOW);
            }

        } else {

            //
            // Disable the cancel button, its meaning is not clear.
            //

            EnableWindow(GetDlgItem(GetParent(hDlg), IDCANCEL), FALSE);

            ShowWindow(GetDlgItem(hDlg, IDC_SEARCHICON), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_SEARCHNAME), SW_HIDE);
            ShowWindow(GetDlgItem(hDlg, IDC_DRVUPD_DRVMSG1), SW_HIDE);

            if (NewDevWiz->WizardType == NDWTYPE_FOUNDNEW) {

                SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_FOUNDNEW_NOTFOUND, IDS_FOUNDNEW_NOTFOUND);

                //
                // Enable the radio buttons for cancelling the install, or finishing disabled
                // Set the Initial radio button state to finish disabled.
                //

                ShowWindow(GetDlgItem(hDlg, IDC_CANCELINSTALL), SW_SHOW);
                ShowWindow(GetDlgItem(hDlg, IDC_FINISHINSTALL), SW_SHOW);

                CheckRadioButton(hDlg,
                                 IDC_FINISHINSTALL,
                                 IDC_CANCELINSTALL,
                                 IDC_FINISHINSTALL
                                 );
                                 
            } else {
                SetDlgText(hDlg, IDC_DRVUPD_DRVMSG2, IDS_DRVUPD_NOTFOUND, IDS_DRVUPD_NOTFOUND);
            }

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_FINISH);
        }

        break;
    }


    case WM_SETCURSOR:
        if (NewDevWiz->CurrCursor) {

            SetCursor(NewDevWiz->CurrCursor);
            break;
        }

        // fall thru to return(FALSE);


    default:
        return FALSE;

    } // end of switch on message


    return TRUE;
}



void
FillDriversList(
    HWND hwndList,
    PNEWDEVWIZ NewDevWiz
    )
{
    int IndexDriver;
    int lvIndex;
    LV_ITEM lviItem;
    BOOL FoundInstalledDriver;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DRVINFO_DETAIL_DATA DriverInfoDetailData;
    SP_DRVINSTALL_PARAMS DriverInstallParams;


    SendMessage(hwndList, WM_SETREDRAW, FALSE, 0L);
    ListView_DeleteAllItems(hwndList);
    ListView_SetExtendedListViewStyle(hwndList, LVS_EX_LABELTIP | LVS_EX_FULLROWSELECT);

    IndexDriver = 0;
    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    DriverInfoDetailData.cbSize = sizeof(DriverInfoDetailData);

    FoundInstalledDriver = FALSE;
    while (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 SPDIT_COMPATDRIVER,
                                 IndexDriver,
                                 &DriverInfoData
                                 )) {

        //
        // Get the DriverInstallParams so we can see if we got this driver from the Internet
        //
        DriverInstallParams.cbSize = sizeof(SP_DRVINSTALL_PARAMS);
        if (SetupDiGetDriverInstallParams(NewDevWiz->hDeviceInfo,
                                          &NewDevWiz->DeviceInfoData,
                                          &DriverInfoData,
                                          &DriverInstallParams))  {

            //
            // Don't show old Internet drivers because we don't have the files locally
            // anymore to install these!  Also don't show BAD drivers.
            //
            if ((DriverInstallParams.Flags & DNF_OLD_INET_DRIVER) ||
                (DriverInstallParams.Flags & DNF_BAD_DRIVER)) {

                IndexDriver++;
                continue;
            }
                                 
            lviItem.mask = LVIF_TEXT | LVIF_PARAM;
            lviItem.iItem = IndexDriver;
            lviItem.iSubItem = 0;
            lviItem.pszText = DriverInfoData.Description;

            //
            // lParam == TRUE if this driver is the currently installed driver.
            //
            lviItem.lParam = !FoundInstalledDriver &&
                             NewDevWiz->WizardType ==  NDWTYPE_UPDATE &&
                             IsInstalledDriver(NewDevWiz, &DriverInfoData);

            lvIndex = ListView_InsertItem(hwndList, &lviItem);

            ListView_SetItemText(hwndList, lvIndex, 1, DriverInfoData.ProviderName);
            ListView_SetItemText(hwndList, lvIndex, 2, DriverInfoData.MfgName);


            if (DriverInstallParams.Flags & DNF_INET_DRIVER) {

                //
                // Driver is from the Internet
                //
                TCHAR WindowsUpdate[MAX_PATH];
                LoadString(hNewDev, IDS_DEFAULT_INTERNET_HOST, WindowsUpdate, SIZECHARS(WindowsUpdate));
                ListView_SetItemText(hwndList, lvIndex, 3, WindowsUpdate);

            } else {           

                //
                // Driver is not from the Internet
                //
                if (SetupDiGetDriverInfoDetail(NewDevWiz->hDeviceInfo,
                                               &NewDevWiz->DeviceInfoData,
                                               &DriverInfoData,
                                               &DriverInfoDetailData,
                                               sizeof(DriverInfoDetailData),
                                               NULL
                                               )
                    ||
                    GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
                    
                    ListView_SetItemText(hwndList, lvIndex, 3, DriverInfoDetailData.InfFileName);

                } else {
                    ListView_SetItemText(hwndList, lvIndex, 3, TEXT(""));
                }
            }            
        }

        IndexDriver++;
    }



    //
    // select the first item in the list, and scroll it into view
    // since this is the highest ranking driver.
    //
    ListView_SetItemState(hwndList,
                          0,
                          LVIS_SELECTED|LVIS_FOCUSED,
                          LVIS_SELECTED|LVIS_FOCUSED
                          );

    ListView_EnsureVisible(hwndList, 0, FALSE);
    ListView_SetColumnWidth(hwndList, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hwndList, 1, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hwndList, 2, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hwndList, 3, LVSCW_AUTOSIZE_USEHEADER);

    SendMessage(hwndList, WM_SETREDRAW, TRUE, 0L);
}



void
SelectDriverFromList(
    HWND hwndList,
    PNEWDEVWIZ NewDevWiz
    )
{
    int lvSelected;
    SP_DRVINFO_DATA DriverInfoData;

    DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
    lvSelected = ListView_GetNextItem(hwndList,
                                      -1,
                                      LVNI_SELECTED
                                      );

    if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                              &NewDevWiz->DeviceInfoData,
                              SPDIT_COMPATDRIVER,
                              lvSelected,
                              &DriverInfoData
                              ))
    {
        SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                 &NewDevWiz->DeviceInfoData,
                                 &DriverInfoData
                                 );
    }

    //
    // if there is no selected driver call DIF_SELECTBESTCOMPATDRV.
    //

    if (!SetupDiGetSelectedDriver(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  &DriverInfoData
                                  ))
    {
        if (SetupDiEnumDriverInfo(NewDevWiz->hDeviceInfo,
                                  &NewDevWiz->DeviceInfoData,
                                  SPDIT_COMPATDRIVER,
                                  0,
                                  &DriverInfoData
                                  ))
        {
            //
            //Pick the best driver from the list we just created
            //
            SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                      NewDevWiz->hDeviceInfo,
                                      &NewDevWiz->DeviceInfoData
                                      );
        }

        else 
        {
            SetupDiSetSelectedDriver(NewDevWiz->hDeviceInfo,
                                     &NewDevWiz->DeviceInfoData,
                                     NULL
                                     );
        }
    }

    return;
}






INT_PTR CALLBACK
ListDriversDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam, 
    LPARAM lParam
    )
{
    PNEWDEVWIZ NewDevWiz = (PNEWDEVWIZ)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (message)  {
        
    case WM_INITDIALOG: {
            
        HWND hwndParentDlg;
        HWND hwndList;
        LV_COLUMN lvcCol;
        LPPROPSHEETPAGE lppsp = (LPPROPSHEETPAGE)lParam;
        TCHAR Buffer[64];

        NewDevWiz = (PNEWDEVWIZ)lppsp->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LONG_PTR)NewDevWiz);

        //
        // Insert columns for listview.
        // 0 == device name
        // 1 == location information
        //
        hwndList = GetDlgItem(hDlg, IDC_LISTDRIVERS_LISTVIEW);

        lvcCol.mask = LVCF_FMT | LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
        lvcCol.fmt = LVCFMT_LEFT;
        lvcCol.pszText = Buffer;

        lvcCol.iSubItem = 0;
        LoadString(hNewDev, IDS_DRIVERDESC, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 0, &lvcCol);

        lvcCol.iSubItem = 1;
        LoadString(hNewDev, IDS_DRIVERPROVIDER, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 1, &lvcCol);

        lvcCol.iSubItem = 2;
        LoadString(hNewDev, IDS_DRIVERMFG, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 2, &lvcCol);

        lvcCol.iSubItem = 3;
        LoadString(hNewDev, IDS_DRIVERINF, Buffer, SIZECHARS(Buffer));
        ListView_InsertColumn(hwndList, 3, &lvcCol);


        SendMessage(hwndList,
                    LVM_SETEXTENDEDLISTVIEWSTYLE,
                    LVS_EX_FULLROWSELECT,
                    LVS_EX_FULLROWSELECT
                    );
    }
    break;


    case WM_COMMAND:
        break;

    case WM_DESTROY:
        break;


    case WM_NOTIFY:

        switch (((NMHDR FAR *)lParam)->code) {
           
        case PSN_SETACTIVE: {
              
            int PrevPage;
            HICON hicon;

            PrevPage = NewDevWiz->PrevPage;
            NewDevWiz->PrevPage = IDD_LISTDRIVERS;

            SetDriverDescription(hDlg, IDC_DRVUPD_DRVDESC, NewDevWiz);

            hicon = NULL;
            if (NewDevWiz->ClassGuidSelected &&
                SetupDiLoadClassIcon(NewDevWiz->ClassGuidSelected, &hicon, NULL))
            {
                hicon = (HICON)SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);

            } else {

                SetupDiLoadClassIcon(&GUID_DEVCLASS_UNKNOWN, &hicon, NULL);
                SendDlgItemMessage(hDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);
            }

            if (hicon) {

                DestroyIcon(hicon);
            }

            // fill the list view
            FillDriversList(GetDlgItem(hDlg, IDC_LISTDRIVERS_LISTVIEW),
                            NewDevWiz
                            );

        }
        break;

        case PSN_RESET:
            NewDevWiz->Cancelled = TRUE;
            break;

        case PSN_WIZBACK:
            SetDlgMsgResult(hDlg, message, IDD_DRVUPD_SEARCHING);
            break;

        case PSN_WIZNEXT:

            //
            // The user selected another driver.
            //
            NewDevWiz->DontReinstallCurrentDriver = FALSE;

            SelectDriverFromList(GetDlgItem(hDlg, IDC_LISTDRIVERS_LISTVIEW),
                                 NewDevWiz
                                 );

            NewDevWiz->EnterInto = IDD_NEWDEVWIZ_INSTALLDEV;
            NewDevWiz->EnterFrom = IDD_LISTDRIVERS;
            SetDlgMsgResult(hDlg, message, IDD_NEWDEVWIZ_INSTALLDEV);
            break;

        case LVN_ITEMCHANGED: {

            LPNM_LISTVIEW   lpnmlv = (LPNM_LISTVIEW)lParam;
            int StringId;

            if ((lpnmlv->uChanged & LVIF_STATE)) {


                StringId = 0;
                if (lpnmlv->uNewState & LVIS_SELECTED) {

                    //
                    // lParam == TRUE means current driver
                    //
                    // iItem == 0 means Best driver (BUGBUG: this is not always TRUE
                    // should use GetSelectedDriver!)
                    //
                    if (lpnmlv->lParam) {

                        if (lpnmlv->iItem == 0) {

                            StringId = IDS_DRIVER_BESTCURR;

                        } else {

                            StringId = IDS_DRIVER_CURR;
                        }

                    } else if (lpnmlv->iItem == 0) {

                        StringId = IDS_DRIVER_BEST;
                    }

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK | PSWIZB_NEXT);

                } else {

                    PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_BACK);
                }

                if (!StringId) {

                    SetDlgItemText(hDlg, IDC_NDW_TEXT, TEXT(""));

                } else {

                    SetDlgText(hDlg, IDC_NDW_TEXT, StringId, StringId);
                }

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
