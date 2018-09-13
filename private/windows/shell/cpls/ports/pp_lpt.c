/** FILE: pp_lpt.c ********** Module Header *******************************
 *
 *  Control panel applet for configuring LPT ports.  This file contains
 *  the dialog and utility functions for managing the UI for setting LPT
 *  port parameters
 *
 @@BEGIN_DDKSPLIT
 * History:
 *      jsenior - 7/10/98 - created
 @@END_DDKSPLIT
 *
 *  Copyright (C) 1990-1995 Microsoft Corporation
 *
 *************************************************************************/

//==========================================================================
//                                Include files
//==========================================================================
#include "ports.h"
#include "pp_lpt.h"
#include <windowsx.h>

TCHAR m_szFilterResourceMethod[]    = TEXT("FilterResourceMethod");
TCHAR m_szParEnableLegacyZip[]      = TEXT("ParEnableLegacyZip");
TCHAR m_szParEnableLegacyZipRegPath[] = TEXT("SYSTEM\\CurrentControlSet\\Services\\Parallel\\Parameters");

const DWORD LptHelpIDs[]=
{
	IDC_STATIC,	IDH_NOHELP,	//Filter resource method text - help not needed
	IDC_FILTERMETHOD_TRYNOT,	idh_devmgr_portset_trynot,	//first radio button
	IDC_FILTERMETHOD_NEVER,	idh_devmgr_portset_never,	//second radio button
	IDC_FILTERMETHOD_ACCEPTANY,	idh_devmgr_portset_acceptany,	//third radio button
	IDC_LPTNUMTEXT,	idh_devmgr_portset_portnum,	//Port number text
    PP_LPT_PORT_NUMBER,	idh_devmgr_portset_LPTchoice,	//the list box for port number
    IDC_LPT_ENABLE_LEGACY, idh_devmgr_enable_legacy,    //Enable legacy detection checkbox
	0, 0
};

#define NUM_FILTER_RESOURCE_METHODS 3
DWORD FilterResourceMethods[NUM_FILTER_RESOURCE_METHODS] = {
            0,  // Try not to use an interrupt
            1,  // Never use an interrupt
            2   // Accept any interrupt given to the port
            }; 

// C Runtime
// @@BEGIN_DDKSPLIT
//
// Prototype for IsUserAdmin (actually in setupapi.dll, just not in the header)
//
BOOL
IsUserAdmin(VOID);
// @@END_DDKSPLIT
            
void
InformDriverOfChanges(BOOL NeedReboot,
                      IN PLPT_PROP_PARAMS LptPropParams);

void
LptPortOnHelp(HWND       ParentHwnd,
              LPHELPINFO HelpInfo);

BOOL
LptPortOnContextMenu(HWND HwndControl,
                     WORD Xpos,
                     WORD Ypos);

BOOL
LptPortOnInitDialog(
    HWND    ParentHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam);

ULONG
LptFillPortSettings(
    IN HWND                 ParentHwnd,
    IN PLPT_PROP_PARAMS     LptPropPageData);

ULONG
LptFillPortNameCb(
    HWND                ParentHwnd,
    PLPT_PROP_PARAMS    LptPropPageData);

void LptInitializeControls(
    IN HWND           ParentHwnd,
    IN PLPT_PROP_PARAMS LptPropPageData);

ULONG
LptSetFilterResourceMethod(
    HWND                ParentHwnd,
    PLPT_PROP_PARAMS    LptPropPageData);

ULONG
LptSavePortSettings(
    IN HWND             ParentHwnd,
    IN PLPT_PROP_PARAMS LptPropParams);

BOOL
LptPortOnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr);

void
LptPortOnCommand(
    HWND ParentHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode);

UINT
LptEnactPortNameChanges(
    IN HWND             ParentHwnd,
    IN PLPT_PROP_PARAMS LptPropParams,
    IN HKEY             hDeviceKey,
    IN UINT             NewLptNum);

void
LptEnumerateUsedPorts(
    IN HWND             ParentHwnd,
    IN PBYTE            Buffer,
    IN DWORD            BufferSize);

void 
LptInitPropParams(IN OUT PLPT_PROP_PARAMS  Params,
                  IN HDEVINFO              DeviceInfoSet,
                  IN PSP_DEVINFO_DATA      DeviceInfoData);

