//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       newdev.c
//
//--------------------------------------------------------------------------

#include "newdevp.h"
#include <regstr.h>

#include <initguid.h>

//
// Define and initialize all device class GUIDs.
// (This must only be done once per module!)
//
#include <devguid.h>

//
// Define and initialize a global variable, GUID_NULL
// (from coguid.h)
//
DEFINE_GUID(GUID_NULL, 0L, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

typedef
BOOL
(*PINSTALLNEWDEVICE)(
    HWND hwndParent,
    LPGUID ClassGuid,
    PDWORD pReboot
    );

PINSTALLNEWDEVICE pInstallNewDevice = NULL;


typedef struct _NewDevWizPropertySheet {
    PROPSHEETHEADER   PropSheetHeader;
    HPROPSHEETPAGE    PropSheetPages[16];
} NDWPROPERTYSHEET, *PNDWPROPERTYSHEET;


HWND  g_hWndNewDevice = NULL;
ULONG g_hTaskNewDevice = 0;


INT CALLBACK
iNDWDlgCallback(
    IN HWND             hwndDlg,
    IN UINT             uMsg,
    IN LPARAM           lParam
    )
/*++

Routine Description:

    Call back used to remove the "?" from the wizard page.

Arguments:

    hwndDlg - Handle to the property sheet dialog box.

    uMsg - Identifies the message being received. This parameter
            is one of the following values:

            PSCB_INITIALIZED - Indicates that the property sheet is
            being initialized. The lParam value is zero for this message.

            PSCB_PRECREATE      Indicates that the property sheet is about
            to be created. The hwndDlg parameter is NULL and the lParam
            parameter is a pointer to a dialog template in memory. This
            template is in the form of a DLGTEMPLATE structure followed
            by one or more DLGITEMTEMPLATE structures.

    lParam - Specifies additional information about the message. The
            meaning of this value depends on the uMsg parameter.

Return Value:

    The function returns zero.

--*/
{

    switch( uMsg ) {

    case PSCB_INITIALIZED:
        break;

    case PSCB_PRECREATE:
        if( lParam ){
        
            //
            // This is done to hide the X and ? at the top of the wizard
            //
            DLGTEMPLATE *pDlgTemplate = (DLGTEMPLATE *)lParam;
            pDlgTemplate->style &= ~(DS_CONTEXTHELP | WS_SYSMENU);
        }
        break;
    }

    return FALSE;
}


PNDWPROPERTYSHEET
InitNDWPropSheet(
   HWND            hwndParent,
   PNEWDEVWIZ      NewDevWiz,
   int             StartPageId
   )
{
    PSP_INSTALLWIZARD_DATA InstallWizard;
    PNDWPROPERTYSHEET NdwPropertySheet;
    LPPROPSHEETHEADER PropSheetHeader;
    PROPSHEETPAGE    psp;
    LPTSTR Title;
    LONG Error;
    HKEY hKeyDeviceInstaller;
    DWORD RegCreated, Type, cbData;

    //
    // Allocate memory for the header and the page array.
    //

    NdwPropertySheet = LocalAlloc(LPTR, sizeof(NDWPROPERTYSHEET));
    
    if (!NdwPropertySheet) {

        NewDevWiz->LastError = ERROR_NOT_ENOUGH_MEMORY;
        return NULL;
    }

    memset(NdwPropertySheet, 0, sizeof(*NdwPropertySheet));

    NewDevWiz->LastError = NdwBuildClassInfoList(NewDevWiz, DIBCI_NOINSTALLCLASS);
    
    if (NewDevWiz->LastError != ERROR_SUCCESS) {

        return NULL;
    }

    //
    // Retrieve any saved information
    //
    if (RegCreateKeyEx(HKEY_CURRENT_USER,
                       REGSTR_PATH_DEVICEINSTALLER,
                       0,
                       NULL,
                       REG_OPTION_NON_VOLATILE,
                       KEY_READ,
                       NULL,
                       &hKeyDeviceInstaller,
                       &RegCreated) == ERROR_SUCCESS) {

        DWORD Error;
        
        //
        // Get the saved search options (locations to search)
        //
        cbData = sizeof(NewDevWiz->SearchOptions);
        
        if (RegQueryValueEx(hKeyDeviceInstaller,
                            REGSTR_VAL_SEARCHOPTIONS,
                            NULL,
                            &Type,
                            (LPBYTE)&(NewDevWiz->SearchOptions),
                            &cbData
                            ) != ERROR_SUCCESS) {

            NewDevWiz->SearchOptions = SEARCH_DEFAULT | SEARCH_FLOPPY | SEARCH_CDROM;
        }                           

        RegCloseKey(hKeyDeviceInstaller);
    }                                   

    //
    // Initialize the PropertySheet Header
    //
    PropSheetHeader = &(NdwPropertySheet->PropSheetHeader);
    PropSheetHeader->dwSize = sizeof(NdwPropertySheet->PropSheetHeader);
    PropSheetHeader->dwFlags = PSH_WIZARD | PSH_USECALLBACK | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;
    PropSheetHeader->pszbmWatermark = MAKEINTRESOURCE(IDB_WATERBMP);
    PropSheetHeader->pszbmHeader = MAKEINTRESOURCE(IDB_BANNERBMP);
    PropSheetHeader->hwndParent = hwndParent;
    PropSheetHeader->hInstance = hNewDev;
    PropSheetHeader->pfnCallback = iNDWDlgCallback;

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

    PropSheetHeader->pszCaption = Title;
    PropSheetHeader->phpage = NdwPropertySheet->PropSheetPages;
    PropSheetHeader->nStartPage = 0;

    PropSheetHeader->nPages = 0;
    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.hInstance = hNewDev;
    psp.lParam = (LPARAM)NewDevWiz;
    psp.pszTitle = Title;

    //
    // The install wizards are always spawned from some other disjoint UI,
    // (HdwWiz, devmgr, cpl applets etc. and so don't have a proper intro
    // page.
    //

    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

    if (StartPageId == IDD_NEWDEVWIZ_INSTALLDEV) {
        //
        // Found New Hardware, with a rank Zero match.
        // jump straight into install page.
        //
        ;

    }

    else {

        //
        // Update driver, or found new hardware without rank Zero driver
        //
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_INTRO);
        psp.pfnDlgProc = IntroDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRVUPD);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRVUPD_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DRVUPD);
        psp.pfnDlgProc = DriverUpdateDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRVUPD_SEARCH);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRVUPD_SEARCH_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DRVUPD_SEARCH);
        psp.pfnDlgProc = DriverSearchDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_DRVUPD_SEARCHING);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_DRVUPD_SEARCHING_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_DRVUPD_SEARCHING);
        psp.pfnDlgProc = DriverSearchingDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_LISTDRIVERS);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_LISTDRIVERS_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_LISTDRIVERS);
        psp.pfnDlgProc = ListDriversDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_SELECTCLASS);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_SELECTCLASS_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_SELECTCLASS);
        psp.pfnDlgProc = NDW_PickClassDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_SELECTDEVICE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_SELECTDEVICE_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_SELECTDEVICE);
        psp.pfnDlgProc = NDW_SelectDeviceDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_ANALYZEDEV);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_ANALYZEDEV_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_ANALYZEDEV);
        psp.pfnDlgProc = NDW_AnalyzeDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    }


    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_INSTALLDEV);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NEWDEVWIZ_INSTALLDEV_INFO);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_INSTALLDEV);
    psp.pfnDlgProc = NDW_InstallDevDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_NEWDEVWIZ_FINISH);
    psp.pfnDlgProc = NDW_FinishDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    //
    // Get the Class Icon Image Lists.
    //
    NewDevWiz->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    
    if (!SetupDiGetClassImageList(&NewDevWiz->ClassImageList)) {

        NewDevWiz->ClassImageList.cbSize = 0;
    }

    NewDevWiz->CurrCursor = NULL;
    NewDevWiz->IdcWait        = LoadCursor(NULL, IDC_WAIT);
    NewDevWiz->IdcAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
    NewDevWiz->IdcArrow = LoadCursor(NULL, IDC_ARROW);

    return NdwPropertySheet;
}




