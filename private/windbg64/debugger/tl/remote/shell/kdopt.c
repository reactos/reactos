/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    kdopt.c

Abstract:


Author:

    Wesley Witt (wesw) 26-July-1993

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


LPSTR KdComPorts[]   = { "COM1", "COM2", "COM3", "COM4" };
LPSTR KdPlatforms[]  = { "X86", "MIPS", "ALPHA", "PPC"};
DWORD KdBaudRates[]  = { 1200, 2400, 4800, 9600, 19200, 38400, 56000 };
DWORD KdCacheSizes[] = { 102400, 512000, 1024000 };

#define KdMaxComPorts   (sizeof(KdComPorts)/sizeof(LPSTR))
#define KdMaxPlatforms  (sizeof(KdPlatforms)/sizeof(LPSTR))
#define KdMaxBaudRates  (sizeof(KdBaudRates)/sizeof(DWORD))
#define KdMaxCacheSizes (sizeof(KdCacheSizes)/sizeof(DWORD))

void EnableControls( HWND hDlg, BOOL fMode );

LPKDPARAMS KdParams;


BOOL
DlgKernelDbg(
    HWND    hDlg,
    UINT    message,
    WPARAM  wParam,
    LONG    lParam
    )
{
    char        rgch[256];
    BOOL        b;
    BOOL        fChange = FALSE;
    int         i;
    HWND        hCtl;
    DWORD       tmp;

    switch (message) {
        case WM_INITDIALOG:
            KdParams = (LPKDPARAMS) lParam;

            if (KdParams->fEnable) {
                CheckDlgButton( hDlg, ID_KD_ENABLE, 1);
            }

            if (KdParams->fGoExit) {
                CheckDlgButton( hDlg, ID_KD_GOEXIT, 1);
            }

            if (KdParams->fVerbose) {
                CheckDlgButton( hDlg, ID_KD_VERBOSE, 1);
            }

            if (KdParams->fInitialBp) {
                CheckDlgButton( hDlg, ID_KD_INITIALBP, 1);
            }

            if (KdParams->fDefer) {
                CheckDlgButton( hDlg, ID_KD_DEFER, 1);
            }

            if (KdParams->fUseModem) {
                CheckDlgButton( hDlg, ID_KD_MODEM, 1);
            }

            hCtl = GetDlgItem(hDlg,ID_KD_PORT);
            sprintf( rgch, "COM%d", KdParams->dwPort );
            for (i=0; i<KdMaxComPorts; i++) {
                SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)KdComPorts[i]);
                if (strcmp( rgch, KdComPorts[i] ) == 0) {
                    SendMessage(hCtl, CB_SETCURSEL, i, 0);
                }
            }

            hCtl = GetDlgItem(hDlg,ID_KD_BAUDRATE);
            for (i=0; i<KdMaxBaudRates; i++) {
                sprintf( rgch, "%d", KdBaudRates[i] );
                SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)rgch);
                if (KdParams->dwBaudRate ==  KdBaudRates[i] ) {
                    SendMessage(hCtl, CB_SETCURSEL, i, 0);
                }
            }

            hCtl = GetDlgItem(hDlg,ID_KD_CACHE);
            for (i=0; i<KdMaxCacheSizes; i++) {
                sprintf( rgch, "%d", KdCacheSizes[i] );
                SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)rgch);
                if (KdParams->dwCache ==  KdCacheSizes[i] ) {
                    SendMessage(hCtl, CB_SETCURSEL, i, 0);
                }
            }

            hCtl = GetDlgItem(hDlg,ID_KD_PLATFORM);
            for (i=0; i<KdMaxPlatforms; i++) {
                SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)KdPlatforms[i]);
                if (KdParams->dwPlatform == (DWORD)i) {
                    SendMessage(hCtl, CB_SETCURSEL, i, 0);
                }
            }

            return TRUE;

        case WM_COMMAND:
            switch (wParam) {
                case IDOK :

                    /*
                     * Transfer the options to global
                     *
                     * Now start looking at the rest and determine what needs to
                     * be updated
                     */

                    b = (IsDlgButtonChecked(hDlg,ID_KD_ENABLE) != 0);
                    if (b != KdParams->fEnable) {
                        KdParams->fEnable = b;
                        fChange = TRUE;
                    }

                    b = (IsDlgButtonChecked(hDlg,ID_KD_GOEXIT) != 0);
                    if (b != KdParams->fGoExit) {
                        KdParams->fGoExit = b;
                        fChange = TRUE;
                    }

                    b = (IsDlgButtonChecked(hDlg,ID_KD_VERBOSE) != 0);
                    if (b != KdParams->fVerbose) {
                        KdParams->fVerbose = b;
                        fChange = TRUE;
                    }

                    b = (IsDlgButtonChecked(hDlg,ID_KD_INITIALBP) != 0);
                    if (b != KdParams->fInitialBp) {
                        KdParams->fInitialBp = b;
                        fChange = TRUE;
                    }

                    b = (IsDlgButtonChecked(hDlg,ID_KD_DEFER) != 0);
                    if (b != KdParams->fDefer) {
                        KdParams->fDefer = b;
                        fChange = TRUE;
                    }

                    b = (IsDlgButtonChecked(hDlg,ID_KD_MODEM) != 0);
                    if (b != KdParams->fUseModem) {
                        KdParams->fUseModem = b;
                        fChange = TRUE;
                    }

                    if (GetDlgItemText( hDlg, ID_KD_PORT, rgch, sizeof(rgch) )) {
                        for (i=0; i<KdMaxComPorts; i++) {
                            if (strcmp(KdComPorts[i],rgch)==0) {
                                KdParams->dwPort = i+1;
                                fChange = TRUE;
                                break;
                            }
                        }
                    }

                    if (GetDlgItemText( hDlg, ID_KD_BAUDRATE, rgch, sizeof(rgch) )) {
                        tmp = atol( rgch );
                        for (i=0; i<KdMaxBaudRates; i++) {
                            if (KdBaudRates[i] == tmp) {
                                KdParams->dwBaudRate = tmp;
                                fChange = TRUE;
                                break;
                            }
                        }
                    }

                    if (GetDlgItemText( hDlg, ID_KD_CACHE, rgch, sizeof(rgch) )) {
                        tmp = atol( rgch );
                        for (i=0; i<KdMaxCacheSizes; i++) {
                            if (KdCacheSizes[i] == tmp) {
                                KdParams->dwCache = tmp;
                                fChange = TRUE;
                                break;
                            }
                        }
                    }

                    if (GetDlgItemText( hDlg, ID_KD_PLATFORM, rgch, sizeof(rgch) )) {
                        for (i=0; i<KdMaxPlatforms; i++) {
                            if (strcmp(KdPlatforms[i],rgch)==0) {
                                KdParams->dwPlatform = i;
                                fChange = TRUE;
                                break;
                            }
                        }
                    }

                    EndDialog(hDlg, TRUE);
                    return (TRUE);

                case IDCANCEL:
                    EndDialog(hDlg, TRUE);
                    return (TRUE);

#if 0
                case IDHELP:                /* User Help */
                    Dbg(WinHelp(hDlg, szHelpFileName, (DWORD) HELP_CONTEXT,(DWORD)ID_DBUGOPT_HELP));
                    return (TRUE);
#endif

                default:
                    break;
            }
            break;
    }
    return FALSE;
}
