/** FILE: ups.c ********* Module Header ********************************
 *
 *  Control panel applet for UPS configuration.
 *  This applet let the user to specify the ups capabilities, including:
 *      - signalling on power failure
 *      - ability to turn itself off
 *      - signal low battery power
 *      - battery life and recharge time
 *      - signal voltage
 *  It also allows the user to specify the time of first notification
 *  and the notification interval thereafter
 *
 * History:
 *  1pm on Wed  08 Apr 1992  -by- Mark Cliggett [markcl]
 *        Created
 *  Saturday   05 Sept 1992  -by- Congpa You    [congpay]
 *        Rewrite the code for the new UPS dialog.
 *
 *  Copyright (C) 1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                              Include files
//==========================================================================
// C Runtime
#define _CTYPE_DISABLE_MACROS
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Application specific
#include "ups.h"


//==========================================================================
//                              Local Functions
//==========================================================================

/* EnableCONFIG enable/disable the UPS configuration groupbox
 * and the comport combobox if bVal = TRUE/FALSE.
 */
void EnableCONFIG (HWND hDlg, BOOL bVal)
{
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_PORTCB), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_UPSGROUP), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_TEXT), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_PFSIGNAL), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_LOWBATTERY), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_TURNOFF), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_COMMANDFILE), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_SIGN), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_TURNOFFHIGH), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_TURNOFFLOW), bVal);
}

/* EnablePFSINGNAL deals with power failure signal checkbox and radiobutton. */
void EnablePFSIGNAL (HWND hDlg, BOOL bVal)
{
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_PFSIGNALHIGH), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_PFSIGNALLOW), bVal);
}

/* EnableLOWBATTERY deals with low battery signal checkbox and radiobutton. */
void EnableLOWBATTERY (HWND hDlg, BOOL bVal)
{
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_LOWBATTERYHIGH), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_LOWBATTERYLOW), bVal);
}

/* EnableFILENAME deals with the Filename text field. */
void EnableFILENAME (HWND hDlg, BOOL bVal)
{
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_FILETEXT), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_FILENAME), bVal);
}

/* EnableCHARACTER deals with the UPS characteristics groupbox. */
void EnableCHARACTER (HWND hDlg, BOOL bVal)
{
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_CHARACTER), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_BLTEXT1), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_BLEDIT), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_BATTERYLIFE), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_BLTEXT2), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_RPMTEXT1), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_RPMEDIT), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_RECHARGEPERMINUTE), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_RPMTEXT2), bVal);
}

/* EnableSERVICE deals with the UPS service groupbox. */
void EnableSERVICE (HWND hDlg, BOOL bVal)
{
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_SERVICE), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_FWTEXT1), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_FWEDIT), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_FIRSTWARNING), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_FWTEXT2), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_WITEXT1), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_WIEDIT), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_WARNINGINTERVAL), bVal);
    EnableWindow (GetDlgItem (hDlg, IDD_UPS_WITEXT2), bVal);
}

/* Show draws the UPS dialog according to the UPS configuation.
 * There are only 8 cases because there are three check box in
 * UPS configuration groupbox.
 */
void Show (HWND hDlg, ULONG ulOptions)
{
    switch (ulOptions) {
    case UPS_POWERFAILSIGNAL:
        CheckDlgButton (hDlg, IDD_UPS_PFSIGNAL, TRUE);
        EnablePFSIGNAL (hDlg, TRUE);
        EnableLOWBATTERY (hDlg, FALSE);
        EnableCHARACTER (hDlg, TRUE);
        EnableSERVICE (hDlg, TRUE);
        break;
    case UPS_LOWBATTERYSIGNAL:
        CheckDlgButton (hDlg, IDD_UPS_LOWBATTERY, TRUE);
        EnablePFSIGNAL (hDlg, FALSE);
        EnableLOWBATTERY (hDlg, TRUE);
        EnableCHARACTER (hDlg, FALSE);
        EnableSERVICE (hDlg, FALSE);
        break;
    case UPS_CANTURNOFF:
        CheckDlgButton (hDlg, IDD_UPS_TURNOFF, TRUE);
        EnablePFSIGNAL (hDlg, FALSE);
        EnableLOWBATTERY (hDlg, FALSE);
        EnableCHARACTER (hDlg, FALSE);
        EnableSERVICE (hDlg, FALSE);
        break;
    case UPS_POWERFAILSIGNAL | UPS_LOWBATTERYSIGNAL:
        CheckDlgButton (hDlg, IDD_UPS_PFSIGNAL, TRUE);
        CheckDlgButton (hDlg, IDD_UPS_LOWBATTERY, TRUE);
        EnablePFSIGNAL (hDlg, TRUE);
        EnableLOWBATTERY (hDlg, TRUE);
        EnableCHARACTER (hDlg, FALSE);
        EnableSERVICE (hDlg, TRUE);
        break;
    case UPS_POWERFAILSIGNAL | UPS_CANTURNOFF:
        CheckDlgButton (hDlg, IDD_UPS_PFSIGNAL, TRUE);
        CheckDlgButton (hDlg, IDD_UPS_TURNOFF, TRUE);
        EnablePFSIGNAL (hDlg, TRUE);
        EnableLOWBATTERY (hDlg, FALSE);
        EnableCHARACTER (hDlg, TRUE);
        EnableSERVICE (hDlg, TRUE);
        break;
    case UPS_LOWBATTERYSIGNAL | UPS_CANTURNOFF:
        CheckDlgButton (hDlg, IDD_UPS_LOWBATTERY, TRUE);
        CheckDlgButton (hDlg, IDD_UPS_TURNOFF, TRUE);
        EnablePFSIGNAL (hDlg, FALSE);
        EnableLOWBATTERY (hDlg, TRUE);
        EnableCHARACTER (hDlg, FALSE);
        EnableSERVICE (hDlg, FALSE);
        break;
    case UPS_POWERFAILSIGNAL | UPS_LOWBATTERYSIGNAL | UPS_CANTURNOFF:
        CheckDlgButton (hDlg, IDD_UPS_PFSIGNAL, TRUE);
        CheckDlgButton (hDlg, IDD_UPS_LOWBATTERY, TRUE);
        CheckDlgButton (hDlg, IDD_UPS_TURNOFF, TRUE);
        EnablePFSIGNAL (hDlg, TRUE);
        EnableLOWBATTERY (hDlg, TRUE);
        EnableCHARACTER (hDlg, FALSE);
        EnableSERVICE (hDlg, TRUE);
        break;
    default: // All checkboxes in configuration groupbox are not checked.
        EnablePFSIGNAL (hDlg, FALSE);
        EnableLOWBATTERY (hDlg, FALSE);
        EnableCHARACTER (hDlg, FALSE);
        EnableSERVICE (hDlg, FALSE);
    }
}

