///////////////////////////////////////////////////////////////////////////
// Advanced Dialog Functions
///////////////////////////////////////////////////////////////////////////

#include "ports.h"
#include "pp.h"

#include <windowsx.h>

//
// write out values in tenths of a sec
//
#define SECONDS_CONVERSION_FACTOR  (10)
#define NUM_POLLING_PERIODS 7

DWORD PollingPeriods[NUM_POLLING_PERIODS] = {
           -1,
            0,
            1 * SECONDS_CONVERSION_FACTOR,
            5 * SECONDS_CONVERSION_FACTOR,
            10 * SECONDS_CONVERSION_FACTOR,
            30 * SECONDS_CONVERSION_FACTOR,
            60 * SECONDS_CONVERSION_FACTOR
            };

TCHAR PeriodDescription[NUM_POLLING_PERIODS+1][40] = {
    { _T("Disabled") },
    { _T("Manually") },
    { _T("Every second") },
    { _T("Every 5 seconds") },
    { _T("Every 10 seconds") },
    { _T("Every 30 seconds") },
    { _T("Every minute") },
    { _T("Other (every %d sec)") }
    };

ULONG RxValues[4] = { 1, 4, 8, 14};

TCHAR m_szRxFIFO[] =        _T("RxFIFO");
TCHAR m_szTxFIFO[] =        _T("TxFIFO");
TCHAR m_szFifoRxMax[] =     _T("FifoRxMax");
TCHAR m_szFifoTxMax[] =     _T("FifoTxMax");

const DWORD AdvanHelpIDs[] =
{
    IDC_DESC_1,             IDH_NOHELP,
    IDC_DESC_2,             IDH_NOHELP,

    IDC_FIFO,               IDH_DEVMGR_PORTSET_ADV_USEFIFO, // "Use FIFO buffers (requires 16550 compatible UART)" (Button)

    IDC_RECEIVE_TEXT,       IDH_NOHELP,                     // "&Receive Buffer:" (Static)
    IDC_RECEIVE_SLIDER,     IDH_DEVMGR_PORTSET_ADV_RECV,    // "" (msctls_trackbar32)
    IDC_RECEIVE_LOW,        IDH_NOHELP,                     // "Low (%d)" (Static)
    IDC_RECEIVE_HIGH,       IDH_NOHELP,                     // "High (%d)" (Static)
    IDC_RXVALUE,            IDH_NOHELP,

    IDC_TRANSMIT_TEXT,      IDH_NOHELP,                     // "&Transmit Buffer:" (Static)
    IDC_TRANSMIT_SLIDER,    IDH_DEVMGR_PORTSET_ADV_TRANS,   // "" (msctls_trackbar32)
    IDC_TRANSMIT_LOW,       IDH_NOHELP,                     // "Low (%d)" (Static)
    IDC_TRANSMIT_HIGH,      IDH_NOHELP,                     // "High (%d)" (Static)
    IDC_TXVALUE,            IDH_NOHELP,

    IDC_POLL_DESC,          IDH_NOHELP,
    IDC_POLL_PERIOD,        IDH_DEVMGR_PORTSET_ADV_DEVICES, // "" (ComboBox)

    PP_PORT_NUMBER,         IDH_DEVMGR_PORTSET_ADV_NUMBER,  // "" (ComboBox)
    IDC_COMNUMTEXT,         IDH_NOHELP,                     // "COM &Port Number:" (Static)

    IDC_RESTORE,            IDH_DEVMGR_PORTSET_ADV_DEFAULTS,// "&Restore Defaults" (Button)
    0, 0
};

#define Trackbar_SetPos(hwndTb, Redraw, Position)\
    (VOID) SendMessage(hwndTb, TBM_SETPOS, (WPARAM) Redraw, (LPARAM) Position)

#define Trackbar_SetRange(hwndTb, Redraw, MinVal, MaxVal)\
    (VOID) SendMessage(hwndTb, TBM_SETRANGE, (WPARAM) Redraw, (LPARAM) MAKELONG(MinVal, MaxVal))

#define Trackbar_SetTic(hwndTb, Tic)\
    (VOID) SendMessage(hwndTb, TBM_SETTIC, (WPARAM) 0, (LPARAM) Tic)

#define Trackbar_GetPos(hwndTb)\
    (DWORD) SendMessage(hwndTb, TBM_GETPOS, (WPARAM) 0, (LPARAM) 0)



BOOL
Advanced_OnCommand(
    HWND ParentHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    );


BOOL
Advanced_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    );

void
Advanced_OnHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    );

BOOL
Advanced_OnInitDialog(
    HWND    ParentHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    );

/*++

Routine Description: AdvancedPortsDlgProc

    The windows proc for the Advanced properties window

Arguments:

    hDlg, uMessage, wParam, lParam: standard windows DlgProc parameters

Return Value:

    BOOL: FALSE if the page could not be created

--*/
INT_PTR APIENTRY
AdvancedPortsDlgProc(
    IN HWND   hDlg,
    IN UINT   uMessage,
    IN WPARAM wParam,
    IN LPARAM lParam
    )
{
    switch(uMessage) {
    case WM_COMMAND:
        return Advanced_OnCommand(hDlg,
                                  (int) LOWORD(wParam),
                                  (HWND)lParam,
                                  (UINT) HIWORD(wParam));

    case WM_CONTEXTMENU:
        return Advanced_OnContextMenu((HWND)wParam, LOWORD(lParam), HIWORD(lParam));

    case WM_HELP:
        Advanced_OnHelp(hDlg, (LPHELPINFO) lParam);
        break;

    case WM_HSCROLL:
        HandleTrackbarChange(hDlg, (HWND) lParam);
        return TRUE;

    case WM_INITDIALOG:
        return Advanced_OnInitDialog(hDlg, (HWND) wParam, lParam);
    }

    return FALSE;
} /* AdvancedPortsDlgProc */

BOOL
Advanced_OnCommand(
    HWND ParentHwnd,
    int  ControlId,
    HWND ControlHwnd,
    UINT NotifyCode
    )
{
    PADVANCED_DATA advancedData =
        (PADVANCED_DATA) GetWindowLongPtr(ParentHwnd, DWLP_USER);

    switch(ControlId) {

    case IDC_FIFO:
        //
        // Disable or enable the sliders
        //
        EnableFifoControls(ParentHwnd, IsDlgButtonChecked(ParentHwnd, IDC_FIFO));
        return TRUE;

    case IDOK:
        SaveAdvancedSettings(ParentHwnd, advancedData);
        // fall through

    case IDCANCEL:
        EndDialog(ParentHwnd, ControlId);
        return TRUE;

    case IDC_RESTORE:
        RestoreAdvancedDefaultState(ParentHwnd, advancedData);
        return TRUE;
    }

    return FALSE;
}

BOOL
Advanced_OnContextMenu(
    HWND HwndControl,
    WORD Xpos,
    WORD Ypos
    )
{
    WinHelp(HwndControl,
            m_szDevMgrHelp,
            HELP_CONTEXTMENU,
            (ULONG_PTR) AdvanHelpIDs);

    return FALSE;
}

