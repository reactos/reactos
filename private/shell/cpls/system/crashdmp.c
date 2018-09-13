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

--*/

#include "sysdm.h"
#include <windowsx.h>

#define KBYTE (1024UI64)
#define MBYTE (1024UI64 * KBYTE)
#define GBYTE (1024UI64 * MBYTE)

//
// CrashDumpEnabled is not a boolean value anymore. It can take on one of the
// following types.
//

#define DUMP_TYPE_NONE              (0)
#define DUMP_TYPE_MINI              (1)
#define DUMP_TYPE_SUMMARY           (2)
#define DUMP_TYPE_FULL              (3)
#define DUMP_TYPE_MAX               (4)

#define REG_LOG_EVENT_VALUE_NAME    TEXT ("LogEvent")
#define REG_SEND_ALERT_VALUE_NAME   TEXT ("SendAlert")
#define REG_OVERWRITE_VALUE_NAME    TEXT ("Overwrite")
#define REG_AUTOREBOOT_VALUE_NAME   TEXT ("AutoReboot")
#define REG_DUMPFILE_VALUE_NAME     TEXT ("DumpFile")
#define REG_MINIDUMP_DIR_VALUE_NAME TEXT ("MinidumpDir")
#define REG_DUMP_TYPE_VALUE_NAME    TEXT ("CrashDumpEnabled")

#define BIG_MEMORY_MAX_BOOT_PF_MB   (2048)
#define CRASH_CONTROL_KEY           TEXT("System\\CurrentControlSet\\Control\\CrashControl")

//
// The crashdump code is hard-coded to generate only summary dumps for
// machines with more than 2 GB of physical memory. Do not change this
// constant unless unless you change the same code in ntos\io\dumpctl.c
//

#define LARGE_MEMORY_THRESHOLD      (2 * GBYTE)

typedef struct _SYSTEM_MEMORY_CONFIGURATION {
    BOOL    BigMemory;
    ULONG   PageSize;
    ULONG64 PhysicalMemorySize;
    ULONG64 BootPartitionPageFileSize;
    TCHAR   BootDrive;
} SYSTEM_MEMORY_CONFIGURATION;

VCREG_RET gvcCrashCtrl =  VCREG_ERROR;
HKEY ghkeyCrashCtrl = NULL;
int  gcrefCrashCtrl = 0;
BOOL gfCoreDumpChanged = FALSE;

TCHAR CrashDumpFile [MAX_PATH] = TEXT("%SystemRoot%\\MEMORY.DMP");
TCHAR MiniDumpDirectory [MAX_PATH] = TEXT("%SystemRoot%\\Minidump");
TCHAR DumpFileText [100];
TCHAR MiniDumpDirText [100];

SYSTEM_MEMORY_CONFIGURATION SystemMemoryConfiguration;

//
// Private function prototypes
//

NTSTATUS
GetMemoryConfiguration(
    OUT SYSTEM_MEMORY_CONFIGURATION * MemoryConfig
    );

VOID
DisableCoreDumpControls(
    HWND hDlg
    );

static
BOOL
CoreDumpInit(
    IN HWND hDlg
    );

static
BOOL
CoreDumpUpdateRegistry(
    IN HWND hDlg,
    IN HKEY hKey
    );

int
CoreDumpHandleOk(
    IN BOOL fInitialized,
    IN HWND hDlg,
    IN WPARAM wParam,
    IN LPARAM lParam
    );


VOID
SwapDumpSelection(
    HWND hDlg
    );
//
// Implementation
//

VCREG_RET
CoreDumpOpenKey(
    )
{
    if (gvcCrashCtrl == VCREG_ERROR) {
        gvcCrashCtrl = OpenRegKey( CRASH_CONTROL_KEY, &ghkeyCrashCtrl );
    }

    if (gvcCrashCtrl != VCREG_ERROR) {
        gcrefCrashCtrl++;
    }

    return gvcCrashCtrl;
}

void
CoreDumpCloseKey(
    )
{
    if (gcrefCrashCtrl > 0) {
        gcrefCrashCtrl--;
        if (gcrefCrashCtrl == 0) {
            CloseRegKey( ghkeyCrashCtrl );
            gvcCrashCtrl = VCREG_ERROR;
        }
    }
}




