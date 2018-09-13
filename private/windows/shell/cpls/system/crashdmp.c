/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    crashdmp.c

Abstract:

    Implements the "Recovery" group on the Startup/Recovery 
    dialog of the System Control Panel Applet

Notes:

    The virtual memory settings and the crash dump (core dump) settings
    are tightly-coupled.  Therefore, crashdmp.c and startup.h have some
    heavy dependencies on virtual.c and virtual.h (and vice versa).

Author:

    Byron Dazey 06-Jun-1992

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul
        
    07-July-1998 jsenior
        Added a new button which implements Kernel only dumping functionality
        within the registy.

--*/
#include "sysdm.h"

/*
 * Core Dump vars
 */
BOOL gfCoreDumpChanged;
VCREG_RET gvcCrashCtrl =  VCREG_ERROR;
HKEY ghkeyCrashCtrl = NULL;
int  gcrefCrashCtrl = 0;

TCHAR szCrashControl[] =
    TEXT("System\\CurrentControlSet\\Control\\CrashControl");

TCHAR szDefDumpFile[MAX_PATH] = TEXT("%SYSTEMROOT%\\MEMORY.DMP");

COREDMP_FIELD acdfControls[] = {
    { IDC_STARTUP_CDMP_LOG,         TEXT("LogEvent"),   REG_DWORD, TRUE },
    { IDC_STARTUP_CDMP_SEND,        TEXT("SendAlert"),  REG_DWORD, TRUE },
    { IDC_STARTUP_CDMP_WRITE,       TEXT("CrashDumpEnabled"),  REG_DWORD, TRUE },
    { IDC_STARTUP_CDMP_OVERWRITE,   TEXT("Overwrite"),  REG_DWORD, TRUE },
    { IDC_STARTUP_CDMP_KERNELONLY,  TEXT("KernelDumpOnly"),    REG_DWORD, TRUE },
    { IDC_STARTUP_CDMP_AUTOREBOOT,  TEXT("AutoReboot"), REG_DWORD, TRUE },
    { IDC_STARTUP_CDMP_FILENAME,    TEXT("DumpFile"),   REG_EXPAND_SZ, (BOOL)NULL},
};

#define CCTL_COREDUMP   (sizeof(acdfControls) / sizeof(COREDMP_FIELD))

VCREG_RET CoreDumpOpenKey( void ) {

    DOUT("In CoreDumpOpenKey");

    if (gvcCrashCtrl == VCREG_ERROR) {
        gvcCrashCtrl = OpenRegKey( szCrashControl, &ghkeyCrashCtrl );
    }

    if (gvcCrashCtrl != VCREG_ERROR)
        gcrefCrashCtrl++;

    DPRINTF((TEXT("SYSCPL: In CoreDumpOpenKey, cref=%d\n"), gcrefCrashCtrl));

    return gvcCrashCtrl;
}

//
// Private function prototypes
//
static 
BOOL 
CoreDumpInit(
    IN HWND hDlg
);

static 
BOOL 
CoreDumpUpdateRegistry(
    IN HWND hDlg
);

int 
CoreDumpHandleOk(
    IN BOOL fInitialized, 
    IN HWND hDlg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
);


//======================================================================
// Implementation

void CoreDumpCloseKey(void) {

    DOUT("In CoreDumpCloseKey");

    if (gcrefCrashCtrl > 0) {
        gcrefCrashCtrl--;
        if (gcrefCrashCtrl == 0) {
            CloseRegKey( ghkeyCrashCtrl );
            gvcCrashCtrl = VCREG_ERROR;
        }
    }


    DPRINTF((TEXT("SYSCPL: In CoreDumpCloseKey, cref=%d\n"), gcrefCrashCtrl));
}




