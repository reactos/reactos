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


#include "include\cntxthlp.h"

typedef struct _KDBAUDRATE {
    DWORD   dwBaudMask;
    DWORD   dwBaudRate;
} KDBAUDRATE, *LPKDBAUDRATE;

typedef struct _COMPORTINFO {
    CHAR    szSymName[16];
    DWORD   dwNum;
    DWORD   dwSettableBaud;
} COMPORT_INFO, *LPCOMPORT_INFO;

COMPORT_INFO   ComPortInfo[100];
DWORD         MaxComPorts;
DWORD         CurComPort;

DWORD KdCacheSizes[] = { 102400, 512000, 1024000, 1024000};

KDBAUDRATE KdBaudRates[] =
    {
    BAUD_075,      75,
    BAUD_110,      110,
    BAUD_150,      150,
    BAUD_300,      300,
    BAUD_600,      600,
    BAUD_1200,     1200,
    BAUD_1800,     1800,
    BAUD_2400,     2400,
    BAUD_4800,     4800,
    BAUD_7200,     7200,
    BAUD_9600,     9600,
    BAUD_14400,    14400,
    BAUD_19200,    19200,
    BAUD_38400,    38400,
    BAUD_56K,      56000,
    BAUD_57600,    57600,
    BAUD_128K,     128000,
    BAUD_115200,   115200,
    };

// Processor identification structures and functions
typedef struct _KDPLATFORMS {
    MPT     mpt;
    LPSTR   lpszName;
} KDPLATFORMS;

KDPLATFORMS KdPlatforms[] = {
    { mptix86,    "X86",  },
    //{ mptmips,    "MIPS" },
    { mptdaxp,    "ALPHA" },
    //{ mptntppc,   "PPC" }
    { mptia64,    "IA64" },
};

#define KdMaxPlatforms  (sizeof(KdPlatforms)/sizeof(KDPLATFORMS))
#define KdMaxBaudRates  (sizeof(KdBaudRates)/sizeof(KDBAUDRATE))
#define KdMaxCacheSizes (sizeof(KdCacheSizes)/sizeof(DWORD))


LPCSTR
GetPlatformNameFromId(
    MPT mpt
    )
/*++

Routine Description

    Given the enum type it will return a constant pointer
    to the name of the platform. The text referenced by the
    pointer must not be changed.

Arguments

    mpt - enum type of processors

Return Value

    Success - Pointer to the text contained in KdPlatforms structure. DO NOT MODIFY.
    Failure - NULL.

--*/
{
    int i;

    for (i=0; i<KdMaxPlatforms; i++) {
        if (KdPlatforms[i].mpt == mpt) {
            // success
            return KdPlatforms[i].lpszName;
        }
    }

    // failure
    return "";
}


BOOL
GetPlatformIdFromName(
    LPSTR lpszPlatformName,
    MPT * pmpt
    )
/*++

Routine Description

    Given the platform name will return the enum type.

Arguments

    lpszPlatformName - Name of the platform you want the enum of.

    pmpt - Enum type of the processor.
            If the requested processor was not found the value pointed to
            by pnId is not changed.

Return Value

    TRUE - if found.
    FALSE - not found

--*/
{
    int i;

    Assert(pmpt);

    for (i=0; i<KdMaxPlatforms; i++) {
        if (!_stricmp(lpszPlatformName, KdPlatforms[i].lpszName)) {
            *pmpt = KdPlatforms[i].mpt;
            return TRUE;
        }
    }

    return FALSE; // Not found
}

MPT
GetPlatformIdFromArrayPos(
    int nArrayPos
    )
{
    Assert(0 <= nArrayPos && nArrayPos < KdMaxPlatforms);

    return KdPlatforms[nArrayPos].mpt;
}


