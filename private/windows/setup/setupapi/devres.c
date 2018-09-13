/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    devres.c

Abstract:

    Routines for displaying resource dialogs.

Author:

    Paula Tomlinson (paulat) 7-Feb-1996

Revision History:

    Jamie Hunter (jamiehun) 19-Mar-1998

--*/

#include "precomp.h"
#pragma hdrstop

//
// Private Prototypes
//


//
// Global Data
//

static INTERFACE_TYPE ResourcePickerReadOnlyInterfaces[] = {
    //
    // List of interface-types that we don't want user to edit properties of
    //

    PCIBus,

    //
    // End of list
    //
    InterfaceTypeUndefined
};

static const BOOL ResTypeEditable[ResType_MAX+1] = {
    //
    // lists resource types that are shown and are editable
    FALSE,  // ResType_None
    TRUE,   // ResType_Mem
    TRUE,   // ResType_IO
    TRUE,   // ResType_DMA
    TRUE,   // ResType_IRQ
    FALSE,  // ResType_DoNotUse
    FALSE   // ResType_BusNumber
};

#if (ResType_MAX+1) != 7
#error Fix SetupAPI devres.c, ResType_MAX has changed
#endif

//
// HELP ID's
//
static const DWORD DevResHelpIDs[]=
{
    IDC_DEVRES_ICON,            IDH_NOHELP,     // "Low (%d)" (Static)
    IDC_DEVRES_DEVDESC,         IDH_NOHELP,
    IDC_DEVRES_SETTINGSTATE,    IDH_DEVMGR_RESOURCES_SETTINGS,
    IDC_DEVRES_SETTINGSLIST,    IDH_DEVMGR_RESOURCES_SETTINGS,
    IDC_DEVRES_LCTEXT,          IDH_DEVMGR_RESOURCES_BASEDON,
    IDC_DEVRES_LOGCONFIGLIST,   IDH_DEVMGR_RESOURCES_BASEDON,
    IDC_DEVRES_CHANGE,          IDH_DEVMGR_RESOURCES_CHANGE,
    IDC_DEVRES_USESYSSETTINGS,  IDH_DEVMGR_RESOURCES_AUTO,
    IDC_DEVRES_CONFLICTDEVTEXT, IDH_DEVMGR_RESOURCES_CONFLICTS,
    IDC_DEVRES_CONFLICTINFOLIST,    IDH_DEVMGR_RESOURCES_CONFLICTS,
    IDC_DEVRES_MFPARENT,        IDH_DEVMGR_RESOURCES_PARENT,
    IDC_DEVRES_MFPARENT_DESC,   IDH_DEVMGR_RESOURCES_PARENT,
    IDC_DEVRES_MAKEFORCED,      IDH_DEVMGR_RESOURCES_SETMANUALLY,
    0, 0
};

//
// HACKHACK (jamiehun)
// after we've changed UI from a MakeForced, we post this message to get back control of keyboard
//

#define WM_USER_FOCUS           (WM_USER+101)


//
// API to obtain a resource-picker page
//

HPROPSHEETPAGE
GetResourceSelectionPage(
    IN HDEVINFO DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    LPDMPROP_DATA     pdmData;
    PROPSHEETPAGE     PropPage;

    //
    // private data
    // anything we "do" here must be "undone" in pResourcePickerPropPageCallback
    //
    pdmData = (LPDMPROP_DATA)MyMalloc(sizeof(DMPROP_DATA));
    if (pdmData == NULL) {
        return NULL;
    }
    ZeroMemory(pdmData,sizeof(DMPROP_DATA));

    pdmData->hDevInfo      = DeviceInfoSet;
    pdmData->lpdi          = DeviceInfoData;

    //
    // validate expectations
    //
    MYASSERT(pdmData->hDevInfo != NULL);
    MYASSERT(pdmData->lpdi != NULL);
    MYASSERT(pdmData->lpdi->DevInst != 0);

    ZeroMemory(&PropPage,sizeof(PropPage));

    //
    // create the Resources Property Page
    //
    PropPage.dwSize        = sizeof(PROPSHEETPAGE);
    PropPage.dwFlags       = PSP_DEFAULT | PSP_USECALLBACK;
    PropPage.hInstance     = MyDllModuleHandle;
    PropPage.pszTemplate   = MAKEINTRESOURCE(IDD_DEF_DEVRESOURCE_PROP);
    PropPage.pszIcon       = NULL;
    PropPage.pszTitle      = NULL;
    PropPage.pfnDlgProc    = pResourcePickerDlgProc;
    PropPage.lParam        = (LPARAM)pdmData;
    PropPage.pfnCallback   = pResourcePickerPropPageCallback;

    return CreatePropertySheetPage(&PropPage);

} // GetResourceSelectionPage


//
// CreatePropertySheetPage - callback function
//
UINT CALLBACK pResourcePickerPropPageCallback(
    HWND hwnd,
    UINT uMsg,
    LPPROPSHEETPAGE ppsp
)
/*++

Routine Description:

    Callback to handle cleanup of the property sheet

Arguments:

   Standard PropSheetPageProc arguments.

Return Value:

   Standard PropSheetPageProc return.

--*/
{
    switch (uMsg) {
        //case PSPCB_ADDREF:
        //    break;

        case PSPCB_CREATE:
            break;

        case PSPCB_RELEASE:
            //
            // release the memory we've previously allocated, outside of the actual dialog
            //
            if (ppsp->lParam != 0) {
                LPDMPROP_DATA pdmData = (LPDMPROP_DATA)(ppsp->lParam);

                MyFree(pdmData);
            }
            break;
    }

    return TRUE;

}

//
// Main dialog proceedure
//


INT_PTR
CALLBACK
pResourcePickerDlgProc(
   HWND   hDlg,
   UINT   message,
   WPARAM wParam,
   LPARAM lParam
   )

/*++

Routine Description:

    This routine provides the dialog box procedure for the main resource
    picker property page. MEMPHIS COMPATIBLE.

Arguments:

   Standard dialog box procedure arguments.

Return Value:

   Standard dialog box procedure return.

--*/

{
    LPDMPROP_DATA   lpdmpd = NULL;

    if (message == WM_INITDIALOG) {
        lpdmpd = (LPDMPROP_DATA)((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, (LPARAM)lpdmpd);
    } else {
        lpdmpd = (LPDMPROP_DATA)GetWindowLongPtr(hDlg, DWLP_USER);
    }

    switch (message) {

        //
        // initialize
        //
        case WM_INITDIALOG: {

            HICON           hIcon = NULL;
            int             iIcon = 0, iIndex = 0;
            ULONG           ulSize;
            PDEVICE_INFO_SET pDeviceInfoSet;
            HMACHINE        hMachine;

            lpdmpd->himlResourceImages = NULL;
            lpdmpd->CurrentLC = 0;
            lpdmpd->CurrentLCType = 0;
            lpdmpd->MatchingLC = 0;
            lpdmpd->MatchingLCType = 0;
            lpdmpd->SelectedLC = 0;
            lpdmpd->SelectedLCType = 0;
            lpdmpd->hDlg = hDlg;
            lpdmpd->dwFlags = 0;

            hMachine = pGetMachine(lpdmpd);

            lpdmpd->dwFlags |= DMPROP_FLAG_CHANGESSAVED; // Nothing to save yet

            //
            // NOTE: On Windows95, since lc info is in memory, they first
            // call CM_Setup_DevNode with CM_SETUP_WRITE_LOG_CONFS flag so
            // that in-memory lc data is flushed to the registry at this
            // point.
            //

            //
            // Init the Resource's image list.
            //
            lpdmpd->himlResourceImages = ImageList_Create(GetSystemMetrics(SM_CXSMICON),
                                                  GetSystemMetrics(SM_CYSMICON),
                                                  ILC_MASK, // | ILC_SHARED,
                                                  1,
                                                  1);
            //
            // add icons to image list
            //
            for (iIcon = IDI_RESOURCEFIRST;iIcon < IDI_RESOURCELAST;++iIcon) {
                //
                // resource icon
                //
                hIcon = LoadIcon(MyDllModuleHandle, MAKEINTRESOURCE(iIcon));
                iIndex = ImageList_AddIcon(lpdmpd->himlResourceImages, hIcon);
            }

            for (iIcon = IDI_RESOURCEOVERLAYFIRST;iIcon <= IDI_RESOURCEOVERLAYLAST;++iIcon) {
                //
                // overlay icon
                //
                hIcon = LoadIcon(MyDllModuleHandle, MAKEINTRESOURCE(iIcon));
                iIndex = ImageList_AddIcon(lpdmpd->himlResourceImages, hIcon);

                //
                // Tag this icon as an overlay icon (the first index is an
                // index into the image list (specifies the icon), the
                // second index is just an index to assign to each mask
                // (starting with 1).
                //
                ImageList_SetOverlayImage(lpdmpd->himlResourceImages,
                                          iIndex,
                                          iIcon-IDI_RESOURCEOVERLAYFIRST+1);
            }

            if(pInitDevResourceDlg(lpdmpd)) {
                lpdmpd->dwFlags &= ~DMPROP_FLAG_CHANGESSAVED; // need to save (prob because there was no config)
            }

            if (!(lpdmpd->dwFlags & DMPROP_FLAG_NO_RESOURCES)) {
                pShowConflicts(lpdmpd);
            }
            if (GuiSetupInProgress) {
                //
                // occasionally legacy devices cause resource-picker popup during setup
                // we do this here instead of create prop sheet, since I don't trust
                // people to cleanup on fail. At least here is less risky
                // clean this up in WM_DESTROY
                //
                lpdmpd->hDialogEvent = CreateEvent(NULL,TRUE,FALSE,SETUP_HAS_OPEN_DIALOG_EVENT);
                if (lpdmpd->hDialogEvent) {
                   SetEvent(lpdmpd->hDialogEvent);
                }
            } else {
                lpdmpd->hDialogEvent = NULL;
            }
            break;
        }


        //
        // cleanup
        //
        case WM_DESTROY: {

            HICON    hIcon;
            LOG_CONF LogConf;
            LONG     nItems, n;
            HWND     hList =  GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
            int    Count, i;

            //
            // Clean up the ICON resource usage
            //
            if ((hIcon = (HICON)LOWORD(SendDlgItemMessage(hDlg,
                         IDC_DEVRES_ICON, STM_GETICON, 0, 0L)))) {
                DestroyIcon(hIcon);
            }

            //
            // free the LC handles that were saved in the combobox data
            //
            nItems = (LONG)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,
                                            CB_GETCOUNT, 0, 0L);

            for (n = 0; n < nItems ; n++) {
                LogConf = (LOG_CONF)SendDlgItemMessage(hDlg,
                                        IDC_DEVRES_LOGCONFIGLIST,
                                        CB_GETITEMDATA, n, 0L);
                CM_Free_Log_Conf_Handle(LogConf);
            }

            if (lpdmpd->CurrentLC != 0) {
                CM_Free_Log_Conf_Handle(lpdmpd->CurrentLC);
            }

            ListView_DeleteAllItems(hList); // this will destroy all data

            if (lpdmpd->himlResourceImages) {
                ImageList_Destroy(lpdmpd->himlResourceImages);
            }

            if (lpdmpd->hDialogEvent) {
                //
                // we were holding up setup, now let setup proceed
                //
                ResetEvent(lpdmpd->hDialogEvent);
                CloseHandle(lpdmpd->hDialogEvent);
                lpdmpd->hDialogEvent = NULL;
            }
            // MyFree(lpdmpd); - do this in pResourcePickerPropPageCallback instead
            break;
        }

        case WM_COMMAND:
            //
            // old-style controls
            //

            switch(LOWORD(wParam)) {
                case IDC_DEVRES_USESYSSETTINGS: {
                    //
                    // toggle system settings
                    //

                    PDEVICE_INFO_SET pDeviceInfoSet;
                    HMACHINE    hMachine = pGetMachine(lpdmpd);

                    if (hMachine) {
                        //
                        // BugBug!!! (jamiehun)
                        // couldn't this be more clever and disable this control to begin with???
                        //
                        if (IsDlgButtonChecked(hDlg, (int)wParam)) {
                            //Turn the button back off
                            CheckDlgButton(hDlg, IDC_DEVRES_USESYSSETTINGS, 0);
                        }else {
                            //Turn the button back on
                            CheckDlgButton(hDlg, IDC_DEVRES_USESYSSETTINGS, 1);
                        }

                        pWarnResSettingNotEditable(hDlg, IDS_DEVRES_NOMODIFYREMOTE);
                        break;
                    }

                    //
                    // consider resource settings to have changed
                    //
                    lpdmpd->dwFlags &= ~DMPROP_FLAG_CHANGESSAVED;
                    PropSheet_Changed(GetParent(hDlg), hDlg);

                    if (IsDlgButtonChecked(hDlg, (int)wParam)) {
                        //
                        // Revert back to allocated display, if any
                        //
                        lpdmpd->dwFlags |= DMPROP_FLAG_USESYSSETTINGS;
                        pSelectLogConf(lpdmpd,(LOG_CONF)0,ALLOC_LOG_CONF,TRUE);
                    } else {
                        //
                        // Allow editing
                        //
                        lpdmpd->dwFlags &= ~DMPROP_FLAG_USESYSSETTINGS;
                    }
                    pShowUpdateEdit(lpdmpd);           // update controls

                    break;
                }

                case IDC_DEVRES_LOGCONFIGLIST: {
                    //
                    // drop-down list action
                    //
                    switch (HIWORD(wParam)) {
                        case CBN_SELENDOK: {
                            ULONG    ulIndex = 0;
                            int      iItem;
                            LOG_CONF SelLC;
                            HWND     hwndLC = GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST);

                            //
                            // If there is not a Log Config selected, then bail
                            //
                            iItem = (int)SendMessage(hwndLC, CB_GETCURSEL, 0, 0);
                            if(iItem != CB_ERR) {
                                SelLC = (LOG_CONF)SendMessage(hwndLC,CB_GETITEMDATA, (WPARAM)iItem,(LPARAM)0);
                            } else {
                                SelLC = (LOG_CONF)0;
                            }
                            if(SelLC != lpdmpd->SelectedLC) {
                                pSelectLogConf(lpdmpd,SelLC,BASIC_LOG_CONF,FALSE);
                            }
                            //
                            // I prob don't need this here, but I'm playing safe!
                            //
                            lpdmpd->dwFlags &= ~DMPROP_FLAG_CHANGESSAVED;
                            break;
                        }
                    }
                    break;
                }

                case IDC_DEVRES_CHANGE: {
                    //
                    // change selected setting
                    //
                    pChangeCurrentResSetting(lpdmpd);
                    break;
                }

                case IDC_DEVRES_MAKEFORCED: {
                    //
                    // possibly allow editing (after we've shown message)
                    // when we get here, always show a configuration
                    //

                    //
                    // BugBug!!! (jamiehun 7/30/99) this should be taken out
                    //
                    pSelectLogConf(lpdmpd,(LOG_CONF)0,ALLOC_LOG_CONF,TRUE);

                    if(lpdmpd->dwFlags & DMPROP_FLAG_FORCEDONLY) {
                        lpdmpd->dwFlags &= ~DMPROP_FLAG_CHANGESSAVED; // need to save
                    }
                    pShowViewAllEdit(lpdmpd);
                    //
                    // select in the first available config to edit
                    //
                    pSelectLogConf(lpdmpd,(LOG_CONF)0,ALLOC_LOG_CONF,TRUE);

                    //
                    // ensure we have reasonable focus for accessability
                    //
                    PostMessage(hDlg,WM_USER_FOCUS,IDC_DEVRES_SETTINGSLIST,0);
                    break;
                }

                default:
                    break;
            }
            break;

        case WM_USER_FOCUS:
            //
            // change focus to DlgItem wParam
            //
            SetFocus(GetDlgItem(hDlg,(int)wParam));
            return TRUE;

        case WM_NOTIFY: {
            //
            // new controls & property codes
            //
            NMHDR * pHdr = (NMHDR*)lParam;

            switch (pHdr->code) {

                case PSN_APPLY:  {
                    BOOL      bRet = FALSE;
                    //
                    // If there were Changes and they haven't been saved,
                    // then save them.
                    // consider some special cases as "haven't been saved"
                    //
                    if((lpdmpd->CurrentLC == 0) && (lpdmpd->dwFlags&DMPROP_FLAG_FIXEDCONFIG)) {
                        lpdmpd->dwFlags &= ~DMPROP_FLAG_CHANGESSAVED;
                    }

                    switch(pOkToSave(lpdmpd)) {
                        case IDNO:
                            //
                            // proceed without saving
                            //
                            bRet = TRUE;
                            break;
                        case IDCANCEL:
                            //
                            // don't proceed
                            //
                            bRet = FALSE;
                            break;
                        case IDYES:
                            //
                            // proceed and save
                            //
                            bRet = pSaveDevResSettings(lpdmpd);
                            #if 0
                            if (bRet) {
                                if ((lpdmpd->lpdi)->Flags &  DI_NEEDREBOOT) {
                                    PropSheet_RebootSystem(GetParent(hDlg));
                                } else if ((lpdmpd->lpdi)->Flags &  DI_NEEDRESTART) {
                                    PropSheet_RestartWindows(GetParent(hDlg));
                                }
                            #endif
                            if (bRet) {
                                //
                                // This page doesn't support roll-back, if we saved
                                // something then we're committed, disable the cancel
                                // botton.
                                //
                                PropSheet_CancelToClose(GetParent(hDlg));
                            }
                            break;
                        default:
                            MYASSERT(FALSE /* pOkToSave returned invalid value */);
                            bRet = FALSE;
                            break;
                    }

                    SetWindowLongPtr(hDlg, DWLP_MSGRESULT, bRet ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE);
                    return TRUE;
                }

                case LVN_DELETEALLITEMS:
                    if (pHdr->idFrom == IDC_DEVRES_SETTINGSLIST) {
                        return FALSE;   // we want LVN_DELETEITEM messages
                    }
                    break;

                case LVN_DELETEITEM: {
                    LPNMLISTVIEW pListView = (LPNMLISTVIEW)pHdr;
                    if (pHdr->idFrom == IDC_DEVRES_SETTINGSLIST) {
                        PITEMDATA   pItemData = (PITEMDATA)(LPVOID)(pListView->lParam);
                        //
                        // when an item is deleted, destroy associated data
                        //
                        if (pItemData->MatchingResDes) {
                            CM_Free_Res_Des_Handle(pItemData->MatchingResDes);
                        }
                        MyFree(pItemData);
                    }
                    break;
                }
                    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), FALSE);
                    break;

                case LVN_ITEMCHANGED:
                    //
                    // If the item change is comming from the resource
                    // list, and there is a logconfig to be edited:
                    //
                    if (pHdr->idFrom == IDC_DEVRES_SETTINGSLIST) {
                        //
                        // see if we should enable resource change
                        //
                        pCheckEnableResourceChange(lpdmpd);
                    }
                    break;

                case NM_DBLCLK:
                    //
                    // If the double click is from the SETTINGS list
                    // AND the DEVRES_CHANGE button is enabled, then
                    // allow the change.
                    //
                    if (pHdr->idFrom == IDC_DEVRES_SETTINGSLIST) {
                        //
                        // this routine should check that we can change settings
                        //
                        pChangeCurrentResSetting(lpdmpd);
                    }
                    break;
            }
            break;
        }

        case WM_SYSCOLORCHANGE: {

            HWND hChildWnd = GetWindow(hDlg, GW_CHILD);

            while (hChildWnd != NULL) {
                SendMessage(hChildWnd, WM_SYSCOLORCHANGE, wParam, lParam);
                hChildWnd = GetWindow(hChildWnd, GW_HWNDNEXT);
            }
            break;
        }

        case WM_HELP:      // F1
            WinHelp(((LPHELPINFO)lParam)->hItemHandle, DEVRES_HELP, HELP_WM_HELP, (ULONG_PTR)DevResHelpIDs);
            break;

        case WM_CONTEXTMENU:      // right mouse click
            WinHelp((HWND)wParam, DEVRES_HELP, HELP_CONTEXTMENU, (ULONG_PTR)DevResHelpIDs);
            break;
   }

   return FALSE;

}

