/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    virtual.c

Abstract:

    Implements the Change Virtual Memory dialog of the System
    Control Panel Applet

Notes:

    The virtual memory settings and the crash dump (core dump) settings
    are tightly-coupled.  Therefore, virtual.c and virtual.h have some
    heavy dependencies on crashdmp.c and startup.h (and vice versa).

Author:

    Byron Dazey 06-Jun-1992

Revision History:

    14-Apr-93 JonPa 
        maintain paging path if != \pagefile.sys

    15-Dec-93 JonPa 
        added Crash Recovery dialog

    02-Feb-1994 JonPa 
        integrated crash recover and virtual memory settings

    18-Sep-1995 Steve Cathcart 
        split system.cpl out from NT3.51 main.cpl

    12-Jan-1996 JonPa 
        made part of the new SUR pagified system.cpl

    15-Oct-1997 scotthal
        Split out CoreDump*() stuff into separate file

--*/
//==========================================================================
//                              Include files
//==========================================================================
// NT base apis
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntdddisk.h>
#include <help.h>

// Application specific
#include "sysdm.h"


//==========================================================================
//                     External Data Declarations
//==========================================================================
extern HFONT   hfontBold;

//==========================================================================
//                            Local Definitions
//==========================================================================

#define MAX_SIZE_LEN        4       // Max chars in the Swap File Size edit.
#define MIN_FREESPACE       5       // Must have 5 meg free after swap file
#define MIN_SUGGEST         22      // Always suggest at least 22 meg
#define CCHMBSTRING         12      // Space for localizing the "MB" string.

/*
 * Space for 26 pagefile info structures and 26 paths to pagefiles.
 */
#define PAGEFILE_INFO_BUFFER_SIZE MAX_DRIVES * sizeof(SYSTEM_PAGEFILE_INFORMATION) + \
                                  MAX_DRIVES * MAX_PATH * sizeof(TCHAR)

/*
 * Maximum length of volume info line in the listbox.
 *                           A:  [   Vol_label  ]   %d   -   %d
 */
#define MAX_VOL_LINE        (3 + 1 + MAX_PATH + 2 + 10 + 3 + 10)


/*
 * This amount will be added to the minimum page file size to determine
 * the maximum page file size if it is not explicitly specified.
 */
#define MAXOVERMINFACTOR    50


#define TABSTOP_VOL         22
#define TABSTOP_SIZE        122


/*
 * My privilege 'handle' structure
 */
typedef struct {
    HANDLE hTok;
    TOKEN_PRIVILEGES tp;
} PRIVDAT, *PPRIVDAT;

//==========================================================================
//                            Typedefs and Structs
//==========================================================================
// registry info for a page file (but not yet formatted).
//Note: since this structure gets passed to FormatMessage, all fields must
//be 4 bytes wide.
typedef struct
{
    LPTSTR pszName;
    DWORD  nMin;
    DWORD  nMax;
    DWORD  chNull;
} PAGEFILDESC;



//==========================================================================
//                     Global Data Declarations
//==========================================================================
HKEY ghkeyMemMgt = NULL;
int  gcrefMemMgt = 0;
VCREG_RET gvcMemMgt =  VCREG_ERROR;
int     gcrefPagingFiles = 0;
TCHAR g_szSysDir[ MAX_PATH ];

//==========================================================================
//                     Local Data Declarations
//==========================================================================
/*
 * Virtual Memory Vars
 */

// Registry Key and Value Names
TCHAR szMemMan[] =
     TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

TCHAR szRegSizeLim[] = TEXT("System\\CurrentControlSet\\Control");

TCHAR szSessionManager[] = TEXT("System\\CurrentControlSet\\Control\\Session Manager");

TCHAR szPendingRename[] = TEXT("PendingFileRenameOperations");
TCHAR szRenameFunkyPrefix[] = TEXT("\\??\\");

#ifndef VM_DBG
TCHAR szPagingFiles[] = TEXT("PagingFiles");
TCHAR szRegistrySizeLimit[] = TEXT("RegistrySizeLimit");
TCHAR szPagedPoolSize[] = TEXT("PagedPoolSize");
#else
// temp values for testing only!
TCHAR szPagingFiles[] = TEXT("TestPagingFiles");
TCHAR szRegistrySizeLimit[] = TEXT("TestRegistrySizeLimit");
TCHAR szPagedPoolSize[] = TEXT("TestPagedPoolSize");
#endif

/* Array of paging files.  This is indexed by the drive letter (A: is 0). */
PAGING_FILE apf[MAX_DRIVES];
PAGING_FILE apfOriginal[MAX_DRIVES];

// Other VM Vars
TCHAR szPagefile[] = TEXT("x:\\pagefile.sys");
TCHAR szNoPageFile[] = TEXT("TempPageFile");
TCHAR szMB[CCHMBSTRING];

HKEY hkeyRSL;
DWORD dwFreeMB;
DWORD cmTotalVM;
DWORD cmRegSizeLim;
DWORD cmPagedPoolLim;
DWORD cmRegUsed;
static DWORD cxLBExtent;
static int cxExtra;

//
// Help IDs
//
DWORD aVirtualMemHelpIds[] = {
    IDC_STATIC,             NO_HELP,
    IDD_VM_VOLUMES,         NO_HELP,
    IDD_VM_DRIVE_HDR,       (IDH_DLG_VIRTUALMEM + 0),
    IDD_VM_PF_SIZE_LABEL,   (IDH_DLG_VIRTUALMEM + 1), 
    IDD_VM_DRIVE_LABEL,     (IDH_DLG_VIRTUALMEM + 2),
    IDD_VM_SF_DRIVE,        (IDH_DLG_VIRTUALMEM + 2),
    IDD_VM_SPACE_LABEL,     (IDH_DLG_VIRTUALMEM + 3),
    IDD_VM_SF_SPACE,        (IDH_DLG_VIRTUALMEM + 3),
    IDD_VM_ST_INITSIZE,     (IDH_DLG_VIRTUALMEM + 4),
    IDD_VM_SF_SIZE,         (IDH_DLG_VIRTUALMEM + 4),
    IDD_VM_ST_MAXSIZE,      (IDH_DLG_VIRTUALMEM + 5),
    IDD_VM_SF_SIZEMAX,      (IDH_DLG_VIRTUALMEM + 5),
    IDD_VM_SF_SET,          (IDH_DLG_VIRTUALMEM + 6),
    IDD_VM_MIN_LABEL,       (IDH_DLG_VIRTUALMEM + 7),
    IDD_VM_MIN,             (IDH_DLG_VIRTUALMEM + 7),
    IDD_VM_RECOMMEND_LABEL, (IDH_DLG_VIRTUALMEM + 8),
    IDD_VM_RECOMMEND,       (IDH_DLG_VIRTUALMEM + 8),
    IDD_VM_ALLOCD_LABEL,    (IDH_DLG_VIRTUALMEM + 9),
    IDD_VM_ALLOCD,          (IDH_DLG_VIRTUALMEM + 9),
    IDD_VM_RSL_ALLOCD_LABEL,(IDH_DLG_VIRTUALMEM + 10),
    IDD_VM_RSL_ALLOCD,      (IDH_DLG_VIRTUALMEM + 10),
    IDD_VM_RSL_LABEL,       (IDH_DLG_VIRTUALMEM + 11),
    IDD_VM_REG_SIZE_LIM,    (IDH_DLG_VIRTUALMEM + 11),
    0,0
};

#if 0


    Plan for maintaining the delta between paged pool and virtual mem sizes:

        1.  In INIT, remember the startup delta between PagedPoolSize and
            total pagefile size.

        2.  In [OK], recheck the delta between PagedPoolSize and total
            pagefile size.  If it is smaller than it was at startup AND
            the user has changed PagedPoolSize (via bumping RSL),
            then put up a popup saying:

                To increase the RegistrySizeLimit by the amount you have
                requested, you will need a bigger paging file. The page
                file size will be increased.

                            [OK]    [Cancel]

        3a. If they press [OK], then we will step through the drives,
            and bump each pagefile until the drive starts to run out
            of space, and then we will go on to the next drive.

        3b. If they press [Cancel], we will break out of the
            WM_COMMAND without calling EndDialog()

        4.  If there is not enough total disk free space for 3a.
            then we will put up a popup saying:


                You do not have enough disk space to increase your
                registry size by the amount you have requested.
                Please choose a smaller amount

                        [OK]

            And then we will break with focus set to RSL editbox.

    --------------------------------------------------------------------------

    Plan for splitting this into propert sheets:

        1.  Make the VM and CC registry keys globals that are inited
            to NULL (or INVALID_HANDLE_VALUE).  Also make gvcVirt and
            vcCore to be globals (so we can tell how the reg was opened
            inside virtinit().)

        1.  Change all RegCloseKey's to VirtualCloseKey and CoreDumpCloseKey

        2.  Change VirtualOpenKey and CoreDumpOpenKey from macros to
            functions that return the global handles if they are already
            opened, or else opens them.

        3.  In the Perf and Startup pages, call VirtualOpenKey,
            CoreDumpOpenKey, and VirtualGetPageFiles.

        -- now we can call VirtualMemComputeAlloced() from the perf page
        -- we can also just execute the CrashDump code in the startup page

        4.  rewrite VirtInit to not try and open the keys again, but instesd
            use gvcVirt, vcCore, hkeyVM and kheyCC.

        4.  Write VirtualCloseKey and CoreDumpCloseKey as follows...
            4.a     If hkey == NULL return
            4.b     RegCloseKey(hkey)
            4.c     hkey = NULL

        5.  In the PSN_RESET and PSN_APPLY cases for Perf and Startup pages
            call VirtualCloseKey and CoreDumpCloseKey