BOOL CoreDumpValidFile( HWND hDlg ) {
    TCHAR szPath[MAX_PATH];
    TCHAR szExpPath[MAX_PATH];
    LPTSTR psz;
    TCHAR ch;
    UINT uType;

    if (IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_WRITE)) {
        /*
         * get the filename
         */
        if( GetDlgItemText(hDlg, IDC_STARTUP_CDMP_FILENAME, szPath,
                ARRAYSIZE(szPath)) == 0) {

            MsgBoxParam(hDlg, SYSTEM+30, INITS+1, MB_ICONSTOP | MB_OK);
            return FALSE;
        }

        /*
         * Expand any environment vars, and then check to make sure it
         * is a fully quallified path
         */
        // if it has a '%' in it, then try to expand it
        if (ExpandEnvironmentStrings(szPath, szExpPath, ARRAYSIZE(szExpPath)) >= ARRAYSIZE(szExpPath)) {
            MsgBoxParam(hDlg, SYSTEM+33, INITS+1, MB_ICONSTOP | MB_OK,
                    (DWORD)MAX_PATH);
            return FALSE;
        }

        // now cannonicalize it
        GetFullPathName( szExpPath, ARRAYSIZE(szPath), szPath, &psz );

        // check to see that it already was cannonicalized
        if (lstrcmp( szPath, szExpPath ) != 0) {
            MsgBoxParam(hDlg, SYSTEM+34, INITS+1, MB_ICONSTOP | MB_OK );
            return FALSE;
        }

        /*
         * check the drive (don't allow remote)
         */
        ch = szPath[3];
        szPath[3] = TEXT('\0');
        if (IsPathSep(szPath[0]) || ((uType = GetDriveType(szPath)) !=
                DRIVE_FIXED && uType != DRIVE_REMOVABLE)) {
            MsgBoxParam(hDlg, SYSTEM+31, INITS+1, MB_ICONSTOP | MB_OK );
            return FALSE;
        }
        szPath[3] = ch;

        /*
         * if path is non-exstant, tell user and let him decide what to do
         */
        if (GetFileAttributes(szPath) == 0xFFFFFFFFL && GetLastError() !=
            ERROR_FILE_NOT_FOUND && MsgBoxParam(hDlg, SYSTEM+32, INITS+1,
                MB_ICONQUESTION | MB_YESNO ) == IDYES) {
            return FALSE;
        }
    }

    return TRUE;
}


/*****************************************************************\
 *  // SAVE THIS COMMENT UNTIL CAIRO
 *
 * ********* From KeithMo ***********************************
 * The Server Manager does this. Unfortunately (for you) it's all in C++.
 *
 * Look at the following APIs:
 *
 *         OpenSCManager
 *         OpenService
 *         StartService
 *         ControlService
 *         QueryServiceStatus
 *
 * Basically, you call OpenSCManager to get a handle to the Service
 * Controller.  Then, using that handle, you call OpenService to get a
 * handle to the actual target service.  Using that service handle, you can
 * call QueryServiceStatus to determine if it's running.  If not, call
 * StartService to initiate the start.  After you initiate the start, you
 * need to poll periodically with QueryServiceStatus until the service
 * either starts or fails.
 *
 * If you want to change a service so that it autostarts at system boot,
 * use the ChangeServiceConfig API.
 *
 * One thing to be aware of: When you query the service status, one of the
 * things you get back is a checkpoint and a wait hint.  In theory, the
 * checkpoint should be updated at least once within the period of the wait
 * hint.  It's been my experience that so me services don't provide very
 * accurate wait hints, so the Server Manager takes the wait hints and
 * multiplies them by 5 (I think) just to be safe.  The worst that can
 * happen is that your app will take a bit longer to timeout for
 * ill-behaved services.
 *
\*****************************************************************/

BOOL IsAlerterSvcStarted(HWND hDlg) {
    SC_HANDLE schSCManager, schService;
    LPQUERY_SERVICE_CONFIG lpqscBuf;
    DWORD dwBytesNeeded;
    BOOL fRunning = FALSE;
    SERVICE_STATUS ssSrvcStat;


    /*
     * Open the Service Controller
     */
    schSCManager = OpenSCManager(
         NULL,                   /* local machine           */
         NULL,                   /* ServicesActive database */
         SC_MANAGER_ALL_ACCESS); /* full access rights      */

    if (schSCManager == NULL) {
        goto iassExit;
    }


    /*
     * Try to open the Alerter Service
     */


    /* Open a handle to the service. */

    schService = OpenService(
         schSCManager,           /* SCManager database  */
         TEXT("Alerter"),        /* name of service     */
         SERVICE_QUERY_STATUS);  /* need QUERY access   */

    if (schService == NULL) {
        goto iassExit;
    }

    /*
     * Query the Alerter service to see if it has been started
     */

    if (!QueryServiceStatus(schService, &ssSrvcStat )) {
        goto iassExit;
    }


    if (ssSrvcStat.dwCurrentState != SERVICE_RUNNING) {
        /*
         * LATER - in cairo time frame (for more info, see comment above)
         *
         *  Start the service
         *
         *  Wait for the service to start
         *
         *  Save the service state so it starts on reboot
         */
        fRunning = FALSE;
    } else {

        fRunning = TRUE;
    }


iassExit:
    if (!fRunning) {
        MsgBoxParam(hDlg, SYSTEM+38, INITS+1, MB_ICONEXCLAMATION );
    }

    if (schService != NULL) {
        CloseServiceHandle(schService);
    }

    if (schSCManager != NULL) {
        CloseServiceHandle(schService);
    }

    return fRunning;
}