//
// Helper functions
//

HMACHINE
pGetMachine(
    LPDMPROP_DATA   lpdmpd
    )
/*++

Routine Description:

    Retrieve Machine Handle

Arguments:

    lpdmpd - Property Data

Return Value:

    handle

--*/
{
    HMACHINE hMachine;
    PDEVICE_INFO_SET pDeviceInfoSet;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(lpdmpd->hDevInfo))) {
        return NULL;
    }
    hMachine = pDeviceInfoSet->hMachine;
    UnlockDeviceInfoSet(pDeviceInfoSet);
    return hMachine;
}

BOOL
pInitDevResourceDlg(
    LPDMPROP_DATA   lpdmpd
    )
/*++

Routine Description:

    This routine intializes the main resource picker property page.
    MEMPHIS COMPATIBLE.

Arguments:


Return Value:

    TRUE if "not saved"

--*/

{
    HWND            hDlg = lpdmpd->hDlg;
    CONFIGRET       Status = CR_SUCCESS;
    HICON           hIcon = NULL, hOldIcon = NULL;
    BOOL            bHasCurrent = FALSE;
    BOOL            bShowCurrent = FALSE;
    BOOL            bHasForced = FALSE;
    BOOL            bNoForcedConfig = FALSE;
    BOOL            bNeedsForcedConfig = FALSE;
    BOOL            bNoBasicConfigs = FALSE;
    LV_COLUMN       LvCol;
    HWND            hWndList = NULL;
    TCHAR           szString[MAX_PATH], szTemp[MAX_PATH], szBasic[MAX_PATH],
                    szConfig[MAX_PATH];
    ULONG           ulIndex = 0, ulSize = 0, DevStatus = 0, DevProblem = 0;
    DWORD           BusType = (DWORD)(-1);
    LOG_CONF        LogConf;
    DWORD           dwPriority = 0;
    WORD            wItem;
    ULONG           ConfigFlags;
    HMACHINE        hMachine = NULL;
    PDEVICE_INFO_SET pDeviceInfoSet;
    int             iIndex;
    BOOL            bHasPrivs = FALSE;
    //
    // Set initial control states
    //
    pHideAllControls(lpdmpd);

    //
    // determine priv token
    // security checks are visual only
    // real security checks are done in umpnpmgr
    //

    bHasPrivs = DoesUserHavePrivilege(SE_LOAD_DRIVER_NAME);

    //
    // Set the ICON and device description
    //
    if (SetupDiLoadClassIcon(&lpdmpd->lpdi->ClassGuid, &hIcon, NULL)) {

        if ((hOldIcon = (HICON)LOWORD(SendDlgItemMessage(hDlg, IDC_DEVRES_ICON,
                                                         STM_SETICON,
                                                         (WPARAM)hIcon, 0L)))) {
            DestroyIcon(hOldIcon);
        }
    }

    hMachine = pGetMachine(lpdmpd);

    //
    // retrieves current configuration, if any
    //
    bHasCurrent = pGetCurrentConfig(lpdmpd);

    //
    // First try to get the device's friendly name, then fall back to its description,
    // and finally, use the "Unknown Device" description.
    //
    ulSize = MAX_PATH * sizeof(TCHAR);
    if (CM_Get_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                         CM_DRP_FRIENDLYNAME,
                                         NULL, (LPBYTE)szString,
                                         &ulSize, 0,hMachine) != CR_SUCCESS) {

        ulSize = MAX_PATH * sizeof(TCHAR);
        if (CM_Get_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                             CM_DRP_DEVICEDESC,
                                             NULL, (LPBYTE)szString,
                                             &ulSize, 0,hMachine) != CR_SUCCESS) {

            LoadString(MyDllModuleHandle, IDS_DEVNAME_UNK, szString, MAX_PATH);
        }
    }
    SetDlgItemText(hDlg, IDC_DEVRES_DEVDESC, szString);

    //
    // We sometimes get called to show this page even if the device
    // doesn't consume any resources. Check for that case and if so, just
    // display an informational message and disable everything else.
    //

    if (!pDevRequiresResources(lpdmpd->lpdi->DevInst,hMachine)) {

        //
        // This device has no resources
        //
        pShowViewNoResources(lpdmpd);
        lpdmpd->dwFlags |= DMPROP_FLAG_NO_RESOURCES;
        goto Final;
    }

    //
    // Initialize the ListView control
    //
    hWndList = GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
    LvCol.mask = LVCF_TEXT;

    if (LoadString(MyDllModuleHandle, IDS_RESOURCETYPE, szString, MAX_PATH)) {
        LvCol.pszText = (LPTSTR)szString;
        ListView_InsertColumn(hWndList, 0, (LV_COLUMN FAR *)&LvCol);
    }

    if (LoadString(MyDllModuleHandle, IDS_RESOURCESETTING, szString, MAX_PATH)) {
        LvCol.pszText = (LPTSTR)szString;
        ListView_InsertColumn(hWndList, 1, (LV_COLUMN FAR *)&LvCol);
    }

    ListView_SetImageList(hWndList,lpdmpd->himlResourceImages, LVSIL_SMALL);
    //
    // Get DevStatus & DevProblem here, we may use this info further down
    //
    if (CM_Get_DevNode_Status_Ex(&DevStatus, &DevProblem, lpdmpd->lpdi->DevInst,
                              0,hMachine) != CR_SUCCESS) {
        //
        // we should never get here
        //
        DevStatus = 0;
    } else if (DevStatus & DN_HAS_PROBLEM) {
        //
        // cache problem flag away
        //
        lpdmpd->dwFlags |= DMPROP_FLAG_HASPROBLEM;
    }

    if (bIsMultiFunctionChild(lpdmpd->lpdi,hMachine)) {
        //
        // If this is a MultiFunction Child, disable all change controls, put up
        // special text, and show the alloc config
        //
        pShowViewMFReadOnly(lpdmpd,FALSE);
        goto Final;
    }

    //
    // begin with read-only view, assuming settings are system
    //
    lpdmpd->dwFlags |= DMPROP_FLAG_USESYSSETTINGS;

    if (CM_Get_First_Log_Conf_Ex(NULL,
                                    lpdmpd->lpdi->DevInst,
                                    FORCED_LOG_CONF,
                                    hMachine) == CR_SUCCESS) {
        //
        // the user currently has a forced config
        //
        lpdmpd->dwFlags &= ~DMPROP_FLAG_USESYSSETTINGS;
        bHasForced = TRUE;
    }

    bShowCurrent = pShowViewReadOnly(lpdmpd,bHasPrivs);
    if (!bHasPrivs) {
        //
        // if we don't have enough priv's, bottle out here
        // we'll either be displaying current resources
        // or displaying a problem
        //
        goto Final;
    }

    //
    // Retrieve alternate configurations for this device
    //
    if (bHasCurrent) {
        //
        // Current config (if any) is indicated with zero handle
        //
        LoadString(MyDllModuleHandle, IDS_CURRENTCONFIG, szString, MAX_PATH);

        iIndex = (int)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,
                                         CB_ADDSTRING, (WPARAM)0, (LPARAM)(LPSTR)szString);
        SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST, CB_SETITEMDATA,(WPARAM)iIndex, (LPARAM)0);
        SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST, CB_SETCURSEL,(WPARAM)0, (LPARAM)0);
    }
    //
    // now fill in basic configurations
    //
    Status = CM_Get_First_Log_Conf_Ex(&LogConf,
                                   lpdmpd->lpdi->DevInst,
                                   BASIC_LOG_CONF,hMachine);

    if (Status == CR_SUCCESS) {
        LoadString(MyDllModuleHandle, IDS_BASICCONFIG, szBasic, MAX_PATH);
        ulIndex = 0;

        if (!pConfigHasNoAlternates(lpdmpd,LogConf)) {
            //
            // first configuration has more than one alternative
            //
            lpdmpd->dwFlags &= ~DMPROP_FLAG_SINGLE_CONFIG;
        } else {
            //
            // begin with the assumption there is a single fixed basic config
            // we will generally be proved wrong
            //
            lpdmpd->dwFlags |= DMPROP_FLAG_SINGLE_CONFIG;
        }

        while (Status == CR_SUCCESS) {
            //
            // Add this config to the Combobox
            //
            wsprintf(szTemp, TEXT("%s %04u"), szBasic, ulIndex);

            wItem = (WORD)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,
                                             CB_ADDSTRING, 0,
                                             (LPARAM)(LPSTR)szTemp);

            //
            // Save the log config handle as the item data in the combobox
            //
            SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST, CB_SETITEMDATA,
                               wItem, (LPARAM)LogConf);

            //
            // Get the next config
            //
            Status = CM_Get_Next_Log_Conf_Ex(&LogConf, LogConf, 0,hMachine);
            ulIndex++;
        }

        if (ulIndex > 1) {
            //
            // there is more than one config
            //
            lpdmpd->dwFlags &= ~DMPROP_FLAG_SINGLE_CONFIG;
        }

        if (lpdmpd->dwFlags & DMPROP_FLAG_SINGLE_CONFIG) {
            bNoForcedConfig = TRUE;
        }

        if (bHasCurrent) {
            //
            // try to find a matching LC now, and if we could find one,
            // re-load current display (this applies editable ranges to the resources)
            //
            if(pFindMatchingAllocConfig(lpdmpd)) {
                pLoadCurrentConfig(lpdmpd,TRUE);
            }
        }
    } else {
        //
        // If there are no basic configs, we cannot allow a forced config
        //
        bNoForcedConfig = TRUE; // cannot force
        bNoBasicConfigs = TRUE; // this is why
        lpdmpd->dwFlags |= DMPROP_FLAG_SINGLE_CONFIG;
    }
    //
    // Get ConfigFlags here, we may use this info further down
    //
    ulSize = sizeof(ConfigFlags);
    if (CM_Get_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                         CM_DRP_CONFIGFLAGS,
                                         NULL, (LPBYTE)&ConfigFlags,
                                         &ulSize, 0,hMachine) != CR_SUCCESS) {
        ConfigFlags = 0;
    }
    if (ConfigFlags & CONFIGFLAG_NEEDS_FORCED_CONFIG) {
        //
        // registry says that we need a forced config
        // registry can only say this to us once
        //
        bNeedsForcedConfig = TRUE;
        ConfigFlags &= ~CONFIGFLAG_NEEDS_FORCED_CONFIG;
        CM_Set_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                         CM_DRP_CONFIGFLAGS,
                                         (LPBYTE)&ConfigFlags,
                                         sizeof(ConfigFlags),
                                         0,
                                         hMachine);
    }
    if(!bNoForcedConfig && pGetMachine(lpdmpd) != NULL) {
        //
        // can't force-config a remote machine
        //
        bNoForcedConfig = TRUE;
        bNoBasicConfigs = TRUE; // treat as we've got nothing to force-config from
    }
    if (!bNoForcedConfig) {
        //
        // Check bus we're using to see if we should set bNoForcedConfig
        //
        ulSize = sizeof(BusType);
        if (CM_Get_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                             CM_DRP_LEGACYBUSTYPE,
                                             NULL, (LPBYTE)&BusType,
                                             &ulSize, 0,hMachine) != CR_SUCCESS) {
            BusType = (DWORD)InterfaceTypeUndefined;
        }

        if (BusType != (DWORD)InterfaceTypeUndefined) {
            int InterfaceItem;

            for(InterfaceItem = 0; ResourcePickerReadOnlyInterfaces[InterfaceItem] != InterfaceTypeUndefined; InterfaceItem++) {
                if (BusType == (DWORD)ResourcePickerReadOnlyInterfaces[InterfaceItem]) {
                    //
                    // Bus is one that we do not allow forced configs
                    //
                    bNoForcedConfig = TRUE;
                    break;
                }
            }
        }
    }

    //
    // determine if it can be software-config'd or not
    // we need to do this prior to any initial display
    //
    dwPriority = pGetMinLCPriority(lpdmpd->lpdi->DevInst, BASIC_LOG_CONF,hMachine);
    if (dwPriority < LCPRI_HARDRECONFIG) {
        //
        // doesn't need to be manually configured
        //
        lpdmpd->dwFlags &= ~DMPROP_FLAG_FORCEDONLY;
    } else {
        //
        // this cannot be software config'd
        // FORCEDONLY & bNoForcedConfig is a quandry, shouldn't happen
        //
        lpdmpd->dwFlags |= DMPROP_FLAG_FORCEDONLY;
        if(bNoBasicConfigs) {
            MYASSERT(!bNoBasicConfigs);
        } else {
            MYASSERT(!bNoForcedConfig);
            bNoForcedConfig = FALSE;
        }
    }

    //
    // Try to determine initial display
    //
    // we've already covered pShowViewNoResources (no actual or potential configs)
    // and pShowViewMFReadOnly (it's a multi-function device)
    //
    // we're currently showing as pShowViewReadOnly
    //
    // some cases....
    // (1) show forced config, don't allow auto-config (config flags say requires forced)
    // (2) show forced config, allow auto-config
    // (3) don't show any config, but maybe show a forced-config button
    // (4) auto-config, don't allow forced config
    // (5) show auto-config, allow forced-config
    //
    if (bNeedsForcedConfig) {
        if (bNoBasicConfigs) {
            MYASSERT(!bNoBasicConfigs);
            bNeedsForcedConfig = FALSE;
        } else {
            MYASSERT(!bNoForcedConfig);
            bNoForcedConfig = FALSE;
            if (bHasForced) {
                //
                // already got one, but we'll go through the motions
                // we'll show what we have, allow user to change it
                // but we wont needlessly save it
                //
                bNeedsForcedConfig = FALSE;
            }
            //
            // caller said that device must have forced config, so go immediately there
            // = case (1) unless we've otherwise said we cannot have a forced config
            //
            lpdmpd->dwFlags |= DMPROP_FLAG_FORCEDONLY;
            pSelectLogConf(lpdmpd,(LOG_CONF)0,ALLOC_LOG_CONF,TRUE);
            pShowViewAllEdit(lpdmpd);
            goto Final;
        }
    }
    if ((!bShowCurrent) || (DevStatus & DN_HAS_PROBLEM)) {
        //
        //  determine between pShowViewNoAlloc and pShowViewNeedForced
        //
        if (bNoForcedConfig) {
            //
            // there is a problem - device doesn't currently have a current configuration
            // but we don't have the option of letting them set forced config
            // so this ends up display only (tough-luck scenario)
            // if there are current resources, show them
            //
            pShowViewReadOnly(lpdmpd,FALSE);
        } else {
            //
            // we show the problem and we give the user
            // an option to force config
            //
            pShowViewNeedForced(lpdmpd);
        }
        goto Final;
    }
    if (bNoBasicConfigs) {
        //
        // If we have a current config, but no basic configs, we just display what we have
        // and don't give option to edit
        //
        pShowViewReadOnly(lpdmpd,FALSE);
        goto Final;
    }
    if ((lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS) && bNoForcedConfig) {
        //
        // we can't force a bNoForcedConfig item - display only
        //
        pShowViewReadOnly(lpdmpd,FALSE);
        goto Final;
    }
    //
    // we already have and will be displaying a current config
    //
    pShowViewAllEdit(lpdmpd);
    bNeedsForcedConfig = (BOOL)!bHasCurrent; // rarely, if ever, will this return TRUE

  Final:

    return bNeedsForcedConfig;

} // InitDevResourceDlg