void
Advanced_OnHelp(
    HWND       ParentHwnd,
    LPHELPINFO HelpInfo
    )
{
    if (HelpInfo->iContextType == HELPINFO_WINDOW) {
        WinHelp((HWND) HelpInfo->hItemHandle,
                 m_szDevMgrHelp,
                 HELP_WM_HELP,
                 (ULONG_PTR) AdvanHelpIDs);
    }
}

BOOL
Advanced_OnInitDialog(
    HWND    ParentHwnd,
    HWND    FocusHwnd,
    LPARAM  Lparam
    )
{
    PADVANCED_DATA advancedData;
    TCHAR          szFormat[200];
    TCHAR          szBuffer[200];
    advancedData = (PADVANCED_DATA) Lparam;

    //
    // Initialize the dialog box parameters
    //
    FillAdvancedDlg(ParentHwnd, advancedData);
    SetWindowLongPtr(ParentHwnd, DWLP_USER, (ULONG_PTR) advancedData);

    //
    // Set up the dialog box with these initialized parameters
    //
    InitializeControls(ParentHwnd, advancedData);

    LoadString(g_hInst, IDS_ADVANCED_SETTINGS_FOR, szFormat, CharSizeOf(szFormat));
    wsprintf(szBuffer, szFormat, advancedData->szComName);
    SetWindowText(ParentHwnd, szBuffer);

    return TRUE;
}

// @@BEGIN_DDKSPLIT
LONG
SerialDisplayAdvancedSettings(
    IN HWND             ParentHwnd,
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData
    )
{
    ADVANCED_DATA advancedData;
    HKEY          hDeviceKey;
    DWORD         dwPortNameSize;

    if (!DeviceInfoSet) {
        return ERROR_INVALID_PARAMETER;
    }

    advancedData.HidePolling = TRUE;
    advancedData.DeviceInfoSet = DeviceInfoSet;
    advancedData.DeviceInfoData = DeviceInfoData;

    //
    // Open the device key for the source device instance, and retrieve its
    // "PortName" value.
    //
    hDeviceKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                      DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_READ);

    if (INVALID_HANDLE_VALUE == hDeviceKey)
        return ERROR_ACCESS_DENIED;

    dwPortNameSize = sizeof(advancedData.szComName);
    if (RegQueryValueEx(hDeviceKey,
                        m_szPortName,
                        NULL,
                        NULL,
                        (PBYTE) advancedData.szComName,
                        &dwPortNameSize) != ERROR_SUCCESS) {
        RegCloseKey(hDeviceKey);
        return  ERROR_INVALID_DATA;
    }
    RegCloseKey(hDeviceKey);

    return DisplayAdvancedDialog(ParentHwnd, &advancedData) ? ERROR_SUCCESS
                                                            : ERROR_ACCESS_DENIED;
}
// @@END_DDKSPLIT

BOOL InternalAdvancedDialog(
    IN     HWND           ParentHwnd,
    IN OUT PADVANCED_DATA AdvancedData
    )
{
    AdvancedData->hComDB = HCOMDB_INVALID_HANDLE_VALUE;
    ComDBOpen(&AdvancedData->hComDB);

    DialogBoxParam(g_hInst,
                   MAKEINTRESOURCE(DLG_PP_ADVPORTS),
                   ParentHwnd,
                   AdvancedPortsDlgProc,
                   (DWORD_PTR) AdvancedData);

    ComDBClose(AdvancedData->hComDB);
    AdvancedData->hComDB = HCOMDB_INVALID_HANDLE_VALUE;

    return TRUE;
}

// @@BEGIN_DDKSPLIT
BOOL
FindAdvancedDialogOverride(
    IN HDEVINFO         DeviceInfoSet,
    IN PSP_DEVINFO_DATA DeviceInfoData,
    IN PTCHAR           Value
    )
/*++

Routine Description:

    Checks the driver key in the devnode for the override value.

Arguments:

    DeviceInfoSet - Supplies a handle to the device information

    DeviceInfoData - Supplies the address of the device information element

    Value          - Value read in from the registry

Return Value:

    If found TRUE, otherwise FALSE

--*/
{
    HKEY    hKey;
    TCHAR   szLine[LINE_LEN];
    DWORD   dwDataType, dwSize;
    TCHAR   szOverrideName[] = _T("EnumAdvancedDialog");

    //
    // Open up the driver key for this device so we can run our INF registry mods
    // against it.
    //
    hKey = SetupDiOpenDevRegKey(DeviceInfoSet,
                                DeviceInfoData,
                                DICS_FLAG_GLOBAL,
                                0,
                                DIREG_DRV,
                                KEY_READ);

    dwSize = sizeof(szLine);
    if (RegQueryValueEx(hKey,
                        szOverrideName,
                        NULL,
                        &dwDataType,
                        (PBYTE) &szLine,
                        &dwSize) != ERROR_SUCCESS ||
        dwDataType != REG_SZ) {
        RegCloseKey(hKey);
        return FALSE;
    }

    lstrcpy(Value, szLine);
    RegCloseKey(hKey);
    return TRUE;
}

BOOL
CallAdvancedDialogOverride(
    IN HWND           ParentHwnd,
    IN PADVANCED_DATA AdvancedData,
    PTCHAR            Override,
    PBOOL             Success
    )
{
    PTCHAR                  szProc = NULL;
    HINSTANCE               hInst = NULL;
    PPORT_ADVANCED_DIALOG   pfnAdvanced = NULL;
    TCHAR                   szNewComName[50];
    HKEY                    hDeviceKey;
    DWORD                   dwSize;
    TCHAR                   szMfg[LINE_LEN];

#ifdef UNICODE
    CHAR                    szFunction[LINE_LEN];
#endif

    szProc = _tcschr(Override, _T(','));
    if (!szProc) {
        return FALSE;
    }

    *szProc = _T('\0');
    szProc++;

    hInst = LoadLibrary(Override);
    if (!hInst) {
        return FALSE;
    }

#ifdef UNICODE
    WideCharToMultiByte(CP_ACP,
                        0,
                        szProc,
                        lstrlen(szProc) + 1,
                        szFunction,
                        sizeof(szFunction),
                        NULL,
                        NULL);

    pfnAdvanced = (PPORT_ADVANCED_DIALOG) GetProcAddress(hInst, szFunction);
#else
    pfnAdvanced = (PPORT_ADVANCED_DIALOG) GetProcAddress(hInst, szProc);
#endif
    if (!pfnAdvanced) {
        FreeLibrary(hInst);
        return FALSE;
    }

    _try
    {
        *Success = (*pfnAdvanced) (ParentHwnd,
                                   AdvancedData->HidePolling,
                                   AdvancedData->DeviceInfoSet,
                                   AdvancedData->DeviceInfoData,
                                   NULL);

    }
    _except(UnhandledExceptionFilter(GetExceptionInformation()))
    {
        *Success = FALSE;
    }

    //
    // Open the device key for the source device instance, and retrieve its
    // "PortName" value in case the override dialog changed it.
    //
    hDeviceKey = SetupDiOpenDevRegKey(AdvancedData->DeviceInfoSet,
                                      AdvancedData->DeviceInfoData,
                                      DICS_FLAG_GLOBAL,
                                      0,
                                      DIREG_DEV,
                                      KEY_READ);

    if (INVALID_HANDLE_VALUE != hDeviceKey) {
        dwSize = sizeof(szNewComName);
        if (RegQueryValueEx(hDeviceKey,
                            m_szPortName,
                            NULL,
                            NULL,
                            (PBYTE) szNewComName,
                            &dwSize) == ERROR_SUCCESS) {
            lstrcpy(AdvancedData->szComName, szNewComName);
        }
        RegCloseKey(hDeviceKey);
    }

    FreeLibrary(hInst);
    return TRUE;
}
// @@END_DDKSPLIT