//
// Used by "Found New Hardware" when a rank zero compatible driver is found
// from the default inf location.
//

BOOL
InstallDeviceWizard(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz
    )
{
    int  PropSheetResult;
    PNDWPROPERTYSHEET NdwPropertySheet;

    NdwPropertySheet = InitNDWPropSheet(hWnd, NewDevWiz, IDD_NEWDEVWIZ_INSTALLDEV);
   
    if (NdwPropertySheet) {
       
        PropSheetResult = (int)PropertySheet(&NdwPropertySheet->PropSheetHeader);

        LocalFree(NdwPropertySheet);
    }

    //
    // Final cleanup of DeviceInfoData and DeviceInfoList.
    //
    if (NewDevWiz->ClassGuidList) {
       
        LocalFree(NewDevWiz->ClassGuidList);
        NewDevWiz->ClassGuidList = NULL;
        NewDevWiz->ClassGuidSize = NewDevWiz->ClassGuidNum = 0;
    }

    // destroy the ClassImageList
    if (NewDevWiz->ClassImageList.cbSize) {
       
        SetupDiDestroyClassImageList(&NewDevWiz->ClassImageList);
        NewDevWiz->ClassImageList.cbSize = 0;
    }

    return NewDevWiz->LastError == ERROR_SUCCESS;
}



