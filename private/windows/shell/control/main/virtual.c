/** FILE: virtual.c ******** Module Header ********************************
 *
 *  Control panel applet for System configuration.  This file holds
 *  everything to do with configuring multiple paging files for NT.
 *
 * History:
 *  06/10/92 - Byron Dazey.
 *  04/14/93 - Jon Parati: maintain paging path if != \pagefile.sys
 *  12/15/93 - Jon Parati: added Crash Recovery dialog
 *  02/02/94 - Jon Parati: integrated crash recover and virtual memory settings
 *
 *  Copyright (C) 1992 Microsoft Corporation
 *
 *************************************************************************/
//==========================================================================
//                              Include files
//==========================================================================



#ifdef VM_DBG
#pragma message(__FILE__"(19): warning !!!! : compiled for DEBUG ONLY!" )
#endif

// NT base apis
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

// C Runtime
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Application specific
#include "main.h"
#include <winioctl.h>


//==========================================================================
//                     External Data Declarations
//==========================================================================
extern HFONT   hfontBold;

//==========================================================================
//                            Local Definitions
//==========================================================================

#define IDRV_DEF_BOOT       2       // Asssume booting from C:
#define MAX_SIZE_LEN        4       // Max chars in the Swap File Size edit.
#define MAX_DRIVES          26      // Max number of drives.
#define MIN_SWAPSIZE        2       // Min swap file size.
#define MIN_FREESPACE       5       // Must have 5 meg free after swap file
#define MIN_SUGGEST         22      // Always suggest at least 22 meg
#define SIZEOVERPHYS        12      // Size over phys mem for recommend. size.
#define ONE_MEG             1048576
#define MAX_SWAPSIZE        (0xFFFF0000 / ONE_MEG)    // magic number from LouP
#define CCHMBSTRING         12      // Space for localizing the "MB" string.

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
 * Error return codes from opening registry
 */
typedef enum {
    VCREG_OK,
    VCREG_READONLY,
    VCREG_ERROR
} VCREG_RET;


/*
 * My privilege 'handle' structure
 */
typedef struct {
    HANDLE hTok;
    TOKEN_PRIVILEGES tp;
} PRIVDAT, *PPRIVDAT;

#define IsPathSep(ch)   ((ch) == TEXT('\\') || (ch) == TEXT('/'))

//==========================================================================
//                            Typedefs and Structs
//==========================================================================

//  Swap file structure
typedef struct
{
    BOOL fCanHavePagefile;      // TRUE if the drive can have a pagefile.
    BOOL fCreateFile;           // TRUE if user hits [SET] and no pagefile
    INT nMinFileSize;           // Minimum size of pagefile in MB.
    INT nMaxFileSize;           // Max size of pagefile in MB.
    INT nMinFileSizePrev;       // Previous minimum size of pagefile in MB.
    INT nMaxFileSizePrev;       // Previous max size of pagefile in MB.
    LPTSTR  pszPageFile;        // Path to page file if it exists on that drv
} PAGING_FILE;

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

/*
 * Core Dump vars
 */
typedef struct {
    INT idCtl;
    LPTSTR pszValueName;
    DWORD dwType;
    union {
        BOOL fValue;
        LPTSTR pszValue;
    } u;
} COREDMP_FIELD;

//==========================================================================
//                     Local Data Declarations
//==========================================================================
/*
 * Virtual Memory Vars
 */

// Registry Key and Value Names
TCHAR szMemMan[] =
     TEXT("System\\CurrentControlSet\\Control\\Session Manager\\Memory Management");

TCHAR szRegSizeLim[] =
     TEXT("System\\CurrentControlSet\\Control");

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


// Other VM Vars
TCHAR szPagefile[] = TEXT("x:\\pagefile.sys");
TCHAR szNoPageFile[] = TEXT("TempPageFile");
TCHAR szMB[CCHMBSTRING];
BOOL gfCoreDumpChanged;

/* Array of paging files.  This is indexed by the drive letter (A: is 0). */
PAGING_FILE apf[MAX_DRIVES];

HKEY hkeyMM;
HKEY hkeyRSL;
DWORD dwTotalPhys;
DWORD dwFreeMB;
INT cmTotalVM;
INT cmRegSizeLim;
INT cmPagedPoolLim;
INT cmRegUsed;
static DWORD cxLBExtent;
static int cxExtra;

#if 0


    Plan:

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
#endif

/*
 * Core Dump vars
 */
HKEY hkeyCC;
TCHAR szCrashControl[] =
    TEXT("System\\CurrentControlSet\\Control\\CrashControl");

TCHAR szDefDumpFile[] = TEXT("%SYSTEMROOT%\\MEMORY.DMP");

COREDMP_FIELD acdfControls[] = {
    { IDD_CDMP_LOG,         TEXT("LogEvent"),   REG_DWORD, TRUE },
    { IDD_CDMP_SEND,        TEXT("SendAlert"),  REG_DWORD, TRUE },
    { IDD_CDMP_WRITE,       TEXT("CrashDumpEnabled"),  REG_DWORD, TRUE },
    { IDD_CDMP_OVERWRITE,   TEXT("Overwrite"),  REG_DWORD, TRUE },
    { IDD_CDMP_AUTOREBOOT,  TEXT("AutoReboot"), REG_DWORD, TRUE },
    { IDD_CDMP_FILENAME,    TEXT("DumpFile"),   REG_EXPAND_SZ, (BOOL)NULL},
};

