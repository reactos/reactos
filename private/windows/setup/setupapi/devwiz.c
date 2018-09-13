/*++

Copyright (c) 1993-1998 Microsoft Corporation

Module Name:

    devwiz.c

Abstract:

    Device Installer functions for install wizard support.

Author:

    Lonny McMichael (lonnym) 22-Sep-1995

Revision History:

--*/

#include "precomp.h"
#pragma hdrstop

//
// Define some macros to make the code a little cleaner.
//

//
// BOOL
// USE_CI_SELSTRINGS(
//     IN PDEVINSTALL_PARAM_BLOCK p
//     );
//
// This macro checks all the appropriate values to determine whether the
// class-installer provided strings may be used in the wizard.
//
#define USE_CI_SELSTRINGS(p)                                                \
                                                                            \
    (((((p)->Flags) & (DI_USECI_SELECTSTRINGS | DI_CLASSINSTALLPARAMS)) ==  \
      (DI_USECI_SELECTSTRINGS | DI_CLASSINSTALLPARAMS)) &&                  \
     (((p)->ClassInstallHeader->InstallFunction) == DIF_SELECTDEVICE))

//
// PTSTR
// GET_CI_SELSTRING(
//     IN PDEVINSTALL_PARAM_BLOCK p,
//     IN <FieldName>             f
//     );
//
// This macro retrieves a pointer to the specified string in a
// SP_SELECTDEVICE_PARAMS structure.
//
#define GET_CI_SELSTRINGS(p, f)                                             \
                                                                            \
    (((PSP_SELECTDEVICE_PARAMS)((p)->ClassInstallHeader))->f)

//
// Definitions for timer used in device selection listboxes.
//
#define SELECTMFG_TIMER_ID              1
#define SELECTMFG_TIMER_DELAY           250

//
// Define a message sent from our auxilliary class driver search thread.
//
#define WMX_CLASSDRVLIST_DONE    (WM_USER+131)
#define WMX_NO_DRIVERS_IN_LIST   (WM_USER+132)

//
// Define structure containing class driver search context that is passed to
// an auxilliary thread while a Select Device dialog is displayed.
//
typedef struct _CLASSDRV_THREAD_CONTEXT {

    HDEVINFO        DeviceInfoSet;
    SP_DEVINFO_DATA DeviceInfoData;

    HWND NotificationWindow;

} CLASSDRV_THREAD_CONTEXT, *PCLASSDRV_THREAD_CONTEXT;


//
// Private function prototypes
//
DWORD
pSetupCreateNewDevWizData(
    IN  PSP_INSTALLWIZARD_DATA  InstallWizardData,
    OUT PNEWDEVWIZ_DATA        *NewDeviceWizardData
    );

UINT
CALLBACK
SelectDevicePropSheetPageProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE ppsp
    );

INT_PTR
CALLBACK
SelectDeviceDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );

BOOL
InitSelectDeviceDlg(
    IN     HWND hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    );

VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
OnSetActive(
    IN     HWND            hwndDlg,
    IN OUT PNEWDEVWIZ_DATA ndwData
    );

DWORD
HandleSelectOEM(
    IN     HWND           hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    );

DWORD
HandleWindowsUpdate(
    IN     HWND           hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    );

DWORD
FillInDeviceList(
    IN HWND           hwndDlg,
    IN PSP_DIALOGDATA lpdd
    );

VOID
ShowListForMfg(
    IN PSP_DIALOGDATA          lpdd,
    IN PDEVICE_INFO_SET        DeviceInfoSet,
    IN PDEVINSTALL_PARAM_BLOCK InstallParamBlock,
    IN PDRIVER_NODE            DriverNode,        OPTIONAL
    IN INT                     iMfg
    );

VOID
LockAndShowListForMfg(
    IN PSP_DIALOGDATA   lpdd,
    IN INT              iMfg
    );

PDRIVER_NODE
GetDriverNodeFromLParam(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PSP_DIALOGDATA   lpdd,
    IN LPARAM           lParam
    );

BOOL
pSetupIsSelectedHardwareIdValid(
    IN HWND           hWnd,
    IN PSP_DIALOGDATA lpdd,
    IN INT            iCur
    );

VOID
SetSelectedDriverNode(
    IN PSP_DIALOGDATA lpdd,
    IN INT            iCur
    );

BOOL
bNoDevsToShow(
    IN PDEVINFO_ELEM DevInfoElem
    );

PNEWDEVWIZ_DATA
GetNewDevWizDataFromPsPage(
    LPPROPSHEETPAGE ppsp
    );

LONG
GetCurDesc(
    IN PSP_DIALOGDATA lpdd
    );

VOID
OnCancel(
    IN PNEWDEVWIZ_DATA ndwData
    );

VOID
__cdecl
ClassDriverSearchThread(
    IN PVOID Context
    );

BOOL
pSetupIsClassDriverListBuilt(
    IN PSP_DIALOGDATA lpdd
    );

VOID
pSetupDevInfoDataFromDialogData(
    IN  PSP_DIALOGDATA   lpdd,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    );

VOID
ToggleDialogControls(
    IN HWND           hwndDlg,
    IN PSP_DIALOGDATA lpdd,
    IN BOOL           Enable
    );

BOOL
CDMIsInternetAvailable(
    void
    );


HPROPSHEETPAGE
WINAPI
SetupDiGetWizardPage(
    IN HDEVINFO               DeviceInfoSet,
    IN PSP_DEVINFO_DATA       DeviceInfoData,    OPTIONAL
    IN PSP_INSTALLWIZARD_DATA InstallWizardData,
    IN DWORD                  PageType,
    IN DWORD                  Flags
    )
/*++

Routine Description:

    This routine retrieves a handle to one of the Setup API-provided wizard
    pages, for an application to include in its own wizard.

Arguments:

    DeviceInfoSet - Supplies the handle of the device information set to
        retrieve a wizard page for.

    DeviceInfoData - Optionally, supplies the address of a device information
        with which the wizard page will be associated.  This parameter is only
        used if the flags parameter includes DIWP_FLAG_USE_DEVINFO_DATA.  If that
        flag is set, and if this parameter is not specified, then the wizard
        page will be associated with the global class driver list.

    InstallWizardData - Supplies the address of a PSP_INSTALLWIZARD_DATA
        structure containing parameters to be used by this wizard page.  The
        cbSize field must be set to the size of the structure, in bytes, or the
        structure will be considered invalid.

    PageType - Supplies an ordinal indicating the type of wizard page to be retreived.
        May be one of the following values:

        SPWPT_SELECTDEVICE - Retrieve a select device wizard page.

    Flags - Supplies flags that specify how the wizard page is to be created.
        May be a combination of the following values:

        SPWP_USE_DEVINFO_DATA - Use the device information element specified
                                by DeviceInfoData, or use the global class
                                driver list if DeviceInfoData is not supplied.
                                If this flag is not supplied, the wizard page
                                will act upon the currently selected device
                                (as selected by SetupDiSetSelectedDevice), or
                                upon the global class driver list if no device
                                is selected.

Return Value:

    If the function succeeds, the return value is the handle to the requested
    wizard page.

    If the function fails, the return value is NULL.  To get extended error
    information, call GetLastError.

Remarks:

    A device information set may not be destroyed as long as there are any active wizard
    pages using it.  In addition, if the wizard page is associated with a particular device
    information element, then that element will not be deletable as long as it is being
    used by a wizard page.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    DWORD Err = NO_ERROR;
    HPROPSHEETPAGE hPage = NULL;
    PNEWDEVWIZ_DATA ndwData = NULL;
    PWIZPAGE_OBJECT WizPageObject = NULL;
    //
    // Store the address of the corresponding wizard object at the
    // end of the PROPSHEETPAGE buffer.
    //
    BYTE pspBuffer[sizeof(PROPSHEETPAGE) + sizeof(PVOID)];
    LPPROPSHEETPAGE Page = (LPPROPSHEETPAGE)pspBuffer;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    try {

        switch(PageType) {

            case SPWPT_SELECTDEVICE :

                Page->pszTemplate = MAKEINTRESOURCE(IDD_DYNAWIZ_SELECTDEV_PAGE);
                Page->pfnDlgProc = SelectDeviceDlgProc;
                Page->pfnCallback = SelectDevicePropSheetPageProc;

#ifndef ANSI_SETUPAPI
                Page->pszHeaderTitle = MAKEINTRESOURCE(IDS_NDW_SELECTDEVICE);
                Page->pszHeaderSubTitle = MAKEINTRESOURCE(IDS_NDW_SELECTDEVICE_INFO);
#endif                

                break;

            default :
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
        }

        //
        // Validate the supplied InstallWizardData structure, and create a private
        // storage buffer for internal use by the wizard page.
        //
        if((Err = pSetupCreateNewDevWizData(InstallWizardData, &ndwData)) != NO_ERROR) {
            goto clean0;
        }

        //
        // Store the device information set handle in the dialogdata structure
        // embedded in the New Device Wizard buffer.
        //
        ndwData->ddData.DevInfoSet = DeviceInfoSet;

        //
        // If the caller specified the SPWP_USE_DEVINFO_DATA flag, then store information
        // in the dialog data structure about the specified devinfo element (if supplied).
        //
        if(Flags & SPWP_USE_DEVINFO_DATA) {
            if(DeviceInfoData) {
                //
                // Verify that the specified device information element is a valid one.
                //
                if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                             DeviceInfoData,
                                                             NULL))) {
                    Err = ERROR_INVALID_PARAMETER;
                    goto clean0;

                } else if(DevInfoElem->DiElemFlags & DIE_IS_LOCKED) {
                    //
                    // Device information element cannot be explicitly used by more than
                    // one wizard page at a time.
                    //
                    Err = ERROR_DEVINFO_DATA_LOCKED;
                    goto clean0;
                }

                DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                ndwData->ddData.DevInfoElem = DevInfoElem;
            }
            ndwData->ddData.flags = DD_FLAG_USE_DEVINFO_ELEM;
        }

        //
        // We've successfully created and initialized the devwiz data structure.
        // Now create a wizpage object so we can keep track of it.
        //
        if(WizPageObject = MyMalloc(sizeof(WIZPAGE_OBJECT))) {
            WizPageObject->RefCount = 0;
            WizPageObject->ndwData = ndwData;
            //
            // Insert this new object into the devinfo set's wizard object list.
            //
            WizPageObject->Next = pDeviceInfoSet->WizPageList;
            pDeviceInfoSet->WizPageList = WizPageObject;

        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        Page->dwSize = sizeof(pspBuffer);
        
#ifndef ANSI_SETUPAPI
        Page->dwFlags = PSP_DEFAULT | PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE | PSP_USECALLBACK;
#else
        Page->dwFlags = PSP_DEFAULT | PSP_USECALLBACK;
#endif
        
        Page->hInstance = MyDllModuleHandle;

        Page->lParam = (LPARAM)DeviceInfoSet;

        *((PVOID *)(&(pspBuffer[sizeof(PROPSHEETPAGE)]))) = WizPageObject;

        if(!(hPage = CreatePropertySheetPage(Page))) {
            Err = ERROR_INVALID_DATA;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(Err != NO_ERROR) {
        if(ndwData) {
            MyFree(ndwData);
        }
        if(WizPageObject) {
            MyFree(WizPageObject);
        }
    }

    SetLastError(Err);
    return hPage;
}


BOOL
WINAPI
SetupDiGetSelectedDevice(
    IN  HDEVINFO          DeviceInfoSet,
    OUT PSP_DEVINFO_DATA  DeviceInfoData
    )
/*++

Routine Description:

    This routine retrieves the currently-selected device for the specified
    device information set.  This is typically used during an installation
    wizard.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for
        which the selected device is to be retrieved.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure
        that receives the currently-selected device.  If there is no device
        currently selected, then the routine will fail, and GetLastError
        will return ERROR_NO_DEVICE_SELECTED.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(pDeviceInfoSet->SelectedDevInfoElem) {

            if(!(DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                  pDeviceInfoSet->SelectedDevInfoElem,
                                                  DeviceInfoData))) {
                Err = ERROR_INVALID_USER_BUFFER;
            }

        } else {
            Err = ERROR_NO_DEVICE_SELECTED;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
WINAPI
SetupDiSetSelectedDevice(
    IN HDEVINFO          DeviceInfoSet,
    IN PSP_DEVINFO_DATA  DeviceInfoData
    )
/*++

Routine Description:

    This routine sets the specified device information element to be the
    currently selected member of a device information set.  This is typically
    used during an installation wizard.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for
        which the selected device is to be set.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure
        specifying the device information element to be selected.

Return Value:

    If the function succeeds, the return value is TRUE.

    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {

        if(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet, DeviceInfoData, NULL)) {
            pDeviceInfoSet->SelectedDevInfoElem = DevInfoElem;
        } else {
            Err = ERROR_INVALID_PARAMETER;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    SetLastError(Err);
    return(Err == NO_ERROR);
}


DWORD
pSetupCreateNewDevWizData(
    IN  PSP_INSTALLWIZARD_DATA  InstallWizardData,
    OUT PNEWDEVWIZ_DATA        *NewDeviceWizardData
    )
/*++

Routine Description:

    This routine validates an InstallWizardData buffer, then allocates and
    fills in a NEWDEVWIZ_DATA buffer based on information supplied therein.

Arguments:

    InstallWizardData - Supplies the address of an installation wizard data
        structure to be validated and used in building the private buffer.

    NewDeviceWizardData - Supplies the address of a variable that receives a pointer
        to the newly-allocated install wizard data buffer.

Return Value:

    If the function succeeds, the return value is NO_ERROR, otherwise, it is
    an ERROR_* code.

--*/
{
    PNEWDEVWIZ_DATA ndwData = NULL;
    DWORD Err = NO_ERROR;

    if((InstallWizardData->ClassInstallHeader.cbSize != sizeof(SP_CLASSINSTALL_HEADER)) ||
       (InstallWizardData->ClassInstallHeader.InstallFunction != DIF_INSTALLWIZARD)) {

        return ERROR_INVALID_USER_BUFFER;
    }

    //
    // The dynamic page entries are currently ignored, as are the Private
    // fields.  Also, the hwndWizardDlg is not validated.
    //

    try {

        if(ndwData = MyMalloc(sizeof(NEWDEVWIZ_DATA))) {
            ZeroMemory(ndwData, sizeof(NEWDEVWIZ_DATA));
        } else {
            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;
        }

        //
        // Initialize the Current Description string table index in the dialog data
        // to -1, so that it will get updated when the wizard page is first entered.
        //
        ndwData->ddData.iCurDesc = -1;

        //
        // Copy the installwizard data.
        //
        CopyMemory(&(ndwData->InstallData),
                   InstallWizardData,
                   sizeof(SP_INSTALLWIZARD_DATA)
                  );

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
    }

    if((Err != NO_ERROR) && ndwData) {
        MyFree(ndwData);
    } else {
        *NewDeviceWizardData = ndwData;
    }

    return Err;
}


UINT
CALLBACK
SelectDevicePropSheetPageProc(
    IN HWND hwnd,
    IN UINT uMsg,
    IN LPPROPSHEETPAGE ppsp
    )