// Local function. Used by UPSDlg. and ErrorOut.
void ShowError (HWND hDlg,
                DWORD dwError)
{
    TCHAR   szErrorMessage[LONGBZ];
    if (!FormatMessage (FORMAT_MESSAGE_FROM_SYSTEM,
                        NULL,
                        dwError,
                        0,
                        szErrorMessage,
                        LONGBZ,
                        NULL))
    {
        MessageBox (hDlg, szErrMem, szCtlPanel, MB_OK|MB_ICONSTOP); //szErrMem is loaded in cpl.c.
    }
    else
    {
        MessageBox (hDlg, szErrorMessage, szCtlPanel, MB_OK|MB_ICONSTOP);
    }
}

// Local Function. Used by UPSDlg.
BOOL ErrorOut (HWND      hDlg,
               DWORD     dwError,
               SC_HANDLE hups,
               SC_HANDLE hsc,
               HKEY      vhKey)
{
    ShowError(hDlg, dwError);
    CloseServiceHandle(hups);
    CloseServiceHandle(hsc);
    RegCloseKey(vhKey);
    EndDialog(hDlg, 0L);
    return(FALSE);
}

// Called after trying to start or stop service. Return TRUE if it gets to
// the final state. Otherwise return FALSE

BOOL FinishCheck (DWORD            dwFinalState,
                  SERVICE_STATUS * pss,
                  SC_HANDLE        hups)
{
    int     max_tries = MAXTRIES;
    int     i = 0;
    DWORD   sleep_time;
    DWORD   old_checkpoint = 0;
    DWORD   new_checkpoint = 0;
    DWORD   dwStartState;
    DWORD   dwPendingState;

    if (dwFinalState == SERVICE_STOPPED)
    {
        dwStartState = SERVICE_RUNNING;
        dwPendingState = SERVICE_STOP_PENDING;
    }
    else if (dwFinalState == SERVICE_RUNNING)
    {
        dwStartState = SERVICE_STOPPED;
        dwPendingState = SERVICE_START_PENDING;
    }
    else
        return(FALSE);

    while ((pss->dwCurrentState != dwFinalState) &&
           (i++ < max_tries))
    {
        if (!QueryServiceStatus (hups, pss))
            return(FALSE);

        if (pss->dwCurrentState != dwPendingState)
            break;

        new_checkpoint = pss->dwCheckPoint;

        if (old_checkpoint != new_checkpoint)
        {
            sleep_time = pss->dwWaitHint;
            if (sleep_time > SLEEP_TIME)
            {
                max_tries = ((3 * sleep_time)/SLEEP_TIME);
                sleep_time = SLEEP_TIME;
                i = 0;
            }
        }
        else
            sleep_time = SLEEP_TIME;

        old_checkpoint = new_checkpoint;

        Sleep (sleep_time);
    }

    if (pss->dwCurrentState != dwFinalState)
    {
        return(FALSE);
    }

    return(TRUE);
}

BOOL IsFilename (TCHAR * szFilename)
{
    if (strchr (szFilename, '\\') == NULL)
        return(TRUE);
    else
        return(FALSE);
}

BOOL IsExecutable (TCHAR * szFilename)
{
    szFilename += (strlen(szFilename) -4);
    if ((_strnicmp (szFilename, ".cmd", 5) == 0) ||
        (_strnicmp (szFilename, ".com", 5) == 0) ||
        (_strnicmp (szFilename, ".exe", 5) == 0) ||
        (_strnicmp (szFilename, ".bat", 5) == 0))
    {
        return(TRUE);
    }
    else
        return(FALSE);
}


