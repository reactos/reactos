/*----------------------------------------------------------------------
 file: pp.c - property page

@@BEGIN_DDKSPLIT

ToDo_NT50
{
   * Bug?: DIF_MOVEDEVICE in class installer opens reg key as READ access
    to perform a write.(ports.c)
}

 * History:
    7-29-97  - Add this module for NT5.0, kpb
    
@@END_DDKSPLIT
----------------------------------------------------------------------*/
#include "ports.h"
#include "pp.h"

// @@BEGIN_DDKSPLIT
//
// Prototype for IsUserAdmin (actually in setupapi.dll, just not in the header)
//
BOOL
IsUserAdmin(
    VOID
    );
// @@END_DDKSPLIT

TCHAR m_szDevMgrHelp[] = _T("devmgr.hlp");

const DWORD HelpIDs[]=
{
    IDC_STATIC,         IDH_NOHELP,
    IDC_ADVANCED,       IDH_DEVMGR_PORTSET_ADVANCED, // "&Advanced" (Button)
    PP_PORT_BAUDRATE,   IDH_DEVMGR_PORTSET_BPS,      // "" (ComboBox)
    PP_PORT_DATABITS,   IDH_DEVMGR_PORTSET_DATABITS, // "" (ComboBox)
    PP_PORT_PARITY,     IDH_DEVMGR_PORTSET_PARITY,   // "" (ComboBox)
    PP_PORT_STOPBITS,   IDH_DEVMGR_PORTSET_STOPBITS, // "" (ComboBox)
    PP_PORT_FLOWCTL,    IDH_DEVMGR_PORTSET_FLOW,     // "" (ComboBox)
    IDC_RESTORE_PORT,   IDH_DEVMGR_PORTSET_DEFAULTS, // "&Restore Defaults" (Button)
    0, 0
};

void InitPortParams(
    IN OUT PPORT_PARAMS      Params,
    IN HDEVINFO              DeviceInfoSet,
    IN PSP_DEVINFO_DATA      DeviceInfoData
    )
{
    BOOL                        showAdvanced = TRUE;
    SP_DEVINFO_LIST_DETAIL_DATA detailData;

    ZeroMemory(Params, sizeof(PORT_PARAMS));

    Params->DeviceInfoSet = DeviceInfoSet;
    Params->DeviceInfoData = DeviceInfoData;
    Params->ChangesEnabled = TRUE;

    //
    // Now we know how big our structure is, so we can allocate memory
    //
    Params->pAdvancedData =
        (PADVANCED_DATA) LocalAlloc(LPTR, sizeof(ADVANCED_DATA));

    if (Params->pAdvancedData == NULL) {
        //
        // Not enough memory
        //
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        showAdvanced = FALSE;
    }

    Params->pAdvancedData->HidePolling = FALSE;

    //
    // See if we are being invoked locally or over the network.  If over the net,
    // then disable all possible changes.
    //
    detailData.cbSize = sizeof(SP_DEVINFO_LIST_DETAIL_DATA);
    if (SetupDiGetDeviceInfoListDetail(DeviceInfoSet, &detailData) &&
        detailData.RemoteMachineHandle != NULL) {
        showAdvanced = FALSE;
        Params->ChangesEnabled = FALSE;
    }

    // @@BEGIN_DDKSPLIT
    //
    // The user can still change the buadrate etc b/c it is written to some 
    // magic place in the registry, not into the devnode
    //
    if (!IsUserAdmin()) {
        showAdvanced = FALSE;
    }
    // @@END_DDKSPLIT

    Params->pAdvancedData->DeviceInfoSet  = DeviceInfoSet;
    Params->pAdvancedData->DeviceInfoData = DeviceInfoData;

    Params->ShowAdvanced = showAdvanced;
}

HPROPSHEETPAGE InitSettingsPage(PROPSHEETPAGE *     psp,
                                OUT PPORT_PARAMS    Params)
{
    //
    // Add the Port Settings property page
    //
    psp->dwSize      = sizeof(PROPSHEETPAGE);
    psp->dwFlags     = PSP_USECALLBACK; // | PSP_HASHELP;
    psp->hInstance   = g_hInst;
    psp->pszTemplate = MAKEINTRESOURCE(DLG_PP_PORTSETTINGS);

    //
    // following points to the dlg window proc
    //
    psp->pfnDlgProc = PortSettingsDlgProc;
    psp->lParam     = (LPARAM) Params;

    //
    // following points to some control callback of the dlg window proc
    //
    psp->pfnCallback = PortSettingsDlgCallback;

    //
    // allocate our "Ports Setting" sheet
    //
    return CreatePropertySheetPage(psp);
}