DWORD
GetComPorts(
    VOID
    )
{
    FILETIME        ft;
    HKEY            hkey;
    CHAR            rgch[256];
    CHAR            szValueName[256];
    CHAR            szValueData[256];
    DWORD           i;
    DWORD           k = 0;
    DWORD           PortNum;
    DWORD           dwSz1, dwSz2;
    DWORD           NumComPorts = 0;
    DWORD           dwType;
    HANDLE          hCommDev;
    COMMPROP        cmmp;


    if (RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                      "Hardware\\DeviceMap\\SerialComm",
                      0,
                      KEY_READ,
                      &hkey ) != ERROR_SUCCESS ) {
        return FALSE;
    }

    i = sizeof(rgch);
    if (RegQueryInfoKey( hkey, rgch, &i, NULL, &i, &i, &i,
                         &NumComPorts, &i, &i, &i, &ft ) != ERROR_SUCCESS) {
        RegCloseKey( hkey );
        return 0;
    }

    if (NumComPorts) {
        k = 0;
        i = 0;
        while( TRUE ) {
            dwSz1 = sizeof(szValueName);
            dwSz2 = sizeof(szValueData);
            if (RegEnumValue( hkey, i, szValueName, &dwSz1, NULL, &dwType,
                              (PUCHAR) szValueData, &dwSz2 ) != ERROR_SUCCESS) {
                break;
            }

            sprintf( rgch, "\\\\.\\%s", szValueData );
            hCommDev = CreateFile(
                             rgch,
                             GENERIC_READ | GENERIC_WRITE,
                             0,
                             NULL,
                             OPEN_ALWAYS,
                             FILE_ATTRIBUTE_NORMAL,
                             NULL
                             );

            PortNum = strtoul(&szValueData[3], NULL, 0);

            if ((hCommDev != INVALID_HANDLE_VALUE) ||
                (hCommDev == INVALID_HANDLE_VALUE &&
                 g_contWorkspace_WkSp.m_bKernelDebugger && DebuggeeActive() &&
                 PortNum == g_contKernelDbgPreferences_WkSp.m_dwPort)) {
                strcpy( ComPortInfo[k].szSymName, szValueData );
                ComPortInfo[k].dwNum = PortNum;
                if (hCommDev != INVALID_HANDLE_VALUE) {
                    GetCommProperties( hCommDev, &cmmp );
                    ComPortInfo[k].dwSettableBaud = cmmp.dwSettableBaud;
                    CloseHandle( hCommDev );
                }
                k++;
            }
            i++;
        }
    }

    RegCloseKey( hkey );

    return k;
}


VOID
KrnlDbg_SetupControls(
    HWND hDlg
    )
{
    CHAR            rgch[256];
    DWORD           i;
    DWORD           j;
    HWND            hCtl;


    CheckDlgButton( hDlg, ID_KD_ENABLE,    g_contWorkspace_WkSp.m_bKernelDebugger );
    CheckDlgButton( hDlg, ID_KD_GOEXIT,    g_contKernelDbgPreferences_WkSp.m_bGoExit );
    CheckDlgButton( hDlg, ID_KD_INITIALBP, g_contKernelDbgPreferences_WkSp.m_bInitialBp );

    hCtl = GetDlgItem(hDlg,ID_KD_PORT);
    Assert(hCtl);
    SendMessage(hCtl, CB_RESETCONTENT, 0, 0);
    for (i=0,j=0; i<MaxComPorts; i++) {
        SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)ComPortInfo[i].szSymName);
        if (ComPortInfo[i].dwNum == g_contKernelDbgPreferences_WkSp.m_dwPort) {
            CurComPort = i;
        }
    }
    SendMessage(hCtl, CB_SETCURSEL, CurComPort, 0);

    hCtl = GetDlgItem(hDlg,ID_KD_BAUDRATE);
    Assert(hCtl);
    SendMessage(hCtl, CB_RESETCONTENT, 0, 0);
    for (i=0,j=0; i<KdMaxBaudRates; i++) {
        if (KdBaudRates[i].dwBaudMask & ComPortInfo[CurComPort].dwSettableBaud) {
            sprintf(rgch,"%d",KdBaudRates[i].dwBaudRate);
            SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)rgch);
            j++;
        }
        if (g_contKernelDbgPreferences_WkSp.m_dwBaudRate == KdBaudRates[i].dwBaudRate) {
            SendMessage(hCtl, CB_SETCURSEL, j-1, 0);
        }
    }

    hCtl = GetDlgItem(hDlg,ID_KD_CACHE);
    Assert(hCtl);
    SendMessage(hCtl, CB_RESETCONTENT, 0, 0);
    for (i=0; i<KdMaxCacheSizes; i++) {
        sprintf( rgch, "%d", KdCacheSizes[i] );
        SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM)rgch);
        if (g_contKernelDbgPreferences_WkSp.m_dwCache ==  KdCacheSizes[i] ) {
            SendMessage(hCtl, CB_SETCURSEL, i, 0);
        }
    }

    hCtl = GetDlgItem(hDlg,ID_KD_PLATFORM);
    Assert(hCtl);
    SendMessage(hCtl, CB_RESETCONTENT, 0, 0);
    for (i=0; i<KdMaxPlatforms; i++) {
        MPT mpt = GetPlatformIdFromArrayPos(i);

        LPCSTR lpsz = GetPlatformNameFromId(mpt);
        SendMessage(hCtl, CB_ADDSTRING, 0, (LPARAM) lpsz);
        if (g_contKernelDbgPreferences_WkSp.m_mptPlatform == mpt) {
            SendMessage(hCtl, CB_SETCURSEL, i, 0);
        }
    }
}