//==========================================================================
//                     Local Data Declarations
//==========================================================================
TCHAR szUpsReg[]        = "System\\CurrentControlSet\\Services\\UPS";
TCHAR szPort[]       = "Port";
TCHAR szPortDefault[]= "COM1:";
TCHAR szOptions[]    = "Options";
TCHAR szBatteryLife[]       = "BatteryLife";
TCHAR szRechargePerMinute[] = "RechargeRate";
TCHAR szFirstWarning[]      = "FirstMessageDelay";
TCHAR szWarningInterval[]   = "MessageInterval";
TCHAR szCommandFile[]       = "CommandFile";
TCHAR  szCOM[] = "COM";
TCHAR  szCOLON[] = ":";
HKEY    vhKey;                  // handle to registry node
WNDPROC lpDefWndProc;

ARROWVSCROLL avs[4] = { { 1, -1, 5, -5, 720, 2, 12, 12 },
                        { 1, -1, 5, -5, 250, 1, 30, 30 },
                        { 1, -1, 5, -5, 120, 0, 30, 30 },
                        { 1, -1, 4, -4, 300, 5, 0, 0 } };

LRESULT NewEditWndProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_CHAR:
        if ((wParam < L'0' || wParam > L'9') &&
            (wParam != VK_BACK) &&
            (wParam != VK_DELETE) &&
            (wParam != VK_END) &&
            (wParam != VK_HOME))
        {
            MessageBeep (0);
            return TRUE;
        }

    default:
        break;
    }

    return(CallWindowProc(lpDefWndProc, hDlg, message, wParam, lParam));
}