/*++

Routine Description: DisplayAdvancedDialog

    Opens the devices instance and checks to see if it is valid.  If so, then the advanced
    dialog is displayed.  Otherwise a message is displayed to the user stating that the user
    does not have write access to this particular key.

Arguments:

    ParentHwnd - Handle to the parent dialog (Port Settings Property Sheet)
    AdvancedData - hDeviceKey will be set with the device's key in the registry upon success,
                    INVALID_HANDLE_VALUE upon error

Return Value:

    None

--*/
BOOL DisplayAdvancedDialog(
    IN      HWND           ParentHwnd,
    IN OUT  PADVANCED_DATA AdvancedData
    )
{
    AdvancedData->hDeviceKey =
        SetupDiOpenDevRegKey(AdvancedData->DeviceInfoSet,
                             AdvancedData->DeviceInfoData,
                             DICS_FLAG_GLOBAL,
                             0,
                             DIREG_DEV,
                             KEY_ALL_ACCESS);

    if (AdvancedData->hDeviceKey == INVALID_HANDLE_VALUE) {
        MyMessageBox(ParentHwnd,
                     IDS_NO_WRITE_PRVILEGE,
                     IDS_NAME_PROPERTIES,
                     MB_OK | MB_ICONINFORMATION);
        return FALSE;
    }
    else {
// @@BEGIN_DDKSPLIT
        TCHAR szOverride[LINE_LEN];
        BOOL  success = FALSE;

        if (FindAdvancedDialogOverride(AdvancedData->DeviceInfoSet,
                                       AdvancedData->DeviceInfoData,
                                       szOverride) &&
            CallAdvancedDialogOverride(ParentHwnd,
                                       AdvancedData,
                                       szOverride,
                                       &success)) {
            RegCloseKey(AdvancedData->hDeviceKey);
            return success;
        }
        else {
// @@END_DDKSPLIT
            return InternalAdvancedDialog(ParentHwnd, AdvancedData);
// @@BEGIN_DDKSPLIT
        }
// @@END_DDKSPLIT
    }
}

/*++

Routine Description: EnableFifoControls

    Enables/Disables all of the controls bounded by the rectangle with the Use Fifo
    checkbox.

Arguments:

    hDlg - Handle to the dialog
    enabled - flag to either enable/disable the controls

Return Value:

    None

--*/
void EnableFifoControls(IN HWND hDlg,
                        IN BOOL enabled)
{
   // The actual trackbar/slider
   EnableWindow(GetDlgItem(hDlg, IDC_RECEIVE_SLIDER), enabled);

   // "Low (xxx)" (Receive)
   EnableWindow(GetDlgItem(hDlg, IDC_RECEIVE_LOW), enabled);

   // "High (xxx)" (Receive)
   EnableWindow(GetDlgItem(hDlg, IDC_RECEIVE_HIGH), enabled);

   // "Receive Buffer:  "
   EnableWindow(GetDlgItem(hDlg, IDC_RECEIVE_TEXT), enabled);

   // "(xxx)" (Actual value of trackbar, Receive)
   EnableWindow(GetDlgItem(hDlg, IDC_RXVALUE), enabled);

   // The actual trackbar/slider
   EnableWindow(GetDlgItem(hDlg, IDC_TRANSMIT_SLIDER), enabled);

   // "Low (xxx)" (Transmit)
   EnableWindow(GetDlgItem(hDlg, IDC_TRANSMIT_LOW), enabled);

   // "High (xxx)" (Transmit)
   EnableWindow(GetDlgItem(hDlg, IDC_TRANSMIT_HIGH), enabled);

   // "Transmit Buffer" (Transmit)
   EnableWindow(GetDlgItem(hDlg, IDC_TRANSMIT_TEXT), enabled);

   // "(xxx)" (Actual value of trackbar, Trasmist)
   EnableWindow(GetDlgItem(hDlg, IDC_TXVALUE), enabled);
}

/*++

Routine Description: HandleTrackbarChange

    Whenever the user changes the trackbar thumb position, update the control
    to its right which displays its actual numeric value

Arguments:

    hDlg - Handle to the parent dialog
    hTrackbar - Handle to the trackbar whose thumb has changed

Return Value:

    None

--*/
void HandleTrackbarChange(IN HWND hDlg,
                          IN HWND hTrackbar
                          )
{
    DWORD ctrlID;
    TCHAR szCurrentValue[10];
    ULONG position;

    position = Trackbar_GetPos(hTrackbar);

    if (GetDlgCtrlID(hTrackbar) == IDC_RECEIVE_SLIDER) {
        //
        // Rx we need to translate the tick position from index to value
        //
        wsprintf(szCurrentValue, TEXT("(%d)"), RxValues[position-1]);
        ctrlID = IDC_RXVALUE;
    }
    else {
        //
        // Tx is just a straight translation between value and index
        //
        wsprintf(szCurrentValue, TEXT("(%d)"), position);
        ctrlID = IDC_TXVALUE;
    }
    SetDlgItemText(hDlg, ctrlID, szCurrentValue);
}

DWORD
RxValueToTrackbarPosition(IN OUT PDWORD RxValue
               )
{
    switch (*RxValue) {
    case 1:  return 1;
    case 4:  return 2;
    case 8:  return 3;
    case 14: return 4;
    }

    //
    // busted value
    //
    *RxValue = 14;
    return 4;
}

/*++

Routine Description:   SetTxTrackbarTicks

    Creates a tick at 1/4, half, and 3/4 across the span of the trackbar

Arguments:

    hTrackbar - handle to the trackbar that will receive the ticks
    minVal, maxVal - Range on the trackbar

Return Value:

    None

--*/
void
SetTxTrackbarTics(
    IN HWND   TrackbarHwnd
    )
{
    Trackbar_SetTic(TrackbarHwnd, 6);
    Trackbar_SetTic(TrackbarHwnd, 11);
}