HPROPSHEETPAGE 
LptInitSettingsPage(PROPSHEETPAGE *      psp,
                    OUT PLPT_PROP_PARAMS Params);

INT_PTR 
APIENTRY
LptPortSettingsDlgProc(IN HWND   hDlg,
                       IN UINT   uMessage,
                       IN WPARAM wParam,
                       IN LPARAM lParam);

UINT 
CALLBACK
LptPortSettingsDlgCallback(HWND hwnd,
                           UINT uMsg,
                           LPPROPSHEETPAGE ppsp);

/*++

Routine Description: ParallelPortPropPageProvider

    Entry-point for adding additional device manager property
    sheet pages.  Registry specifies this routine under
    Control\Class\PortNode::EnumPropPage32="msports.dll,thisproc"
    entry.  This entry-point gets called only when the Device
    Manager asks for additional property pages.

Arguments:

    Info  - points to PROPSHEETPAGE_REQUEST, see setupapi.h
    AddFunc - function ptr to call to add sheet.
    Lparam - add sheet functions private data handle.

Return Value:

    BOOL: FALSE if pages could not be added, TRUE on success

--*/
BOOL
APIENTRY 
ParallelPortPropPageProvider(LPVOID                 Info,
                             LPFNADDPROPSHEETPAGE   AddFunc,
                             LPARAM                 Lparam)
{
   PSP_PROPSHEETPAGE_REQUEST pprPropPageRequest;
   PROPSHEETPAGE             psp;
   HPROPSHEETPAGE            hpsp;
   PLPT_PROP_PARAMS          params = NULL; 

   pprPropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) Info;

   // @@BEGIN_DDKSPLIT
   //
   // Only administrators are allowed to see this page
   //
   if (!IsUserAdmin()) {
       return FALSE;
   }
   // @@END_DDKSPLIT
   
   //
   // Allocate and zero out memory for the struct that will contain
   // page specific data
   //
   params = (PLPT_PROP_PARAMS) LocalAlloc(LPTR, sizeof(LPT_PROP_PARAMS));

   if (!params) {
       ErrMemDlg(GetFocus());
       return FALSE;
   }

   if (pprPropPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {
        LptInitPropParams(params,
                          pprPropPageRequest->DeviceInfoSet,
                          pprPropPageRequest->DeviceInfoData);

        hpsp = LptInitSettingsPage(&psp, params);
      
        if (!hpsp) {
            return FALSE;
        }
        
        if (!(*AddFunc)(hpsp, Lparam)) {
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }
   }

   return TRUE;
} // ParallelPortPropPageProvider