/*++

Routine Description:

    This routine is called when the Select Device wizard page is created or destroyed.

Arguments:

    hwnd - Reserved

    uMsg - Action flag, either PSPCB_CREATE or PSPCB_RELEASE

    ppsp - Supplies the address of the PROPSHEETPAGE structure being created or destroyed.

Return Value:

    If uMsg is PSPCB_CREATE, then return non-zero to allow the page to be created, or
    zero to prevent it.

    if uMsg is PSPCB_RELEASE, the return value is ignored.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    UINT ret;
    PVOID WizObjectId;
    PWIZPAGE_OBJECT CurWizObject, PrevWizObject;

    //
    // Access the device info set handle stored in the propsheetpage's lParam.
    //
    if(!(pDeviceInfoSet = AccessDeviceInfoSet((HDEVINFO)(ppsp->lParam)))) {
        return FALSE;
    }

    ret = TRUE;

    try {
        //
        // The ObjectID (pointer, actually) for the corresponding wizard
        // object for this page is stored at the end of the ppsp structure.
        // Retrieve this now, and look for it in the devinfo set's list of
        // wizard objects.
        //
        WizObjectId = *((PVOID *)(&(((PBYTE)ppsp)[sizeof(PROPSHEETPAGE)])));

        for(CurWizObject = pDeviceInfoSet->WizPageList, PrevWizObject = NULL;
            CurWizObject;
            PrevWizObject = CurWizObject, CurWizObject = CurWizObject->Next) {

            if(WizObjectId == CurWizObject) {
                //
                // We found our object.
                //
                break;
            }
        }

        if(!CurWizObject) {
            ret = FALSE;
            goto clean0;
        }

        switch(uMsg) {

            case PSPCB_CREATE :
                //
                // Fail the create if we've already been created once (hopefully, this
                // will never happen).
                //
                if(CurWizObject->RefCount) {
                    ret = FALSE;
                    goto clean0;
                } else {
                    CurWizObject->RefCount++;
                }
                break;

            case PSPCB_RELEASE :
                //
                // Decrement the wizard object refcount.  If it goes to zero (or if it
                // already was zero because we never got a PSPCB_CREATE message), then
                // remove the object from the linked list, and free all associated memory.
                //
                if(CurWizObject->RefCount) {
                    CurWizObject->RefCount--;
                }

                MYASSERT(!CurWizObject->RefCount);

                if(!CurWizObject->RefCount) {
                    //
                    // Remove the object from the object list.
                    //
                    if(PrevWizObject) {
                        PrevWizObject->Next = CurWizObject->Next;
                    } else {
                        pDeviceInfoSet->WizPageList = CurWizObject->Next;
                    }

                    //
                    // If this wizard object was explicitly tied to a particular device
                    // information element, then unlock that element now.
                    //
                    if((CurWizObject->ndwData->ddData.flags & DD_FLAG_USE_DEVINFO_ELEM) &&
                       (DevInfoElem = CurWizObject->ndwData->ddData.DevInfoElem)) {

                        MYASSERT(DevInfoElem->DiElemFlags & DIE_IS_LOCKED);

                        DevInfoElem->DiElemFlags ^= DIE_IS_LOCKED;
                    }

                    MyFree(CurWizObject->ndwData);
                    MyFree(CurWizObject);
                }
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ret = FALSE;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return ret;
}


INT_PTR
CALLBACK
SelectDeviceDlgProc(
    IN HWND hwndDlg,
    IN UINT uMsg,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
/*++

Routine Description:

    This is the dialog proc for the Select Device wizard page.

--*/
{
    INT iCur;
    HICON hicon;
    PNEWDEVWIZ_DATA ndwData;
    PSP_INSTALLWIZARD_DATA iwd;
    LV_ITEM lvItem;
    TCHAR TempString[LINE_LEN];
    PCLASSDRV_THREAD_CONTEXT ClassDrvThreadContext;
    HCURSOR hOldCursor;

    if(uMsg == WM_INITDIALOG) {

        LPPROPSHEETPAGE Page = (LPPROPSHEETPAGE)lParam;

        //
        // Retrieve a pointer to the device wizard data associated with
        // this wizard page.
        //
        ndwData = GetNewDevWizDataFromPsPage(Page);
        SetWindowLongPtr(hwndDlg, DWLP_USER, (LONG_PTR)ndwData);

        if(ndwData) {
            ndwData->bInit = TRUE;
            ndwData->idTimer = 0;
            ndwData->bInit = FALSE;
        } else {
            //
            // This is really bad--we can't simply call EndDialog() since we
            // don't know whether we're a dialog or a wizard page.  This should
            // never happen.
            //
            return TRUE;  // we didn't set the focus
        }

        if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
            //
            // For the stand-alone dialog box version, we initialize here.
            //
            ndwData->bInit = TRUE;       // Still doing some init stuff

            //
            // Make sure our "waiting for class list" static text control is hidden!
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);

            if (!InitSelectDeviceDlg(hwndDlg, &(ndwData->ddData))) {

                //
                // We don't have any items displayed so ask the user if they want
                // to go directly to Have Disk, or just cancel.
                //
                PostMessage(hwndDlg, WMX_NO_DRIVERS_IN_LIST, 0, 0);
            }

            ndwData->bInit = FALSE;      // Done with init stuff

            return FALSE;   // we already set the focus.

        } else {
            return TRUE;    // we didn't set the focus
        }

    } else {
        //
        // For the small set of messages that we get before WM_INITDIALOG, we
        // won't have a devwizdata pointer!
        //
        if(ndwData = (PNEWDEVWIZ_DATA)GetWindowLongPtr(hwndDlg, DWLP_USER)) {
            iwd = &(ndwData->InstallData);
        } else {
            //
            // If we haven't gotten a WM_INITDIALOG message yet, or if for some reason
            // we weren't able to retrieve the ndwData pointer when we did, then we
            // simply return FALSE for all messages.
            //
            // (If we ever need to process messages before WM_INITDIALOG (e.g., set font),
            // then we'll need to modify this approach.)
            //
            return FALSE;
        }
    }

    switch(uMsg) {

        case WMX_CLASSDRVLIST_DONE :

            MYASSERT(ndwData->ddData.AuxThreadRunning);
            ndwData->ddData.AuxThreadRunning = FALSE;

            //
            // wParam is a boolean indicating the result of the class driver search.
            // lParam is NO_ERROR upon success, or a Win32 error code indicating cause of failure.
            //
            switch(ndwData->ddData.PendingAction) {

                case PENDING_ACTION_NONE :
                    //
                    // Then the thread has completed, but the user is still mulling over the
                    // choices on the compatible driver list.  If the class driver list was
                    // successfully built, then there's nothing to do here.  If it failed for
                    // some reason (highly unlikely), then we (silently) disable the class list
                    // radio button.
                    //
                    if(!wParam) {
                        ndwData->ddData.flags |= DD_FLAG_CLASSLIST_FAILED;
                        EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), FALSE);
                    }
                    break;

                case PENDING_ACTION_SELDONE :
                    //
                    // In this case, we don't care what happened in the other thread.  The
                    // user has made their selection, and we're ready to return success.
                    //
                    SetSelectedDriverNode(&(ndwData->ddData),
                                          ndwData->ddData.CurSelectionForSuccess
                                         );
                    EndDialog(hwndDlg, NO_ERROR);
                    break;

                case PENDING_ACTION_SHOWCLASS :
                    //
                    // Then we've been waiting on the class driver search to complete, so that
                    // we can show the list.  Hopefully, the search was successful.  If not,
                    // we'll give the user a popup saying that the list could not be shown, and
                    // then leave them in the compatible list view (with the class list radio
                    // button now disabled).
                    //
                    ndwData->ddData.PendingAction = PENDING_ACTION_NONE;

                    if(wParam) {
                        //
                        // The class driver list was built successfully.
                        //
                        if(ndwData->ddData.CurSelectionForSuccess != LB_ERR) {

                            lvItem.mask = LVIF_TEXT;
                            lvItem.iItem = ndwData->ddData.CurSelectionForSuccess;
                            lvItem.iSubItem = 0;
                            lvItem.pszText = TempString;
                            lvItem.cchTextMax = SIZECHARS(TempString);

                            if(ListView_GetItem((ndwData->ddData).hwndDrvList, &lvItem)) {
                                //
                                // Now retrieve the (case-insensitive) string ID of this
                                // string, and store it as the current description ID.
                                //
                                (ndwData->ddData).iCurDesc = LookUpStringInDevInfoSet((ndwData->ddData).DevInfoSet,
                                                                                      TempString,
                                                                                      FALSE
                                                                                     );
                            }
                        }

                        ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);

                        if(FillInDeviceList(hwndDlg, &(ndwData->ddData)) == NO_ERROR) {
                            EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);
                            break;
                        }
                    }

                    //
                    // Inform the user that the class driver search failed.
                    //
                    if(!LoadString(MyDllModuleHandle,
                                   IDS_SELECT_DEVICE,
                                   TempString,
                                   SIZECHARS(TempString))) {
                        *TempString = TEXT('\0');
                    }

                    FormatMessageBox(MyDllModuleHandle,
                                     hwndDlg,
                                     MSG_NO_CLASSDRVLIST_ERROR,
                                     TempString,
                                     MB_OK | MB_TASKMODAL
                                    );

                    //
                    // Re-select the "Show compatible devices" radio button, and disable
                    // the 'Show all devices" radio button.
                    //
                    ndwData->ddData.ListType = IDC_NDW_PICKDEV_SHOWCOMPAT;
                    CheckRadioButton(hwndDlg,
                                     IDC_NDW_PICKDEV_SHOWCOMPAT,
                                     IDC_NDW_PICKDEV_SHOWALL,
                                     IDC_NDW_PICKDEV_SHOWCOMPAT
                                    );

                    ndwData->ddData.flags |= DD_FLAG_CLASSLIST_FAILED;
                    EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), FALSE);
                    //
                    // We also must unhide the compatible driver list controls, and re-enable
                    // the OK button.
                    //
                    ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_SHOW);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_SHOW);
                    EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);

                    break;

                case PENDING_ACTION_CANCEL :
                    //
                    // This is an easy one.  No matter what happened in the other thread,
                    // we simply want to clean up and return.
                    //
                    OnCancel(ndwData);
                    EndDialog(hwndDlg, ERROR_CANCELLED);
                    break;

                case PENDING_ACTION_OEM :
                case PENDING_ACTION_WINDOWSUPDATE:
                    //
                    // The user clicked the "Have Disk" button or the "Windows Update" button.
                    // Pass this off to either HandleSelectOEM() or HandleWindowsUpdate().
                    // If we get back success, then we are done, and can end the dialog.
                    //
                    ndwData->ddData.PendingAction = PENDING_ACTION_NONE;

                    if(((ndwData->ddData.PendingAction == PENDING_ACTION_OEM) &&
                        (HandleSelectOEM(hwndDlg, &(ndwData->ddData)) == NO_ERROR)) ||
                       ((ndwData->ddData.PendingAction == PENDING_ACTION_WINDOWSUPDATE) &&
                        (HandleWindowsUpdate(hwndDlg, &(ndwData->ddData)) == NO_ERROR))) {
                        EndDialog(hwndDlg, NO_ERROR);
                    } else {
                        //
                        // The OEM selection was not made, so we'll just continue as though
                        // nothing had happened.  Re-enable the dialog controls.
                        //
                        if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {

                            ToggleDialogControls(hwndDlg, &(ndwData->ddData), TRUE);
                        }

                        ndwData->bInit = FALSE;

                        //
                        // We got here by aborting the class driver search.  Since we may need
                        // it after all, we must re-start the search (unless the auxilliary thread
                        // happened to have already finished before we sent it the abort request).
                        //
                        if(!(ndwData->ddData.flags & DD_FLAG_CLASSLIST_FAILED) &&
                           !pSetupIsClassDriverListBuilt(&(ndwData->ddData)))
                        {
                            //
                            // Allocate a context structure to pass to the auxilliary thread (the
                            // auxilliary thread will take care of freeing the memory).
                            //
                            if(ClassDrvThreadContext = MyMalloc(sizeof(CLASSDRV_THREAD_CONTEXT))) {
                                //
                                // Fill in the context structure, and fire off the thread.
                                //
                                ClassDrvThreadContext->DeviceInfoSet = ndwData->ddData.DevInfoSet;

                                //
                                // SP_DEVINFO_DATA can only be retrieved whilst the device information
                                // set is locked.
                                //
                                pSetupDevInfoDataFromDialogData(&(ndwData->ddData),
                                                                &(ClassDrvThreadContext->DeviceInfoData)
                                                               );

                                ClassDrvThreadContext->NotificationWindow = hwndDlg;

                                if(_beginthread(ClassDriverSearchThread, 0, ClassDrvThreadContext) == -1) {
                                    MyFree(ClassDrvThreadContext);
                                } else {

                                    ndwData->ddData.AuxThreadRunning = TRUE;

                                    //
                                    // If we're currently in the class driver list view, then disable
                                    // the OK button, since the user can't select a class driver yet.
                                    //
                                    EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);
                                }
                            }

                            if(!(ndwData->ddData.AuxThreadRunning)) {
                                //
                                // We couldn't start the class driver search thread.  Disable the
                                // "Show all devices" radio button (and select the "Show compatible
                                // devices" button, if it's not already selected).
                                //
                                if(ndwData->ddData.ListType != IDC_NDW_PICKDEV_SHOWCOMPAT) {

                                    ndwData->ddData.ListType = IDC_NDW_PICKDEV_SHOWCOMPAT;
                                    CheckRadioButton(hwndDlg,
                                                     IDC_NDW_PICKDEV_SHOWCOMPAT,
                                                     IDC_NDW_PICKDEV_SHOWALL,
                                                     IDC_NDW_PICKDEV_SHOWCOMPAT
                                                    );
                                }

                                ndwData->ddData.flags |= DD_FLAG_CLASSLIST_FAILED;
                                EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), FALSE);
                            }
                        }
                    }
            }

            break;

        case WMX_NO_DRIVERS_IN_LIST: {

            TCHAR Title[LINE_LEN];

            LoadString(MyDllModuleHandle, IDS_SELECT_DEVICE, Title, SIZECHARS(Title));
            LoadString(MyDllModuleHandle, IDS_NDW_NODRIVERS_WARNING, TempString, SIZECHARS(TempString));

            if (IDOK == MessageBox(hwndDlg, TempString, Title, MB_OKCANCEL | MB_ICONEXCLAMATION)) {

                PostMessage(hwndDlg, WM_COMMAND, IDC_NDW_PICKDEV_HAVEDISK, 0);
            } else {

                PostMessage(hwndDlg, WM_COMMAND, IDCANCEL, 0);
            }

            break;
        }

        case WM_DESTROY:

            if(ndwData->ddData.AuxThreadRunning) {
                //
                // This should never happen.  But just to be on the safe side, if it does,
                // we'll cancel the search.  We _will not_ however, wait for the
                // WMX_CLASSDRVLIST_DONE message, to signal that the thread has terminated.
                // This should be OK, since the worst that can happen is that it will try
                // to send a message to a window that no longer exists.
                //
                SetupDiCancelDriverInfoSearch(ndwData->ddData.DevInfoSet);
            }

            if(ndwData->idTimer) {
                ndwData->bInit = TRUE;
                KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
            }

            if(hicon = (HICON)SendDlgItemMessage(hwndDlg, IDC_CLASSICON, STM_GETICON, 0, 0)) {
                DestroyIcon(hicon);
            }
            break;

        case WM_COMMAND:

            switch(LOWORD(wParam)) {

                case IDC_NDW_PICKDEV_SHOWCOMPAT :
                case IDC_NDW_PICKDEV_SHOWALL :

                    if((HIWORD(wParam) == BN_CLICKED) &&
                       IsWindowVisible(GetDlgItem(hwndDlg, LOWORD(wParam))) &&
                       !IsDlgButtonChecked(hwndDlg, LOWORD(wParam))) {
                        //
                        // Due to focus/click weirdness in USER, pre-click
                        // the button and unclick if needed.
                        //
                        CheckRadioButton(hwndDlg,
                                         IDC_NDW_PICKDEV_SHOWCOMPAT,
                                         IDC_NDW_PICKDEV_SHOWALL,
                                         LOWORD(wParam)
                                        );
                        ndwData->ddData.ListType = (INT)LOWORD(wParam);
                        //
                        // Update the current description ID in the dialog data so that
                        // the same device will be highlighted when we switch from one
                        // view to the other.
                        //
                        iCur = (int)ListView_GetNextItem((ndwData->ddData).hwndDrvList,
                                                         -1,
                                                         LVNI_SELECTED
                                                        );

                        if(ndwData->ddData.AuxThreadRunning) {
                            //
                            // There are two possibilities here:
                            //
                            // 1. The user was looking at the compatible driver list, and then
                            //    decided to look at the class driver list, which we're not done
                            //    building yet.  In that case, hide the compatible driver listbox,
                            //    and unhide our "waiting for class list" static text control.
                            //
                            // 2. The user switched to the class driver list view, saw that we
                            //    were still working on it, and then decided to switch back to
                            //    the compatible list.  In that case, we simply need to re-hide
                            //    the "waiting for class list" static text control, and show
                            //    the compatible driver listbox again.  In this case, we don't
                            //    want to attempt to re-initialize the listbox, as that will
                            //    require acquiring the HDEVINFO lock, and we will hang.
                            //
                            if(ndwData->ddData.ListType == IDC_NDW_PICKDEV_SHOWCOMPAT) {

                                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_SHOW);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_SHOW);
                                EnableWindow(GetDlgItem(hwndDlg, IDOK), TRUE);

                                //
                                // We no longer have a pending action.
                                //
                                ndwData->ddData.PendingAction = PENDING_ACTION_NONE;

                            } else {
                                //
                                // Temporarily hide the compatible driver listbox, and unhide the
                                // "waiting for class list" static text control.
                                //
                                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_HIDE);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_HIDE);
                                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_SHOW);

                                LoadString(MyDllModuleHandle, IDS_NDW_RETRIEVING_LIST, TempString, SIZECHARS(TempString));
                                SetDlgItemText(hwndDlg, IDC_NDW_STATUS_TEXT, TempString);

                                //
                                // Disable the OK button, because the user can't select a class driver
                                // yet.
                                //
                                EnableWindow(GetDlgItem(hwndDlg, IDOK), FALSE);

                                MYASSERT(ndwData->ddData.PendingAction == PENDING_ACTION_NONE);

                                ndwData->ddData.PendingAction = PENDING_ACTION_SHOWCLASS;
                                ndwData->ddData.CurSelectionForSuccess = iCur;
                            }

                        } else {

                            if(iCur != LB_ERR) {

                                lvItem.mask = LVIF_TEXT;
                                lvItem.iItem = iCur;
                                lvItem.iSubItem = 0;
                                lvItem.pszText = TempString;
                                lvItem.cchTextMax = SIZECHARS(TempString);

                                if(ListView_GetItem((ndwData->ddData).hwndDrvList, &lvItem)) {
                                    //
                                    // Now retrieve the (case-insensitive) string ID of this
                                    // string, and store it as the current description ID.
                                    //
                                    (ndwData->ddData).iCurDesc = LookUpStringInDevInfoSet((ndwData->ddData).DevInfoSet,
                                                                                          TempString,
                                                                                          FALSE
                                                                                         );
                                }
                            }

                            FillInDeviceList(hwndDlg, &(ndwData->ddData));

                            //
                            // If we just filled in the compatible driver list, then make sure there
                            // aren't isn't a time waiting to pounce and destroy our list!
                            //
                            if((ndwData->ddData.ListType == IDC_NDW_PICKDEV_SHOWCOMPAT) &&
                               (ndwData->idTimer)) {

                                KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
                                ndwData->idTimer = 0;
                            }
                        }
                    }
                    break;

                case IDC_NDW_PICKDEV_HAVEDISK :
                    //
                    // If we're doing a dialog box, then pressing "Have Disk" will popup another
                    // Select Device dialog.  Disable all controls on this one first, to avoid
                    // user confusion.
                    //
                    if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                        ToggleDialogControls(hwndDlg, &(ndwData->ddData), FALSE);
                    }

                    //
                    // If HandleSelectOEM returns success, we are done, and can either end
                    // the dialog, or proceed to the next wizard page.
                    //
                    if(ndwData->ddData.AuxThreadRunning) {
                        //
                        // The auxilliary thread is still running.  Set our cursor to an
                        // hourglass, and set our pending action to be OEM Select while we
                        // wait for the thread to respond to our cancel request.
                        //
                        MYASSERT((ndwData->ddData.PendingAction == PENDING_ACTION_NONE) ||
                                 (ndwData->ddData.PendingAction == PENDING_ACTION_SHOWCLASS));

                        hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                        SetupDiCancelDriverInfoSearch(ndwData->ddData.DevInfoSet);
                        //
                        // Disable all dialog controls, so that no other button may be pressed
                        // until we respond to this pending action.  Also, kill the timer, so
                        // that it doesn't fire in the meantime.
                        //
                        ndwData->bInit = TRUE;
                        if(ndwData->idTimer) {
                            KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
                            ndwData->idTimer = 0;
                        }
                        ndwData->ddData.PendingAction = PENDING_ACTION_OEM;

                        SetCursor(hOldCursor);

                    } else {

                        if(HandleSelectOEM(hwndDlg, &(ndwData->ddData)) == NO_ERROR) {

                            if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                                EndDialog(hwndDlg, NO_ERROR);
                            } else {
                                iwd->Flags |= NDW_INSTALLFLAG_CI_PICKED_OEM;
                                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
                            }

                        } else if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                            //
                            // The user didn't make an OEM selection, so we need to re-enable
                            // the controls on our dialog.
                            //
                            ToggleDialogControls(hwndDlg, &(ndwData->ddData), TRUE);
                        }
                    }
                    break;

                case IDC_NDW_PICKDEV_WINDOWSUPDATE:
                    //
                    // If we're doing a dialog box, then pressing "Have Disk" will popup another
                    // Select Device dialog.  Disable all controls on this one first, to avoid
                    // user confusion.
                    //
                    if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                        ToggleDialogControls(hwndDlg, &(ndwData->ddData), FALSE);
                    }

                    //
                    // If HandleWindowsUpdate returns success, we are done, and can either end
                    // the dialog, or proceed to the next wizard page.
                    //
                    if(ndwData->ddData.AuxThreadRunning) {

                        //
                        // The auxilliary thread is still running.  Set our cursor to an
                        // hourglass, and set our pending action to be Windows Update Select while we
                        // wait for the thread to respond to our cancel request.
                        //
                        MYASSERT((ndwData->ddData.PendingAction == PENDING_ACTION_NONE) ||
                                 (ndwData->ddData.PendingAction == PENDING_ACTION_SHOWCLASS));

                        hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                        SetupDiCancelDriverInfoSearch(ndwData->ddData.DevInfoSet);
                        //
                        // Disable all dialog controls, so that no other button may be pressed
                        // until we respond to this pending action.  Also, kill the timer, so
                        // that it doesn't fire in the meantime.
                        //
                        ndwData->bInit = TRUE;
                        if(ndwData->idTimer) {
                            KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
                            ndwData->idTimer = 0;
                        }
                        ndwData->ddData.PendingAction = PENDING_ACTION_WINDOWSUPDATE;

                        SetCursor(hOldCursor);

                    } else {

                        if(HandleWindowsUpdate(hwndDlg, &(ndwData->ddData)) == NO_ERROR) {

                            if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                                EndDialog(hwndDlg, NO_ERROR);
                            } else {
                                iwd->Flags |= NDW_INSTALLFLAG_CI_PICKED_OEM;
                                PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
                            }

                        } else if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                            //
                            // The user didn't make an OEM selection, so we need to re-enable
                            // the controls on our dialog.
                            //
                            ToggleDialogControls(hwndDlg, &(ndwData->ddData), TRUE);
                        }
                    }
                    break;

                case IDOK :