PITEMDATA
pGetResourceToChange(
    IN  LPDMPROP_DATA   lpdmpd,
    OUT int             *pCur
    )
/*++

Routine Description:

    Gets resource to change
    NULL if we cannot change resource

Arguments:

    lpdmpd = dialog data
    pCur = (out) index

Return Value:

    PITEMDATA saved for selected resource

--*/
{
    HWND     hList =  GetDlgItem(lpdmpd->hDlg, IDC_DEVRES_SETTINGSLIST);
    PITEMDATA pItemData = NULL;
    int     iCur;

    //
    // first check the obvious
    //
    if (lpdmpd->dwFlags & DMPROP_FLAG_VIEWONLYRES) {
        //
        // no editing allowed
        //
        return NULL;
    }
    if (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS) {
        //
        // showing system settings
        //
        return NULL;
    }


    //
    // Check if there is a selected item.
    // If yes, then activate the change button
    // if the LC allows editing.
    //
    iCur = (int)ListView_GetNextItem(hList,-1, LVNI_SELECTED);
    if (iCur == LB_ERR) {
        //
        // no selection
        //
        return NULL;
    }
    pItemData = (PITEMDATA)pGetListViewItemData(hList, iCur, 0);
    if (pItemData == NULL) {
        //
        // shouldn't happen
        //
        MYASSERT(pItemData);
        return NULL;
    }
    if (pItemData->bFixed) {
        //
        // this is an un-editable setting
        //
        return NULL;
    }
    if (pItemData->MatchingResDes == (RES_DES)0) {
        //
        // should be caught by bFixed
        //
        MYASSERT(pItemData->MatchingResDes != (RES_DES)0);
        return NULL;
    }
    //
    // we're happy
    //
    if (pCur) {
        *pCur = iCur;
    }
    return pItemData;
}

VOID
pCheckEnableResourceChange(
    LPDMPROP_DATA   lpdmpd
    )
/*++

Routine Description:

    enables/disable change button

Arguments:


Return Value:

    none

--*/
{
#if 0 // this seems to confuse people
    EnableWindow(GetDlgItem(lpdmpd->hDlg, IDC_DEVRES_CHANGE),
                    pGetResourceToChange(lpdmpd,NULL)!=NULL);
#endif // 0

    //
    // show this button enabled if we are in EDIT mode
    //
    EnableWindow(GetDlgItem(lpdmpd->hDlg, IDC_DEVRES_CHANGE),
                 (lpdmpd->dwFlags & DMPROP_FLAG_VIEWONLYRES)==0 &&
                 (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS)==0);
}

BOOL
pDevHasConfig(
    DEVINST     DevInst,
    ULONG       ulConfigType,
    HMACHINE    hMachine
    )

/*++

Routine Description:

    This routine determines whether a log config of the specified type
    exists for this device instance.
    MEMPHIS COMPATIBLE.

Arguments:

    DevInst         Device instance to query log configs for.

    ulConfigType    Specifies the type of log conf to check for the presense of.

Return Value:

   TRUE if the device has a config of that type and FALSE if it does not.

--*/

{
    BOOL bRet = (CM_Get_First_Log_Conf_Ex(NULL, DevInst, ulConfigType,hMachine) == CR_SUCCESS);
    return bRet;

} // DevHasConfig

DWORD
pGetMinLCPriority(
    IN DEVINST DevInst,
    IN ULONG   ulConfigType,
    IN HMACHINE hMachine
    )

/*++

Routine Description:

    This routine returns the minimum priority value of all log confs of the
    specified type for this device. MEMPHIS COMPATIBLE.

Arguments:

    DevInst         Device instance to query log configs for.

    ulConfigType    Specifies the type of log conf.

Return Value:

   Returns the minimum priority value found or LCPRI_LASTSOFTCONFIG if no priorities
   are found.

--*/

{
    CONFIGRET Status = CR_SUCCESS;
    ULONG priority, minPriority = MAX_LCPRI;
    LOG_CONF LogConf, tempLC;
    BOOL FoundOneLogConfWithPriority = FALSE;

    //
    // Walk through each log conf of this type for this device and
    // save the smallest value.
    //

    Status = CM_Get_First_Log_Conf_Ex(&LogConf, DevInst, ulConfigType,hMachine);
    while (Status == CR_SUCCESS) {

        if (CM_Get_Log_Conf_Priority_Ex(LogConf, &priority, 0,hMachine) == CR_SUCCESS) {
            FoundOneLogConfWithPriority = TRUE;
            minPriority = min(minPriority, priority);
        }

        tempLC = LogConf;
        Status = CM_Get_Next_Log_Conf_Ex(&LogConf, LogConf, 0,hMachine);
        CM_Free_Log_Conf_Handle(tempLC);
    }

    if(FoundOneLogConfWithPriority) {
        return minPriority;
    } else {
        //
        // None of the LogConfigs had an associated priority. This is common on
        // NT, because the bus drivers don't specify ConfigMgr-style priorities
        // when responding to IRQ_MN_QUERY_RESOURCE_REQUIREMENTS.  Since these
        // cases are all PnP bus drivers, however, it is most correct to specify
        // these LogConfigs as soft-settable.
        //
        return LCPRI_LASTSOFTCONFIG;
    }

} // GetMinLCPriority

BOOL
pDevRequiresResources(
    DEVINST DevInst,
    HMACHINE hMachine
    )
{
    if (CM_Get_First_Log_Conf_Ex(NULL, DevInst, BASIC_LOG_CONF,hMachine) == CR_SUCCESS) {
        return TRUE;
    }

    if (CM_Get_First_Log_Conf_Ex(NULL, DevInst, FILTERED_LOG_CONF,hMachine) == CR_SUCCESS) {
        return TRUE;
    }

    if (CM_Get_First_Log_Conf_Ex(NULL, DevInst, OVERRIDE_LOG_CONF,hMachine) == CR_SUCCESS) {
        return TRUE;
    }

    if (CM_Get_First_Log_Conf_Ex(NULL, DevInst, FORCED_LOG_CONF,hMachine) == CR_SUCCESS) {
        return TRUE;
    }

    if (CM_Get_First_Log_Conf_Ex(NULL, DevInst, BOOT_LOG_CONF,hMachine) == CR_SUCCESS) {
        return TRUE;
    }

    if (CM_Get_First_Log_Conf_Ex(NULL, DevInst, ALLOC_LOG_CONF,hMachine) == CR_SUCCESS) {
        return TRUE;
    }

    return FALSE;

} // DevRequiresResources

BOOL
pGetCurrentConfig(
    IN OUT  LPDMPROP_DATA lpdmpd
    )

/*++

Routine Description:

    This routine determines the current known configuration
    current configs are either forced, alloc or boot configs.

Arguments:

    lpdmpd          Property data.

Return Value:

   TRUE if we set the current config

--*/

{
    PDEVICE_INFO_SET pDeviceInfoSet;
    HMACHINE         hMachine;
    ULONG            Status;
    ULONG            Problem;

    MYASSERT(lpdmpd!=NULL);
    MYASSERT(lpdmpd->lpdi!=NULL);
    MYASSERT(lpdmpd->CurrentLC==0);
    MYASSERT(lpdmpd->lpdi->DevInst!=0);

    if (lpdmpd==NULL ||
        lpdmpd->lpdi==NULL ||
        lpdmpd->lpdi->DevInst==0) {
        return FALSE;
    }

    lpdmpd->dwFlags &= ~DMPROP_FLAG_DISPLAY_MASK;

    if(!(pDeviceInfoSet = AccessDeviceInfoSet(lpdmpd->hDevInfo))) {
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
    }

    hMachine = pDeviceInfoSet->hMachine;

    UnlockDeviceInfoSet (pDeviceInfoSet);

    if (CM_Get_DevNode_Status_Ex(&Status, &Problem, lpdmpd->lpdi->DevInst,
                              0,hMachine) != CR_SUCCESS) {
        Problem = 0;
        Status = 0;
    } else if((Status & DN_HAS_PROBLEM)==0) {
        //
        // If this device is running, does this devinst have a ALLOC log config?
        //
        if (CM_Get_First_Log_Conf_Ex(&lpdmpd->CurrentLC,
                                     lpdmpd->lpdi->DevInst,
                                     ALLOC_LOG_CONF,
                                     hMachine) == CR_SUCCESS) {

            lpdmpd->dwFlags |= DMPROP_FLAG_DISPLAY_ALLOC;
            lpdmpd->CurrentLCType = ALLOC_LOG_CONF;
            return TRUE;
        }
    }
    //
    // If no config so far, does it have a FORCED log config?
    //

    if (CM_Get_First_Log_Conf_Ex(&lpdmpd->CurrentLC,
                                    lpdmpd->lpdi->DevInst,
                                    FORCED_LOG_CONF,
                                    hMachine) == CR_SUCCESS) {

        lpdmpd->dwFlags |= DMPROP_FLAG_DISPLAY_FORCED;
        lpdmpd->CurrentLCType = FORCED_LOG_CONF;
        return TRUE;
    }

    //
    // if there's a hardware-disabled problem, boot-config isn't valid
    //
    if((Status & DN_HAS_PROBLEM)==0 || Problem != CM_PROB_HARDWARE_DISABLED) {
        //
        // Does it have a BOOT log config?
        //
        if (CM_Get_First_Log_Conf_Ex(&lpdmpd->CurrentLC,
                                        lpdmpd->lpdi->DevInst,
                                        BOOT_LOG_CONF,
                                        hMachine) == CR_SUCCESS) {

            lpdmpd->dwFlags |= DMPROP_FLAG_DISPLAY_BOOT;
            lpdmpd->CurrentLCType = BOOT_LOG_CONF;
            return TRUE;
        }
    }

    return FALSE;
}

void
pGetHdrValues(
    IN  LPBYTE      pData,
    IN  RESOURCEID  ResType,
    OUT PULONG      pulValue,
    OUT PULONG      pulLen,
    OUT PULONG      pulEnd,
    OUT PULONG      pulFlags
    )
{
    switch (ResType) {

        case ResType_Mem: {

            PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)pData;

            *pulValue = (ULONG)pMemData->MEM_Header.MD_Alloc_Base;
            *pulLen   = (ULONG)(pMemData->MEM_Header.MD_Alloc_End -
                        pMemData->MEM_Header.MD_Alloc_Base + 1);
            *pulEnd   = (ULONG)pMemData->MEM_Header.MD_Alloc_End;
            *pulFlags = pMemData->MEM_Header.MD_Flags;
            break;
        }

        case ResType_IO: {

            PIO_RESOURCE   pIoData = (PIO_RESOURCE)pData;

            *pulValue = (ULONG)pIoData->IO_Header.IOD_Alloc_Base;
            *pulLen   = (ULONG)(pIoData->IO_Header.IOD_Alloc_End -
                        pIoData->IO_Header.IOD_Alloc_Base + 1);
            *pulEnd   = (ULONG)pIoData->IO_Header.IOD_Alloc_End;
            *pulFlags = pIoData->IO_Header.IOD_DesFlags;
            break;
        }

        case ResType_DMA: {

            PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)pData;

            *pulValue = (ULONG)pDmaData->DMA_Header.DD_Alloc_Chan;
            *pulLen   = 1;
            *pulEnd   = *pulValue;
            *pulFlags = pDmaData->DMA_Header.DD_Flags;
            break;
        }

        case ResType_IRQ: {

            PIRQ_RESOURCE  pIrqData = (PIRQ_RESOURCE)pData;

            *pulValue = (ULONG)pIrqData->IRQ_Header.IRQD_Alloc_Num;
            *pulLen   = 1;
            *pulEnd   = *pulValue;
            *pulFlags = pIrqData->IRQ_Header.IRQD_Flags;
            break;
        }
    }

    if(*pulEnd < *pulValue) {
        //
        // filter out bad/zero-length range
        //
        *pulLen = 0;
    }

    return;

} // GetHdrValues

void
pGetRangeValues(
    IN  LPBYTE      pData,
    IN  RESOURCEID  ResType,
    IN  ULONG       ulIndex,
    OUT PULONG      pulValue, OPTIONAL
    OUT PULONG      pulLen, OPTIONAL
    OUT PULONG      pulEnd, OPTIONAL
    OUT PULONG      pulAlign, OPTIONAL
    OUT PULONG      pulFlags OPTIONAL
    )
{
    //
    // keep local copies
    // we transfer to parameters at end
    //
    ULONG ulValue;
    ULONG ulLen;
    ULONG ulEnd;
    ULONG ulAlign;
    ULONG ulFlags;

    switch (ResType) {

        case ResType_Mem: {

            PMEM_RESOURCE  pMemData = (PMEM_RESOURCE)pData;

            ulValue = (ULONG)pMemData->MEM_Data[ulIndex].MR_Min;
            ulLen   = (ULONG)pMemData->MEM_Data[ulIndex].MR_nBytes;
            ulEnd   = (ULONG)pMemData->MEM_Data[ulIndex].MR_Max;
            ulFlags = (ULONG)pMemData->MEM_Data[ulIndex].MR_Flags;
            ulAlign = (ULONG)pMemData->MEM_Data[ulIndex].MR_Align;
            break;
        }

        case ResType_IO:  {

            PIO_RESOURCE   pIoData = (PIO_RESOURCE)pData;

            ulValue = (ULONG)pIoData->IO_Data[ulIndex].IOR_Min;
            ulLen   = (ULONG)pIoData->IO_Data[ulIndex].IOR_nPorts;
            ulEnd   = (ULONG)pIoData->IO_Data[ulIndex].IOR_Max;
            ulFlags = (ULONG)pIoData->IO_Data[ulIndex].IOR_RangeFlags;
            ulAlign = (ULONG)pIoData->IO_Data[ulIndex].IOR_Align;
            break;
        }

        case ResType_DMA: {

            PDMA_RESOURCE  pDmaData = (PDMA_RESOURCE)pData;

            ulValue = (ULONG)pDmaData->DMA_Data[ulIndex].DR_Min;
            ulLen   = 1;
            ulEnd   = ulValue;
            ulFlags = (ULONG)pDmaData->DMA_Data[ulIndex].DR_Flags;
            ulAlign = 1;
            break;
        }

        case ResType_IRQ: {

            PIRQ_RESOURCE  pIrqData = (PIRQ_RESOURCE)pData;

            ulValue = (ULONG)pIrqData->IRQ_Data[ulIndex].IRQR_Min;
            ulLen   = 1;
            ulEnd   = ulValue;
            ulFlags = (ULONG)pIrqData->IRQ_Data[ulIndex].IRQR_Flags;
            ulAlign = 1;
            break;
        }
    }

    if(ulEnd < ulValue) {
        //
        // filter out bad/zero-length range
        //
        ulLen = 0;
    }

    pAlignValues(&ulValue, ulValue, ulLen, ulEnd, ulAlign,1);

    //
    // copy return parameters
    //
    if (pulValue) {
        *pulValue = ulValue;
    }
    if (pulLen) {
        *pulLen = ulLen;
    }
    if (pulEnd) {
        *pulEnd = ulEnd;
    }
    if (pulAlign) {
        *pulAlign = ulAlign;
    }
    if (pulFlags) {
        *pulFlags = ulFlags;
    }


    return;

}

BOOL
pAlignValues(
    IN OUT PULONG  pulValue,
    IN     ULONG   ulStart,
    IN     ULONG   ulLen,
    IN     ULONG   ulEnd,
    IN     ULONG   ulAlignment,
    IN     int     Increment
    )
{
    ULONG NtAlign = ~ulAlignment + 1;   // convert from mask to modulus
    ULONG Value;
    ULONG Upp;
    ULONG Remainder;

    Value = *pulValue;

    if (NtAlign == 0) {
        return FALSE;   // bogus alignment value
    }

    if (NtAlign != 1 && Increment != 0) {
        //
        // see if we are aligned
        //

        Remainder = Value % NtAlign;

        if (Remainder != 0) {
            //
            // need to re-align
            //
            if (Increment>0) {
                //
                // Return the first valid aligned value greater than this value
                //
                Value += NtAlign - Remainder;

                if (Value <= *pulValue) {
                    //
                    // overflow detected
                    //
                    return FALSE;
                }

            } else {
                //
                // Return the first valid aligned value less than this value
                //
                Value -= Remainder;
                //
                // we never overflow going down, since zero is a common denominator
                // of alignment
                //
            }

        }
    }

    //
    // now check boundaries
    //

    if (Value < ulStart) {
        return FALSE;
    }

    Upp = Value+ulLen-1;
    if (Upp < Value) {
        //
        // catch overflow error
        //
        return FALSE;
    }
    if (Upp > ulEnd) {
        return FALSE;
    }

    //
    // set newly aligned value
    //

    *pulValue = Value;

    return TRUE;

}

