//*************************************************************
//
//  Debug.c     -   Debugging utility for User Environments
//
//  Microsoft Confidential
//  Copyright (c) Microsoft Corporation 1995
//  All rights reserved
//
//*************************************************************

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <ntexapi.h>
#include "debug.h"


int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                     LPSTR lpCmdLine, INT nCmdShow)
{

    DialogBox (hInstance, TEXT("DEBUG"), NULL, DebugDlgProc);

    return 0;

}

BOOL CALLBACK DebugDlgProc (HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg) {

        case WM_INITDIALOG:
            {
            HKEY hKey;
            LONG lResult;
            DWORD dwButton = IDD_NORMAL;
            DWORD dwType, dwSize, dwValue;

            lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   WINLOGON_KEY,
                                   0,
                                   KEY_READ,
                                   &hKey);

            if (lResult == ERROR_SUCCESS) {

                dwSize = sizeof(dwValue);
                lResult = RegQueryValueEx(hKey,
                                          USERENV_DEBUG_LEVEL,
                                          NULL,
                                          &dwType,
                                          (LPBYTE)&dwValue,
                                          &dwSize);

                if (lResult == ERROR_SUCCESS) {

                    if (LOWORD(dwValue) == DL_NONE) {
                        dwButton = IDD_NONE;
                    } else if (LOWORD(dwValue) == DL_VERBOSE) {
                        dwButton = IDD_VERBOSE;
                    }

                }

                RegCloseKey(hKey);
            }

            CheckRadioButton (hDlg, IDD_NONE, IDD_VERBOSE, dwButton);

            if (dwValue & DL_LOGFILE) {
                CheckDlgButton (hDlg, IDD_LOGFILE, 1);
            }


            //
            // Now check for winlogon
            //

            dwButton = 0;
            if (RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                   EXEC_OPTIONS_KEY,
                                   0,
                                   KEY_READ,
                                   &hKey) == ERROR_SUCCESS) {

                dwButton = 1;
                RegCloseKey(hKey);
            }

            CheckDlgButton (hDlg, IDD_WINLOGON, dwButton);

            }
            return TRUE;

        case WM_COMMAND:
            switch (LOWORD(wParam)) {
                case IDOK:
                    {
                    HKEY hKey;
                    LONG lResult;
                    DWORD dwType, dwValue = DL_NORMAL;
                    DWORD dwButton, dwSize, dwDisp;

                    if (IsDlgButtonChecked(hDlg, IDD_NONE)) {
                        dwValue = DL_NONE;
                    } else if (IsDlgButtonChecked(hDlg, IDD_VERBOSE)) {
                        dwValue = (DL_VERBOSE | DL_DEBUGGER);
                    }

                    if (IsDlgButtonChecked(hDlg, IDD_LOGFILE)) {
                        dwValue |= DL_LOGFILE;
                    }

                    lResult = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                           WINLOGON_KEY,
                                           0,
                                           KEY_WRITE,
                                           &hKey);

                    if (lResult == ERROR_SUCCESS) {

                        lResult = RegSetValueEx(hKey,
                                                USERENV_DEBUG_LEVEL,
                                                0,
                                                REG_DWORD,
                                                (LPBYTE)&dwValue,
                                                sizeof(dwValue));

                        if (lResult != ERROR_SUCCESS) {
                            MessageBox(hDlg, TEXT("Failed to save settings."), NULL, MB_OK);
                        }

                        RegCloseKey(hKey);
                    }


                    //
                    // Debug output for winlogon / msgina
                    //

                    if (dwValue & DL_VERBOSE) {

                        WriteProfileString (TEXT("Winlogon"),
                                            TEXT("DebugFlags"),
                                            TEXT("Error,Warning,Trace,Init,Timeout,Sas,State,CoolSwitch,Profile,Notify,Job"));

                        WriteProfileString (TEXT("Winlogon"),
                                            TEXT("EnableDesktopSwitching"),
                                            TEXT("1"));

                        WriteProfileString (TEXT("MSGina"),
                                            TEXT("DebugFlags"),
                                            TEXT("Error,Warning,Trace,Domain,Cache"));

                    } else {

                        WriteProfileString (TEXT("Winlogon"),
                                            TEXT("DebugFlags"),
                                            NULL);

                        WriteProfileString (TEXT("Winlogon"),
                                            TEXT("EnableDesktopSwitching"),
                                            NULL);

                        WriteProfileString (TEXT("MSGina"),
                                            TEXT("DebugFlags"),
                                            NULL);
                    }



                    //
                    // Now check for winlogon
                    //

                    if (IsDlgButtonChecked(hDlg, IDD_WINLOGON)) {

                        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                           EXEC_OPTIONS_KEY,
                                           0,
                                           NULL,
                                           REG_OPTION_NON_VOLATILE,
                                           KEY_ALL_ACCESS,
                                           NULL,
                                           &hKey,
                                           &dwDisp) == ERROR_SUCCESS) {

                            RegSetValueEx (hKey,
                                           TEXT("Debugger"),
                                           0, REG_SZ,
                                           (LPBYTE) TEXT("ntsd -d -G"),
                                           16);

                            RegCloseKey (hKey);
                        }

                    } else {
                        RegDeleteKey (HKEY_LOCAL_MACHINE,
                                      EXEC_OPTIONS_KEY);
                    }

                    EndDialog(hDlg, FALSE);
                    return TRUE;
                    }

                case IDCANCEL:
                    EndDialog(hDlg, FALSE);
                    return TRUE;

            }
            break;

        default:
            break;
    }

    return FALSE;
}