#endif



//==========================================================================
//                      Local Function Prototypes
//==========================================================================
static BOOL VirtualMemInit(HWND hDlg);
static BOOL ParsePageFileDesc(LPTSTR *ppszDesc, INT *pnDrive,
                  INT *pnMinFileSize, INT *pnMaxFileSize, LPTSTR *ppszName);
static VOID VirtualMemBuildLBLine(LPTSTR pszBuf, INT iDrive);
static INT GetMaxSpaceMB(INT iDrive);
static VOID VirtualMemSelChange(HWND hDlg);
static VOID VirtualMemUpdateAllocated(HWND hDlg);
int VirtualMemComputeTotalMax( void );
static BOOL VirtualMemSetNewSize(HWND hDlg);
static UINT VMGetDriveType(LPCTSTR lpszDrive);
void VirtualMemReconcileState();

void GetCurrRSL( LPINT pcmRSL, LPINT pcmUsed, LPINT pcmPPLim );
void GetAPrivilege( LPTSTR pszPrivilegeName, PPRIVDAT ppd );
void ResetOldPrivilege( PPRIVDAT ppdOld );

DWORD VirtualMemDeletePagefile( LPTSTR szPagefile );

#define GetPageFilePrivilege( ppd )         \
        GetAPrivilege(SE_CREATE_PAGEFILE_NAME, ppd)

#define GetRegistryQuotaPrivilege( ppd )    \
        GetAPrivilege(SE_INCREASE_QUOTA_NAME, ppd)

#define RSLOpenKey( phkRSL )        OpenRegKey( szRegSizeLim, phkRSL )


//==========================================================================
VCREG_RET VirtualOpenKey( void ) {

    DOUT("In VirtOpenKey" );

    if (gvcMemMgt == VCREG_ERROR) {
        gvcMemMgt = OpenRegKey( szMemMan, &ghkeyMemMgt );
    }

    if (gvcMemMgt != VCREG_ERROR)
        gcrefMemMgt++;

    DPRINTF((TEXT("SYSCPL.CPL: VirtOpenKey, cref=%d\n"), gcrefMemMgt ));
    return gvcMemMgt;
}

void VirtualCloseKey(void) {

    DOUT( "In VirtCloseKey" );

    if (gcrefMemMgt > 0) {
        gcrefMemMgt--;
        if (gcrefMemMgt == 0) {
            CloseRegKey( ghkeyMemMgt );
            gvcMemMgt = VCREG_ERROR;
        }
    }


    DPRINTF((TEXT("SYSCPL.CPL: VirtCloseKey, cref=%d\n"), gcrefMemMgt ));
}

LPTSTR SkipNonWhiteSpace( LPTSTR sz ) {
    while( *sz != TEXT('\0') && !IsWhiteSpace(*sz))
        sz++;

    return sz;
}

INT TranslateDlgItemInt( HWND hDlg, int id ) {
    /*
     * We can't just call GetDlgItemInt because the
     * string we are trying to translate looks like:
     *  nnn (MB), and the '(MB)' would break GetDlgInt.
     */
    TCHAR szBuffer[256];
    int i = 0;

    if (GetDlgItemText(hDlg, id, szBuffer,
            sizeof(szBuffer) / sizeof(*szBuffer))) {
        i = StringToInt( szBuffer );
    }

    return i;
}


LPTSTR SZPageFileName (int i)
{
    if (apf[i].pszPageFile != NULL) {
        return  apf[i].pszPageFile;
    }

    szPagefile[0] = (TCHAR)(i + (int)TEXT('A'));
    return szPagefile;
}

LONG GetRegistryInt( HKEY hkey, LPTSTR pszValue, LONG lDefault ) {
    DWORD dwType;
    DWORD cbTemp;
    DWORD dwVal;

    cbTemp = sizeof(DWORD);

    if (RegQueryValueEx (hkey, pszValue, NULL,
            &dwType, (LPBYTE)&dwVal, &cbTemp) != ERROR_SUCCESS ||
            dwType != REG_DWORD || cbTemp != sizeof(DWORD)) {
        dwVal = (DWORD)lDefault;
    }

    return (LONG)dwVal;
}

BOOL SetRegistryInt( HKEY hkey, LPTSTR pszValue, LONG iValue ) {
    return RegSetValueEx(hkey, pszValue, 0L, REG_DWORD, (LPBYTE)&iValue,
            sizeof(iValue)) == ERROR_SUCCESS;
}

void VirtualCopyPageFiles( PAGING_FILE *apfDest, BOOL fFreeOld, PAGING_FILE *apfSrc, BOOL fCloneStrings ) {
    int i;

    for (i = 0; i < MAX_DRIVES; i++) {
        if (fFreeOld && apfDest[i].pszPageFile != NULL) {
            MemFree(apfDest[i].pszPageFile);
        }

        if (apfSrc != NULL) {
            apfDest[i] = apfSrc[i];

            if (fCloneStrings && apfDest[i].pszPageFile != NULL) {
                apfDest[i].pszPageFile = CloneString(apfDest[i].pszPageFile);
            }
        }
    }
}


/*
 * int CheckForRSLChange(void) *
 *
 * Recheck the delta between PagedPoolSize and total
 * pagefile size.  If it is smaller than it was at startup AND
 * the user has changed PagedPoolSize (via bumping RSL),
 * then put up a popup saying:
 *
 *     To increase the Registry Size Limit by the amount you have
 *     requested, you will need a bigger paging file. The page
 *     file size will be increased.
 *
 *                 [OK]    [Cancel]
 *
 * If they press [OK], then we will step through the drives,
 * and bump each pagefile until the drive starts to run out
 * of space, and then we will go on to the next drive.
 *
 * If they press [Cancel], we will break out of the
 * WM_COMMAND without calling EndDialog()
 *
 *
 */
int CheckForRSLChange(HWND hDlg) {
    DWORD cmRSL;
    DWORD cmVM;
    DWORD cmPPL;
    int iRet = RET_VIRTUAL_CHANGE;


    cmRSL = TranslateDlgItemInt(hDlg, IDD_VM_REG_SIZE_LIM);

    if (cmRSL < cmRegUsed ) {
        MsgBoxParam(hDlg, SYSTEM+37, INITS+1, MB_ICONSTOP | MB_OK);
        return RET_ERROR;
    }

    cmVM = VirtualMemComputeTotalMax();

    // make a copy of cmPPL incase we have to abort
    cmPPL = cmPagedPoolLim;



    if ((cmRSL != 0 && cmRSL != cmRegSizeLim) ||
        (cmVM != cmTotalVM && GetRegistryInt(ghkeyMemMgt, szPagedPoolSize, 0) != 0)){
        /*
         * It changed!
         */

        // compute PagePoolLim, such that RSL < 80% of PagedPool Lim
        if (cmPPL * 8 / 10 < cmRSL ) {
            INT cmPgdVMDelta;

            // compute original delta between total Virt Mem and PagedPool
            cmPgdVMDelta = cmTotalVM - cmPPL;

            // RSL > 80% PagePoolLim, we have to bump Paged Pool Limit
            cmPPL = cmRSL * 10 / 8;

            // now check if we have to bump Total VM size
            if( cmVM < cmPPL + cmPgdVMDelta ) {

                // cmVM is now the extra VM amount we need
                cmVM = cmPPL + cmPgdVMDelta - cmVM;

                // we have to bump total VM size as well.
                if (MsgBoxParam(hDlg, SYSTEM+35, INITS+1, MB_ICONASTERISK |
                        MB_OKCANCEL) == IDOK ) {
                    int i;

                    /*
                     * They pressed [OK]. We will step through the drives,
                     * and bump each pagefile until the drive starts to run out
                     * of space, and then we will go on to the next drive.
                     */
                    for (i = 0; i < MAX_DRIVES && cmVM != 0; i++) {
                        DWORD cmFree;
                        // first we will only try those drives that already
                        // have a page file.

                        if (apf[i].nMinFileSize) {
                            // we'll leave at least 5 meg free on each drive
                            cmFree = GetFreeSpaceMB(i) - apf[i].nMaxFileSize -
                                    MIN_FREESPACE;

                            cmFree = max(cmFree, 0);

                            if (cmFree > 0) {
                                cmFree = min(cmFree, cmVM);
                                cmVM -= cmFree;

                                apf[i].nMaxFileSize += cmFree;
                            }
                        }
                    }

                    for (i = 0; i < MAX_DRIVES && cmVM != 0; i++) {
                        DWORD cmFree;
                        // If we weren't able to grow the existing files
                        // to be big enough, then alloc new ones

                        if(apf[i].nMinFileSize == 0 && apf[i].fCanHavePagefile){
                            // we'll leave at least 5 meg free on each drive
                            cmFree = GetFreeSpaceMB(i) - MIN_FREESPACE;

                            cmFree = max(cmFree, 0);

                            if (cmFree > 0) {
                                cmFree = min(cmFree, cmVM);
                                cmVM -= cmFree;

                                apf[i].nMaxFileSize += cmFree;
                                apf[i].nMinFileSize = MIN_SWAPSIZE;
                                apf[i].fCreateFile = TRUE;

                                // Remember if the page file does not exist so
                                // we can create it later
                                if (GetFileAttributes(SZPageFileName(i)) ==
                                        0xFFFFFFFF && GetLastError() ==
                                        ERROR_FILE_NOT_FOUND) {
                                    apf[i].fCreateFile = TRUE;
                                }
                            }
                        }
                    }

                    if (cmVM) {
                        // Not enough space to grow the page file, tell user
                        // to lower the RSL.
                        MsgBoxParam(hDlg, SYSTEM+36, INITS+1, MB_ICONSTOP);
                        return RET_ERROR;
                    }
                } else {
                    return RET_ERROR;
                }
            }

            //Write out the new PagedPoolLimit to the registry.
            //BUGBUG - Deal better with setting default pagedpool values to 0
            // in registry.
            SetRegistryInt(ghkeyMemMgt, szPagedPoolSize, cmPPL * ONE_MEG);

        } else {
            // we have enough PagedPool... try to set new RSL on the fly.
            SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;
            PRIVDAT pdQuota;
            NTSTATUS Status;

            GetRegistryQuotaPrivilege( &pdQuota );

            srqi.RegistryQuotaAllowed = cmRSL * ONE_MEG;
            Status = NtSetSystemInformation(SystemRegistryQuotaInformation,
                    &srqi, sizeof(srqi));

            if (NT_SUCCESS(Status)) {
                // setting the new RSL on the fly worked!
                iRet = RET_CHANGE_NO_REBOOT;
            }

            ResetOldPrivilege( &pdQuota );
        }

        /*
         * Write out new Reg Size Limit to the registry
         */
        if (cmRSL != cmPPL / 4) {
            //Write out new Reg Size Limit to the registry
            SetRegistryInt(hkeyRSL, szRegistrySizeLimit, cmRSL * ONE_MEG);
        } else {
            //Delete the RSL registry entry. (it will default to 25% PagedPool)
            RegDeleteValue(hkeyRSL, szRegistrySizeLimit );
        }

    } else {
        iRet = RET_NO_CHANGE;
    }

    return iRet;
}