HandleOK:
                    iCur = (int)ListView_GetNextItem((ndwData->ddData).hwndDrvList,
                                                     -1,
                                                     LVNI_SELECTED
                                                    );
                    if(iCur != LB_ERR) {
                        //
                        // We have retrieved a valid selection from our listbox.
                        //
                        if(ndwData->ddData.AuxThreadRunning) {
                            //
                            // The auxilliary thread is still running.  Set our cursor to an
                            // hourglass, while we wait for the thread to terminate.
                            //
                            MYASSERT((ndwData->ddData.PendingAction == PENDING_ACTION_NONE) ||
                                     (ndwData->ddData.PendingAction == PENDING_ACTION_SHOWCLASS));

                            hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                            SetupDiCancelDriverInfoSearch(ndwData->ddData.DevInfoSet);
                            //
                            // Disable all dialog controls, so that no other button may be pressed
                            // until we respond to this pending action.  Also, kill the timer, so
                            // that it doesn't fire in the meantime.
                            //
                            ToggleDialogControls(hwndDlg, &(ndwData->ddData), FALSE);
                            ndwData->bInit = TRUE;
                            if(ndwData->idTimer) {
                                KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
                                ndwData->idTimer = 0;
                            }
                            ndwData->ddData.PendingAction = PENDING_ACTION_SELDONE;
                            ndwData->ddData.CurSelectionForSuccess = iCur;

                            SetCursor(hOldCursor);

                        } else {
                            //
                            // The auxilliary thread has already returned. We can return
                            // success right here.
                            //
                            SetSelectedDriverNode(&(ndwData->ddData), iCur);
                            EndDialog(hwndDlg, NO_ERROR);
                        }

                    } else {

                        //
                        // If the list box is empty, then just leave. We will treat this
                        // just like the user canceled.
                        //
                        if (0 == ListView_GetItemCount((ndwData->ddData).hwndDrvList)) {

                            PostMessage(hwndDlg, WM_COMMAND, IDCANCEL, 0);

                        } else {

                            //
                            // Tell user to select something since there are items in the list
                            //
                            if(!LoadString(MyDllModuleHandle,
                                           IDS_SELECT_DEVICE,
                                           TempString,
                                           SIZECHARS(TempString))) {
                                *TempString = TEXT('\0');
                            }

                            FormatMessageBox(MyDllModuleHandle,
                                             hwndDlg,
                                             MSG_SELECTDEVICE_ERROR,
                                             TempString,
                                             MB_OK | MB_ICONEXCLAMATION
                                            );
                        }
                    }
                    break;

                case IDCANCEL :

                    if(ndwData->ddData.AuxThreadRunning) {
                        //
                        // The auxilliary thread is running, so we have to ask it to cancel,
                        // and set our pending action to do the cancel upon the thread's
                        // termination notification.
                        //
                        MYASSERT((ndwData->ddData.PendingAction == PENDING_ACTION_NONE) ||
                                 (ndwData->ddData.PendingAction == PENDING_ACTION_SHOWCLASS));

                        hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));

                        SetupDiCancelDriverInfoSearch(ndwData->ddData.DevInfoSet);
                        //
                        // Disable all dialog controls, so that no other button may be pressed
                        // until we respond to this pending action.  Also, kill the timer, so
                        // that it doesn't fire in the meantime.
                        //
                        ToggleDialogControls(hwndDlg, &(ndwData->ddData), FALSE);
                        ndwData->bInit = TRUE;
                        if(ndwData->idTimer) {
                            KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
                            ndwData->idTimer = 0;
                        }
                        ndwData->ddData.PendingAction = PENDING_ACTION_CANCEL;

                        SetCursor(hOldCursor);

                    } else {
                        //
                        // The auxilliary thread isn't running, so we can return right here.
                        //
                        OnCancel(ndwData);
                        EndDialog(hwndDlg, ERROR_CANCELLED);
                    }
                    break;

                default :
                    return FALSE;
            }
            break;

        case WM_NOTIFY :

            switch(((LPNMHDR)lParam)->code) {

                case PSN_SETACTIVE :
                    //
                    // Init the text in set active since a class installer
                    // has the option of replacing it.
                    //
                    SetDlgText(hwndDlg, IDC_NDW_TEXT, IDS_NDW_PICKDEV1, IDS_NDW_PICKDEV1);

                    ndwData->bInit = TRUE;       // Still doing some init stuff

                    if(!OnSetActive(hwndDlg, ndwData)) {
                        SetDlgMsgResult(hwndDlg, uMsg, -1);
                    }

                    ndwData->bInit = FALSE;      // Done with init stuff
                    break;

                case PSN_WIZBACK :
                    if(iwd->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {
                        SetDlgMsgResult(hwndDlg, uMsg, IDD_DYNAWIZ_SELECT_PREVPAGE);
                    } else {
                        SetDlgMsgResult(hwndDlg, uMsg, IDD_DYNAWIZ_SELECTCLASS_PAGE);
                    }
                    break;

                case PSN_WIZNEXT :
                    if(!(iwd->Flags & NDW_INSTALLFLAG_CI_PICKED_OEM)) {

                        iCur = (int)ListView_GetNextItem((ndwData->ddData).hwndDrvList,
                                                         -1,
                                                         LVNI_SELECTED
                                                        );
                        if(iCur != LB_ERR) {
                            //
                            // We have retrieved a valid selection from our listbox.
                            //
                            if (pSetupIsSelectedHardwareIdValid(hwndDlg, &(ndwData->ddData), iCur)) {
                                SetSelectedDriverNode(&(ndwData->ddData), iCur);
                            } else {
                                SetDlgMsgResult(hwndDlg, uMsg, (LRESULT)-1);
                                break;
                            }

                        } else {        // Invalid Listview selection
                            //
                            // Fail the call and end the case
                            //
                            SetDlgMsgResult(hwndDlg, uMsg, (LRESULT)-1);
                            break;
                        }
                    }

                    //
                    // Update the current description in the dialog data so that we'll hi-lite
                    // the correct selection if the user comes back to this page.
                    //
                    (ndwData->ddData).iCurDesc = GetCurDesc(&(ndwData->ddData));

                    if(iwd->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {
                        SetDlgMsgResult(hwndDlg, uMsg, IDD_DYNAWIZ_SELECT_NEXTPAGE);
                    } else {
                        SetDlgMsgResult(hwndDlg, uMsg, IDD_DYNAWIZ_ANALYZEDEV_PAGE);
                    }
                    break;

                case LVN_ITEMCHANGED :
                    //
                    // If the idFrom is the MFG list, then update the Drv list.
                    //
                    if(((((LPNMHDR)lParam)->idFrom) == IDC_NDW_PICKDEV_MFGLIST) && !ndwData->bInit) {

                        if(ndwData->idTimer) {
                            KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
                        }

                        ndwData->idTimer = SetTimer(hwndDlg,
                                                    SELECTMFG_TIMER_ID,
                                                    SELECTMFG_TIMER_DELAY,
                                                    NULL
                                                   );

                        if(ndwData->idTimer == 0) {
                            goto SelectMfgItemNow;
                        }
                    }
                    break;

                case NM_DBLCLK :
                    if(((((LPNMHDR)lParam)->idFrom) == IDC_NDW_PICKDEV_DRVLIST) ||
                       ((((LPNMHDR)lParam)->idFrom) == IDC_NDW_PICKDEV_ONEMFG_DRVLIST)) {

                        if(ndwData->ddData.flags & DD_FLAG_IS_DIALOGBOX) {
                            goto HandleOK;
                        } else {
                            PropSheet_PressButton(GetParent(hwndDlg), PSBTN_NEXT);
                        }
                    }
                    break;
            }

            break;

        case WM_TIMER :
            KillTimer(hwndDlg, SELECTMFG_TIMER_ID);
            ndwData->idTimer = 0;

SelectMfgItemNow:
            iCur = ListView_GetNextItem((ndwData->ddData).hwndMfgList,
                                        -1,
                                        LVNI_SELECTED
                                       );
            if(iCur != -1) {

                RECT rcTo, rcFrom;

                ListView_EnsureVisible((ndwData->ddData).hwndMfgList, iCur, FALSE);
                UpdateWindow((ndwData->ddData).hwndMfgList);

                GetWindowRect((ndwData->ddData).hwndDrvList, &rcTo);
                MapWindowPoints(NULL, hwndDlg, (LPPOINT)&rcTo, 2);

                ListView_GetItemRect((ndwData->ddData).hwndMfgList,
                                     iCur,
                                     &rcFrom,
                                     LVIR_LABEL
                                    );
                MapWindowPoints((ndwData->ddData).hwndMfgList,
                                hwndDlg,
                                (LPPOINT)&rcFrom,
                                2
                               );

                DrawAnimatedRects(hwndDlg, IDANI_OPEN, &rcFrom, &rcTo);
                LockAndShowListForMfg(&(ndwData->ddData), iCur);
            }
            break;

        case WM_SYSCOLORCHANGE :
            _OnSysColorChange(hwndDlg, wParam, lParam);
            break;

        default :

            if (!g_uQueryCancelAutoPlay) {
                g_uQueryCancelAutoPlay = RegisterWindowMessage(TEXT("QueryCancelAutoPlay"));
            }

            if (uMsg == g_uQueryCancelAutoPlay) {
                SetWindowLongPtr( hwndDlg, DWLP_MSGRESULT, 1 );
                return 1;       // cancel auto-play
            }

            return(FALSE);
    }

    return TRUE;
}


VOID
_OnSysColorChange(
    HWND hWnd,
    WPARAM wParam,
    LPARAM lParam
    )
/*++

Routine Description:

    This routine notifies all child windows of the specified window when there is
    a system color change.

Return Value:

    None.

--*/
{
    HWND hChildWnd;

    hChildWnd = GetWindow(hWnd, GW_CHILD);

    while(hChildWnd != NULL) {

        SendMessage(hChildWnd, WM_SYSCOLORCHANGE, wParam, lParam);
        hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);

    }
}


DWORD
FillInDeviceList(
    IN HWND           hwndDlg,
    IN PSP_DIALOGDATA lpdd
    )