/*++

Routine Description: SerialPortPropPageProvider

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
BOOL APIENTRY SerialPortPropPageProvider(LPVOID               Info,
                                         LPFNADDPROPSHEETPAGE AddFunc,
                                         LPARAM               Lparam
                                         )
{
   PSP_PROPSHEETPAGE_REQUEST pprPropPageRequest;
   PROPSHEETPAGE             psp;
   HPROPSHEETPAGE            hpsp;
   PPORT_PARAMS              params = NULL; 

   pprPropPageRequest = (PSP_PROPSHEETPAGE_REQUEST) Info;

   if (IsParPort(pprPropPageRequest->DeviceInfoSet,
                 pprPropPageRequest->DeviceInfoData)) {
       return FALSE;
   }

   //
   // Allocate and zero out memory for the struct that will contain
   // page specific data
   //
   params = (PPORT_PARAMS) LocalAlloc(LPTR, sizeof(PORT_PARAMS));

   if (!params) {
       ErrMemDlg(GetFocus());
       return FALSE;
   }

   if (pprPropPageRequest->PageRequested == SPPSR_ENUM_ADV_DEVICE_PROPERTIES) {
        InitPortParams(params,
                       pprPropPageRequest->DeviceInfoSet,
                       pprPropPageRequest->DeviceInfoData);

        hpsp = InitSettingsPage(&psp, params);
      
        if (!hpsp) {
            return FALSE;
        }
        
        if (!(*AddFunc)(hpsp, Lparam)) {
            DestroyPropertySheetPage(hpsp);
            return FALSE;
        }
   }

   return TRUE;
} /* SerialPortPropPageProvider */


UINT CALLBACK
PortSettingsDlgCallback(HWND hwnd,
                        UINT uMsg,
                        LPPROPSHEETPAGE ppsp)
{
    PPORT_PARAMS params;

    switch (uMsg) {
    case PSPCB_CREATE:
        return TRUE;    // return TRUE to continue with creation of page

    case PSPCB_RELEASE:
        params = (PPORT_PARAMS) ppsp->lParam;
        if (params->pAdvancedData) {
            LocalFree(params->pAdvancedData);
        }
        LocalFree(params);

        return 0;       // return value ignored

    default:
        break;
    }

    return TRUE;
}

void
Port_OnCommand(
    HWND DialogHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    );

BOOL
Port_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    );

void
Port_OnHelp(
    HWND       DialogHwnd,
    LPHELPINFO HelpInfo
    );

BOOL
Port_OnInitDialog(
    HWND    DialogHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    );

BOOL
Port_OnNotify(
    HWND    DialogHwnd,
    LPNMHDR NmHdr
    );

/*++

Routine Description: PortSettingsDlgProc

    The windows control function for the Port Settings properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
INT_PTR APIENTRY
PortSettingsDlgProc(IN HWND   hDlg,
                    IN UINT   uMessage,
                    IN WPARAM wParam,
                    IN LPARAM lParam)
{
    switch(uMessage) {
    case WM_COMMAND:
        Port_OnCommand(hDlg, (int) LOWORD(wParam), (HWND)lParam, (UINT)HIWORD(wParam));
        break;

    case WM_CONTEXTMENU:
        return Port_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP: 
        Port_OnHelp(hDlg, (LPHELPINFO) lParam);
        break;
    
    case WM_INITDIALOG:
        return Port_OnInitDialog(hDlg, (HWND)wParam, lParam); 

    case WM_NOTIFY:
        return Port_OnNotify(hDlg,  (NMHDR *)lParam);
    }

    return FALSE;
} /* PortSettingsDialogProc */

void 
Port_OnAdvancedClicked(
    HWND            DialogHwnd,
    PPORT_PARAMS    Params 
    )
{
    if (DisplayAdvancedDialog(DialogHwnd, Params->pAdvancedData)) {
        lstrcpy(Params->PortSettings.szComName, 
                Params->pAdvancedData->szNewComName);
    }
}

void
Port_OnRestorePortClicked(
    HWND            DialogHwnd,
    PPORT_PARAMS    Params
    )
{
    RestorePortSettings(DialogHwnd, Params);
    PropSheet_Changed(GetParent(DialogHwnd), DialogHwnd);
}