/*
 * VirtualMemDlg
 *
 *
 *
 */

INT_PTR
APIENTRY
VirtualMemDlg(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
    static int fEdtCtlHasFocus = 0;

    switch (message)
    {
    case WM_INITDIALOG:
        VirtualMemInit(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam))
        {
        case IDD_VM_VOLUMES:
            /*
             * Make edit control reflect the listbox selection.
             */
            if (HIWORD(wParam) == LBN_SELCHANGE)
                VirtualMemSelChange(hDlg);

            break;

        case IDD_VM_SF_SET:
            if (VirtualMemSetNewSize(hDlg))
                SetDefButton(hDlg, IDOK);
            break;

        case IDOK:
        {
            int iRet;

            iRet = CheckForRSLChange(hDlg);

            if (iRet == RET_ERROR) {
                // The Reg Size Lim is in error, make the user
                // reset it.
                SetFocus(GetDlgItem(hDlg, IDD_VM_REG_SIZE_LIM));
                SendDlgItemMessage(hDlg, IDD_VM_REG_SIZE_LIM, EM_SETSEL, 0, -1);
                break;
            }

            iRet |= VirtualMemPromptForReboot(hDlg);
            // RET_ERROR means the user told us not to overwrite an
            // existing file called pagefile.sys, so we shouldn't
            // end the dialog just yet.
            if (RET_ERROR == iRet) {
                break;
            }

            VirtualMemUpdateRegistry();
            VirtualMemReconcileState();

            VirtualCloseKey();
#if 0
            CoreDumpCloseKey();
#endif
            CloseRegKey(hkeyRSL);

#if 0
            if (gfCoreDumpChanged)
                iRet |= RET_RECOVER_CHANGE;
#endif

            //
            // get rid of backup copy of pagefile structs
            //
            VirtualCopyPageFiles( apfOriginal, TRUE, NULL, FALSE );
            EndDialog(hDlg, iRet);
            HourGlass(FALSE);
            break;
        }

        case IDCANCEL:
            //
            // get rid of changes and restore original values
            //
            VirtualCopyPageFiles( apf, TRUE, apfOriginal, FALSE );

            VirtualCloseKey();
#if 0
            CoreDumpCloseKey();
#endif
            CloseRegKey(hkeyRSL);
            EndDialog(hDlg, RET_NO_CHANGE);
            HourGlass(FALSE);
            break;

        case IDD_VM_SF_SIZE:
        case IDD_VM_SF_SIZEMAX:
            switch(HIWORD(wParam))
            {
            case EN_CHANGE:
                if (fEdtCtlHasFocus != 0)
                    SetDefButton( hDlg, IDD_VM_SF_SET);
                break;

            case EN_SETFOCUS:
                fEdtCtlHasFocus++;
                break;

            case EN_KILLFOCUS:
                fEdtCtlHasFocus--;
                break;
            }
            break;

        default:
            break;
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD_PTR) (LPSTR) aVirtualMemHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD_PTR) (LPSTR) aVirtualMemHelpIds);
        break;


    case WM_DESTROY:
    {

        VirtualFreePageFiles(apf);
        /*
         * The docs were not clear as to what a dialog box should return
         * for this message, so I am going to punt and let the defdlgproc
         * doit.
         */

        /* FALL THROUGH TO DEFAULT CASE! */
    }

    default:
        return FALSE;
        break;
    }

    return TRUE;
}

/*
 * UINT VMGetDriveType( LPCTSTR lpszDrive )
 *
 * Gets the drive type.  This function differs from Win32's GetDriveType
 * in that it returns DRIVE_FIXED for lockable removable drives (like
 * bernolli boxes, etc).
 */
TCHAR szDevice[] = TEXT("\\Device");

UINT VMGetDriveType( LPCTSTR lpszDrive ) {
    UINT i;
    TCHAR szDevName[MAX_PATH];

    // Check for subst drive
    if (QueryDosDevice( lpszDrive, szDevName, ARRAYSIZE( szDevName ) ) != 0) {

        // If drive does not start with '\Device', then it is not FIXED
        szDevName[ARRAYSIZE(szDevice) - 1] = '\0';
        if ( lstrcmpi(szDevName, szDevice) != 0 ) {
            return DRIVE_REMOTE;
        }
    }

    i = GetDriveType( lpszDrive );
    if ( i == DRIVE_REMOVABLE ) {
        TCHAR szNtDrive[20];
        DWORD cb;
        DISK_GEOMETRY dgMediaInfo;
        HANDLE hDisk;

        /*
         * 'Removable' drive.  Check to see if it is a Floppy or lockable
         * drive.
         */

        cb = wsprintf( szNtDrive, TEXT("\\\\.\\%s"), lpszDrive );

        if ( cb != 0 && IsPathSep(szNtDrive[--cb]) ) {
            szNtDrive[cb] = TEXT('\0');
        }

        hDisk = CreateFile(
                    szNtDrive,
                    /* GENERIC_READ */ 0,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL
                    );

        if (hDisk != INVALID_HANDLE_VALUE ) {

            if (DeviceIoControl( hDisk, IOCTL_DISK_GET_MEDIA_TYPES, NULL,
                    0, &dgMediaInfo, sizeof(dgMediaInfo), &cb, NULL) == FALSE &&
                    GetLastError() != ERROR_MORE_DATA) {
                /*
                 * Drive is not a floppy
                 */
                i = DRIVE_FIXED;
            }

            CloseHandle(hDisk);
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            /*
             * Could not open the drive, either it is bad, or else we
             * don't have permission.  Since everyone has permission
             * to open floppies, then this must be a bernoulli type device.
             */
            i = DRIVE_FIXED;
        }
    }

    return i;
}

/*
 * BOOL VirtualGetPageFiles(PAGING_FILE *apf)
 *
 *  Fills in the PAGING_FILE array from the values stored in the registry
 */
BOOL VirtualGetPageFiles(PAGING_FILE *apf) {
    DWORD cbTemp;
    LPTSTR szTemp;
    DWORD dwType;
    INT nDrive;
    INT nMinFileSize;
    INT nMaxFileSize;
    LPTSTR psz;
    DWORD dwDriveMask;
    int i;
    static TCHAR szDir[] = TEXT("?:");

    DPRINTF((TEXT("SYSCPL: In VirtualGetPageFile, cref=%d\n"), gcrefPagingFiles));

    if (gcrefPagingFiles++ > 0) {
        // Paging files already loaded
        return TRUE;
    }

    dwDriveMask = GetLogicalDrives();

    for (i = 0; i < MAX_DRIVES; dwDriveMask >>= 1, i++)
    {
        apf[i].fCanHavePagefile = FALSE;
        apf[i].nMinFileSize = 0;
        apf[i].nMaxFileSize = 0;
        apf[i].nMinFileSizePrev = 0;
        apf[i].nMaxFileSizePrev = 0;
        apf[i].pszPageFile = NULL;

        if (dwDriveMask & 0x01)
        {
            szDir[0] = TEXT('A') + i;
            switch (VMGetDriveType(szDir))
            {
                case DRIVE_FIXED:
                    apf[i].fCanHavePagefile = TRUE;
                    break;

                default:
                    break;
            }
        }
    }

    if (RegQueryValueEx (ghkeyMemMgt, szPagingFiles, NULL, &dwType,
                         (LPBYTE) NULL, &cbTemp) != ERROR_SUCCESS)
    {
        // Could not get the current virtual memory settings size.
        return FALSE;
    }

    if ((szTemp = MemAlloc(LPTR, cbTemp)) == NULL)
    {
        // Could not alloc a buffer for the vmem settings
        return FALSE;
    }


    szTemp[0] = 0;
    if (RegQueryValueEx (ghkeyMemMgt, szPagingFiles, NULL, &dwType,
                         (LPBYTE) szTemp, &cbTemp) != ERROR_SUCCESS)
    {
        // Could not read the current virtual memory settings.
        MemFree(szTemp);
        return FALSE;
    }

    psz = szTemp;
    while (*psz)
    {
        LPTSTR pszPageName;

        /*
         * If the parse works, and this drive can have a pagefile on it,
         * update the apf table.  Note that this means that currently
         * specified pagefiles for invalid drives will be stripped out
         * of the registry if the user presses OK for this dialog.
         */
        if (ParsePageFileDesc(&psz, &nDrive, &nMinFileSize, &nMaxFileSize, &pszPageName))
        {
            if (apf[nDrive].fCanHavePagefile)
            {
                apf[nDrive].nMinFileSize =
                apf[nDrive].nMinFileSizePrev = nMinFileSize;

                apf[nDrive].nMaxFileSize =
                apf[nDrive].nMaxFileSizePrev = nMaxFileSize;

                apf[nDrive].pszPageFile = pszPageName;
            }
        }
    }

    MemFree(szTemp);
    return TRUE;
}