/*++

Routine Description:

    This routine sets the dialog to have the appropriate description strings.
    It also alternates dialog between showing the manufacturer "double list"
    and the single list.   This is done by showing/hiding overlapping listview.

    NOTE:  DO NOT CALL THIS ROUTINE TO WHILE ANOTHER THREAD IS BUSY BUILDING A
    CLASS DRIVER LIST.  WE WILL HANG HERE UNTIL THE OTHER THREAD COMPLETES!!!!

Arguments:

    hwndDlg - Supplies the handle of the dialog window.

    lpdd - Supplies the address of a dialog data buffer containing parameters to
        be used in filling in the device list.

Return Value:

    If success, the return value is NO_ERROR, otherwise, it is an ERROR_* code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    PDRIVER_NODE DriverNodeHead, CurDriverNode;
    DWORD DriverNodeType;
    LONG MfgNameId;
    INT i;
    LPTSTR lpszMfg;
    LV_ITEM lviItem;
    BOOL bDidDrvList = FALSE;
    PDEVINSTALL_PARAM_BLOCK dipb;
    DWORD Err = NO_ERROR;
    TCHAR szBuf[LINE_LEN];
    TCHAR szMessage[MAX_INSTRUCTION_LEN];
    TCHAR szText[SDT_MAX_TEXT];
    LPTSTR lpszText;
    LPGUID ClassGuid;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {
            dipb = &(DevInfoElem->InstallParamBlock);
            ClassGuid = &(DevInfoElem->ClassGuid);

            if(lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL) {
                DriverNodeHead = DevInfoElem->ClassDriverHead;
                DriverNodeType = SPDIT_CLASSDRIVER;
            } else {
                DriverNodeHead = DevInfoElem->CompatDriverHead;
                DriverNodeType = SPDIT_COMPATDRIVER;
            }

        } else {
            //
            // We better not be trying to display a compatible driver list if
            // we don't have a devinfo element!
            //
            MYASSERT(lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL);


            dipb = &(pDeviceInfoSet->InstallParamBlock);
            DriverNodeHead = pDeviceInfoSet->ClassDriverHead;
            DriverNodeType = SPDIT_CLASSDRIVER;

            if(pDeviceInfoSet->HasClassGuid) {
                ClassGuid = &(pDeviceInfoSet->ClassGuid);
            } else {
                //
                // Cast away const-ness of this global GUID, since we only use
                // LPGUID type (in fact, there is no 'LPCGUID' type defined).
                //
                ClassGuid = (LPGUID)&GUID_DEVCLASS_UNKNOWN;
            }
        }

        if(!DriverNodeHead) {

            if(!(lpdd->flags & DD_FLAG_IS_DIALOGBOX)) {
                //
                // We can't just go away, so we have to do something useful.  For now, simply
                // display the UI as if we had a single-Mfg list, except that the list is empty.
                //
                // Hide the mult mfg controls
                //
                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLABEL), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLIST), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MODELSLABEL), SW_HIDE);
                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_DRVLIST), SW_HIDE);

                //
                // Show the Single MFG controls
                //
                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_SHOW);
                ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_SHOW);

                //
                // Set the Models string
                //
                if(USE_CI_SELSTRINGS(dipb)) {
                    SetDlgItemText(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL, GET_CI_SELSTRINGS(dipb, ListLabel));
                } else {
                    if(!(LoadString(MyDllModuleHandle,
                                    IDS_NDWSEL_MODELSLABEL,
                                    szBuf,
                                    SIZECHARS(szBuf)))) {
                        szBuf[0] = TEXT('\0');
                    }
                    SetDlgItemText(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL, szBuf);
                }

                //
                // Use the single listbox view for the driver list.
                //
                lpdd->hwndDrvList = GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST);

                ListView_DeleteAllItems(lpdd->hwndDrvList);
            }

            Err = ERROR_DI_BAD_PATH;
            goto clean0;
        }

        if((lpdd->flags & DD_FLAG_IS_DIALOGBOX) && !USE_CI_SELSTRINGS(dipb)) {
            //
            // If a class installer didn't supply strings for us to use in this dialogbox,
            // then retrieve the instruction text to be used.
            //
            // First, get the class description to use for the dialog text.
            //
            if(!SetupDiGetClassDescription(ClassGuid, szBuf, SIZECHARS(szBuf), NULL)) {
                //
                // Fall back to the generic description "device"
                //
                LoadString(MyDllModuleHandle,
                           IDS_GENERIC_DEVNAME,
                           szBuf,
                           SIZECHARS(szBuf)
                          );
            }

            if(lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL) {
                //
                // Show class list.
                //
                if(LoadString(MyDllModuleHandle,
                              IDS_INSTALLSTR1,
                              szMessage,
                              SIZECHARS(szMessage))) {
                    wsprintf(szText, szMessage, szBuf);
                }

            } else {
                //
                // Show compatible list.
                //
                if(LoadString(MyDllModuleHandle,
                              IDS_INSTALLSTR0,
                              szMessage,
                              SIZECHARS(szMessage))) {
                    wsprintf(szText, szMessage, szBuf);
                }
                lpszText = szText + lstrlen(szText);
                if(LoadString(MyDllModuleHandle,
                              IDS_INSTALLCLASS,
                              szMessage,
                              SIZECHARS(szMessage))) {
                    wsprintf(lpszText, szMessage, szBuf);
                }
            }

            if(dipb->DriverPath != -1) {

                lpszText = szText + lstrlen(szText);
                LoadString(MyDllModuleHandle,
                           IDS_INSTALLOEM1,
                           lpszText,
                           SIZECHARS(szText) - lstrlen(szText)
                          );

            } else if (dipb->Flags & DI_SHOWOEM) {

                lpszText = szText + lstrlen(szText);
                LoadString(MyDllModuleHandle,
                           IDS_INSTALLOEM,
                           lpszText,
                           SIZECHARS(szText) - lstrlen(szText)
                          );
            }

            SetDlgItemText(hwndDlg, IDC_NDW_TEXT, szText);
        }

        if((lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL) && (dipb->Flags & DI_MULTMFGS)) {
            //
            // Hide the Single MFG controls
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);

            //
            // Show the Multiple MFG controls
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLABEL), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLIST), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MODELSLABEL), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_DRVLIST), SW_SHOW);

            //
            // Set the colunm heading for the Driver list
            //
            if(USE_CI_SELSTRINGS(dipb)) {
                SetDlgItemText(hwndDlg, IDC_NDW_PICKDEV_MODELSLABEL, GET_CI_SELSTRINGS(dipb, ListLabel));
            } else {
                if(!(LoadString(MyDllModuleHandle,
                                IDS_NDWSEL_MODELSLABEL,
                                szBuf,
                                SIZECHARS(szBuf)))) {
                    szBuf[0] = TEXT('\0');
                }
                SetDlgItemText(hwndDlg, IDC_NDW_PICKDEV_MODELSLABEL, szBuf);
            }

            //
            // Use the 2nd listbox of the Manufacturers/Models view for the driver list.
            //
            lpdd->hwndDrvList = GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_DRVLIST);

#ifndef ANSI_SETUPAPI
            ListView_SetExtendedListViewStyle(lpdd->hwndDrvList, LVS_EX_LABELTIP);
#endif

            //
            // No redraw for faster insert
            //
            SendMessage(lpdd->hwndMfgList, WM_SETREDRAW, FALSE, 0L);

            //
            // Clean out the MFG list before filling it.
            //
            ListView_DeleteAllItems(lpdd->hwndMfgList);

            lviItem.mask = LVIF_TEXT | LVIF_PARAM;
            lviItem.iItem = 0;
            lviItem.iSubItem = 0;

            //
            // Setup the Column Header
            //
            MfgNameId = -1;

            for(CurDriverNode = DriverNodeHead; CurDriverNode; CurDriverNode = CurDriverNode->Next) {

                //
                // Skip this driver node if it is to be excluded of if it is an old INET driver or
                // if it is a BAD driver.
                //
                if ((CurDriverNode->Flags & DNF_OLD_INET_DRIVER) ||
                    (CurDriverNode->Flags & DNF_BAD_DRIVER) ||
                    ((CurDriverNode->Flags & DNF_EXCLUDEFROMLIST) &&
                   !(dipb->FlagsEx & DI_FLAGSEX_ALLOWEXCLUDEDDRVS))) {

                    continue;
                }


                if((MfgNameId == -1) || (MfgNameId != CurDriverNode->MfgName)) {

                    MfgNameId = CurDriverNode->MfgName;

                    MYASSERT(CurDriverNode->MfgDisplayName != -1);
                    lpszMfg = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                       CurDriverNode->MfgDisplayName
                                                      );
                    lviItem.pszText = lpszMfg;

                    lviItem.lParam = (LPARAM)CurDriverNode;
                    i = ListView_InsertItem(lpdd->hwndMfgList, &lviItem);
                }

                //
                // If this driver node is the selected one, preselect here.
                //
                if(lpdd->iCurDesc == CurDriverNode->DevDescription) {
                    ListView_SetItemState(lpdd->hwndMfgList,
                                          i,
                                          (LVIS_SELECTED|LVIS_FOCUSED),
                                          (LVIS_SELECTED|LVIS_FOCUSED)
                                         );
                    ShowListForMfg(lpdd, pDeviceInfoSet, dipb, NULL, i);
                    bDidDrvList = TRUE;
                }
            }

            //
            // Resize the Column
            //
            ListView_SetColumnWidth(lpdd->hwndMfgList, 0, LVSCW_AUTOSIZE_USEHEADER);

            //
            // If we did not expand one of the MFGs by default, then
            // expand the First MFG.
            //
            if(!bDidDrvList) {

                ListView_SetItemState(lpdd->hwndMfgList,
                                      0,
                                      (LVIS_SELECTED|LVIS_FOCUSED),
                                      (LVIS_SELECTED|LVIS_FOCUSED)
                                     );
                ShowListForMfg(lpdd, pDeviceInfoSet, dipb, NULL, 0);

                SendMessage(lpdd->hwndMfgList, WM_SETREDRAW, TRUE, 0L);

            } else {
                //
                // We must set redraw back to true before sending the LVM_ENSUREVISIBLE
                // message, or otherwise, the listbox item may only be partially exposed.
                //
                SendMessage(lpdd->hwndMfgList, WM_SETREDRAW, TRUE, 0L);

                ListView_EnsureVisible(lpdd->hwndMfgList,
                                       ListView_GetNextItem(lpdd->hwndMfgList, -1, LVNI_SELECTED),
                                       FALSE
                                      );
            }

        } else {
            //
            // Hide the mult mfg controls
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLIST), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MODELSLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_DRVLIST), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);

            //
            // Show the Single MFG controls
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_SHOW);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_SHOW);

            //
            // Set the Models string
            //
            if(USE_CI_SELSTRINGS(dipb)) {
                SetDlgItemText(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL, GET_CI_SELSTRINGS(dipb, ListLabel));
            } else {
                if(!(LoadString(MyDllModuleHandle,
                                IDS_NDWSEL_MODELSLABEL,
                                szBuf,
                                SIZECHARS(szBuf)))) {
                    szBuf[0] = TEXT('\0');
                }
                SetDlgItemText(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL, szBuf);
            }

            //
            // Use the single listbox view for the driver list.
            //
            lpdd->hwndDrvList = GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST);

            ShowListForMfg(lpdd, pDeviceInfoSet, dipb, DriverNodeHead, -1);
        }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return Err;
}


VOID
ShowListForMfg(
    IN PSP_DIALOGDATA          lpdd,
    IN PDEVICE_INFO_SET        DeviceInfoSet,
    IN PDEVINSTALL_PARAM_BLOCK InstallParamBlock,
    IN PDRIVER_NODE            DriverNode,        OPTIONAL
    IN INT                     iMfg
    )
/*++

Routine Description:

    This routine builds the driver description list.
    THE LOCK MUST ALREADY BE ACQUIRED BEFORE CALLING THIS ROUTINE!

Arguments:

    lpdd - Supplies the address of a dialog data buffer containing parameters
        to be used in filling in the driver description list.

    DeviceInfoSet - Supplies the address of the device information set structure
        for which the driver description list is to be built.

    InstallParamBlock - Supplies the address of a device installation parameter
        block that controls how the list is displayed.

    DriverNode - Optionally, supplies a pointer to the first node in a driver node
        list to traverse, adding to the list for each node.  If DriverNode
        is not specified, then the list is to be built based on a particular
        manufacturer, whose index in the Manufacturer list is given by iMfg.

    iMfg - Supplies the index within the Manufacturer list that the driver
        description list is to be based on.  This parameter is ignored if a
        DriverNode is specified.

Return Value:

    None.

--*/
{
    INT         i;
    LV_ITEM     lviItem;
    LV_FINDINFO lvfiFind;
    LONG        MfgNameId = -1;
    TCHAR       szTemp[LINE_LEN];
    SYSTEMTIME  SystemTime;
    TCHAR       szDate[LINE_LEN];
    TCHAR       FormatString[LINE_LEN];
    TCHAR       VersionString[LINE_LEN]; // Has format a.b.c.d

    //
    // Set listview sortascending style based on DI_INF_IS_SORTED flag
    //
    SetWindowLong(lpdd->hwndDrvList,
                  GWL_STYLE,
                  (GetWindowLong(lpdd->hwndDrvList, GWL_STYLE) & ~(LVS_SORTASCENDING | LVS_SORTDESCENDING)) |
                      ((InstallParamBlock->Flags & DI_INF_IS_SORTED)
                          ? 0
                          : LVS_SORTASCENDING)
                 );

    SendMessage(lpdd->hwndDrvList, WM_SETREDRAW, FALSE, 0L);

    //
    // Clean out the List.
    //
    ListView_DeleteAllItems(lpdd->hwndDrvList);

    if(!DriverNode) {

        if (ListView_GetItemCount(lpdd->hwndMfgList) > 0) {
        
            lviItem.mask = LVIF_PARAM;
            lviItem.iItem = iMfg;
            lviItem.iSubItem = 0;
            if(!ListView_GetItem(lpdd->hwndMfgList, &lviItem) ||
               !(DriverNode = GetDriverNodeFromLParam(DeviceInfoSet, lpdd, lviItem.lParam))) {

                return;
            }
            MfgNameId = DriverNode->MfgName;
        } else {
            //
            // This means that there are no Manufacturers so we just have a empty list.
            //
            return;
        }
    }

    lviItem.mask = LVIF_TEXT | LVIF_PARAM;
    lviItem.iItem = 0;
    lviItem.iSubItem = 0;

    //
    // Add descriptions to the list
    //
    for( ; DriverNode; DriverNode = DriverNode->Next) {

        if((MfgNameId != -1) && (MfgNameId != DriverNode->MfgName)) {
            //
            // We've gone beyond the manufacturer list--break out of loop.
            //
            break;
        }

        //
        // If this is a special "Don't show me" one, then skip it
        //
        if ((DriverNode->Flags & DNF_OLD_INET_DRIVER) ||
            (DriverNode->Flags & DNF_BAD_DRIVER) ||
            ((DriverNode->Flags & DNF_EXCLUDEFROMLIST) &&
           !(InstallParamBlock->FlagsEx & DI_FLAGSEX_ALLOWEXCLUDEDDRVS))) {

            continue;
        }

        if(DriverNode->Flags & DNF_DUPDESC) {

            lstrcpy(szTemp,
                    pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->DevDescriptionDisplayName)
                   );

            //
            //For drivers with duplicate descriptions add the provider name
            //in parens.
            //
            lstrcat(szTemp, pszSpaceLparen);
            if(DriverNode->ProviderDisplayName != -1) {
                lstrcat(szTemp,
                        pStringTableStringFromId(DeviceInfoSet->StringTable,
                                                 DriverNode->ProviderDisplayName)
                       );
            }
            
            lstrcat(szTemp, pszRparen);
    
        } else if (DriverNode->Flags & DNF_DUPPROVIDER) {

            lstrcpy(szTemp,
                    pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->DevDescriptionDisplayName)
                   );

            //
            //For drivers with duplicate descriptions and providers, add the 
            //driver version and driver date if there is one, in brackets.
            //
            if (DriverNode->DriverVersion != 0) {

                ULARGE_INTEGER Version;

                Version.QuadPart = DriverNode->DriverVersion;
                
                LoadString(MyDllModuleHandle, IDS_VERSION, FormatString, SIZECHARS(FormatString));                                        

                wsprintf(VersionString, FormatString,
                        HIWORD(Version.HighPart), LOWORD(Version.HighPart),
                        HIWORD(Version.LowPart), LOWORD(Version.LowPart));

                lstrcat(szTemp, VersionString);
            }

            if ((DriverNode->DriverDate.dwLowDateTime != 0) ||
                (DriverNode->DriverDate.dwHighDateTime != 0)) {

                if (FileTimeToSystemTime(&(DriverNode->DriverDate), &SystemTime)) {

                    if (GetDateFormat(LOCALE_USER_DEFAULT,
                                      DATE_SHORTDATE,
                                      &SystemTime,
                                      NULL,
                                      FormatString,
                                      SIZECHARS(FormatString)
                                      ) != 0) {

                        wsprintf(szDate, TEXT(" [%s]"), FormatString);

                        lstrcat(szTemp, szDate);
                    }
                }
            }

        } else {

            lstrcpy(szTemp,
                    pStringTableStringFromId(DeviceInfoSet->StringTable,
                                             DriverNode->DevDescriptionDisplayName));
        }

        lviItem.pszText = szTemp;

        lviItem.lParam = (LPARAM)DriverNode;

        if(ListView_InsertItem(lpdd->hwndDrvList, &lviItem) != -1) {
            lviItem.iItem++;
        }
    }

    //
    // Resize the Column
    //
    ListView_SetColumnWidth(lpdd->hwndDrvList, 0, LVSCW_AUTOSIZE_USEHEADER);

    //
    // select the current description string
    //
    if(lpdd->iCurDesc == -1) {
        i = 0;
    } else {
        lvfiFind.flags = LVFI_STRING;
        lvfiFind.psz = pStringTableStringFromId(DeviceInfoSet->StringTable,
                                                lpdd->iCurDesc
                                               );
        i = ListView_FindItem(lpdd->hwndDrvList, -1, &lvfiFind);
        if(i == -1) {
            i = 0;
        }
    }
    ListView_SetItemState(lpdd->hwndDrvList,
                          i,
                          (LVIS_SELECTED|LVIS_FOCUSED),
                          (LVIS_SELECTED|LVIS_FOCUSED)
                         );

    //
    // We must turn redraw back on before sending the LVM_ENSUREVISIBLE message, or
    // otherwise the item may only be partially visible.
    //
    SendMessage(lpdd->hwndDrvList, WM_SETREDRAW, TRUE, 0L);
    ListView_EnsureVisible(lpdd->hwndDrvList, i, FALSE);
}


VOID
LockAndShowListForMfg(
    IN PSP_DIALOGDATA   lpdd,
    IN INT              iMfg
    )