/*++

Routine Description: LptPortSettingsDlgProc

    The windows control function for the Port Settings properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
INT_PTR 
APIENTRY
LptPortSettingsDlgProc(IN HWND   hDlg,
                       IN UINT   uMessage,
                       IN WPARAM wParam,
                       IN LPARAM lParam)
{
    switch(uMessage) {
    case WM_COMMAND:
        LptPortOnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;
 
    case WM_CONTEXTMENU:
        return LptPortOnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP: 
        LptPortOnHelp(hDlg, (LPHELPINFO) lParam);
        break;
    
    case WM_INITDIALOG:
        return LptPortOnInitDialog(hDlg, (HWND)wParam, lParam); 

    case WM_NOTIFY:
        return LptPortOnNotify(hDlg, (NMHDR *)lParam);
 
    }

    return FALSE;
} // PortSettingsDialogProc

void 
LptInitPropParams(IN OUT PLPT_PROP_PARAMS  Params,
                  IN HDEVINFO              DeviceInfoSet,
                  IN PSP_DEVINFO_DATA      DeviceInfoData)
{
    BOOL                        bResult;
    DWORD                       requiredSize = 0;
    SP_DEVINFO_LIST_DETAIL_DATA detailData;

    ZeroMemory(Params, sizeof(LPT_PROP_PARAMS));

    Params->DeviceInfoSet = DeviceInfoSet;
    Params->DeviceInfoData = DeviceInfoData;
    Params->ChangesEnabled = TRUE;

    //
    // Get the device ID first: if the device path is larger then
    // MAX_PATH, we will try again with a bigger buffer
    //
    bResult = SetupDiGetDeviceInstanceId(DeviceInfoSet,
                                         DeviceInfoData,
                                         NULL,
                                         MAX_PATH,
                                         &requiredSize);

    //
    // See if we are being invoked locally or over the network.  If over the net,
    // then disable all possible changes.
    //
    detailData.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
    if (SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &detailData) &&
        detailData.RemoteMachineHandle != NULL) {
        Params->ChangesEnabled = FALSE;
    }

} // LptInitPropParams

HPROPSHEETPAGE LptInitSettingsPage(PROPSHEETPAGE *      psp,
                                   OUT PLPT_PROP_PARAMS Params)
{
    //
    // Add the Port Settings property page
    //
    psp->dwSize      = sizeof(PROPSHEETPAGE);
    psp->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    psp->hInstance   = g_hInst;
    psp->pszTemplate = MAKEINTRESOURCE(DLG_PP_LPT_PORTSETTINGS);

    //
    // following points to the dlg window proc
    //
    psp->pfnDlgProc = LptPortSettingsDlgProc;
    psp->lParam     = (LPARAM) Params;

    //
    // following points to some control callback of the dlg window proc
    //
    psp->pfnCallback = LptPortSettingsDlgCallback;

    //
    // allocate our "Ports Setting" sheet
    //
    return CreatePropertySheetPage(psp);
}


UINT 
CALLBACK
LptPortSettingsDlgCallback(HWND hwnd,
                           UINT uMsg,
                           LPPROPSHEETPAGE ppsp)
{
    PLPT_PROP_PARAMS params;

    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        params = (PLPT_PROP_PARAMS) ppsp->lParam;
        LocalFree(params);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

BOOL
LptPortOnInitDialog(
    HWND    ParentHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    )
{
    PLPT_PROP_PARAMS lptPropParams;
    lptPropParams = (PLPT_PROP_PARAMS) ((LPPROPSHEETPAGE)Lparam)->lParam;
    //
    // Save value away
    SetWindowLongPtr(ParentHwnd, DWLP_USER, (ULONG_PTR) lptPropParams);

    //
    // Initialize the dialog box parameters
    //
    LptFillPortSettings(ParentHwnd, lptPropParams);

    //
    // Set up the dialog box with these initialized parameters
    //
    LptInitializeControls(ParentHwnd, lptPropParams);

    return TRUE; 
}

/*++

Routine Description: LptFillPortSettings

    Gets the settings out of the registry ready for initializing the dialog box
    with.

Arguments:

    LptPropPageData:        the data to fill in
    ParentHwnd:             address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
LptFillPortSettings(
    IN HWND                 ParentHwnd,
    IN PLPT_PROP_PARAMS     LptPropPageData
    )
{
    HKEY  hKey;
    DWORD dwPortNameSize, dwError;
    TCHAR szCharBuffer[81];
    DWORD dwSize, dwData, dwMethod;

    //
    // Open the device key for the source device instance, and retrieve its
    // "PortName" value.
    //
    hKey = SetupDiOpenDevRegKey(LptPropPageData->DeviceInfoSet,
                                      LptPropPageData->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_ALL_ACCESS);

    if (INVALID_HANDLE_VALUE == hKey) {
        return GetLastError();
    }

    dwPortNameSize = sizeof(LptPropPageData->szLptName);
    dwError = RegQueryValueEx(hKey,
                              m_szPortName,  // "PortName"
                              NULL,
                              NULL,
                              (PBYTE)LptPropPageData->szLptName,
                              &dwPortNameSize);

    if(ERROR_SUCCESS != dwError) {
        RegCloseKey(hKey);
        return dwError;
    }

    //
    // create "lpt#:"
    //
    lstrcpy(szCharBuffer, LptPropPageData->szLptName);
    lstrcat(szCharBuffer, m_szColon);

    dwSize = sizeof(LptPropPageData->FilterResourceMethod);
    dwError = RegQueryValueEx(hKey,
                             m_szFilterResourceMethod,
                             NULL,
                             NULL,
                             (LPBYTE)(&LptPropPageData->FilterResourceMethod),
                             &dwSize);

    if (dwError != ERROR_SUCCESS) {
        //
        // value does not exist. Create our own:
        // Get Filter Resource Method information
        //
        LptPropPageData->FilterResourceMethod = 
            FilterResourceMethods[RESOURCE_METHOD_DEFAULT_IDX];
    
        dwError = RegSetValueEx(hKey,
                              m_szFilterResourceMethod,
                              0,
                              REG_DWORD,
                              (LPBYTE)(&LptPropPageData->FilterResourceMethod),
                              sizeof(LptPropPageData->FilterResourceMethod));
    }              
    RegCloseKey(hKey);

    dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           m_szParEnableLegacyZipRegPath,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey);
    if (dwError != ERROR_SUCCESS) {
        //
        // Don't have access maybe?
        //
        LptPropPageData->ParEnableLegacyZip = ENABLELEGACYZIPDEFAULT;
        return dwError;
    }
    dwSize = sizeof(LptPropPageData->ParEnableLegacyZip);
    dwError = RegQueryValueEx(hKey,
                              TEXT("ParEnableLegacyZip"),
                              NULL,
                              NULL,
                              (LPBYTE)(&LptPropPageData->ParEnableLegacyZip),
                              &dwSize);
    if (dwError != ERROR_SUCCESS) {
        //
        // value does not exist. Create our own
        //
        LptPropPageData->ParEnableLegacyZip = ENABLELEGACYZIPDEFAULT;
        dwError = RegSetValueEx(hKey,
                                m_szParEnableLegacyZip,
                                0,
                                REG_DWORD,
                                (LPBYTE)(&LptPropPageData->ParEnableLegacyZip),
                                sizeof(LptPropPageData->ParEnableLegacyZip));
    }
    RegCloseKey(hKey);

    return dwError;
} // LptFillPortSettings


/*++

Routine Description: LptInitializeControls

    Initializes all of the controls that represent Fifo

Arguments:

    ParentHwnd - handle to the dialog
    LptPropPageData - Contains all of the initial values

Return Value:

    None

--*/
void LptInitializeControls(
    IN HWND           ParentHwnd,
    IN PLPT_PROP_PARAMS LptPropPageData
    )
{
    TCHAR    szCurrentValue[40];
    HWND     hwnd;
    int      i, periodIdx;

    LptFillPortNameCb(ParentHwnd, LptPropPageData);

    LptSetFilterResourceMethod(ParentHwnd, LptPropPageData);

    //
    // Set the state of the Enable Legacy Detection checkbox
    //
    if (LptPropPageData->ParEnableLegacyZip) {
        CheckDlgButton(ParentHwnd,
                       IDC_LPT_ENABLE_LEGACY,
                       BST_CHECKED);
    } else {
        CheckDlgButton(ParentHwnd,
                       IDC_LPT_ENABLE_LEGACY,
                       BST_UNCHECKED);
    }

    if (!LptPropPageData->ChangesEnabled) {
//        EnableWindow(GetDlgItem(ParentHwnd, IDC_FIFO), FALSE);

    }
} // LptInitializeControls 

