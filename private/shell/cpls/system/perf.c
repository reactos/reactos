/*++

Microsoft Confidential
Copyright (c) 1992-1997  Microsoft Corporation
All rights reserved

Module Name:

    perf.c

Abstract:

    Implements the Performance dialog of the System Control Panel Applet

Author:

    Eric Flo (ericflo) 19-Jun-1995

Revision History:

    15-Oct-1997 scotthal
        Complete overhaul

--*/
#include <sysdm.h>
#include <help.h>

#define PROCESS_PRIORITY_SEPARATION_MASK    0x00000003
#define PROCESS_PRIORITY_SEPARATION_MAX     0x00000002
#define PROCESS_PRIORITY_SEPARATION_MIN     0x00000000

#define PROCESS_QUANTUM_VARIABLE_MASK       0x0000000c
#define PROCESS_QUANTUM_VARIABLE_DEF        0x00000000
#define PROCESS_QUANTUM_VARIABLE_VALUE      0x00000004
#define PROCESS_QUANTUM_FIXED_VALUE         0x00000008
#define PROCESS_QUANTUM_LONG_MASK           0x00000030
#define PROCESS_QUANTUM_LONG_DEF            0x00000000
#define PROCESS_QUANTUM_LONG_VALUE          0x00000010
#define PROCESS_QUANTUM_SHORT_VALUE         0x00000020

#define X86PROG_FILENAME TEXT("x86prog.exe")
//
// Globals
//

HKEY  m_hKeyPerf = NULL;
TCHAR m_szRegPriKey[] = TEXT( "SYSTEM\\CurrentControlSet\\Control\\PriorityControl" );
TCHAR m_szRegPriority[] = TEXT( "Win32PrioritySeparation" );


//
// Help ID's
//

DWORD aPerformanceHelpIds[] = {
    IDC_STATIC,                  NO_HELP,
    IDC_PERF_VM_ALLOCD,          (IDH_PERF + 1),
    IDC_PERF_VM_ALLOCD_LABEL,    (IDH_PERF + 1),
    IDC_PERF_GROUP,              NO_HELP,
    IDC_PERF_TEXT,               (IDH_PERF + 3),
    IDC_PERF_WORKSTATION,        (IDH_PERF + 4),
    IDC_PERF_SERVER,             (IDH_PERF + 5),
    IDC_PERF_VM_GROUP,           NO_HELP,
    IDC_PERF_CHANGE,             (IDH_PERF + 7),
    IDC_PERF_X86PROG_GROUP,      NO_HELP,
    IDC_PERF_X86PROG_ICON,       (IDH_PERF + 8),
    IDC_PERF_X86PROG_TEXT,       (IDH_PERF + 8),
    IDC_PERF_X86PROG_BUTTON,     (IDH_PERF + 9),
    0, 0
};