/*++

Routine Description:

    This routine is a wrapper for ShowListForMfg.  It is to be called from points
    where the device information set lock is not already owned (e.g., the dialog
    prop message loop.

Arguments:

    lpdd - Supplies the address of a dialog data buffer containing parameters
        to be used in filling in the driver description list.

    iMfg - Supplies the index within the Manufacturer list that the driver
        description list is to be based on.

Return Value:

    None.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINSTALL_PARAM_BLOCK dipb;
    PDEVINFO_ELEM DevInfoElem;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        dipb = DevInfoElem ? &(DevInfoElem->InstallParamBlock)
                           : &(pDeviceInfoSet->InstallParamBlock);

        ShowListForMfg(lpdd,
                       pDeviceInfoSet,
                       dipb,
                       NULL,
                       iMfg
                      );

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);
}


BOOL
InitSelectDeviceDlg(
    IN     HWND hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    )
/*++

Routine Description:

    This routine initializes the select device wizard page.  It
    builds the class list if it is needed, shows/hides necessary
    controls based on Flags, and comes up with the right text
    description of what's going on.

Arguments:

    hwndDlg - Handle to dialog window

    lpdd - Supplies the address of a dialog data buffer that is
        initialized with information concerning this dialog.

Return Value:

    TRUE if we have at least one driver (compat or class) displayed.
    FALSE if we do not have any drivers (class and compat) displayed.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb;
    SP_DEVINFO_DATA DevInfoData;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    DWORD DriverType = SPDIT_CLASSDRIVER;
    DWORD Err;
    INT ShowWhat;
    LPGUID ClassGuid;
    HICON hicon;
    LV_COLUMN lvcCol;
    BOOL SpawnClassDriverSearch = FALSE;
    PCLASSDRV_THREAD_CONTEXT ClassDrvThreadContext;
    HCURSOR hOldCursor;
    BOOL Return = TRUE;

    if(!lpdd->hwndMfgList) {
        //
        // Then this is the first time we've initialized this dialog (we may hit
        // this routine multiple times in the wizard case, because the user can
        // go back and forth between pages).
        //
        lpdd->hwndMfgList = GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLIST);
        //
        // Don't worry--hwndDrvList will be set later in FillInDeviceList().
        //

#ifndef ANSI_SETUPAPI
        ListView_SetExtendedListViewStyle(lpdd->hwndMfgList, LVS_EX_LABELTIP);
#endif

        //
        // Insert a ListView column for each of the listboxes.
        //
        lvcCol.mask = 0;

        ListView_InsertColumn(lpdd->hwndMfgList, 0, &lvcCol);
        ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_DRVLIST), 0, &lvcCol);
        ListView_InsertColumn(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), 0, &lvcCol);
    }

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {
            dipb = &(DevInfoElem->InstallParamBlock);
            ClassGuid = &(DevInfoElem->ClassGuid);
            //
            // Fill in a SP_DEVINFO_DATA structure for a later call to
            // SetupDiBuildDriverInfoList.
            //
            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                             DevInfoElem,
                                             &DevInfoData
                                            );
            //
            // Set flags indicating which driver lists already exist.
            //
            if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDCOMPATINFO) {
                lpdd->bKeeplpCompatDrvList = TRUE;
            }

            if(DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDINFOLIST) {
                lpdd->bKeeplpClassDrvList = TRUE;
            }

            if(DevInfoElem->SelectedDriver) {
                lpdd->bKeeplpSelectedDrv = TRUE;
            }

            //
            // We want to start out with the compatible driver list.
            //
            DriverType = SPDIT_COMPATDRIVER;

        } else {
            dipb = &(pDeviceInfoSet->InstallParamBlock);
            if(pDeviceInfoSet->HasClassGuid) {
                ClassGuid = &(pDeviceInfoSet->ClassGuid);
            } else {
                //
                // Cast away const-ness of this global GUID, since we only use
                // LPGUID type (in fact, there is no 'LPCGUID' type defined).
                //
                ClassGuid = (LPGUID)&GUID_DEVCLASS_UNKNOWN;
            }

            if(pDeviceInfoSet->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDINFOLIST) {
                lpdd->bKeeplpClassDrvList = TRUE;
            }

            if(pDeviceInfoSet->SelectedClassDriver) {
                lpdd->bKeeplpSelectedDrv = TRUE;
            }
        }

        //
        // Get/set class icon
        //
        SetupDiLoadClassIcon(ClassGuid, &hicon, &(lpdd->iBitmap));
        SendDlgItemMessage(hwndDlg, IDC_CLASSICON, STM_SETICON, (WPARAM)hicon, 0L);

        //
        // If we are supposed to override the instructions and title with the class
        // installer-provided strings, do it now.
        //
        if(USE_CI_SELSTRINGS(dipb)) {

            if(lpdd->flags & DD_FLAG_IS_DIALOGBOX) {
                SetWindowText(hwndDlg, GET_CI_SELSTRINGS(dipb, Title));
            } else {

#ifndef ANSI_SETUPAPI
                // Set wizard title and subtitle
                //
                PropSheet_SetHeaderTitle(GetParent(hwndDlg),
                        PropSheet_HwndToIndex(GetParent(hwndDlg), hwndDlg),
                        GET_CI_SELSTRINGS(dipb, Title));

                PropSheet_SetHeaderSubTitle(GetParent(hwndDlg),
                        PropSheet_HwndToIndex(GetParent(hwndDlg), hwndDlg),
                        GET_CI_SELSTRINGS(dipb, SubTitle));
#endif
            }
            SetDlgItemText(hwndDlg, IDC_NDW_TEXT, GET_CI_SELSTRINGS(dipb, Instructions));
        }

        //
        // If we should not allow OEM driver, then hide the HAVE disk button.
        //
        if(!(dipb->Flags & DI_SHOWOEM)) {
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_HAVEDISK), SW_HIDE);
        }

#ifdef UNICODE
        //
        // Hide the Windows Update button if we should not search the web.
        //
        if ((!(dipb->FlagsEx & DI_FLAGSEX_SHOWWINDOWSUPDATE)) ||
            (CDMIsInternetAvailable() == FALSE)) {

            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_WINDOWSUPDATE), SW_HIDE);
        }
#else

        ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_WINDOWSUPDATE), SW_HIDE);
#endif

        //
        // In order to decrease the amount of time the user must wait before they're able
        // to work with the Select Device dialog, we have adopted a 'hybrid' multi-threaded
        // approach.  As soon as we get the first displayable list built, then we will return,
        // and build the other list (if necessary) in another thread.
        //
        // We do it this way because it's easier, it maintains the existing external behavior,
        // and because it's easier.
        //
        hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));  // Potentially slow operations ahead!

        if(DriverType == SPDIT_COMPATDRIVER) {

            //
            // OR in the DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS flag so that we don't include
            // old internet drivers in the list that we get back.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(lpdd->DevInfoSet,
                                              &DevInfoData,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;

                SetupDiSetDeviceInstallParams(lpdd->DevInfoSet,
                                              &DevInfoData,
                                              &DeviceInstallParams
                                              );
            }

            SetupDiBuildDriverInfoList(lpdd->DevInfoSet,
                                       &DevInfoData,
                                       SPDIT_COMPATDRIVER
                                      );

            //
            // Verify that there are some devices in the list to show.
            //
            if(bNoDevsToShow(DevInfoElem)) {
                if(!lpdd->bKeeplpCompatDrvList) {
                    SetupDiDestroyDriverInfoList(lpdd->DevInfoSet, &DevInfoData, SPDIT_COMPATDRIVER);
                }
                DriverType = SPDIT_CLASSDRIVER;

            } else if(!lpdd->bKeeplpClassDrvList) {
                //
                // We have a list to get our UI up and running, but we don't have a class driver
                // list yet.  Set a flag that causes us to spawn a thread for this later.
                //
                SpawnClassDriverSearch = TRUE;
            }
        }

        if(DriverType == SPDIT_CLASSDRIVER) {
            //
            // We couldn't find any compatible drivers, so we fall back on the class driver
            // list.  In this case we have to have this list before continuing.  In the
            // future, maybe we'll get fancier and do this in a separate thread, but for now,
            // we just make the user wait.
            //

            //
            // OR in the DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS flag so that we don't include
            // old internet drivers in the list that we get back.
            //
            DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
            if (SetupDiGetDeviceInstallParams(lpdd->DevInfoSet,
                                              DevInfoElem ? &DevInfoData : NULL,
                                              &DeviceInstallParams
                                              ))
            {
                DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;

                SetupDiSetDeviceInstallParams(lpdd->DevInfoSet,
                                              DevInfoElem ? &DevInfoData : NULL,
                                              &DeviceInstallParams
                                              );
            }

            SetupDiBuildDriverInfoList(lpdd->DevInfoSet,
                                       DevInfoElem ? &DevInfoData : NULL,
                                       SPDIT_CLASSDRIVER
                                      );
        }

        SetCursor(LoadCursor(NULL, IDC_ARROW));  // Done with slow operations.

        if(DriverType == SPDIT_COMPATDRIVER) {
            //
            // Since we ran this through bNoDevsToShow() above, and it succeeded, we know
            // there's at least one driver in the compatible driver list.
            //
            // (NOTE: We don't call the class installer with a DIF_SELECTBESTCOMPATDRV
            // here.  The assumption is that any class installer that wants to get in
            // the act should check for the DI_FLAGSEX_AUTOSELECTRANK0, and try to select
            // a driver _before_ going ahead and letting the default behavior of
            // SetupDiSelectDevice occur.)
            //
            if((dipb->FlagsEx & DI_FLAGSEX_AUTOSELECTRANK0) &&
               !(DevInfoElem->CompatDriverHead->Rank)) {

                if(!SetupDiSelectBestCompatDrv(lpdd->DevInfoSet, &DevInfoData)) {
                    //
                    // The only time this failure should happen is if the device is
                    // a member of a set with an associated class, and the best driver
                    // match we found is from an INF of a different class.
                    //
                    if(lpdd->flags & DD_FLAG_IS_DIALOGBOX) {
                        EndDialog(hwndDlg, GetLastError());
                        goto clean0;
                    }
                }

                //
                // No need to spawn a class driver search thread.
                //
                SpawnClassDriverSearch = FALSE;

                EndDialog(hwndDlg, NO_ERROR);
                goto clean0;
            }
            lpdd->ListType = IDC_NDW_PICKDEV_SHOWCOMPAT;
            CheckRadioButton(hwndDlg,
                             IDC_NDW_PICKDEV_SHOWCOMPAT,
                             IDC_NDW_PICKDEV_SHOWALL,
                             IDC_NDW_PICKDEV_SHOWCOMPAT
                            );
        } else {
            //
            // There is no compatible list, so hide the radio buttons.
            //
            lpdd->ListType = IDC_NDW_PICKDEV_SHOWALL;
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWCOMPAT), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), SW_HIDE);
        }

        //
        // Initial current description.  This will be used to set
        // the Default ListView selection.
        //
        if(lpdd->iCurDesc == -1) {
            //
            // If we already have a selected driver for the devinfo set or element,
            // then we'll use that, otherwise, we'll use the devinfo element's
            // description (if applicable).
            //
            if(DevInfoElem) {
                if(DevInfoElem->SelectedDriver) {
                    lpdd->iCurDesc = DevInfoElem->SelectedDriver->DevDescription;
                } else {

                    TCHAR TempString[LINE_LEN];
                    ULONG TempStringSize;
                    //
                    // Use the caller-supplied device description, if there is one.
                    // If not, then see if we can retrieve the DeviceDesc registry
                    // property.
                    //
                    TempStringSize = sizeof(TempString);

                    if((DevInfoElem->DeviceDescription == -1) &&
                       (CM_Get_DevInst_Registry_Property(DevInfoElem->DevInst,
                                                         CM_DRP_DEVICEDESC,
                                                         NULL,
                                                         TempString,
                                                         &TempStringSize,
                                                         0) == CR_SUCCESS)) {
                        //
                        // We were able to retrieve a device description.  Now store it
                        // (case-insensitive only) in the devinfo element.
                        //
                        DevInfoElem->DeviceDescription = pStringTableAddString(
                                                           pDeviceInfoSet->StringTable,
                                                           TempString,
                                                           STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                                           NULL,0
                                                           );
                    }

                    lpdd->iCurDesc = DevInfoElem->DeviceDescription;
                }
            } else {
                if(pDeviceInfoSet->SelectedClassDriver) {
                    lpdd->iCurDesc = pDeviceInfoSet->SelectedClassDriver->DevDescription;
                }
            }
        }

        Err = FillInDeviceList(hwndDlg, lpdd);

        if(lpdd->flags & DD_FLAG_IS_DIALOGBOX) {

            HWND hLineWnd;
            RECT Rect;

            //
            // If FillInDeviceList() fails during init time, don't even bring up the dialog.
            //
            if(Err != NO_ERROR) {
                EndDialog(hwndDlg, Err);
                goto clean0;
            }

            //
            // Set the initial focus on the OK button.
            //
            SetFocus(GetDlgItem(hwndDlg, IDOK));

            //
            // Use the fancy etched frame style for the separator bar in the dialog.
            //
            hLineWnd = GetDlgItem(hwndDlg, IDD_DEVINSLINE);
            SetWindowLong(hLineWnd,
                          GWL_EXSTYLE,
                          (GetWindowLong(hLineWnd, GWL_EXSTYLE) | WS_EX_STATICEDGE)
                         );
            GetClientRect(hLineWnd, &Rect);
            SetWindowPos(hLineWnd,
                         HWND_TOP,
                         0,
                         0,
                         Rect.right,
                         GetSystemMetrics(SM_CYEDGE),
                         SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED
                        );
        }

        //
        // If DriverType is SPDIT_CLASSDRIVER and ListView_GetItemCount returns 0 then
        // we don't have any items to show at all. This will only happen when we search
        // the default Windows INF directory and we do not have any INFs for a device.
        //
        if ((DriverType == SPDIT_CLASSDRIVER) &&
            (0 == ListView_GetItemCount(lpdd->hwndDrvList))) {

            TCHAR TempString[LINE_LEN];

            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_MODELSLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_ONEMFG_DRVLIST), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MFGLIST), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_MODELSLABEL), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_DRVLIST), SW_HIDE);

            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_SHOW);

            LoadString(MyDllModuleHandle, IDS_NDW_NO_DRIVERS, TempString, SIZECHARS(TempString));
            SetDlgItemText(hwndDlg, IDC_NDW_STATUS_TEXT, TempString);

            Return = FALSE;
        }


clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // If we're doing the dialog box version, then an exception should cause us to
        // terminate the dialog and return an error.  Note that presently, the dialog
        // case is the only time we'll try to spawn off a driver search thread, so we're
        // safe in resetting that flag here as well.
        //
        if(lpdd->flags & DD_FLAG_IS_DIALOGBOX) {
            EndDialog(hwndDlg, ERROR_INVALID_DATA);
            SpawnClassDriverSearch = FALSE;
        }
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    if(SpawnClassDriverSearch) {
        //
        // Allocate a context structure to pass to the auxilliary thread (the auxilliary
        // thread will take care of freeing the memory).
        //
        if(!(ClassDrvThreadContext = MyMalloc(sizeof(CLASSDRV_THREAD_CONTEXT)))) {
            EndDialog(hwndDlg, ERROR_NOT_ENOUGH_MEMORY);
        } else {
            //
            // Fill in the context structure, and fire off the thread.  NOTE: The DevInfoData
            // struct has to have been filled in above for us to have gotten to this point.
            //
            ClassDrvThreadContext->DeviceInfoSet = lpdd->DevInfoSet;

            CopyMemory(&(ClassDrvThreadContext->DeviceInfoData),
                       &DevInfoData,
                       sizeof(DevInfoData)
                      );

            ClassDrvThreadContext->NotificationWindow = hwndDlg;

            if(_beginthread(ClassDriverSearchThread, 0, ClassDrvThreadContext) == -1) {
                //
                // Assume out-of-memory
                //
                MyFree(ClassDrvThreadContext);
                EndDialog(hwndDlg, ERROR_NOT_ENOUGH_MEMORY);
            } else {
                lpdd->AuxThreadRunning = TRUE;
            }
        }
    }

    return Return;
}


PDRIVER_NODE
GetDriverNodeFromLParam(
    IN PDEVICE_INFO_SET DeviceInfoSet,
    IN PSP_DIALOGDATA   lpdd,
    IN LPARAM           lParam
    )
/*++

Routine Description:

    This routine interprets lParam as a pointer to a driver node, and tries to
    find the node in the class driver list for either the selected devinfo element,
    or the set itself.  If the lpdd flags field has the DD_FLAG_USE_DEVINFO_ELEM bit
    set, then the lpdd's DevInfoElem will be used instead of the currently selected
    device.

Arguments:

    DeviceInfoSet - Supplies the address of the device information set structure
        to search for the driver node in.

    lpdd - Supplies the address of a dialog data structure that specifies whether
        the wizard has an explicit association to the global class driver list or
        to a particular device information element, and if so, what it's associated
        with.

    lParam - Supplies a value which may be the address of a driver node.  The
        appropriate linked list of driver nodes is searched to see if one of them
        has this value as its address, and if so, a pointer to that driver node is
        returned.

Return Value:

    If success, the return value is the address of the matching driver node, otherwise,
    it is NULL.

--*/
{
    PDRIVER_NODE CurDriverNode;
    PDEVINFO_ELEM DevInfoElem;

    if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
        DevInfoElem = lpdd->DevInfoElem;
    } else {
        DevInfoElem = DeviceInfoSet->SelectedDevInfoElem;
    }

    if(DevInfoElem) {
        CurDriverNode = (lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL) ? DevInfoElem->ClassDriverHead
                                                                    : DevInfoElem->CompatDriverHead;
    } else {
        MYASSERT(lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL);
        CurDriverNode = DeviceInfoSet->ClassDriverHead;
    }

    while(CurDriverNode) {
        if(CurDriverNode == (PDRIVER_NODE)lParam) {
            return CurDriverNode;
        } else {
            CurDriverNode = CurDriverNode->Next;
        }
    }

    return NULL;
}


BOOL
OnSetActive(
    IN     HWND            hwndDlg,
    IN OUT PNEWDEVWIZ_DATA ndwData
    )
/*++

Routine Description:

    This routine handles the PSN_SETACTIVE message of the select device wizard
    page.

Arguments:

    hwndDlg - Supplies the window handle of the wizard dialog page.

    ndwData - Supplies the address of a New Device Wizard data block to be used
        during the processing of this message.

Return Value:

    If success, the return value is TRUE, otherwise, it is FALSE.

--*/
{
    BOOL b = TRUE;
    PSP_INSTALLWIZARD_DATA iwd;
    PSP_DIALOGDATA lpdd;
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb;

    iwd = &(ndwData->InstallData);
    lpdd = &(ndwData->ddData);

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    //
    // Make sure our "waiting for class list" static text control is hidden!
    //
    ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_STATUS_TEXT), SW_HIDE);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {
            dipb = &(DevInfoElem->InstallParamBlock);
        } else {
            dipb = &(pDeviceInfoSet->InstallParamBlock);
        }

        //
        // Set the Button State
        //
        if((iwd->Flags & NDW_INSTALLFLAG_SKIPCLASSLIST) &&
           (iwd->Flags & NDW_INSTALLFLAG_EXPRESSINTRO) &&
           !(iwd->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED)) {
            //
            // No back if we skipped the Class list, and are in express mode
            //
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_NEXT);
        } else {
            PropSheet_SetWizButtons(GetParent(hwndDlg), PSWIZB_BACK | PSWIZB_NEXT);
        }

        //
        // Set the New Class install params.
        // If we are being jumped to by a dyna wiz page,
        // then do not call the class installer
        //
        if(iwd->DynamicPageFlags & DYNAWIZ_FLAG_PAGESADDED) {
            InitSelectDeviceDlg(hwndDlg, lpdd);
        } else {

            BOOL FlagNeedsReset = FALSE;
            SP_DEVINFO_DATA DeviceInfoData;
            DWORD CiErr;
            PDEVINFO_ELEM CurDevInfoElem;

            //
            // Call the Class Installer
            //
            if(!(dipb->Flags & DI_NODI_DEFAULTACTION)) {
                dipb->Flags |= DI_NODI_DEFAULTACTION;
                FlagNeedsReset = TRUE;
            }

            if(DevInfoElem) {
                //
                // Initialize a SP_DEVINFO_DATA buffer to use as an argument to
                // SetupDiCallClassInstaller.
                //
                DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                 DevInfoElem,
                                                 &DeviceInfoData
                                                );
            }

            //
            // We need to unlock the HDEVINFO before calling the class installer.
            //
            UnlockDeviceInfoSet(pDeviceInfoSet);
            pDeviceInfoSet = NULL;

            if(_SetupDiCallClassInstaller(DIF_SELECTDEVICE,
                                          lpdd->DevInfoSet,
                                          DevInfoElem ? &DeviceInfoData : NULL,
                                          CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS)) {
                CiErr = NO_ERROR;
            } else {
                CiErr = GetLastError();
            }

            //
            // Re-acquire the HDEVINFO lock.
            //
            pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
            MYASSERT(pDeviceInfoSet);

            if(DevInfoElem && !(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM)) {
                //
                // Verify that the class installer didn't do something nasty like delete
                // the currently selected devinfo element!
                //
                for(CurDevInfoElem = pDeviceInfoSet->DeviceInfoHead;
                    CurDevInfoElem;
                    CurDevInfoElem = CurDevInfoElem->Next) {

                    if(CurDevInfoElem = DevInfoElem) {
                        break;
                    }
                }

                if(!CurDevInfoElem) {
                    //
                    // The class installer deleted the selected devinfo element.  Get
                    // the newly-selected one, or fall back to the global driver list
                    // if none selected.
                    //
                    if(DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem) {
                        dipb = &(DevInfoElem->InstallParamBlock);
                    } else {
                        dipb = &(pDeviceInfoSet->InstallParamBlock);
                    }

                    //
                    // Don't need to reset the default action flag.
                    //
                    FlagNeedsReset = FALSE;
                }
            }

            //
            // Reset the DI_NODI_DEFAULTACTION flag if necessary.
            //
            if(FlagNeedsReset) {
                dipb->Flags &= ~DI_NODI_DEFAULTACTION;
            }

            switch(CiErr) {
                //
                // Class installer did the select, so goto analyze
                //
                case NO_ERROR :

                    SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DYNAWIZ_ANALYZEDEV_PAGE);
                    break;

                //
                // Class installer wants us to do default.
                //
                case ERROR_DI_DO_DEFAULT :

                    InitSelectDeviceDlg(hwndDlg, lpdd);
                    break;

                default :
                    //
                    // If we are doing an OEM select, and we fail, then
                    // we should init after clearing the OEM stuff.
                    //
                    if(iwd->Flags & NDW_INSTALLFLAG_CI_PICKED_OEM) {

                        iwd->Flags &= ~NDW_INSTALLFLAG_CI_PICKED_OEM;

                        //
                        // Destroy the existing class driver list.
                        //
                        if(DevInfoElem) {
                            //
                            // Initialize a SP_DEVINFO_DATA buffer to use as an argument to
                            // SetupDiDestroyDriverInfoList.
                            //
                            DeviceInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                             DevInfoElem,
                                                             &DeviceInfoData
                                                            );
                        }

                        SetupDiDestroyDriverInfoList(lpdd->DevInfoSet,
                                                     DevInfoElem ? &DeviceInfoData : NULL,
                                                     SPDIT_CLASSDRIVER
                                                    );

                        //
                        // Make sure the OEM button is shown.
                        //
                        dipb->Flags |= DI_SHOWOEM;

                        InitSelectDeviceDlg(hwndDlg, lpdd);

                    } else {
                        SetWindowLongPtr(hwndDlg, DWLP_MSGRESULT, IDD_DYNAWIZ_SELECTCLASS_PAGE);
                    }
                    break;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        b = FALSE;
        //
        // Reference the following variable so that the compiler will respect ordering w.r.t.
        // assignment.
        //
        pDeviceInfoSet = pDeviceInfoSet;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return b;
}

