//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       init.c
//
//--------------------------------------------------------------------------

#include "hdwwiz.h"

HMODULE hHdwWiz;

typedef
int
(*PDEVICEPROBLEMWIZARD)(
   HWND hwndParent,
   PTCHAR MachineName,
   PTCHAR DeviceID
   );

PDEVICEPROBLEMWIZARD pDeviceProblemWizard = NULL;

TASKINITDATA
TaskInitDataList[]= {
   {IDC_HWTASK_TROUBLE,IDS_HWTASK_TROUBLE,TEXT("devmgr.dll"), "DeviceProblemWizardW", DevProbInvokeTask},
   };



PTASKINITDATA
FindTaskInitData(
    INT idTask
    )
{
    return  &TaskInitDataList[idTask - IDC_HWTASK_TROUBLE];
}




INT CALLBACK
iHdwWizardDlgCallback(
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
        
            DLGTEMPLATE *pDlgTemplate = (DLGTEMPLATE *)lParam;
            pDlgTemplate->style &= ~(DS_CONTEXTHELP | WS_SYSMENU);
        }
        break;
    }

    return FALSE;
}


PHDWPROPERTYSHEET
HdwWizard(
    HWND hwndParent,
    PHARDWAREWIZ HardwareWiz,
    int StartPageId
    )
{
    PHDWPROPERTYSHEET HdwPropertySheet;
    LPPROPSHEETHEADER PropSheetHeader;
    PROPSHEETPAGE psp;
    LPTSTR Title;
    LONG Error;
    LONG DialogBaseUnits;
    int Index;


    //
    // Allocate memory for the header and the page array.
    //

    HdwPropertySheet = LocalAlloc(LPTR, sizeof(HDWPROPERTYSHEET));
    if (!HdwPropertySheet) {
    
        return NULL;
    }

    memset(HdwPropertySheet, 0, sizeof(*HdwPropertySheet));

    if (ERROR_SUCCESS != HdwBuildClassInfoList(HardwareWiz, 
                                               DIBCI_NOINSTALLCLASS,
                                               HardwareWiz->hMachine ? HardwareWiz->MachineName : NULL)) {

        return NULL;
    }

    //
    // Initialize the PropertySheet Header
    //
    PropSheetHeader = &HdwPropertySheet->PropSheetHeader;
    PropSheetHeader->dwSize = sizeof(HdwPropertySheet->PropSheetHeader);
    PropSheetHeader->dwFlags = PSH_WIZARD | PSH_USECALLBACK | PSH_WIZARD97 | PSH_WATERMARK | PSH_STRETCHWATERMARK | PSH_HEADER;

    PropSheetHeader->hwndParent = hwndParent;
    PropSheetHeader->hInstance = hHdwWiz;
    PropSheetHeader->pszCaption = MAKEINTRESOURCE(IDS_HDWWIZNAME);
    PropSheetHeader->phpage = HdwPropertySheet->PropSheetPages;
    PropSheetHeader->pszbmWatermark = MAKEINTRESOURCE(IDB_WATERMARK);
    PropSheetHeader->pszbmHeader = MAKEINTRESOURCE(IDB_BANNER);
    PropSheetHeader->pfnCallback = iHdwWizardDlgCallback;

    PropSheetHeader->nStartPage = 0;
    PropSheetHeader->nPages = 0;

    psp.dwSize = sizeof(PROPSHEETPAGE);
    psp.hInstance = hHdwWiz;
    psp.lParam = (LPARAM)HardwareWiz;
    psp.pszTitle = MAKEINTRESOURCE(IDS_HDWWIZNAME);


    //
    // If the StartPageId is IDD_INSTALLNEWDEVICE then we don't need to create the detection
    // and removal pages.
    //
    if (IDD_INSTALLNEWDEVICE == StartPageId) {
        
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_INSTALLNEWDEVICE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_INSTALLNEWDEVICE_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_INSTALLNEWDEVICE);
        psp.pfnDlgProc = InstallNewDeviceDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    }

    else {
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_HDWINTRO);
        psp.pfnDlgProc = HdwIntroDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;

        //
        // Don't show the Intro choice page if we are jumping directly to enumeration
        //
        if (IDD_ADDDEVICE_PNPENUM != StartPageId) {
        
            psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_HDWWIZTASKS);
            psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_HDWWIZTASKSINFO);
            psp.pszTemplate = MAKEINTRESOURCE(IDD_HDWTASKS);
            psp.pfnDlgProc = HdwTasksDlgProc;
            PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
        }
    
        //
        // Add New Hardware wizard pages
        //
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_PNPENUM);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_PNPENUM_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_PNPENUM);
        psp.pfnDlgProc = HdwPnpEnumDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        //
        // Finish page
        //
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_PNPFINISH);
        psp.pfnDlgProc = HdwPnpFinishDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_PROBLIST);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_PROBLIST_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_PROBLIST);
        psp.pfnDlgProc = HdwProbListDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_PROBLIST_FINISH);
        psp.pfnDlgProc = HdwProbListFinishDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_ASKDETECT);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_ASKDETECT_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_ASKDETECT);
        psp.pfnDlgProc = HdwAskDetectDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_DETECTION);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_DETECTION_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_DETECTION);
        psp.pfnDlgProc = HdwDetectionDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_DETECTINSTALL);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_DETECTINSTALL_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_DETECTINSTALL);
        psp.pfnDlgProc = HdwDetectInstallDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        //
        // Finish page
        //
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_DETECTREBOOT);
        psp.pfnDlgProc = HdwDetectRebootDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    }

    //
    // These pages are always shown
    //
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_SELECTCLASS);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_SELECTCLASS_INFO);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_SELECTCLASS);
    psp.pfnDlgProc = HdwPickClassDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_SELECTDEVICE);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_SELECTDEVICE_INFO);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_SELECTDEVICE);
    psp.pfnDlgProc = HdwSelectDeviceDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_ANALYZEDEV);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_ANALYZEDEV_INFO);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_ANALYZEDEV);
    psp.pfnDlgProc = HdwAnalyzeDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
    psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_INSTALLDEV);
    psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_ADDDEVICE_INSTALLDEV_INFO);
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_INSTALLDEV);
    psp.pfnDlgProc = HdwInstallDevDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    //
    // Finish page
    //
    psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
    psp.pszTemplate = MAKEINTRESOURCE(IDD_ADDDEVICE_FINISH);
    psp.pfnDlgProc = HdwAddDeviceFinishDlgProc;
    PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);

    //
    // Uninstall/Unplug wizard pages.  These are not shown if we are jumping directly
    // to the InstallDevice page or the enumeration page.
    //
    if ((IDD_INSTALLNEWDEVICE != StartPageId) &&
        (IDD_ADDDEVICE_PNPENUM != StartPageId)) {

        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_REMDEVICE_CHOICE);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_REMDEVICE_CHOICE_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_REMDEVICE_CHOICE);
        psp.pfnDlgProc = HdwRemDeviceChoiceDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_REMDEVICE_UNINSTALLLIST);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_REMDEVICE_UNINSTALLLIST_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_REMDEVICE_UNINSTALLLIST);
        psp.pfnDlgProc = HdwUninstallListDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        psp.pszHeaderTitle = MAKEINTRESOURCE(IDS_REMDEVICE_UNINSTALL_CONFIRM);
        psp.pszHeaderSubTitle = MAKEINTRESOURCE(IDS_REMDEVICE_UNINSTALL_CONFIRM_INFO);
        psp.pszTemplate = MAKEINTRESOURCE(IDD_REMDEVICE_UNINSTALL_CONFIRM);
        psp.pfnDlgProc = HdwRemDeviceUninstallConfirmDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    
        //
        // Finish page
        //
        psp.dwFlags = PSP_DEFAULT | PSP_USETITLE | PSP_HIDEHEADER;
        psp.pszTemplate = MAKEINTRESOURCE(IDD_REMDEVICE_UNINSTALL_FINISH);
        psp.pfnDlgProc = HdwRemDeviceUninstallFinishDlgProc;
        PropSheetHeader->phpage[PropSheetHeader->nPages++] = CreatePropertySheetPage(&psp);
    }

    //
    // check for failure on CreatePropertySheetPage.
    //
    Index = PropSheetHeader->nPages;
    while (Index--) {
    
       if (!PropSheetHeader->phpage[Index]) {
       
           break;
       }
   }

    if (Index >= 0) {
    
        Index = PropSheetHeader->nPages;
        while (Index--) {
        
            if (PropSheetHeader->phpage[Index]) {
           
                DestroyPropertySheetPage(PropSheetHeader->phpage[Index]);
            }
        }

        LocalFree(HdwPropertySheet);
        return NULL;
    }

    HardwareWiz->PrevPage = 0;

    LoadString(hHdwWiz,
               IDS_UNKNOWN,
               (PTCHAR)szUnknown,
               SIZECHARS(szUnknown)
               );

    LenUnknown = lstrlen(szUnknown) * sizeof(TCHAR) + sizeof(TCHAR);


    LoadString(hHdwWiz,
               IDS_UNKNOWNDEVICE,
               (PTCHAR)szUnknownDevice,
               SIZECHARS(szUnknownDevice)
               );

    LenUnknownDevice = lstrlen(szUnknownDevice) * sizeof(TCHAR) + sizeof(TCHAR);

    //
    // Get the Class Icon Image Lists.
    //
    HardwareWiz->ClassImageList.cbSize = sizeof(SP_CLASSIMAGELIST_DATA);
    if (!SetupDiGetClassImageList(&HardwareWiz->ClassImageList)) {

        HardwareWiz->ClassImageList.cbSize = 0;
    }

    //
    // Load the cursors that the wizard will need
    //
    HardwareWiz->CurrCursor     = NULL;
    HardwareWiz->IdcWait        = LoadCursor(NULL, IDC_WAIT);
    HardwareWiz->IdcAppStarting = LoadCursor(NULL, IDC_APPSTARTING);
    HardwareWiz->IdcArrow       = LoadCursor(NULL, IDC_ARROW);

    return HdwPropertySheet;
}