/*
 * CoreDumpDlg
 *
 *
 *
 */

int APIENTRY CoreDumpDlgProc( HWND hDlg, UINT message, DWORD wParam, LONG lParam ) {
    static BOOL fInitialized = FALSE;

    switch (message)
    {
    case WM_INITDIALOG:
        g_fStartupInitializing = TRUE;
        fInitialized = CoreDumpInit(hDlg);
        g_fStartupInitializing = FALSE;
        return RET_CONTINUE;
        break;

    case WM_COMMAND:

        switch (LOWORD(wParam)) {
            case IDOK: 
                return(CoreDumpHandleOk(fInitialized, hDlg, wParam, lParam));
                break;

            case IDCANCEL:
                if (fInitialized) {
                    VirtualFreePageFiles(apf);
                    VirtualCloseKey();
                    CoreDumpCloseKey();
                }
                // Let the Startup/Recovery dlg proc also handle IDOK
                return(RET_NO_CHANGE);
                break;

            case IDC_STARTUP_CDMP_WRITE: {
                BOOL fChecked = IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_WRITE);
                EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME), fChecked);
                EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OVERWRITE), fChecked);
                EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_KERNELONLY), fChecked);
                break;
            }
            default: {
                // indicat not handled
                return RET_CONTINUE;
            }
        }
        break;

    case WM_DESTROY:
        { int i;
            for( i = 0; i < CCTL_COREDUMP; i++ ) {
                switch (acdfControls[i].dwType ) {
                case REG_SZ:
                case REG_EXPAND_SZ:
                case REG_MULTI_SZ:
                    if (acdfControls[i].u.pszValue != NULL)
                        MemFree(acdfControls[i].u.pszValue);

                    break;

                default:
                    break;
                }
            }
        }
        return RET_CONTINUE;
        break;

    default:
        return RET_CONTINUE;
    }

    return RET_BREAK;
}