/*++

Routine Description:  SetLabelText

    Sets the label's to the string identified by resID concated with the passed
    in value and closing paren.

    The final string is  [resID string][value])

Arguments:

    hLabel - handle to the control whose text is going to change
    resID  - resource ID for the beginning of the string that will become the
              label's text
    value  - number to be concated into the string


Return Value:

    None

--*/
void
SetLabelText(
    IN HWND     LabelHwnd,
    IN DWORD    ResId,
    IN ULONG    Value
    )
{
    TCHAR szTemp[258], txt[258];

    if (LoadString(g_hInst, ResId, szTemp, CharSizeOf(szTemp))) {
        lstrcpy(txt, szTemp);
        wsprintf(szTemp, _T("%d)"), Value);
        lstrcat(txt, szTemp);
    }
    else {
        lstrcpy(txt, _T("Low"));
    }
    SetWindowText(LabelHwnd, txt);
}

/*++

Routine Description: InitializeControls

    Initializes all of the controls that represent Fifo

Arguments:

    ParentHwnd - handle to the dialog
    AdvancedData - Contains all of the initial values

Return Value:

    None

--*/
void InitializeControls(
    IN HWND           ParentHwnd,
    IN PADVANCED_DATA AdvancedData
    )
{
    TCHAR    szCurrentValue[40];
    HWND     hwnd;
    int      i, periodIdx;

    //
    // Set up the Fifo buffers checkbox
    //
    if (!AdvancedData->UseFifoBuffersControl) {
        //
        // Something went wrong with the Fifo buffers control. Disable
        // the checkbox
        //
        CheckDlgButton(ParentHwnd, IDC_FIFO, BST_UNCHECKED);
        EnableWindow(GetDlgItem(ParentHwnd, IDC_FIFO), FALSE);
        EnableFifoControls(ParentHwnd, FALSE);
    }
    else {
        EnableWindow(GetDlgItem(ParentHwnd, IDC_FIFO), TRUE);

        if (!AdvancedData->UseFifoBuffers) {
            EnableFifoControls(ParentHwnd, FALSE);
            CheckDlgButton(ParentHwnd, IDC_FIFO, BST_UNCHECKED);
        }
        else {
            EnableFifoControls(ParentHwnd, TRUE);
            CheckDlgButton(ParentHwnd, IDC_FIFO, BST_CHECKED);
        }
    }

    //
    // Set up the sliders
    //
    if (!AdvancedData->UseRxFIFOControl ||
        !AdvancedData->UseTxFIFOControl) {
        //
        // Something went wrong with the sliders.
        // Disable them
        //
        CheckDlgButton(ParentHwnd, IDC_FIFO, BST_UNCHECKED);
        EnableWindow(GetDlgItem(ParentHwnd, IDC_FIFO), FALSE);
        EnableFifoControls(ParentHwnd, FALSE);
    }
    else {
        //
        // Set up Rx Slider
        //
        hwnd = GetDlgItem(ParentHwnd, IDC_RECEIVE_SLIDER);

        Trackbar_SetRange(hwnd, TRUE, RX_MIN, 4);
        Trackbar_SetPos(hwnd,
                        TRUE,
                        RxValueToTrackbarPosition(&AdvancedData->RxFIFO));

        SetLabelText(GetDlgItem(ParentHwnd, IDC_RECEIVE_LOW),
                     IDS_LOW,
                     RX_MIN);
        SetLabelText(GetDlgItem(ParentHwnd, IDC_RECEIVE_HIGH),
                     IDS_HIGH,
                     AdvancedData->FifoRxMax);

        wsprintf(szCurrentValue, TEXT("(%d)"), AdvancedData->RxFIFO);
        SetDlgItemText(ParentHwnd, IDC_RXVALUE, szCurrentValue);

        //
        // Set up the Tx slider
        //
        hwnd = GetDlgItem(ParentHwnd, IDC_TRANSMIT_SLIDER);
        Trackbar_SetRange(hwnd, TRUE, TX_MIN, AdvancedData->FifoTxMax);
        Trackbar_SetPos(hwnd, TRUE, AdvancedData->TxFIFO);

        SetTxTrackbarTics(hwnd);

        SetLabelText(GetDlgItem(ParentHwnd, IDC_TRANSMIT_LOW),
                     IDS_LOW,
                     TX_MIN);
        SetLabelText(GetDlgItem(ParentHwnd, IDC_TRANSMIT_HIGH),
                     IDS_HIGH,
                     AdvancedData->FifoTxMax);

        wsprintf(szCurrentValue, TEXT("(%d)"), AdvancedData->TxFIFO);
        SetDlgItemText(ParentHwnd, IDC_TXVALUE, szCurrentValue);
    }

    FillPortNameCb(ParentHwnd, AdvancedData);

    if (!AdvancedData->HidePolling) {

        //
        // Add the descriptions for each polling period and select the current
        // setting
        //
        hwnd = GetDlgItem(ParentHwnd, IDC_POLL_PERIOD);
        periodIdx = NUM_POLLING_PERIODS;
        for (i = 0; i < NUM_POLLING_PERIODS; i++) {
            ComboBox_AddString(hwnd, PeriodDescription[i]);
            if (PollingPeriods[i] == AdvancedData->PollingPeriod) {
                periodIdx = i;
            }
        }

        if (periodIdx == NUM_POLLING_PERIODS) {
            wsprintf(szCurrentValue,
                     PeriodDescription[NUM_POLLING_PERIODS],
                     AdvancedData->PollingPeriod / SECONDS_CONVERSION_FACTOR);
            ComboBox_AddString(hwnd, szCurrentValue);
        }

        ComboBox_SetCurSel(hwnd, periodIdx);
    }
    else {
       ShowWindow(GetDlgItem(ParentHwnd, IDC_POLL_PERIOD), SW_HIDE);
       ShowWindow(GetDlgItem(ParentHwnd, IDC_POLL_DESC), SW_HIDE);
    }
} /* InitializeControls */


/*++

Routine Description:  RestoreAdvancedDefaultState

    Restores all values and UI to their default state, specifically:
    o All Fifo related child controls are enabled
    o The Rx trackbar is set to its max value
    o The Tx trackbar is set to its max value
    o The number of the comport is reset to its original value

Return Value:

    None

--*/
void RestoreAdvancedDefaultState(
    IN HWND           ParentHwnd,
    IN PADVANCED_DATA AdvancedData
    )
{
    USHORT ushIndex;
    TCHAR  szCurrentValue[10];
    int    i;

    //
    // Set up the Fifo buffers checkbox
    //
    EnableWindow(GetDlgItem(ParentHwnd, IDC_FIFO), TRUE);

    EnableFifoControls(ParentHwnd, TRUE);
    CheckDlgButton(ParentHwnd, IDC_FIFO, BST_CHECKED);

    //
    // Set up the sliders and the static control that show their numberic value
    //
    Trackbar_SetPos(GetDlgItem(ParentHwnd, IDC_RECEIVE_SLIDER),
                    TRUE,
                    RxValueToTrackbarPosition(&AdvancedData->FifoRxMax));
    wsprintf(szCurrentValue, TEXT("(%d)"), AdvancedData->FifoRxMax);
    SetDlgItemText(ParentHwnd, IDC_RXVALUE, szCurrentValue);

    Trackbar_SetPos(GetDlgItem(ParentHwnd, IDC_TRANSMIT_SLIDER), TRUE, AdvancedData->FifoTxMax);
    wsprintf(szCurrentValue, TEXT("(%d)"), AdvancedData->FifoTxMax);
    SetDlgItemText(ParentHwnd, IDC_TXVALUE, szCurrentValue);

    //
    // Set the COM name to whatever it is currently set to in the registry
    //
    ushIndex =
        (USHORT) ComboBox_FindString(GetDlgItem(ParentHwnd, PP_PORT_NUMBER),
                                     -1,
                                     AdvancedData->szComName);

    ushIndex = (ushIndex == CB_ERR) ? 0 : ushIndex;

    ComboBox_SetCurSel(GetDlgItem(ParentHwnd, PP_PORT_NUMBER), ushIndex);
    ComboBox_SetCurSel(GetDlgItem(ParentHwnd, IDC_POLL_PERIOD), POLL_PERIOD_DEFAULT_IDX);
} /* RestoreAdvancedDefaultStates */