BOOL
pSetupIsSelectedHardwareIdValid(
    IN HWND           hWnd,
    IN PSP_DIALOGDATA lpdd,
    IN INT            iCur
    )
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDRIVER_NODE DriverNode;
    LV_ITEM lviItem;
    PDEVINFO_ELEM DevInfoElem;
    TCHAR Title[LINE_LEN];
    TCHAR Message[SDT_MAX_TEXT];
    UINT MessageLen;
    BOOL bRet = FALSE;
    TCHAR IDBuffer[REGSTR_VAL_MAX_HCID_LEN];
    ULONG IDBufferLen;
    PTSTR pID, SelectedDriverHardwareId, SelectedDriverCompatibleId;
    DWORD i, j;
    int FailCount = 0;

    //
    //If the DD_FLAG_USE_DEVINFO_ELEM flag is not set then this device did not have
    //we don't have a DevNode to validate the choosen ID against...so just return
    //TRUE.
    //
    if (!(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM))
        return TRUE;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        //
        // We won't verify the selected driver for certain classes of devices. This is 
        // usually because the device is not very Plug and Play and so we can't tell
        // what is a valid driver and what isn't a valid driver.  Currently the
        // list of classes that we don't verify are:
        //
        //  Monitor
        //
        if (DevInfoElem &&
            (IsEqualGUID(&GUID_DEVCLASS_MONITOR, &(DevInfoElem->ClassGuid)))) {
            return TRUE;
        }

        lviItem.mask = LVIF_PARAM;
        lviItem.iItem = iCur;
        lviItem.iSubItem = 0;

        if(ListView_GetItem(lpdd->hwndDrvList, &lviItem)) {
            DriverNode = GetDriverNodeFromLParam(pDeviceInfoSet, lpdd, lviItem.lParam);
        } else {
            DriverNode = NULL;
        }

        if (DriverNode) {

            //
            //Retrieve the HardwareId for the selected driver.
            //
            SelectedDriverHardwareId = pStringTableStringFromId(pDeviceInfoSet->StringTable,
                                                                DriverNode->HardwareId
                                                               );

            for (i=0; !bRet, i<2; i++) {

                IDBufferLen = sizeof(IDBuffer);
                if (CM_Get_DevInst_Registry_Property_Ex(
                        DevInfoElem->DevInst,
                        (i ? CM_DRP_COMPATIBLEIDS : CM_DRP_HARDWAREID),
                        NULL,
                        IDBuffer,
                        &IDBufferLen,
                        0,
                        pDeviceInfoSet->hMachine) == CR_SUCCESS) {

                    //
                    //See if the HardwareID for the selected driver matches any of the
                    //Hardware or Compatible IDs for the actual hardware.
                    //
                    for (pID = IDBuffer; !bRet, *pID; pID += (lstrlen(pID) + 1)) {

                        if (lstrcmpi(SelectedDriverHardwareId, pID) == 0) {

                            bRet = TRUE;
                            break;
                        }
                    }

                    //
                    //See if any of the Compatible IDs for the selceted driver matches any
                    //of the Hardware or Compatible IDs for the actual hardware
                    //
                    for (j=0; !bRet, j<DriverNode->NumCompatIds; j++) {

                        SelectedDriverCompatibleId = pStringTableStringFromId(
                                        pDeviceInfoSet->StringTable,
                                        DriverNode->CompatIdList[j]);

                        for (pID = IDBuffer; !bRet, *pID; pID += (lstrlen(pID) + 1)) {

                            if (lstrcmpi(SelectedDriverCompatibleId, pID) == 0) {

                                bRet = TRUE;
                                break;
                            }
                        }
                    }

                } else {

                    FailCount++;
                }
            }

            //
            //If FailCount is 2 then CM_Get_DevInst_Registry_Property_Ex failed for both
            //CM_DRP_HARDWAREID and CM_DRP_COMPATIBLEIDS.  Since this DevNode does not
            //have any Hardware or Compatible IDs we will let the user install any
            //driver they want on this device.  This usually happens only with manually
            //installed devices.
            //
            if (FailCount == 2) {

                bRet = TRUE;
            }

            //
            //The ID of the driver that the user selected does not match any of the Hardware or
            //Compatible IDs of this device. Warn the user that this is a bad thing and see if they
            //want to continue.
            //
            if (!bRet) {

                LoadString(MyDllModuleHandle, IDS_DRIVER_UPDATE_TITLE, Title, LINE_LEN);

                MessageLen  = LoadString(MyDllModuleHandle, IDS_DRIVER_NOMATCH1, Message, SIZECHARS(Message));
                MessageLen += LoadString(MyDllModuleHandle, IDS_DRIVER_NOMATCH2, Message + MessageLen, SIZECHARS(Message) - MessageLen);
                MessageLen += LoadString(MyDllModuleHandle, IDS_DRIVER_NOMATCH3, Message + MessageLen, SIZECHARS(Message) - MessageLen);

                if(MessageLen) {

                    if (MessageBox(hWnd, Message, Title, MB_YESNO | MB_TASKMODAL | MB_ICONEXCLAMATION |
                            MB_DEFBUTTON2) == IDYES) {

                        bRet = TRUE;
                    }
                }
            }
        } else {

            //
            //I am not sure how we could get here and not have a DRIVER_NODE for the selected device.
            //Maybe a different error message needs to be displayed here.
            //
            bRet = TRUE;
        }

//clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return bRet;
}

VOID
SetSelectedDriverNode(
    IN PSP_DIALOGDATA lpdd,
    IN INT            iCur
    )
