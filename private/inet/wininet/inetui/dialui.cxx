/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    dialui.cxx

Abstract:

    Contains the implementation of all ui for wininet's dialing support

    Contents:

Author:

    Darren Mitchell (darrenmi) 22-Apr-1997

Environment:

    Win32(s) user-mode DLL

Revision History:

    22-Apr-1997 darrenmi
        Created


--*/

#include <wininetp.h>
#include <commctrl.h>
#include "autodial.h"
#include "rashelp.h"

static const CHAR szRegValDefaultEntry[] = "Default";

// function prototype in inetcpl to launch connections tab
typedef BOOL (WINAPI *LAUNCHCPL)(HWND);

// constant used to signal that we're not dialing
#define NOT_DIALING     0xffffff


/****************************************************************************

    FUNCTION: CenterWindow (HWND, HWND)

    PURPOSE:  Center one window on the desktop

****************************************************************************/
BOOL CenterWindow (HWND hwndChild)
{
    RECT    rChild, rParent;
    int     wChild, hChild, wParent, hParent;
    int     xNew, yNew;
    HDC     hdc;

    // Get the Height and Width of the child window
    GetWindowRect (hwndChild, &rChild);
    wChild = rChild.right - rChild.left;
    hChild = rChild.bottom - rChild.top;

    // Get the Height and Width of the parent window
    GetWindowRect (GetDesktopWindow(), &rParent);
    wParent = rParent.right - rParent.left;
    hParent = rParent.bottom - rParent.top;

    // Calculate new X position
    xNew = rParent.left + ((wParent - wChild) /2);

    // Calculate new Y position
    yNew = rParent.top  + ((hParent - hChild) /2);

    // Set it, and return
    return SetWindowPos (hwndChild, NULL,
        xNew, yNew, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
// Prompt to go offline dialog
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
INT_PTR CALLBACK GoOfflinePromptDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
        LPARAM lParam)
{
    switch (uMsg) {
    case WM_INITDIALOG:
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        return TRUE;
        break;
    case WM_COMMAND:
        switch (wParam) {
        case IDS_WORK_OFFLINE:
        case IDCANCEL:
            EndDialog(hDlg,TRUE);
            return TRUE;
            break;
        case IDS_TRY_AGAIN:
            EndDialog(hDlg,FALSE);
            return TRUE;
            break;
        default:
            break;
        }
        break;
   default:
        break;
   }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                          Go Online dialog
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

INT_PTR CALLBACK OnlineDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam,
    LPARAM lParam)
{
    switch(uMsg) {
    case WM_INITDIALOG:
        SetWindowPos(hDlg, HWND_TOPMOST, 0, 0, 0, 0,
                SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
        break;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
            case ID_CONNECT:
                EndDialog(hDlg, TRUE);
                break;
            case IDCANCEL:
            case ID_STAYOFFLINE:
                EndDialog(hDlg, FALSE);
                break;
        }
        break;
    }

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////
//
//                Progress UI for dialing a connectoid
//
///////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////

typedef struct _tagRASCONNSTATEMAP {
    RASCONNSTATE    rascs;
    UINT            uResourceID;
} RASCONNSTATEMAP;

RASCONNSTATEMAP rgRasStates[] = {
    { RASCS_OpenPort,                 IDS_DIALING       },
    { RASCS_AllDevicesConnected,      IDS_CONNECTED     },
    { RASCS_Authenticate,             IDS_AUTHENTICATE  },
    { RASCS_Disconnected,             IDS_DISCONNECTED  },
    { (RASCONNSTATE)0, 0 }
};

UINT uRasMsg = 0;

//
// Add a string to the details edit box
//
void PostString(HWND hDlg, LPWSTR pszString)
{
    HWND hwndEdit = GetDlgItem(hDlg, IDC_DETAILS_LIST);
    WCHAR szCR[] = L"\r\n";

    // move caret to end
    SendMessageWrapW(hwndEdit, EM_SETSEL, -1, 0);
    SendMessageWrapW(hwndEdit, EM_SETSEL, -1, -1);

    // replace selection (nothing) with new string
    SendMessageWrapW(hwndEdit, EM_REPLACESEL, 0, (LPARAM)pszString);

    // move caret to end
    SendMessageWrapW(hwndEdit, EM_SETSEL, -1, 0);
    SendMessageWrapW(hwndEdit, EM_SETSEL, -1, -1);

    // replace selection (nothing) with CR
    SendMessageWrapW(hwndEdit, EM_REPLACESEL, 0, (LPARAM)szCR);

    // scroll to end
    SendMessageWrapW(hwndEdit, EM_SCROLLCARET, 0, 0);
}

BOOL FillConnectoidComboBox(HWND hDlg, DIALSTATE *pInfo)
{
    HWND hwndCombo = GetDlgItem(hDlg, IDC_CONN_LIST);
    INET_ASSERT(hwndCombo);
    BOOL fSuccess = FALSE;

    ComboBox_ResetContent(hwndCombo);

    DWORD dwEntries, dwRet;
    RasEnumHelp RasEnum;
    dwRet = RasEnum.GetError();
    dwEntries = RasEnum.GetEntryCount();
    if(ERROR_SUCCESS == dwRet)
    {
        fSuccess = TRUE;

        // insert connectoid names from buffer into combo box
        DWORD i;
        for(i=0; i<dwEntries; i++)
        {
            SendMessageWrapW(hwndCombo, CB_ADDSTRING, 0, (LPARAM)RasEnum.GetEntryW(i));
        }

        // try to find connectoid from pinfo
        int iSel = (int)SendMessageWrapW(hwndCombo, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)pInfo->params.szEntryName);
        if(CB_ERR == iSel)
        {
            iSel = 0;
        }
        ComboBox_SetCurSel(hwndCombo, iSel);
    }

    return fSuccess;
}


BOOL RefreshConnectoidSettings(HWND hDlg, DIALSTATE *pInfo, BOOL fSetFocus)
{
    TCHAR szUser[UNLEN + 1];
    DWORD dwLen = UNLEN + 1, dwFocus = 0;
    BOOL fPassword = FALSE, fCanSave;

    // clear params
    memset(&pInfo->params, 0, sizeof(RASDIALPARAMSW));
    pInfo->params.dwSize = sizeof(RASDIALPARAMSW);

    // clear user name and password fields
    SetWindowTextWrapW(GetDlgItem(hDlg, IDC_USER_NAME), L"");
    SetWindowTextWrapW(GetDlgItem(hDlg, IDC_PASSWORD), L"");

    // get connectoid name from combo box and settings from Ras
    fPassword = FALSE;
    int iSel = ComboBox_GetCurSel(GetDlgItem(hDlg, IDC_CONN_LIST));
    if(CB_ERR == iSel)
        iSel = 0;
    if(CB_ERR != SendMessageWrapW(GetDlgItem(hDlg, IDC_CONN_LIST), CB_GETLBTEXT, (WPARAM)iSel, (LPARAM)pInfo->params.szEntryName))
    {
        RasEntryDialParamsHelp RasEntryDialParams;
        if(0 == RasEntryDialParams.GetError())
        {
            if(0 != RasEntryDialParams.GetW(NULL, &pInfo->params, &fPassword))
                fPassword = FALSE;  // paranoia
        }
    }

    // Set user name, password, and password save if present
    if(*pInfo->params.szUserName) {
        SetWindowTextWrapW(GetDlgItem(hDlg, IDC_USER_NAME), pInfo->params.szUserName);
        if(fPassword) {
            SetWindowTextWrapW(GetDlgItem(hDlg, IDC_PASSWORD), pInfo->params.szPassword);
            dwFocus = ID_CONNECT;
        } else {
            // no password, put focus there
            dwFocus = IDC_PASSWORD;
        }
    } else {
        // no user, put focus there
        dwFocus = IDC_USER_NAME;
    }

    if(fSetFocus && dwFocus)
        SetFocus(GetDlgItem(hDlg, dwFocus));

    // if user isn't logged on, can't save password or dial automatically
    fCanSave = (0 != GetUserName(szUser, &dwLen));
    EnableWindow(GetDlgItem(hDlg, IDC_SAVE_PASSWORD), fCanSave);

    // if password isn't saved, auto connect isn't an option
    CheckDlgButton(hDlg, IDC_SAVE_PASSWORD, fPassword ? BST_CHECKED : BST_UNCHECKED);
    EnableWindow(GetDlgItem(hDlg, IDC_AUTOCONNECT), fPassword && fCanSave);

    return fPassword;
}

void ActivateControls(HWND hDlg, BOOL fActive, DIALSTATE *pInfo)
{
    WCHAR pszTemp[MAX_PATH];
    int i;
    UINT uIDs[] =  {IDC_CONN_TXT, IDC_CONN_LIST, IDC_NAME_TXT, IDC_USER_NAME,
                    IDC_PASSWORD_TXT, IDC_PASSWORD, IDC_SAVE_PASSWORD,
                    ID_CONNECT, IDC_SETTINGS};
#define NUM_CONTROLS (sizeof(uIDs) / sizeof(UINT))

    for(i=0; i<NUM_CONTROLS; i++)
        EnableWindow(GetDlgItem(hDlg, uIDs[i]), fActive);

    // special case - Autoconnect is disabled if save password not picked
    EnableWindow(
        GetDlgItem(hDlg, IDC_AUTOCONNECT),
        fActive && IsDlgButtonChecked(hDlg, IDC_SAVE_PASSWORD));

    // fix cancel button
    if(TRUE == fActive && (pInfo->dwFlags & CI_SHOW_OFFLINE)) {
        i = IDS_WORK_OFFLINE;
    } else {
        i = IDS_CANCEL;
    }

    if(FALSE == fActive) {
        // Just hit connect and it's no longer enabled - set focus to cancel
        SetFocus(GetDlgItem(hDlg, IDCANCEL));
    }

    LoadStringWrapW(GlobalDllHandle, i, pszTemp, MAX_PATH);
    SetWindowTextWrapW(GetDlgItem(hDlg, IDCANCEL), pszTemp);
}


void SetDialStatus(HWND hDlg, RASCONNSTATE rcState)
{
    WCHAR   pszText[128];

    for (int nIndex = 0; rgRasStates[nIndex].uResourceID != 0; nIndex++) {
        if (rcState == rgRasStates[nIndex].rascs) {
            LoadStringWrapW(GlobalDllHandle, rgRasStates[nIndex].uResourceID,
                pszText, 128);
            PostString(hDlg, pszText);
            break;
        }
    }
}

#define RAS_BOGUS_AUTHFAILCODE_1    84
#define RAS_BOGUS_AUTHFAILCODE_2    74389484

DWORD RasErrorToIDS(DWORD dwErr)
{
    if(dwErr==RAS_BOGUS_AUTHFAILCODE_1 || dwErr==RAS_BOGUS_AUTHFAILCODE_2)
    {
        return IDS_PPPRANDOMFAILURE;
    }

    if((dwErr>=653 && dwErr<=663) || (dwErr==667) || (dwErr>=669 && dwErr<=675))
    {
        return IDS_MEDIAINIERROR;
    }

    switch(dwErr)
    {
    default:
        return IDS_PPPRANDOMFAILURE;

    case ERROR_LINE_BUSY:
        return IDS_PHONEBUSY;

    case ERROR_NO_ANSWER:
        return IDS_NOANSWER;

    case ERROR_NO_DIALTONE:
        return IDS_NODIALTONE;

    case ERROR_HARDWARE_FAILURE:    // modem turned off
    case ERROR_PORT_ALREADY_OPEN:   // procomm/hypertrm/RAS has COM port
    case ERROR_PORT_OR_DEVICE:      // got this when hypertrm had the device open -- jmazner
        return IDS_NODEVICE;

    case ERROR_BUFFER_INVALID:              // bad/empty rasdilap struct
    case ERROR_BUFFER_TOO_SMALL:            // ditto?
    case ERROR_CANNOT_FIND_PHONEBOOK_ENTRY: // if connectoid name in registry is wrong
    case ERROR_INTERACTIVE_MODE:
        return IDS_TCPINSTALLERROR;

    case ERROR_AUTHENTICATION_FAILURE:      // get this on actual CHAP reject
        return IDS_AUTHFAILURE;

    case ERROR_VOICE_ANSWER:
    case ERROR_NO_CARRIER:
    case ERROR_PPP_TIMEOUT:                 // get this on CHAP timeout
    case ERROR_REMOTE_DISCONNECTION:        // Ascend drops connection on auth-fail
    case ERROR_AUTH_INTERNAL:               // got this on random POP failure
    case ERROR_PROTOCOL_NOT_CONFIGURED:     // get this if LCP fails
    case ERROR_PPP_NO_PROTOCOLS_CONFIGURED: // get this if IPCP addr download gives garbage
        return IDS_PPPRANDOMFAILURE;
    }
    return 0;
}

void SetDialError(HWND hDlg, DIALSTATE *pInfo, DWORD dwError)
{
    WCHAR pszBuf[200];
    DWORD dwRes;

    dwRes = RasErrorToIDS(dwError);

    if(dwRes) {

        // we have a resource - use it
        LoadStringWrapW(GlobalDllHandle, dwRes, pszBuf, 200);

    } else {

        // couldn't get ras error, try system error
        if(0 == FormatMessageWrapW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, dwError, 0, pszBuf, 200, NULL)) {

            // couldn't get system error, get system error E_FAIL == Unknown error
            FormatMessageWrapW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, E_FAIL, 0, pszBuf, 200, NULL);
        }
    }

    PostString(hDlg, pszBuf);
}

