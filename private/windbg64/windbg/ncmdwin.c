/*++

Copyright (c) 1997  Microsoft Corporation

Module Name:

    ncmdwin.c

Abstract:

    New command window UI.

Author:

    Carlos Klapp (a-caklap) 1997

Environment:

    Win32, User Mode

--*/

#include "precomp.h"
#pragma hdrstop

#if defined( NEW_WINDOWING_CODE )

// Minimum window size
#define MINWND_SIZE 200


//
// Static data for structures
//
LPCSTR _COMMONWIN_DATA::lpcszName_CommonWinData = "Common Win Data Struct";
LPCSTR _CMDWIN_DATA::lpcszName_CmdWinData = "Cmd Win Data Struct";

PCOMMONWIN_DATA
GetCommonWinData(
    HWND hwnd
    )
{
    if (!hwnd) {
        return NULL;
    }

    PCOMMONWIN_DATA pWinData = (PCOMMONWIN_DATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // Sanity check.
    if (pWinData) {
        pWinData->Validate();
    }

    return pWinData;
}

PCOMMONWIN_DATA
SetCommonWinData(
    HWND hwnd, 
    PCOMMONWIN_DATA pWinData_New
    )
{
    PCOMMONWIN_DATA pWinData_Old = (PCOMMONWIN_DATA) SetWindowLongPtr(hwnd, 
                                                                      GWLP_USERDATA, 
                                                                      (LONG_PTR) pWinData_New
                                                                      );

    return pWinData_Old;
}

PCMDWIN_DATA
GetCmdWinData(
    HWND hwnd
    )
{
    PCMDWIN_DATA pCmdWinData = (PCMDWIN_DATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // Sanity check.
    if (pCmdWinData) {
        pCmdWinData->Validate();
    }

    return pCmdWinData;
}

PCMDWIN_DATA
SetCmdWinData(
    HWND hwnd, 
    PCMDWIN_DATA pCmdWinData_New
    )
{
    PCMDWIN_DATA pCmdWinData_Old = 
        (PCMDWIN_DATA) (PVOID)
        SetCommonWinData(
                         hwnd, 
                         (PCOMMONWIN_DATA) (PVOID) pCmdWinData_New
                         );

    //
    // Sanity check.
    //
    if (pCmdWinData_Old) {
        pCmdWinData_Old->Validate();
    }

    if (pCmdWinData_New) {
        pCmdWinData_New->Validate();
    }

    return pCmdWinData_Old;
}

PCALLSWIN_DATA
GetCallsWinData(
    HWND hwnd
    )
{
    PCALLSWIN_DATA pCallsWinData = (PCALLSWIN_DATA) GetWindowLongPtr(hwnd, GWLP_USERDATA);

    // Sanity check.
    if (pCallsWinData) {
        pCallsWinData->Validate();
    }

    return pCallsWinData;
}

PCALLSWIN_DATA
SetCallsWinData(
    HWND hwnd, 
    PCALLSWIN_DATA pCallsWinData_New
    )
{
    PCALLSWIN_DATA pCallsWinData_Old = 
            (PCALLSWIN_DATA) (PVOID)
            SetCommonWinData(
                             hwnd, 
                             (PCOMMONWIN_DATA) (PVOID) pCallsWinData_New
                             );

    //
    // Sanity check.
    //
    if (pCallsWinData_Old) {
        pCallsWinData_Old->Validate();
    }

    if (pCallsWinData_New) {
        pCallsWinData_New->Validate();
    }

    return pCallsWinData_Old;
}

HWND
NewCmd_CreateWindow(
    HWND hwndParent
    )
/*++
Routine Description:

  Create the command window.

Arguments:

    hwndParent - The parent window to the command window. In an MDI document,
        this is usually the handle to the MDI client window: g_hwndMDIClient

Return Value:

    If successful, creates a valid window handle to the new command window.

    NULL if the window was not created.

--*/
{
    char szClassName[MAX_MSG_TXT];
    char szWinTitle[MAX_MSG_TXT];
    CREATESTRUCT cs;
    HWND hwnd;

    // get class name
    Dbg(LoadString(g_hInst, SYS_NewCmd_wClass, szClassName, sizeof(szClassName)));

    // Get the title
    {
        char sz[MAX_MSG_TXT];

        Dbg(LoadString(g_hInst, SYS_CmdWin_Title, sz, sizeof(sz)));

        RemoveMnemonic(sz, szWinTitle);
    }

    ZeroMemory(&cs, sizeof(cs));

    return CreateWindowEx(
        WS_EX_MDICHILD | WS_EX_CONTROLPARENT,       // Extended style
        szClassName,                                // class name
        szWinTitle,                                 // title
        WS_CLIPCHILDREN | WS_CLIPSIBLINGS
        | WS_OVERLAPPEDWINDOW | WS_VISIBLE,         // style
        CW_USEDEFAULT,                              // x
        CW_USEDEFAULT,                              // y
        CW_USEDEFAULT,                              // width
        CW_USEDEFAULT,                              // height
        hwndParent,                                 // parent
        NULL,                                       // menu
        g_hInst,                                    // hInstance
        NULL                                        // user defined data
        );
}


LRESULT
CALLBACK
NewCmd_WindowProc(
    HWND hwnd,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    )
{
    PCMDWIN_DATA pCmdWinData = GetCmdWinData(hwnd);

    switch (uMsg) {
    default:
        return DefMDIChildProc(hwnd, uMsg, wParam, lParam);
    
    case WM_CREATE:
        {
            RECT rc;

            Assert(NULL == pCmdWinData);

            pCmdWinData = new CMDWIN_DATA;
            if (!pCmdWinData) {
                return -1; // Fail window creation
            }

            pCmdWinData->hwndHistory = CreateWindowEx(
                WS_EX_CLIENTEDGE,                           // Extended style
                "RichEdit",                                 // class name
                NULL,                                       // title
                WS_CLIPSIBLINGS
                | WS_CHILD | WS_VISIBLE
                | WS_HSCROLL | WS_VSCROLL
                | ES_AUTOHSCROLL | ES_AUTOVSCROLL
                | ES_MULTILINE | ES_READONLY,               // style
                0,                                          // x
                0,                                          // y
                100, //CW_USEDEFAULT,                              // width
                100, //CW_USEDEFAULT,                              // height
                hwnd,                                       // parent
                (HMENU) IDC_RICHEDIT_CMD_HISTORY,           // control id
                g_hInst,                                      // hInstance
                NULL);                                      // user defined data

            if (!pCmdWinData->hwndHistory) {
                delete pCmdWinData;
                return -1; // Fail window creation
            }


            pCmdWinData->hwndEdit = CreateWindowEx(
                WS_EX_CLIENTEDGE,                           // Extended style
                "RichEdit",                                 // class name
                NULL,                                       // title
                WS_CLIPSIBLINGS
                | WS_CHILD | WS_VISIBLE
                | WS_VSCROLL | ES_AUTOVSCROLL
                | ES_MULTILINE,                             // style
                0,                                          // x
                100,                                        // y
                100, //CW_USEDEFAULT,                              // width
                100, //CW_USEDEFAULT,                              // height
                hwnd,                                       // parent
                (HMENU) IDC_RICHEDIT_CMD_EDIT,              // control id
                g_hInst,                                      // hInstance
                NULL);                                      // user defined data

            if (!pCmdWinData->hwndEdit) {
                delete pCmdWinData;
                return -1; // Fail window creation
            }

            if (pCmdWinData->bHistoryActive) {
                SetFocus(pCmdWinData->hwndHistory);
            } else {
                SetFocus(pCmdWinData->hwndEdit);
            }

            GetClientRect(hwnd, &rc);
            pCmdWinData->nDividerPosition = rc.bottom / 2;
            g_DebuggerWindows.hwndCmd = hwnd;
 
            // Tell the edit controls, that we want notification of keyboard input
            // This is so we can process the enter key, and then send that text into the
            // History window.
            SendMessage(pCmdWinData->hwndEdit, EM_SETEVENTMASK, 0, ENM_KEYEVENTS);

            // store this in the window
            SetCmdWinData(hwnd, pCmdWinData);
        }
        return 0;

    case WU_INITDEBUGWIN:

        PCTRLC_HANDLER
        AddCtrlCHandler(
            CTRLC_HANDLER_PROC pfnFunc,
            DWORD              dwParam
            );

        BOOL
        DoCtrlCAsyncStop(
            DWORD dwParam
            );


        //
        // set up ctrlc handler
        //
        AddCtrlCHandler(DoCtrlCAsyncStop, FALSE);

        //
        // Initialize cmd processor, show initial prompt.
        //

        CmdSetDefaultCmdProc();
        if (arNone != AutoRun) {
            CmdDoPrompt(TRUE, TRUE);
        }

        return FALSE;

    case WM_NOTIFY:
        {
            switch (wParam) {
            case IDC_RICHEDIT_CMD_EDIT:
                {
                    MSGFILTER * lpMsgFilter = (MSGFILTER *) lParam;

                    if (EN_MSGFILTER == lpMsgFilter->nmhdr.code) {
                        if (WM_CHAR == lpMsgFilter->msg && VK_RETURN == lpMsgFilter->wParam) {
                            long lLen;
                            TEXTRANGE TextRange;

                            // Get length.
                            // +1, we have to take into account the null terminator.
                            // +1 for the '>' character
                            // +4 for "\r\n\r\n"
                            lLen = (long) SendMessage(lpMsgFilter->nmhdr.hwndFrom, 
                                                      WM_GETTEXTLENGTH, 
                                                      0, 
                                                      0
                                                      ) +6;

                            // Get everything
                            TextRange.chrg.cpMin = 0;
                            TextRange.chrg.cpMax = -1;
                            TextRange.lpstrText = (PSTR) GlobalAlloc(GPTR, lLen);

                            if (NULL == TextRange.lpstrText) {
                                FatalErrorBox(ERR_Cannot_Allocate_Memory, NULL);
                            } else {
                                *(TextRange.lpstrText) = '>';

                                // Okay got the text
                                SendMessage(pCmdWinData->hwndEdit, 
                                            WM_GETTEXT, 
                                            lLen,
                                            (LPARAM) (TextRange.lpstrText +1));

                                TextRange.chrg.cpMin = INT_MAX -1;
                                TextRange.chrg.cpMax = INT_MAX;
                                SendMessage(pCmdWinData->hwndHistory, 
                                            EM_EXSETSEL, 
                                            0,
                                            (LPARAM) &TextRange.chrg);

                                strcat(TextRange.lpstrText, "\n");

                                SendMessage(pCmdWinData->hwndHistory, 
                                            EM_REPLACESEL, 
                                            FALSE,
                                            (LPARAM) TextRange.lpstrText
                                            );

                                SendMessage(pCmdWinData->hwndHistory, 
                                            EM_SCROLLCARET, 
                                            0, 
                                            0
                                            );

                                SendMessage(pCmdWinData->hwndEdit, 
                                            WM_SETTEXT, 
                                            0, 
                                            (LPARAM) ""
                                            );

                                GlobalFree(TextRange.lpstrText);
                            }

                            // ignore the event
                            return 1;
                        }
                    }
                }
                // process the event
                return 0;
            }
        }
        return 0;

    case WM_GETMINMAXINFO:
        {
            LPMINMAXINFO lpMinMaxInfo = (LPMINMAXINFO) lParam;

            lpMinMaxInfo->ptMinTrackSize.x = MINWND_SIZE;
            lpMinMaxInfo->ptMinTrackSize.y = MINWND_SIZE;
        }
        return 0;

    case WM_LBUTTONDOWN:
        {
            pCmdWinData->bTrackingMouse = TRUE;
            SetCapture(hwnd);
        }
        return 0;

    case WM_MOUSEMOVE:
        {
            if (MK_LBUTTON & wParam && pCmdWinData->bTrackingMouse) {
                // We are resizing the History & Edit Windows
                RECT rc;

                GetClientRect(hwnd, &rc);

                // y position centered vertically around the cursor
                pCmdWinData->nDividerPosition = HIWORD(lParam) - GetSystemMetrics(SM_CYEDGE) /2;

                SendMessage(hwnd, 
                            WM_SIZE, 
                            SIZE_RESTORED, 
                            MAKELPARAM(rc.right, rc.bottom)
                            );
            }
        }
        return 0;


    case WM_LBUTTONUP:
        {
            pCmdWinData->bTrackingMouse = FALSE;
            ReleaseCapture();
        }
        return 0;

    case WM_SIZE:
        {
            int nWidth = LOWORD(lParam);
            int nHeight = HIWORD(lParam);
            const int nDividerHeight = GetSystemMetrics(SM_CYEDGE);
            int nHistoryHeight = pCmdWinData->nDividerPosition;

            MoveWindow(pCmdWinData->hwndHistory,0, 0, nWidth, nHistoryHeight, TRUE);

            MoveWindow(pCmdWinData->hwndEdit, 0, nHistoryHeight + nDividerHeight,
                nWidth, nHeight - nHistoryHeight - nDividerHeight, TRUE);
        }
        return 0;

    case WM_DESTROY:
        {
            // Clean up
            PVOID pv = SetCmdWinData(hwnd, NULL);
            if (pv) {
                delete pv;
            }

            g_DebuggerWindows.hwndCmd = NULL;
        }
        return 0;
    }
}

#endif  // NEW_WINDOWING_CODE 