//
// Used by "Update Driver", or "Found New Hardware" when there are no rank zero
// compatible drivers.
//

BOOL
UpdateDeviceWizard(
    HWND hWnd,
    PNEWDEVWIZ NewDevWiz
    )
{
    int  PropSheetResult;
    PNDWPROPERTYSHEET NdwPropertySheet;


    NdwPropertySheet = InitNDWPropSheet(hWnd, NewDevWiz, IDD_DRVUPD);
   
    if (NdwPropertySheet) {
       
        PropSheetResult = (int)PropertySheet(&NdwPropertySheet->PropSheetHeader);

        if (0 == PropSheetResult) {

            NewDevWiz->LastError = ERROR_CANCELLED;
        }

        LocalFree(NdwPropertySheet);
    }


    //
    // Final cleanup of DeviceInfoData and DeviceInfoList.
    //

    if (NewDevWiz->ClassGuidList) {
       
        LocalFree(NewDevWiz->ClassGuidList);
        NewDevWiz->ClassGuidList = NULL;
        NewDevWiz->ClassGuidSize = NewDevWiz->ClassGuidNum = 0;
    }

    // destroy the ClassImageList
    if (NewDevWiz->ClassImageList.cbSize) {
       
        SetupDiDestroyClassImageList(&NewDevWiz->ClassImageList);
        NewDevWiz->ClassImageList.cbSize = 0;
    }

    return NewDevWiz->LastError == ERROR_SUCCESS;
}

BOOL
InstallSelectedDriver(
   HWND hwndParent,
   HDEVINFO hDeviceInfo,
   LPCWSTR DisplayName,
   BOOL Backup,
   PDWORD pReboot
   )
/*++

Routine Description:

   Installs the selected driver on the selected device in the hDeviceInfo.

Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   HDEVINFO hDeviceInfo - DeviceInfoList which supplies the selected device to install the
                          selected driver on.
                          
   DisplayName - Friendly backup string
   
   Backup - BOOL that indicates whether or not we should back up the current drivers before
            installing the new ones.                          

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)


Return Value:

    BOOL    TRUE if the driver was installed
            FALSE if the driver was not installed.  Check GetLastError() to see if the specific
                  error.

--*/
{
    NEWDEVWIZ  NewDevWiz;
    SP_DRVINFO_DATA DriverInfoData;
    UPDATEDRIVERINFO UpdateDriverInfo;

    memset(&NewDevWiz, 0, sizeof(NewDevWiz));
    NewDevWiz.WizardType = NDWTYPE_UPDATE;
    NewDevWiz.hDeviceInfo = hDeviceInfo;

    NewDevWiz.PromptForReboot = pReboot == NULL;


    try {

        NewDevWiz.DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        if (!SetupDiGetSelectedDevice(NewDevWiz.hDeviceInfo,
                                      &NewDevWiz.DeviceInfoData
                                      ))
        {
            NewDevWiz.LastError = GetLastError();
            goto INDLeaveExcept;
        }

        NewDevWiz.ClassGuidSelected = &NewDevWiz.DeviceInfoData.ClassGuid;

        if (!SetupDiGetClassDescription(NewDevWiz.ClassGuidSelected,
                                        NewDevWiz.ClassDescription,
                                        sizeof(NewDevWiz.ClassDescription)/sizeof(TCHAR),
                                        NULL
                                        )
            ||
            !SetupDiClassNameFromGuid(NewDevWiz.ClassGuidSelected,
                                       NewDevWiz.ClassName,
                                       sizeof(NewDevWiz.ClassName)/sizeof(TCHAR),
                                       NULL
                                       ))
        {
            NewDevWiz.LastError = GetLastError();
            goto INDLeaveExcept;
        }

        if (Backup) {

            UpdateDriverInfo.DisplayName = DisplayName;
            UpdateDriverInfo.Backup = TRUE;
            UpdateDriverInfo.FromInternet = TRUE;

            NewDevWiz.UpdateDriverInfo = &UpdateDriverInfo;

        } else {

            NewDevWiz.UpdateDriverInfo = NULL;
        }

        //
        // Do the install quietly since we may have a batch of installs to do,
        // only showing UI when really needed.        
        //
        NewDevWiz.SilentMode = TRUE;

        InstallDeviceWizard(hwndParent, &NewDevWiz);

        //
        // copy out the reboot flags for the caller
        //

        *pReboot = NewDevWiz.Reboot;

INDLeaveExcept:;

    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
        
          NewDevWiz.LastError = RtlNtStatusToDosError(GetExceptionCode());
    }

    SetLastError(NewDevWiz.LastError);
    
    return NewDevWiz.LastError == ERROR_SUCCESS;
}