#define CD_LOG      0
#define CD_SEND     1
#define CD_WRITE    2
#define CD_OVERW    3
#define CD_ABOOT    4
#define CD_FILE     5


#define CCTL_COREDUMP   (sizeof(acdfControls) / sizeof(COREDMP_FIELD))

//==========================================================================
//                      Local Function Prototypes
//==========================================================================

static BOOL VirtualGetPageFiles(PAGING_FILE *apf);
static BOOL VirtualMemInit(HWND hDlg);
static BOOL ParsePageFileDesc(LPTSTR *ppszDesc, INT *pnDrive,
                  INT *pnMinFileSize, INT *pnMaxFileSize, LPTSTR *ppszName);
static VOID VirtualMemBuildLBLine(LPTSTR pszBuf, INT iDrive);
static VOID SetDlgItemMB(HWND hDlg, INT idControl, DWORD dwMBValue);
static INT GetFreeSpaceMB(INT iDrive);
static INT GetMaxSpaceMB(INT iDrive);
static VOID VirtualMemSelChange(HWND hDlg);
static VOID VirtualMemUpdateAllocated(HWND hDlg);
int VirtualMemComputeTotalMax( void );
static BOOL VirtualMemSetNewSize(HWND hDlg);
static BOOL VirtualMemUpdateRegistry(VOID);
static int VirtualMemPromptForReboot(HWND hDlg);
static LPTSTR CloneString(LPTSTR psz);
static UINT VMGetDriveType(LPCTSTR lpszDrive);

VCREG_RET OpenRegKey( LPTSTR szKeyName, PHKEY phkMM );

void GetCurrRSL( LPINT pcmRSL, LPINT pcmUsed, LPINT pcmPPLim );
void GetAPrivilege( LPTSTR pszPrivilegeName, PPRIVDAT ppd );
void ResetOldPrivilege( PPRIVDAT ppdOld );

#define GetPageFilePrivilege( ppd )         \
        GetAPrivilege(SE_CREATE_PAGEFILE_NAME, ppd)

#define GetRegistryQuotaPrivilege( ppd )    \
        GetAPrivilege(SE_INCREASE_QUOTA_NAME, ppd)

#define CoreDumpOpenKey( phkCC )    OpenRegKey( szCrashControl, phkCC )
#define VirtualOpenKey( phkMM )     OpenRegKey( szMemMan, phkMM )
#define RSLOpenKey( phkRSL )        OpenRegKey( szRegSizeLim, phkRSL )

/*
 * Core dump functions
 */
static void CoreDumpGetValue(int i);
static BOOL CoreDumpPutValue(int i);
static void CoreDumpInit(HWND hDlg);
static BOOL CoreDumpUpdateRegistry(HWND hDlg);

/*
 * External functions (located in system.c)
 */
VOID SetDefButton(HWND hwndDlg, int idButton);

void SetGenLBWidth (HWND hwndLB, LPTSTR szBuffer, LPDWORD pdwWidth,
                    HANDLE hfontNew, DWORD cxExtra);

//==========================================================================

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
        _stscanf( szBuffer, TEXT("%d"), &i );
    }

    return i;
}