/*++

Routine Description:

    This routine sets the selected driver for the currently selected device (or global
    class driver list if no device selected) in the device information set referenced
    in the SP_DIALOGDATA structure.  If the DD_FLAG_USE_DEVINFO_ELEM flag in the
    structure is set, then the driver is selected for the set or element based on the
    DevInfoElem pointer instead of the currently selected one.

Arguments:

    lpdd - Supplies the address of a dialog data structure that contains information
        about the device information set being used.

    iCur - Supplies the index within the driver listbox window containing the driver
        to be selected.

Return Value:

    None.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDRIVER_NODE DriverNode;
    LV_ITEM lviItem;
    PDEVINFO_ELEM DevInfoElem;
    TCHAR ClassGuidString[GUID_STRING_LEN];
    SP_DEVINFO_DATA DevInfoData;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        lviItem.mask = LVIF_PARAM;
        lviItem.iItem = iCur;
        lviItem.iSubItem = 0;

        if(ListView_GetItem(lpdd->hwndDrvList, &lviItem)) {
            DriverNode = GetDriverNodeFromLParam(pDeviceInfoSet, lpdd, lviItem.lParam);
        } else {
            DriverNode = NULL;
        }

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {
            //
            // If a driver node is being selected, update the device's class to ensure
            // it matches the class of the INF containing the selected driver node.
            //
            if(DriverNode) {
                //
                // Get the INF class GUID for this driver node in string form, because
                // this property is stored as a REG_SZ.
                //
                pSetupStringFromGuid(&(pDeviceInfoSet->GuidTable[DriverNode->GuidIndex]),
                                     ClassGuidString,
                                     SIZECHARS(ClassGuidString)
                                    );

                //
                // Fill in a SP_DEVINFO_DATA structure for the upcoming call to
                // SetupDiSetDeviceRegistryProperty.
                //
                DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
                DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                                 DevInfoElem,
                                                 &DevInfoData
                                                );

                if(!SetupDiSetDeviceRegistryProperty(lpdd->DevInfoSet,
                                                     &DevInfoData,
                                                     SPDRP_CLASSGUID,
                                                     (PBYTE)ClassGuidString,
                                                     sizeof(ClassGuidString))) {
                    //
                    // The class cannot be updated--don't change the selected driver.
                    //
                    goto clean0;
                }
            }

            DevInfoElem->SelectedDriver = DriverNode;
            if(DriverNode) {
                DevInfoElem->SelectedDriverType = (lpdd->ListType == IDC_NDW_PICKDEV_SHOWALL)
                                                      ? SPDIT_CLASSDRIVER
                                                      : SPDIT_COMPATDRIVER;
            } else {
                DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
            }
        } else {
            pDeviceInfoSet->SelectedClassDriver = DriverNode;
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);
}


DWORD
HandleSelectOEM(
    IN     HWND           hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    )
/*++

Routine Description:

    This routine selects a new device based on a user-supplied path.  Calling this
    routine may cause a driver list to get built, which is a potentially slow operation.

Arguments:

    hwndDlg - Supplies the window handle of the select device wizard page.

    lpdd - Supplies the address of a dialog data structure that contains information
        used in device selection.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an ERROR_* code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    SP_DEVINFO_DATA DevInfoData;
    DWORD Err;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        //
        // If this is for a particular device, then initialize a device
        // information structure to use for SelectOEMDriver.
        //
        if(DevInfoElem) {

            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                             DevInfoElem,
                                             &DevInfoData
                                            );
        }

        //
        // Unlock the device information set before popping up the OEM Driver Selection
        // UI.  Otherwise, our multi-threaded dialog will deadlock.
        //
        UnlockDeviceInfoSet(pDeviceInfoSet);
        pDeviceInfoSet = NULL;

        if((Err = SelectOEMDriver(hwndDlg,
                                  lpdd->DevInfoSet,
                                  DevInfoElem ? &DevInfoData : NULL,
                                  !(lpdd->flags & DD_FLAG_IS_DIALOGBOX)
                                 )) == ERROR_DI_DO_DEFAULT) {
            //
            // Fill in the list to select from
            //
            lpdd->ListType = IDC_NDW_PICKDEV_SHOWALL;
            CheckRadioButton(hwndDlg,
                             IDC_NDW_PICKDEV_SHOWCOMPAT,
                             IDC_NDW_PICKDEV_SHOWALL,
                             IDC_NDW_PICKDEV_SHOWALL
                            );

            FillInDeviceList(hwndDlg, lpdd);

            //
            // Since we only show the class list when selecting from an OEM list, hide
            // the selection buttons.
            //
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWCOMPAT), SW_HIDE);
            ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), SW_HIDE);
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = GetLastError();
        //
        // Access the pDeviceInfoSet variable so that the compiler will respect our statement
        // ordering w.r.t. this value.
        //
        pDeviceInfoSet = pDeviceInfoSet;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return Err;
}

#ifdef UNICODE

DWORD
SelectWindowsUpdateDriver(
    IN     HWND             hwndParent,     OPTIONAL
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData, OPTIONAL
    IN     PTSTR            CDMPath,
    IN     BOOL             IsWizard
    )
/*++

Routine Description:

    This is the worker routine that actually allows for the selection of an Windows Update driver.

Arguments:

    hwndParent - Optionally, supplies the window handle that is to be the parent for any
        selection UI.  If this parameter is not supplied, then the hwndParent field of
        the devinfo set or element will be used.

    DeviceInfoSet - Supplies the handle of the device info set for which an Windows Update driver
        selection is to be performed.

    DeviceInfoData - Optionally, supplies the address of the device information element to
        select a driver for.  If this parameter is not supplied, then an OEM driver for
        the global class driver list will be selected.

        If a compatible driver was found for this device, the device information element
        will have its class GUID updated upon return to reflect the device's new class.

    IsWizard - Specifies whether this routine is being called in the context of a select
        device wizard page.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an ERROR_* code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PDEVINSTALL_PARAM_BLOCK dipb;
    DWORD Err = NO_ERROR;
    HWND hwndSave;
    LONG DriverPathSave;
    DWORD DriverPathFlagsSave;
    BOOL bRestoreHwnd = FALSE, bRestoreDriverPath = FALSE, bUnlockDevInfoElem = FALSE;
    UINT NewClassDriverCount;
    TCHAR Title[MAX_TITLE_LEN];
    DWORD SavedFlags;
    DWORD SavedFlagsEx;

    PDRIVER_NODE lpOrgCompat;
    PDRIVER_NODE lpOrgCompatTail;
    UINT         OrgCompatCount;
    PDRIVER_NODE lpOrgClass;
    PDRIVER_NODE lpOrgClassTail;
    UINT         OrgClassCount;
    PDRIVER_NODE lpOrgSel;
    DWORD        dwOrgSelType;
    DWORD        dwOrgFlags;
    DWORD        dwOrgFlagsEx;
    BOOL         bRestoreDeviceInfo = FALSE;
    LONG         DriverPathId;

    pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);

    try {

        if(DeviceInfoData) {
            //
            // Then we're working with a particular device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // If the DevInfoElem isn't already locked, then lock it now, because
            // we're going to be calling the class installer, and we don't want to
            // allow it to delete this element!
            //
            if(!(DevInfoElem->DiElemFlags & DIE_IS_LOCKED)) {
                DevInfoElem->DiElemFlags |= DIE_IS_LOCKED;
                bUnlockDevInfoElem = TRUE;
            }

            dipb = &(DevInfoElem->InstallParamBlock);

        } else {
            dipb = &(pDeviceInfoSet->InstallParamBlock);
        }

        //
        // Make this selection window the parent window for Windows Update stuff
        //
        if(hwndParent) {
            hwndSave = dipb->hwndParent;
            dipb->hwndParent = hwndParent;
            bRestoreHwnd = TRUE;
        }

        //
        // Don't assume there is no old path.  Save old one and
        // pretend there is no old one in case of cancel.
        //
        DriverPathSave = dipb->DriverPath;
        dipb->DriverPath = -1;

        //
        // Clear the DI_ENUMSINGLEINF flag, because we're going to be getting
        // a path to a directory, _not_ to an individual INF.  Also, clear the
        // DI_COMPAT_FROM_CLASS flag, because we don't want to build the compatible
        // driver list based on any class driver list.
        //
        DriverPathFlagsSave = dipb->Flags & (DI_ENUMSINGLEINF | DI_COMPAT_FROM_CLASS);
        dipb->Flags &= ~(DI_ENUMSINGLEINF | DI_COMPAT_FROM_CLASS);
        bRestoreDriverPath = TRUE;

        //
        // Set the DriverPath to the path returned by CDM
        //
        if((DriverPathId = pStringTableAddString(
                                      pDeviceInfoSet->StringTable,
                                      CDMPath,
                                      STRTAB_CASE_INSENSITIVE | STRTAB_BUFFER_WRITEABLE,
                                      NULL,0)) == -1) {

            Err = ERROR_NOT_ENOUGH_MEMORY;
            goto clean0;

        } else {

            dipb->DriverPath = DriverPathId;

        }

        //
        // Save the Original List info, in case we get
        // an empty list on the Windows Update information.
        //
        // (Note: we don't attempt to save/restore our driver enumeration hints)
        //
        if(DevInfoElem) {
            lpOrgCompat     = DevInfoElem->CompatDriverHead;
            lpOrgCompatTail = DevInfoElem->CompatDriverTail;
            OrgCompatCount  = DevInfoElem->CompatDriverCount;

            lpOrgClass      = DevInfoElem->ClassDriverHead;
            lpOrgClassTail  = DevInfoElem->ClassDriverTail;
            OrgClassCount   = DevInfoElem->ClassDriverCount;

            lpOrgSel        = DevInfoElem->SelectedDriver;
            dwOrgSelType    = DevInfoElem->SelectedDriverType;
        } else {
            lpOrgClass      = pDeviceInfoSet->ClassDriverHead;
            lpOrgClassTail  = pDeviceInfoSet->ClassDriverTail;
            OrgClassCount   = pDeviceInfoSet->ClassDriverCount;

            lpOrgSel        = pDeviceInfoSet->SelectedClassDriver;
            dwOrgSelType    = lpOrgSel ? SPDIT_CLASSDRIVER : SPDIT_NODRIVER;
        }

        dwOrgFlags = dipb->Flags;
        dwOrgFlagsEx = dipb->FlagsEx;

        bRestoreDeviceInfo = TRUE;

        if(DevInfoElem) {
            DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
            DevInfoElem->CompatDriverCount = 0;
            DevInfoElem->CompatDriverEnumHint = NULL;
            DevInfoElem->CompatDriverEnumHintIndex = INVALID_ENUM_INDEX;

            DevInfoElem->ClassDriverHead = DevInfoElem->ClassDriverTail = NULL;
            DevInfoElem->ClassDriverCount = 0;
            DevInfoElem->ClassDriverEnumHint = NULL;
            DevInfoElem->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;

            DevInfoElem->SelectedDriver = NULL;
            DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
        } else {
            lpOrgCompat     = NULL; // just so we won't ever try to free this list.

            pDeviceInfoSet->ClassDriverHead = pDeviceInfoSet->ClassDriverTail = NULL;
            pDeviceInfoSet->ClassDriverCount = 0;
            pDeviceInfoSet->ClassDriverEnumHint = NULL;
            pDeviceInfoSet->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;

            pDeviceInfoSet->SelectedClassDriver = NULL;
        }

        SavedFlags = dipb->Flags & (DI_SHOWOEM | DI_NODI_DEFAULTACTION);
        SavedFlagsEx = dipb->FlagsEx & (DI_FLAGSEX_SHOWWINDOWSUPDATE);

        dipb->Flags   &= ~(DI_DIDCOMPAT | DI_DIDCLASS | DI_MULTMFGS | DI_SHOWOEM);
        dipb->FlagsEx &= ~(DI_FLAGSEX_DIDINFOLIST | DI_FLAGSEX_DIDCOMPATINFO | DI_FLAGSEX_SHOWWINDOWSUPDATE);

        if(IsWizard) {
            //
            // We don't want default action taken in the wizard case.
            //
            dipb->Flags |= DI_NODI_DEFAULTACTION;
        }

        //
        // Unlock the HDEVINFO before handling the Select Device.  Otherwise, our
        // multi-threaded dialog will deadlock!
        //
        UnlockDeviceInfoSet(pDeviceInfoSet);
        pDeviceInfoSet = NULL;

        if(_SetupDiCallClassInstaller(DIF_SELECTDEVICE,
                                      DeviceInfoSet,
                                      DeviceInfoData,
                                      CALLCI_LOAD_HELPERS | CALLCI_CALL_HELPERS)) {
            Err = NO_ERROR;
        } else {
            Err = GetLastError();
        }

        //
        // Now, re-acquire the lock on our device information set.
        //
        pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);
        MYASSERT(pDeviceInfoSet);

        //
        // Restore the saved flags.
        //
        dipb->Flags = (dipb->Flags & ~(DI_SHOWOEM | DI_NODI_DEFAULTACTION)) | SavedFlags;
        dipb->FlagsEx = (dipb->FlagsEx & ~(DI_FLAGSEX_SHOWWINDOWSUPDATE)) | SavedFlagsEx;

        //
        // If the class installer returned ERROR_DI_DO_DEFAULT, then
        // they either did not process the DIF_SELECTDEVICE, or they
        // have setup our device info structure with an OEM INF.
        //
        switch(Err) {

            case ERROR_DI_DO_DEFAULT :
                //
                // This case is only handled if we're in a wizard.  Otherwise, send it down
                // for default processing.
                //
                if(!IsWizard) {
                    goto DefaultHandling;
                }

                //
                // This will be the most likely return, since we are not allowing the
                // default handler to be called.  So we will build a new class Drv list
                // If it is empty we will ask again, otherwise we will accept the new
                // selection and go on.
                //
                SetCursor(LoadCursor(NULL, IDC_WAIT));

                //
                // Any drivers we find are Internet drivers.
                //
                dipb->FlagsEx |= DI_FLAGSEX_INET_DRIVER;

                SetupDiBuildDriverInfoList(DeviceInfoSet,
                                           DeviceInfoData,
                                           SPDIT_CLASSDRIVER
                                          );

                dipb->FlagsEx &= ~DI_FLAGSEX_INET_DRIVER;

                SetCursor(LoadCursor(NULL, IDC_ARROW));

                if(DevInfoElem) {
                    NewClassDriverCount = DevInfoElem->ClassDriverCount;
                } else {
                    NewClassDriverCount = pDeviceInfoSet->ClassDriverCount;
                }

                if(!NewClassDriverCount) {
                    //
                    // Error.
                    //
                    if(!LoadString(MyDllModuleHandle,
                                   IDS_SELECT_DEVICE,
                                   Title,
                                   SIZECHARS(Title))) {
                        *Title = TEXT('\0');
                    }

                    FormatMessageBox(MyDllModuleHandle,
                                     NULL,
                                     MSG_NO_DEVICEINFO_ERROR,
                                     Title,
                                     MB_OK | MB_TASKMODAL
                                    );


                    //
                    // Clean up anything that happened to get put in here.
                    //
                    if(DevInfoElem &&
                       (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDCOMPATINFO)) {
                        //
                        // The class installer built a compatible driver list--kill it here.
                        //
                        DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);

                        DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
                        DevInfoElem->CompatDriverCount = 0;
                        DevInfoElem->InstallParamBlock.Flags   &= ~DI_DIDCOMPAT;
                        DevInfoElem->InstallParamBlock.FlagsEx &= ~DI_FLAGSEX_DIDCOMPATINFO;

                        DevInfoElem->SelectedDriver = NULL;
                        DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
                    }
                    dipb->DriverPath = -1;

                    break;
                }
                //
                // Allow to fall through to handling of NO_ERROR which does clean-up for us.
                //

            case NO_ERROR :
                //
                // Destroy the original lists
                //
                if(bRestoreDeviceInfo) {
                    DestroyDriverNodes(lpOrgCompat, pDeviceInfoSet);
                    DereferenceClassDriverList(pDeviceInfoSet, lpOrgClass);

                    bRestoreDeviceInfo = FALSE;
                }

                bRestoreDriverPath = FALSE;
                break;

            case ERROR_DI_BAD_PATH :
                //
                // Pop up an error messagebox, and then leave
                //
                if(!LoadString(MyDllModuleHandle,
                               IDS_SELECT_DEVICE,
                               Title,
                               SIZECHARS(Title))) {
                    *Title = TEXT('\0');
                }

                FormatMessageBox(MyDllModuleHandle,
                                 NULL,
                                 MSG_NO_DEVICEINFO_ERROR,
                                 Title,
                                 MB_OK | MB_TASKMODAL
                                );

                dipb->DriverPath = -1;

                //
                // Allow to fall through to default processing to delete the current
                // driver list(s).
                //

            default :
DefaultHandling:
                //
                // Destroy the current driver list(s).
                //
                if(DevInfoElem) {

                    DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);
                    DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
                    DevInfoElem->CompatDriverCount = 0;

                    DereferenceClassDriverList(pDeviceInfoSet, DevInfoElem->ClassDriverHead);
                    DevInfoElem->ClassDriverHead = DevInfoElem->ClassDriverTail = NULL;
                    DevInfoElem->ClassDriverCount = 0;

                    DevInfoElem->SelectedDriver = NULL;
                    DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;

                } else {

                    DereferenceClassDriverList(pDeviceInfoSet, pDeviceInfoSet->ClassDriverHead);
                    pDeviceInfoSet->ClassDriverHead = pDeviceInfoSet->ClassDriverTail = NULL;
                    pDeviceInfoSet->ClassDriverCount = 0;

                    pDeviceInfoSet->SelectedClassDriver = NULL;
                }
            }

clean0: ;   // nothing to do

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = GetLastError();
        //
        // Access the following variables so that the compiler will respect
        // the statement ordering in the try clause.
        //
        bRestoreDeviceInfo = bRestoreDeviceInfo;
        bUnlockDevInfoElem = bUnlockDevInfoElem;
        bRestoreHwnd = bRestoreHwnd;
        bRestoreDriverPath = bRestoreDriverPath;
        pDeviceInfoSet = pDeviceInfoSet;
    }

    //
    // If we need to restore any state, then we must make sure that we have the HDEVINFO
    // locked.
    //
    if(bRestoreDeviceInfo || bUnlockDevInfoElem || bRestoreHwnd || bRestoreDriverPath) {

        if(!pDeviceInfoSet) {
            pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);
            MYASSERT(pDeviceInfoSet);
        }

        try {
            //
            // If necessary, restore the original list(s).
            //
            if(bRestoreDeviceInfo) {

                if(DevInfoElem) {

                    DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);
                    DevInfoElem->CompatDriverHead = lpOrgCompat;
                    DevInfoElem->CompatDriverTail = lpOrgCompatTail;
                    DevInfoElem->CompatDriverCount = OrgCompatCount;
                    lpOrgCompat = NULL;

                    DereferenceClassDriverList(pDeviceInfoSet, DevInfoElem->ClassDriverHead);
                    DevInfoElem->ClassDriverHead = lpOrgClass;
                    DevInfoElem->ClassDriverTail = lpOrgClassTail;
                    DevInfoElem->ClassDriverCount = OrgClassCount;
                    lpOrgClass = NULL;

                    DevInfoElem->SelectedDriver = lpOrgSel;
                    DevInfoElem->SelectedDriverType = dwOrgSelType;

                } else {

                    DereferenceClassDriverList(pDeviceInfoSet, pDeviceInfoSet->ClassDriverHead);
                    pDeviceInfoSet->ClassDriverHead = lpOrgClass;
                    pDeviceInfoSet->ClassDriverTail = lpOrgClassTail;
                    pDeviceInfoSet->ClassDriverCount = OrgClassCount;
                    lpOrgClass = NULL;

                    pDeviceInfoSet->SelectedClassDriver = lpOrgSel;
                }

                dipb->Flags = dwOrgFlags;
                dipb->FlagsEx = dwOrgFlagsEx;
            }

            //
            // If we locked the DevInfoElem just for this API, then unlock it now.
            //
            if(bUnlockDevInfoElem) {
                MYASSERT(DevInfoElem);
                DevInfoElem->DiElemFlags &= ~DIE_IS_LOCKED;
            }

            //
            // If the install param block needs its parent hwnd restored, do so now.
            //
            if(bRestoreHwnd) {
                dipb->hwndParent = hwndSave;
            }

            //
            // Likewise, restore the old driver path if necessary.
            //
            if(bRestoreDriverPath) {
                dipb->DriverPath = DriverPathSave;
                dipb->Flags |= DriverPathFlagsSave;
            }

            ;   // nothing to do

        } except(EXCEPTION_EXECUTE_HANDLER) {

            if(bRestoreDeviceInfo) {
                //
                // If we hit an exception before we got a chance to restore any of our stored-away
                // driver lists, then clean those up here.
                //
                if(DevInfoElem) {
                    if(lpOrgCompat) {
                        DestroyDriverNodes(lpOrgCompat, pDeviceInfoSet);
                    }
                    if(lpOrgClass) {
                        DereferenceClassDriverList(pDeviceInfoSet, lpOrgClass);
                    }
                } else {
                    if(lpOrgClass) {
                        DereferenceClassDriverList(pDeviceInfoSet, lpOrgClass);
                    }
                }
            }
        }
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return Err;
}

DWORD
HandleWindowsUpdate(
    IN     HWND           hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    )
/*++

Routine Description:

    This routine selects a new driver based on a list from INF file(s) that get
    downloaded from the Windows Update Internet site.

Arguments:

    hwndDlg - Supplies the window handle of the select device wizard page.

    lpdd - Supplies the address of a dialog data structure that contains information
        used in device selection.

Return Value:

    If successful, the return value is NO_ERROR, otherwise, it is an ERROR_* code.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINSTALL_PARAM_BLOCK dipb;
    SP_WINDOWSUPDATE_PARAMS WindowsUpdateParams;
    PDEVINFO_ELEM DevInfoElem;
    SP_DEVINFO_DATA DevInfoData;
    TCHAR CDMPath[MAX_PATH];
    ULONG BufferLen;
    DOWNLOADINFO DownloadInfo;
    HMODULE hModCDM;
    DOWNLOAD_UPDATED_FILES_PROC DownloadUpdateFilesProc;
    DWORD Err;
    TCHAR Title[MAX_TITLE_LEN];

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        //
        // If this is for a particular device, then initialize a device
        // information structure.
        //
        if(DevInfoElem) {

            DevInfoData.cbSize = sizeof(SP_DEVINFO_DATA);
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                             DevInfoElem,
                                             &DevInfoData
                                            );
            dipb = &(DevInfoElem->InstallParamBlock);

        } else {

            dipb = &(pDeviceInfoSet->InstallParamBlock);
        }

        //
        // Call the class installer to get the package ID and so it can open
        // a handle to the Windows Update server.
        //
        memset(&WindowsUpdateParams, 0, sizeof(SP_WINDOWSUPDATE_PARAMS));
        WindowsUpdateParams.ClassInstallHeader.InstallFunction = DIF_GETWINDOWSUPDATEINFO;
        WindowsUpdateParams.ClassInstallHeader.cbSize = sizeof(SP_CLASSINSTALL_HEADER);

        if ((Err = DoInstallActionWithParams(DIF_GETWINDOWSUPDATEINFO,
                                             pDeviceInfoSet,
                                             DevInfoElem ? &DevInfoData : NULL,
                                             &WindowsUpdateParams.ClassInstallHeader,
                                             sizeof(SP_WINDOWSUPDATE_PARAMS),
                                             INSTALLACTION_CALL_CI)) == NO_ERROR) {


            //
            // We now have a PackageId and a handle to the Windows Update server
            //

            //
            // Fill In the DOWNLOADINFO structure to pass to CDM.DLL
            //
            ZeroMemory(&DownloadInfo, sizeof(DOWNLOADINFO));
            DownloadInfo.dwDownloadInfoSize = sizeof(DOWNLOADINFO);
            DownloadInfo.lpFile = NULL;

            DownloadInfo.lpHardwareIDs = (LPCWSTR)WindowsUpdateParams.PackageId;

            DownloadInfo.lpDeviceInstanceID = NULL;

            GetVersionEx((OSVERSIONINFOW *)&DownloadInfo.OSVersionInfo);

            //
            // Set dwArchitecture to PROCESSOR_ARCHITECTURE_UNKNOWN, this
            // causes Windows Update to check get the architecture of the
            // machine itself.  You only need to explictly set the value if
            // you want to download drivers for a different architecture then
            // the machine this is running on.
            //
            DownloadInfo.dwArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;
            DownloadInfo.dwFlags = 0;
            DownloadInfo.dwClientID = 0;
            DownloadInfo.localid = 0;

            CDMPath[0] = TEXT('\0');


            if ((hModCDM = LoadLibrary(TEXT("cdm.dll"))) &&
                (DownloadUpdateFilesProc = (PVOID)GetProcAddress(hModCDM, "DownloadUpdatedFiles")) &&
                (DownloadUpdateFilesProc(WindowsUpdateParams.CDMContext,
                                         hwndDlg,
                                         &DownloadInfo,
                                         CDMPath,
                                         sizeof(CDMPath),
                                         &BufferLen)) &&

                (CDMPath[0] != TEXT('\0'))) {

                //
                // We successfully downloaded a package from Windows Update
                //

                //
                // Unlock the device information set before popping up the OEM Driver Selection
                // UI.  Otherwise, our multi-threaded dialog will deadlock.
                //
                UnlockDeviceInfoSet(pDeviceInfoSet);
                pDeviceInfoSet = NULL;

                if((Err = SelectWindowsUpdateDriver(hwndDlg,
                                              lpdd->DevInfoSet,
                                              DevInfoElem ? &DevInfoData : NULL,
                                              CDMPath,
                                              !(lpdd->flags & DD_FLAG_IS_DIALOGBOX)
                                             )) == ERROR_DI_DO_DEFAULT) {
                    //
                    // Fill in the list to select from
                    //
                    lpdd->ListType = IDC_NDW_PICKDEV_SHOWALL;
                    CheckRadioButton(hwndDlg,
                                     IDC_NDW_PICKDEV_SHOWCOMPAT,
                                     IDC_NDW_PICKDEV_SHOWALL,
                                     IDC_NDW_PICKDEV_SHOWALL
                                    );

                    FillInDeviceList(hwndDlg, lpdd);

                    //
                    // Since we only show the class list when selecting from an OEM list, hide
                    // the selection buttons.
                    //
                    ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWCOMPAT), SW_HIDE);
                    ShowWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), SW_HIDE);
                }

            }  else {

                //
                // Something failed when we tried to download the INF from the Internet
                //
                Err = ERROR_INVALID_PARAMETER;

                //
                // Pop up an error messagebox, then go try again.
                //
                if(!LoadString(MyDllModuleHandle,
                               IDS_SELECT_DEVICE,
                               Title,
                               SIZECHARS(Title))) {
                    *Title = TEXT('\0');
                }

                FormatMessageBox(MyDllModuleHandle,
                                 NULL,
                                 MSG_NO_DEVICEINFO_ERROR,
                                 Title,
                                 MB_OK | MB_TASKMODAL
                                );
            }

            if (hModCDM) {

                FreeLibrary(hModCDM);
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = GetLastError();
        //
        // Access the pDeviceInfoSet variable so that the compiler will respect our statement
        // ordering w.r.t. this value.
        //
        pDeviceInfoSet = pDeviceInfoSet;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return Err;
}
#else

DWORD
HandleWindowsUpdate(
    IN     HWND           hwndDlg,
    IN OUT PSP_DIALOGDATA lpdd
    )
{
    return ERROR_INVALID_PARAMETER;
}


#endif

PNEWDEVWIZ_DATA
GetNewDevWizDataFromPsPage(
    LPPROPSHEETPAGE ppsp
    )
/*++

Routine Description:

    This routine retrieves a pointer to a NEWDEVWIZDATA structure to be used by a
    wizard page dialog proc.  It is called during the WM_INITDIALOG handling.

Arguments:

    Page - Property sheet page structure for this wizard page.

Return Value:

    If success, a pointer to the structure, NULL otherwise.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PVOID WizObjectId;
    PWIZPAGE_OBJECT CurWizObject = NULL;

    //
    // Access the device info set handle stored in the propsheetpage's lParam.
    //
    if(pDeviceInfoSet = AccessDeviceInfoSet((HDEVINFO)(ppsp->lParam))) {

        try {
            //
            // The ObjectID (pointer, actually) for the corresponding wizard
            // object for this page is stored at the end of the ppsp structure.
            // Retrieve this now, and look for it in the devinfo set's list of
            // wizard objects.
            //
            WizObjectId = *((PVOID *)(&(((PBYTE)ppsp)[sizeof(PROPSHEETPAGE)])));

            for(CurWizObject = pDeviceInfoSet->WizPageList;
                CurWizObject;
                CurWizObject = CurWizObject->Next) {

                if(WizObjectId == CurWizObject) {
                    //
                    // We found our object.
                    //
                    break;
                }
            }

        } except(EXCEPTION_EXECUTE_HANDLER) {
            ;   // nothing to do
        }

        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    return CurWizObject ? CurWizObject->ndwData : NULL;
}


BOOL
WINAPI
SetupDiSelectDevice(
    IN     HDEVINFO         DeviceInfoSet,
    IN OUT PSP_DEVINFO_DATA DeviceInfoData OPTIONAL
    )
/*++

Routine Description:

    Default hander for DIF_SELECTDEVICE

    This routine will handle the UI for allowing a user to select a driver
    for the specified device information set or element. By using the Flags field
    of the installation parameter block struct, the caller can specify special
    handling of the UI, such as allowing selecting from OEM disks.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information set for which a
        driver is to be selected.

    DeviceInfoData - Optionally, supplies the address of a SP_DEVINFO_DATA
        structure for which a driver is to be selected.  If this parameter is not
        specified, then a driver will be selected for the global class driver list
        associated with the device information set itself.

        This is an IN OUT parameter because the class GUID for the device will be
        updated to reflect the class of the most-compatible driver, if a compatible
        driver list was built.

Return Value:

    If the function succeeds, the return value is TRUE.
    If the function fails, the return value is FALSE.  To get extended error
    information, call GetLastError.

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    DWORD Err;
    PDEVINFO_ELEM DevInfoElem = NULL;
    PDEVINSTALL_PARAM_BLOCK dipb;
    WIZPAGE_OBJECT WizPageObject;
    NEWDEVWIZ_DATA ndwData;
    PWIZPAGE_OBJECT CurWizObject, PrevWizObject;
    //
    // Store the address of the corresponding wizard object at the
    // end of the PROPSHEETPAGE buffer.
    //
    BYTE pspBuffer[sizeof(PROPSHEETPAGE) + sizeof(PVOID)];
    LPPROPSHEETPAGE Page = (LPPROPSHEETPAGE)pspBuffer;

    //
    // Make sure we're running interactively.
    //
    if(GlobalSetupFlags & PSPGF_NONINTERACTIVE) {
        SetLastError(ERROR_REQUIRES_INTERACTIVE_WINDOWSTATION);
        return FALSE;
    }

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet))) {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    Err = NO_ERROR;

    try {
        //
        // This routine cannot be called when the lock level is nested (i.e., > 1).  This
        // is explicitly disallowed, so that our multi-threaded dialog won't deadlock.
        //
        if(pDeviceInfoSet->LockRefCount > 1) {
            Err = ERROR_DEVINFO_LIST_LOCKED;
            goto clean0;
        }

        if(DeviceInfoData) {
            //
            // Special check to make sure we aren't being passed a zombie (different from
            // phantom, the zombie devinfo element is one whose corresponding devinst was
            // deleted via SetupDiRemoveDevice, but who lingers on until the caller kills
            // it via SetupDiDeleteDeviceInfo or SetupDiDestroyDeviceInfoList).
            //
            if(!DeviceInfoData->DevInst) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            //
            // Then we are to select a driver for a particular device.
            //
            if(!(DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet,
                                                         DeviceInfoData,
                                                         NULL))) {
                Err = ERROR_INVALID_PARAMETER;
                goto clean0;
            }

            dipb = &(DevInfoElem->InstallParamBlock);
        } else {
            dipb = &(pDeviceInfoSet->InstallParamBlock);
        }

        ZeroMemory(&ndwData, sizeof(ndwData));
        ndwData.ddData.iCurDesc = -1;
        ndwData.ddData.DevInfoSet = DeviceInfoSet;
        ndwData.ddData.DevInfoElem = DevInfoElem;
        ndwData.ddData.flags = DD_FLAG_USE_DEVINFO_ELEM | DD_FLAG_IS_DIALOGBOX;

        WizPageObject.RefCount = 1;
        WizPageObject.ndwData = &ndwData;
        //
        // We're safe in placing this stack object in the devinfo set's linked
        // list, since nobody will ever attempt to free it.
        //
        WizPageObject.Next = pDeviceInfoSet->WizPageList;
        pDeviceInfoSet->WizPageList = &WizPageObject;

        //
        // Since we're using the same code as the Add New Device Wizard, we
        // have to supply a LPROPSHEETPAGE as the lParam to the DialogProc.
        // (All we care about is the lParam field, and the DWORD at the end
        // of the buffer.)
        //
        Page->lParam = (LPARAM)DeviceInfoSet;

        *((PVOID *)(&(pspBuffer[sizeof(PROPSHEETPAGE)]))) = &WizPageObject;

        //
        // Release the lock, so other stuff can happen while this dialog is up.
        //
        UnlockDeviceInfoSet(pDeviceInfoSet);
        pDeviceInfoSet = NULL;

        Err = (DWORD)DialogBoxParam(MyDllModuleHandle,
                                    MAKEINTRESOURCE(DLG_DEVINSTALL),
                                    dipb->hwndParent,
                                    SelectDeviceDlgProc,
                                    (LPARAM)Page
                                   );

        //
        // Re-acquire the devinfo set lock.
        //
        pDeviceInfoSet = AccessDeviceInfoSet(DeviceInfoSet);
        MYASSERT(pDeviceInfoSet);

        //
        // Now remove the wizard page object from the devinfo set's list.  We can't
        // assume that it's still at the head of the list, since someone else couldn've
        // added another page.
        //
        for(CurWizObject = pDeviceInfoSet->WizPageList, PrevWizObject = NULL;
            CurWizObject;
            PrevWizObject = CurWizObject, CurWizObject = CurWizObject->Next) {

            if(CurWizObject == &WizPageObject) {
                break;
            }
        }

        MYASSERT(CurWizObject);

        if(PrevWizObject) {
            PrevWizObject->Next = CurWizObject->Next;
        } else {
            pDeviceInfoSet->WizPageList = CurWizObject->Next;
        }

        if(DeviceInfoData) {
            //
            // Update the caller's device information element with its (potentially) new class.
            //
            DevInfoDataFromDeviceInfoElement(pDeviceInfoSet,
                                             DevInfoElem,
                                             DeviceInfoData
                                            );
        }

clean0: ;   // nothing to do.

    } except(EXCEPTION_EXECUTE_HANDLER) {
        Err = ERROR_INVALID_PARAMETER;
        //
        // Access the pDeviceInfoSet variable so that the compiler will respect
        // the statement ordering in the try clause.
        //
        pDeviceInfoSet = pDeviceInfoSet;
    }

    if(pDeviceInfoSet) {
        UnlockDeviceInfoSet(pDeviceInfoSet);
    }

    SetLastError(Err);
    return(Err == NO_ERROR);
}


BOOL
bNoDevsToShow(
    IN PDEVINFO_ELEM DevInfoElem
    )
/*++

Routine Description:

    This routine determines whether or not there are any compatible devices to be
    displayed for the specified devinfo element.

Arguments:

    DevInfoElem - Supplies the address of a devinfo element to check.

Return Value:

    If there are no devices to show, the return value is TRUE.
    If there is at least one device (driver node) without the DNF_EXCLUDEFROMLIST flag
    set, the return value is FALSE.

--*/
{
    PDRIVER_NODE CurDriverNode;

    for(CurDriverNode = DevInfoElem->CompatDriverHead;
        CurDriverNode;
        CurDriverNode = CurDriverNode->Next) {

        if (!(CurDriverNode->Flags & DNF_OLD_INET_DRIVER) &&
            !(CurDriverNode->Flags & DNF_BAD_DRIVER) &&
            (!(CurDriverNode->Flags & DNF_EXCLUDEFROMLIST) ||
            (DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_ALLOWEXCLUDEDDRVS))) {

            return FALSE;
        }
    }

    return TRUE;
}