void
pFormatResString(
    LPTSTR      lpszString,
    ULONG       ulVal,
    ULONG       ulLen,
    RESOURCEID  ResType
    )
{
    if (ulLen == 0) {
        wsprintf(lpszString, szNoValue);
    } else if ((ResType == ResType_DMA) || (ResType == ResType_IRQ)) {
        wsprintf(lpszString, szOneDecNoConflict, ulVal);
    } else if (ResType == ResType_IO) {
        wsprintf(lpszString, szTwoWordHexNoConflict, (WORD)ulVal,
                 (WORD)(ulVal + ulLen - 1));
    } else {
        wsprintf(lpszString, szTwoDWordHexNoConflict, ulVal,
                 (ulVal + ulLen - 1));
    }

}

BOOL
pUnFormatResString(
    LPTSTR      lpszString,
    PULONG      pulVal,
    PULONG      pulEnd,
    RESOURCEID  ridResType
    )
{
    BOOL     bRet = FALSE;
    LPTSTR   lpszTemp = NULL;
    LPTSTR   lpszTemp2 = NULL;
    LPTSTR   lpszCopy;

    // BUGBUG - extend this to handling DWORDLONG values

    //
    // Allocate space for, and make a copy of the input string
    //
    lpszCopy = MyMalloc((lstrlen(lpszString)+1) * sizeof(TCHAR));

    if (lpszCopy == NULL) {
        return FALSE;
    }

    lstrcpy(lpszCopy, lpszString);

    //
    // Locate the dash if there is one, and convert the white space prev to
    // the dash to a NULL. (ie 0200 - 0400 while be 0200)
    //
    lpszTemp = lpszCopy;
    while ((*lpszTemp != '-') && (*lpszTemp != '\0')) {
        lpszTemp++;     // AnsiNext BUGBUG ??
    }

    if (*lpszTemp != '\0') {
        lpszTemp2 = lpszTemp-1;
        ++lpszTemp;
    }

    //
    // Search back to set the NULL for the Value
    //
    if (lpszTemp2 != NULL) {
        while ((*lpszTemp2 == ' ') || (*lpszTemp2 == '\t'))
            lpszTemp2--;    // AnsiPrev BUGBUG ??
        *(lpszTemp2+1)= '\0';
    }

    //
    // Convert the first entry
    //
    if (pConvertEditText(lpszCopy, pulVal, ridResType)) {
        //
        // If there is a second entry, convert it, otherwise assume a length
        // of one.
        //
        if (*lpszTemp != '\0') {
            if (pConvertEditText(lpszTemp, pulEnd,ridResType)) {
                bRet = TRUE;
            }
        } else {
            *pulEnd = *pulVal;
            bRet = TRUE;
        }
    }

    MyFree(lpszCopy);
    return bRet;

}

BOOL
pConvertEditText(
    LPTSTR      lpszConvert,
    PULONG      pulVal,
    RESOURCEID  ridResType
    )
{
    LPTSTR   lpConvert;

    if ((ridResType == ResType_Mem) || (ridResType == ResType_IO)) {
        *pulVal = _tcstoul(lpszConvert, &lpConvert, (WORD)16);
    } else {
        *pulVal = _tcstoul(lpszConvert, &lpConvert, (WORD)10);
    }

    if (lpConvert == lpszConvert+lstrlen(lpszConvert)) {
        return TRUE;
    } else {
        return FALSE;
    }

} // ConvertEditText

void
pWarnResSettingNotEditable(
    HWND    hDlg,
    WORD    idWarning
    )
{
    TCHAR    szTitle[MAX_PATH];
    TCHAR    szMessage[MAX_PATH * 2];

    //
    // Give some warning Messages.  If there is no logconfig,
    // then we cannot edit any settings, if there is, then
    // just the setting they are choosing is not editable.
    //
    LoadString(MyDllModuleHandle, IDS_DEVRES_NOMODIFYTITLE, szTitle, MAX_PATH);
    LoadString(MyDllModuleHandle, idWarning, szMessage, MAX_PATH * 2);
    MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);

} // WarnResSettingsNotEditable

int
pWarnNoSave(
    HWND    hDlg,
    WORD    idWarning
    )
/*++

Routine Description:

    Warn that the settings will not be saved

Arguments:

Return Value:

    IDCANCEL = don't proceed
    IDOK/IDYES/IDNO = proceed without saving

--*/
{
    TCHAR    szTitle[MAX_PATH];
    TCHAR    szMessage[MAX_PATH * 2];
    int      res;

    //
    // Give a warning message of why we can't save settings
    //
    LoadString(MyDllModuleHandle, IDS_MAKE_FORCED_TITLE, szTitle, MAX_PATH);
    LoadString(MyDllModuleHandle, idWarning, szMessage, MAX_PATH * 2);

    //res = MessageBox(hDlg, szMessage, szTitle, MB_OKCANCEL | MB_TASKMODAL | MB_ICONEXCLAMATION);
    //return res;
    res = MessageBox(hDlg, szMessage, szTitle, MB_OK | MB_TASKMODAL | MB_ICONEXCLAMATION);
    return IDCANCEL;
}

LPVOID
pGetListViewItemData(
    HWND hList,
    int iItem,
    int iSubItem
    )
{
    LV_ITEM lviItem;

    lviItem.mask = LVIF_PARAM;
    lviItem.iItem = iItem;
    lviItem.iSubItem = iSubItem;

    if (ListView_GetItem(hList, &lviItem)) {
        return (LPVOID)lviItem.lParam;
    } else {
        return NULL;
    }

} // GetListViewItemData

BOOL
pSaveDevResSettings(
    LPDMPROP_DATA   lpdmpd
    )

/*++

Routine Description:

    This routine saves the resources based on the users selections.
    MEMPHIS COMPATIBLE.

Arguments:

    lpdmpd          Property data.

Return Value:

   Returns TRUE if the function succeeded and FALSE if it failed.

--*/

{
    HWND        hDlg = lpdmpd->hDlg;
    HWND        hList =  GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);

    CONFIGRET   Status = CR_SUCCESS;
    LOG_CONF    ForcedLogConf;
    RES_DES     ResDes, ResDesTemp, ResDes1;
    RESOURCEID  ResType;
    ULONG       ulSize = 0, ulCount = 0, i = 0, iCur = 0;
    LPBYTE      pData = NULL;
    PITEMDATA   pItemData = NULL;
    BOOL        bRet = TRUE;
    SP_PROPCHANGE_PARAMS PropChangeParams;
    HMACHINE        hMachine = pGetMachine(lpdmpd);

    if ((lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS)!=0) {

        //-------------------------------------------------------------------
        // If the user checked the "Use Automatic Settings" checkbox, then
        // delete any Boot/Forced configs, otherwise write the current settings
        // as a forced config.
        //-------------------------------------------------------------------

        if (CM_Get_First_Log_Conf_Ex(&ForcedLogConf, lpdmpd->lpdi->DevInst,
                                     FORCED_LOG_CONF,hMachine) == CR_SUCCESS) {
            CM_Free_Log_Conf_Ex(ForcedLogConf, 0,hMachine);
            CM_Free_Log_Conf_Handle(ForcedLogConf);
        }

        // Let the helper modules (class installer/co-installers) get in on the act...
        //
        PropChangeParams.ClassInstallHeader.cbSize          = sizeof(SP_CLASSINSTALL_HEADER);
        PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

        PropChangeParams.StateChange = DICS_PROPCHANGE;
        PropChangeParams.Scope       = DICS_FLAG_GLOBAL;
        // no need to set PropChangeParams.HwProfile, since this is a global property change.

        DoInstallActionWithParams(DIF_PROPERTYCHANGE,
                                  lpdmpd->hDevInfo,
                                  lpdmpd->lpdi,
                                  (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                  sizeof(PropChangeParams),
                                  INSTALLACTION_CALL_CI
                                  );
    } else {

        //-------------------------------------------------------------------
        // The Use Automatic Settings is not selected.
        //-------------------------------------------------------------------

        bRet = pSaveCustomResSettings(lpdmpd,hMachine);
    }

    return bRet;
}

BOOL
pSaveCustomResSettings(
    LPDMPROP_DATA   lpdmpd,
    IN HMACHINE     hMachine
    )

/*++

Routine Description:

    This routine saves custom (user edited) resources. MEMPHIS COMPATIBLE but
    extracted from Memphis version of SaveDevResSetting().

Arguments:

    lpdmpd      Property data.

Return Value:

   Returns TRUE if the function succeeded and FALSE if it failed.

--*/

{
    HWND        hDlg = lpdmpd->hDlg;

    TCHAR       szWarn[MAX_MSG_LEN];
    TCHAR       szTitle[MAX_MSG_LEN];
    TCHAR       szTemp[MAX_MSG_LEN];
    DWORD       dwPriority, dwLCPri;
    LOG_CONF    ForcedLogConf;
    RES_DES     ResDes;
    HWND        hList = GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
    PITEMDATA   pItemData = NULL;
    LONG        iCur;
    BOOL        bRet = FALSE;
    SP_PROPCHANGE_PARAMS PropChangeParams;
    DWORD       HardReconfigFlag;
    SP_DEVINSTALL_PARAMS DevInstallParams;
    PRESDES_ENTRY pResList = NULL, pResDesEntry = NULL, pTemp = NULL;
    PITEMDATA_LISTNODE ItemDataList = NULL, ItemDataListEntry, ItemDataListEnd = NULL;
    PGENERIC_RESOURCE pGenRes;
    ULONG i, ulValue, ulLen, ulEnd, ulFlags;
    LOG_CONF    LogConf;
    ULONG       ulSize;
    ULONG       ulConfigFlags;
    HCURSOR     hOldCursor;
    BOOL        UsingMatch = FALSE;

    LogConf = lpdmpd->SelectedLC;
    if (LogConf == 0) {
        LogConf = lpdmpd->MatchingLC;
        UsingMatch = TRUE;
    }
    if (LogConf == 0) {
        LogConf = lpdmpd->CurrentLC;
        UsingMatch = FALSE;
    }
    if (LogConf == 0) {
        //MYASSERT(FALSE);
        return FALSE;
    }
    //
    // form the "warning - do you want to continue" message
    //
    LoadString(MyDllModuleHandle, IDS_MAKE_FORCED_TITLE, szTitle, MAX_MSG_LEN);
    LoadString(MyDllModuleHandle, IDS_FORCEDCONFIG_WARN1, szWarn, MAX_MSG_LEN);
    LoadString(MyDllModuleHandle, IDS_FORCEDCONFIG_WARN2, szTemp, MAX_MSG_LEN);
    lstrcat(szWarn, szTemp);
    LoadString(MyDllModuleHandle, IDS_FORCEDCONFIG_WARN3, szTemp, MAX_MSG_LEN);
    lstrcat(szWarn, szTemp);
    LoadString(MyDllModuleHandle, IDS_FORCEDCONFIG_WARN4, szTemp, MAX_MSG_LEN);
    lstrcat(szWarn, szTemp);

    //
    // If the LCPRI is soft configurable, and the user chooses YES to the
    // warning, then save the new config.  If the LCPRI is not soft
    // configurable, just save with no warning
    //
    dwLCPri = pGetMinLCPriority(lpdmpd->lpdi->DevInst, BASIC_LOG_CONF,hMachine);

    if (((dwLCPri >= LCPRI_DESIRED) && (dwLCPri <= LCPRI_LASTSOFTCONFIG)) &&
          (MessageBox(hDlg, szWarn, szTitle, MB_YESNO|MB_ICONEXCLAMATION) == IDNO)) {
        //
        // user doesn't want to change anything
        //
        bRet = FALSE;

    } else {
        //
        // We're still using the selected basic LC, but use the range index
        // embedded in the listview control
        // BUGBUG - also need to check the value to see if a user
        // overrode it (is this possible?)
        //
        bRet = TRUE;

        if (CM_Get_First_Log_Conf_Ex(&ForcedLogConf, lpdmpd->lpdi->DevInst,
                                  FORCED_LOG_CONF,hMachine) == CR_SUCCESS) {
            CM_Free_Log_Conf_Ex(ForcedLogConf, 0,hMachine);
            CM_Free_Log_Conf_Handle(ForcedLogConf);
        }

        //
        // Save the current choices as the forced config
        //
        CM_Add_Empty_Log_Conf_Ex(&ForcedLogConf, lpdmpd->lpdi->DevInst, LCPRI_FORCECONFIG,
                              FORCED_LOG_CONF | PRIORITY_EQUAL_FIRST,hMachine);

        pGetResDesDataList(LogConf, &pResList, FALSE,hMachine);
        pResDesEntry = pResList;

        if (UsingMatch && (lpdmpd->dwFlags & DMPROP_FLAG_MATCH_OUT_OF_ORDER)) {
            //
            // The resource descriptors are out-of-order.  Maintain the original ordering.
            //
            // First, build up a linked list of the data in the listview resource items.
            //
            iCur = (int)ListView_GetNextItem(hList, -1, LVNI_ALL);

            while (iCur != -1) {

                pItemData = (PITEMDATA)pGetListViewItemData(hList, iCur, 0);
                if (pItemData) {
                    //
                    // Allocate an item data list node for this data.
                    //
                    ItemDataListEntry = MyMalloc(sizeof(ITEMDATA_LISTNODE));
                    if (!ItemDataListEntry) {
                        bRet = FALSE;
                        goto clean0;
                    }

                    ItemDataListEntry->ItemData = pItemData;
                    ItemDataListEntry->Next = NULL;

                    //
                    // Append this new item to the end of our list.
                    //
                    if (ItemDataListEnd) {
                        ItemDataListEnd->Next = ItemDataListEntry;
                    } else {
                        ItemDataList = ItemDataListEntry;
                    }
                    ItemDataListEnd = ItemDataListEntry;
                }

                iCur = (int)ListView_GetNextItem(hList, iCur, LVNI_ALL);
            }

            //
            // Now loop through each resdes entry, writing each one out.  For each one, check
            // to see if it has a corresponding entry in our listview item data list.
            //
            while (pResDesEntry) {
                pGenRes = (PGENERIC_RESOURCE)pResDesEntry->ResDesData;

                for(ItemDataListEntry = ItemDataList, ItemDataListEnd = NULL;
                    ItemDataListEntry;
                    ItemDataListEnd = ItemDataListEntry, ItemDataListEntry = ItemDataListEntry->Next)
                {
                    if(pResDesEntry->ResDesType == ItemDataListEntry->ItemData->ResType) {

                        for (i = 0; i < pGenRes->GENERIC_Header.GENERIC_Count; i++) {

                            pGetRangeValues(pResDesEntry->ResDesData, pResDesEntry->ResDesType, i,
                                           &ulValue, &ulLen, &ulEnd, NULL, &ulFlags);

                            if ((ItemDataListEntry->ItemData->ulLen == ulLen) &&
                                (ItemDataListEntry->ItemData->ulValue >= ulValue) &&
                                (ItemDataListEntry->ItemData->ulEnd <= ulEnd)) {
                                //
                                // We found the matching resource descriptor.  Write this out.
                                //
                                pWriteValuesToForced(ForcedLogConf,
                                                    ItemDataListEntry->ItemData->ResType,
                                                    ItemDataListEntry->ItemData->RangeCount,
                                                    ItemDataListEntry->ItemData->MatchingResDes,
                                                    ItemDataListEntry->ItemData->ulValue,
                                                    ItemDataListEntry->ItemData->ulLen,
                                                    ItemDataListEntry->ItemData->ulEnd,
                                                    hMachine );
                                //
                                // Remove this item from our list.
                                //
                                if (ItemDataListEnd) {
                                    ItemDataListEnd->Next = ItemDataListEntry->Next;
                                } else {
                                    ItemDataList = ItemDataListEntry->Next;
                                }
                                MyFree(ItemDataListEntry);

                                break;
                            }
                        }

                        if(i < pGenRes->GENERIC_Header.GENERIC_Count) {
                            //
                            // Then we broke out of the loop early, which means we found a match
                            // already.
                            //
                            break;
                        }
                    }
                }

                //
                // If we didn't find a match, then go ahead and write out the non-arbitrated
                // resdes.
                //
                if (!ItemDataListEntry) {
                    pWriteResDesRangeToForced(ForcedLogConf,
                                             pResDesEntry->ResDesType,
                                             0,
                                             0,
                                             pResDesEntry->ResDesData,
                                             hMachine);
                }

                pResDesEntry = (PRESDES_ENTRY)pResDesEntry->Next;
            }

        } else {

            iCur = (int)ListView_GetNextItem(hList, -1, LVNI_ALL);

            while (iCur != -1) {

                pItemData = (PITEMDATA)pGetListViewItemData(hList, iCur, 0);

                if (pItemData) {

                    // retrieve values

                    while (pResDesEntry &&
                           (pItemData->ResType != pResDesEntry->ResDesType)) {
                        //
                        // write out any preceding non arbitrated resources
                        //
                        pWriteResDesRangeToForced(ForcedLogConf,
                                                 pResDesEntry->ResDesType,
                                                 0,
                                                 0,
                                                 pResDesEntry->ResDesData,
                                                 hMachine);

                        pResDesEntry = (PRESDES_ENTRY)pResDesEntry->Next;
                    }
                    if (pGetMatchingResDes(pItemData->ulValue,
                                          pItemData->ulLen,
                                          pItemData->ulEnd,
                                          pItemData->ResType,
                                          LogConf,
                                          &ResDes,
                                          hMachine)) {
                        //
                        // Write the first range as the chosen forced resource
                        //
                        pWriteValuesToForced(ForcedLogConf, pItemData->ResType,
                                            pItemData->RangeCount, ResDes,
                                            pItemData->ulValue,
                                            pItemData->ulLen,
                                            pItemData->ulEnd,
                                            hMachine);
                    }
                }

                if (pResDesEntry) {
                    pResDesEntry = (PRESDES_ENTRY)pResDesEntry->Next;
                } else {
                    MYASSERT(pResDesEntry);
                }
                iCur = (int)ListView_GetNextItem(hList, iCur, LVNI_ALL);
            }

            while (pResDesEntry) {
                //
                // write out any subsequent non arbitrated resources
                //
                pWriteResDesRangeToForced(ForcedLogConf,
                                         pResDesEntry->ResDesType,
                                         0,
                                         0,
                                         pResDesEntry->ResDesData,
                                         hMachine);

                pResDesEntry = (PRESDES_ENTRY)pResDesEntry->Next;
            }
        }

        CM_Free_Log_Conf_Handle(ForcedLogConf);

        //
        // consider clearing problem flags
        //
        ulSize = sizeof(ulConfigFlags);
        if (CM_Get_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                             CM_DRP_CONFIGFLAGS,
                                             NULL, (LPBYTE)&ulConfigFlags,
                                             &ulSize, 0,hMachine) == CR_SUCCESS) {
            if ((ulConfigFlags & CONFIGFLAG_PARTIAL_LOG_CONF) != 0) {
                //
                // have flag(s) to change
                // CONFIGFLAG_PARTIAL_LOG_CONF should be cleared - we should have written a complete config now
                //
                ulConfigFlags &= ~ (CONFIGFLAG_PARTIAL_LOG_CONF);
                CM_Set_DevInst_Registry_Property_Ex(lpdmpd->lpdi->DevInst,
                                                 CM_DRP_CONFIGFLAGS,
                                                 (LPBYTE)&ulConfigFlags,
                                                 sizeof(ulConfigFlags),
                                                 0,
                                                 hMachine);
            }

        }

        //
        // Give the class installer/co-installers a crack at the propchange process.
        //
        PropChangeParams.ClassInstallHeader.cbSize          = sizeof(SP_CLASSINSTALL_HEADER);
        PropChangeParams.ClassInstallHeader.InstallFunction = DIF_PROPERTYCHANGE;

        PropChangeParams.StateChange = DICS_PROPCHANGE;
        PropChangeParams.Scope       = DICS_FLAG_GLOBAL;
        // no need to set PropChangeParams.HwProfile, since this is a global property change.

        DoInstallActionWithParams(DIF_PROPERTYCHANGE,
                                  lpdmpd->hDevInfo,
                                  lpdmpd->lpdi,
                                  (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                  sizeof(PropChangeParams),
                                  INSTALLACTION_CALL_CI | INSTALLACTION_NO_DEFAULT
                                 );

        //
        // Check the Priority of this LC.  If it is greater
        // than LCPRI_LASTSOFTCONFIG, then we need to reboot
        // otherwise try the dynamic changestate route.
        //

        if (CM_Get_Log_Conf_Priority_Ex(LogConf, &dwPriority, 0,hMachine) != CR_SUCCESS) {
            dwPriority = LCPRI_LASTSOFTCONFIG;
        }

        if (dwPriority <= LCPRI_LASTSOFTCONFIG) {
            //
            // Do the default action for SoftConfigable devices, which
            // will attempt to restart the device with the new config
            // This could take a while so use an hourglass
            //
            hOldCursor = SetCursor(LoadCursor(NULL, IDC_WAIT));
            DoInstallActionWithParams(DIF_PROPERTYCHANGE,
                                      lpdmpd->hDevInfo,
                                      lpdmpd->lpdi,
                                      (PSP_CLASSINSTALL_HEADER)&PropChangeParams,
                                      sizeof(PropChangeParams),
                                      0  // don't call class-installer, just do default action
                                     );
            SetCursor(hOldCursor);
            HardReconfigFlag = 0;

        } else if((dwPriority > LCPRI_LASTSOFTCONFIG) && (dwPriority <= LCPRI_RESTART)) {
            HardReconfigFlag = DI_NEEDRESTART;
        } else {
            HardReconfigFlag = DI_NEEDREBOOT;
                //
                // Set hardreconfig flag if user needs to change hardware jumpers
                //
                // BUGBUG (lonnym): This is a DevMgr flag, not a Device Installer one.
                // Do we need to convey this information to DevMgr, and if so, how???
                //
                // if (dwPriority >= LCPRI_HARDRECONFIG)
                //     *((LPDWORD)(lpdmpd->psp.lParam)) |= PROPCHG_FLAG_SHUTDOWN;
        }

        lpdmpd->dwFlags |= DMPROP_FLAG_CHANGESSAVED;

        //
        // Properties have changed, so set flags to indicate if restart/reboot is required,
        // and to tell DevMgr to re-init the UI.
        //
        DevInstallParams.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
        if(SetupDiGetDeviceInstallParams(lpdmpd->hDevInfo,
                                         lpdmpd->lpdi,
                                         &DevInstallParams)) {

            DevInstallParams.Flags |= (HardReconfigFlag | DI_PROPERTIES_CHANGE);

            SetupDiSetDeviceInstallParams(lpdmpd->hDevInfo,
                                          lpdmpd->lpdi,
                                          &DevInstallParams
                                         );
        }

        //
        // If we need to reboot, then set a problem on the device that indicates this (in case
        // the user doesn't listen to us, we want to flag this devnode so that the user will see
        // that this devnode needs a reboot if they go into DevMgr, etc.)
        //
        if(HardReconfigFlag) {
            PDEVICE_INFO_SET pDeviceInfoSet;
            PDEVINFO_ELEM DevInfoElem;

            pDeviceInfoSet = AccessDeviceInfoSet(lpdmpd->hDevInfo);
            //
            // We'd better be able to access this device information set!
            //
            MYASSERT(pDeviceInfoSet);
            //
            // In case we couldn't don't bother trying to set the needs-reboot problem,
            // because the whole mess is invalid!
            //
            if(pDeviceInfoSet) {

                try {
                    DevInfoElem = FindAssociatedDevInfoElem(pDeviceInfoSet, lpdmpd->lpdi, NULL);
                    //
                    // We'd better be able to find this element!
                    //
                    MYASSERT(DevInfoElem);
                    //
                    // In case we can't find it, don't try to set any problem on the devnode.
                    //
                    if(DevInfoElem) {
                        //
                        // BUGBUG!!! (jamiehun) Change text below if/when this does set problem
                        //
                        SetDevnodeNeedsRebootProblem(DevInfoElem->DevInst,
                                                     NULL,
                                                     DevInfoElem->InstallParamBlock.LogContext,
                                                     MSG_LOG_REBOOT_DEVRES
                                                    );
                    }
                } finally {
                    UnlockDeviceInfoSet(pDeviceInfoSet);
                }
            }

        }
    }

clean0:

    while (ItemDataList) {
        ItemDataListEntry = ItemDataList->Next;
        MyFree(ItemDataList);
        ItemDataList = ItemDataListEntry;
    }

    pDeleteResDesDataList(pResList);

    return bRet;

} // SaveCustomResSettings