INT_PTR
CALLBACK
DlgProc_KernelDebugger(
    HWND hwndDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static DWORD HelpArray[]=
    {
       ID_KD_INITIALBP, IDH_BP1,
       ID_KD_ENABLE, IDH_KD,
       ID_KD_GOEXIT, IDH_GEXIT,
       ID_KD_BAUDRATE_LABEL, IDH_BAUD,
       ID_KD_BAUDRATE, IDH_BAUD,
       ID_KD_PORT_LABEL, IDH_PORT,
       ID_KD_PORT, IDH_PORT,
       ID_KD_CACHE_LABEL, IDH_CACHE,
       ID_KD_CACHE, IDH_CACHE,
       ID_KD_PLATFORM_LABEL, IDH_PLATFORM,
       ID_KD_PLATFORM, IDH_PLATFORM,
       0, 0
    };

    switch (uMsg) {
    case WM_INITDIALOG:
        //
        // Set up the controls to reflect current values
        //
        MaxComPorts = GetComPorts();
        KrnlDbg_SetupControls( hwndDlg );
        return TRUE;


      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_NOTIFY:
        switch (((NMHDR FAR *) lParam)->code) {
        case PSN_APPLY:
            {
                BOOL            bEnabled = FALSE;
                int             i;
                char            szMisc[MAX_PATH];
                PIOCTLGENERIC   pig;

                //
                // Transfer the options to global
                //
                // Now start looking at the rest and determine what needs to
                // be updated
                //

                bEnabled = (IsDlgButtonChecked(hwndDlg,ID_KD_ENABLE) != 0);
                if (bEnabled != g_contWorkspace_WkSp.m_bKernelDebugger) {
                    g_contWorkspace_WkSp.m_bKernelDebugger = bEnabled;
                    if (bEnabled) {
                        g_Windbg_WkSp.SetCurrentProgramName(NT_KERNEL_NAME);
                    } else {
                        g_Windbg_WkSp.SetCurrentProgramName(g_Windbg_WkSp.m_pszNoProgramLoaded);
                    }
                }

                bEnabled = (IsDlgButtonChecked(hwndDlg,ID_KD_GOEXIT) != 0);
                if (bEnabled != g_contKernelDbgPreferences_WkSp.m_bGoExit) {
                    g_contKernelDbgPreferences_WkSp.m_bGoExit = bEnabled;
                }

                bEnabled = (IsDlgButtonChecked(hwndDlg,ID_KD_INITIALBP) != 0);
                if (bEnabled != g_contKernelDbgPreferences_WkSp.m_bInitialBp) {
                    g_contKernelDbgPreferences_WkSp.m_bInitialBp = bEnabled;
                }

                i = (int) SendDlgItemMessage(hwndDlg, ID_KD_PORT, CB_GETCURSEL, 0, 0);
                g_contKernelDbgPreferences_WkSp.m_dwPort = ComPortInfo[i].dwNum;

                i = (int) SendDlgItemMessage(hwndDlg, ID_KD_BAUDRATE, CB_GETCURSEL, 0, 0);
                SendDlgItemMessage(hwndDlg, ID_KD_BAUDRATE, CB_GETLBTEXT, i, (LPARAM)szMisc);
                g_contKernelDbgPreferences_WkSp.m_dwBaudRate = strtoul(szMisc, NULL, 0);

                i = (int) SendDlgItemMessage(hwndDlg, ID_KD_CACHE, CB_GETCURSEL, 0, 0);
                g_contKernelDbgPreferences_WkSp.m_dwCache = KdCacheSizes[i];

                GetDlgItemText(hwndDlg, ID_KD_PLATFORM, szMisc, sizeof(szMisc));
                Dbg(GetPlatformIdFromName(szMisc, &(g_contKernelDbgPreferences_WkSp.m_mptPlatform) ));

                if (LptdCur && DebuggeeAlive()) {
                    DWORD dwRet;

                    FormatKdParams( szMisc );
                    pig = (PIOCTLGENERIC) malloc( strlen(szMisc) + 1 + sizeof(IOCTLGENERIC) );
                    if (!pig) {
                        return FALSE;
                    }
                    pig->ioctlSubType = IG_DM_PARAMS;
                    pig->length = strlen(szMisc) + 1;
                    strcpy((LPSTR)pig->data,szMisc);
                    OSDSystemService( LppdCur->hpid,
                        LptdCur->htid,
                        (SSVC) ssvcGeneric,
                        (LPV)pig,
                        strlen(szMisc) + 1 + sizeof(IOCTLGENERIC),
                        &dwRet
                        );
                    free( pig );
                }

                EnableToolbarControls();
            }
            return TRUE;
        }
        break;
    }

    return FALSE;
}