/*++

Routine Description: FillPortNameCb

    fill in the Port Name combo box selection with a list
    of possible un-used portnames

Arguments:

    poppOurPropParams: where to save the data to
    hDlg:              address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
FillPortNameCb(
    HWND            ParentHwnd,
    PADVANCED_DATA  AdvancedData
    )
{
    BYTE  portUsage[MAX_COM_PORT/8];
    DWORD tmp, portsReported;
    int   i, j, nEntries;
    int   nCurPortNum;
    TCHAR szCom[40];
    TCHAR szInUse[40];
    char  mask, *current;
    HWND  portHwnd;

    portHwnd = GetDlgItem(ParentHwnd, PP_PORT_NUMBER);

    //
    // Check if our ComName is blank. If it is, disable the com port selector
    //
    if (_tcscmp(AdvancedData->szComName, TEXT("")) == 0) {
        EnableWindow(portHwnd, FALSE);
        EnableWindow(GetDlgItem(ParentHwnd, IDC_COMNUMTEXT), FALSE);
        return 0;
    }

    //
    // assumes szComPort filled in...
    //
    nCurPortNum = myatoi(&AdvancedData->szComName[3]);

    if (!LoadString(g_hInst, IDS_IN_USE, szInUse, CharSizeOf(szInUse))) {
        wcscpy(szInUse, _T(" (in use)"));
    }

    //
    // first tally up which ports NOT to offer in list box
    //
    ZeroMemory(portUsage, MAX_COM_PORT/8);

    if (AdvancedData->hComDB != HCOMDB_INVALID_HANDLE_VALUE) {
        ComDBGetCurrentPortUsage(AdvancedData->hComDB,
                                 portUsage,
                                 MAX_COM_PORT / 8,
                                 CDB_REPORT_BITS,
                                 &portsReported);
    }

    //
    // tag the current port as not in use so it shows up in the CB
    //
    current = portUsage + (nCurPortNum-1) / 8;
    if ((i = nCurPortNum % 8))
        *current &= ~(1 << (i-1));
    else
        *current &= ~(0x80);

    current = portUsage;
    mask = 0x1;
    for(nEntries = j = 0, i = MIN_COM-1; i < MAX_COM_PORT; i++) {

       wsprintf(szCom, TEXT("COM%d"), i+1);
       if (*current & mask) {
           wcscat(szCom, szInUse);
       }

       if (mask == (char) 0x80) {
           mask = 0x01;
           current++;
       }
       else {
           mask <<= 1;
       }

       ComboBox_AddString(portHwnd, szCom);
   }

   ComboBox_SetCurSel(portHwnd, nCurPortNum-1);
   return 0;
} /* FillPortNamesCb */