BOOL
pWriteResDesRangeToForced(
    IN LOG_CONF     ForcedLogConf,
    IN RESOURCEID   ResType,
    IN ULONG        RangeIndex,
    IN RES_DES      RD,             OPTIONAL
    IN LPBYTE       ResDesData,     OPTIONAL
    IN HMACHINE     hMachine        OPTIONAL
    )
{
    RES_DES ResDes;
    ULONG   ulSize;
    LPBYTE  pData = NULL;


    if ((RD == 0) && (ResDesData == NULL)) {
        return FALSE;   // pass in data or handle!
    }

    if (!ResDesData) {

        if (CM_Get_Res_Des_Data_Size_Ex(&ulSize, RD, 0,hMachine) != CR_SUCCESS) {
            CM_Free_Res_Des_Handle(RD);
            return FALSE;
        }

        pData = MyMalloc(ulSize);
        if (pData == NULL) {
            CM_Free_Res_Des_Handle(RD);
            return FALSE;
        }

        if (CM_Get_Res_Des_Data_Ex(RD, pData, ulSize, 0,hMachine) != CR_SUCCESS) {
            CM_Free_Res_Des_Handle(RD);
            MyFree(pData);
            return FALSE;
        }
    } else {
        pData = ResDesData;
    }

    //
    // convert the first range data into hdr data
    //
    switch (ResType) {

        case ResType_Mem: {

            PMEM_RESOURCE pMemData = (PMEM_RESOURCE)pData;
            PMEM_RESOURCE pForced = (PMEM_RESOURCE)MyMalloc(sizeof(MEM_RESOURCE));

            pForced->MEM_Header.MD_Count      = 0;
            pForced->MEM_Header.MD_Type       = MType_Range;
            pForced->MEM_Header.MD_Alloc_Base = pMemData->MEM_Data[RangeIndex].MR_Min;
            pForced->MEM_Header.MD_Alloc_End  = pMemData->MEM_Data[RangeIndex].MR_Min +
                                                pMemData->MEM_Data[RangeIndex].MR_nBytes - 1;
            pForced->MEM_Header.MD_Flags      = pMemData->MEM_Data[RangeIndex].MR_Flags;
            pForced->MEM_Header.MD_Reserved   = 0;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_Mem, pForced,
                           sizeof(MEM_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_IO:  {

            PIO_RESOURCE pIoData = (PIO_RESOURCE)pData;
            PIO_RESOURCE pForced = (PIO_RESOURCE)MyMalloc(sizeof(IO_RESOURCE));

            pForced->IO_Header.IOD_Count      = 0;
            pForced->IO_Header.IOD_Type       = IOType_Range;
            pForced->IO_Header.IOD_Alloc_Base = pIoData->IO_Data[RangeIndex].IOR_Min;
            pForced->IO_Header.IOD_Alloc_End  = pIoData->IO_Data[RangeIndex].IOR_Min +
                                                pIoData->IO_Data[RangeIndex].IOR_nPorts - 1;
            pForced->IO_Header.IOD_DesFlags   = pIoData->IO_Data[RangeIndex].IOR_RangeFlags;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_IO, pForced,
                           sizeof(IO_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_DMA: {

            PDMA_RESOURCE pDmaData = (PDMA_RESOURCE)pData;
            PDMA_RESOURCE pForced = (PDMA_RESOURCE)MyMalloc(sizeof(DMA_RESOURCE));

            pForced->DMA_Header.DD_Count      = 0;
            pForced->DMA_Header.DD_Type       = DType_Range;
            pForced->DMA_Header.DD_Flags      = pDmaData->DMA_Data[RangeIndex].DR_Flags;
            pForced->DMA_Header.DD_Alloc_Chan = pDmaData->DMA_Data[RangeIndex].DR_Min;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_DMA, pForced,
                           sizeof(DMA_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_IRQ: {

            PIRQ_RESOURCE pIrqData = (PIRQ_RESOURCE)pData;
            PIRQ_RESOURCE pForced = (PIRQ_RESOURCE)MyMalloc(sizeof(IRQ_RESOURCE));

            pForced->IRQ_Header.IRQD_Count     = 0;
            pForced->IRQ_Header.IRQD_Type      = IRQType_Range;
            pForced->IRQ_Header.IRQD_Flags     = pIrqData->IRQ_Data[RangeIndex].IRQR_Flags;
            pForced->IRQ_Header.IRQD_Alloc_Num = pIrqData->IRQ_Data[RangeIndex].IRQR_Min;
            pForced->IRQ_Header.IRQD_Affinity  = 0xFFFFFFFF;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_IRQ, pForced,
                           sizeof(IRQ_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_BusNumber: {

            PBUSNUMBER_RESOURCE pBusData = (PBUSNUMBER_RESOURCE)pData;
            PBUSNUMBER_RESOURCE pForced = (PBUSNUMBER_RESOURCE)MyMalloc(sizeof(BUSNUMBER_RESOURCE));

            pForced->BusNumber_Header.BUSD_Count      = 0;
            pForced->BusNumber_Header.BUSD_Type       = BusNumberType_Range;
            pForced->BusNumber_Header.BUSD_Flags      = pBusData->BusNumber_Data[RangeIndex].BUSR_Flags;
            pForced->BusNumber_Header.BUSD_Alloc_Base = pBusData->BusNumber_Data[RangeIndex].BUSR_Min;
            pForced->BusNumber_Header.BUSD_Alloc_End  = pBusData->BusNumber_Data[RangeIndex].BUSR_Min +
                                                  pBusData->BusNumber_Data[RangeIndex].BUSR_nBusNumbers;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_BusNumber, pForced,
                           sizeof(BUSNUMBER_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_DevicePrivate: {

            PDEVPRIVATE_RESOURCE pPrvData = (PDEVPRIVATE_RESOURCE)pData;
            PDEVPRIVATE_RESOURCE pForced = (PDEVPRIVATE_RESOURCE)MyMalloc(sizeof(DEVPRIVATE_RESOURCE));

            pForced->PRV_Header.PD_Count = 0;
            pForced->PRV_Header.PD_Type  = PType_Range;
            pForced->PRV_Header.PD_Data1 = pPrvData->PRV_Data[RangeIndex].PR_Data1;
            pForced->PRV_Header.PD_Data2 = pPrvData->PRV_Data[RangeIndex].PR_Data2;
            pForced->PRV_Header.PD_Data3 = pPrvData->PRV_Data[RangeIndex].PR_Data3;
            pForced->PRV_Header.PD_Flags = 0;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_DevicePrivate, pForced,
                           sizeof(DEVPRIVATE_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_PcCardConfig: {

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_PcCardConfig, pData,
                           sizeof(PCCARD_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            break;
        }
    }

    return TRUE;

} // WriteResDesRangeToForced

BOOL
pWriteValuesToForced(
    IN LOG_CONF     ForcedLogConf,
    IN RESOURCEID   ResType,
    IN ULONG        RangeIndex,
    IN RES_DES      RD,
    IN ULONG        ulValue,
    IN ULONG        ulLen,
    IN ULONG        ulEnd,
    IN HMACHINE     hMachine
    )
{
    RES_DES ResDes;
    ULONG   ulSize;
    LPBYTE  pData = NULL;


    if (CM_Get_Res_Des_Data_Size_Ex(&ulSize, RD, 0,hMachine) != CR_SUCCESS) {
        CM_Free_Res_Des_Handle(RD);
        return FALSE;
    }

    pData = MyMalloc(ulSize);
    if (pData == NULL) {
        CM_Free_Res_Des_Handle(RD);
        return FALSE;
    }

    if (CM_Get_Res_Des_Data_Ex(RD, pData, ulSize, 0,hMachine) != CR_SUCCESS) {
        CM_Free_Res_Des_Handle(RD);
        MyFree(pData);
        return FALSE;
    }

    //
    // convert the first range data into hdr data
    //
    switch (ResType) {

        case ResType_Mem: {

            PMEM_RESOURCE pMemData = (PMEM_RESOURCE)pData;
            PMEM_RESOURCE pForced = (PMEM_RESOURCE)MyMalloc(sizeof(MEM_RESOURCE));

            pForced->MEM_Header.MD_Count      = 0;
            pForced->MEM_Header.MD_Type       = MType_Range;
            pForced->MEM_Header.MD_Alloc_Base = ulValue;
            pForced->MEM_Header.MD_Alloc_End  = ulEnd;
            pForced->MEM_Header.MD_Flags      = pMemData->MEM_Data[RangeIndex].MR_Flags;
            pForced->MEM_Header.MD_Reserved   = 0;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_Mem, pForced,
                              sizeof(MEM_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_IO:  {

            PIO_RESOURCE pIoData = (PIO_RESOURCE)pData;
            PIO_RESOURCE pForced = (PIO_RESOURCE)MyMalloc(sizeof(IO_RESOURCE));

            pForced->IO_Header.IOD_Count      = 0;
            pForced->IO_Header.IOD_Type       = IOType_Range;
            pForced->IO_Header.IOD_Alloc_Base = ulValue;
            pForced->IO_Header.IOD_Alloc_End  = ulEnd;
            pForced->IO_Header.IOD_DesFlags   = pIoData->IO_Data[RangeIndex].IOR_RangeFlags;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_IO, pForced,
                              sizeof(IO_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_DMA: {

            PDMA_RESOURCE pDmaData = (PDMA_RESOURCE)pData;
            PDMA_RESOURCE pForced = (PDMA_RESOURCE)MyMalloc(sizeof(DMA_RESOURCE));

            pForced->DMA_Header.DD_Count      = 0;
            pForced->DMA_Header.DD_Type       = DType_Range;
            pForced->DMA_Header.DD_Flags      = pDmaData->DMA_Data[RangeIndex].DR_Flags;
            pForced->DMA_Header.DD_Alloc_Chan = ulValue;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_DMA, pForced,
                              sizeof(DMA_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_IRQ: {

            PIRQ_RESOURCE pIrqData = (PIRQ_RESOURCE)pData;
            PIRQ_RESOURCE pForced = (PIRQ_RESOURCE)MyMalloc(sizeof(IRQ_RESOURCE));

            pForced->IRQ_Header.IRQD_Count     = 0;
            pForced->IRQ_Header.IRQD_Type      = IRQType_Range;
            pForced->IRQ_Header.IRQD_Flags     = pIrqData->IRQ_Data[RangeIndex].IRQR_Flags;
            pForced->IRQ_Header.IRQD_Alloc_Num = ulValue;
            pForced->IRQ_Header.IRQD_Affinity  = 0xFFFFFFFF;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_IRQ, pForced,
                              sizeof(IRQ_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_BusNumber: {

            PBUSNUMBER_RESOURCE pBusData = (PBUSNUMBER_RESOURCE)pData;
            PBUSNUMBER_RESOURCE pForced = (PBUSNUMBER_RESOURCE)MyMalloc(sizeof(BUSNUMBER_RESOURCE));

            pForced->BusNumber_Header.BUSD_Count      = 0;
            pForced->BusNumber_Header.BUSD_Type       = BusNumberType_Range;
            pForced->BusNumber_Header.BUSD_Flags      = pBusData->BusNumber_Data[RangeIndex].BUSR_Flags;
            pForced->BusNumber_Header.BUSD_Alloc_Base = ulValue;
            pForced->BusNumber_Header.BUSD_Alloc_End  = ulEnd;

            CM_Add_Res_Des_Ex(&ResDes, ForcedLogConf, ResType_BusNumber, pForced,
                              sizeof(BUSNUMBER_RESOURCE), 0,hMachine);
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pForced);
            break;
        }

        case ResType_DevicePrivate: {
            break;
        }

        case ResType_PcCardConfig: {
            break;
        }
    }

    return TRUE;

} // WriteValuesToForced

BOOL
MakeResourceData(
    OUT LPBYTE     *ppResourceData,
    OUT PULONG     pulSize,
    IN  RESOURCEID ResType,
    IN  ULONG      ulValue,
    IN  ULONG      ulLen,
    IN  ULONG      ulFlags
    )
{
    BOOL bStatus = TRUE;

    try {

        switch (ResType) {

            case ResType_Mem: {

                PMEM_RESOURCE p;

                *pulSize = sizeof(MEM_RESOURCE);
                if (ppResourceData) {
                    *ppResourceData = MyMalloc(*pulSize);
                    p = (PMEM_RESOURCE)(*ppResourceData);

                    p->MEM_Header.MD_Count      = 0;
                    p->MEM_Header.MD_Type       = MType_Range;
                    p->MEM_Header.MD_Alloc_Base = ulValue;
                    p->MEM_Header.MD_Alloc_End  = ulValue + ulLen - 1;
                    p->MEM_Header.MD_Flags      = ulFlags;
                    p->MEM_Header.MD_Reserved   = 0;
                }
                break;
            }

            case ResType_IO:  {

                PIO_RESOURCE p;

                *pulSize = sizeof(IO_RESOURCE);
                if (ppResourceData) {
                    *ppResourceData = MyMalloc(*pulSize);
                    p = (PIO_RESOURCE)(*ppResourceData);

                    p->IO_Header.IOD_Count      = 0;
                    p->IO_Header.IOD_Type       = IOType_Range;
                    p->IO_Header.IOD_Alloc_Base = ulValue;
                    p->IO_Header.IOD_Alloc_End  = ulValue + ulLen - 1;
                    p->IO_Header.IOD_DesFlags   = ulFlags;
                }
                break;
            }

            case ResType_DMA: {

                PDMA_RESOURCE p;

                *pulSize = sizeof(DMA_RESOURCE);
                if (ppResourceData) {
                    *ppResourceData = MyMalloc(*pulSize);
                    p = (PDMA_RESOURCE)(*ppResourceData);

                    p->DMA_Header.DD_Count      = 0;
                    p->DMA_Header.DD_Type       = DType_Range;
                    p->DMA_Header.DD_Flags      = ulFlags;
                    p->DMA_Header.DD_Alloc_Chan = ulValue;
                }
                break;
            }

            case ResType_IRQ: {

                PIRQ_RESOURCE p;

                *pulSize = sizeof(IRQ_RESOURCE);
                if (ppResourceData) {
                    *ppResourceData = MyMalloc(*pulSize);
                    p = (PIRQ_RESOURCE)(*ppResourceData);

                    p->IRQ_Header.IRQD_Count     = 0;
                    p->IRQ_Header.IRQD_Type      = IRQType_Range;
                    p->IRQ_Header.IRQD_Flags     = ulFlags;
                    p->IRQ_Header.IRQD_Alloc_Num = ulValue;
                    p->IRQ_Header.IRQD_Affinity  = 0xFFFFFFFF; //BUGBUG
                }
                break;
            }

            default:
                //
                // ResTypeEditable or ResType_MAX may be wrong if this ASSERT's
                //
                MYASSERT(FALSE);
                bStatus = FALSE;
        }

    } except(EXCEPTION_EXECUTE_HANDLER) {
        bStatus = FALSE;
    }

    return bStatus;

} // MakeResourceData


BOOL
pShowWindow(
    IN HWND hWnd,
    IN int nShow
    )
/*++

Routine Description:

    A variation of ShowWindow that enables/disables window

Arguments:

    (See ShowWindow)
    hWnd  - handle of window to show
    nShow - typically SW_HIDE or SW_SHOW

Return Value:

    success status of ShowWindow

--*/
{
    EnableWindow(hWnd,nShow!=SW_HIDE);
    return ShowWindow(hWnd,nShow);
}


BOOL
pEnableWindow(
    IN HWND hWnd,
    IN BOOL Enable
    )
/*++

Routine Description:

    A variation of EnableWindow that only enables a window if it is visible

Arguments:

    (See EnableWindow)
    hWnd  - handle of window to enable/disable
    Enable - TRUE enables window (if window visible) FALSE disables window

Return Value:

    success status of EnableWindow

--*/
{
    //
    // I had to use GetWindowLong, as IsWindowVisible also checks parent flag
    // and parent is hidden until dialog is initialized
    //
    if((GetWindowLong(hWnd,GWL_STYLE) & WS_VISIBLE) == FALSE) {
        Enable = FALSE;
    }
    return EnableWindow(hWnd,Enable);
}

BOOL
pGetResDesDataList(
    IN LOG_CONF LogConf,
    IN OUT PRESDES_ENTRY *pResList,
    IN BOOL bArbitratedOnly,
    IN HMACHINE hMachine
    )
/*++

Routine Description:

    Creates a list of resource descriptors for further processing

Arguments:

    LogConf  - log config of interest
    pResList - list out
    bArbitratedOnly - filter out non-arbitrated resources
    hMachine - machine that LogConf is on

Return Value:

   None.

--*/
{
    BOOL bStatus = TRUE;
    CONFIGRET Status = CR_SUCCESS;
    PRESDES_ENTRY pHead = NULL, pEntry = NULL, pPrevious = NULL, pTemp = NULL;
    RES_DES     ResDes;
    RESOURCEID  ResType;
    ULONG       ulSize;
    LPBYTE      pData = NULL;

    //
    // Retrieve each res des in this log conf
    //

    Status = CM_Get_Next_Res_Des_Ex(&ResDes, LogConf, ResType_All, &ResType, 0,hMachine);

    while (Status == CR_SUCCESS) {

        if (bArbitratedOnly && (ResType <= ResType_None || ResType > ResType_MAX)) {
            goto NextResDes;
        }
        if (bArbitratedOnly && ResTypeEditable[ResType] == FALSE) {
            goto NextResDes;
        }

        if (CM_Get_Res_Des_Data_Size_Ex(&ulSize, ResDes, 0,hMachine) != CR_SUCCESS) {
            CM_Free_Res_Des_Handle(ResDes);
            bStatus = FALSE;
            goto Clean0;
        }

        if (ulSize>0) {
            pData = MyMalloc(ulSize);
            if (pData == NULL) {
                CM_Free_Res_Des_Handle(ResDes);
                bStatus = FALSE;
                goto Clean0;
            }

            if (CM_Get_Res_Des_Data_Ex(ResDes, pData, ulSize, 0,hMachine) != CR_SUCCESS) {
                CM_Free_Res_Des_Handle(ResDes);
                MyFree(pData);
                bStatus = FALSE;
                goto Clean0;
            }
        } else {
            pData = NULL;
        }

        //
        // Allocate a node for this res des and attach it to the list
        //

        pEntry = MyMalloc(sizeof(RESDES_ENTRY));
        if (pEntry == NULL) {
            CM_Free_Res_Des_Handle(ResDes);
            MyFree(pData);
            bStatus = FALSE;
            goto Clean0;
        }

        pEntry->ResDesData = pData;
        pEntry->ResDesType = ResType;
        pEntry->ResDesDataSize = ulSize;
        pEntry->ResDesHandle = ResDes;
        pEntry->Next = NULL;
        pEntry->CrossLink = NULL;

        if (!pHead) {
            pHead = pEntry;             // first entry
        }

        if (pPrevious) {
            pPrevious->Next = pEntry; // attach to previous entry
        }

        pPrevious = pEntry;

        //
        // Get next res des in LogConf
        //
    NextResDes:

        Status = CM_Get_Next_Res_Des_Ex(&ResDes, ResDes, ResType_All, &ResType, 0,hMachine);
    }

    bStatus = TRUE;

    Clean0:

    if (!bStatus) {
        pDeleteResDesDataList(pHead);
    } else {
        *pResList = pHead;
    }

    return bStatus;

} // GetResDesDataList

VOID
pDeleteResDesDataList(
    IN PRESDES_ENTRY pResList
    )
/*++

Routine Description:

    Deletes memory used by RESDES list

Arguments:

    pResList - list returned by GetResDesDataList

Return Value:

   None.

--*/
{
    PRESDES_ENTRY pTemp;
    while (pResList) {
        pTemp = pResList;
        pResList = (PRESDES_ENTRY)pResList->Next;
        if (pTemp->ResDesData) {
            MyFree(pTemp->ResDesData);
        }
        if (pTemp->ResDesHandle) {
            CM_Free_Res_Des_Handle(pTemp->ResDesHandle);
        }
        MyFree(pTemp);
    }
}

VOID
pHideAllControls(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Hide (and disable) all controls - start off with a clean slate
    Only Icon & device description will be visible

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;

    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_SETTINGSTATE), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_RESOURCES_TEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NOALLOCTEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_CHANGE_TEXT ), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MAKEFORCED), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MFPARENT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MFPARENT_DESC), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTDEVTEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTINFOLIST), SW_HIDE);

    lpdmpd->dwFlags |= DMPROP_FLAG_VIEWONLYRES;
}

VOID
pShowViewNoResources(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Show page indicating this device has no resources

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;
    TCHAR           szString[MAX_PATH];

    pHideAllControls(lpdmpd); // all hidden and disabled
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_RESOURCES_TEXT), SW_SHOW);   // show and enable text
    LoadString(MyDllModuleHandle, IDS_DEVRES_NO_RESOURCES, szString, MAX_PATH);
    SetDlgItemText(hDlg, IDC_DEVRES_NO_RESOURCES_TEXT, szString);
}

BOOL
pShowViewMFReadOnly(
    IN LPDMPROP_DATA lpdmpd,
    IN BOOL HideIfProb
    )
/*++

Routine Description:

    Show page apropriate for multifunction card that cannot be edited
    Resource settings are visible

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    none

--*/
{
    TCHAR           szString[MAX_PATH];
    DEVNODE         dnParent;
    ULONG           ulSize;
    HWND hDlg = lpdmpd->hDlg;
    HMACHINE        hMachine = pGetMachine(lpdmpd);

    pHideAllControls(lpdmpd); // all hidden and disabled
    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), SW_SHOW);   // show config information
    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), SW_SHOW); // show
    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTDEVTEXT), SW_SHOW); // show conflict information space
    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTINFOLIST), SW_SHOW);
    //
    // indicate we cannot change as it's multi-function
    //
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_CHANGE_TEXT), SW_SHOW);
    if (LoadString(MyDllModuleHandle, IDS_DEVRES_NO_CHANGE_MF, szString, MAX_PATH)) {
        SetDlgItemText(hDlg, IDC_DEVRES_NO_CHANGE_TEXT,  szString);
    }
    //
    // for parent description
    //
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MFPARENT), SW_SHOW);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MFPARENT_DESC), SW_SHOW);
    //
    // Get the Parent's Description.
    //
    LoadString(MyDllModuleHandle, IDS_DEVNAME_UNK, szString, MAX_PATH);

    if (lpdmpd->lpdi->DevInst) {

        if (CM_Get_Parent_Ex(&dnParent, lpdmpd->lpdi->DevInst, 0,hMachine)
                          == CR_SUCCESS) {


            //
            // First, try to retrieve friendly name, then fall back to device description.
            //
            ulSize = MAX_PATH * sizeof(TCHAR);
            if(CM_Get_DevNode_Registry_Property_Ex(dnParent, CM_DRP_FRIENDLYNAME,
                                                NULL, szString, &ulSize, 0,hMachine) != CR_SUCCESS) {

                ulSize = MAX_PATH * sizeof(TCHAR);
                CM_Get_DevNode_Registry_Property_Ex(dnParent, CM_DRP_DEVICEDESC,
                                                 NULL, szString, &ulSize, 0,hMachine);
            }
        }
    }

    SetDlgItemText(hDlg, IDC_DEVRES_MFPARENT_DESC, szString);

    //
    // load and display current config (if any)
    // return FALSE if no current config
    //
    return pLoadCurrentConfig(lpdmpd,HideIfProb);
}

