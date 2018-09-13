/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    callswin.c

Abstract:

    This module contains the main line code for display of calls window.

Author:

    Wesley Witt (wesw) 6-Sep-1993

Environment:

    Win32, User Mode

--*/


#include "precomp.h"
#pragma hdrstop

#define MAX_TASKS ((1024-sizeof(IOCTLGENERIC))/sizeof(TASK_LIST))


#include "include\cntxthlp.h"


// kcarlos
// BUGBUG - In a hurry.
// BUGBUG - copy/pasted code. Place in a function and cleanup. Remove code duplication.
DWORD
GetProcessIdGivenName(
    PCSTR pszProcessName
    )
{
    DWORD           i;
    CHAR            buf[80];
    LPSTR           p;
    PIOCTLGENERIC   pig;
    PTASK_LIST      pTask;
    LPSTR           fmt;
    DWORD           dw;
    BOOL            fReconnecting;

    if (ConnectDebugger(&fReconnecting)  &&
        NULL != (pig = (PIOCTLGENERIC) malloc((sizeof(TASK_LIST)*MAX_TASKS) +
        sizeof(IOCTLGENERIC)))) {
        
        //Load exclusion list
        char szExclusionList[MAX_MSG_TXT];
        Dbg(LoadString(g_hInst, IDS_PROCESS_EXCLUSION_LIST, szExclusionList, sizeof(szExclusionList)));
        
        ZeroMemory( pig, (sizeof(TASK_LIST)*MAX_TASKS) + sizeof(IOCTLGENERIC) );
        pig->ioctlSubType = IG_TASK_LIST;
        pig->length = sizeof(TASK_LIST)*MAX_TASKS;
        pTask = (PTASK_LIST)pig->data;
        pTask->dwProcessId = MAX_TASKS;
        OSDSystemService( LppdCur->hpid,
            NULL,
            (SSVC) ssvcGeneric,
            (LPV)pig,
            pig->length + sizeof(IOCTLGENERIC),
            &dw
            );
        for (i=0; i<MAX_TASKS; i++) {
            LPSTR lpszExclude = szExclusionList;
            BOOL bExclude = FALSE;
            
            if (pTask[i].dwProcessId == 0) {
                break;
            }
            if (pTask[i].dwProcessId == (DWORD)-2) {
                continue;
            }
            
            while (*lpszExclude) {
                if (!_stricmp(pTask[i].ProcessName, lpszExclude)) {
                    // Exclude from the displayed list
                    bExclude = TRUE;
                    break;
                }
                // Move to the next string.
                lpszExclude += strlen(lpszExclude) +1;
            }
            
            if (bExclude) {
                continue;
            }
            
            {
                char szFName[_MAX_FNAME] = {0};
                char szExt[_MAX_EXT] = {0};

                PSTR pszDot = (PSTR) strchr((PUCHAR) pTask[i].ProcessName, '.');
                if (pszDot) {
                    strncpy(szFName, pTask[i].ProcessName, (size_t) (pszDot - pTask[i].ProcessName));
                    strcpy(szExt, pszDot);
                } else {
                    strcpy(szFName, pTask[i].ProcessName);
                }

                if (!_stricmp(pszProcessName, pTask[i].ProcessName) 
                    || !_stricmp(pszProcessName, szFName)) {
                        
                    DWORD dw = pTask[i].dwProcessId;
                    free(pig);
                    return dw;
                }
            }

        }
        free( pig );
    }

    return 0;
}



INT_PTR
CALLBACK
DlgTaskList(
    HWND   hDlg,
    UINT   message,
    WPARAM wParam,
    LPARAM lParam
    )

/*++

Routine Description:

    Dialog procedure for the calls stack options dialog.

Arguments:

    hwnd       - window handle
    msg        - message number
    wParam     - first message parameter
    lParam     - second message parameter

Return Value:

    TRUE       - did not process the message
    FALSE      - did process the message

--*/