/*
 * VirtualFreePageFiles
 *
 * Frees data alloced by VirtualGetPageFiles
 *
 */
void VirtualFreePageFiles(PAGING_FILE *apf) {
    int i;

    DPRINTF((TEXT("SYSCPL: In VirtualFreePageFile, cref=%d\n"), gcrefPagingFiles));

    if (gcrefPagingFiles > 0) {
        gcrefPagingFiles--;

        if (gcrefPagingFiles == 0) {
            for (i = 0; i < MAX_DRIVES; i++) {
                if (apf[i].pszPageFile != NULL)
                    MemFree(apf[i].pszPageFile);
            }
        }
    }
}



/*
 * VirtualInitStructures()
 *
 * Calls VirtualGetPageFiles so other helpers can be called from the Perf Page.
 *
 * Returns:
 *  TRUE if success, FALSE if failure
 */
BOOL VirtualInitStructures( void ) {
    VCREG_RET vcVirt;
    BOOL fRet = FALSE;

    vcVirt = VirtualOpenKey();

    if (vcVirt != VCREG_ERROR)
        fRet = VirtualGetPageFiles( apf );

    LoadString(hInstance, SYSTEM + 18, szMB, CCHMBSTRING);

    return fRet;
}

void VirtualFreeStructures( void ) {
    VirtualFreePageFiles(apf);
    VirtualCloseKey();
}

/*
 * LPTSTR BackslashTerm( LPTSTR pszPath )
 */
LPTSTR BackslashTerm( LPTSTR pszPath )
{
    LPTSTR pszEnd;

    pszEnd = pszPath + lstrlen( pszPath );

    //
    //  Get the end of the source directory
    //

    switch( *CharPrev( pszPath, pszEnd ) )
    {
    case TEXT('\\'):
    case TEXT(':'):
        break;

    default:
        *pszEnd++ = TEXT( '\\' );
        *pszEnd = TEXT( '\0' );
    }
    return( pszEnd );
}

/*
 * VirtualMemInit
 *
 * Initializes the Virtual Memory dialog.
 *
 * Arguments:
 *  HWND hDlg - Handle to the dialog window.
 *
 * Returns:
 *  TRUE
 */

static
BOOL
VirtualMemInit(
    HWND hDlg
    )
{
    TCHAR szTemp[MAX_VOL_LINE];
    DWORD i;
    INT iItem;
    HWND hwndLB;
    INT aTabs[2];
    RECT rc;
    VCREG_RET vcVirt, vcRSL;
    SYSTEM_BASIC_INFORMATION BasicInfo;
    NTSTATUS status;
    unsigned __int64 TotalPhys;

    HourGlass(TRUE);


    //
    // Load the "MB" string.
    //
    LoadString(hInstance, SYSTEM + 18, szMB, CCHMBSTRING);

    ////////////////////////////////////////////////////////////////////
    //  List all drives
    ////////////////////////////////////////////////////////////////////

    vcVirt = VirtualOpenKey();

    vcRSL = RSLOpenKey(&hkeyRSL);
    if (vcRSL == VCREG_ERROR) {
        hkeyRSL = NULL;
    }

    if (vcVirt == VCREG_ERROR || vcRSL == VCREG_ERROR) {
        //  Error - cannot even get list of paging files from  registry
        MsgBoxParam(hDlg, SYSTEM+11, INITS+1, MB_ICONEXCLAMATION);
        EndDialog(hDlg, RET_NO_CHANGE);
        HourGlass(FALSE);

        if (ghkeyMemMgt != NULL)
            VirtualCloseKey();
        if (hkeyRSL != NULL)
            CloseRegKey(hkeyRSL);
        return FALSE;
    }

    /*
     * To change Virtual Memory size or Crash control, we need access
     * to both the CrashCtl key and the PagingFiles value in the MemMgr key
     */
    if (vcVirt == VCREG_READONLY ) {
        /*
         * Disable some fields, because they only have Read access.
         */
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_ST_INITSIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_ST_MAXSIZE), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_SF_SET), FALSE);
    }

    /*
     * To change Registry size limit, we need access to both
     * the RegSizeLim key and the PagedPoolLimit value in the MemMgr key
     */
    if (vcRSL == VCREG_READONLY || vcVirt == VCREG_READONLY) {
        EnableWindow(GetDlgItem(hDlg, IDD_VM_REG_SIZE_LIM), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_VM_REG_SIZE_TXT), FALSE);
    }

    if (!VirtualGetPageFiles(apf)) {
        // Could not read the current virtual memory settings.
        MsgBoxParam(hDlg, SYSTEM+17, INITS+1, MB_ICONEXCLAMATION);
    }

    //
    // Save a backup copy of the current pagefile structs
    //
    VirtualCopyPageFiles( apfOriginal, FALSE, apf, TRUE );

    hwndLB = GetDlgItem(hDlg, IDD_VM_VOLUMES);
    aTabs[0] = TABSTOP_VOL;
    aTabs[1] = TABSTOP_SIZE;
    SendMessage(hwndLB, LB_SETTABSTOPS, 2, (LPARAM)&aTabs);

    /*
     * Since SetGenLBWidth only counts tabs as one character, we must compute
     * the maximum extra space that the tab characters will expand to and
     * arbitrarily tack it onto the end of the string width.
     *
     * cxExtra = 1st Tab width + 1 default tab width (8 chrs) - strlen("d:\t\t");
     *
     * (I know the docs for LB_SETTABSTOPS says that a default tab == 2 dlg
     * units, but I have read the code, and it is really 8 chars)
     */
    rc.top = rc.left = 0;
    rc.bottom = 8;
    rc.right = TABSTOP_VOL + (4 * 8) - (4 * 4);
    MapDialogRect( hDlg, &rc );

    cxExtra = rc.right - rc.left;
    cxLBExtent = 0;

    for (i = 0; i < MAX_DRIVES; i++)
    {
        // Assume we don't have to create anything
        apf[i].fCreateFile = FALSE;

        if (apf[i].fCanHavePagefile)
        {
            VirtualMemBuildLBLine(szTemp, i);
            iItem = (INT)SendMessage(hwndLB, LB_ADDSTRING, 0, (LPARAM)szTemp);
            SendMessage(hwndLB, LB_SETITEMDATA, iItem, i);
            // SetGenLBWidth(hwndLB, szTemp, &cxLBExtent, hfontBold, cxExtra);
            cxLBExtent = SetLBWidthEx( hwndLB, szTemp, cxLBExtent, cxExtra);
        }
    }

    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZE, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);
    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZEMAX, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);

    /*
     * Get the total physical memory in the machine.
     */
    status = NtQuerySystemInformation(
        SystemBasicInformation,
        &BasicInfo,
        sizeof(BasicInfo),
        NULL
    );
    if (NT_SUCCESS(status)) {
        TotalPhys = (unsigned __int64) BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
    }
    else {
        TotalPhys = 0;
    }

    SetDlgItemMB(hDlg, IDD_VM_MIN, MIN_SWAPSIZE);

    // Recommended pagefile size is 1.5 * RAM size these days.
    // Nonintegral multiplication with unsigned __int64s is fun!
    // BUGBUG:  This will obviously fail if the machine has total RAM
    // greater than 13194139533312 MB (75% of a full 64-bit address
    // space).  Hopefully by the time someone has such a beast we'll
    // have __int128s to hold the results of this calculation.
    TotalPhys >>= 20; // Bytes to MB
    TotalPhys *= 3; // This will always fit because of the operation above
    TotalPhys >>= 1; // x*3/2 == 1.5*x, more or less
    i = (DWORD) TotalPhys; // BUGBUG:  This cast actually causes the
                           // algorithm to fail if the machine has
                           // more than ~ 3.2 billion MB of RAM.
                           // At that point, either the Win32 API has
                           // to change to allow me to pass __int64s
                           // as message params, or we have to start
                           // reporting these stats in GB.
    SetDlgItemMB(hDlg, IDD_VM_RECOMMEND, max(i, MIN_SUGGEST));

    /*
     * Select the first drive in the listbox.
     */
    SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, 0, 0L);
    VirtualMemSelChange(hDlg);

    VirtualMemUpdateAllocated(hDlg);

    /*
     * Show RegQuota
     */
    cmTotalVM = VirtualMemComputeTotalMax();

    GetCurrRSL(&cmRegSizeLim, &cmRegUsed, &cmPagedPoolLim);

    SetDlgItemMB(hDlg, IDD_VM_RSL_ALLOCD, cmRegUsed);

    SetDlgItemInt(hDlg, IDD_VM_REG_SIZE_LIM, cmRegSizeLim, FALSE );

    HourGlass(FALSE);

    return TRUE;
}