BOOL
pShowViewReadOnly(
    IN LPDMPROP_DATA lpdmpd,
    IN BOOL HideIfProb
    )
/*++

Routine Description:

    Show page of resources, don't allow editing, don't show editing controls

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;

    pHideAllControls(lpdmpd); // all hidden and disabled
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), SW_SHOW);   // show
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), FALSE);
    ShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), SW_SHOW); // shown disabled
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), FALSE);
    ShowWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), SW_SHOW); // shown disabled
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), FALSE);
    ShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), SW_SHOW); // shown disabled
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), FALSE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTDEVTEXT), SW_SHOW); // show conflict information space
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTINFOLIST), SW_SHOW);

    //
    // will indicate if we're showing system settings or forced settings
    //
    CheckDlgButton(hDlg, IDC_DEVRES_USESYSSETTINGS, (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS ) ? TRUE : FALSE);

    //
    // load and display current config (if any)
    // return FALSE if no current config
    //
    return pLoadCurrentConfig(lpdmpd,HideIfProb);
}

VOID
pShowViewNoAlloc(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Modify the middle part of the control to indicate there is a problem (and there isn't much we can do about it)

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;

    //
    // hide all middle controls
    //
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), SW_HIDE);
    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_CHANGE_TEXT ), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), SW_HIDE);
    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MAKEFORCED), SW_HIDE);
    lpdmpd->dwFlags |= DMPROP_FLAG_VIEWONLYRES;

    //pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_CHANGE_TEXT), SW_SHOW);  // this may say why
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MAKEFORCED), SW_HIDE);

    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTDEVTEXT), SW_HIDE); // no alloc, so hide this header & textbox
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTINFOLIST), SW_HIDE);
}

VOID
pShowViewNeedForced(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Modify the middle part of the control to indicate a forced config is required

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;

    pShowViewNoAlloc(lpdmpd);
    //
    // show what we need for make forced config
    //
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_CHANGE_TEXT), SW_SHOW);  // this may say why
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MAKEFORCED), SW_SHOW);
}

VOID
pShowViewAllEdit(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Allow editing

Arguments:

    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;

    //
    // show middle controls for editing
    //
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), SW_SHOW);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), SW_SHOW);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_CHANGE_TEXT ), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), SW_SHOW);
    ShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), SW_SHOW); // shown, but disabled
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), FALSE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_MAKEFORCED), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTDEVTEXT), SW_SHOW); // show conflict information space
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CONFLICTINFOLIST), SW_SHOW);

    pShowUpdateEdit(lpdmpd);
}

VOID
pShowUpdateEdit(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Allow editing

Arguments:

    lpdmpd = property data

Return Value:

    none

--*/
{
    HWND hDlg = lpdmpd->hDlg;

    //
    // modify editing status - we can edit
    //
    lpdmpd->dwFlags &= ~DMPROP_FLAG_VIEWONLYRES;

    if(lpdmpd->dwFlags & DMPROP_FLAG_FORCEDONLY) {
        //
        // in this case, we will never be able to use system settings
        //
        lpdmpd->dwFlags &= ~ DMPROP_FLAG_USESYSSETTINGS;
        EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), FALSE);
    }
    //
    // indicate if it's system settings or not
    //
    CheckDlgButton(hDlg, IDC_DEVRES_USESYSSETTINGS,
                    (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS)?TRUE:FALSE);
    //
    // we can change logconfiglist if it's not system settings
    //
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS)?FALSE:TRUE);
    EnableWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS)?FALSE:TRUE);
    //
    // change "Change Settings" button
    //
    pCheckEnableResourceChange(lpdmpd);
}

BOOL
pLoadCurrentConfig(
    IN LPDMPROP_DATA lpdmpd,
    BOOL HideIfProb
    )
