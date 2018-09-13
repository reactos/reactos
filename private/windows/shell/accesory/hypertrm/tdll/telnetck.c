/*	File: D:\WACKER\tdll\telnetck.c (Created: 26-Nov-1996 by cab)
 *
 *	Copyright 1996 by Hilgraeve Inc. -- Monroe, MI
 *	All rights reserved
 *
 *  Description:
 *      Implements the functions used to implement "telnet checking".
 *      This is HyperTerminal's way of assuring that it is the
 *      default telnet app for Internet Explorer and Netscape Navigator.
 *
 *	$Revision: 2 $
 *	$Date: 3/26/99 8:07a $
 */

#include <windows.h>
#pragma hdrstop

#include "features.h"

#ifdef INCL_DEFAULT_TELNET_APP

#include "assert.h"
#include "stdtyp.h"
#include "globals.h"
#include "tchar.h"
#include "registry.h"

#include "hlptable.h"

// Control IDs for the dialog:
//
#define IDC_PB_YES          IDOK
#define IDC_PB_NO           IDCANCEL
#define IDC_CK_STOP_ASKING  200
#define IDC_ST_QUESTION     201
#define IDC_IC_EXCLAMATION  202

// Registry key for HyperTerminal:
//
#ifndef NT_EDITION
static const TCHAR g_achHyperTerminalRegKey[] =
    TEXT("SOFTWARE\\Hilgraeve Inc\\HyperTerminal PE\\3.0");
#else
static const TCHAR g_achHyperTerminalRegKey[] =
    TEXT("SOFTWARE\\Microsoft\\HyperTerminal");
#endif

// Registry value for telnet checking:
//
static const TCHAR g_achTelnetCheck[] = TEXT("Telnet Check");

// Registry keys for the web browsers:
//
static const TCHAR g_achIERegKey[] =
    TEXT("SOFTWARE\\Classes\\telnet\\shell\\open\\command");

static const TCHAR g_achNetscapeRegKey[] =
    TEXT("SOFTWARE\\Netscape\\Netscape Navigator\\Viewers");

// Registry values for the browser telnet apps:
//
static const TCHAR g_achIERegValue[] = TEXT("");

static const TCHAR g_achNetscapeRegValue[] = TEXT("telnet");

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	IsHyperTerminalDefaultTelnetApp
 *
 * DESCRIPTION:
 *	Determines if HyperTerminal is the default telnet app for Internet
 *  Explorer and Netscape Navigator.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  C. Baumgartner, 11/26/96
 */