void GetCurrRSL( LPINT pcmRSL, LPINT pcmUsed, LPINT pcmPPLim ) {
    SYSTEM_REGISTRY_QUOTA_INFORMATION srqi;
    NTSTATUS Status;
    long cmDefRSL, cmDefPPL;

    Status = NtQuerySystemInformation(SystemRegistryQuotaInformation,
                                  &srqi, sizeof(srqi), NULL);

    if (NT_SUCCESS(Status)) {
        cmDefPPL = srqi.PagedPoolSize;
        *pcmUsed  = srqi.RegistryQuotaUsed;
        cmDefRSL   = srqi.RegistryQuotaAllowed;
    } else {
        // Get the info from the registry
        cmDefPPL = 5 * ONE_MEG;
        *pcmUsed  = *pcmPPLim / 4;
        cmDefRSL = -1;
    }

    *pcmPPLim = GetRegistryInt(ghkeyMemMgt, szPagedPoolSize, cmDefPPL);

    // If the registry was set to '0', then use the default
    if (*pcmPPLim <= 0)
        *pcmPPLim = cmDefPPL;

    // If we couldn't get the registry limit, then assume it is 80% PPL
    if (cmDefRSL < 0)
        cmDefRSL = cmDefPPL * 8 / 10;

    *pcmRSL   = GetRegistryInt(hkeyRSL, szRegistrySizeLimit, cmDefRSL);

    *pcmPPLim = (*pcmPPLim + ONE_MEG - 1) / ONE_MEG;
    *pcmUsed  = (*pcmUsed  + ONE_MEG - 1) / ONE_MEG;
    *pcmRSL   = (*pcmRSL   + ONE_MEG - 1) / ONE_MEG;
}

/*
 * ParseSDD
 */

int ParseSDD( LPTSTR psz, LPTSTR szPath, INT *pnMinFileSize, INT *pnMaxFileSize) {
    int cMatched = 0;
    LPTSTR pszNext;

    psz = SkipWhiteSpace(psz);

    if (*psz) {
        int cch;

        cMatched++;
        pszNext = SkipNonWhiteSpace(psz);
        cch = (int)(pszNext - psz);
        CopyMemory( szPath, psz, sizeof(TCHAR) * cch );
        szPath[cch] = TEXT('\0');

        psz = SkipWhiteSpace(pszNext);

        if (*psz) {
            cMatched++;
            pszNext = SkipNonWhiteSpace(psz);
            *pnMinFileSize = StringToInt( psz );

            psz = SkipWhiteSpace(pszNext);

            if (*psz) {
                cMatched++;
                *pnMaxFileSize = StringToInt( psz );
            }
        }
    }

    return cMatched;
}

/*
 * ParsePageFileDesc
 *
 *
 *
 * Arguments:
 *
 * Returns:
 *
 */

static
BOOL
ParsePageFileDesc(
    LPTSTR *ppszDesc,
    INT *pnDrive,
    INT *pnMinFileSize,
    INT *pnMaxFileSize,
    LPTSTR *ppstr
    )
{
    LPTSTR psz;
    LPTSTR pszName = NULL;
    int cFields;
    TCHAR chDrive;
    TCHAR szPath[MAX_PATH];

    /*
     * Find the end of this REG_MULTI_SZ string and point to the next one
     */
    psz = *ppszDesc;
    *ppszDesc = psz + lstrlen(psz) + 1;

    /*
     * Parse the string from "filename minsize maxsize"
     */
    szPath[0] = TEXT('\0');
    *pnMinFileSize = 0;
    *pnMaxFileSize = 0;

    /* Try it without worrying about quotes */
    cFields = ParseSDD( psz, szPath, pnMinFileSize, pnMaxFileSize);

    if (cFields < 2)
        return FALSE;

    /*
     * Find the drive index
     */
    chDrive = (TCHAR)CharUpper((LPTSTR)*szPath);

    if (chDrive < TEXT('A') || chDrive > TEXT('Z'))
        return FALSE;

    *pnDrive = (INT)(chDrive - TEXT('A'));

    /* if the path != x:\pagefile.sys then save it */
    if (lstrcmpi(szPagefile + 1, szPath + 1) != 0)
    {
        pszName = CloneString(szPath);
    }

    *ppstr = pszName;

    if (cFields < 3)
    {
        INT nSpace;

        // don't call GetDriveSpace if the drive is invalid
        if (apf[*pnDrive].fCanHavePagefile)
            nSpace = GetMaxSpaceMB(*pnDrive);
        else
            nSpace = 0;
        *pnMaxFileSize = min(*pnMinFileSize + MAXOVERMINFACTOR, nSpace);
    }

    return TRUE;
}



/*
 * VirtualMemBuildLBLine
 *
 *
 *
 */

static
VOID
VirtualMemBuildLBLine(
    LPTSTR pszBuf,
    INT iDrive
    )
{
    TCHAR szVolume[MAX_PATH];
    TCHAR szTemp[MAX_PATH];

    szTemp[0] = TEXT('A') + iDrive;
    szTemp[1] = TEXT(':');
    szTemp[2] = TEXT('\\');
    szTemp[3] = 0;

    *szVolume = 0;
    GetVolumeInformation(szTemp, szVolume, MAX_PATH,
            NULL, NULL, NULL, NULL, 0);

    szTemp[2] = TEXT('\t');
    lstrcpy(pszBuf, szTemp);

    if (*szVolume)
    {
        lstrcat(pszBuf, TEXT("["));
        lstrcat(pszBuf, szVolume);
        lstrcat(pszBuf, TEXT("]"));
    }

    if (apf[iDrive].nMinFileSize)
    {
        wsprintf(szTemp, TEXT("\t%d - %d"),
                apf[iDrive].nMinFileSize, apf[iDrive].nMaxFileSize);
        lstrcat(pszBuf, szTemp);
    }
}



/*
 * SetDlgItemMB
 *
 *
 */

VOID SetDlgItemMB( HWND hDlg, INT idControl, DWORD dwMBValue ) {
    TCHAR szBuf[32];

    wsprintf(szBuf, TEXT("%d %s"), dwMBValue, szMB),
    SetDlgItemText(hDlg, idControl, szBuf);
}



/*
 * GetFreeSpaceMB
 *
 *
 *
 */

DWORD
GetFreeSpaceMB(
    INT iDrive
)
{
    TCHAR szDriveRoot[4];
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwClusters;
    DWORD iSpace;
    DWORD iSpaceExistingPagefile;
    HANDLE hff;
    WIN32_FIND_DATA ffd;


    szDriveRoot[0] = TEXT('A') + iDrive;
    szDriveRoot[1] = TEXT(':');
    szDriveRoot[2] = TEXT('\\');
    szDriveRoot[3] = (TCHAR) 0;

    if (!GetDiskFreeSpace(szDriveRoot, &dwSectorsPerCluster, &dwBytesPerSector,
            &dwFreeClusters, &dwClusters))
        return 0;

    iSpace = (INT)((dwSectorsPerCluster * dwFreeClusters) /
            (ONE_MEG / dwBytesPerSector));

    //
    // Be sure to include the size of any existing pagefile.
    // Because this space can be reused for a new paging file,
    // it is effectively "disk free space" as well.  The
    // FindFirstFile api is safe to use, even if the pagefile
    // is in use, because it does not need to open the file
    // to get its size.
    //
    iSpaceExistingPagefile = 0;
    if ((hff = FindFirstFile(SZPageFileName(szDriveRoot[0] - TEXT('A')), &ffd)) !=
        INVALID_HANDLE_VALUE)
    {
        iSpaceExistingPagefile = (INT)(ffd.nFileSizeLow / ONE_MEG);
        FindClose(hff);
    }

    return iSpace + iSpaceExistingPagefile;
}


/*
 * GetMaxSpaceMB
 *
 *
 *
 */

static
INT
GetMaxSpaceMB(
    INT iDrive
    )
{
    TCHAR szDriveRoot[4];
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwClusters;
    INT iSpace;


    szDriveRoot[0] = (TCHAR)(TEXT('A') + iDrive);
    szDriveRoot[1] = TEXT(':');
    szDriveRoot[2] = TEXT('\\');
    szDriveRoot[3] = (TCHAR) 0;

    if (!GetDiskFreeSpace(szDriveRoot, &dwSectorsPerCluster, &dwBytesPerSector,
                          &dwFreeClusters, &dwClusters))
        return 0;

    iSpace = (INT)((dwSectorsPerCluster * dwClusters) /
                   (ONE_MEG / dwBytesPerSector));

    return iSpace;
}


/*
 * VirtualMemSelChange
 *
 *
 *
 */