/*++

Routine Description:

    Modify the top part, to show current configuration, if any

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data

Return Value:

    TRUE if we're showing current config

--*/
{
    TCHAR    szMessage[MAX_PATH];
    ULONG    Problem;
    ULONG    Status;
    HWND hDlg = lpdmpd->hDlg;
    HMACHINE hMachine = pGetMachine(lpdmpd);
    BOOL     DoLoadConfig = FALSE;

    lpdmpd->SelectedLC = 0;
    lpdmpd->SelectedLCType = lpdmpd->CurrentLCType;

    if (lpdmpd->CurrentLC != 0) {
        DoLoadConfig = TRUE;
    }
    if ((lpdmpd->lpdi->DevInst==0)
        || (CM_Get_DevNode_Status_Ex(&Status, &Problem, lpdmpd->lpdi->DevInst,
                                      0,hMachine) != CR_SUCCESS)) {
        Status = 0;
        Problem = 0;
    }
    if(HideIfProb && (Status & DN_HAS_PROBLEM)!=0) {
        //
        // if there's a problem and HideIfProb is TRUE, don't bother showing current config
        //
        DoLoadConfig = FALSE;
    }
    if (DoLoadConfig) {
        //
        // load in current configuration
        //
        pLoadConfig(lpdmpd,lpdmpd->CurrentLC,lpdmpd->CurrentLCType);
        return TRUE;
    }
    //
    // case where there is no suitable configuration
    //
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_RESOURCES_TEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LCTEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_LOGCONFIGLIST), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_USESYSSETTINGS), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_CHANGE), SW_HIDE);
    pShowViewNoAlloc(lpdmpd);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_SETTINGSTATE), SW_SHOW);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NOALLOCTEXT), SW_SHOW);

    //
    // explain why there is a problem
    // goes into NOALLOCTEXT
    //
    // BugBug!!! (jamiehun) 7/27/99
    // If "HideIfProb" is set, we may be lying
    // need to fix post Win2k
    //
    LoadString(MyDllModuleHandle, IDS_DEVRES_NOALLOC_PROBLEM, szMessage, MAX_PATH);

    //
    // consider being more descriptive
    //

    if ((Status & DN_HAS_PROBLEM)!=0) {

        switch (Problem) {
            case CM_PROB_DISABLED:
            case CM_PROB_HARDWARE_DISABLED:
                LoadString(MyDllModuleHandle, IDS_DEVRES_NOALLOC_DISABLED, szMessage, MAX_PATH);
                break;

            case CM_PROB_NORMAL_CONFLICT:
                LoadString(MyDllModuleHandle, IDS_DEVRES_NORMAL_CONFLICT, szMessage, MAX_PATH);
                break;
            default:
                break;
        }
    }
    SetDlgItemText(hDlg, IDC_DEVRES_NOALLOCTEXT, szMessage);

    return FALSE; // display in NoAlloc state
}

BOOL
pConfigHasNoAlternates(
    LPDMPROP_DATA lpdmpd,
    LOG_CONF testLC
    )
/*++

Routine Description:

    A Basic config could be restrictive "these are the set of resources to use"
    This determines if the basic config passed is such a config

Arguments:

    testLC = basic config to test

Return Value:

    TRUE if it's a singular config

--*/
{
    HMACHINE      hMachine = NULL;
    PRESDES_ENTRY pConfigValues = NULL;
    PRESDES_ENTRY pValue = NULL;
    BOOL          bSuccess = TRUE;
    ULONG         ulValue = 0, ulLen = 0, ulEnd = 0, ulFlags = 0;
    PGENERIC_RESOURCE pGenRes = NULL;

    hMachine = pGetMachine(lpdmpd);
    pGetResDesDataList(testLC, &pConfigValues, TRUE, hMachine); // arbitratable resources
    for(pValue = pConfigValues;pValue;pValue = pValue->Next) {
        //
        // is this a singular value?
        //
        pGenRes = (PGENERIC_RESOURCE)(pValue->ResDesData);
        if(pGenRes->GENERIC_Header.GENERIC_Count != 1) {
            //
            // more than one entry - not singular
            //
            bSuccess = FALSE;
            break;
        }
        pGetRangeValues(pValue->ResDesData, pValue->ResDesType, 0, &ulValue, &ulLen, &ulEnd, NULL, &ulFlags);
        if (ulValue+(ulLen-1) != ulEnd) {
            //
            // not singular
            //
            bSuccess = FALSE;
            break;
        }
    }
    pDeleteResDesDataList(pConfigValues);

    return bSuccess;
}

BOOL
pLoadConfig(
    LPDMPROP_DATA lpdmpd,
    LOG_CONF forceLC,
    ULONG forceLCType
    )
/*++

Routine Description:

    Display a configuration

Arguments:

    hDlg = dialog handle of controls
    lpdmpd = property data
    forceLC = LogConf to display
    forceLCType = type for LogConf

Return Value:

    TRUE if config loaded

--*/
{
    HWND hDlg = lpdmpd->hDlg;
    CONFIGRET   Status = CR_SUCCESS;
    HWND        hWndList;
    LV_ITEM     lviItem;
    TCHAR       szTemp[MAX_PATH];
    int         iNewItem = 0;
    ULONG       ulValue, ulLen, ulEnd, ulAlign, ulSize, ulFlags;
    ULONG       ulRange;
    LPBYTE      pData = NULL;
    RES_DES     ResDes;
    RESOURCEID  ResType;
    PITEMDATA   pItemData = NULL;
    HMACHINE    hMachine = NULL;
    PDEVICE_INFO_SET pDeviceInfoSet;
    BOOL        RetCode = FALSE;
    PRESDES_ENTRY pKnownValues = NULL;
    PRESDES_ENTRY pShowValues = NULL;
    PRESDES_ENTRY pShowEntry = NULL;
    BOOL        bFixedConfig = FALSE;
    BOOL        bNoMatch;
    BOOL        bFixed;
    ULONG       MatchLevel = NO_LC_MATCH;


    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_SETTINGSTATE), SW_SHOW);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NO_RESOURCES_TEXT), SW_HIDE);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST), SW_SHOW);
    pShowWindow(GetDlgItem(hDlg, IDC_DEVRES_NOALLOCTEXT), SW_HIDE);

    hWndList = GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
    SendMessage(hWndList, WM_SETREDRAW, (WPARAM)FALSE, 0);
    ListView_DeleteAllItems(hWndList);

    lpdmpd->dwFlags |= DMPROP_FLAG_FIXEDCONFIG; // until we determine there is at least one setting we can edit


    if (forceLC == 0) {
        forceLC = lpdmpd->CurrentLC;
        forceLCType = lpdmpd->CurrentLCType;
    }
    if (forceLC == 0) {
        MYASSERT(FALSE);
        goto Final;
    }
    hMachine = pGetMachine(lpdmpd);

    //
    // setup values that will remain the same each time I add an item
    //
    lviItem.mask     = LVIF_TEXT | LVIF_PARAM | LVIF_IMAGE;
    lviItem.pszText  = szTemp;          // reuse the szTemp buffer
    lviItem.iSubItem = 0;
    lviItem.iImage   = IDI_RESOURCE - IDI_RESOURCEFIRST;

    pGetResDesDataList(forceLC, &pShowValues, TRUE, hMachine); // arbitratable resources
    if (forceLCType == BOOT_LOG_CONF || forceLCType == FORCED_LOG_CONF || forceLCType == ALLOC_LOG_CONF) {
        bFixedConfig = TRUE;
        if (forceLC == lpdmpd->CurrentLC && lpdmpd->MatchingLC != 0) {
            //
            // we're displaying CurrentLC, use flags & resdes's from matching LC where possible
            //
            if (pGetResDesDataList(lpdmpd->MatchingLC, &pKnownValues, TRUE, hMachine)) {
                //
                // match-up currentLC with some matching LC, so we can use flags/ranges from matching LC
                //
                MatchLevel = pMergeResDesDataLists(pShowValues,pKnownValues,NULL);
            }
        }
    } else if (lpdmpd->CurrentLC != 0) {
        //
        // the config we're displaying may allow ranges of values
        // we're going to try and match up what we are displaying to current config
        //
        if (pGetResDesDataList(lpdmpd->CurrentLC, &pKnownValues, TRUE, hMachine)) {
            //
            // try and use current values where possible
            //
            MatchLevel = pMergeResDesDataLists(pKnownValues,pShowValues,NULL);
        }
    }

    pShowEntry = pShowValues;

    while (pShowEntry) {
        bNoMatch = FALSE;
        bFixed = FALSE;
        ResDes = (RES_DES)0;
        ResType = pShowEntry->ResDesType;
        ulRange = 0;

        if (bFixedConfig) {
            //
            // we've got a current config
            //
            pGetHdrValues(pShowEntry->ResDesData, pShowEntry->ResDesType, &ulValue, &ulLen, &ulEnd, &ulFlags);
            if (pShowEntry->CrossLink) {
                //
                // use range's res-des
                //
                ResDes = pShowEntry->CrossLink->ResDesHandle;
                pShowEntry->CrossLink->ResDesHandle = (RES_DES)0;
                //
                // allow adjustment based on nearest basic config
                //
                pGetMatchingRange(ulValue,ulLen,pShowEntry->CrossLink->ResDesData, pShowEntry->CrossLink->ResDesType,&ulRange,&bFixed,NULL);
            } else {
                //
                // no range res-des
                //
                ResDes = (RES_DES)0;
                //
                // indicate that this is a non-adjustable value
                //
                bFixed = TRUE;
            }
        } else {
            //
            // we've got resource-ranges
            //
            if (pShowEntry->CrossLink) {
                //
                // take current settings from what we merged in
                //
                pGetHdrValues(pShowEntry->CrossLink->ResDesData, pShowEntry->CrossLink->ResDesType, &ulValue, &ulLen, &ulEnd, &ulFlags);
            } else {
                //
                // just take first range
                //
                pGetRangeValues(pShowEntry->ResDesData, pShowEntry->ResDesType, 0, &ulValue, &ulLen, &ulEnd, &ulAlign, &ulFlags);
            }
            pGetMatchingRange(ulValue,ulLen,pShowEntry->ResDesData, pShowEntry->ResDesType,&ulRange,&bFixed,&ulFlags);
            //
            // use res-des from range
            //
            ResDes = pShowEntry->ResDesHandle;
            pShowEntry->ResDesHandle = (RES_DES)0;

            if (pShowEntry->CrossLink == NULL && bFixed == FALSE) {
                //
                // unknown value
                //
                bNoMatch = TRUE;
            }
        }

        if (ulLen>0) {
            //
            // Write first column text field (uses szTemp, lParam is res type)
            //
            LoadString(MyDllModuleHandle, IDS_RESTYPE_FULL + ResType, szTemp, MAX_PATH);
            ulRange = 0;

            pItemData = (PITEMDATA)MyMalloc(sizeof(ITEMDATA));
            if (pItemData != NULL) {
                pItemData->ResType = ResType;
                pItemData->MatchingResDes = ResDes;
                pItemData->RangeCount = ulRange;
                pItemData->ulValue = ulValue;                   // selected value
                pItemData->ulLen = ulLen;
                pItemData->ulEnd = ulValue + ulLen - 1;
                pItemData->ulFlags = ulFlags;
                pItemData->bValid = !bNoMatch;                  // if no chosen value
                pItemData->bFixed = bFixed;
            }
            if (bFixed == FALSE) {
                //
                // we have at least one editable value
                //
                lpdmpd->dwFlags &= ~DMPROP_FLAG_FIXEDCONFIG;
            }

            lviItem.iItem = iNewItem;
            lviItem.lParam = (LPARAM)pItemData;
            ListView_InsertItem(hWndList, &lviItem);

            //
            // Write second column text field (uses szTemp, lParam is res handle)
            //
            if (bNoMatch) {
                pFormatResString(szTemp, 0, 0, ResType);
            } else {
                pFormatResString(szTemp, ulValue, ulLen, ResType);
            }
            ListView_SetItemText(hWndList, iNewItem, 1, szTemp);

            ++iNewItem;
        }
        pShowEntry = pShowEntry->Next;
    }

    SendMessage(hWndList, WM_SETREDRAW, (WPARAM)TRUE, 0);

    RetCode = TRUE;

Final:

    pDeleteResDesDataList(pKnownValues);
    pDeleteResDesDataList(pShowValues);

    //
    // initialize listview headings here
    // see also...
    // "BUGBUG!!! (jamiehun) doing this causes poor default display"
    // in Dialog Init
    //
    ListView_SetColumnWidth(hWndList, 0, LVSCW_AUTOSIZE_USEHEADER);
    ListView_SetColumnWidth(hWndList, 1, LVSCW_AUTOSIZE_USEHEADER);
    //
    // change "Change Settings" button
    //
    pCheckEnableResourceChange(lpdmpd);

    return RetCode;
}

BOOL
bIsMultiFunctionChild(
    PSP_DEVINFO_DATA lpdi,
    HMACHINE         hMachine
    )
/*++

Routine Description:

    Returns flag indicating if this is a child of a
    multifunction device
Arguments:

Return Value:

    TRUE if MF child

--*/
{
    ULONG   Status;
    ULONG   ProblemNumber;

    if (lpdi->DevInst) {

        if (CM_Get_DevNode_Status_Ex(&Status, &ProblemNumber,
                                  lpdi->DevInst, 0,hMachine) == CR_SUCCESS) {
            //
            // If the passed in dev is not an MF child, then it is the top
            // level MF_Parent
            //
            if (Status & DN_MF_CHILD) {
                return TRUE;
            } else {
                return FALSE;
            }
        }
    }

    return FALSE;

}

VOID
pSelectLogConf(
    LPDMPROP_DATA lpdmpd,
    LOG_CONF forceLC,
    ULONG forceLCType,
    BOOL Always
)
/*++

Routine Description:

    Selects a LogConf, showing the config in the LC control

Arguments:

Return Value:

    TRUE if MF child

--*/
{
    HWND hDlg = lpdmpd->hDlg;
    int count;
    int i;
    LOG_CONF LogConf;

    if (Always == FALSE && forceLC == lpdmpd->SelectedLC) {
        //
        // selection remains the same
        //
        return;
    }

    count = (int)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,CB_GETCOUNT, (WPARAM)0, (LPARAM)0);
    if (count == 0) {
        MYASSERT(FALSE/*shouldn't get here*/);
        pLoadCurrentConfig(lpdmpd,FALSE);
        return;
    }

    if (forceLC == 0 && lpdmpd->CurrentLC == 0) {
        //
        // no currentLC, so select first default
        //
        forceLC = (LOG_CONF)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,
                                               CB_GETITEMDATA, (WPARAM)0, (LPARAM)0);
        if (forceLC == (LOG_CONF)0) {
            MYASSERT(FALSE/*shouldn't get here*/);
            pLoadCurrentConfig(lpdmpd,FALSE);
            return;
        }
        forceLCType = BASIC_LOG_CONF;
    }

    for (i=0;i<count;i++) {
        LogConf = (LOG_CONF)SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,
                                               CB_GETITEMDATA, (WPARAM)i, (LPARAM)0);
        if (LogConf == forceLC) {
            //
            // set these first so we don't recurse around
            //
            lpdmpd->SelectedLC = forceLC;
            lpdmpd->SelectedLCType = forceLCType;
            //
            // change dialog to reflect new selection
            //
            SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,CB_SETCURSEL, (WPARAM)i, (LPARAM)0);
            pLoadConfig(lpdmpd,forceLC,forceLCType);
            pShowConflicts(lpdmpd);
            return;
        }
    }
    SendDlgItemMessage(hDlg, IDC_DEVRES_LOGCONFIGLIST,CB_SETCURSEL, (WPARAM)(-1), (LPARAM)0);
    pLoadConfig(lpdmpd,forceLC,forceLCType);
    pShowConflicts(lpdmpd);
}