BOOL
WINAPI
InstallNewDevice(
   HWND hwndParent,
   LPGUID ClassGuid,
   PDWORD pReboot
   )
/*++

Routine Description:

   Exported Entry point from hdwwiz.cpl. Installs a new device. A new Devnode is
   created and the user is prompted to select the device. If the class guid
   is not specified then then the user begins at class selection.

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
    HARDWAREWIZ HardwareWiz;
    PHDWPROPERTYSHEET HdwPropertySheet;
    int PropSheetResult;
    SEARCHTHREAD SearchThread;
    BOOL StartDetect;

    if (NoPrivilegeWarning(hwndParent, FALSE)) {

        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    //
    // Check to make sure another Device install is underway.
    // This entry point is primarily for manual legacy installs.
    // While Base PNP has queued found new hdw installs, we don't
    // allow the user to install anything manually, since we may get
    // duplicate entries.
    //
    //
    if (CMP_WaitNoPendingInstallEvents(5000) == WAIT_TIMEOUT) {

        HdwMessageBox(hwndParent, 
                      MAKEINTRESOURCE(IDS_HDW_RUNNING_MSG), 
                      MAKEINTRESOURCE(IDS_HDW_RUNNING_TITLE), 
                      MB_OK | MB_ICONINFORMATION
                      );
        return FALSE;
    }

    memset(&HardwareWiz, 0, sizeof(HardwareWiz));

    HardwareWiz.PromptForReboot = pReboot == NULL;

    StartDetect = (ClassGuid == NULL);

    //
    // Create a DeviceInfoList, using the classers Class guid if any.
    //
    if (ClassGuid &&
        (IsEqualGUID(ClassGuid, &GUID_NULL) ||
        IsEqualGUID(ClassGuid, &GUID_DEVCLASS_UNKNOWN))) {

        ClassGuid = NULL;
    }

    HardwareWiz.hDeviceInfo = SetupDiCreateDeviceInfoList(ClassGuid, hwndParent);
    if (HardwareWiz.hDeviceInfo == INVALID_HANDLE_VALUE) {

        return FALSE;
    }

    try {

        //
        // If the caller specified a ClassGuid, retrieve the class information
        // and create a DeviceInfo for it.
        //
        if (ClassGuid) {

            HardwareWiz.ClassGuidSelected = ClassGuid;

            //
            // Add a new element to the DeviceInfo from the GUID and class name
            //
            HardwareWiz.DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);

            if (!SetupDiGetClassDescription(HardwareWiz.ClassGuidSelected,
                                            HardwareWiz.ClassDescription,
                                            sizeof(HardwareWiz.ClassDescription)/sizeof(TCHAR),
                                            NULL
                                            ) 
                ||
                !SetupDiClassNameFromGuid(HardwareWiz.ClassGuidSelected,
                                          HardwareWiz.ClassName,
                                          sizeof(HardwareWiz.ClassName)/sizeof(TCHAR),
                                          NULL
                                          ))
            {
                HardwareWiz.LastError = GetLastError();
                goto INDLeaveExcept;
            }
        
            if (!SetupDiCreateDeviceInfo(HardwareWiz.hDeviceInfo,
                                         HardwareWiz.ClassName,
                                         ClassGuid,
                                         NULL,
                                         hwndParent,
                                         DICD_GENERATE_ID,
                                         &HardwareWiz.DeviceInfoData
                                         )
                ||
                !SetupDiSetSelectedDevice(HardwareWiz.hDeviceInfo,
                                          &HardwareWiz.DeviceInfoData
                                          ))
            {
                HardwareWiz.LastError = GetLastError();
                goto INDLeaveExcept;
            }
        }

        memset(&SearchThread, 0, sizeof(SEARCHTHREAD));
        HardwareWiz.SearchThread = &SearchThread;
        
        HardwareWiz.LastError = CreateSearchThread(&HardwareWiz);

        if (HardwareWiz.LastError != ERROR_SUCCESS) {

            goto INDLeaveExcept;
        }

        //
        // Load the libraries that we will need
        //
        hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
        hHotPlug = LoadLibrary(TEXT("hotplug.dll"));
        hNewDev = LoadLibrary(TEXT("newdev.dll"));

        //
        // Create the property sheet
        //
        HdwPropertySheet = HdwWizard(hwndParent,
                                    &HardwareWiz,
                                    StartDetect ? IDD_ADDDEVICE_PNPENUM : IDD_INSTALLNEWDEVICE
                                    );

        if (HdwPropertySheet) {

            PropSheetResult = (int)PropertySheet(&HdwPropertySheet->PropSheetHeader);
            LocalFree(HdwPropertySheet);
        }
        
        //
        // See if we need to run a troubleshooter
        //
        if (HardwareWiz.RunTroubleShooter) {

           TCHAR DeviceID[MAX_DEVICE_ID_LEN];

           if (CM_Get_Device_ID_Ex(HardwareWiz.ProblemDevInst,
                                   DeviceID,
                                   sizeof(DeviceID) / sizeof(TCHAR),
                                   0,
                                   HardwareWiz.hMachine
                                   ) == CR_SUCCESS)
           {
               if (!pDeviceProblemWizard) {

                   pDeviceProblemWizard = (PVOID) GetProcAddress(hDevMgr, "DeviceProblemWizardW");
               }

               if (pDeviceProblemWizard) {

                   (pDeviceProblemWizard)(hwndParent,
                                         HardwareWiz.MachineName,
                                         DeviceID
                                         );
               }
           }
        }

        //
        // Final cleanup of DeviceInfoData and DeviceInfoList.
        //
        if (HardwareWiz.ClassGuidList) {

            LocalFree(HardwareWiz.ClassGuidList);
            HardwareWiz.ClassGuidList = NULL;
            HardwareWiz.ClassGuidSize = HardwareWiz.ClassGuidNum = 0;
        }

        if (HardwareWiz.ClassImageList.cbSize) {

            SetupDiDestroyClassImageList(&HardwareWiz.ClassImageList);
            HardwareWiz.ClassImageList.cbSize = 0;
        }

        if (HardwareWiz.Cancelled ||
            (HardwareWiz.Registered && !HardwareWiz.Installed)) {

            HdwRemoveDevice(&HardwareWiz);
            HardwareWiz.Reboot = 0;
        }

        SetupDiDestroyDeviceInfoList(HardwareWiz.hDeviceInfo);
        HardwareWiz.hDeviceInfo = NULL;

        //
        // Copy out the reboot flags for the caller
        // or put up the restart dialog if caller didn't ask for the reboot flag
        //
        if (pReboot) {

            *pReboot = HardwareWiz.Reboot;

        } else if (HardwareWiz.Reboot) {

             RestartDialog(hwndParent, NULL, EWX_REBOOT);
        }

INDLeaveExcept:;

    } except(HdwUnhandledExceptionFilter(GetExceptionInformation())) {

        HardwareWiz.LastError = RtlNtStatusToDosError(GetExceptionCode());
    }

    
    if (HardwareWiz.hDeviceInfo && HardwareWiz.hDeviceInfo != INVALID_HANDLE_VALUE) {

        SetupDiDestroyDeviceInfoList(HardwareWiz.hDeviceInfo);
        HardwareWiz.hDeviceInfo = NULL;
    }

    if (HardwareWiz.SearchThread) {

        DestroySearchThread(&SearchThread);
    }

    if (hDevMgr) {

        FreeLibrary(hDevMgr);
    }

    if (hHotPlug) {

        FreeLibrary(hHotPlug);
    }

    if (hNewDev) {

        FreeLibrary(hNewDev);
    }

    SetLastError(HardwareWiz.LastError);
    return HardwareWiz.LastError == ERROR_SUCCESS;
}


void
HdwTasksWiz(
   HWND hwnd,
   PTCHAR MachineName
   )
/*++

Routine Description:


Arguments:

   hwnd - Window handle of the top-level window to use for any UI related
          to installing the device.


Return Value:

--*/
{

    HARDWAREWIZ HardwareWiz;
    PHDWPROPERTYSHEET HdwPropertySheet;
    int PropSheetResult;
    SEARCHTHREAD SearchThread;
    CONFIGRET ConfigRet;

    //
    // If the user does not have SE_LOAD_DRIVER_NAME privileges then just display a warning
    // message and exit.
    //
    if (NoPrivilegeWarning(hwnd, TRUE)) {

        SetLastError(ERROR_ACCESS_DENIED);
        return;
    }

    if (MachineName) {

        lstrcpy(HardwareWiz.MachineName, MachineName);
        ConfigRet = CM_Connect_Machine(MachineName, &HardwareWiz.hMachine);
        if (ConfigRet != CR_SUCCESS) {

            return;
        }
    }

    //
    // Check to make sure another Device install is underway.
    // This entry point is primarily for manual legacy installs.
    // While Base PNP has queued found new hdw installs, we don't
    // allow the user to install anything manually, since we may get
    // duplicate entries.
    //
    //
    if (CMP_WaitNoPendingInstallEvents(5000) == WAIT_TIMEOUT) {

        HdwMessageBox(hwnd, 
                      MAKEINTRESOURCE(IDS_HDW_RUNNING_MSG), 
                      MAKEINTRESOURCE(IDS_HDW_RUNNING_TITLE), 
                      MB_OK | MB_ICONINFORMATION
                      );
        return;
    }

    memset(&HardwareWiz, 0, sizeof(HardwareWiz));

    //
    // Create a DeviceInfoList
    HardwareWiz.hDeviceInfo = SetupDiCreateDeviceInfoList(NULL, hwnd);
    if (HardwareWiz.hDeviceInfo == INVALID_HANDLE_VALUE) {

        return;
    }


    //
    // Create the Search thread to look for compatible drivers.
    // This thread will sit around waiting for requests until
    // told to go away.
    //
    memset(&SearchThread, 0, sizeof(SEARCHTHREAD));
    HardwareWiz.SearchThread = &SearchThread;

    if (CreateSearchThread(&HardwareWiz) != ERROR_SUCCESS) {

        SetupDiDestroyDeviceInfoList(HardwareWiz.hDeviceInfo);
        return;
    }

    //
    // Load the libraries that we will need
    //
    hDevMgr = LoadLibrary(TEXT("devmgr.dll"));
    hHotPlug = LoadLibrary(TEXT("hotplug.dll"));
    hNewDev = LoadLibrary(TEXT("newdev.dll"));

    HdwPropertySheet = HdwWizard(hwnd, &HardwareWiz, 0);
    if (HdwPropertySheet) {
    
        PropSheetResult = (int)PropertySheet(&HdwPropertySheet->PropSheetHeader);
        LocalFree(HdwPropertySheet);
    }

    //
    // See if we need to run a troubleshooter
    //
    if (HardwareWiz.RunTroubleShooter) {

       TCHAR DeviceID[MAX_DEVICE_ID_LEN];

       if (CM_Get_Device_ID_Ex(HardwareWiz.ProblemDevInst,
                               DeviceID,
                               sizeof(DeviceID) / sizeof(TCHAR),
                               0,
                               HardwareWiz.hMachine
                               ) == CR_SUCCESS)
       {
           if (!pDeviceProblemWizard) {
    
               pDeviceProblemWizard = (PVOID) GetProcAddress(hDevMgr, "DeviceProblemWizardW");
           }
    
           if (pDeviceProblemWizard) {
    
               (pDeviceProblemWizard)(hwnd,
                                     HardwareWiz.MachineName,
                                     DeviceID
                                     );
           }
       }
    }

    //
    // Final cleanup of DeviceInfoData and DeviceInfoList
    //
    if (HardwareWiz.ClassGuidList) {

        LocalFree(HardwareWiz.ClassGuidList);
        HardwareWiz.ClassGuidList = NULL;
        HardwareWiz.ClassGuidSize = HardwareWiz.ClassGuidNum = 0;
    }

    if (HardwareWiz.ClassImageList.cbSize) {

        SetupDiDestroyClassImageList(&HardwareWiz.ClassImageList);
        HardwareWiz.ClassImageList.cbSize = 0;
    }

    if (HardwareWiz.Cancelled || 
        (HardwareWiz.Registered && !HardwareWiz.Installed)) {

        HdwRemoveDevice(&HardwareWiz);
        HardwareWiz.Reboot = 0;
    }

    SetupDiDestroyDeviceInfoList(HardwareWiz.hDeviceInfo);
    HardwareWiz.hDeviceInfo = NULL;

    if (HardwareWiz.SearchThread) {

        DestroySearchThread(HardwareWiz.SearchThread);
    }

    if (hDevMgr) {

        FreeLibrary(hDevMgr);
    }

    if (hHotPlug) {

        FreeLibrary(hHotPlug);
    }

    if (hNewDev) {

        FreeLibrary(hNewDev);
    }

    //
    // Do we need to reboot?
    //
    if (HardwareWiz.Reboot) {

        RestartDialog(hwnd, NULL, EWX_REBOOT);
    }

    return;
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
           lpOldCPlInfo->idIcon = IDI_HDWWIZICON;
           lpOldCPlInfo->idName = IDS_HDWWIZ;
           lpOldCPlInfo->idInfo = IDS_HDWWIZINFO;
           return TRUE;

       case CPL_NEWINQUIRE:
           lpCPlInfo = (LPNEWCPLINFO)(LPARAM)lParam2;
           lpCPlInfo->hIcon = LoadIcon(hHdwWiz, MAKEINTRESOURCE(IDI_HDWWIZICON));
           LoadString(hHdwWiz, IDS_HDWWIZ, lpCPlInfo->szName, sizeof(lpCPlInfo->szName));
           LoadString(hHdwWiz, IDS_HDWWIZINFO, lpCPlInfo->szInfo, sizeof(lpCPlInfo->szInfo));
           lpCPlInfo->dwHelpContext = IDH_HDWWIZAPPLET;
           lpCPlInfo->dwSize = sizeof(NEWCPLINFO);
           lpCPlInfo->lData = 0;
           lpCPlInfo->szHelpFile[0] = '\0';
           return TRUE;

       case CPL_DBLCLK:
           HdwTasksWiz(hWnd, NULL);
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
    hHdwWiz = hmod;

    if (ulReason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hmod);
        //InitCommonControls();
        }


    return TRUE;
}