LPTSTR SZPageFileName (int i)
{
    if (apf[i].pszPageFile != NULL)
        return  apf[i].pszPageFile;

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
    int cmRSL;
    int cmVM;
    int cmPPL;
    int iRet = RET_VIRTUAL_CHANGE;


    cmRSL = TranslateDlgItemInt(hDlg, IDD_VM_REG_SIZE_LIM);

    if (cmRSL < cmRegUsed ) {
        MyMessageBox(hDlg, SYSTEM+37, INITS+1, MB_ICONSTOP | MB_OK);
        return RET_ERROR;
    }

    cmVM = VirtualMemComputeTotalMax();

    // make a copy of cmPPL incase we have to abort
    cmPPL = cmPagedPoolLim;



    if ((cmRSL != 0 && cmRSL != cmRegSizeLim) ||
        (cmVM != cmTotalVM && GetRegistryInt(hkeyMM, szPagedPoolSize, 0) != 0)){
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
                if (MyMessageBox(hDlg, SYSTEM+35, INITS+1, MB_ICONASTERISK |
                        MB_OKCANCEL) == IDOK ) {
                    int i;

                    /*
                     * They pressed [OK]. We will step through the drives,
                     * and bump each pagefile until the drive starts to run out
                     * of space, and then we will go on to the next drive.
                     */
                    for (i = 0; i < MAX_DRIVES && cmVM != 0; i++) {
                        int cmFree;
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
                        int cmFree;
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
                        MyMessageBox(hDlg, SYSTEM+36, INITS+1, MB_ICONSTOP);
                        return RET_ERROR;
                    }
                } else {
                    return RET_ERROR;
                }
            }

            //Write out the new PagedPoolLimit to the registry.
            //BUGBUG - Deal better with setting default pagedpool values to 0
            // in registry.
            SetRegistryInt(hkeyMM, szPagedPoolSize, cmPPL * ONE_MEG);

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

BOOL
APIENTRY
VirtualMemDlg(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam
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

            VirtualMemUpdateRegistry();
            iRet |= VirtualMemPromptForReboot(hDlg);

            if (iRet & RET_CHANGE_NO_REBOOT) {
                //
                // We created a pagefile, turn off temp page file flag
                //
                DWORD dwRegData;

                dwRegData = 0;
                RegSetValueEx(hkeyMM, szNoPageFile, 0, REG_DWORD,
                        (LPBYTE)&dwRegData, sizeof(dwRegData));
            }

            RegCloseKey(hkeyMM);
            RegCloseKey(hkeyCC);
            RegCloseKey(hkeyRSL);

            if (gfCoreDumpChanged)
                iRet |= RET_RECOVER_CHANGE;

            EndDialog(hDlg, iRet );
            HourGlass(FALSE);
            break;
        }

        case IDCANCEL:
            RegCloseKey(hkeyMM);
            RegCloseKey(hkeyCC);
            EndDialog(hDlg, RET_NO_CHANGE);
            HourGlass(FALSE);
            break;

        case IDD_HELP:
            goto DoHelp;

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

    case WM_DESTROY:
    {
        int i;

        for (i = 0; i < MAX_DRIVES; i++)
        {
            if (apf[i].pszPageFile != NULL)
                LocalFree(apf[i].pszPageFile);
        }

        /*
         * The docs were not clear as to what a dialog box should return
         * for this message, so I am going to punt and let the defdlgproc
         * doit.
         */

        /* FALL THROUGH TO DEFAULT CASE! */
    }

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
        }
        else
            return FALSE;
        break;
    }

    return TRUE;
}


/*
 * VCREG_RET OpenRegKey( LPTSTR pszKeyName, PHKEY phk )
 *
 * Opens the a section of the registry
 */
VCREG_RET OpenRegKey( LPTSTR pszKeyName, PHKEY phk ) {
    LONG Error;

    Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0,
            KEY_READ | KEY_WRITE, phk);

    if (Error != ERROR_SUCCESS)
    {
        Error = RegOpenKeyEx(HKEY_LOCAL_MACHINE, pszKeyName, 0, KEY_READ, phk);
        if (Error != ERROR_SUCCESS)
        {
            return VCREG_ERROR;
        }

        /*
         * We only have Read access.
         */
        return VCREG_READONLY;
    }

    return VCREG_OK;
}

/*
 * UINT VMGetDriveType( LPCTSTR lpszDrive )
 *
 * Gets the drive type.  This function differs from Win32's GetDriveType
 * in that it returns DRIVE_FIXED for lockable removable drives (like
 * bernolli boxes, etc).
 */