BOOL IsHyperTerminalDefaultTelnetApp(void)
    {
    TCHAR acExePath[MAX_PATH];
    TCHAR acRegistryData[MAX_PATH * 2];
    long  lRet = 0;
    DWORD dwSize = sizeof(acRegistryData);

    // Get the path name of HyperTerminal.
    //
    acExePath[0] = TEXT('\0');
    GetModuleFileName(glblQueryHinst(), acExePath, MAX_PATH);

    // Get IE's default telnet app.
    //
    acRegistryData[0] = TEXT('\0');
    if ( regQueryValue(HKEY_LOCAL_MACHINE, g_achIERegKey,
            g_achIERegValue, acRegistryData, &dwSize) == 0 )
        {
        if ( StrCharStrStr(acRegistryData, acExePath) == NULL )
            {
            return FALSE;
            }
        }
        
    // Get Netscape's default telnet app.
    //
    acRegistryData[0] = TEXT('\0');
    if ( regQueryValue(HKEY_CURRENT_USER, g_achNetscapeRegKey,
            g_achNetscapeRegValue, acRegistryData, &dwSize) == 0 )
        {
        if ( StrCharStrStr(acRegistryData, acExePath) == NULL )
            {
            return FALSE;
            }
        }

    return TRUE;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	QueryTelnetCheckFlag
 *
 * DESCRIPTION:
 *	Returns the value of the "telnet checking" flag. If this is TRUE,
 *  the app should check whether it is the default telnet app for IE
 *  and Netscape. If it isn't the default telnet app, then display
 *  the "Default Telnet App" dialog. The user can disable "telnet
 *  checking" by checking the "Stop asking me this question" box.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  C. Baumgartner, 11/26/96
 */
 BOOL QueryTelnetCheckFlag(void)
    {
    DWORD dwTelnetCheck = TRUE;
    DWORD dwSize = sizeof(dwTelnetCheck);

    if ( regQueryValue(HKEY_CURRENT_USER, g_achHyperTerminalRegKey,
            g_achTelnetCheck, (LPBYTE) &dwTelnetCheck, &dwSize) == 0 )
        {
        return dwTelnetCheck;
        }

    return TRUE;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SetTelnetCheckFlag
 *
 * DESCRIPTION:
 *	Sets the "telnet checking" flag which will either turn on or off
 *  this feature the next time HyperTerminal starts.
 *
 * PARAMETERS:
 *	fCheck - Check if HyperTerminal is the default telnet app?
 *
 * RETURNS:
 *	0 if successful, -1 if error
 *
 * AUTHOR:  C. Baumgartner, 11/27/96
 */
int SetTelnetCheckFlag(BOOL fCheck)
    {
    int iRet = 0;

    if ( regSetDwordValue(HKEY_CURRENT_USER, g_achHyperTerminalRegKey,
            g_achTelnetCheck, (DWORD)fCheck) != 0 )
        {
        iRet = -1;
        }

    return iRet;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	SetDefaultTelnetApp
 *
 * DESCRIPTION:
 *	Sets the default telnet application for IE and Netscape to HyperTerminal.
 *
 * PARAMETERS:
 *	None
 *
 * RETURNS:
 *	0 if successful, -1 if error
 *
 * AUTHOR:  C. Baumgartner, 11/27/96
 */
 int SetDefaultTelnetApp(void)
    {
    int   iRet = 0;
    TCHAR acExePath[MAX_PATH];
    TCHAR acRegistryData[MAX_PATH * 2];

    // Get the path name of HyperTerminal.
    //
    acExePath[0] = TEXT('\0');
    GetModuleFileName(glblQueryHinst(), acExePath, MAX_PATH);

    // Create the Netscape telnet command string.
    //
    acRegistryData[0] = TEXT('\0');
    wsprintf(acRegistryData, "%s /t", acExePath);

    // Write it to the registry.
    //
    if ( regSetStringValue(HKEY_CURRENT_USER, g_achNetscapeRegKey,
            g_achNetscapeRegValue, acRegistryData) != 0 )
        {
        // Just set the return flag to mark that we failed.
        //
        iRet = -1;
        }

    // Create the IE telnet command string.
    //
    acRegistryData[0] = TEXT('\0');
    wsprintf(acRegistryData, "%s /t %%1", acExePath);

    // Write it to the registry.
    //
    if ( regSetStringValue(HKEY_LOCAL_MACHINE, g_achIERegKey, g_achIERegValue,
            acRegistryData) != 0 )
        {
        // Just set the return flag to mark that we failed.
        //
        iRet = -1;
        }

    return iRet;
    }

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
 * FUNCTION:
 *	DefaultTelnetAppDlgProc
 *
 * DESCRIPTION:
 *	The dialog procedure for the "Default Telnet App" dialog.
 *  This dialog asks the user if he/she wants HyperTerminal
 *  to be the default telnet app for IE and NN. There also is
 *  a check box to disable this potentially annoying feature.
 *
 * PARAMETERS:
 *	hDlg - The dialog's window handle.
 *  wMsg - The message being sent to the window.
 *  wPar - The message's wParam.
 *  lPar - The message's lParam.
 *
 * RETURNS:
 *	TRUE or FALSE
 *
 * AUTHOR:  C. Baumgartner, 11/26/96
 */
BOOL CALLBACK DefaultTelnetAppDlgProc(HWND hDlg, UINT wMsg,
        WPARAM wPar, LPARAM lPar)
    {

	static	DWORD aHlpTable[] = {IDC_CK_STOP_ASKING,	IDH_TELNETCK_STOP_ASKING,
                                IDC_PB_YES,             IDH_TELNETCK_YES,
								IDC_PB_NO,				IDH_TELNETCK_NO,	
								0, 						0};

	switch (wMsg)
		{
	case WM_DESTROY:
        // Check the value of the "Stop asking me" checkbox.
        //
        SetTelnetCheckFlag(!IsDlgButtonChecked(hDlg, IDC_CK_STOP_ASKING));
		break;

	case WM_HELP:
        doContextHelp(aHlpTable, wPar, lPar, FALSE, FALSE);
		break;

	case WM_COMMAND:
		switch (wPar)
			{
		case IDC_PB_YES:
            SetDefaultTelnetApp();
			EndDialog(hDlg, TRUE);
			break;

		case IDC_PB_NO:
			EndDialog(hDlg, FALSE);
			break;

		default:
			return FALSE;
			}
		break;

	default:
		return FALSE;
		}

	return TRUE;
    }

#endif
