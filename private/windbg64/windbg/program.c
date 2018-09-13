/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Program.c

Abstract:

    This module contains the support for Windbg's program menu.

Author:

    Ramon J. San Andres (ramonsa)  07-July-1992

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop


#include "include\cntxthlp.h"

#define DEFAULT_WORKSPACE   "Common Workspace"
#define CURRENT_WORKSPACE   "Current Workspace"


//
//  Workspace marked to be deleted.
//
typedef struct _WORKSPACE_TO_DELETE *PWORKSPACE_TO_DELETE;
typedef struct _WORKSPACE_TO_DELETE {
    PWORKSPACE_TO_DELETE    Next;
    char                    WorkSpace[ MAX_PATH ];
} WORKSPACE_TO_DELETE;


//
//  Program marked to be deleted.
//
typedef struct _PROGRAM_TO_DELETE *PPROGRAM_TO_DELETE;
typedef struct _PROGRAM_TO_DELETE {
    PPROGRAM_TO_DELETE      Next;
    PWORKSPACE_TO_DELETE    WorkSpaceToDelete;
    BOOL                    DeleteAll;
    char                    ProgramName[ MAX_PATH ];
} PROGRAM_TO_DELETE;


//
//  External variables
//
extern BOOL AutoTest;


//
//  Exported variables
//
BOOL                ExitingDebugger = FALSE;

//
//  Global variables, for communication with dialogs.
//
BOOL                 DialogCancelled;
PPROGRAM_TO_DELETE   ProgramToDeleteHead = NULL;




//
//  Local prototypes
//
VOID FreeProgramToDelete ( VOID );
BOOL ProgramHookProc (HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam);


//  **********************************************************
//                          DIALOGS
//  **********************************************************
BOOL 
ProgramHookProc(
    HWND hDlg, 
    UINT message, 
    WPARAM wParam, 
    LPARAM lParam
    )
{
    static LPCHOOSEFONT    Cf;
    
    switch( message ) {
        
    case WM_COMMAND:
        
        switch( LOWORD( wParam )) {
            
        case IDWINDBGHELP:
        case pshHelp:
            
            Dbg(WinHelp(hDlg, szHelpFileName, (DWORD) HELP_CONTEXT,(DWORD)ID_PROGOPEN_NEW_HELP));
            return TRUE;
            
        }
    }
    return FALSE;
}













//  **********************************************************
//                        HELPER FUNCTIONS
//  **********************************************************






VOID
FreeProgramToDelete (
    VOID
    )
/*++

Routine Description:

    Frees up memory used by the ProgramToDelete list

Arguments:

    None

Return Value:

    None

--*/
{
    PPROGRAM_TO_DELETE      ProgramToDelete;
    PPROGRAM_TO_DELETE      ProgramToDeleteTmp;
    PWORKSPACE_TO_DELETE    WorkSpaceToDelete;
    PWORKSPACE_TO_DELETE    WorkSpaceToDeleteTmp;

    //
    //  Free all memory used by the deletion structures.
    //
    ProgramToDelete = ProgramToDeleteHead;
    while ( ProgramToDelete ) {
        //
        //  Free all workspace memory
        //
        WorkSpaceToDelete = ProgramToDelete->WorkSpaceToDelete;
        while ( WorkSpaceToDelete ) {
            WorkSpaceToDeleteTmp = WorkSpaceToDelete;
            WorkSpaceToDelete    = WorkSpaceToDelete->Next;
            free( WorkSpaceToDeleteTmp );
        }

        ProgramToDeleteTmp  = ProgramToDelete;
        ProgramToDelete     = ProgramToDelete->Next;
        free( ProgramToDeleteTmp );
    }
}