void
Port_OnCommand(
    HWND DialogHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PPORT_PARAMS params = (PPORT_PARAMS)GetWindowLongPtr(DialogHwnd, DWLP_USER);

    if (NotifyCode == CBN_SELCHANGE) {
        PropSheet_Changed(GetParent(DialogHwnd), DialogHwnd);
    }
    else {
        switch (ControlId) {
        case IDC_ADVANCED:
            Port_OnAdvancedClicked(DialogHwnd, params);
            break; 
        
        case IDC_RESTORE_PORT:
            Port_OnRestorePortClicked(DialogHwnd, params);
            break; 
        
        //
        // Because this is a prop sheet, we should never get this.
        // All notifications for ctrols outside of the sheet come through
        // WM_NOTIFY
        //
        case IDCANCEL:
            EndDialog(DialogHwnd, 0); 
            return;
        }
    }
}

BOOL
Port_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            m_szDevMgrHelp,
            HELP_CONTEXTMENU,
            (ULONG_PTR) HelpIDs);

    return FALSE;
}

void
Port_OnHelp(
    HWND       DialogHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                m_szDevMgrHelp,
                HELP_WM_HELP, 
                (ULONG_PTR) HelpIDs);
    }
}

BOOL
Port_OnInitDialog(
    HWND    DialogHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    )
{
    PPORT_PARAMS params;

    //
    // on WM_INITDIALOG call, lParam points to the property
    // sheet page.
    //
    // The lParam field in the property sheet page struct is set by the
    // caller. When I created the property sheet, I passed in a pointer
    // to a struct containing information about the device. Save this in
    // the user window long so I can access it on later messages.
    //
    params = (PPORT_PARAMS) ((LPPROPSHEETPAGE)Lparam)->lParam;
    SetWindowLongPtr(DialogHwnd, DWLP_USER, (ULONG_PTR) params);
    
    //
    // Set up the combo boxes with choices
    //
    FillCommDlg(DialogHwnd);
    
    //
    // Read current settings
    //
    FillPortSettingsDlg(DialogHwnd, params);

    EnableWindow(GetDlgItem(DialogHwnd, IDC_ADVANCED),
                 params->ShowAdvanced);
    EnableWindow(GetDlgItem(DialogHwnd, IDC_RESTORE_PORT),
                 params->ChangesEnabled);
    
    return TRUE;  // No need for us to set the focus.
}

BOOL
Port_OnNotify(
    HWND    DialogHwnd,
    LPNMHDR NmHdr
    )
{
    PPORT_PARAMS params = (PPORT_PARAMS)GetWindowLongPtr(DialogHwnd, DWLP_USER);

    switch (NmHdr->code) {
    //
    // Sent when the user clicks on Apply OR OK !!
    //
    case PSN_APPLY:
        //
        // Write out the com port options to the registry
        //
        SavePortSettingsDlg(DialogHwnd, params);
        SetWindowLongPtr(DialogHwnd, DWLP_MSGRESULT, PSNRET_NOERROR);
        return TRUE;
        
    default:
        return FALSE;
    }
}

VOID
SetCBFromRes(
    HWND   HwndCB,
    DWORD  ResId, 
    DWORD  Default,
    BOOL   CheckDecimal)
{
    TCHAR   szTemp[258], szDecSep[2], cSep;
    LPTSTR  pThis, pThat, pDecSep;
    int     iRV;
    
    if (CheckDecimal) {
        iRV = GetLocaleInfo(GetUserDefaultLCID(), LOCALE_SDECIMAL,szDecSep,2);

        if (iRV == 0) {
            //
            // following code can take only one char for decimal separator,
            // better leave the point as separator
            //
            CheckDecimal = FALSE;
        }
    }

    if (!LoadString(g_hInst, ResId, szTemp, CharSizeOf(szTemp)))
        return;

    for (pThis = szTemp, cSep = *pThis++; pThis; pThis = pThat) {
        if (pThat = _tcschr( pThis, cSep))
            *pThat++ = TEXT('\0');

        if(CheckDecimal) {
            //
            // Assume dec separator in resource is '.', comment was put to this
            // effect
            //
            pDecSep = _tcschr(pThis,TEXT('.'));
            if (pDecSep) {
                //
                // assume decimal sep width == 1
                //
                *pDecSep = *szDecSep;
            }
        }
        SendMessage(HwndCB, CB_ADDSTRING, 0, (LPARAM) pThis);
    }

    SendMessage(HwndCB, CB_SETCURSEL, Default, 0L);
}