BOOL
InstallSelectedDevice(
   HWND hwndParent,
   HDEVINFO hDeviceInfo,
   PDWORD pReboot
   )
/*++

Routine Description:

   Installs the selected device in the hDeviceInfo.

Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   HDEVINFO hDeviceInfo - DeviceInfoList which supplies the selected device to install.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)


Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/
{
    BOOL DriversFound;
    int PropSheetResult;
    NEWDEVWIZ  NewDevWiz;
    PNDWPROPERTYSHEET NdwPropertySheet;
    SEARCHTHREAD SearchThread;
    SP_DRVINFO_DATA DriverInfoData;
    SP_DEVINSTALL_PARAMS  DeviceInstallParams;

    memset(&NewDevWiz, 0, sizeof(NewDevWiz));
    NewDevWiz.WizardType = NDWTYPE_FOUNDNEW;
    NewDevWiz.hDeviceInfo = hDeviceInfo;

    NewDevWiz.PromptForReboot = pReboot == NULL;


    try {

        NewDevWiz.DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
        
        if (!SetupDiGetSelectedDevice(NewDevWiz.hDeviceInfo,
                                  &NewDevWiz.DeviceInfoData
                                  ))
        {
            NewDevWiz.LastError = GetLastError();
            goto INDLeaveExcept;
        }


        NewDevWiz.ClassGuidSelected = &NewDevWiz.DeviceInfoData.ClassGuid;

        if (!SetupDiGetClassDescription(NewDevWiz.ClassGuidSelected,
                                       NewDevWiz.ClassDescription,
                                       sizeof(NewDevWiz.ClassDescription)/sizeof(TCHAR),
                                       NULL
                                       )
            ||
            !SetupDiClassNameFromGuid(NewDevWiz.ClassGuidSelected,
                                      NewDevWiz.ClassName,
                                      sizeof(NewDevWiz.ClassName)/sizeof(TCHAR),
                                      NULL
                                      ))
         {
            NewDevWiz.LastError = GetLastError();
            goto INDLeaveExcept;
        }

        memset(&SearchThread, 0, sizeof(SEARCHTHREAD));
        NewDevWiz.SearchThread = &SearchThread;
        NewDevWiz.LastError =  CreateSearchThread(&NewDevWiz);
        
        if (NewDevWiz.LastError != ERROR_SUCCESS) {
            
            goto INDLeaveExcept;
        }


        //
        // Do the install quietly since we may have a batch of installs to do,
        // only showing UI when really needed. During legacy detect the
        // detect summary page is showing.
        //

        NewDevWiz.SilentMode = TRUE;
        DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    
        if (SetupDiGetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &DeviceInstallParams
                                          ))
       {
            DeviceInstallParams.Flags |= DI_SHOWOEM | DI_QUIETINSTALL;
            DeviceInstallParams.hwndParent = hwndParent;
            wcscpy(DeviceInstallParams.DriverPath, L"");
            SetupDiSetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &DeviceInstallParams
                                          );
        }



        //
        // If no driver list search the win inf default locations
        // If we still can't find a driver, then start at the driver
        // search page.
        //
        // otherwise go straight to the finish page and install.
        // To preserve drivers preselected by the caller (legacy detect)
        // the currently SelectedDriver is used, but if there is no selected
        // driver the highest ranking  driver is used.
        //

        DriverInfoData.cbSize = sizeof(SP_DRVINFO_DATA);
        DriversFound = SetupDiEnumDriverInfo(NewDevWiz.hDeviceInfo,
                                            &NewDevWiz.DeviceInfoData,
                                            SPDIT_COMPATDRIVER,
                                            0,
                                            &DriverInfoData
                                            );

        if (!DriversFound) {
        
            SetupDiDestroyDriverInfoList(NewDevWiz.hDeviceInfo,
                                         &NewDevWiz.DeviceInfoData,
                                         SPDIT_COMPATDRIVER
                                         );

            if (SetupDiGetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                              &NewDevWiz.DeviceInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                wcscpy(DeviceInstallParams.DriverPath, L"");
                SetupDiSetDeviceInstallParams(NewDevWiz.hDeviceInfo,
                                              &NewDevWiz.DeviceInfoData,
                                              &DeviceInstallParams
                                              );
            }



            if (SetupDiBuildDriverInfoList(NewDevWiz.hDeviceInfo,
                                           &NewDevWiz.DeviceInfoData,
                                           SPDIT_COMPATDRIVER
                                           ))
            {
                SetupDiCallClassInstaller(DIF_SELECTBESTCOMPATDRV,
                                          NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData
                                          );
            }



            DriversFound = SetupDiEnumDriverInfo(NewDevWiz.hDeviceInfo,
                                                &NewDevWiz.DeviceInfoData,
                                                SPDIT_COMPATDRIVER,
                                                0,
                                                &DriverInfoData
                                                );
        }

        if (DriversFound) {
        
            SP_DRVINFO_DATA SelectedDriverInfo;

            SelectedDriverInfo.cbSize = sizeof(SP_DRVINFO_DATA);
        
            if (!SetupDiGetSelectedDriver(NewDevWiz.hDeviceInfo,
                                          &NewDevWiz.DeviceInfoData,
                                          &SelectedDriverInfo
                                          ))
            {
                SetupDiSetSelectedDriver(NewDevWiz.hDeviceInfo,
                                         &NewDevWiz.DeviceInfoData,
                                         &DriverInfoData
                                         );
            }

            InstallDeviceWizard(hwndParent, &NewDevWiz);
        }
    
        else {
        
            UpdateDeviceWizard(hwndParent, &NewDevWiz);
        }

        //
        // copy out the reboot flags for the caller
        //

        *pReboot = NewDevWiz.Reboot;

INDLeaveExcept:;

    } except(NdwUnhandledExceptionFilter(GetExceptionInformation())) {
        
          NewDevWiz.LastError = RtlNtStatusToDosError(GetExceptionCode());
    }

    if (NewDevWiz.SearchThread) {
        
        DestroySearchThread(&SearchThread);
    }

    SetLastError(NewDevWiz.LastError);
    
    return NewDevWiz.LastError == ERROR_SUCCESS;
}




BOOL
InstallNewDevice(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD pReboot
   )
/*++

Routine Description:

   Exported Entry point from newdev.dll. Installs a new device. A new Devnode is
   created and the user is prompted to select the device. If the class guid
   is not specified then then the user begins at class selection.
   
   This function has been moved to hdwwiz.cpl (which handles all legacy device
   functions now).  This entry point just forwards the function call onto hdwwiz.cpl
   now.

Arguments:

   hwndParent - Window handle of the top-level window to use for any UI related
                to installing the device.

   LPGUID ClassGuid - Optional class of the new device to install.
                      If ClassGuid is NULL we start at detection choice page.
                      If ClassGuid == GUID_NULL or GUID_DEVCLASS_UNKNOWN
                         we start at class selection page.

   pReboot - Optional address of variable to receive reboot flags (DI_NEEDRESTART,DI_NEEDREBOOT)


Return Value:

   BOOL TRUE for success (does not mean device was installed or updated),
        FALSE unexpected error. GetLastError returns the winerror code.

--*/
{
    HMODULE hHdwWiz;
    BOOL Return = FALSE;

    hHdwWiz = LoadLibrary(TEXT("hdwwiz.cpl"));

    if (NULL == hHdwWiz) {

        return FALSE;
    }

    if (NULL == pInstallNewDevice) {

        pInstallNewDevice = (PINSTALLNEWDEVICE)GetProcAddress(hHdwWiz, "InstallNewDevice");
    }

    Return = (pInstallNewDevice)(hwndParent, ClassGuid, pReboot);

    FreeLibrary(hHdwWiz);

    return Return;
}