static
VOID
VirtualMemSelChange(
    HWND hDlg
    )
{
    TCHAR szDriveRoot[4];
    TCHAR szTemp[MAX_PATH];
    TCHAR szVolume[MAX_PATH];
    INT iSel;
    INT iDrive;

    if ((iSel = (INT)SendDlgItemMessage(
            hDlg, IDD_VM_VOLUMES, LB_GETCURSEL, 0, 0)) == LB_ERR)
        return;

    iDrive = (INT)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES,
            LB_GETITEMDATA, iSel, 0);

    szDriveRoot[0] = TEXT('A') + iDrive;
    szDriveRoot[1] = TEXT(':');
    szDriveRoot[2] = TEXT('\\');
    szDriveRoot[3] = (TCHAR) 0;

    *szVolume = (TCHAR) 0;
    GetVolumeInformation(szDriveRoot, szVolume, MAX_PATH,
            NULL, NULL, NULL, NULL, 0);
    szTemp[0] = TEXT('A') + iDrive;
    szTemp[1] = TEXT(':');
    szTemp[2] = (TCHAR) 0;

    if (*szVolume)
    {
        lstrcat(szTemp, TEXT("  ["));
        lstrcat(szTemp, szVolume);
        lstrcat(szTemp, TEXT("]"));
    }


    //LATER: should we also put up total drive size as well as free space?

    SetDlgItemText(hDlg, IDD_VM_SF_DRIVE, szTemp);
    SetDlgItemMB(hDlg, IDD_VM_SF_SPACE, GetFreeSpaceMB(iDrive));

    if (apf[iDrive].nMinFileSize) {
        SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, apf[iDrive].nMinFileSize, FALSE);
        SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, apf[iDrive].nMaxFileSize, FALSE);
    }
    else {
        SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
        SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));
    }
}



/*
 * VirtualMemUpdateAllocated
 *
 *
 *
 */

INT VirtualMemComputeAllocated( HWND hWnd , BOOL *pfTempPf) 
{
    BOOL fSuccess = FALSE;
    static BOOL fWarned = FALSE;
    ULONG ulPagefileSize = 0;
    unsigned __int64 PagefileSize;
    NTSTATUS result = ERROR_ACCESS_DENIED;
    SYSTEM_INFO SysInfo;
    PSYSTEM_PAGEFILE_INFORMATION pPagefileInfo = NULL;
    PSYSTEM_PAGEFILE_INFORMATION pCurrentPagefile = NULL;
    LONG lResult = ERROR_ACCESS_DENIED;
    DWORD dwValueType = 0;
    DWORD fTempPagefile = 0;
    DWORD cbSize = sizeof(DWORD);

    __try {
        pCurrentPagefile = pPagefileInfo = (PSYSTEM_PAGEFILE_INFORMATION) MemAlloc(
            LPTR,
            PAGEFILE_INFO_BUFFER_SIZE
        );
        if (!pPagefileInfo) {
            __leave;
        } // if        
    
        // Get the page size in bytes
        GetSystemInfo(&SysInfo);

        // Get the sizes (in pages) of all of the pagefiles on the system
        result = NtQuerySystemInformation(
            SystemPageFileInformation,
            pPagefileInfo,
            PAGEFILE_INFO_BUFFER_SIZE,
            NULL
        );
        if (ERROR_SUCCESS != result) {
            __leave;
        } // if

        if (pfTempPf) {
            // Check to see if the system created a temporary pagefile
            lResult = RegQueryValueEx(
                ghkeyMemMgt,
                szNoPageFile,
                NULL,
                &dwValueType,
                (LPBYTE) &fTempPagefile,
                &cbSize
            );

            if ((ERROR_SUCCESS == lResult) && fTempPagefile) {
                *pfTempPf = TRUE;
            } // if (ERROR_SUCCESS...
            else {
                *pfTempPf = FALSE;
            } // else
        } // if (pfTempPf)
        
        // Add up pagefile sizes
        while (pCurrentPagefile->NextEntryOffset) {
            ulPagefileSize += pCurrentPagefile->TotalSize;
            ((LPBYTE) pCurrentPagefile) += pCurrentPagefile->NextEntryOffset;
        } // while
        ulPagefileSize += pCurrentPagefile->TotalSize;

        // Convert pages to bytes
        PagefileSize = (unsigned __int64) ulPagefileSize * SysInfo.dwPageSize;

        // Convert bytes to MB
        ulPagefileSize = (ULONG) (PagefileSize / ONE_MEG);

        fSuccess = TRUE;
        
    } // __try
    __finally {

        // If we failed to determine the pagefile size, then
        // warn the user that the reported size is incorrect,
        // once per applet invokation.
        if (!fSuccess && !fWarned) {
            MsgBoxParam(
                hWnd,
                SYSTEM + 43,
                INITS + 1,
                MB_ICONERROR | MB_OK
            );
            fWarned = TRUE;
        } // if

        if (pPagefileInfo) {
            MemFree((HLOCAL) pPagefileInfo);
        } // if

    } // __finally

    return(ulPagefileSize);
}

static VOID VirtualMemUpdateAllocated(
    HWND hDlg
    )
{

    SetDlgItemMB(hDlg, IDD_VM_ALLOCD, VirtualMemComputeAllocated(hDlg, NULL));
}


int VirtualMemComputeTotalMax( void ) {
    INT nTotalAllocated;
    INT i;

    for (nTotalAllocated = 0, i = 0; i < MAX_DRIVES; i++)
    {
        nTotalAllocated += apf[i].nMaxFileSize;
    }

    return nTotalAllocated;
}


/*
 * VirtualMemSetNewSize
 *
 *
 *
 */

static
BOOL
VirtualMemSetNewSize(
    HWND hDlg
    )
{
    DWORD nSwapSize;
    DWORD nSwapSizeMax;
    BOOL fTranslated;
    INT iSel;
    INT iDrive;
    TCHAR szTemp[MAX_PATH];
    DWORD nFreeSpace;
    DWORD CrashDumpSizeInMbytes;
    TCHAR Drive;
    INT iBootDrive;

    //
    // Initialize variables for crashdump.
    //

    if (GetSystemDrive (&Drive)) {
        iBootDrive = tolower (Drive) - 'a';
    } else {
        iBootDrive = 0;
    }

    CrashDumpSizeInMbytes =
            (DWORD) ( CoreDumpGetRequiredFileSize (NULL) / ONE_MEG );
    
#if 0
    CoreDumpGetValue(CD_LOG);
    CoreDumpGetValue(CD_SEND);
    CoreDumpGetValue(CD_WRITE);

    if (acdfControls[CD_WRITE].u.fValue) {
        nBootPF = -1;
    } else if (acdfControls[CD_LOG].u.fValue ||
            acdfControls[CD_SEND].u.fValue) {
        nBootPF = MIN_SWAPSIZE;
    }

    if (nBootPF != 0) {
        SYSTEM_BASIC_INFORMATION BasicInfo;
        NTSTATUS status;
        unsigned __int64 TotalPhys;
        TCHAR szBootPath[MAX_PATH];

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

        if (nBootPF == -1) {
            // Get the number of Meg in the system, rounding up
            // (note that we round up a zero remainder)
            status = NtQuerySystemInformation(
                SystemBasicInformation,
                &BasicInfo,
                sizeof(BasicInfo),
                NULL
            );
            if (NT_SUCCESS(status)) {
                TotalPhys = (unsigned __int64) BasicInfo.NumberOfPhysicalPages * BasicInfo.PageSize;
            }
            else {
                TotalPhys = 0;
            }

            nBootPF = (DWORD) ((TotalPhys / ONE_MEG) + 1);
        }
    }
#endif


    /*
     * If they blanked out the field (there is no text to return), then
     * they have implicitly selected no swap file for this drive.  This
     * has to be checked for here, because GetDlgItemInt says that a
     * blank field is an error (fTranslated gets set to FALSE).
     */
    if (!GetDlgItemText(hDlg, IDD_VM_SF_SIZE, szTemp, ARRAYSIZE(szTemp)))
    {
        nSwapSize = 0;
        nSwapSizeMax = 0;
        fTranslated = TRUE;
    }
    else
    {
        nSwapSize = (INT)GetDlgItemInt(hDlg, IDD_VM_SF_SIZE,
                &fTranslated, FALSE);
        if (!fTranslated)
        {
            MsgBoxParam(hDlg, SYSTEM+40, INITS+1, MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if ((nSwapSize < MIN_SWAPSIZE && nSwapSize != 0))
        {
            MsgBoxParam(hDlg, SYSTEM+13, INITS+1, MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSize == 0)
        {
            nSwapSizeMax = 0;
        }
        else
        {
            nSwapSizeMax = (INT)GetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX,
                    &fTranslated, FALSE);
            if (!fTranslated)
            {
                MsgBoxParam(hDlg, SYSTEM+41, INITS+1, MB_ICONEXCLAMATION,
                        MAX_SWAPSIZE);
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                return FALSE;
            }

            if (nSwapSizeMax < nSwapSize || nSwapSizeMax > MAX_SWAPSIZE)
            {
                MsgBoxParam(hDlg, SYSTEM+14, INITS+1, MB_ICONEXCLAMATION,
                        MAX_SWAPSIZE);
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                return FALSE;
            }
        }
    }

    if (fTranslated &&
            (iSel = (INT)SendDlgItemMessage(
            hDlg, IDD_VM_VOLUMES, LB_GETCURSEL, 0, 0)) != LB_ERR)
    {
        iDrive = (INT)SendDlgItemMessage(hDlg, IDD_VM_VOLUMES,
                LB_GETITEMDATA, iSel, 0);

        nFreeSpace = GetMaxSpaceMB(iDrive);

        if (nSwapSizeMax > nFreeSpace)
        {
            MsgBoxParam(hDlg, SYSTEM+16, INITS+1, MB_ICONEXCLAMATION,
                         (TCHAR)(iDrive + TEXT('A')));
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
            return FALSE;
        }

        nFreeSpace = GetFreeSpaceMB(iDrive);

        if (nSwapSize > nFreeSpace)
        {
            MsgBoxParam(hDlg, SYSTEM+15, INITS+1, MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSize != 0 && nFreeSpace - nSwapSize < MIN_FREESPACE)
        {
            MsgBoxParam(hDlg, SYSTEM+26, INITS+1, MB_ICONEXCLAMATION,
                    (int)MIN_FREESPACE);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSizeMax > nFreeSpace)
        {
            if (MsgBoxParam(hDlg, SYSTEM+20, INITS+1, MB_ICONINFORMATION |
                       MB_OKCANCEL, (TCHAR)(iDrive + TEXT('A'))) == IDCANCEL)
            {
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                return FALSE;
            }
        }

        if (iDrive == iBootDrive &&
            (ULONG64) nSwapSize < CrashDumpSizeInMbytes) {

            DWORD Ret;
            
            //
            // The new boot drive page file size is less than we need for
            // crashdump. The message notifies the user that the resultant
            // dump file may be truncated.
            //
            // NOTE: DO NOT, turn off dumping at this point, because a valid
            // dump could still be generated.
            //
             
            Ret = MsgBoxParam (hDlg,
                               SYSTEM+29,
                               INITS+1,
                               MB_ICONEXCLAMATION | MB_YESNO,
                               (TCHAR) ( iBootDrive + TEXT ('A') ),
                               (DWORD) CrashDumpSizeInMbytes
                               );

            if (Ret != IDYES) {
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
                return FALSE;
            }
        }

        apf[iDrive].nMinFileSize = nSwapSize;
        apf[iDrive].nMaxFileSize = nSwapSizeMax;

        // Remember if the page file does not exist so we can create it later
        if (GetFileAttributes(SZPageFileName(iDrive)) == 0xFFFFFFFF &&
                GetLastError() == ERROR_FILE_NOT_FOUND) {
            apf[iDrive].fCreateFile = TRUE;
        }

        VirtualMemBuildLBLine(szTemp, iDrive);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_DELETESTRING, iSel, 0);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_INSERTSTRING, iSel,
                (LPARAM)szTemp);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETITEMDATA, iSel,
                (LPARAM)iDrive);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, iSel, 0L);

        cxLBExtent = SetLBWidthEx(GetDlgItem(hDlg, IDD_VM_VOLUMES), szTemp, cxLBExtent, cxExtra);

        if (apf[iDrive].nMinFileSize) {
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, apf[iDrive].nMinFileSize, FALSE);
            SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, apf[iDrive].nMaxFileSize, FALSE);
        }
        else {
            SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
            SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));
        }

        VirtualMemUpdateAllocated(hDlg);
        SetFocus(GetDlgItem(hDlg, IDD_VM_VOLUMES));
    }

    return TRUE;
}