/*++

Routine Description: FillCommDlg

    Fill in baud rate, parity, etc in port dialog box

Arguments:

    hDlg: the window address

Return Value:

    BOOL: FALSE if function fails, TRUE if function passes

--*/
BOOL
FillCommDlg(
    HWND DialogHwnd
    )
{
    SHORT shIndex;
    TCHAR szTemp[81];

    //
    //  just list all of the baud rates
    //
    for(shIndex = 0; m_nBaudRates[shIndex]; shIndex++) {
        MyItoa(m_nBaudRates[shIndex], szTemp, 10);

        SendDlgItemMessage(DialogHwnd,
                           PP_PORT_BAUDRATE,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)szTemp);
    }

    //
    //  Set 9600 as default baud selection
    //
    shIndex = (USHORT) SendDlgItemMessage(DialogHwnd,
                                          PP_PORT_BAUDRATE,
                                          CB_FINDSTRING,
                                          (WPARAM)-1,
                                          (LPARAM)m_sz9600);

    shIndex = (shIndex == CB_ERR) ? 0 : shIndex;

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_BAUDRATE,
                       CB_SETCURSEL,
                       shIndex,
                       0L);

    for(shIndex = 0; m_nDataBits[shIndex]; shIndex++) {
        MyItoa(m_nDataBits[shIndex], szTemp, 10);

        SendDlgItemMessage(DialogHwnd,
                           PP_PORT_DATABITS,
                           CB_ADDSTRING,
                           0,
                           (LPARAM)szTemp);
    }

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_DATABITS,
                       CB_SETCURSEL,
                       DEF_WORD,
                       0L);
    
    SetCBFromRes(GetDlgItem(DialogHwnd, PP_PORT_PARITY),
                 IDS_PARITY,
                 DEF_PARITY,
                 FALSE);
    
    SetCBFromRes(GetDlgItem(DialogHwnd, PP_PORT_STOPBITS),
                 IDS_BITS,
                 DEF_STOP,
                 TRUE);
    
    SetCBFromRes(GetDlgItem(DialogHwnd, PP_PORT_FLOWCTL),
                 IDS_FLOWCONTROL,
                 DEF_SHAKE,
                 FALSE);
    
    return 0;

} /* FillCommDlg */

/*++

Routine Description: FillPortSettingsDlg

    fill in the port settings dlg sheet

Arguments:

    params: the data to fill in
    hDlg:              address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
FillPortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{
    HKEY  hDeviceKey;
    DWORD dwPortNameSize, dwError;
    TCHAR szCharBuffer[81];

    //
    // Open the device key for the source device instance, and retrieve its
    // "PortName" value.
    //
    hDeviceKey = SetupDiOpenDevRegKey(Params->DeviceInfoSet,
                                      Params->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_READ);

    if (INVALID_HANDLE_VALUE == hDeviceKey) {
        goto RetGetLastError;
    }

    dwPortNameSize = sizeof(Params->PortSettings.szComName);
    dwError = RegQueryValueEx(hDeviceKey,
                              m_szPortName,  // "PortName"
                              NULL,
                              NULL,
                              (PBYTE)Params->PortSettings.szComName,
                              &dwPortNameSize);

    RegCloseKey(hDeviceKey);

    if(ERROR_SUCCESS != dwError) {
        goto RetERROR;
    }

    //
    // create "com#:"
    //
    lstrcpy(szCharBuffer, Params->PortSettings.szComName);
    lstrcat(szCharBuffer, m_szColon);

    //
    // get values from system, fills in baudrate, parity, etc.
    //
    GetPortSettings(DialogHwnd, szCharBuffer, Params);

    if (!Params->ChangesEnabled) {
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_BAUDRATE), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_PARITY), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_DATABITS), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_STOPBITS), FALSE);
        EnableWindow(GetDlgItem(DialogHwnd, PP_PORT_FLOWCTL), FALSE);
    }

    return 0;

RetERROR:
    return dwError;

RetGetLastError:
   return GetLastError();
} /* FillPortSettingsDlg */