UINT VMGetDriveType( LPCTSTR lpszDrive ) {
    UINT i;

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

            if (DeviceIoControl( hDisk, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL,
                    0, &dgMediaInfo, sizeof(dgMediaInfo), &cb, NULL) &&
                    dgMediaInfo.MediaType == RemovableMedia) {
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
    static TCHAR szDir[] = TEXT("?:\\");



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

    if (RegQueryValueEx (hkeyMM, szPagingFiles, NULL, &dwType,
                         (LPBYTE) NULL, &cbTemp) != ERROR_SUCCESS)
    {
        // Could not get the current virtual memory settings size.
        return FALSE;
    }

    if ((szTemp = LocalAlloc(LPTR, cbTemp)) == NULL)
    {
        // Could not alloc a buffer for the vmem settings
        return FALSE;
    }


    szTemp[0] = 0;
    if (RegQueryValueEx (hkeyMM, szPagingFiles, NULL, &dwType,
                         (LPBYTE) szTemp, &cbTemp) != ERROR_SUCCESS)
    {
        // Could not read the current virtual memory settings.
        LocalFree(szTemp);
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

    LocalFree(szTemp);
    return TRUE;
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
    INT i;
    INT iItem;
    HWND hwndLB;
    MEMORYSTATUS mst;
    INT aTabs[2];
    RECT rc;
    VCREG_RET vcVirt, vcCore, vcRSL;

    HourGlass(TRUE);

    //
    // Load the "MB" string.
    //
    LoadString(hModule, SYSTEM + 18, szMB, CCHMBSTRING);

    ////////////////////////////////////////////////////////////////////
    //  List all drives
    ////////////////////////////////////////////////////////////////////

    gfCoreDumpChanged = FALSE;
    vcCore = CoreDumpOpenKey(&hkeyCC);
    if (vcCore == VCREG_ERROR) {
        hkeyCC = NULL;
    }

    vcVirt = VirtualOpenKey(&hkeyMM);
    if (vcVirt == VCREG_ERROR) {
        hkeyMM = NULL;
    }

    vcRSL = RSLOpenKey(&hkeyRSL);
    if (vcRSL == VCREG_ERROR) {
        hkeyRSL = NULL;
    }

    if (vcVirt == VCREG_ERROR || vcCore == VCREG_ERROR || vcRSL == VCREG_ERROR) {
        //  Error - cannot even get list of paging files from  registry
        MyMessageBox(hDlg, SYSTEM+11, INITS+1, MB_ICONEXCLAMATION);
        EndDialog(hDlg, RET_NO_CHANGE);
        HourGlass(FALSE);
        if (hkeyCC != NULL)
            RegCloseKey(hkeyCC);
        if (hkeyMM != NULL)
            RegCloseKey(hkeyMM);
        if (hkeyRSL != NULL)
            RegCloseKey(hkeyRSL);
        return FALSE;
    }

    /*
     * To change Virtual Memory size or Crash control, we need access
     * to both the CrashCtl key and the PagingFiles value in the MemMgr key
     */
    if (vcVirt == VCREG_READONLY || vcCore == VCREG_READONLY) {
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
        MyMessageBox(hDlg, SYSTEM+17, INITS+1, MB_ICONEXCLAMATION);
    }

    hwndLB = GetDlgItem(hDlg, IDD_VM_VOLUMES);
    aTabs[0] = TABSTOP_VOL;
    aTabs[1] = TABSTOP_SIZE;
    SendMessage(hwndLB, LB_SETTABSTOPS, 2, (DWORD)&aTabs);

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
            iItem = (INT)SendMessage(hwndLB, LB_ADDSTRING, 0, (DWORD)szTemp);
            SendMessage(hwndLB, LB_SETITEMDATA, iItem, (DWORD)i);
            SetGenLBWidth(hwndLB, szTemp, &cxLBExtent, hfontBold, cxExtra);
        }
    }

    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZE, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);
    SendDlgItemMessage(hDlg, IDD_VM_SF_SIZEMAX, EM_LIMITTEXT, MAX_SIZE_LEN, 0L);

    /*
     * Get the total physical memory in the machine.
     */
    mst.dwLength = sizeof(MEMORYSTATUS);
    GlobalMemoryStatus(&mst);
    dwTotalPhys = mst.dwTotalPhys;

    SetDlgItemMB(hDlg, IDD_VM_MIN, MIN_SWAPSIZE);

    i = (dwTotalPhys / ONE_MEG) + SIZEOVERPHYS;
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

    *pcmPPLim = GetRegistryInt(hkeyMM, szPagedPoolSize, cmDefPPL);

    if (cmDefRSL < 0)
        cmDefRSL = cmDefPPL * 8 / 10;

    *pcmRSL   = GetRegistryInt(hkeyRSL, szRegistrySizeLimit, cmDefRSL);

    *pcmPPLim = (*pcmPPLim + ONE_MEG - 1) / ONE_MEG;
    *pcmUsed  = (*pcmUsed  + ONE_MEG - 1) / ONE_MEG;
    *pcmRSL   = (*pcmRSL   + ONE_MEG - 1) / ONE_MEG;
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
#ifdef UNICODE
    cFields = swscanf( psz, TEXT(" %ws %d %d"), szPath, pnMinFileSize,
            pnMaxFileSize);
#else
    cFields = sscanf( psz, TEXT(" %s %d %d"), szPath, pnMinFileSize,
            pnMaxFileSize);