//==========================================================================
//                     UPSDlg is the winproc for ups dialog.
//==========================================================================
BOOL UPSDlg (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    int     n;
    int     selection;
    int     num;
    int     focusid;
    int     id;
    int     i;
    int     RetVal;
    int     rc;
    DWORD   ulOptions;
    DWORD   ulBatteryLife;
    DWORD   ulRechargePerMinute;
    DWORD   ulFirstWarning;
    DWORD   ulWarningInterval;
    DWORD   cb;
    DWORD   dwStart;
    DWORD   dwError;
    DWORD   type;
    TCHAR * pMemory;
    TCHAR * pszKeyName;
    TCHAR * pszAug;
    TCHAR   szTemport[PORTLEN];
    TCHAR   szTemp[LONGBZ];
    TCHAR   szFilename[PATHMAX];
    TCHAR   szErrorMessage[10*LONGBZ];
    TCHAR   szSysDir[PATHMAX];
    TCHAR   szNum[SHORTBZ];
    TCHAR   szStatus[MIDBZ];
    BOOL    bOK;
    BOOL    fUpsSelected = FALSE;
    static BOOL    bPFSIGNAL = FALSE;
    static BOOL    bLOWBATTERY = FALSE;
    static BOOL    bTURNOFF = FALSE;
    static BOOL    bFILENAME = FALSE;
    SC_HANDLE hsc;
    SC_HANDLE hups;
    SERVICE_STATUS ss;
    WIN32_FIND_DATA ffd;
    HANDLE hFile;
    LONG   lpfnWndProc;

    switch (message)
    {
    case WM_INITDIALOG:

        HourGlass (TRUE);

        //  Read UPS info from registry

        //rc = RegOpenKey(HKEY_LOCAL_MACHINE, szUpsReg, &vhKey);
        rc = RegOpenKey(HKEY_LOCAL_MACHINE, szUpsReg, &vhKey);

        if (rc == ERROR_ACCESS_DENIED)
            {
                MyMessageBox (hDlg, UPS_ACCESS_ERROR, CPCAPTION, MB_OK|MB_ICONSTOP);
                RegCloseKey (vhKey);
                EndDialog (hDlg, 0L);
                return(FALSE);
            }

        else if (rc)
        {
            HourGlass (FALSE);
            MyMessageBox (hDlg, UPS_REGISTRY_ERROR, CPCAPTION, MB_OK|MB_ICONSTOP);
            RegCloseKey (vhKey);
            EndDialog (hDlg, 0L);
            return(FALSE);
        }

        // get UPS values
        cb = sizeof(ULONG);
        if (RegQueryValueEx(vhKey, szOptions, NULL, &type,
                (LPTSTR)&ulOptions, &cb))

            // if no data exists, UPS is not installed
            ulOptions = 0;

        cb = sizeof(ulOptions);
        if (RegQueryValueEx(vhKey, szBatteryLife, NULL, &type,
             (LPTSTR)&ulBatteryLife, &cb))
            ulBatteryLife = DEFAULTBATTERYLIFE;

        cb = sizeof(ulOptions);
        if (RegQueryValueEx(vhKey, szRechargePerMinute, NULL, &type,
                            (LPTSTR)&ulRechargePerMinute, &cb))
            ulRechargePerMinute = DEFAULTRECHARGEPERMINUTE;

        cb = sizeof(ulOptions);
        if (RegQueryValueEx(vhKey, szFirstWarning, NULL, &type,
                            (LPTSTR)&ulFirstWarning, &cb))
            ulFirstWarning = DEFAULTFIRSTWARNING;

        cb = sizeof(ulOptions);
        if (RegQueryValueEx(vhKey, szWarningInterval, NULL, &type,
                            (LPTSTR)&ulWarningInterval, &cb))
            ulWarningInterval = DEFAULTWARNINGINTERVAL;

        cb = sizeof (szTemport) / sizeof (szTemport[0]);
        if (RegQueryValueEx(vhKey, szPort, NULL, &type, szTemport, &cb))
            szTemport[0] = 0;

        cb = sizeof (szFilename) / sizeof (szFilename[0]);
        if (RegQueryValueEx(vhKey, szCommandFile, NULL, &type, szFilename, &cb))
            szFilename[0] = 0;

        // check to see if the current user can change settings
        if (RegSetValueEx(vhKey, szOptions, 0, REG_DWORD,
                              (LPSTR)&ulOptions, sizeof(ulOptions)) ==
                            ERROR_ACCESS_DENIED)
            {
                MyMessageBox(hDlg, UPS_ACCESS_ERROR, CPCAPTION, MB_OK|MB_ICONSTOP);
                RegCloseKey (vhKey);
                EndDialog (hDlg, 0L);
                return(FALSE);
            }

        SetDlgItemText (hDlg, IDD_UPS_FILENAME, szFilename);

        // Show recorded time values in registry.
        _itoa (ulBatteryLife, szNum, 10);
        SetDlgItemText (hDlg, IDD_UPS_BLEDIT, (LPSTR)szNum);
        _itoa (ulRechargePerMinute, szNum, 10);
        SetDlgItemText (hDlg, IDD_UPS_RPMEDIT, (LPSTR)szNum);
        _itoa (ulFirstWarning, szNum, 10);
        SetDlgItemText (hDlg, IDD_UPS_FWEDIT, (LPSTR)szNum);
        _itoa (ulWarningInterval, szNum, 10);
        SetDlgItemText (hDlg, IDD_UPS_WIEDIT, (LPSTR)szNum);


        // Show recorded signal voltage.
        CheckRadioButton(hDlg, IDD_UPS_PFSIGNALHIGH, IDD_UPS_PFSIGNALLOW,
                          ulOptions & UPS_POWERFAIL_LOW ?
                            IDD_UPS_PFSIGNALLOW : IDD_UPS_PFSIGNALHIGH);
        CheckRadioButton(hDlg, IDD_UPS_LOWBATTERYHIGH, IDD_UPS_LOWBATTERYLOW,
                          ulOptions & UPS_LOWBATTERY_LOW ?
                            IDD_UPS_LOWBATTERYLOW : IDD_UPS_LOWBATTERYHIGH);
        CheckRadioButton(hDlg, IDD_UPS_TURNOFFHIGH, IDD_UPS_TURNOFFLOW,
                          ulOptions & UPS_TURNOFF_LOW ?
                            IDD_UPS_TURNOFFLOW : IDD_UPS_TURNOFFHIGH);

        // Show port names from WIN.INI [ports] section
        SendDlgItemMessage (hDlg, IDD_UPS_PORTCB, CB_RESETCONTENT, 0, 0L);

        if (!(pszKeyName = pMemory = AllocMem(KEYBZ)))
        {
            MessageBox (hDlg, szErrMem, szCtlPanel, MB_OK|MB_ICONSTOP);
            RegCloseKey (vhKey);
            EndDialog (hDlg, 0L);
            return(FALSE);
        }

        GetProfileString ("ports", NULL, (LPSTR) szPortDefault, (LPSTR) pszKeyName, KEYBZ);

        n = selection = 0;
        while (*pszKeyName)
        {
            // Check if the string is COM?:
            if ((_strnicmp (pszKeyName, szCOM, 3) == 0) &&
                (isdigit (*(pszKeyName + 3))) &&
                (_strnicmp (pszKeyName + 4, szCOLON, 1) == 0))
            {
                SendDlgItemMessage(hDlg, IDD_UPS_PORTCB, CB_INSERTSTRING,
                                   (WPARAM)-1, (LPARAM)pszKeyName);
                if (!strcmp (pszKeyName, szTemport))
                    selection = n;
                n++;
            }

            SendMessage (GetDlgItem(hDlg, IDD_UPS_PORTCB),
                          CB_SETCURSEL, selection, 0L);

            /* Point to next string in buffer */
            pszKeyName += strlen (pszKeyName) + sizeof(TCHAR);
        }


        FreeMem(pMemory, KEYBZ);

        // Display other fields properly.
        if (ulOptions & UPS_INSTALLED)
        {
            CheckDlgButton (hDlg, IDD_UPS_EXISTS, TRUE);
            EnableCONFIG (hDlg, TRUE);

            // MASK gets the choice in configuration groupbox.
            Show (hDlg, ulOptions & MASK);

            // Enable command file selection?
            if (ulOptions & UPS_COMMANDFILE)
            {
                CheckDlgButton (hDlg, IDD_UPS_COMMANDFILE, TRUE);
                EnableFILENAME (hDlg, TRUE);
            }
            else
                EnableFILENAME (hDlg, FALSE);
        }

        // Set the ranges of the edit fields
        SendMessage(GetDlgItem(hDlg, IDD_UPS_FILENAME),
                    EM_LIMITTEXT,
                    PATHMAX-1,
                    0L);

        SendMessage(GetDlgItem(hDlg, IDD_UPS_BLEDIT),
                    EM_LIMITTEXT,
                    3,
                    0L);
        SendMessage(GetDlgItem(hDlg, IDD_UPS_RPMEDIT),
                    EM_LIMITTEXT,
                    3,
                    0L);
        SendMessage(GetDlgItem(hDlg, IDD_UPS_FWEDIT),
                    EM_LIMITTEXT,
                    3,
                    0L);
        SendMessage(GetDlgItem(hDlg, IDD_UPS_WIEDIT),
                    EM_LIMITTEXT,
                    3,
                    0L);

        // Change the window proc of the edit fields.
        lpDefWndProc = (WNDPROC)GetWindowLongPtr(GetDlgItem(hDlg, IDD_UPS_BLEDIT),
                                                 GWLP_WNDPROC);

        SetWindowLongPtr (GetDlgItem(hDlg, IDD_UPS_BLEDIT),
                          GWLP_WNDPROC,
                          (LONG_PTR)NewEditWndProc);

        SetWindowLongPtr (GetDlgItem(hDlg, IDD_UPS_RPMEDIT),
                          GWLP_WNDPROC,
                          (LONG_PTR)NewEditWndProc);

        SetWindowLongPtr (GetDlgItem(hDlg, IDD_UPS_FWEDIT),
                          GWLP_WNDPROC,
                          (LONG_PTR)NewEditWndProc);

        SetWindowLongPtr (GetDlgItem(hDlg, IDD_UPS_WIEDIT),
                          GWLP_WNDPROC,
                          (LONG_PTR)NewEditWndProc);

        if (!(ulOptions & UPS_INSTALLED))
        {
            EnableCONFIG (hDlg, FALSE);
            EnablePFSIGNAL (hDlg, FALSE);
            EnableLOWBATTERY (hDlg, FALSE);
            EnableCHARACTER (hDlg, FALSE);
            EnableSERVICE (hDlg, FALSE);
            EnableFILENAME (hDlg, FALSE);
            EnableWindow (GetDlgItem (hDlg, IDD_UPS_SIGN), FALSE);
        }

        HourGlass (FALSE);
        break;

    case WM_VSCROLL:
        focusid = GetWindowLong(GetFocus(), GWL_ID);
        switch (HIWORD(wParam))
        {
        case IDD_UPS_BATTERYLIFE:
            id = IDD_UPS_BLEDIT;
            i = 0;
            break;
        case IDD_UPS_RECHARGEPERMINUTE:
            id = IDD_UPS_RPMEDIT;
            i = 1;
            break;
        case IDD_UPS_FIRSTWARNING:
            id = IDD_UPS_FWEDIT;
            i = 2;
            break;
        case IDD_UPS_WARNINGINTERVAL:
            id = IDD_UPS_WIEDIT;
            i = 3;
            break;
        default:
            return (FALSE);
        }

        if (focusid != id)
            SetFocus(GetDlgItem(hDlg, id));

        switch(LOWORD(wParam))
        {
        case SB_THUMBTRACK:
        case SB_ENDSCROLL:
            return (TRUE);
            break;

        default:

            num = GetDlgItemInt (hDlg, id, &bOK, FALSE);
            num = ArrowVScrollProc (LOWORD(wParam), (short)num,
                                            (LPARROWVSCROLL) (avs + i));
            _itoa (num, szNum, 10);
            SetDlgItemText (hDlg, id, (LPSTR)szNum);
            SendDlgItemMessage (hDlg, id, EM_SETSEL, 0, -1L);

            break;
        }

        break;

    case WM_COMMAND:
        // Initialize ulOptions to 0 in order to avoid side effect.
        ulOptions = 0;

        switch (LOWORD(wParam))
        {
        case IDD_UPS_EXISTS:
            // Enable/Disable UPS options controls.

            if (!IsDlgButtonChecked (hDlg, IDD_UPS_EXISTS))
            {

                if (IsDlgButtonChecked (hDlg, IDD_UPS_PFSIGNAL))
                {
                    bPFSIGNAL = TRUE;
                }
                if (IsDlgButtonChecked (hDlg, IDD_UPS_LOWBATTERY))
                {
                    bLOWBATTERY = TRUE;
                }
                if (IsDlgButtonChecked (hDlg, IDD_UPS_TURNOFF))
                {
                    bTURNOFF = TRUE;
                }
                if (IsDlgButtonChecked (hDlg, IDD_UPS_COMMANDFILE))
                {
                    bFILENAME = TRUE;
                }
                SendMessage (GetDlgItem (hDlg, IDD_UPS_PFSIGNAL), BM_SETCHECK, FALSE, 0L);
                SendMessage (GetDlgItem (hDlg, IDD_UPS_LOWBATTERY), BM_SETCHECK, FALSE, 0L);
                SendMessage (GetDlgItem (hDlg, IDD_UPS_TURNOFF), BM_SETCHECK, FALSE, 0L);
                SendMessage (GetDlgItem (hDlg, IDD_UPS_COMMANDFILE), BM_SETCHECK, FALSE, 0L);
                EnableCONFIG (hDlg, FALSE);
                EnablePFSIGNAL (hDlg, FALSE);
                EnableLOWBATTERY (hDlg, FALSE);
                EnableCHARACTER (hDlg, FALSE);
                EnableSERVICE (hDlg, FALSE);
                EnableFILENAME (hDlg, FALSE);
            }
            else
            {
                EnableCONFIG (hDlg, TRUE);
                if (bPFSIGNAL)
                {
                    ulOptions |= UPS_POWERFAILSIGNAL;
                }
                if (bLOWBATTERY)
                {
                    ulOptions |= UPS_LOWBATTERYSIGNAL;
                }
                if (bTURNOFF)
                {
                    ulOptions |= UPS_CANTURNOFF;
                }
                Show (hDlg, (ulOptions & MASK));
                CheckDlgButton (hDlg, IDD_UPS_COMMANDFILE, bFILENAME);
                EnableFILENAME (hDlg, bFILENAME);
            }
            break;

        case IDD_UPS_PFSIGNAL:
        case IDD_UPS_LOWBATTERY:
        case IDD_UPS_TURNOFF:
            // Show the config properly.
            if (IsDlgButtonChecked (hDlg, IDD_UPS_PFSIGNAL))
            {
                bPFSIGNAL = TRUE;
                ulOptions |= UPS_POWERFAILSIGNAL;
            }
            else
                bPFSIGNAL = FALSE;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_LOWBATTERY))
            {
                bLOWBATTERY = TRUE;
                ulOptions |= UPS_LOWBATTERYSIGNAL;
            }
            else
                bLOWBATTERY = FALSE;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_TURNOFF))
            {
                bTURNOFF = TRUE;
                ulOptions |= UPS_CANTURNOFF;
            }
            else
                bTURNOFF = FALSE;
            Show (hDlg, (ulOptions & MASK));
            break;

        case IDD_UPS_COMMANDFILE:
            if (IsDlgButtonChecked (hDlg, IDD_UPS_COMMANDFILE))
            {
                bFILENAME = TRUE;
                ulOptions |= UPS_COMMANDFILE;
                EnableFILENAME (hDlg, TRUE);
            }
            else
            {
                bFILENAME = FALSE;
                EnableFILENAME (hDlg, FALSE);
            }
            break;

        case IDD_UPS_PFSIGNALHIGH:
        case IDD_UPS_PFSIGNALLOW:
        case IDD_UPS_LOWBATTERYHIGH:
        case IDD_UPS_LOWBATTERYLOW:
        case IDD_UPS_TURNOFFHIGH:
        case IDD_UPS_TURNOFFLOW:
        case IDD_UPS_BATTERYLIFE:
        case IDD_UPS_RECHARGEPERMINUTE:
        case IDD_UPS_FIRSTWARNING:
        case IDD_UPS_WARNINGINTERVAL:
            break;

        case IDOK:
            ulOptions = 0;
            HourGlass (TRUE);

            // Get the current status of UPS service.
            if (!(hsc = OpenSCManager(NULL, NULL, GENERIC_ALL)) ||
                !(hups = OpenService(hsc, "UPS", GENERIC_ALL)) ||
                !(QueryServiceStatus(hups, &ss)) )
            {
                dwError = GetLastError();
                return( ErrorOut (hDlg, dwError, hups, hsc, vhKey));
            }

            // Check whether the parameters are set correctly.
            if (IsDlgButtonChecked (hDlg, IDD_UPS_EXISTS)) // If UPS is selected.
            {
                // If neither Power failure signal nor Low battery signal is select,
                // popup a message box to tell the use to select one.
                if (!(IsDlgButtonChecked (hDlg, IDD_UPS_PFSIGNAL)) &&
                    !(IsDlgButtonChecked (hDlg, IDD_UPS_LOWBATTERY)))
                {
                    MyMessageBox(hDlg, UPS_OPTIONS_ERROR, CPCAPTION, MB_OK|MB_ICONSTOP);
                    break;
                }
            }
            else // If UPS is not selected set static variable bPFSIGNAL etc to 0.
            {
                bPFSIGNAL = FALSE;
                bLOWBATTERY = FALSE;
                bTURNOFF = FALSE;
                bFILENAME = FALSE;
            }

            //Get ulOptions
            if (IsDlgButtonChecked (hDlg, IDD_UPS_PFSIGNAL))
                ulOptions |= UPS_POWERFAILSIGNAL;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_LOWBATTERY))
                ulOptions |= UPS_LOWBATTERYSIGNAL;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_TURNOFF))
                ulOptions |= UPS_CANTURNOFF;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_PFSIGNALLOW))
                ulOptions |= UPS_POWERFAIL_LOW;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_LOWBATTERYLOW))
                ulOptions |= UPS_LOWBATTERY_LOW;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_TURNOFFLOW))
                ulOptions |= UPS_TURNOFF_LOW;
            if (IsDlgButtonChecked (hDlg, IDD_UPS_COMMANDFILE))
                ulOptions |= UPS_COMMANDFILE;

            fUpsSelected = IsDlgButtonChecked (hDlg, IDD_UPS_EXISTS);
            if (fUpsSelected)
            {
                ulOptions |= UPS_INSTALLED;
            }

            //Get the UPS parameters
            GetDlgItemText(hDlg, IDD_UPS_PORTCB, szTemport, PORTLEN);
            GetDlgItemText(hDlg, IDD_UPS_FWEDIT, szNum, 10);
            ulFirstWarning = atoi(szNum);
            GetDlgItemText(hDlg, IDD_UPS_WIEDIT, szNum, 10);
            ulWarningInterval = atoi(szNum);
            GetDlgItemText(hDlg, IDD_UPS_BLEDIT, szNum, 10);
            ulBatteryLife = atoi(szNum);
            GetDlgItemText(hDlg, IDD_UPS_RPMEDIT, szNum, 10);
            ulRechargePerMinute = atoi(szNum);

            if (fUpsSelected && (ulOptions & UPS_POWERFAILSIGNAL))
            {
                // Give warning if ulFirstWarning is not in the valid range.
                if ((ulFirstWarning < (ULONG) avs[2].bottom) | (ulFirstWarning > (ULONG) avs[2].top))
                {
                    MyMessageBox (hDlg, UPS_FWRange, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_FWEDIT));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_FWEDIT),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }

                // Give warning if ulWarningInterval is not in the valid range.
                if ((ulWarningInterval < (ULONG) avs[3].bottom) | (ulWarningInterval > (ULONG) avs[3].top))
                {
                    MyMessageBox (hDlg, UPS_WIRange, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_WIEDIT));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_WIEDIT),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }
            }

            if (fUpsSelected &&
                (ulOptions & UPS_POWERFAILSIGNAL) &&
                !(ulOptions & UPS_LOWBATTERYSIGNAL))
            {
                // Give warning if ulBatteryLife is not in the valid range.
                if ((ulBatteryLife <  (ULONG) avs[0].bottom) | (ulBatteryLife > (ULONG) avs[0].top))
                {
                    MyMessageBox (hDlg, UPS_BLRange, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_BLEDIT));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_BLEDIT),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }

                // Give warning if ulRechargePerMinute is not in the valid range.
                if ((ulRechargePerMinute < (ULONG) avs[1].bottom) | (ulRechargePerMinute > (ULONG) avs[1].top))
                {
                    MyMessageBox (hDlg, UPS_RPMRange, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_RPMEDIT));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_RPMEDIT),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }

                // Give warning if set first warning comes later then battery life.
                if (ulFirstWarning > ulBatteryLife * 60)
                {
                    MyMessageBox(hDlg, UPS_FW_WARNING, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_FWEDIT));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_FWEDIT),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }

                // Give warning if set warning interval time longer than battery life.
                if (ulWarningInterval > ulBatteryLife * 60)
                {
                    MyMessageBox(hDlg, UPS_DELAY_WARNING, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_WIEDIT));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_WIEDIT),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }
            }

            if (fUpsSelected && (ulOptions & UPS_COMMANDFILE))
            {
                GetDlgItemText(hDlg, IDD_UPS_FILENAME, szFilename, PATHMAX);
                if (!IsFilename (szFilename))
                {
                    MyMessageBox(hDlg, UPS_INVALID_PATH, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_FILENAME));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_FILENAME),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }

                if (!IsExecutable (szFilename))
                {
                    MyMessageBox(hDlg, UPS_INVALID_FILENAME, CPCAPTION, MB_OK|MB_ICONSTOP);
                    SetFocus (GetDlgItem (hDlg, IDD_UPS_FILENAME));
                    SendMessage(GetDlgItem (hDlg, IDD_UPS_FILENAME),
                                EM_SETSEL,
                                0,
                                -1L);
                    break;
                }

                // Check if the file is in the system directory.
                n = GetSystemDirectory(szSysDir, PATHMAX);
                if ((n == 0) || (n > PATHMAX))
                {
                    MyMessageBox(hDlg, UPS_CANT_FIND_SYSDIR, CPCAPTION, MB_OK|MB_ICONSTOP);
                    break;
                }
                else
                {
                    strcpy (szTemp, szSysDir);
                    strcat (szTemp, "\\");
                    strcat (szTemp, szFilename);
                    hFile = FindFirstFile (szTemp, &ffd);
                    if (hFile == INVALID_HANDLE_VALUE)
                    {
                        if (!LoadString (hModule, UPS_FILE_NOT_EXIST, szTemp, sizeof(szTemp)))
                        {
                            ErrLoadString (hDlg);
                            break;
                        }

                        pszAug = szSysDir;

                        if (!FormatMessage (FORMAT_MESSAGE_FROM_STRING|FORMAT_MESSAGE_ARGUMENT_ARRAY,
                                            szTemp,
                                            0,
                                            0,
                                            szErrorMessage,
                                            LONGBZ,
                                            (va_list *)&(pszAug)))
                        {
                            ShowError (hDlg, GetLastError());
                        }
                        else
                        {
                            MessageBox (hDlg, szErrorMessage, szCtlPanel, MB_OK|MB_ICONSTOP);
                        }
                        SetFocus (GetDlgItem (hDlg, IDD_UPS_FILENAME));
                        SendMessage(GetDlgItem (hDlg, IDD_UPS_FILENAME),
                                    EM_SETSEL,
                                    0,
                                    -1L);
                        break;
                    }
                    else
                    {
                        FindClose (hFile);
                    }
                }

                RegSetValueEx(vhKey, szCommandFile, 0, REG_SZ, szFilename, (strlen(szFilename)+1));
            }

            // Write the configuration to registry.
            RegSetValueEx(vhKey, szPort, 0, REG_SZ, szTemport, (strlen(szTemport)+1));

            RegSetValueEx(vhKey, szOptions, 0, REG_DWORD,
                          (LPSTR)&ulOptions, sizeof(ulOptions));
            RegSetValueEx(vhKey, szBatteryLife, 0, REG_DWORD,
                          (LPSTR)&ulBatteryLife, sizeof(ulBatteryLife));
            RegSetValueEx(vhKey, szRechargePerMinute, 0, REG_DWORD,
                          (LPSTR)&ulRechargePerMinute, sizeof(ulRechargePerMinute));
            RegSetValueEx(vhKey, szFirstWarning, 0, REG_DWORD,
                          (LPSTR)&ulFirstWarning, sizeof(ulFirstWarning));
            RegSetValueEx(vhKey, szWarningInterval, 0, REG_DWORD,
                          (LPSTR)&ulWarningInterval, sizeof(ulWarningInterval));

            // Start or stop the service according to user's choice.

            // If user select UPS, and the service is not running, start it.
            if (ss.dwCurrentState == SERVICE_STOPPED && fUpsSelected)
            {
                RetVal = MyMessageBox (hDlg, UPS_START_MSG, CPCAPTION, MB_YESNOCANCEL|MB_ICONINFORMATION);

                if (RetVal == IDCANCEL)
                {
                    break;
                }

                if (RetVal == IDYES)
                {
                    if (!StartService (hups, 0, NULL))
                    {
                        dwError = GetLastError();
                        return (ErrorOut (hDlg, dwError, hups, hsc, vhKey));
                    }

                    if (!FinishCheck (SERVICE_RUNNING, &ss, hups))
                    {
                        if (MyMessageBox (hDlg, UPS_STARTFAIL_MSG, CPCAPTION, MB_OKCANCEL|MB_ICONSTOP)
                            == IDOK)
                        {
                            break;
                        }
                    }
                }
            }

            // If the service is running, stop it first.
            else if ((ss.dwCurrentState == SERVICE_RUNNING) ||
                     (ss.dwCurrentState == SERVICE_PAUSED))
            {
                RetVal = MyMessageBox (hDlg,
                                       fUpsSelected? UPS_RESTART_MSG: UPS_STOP_MSG,
                                       CPCAPTION,
                                       MB_YESNOCANCEL|MB_ICONINFORMATION);

                if (RetVal == IDCANCEL)
                {
                    break;
                }

                if (RetVal == IDYES)
                {
                    // Stop the service first.
                    if (!ControlService (hups, SERVICE_CONTROL_STOP, &ss))
                    {
                        dwError = GetLastError();
                        return( ErrorOut (hDlg, dwError, hups, hsc, vhKey));
                    }

                    if (!FinishCheck (SERVICE_STOPPED, &ss, hups))
                    {
                        if (MyMessageBox (hDlg, UPS_STOPFAIL_MSG, CPCAPTION, MB_OKCANCEL|MB_ICONSTOP)
                            == IDCANCEL)
                        {
                            break;
                        }
                        else
                            fUpsSelected = FALSE; // For error break out.
                    }

                    // If user selected UPS, start it again. Otherwise exit.
                    if (fUpsSelected)
                    {
                        if (!StartService (hups, 0, NULL))
                        {
                            dwError = GetLastError();
                            return (ErrorOut (hDlg, dwError, hups, hsc, vhKey));
                        }

                        if (!FinishCheck (SERVICE_RUNNING, &ss, hups))
                        {
                            if (MyMessageBox (hDlg, UPS_STARTFAIL_MSG, CPCAPTION, MB_OKCANCEL|MB_ICONSTOP)
                                == IDOK)
                            {
                                break;
                            }
                        }
                    }
                }
            }

            // If the service is in pending state. ask user to use Services applet.
            else if ((ss.dwCurrentState == SERVICE_START_PENDING)    ||
                     (ss.dwCurrentState == SERVICE_STOP_PENDING)     ||
                     (ss.dwCurrentState == SERVICE_CONTINUE_PENDING) ||
                     (ss.dwCurrentState == SERVICE_PAUSE_PENDING))
            {
                if (MyMessageBox (hDlg, UPS_PENDING_MSG, CPCAPTION, MB_OKCANCEL|MB_ICONSTOP)
                    == IDCANCEL)
                    break;
            }

            // If the service in Unknow state, report error.
            else
                MyMessageBox (hDlg, UPS_UNKNOWNSTATE_MSG, CPCAPTION, MB_OK|MB_ICONSTOP);

            // Set UPS service to be auto start if the UPS installed box is checked.
            // Set UPS service to be manual start if the installed box is not checked.
            dwStart = fUpsSelected? SERVICE_AUTO_START : SERVICE_DEMAND_START;

            if (!ChangeServiceConfig (hups,
                                      SERVICE_NO_CHANGE,
                                      dwStart,
                                      SERVICE_ERROR_NORMAL,
                                      NULL,
                                      NULL,
                                      NULL,
				      NULL,
				      NULL,
                                      NULL,
                                      NULL))
            {
                dwError = GetLastError();
                return (ErrorOut (hDlg, dwError, hups, hsc, vhKey));
            }

            // Close opened services handles.
            CloseServiceHandle(hups);
            CloseServiceHandle(hsc);

            // Close registry handle and exit dialog.
            RegCloseKey(vhKey);
            EndDialog (hDlg, 0L);
            break;

        case IDCANCEL:
            RegCloseKey(vhKey);
            EndDialog (hDlg, 0L);
            break;

        case IDD_HELP:
            CPHelp(hDlg);
            break;

        default:
            break;
        }
        break;

    default:
        if (message == wHelpMessage)
        {
            CPHelp(hDlg);
        }
        else
            return FALSE;
        break;
    }
  return(TRUE);
}

// Turn hourglass on or off

void HourGlass (BOOL bOn)
{
   if (!GetSystemMetrics(SM_MOUSEPRESENT))
      ShowCursor(bOn);

   SetCursor(LoadCursor(NULL, bOn ? IDC_WAIT : IDC_ARROW));
}