/*++

Routine Description: GetPortSettings

    Read in port settings from the system

Arguments:

    DialogHwnd:      address of the window
    ComName: the port we're dealing with
    Params:      where to put the information we're getting

Return Value:

    ULONG: returns error messages

--*/
void
GetPortSettings(
    IN HWND             DialogHwnd,
    IN PTCHAR           ComName,
    IN PPORT_PARAMS     Params
    )
{
    TCHAR  szParms[81];
    PTCHAR szCur, szNext;
    int    nIndex;
    int    nBaud;

    //
    // read settings in from system
   //
    GetProfileString(m_szPorts,
                     ComName,
                     g_szNull,
                     szParms,
                     81);

    StripBlanks(szParms);
    if (lstrlen(szParms) == 0) {
        lstrcpy(szParms, m_szDefParams);
        WriteProfileString(m_szPorts, ComName, szParms);
    }

    szCur = szParms;

    //
    //  baud rate
    //
    szNext = strscan(szCur, m_szComma);
    if (*szNext) {
        //
        // If we found a comma, terminate
        //
        *szNext++ = 0;
    }

    //
    // current Baud Rate selection
    //
    if (*szCur) {
        Params->PortSettings.BaudRate = myatoi(szCur);
    }
    else {
        //
        // must not have been written, use default
        //
        Params->PortSettings.BaudRate = m_nBaudRates[DEF_BAUD];
    }

    //
    // set the current value in the dialog sheet
    //
    nIndex = (int)SendDlgItemMessage(DialogHwnd,
                                     PP_PORT_BAUDRATE,
                                     CB_FINDSTRING,
                                     (WPARAM)-1,
                                     (LPARAM)szCur);

    nIndex = (nIndex == CB_ERR) ? 0 : nIndex;

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_BAUDRATE,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    szCur = szNext;
 
    //
    //  parity
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
        *szNext++ = 0;
    }
    StripBlanks(szCur);

    switch(*szCur) {
    case TEXT('o'):
        nIndex = PAR_ODD;
        break;

    case TEXT('e'):
        nIndex = PAR_EVEN;
        break;

    case TEXT('n'):
        nIndex = PAR_NONE;
        break;

    case TEXT('m'):
        nIndex = PAR_MARK;
        break;

    case TEXT('s'):
        nIndex = PAR_SPACE;
        break;

    default:
        nIndex = DEF_PARITY;
        break;
    }

    Params->PortSettings.Parity = nIndex;
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_PARITY,
                        CB_SETCURSEL,
                       nIndex,
                       0L);
    szCur = szNext;

    //
    //  word length: 4 - 8
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
        *szNext++ = 0;
    }

    StripBlanks(szCur);
    nIndex = *szCur - TEXT('4');

    if (nIndex < 0 || nIndex > 4) {
        nIndex = DEF_WORD;
    }

    Params->PortSettings.DataBits = nIndex;
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_DATABITS,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    szCur = szNext;

    //
    //  stop bits
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
       *szNext++ = 0;
    }

    StripBlanks(szCur);

    if (!lstrcmp(szCur, TEXT("1"))) {
        nIndex = STOP_1;
    }
    else if(!lstrcmp(szCur, TEXT("1.5"))) {
        nIndex = STOP_15;
    }
    else if(!lstrcmp(szCur, TEXT("2"))) {
        nIndex = STOP_2;
    }
    else {
        nIndex = DEF_STOP;
    }

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_STOPBITS,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    Params->PortSettings.StopBits = nIndex;
    szCur = szNext;

    //
    //  handshaking: Hardware, xon/xoff, or none
    //
    szNext = strscan(szCur, m_szComma);

    if (*szNext) {
        *szNext++ = 0;
    }

    StripBlanks(szCur);

    if (*szCur == TEXT('p')) {
        nIndex = FLOW_HARD;
    }
    else if (*szCur == TEXT('x')) {
        nIndex = FLOW_XON;
    }
    else {
        nIndex = FLOW_NONE;
    }

    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_FLOWCTL,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    Params->PortSettings.FlowControl = nIndex;
} /* GetPortSettings */

void
RestorePortSettings(
    HWND            DialogHwnd,
    PPORT_PARAMS    Params
    )
{
    UINT nIndex;

    //
    //  baud rate
    //
    nIndex = (UINT)SendDlgItemMessage(DialogHwnd,
                                      PP_PORT_BAUDRATE,
                                      CB_FINDSTRING,
                                      (WPARAM)-1,
                                      (LPARAM)TEXT("9600"));

    nIndex = (nIndex == CB_ERR) ? 0 : nIndex;
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_BAUDRATE,
                       CB_SETCURSEL,
                       nIndex,
                       0L);

    //
    //  parity
    //
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_PARITY,
                       CB_SETCURSEL,
                       PAR_NONE,
                       0L);

    //
    //  word length: 4 - 8
    //
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_DATABITS,
                       CB_SETCURSEL,
                       4, // the 4th index is 8, what we want
                       0L);

    //
    //  stop bits
    //
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_STOPBITS,
                       CB_SETCURSEL,
                       STOP_1,
                       0L);

    //
    //  handshaking: Hardware, xon/xoff, or none
    //
    SendDlgItemMessage(DialogHwnd,
                       PP_PORT_FLOWCTL,
                       CB_SETCURSEL,
                       FLOW_NONE,
                       0L);
 
    //    nIndex = FLOW_HARD;
    //    nIndex = FLOW_XON;
    //    nIndex = FLOW_NONE;
}