void DoRasDial(HWND hDlg, DIALSTATE *pInfo)
{
    WCHAR   pszBuffer[128], pszTemplate[128];

    // we're trying to dial
    pInfo->dwTryCurrent++;
    if(pInfo->dwTryCurrent > pInfo->dwTry) {
        // all tried out
        EndDialog(hDlg, FALSE);
        return;
    }
    LoadStringWrapW(GlobalDllHandle, IDS_REDIAL_ATTEMPT, pszTemplate, ARRAYSIZE(pszTemplate));
    wnsprintfW(pszBuffer, ARRAYSIZE(pszBuffer), pszTemplate, pInfo->dwTryCurrent, pInfo->dwTry);
    PostString(hDlg, pszBuffer);

    // if there's a space in domain, replace with null
    if( (L' ' == pInfo->params.szDomain[0]) &&
        (0 == pInfo->params.szDomain[1]))
        pInfo->params.szDomain[0] = 0;

    pInfo->hConn = NULL;
    RasDialHelp RasDial(NULL, NULL, &pInfo->params, 0xFFFFFFFF, hDlg, &pInfo->hConn);
    if(0 != RasDial.GetError()) {
        // failed!

        // ras may return a connection anyway...
        if(pInfo->hConn) {
            _RasHangUp(pInfo->hConn);
            pInfo->hConn = NULL;
        }

        // return error
        pInfo->dwResult = ERROR_NO_CONNECTION;
        EndDialog(hDlg, FALSE);
    }
}