int
CoreDumpHandleOk(
    IN BOOL fInitialized,
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
)
{
    BOOL fRegChg;
    INT cMegBootPF;
    TCHAR szBootPath[MAX_PATH];
    int iBootDrive;
    int iRet = RET_NO_CHANGE;

    if (fInitialized) {

        /*
         * Validate core dump filename
         */
        if (!CoreDumpValidFile(hDlg)) {
            SetFocus(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME));
            SetWindowLong (hDlg, DWL_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            iRet = RET_ERROR;
            return(iRet);
        }

        /*
         * If we are to do anything, then we need a pagefile on the boot
         * drive.
         *
         * If we are to write the dump file, it must be >= sizeof
         * phyical memory.
         */
        cMegBootPF = 0;

        if (IsDlgButtonChecked(hDlg, acdfControls[CD_WRITE].idCtl)) {
            cMegBootPF = -1;
        } else if (IsDlgButtonChecked(hDlg, acdfControls[CD_LOG].idCtl) ||
                IsDlgButtonChecked(hDlg, acdfControls[CD_SEND].idCtl)) {
            cMegBootPF = MIN_SWAPSIZE;
        }

        if (cMegBootPF != 0) {
            if (GetWindowsDirectory(szBootPath, MAX_PATH) == 0) {
                iBootDrive = IDRV_DEF_BOOT;
            } else {
                iBootDrive = szBootPath[0];

                if (iBootDrive > TEXT('Z'))
                    iBootDrive -= TEXT('a');
                else {
                    iBootDrive -= TEXT('A');
                }
            }

            /*
             * We need to compute cMegBootPF so it includes all RAM plus
             * one page
             */
            if (cMegBootPF == -1) {
                SYSTEM_BASIC_INFORMATION BasicInfo;
                NTSTATUS Status;
                DWORD dwTotalPhys;

                // Get the number of Meg in the system
                Status = NtQuerySystemInformation(
                    SystemBasicInformation,
                    &BasicInfo,
                    sizeof(BasicInfo),
                    NULL
                );

                if (!NT_SUCCESS(Status)) {
                    dwTotalPhys = 0;
                } /* if */
                else {
                    dwTotalPhys = BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
                } /* else */

                cMegBootPF = (dwTotalPhys / ONE_MEG) + 1;

            }

            /*
             * Check to see if the boot drive page file is big enough
             */
            if (apf[iBootDrive].nMinFileSize < cMegBootPF) {

                //We got the OK... Try to make it bigger
                if (cMegBootPF >= GetFreeSpaceMB(iBootDrive) ) {
                    MsgBoxParam(hDlg, SYSTEM+27, INITS+1,
                            MB_ICONEXCLAMATION, cMegBootPF,
                            (TCHAR)(TEXT('A') + iBootDrive));
                    iRet = RET_ERROR;
                    return(iRet);
                }

                //Its too small, ask if we can make it bigger
                if (MsgBoxParam(hDlg, SYSTEM+28, INITS+1,
                        MB_ICONEXCLAMATION | MB_OKCANCEL,
                        (TCHAR)(iBootDrive+TEXT('A')), cMegBootPF) == IDOK){

                    apf[iBootDrive].nMinFileSize = cMegBootPF;

                    if ( apf[iBootDrive].nMaxFileSize <
                                apf[iBootDrive].nMinFileSize) {
                        apf[iBootDrive].nMaxFileSize =
                                apf[iBootDrive].nMinFileSize;
                    }
                    VirtualMemUpdateRegistry();
                    iRet = VirtualMemPromptForReboot(hDlg);
                } else {
                    iRet = RET_ERROR;
                    return(iRet);
                }
            }
        }


        /*
         * If the Alert button is checked, make sure the alerter service
         * is started.
         */
        if (IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_SEND)) {
            IsAlerterSvcStarted(hDlg);
        }


        fRegChg = CoreDumpUpdateRegistry(hDlg);

        //
        // Clean up registry stuff
        //
        CoreDumpCloseKey();
        VirtualFreePageFiles(apf);
        VirtualCloseKey();

        if (fRegChg) {
            iRet = RET_RECOVER_CHANGE;
        }
    } else {
        iRet = RET_NO_CHANGE;
    }

    return(iRet);

}


void CoreDumpGetValue(int i) {
    DWORD dwType, dwTemp, cbTemp;
    LPBYTE lpbTemp;
    TCHAR szTemp[MAX_PATH];
    TCHAR szTemp2[MAX_PATH];

    switch( acdfControls[i].dwType ) {
    case REG_DWORD:
        cbTemp = sizeof(dwTemp);
        lpbTemp = (LPBYTE)&dwTemp;
        break;

    default:
        szTemp[0] = 0;
        cbTemp = sizeof(szTemp);
        lpbTemp = (LPBYTE)szTemp;
        break;
    }

    if (RegQueryValueEx (ghkeyCrashCtrl, acdfControls[i].pszValueName, NULL,
            &dwType, lpbTemp, &cbTemp) == ERROR_SUCCESS) {
        /*
         * Copy the reg data into the array
         */
        switch( acdfControls[i].dwType) {
        case REG_DWORD:
            acdfControls[i].u.fValue = (dwTemp == 0) ? FALSE : TRUE;
            break;
            
        default:
            ExpandEnvironmentStrings(szTemp, szTemp2, MAX_PATH);
            acdfControls[i].u.pszValue = CloneString(szTemp2);
            break;
        }
    } else {
        /* no reg data, use default values */
        switch( acdfControls[i].dwType) {
        case REG_DWORD:
            if (acdfControls[i].idCtl == IDC_STARTUP_CDMP_KERNELONLY) {
                // This is for the case where there is no KERNELONLY value
                // under the key.  This will be until the kernel only option
                // is changed so that the value is created.  Currently, the 
                // default is for kernel only to be turned off by default. If
                // this default is ever changed, however, this code will 
                // safeguard against a bug. To make more robust, have the 
                // setup guys create the KERNELDUMPONLY only key and set it
                // according to the default configuration.
                int j;
                for (j = 0; j < CCTL_COREDUMP; j++ ) {
                    if (acdfControls[j].idCtl == IDC_STARTUP_CDMP_WRITE) {
                        break;
                    }
                }
                if (RegQueryValueEx (ghkeyCrashCtrl, acdfControls[j].pszValueName, NULL,
                    &dwType, lpbTemp, &cbTemp) == ERROR_SUCCESS) {
                    if (2 == dwTemp) {
                        acdfControls[i].u.fValue = TRUE;
                    } else {
                        acdfControls[i].u.fValue = FALSE;
                    }
                } else {
                    acdfControls[i].u.fValue = FALSE;
                }
            } else {
                acdfControls[i].u.fValue = FALSE;
            }
            break;

        default:
            LoadString (hInstance, IDS_DUMPFILE, szDefDumpFile, ARRAYSIZE(szDefDumpFile));
            ExpandEnvironmentStrings(szDefDumpFile, szTemp2, MAX_PATH);
            acdfControls[i].u.pszValue = CloneString(szTemp2);
            break;
        }
    }
}