VOID
pChangeCurrentResSetting(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Brings up edit dialog to change currently selected resource

Arguments:

Return Value:

    none

--*/
{
    HWND                hDlg = lpdmpd->hDlg;
    RESOURCEEDITINFO    rei;
    HWND                hList =  GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
    int                 iCur;
    PITEMDATA           pItemData = NULL;
    LV_ITEM             lviItem;
    GENERIC_RESOURCE    GenResInfo;
    PDEVICE_INFO_SET    pDeviceInfoSet;
    BOOL                changed = FALSE;
    TCHAR               szTemp[MAX_PATH];

    pItemData = pGetResourceToChange(lpdmpd,&iCur);
    if (pItemData == NULL) {
        //
        // we cannot edit this resource for some reason, give the user a hint
        // and maybe I'll get less ear-ache "I cannot change the settings"
        //
        if ((lpdmpd->dwFlags & DMPROP_FLAG_VIEWONLYRES)!=0 ||
                (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS)!=0) {
            //
            // editing not allowed - prob double-clicked on settings
            //
            return;
        }
        if (lpdmpd->dwFlags & DMPROP_FLAG_FIXEDCONFIG) {
            pWarnResSettingNotEditable(hDlg, IDS_DEVRES_NOMODIFYALL);
        } else {
            //
            // see if user needs to select a resource
            //
            iCur = (int)ListView_GetNextItem(hList,-1, LVNI_SELECTED);
            if (iCur == LB_ERR) {
                //
                // no selection
                //
                pWarnResSettingNotEditable(hDlg, IDS_DEVRES_NOMODIFYSELECT);
            } else {
                //
                // resource is just not editable
                //
                pWarnResSettingNotEditable(hDlg, IDS_DEVRES_NOMODIFYSINGLE);
            }
        }
        goto clean0;
    }

    ZeroMemory(&rei,sizeof(rei));
    rei.hMachine = pGetMachine(lpdmpd);
    rei.KnownLC = lpdmpd->CurrentLC;
    rei.MatchingBasicLC = lpdmpd->MatchingLC;
    rei.SelectedBasicLC = lpdmpd->SelectedLC;
    rei.lpdi = lpdmpd->lpdi;
    rei.dwPropFlags = lpdmpd->dwFlags;
    rei.bShareable = FALSE; //BUGBUG (jamiehun) do we need to fix this?
    rei.ridResType = pItemData->ResType;
    rei.ResDes = pItemData->MatchingResDes;
    rei.ulCurrentVal = pItemData->ulValue;
    rei.ulCurrentLen = pItemData->ulLen;
    rei.ulCurrentEnd = pItemData->ulEnd;
    rei.ulCurrentFlags = pItemData->ulFlags;
    rei.ulRangeCount = pItemData->RangeCount;
    rei.pData = NULL;

    if (rei.hMachine) {
        //
        // Warn the item is not editable
        //
        pWarnResSettingNotEditable(hDlg, IDS_DEVRES_NOMODIFYREMOTE);
        goto clean0;
    }

    if (DialogBoxParam(MyDllModuleHandle,
                       MAKEINTRESOURCE(IDD_EDIT_RESOURCE),
                       hDlg,
                       EditResourceDlgProc,
                       (LPARAM)(PRESOURCEEDITINFO)&rei) != IDOK) {
        goto clean0;
    }
    //
    // Update The Current Resource settings to Future
    // Settings, and update the Conflict list.
    //
    pItemData->ulValue = rei.ulCurrentVal;
    pItemData->ulLen = rei.ulCurrentLen;
    pItemData->ulEnd = rei.ulCurrentEnd;
    pItemData->ulFlags = rei.ulCurrentFlags;
    pItemData->RangeCount = rei.ulRangeCount;
    pItemData->bValid = TRUE; // indicate that user has explicitly changed this value

    pFormatResString(szTemp,
                    rei.ulCurrentVal,
                    rei.ulCurrentLen,
                    rei.ridResType);

    ListView_SetItemText(hList, iCur, 1, szTemp);
    pShowConflicts(lpdmpd);

    //
    // clear the flag for saving changes
    //
    lpdmpd->dwFlags &= ~DMPROP_FLAG_CHANGESSAVED;
    PropSheet_Changed(GetParent(hDlg), hDlg);

clean0:
    ;
}

VOID
pShowConflicts(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Selects a LogConf, showing the config in the LC control

Arguments:

Return Value:

    TRUE if MF child

--*/
{
    HWND        hDlg = lpdmpd->hDlg;
    CONFIGRET   Status = CR_SUCCESS;
    LPVOID      vaArray[4];
    TCHAR       szTemp[MAX_PATH+4], szBuffer[MAX_PATH+16], szSetting[MAX_PATH];
    TCHAR       szFormat[MAX_PATH], szItemFormat[MAX_PATH];
    TCHAR       szUnavailable[MAX_PATH];
    LPTSTR      pszConflictList = NULL, pszConflictList2 = NULL;
    ULONG       ulSize = 0, ulLength, ulBufferLen, ulNewLength;
    ULONG       ulStartOffset = 0;
    int         Count = 0, i = 0;
    PITEMDATA   pItemData = NULL;
    LPBYTE      pResourceData = NULL;
    HWND        hwndResList = GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
    HMACHINE    hMachine;
    ULONG       ConflictCount = 0;
    ULONG       ConflictIndex = 0;
    CONFLICT_LIST ConflictList = 0;
    PDEVICE_INFO_SET pDeviceInfoSet;
    CONFLICT_DETAILS ConflictDetails;
    BOOL        ReservedResource = FALSE;
    BOOL        AnyReportedResources = FALSE;
    BOOL        AnyBadResources = FALSE;
    PCONFLICT_EXCEPTIONS pConflictExceptions = NULL;
    //
    // number of resources listed
    //
    Count = ListView_GetItemCount(hwndResList);
    if (Count <= 0) {
       goto Clean0;
    }

    //
    // initial buffer that holds the strings
    // with all the conflict info in them
    //
    ulBufferLen = 2048;
    ulLength = 0;

    pszConflictList = MyMalloc(ulBufferLen * sizeof(TCHAR));
    if (pszConflictList == NULL) {
        goto Clean0;
    }
    pszConflictList[0] = 0;

    //
    // obtain machine
    //
    if(!(pDeviceInfoSet = AccessDeviceInfoSet(lpdmpd->hDevInfo))) {
            SetLastError(ERROR_INVALID_HANDLE);
            return ;
    }
    hMachine = pDeviceInfoSet->hMachine;
    UnlockDeviceInfoSet (pDeviceInfoSet);

    //
    // do these once - these format strings use %1!s! type formats (FormatMessage)
    //
    LoadString(MyDllModuleHandle, IDS_CONFLICT_FMT, szFormat, MAX_PATH);
    LoadString(MyDllModuleHandle, IDS_CONFLICT_UNAVAILABLE, szUnavailable, MAX_PATH);

    //
    // for every listed resource
    //

    for (i = 0; i < Count; i++) {

        ConflictList = 0;
        ConflictCount = 0;

        //
        // get the resource we're about to test
        //
        pItemData = (PITEMDATA)pGetListViewItemData(hwndResList, i, 0);
        if (pItemData == NULL || pItemData->bValid == FALSE) {
            //
            // for whatever reason, we don't want to show conflict information on this resource
            //
            ListView_SetItemState(hwndResList, i,
                                  INDEXTOOVERLAYMASK(0),
                                  LVIS_OVERLAYMASK);
            goto NextResource;
        }

        //
        // this is set to indicate conflict not reported, but is reserved
        //
        ReservedResource = FALSE;

        //
        // need resource-data for determining conflict
        //
        if (MakeResourceData(&pResourceData, &ulSize,
                             pItemData->ResType,
                             pItemData->ulValue,
                             pItemData->ulLen,
                             pItemData->ulFlags)) {

            Status = CM_Query_Resource_Conflict_List(&ConflictList,
                                                        lpdmpd->lpdi->DevInst,
                                                        pItemData->ResType,
                                                        pResourceData,
                                                        ulSize,
                                                        0,
                                                        hMachine);

            if (Status != CR_SUCCESS) {
                //
                // on the unlikely event of an error, remember an error occurred
                //
                ConflictList = 0;
                ConflictCount =  0;
                AnyBadResources = TRUE;
            } else {
                //
                // find out how many things conflicted
                //
                Status = CM_Get_Resource_Conflict_Count(ConflictList,&ConflictCount);
                if (Status != CR_SUCCESS) {
                    MYASSERT(Status == CR_SUCCESS);
                    ConflictCount =  0;
                    AnyBadResources = TRUE;
                }
            }
            if(ConflictCount && (lpdmpd->dwFlags & DMPROP_FLAG_SINGLE_CONFIG) && !(lpdmpd->dwFlags & DMPROP_FLAG_HASPROBLEM)) {
                //
                // BUGBUG!!! conflict suppression hack 5/25/99 jamiehun (Win2k pre RC1)
                // this stuff needs to be fixed proper
                //
                // rules are
                //   (1) device doesn't have a problem
                //   (2) device can only have one configuration (ie, there's no basic config, or the basic config is singular)
                //   (3) it has a ResourcePickerExceptions string, and that string indicates that the exception is allowed for the specific conflict

                if(pConflictExceptions==NULL) {
                    pConflictExceptions = pLoadConflictExceptions(lpdmpd);
                }

                if (pConflictExceptions) {

                    BOOL muted = TRUE;
                    //
                    // count from 0 (first conflict) through to ConflictCount (excl)
                    //
                    for(ConflictIndex = 0; ConflictIndex < ConflictCount; ConflictIndex ++) {
                        //
                        // obtain details for this conflict
                        //
                        ZeroMemory(&ConflictDetails,sizeof(ConflictDetails));
                        ConflictDetails.CD_ulSize = sizeof(ConflictDetails);
                        ConflictDetails.CD_ulMask = CM_CDMASK_DEVINST | CM_CDMASK_DESCRIPTION | CM_CDMASK_FLAGS;
                        Status = CM_Get_Resource_Conflict_Details(ConflictList,ConflictIndex,&ConflictDetails);
                        if (Status == CR_SUCCESS) {
                            if (!pIsConflictException(lpdmpd,pConflictExceptions,ConflictDetails.CD_dnDevInst,ConflictDetails.CD_szDescription,pItemData->ResType,pItemData->ulValue,pItemData->ulLen)) {
                                muted = FALSE;
                                break;
                            }
                        }
                    }
                    if(muted) {
                        ConflictCount = 0;
                    }
                }
            }
            if (ConflictCount || ReservedResource) {
                ulStartOffset = ulLength;  // record start in case we decide to backtrack
                AnyReportedResources = TRUE; // say we reported at least one problem

            TreatAsReserved:

                ulLength = ulStartOffset;
                pszConflictList[ulLength] = 0;
                //
                // we're going to mark the resource as a problem
                //

                ListView_GetItemText(hwndResList, i, 1, szSetting, MAX_PATH);


                switch (pItemData->ResType) {
                    case ResType_Mem:
                        LoadString(MyDllModuleHandle, IDS_MEMORY_FULL, szBuffer, MAX_PATH);
                        break;
                    case ResType_IO:
                        LoadString(MyDllModuleHandle, IDS_IO_FULL, szBuffer, MAX_PATH);
                        break;
                    case ResType_DMA:
                        LoadString(MyDllModuleHandle, IDS_DMA_FULL, szBuffer, MAX_PATH);
                        break;
                    case ResType_IRQ:
                        LoadString(MyDllModuleHandle, IDS_IRQ_FULL, szBuffer, MAX_PATH);
                        break;
                    default:
                        MYASSERT(FALSE);
                }

                if ( ReservedResource == FALSE) {

                    //
                    // count from 0 (header) 1 (first conflict) through to ConflictCount
                    //
                    for(ConflictIndex = 0; ConflictIndex <= ConflictCount; ConflictIndex ++) {
                        if (ConflictIndex == 0) {
                            //
                            // first pass through, do header message
                            //
                            vaArray[0] = szBuffer;
                            vaArray[1] = szSetting;
                            FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY ,
                                                szFormat,
                                                0,0,
                                                szTemp,MAX_PATH,
                                                (va_list*)vaArray); // FORMAT_MESSAGE_ARGUMENT_ARRAY

                        } else {

                            //
                            // obtain details for this conflict
                            //
                            ZeroMemory(&ConflictDetails,sizeof(ConflictDetails));
                            ConflictDetails.CD_ulSize = sizeof(ConflictDetails);
                            ConflictDetails.CD_ulMask = CM_CDMASK_DEVINST | CM_CDMASK_DESCRIPTION | CM_CDMASK_FLAGS;

                            Status = CM_Get_Resource_Conflict_Details(ConflictList,ConflictIndex-1,&ConflictDetails);
                            if (Status == CR_SUCCESS) {
                                if ((ConflictDetails.CD_ulFlags & CM_CDFLAGS_RESERVED) != 0) {
                                    //
                                    // treat as reserved - backtrack
                                    //
                                    ReservedResource = TRUE;
                                    goto TreatAsReserved;
                                } else {
                                    if (ConflictDetails.CD_szDescription[0] == 0) {
                                        //
                                        // treat as reserved - backtrack
                                        //
                                        ReservedResource = TRUE;
                                        goto TreatAsReserved;
                                    }
                                    wsprintf(szBuffer,TEXT("  %s\r\n"),ConflictDetails.CD_szDescription);
                                }
                            } else {
                                //
                                // treat as reserved
                                //
                                ReservedResource = TRUE;
                                goto TreatAsReserved;
                            }
                            lstrcpy(szTemp,szBuffer);
                        }

                        ulNewLength = ulLength + lstrlen(szTemp);   // excluding terminating NUL

                        if ((ulNewLength+1) < ulBufferLen) {
                            //
                            // need to allocate more space - we'll double it and add some more every time
                            //
                            pszConflictList2 = MyRealloc(pszConflictList,(ulBufferLen+ulNewLength+1)  * sizeof(TCHAR));
                            if (pszConflictList2 != NULL) {
                                //
                                // succeeded in resizing buffer
                                //
                                pszConflictList = pszConflictList2;
                                ulBufferLen = ulBufferLen+ulNewLength+1;
                            }
                        }
                        if ((ulNewLength+1) < ulBufferLen) {
                            lstrcpy(pszConflictList + ulLength , szTemp);
                            ulLength = ulNewLength;
                        }

                    }
                } else {
                    //
                    // there is some other problem with resource
                    //

                    vaArray[0] = szBuffer;
                    vaArray[1] = szSetting;
                    FormatMessage(FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY ,
                                        szUnavailable,
                                        0,0,
                                        szTemp,MAX_PATH,
                                        (va_list*)vaArray); // FORMAT_MESSAGE_ARGUMENT_ARRAY

                    ulNewLength = ulLength + lstrlen(szTemp);   // excluding terminating NUL

                    if ((ulNewLength+1) < ulBufferLen) {
                        //
                        // need to allocate more space - we'll double it and add some more every time
                        //
                        pszConflictList2 = MyRealloc(pszConflictList,(ulBufferLen+ulNewLength+1)  * sizeof(TCHAR));
                        if (pszConflictList2 != NULL) {
                            //
                            // succeeded in resizing buffer
                            //
                            pszConflictList = pszConflictList2;
                            ulBufferLen = ulBufferLen+ulNewLength+1;
                        }
                    }
                    if ((ulNewLength+1) < ulBufferLen) {
                        lstrcpy(pszConflictList + ulLength , szTemp);
                        ulLength = ulNewLength;
                    }
                }

                //
                // Set the Conflict Overlay for this resource.
                //
                ListView_SetItemState(hwndResList, i,
                               INDEXTOOVERLAYMASK(IDI_CONFLICT - IDI_RESOURCEOVERLAYFIRST + 1),
                               LVIS_OVERLAYMASK);

            } else {
                //
                // resource is (aparently) working fine
                //
                ListView_SetItemState(hwndResList, i,
                                      INDEXTOOVERLAYMASK(0),
                                      LVIS_OVERLAYMASK);
            }

            if (ConflictList) {
                CM_Free_Resource_Conflict_Handle(ConflictList);
            }

            if (pResourceData != NULL) {
                MyFree(pResourceData);
            }
        } else {
            //
            // couldn't make the resource descriptor
            AnyBadResources = TRUE;
        }

        NextResource:
            ;
    }


Clean0:
    ;

    //
    // If there were any conflicts, put the list in the multiline edit box.
    //
    if (AnyReportedResources) {
        SetDlgItemText(hDlg, IDC_DEVRES_CONFLICTINFOLIST, pszConflictList);
    } else if (AnyBadResources) {
        //
        // this would most likely occur on
        // (1) running this on 95/98 (shouldn't happen)
        // (2) using new setupapi on old cfgmgr32
        //
        LoadString(MyDllModuleHandle, IDS_CONFLICT_GENERALERROR, szBuffer, MAX_PATH);
        SetDlgItemText(hDlg, IDC_DEVRES_CONFLICTINFOLIST, szBuffer);
    } else {
        LoadString(MyDllModuleHandle, IDS_DEVRES_NOCONFLICTDEVS, szBuffer, MAX_PATH);
        SetDlgItemText(hDlg, IDC_DEVRES_CONFLICTINFOLIST, szBuffer);
    }
    if(pszConflictList != NULL) {
        MyFree(pszConflictList);
    }
    if (pConflictExceptions != NULL) {
        pFreeConflictExceptions(pConflictExceptions);
    }

    return;

}

int
pOkToSave(
    IN LPDMPROP_DATA lpdmpd
    )
/*++

Routine Description:

    Check to see if there's something the user hasn't done

Arguments:

Return Value:

    IDYES = save settings
    IDNO  = don't save settings
    IDCANCEL = don't exit

--*/
{
    HWND        hDlg = lpdmpd->hDlg;
    HWND        hList = GetDlgItem(hDlg, IDC_DEVRES_SETTINGSLIST);
    int         iCur;
    int         nRes;
    PITEMDATA   pItemData;

    if (lpdmpd->dwFlags & DMPROP_FLAG_NO_RESOURCES) {
        //
        // no changes - because there are no resources
        //
        return IDNO;
    }
    if (lpdmpd->dwFlags & DMPROP_FLAG_CHANGESSAVED) {
        //
        // no changes
        //
        return IDNO;
    }
    if (lpdmpd->dwFlags & DMPROP_FLAG_USESYSSETTINGS) {
        //
        // always ok to "use sys settings"
        //
        return IDYES;
    }
    //
    // user is forcing a config - let's see if all settings are valid
    //
    //
    // The resource descriptors are out-of-order.  Maintain the original ordering.
    //
    // First, build up a linked list of the data in the listview resource items.
    //
    iCur = (int)ListView_GetNextItem(hList, -1, LVNI_ALL);

    while (iCur >= 0) {

        pItemData = (PITEMDATA)pGetListViewItemData(hList, iCur, 0);
        if (pItemData) {
            if (pItemData->bValid == FALSE) {
                //
                // we've got an invalid entry - can't save
                //
                nRes = pWarnNoSave(hDlg,IDS_FORCEDCONFIG_PARTIAL);
                if (nRes != IDOK) {
                    return IDCANCEL;
                }
                return IDNO;
            }
        }

        iCur = (int)ListView_GetNextItem(hList, iCur, LVNI_ALL);
    }

    //
    // everything checks out
    //

    return IDYES;
}