/*++

Routine Description: SavePortSettingsDlg

    save changes in the Ports Settings dlg sheet

Arguments:

    Params: where to save the data to
    ParentHwnd:              address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
SavePortSettingsDlg(
    IN HWND             DialogHwnd,
    IN PPORT_PARAMS     Params
    )
{
    TCHAR szCharBuffer[81];
    DWORD dwPortnum, dwOldPortnum;
    DWORD dwPortNameSize, dwError;
    TCHAR szNewComName[21];
    TCHAR szSerialKey[41];
    TCHAR szTitle[81];
    TCHAR szTitleFormat[81];
    HKEY  hDeviceKey, hKey;

    //
    // create "com#:"
    //
    // lstrcpy(szCharBuffer, Params->pAdvancedData->szNewComName);
    lstrcpy(szCharBuffer, Params->PortSettings.szComName);
    lstrcat(szCharBuffer, m_szColon);
 
    //
    //  store changes to win.ini; broadcast changes to apps
    //
    SavePortSettings(DialogHwnd, szCharBuffer, Params);
 
    return 0;
} /* SavePortSettingsDlg */




/*++

Routine Description: SavePortSettings

    Read the dlg screen selections for baudrate, parity, etc.
    If changed from what we started with, then save them

Arguments:

    hDlg:      address of the window
    szComName: which comport we're dealing with
    Params:      contains, baudrate, parity, etc

Return Value:

    ULONG: returns error messages

--*/
void
SavePortSettings(
    IN HWND            DialogHwnd,
    IN PTCHAR          ComName,
    IN PPORT_PARAMS    Params
    )
{
    TCHAR           szBuild[PATHMAX];
    ULONG           i;
    PP_PORTSETTINGS pppNewPortSettings;

    //
    //  Get the baud rate
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_BAUDRATE,
                                  WM_GETTEXT,
                                  18,
                                  (LPARAM)szBuild);
    if (!i) {
       goto Return;
    }

    pppNewPortSettings.BaudRate = myatoi(szBuild);

    //
    //  Get the parity setting
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_PARITY,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.Parity = i;
    lstrcat(szBuild, m_pszParitySuf[i]);

    //
    //  Get the word length
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_DATABITS,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.DataBits = i;
    lstrcat(szBuild, m_pszLenSuf[i]);

    //
    //  Get the stop bits
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_STOPBITS,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.StopBits = i;
    lstrcat(szBuild, m_pszStopSuf[i]);

    //
    //  Get the flow control
    //
    i = (ULONG)SendDlgItemMessage(DialogHwnd,
                                  PP_PORT_FLOWCTL,
                                  CB_GETCURSEL,
                                  0,
                                  0L);

    if (i == CB_ERR || i == CB_ERRSPACE) {
        goto Return;
    }

    pppNewPortSettings.FlowControl = i;
    lstrcat(szBuild, m_pszFlowSuf[i]);

    //
    // if any of the values changed, then save it off
    //
    if (Params->PortSettings.BaudRate    != pppNewPortSettings.BaudRate ||
        Params->PortSettings.Parity      != pppNewPortSettings.Parity   ||
        Params->PortSettings.DataBits    != pppNewPortSettings.DataBits ||
        Params->PortSettings.StopBits    != pppNewPortSettings.StopBits ||
        Params->PortSettings.FlowControl != pppNewPortSettings.FlowControl) {

        //
        // Write settings string to [ports] section in win.ini
        // NT translates this if a translate key is set in registry
        // and it winds up getting written to
        // HKLM\Software\Microsoft\Windows NT\CurrentVersion\Ports
        //
        WriteProfileString(m_szPorts, ComName, szBuild);

        //
        // Send global notification message to all windows
        //
        SendWinIniChange((LPTSTR)m_szPorts);

        if (!SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                                       Params->DeviceInfoSet,
                                       Params->DeviceInfoData)) {
            //
            // Possibly do something here
            //
        }
    }

Return:
   return;

} /* SavePortSettings */