BOOL CoreDumpPutValue(int i) {
    LPBYTE lpb;
    DWORD cb;
    DWORD dwTmp;
    BOOL fErr = FALSE;

    switch(acdfControls[i].dwType) {
    case REG_DWORD:
        if (acdfControls[i].idCtl == IDC_STARTUP_CDMP_WRITE){
            //
            // Check first if writing a dump file is enabled
            if (acdfControls[i].u.fValue != TRUE) 
                dwTmp = 0;
            else {
                //
                // Now check what type of dump file is to be written
                // Is it a full dump or just a summary (kernel only) dump
                int j;
                for (j = 0; j < CCTL_COREDUMP; j++ ) {
                    if (acdfControls[j].idCtl == IDC_STARTUP_CDMP_KERNELONLY) {
                        break;
                    }
                }
                if (acdfControls[j].u.fValue != TRUE)
                    dwTmp = 1;
                else
                    dwTmp = 2;
            }
        } else {
            if (acdfControls[i].idCtl == IDC_STARTUP_CDMP_KERNELONLY) {
                //
                // This was implemented for the case where kernel debugging
                // was enabled, but the Write checkbox didn't change.  We
                // have to update the CrashDumpEnabled value, as it is 
                // affected by the "Kernel Only" checkbox
                int w;
                DWORD dwTmp2;
                for (w = 0; w < CCTL_COREDUMP; w++ ) {
                    if (acdfControls[w].idCtl == IDC_STARTUP_CDMP_WRITE) {
                        break;
                    }
                }
                if (acdfControls[w].u.fValue != TRUE) 
                    dwTmp2 = 0;
                else {
                    //
                    // Now check what type of dump file is to be written
                    // Is it a full dump or just a summary (kernel only) dump
                    if (acdfControls[i].u.fValue != TRUE)
                        dwTmp2 = 1;
                    else
                        dwTmp2 = 2;
                }
                lpb = (LPBYTE)&dwTmp2;
                cb = sizeof(dwTmp);
                if (RegSetValueEx (ghkeyCrashCtrl, acdfControls[w].pszValueName, 0,
                        acdfControls[w].dwType, lpb, cb) != ERROR_SUCCESS) {
                    fErr = TRUE;
                }
            }
            dwTmp = (DWORD)acdfControls[i].u.fValue;
        }
        lpb = (LPBYTE)&dwTmp;
        cb = sizeof(dwTmp);
        break;

    default:

        lpb = (LPBYTE)(acdfControls[i].u.pszValue);
        cb = (lstrlen((LPTSTR)lpb) + 1) * sizeof(TCHAR);
        break;
    }

    if (RegSetValueEx (ghkeyCrashCtrl, acdfControls[i].pszValueName, 0,
            acdfControls[i].dwType, lpb, cb) != ERROR_SUCCESS) {
        fErr = TRUE;
    }

    return fErr;
}

void DisableCoreDumpControls(HWND hDlg) {
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_GRP     ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_TXT1    ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_LOG     ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_SEND    ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_WRITE   ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_KERNELONLY	   ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OVERWRITE       ), FALSE);
    EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_AUTOREBOOT      ), FALSE);
}

void CoreDumpInitErrorExit(HWND hDlg, HKEY hk) {
    MsgBoxParam(hDlg, SYSTEM+22, INITS+1, MB_ICONEXCLAMATION);
    if( hk == ghkeyMemMgt )
        VirtualCloseKey();

    DisableCoreDumpControls(hDlg);

    HourGlass(FALSE);
    return;
}