BOOL
StartAlerterService(
    IN SC_HANDLE hAlerter
    )
{
    BOOL fResult = FALSE;

    fResult = ChangeServiceConfig(
        hAlerter,
        SERVICE_NO_CHANGE,
        SERVICE_AUTO_START,
        SERVICE_NO_CHANGE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL
    );

    fResult = StartService(hAlerter, 0, NULL);

    return(fResult);

}

BOOL
IsAlerterSvcStarted(
    HWND hDlg
    )
{
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
         SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_CHANGE_CONFIG | SERVICE_START
    );

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
        fRunning = StartAlerterService(schService);
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


BOOL
CoreDumpValidFile(
    HWND hDlg
    )
{

    //
    // If this is a minidump, fine. Otherwise, check if the path is
    // valid.
    //

#if 0
    TCHAR szPath[MAX_PATH];
    TCHAR szExpPath[MAX_PATH];
    LPTSTR psz;
    TCHAR ch;
    UINT uType;


    if (IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_WRITE)) {
        /*
         * get the filenamecored
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
#endif


    return TRUE;
}


int
APIENTRY
CoreDumpDlgProc(
    HWND hDlg,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
    )
{
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
                    VirtualCloseKey();
                    CoreDumpCloseKey();
                }
                // Let the Startup/Recovery dlg proc also handle IDOK
                return(RET_NO_CHANGE);
                break;

            case IDC_STARTUP_CDMP_TYPE: {
                SwapDumpSelection (hDlg);
            }
            // Fall through

            case IDC_STARTUP_CDMP_FILENAME:
            case IDC_STARTUP_CDMP_LOG:
            case IDC_STARTUP_CDMP_SEND:
            case IDC_STARTUP_CDMP_OVERWRITE:
            case IDC_STARTUP_CDMP_AUTOREBOOT:
                if (!g_fStartupInitializing) {
                    gfCoreDumpChanged = TRUE;
                }
                break;
            default: {
                // indicat not handled
                return RET_CONTINUE;
            }
        }
        break; // WM_COMMAND

    case WM_DESTROY:
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
    NTSTATUS Status;
    DWORD Ret;
    int iRet = RET_NO_CHANGE;
    SYSTEM_MEMORY_CONFIGURATION MemoryConfig;

    if (fInitialized && gfCoreDumpChanged) {

        //
        // Validate crashdump file name.
        //

        if (!CoreDumpValidFile(hDlg)) {
            SetFocus(GetDlgItem(hDlg, IDC_STARTUP_CDMP_FILENAME));
            SetWindowLongPtr (hDlg, DWLP_MSGRESULT, PSNRET_INVALID_NOCHANGEPAGE);
            iRet = RET_ERROR;
            return iRet;
        }


        Status = GetMemoryConfiguration (&MemoryConfig);

        if (NT_SUCCESS (Status) &&
            MemoryConfig.BootPartitionPageFileSize <
            CoreDumpGetRequiredFileSize (hDlg)) {

            //
            // Warn that the dump file may be truncated.
            //

            Ret = MsgBoxParam (hDlg,
                               SYSTEM + 29,
                               INITS+1,
                               MB_ICONEXCLAMATION | MB_YESNO,
                               MemoryConfig.BootDrive,
                               (DWORD) (CoreDumpGetRequiredFileSize (hDlg) / MBYTE)
                               );

            if (Ret == IDNO) {
                return RET_ERROR;
            }
        }

        //
        // If the Alert button is checked, make sure the alerter service
        // is started.
        //

        if (IsDlgButtonChecked(hDlg, IDC_STARTUP_CDMP_SEND)) {
            IsAlerterSvcStarted(hDlg);
        }

        fRegChg = CoreDumpUpdateRegistry (hDlg, ghkeyCrashCtrl);

        //
        // Clean up registry stuff
        //
        CoreDumpCloseKey();
        VirtualCloseKey();

        if (fRegChg) {
            iRet = RET_RECOVER_CHANGE;
        }
    } else {
        iRet = RET_NO_CHANGE;
    }

    return(iRet);
}

void
CoreDumpInitErrorExit(
    HWND hDlg,
    HKEY hk
    )
{
    MsgBoxParam(hDlg, SYSTEM+22, INITS+1, MB_ICONEXCLAMATION);
    if( hk == ghkeyMemMgt )
        VirtualCloseKey();

    DisableCoreDumpControls(hDlg);

    HourGlass(FALSE);
    return;
}


DWORD
GetDumpSelection(
    HWND hDlg
    )
{
    HWND hControl;

    hControl = GetDlgItem (hDlg, IDC_STARTUP_CDMP_TYPE);
    return ComboBox_GetCurSel ( hControl );
}

VOID
DisableCoreDumpControls(
    HWND hDlg
    )
{
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_GRP), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_TXT1), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_LOG ), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_SEND), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_TYPE), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_OVERWRITE), FALSE);
    EnableWindow( GetDlgItem (hDlg, IDC_STARTUP_CDMP_AUTOREBOOT), FALSE);
}

VOID
SwapDumpSelection(
    HWND hDlg
    )
{
    //
    // If there is no dump type, disable some controls. If this is a minidump
    // disable overwrite and change "File Name:" to "Mini Dump Directory:"
    //

    switch (GetDumpSelection (hDlg)) {

        case DUMP_TYPE_NONE:
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_OVERWRITE), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILE_LABEL), FALSE);
            SetWindowText (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME),
                           CrashDumpFile
                           );
            Static_SetText (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILE_LABEL),
                            DumpFileText
                            );
            break;

        case DUMP_TYPE_MINI:
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_OVERWRITE), FALSE);
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME), TRUE);
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILE_LABEL), TRUE);
            SetWindowText (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME),
                           MiniDumpDirectory
                           );
            Static_SetText (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILE_LABEL),
                            MiniDumpDirText
                            );
            break;

        default:
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_OVERWRITE), TRUE);
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME), TRUE);
            EnableWindow (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILE_LABEL), TRUE);
            SetWindowText (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILENAME),
                           CrashDumpFile
                           );
            Static_SetText (GetDlgItem (hDlg, IDC_STARTUP_CDMP_FILE_LABEL),
                             DumpFileText
                             );
    }
}

BOOL
GetSystemDrive(
    OUT TCHAR * Drive
    )
{
    TCHAR WindowsDir [ MAX_PATH ];

    if (!GetWindowsDirectory (WindowsDir, sizeof (WindowsDir))) {
        return FALSE;
    }

    if (!isalpha (*WindowsDir)) {
        return FALSE;
    }

    *Drive = *WindowsDir;

    return TRUE;
}

NTSTATUS
GetMemoryConfiguration(
    OUT SYSTEM_MEMORY_CONFIGURATION * MemoryConfig
    )
{
    BOOL Succ;
    TCHAR SystemDrive;
    NTSTATUS Status;
    SYSTEM_BASIC_INFORMATION BasicInfo;

    Status = NtQuerySystemInformation(
                        SystemBasicInformation,
                        &BasicInfo,
                        sizeof (BasicInfo),
                        NULL
                        );

    if (NT_SUCCESS (Status)) {
        Status;
    }

    MemoryConfig->PhysicalMemorySize =
            (ULONG64) BasicInfo.NumberOfPhysicalPages *
            (ULONG64) BasicInfo.PageSize;

    MemoryConfig->PageSize = BasicInfo.PageSize;

    //
    // A big memory machine is one that has more memory than we can write to
    // at crashdump time.
    //

    if (MemoryConfig->PhysicalMemorySize >= MAX_SWAPSIZE) {
        MemoryConfig->BigMemory = TRUE;
    } else {
        MemoryConfig->BigMemory = FALSE;
    }

    //
    // Get the Boot-partition pagefile size.
    //

    Succ = GetSystemDrive (&SystemDrive);

    if (!Succ) {
        return FALSE;
    }

    MemoryConfig->BootDrive = (WCHAR) toupper (SystemDrive);

    SystemDrive = tolower (SystemDrive) - 'a';

    //
    // NOTE: apf is a global exposed by virtual.c
    //

    Succ = VirtualGetPageFiles ( apf );

    if (!Succ) {
        return FALSE;
    }

    //
    // This is the file size in terms of megabytes.
    //

    MemoryConfig->BootPartitionPageFileSize = apf [ SystemDrive ].nMinFileSize;

    //
    // Convert to bytes.
    //

    MemoryConfig->BootPartitionPageFileSize *= MBYTE;

    VirtualFreePageFiles ( apf );

    return STATUS_SUCCESS;

}

BOOL
CheckInitFromRegistry(
    IN HWND hDlg,
    IN DWORD ControlId,
    IN HKEY RegKey,
    IN LPTSTR ValueName,
    IN BOOL Default
    )
{
    BOOL Succ;
    DWORD Type;
    DWORD Data;
    BOOL DataSize;
    BOOL Value;

    DataSize = sizeof (Data);

    Succ = RegQueryValueEx (
                     RegKey,
                     ValueName,
                     NULL,
                     &Type,
                     (LPBYTE) &Data,
                     &DataSize
                     );

    if (Succ != ERROR_SUCCESS || Type != REG_DWORD) {
        Value = Default;
    } else {
        Value = Data ? TRUE : FALSE;
    }

    return CheckDlgButton (hDlg, ControlId, Value);
}


BOOL
ComboAddStringFromResource(
    IN HWND hDlg,
    IN DWORD ControlId,
    IN HINSTANCE ModuleHandle,
    IN DWORD ResourceId,
    IN DWORD ItemData
    )
{
    DWORD Res;
    DWORD Item;
    HWND hControl;
    DWORD Result;
    WCHAR Buffer [ 512 ];

    Res = LoadString (
                ModuleHandle,
                ResourceId,
                Buffer,
                sizeof (Buffer)
                );

    if (Res == 0) {
        return FALSE;
    }

    hControl = GetDlgItem (hDlg, ControlId);
    Item = ComboBox_InsertString (hControl, -1, Buffer);
    ComboBox_SetItemData (hControl, Item, ItemData);

    return TRUE;
}


BOOL
StoreCheckboxToReg(
    IN HWND hDlg,
    IN DWORD ControlId,
    IN HKEY hKey,
    IN LPCTSTR RegValueName
    )
{
    DWORD Checked;

    Checked = IsDlgButtonChecked (hDlg, ControlId);

    RegSetValueEx(
            hKey,
            RegValueName,
            0,
            REG_DWORD,
            (LPBYTE) &Checked,
            sizeof (Checked)
            );

    return TRUE;
}


BOOL
StoreStringToReg(
    IN HWND hDlg,
    IN DWORD ControlId,
    IN HKEY hKey,
    IN LPCTSTR RegValueName
    )
{
    TCHAR Buffer [ MAX_PATH ];

    GetDlgItemText (hDlg, ControlId, Buffer, ARRAYSIZE (Buffer));

    //
    // Check the buffer for valid file-name??
    //

    RegSetValueEx (
            hKey,
            RegValueName,
            0,
            REG_EXPAND_SZ,
            (LPBYTE) Buffer,
            (wcslen (Buffer) + 1) * sizeof (TCHAR)
            );

    return TRUE;
}

static DWORD SelectionToType [] = { 0, 3, 2, 1 };

DWORD
GetDumpTypeFromRegistry(
    HKEY Key
    )
{
    DWORD DataSize;
    DWORD Type;
    DWORD DumpType;

    DataSize = sizeof (DWORD);
    RegQueryValueEx (
                    Key,
                    REG_DUMP_TYPE_VALUE_NAME,
                    NULL,
                    &Type,
                    (LPBYTE) &DumpType,
                    &DataSize
                    );

    if (DumpType > 3) {
        DumpType = DUMP_TYPE_MINI;
    } else {
        DumpType = SelectionToType [ DumpType ];
    }

    return DumpType;
}


BOOL
CoreDumpInit(
    HWND hDlg
    )
{
    BOOL Succ;
    NTSTATUS Status;
    DWORD DataSize;
    DWORD DumpType;
    DWORD Type;
    VCREG_RET vcVirt;
    VCREG_RET vcCore;
    SYSTEM_MEMORY_CONFIGURATION MemoryConfig;

    HourGlass (TRUE);

    //
    // Do no put anything before the initialization of the globals, here.
    //
    
    vcVirt = VirtualOpenKey();

    if( vcVirt == VCREG_ERROR ) {
        CoreDumpInitErrorExit(hDlg, NULL);
        return FALSE;
    }

    vcCore = CoreDumpOpenKey();

    if (vcCore == VCREG_ERROR) {

        CoreDumpInitErrorExit(hDlg, ghkeyMemMgt);
        return FALSE;

    } else if (vcCore == VCREG_READONLY || vcVirt == VCREG_READONLY) {

        DisableCoreDumpControls (hDlg);

    } else {

        Status = GetMemoryConfiguration (&SystemMemoryConfiguration);
        if (!NT_SUCCESS (Status)) {
            return FALSE;
        }
    }

    Status = GetMemoryConfiguration (&MemoryConfig);
    if (!NT_SUCCESS (Status)) {
        return FALSE;
    }

    Succ = LoadString (hInstance,
                       IDS_CRASHDUMP_DUMP_FILE,
                       DumpFileText,
                       sizeof (DumpFileText)
                       );

    Succ = LoadString (hInstance,
                       IDS_CRASHDUMP_MINI_DIR,
                       MiniDumpDirText,
                       sizeof (MiniDumpDirText)
                       );

    //
    // Special Case: Server Product does not want ability to disable logging
    // of crashdumps.
    //
    
    if (IsWorkstationProduct ()) {

        CheckInitFromRegistry(
                    hDlg,
                    IDC_STARTUP_CDMP_LOG,
                    ghkeyCrashCtrl,
                    REG_LOG_EVENT_VALUE_NAME,
                    TRUE
                    );
    } else {

        CheckDlgButton (hDlg, IDC_STARTUP_CDMP_LOG, TRUE);
        EnableWindow ( GetDlgItem (hDlg, IDC_STARTUP_CDMP_LOG), FALSE);
    }

    CheckInitFromRegistry(
                hDlg,
                IDC_STARTUP_CDMP_SEND,
                ghkeyCrashCtrl,
                REG_SEND_ALERT_VALUE_NAME,
                TRUE
                );

    CheckInitFromRegistry(
                hDlg,
                IDC_STARTUP_CDMP_OVERWRITE,
                ghkeyCrashCtrl,
                REG_OVERWRITE_VALUE_NAME,
                TRUE
                );

    CheckInitFromRegistry(
                hDlg,
                IDC_STARTUP_CDMP_AUTOREBOOT,
                ghkeyCrashCtrl,
                REG_AUTOREBOOT_VALUE_NAME,
                TRUE
                );

    ComboAddStringFromResource (
                        hDlg,
                        IDC_STARTUP_CDMP_TYPE,
                        hInstance,                  // Global hInstance
                        IDS_CRASHDUMP_NONE,
                        0
                        );

    ComboAddStringFromResource (
                        hDlg,
                        IDC_STARTUP_CDMP_TYPE,
                        hInstance,
                        IDS_CRASHDUMP_MINI,
                        0
                        );

    ComboAddStringFromResource (
                        hDlg,
                        IDC_STARTUP_CDMP_TYPE,
                        hInstance,
                        IDS_CRASHDUMP_SUMMARY,
                        0
                        );

    //
    // Special case: Server Products do not allow full memory dumps.
    //
    
    DumpType = GetDumpTypeFromRegistry (ghkeyCrashCtrl);

    if ( MemoryConfig.PhysicalMemorySize < LARGE_MEMORY_THRESHOLD ) {
        
        ComboAddStringFromResource (
                            hDlg,
                            IDC_STARTUP_CDMP_TYPE,
                            hInstance,
                            IDS_CRASHDUMP_FULL,
                            0
                            );

    } else {

        if (DumpType == DUMP_TYPE_FULL) {
            DumpType = DUMP_TYPE_SUMMARY;
        }
    }

    ComboBox_SetCurSel (
                GetDlgItem (hDlg, IDC_STARTUP_CDMP_TYPE),
                DumpType
                );

    DataSize = sizeof (CrashDumpFile);
    RegQueryValueEx (
                    ghkeyCrashCtrl,
                    REG_DUMPFILE_VALUE_NAME,
                    NULL,
                    &Type,
                    (LPBYTE) CrashDumpFile,
                    &DataSize
                    );

    DataSize = sizeof (MiniDumpDirectory);
    RegQueryValueEx (
                    ghkeyCrashCtrl,
                    REG_MINIDUMP_DIR_VALUE_NAME,
                    NULL,
                    &Type,
                    (LPBYTE) MiniDumpDirectory,
                    &DataSize
                    );


    //
    // Update the selection fields of the dialog.
    //

    SwapDumpSelection (hDlg);

    HourGlass(FALSE);

    return TRUE;
}



BOOL
CoreDumpUpdateRegistry(
    HWND hDlg,
    HKEY hKey
    )
{
    DWORD Selection;

    StoreCheckboxToReg(
                hDlg,
                IDC_STARTUP_CDMP_LOG,
                hKey,
                REG_LOG_EVENT_VALUE_NAME
                );

    StoreCheckboxToReg(
                hDlg,
                IDC_STARTUP_CDMP_SEND,
                hKey,
                REG_SEND_ALERT_VALUE_NAME
                );

    StoreCheckboxToReg(
                hDlg,
                IDC_STARTUP_CDMP_OVERWRITE,
                hKey,
                REG_OVERWRITE_VALUE_NAME
                );

    StoreCheckboxToReg(
                hDlg,
                IDC_STARTUP_CDMP_AUTOREBOOT,
                hKey,
                REG_AUTOREBOOT_VALUE_NAME
                );

    Selection = GetDumpSelection (hDlg);

    if (Selection == DUMP_TYPE_MINI) {

        StoreStringToReg (
                    hDlg,
                    IDC_STARTUP_CDMP_FILENAME,
                    hKey,
                    REG_MINIDUMP_DIR_VALUE_NAME
                    );

    } else {

        StoreStringToReg(
                    hDlg,
                    IDC_STARTUP_CDMP_FILENAME,
                    hKey,
                    REG_DUMPFILE_VALUE_NAME
                    );
    }


    if (Selection > 3) {
        Selection = 3;
    }

    Selection = SelectionToType [ Selection ];
    RegSetValueEx (
            hKey,
            REG_DUMP_TYPE_VALUE_NAME,
            0,
            REG_DWORD,
            (LPBYTE) &Selection,
            sizeof (Selection)
            );

    return TRUE;
}


ULONG64
EstimateSummaryDumpSize(
    ULONG64 PhysicalMemorySize
    )
{
    ULONG64 Size;

    //
    // Very rough guesses at the size of the summary dump.
    //

    if (PhysicalMemorySize < 128 * MBYTE) {

        Size = 50 * MBYTE;

    } else if (PhysicalMemorySize < 4 * GBYTE) {

        Size = 200 * MBYTE;

    } else if (PhysicalMemorySize < 8 * GBYTE) {

        Size = 400 * MBYTE;

    } else {

        Size = 800 * MBYTE;
    }

    return Size;
}


ULONG64
CoreDumpGetRequiredFileSize(
    IN HWND hDlg OPTIONAL
    )
{
    ULONG64 Size;
    DWORD DumpType;
    NTSTATUS Status;
    SYSTEM_MEMORY_CONFIGURATION MemoryConfig;


    //
    // If we were passed a hDlg, get the selection from the dlg. Otherwise,
    // get the selection from the registry.
    //

    if (hDlg != NULL) {

        //
        // Get selection from dlg.
        //

        DumpType = GetDumpSelection ( hDlg );

    } else {

        HKEY hKey;
        DWORD Err;

        //
        // Get selection from registry.
        //

        Err = OpenRegKey (CRASH_CONTROL_KEY,
                          &hKey
                          );

        if (Err == VCREG_ERROR) {
            return DUMP_TYPE_MINI;
        }

        ASSERT ( hKey );
        DumpType = GetDumpTypeFromRegistry ( hKey );
        CloseRegKey ( hKey );
    }

    switch (DumpType) {

        case DUMP_TYPE_NONE:
            Size = 0;
            break;

        case DUMP_TYPE_MINI:
            Size = 64 * KBYTE;
            break;

        case DUMP_TYPE_SUMMARY:

            Status = GetMemoryConfiguration (&MemoryConfig);

            if (NT_SUCCESS (Status)) {
                Size = EstimateSummaryDumpSize (MemoryConfig.PhysicalMemorySize);
            } else {
                //
                // A (large) shot in the dark.
                //
                Size = 800 * MBYTE;
            }
            break;

        case DUMP_TYPE_FULL:

            Status = GetMemoryConfiguration (&MemoryConfig);

            if (NT_SUCCESS (Status)) {
                Size = MemoryConfig.PhysicalMemorySize;
            } else {
                Size = 0;
            }

        break;

        default:
            ASSERT (FALSE);
    }

    return Size;
}