/*
 * VirtualMemUpdateRegistry
 *
 *
 *
 */

BOOL
VirtualMemUpdateRegistry(
    VOID
    )
{
    LPTSTR szBuf;
    TCHAR szTmp[MAX_DRIVES * 22];  //max_drives * sizeof(fmt_string)
    LONG i;
    INT c;
    int j;
    PAGEFILDESC aparm[MAX_DRIVES];
    static TCHAR szNULLs[] = TEXT("\0\0");

    c = 0;
    szTmp[0] = TEXT('\0');
    szBuf = szTmp;

    for (i = 0; i < MAX_DRIVES; i++)
    {
        /*
         * Does this drive have a pagefile specified for it?
         */
        if (apf[i].nMinFileSize)
        {
            j = (c * 4);
            aparm[c].pszName = CloneString(SZPageFileName(i));
            aparm[c].nMin = apf[i].nMinFileSize;
            aparm[c].nMax = apf[i].nMaxFileSize;
            aparm[c].chNull = (DWORD)TEXT('\0');
            szBuf += wsprintf( szBuf, TEXT("%%%d!s! %%%d!d! %%%d!d!%%%d!c!"),
                 j+1, j+2, j+3, j+4);
            c++;
        }
    }

    /*
     * Alloc and fill in the page file registry string
     */
    //since FmtMsg returns 0 for error, it can not return a zero length string
    //therefore, force string to be at least one space long.

    if (szTmp[0] == TEXT('\0')) {
        szBuf = szNULLs;
        j = 1; //Length of string == 1 char (ZTerm null will be added later).
    } else {

        j = FormatMessage( FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_MAX_WIDTH_MASK |
            FORMAT_MESSAGE_ARGUMENT_ARRAY,
            szTmp, 0, 0, (LPTSTR)&szBuf, 1, (va_list *)&aparm);
    }


    for( i = 0; i < c; i++ )
        MemFree(aparm[i].pszName);

    if (j == 0)
        return FALSE;

    i = RegSetValueEx (ghkeyMemMgt, szPagingFiles, 0, REG_MULTI_SZ,
                       (LPBYTE)szBuf, SIZEOF(TCHAR) * (j+1));

    // free the string now that it is safely stored in the registry
    if (szBuf != szNULLs)
        FmtFree(szBuf);

    // if the string didn't get there, then return error
    if (i != ERROR_SUCCESS)
        return FALSE;


    /*
     * Now be sure that any previous pagefiles will be deleted on
     * the next boot.
     */
    for (i = 0; i < MAX_DRIVES; i++)
    {
        /*
         * Did this drive have a pagefile before, but does not have
         * one now?
         */
        if (apf[i].nMinFileSizePrev != 0 && apf[i].nMinFileSize == 0)
        {
            //
            // Hack workaround -- MoveFileEx() is broken
            //
            TCHAR szPagefilePath[MAX_PATH];

            lstrcpy(szPagefilePath, szRenameFunkyPrefix);
            lstrcat(szPagefilePath, SZPageFileName(i));
            VirtualMemDeletePagefile(szPagefilePath);
//            MoveFileEx(SZPageFileName(i), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
        }
    }

    return TRUE;
}

void GetAPrivilege( LPTSTR szPrivilegeName, PPRIVDAT ppd ) {
    HANDLE hTok;
    LUID luid;
    TOKEN_PRIVILEGES tpNew;
    DWORD cb;

    if (LookupPrivilegeValue( NULL, szPrivilegeName, &luid ) &&
                OpenProcessToken(GetCurrentProcess(),
                TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hTok)) {

        tpNew.PrivilegeCount = 1;
        tpNew.Privileges[0].Luid = luid;
        tpNew.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

        if (!AdjustTokenPrivileges(hTok, FALSE, &tpNew, sizeof(ppd->tp),
                &(ppd->tp), &cb)) {
            GetLastError();
        }

        ppd->hTok = hTok;
    } else {
        ppd->hTok = NULL;
    }
}



void ResetOldPrivilege( PPRIVDAT ppdOld ) {
    if (ppdOld->hTok != NULL ) {

        AdjustTokenPrivileges(ppdOld->hTok, FALSE, &(ppdOld->tp), 0, NULL,
                NULL);

        CloseHandle( ppdOld->hTok );
        ppdOld->hTok = NULL;
    }
}

/*
 * VirtualMemReconcileState
 *
 * Reconciles the n*FileSizePrev fields of apf with the n*FileSize fields.
 *
 */
void
VirtualMemReconcileState(
)
{
    INT i;

    for (i = 0; i < MAX_DRIVES; i++) {
        apf[i].nMinFileSizePrev = apf[i].nMinFileSize;
        apf[i].nMaxFileSizePrev = apf[i].nMaxFileSize;
    } // for

}

/*
 * VirtualMemDeletePagefile
 *
 * Hack workaround -- MoveFileEx() is broken.
 *
 */
DWORD
VirtualMemDeletePagefile(
    IN LPTSTR szPagefile
)
{
    HKEY hKey;
    BOOL fhKeyOpened = FALSE;
    DWORD dwResult;
    LONG lResult;
    LPTSTR szBuffer = NULL;
    LPTSTR szBufferEnd = NULL;
    DWORD dwValueType;
    DWORD cbRegistry;
    DWORD cbBuffer;
    DWORD cchPagefile;
    DWORD dwRetVal = ERROR_SUCCESS;

    __try {
        cchPagefile = lstrlen(szPagefile) + 1;

        lResult = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            szSessionManager,
            0L,
            KEY_READ | KEY_WRITE,
            &hKey
        );
        if (ERROR_SUCCESS != lResult) {
            dwRetVal = lResult;
            __leave;
        } // if
        
        //
        // Find out of PendingFileRenameOperations exists, and,
        // if it does, how big it is
        //
        lResult = RegQueryValueEx(
            hKey,
            szPendingRename,
            0L,
            &dwValueType,
            (LPBYTE) NULL,
            &cbRegistry
        );
        if (ERROR_SUCCESS != lResult) {
            //
            // If the value doesn't exist, we still need to set
            // it's size to one character so the formulas below (which are
            // written for the "we're appending to an existing string"
            // case) still work.
            //
            cbRegistry = sizeof(TCHAR);
        } // if

        //
        // Buffer needs to hold the existing registry value
        // plus the supplied pagefile path, plus two extra
        // terminating NULL characters.  However, we only have to add
        // room for one extra character, because we'll be overwriting
        // the terminating NULL character in the existing buffer.
        //
        cbBuffer = cbRegistry + ((cchPagefile + 1) * sizeof(TCHAR));

        szBufferEnd = szBuffer = (LPTSTR) MemAlloc(LPTR, cbBuffer);
        if (!szBuffer) {
            dwRetVal = ERROR_NOT_ENOUGH_MEMORY;
            __leave;
        } // if

        // 
        // Grab the existing value, if there is one
        //
        if (ERROR_SUCCESS == lResult) {
            lResult = RegQueryValueEx(
                hKey,
                szPendingRename,
                0L,
                &dwValueType,
                (LPBYTE) szBuffer,
                &cbRegistry
            );
            if (ERROR_SUCCESS != lResult) {
                dwRetVal = ERROR_FILE_NOT_FOUND;
                __leave;
            } // if

            //
            // We'll start our scribbling right on the final
            // terminating NULL character of the existing 
            // value.
            //
            szBufferEnd += (cbRegistry / sizeof(TCHAR)) - 1;
        } // if

        //
        // Copy in the supplied pagefile path.
        //
        lstrcpy(szBufferEnd, szPagefile);

        //
        // Add the final two terminating NULL characters
        // required for REG_MULTI_SZ-ness.  Yes, those indeces
        // are correct--when cchPagfile was calculated above,
        // we added one for its own terminating NULL character.
        //
        szBufferEnd[cchPagefile] = TEXT('\0');
        szBufferEnd[cchPagefile + 1] = TEXT('\0');

        dwValueType = REG_MULTI_SZ;

        lResult = RegSetValueEx(
            hKey,
            szPendingRename,
            0L,
            dwValueType,
            (CONST BYTE *) szBuffer,
            cbBuffer
        );

        if (ERROR_SUCCESS != lResult) {
            dwRetVal = lResult;
        } // if

    } // __try
    __finally {
        if (fhKeyOpened) {
            RegCloseKey(hKey);
        } // if
        if (szBuffer) {
            MemFree((HLOCAL) szBuffer);
        } // if
    } // __finally

    return dwRetVal;
}