/*++

Routine Description: FillAdvancedDlg

    fill in the advanced dialog window

Arguments:

    poppOurPropParams: the data to fill in
    ParentHwnd:              address of the window

Return Value:

    ULONG: returns error messages

--*/
ULONG
FillAdvancedDlg(
    IN HWND             ParentHwnd,
    IN PADVANCED_DATA   AdvancedData
    )
{
   PSP_DEVINFO_DATA DeviceInfoData = AdvancedData->DeviceInfoData;
   HKEY  hDeviceKey;
   DWORD dwSize, dwData, dwFifo, dwError = ERROR_SUCCESS;

   //
   // Open the device key for the source device instance
   //
   hDeviceKey = AdvancedData->hDeviceKey;

   //
   // Get COM Name
   //
   dwSize = sizeof(AdvancedData->szComName);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szPortName,
                             NULL,
                             NULL,
                             (PBYTE)AdvancedData->szComName,
                             &dwSize);

   if (dwError != ERROR_SUCCESS) {
      wsprintf(AdvancedData->szComName, TEXT("COMX"));
   }

   //
   // Get ForceFifoEnable information
   //
   AdvancedData->UseFifoBuffersControl = TRUE;

   dwSize = sizeof(dwFifo);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szFIFO,
                             NULL,
                             NULL,
                             (LPBYTE)(&dwFifo),
                             &dwSize);

   if (dwError == ERROR_SUCCESS) {
      //
      // Save this initial value
      //
      AdvancedData->UseFifoBuffersControl = TRUE;
      if (dwFifo == 0) {
         AdvancedData->UseFifoBuffers = FALSE;
      }
      else {
         AdvancedData->UseFifoBuffers = TRUE;
      }
   }
   else {
      //
      // value does not exist. Create our own
      //
      dwData = 1;
      dwSize = sizeof(dwSize);
      dwError = RegSetValueEx(hDeviceKey,
                              m_szFIFO,
                              0,
                              REG_DWORD,
                              (CONST BYTE *)(&dwData),
                              dwSize);

      if (dwError == ERROR_SUCCESS) {
         AdvancedData->UseFifoBuffers = TRUE;
      }
      else {
         AdvancedData->UseFifoBuffers = FALSE;
         AdvancedData->UseFifoBuffersControl = FALSE;
      }
   }

   //
   // Get FifoRxMax information
   //
   dwSize = sizeof(dwFifo);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szFifoRxMax,
                             NULL,
                             NULL,
                             (LPBYTE)(&dwFifo),
                             &dwFifo);

   if (dwError == ERROR_SUCCESS) {
      //
      // Save this initial value
      //
      AdvancedData->FifoRxMax = dwFifo;
      if (AdvancedData->FifoRxMax > RX_MAX) {
          AdvancedData->FifoRxMax = RX_MAX;
      }
   }
   else {
      //
      // value does not exist. Create our own
      //
      AdvancedData->FifoRxMax = RX_MAX;
   }

   //
   // Get RxFIFO information
   //
   dwSize = sizeof(dwFifo);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szFifoTxMax,
                             NULL,
                             NULL,
                             (LPBYTE)(&dwFifo),
                             &dwSize);

   if (dwError == ERROR_SUCCESS) {
      //
      // Save this initial value
      //
      AdvancedData->FifoTxMax = dwFifo;
      if (AdvancedData->FifoTxMax > TX_MAX) {
          AdvancedData->FifoTxMax = TX_MAX;
      }
   }
   else {
      //
      // value does not exist. Create our own
      //
      AdvancedData->FifoTxMax = TX_MAX;
   }

   //
   // Get RxFIFO information
   //
   AdvancedData->UseRxFIFOControl = TRUE;

   dwSize = sizeof(dwFifo);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szRxFIFO,
                             NULL,
                             NULL,
                             (LPBYTE)(&dwFifo),
                             &dwSize);

   if (dwError == ERROR_SUCCESS) {
      //
      // Save this initial value
      //
      AdvancedData->RxFIFO = dwFifo;
      if (AdvancedData->RxFIFO > RX_MAX) {
          goto SetRxFIFO;
      }
   }
   else {
SetRxFIFO:
      //
      // value does not exist. Create our own
      //
      dwData = AdvancedData->FifoRxMax;
      dwSize = sizeof(dwData);
      dwError = RegSetValueEx(hDeviceKey,
                              m_szRxFIFO,
                              0,
                              REG_DWORD,
                              (CONST BYTE *)(&dwData),
                              dwSize);

      if (dwError == ERROR_SUCCESS) {
         AdvancedData->RxFIFO = AdvancedData->FifoRxMax;
      }
      else {
         AdvancedData->RxFIFO = 0;
         AdvancedData->UseRxFIFOControl = FALSE;
      }
   }

   //
   // Get TxFIFO information
   //
   AdvancedData->UseTxFIFOControl = TRUE;

   dwSize = sizeof(dwFifo);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szTxFIFO,
                             NULL,
                             NULL,
                             (LPBYTE)(&dwFifo),
                             &dwSize);

   if (dwError == ERROR_SUCCESS) {
      //
      // Save this initial value
      //
      AdvancedData->TxFIFO = dwFifo;
      if (AdvancedData->TxFIFO > TX_MAX) {
          goto SetTxFIFO;
      }
   }
   else {
SetTxFIFO:
      //
      // value does not exist. Create our own
      //
      dwData = AdvancedData->FifoTxMax;
      dwSize = sizeof(dwData);
      dwError = RegSetValueEx(hDeviceKey,
                              m_szTxFIFO,
                              0,
                              REG_DWORD,
                              (LPBYTE)(&dwData),
                              dwSize);

      if (dwError == ERROR_SUCCESS) {
         AdvancedData->TxFIFO = AdvancedData->FifoTxMax;
      }
      else {
         AdvancedData->TxFIFO = 0;
         AdvancedData->UseTxFIFOControl = FALSE;
      }
   }

   //
   // Get Polling Period information
   //
   AdvancedData->PollingPeriod = PollingPeriods[POLL_PERIOD_DEFAULT_IDX];

   dwSize = sizeof(dwFifo);
   dwError = RegQueryValueEx(hDeviceKey,
                             m_szPollingPeriod,
                             NULL,
                             NULL,
                             (LPBYTE)(&dwFifo),
                             &dwSize);

   if (dwError == ERROR_SUCCESS) {
      //
      // Save this initial value
      //
      AdvancedData->PollingPeriod = dwFifo;
   }
   else {
      //
      // value does not exist. Create our own
      //
      dwData = AdvancedData->PollingPeriod;
      dwSize = sizeof(dwData);
      dwError = RegSetValueEx(hDeviceKey,
                              m_szPollingPeriod,
                              0,
                              REG_DWORD,
                              (LPBYTE)(&dwData),
                              dwSize);
   }

   RegCloseKey(hDeviceKey);

   if (ERROR_SUCCESS != dwError) {
      return dwError;
   }
   else {
      return ERROR_SUCCESS;
   }
} /* FillAdvancedDlg*/

void
ChangeParentTitle(
    IN HWND    Hwnd,
    IN LPCTSTR OldComName,
    IN LPCTSTR NewComName
    )
{
    INT    textLength, offset, newNameLen, oldNameLen;
    PTCHAR oldTitle = NULL, newTitle = NULL;
    PTCHAR oldLocation;

    textLength = GetWindowTextLength(Hwnd);
    if (textLength == 0) {
        return;
    }

    //
    // Account for null char and unicode
    //
    textLength++;
    oldTitle = (PTCHAR) LocalAlloc(LPTR, textLength * sizeof(TCHAR));
    if (!oldTitle) {
        return;
    }

    if (!GetWindowText(Hwnd, oldTitle, textLength)) {
        goto exit;
    }

    oldLocation = wcsstr(oldTitle, OldComName);
    if (!oldLocation) {
        goto exit;
    }

    newNameLen = lstrlen(NewComName);
    oldNameLen = lstrlen(OldComName);
    offset = newNameLen - oldNameLen;
    if (offset > 0) {
        textLength += offset;
    }
    newTitle = (PTCHAR) LocalAlloc(LPTR, textLength * sizeof(TCHAR));
    if (!newTitle) {
        goto exit;
    }

    //
    // Find the OldComName in the title and do the following
    // 1)  up to that location in the string
    // 2)  copy the new name
    // 3)  copy the remainder of the string after OldComName
    //
    offset = (INT)(oldLocation - oldTitle);
    CopyMemory(newTitle, oldTitle, offset * sizeof(TCHAR));                 // 1
    CopyMemory(newTitle + offset, NewComName, newNameLen * sizeof(TCHAR));  // 2
    lstrcpy(newTitle + offset + newNameLen, oldLocation + oldNameLen);      // 3

    SetWindowText(Hwnd, newTitle);

exit:
    if (oldTitle) {
        LocalFree(oldTitle);
    }
    if (newTitle) {
        LocalFree(newTitle);
    }
}

void
MigratePortSettings(
    LPCTSTR OldComName,
    LPCTSTR NewComName
    )
{
    TCHAR settings[BUFFER_SIZE];
    TCHAR szNew[20], szOld[20];

    lstrcpy(szOld, OldComName);
    wcscat(szOld, m_szColon);

    lstrcpy(szNew, NewComName);
    wcscat(szNew, m_szColon);

    settings[0] = TEXT('\0');
    GetProfileString(m_szPorts,
                     szOld,
                     TEXT(""),
                     settings,
                     sizeof(settings) / sizeof(TCHAR) );

    //
    // Insert the new key based on the old one
    //
    if (settings[0] == TEXT('\0')) {
        WriteProfileString(m_szPorts, szNew, m_szDefParams);
    }
    else {
        WriteProfileString(m_szPorts, szNew, settings);
    }

    //
    // Notify everybody of the changes and blow away the old key
    //
    SendWinIniChange((LPTSTR)m_szPorts);
    WriteProfileString(m_szPorts, szOld, NULL);
}