INT_PTR
APIENTRY 
PerformanceDlgProc(
    IN HWND hDlg, 
    IN UINT uMsg, 
    IN WPARAM wParam, 
    IN LPARAM lParam
)
/*++

Routine Description:

    Handles messages sent to Performance dialog

Arguments:

    hDlg -
        Supplies window handle

    uMsg -
        Supplies message being sent

    wParam -
        Supplies message parameter

    lParam -
        Supplies message parameter

Return Value:

    TRUE if message was handled
    FALSE if message was unhandled

--*/
{
    static int    iNewChoice = 0;
    LONG   RegRes;
    DWORD  Type, Value, Length;
    static int InitPos;
    static int InitRegVal;
    static int NewRegVal;
    static BOOL fVMInited = FALSE;
    static BOOL fTempPfWarningShown = FALSE;
    BOOL fTempPf;
    BOOL fWorkstationProduct = IsWorkstationProduct();
    BOOL fAdministrator = IsUserAdmin();
    BOOL fVariableQuanta = FALSE;
    BOOL fShortQuanta = FALSE;

    switch (uMsg)
    {
    case WM_INITDIALOG:
        InitPos = 0;
        InitRegVal = 0;

        //
        // initialize from the registry
        //

        RegRes = RegOpenKeyEx( HKEY_LOCAL_MACHINE,
                               m_szRegPriKey,
                               0,
                               fAdministrator ? KEY_QUERY_VALUE | KEY_SET_VALUE : KEY_QUERY_VALUE,
                               &m_hKeyPerf );

        if( RegRes == ERROR_SUCCESS ) {
            Length = sizeof( Value );
            RegRes = RegQueryValueEx( m_hKeyPerf,
                                      m_szRegPriority,
                                      NULL,
                                      &Type,
                                      (LPBYTE) &Value,
                                      &Length );

            if( RegRes == ERROR_SUCCESS ) {
                InitRegVal = Value;
                InitPos = InitRegVal & PROCESS_PRIORITY_SEPARATION_MASK;
                if ( InitPos > PROCESS_PRIORITY_SEPARATION_MAX ) {
                    InitPos = PROCESS_PRIORITY_SEPARATION_MAX;
                    }

            }
        } 
        
        if ((RegRes != ERROR_SUCCESS) || (!fAdministrator))  {
            EnableWindow(GetDlgItem(hDlg, IDC_PERF_WORKSTATION), FALSE);
            EnableWindow(GetDlgItem(hDlg, IDC_PERF_SERVER), FALSE);
        }

        NewRegVal = InitRegVal;

        //
        // determine if we are using fixed or variable quantums
        //
        switch ( InitRegVal & PROCESS_QUANTUM_VARIABLE_MASK ) {
            case PROCESS_QUANTUM_VARIABLE_VALUE:
                fVariableQuanta = TRUE;
                break;

            case PROCESS_QUANTUM_FIXED_VALUE:
                fVariableQuanta = FALSE;
                break;

            case PROCESS_QUANTUM_VARIABLE_DEF:
            default:
                if ( fWorkstationProduct ) {
                    fVariableQuanta = TRUE;
                    }
                else {
                    fVariableQuanta = FALSE;
                    }
                break;
            }

        //
        // determine if we are using long or short
        //
        switch ( InitRegVal & PROCESS_QUANTUM_LONG_MASK ) {
            case PROCESS_QUANTUM_LONG_VALUE:
                fShortQuanta = FALSE;
                break;

            case PROCESS_QUANTUM_SHORT_VALUE:
                fShortQuanta = TRUE;
                break;

            case PROCESS_QUANTUM_LONG_DEF:
            default:
                if ( fWorkstationProduct ) {
                    fShortQuanta = TRUE;
                    }
                else {
                    fShortQuanta = FALSE;
                    }
                break;
            }
        
        //
        // Short, Variable Quanta == Workstation-like interactive response
        // Long, Fixed Quanta == Server-like interactive response
        //
        if (fVariableQuanta && fShortQuanta) {
            iNewChoice = PROCESS_PRIORITY_SEPARATION_MAX;

            CheckRadioButton(
                hDlg,
                IDC_PERF_WORKSTATION,
                IDC_PERF_SERVER,
                IDC_PERF_WORKSTATION
            );
        } // if
        else {
            iNewChoice = PROCESS_PRIORITY_SEPARATION_MIN;

            CheckRadioButton(
                hDlg,
                IDC_PERF_WORKSTATION,
                IDC_PERF_SERVER,
                IDC_PERF_SERVER
            );
        } // else

        //
        // Init the virtual memory part
        //
        if (VirtualInitStructures()) {
            fVMInited = TRUE;
            SetDlgItemMB( hDlg, IDC_PERF_VM_ALLOCD, VirtualMemComputeAllocated(hDlg, &fTempPf) );
            //
            // If the system created a temp pagefile, warn the user that
            // the total pagefile size may appear a bit large, but only
            // do so once per System Applet invokation.
            //
            if (fTempPf && !fTempPfWarningShown) {
                MsgBoxParam(
                    hDlg,
                    IDS_TEMP_PAGEFILE_WARN,
                    INITS + 1,
                    MB_ICONINFORMATION | MB_OK
                );
                fTempPfWarningShown = TRUE;
            } // if (fTempPf...
        }
        break;

    case WM_DESTROY:
        //
        // If the dialog box is going away, then close the
        // registry key.
        //


        if (m_hKeyPerf) {
            RegCloseKey( m_hKeyPerf );
            m_hKeyPerf = NULL;
        }

        if (fVMInited) {
            VirtualFreeStructures();
        }
        break;

    case WM_COMMAND: {
        LRESULT lres;

        switch (LOWORD(wParam)) {
            case IDOK:
                // 
                // Save new time quantum stuff, if it has changed
                //
                NewRegVal &= ~PROCESS_PRIORITY_SEPARATION_MASK;
                NewRegVal |= iNewChoice;
    
                if ( NewRegVal != InitRegVal ) {
    
                    Value = NewRegVal;
    
                    if( m_hKeyPerf )
                    {
                        Type = REG_DWORD;
                        Length = sizeof( Value );
                        RegSetValueEx( m_hKeyPerf,
                                       m_szRegPriority,
                                       0,
                                       REG_DWORD,
                                       (LPBYTE) &Value,
                                       Length );
                        InitRegVal = Value;
    
                        //
                        // Kernel monitors this part of the 
                        // registry, so don't tell user he has to reboot
                        //
                    }
                }
    
                EndDialog(hDlg, 0);
                break;

            case IDCANCEL:
                EndDialog(hDlg, 0);
                break;

            case IDC_PERF_CHANGE: {
                lres = DialogBox(
                    hInstance, 
                    MAKEINTRESOURCE(DLG_VIRTUALMEM),
                    hDlg, 
                    VirtualMemDlg
                );

                if (fVMInited) {
                    SetDlgItemMB( 
                        hDlg, 
                        IDC_PERF_VM_ALLOCD, 
                        VirtualMemComputeAllocated(hDlg, NULL) 
                    );
                }
                if ((lres != RET_NO_CHANGE) && (lres != RET_CHANGE_NO_REBOOT)) {
                    MsgBoxParam(
                        hDlg,
                        SYSTEM + 42,
                        INITS + 1,
                        MB_OK | MB_ICONINFORMATION
                    );

                    g_fRebootRequired = TRUE;
                }
                }
                break;
            
            case IDC_PERF_WORKSTATION:
                if (BN_CLICKED == HIWORD(wParam)) {
                    //
                    // Workstations have maximum foreground boost
                    //
                    iNewChoice = PROCESS_PRIORITY_SEPARATION_MAX;

                    //
                    // Workstations have variable, short quanta
                    NewRegVal &= ~PROCESS_QUANTUM_VARIABLE_MASK;
                    NewRegVal |= PROCESS_QUANTUM_VARIABLE_VALUE;
                    NewRegVal &= ~PROCESS_QUANTUM_LONG_MASK;
                    NewRegVal |= PROCESS_QUANTUM_SHORT_VALUE;
                } // if    
                break;

            case IDC_PERF_SERVER:
                if (BN_CLICKED == HIWORD(wParam)) {
                    //
                    // Servers have minimum foreground boost
                    //
                    iNewChoice = PROCESS_PRIORITY_SEPARATION_MIN;

                    //
                    // Servers have fixed, long quanta
                    //
                    NewRegVal &= ~PROCESS_QUANTUM_VARIABLE_MASK;
                    NewRegVal |= PROCESS_QUANTUM_FIXED_VALUE;
                    NewRegVal &= ~PROCESS_QUANTUM_LONG_MASK;
                    NewRegVal |= PROCESS_QUANTUM_LONG_VALUE;
                } // if
                break;

            case IDC_PERF_X86PROG_BUTTON: 
                if (BN_CLICKED == HIWORD(wParam)) {
                    SHELLEXECUTEINFO sei;

                    memset(&sei, 0, sizeof(SHELLEXECUTEINFO));

                    sei.cbSize = sizeof(SHELLEXECUTEINFO);
                    sei.hwnd = hDlg;
                    sei.lpVerb = TEXT("Open");
                    sei.lpFile = X86PROG_FILENAME;
                    sei.nShow = SW_SHOW;

                    ShellExecuteEx(&sei);
                } // if
                break;

            default: {
                break;
                }
            }
        }
        break;

    case WM_HELP:      // F1
        WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, HELP_FILE, HELP_WM_HELP,
        (DWORD_PTR) (LPSTR) aPerformanceHelpIds);
        break;

    case WM_CONTEXTMENU:      // right mouse click
        WinHelp((HWND) wParam, HELP_FILE, HELP_CONTEXTMENU,
        (DWORD_PTR) (LPSTR) aPerformanceHelpIds);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}