{
    DWORD           i;
    CHAR            buf[80];
    LPSTR           p;
    PIOCTLGENERIC   pig;
    PTASK_LIST      pTask;
    LPSTR           fmt;
    DWORD           dw;
    BOOL            fReconnecting;
    LRESULT         lResult;
    static LPPD     TmpLppdCur;

    static DWORD HelpArray[]=
    {
       IDC_TL_TASK_LIST_LABEL, IDH_PROCESSLIST,
       IDC_TL_TASK_LIST, IDH_PROCESSLIST,
       0, 0
    };

    switch (message) {
    case WM_INITDIALOG:
        SendDlgItemMessage( hDlg, IDC_TL_TASK_LIST, WM_SETFONT,
            (WPARAM)GetStockObject( SYSTEM_FIXED_FONT ), (LPARAM)FALSE );
        TmpLppdCur = LppdCur;
        if (ConnectDebugger(&fReconnecting)  &&
            NULL != (pig = (PIOCTLGENERIC) malloc((sizeof(TASK_LIST)*MAX_TASKS) +
            sizeof(IOCTLGENERIC)))) {

            //Load exclusion list
            char szExclusionList[MAX_MSG_TXT];
            Dbg(LoadString(g_hInst, IDS_PROCESS_EXCLUSION_LIST, szExclusionList, sizeof(szExclusionList)));

            ZeroMemory( pig, (sizeof(TASK_LIST)*MAX_TASKS) + sizeof(IOCTLGENERIC) );
            pig->ioctlSubType = IG_TASK_LIST;
            pig->length = sizeof(TASK_LIST)*MAX_TASKS;
            pTask = (PTASK_LIST)pig->data;
            pTask->dwProcessId = MAX_TASKS;
            OSDSystemService( LppdCur->hpid,
                NULL,
                (SSVC) ssvcGeneric,
                (LPV)pig,
                pig->length + sizeof(IOCTLGENERIC),
                &dw
                );
            for (i=0; i<MAX_TASKS; i++) {
                LPSTR lpszExclude = szExclusionList;
                BOOL bExclude = FALSE;

                if (pTask[i].dwProcessId == 0) {
                    break;
                }
                if (pTask[i].dwProcessId == (DWORD)-2) {
                    continue;
                }

                while (*lpszExclude) {
                    if (!_stricmp(pTask[i].ProcessName, lpszExclude)) {
                        // Exclude from the displayed list
                        bExclude = TRUE;
                        break;
                    }
                    // Move to the next string.
                    lpszExclude += strlen(lpszExclude) +1;
                }

                if (bExclude) {
                    continue;
                }

                if ((radix == 10) || (pTask[i].dwProcessId == (DWORD)-1)) {
                    fmt = "%4d %s";
                } else if (radix == 16) {
                    fmt = "%4x %s";
                } else {
                    fmt = "%4d %s";
                }
                sprintf(buf, fmt, pTask[i].dwProcessId, pTask[i].ProcessName );
                SendDlgItemMessage( hDlg, IDC_TL_TASK_LIST, LB_ADDSTRING, 0, (LPARAM)buf );
            }
            if (i) {
                --i;
            }
            SendDlgItemMessage( hDlg, IDC_TL_TASK_LIST, LB_SETCURSEL, i, 0 );
            free( pig );
        } else {
            EndDialog( hDlg, TRUE );
        }
        return TRUE;

      case WM_HELP:
          WinHelp((HWND)((LPHELPINFO) lParam)->hItemHandle, "windbg.hlp", HELP_WM_HELP,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

      case WM_CONTEXTMENU:
          WinHelp ((HWND) wParam, "windbg.hlp", HELP_CONTEXTMENU,
             (DWORD_PTR)(LPVOID) HelpArray );
          return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDC_TL_TASK_LIST:
            if (LBN_DBLCLK != HIWORD(wParam)) {
                break;
            }
            // User Dbl clicked on an item. We fall through, and simulate a button press.
        case IDOK :
            lResult = SendDlgItemMessage( hDlg, IDC_TL_TASK_LIST, LB_GETCURSEL, 0, 0 );
            SendDlgItemMessage( hDlg, IDC_TL_TASK_LIST, LB_GETTEXT, lResult, (LPARAM)buf );

            buf[4] = 0;
            i = strtoul( buf, NULL, radix );

            if (i == (DWORD)-1) {
                if (MessageBox( hDlg,
                    "Are you sure that you want to attach to CSR?",
                    "WinDbg Process Attach",
                    MB_ICONASTERISK | MB_YESNO ) == IDNO) {

                    EndDialog( hDlg, TRUE );
                    return TRUE;
                }
            }

            sprintf( buf, ".attach 0x%x", i );
            p = (PSTR) malloc( strlen(buf)+16 );
            strcpy( p, buf );

            PostMessage(
                Views[cmdView].hwndClient,
                WU_LOG_REMOTE_CMD,
                TRUE,
                (LPARAM)p
                );

            EndDialog( hDlg, TRUE );
            return TRUE;

        case IDCANCEL:
            if(TmpLppdCur == NULL)
            {
                DisconnectDebuggee();
            }
            EndDialog( hDlg, TRUE );
            return TRUE;

        }
    }

    return FALSE;
}