/*++

Routine Description: LptSetFilterResourceMethod

    Checks the appropriate resource method to use

Arguments:

    LptPropPageData:    where to get the data from
    ParentHwnd:         address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
LptSetFilterResourceMethod(
    HWND                ParentHwnd,
    PLPT_PROP_PARAMS    LptPropPageData
    )
{
    switch (LptPropPageData->FilterResourceMethod) {
    case FILTERMETHOD_TRYNOT:
        CheckRadioButton( ParentHwnd, // handle to dialog box                 
                          IDC_FILTERMETHOD_TRYNOT, // first button in group 
                          IDC_FILTERMETHOD_ACCEPTANY, // last button in group 
                          IDC_FILTERMETHOD_TRYNOT // selected
                          ); 
        break;
    case FILTERMETHOD_ACCEPTANY:
        CheckRadioButton( ParentHwnd, // handle to dialog box                 
                          IDC_FILTERMETHOD_TRYNOT, // first button in group 
                          IDC_FILTERMETHOD_ACCEPTANY, // last button in group 
                          IDC_FILTERMETHOD_ACCEPTANY // selected
                          );
        break;
    case FILTERMETHOD_NEVER:
        CheckRadioButton( ParentHwnd, // handle to dialog box                 
                          IDC_FILTERMETHOD_TRYNOT, // first button in group 
                          IDC_FILTERMETHOD_ACCEPTANY, // last button in group 
                          IDC_FILTERMETHOD_NEVER // selected
                          );
        break;
    default:
        CheckRadioButton( ParentHwnd, // handle to dialog box                 
                          IDC_FILTERMETHOD_TRYNOT, // first button in group 
                          IDC_FILTERMETHOD_ACCEPTANY, // last button in group 
                          IDC_FILTERMETHOD_NEVER // selected
                          );
        break;
    }
    return 0;
}

/*++

Routine Description: LptFillPortNameCb

    fill in the Port Name combo box selection with a list
    of possible un-used portnames

Arguments:

    LptPropPageData:    where to get the data from
    hDlg:               address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
LptFillPortNameCb(
    HWND                ParentHwnd,
    PLPT_PROP_PARAMS    LptPropPageData
    )
{
    BYTE  portUsage[MAX_LPT_PORT];
    DWORD tmp, portsReported;
    int   i, nEntries;
    int   nCurPortNum;
    TCHAR szLpt[40];
    TCHAR szInUse[40];
    HWND  portHwnd;

    portHwnd = GetDlgItem(ParentHwnd, PP_LPT_PORT_NUMBER);

    //
    // Check if our LptName is blank. If it is, disable the lpt port selector
    //
    if (_tcscmp(LptPropPageData->szLptName, TEXT("")) == 0) {
        EnableWindow(portHwnd, FALSE);
        EnableWindow(GetDlgItem(ParentHwnd, IDC_LPTNUMTEXT), FALSE);
        return 0;
    }

    //
    // assumes szLptPort filled in...
    //
    nCurPortNum = myatoi(&LptPropPageData->szLptName[3]);

    if (!LoadString(g_hInst, IDS_IN_USE, szInUse, CharSizeOf(szInUse))) {
        wcscpy(szInUse, _T(" (in use)"));
    }

    //
    // first tally up which ports NOT to offer in list box
    //
    ZeroMemory(portUsage, MAX_LPT_PORT/8);

    // Find out which ports not to offer
    LptEnumerateUsedPorts(ParentHwnd,
                          portUsage,
                          MAX_LPT_PORT);
    
    for(i = MIN_LPT-1; i < MAX_LPT_PORT; i++) {

       wsprintf(szLpt, TEXT("LPT%d"), i+1);
       if (portUsage[i] &&
           _tcscmp(szLpt, LptPropPageData->szLptName)) {
           wcscat(szLpt, szInUse);
       }

       ComboBox_AddString(portHwnd, szLpt);
   }

   ComboBox_SetCurSel(portHwnd, nCurPortNum-1);
   return 0;
} // FillPortNamesCb

void
LptPortOnCommand(
    HWND ParentHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PLPT_PROP_PARAMS params =
        (PLPT_PROP_PARAMS) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    if (NotifyCode == CBN_SELCHANGE) {
        PropSheet_Changed(GetParent(ParentHwnd), ParentHwnd);
    }
    else {
        switch (ControlId) {
        //
        // Because this is a prop sheet, we should never get this.
        // All notifications for ctrols outside of the sheet come through
        // WM_NOTIFY
        //
        case IDOK:
        case IDCANCEL:
            EndDialog(ParentHwnd, 0); 
            return;
        }
    }
} // LptPortOnCommand

BOOL
LptPortOnNotify(
    HWND    ParentHwnd,
    LPNMHDR NmHdr
    )
{
    PLPT_PROP_PARAMS params =
        (PLPT_PROP_PARAMS) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    switch (NmHdr->code) {
    //
    // Sent when the user clicks on Apply OR OK !!
    //
    case PSN_APPLY:
        //
        // Write out the lpt port options to the registry
        //
        LptSavePortSettings(ParentHwnd, params);
        SetWindowLongPtr(ParentHwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        return TRUE;
        
    default:
        return FALSE;
    }
} // LptPortOnNotify

/*++

Routine Description: LptSavePortSettings

    saves the advanced box settings back to the registry, if any were
    changed

Arguments:

    AdvancedData: holds the current settings and the location of of
                   the device in the registry
    ParentHwnd:          address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
LptSavePortSettings(
    IN HWND ParentHwnd,
    IN PLPT_PROP_PARAMS LptPropParams
    )
{
    HKEY   hKey;
    DWORD  dwSize, dwData;
    
    UINT  i = CB_ERR, curLptNum, newLptNum = CB_ERR;
    UINT  uiTryNotChecked, uiNeverChecked, uiAcceptAnyChecked;
    DWORD curFilterResourceMethod, newFilterResourceMethod;
    DWORD curParEnableLegacyZip, newParEnableLegacyZip;
    ULONG error = ERROR_SUCCESS;
    
    SP_DEVINSTALL_PARAMS spDevInstall;
    
    //
    // Grab all of the new settings
    //
    
    // Filter resource method
    curFilterResourceMethod = LptPropParams->FilterResourceMethod;
    if (BST_CHECKED == 
        IsDlgButtonChecked(ParentHwnd, IDC_FILTERMETHOD_TRYNOT))
        newFilterResourceMethod = 0;
    else if (BST_CHECKED == 
        IsDlgButtonChecked(ParentHwnd, IDC_FILTERMETHOD_NEVER)) 
        newFilterResourceMethod = 1;
    else if (BST_CHECKED == 
        IsDlgButtonChecked(ParentHwnd, IDC_FILTERMETHOD_ACCEPTANY))
        newFilterResourceMethod = 2;
    

    // LPT port number
    curLptNum = myatoi(LptPropParams->szLptName + wcslen(m_szLPT));
    newLptNum = ComboBox_GetCurSel(GetDlgItem(ParentHwnd, PP_LPT_PORT_NUMBER));
    if (newLptNum == CB_ERR) {
        newLptNum = curLptNum;
    } else {
        newLptNum++;
    }

    // Legacy device detection
    curParEnableLegacyZip = LptPropParams->ParEnableLegacyZip;
    if (BST_CHECKED == IsDlgButtonChecked(ParentHwnd, IDC_LPT_ENABLE_LEGACY)) {
        newParEnableLegacyZip = 0x1;
    } else {
        newParEnableLegacyZip = 0x0;
    }

    //
    // See if they changed anything
    //
    if ((curLptNum == newLptNum) &&
        (curFilterResourceMethod == newFilterResourceMethod) &&
        (curParEnableLegacyZip == newParEnableLegacyZip)) {
        //
        // They didn't change anything. Just exit.
        //
        return ERROR_SUCCESS;
    }
    
    //
    // Open the device key for the source device instance
    //
    hKey = SetupDiOpenDevRegKey(LptPropParams->DeviceInfoSet,
                                     LptPropParams->DeviceInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DEV,
                                     KEY_ALL_ACCESS);
    
    if (INVALID_HANDLE_VALUE == hKey) {
        // BUGBUG
        return ERROR_SUCCESS;
    }
    
    // Check the LPT port name for changes
    if (newLptNum != curLptNum) {
        LptEnactPortNameChanges(ParentHwnd,
                                LptPropParams,
                                hKey,
                                newLptNum);
    }
    
    // Check the Filter resource method for changes
    if (curFilterResourceMethod != newFilterResourceMethod) {
        //
        // They changed the Filter Resource Method
        //
        dwData = newFilterResourceMethod;
        dwSize = sizeof(dwData);
        RegSetValueEx(hKey,
                m_szFilterResourceMethod,
                0,
                REG_DWORD,
                (CONST BYTE *)(&dwData),
                dwSize);
    }
    RegCloseKey(hKey);

    if (curParEnableLegacyZip != newParEnableLegacyZip) {
        //
        // Open the services path and set the new value for Legacy Parallel device
        // detection.
        //
        error = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                           m_szParEnableLegacyZipRegPath,
                           0,
                           KEY_ALL_ACCESS,
                           &hKey);
        if (error == ERROR_SUCCESS) {
            error = RegSetValueEx(hKey,
                                    m_szParEnableLegacyZip,
                                    0,
                                    REG_DWORD,
                                    (LPBYTE)(&newParEnableLegacyZip),
                                    sizeof(newParEnableLegacyZip));
            RegCloseKey(hKey);
            if (error != ERROR_SUCCESS) {
                goto ParEnableLegacyZipSetParamFailed;
            }
            if (newParEnableLegacyZip == 0) {
                //
                // We want a reboot when disabling this thing, because the parallel
                // enumerator won't get rid of legacy devices.
                //
                InformDriverOfChanges(TRUE, LptPropParams);
            } else {
                InformDriverOfChanges(FALSE, LptPropParams);
            }
        } else {
ParEnableLegacyZipSetParamFailed:
            MyMessageBox(ParentHwnd, 
                         IDS_LPT_LEGACY_FAILED, 
                         IDS_LPT_PROPERTIES,
                         MB_OK);
            //
            // Don't want to overload the user by telling them they have to 
            // reboot. Since we were unable to set things correctly, just
            // rebuild the stack.
            //
            InformDriverOfChanges(FALSE, LptPropParams);
        }   
    } else {
        InformDriverOfChanges(FALSE, LptPropParams);
    }

    return error;
} // LptSaveAdvancedSettings
    
UINT
LptEnactPortNameChanges(
    IN HWND             ParentHwnd,
    IN PLPT_PROP_PARAMS LptPropParams,
    IN HKEY             hDeviceKey,
    IN UINT             NewLptNum)
{
    HANDLE  hLpt;
    DWORD   dwError, dwNewLptNameLen;
    UINT    curLptNum;
    BYTE    portUsage[MAX_LPT_PORT];
    TCHAR   charBuffer[LINE_LEN],
            friendlyNameFormat[LINE_LEN],
            deviceDesc[LINE_LEN],
            buffer[BUFFER_SIZE],
            szNewLptName[20];
    
    //
    // Check if we're trying to rename the port to the same name.
    //
    wsprintf(szNewLptName,_T("\\DosDevices\\LPT%d"),NewLptNum);
    if (wcscmp(szNewLptName, LptPropParams->szLptName) == 0) {
        return ERROR_SUCCESS;
    } 

    //
    // Check if a valid port number has been passed in
    //
    if (MAX_LPT_PORT < NewLptNum) {
        //
        // Get out of here - exceeding array bounds
        // This should never happen in the property page since it is a hardcoded
        // selection box.  The user can't simply type a number.
        //
        MyMessageBox(ParentHwnd, IDS_LPT_NUM_ERROR, IDS_LPT_PROPERTIES,
            MB_OK | MB_ICONINFORMATION);
        return ERROR_SUCCESS;
    }

    //
    // Get an array of used ports
    //
    LptEnumerateUsedPorts(ParentHwnd,
                          portUsage,
                          MAX_LPT_PORT);
    if (portUsage[NewLptNum-1]) {
        //
        // Port name is taken by another port.  Check if user wants system to 
        // get into inconsistent state.
        //
        if (IDNO == MyMessageBox(ParentHwnd, IDS_LPT_PORT_INUSE, 
                                 IDS_LPT_PROPERTIES, MB_YESNO | 
                                 MB_ICONINFORMATION)) {
            return ERROR_SUCCESS;
        }
    }
    
    curLptNum = myatoi(LptPropParams->szLptName + wcslen(m_szLPT));

    //
    // Make sure that the port has not been opened by another application
    //
    wsprintf(buffer, L"\\\\.\\%ws", LptPropParams->szLptName); 
    hLpt = CreateFile(buffer, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                      FILE_ATTRIBUTE_NORMAL, NULL);

    //
    // If the file handle is invalid, then the Lpt port is open, warn the user
    //
    if (hLpt == INVALID_HANDLE_VALUE &&
        MyMessageBox(ParentHwnd, IDS_PORT_OPEN, IDS_LPT_PROPERTIES,
                     MB_YESNO | MB_ICONINFORMATION) == IDNO) {
        return GetLastError();
    }
    CloseHandle(hLpt);
    
    wsprintf(szNewLptName, _T("LPT%d"), NewLptNum);
    
    //
    // Open the device key for the source device instance, and write its
    // new "PortName" value.
    //
    hDeviceKey = SetupDiOpenDevRegKey(LptPropParams->DeviceInfoSet,
                                      LptPropParams->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_ALL_ACCESS);

    if (INVALID_HANDLE_VALUE == hDeviceKey) {
        return GetLastError();
    }

    dwNewLptNameLen = ByteCountOf(wcslen(szNewLptName) + 1);
    
    dwError = RegSetValueEx(hDeviceKey,
                            m_szPortName,
                            0,
                            REG_SZ,
                            (PBYTE) szNewLptName,
                            dwNewLptNameLen);
    if (ERROR_SUCCESS == dwError) {
        wcscpy(LptPropParams->szLptName, szNewLptName);
    } else {
        return dwError;
    }

    // Now generate a string, to be used for the device's friendly name, that
    // incorporates both the INF-specified device description, and the port 
    // name.  For example,
    //
    //     ECP Printer Port (LPT1)
    //

    if (LoadString(g_hInst, 
                   IDS_FRIENDLY_FORMAT, 
                   friendlyNameFormat, 
                   CharSizeOf(friendlyNameFormat)) &&
       SetupDiGetDeviceRegistryProperty(LptPropParams->DeviceInfoSet,
                                        LptPropParams->DeviceInfoData,
                                        SPDRP_DEVICEDESC,
                                        NULL,
                                        (PBYTE)deviceDesc,
                                        sizeof(deviceDesc),
                                        NULL)) {
        wsprintf(charBuffer, friendlyNameFormat, deviceDesc, szNewLptName);
    }
    else {
        //
        // Simply use LPT port name.
        //
        lstrcpy(charBuffer, szNewLptName);
    }

    SetupDiSetDeviceRegistryProperty(LptPropParams->DeviceInfoSet,
                                     LptPropParams->DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (PBYTE)charBuffer,
                                     ByteCountOf(lstrlen(charBuffer) + 1)
                                    );

    return ERROR_SUCCESS;

} // LptEnactPortNameChanges
                            
void
InformDriverOfChanges(BOOL NeedReboot,
                      IN PLPT_PROP_PARAMS LptPropParams)
{
    SP_DEVINSTALL_PARAMS spDevInstall;

    //
    // Now broadcast this change to the device manager
    //
    ZeroMemory(&spDevInstall, sizeof(SP_DEVINSTALL_PARAMS));
    spDevInstall.cbSize = sizeof(SP_DEVINSTALL_PARAMS);
    
    if (SetupDiGetDeviceInstallParams(LptPropParams->DeviceInfoSet,
                                      LptPropParams->DeviceInfoData,
                                      &spDevInstall)) {
        if (NeedReboot) {
            spDevInstall.Flags |= DI_PROPERTIES_CHANGE | DI_NEEDREBOOT;
        } else {
            spDevInstall.FlagsEx |= DI_FLAGSEX_PROPCHANGE_PENDING;
        }
        SetupDiSetDeviceInstallParams(LptPropParams->DeviceInfoSet,
                                      LptPropParams->DeviceInfoData,
                                      &spDevInstall);
    }
}

//
// Takes a Buffer of bytes and marks out which port names are taken
//
void
LptEnumerateUsedPorts(
    IN HWND             ParentHwnd,
    IN PBYTE            Buffer,
    IN DWORD            BufferSize)
{
    HKEY    hParallelMap;
    TCHAR   szParallel[BUFFER_SIZE];
    DWORD   dwParallelSize, dwLptSize, dwEnum, dwType, dwResult, dwPortNum;
    TCHAR   szLpt[BUFFER_SIZE];
    PTCHAR  szParPortNum;
    
    ZeroMemory(Buffer, BufferSize);

    //
    // 
    //
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                     m_szRegParallelMap,
                     0,
                     KEY_ALL_ACCESS,
                     &hParallelMap) == ERROR_SUCCESS) {
    
        dwEnum = 0;
        do {
            dwParallelSize = sizeof(szParallel);
            dwLptSize = sizeof(szLpt);
            dwResult = RegEnumValue(hParallelMap,
                                  dwEnum++,
                                  szParallel,
                                  &dwParallelSize,
                                  NULL,
                                  &dwType,
                                  (LPBYTE)szLpt,
                                  &dwLptSize);
    
            if (dwResult != ERROR_SUCCESS ||
                dwType != REG_SZ) {
                continue;
            }

            szParPortNum = _tcsspnp(szLpt,_T("\\DosDevices\\LPT"));
            if (szParPortNum) {
                //
                // Find out if this is an actual port and not simply a
                // device attached to the port
                //
                if (_tcscspn(szParPortNum,_T(".")) == 
                    _tcslen(szParPortNum)) {
                    dwPortNum = myatoi(szParPortNum);
                    if (dwPortNum-1 < BufferSize) {
                        Buffer[dwPortNum-1] = TRUE;
                    }
                }
            }
        } while (dwResult == ERROR_SUCCESS);
        RegCloseKey(hParallelMap);
    }
} // LptEnumerateUsedPorts

BOOL
LptPortOnContextMenu(HWND HwndControl,
                     WORD Xpos,
                     WORD Ypos)
{
    WinHelp(HwndControl,
            _T("devmgr.hlp"),
            HELP_CONTEXTMENU,
            (ULONG_PTR) LptHelpIDs);

    return FALSE;
}

void
LptPortOnHelp(HWND       ParentHwnd,
              LPHELPINFO HelpInfo)
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
				_T("devmgr.hlp"),
                HELP_WM_HELP, 
                (ULONG_PTR) LptHelpIDs);
    }
}