/*
 * VirtualMemCreatePagefileFromIndex
 *
 *
 */
NTSTATUS
VirtualMemCreatePagefileFromIndex(
    IN INT i
)
{
    UNICODE_STRING us;
    LARGE_INTEGER liMin, liMax;
    NTSTATUS status;
    WCHAR wszPath[MAX_PATH*2];
    TCHAR szDrive[3];
    DWORD cch;

    HourGlass(TRUE);

    // convert path drive letter to an NT device path
    wsprintf(szDrive, TEXT("%c:"), (TCHAR)(i + (int)TEXT('A')));
    cch = QueryDosDevice( szDrive, wszPath, sizeof(wszPath) /
            sizeof(TCHAR));

    if (cch != 0) {

        // Concat the filename only (skip 'd:') to the nt device
        // path, and convert it to a UNICODE_STRING
        lstrcat( wszPath, SZPageFileName(i) + 2 );
        RtlInitUnicodeString( &us, wszPath );

        liMin.QuadPart = (LONGLONG)(apf[i].nMinFileSize * ONE_MEG);
        liMax.QuadPart = (LONGLONG)(apf[i].nMaxFileSize * ONE_MEG);

        status = NtCreatePagingFile ( &us, &liMin, &liMax, 0L );

    }
    HourGlass(FALSE);

    return(status);
}

/*
 * VirtualMemUpdateListboxFromIndex
 *
 */
void
VirtualMemUpdateListboxFromIndex(
    HWND hDlg,
    INT  i
)
{
    int j, cLBEntries, iTemp;
    int iLBEntry = -1;
    TCHAR szTemp[MAX_PATH];

    cLBEntries = (int)SendDlgItemMessage(
        (HWND) hDlg,
        (int) IDD_VM_VOLUMES,
        (UINT) LB_GETCOUNT,
        (WPARAM) 0,
        (LPARAM) 0
    );

    if (LB_ERR != cLBEntries) {
        // Loop through all the listbox entries, looking for the one
        // that corresponds to the drive index we were supplied.
        for (j = 0; j < cLBEntries; j++) {
            iTemp = (int)SendDlgItemMessage(
                (HWND) hDlg,
                (int) IDD_VM_VOLUMES,
                (UINT) LB_GETITEMDATA,
                (WPARAM) j,
                (LPARAM) 0
            );
            if (iTemp == i) {
                iLBEntry = j;
                break;
            } // if
        } // for

        if (-1 != iLBEntry) {
            // Found the desired entry, so update it.
            VirtualMemBuildLBLine(szTemp, i);

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_DELETESTRING,
                (WPARAM) iLBEntry,
                0
            );

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_INSERTSTRING,
                (WPARAM) iLBEntry,
                (LPARAM) szTemp
            );

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_SETITEMDATA,
                (WPARAM) iLBEntry,
                (LPARAM) i
            );

            SendDlgItemMessage(
                hDlg,
                IDD_VM_VOLUMES,
                LB_SETCURSEL,
                (WPARAM) iLBEntry,
                0
            );

            if (apf[i].nMinFileSize) {
                SetDlgItemInt(hDlg, IDD_VM_SF_SIZE, apf[i].nMinFileSize, FALSE);
                SetDlgItemInt(hDlg, IDD_VM_SF_SIZEMAX, apf[i].nMaxFileSize, FALSE);
            }
            else {
                SetDlgItemText(hDlg, IDD_VM_SF_SIZE, TEXT(""));
                SetDlgItemText(hDlg, IDD_VM_SF_SIZEMAX, TEXT(""));
            }

            VirtualMemUpdateAllocated(hDlg);

        } // if (-1 != iLBEntry)


    } // if (LB_ERR...

    return;

}

/*
 * VirtualMemPromptForReboot
 *
 *
 *
 */

int
VirtualMemPromptForReboot(
    HWND hDlg
    )
{
    INT i, result;
    int iReboot = RET_NO_CHANGE;
    int iThisDrv;
    UNICODE_STRING us;
    LARGE_INTEGER liMin, liMax;
    NTSTATUS status;
    TCHAR szDrive[3];
    PRIVDAT pdOld;

    GetPageFilePrivilege( &pdOld );

    // Have to make two passes through the list of pagefiles.
    // The first checks to see if files called "pagefile.sys" exist
    // on any of the drives that will be getting new pagefiles.
    // If there are existing files called "pagefile.sys" and the user
    // doesn't want any one of them to be overwritten, we bail out.
    // The second pass through the list does the actual work of
    // creating the pagefiles.

    for (i = 0; i < MAX_DRIVES; i++) {
        //
        // Did something change?
        //
        if (apf[i].nMinFileSize != apf[i].nMinFileSizePrev ||
                apf[i].nMaxFileSize != apf[i].nMaxFileSizePrev ||
                apf[i].fCreateFile ) {
            // Assume we have permission to nuke existing files called pagefile.sys
            // (we'll confirm the assumption later)
            result = IDYES;
            if (0 != apf[i].nMinFileSize) { // Pagefile wanted for this drive
                if (0 == apf[i].nMinFileSizePrev) { // There wasn't one there before
                    if (!(((GetFileAttributes(SZPageFileName(i)) == 0xFFFFFFFF)) || (GetLastError() == ERROR_FILE_NOT_FOUND))) {
                        // A file named pagefile.sys exists on the drive
                        // We need to confirm that we can overwrite it
                        result = MsgBoxParam(
                            hDlg,
                            SYSTEM + 25,
                            INITS + 1,
                            MB_ICONQUESTION | MB_YESNO,
                            SZPageFileName(i)
                        );
                    } // if (!((GetFileAttributes...
                } // if (0 == apf[i].nMinFileSizePrev)

                if (IDYES != result) {
                    // User doesn't want us overwriting an existing
                    // file called pagefile.sys, so back out the changes
                    apf[i].nMinFileSize = apf[i].nMinFileSizePrev;
                    apf[i].nMaxFileSize = apf[i].nMaxFileSizePrev;
                    apf[i].fCreateFile = FALSE;

                    // Update the listbox
                    VirtualMemUpdateListboxFromIndex(hDlg, i);
                    SetFocus(GetDlgItem(hDlg, IDD_VM_VOLUMES));

                    // Bail, telling the DlgProc not to end the dialog
                    iReboot = RET_ERROR;
                    goto bailout;
                } // if (IDYES != result)
            } // if (0 != apf[i].nMinFileSize)
            
        } // if
    } // for

    for (i = 0; i < MAX_DRIVES; i++)
    {
        //
        // Did something change?
        //
        if (apf[i].nMinFileSize != apf[i].nMinFileSizePrev ||
                apf[i].nMaxFileSize != apf[i].nMaxFileSizePrev ||
                apf[i].fCreateFile ) {
            /*
             * If we are strictly creating a *new* page file, or *enlarging*
             * the minimum or maximum size of an existing page file, then
             * we can try do it on the fly.  If no errors are returned by
             * the system then no reboot will be required.
             */

            // assume we will have to reboot
            iThisDrv = RET_VIRTUAL_CHANGE;

            /*
             * IF we are creating a new page file
             */
            if ((0 != apf[i].nMinFileSize) && (0 == apf[i].nMinFileSizePrev)) {

                status = VirtualMemCreatePagefileFromIndex(i);

                if (NT_SUCCESS(status)) {
                    // made it on the fly, no need to reboot for this drive!
                    iThisDrv = RET_CHANGE_NO_REBOOT;
                }
            }
            /*
             * If we're enlarging the minimum or maximum size of an existing
             * page file, we can try to do it on the fly
             */
            else if ((apf[i].nMinFileSize != 0) &&
                ((apf[i].nMinFileSize > apf[i].nMinFileSizePrev) ||
                (apf[i].nMaxFileSize > apf[i].nMaxFileSizePrev))) {

                status = VirtualMemCreatePagefileFromIndex(i);
                if (NT_SUCCESS(status)) {
                    iThisDrv = RET_CHANGE_NO_REBOOT;
                }

            } /* else if */

            iReboot |= iThisDrv;
        }
    }

bailout:
    ResetOldPrivilege( &pdOld );

    //
    // If Nothing changed, then change our IDOK to IDCANCEL so System.cpl will
    // know not to reboot.
    //
    return iReboot;
}