#endif

    if (cFields < 2)
        return FALSE;

    /*
     * Find the drive index
     */
    chDrive = _totupper(*szPath);
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
    TCHAR szVolume[PATHMAX];
    TCHAR szTemp[PATHMAX];

    szTemp[0] = TEXT('A') + iDrive;
    szTemp[1] = TEXT(':');
    szTemp[2] = TEXT('\\');
    szTemp[3] = 0;

    *szVolume = 0;
    GetVolumeInformation(szTemp, szVolume, PATHMAX,
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

static
VOID
SetDlgItemMB(
    HWND hDlg,
    INT idControl,
    DWORD dwMBValue
    )
{
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

static
INT
GetFreeSpaceMB(
    INT iDrive
    )
{
    TCHAR szDriveRoot[4];
    DWORD dwSectorsPerCluster;
    DWORD dwBytesPerSector;
    DWORD dwFreeClusters;
    DWORD dwClusters;
    INT iSpace;
    INT iSpaceExistingPagefile;
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
    TCHAR szTemp[PATHMAX];
    TCHAR szVolume[PATHMAX];
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
    GetVolumeInformation(szDriveRoot, szVolume, PATHMAX,
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

static
VOID
VirtualMemUpdateAllocated(
    HWND hDlg
    )
{
    INT nTotalAllocated;
    INT i;

    for (nTotalAllocated = 0, i = 0; i < MAX_DRIVES; i++)
    {
        nTotalAllocated += apf[i].nMinFileSize;
    }

    SetDlgItemMB(hDlg, IDD_VM_ALLOCD, nTotalAllocated);
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
    INT nSwapSize;
    INT nSwapSizeMax;
    BOOL fTranslated;
    INT iSel;
    INT iDrive;
    TCHAR szTemp[PATHMAX];
    INT nFreeSpace;
    INT nBootPF = 0;
    INT iBootDrive = 0;

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
        MEMORYSTATUS ms;
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
            SYSTEM_INFO si;

            // Get the number of Meg in the system, rounding up
            // (note that we round up a zero remainder)

            GlobalMemoryStatus(&ms);
            nBootPF = (ms.dwTotalPhys / ONE_MEG) + 1;

            // If the slop at the end is less than one page, then
            // make it bigger.

            GetSystemInfo(&si);

            if ((nBootPF * ONE_MEG - ms.dwTotalPhys) <
                    si.dwPageSize) {
                nBootPF += 1;
            }
        }
    }


    /*
     * If they blanked out the field (there is no text to return), then
     * they have implicitly selected no swap file for this drive.  This
     * has to be checked for here, because GetDlgItemInt says that a
     * blank field is an error (fTranslated gets set to FALSE).
     */
    if (!GetDlgItemText(hDlg, IDD_VM_SF_SIZE, szTemp, CharSizeOf(szTemp)))
    {
        nSwapSize = 0;
        nSwapSizeMax = 0;
        fTranslated = TRUE;
    }
    else
    {
        nSwapSize = (INT)GetDlgItemInt(hDlg, IDD_VM_SF_SIZE,
                &fTranslated, FALSE);
        if (!fTranslated || (nSwapSize < MIN_SWAPSIZE && nSwapSize != 0))
        {
            MyMessageBox(hDlg, SYSTEM+13, INITS+1, MB_ICONEXCLAMATION);
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
            if (!fTranslated || nSwapSizeMax < nSwapSize ||
                    nSwapSizeMax > MAX_SWAPSIZE)
            {
                MyMessageBox(hDlg, SYSTEM+14, INITS+1, MB_ICONEXCLAMATION,
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
            MyMessageBox(hDlg, SYSTEM+16, INITS+1, MB_ICONEXCLAMATION,
                         (TCHAR)(iDrive + TEXT('A')));
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
            return FALSE;
        }

        nFreeSpace = GetFreeSpaceMB(iDrive);

        if (nSwapSize > nFreeSpace)
        {
            MyMessageBox(hDlg, SYSTEM+15, INITS+1, MB_ICONEXCLAMATION);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSize != 0 && nFreeSpace - nSwapSize < MIN_FREESPACE)
        {
            MyMessageBox(hDlg, SYSTEM+26, INITS+1, MB_ICONEXCLAMATION,
                    (int)MIN_FREESPACE);
            SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZE));
            return FALSE;
        }

        if (nSwapSizeMax > nFreeSpace)
        {
            if (MyMessageBox(hDlg, SYSTEM+20, INITS+1, MB_ICONINFORMATION |
                       MB_OKCANCEL, (TCHAR)(iDrive + TEXT('A'))) == IDCANCEL)
            {
                SetFocus(GetDlgItem(hDlg, IDD_VM_SF_SIZEMAX));
                return FALSE;
            }
        }

        if (iDrive == iBootDrive && nSwapSize < nBootPF) {

            /*
             * The new boot drive page file size is less than we need for
             * crash control.  Inform the user
             */
            if (MyMessageBox(hDlg, SYSTEM+29, INITS+1, MB_ICONEXCLAMATION |
                MB_OKCANCEL, (TCHAR)(iBootDrive+TEXT('A')),nBootPF)  == IDOK) {
                // turn off dumping
                acdfControls[CD_LOG].u.fValue = FALSE;
                acdfControls[CD_SEND].u.fValue = FALSE;
                acdfControls[CD_WRITE].u.fValue = FALSE;

                CoreDumpPutValue(CD_LOG);
                CoreDumpPutValue(CD_SEND);
                CoreDumpPutValue(CD_WRITE);

                gfCoreDumpChanged = TRUE;
            } else {
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
                (DWORD)szTemp);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETITEMDATA, iSel,
                (DWORD)iDrive);
        SendDlgItemMessage(hDlg, IDD_VM_VOLUMES, LB_SETCURSEL, iSel, 0L);
        SetGenLBWidth(GetDlgItem(hDlg, IDD_VM_VOLUMES), szTemp, &cxLBExtent,
                hfontBold, cxExtra);

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

static
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
        LocalFree(aparm[i].pszName);

    if (j == 0)
        return FALSE;

    i = RegSetValueEx (hkeyMM, szPagingFiles, 0, REG_MULTI_SZ,
                       (LPBYTE)szBuf, ByteCountOf(j+1));

    // free the string now that it is safely stored in the registry
    if (szBuf != szNULLs)
        LocalFree(szBuf);

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
            MoveFileEx(SZPageFileName(i), NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
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
 * VirtualMemPromptForReboot
 *
 *
 *
 */

static
int
VirtualMemPromptForReboot(
    HWND hDlg
    )
{
    INT i;
    int iReboot = RET_NO_CHANGE;
    int iThisDrv;
    UNICODE_STRING us;
    LARGE_INTEGER liMin, liMax;
    NTSTATUS status;
    WCHAR wszPath[MAX_PATH*2];
    TCHAR szDrive[3];
    PRIVDAT pdOld;

    GetPageFilePrivilege( &pdOld );

    for (i = 0; i < MAX_DRIVES; i++)
    {
        //
        // Did something change?
        //
        if (apf[i].nMinFileSize != apf[i].nMinFileSizePrev ||
                apf[i].nMaxFileSize != apf[i].nMaxFileSizePrev ||
                apf[i].fCreateFile ) {
            /*
             * If we are strictly creating a *new* page file, then
             * we can do it on the fly, otherwise we have to reboot
             */

            // assume we will have to reboot
            iThisDrv = RET_VIRTUAL_CHANGE;

            /*
             * IF we are not deleting a page file
             *          - AND -
             *    The Page file does not exist
             *          - OR -
             *    (This is a New page file AND We are allowed to erase the
             *      old, unused pagefile that exists there now)
             */
            if (apf[i].nMinFileSize != 0 &&
                    ((GetFileAttributes(SZPageFileName(i)) == 0xFFFFFFFF &&
                    GetLastError() == ERROR_FILE_NOT_FOUND) ||
                    (apf[i].nMinFileSizePrev == 0 && MyMessageBox(hDlg,
                    SYSTEM+25, INITS+1, MB_ICONQUESTION | MB_YESNO,
                    SZPageFileName(i)) == IDYES)) ) {

                DWORD cch;

                /*
                 * Create the page file on the fly so JVert and MGlass will
                 * stop bugging me!
                 */

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

                    if (NT_SUCCESS(status)) {
                        // made it on the fly, no need to reboot for this drive!
                        iThisDrv = RET_CHANGE_NO_REBOOT;
                    }
                }
                HourGlass(FALSE);
            }

            iReboot |= iThisDrv;
        }
    }

    ResetOldPrivilege( &pdOld );

    //
    // If Nothing changed, then change our IDOK to IDCANCEL so System.cpl will
    // know not to reboot.
    //
    return iReboot;
}


/*
 * SetDefButton
 *
 *
 */
VOID SetDefButton(
    HWND hwndDlg,
    int idButton
    )
{
    LRESULT lr;

    if (HIWORD(lr = SendMessage(hwndDlg, DM_GETDEFID, 0, 0)) == DC_HASDEFID)
    {
        HWND hwndOldDefButton = GetDlgItem(hwndDlg, LOWORD(lr));

        SendMessage (hwndOldDefButton,
                     BM_SETSTYLE,
                     MAKEWPARAM(BS_PUSHBUTTON, 0),
                     MAKELPARAM(TRUE, 0));
    }

    SendMessage( hwndDlg, DM_SETDEFID, idButton, 0L );
    SendMessage( GetDlgItem(hwndDlg, idButton),
                 BM_SETSTYLE,
                 MAKEWPARAM( BS_DEFPUSHBUTTON, 0 ),
                 MAKELPARAM( TRUE, 0 ));
}


BOOL ExpandEnvironmentString( LPTSTR pszDst, LPCTSTR pszSrc, DWORD cchDst ) {
    TCHAR ch;
    LPTSTR p;
    TCHAR szVar[MAX_PATH];
    DWORD cch;

    do {

        ch = *pszSrc++;

        if (ch != TEXT('%') ) {

            // no space left, truncate string and return false
            if (--cchDst == 0) {
                *pszDst = TEXT('\0');
                return FALSE;
            }

            *pszDst++ = ch;

        } else {
            /*
             * Expand variable
             */
            // look for the next '%'
            p = szVar;
            while( *pszSrc != TEXT('\0') && *pszSrc != TEXT('%') )
                    *p++ = *pszSrc++;

            *p = TEXT('\0');

            if (*pszSrc == TEXT('\0')) {
                // end of string, first '%' must be literal
                cch = lstrlen(szVar) + 1;

                // no more space, return false
                if (cch + 1 > cchDst) {
                    *pszDst++ = TEXT('\0');
                    return FALSE;
                }

                *pszDst++ = TEXT('%');
                CopyMemory( pszDst, szVar, cch * sizeof(TCHAR));
                return TRUE;

            } else {
                // we found the ending '%' sign, expand that string

                cch = GetEnvironmentVariable(szVar, pszDst, cchDst);

                if (cch == 0 || cch >= cchDst) {
                    //String didn't expand, copy it as a literal
                    cch = lstrlen(szVar);

                    // no space left, trunc string and return FALSE
                    if (cch + 2 + 1 > cchDst ) {
                        *pszDst = TEXT('\0');
                        return FALSE;
                    }

                    *pszDst++ = TEXT('%');

                    CopyMemory(pszDst, szVar, cch * sizeof(TCHAR));
                    pszDst += cch;

                    *pszDst++ = TEXT('%');

                    // cchDst -= two %'s and the string
                    cchDst -= (2 + cch);

                } else {
                    // string was expanded in place, bump pointer past its end
                    pszDst += cch;
                    cchDst -= cch;
                }

                // continue with next char after ending '%'
                pszSrc++;
            }
        }

    } while( ch != TEXT('\0') );

    return TRUE;
}

BOOL CoreDumpValidFile( HWND hDlg ) {
    TCHAR szPath[MAX_PATH];
    TCHAR szExpPath[MAX_PATH];
    LPTSTR psz;
    TCHAR ch;
    UINT uType;

    if (IsDlgButtonChecked(hDlg, IDD_CDMP_WRITE)) {
        /*
         * get the filename
         */
        if( GetDlgItemText(hDlg, IDD_CDMP_FILENAME, szPath,
                CharSizeOf(szPath)) == 0) {

            MyMessageBox(hDlg, SYSTEM+30, INITS+1, MB_ICONSTOP | MB_OK);
            return FALSE;
        }

        /*
         * Expand any environment vars, and then check to make sure it
         * is a fully quallified path
         */
        // if it has a '%' in it, then try to expand it
        if (!ExpandEnvironmentString(szExpPath, szPath,
                    CharSizeOf(szExpPath))) {
            MyMessageBox(hDlg, SYSTEM+33, INITS+1, MB_ICONSTOP | MB_OK,
                    (DWORD)MAX_PATH);
            return FALSE;
        }

        // now cannonicalize it
        GetFullPathName( szExpPath, CharSizeOf(szPath), szPath, &psz );

        // check to see that it already was cannonicalized
        if (lstrcmp( szPath, szExpPath ) != 0) {
            MyMessageBox(hDlg, SYSTEM+34, INITS+1, MB_ICONSTOP | MB_OK );
            return FALSE;
        }

        /*
         * check the drive (don't allow remote)
         */
        ch = szPath[3];
        szPath[3] = TEXT('\0');
        if (IsPathSep(szPath[0]) || ((uType = GetDriveType(szPath)) !=
                DRIVE_FIXED && uType != DRIVE_REMOVABLE)) {
            MyMessageBox(hDlg, SYSTEM+31, INITS+1, MB_ICONSTOP | MB_OK );
            return FALSE;
        }
        szPath[3] = ch;

        /*
         * if path is non-exstant, tell user and let him decide what to do
         */
        if (GetFileAttributes(szPath) == 0xFFFFFFFFL && GetLastError() !=
            ERROR_FILE_NOT_FOUND && MyMessageBox(hDlg, SYSTEM+32, INITS+1,
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
        MyMessageBox(hDlg, SYSTEM+38, INITS+1, MB_ICONEXCLAMATION );
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

BOOL
APIENTRY
CoreDumpDlg(
    HWND hDlg,
    UINT message,
    DWORD wParam,
    LONG lParam
    )
{


    switch (message)
    {
    case WM_INITDIALOG:
        CoreDumpInit(hDlg);
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {

        case IDOK: {
            int iRet = RET_NO_CHANGE;
            BOOL fRegChg;
            INT cMegBootPF;
            TCHAR szBootPath[MAX_PATH];
            int iBootDrive;
            MEMORYSTATUS ms;

            /*
             * Validate core dump filename
             */
            if (!CoreDumpValidFile(hDlg)) {
                SetFocus(GetDlgItem(hDlg, IDD_CDMP_FILENAME));
                break;
            }

            /*
             * If we are to do anything, then we need a pagefile on the boot
             * drive.
             *
             * If we are to write the dump file, it must be >= sizeof
             * phyical memory.
             */
            cMegBootPF = 0;

            if (IsDlgButtonChecked(hDlg, IDD_CDMP_WRITE)) {
                cMegBootPF = -1;
            } else if (IsDlgButtonChecked(hDlg, IDD_CDMP_LOG) ||
                    IsDlgButtonChecked(hDlg, IDD_CDMP_SEND)) {
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
                    SYSTEM_INFO si;

                    // Get the number of Meg in the system, rounding up
                    // (note that we round up a zero remainder)

                    GlobalMemoryStatus(&ms);
                    cMegBootPF = (ms.dwTotalPhys / ONE_MEG) + 1;

                    // If the slop at the end is less than one page, then
                    // make it bigger.

                    GetSystemInfo(&si);

                    if ((cMegBootPF * ONE_MEG - ms.dwTotalPhys) <
                            si.dwPageSize) {
                        cMegBootPF += 1;
                    }
                }

                /*
                 * Check to see if the boot drive page file is big enough
                 */
                if (apf[iBootDrive].nMinFileSize < cMegBootPF) {

                    //We got the OK... Try to make it bigger
                    if (cMegBootPF >= GetFreeSpaceMB(iBootDrive) ) {
                        MyMessageBox(hDlg, SYSTEM+27, INITS+1,
                                MB_ICONEXCLAMATION, cMegBootPF,
                                (TCHAR)(TEXT('A') + iBootDrive));
                        break;
                    }

                    //Its too small, ask if we can make it bigger
                    if (MyMessageBox(hDlg, SYSTEM+28, INITS+1,
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
                        break;
                    }
                }
            }


            /*
             * If the Alert button is checked, make sure the alerter service
             * is started.
             */
            if (IsDlgButtonChecked(hDlg, IDD_CDMP_SEND)) {
                IsAlerterSvcStarted(hDlg);
            }


            fRegChg = CoreDumpUpdateRegistry(hDlg);

            RegCloseKey(hkeyCC);
            RegCloseKey(hkeyMM);

            if (fRegChg)
                iRet = RET_RECOVER_CHANGE;

            EndDialog(hDlg, iRet);
            HourGlass(FALSE);
            break;
        }

        case IDCANCEL:
            RegCloseKey(hkeyMM);
            RegCloseKey(hkeyCC);
            EndDialog(hDlg, RET_NO_CHANGE);
            HourGlass(FALSE);
            break;

        case IDD_CDMP_WRITE: {
            BOOL fChecked = IsDlgButtonChecked(hDlg, IDD_CDMP_WRITE);
            EnableWindow(GetDlgItem(hDlg, IDD_CDMP_FILENAME), fChecked);
            EnableWindow(GetDlgItem(hDlg, IDD_CDMP_OVERWRITE), fChecked);
            break;
        }

        case IDD_HELP:
            goto DoHelp;

        default:
            break;
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
                        LocalFree(acdfControls[i].u.pszValue);

                    break;

                default:
                    break;
                }
            }
        }
        break;

    default:
        if (message == wHelpMessage)
        {
DoHelp:
            CPHelp(hDlg);
        }
        else
            return FALSE;
        break;
    }

    return TRUE;
}

void CoreDumpGetValue(int i) {
    DWORD dwType, dwTemp, cbTemp;
    LPBYTE lpbTemp;
    TCHAR szTemp[MAX_PATH];

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

    if (RegQueryValueEx (hkeyCC, acdfControls[i].pszValueName, NULL,
            &dwType, lpbTemp, &cbTemp) == ERROR_SUCCESS) {

        /*
         * Copy the reg data into the array
         */
        switch( acdfControls[i].dwType) {
        case REG_DWORD:
            acdfControls[i].u.fValue = (BOOL)dwTemp;
            break;

        default:
            acdfControls[i].u.pszValue = CloneString(szTemp);
            break;
        }
    } else {
        /* no reg data, use default values */
        switch( acdfControls[i].dwType) {
        case REG_DWORD:
            acdfControls[i].u.fValue = FALSE;
            break;

        default:
            acdfControls[i].u.pszValue = CloneString(szDefDumpFile);
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
        dwTmp = (DWORD)acdfControls[i].u.fValue;
        lpb = (LPBYTE)&dwTmp;
        cb = sizeof(dwTmp);
        break;

    default:

        lpb = (LPBYTE)(acdfControls[i].u.pszValue);
        cb = (lstrlen((LPTSTR)lpb) + 1) * sizeof(TCHAR);
        break;
    }

    if (RegSetValueEx (hkeyCC, acdfControls[i].pszValueName, 0,
            acdfControls[i].dwType, lpb, cb) != ERROR_SUCCESS) {
        fErr = TRUE;
    }

    return fErr;
}


void CoreDumpInitErrorExit(HWND hDlg, HKEY hk) {
    MyMessageBox(hDlg, SYSTEM+22, INITS+1, MB_ICONEXCLAMATION);
    if( hk != NULL )
        RegCloseKey(hk);

    EndDialog(hDlg, RET_NO_CHANGE);
    HourGlass(FALSE);
    return;
}

void CoreDumpInit(HWND hDlg) {
    int i;
    VCREG_RET vcVirt, vcCore;

    HourGlass(TRUE);

    if( (vcVirt = VirtualOpenKey(&hkeyMM)) == VCREG_ERROR ) {
        CoreDumpInitErrorExit(hDlg, NULL);
        return;
    }

    if (!VirtualGetPageFiles(apf) ) {
        CoreDumpInitErrorExit(hDlg, hkeyMM);
        return;
    }



    vcCore = CoreDumpOpenKey(&hkeyCC);

    if (vcCore == VCREG_ERROR) {
        //  Error - cannot even get the current settings from the reg.
        CoreDumpInitErrorExit(hDlg, hkeyMM);
        return;
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
     * Special case disable the overwrite and logfile controls if the
     * write file check box is not set.
     */
    if (!IsDlgButtonChecked(hDlg, IDD_CDMP_WRITE)) {
        EnableWindow(GetDlgItem(hDlg, IDD_CDMP_FILENAME), FALSE);
        EnableWindow(GetDlgItem(hDlg, IDD_CDMP_OVERWRITE), FALSE);
    }

    HourGlass(FALSE);
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
                LocalFree(acdfControls[i].u.pszValue);
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
        MyMessageBox(hDlg, SYSTEM+24, INITS+1, MB_ICONEXCLAMATION);
    }

    return fRegChanged;
}

LPTSTR CloneString(LPTSTR psz) {
    LPTSTR pszName;

    pszName = LocalAlloc(LPTR, ByteCountOf(lstrlen(psz) + 1));

    if (pszName != NULL)
        lstrcpy(pszName, psz);

    return pszName;
}