VOID
OnCancel(
    IN PNEWDEVWIZ_DATA ndwData
    )
/*++

Routine Description:

    This routine is only called in the select device dialog (not wizard) case.  Its
    sole purpose is to destroy any driver lists that weren't present before
    SetupDiSelectDevice was called.

Arguments:

    ndwData - Supplies the address of a data structure containing information on the
        driver lists to be (possibly) destroyed.

Return Value:

    None.

--*/
{
    PSP_DIALOGDATA lpdd;
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    PDEVINSTALL_PARAM_BLOCK dipb;
    DWORD SelectedDriverType = SPDIT_NODRIVER;

    lpdd = &(ndwData->ddData);

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {

            if(lpdd->bKeeplpSelectedDrv) {
                SelectedDriverType = DevInfoElem->SelectedDriverType;
            } else {
                DevInfoElem->SelectedDriver = NULL;
                DevInfoElem->SelectedDriverType = SPDIT_NODRIVER;
            }

            if((DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDINFOLIST) &&
               !lpdd->bKeeplpClassDrvList && (SelectedDriverType != SPDIT_CLASSDRIVER)) {

                DereferenceClassDriverList(pDeviceInfoSet, DevInfoElem->ClassDriverHead);
                DevInfoElem->ClassDriverHead = DevInfoElem->ClassDriverTail = NULL;
                DevInfoElem->ClassDriverCount = 0;
                DevInfoElem->ClassDriverEnumHint = NULL;
                DevInfoElem->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;
                DevInfoElem->InstallParamBlock.Flags   &= ~(DI_DIDCLASS | DI_MULTMFGS);
                DevInfoElem->InstallParamBlock.FlagsEx &= ~DI_FLAGSEX_DIDINFOLIST;
            }

            if((DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDCOMPATINFO) &&
               !lpdd->bKeeplpCompatDrvList && (SelectedDriverType != SPDIT_COMPATDRIVER)) {

                DestroyDriverNodes(DevInfoElem->CompatDriverHead, pDeviceInfoSet);
                DevInfoElem->CompatDriverHead = DevInfoElem->CompatDriverTail = NULL;
                DevInfoElem->CompatDriverCount = 0;
                DevInfoElem->CompatDriverEnumHint = NULL;
                DevInfoElem->CompatDriverEnumHintIndex = INVALID_ENUM_INDEX;
                DevInfoElem->InstallParamBlock.Flags   &= ~DI_DIDCOMPAT;
                DevInfoElem->InstallParamBlock.FlagsEx &= ~DI_FLAGSEX_DIDCOMPATINFO;
            }

        } else {

            if(lpdd->bKeeplpSelectedDrv) {
                if(pDeviceInfoSet->SelectedClassDriver) {
                    SelectedDriverType = SPDIT_CLASSDRIVER;
                }
            } else {
                pDeviceInfoSet->SelectedClassDriver = NULL;
            }

            if((pDeviceInfoSet->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDINFOLIST) &&
               !lpdd->bKeeplpClassDrvList && (SelectedDriverType != SPDIT_CLASSDRIVER)) {

                DereferenceClassDriverList(pDeviceInfoSet, pDeviceInfoSet->ClassDriverHead);
                pDeviceInfoSet->ClassDriverHead = pDeviceInfoSet->ClassDriverTail = NULL;
                pDeviceInfoSet->ClassDriverCount = 0;
                pDeviceInfoSet->ClassDriverEnumHint = NULL;
                pDeviceInfoSet->ClassDriverEnumHintIndex = INVALID_ENUM_INDEX;
                pDeviceInfoSet->InstallParamBlock.Flags   &= ~(DI_DIDCLASS | DI_MULTMFGS);
                pDeviceInfoSet->InstallParamBlock.FlagsEx &= ~DI_FLAGSEX_DIDINFOLIST;
            }
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);
}


LONG
GetCurDesc(
    IN PSP_DIALOGDATA lpdd
    )
/*++

Routine Description:

    This routine returns the (case-insensitive) string table index for the description
    of the currently selected driver.  This is used to select a particular entry in a
    listview control.

Arguments:

    lpdd - Supplies the address of a dialog data structure that contains information
        about the device information set being used.

Return Value:

    The string table ID for the device description, as stored in the currently-selected
    driver node.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    LONG ret;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {
            ret = DevInfoElem->SelectedDriver
                      ? DevInfoElem->SelectedDriver->DevDescription
                      : -1;
        } else {
            ret = pDeviceInfoSet->SelectedClassDriver
                      ? pDeviceInfoSet->SelectedClassDriver->DevDescription
                      : -1;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        ret = -1;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return ret;
}


VOID
__cdecl
ClassDriverSearchThread(
    IN PVOID Context
    )

/*++

Routine Description:

    Thread entry point to build a class driver list asynchronously to the main
    thread which is displaying a Select Device dialog.  This thread will free
    the memory containing its context, so the main thread should not access it
    after passing it to this thread.

Arguments:

    Context - supplies driver search context.

Return Value:

    None.

--*/

{
    PCLASSDRV_THREAD_CONTEXT ClassDrvThreadContext = Context;
    SP_DEVINSTALL_PARAMS DeviceInstallParams;
    BOOL b;
    DWORD Err;

    //
    // OR in the DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS flag so that we don't include
    // old internet drivers in the list that we get back.
    //
    DeviceInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    if (SetupDiGetDeviceInstallParams(ClassDrvThreadContext->DeviceInfoSet,
                                      &(ClassDrvThreadContext->DeviceInfoData),
                                      &DeviceInstallParams
                                      ))
    {
        DeviceInstallParams.FlagsEx |= DI_FLAGSEX_EXCLUDE_OLD_INET_DRIVERS;

        SetupDiSetDeviceInstallParams(ClassDrvThreadContext->DeviceInfoSet,
                                      &(ClassDrvThreadContext->DeviceInfoData),
                                      &DeviceInstallParams
                                      );
    }

    if(b = SetupDiBuildDriverInfoList(ClassDrvThreadContext->DeviceInfoSet,
                                      &(ClassDrvThreadContext->DeviceInfoData),
                                      SPDIT_CLASSDRIVER)) {
        Err = NO_ERROR;
    } else {
        Err = GetLastError();
    }

    //
    // Now send a message to our notification window informing them of the outcome.
    //
    PostMessage(ClassDrvThreadContext->NotificationWindow,
                WMX_CLASSDRVLIST_DONE,
                (WPARAM)b,
                (LPARAM)Err
               );

    MyFree(Context);

    //
    // Done.
    //
    _endthread();
}


BOOL
pSetupIsClassDriverListBuilt(
    IN PSP_DIALOGDATA lpdd
    )
/*++

Routine Description:

    This routine determines whether or not a class driver list has already been
    built for the specified dialog data.

Arguments:

    lpdd - Supplies the address of a dialog data buffer that is being queried for
        the presence of a class driver list.

Return Value:

    If a class driver list has already been built, the return value is TRUE, otherwise,
    it is FALSE.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;
    BOOL b = FALSE;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        if(DevInfoElem) {
            b = DevInfoElem->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDINFOLIST;
        } else {
            b = pDeviceInfoSet->InstallParamBlock.FlagsEx & DI_FLAGSEX_DIDINFOLIST;
        }
    } except(EXCEPTION_EXECUTE_HANDLER) {
        ;   // nothing to do.
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);

    return b;
}


VOID
pSetupDevInfoDataFromDialogData(
    IN  PSP_DIALOGDATA   lpdd,
    OUT PSP_DEVINFO_DATA DeviceInfoData
    )
/*++

Routine Description:

    This routine fills in a SP_DEVINFO_DATA structure based on the device information
    element specified in the supplied dialog data.

Arguments:

    lpdd - Supplies the address of a dialog data buffer that specifies a devinfo element
        to be used in filling in the DeviceInfoData buffer.

    DeviceInfoData - Supplies the address of a SP_DEVINFO_DATA structure that is filled
        in with information about the devinfo element specified in the dialog data.

Return Value:

    None.

--*/
{
    PDEVICE_INFO_SET pDeviceInfoSet;
    PDEVINFO_ELEM DevInfoElem;

    pDeviceInfoSet = AccessDeviceInfoSet(lpdd->DevInfoSet);
    MYASSERT(pDeviceInfoSet);

    try {

        if(lpdd->flags & DD_FLAG_USE_DEVINFO_ELEM) {
            DevInfoElem = lpdd->DevInfoElem;
        } else {
            DevInfoElem = pDeviceInfoSet->SelectedDevInfoElem;
        }

        //
        // The dialog data had better be referencing a devinfo element!
        //
        MYASSERT(DevInfoElem);

        DeviceInfoData->cbSize = sizeof(SP_DEVINFO_DATA);
        DevInfoDataFromDeviceInfoElement(pDeviceInfoSet, DevInfoElem, DeviceInfoData);

    } except(EXCEPTION_EXECUTE_HANDLER) {
        //
        // What to do, what to do???
        // We'll just invalidate the DeviceInfoData structure.
        //
        DeviceInfoData->cbSize = 0;
    }

    UnlockDeviceInfoSet(pDeviceInfoSet);
}


VOID
ToggleDialogControls(
    IN HWND           hwndDlg,
    IN PSP_DIALOGDATA lpdd,
    IN BOOL           Enable
    )
/*++

Routine Description:

    This routine either enables or disables all controls on a Select Device dialog box,
    depending on the value of Enable.

Arguments:

    hwndDlg - Supplies the handle of the Select Device dialog

    lpdd - Supplies the address of the dialog data for this dialog.

    Enable - If TRUE, then enable all controls (with possible exception of "Show all devices"
        radio button (if class list search failed).  If FALSE, disable all controls.

Return Value:

    None.

--*/
{
    //
    // We should only be calling this for the dialog box version.
    //
    MYASSERT(lpdd->flags & DD_FLAG_IS_DIALOGBOX);

    //
    // If we're enabling controls, make sure we only enable the "Show all devices" radio
    // button if we successfully built a class list.
    //
    if(Enable) {
        if(!(lpdd->flags & DD_FLAG_CLASSLIST_FAILED)) {
            EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), TRUE);
        }
    } else {
        EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWALL), FALSE);
    }

    EnableWindow(lpdd->hwndDrvList, Enable);
    EnableWindow(lpdd->hwndMfgList, Enable);

    EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_SHOWCOMPAT), Enable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_HAVEDISK), Enable);
    EnableWindow(GetDlgItem(hwndDlg, IDC_NDW_PICKDEV_WINDOWSUPDATE), Enable);
    EnableWindow(GetDlgItem(hwndDlg, IDOK), Enable);
    EnableWindow(GetDlgItem(hwndDlg, IDCANCEL), Enable);
}

BOOL
CDMIsInternetAvailable(
    void
    )
/*++

Routine Description:

    This routine will return TRUE or FALSE based on if this machine can get to the
    Internet or not.

Arguments:

    None

Return Value:

    TRUE if the machine can get to the Internet.
    FALSE if the machine can NOT get to the Internet.

--*/
{
    BOOL IsInternetAvailable = FALSE;
    HMODULE hModCDM = NULL;
    CDM_INTERNET_AVAILABLE_PROC CdmInternetAvailable;

    if ((hModCDM = LoadLibrary(TEXT("cdm.dll"))) &&
        (CdmInternetAvailable = (PVOID)GetProcAddress(hModCDM, "DownloadIsInternetAvailable")) &&
        (CdmInternetAvailable())) {

        IsInternetAvailable = TRUE;

    }

    if (hModCDM) {

        FreeLibrary(hModCDM);
    }

    return IsInternetAvailable;
}