void
EnactComNameChanges(
    IN HWND             ParentHwnd,
    IN PADVANCED_DATA   AdvancedData,
    IN HKEY             hDeviceKey,
    IN UINT             NewComNum)
{
    DWORD  dwNewComNameLen;
    TCHAR  buffer[BUFFER_SIZE];
    TCHAR  szFriendlyNameFormat[LINE_LEN];
    TCHAR  szDeviceDesc[LINE_LEN];
    PTCHAR szNewComName;
    UINT   i;
    UINT   curComNum;
    BOOLEAN updateMapping = TRUE;

    SP_DEVINSTALL_PARAMS spDevInstall;

    curComNum = myatoi(AdvancedData->szComName + wcslen(m_szCOM));

    if (AdvancedData->hComDB != HCOMDB_INVALID_HANDLE_VALUE) {
        BYTE   portUsage[MAX_COM_PORT/8];
        DWORD  portsReported;
        char   mask;

        //
        // Check to see if the desired new COM number has been claimed in the
        // com name database.  If so, ask the user if they are *really* sure
        //
        ComDBGetCurrentPortUsage(AdvancedData->hComDB,
                                 portUsage,
                                 MAX_COM_PORT / 8,
                                 CDB_REPORT_BITS,
                                 &portsReported);

        if ((i = NewComNum % 8))
            mask = 1 << (i-1);
        else
            mask = (char) 0x80;
        if ((portUsage[(NewComNum-1)/8] & mask) &&
            MyMessageBox(ParentHwnd, IDS_PORT_IN_USE, IDS_NAME_PROPERTIES,
                         MB_YESNO | MB_ICONINFORMATION) == IDNO) {
            //
            // Port has been previously claimed and user doesn't want to override
            //
            return;
        }
    }

    if (!QueryDosDevice(AdvancedData->szComName, buffer, BUFFER_SIZE-1)) {
        //
        // The old com name does not exist in the mapping.  Basically, the symbolic
        // link from COMX => \Device\SerialY has been broken.  Just change the
        // value in the registry and the friendly name for the device; don't
        // change the dos symbolic name b/c one does not exist
        //
        updateMapping = FALSE;
    }
    else {
        TCHAR  szComFileName[20]; // more than enough for "\\.\COMXxxx"
        HANDLE hCom;

        lstrcpy(szComFileName, L"\\\\.\\");
        lstrcat(szComFileName, AdvancedData->szComName);

        //
        // Make sure that the port has not been opened by another application
        //
        hCom = CreateFile(szComFileName, GENERIC_READ, 0, NULL, OPEN_EXISTING,
                          FILE_ATTRIBUTE_NORMAL, NULL);

        //
        // If the file handle is invalid, then the com port is open, warn the user
        //
        if (hCom == INVALID_HANDLE_VALUE &&
            MyMessageBox(ParentHwnd, IDS_PORT_OPEN, IDS_NAME_PROPERTIES,
                         MB_YESNO | MB_ICONERROR) == IDNO) {
            return;
        }

        if (hCom != INVALID_HANDLE_VALUE) {
            CloseHandle(hCom);
        }
    }

    szNewComName = AdvancedData->szNewComName;
    wsprintf(szNewComName, _T("COM%d"), NewComNum);
    dwNewComNameLen = ByteCountOf(wcslen(szNewComName) + 1);

    //
    // Change the name in the symbolic namespace.
    // First try to get what device the old com name mapped to
    // (ie something like \Device\Serial0).  Then remove the mapping.  If
    // the user isn't an admin, then this will fail and the dialog will popup.
    // Finally, map the new name to the old device retrieved from the
    // QueryDosDevice
    //
    if (updateMapping) {
        BOOL removed;
        HKEY hSerialMap;

        if (!QueryDosDevice(AdvancedData->szComName, buffer, BUFFER_SIZE-1)) {
            //
            // This shouldn't happen because the previous QueryDosDevice call
            // succeeded
            //
            MyMessageBox(ParentHwnd, IDS_PORT_RENAME_ERROR, IDS_NAME_PROPERTIES,
                         MB_ICONERROR);
            return;
        }

        //
        // If this fails, then the following define will just replace the current
        // mapping.
        //
        removed = DefineDosDevice(DDD_REMOVE_DEFINITION, AdvancedData->szComName, NULL);

        if (!DefineDosDevice(DDD_RAW_TARGET_PATH, szNewComName, buffer)) {
            //
            // error, first fix up the remove definition and restore the old
            // mapping
            //
            if (removed) {
                DefineDosDevice(DDD_RAW_TARGET_PATH, AdvancedData->szComName, buffer);
            }

            MyMessageBox(ParentHwnd, IDS_PORT_RENAME_ERROR, IDS_NAME_PROPERTIES,
                         MB_ICONERROR);

            return;
        }

        //
        // Set the \\HARDWARE\DEVICEMAP\SERIALCOMM field
        //
        if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         m_szRegSerialMap,
                         0,
                         KEY_ALL_ACCESS,
                         &hSerialMap) == ERROR_SUCCESS) {

            TCHAR  szSerial[BUFFER_SIZE];
            DWORD  dwSerialSize, dwEnum, dwType, dwComSize;
            TCHAR  szCom[BUFFER_SIZE];

            i = 0;
            do {
                dwSerialSize = sizeof(szSerial);
                dwComSize = sizeof(szCom);
                dwEnum = RegEnumValue(hSerialMap,
                                      i++,
                                      szSerial,
                                      &dwSerialSize,
                                      NULL,
                                      &dwType,
                                      (LPBYTE)szCom,
                                      &dwComSize);

                if (dwEnum == ERROR_SUCCESS) {
                    if(dwType != REG_SZ)
                        continue;

                    if (wcscmp(szCom, AdvancedData->szComName) == 0) {
                        RegSetValueEx(hSerialMap,
                                        szSerial,
                                        0,
                                        REG_SZ,
                                        (PBYTE) szNewComName,
                                        dwNewComNameLen);
                                        break;
                    }
                }

                dwSerialSize = sizeof(szSerial);
                dwComSize = sizeof(szCom);

            } while (dwEnum == ERROR_SUCCESS);
        }

        RegCloseKey(hSerialMap);
    }

    //
    // Update the com db
    //
    if (AdvancedData->hComDB != HCOMDB_INVALID_HANDLE_VALUE) {
        ComDBReleasePort(AdvancedData->hComDB, (DWORD) curComNum);

        ComDBClaimPort(AdvancedData->hComDB, (DWORD) NewComNum, TRUE, NULL);
    }

    //
    // Set the friendly name in the form of DeviceDesc (COM#)
    //
    if (LoadString(g_hInst,
                   IDS_FRIENDLY_FORMAT,
                   szFriendlyNameFormat,
                   CharSizeOf(szFriendlyNameFormat)) &&
        SetupDiGetDeviceRegistryProperty(AdvancedData->DeviceInfoSet,
                                         AdvancedData->DeviceInfoData,
                                         SPDRP_DEVICEDESC,
                                         NULL,
                                         (PBYTE) szDeviceDesc,
                                         sizeof(szDeviceDesc),
                                         NULL)) {
        wsprintf(buffer, szFriendlyNameFormat, szDeviceDesc, szNewComName);
    }
    else {
        //
        // Use the COM port name straight out
        //
        lstrcpy(buffer, szNewComName);
    }

    SetupDiSetDeviceRegistryProperty(AdvancedData->DeviceInfoSet,
                                     AdvancedData->DeviceInfoData,
                                     SPDRP_FRIENDLYNAME,
                                     (PBYTE) buffer,
                                     ByteCountOf(wcslen(buffer)+1));

    //
    // Set the parent dialog's title to reflect the change in the com port's name
    //
    ChangeParentTitle(GetParent(ParentHwnd), AdvancedData->szComName, szNewComName);
    MigratePortSettings(AdvancedData->szComName, szNewComName);

    //
    // Update the PortName value in the devnode
    //
    RegSetValueEx(hDeviceKey,
                  m_szPortName,
                  0,
                  REG_SZ,
                  (PBYTE)szNewComName,
                  dwNewComNameLen);

    //
    // Now broadcast this change to the device manager
    //
    ZeroMemory(&spDevInstall, sizeof(SP_DEVINSTALL_PARAMS));
    spDevInstall.cbSize = sizeof(SP_DEVINSTALL_PARAMS);

    if (SetupDiGetDeviceInstallParams(AdvancedData->DeviceInfoSet,
                                      AdvancedData->DeviceInfoData,
                                      &spDevInstall)) {
        spDevInstall.Flags |= DI_PROPERTIES_CHANGE;
        SetupDiSetDeviceInstallParams(AdvancedData->DeviceInfoSet,
                                      AdvancedData->DeviceInfoData,
                                      &spDevInstall);
    }
}