void DialEvent(HWND hDlg, DIALSTATE *pInfo, RASCONNSTATE rcState, DWORD dwError)
{
    if(dwError != SUCCESS) {

        // win95 returns this error if authentication failed.
        if(ERROR_UNKNOWN == dwError)
            dwError = ERROR_AUTHENTICATION_FAILURE;

        // save error
        pInfo->dwResult = dwError;

        // update progress
        SetDialError(hDlg, pInfo, dwError);

        // clean up connection
        _RasHangUp(pInfo->hConn);
        pInfo->hConn = NULL;

        switch(dwError) {
        case ERROR_AUTHENTICATION_FAILURE:
            memset(pInfo->params.szPassword, 0, ARRAYSIZE(pInfo->params.szPassword));
            pInfo->dwFlags &= ~(CI_SAVE_PASSWORD | CI_AUTO_CONNECT);
            PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
            break;
        case ERROR_USER_DISCONNECTION:
            // we hit cancel and called RasHangUp.  Nothing to do here - 
            // cancel code has cleaned up as necessary.
            break;
        case ERROR_LINE_BUSY:
        case ERROR_NO_ANSWER:
        case ERROR_NO_CARRIER:
            // this is retryable
            if(pInfo->dwTryCurrent < pInfo->dwTry) {
                // set timer for redial
                pInfo->dwWaitCurrent = pInfo->dwWait;
                pInfo->uTimerId = SetTimer(hDlg, 1, 1000, NULL);
                break;
            }

            // fall through
        default:
            PostMessage(hDlg, WM_COMMAND, IDCANCEL, 0);
        }
    } else {
        // we're getting status
        if(rcState == RASCS_Connected) {
            // we're done
            pInfo->dwResult = ERROR_SUCCESS;
            EndDialog(hDlg, TRUE);
        }
        SetDialStatus(hDlg, rcState);
    }
}