BOOL CoreDumpInit(HWND hDlg) {
    int i;
    VCREG_RET vcVirt, vcCore;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS Status;
    DWORD dwTotalPhys;
    INT nBootPF;

    HourGlass(TRUE);
    vcVirt = VirtualOpenKey();

    if( vcVirt == VCREG_ERROR ) {
        CoreDumpInitErrorExit(hDlg, NULL);
        return FALSE;
    }

    if (!VirtualGetPageFiles(apf) ) {
        CoreDumpInitErrorExit(hDlg, ghkeyMemMgt);
        return FALSE;
    }



    vcCore = CoreDumpOpenKey();

    if (vcCore == VCREG_ERROR) {
        //  Error - cannot even get the current settings from the reg.
        CoreDumpInitErrorExit(hDlg, ghkeyMemMgt);
        return FALSE;
    } else if (vcCore == VCREG_READONLY || vcVirt == VCREG_READONLY) {
        /*
         * Disable some fields, because they only have Read access.
         */
        for (i = 0; i < CCTL_COREDUMP; i++ ) {
            EnableWindow(GetDlgItem(hDlg, acdfControls[i].idCtl), FALSE);
        }
    }

    /*
     * For each control in the dialog...
     */
    for( i = 0; i < CCTL_COREDUMP; i++ ) {
        /*
         * Get the value out of the registry
         */
        CoreDumpGetValue(i);

        /*
         * Init the control value in the dialog
         */
        switch( acdfControls[i].dwType ) {
        case REG_DWORD:
            CheckDlgButton(hDlg, acdfControls[i].idCtl,
                    acdfControls[i].u.fValue);
            break;

        default:
            SetDlgItemText(hDlg, acdfControls[i].idCtl,
                    acdfControls[i].u.pszValue);
            break;
        }
    }

    /*
     * Special case -- if the machine has more than (MAX_SWAPSIZE - 1) MB of RAM
     * installed, it cannot create crash dumps.
     */
    Status = NtQuerySystemInformation(
        SystemBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL
    );

    if (!NT_SUCCESS(Status)) {
        dwTotalPhys = 0;
    } /* if */
    else {
        dwTotalPhys = BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
    } /* else */

    nBootPF = (dwTotalPhys / ONE_MEG) + 1;

    if (MAX_SWAPSIZE < nBootPF) {
        acdfControls[CD_WRITE].u.fValue = FALSE;
        CheckDlgButton(hDlg, acdfControls[CD_WRITE].idCtl, FALSE);
        CoreDumpPutValue(CD_WRITE);
        EnableWindow(GetDlgItem(hDlg, acdfControls[CD_WRITE].idCtl), FALSE);
    } /* if */


    /*
     * Special case disable the overwrite and logfile controls if the
     * write file check box is not set.
     */
    if (!IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_WRITE)) {
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_OVERWRITE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDC_STARTUP_CDMP_KERNELONLY), FALSE);
    }

    HourGlass(FALSE);

    return TRUE;
}

BOOL CoreDumpUpdateRegistry(HWND hDlg) {
    int i;
    BOOL fErr = FALSE;
    TCHAR szPath[MAX_PATH];
    BOOL fRegChanged = FALSE;
    BOOL fThisChanged;

    for( i = 0; i < CCTL_COREDUMP; i++) {

        fThisChanged = FALSE;

        switch(acdfControls[i].dwType) {
        case REG_DWORD: {
            BOOL fTmp;
            fTmp = (BOOL)IsDlgButtonChecked(hDlg,
                    acdfControls[i].idCtl);


            if (fTmp != acdfControls[i].u.fValue) {
                fThisChanged = TRUE;
                acdfControls[i].u.fValue = fTmp;
            }

            break;
        }

        default:

            //BUGBUG - check return value
            GetDlgItemText(hDlg, acdfControls[i].idCtl, szPath, MAX_PATH);

            if (lstrcmpi(acdfControls[i].u.pszValue, szPath) != 0) {
                fThisChanged = TRUE;
                MemFree(acdfControls[i].u.pszValue);
                acdfControls[i].u.pszValue = CloneString(szPath);
            }
            break;
        }

        if (fThisChanged) {
            if (CoreDumpPutValue(i)) {
                fErr = TRUE;
            }

            fRegChanged = TRUE;
        }
    }

    if (fErr) {
        MsgBoxParam(hDlg, SYSTEM+24, INITS+1, MB_ICONEXCLAMATION);
    }

    return fRegChanged;
}