/*++

Routine Description: SaveAdvancedSettings

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
SaveAdvancedSettings(
    IN HWND ParentHwnd,
    IN PADVANCED_DATA AdvancedData
    )
{
   HKEY   hDeviceKey;
   DWORD  dwSize, dwData;

   UINT  i = CB_ERR, curComNum, newComNum = CB_ERR;
   UINT  uiDlgButtonChecked;
   DWORD dwRxPosition, dwTxPosition, dwPollingPeriod;

   SP_DEVINSTALL_PARAMS spDevInstall;

   //
   // Grab all of the new settings
   //

   uiDlgButtonChecked = IsDlgButtonChecked(ParentHwnd, IDC_FIFO);

   dwTxPosition = Trackbar_GetPos(GetDlgItem(ParentHwnd, IDC_TRANSMIT_SLIDER));
   dwRxPosition = Trackbar_GetPos(GetDlgItem(ParentHwnd, IDC_RECEIVE_SLIDER));

   //
   // Index is actually into the array of values
   //
   dwRxPosition = RxValues[dwRxPosition-1];

   curComNum = myatoi(AdvancedData->szComName + wcslen(m_szCOM));
   newComNum = ComboBox_GetCurSel(GetDlgItem(ParentHwnd, PP_PORT_NUMBER));

   if (newComNum == CB_ERR) {
       newComNum = curComNum;
   }
   else {
       newComNum++;
   }

   i = ComboBox_GetCurSel(GetDlgItem(ParentHwnd, IDC_POLL_PERIOD));

   if (i == CB_ERR || i >= NUM_POLLING_PERIODS) {
       dwPollingPeriod = AdvancedData->PollingPeriod;
   }
   else {
       dwPollingPeriod = PollingPeriods[i];
   }

   //
   // See if they changed anything
   //
   if (((AdvancedData->UseFifoBuffers  && uiDlgButtonChecked == BST_CHECKED) ||
        (!AdvancedData->UseFifoBuffers && uiDlgButtonChecked == BST_UNCHECKED)) &&
       AdvancedData->RxFIFO == dwRxPosition &&
       AdvancedData->TxFIFO == dwTxPosition &&
       AdvancedData->PollingPeriod == dwPollingPeriod &&
       newComNum == curComNum) {
      //
      // They didn't change anything. Just exit.
      //
      return ERROR_SUCCESS;
   }

   //
   // Open the device key for the source device instance
   //
   hDeviceKey = SetupDiOpenDevRegKey(AdvancedData->DeviceInfoSet,
                                     AdvancedData->DeviceInfoData,
                                     DICS_FLAG_GLOBAL,
                                     0,
                                     DIREG_DEV,
                                     KEY_ALL_ACCESS);

   if (INVALID_HANDLE_VALUE == hDeviceKey) {
       // BUGBUG
      return ERROR_SUCCESS;
   }

   //
   // Check to see if the user changed the COM port name
   //
   if (newComNum != curComNum) {
      EnactComNameChanges(ParentHwnd,
                          AdvancedData,
                          hDeviceKey,
                          newComNum);
   }


   if ((AdvancedData->UseFifoBuffers  && uiDlgButtonChecked == BST_UNCHECKED) ||
       (!AdvancedData->UseFifoBuffers && uiDlgButtonChecked == BST_CHECKED)) {
      //
      // They changed the Use Fifo checkbox.
      //
      dwData = (uiDlgButtonChecked == BST_CHECKED) ? 1 : 0;
      dwSize = sizeof(dwData);
      RegSetValueEx(hDeviceKey,
                    m_szFIFO,
                    0,
                    REG_DWORD,
                    (CONST BYTE *)(&dwData),
                    dwSize);
   }

   if (AdvancedData->RxFIFO != dwRxPosition) {
      //
      // They changed the RxFIFO setting
      //
      dwData = dwRxPosition;
      dwSize = sizeof(dwData);
      RegSetValueEx(hDeviceKey,
                    m_szRxFIFO,
                    0,
                    REG_DWORD,
                    (CONST BYTE *)(&dwData),
                    dwSize);
   }

   if (AdvancedData->TxFIFO != dwTxPosition) {
      //
      // They changed the TxFIFO setting
      //
      dwData = dwTxPosition;
      dwSize = sizeof(dwData);
      RegSetValueEx(hDeviceKey,
                    m_szTxFIFO,
                    0,
                    REG_DWORD,
                    (CONST BYTE *)(&dwData),
                    dwSize);
   }

   if (AdvancedData->PollingPeriod != dwPollingPeriod) {
      //
      // They changed the polling period
      //
      dwData = dwPollingPeriod;
      dwSize = sizeof(dwData);
      RegSetValueEx(hDeviceKey,
                    m_szPollingPeriod,
                    0,
                    REG_DWORD,
                    (CONST BYTE *)(&dwData),
                    dwSize);

      //
      // Don't really care if this fails, nothing else we can do
      //
      CM_Reenumerate_DevNode(AdvancedData->DeviceInfoData->DevInst,
                             CM_REENUMERATE_NORMAL);
   }

   RegCloseKey(hDeviceKey);

   SetupDiCallClassInstaller(DIF_PROPERTYCHANGE,
                             AdvancedData->DeviceInfoSet,
                             AdvancedData->DeviceInfoData);

   return ERROR_SUCCESS;
} /* SaveAdvancedSettings*/