void SaveInfo(HWND hDlg, DIALSTATE *pInfo)
{
    // Save user name and password back into passed struct
    GetWindowTextWrapW(GetDlgItem(hDlg, IDC_USER_NAME),
                pInfo->params.szUserName, UNLEN);
    GetWindowTextWrapW(GetDlgItem(hDlg, IDC_PASSWORD),
                pInfo->params.szPassword, PWLEN);

    // Save off flags
    pInfo->dwFlags &= ~(CI_SAVE_PASSWORD | CI_AUTO_CONNECT);
    if(BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_SAVE_PASSWORD))
        pInfo->dwFlags |= CI_SAVE_PASSWORD;
    if(BST_CHECKED == IsDlgButtonChecked(hDlg, IDC_AUTOCONNECT))
        pInfo->dwFlags |= CI_AUTO_CONNECT;
}

INT_PTR CALLBACK ConnectDlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    DIALSTATE *pInfo = (DIALSTATE *)GetWindowLongPtr(hDlg,DWLP_USER);
    TCHAR pszTemp[MAX_PATH*2];
    DWORD dwLen = MAX_PATH*2;
    BOOL  fPassword = FALSE;

    // Handle RAS events
    if(uMsg == uRasMsg) {
        DialEvent(hDlg, pInfo, (RASCONNSTATE)wParam, (DWORD)lParam);
        return FALSE;
    }

    switch(uMsg) {
    case WM_INITDIALOG:
        // lParam contains pointer to url
        pInfo = (DIALSTATE *)lParam;

        // if we didn't get a pInfo, we can't do anything useful
        if(NULL == pInfo) {
            EndDialog(hDlg, FALSE);
            break;
        }

        // save pointer for later
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);

        // Find the message we're going to get
        uRasMsg = RegisterWindowMessageA(RASDIALEVENT);
        if(0 == uRasMsg)
            uRasMsg = WM_RASDIALEVENT;

        // fill and select appropriate connectoid 
        FillConnectoidComboBox(hDlg, pInfo);

        // set up controls and offline button
        ActivateControls(hDlg, TRUE, pInfo);

        // refresh user name / password for selected connectoid
        fPassword = RefreshConnectoidSettings(hDlg, pInfo, TRUE);

        // center this dialog and bring it to the foreground
        CenterWindow(hDlg);
        SetForegroundWindow(hDlg);

        // set try to special value to signal we're not currently dialing
        pInfo->dwTryCurrent = NOT_DIALING;

        // turn on auto connect if necessary
        if(pInfo->dwFlags & CI_AUTO_CONNECT) {
            if(fPassword) {
                CheckDlgButton(hDlg, IDC_AUTOCONNECT, BST_CHECKED);
                PostMessage(hDlg, WM_COMMAND, ID_CONNECT, 0);
            }
        } else if(pInfo->dwFlags & CI_DIAL_UNATTENDED) {
            if(fPassword) {
                // We have a password - do the unattended connect
                PostMessage(hDlg, WM_COMMAND, ID_CONNECT, 0);
            } else {
                // can't dial unattended - bail
                pInfo->dwResult = ERROR_INVALID_PASSWORD;
                EndDialog(hDlg, FALSE);
            }
        }
        break;

    case WM_TIMER:
        pInfo->dwWaitCurrent--;
        if(0 == pInfo->dwWaitCurrent) {
            KillTimer(hDlg, pInfo->uTimerId);
            pInfo->uTimerId = 0;
            DoRasDial(hDlg, pInfo);
        }
        break;

    case WM_COMMAND:
        // handle combo box messages
        if(HIWORD(wParam) == CBN_SELCHANGE) {
            RefreshConnectoidSettings(hDlg, pInfo, FALSE);
            break;
        }

        switch (LOWORD(wParam)) {
        case ID_CONNECT:
        {
            CDHINFO cdh;
            BOOL    fRasDial = TRUE;

            // grey out all the stuff you can't grope anymore
            ActivateControls(hDlg, FALSE, pInfo);

            // save all the info the user entered
            SaveInfo(hDlg, pInfo);

            // check for custom dial handler
            if(IsCDH(pInfo->params.szEntryName, &cdh))
            {
                ShowWindow(hDlg, SW_HIDE);

                // BUGBUG should do the customdial offline thing here if
                // necessary.  There are currently no known handlers that
                // pay attention to it though.  Beware: Early versions of CM
                // bag out if they get flags they don't recognize so we'll
                // just do the dial thing.
                if(FALSE == CallCDH(hDlg, pInfo->params.szEntryName,
                    &cdh, INTERNET_CUSTOMDIAL_CONNECT, &pInfo->dwResult))
                {
                    // cdh didn't handle it - flag to dial ourselves
                    pInfo->dwResult = ERROR_INVALID_PARAMETER;
                }

                if(ERROR_USER_DISCONNECTION == pInfo->dwResult)
                {
                    // user hit cancel - show our window again to allow a
                    // different choice
                    // BUGBUG perhaps a CDH cancel should cancel the entire
                    // dialing operation
                    ShowWindow(hDlg, SW_SHOW);
                }
                else if(ERROR_SUCCESS == pInfo->dwResult ||
                        ERROR_ALREADY_EXISTS == pInfo->dwResult)
                {
                    // successful connection
                    EndDialog(hDlg, TRUE);
                }
                else if(ERROR_INVALID_PARAMETER == pInfo->dwResult)
                {
                    // something busted in the custom dial handler.  Try to
                    // dial the connection with ras.
                    ShowWindow(hDlg, SW_SHOW);
                    fRasDial = TRUE;
                }
                else
                {
                    // some unknown error
                    EndDialog(hDlg, TRUE);
                }
            }

            if(DialIfWin2KCDH(pInfo->params.szEntryName, hDlg, TRUE, &pInfo->dwResult, NULL))
            {
                // don't do any more
                fRasDial = FALSE;
                EndDialog(hDlg, TRUE);
            }

            if(fRasDial)
            {
                // No custom dial handler or it didn't complete.  Do it 
                // through ras.

                // get redial info and set to first dial
                GetRedialParameters(pInfo->params.szEntryName, &pInfo->dwTry, &pInfo->dwWait); 
                pInfo->dwTryCurrent = 0;
                if(0 == pInfo->dwTry)
                    pInfo->dwTry = 1;

                // start the dial operation
                DoRasDial(hDlg, pInfo);
            }
            break;
        }

        case IDCANCEL:
            // clean up any dialing goo
            if(pInfo->hConn) {
                _RasHangUp(pInfo->hConn);
                pInfo->hConn = NULL;
            }

            if(pInfo->uTimerId) {
                KillTimer(hDlg, pInfo->uTimerId);
                pInfo->uTimerId = 0;
            }

            if((NOT_DIALING == pInfo->dwTryCurrent) || (pInfo->dwFlags & CI_DIAL_UNATTENDED))
            {
                // not currently dialing or cancelled an unattended dial 
                // bail out!
                SaveInfo(hDlg, pInfo);
                pInfo->dwResult = ERROR_USER_DISCONNECTION;
                EndDialog(hDlg, FALSE);
            } else {
                // Signal we're not dialing and turn on controls
                pInfo->dwTryCurrent = NOT_DIALING;
                ActivateControls(hDlg, TRUE, pInfo);

                // need to do this because ActivateControls will have turned
                // on the save password field and connect automatically
                // fields and they may not be valid.
                RefreshConnectoidSettings(hDlg, pInfo, TRUE);
            }
            break;
        case IDC_SAVE_PASSWORD:
            EnableWindow(
                GetDlgItem(hDlg, IDC_AUTOCONNECT),
                IsDlgButtonChecked(hDlg, IDC_SAVE_PASSWORD));
            break;
        case IDC_SETTINGS:
            {
                HMODULE hInetcpl = LoadLibrary("inetcpl.cpl");
                if(hInetcpl) {
                    LAUNCHCPL cpl = (LAUNCHCPL)GetProcAddress(hInetcpl, "LaunchConnectionDialog");
                    if(cpl) {
                        cpl(hDlg);

                        // refresh to new default if any
                        AUTODIAL config;
                        memset(&config, 0, sizeof(config));
                        if(IsAutodialEnabled(NULL, &config) && config.fHasEntry)
                        {
                            StrCpyW(pInfo->params.szEntryName, config.pszEntryName);
                        }

                        // refresh settings
                        FillConnectoidComboBox(hDlg, pInfo);
                        RefreshConnectoidSettings(hDlg, pInfo, TRUE);
                    }
                    FreeLibrary(hInetcpl);
                }
            }
            break;
        }
        break;
    }

    return FALSE;
}

