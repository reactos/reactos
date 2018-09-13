/****************************** Module Header ******************************\
* Module Name: msgbox.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the MessageBox API and related functions.
*
* History:
* 10-23-90 DarrinM     Created.
* 02-08-91 IanJa       HWND revalidation added
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

//
// Dimension constants  --  D.U. == dialog units
//
#define DU_OUTERMARGIN    7
#define DU_INNERMARGIN    10

#define DU_BTNGAP         4   // D.U. of space between buttons
#define DU_BTNHEIGHT      14  // D.U. of button height
// This is used only in kernel\inctlpan.c, so move it there
//
// #define DU_BTNWIDTH       50  // D.U. of button width, minimum
//

LPBYTE MB_UpdateDlgHdr(LPDLGTEMPLATE lpDlgTmp, long lStyle, long lExtendedStyle, BYTE bItemCount,
           int iX, int iY, int iCX, int iCY, LPWSTR lpszCaption, int iCaptionLen);
LPBYTE MB_UpdateDlgItem(LPDLGITEMTEMPLATE lpDlgItem, int iCtrlId, long lStyle, long lExtendedStyle,
           int iX, int iY, int iCX, int iCY, LPWSTR lpszText, UINT wTextLen,
           int iControlClass);
UINT   MB_GetIconOrdNum(UINT rgBits);
LPBYTE MB_AddPushButtons(
    LPDLGITEMTEMPLATE lpDlgTmp,
    LPMSGBOXDATA      lpmb,
    UINT wLEdge,
    UINT wBEdge,
    DWORD dwStyleMsg);
UINT MB_FindDlgTemplateSize( LPMSGBOXDATA lpmb );
int MessageBoxWorker(LPMSGBOXDATA pMsgBoxParams);
VOID EndTaskModalDialog(HWND hwndDlg);
VOID StartTaskModalDialog(HWND hwndDlg);

#ifdef _JANUS_

// the definition for message file
#include "strid.h"
#include "imagehlp.h"

// constant strings
CONST WCHAR szEMIKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Control\\Error Message Instrument\\";
CONST WCHAR szEMIEnable[] = L"EnableLogging";
CONST WCHAR szEMISeverity[] = L"LogSeverity";
CONST WCHAR szDMREnable[] = L"EnableDefaultReply";
CONST WCHAR szUnknown[] = L"Unknown";
CONST WCHAR szEventKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\EventLog\\Application\\Error Instrument\\";
CONST WCHAR szEventMsgFile[] = L"EventMessageFile";
CONST WCHAR szEventType[] = L"TypesSupported";

#define TITLE_SIZE          64
#define DATETIME_SIZE       32

#define EMI_SEVERITY_ALL          0
#define EMI_SEVERITY_USER         1
#define EMI_SEVERITY_INFORMATION  2
#define EMI_SEVERITY_QUESTION     3
#define EMI_SEVERITY_WARNING      4
#define EMI_SEVERITY_ERROR        5
#define EMI_SEVERITY_MAX_VALUE    5

// element of error message
PVOID gpReturnAddr = 0;
HANDLE gdwEMIThreadID = 0;
typedef struct _ERROR_ELEMENT {
    WCHAR       ProcessName[MAX_PATH];
    WCHAR       WindowTitle[TITLE_SIZE];
    DWORD       dwStyle;
    DWORD       dwErrorCode;
    WCHAR       CallerModuleName[MAX_PATH];
    PVOID       BaseAddr;
    DWORD       dwImageSize;
    PVOID       ReturnAddr;
    LPWSTR      lpszCaption;
    LPWSTR      lpszText;
} ERROR_ELEMENT, *LPERROR_ELEMENT;

BOOL ErrorMessageInst(LPMSGBOXDATA pMsgBoxParams);
BOOL InitInstrument(LPDWORD lpEMIControl);

// eventlog stuff
FARPROC gfnRegisterEventSource;
FARPROC gfnDeregisterEventSource;
FARPROC gfnReportEvent;
FARPROC gfnGetTokenInformation;
FARPROC gfnOpenProcessToken;
FARPROC gfnOpenThreadToken;
HANDLE gEventSource;
NTSTATUS CreateLogSource();
HANDLE RegisterLogSource(LPWSTR lpszSource);
BOOL LogMessageBox(LPERROR_ELEMENT lpErrEle);
BOOL GetUserSid(PTOKEN_USER *ppTokenUser);
BOOL GetTokenHandle(PHANDLE pTokenHandle);

#define EMIGETRETURNADDRESS()                                    \
{                                                                \
    if (gfEMIEnable) {                                           \
        if (InterlockedCompareExchangePointer(&gdwEMIThreadID,   \
                                              GETTHREADID(),     \
                                              0)                 \
             == 0) {                                             \
            gpReturnAddr = _ReturnAddress();                     \
        }                                                        \
    }                                                            \
}
#else
#define EMIGETRETURNADDRESS()
#endif //_JANUS_



#define MB_MASKSHIFT    4

CONST WCHAR szEmpty[] = L"";
WCHAR szERROR[10];
ATOM atomBwlProp;
ATOM atomMsgBoxCallback;

/***************************************************************************\
* SendHelpMessage
*
*
\***************************************************************************/

void SendHelpMessage(
    HWND   hwnd,
    int    iType,
    int    iCtrlId,
    HANDLE hItemHandle,
    DWORD  dwContextId,
    MSGBOXCALLBACK lpfnCallback)
{
    HELPINFO    HelpInfo;
    long        lValue;

    HelpInfo.cbSize = sizeof(HELPINFO);
    HelpInfo.iContextType = iType;
    HelpInfo.iCtrlId = iCtrlId;
    HelpInfo.hItemHandle = hItemHandle;
    HelpInfo.dwContextId = dwContextId;

    lValue = NtUserGetMessagePos();
    HelpInfo.MousePos.x = GET_X_LPARAM(lValue);
    HelpInfo.MousePos.y = GET_Y_LPARAM(lValue);

    // Check if there is an app supplied callback.
    if(lpfnCallback != NULL) {
        if (IsWOWProc(lpfnCallback)) {
            (*pfnWowMsgBoxIndirectCallback)(PtrToUlong(lpfnCallback), &HelpInfo);
        } else {
            (*lpfnCallback)(&HelpInfo);
        }
    } else {
        SendMessage(hwnd, WM_HELP, 0, (LPARAM)&HelpInfo);
    }
}


/***************************************************************************\
* ServiceMessageBox
*
*
\***************************************************************************/

CONST int aidReturn[] = { 0, 0, IDABORT, IDCANCEL, IDIGNORE, IDNO, IDOK, IDRETRY, IDYES };

int ServiceMessageBox(
    LPCWSTR pText,
    LPCWSTR pCaption,
    UINT wType)
{
    NTSTATUS Status;
    ULONG_PTR Parameters[3];
    ULONG Response = ResponseNotHandled;
    UNICODE_STRING Text, Caption;

    /*
     * For Terminal Services we must decided the session in which this message
     * box should be displayed.  We do this by looking at the impersonation token
     * and use the session on which the client is running.
     */
    if (ISTS()) {
        HANDLE      TokenHandle;
        NTSTATUS    Status;
        ULONG       ClientSessionId;
        ULONG       ProcessSessionId;
        ULONG       ReturnLength;
        BOOLEAN     bResult;

        /*
         * Obtain access to the impersonation token if it's present.
         */
        Status = NtOpenThreadToken (
            GetCurrentThread(),
            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
            TRUE,
            &TokenHandle
            );
        if ( NT_SUCCESS(Status) ) {
            /*
             * Query the Session ID out of the Token
             */
            Status = NtQueryInformationToken (
                TokenHandle,
                TokenSessionId,
                (PVOID)&ClientSessionId,
                sizeof(ClientSessionId),
                &ReturnLength
                );
            CloseHandle(TokenHandle);
            if (NT_SUCCESS(Status)) {
                /*
                 * Get the process session Id.  Use the Kernel32 API first because
                 * the PEB is writable in case someone is hacking it.
                 */
                if (!ProcessIdToSessionId(GetCurrentProcessId(), &ProcessSessionId)) {
                    ProcessSessionId = NtCurrentPeb()->SessionId;
                }

                if (ClientSessionId != ProcessSessionId)
                {
                    /*
                     * Make sure WinSta.Dll is loaded
                     */
                    if (ghinstWinStaDll == NULL) {
                        ghinstWinStaDll = LoadLibrary(L"winsta.dll");

                        /*
                         * Get the function pointer if WinSta.Dll loaded.
                         */
                        if (ghinstWinStaDll != NULL) {
                            gfnWinStationSendMessageW = GetProcAddress(ghinstWinStaDll, "WinStationSendMessageW");
                        }
                    }

                    /*
                     * This message box was intended for session other than the
                     * one on which this process is running.  Forward it to the
                     * right session with WinStationSendMessage().
                     */
                    if (gfnWinStationSendMessageW != NULL) {

                        /*
                         * Handle case where Caption or Title is NULL
                         */
                        if (pCaption == NULL)
                            pCaption = szEmpty;
                        if (pText == NULL)
                            pText = szEmpty;

                        bResult = (BOOLEAN)gfnWinStationSendMessageW(
                                      SERVERNAME_CURRENT,
                                      ClientSessionId,
                                      pCaption,
                                      wcslen( pCaption ) * sizeof( WCHAR ),
                                      pText,
                                      wcslen( pText ) * sizeof( WCHAR ),
                                      wType,
                                      -1,           // Wait forever
                                      &Response,
                                      FALSE         // always wait
                                  );
                        if (bResult != TRUE)
                            Response = aidReturn[ResponseNotHandled];
                        else {
                            if (Response == IDTIMEOUT || Response == IDERROR) {
                                Response = aidReturn[ResponseNotHandled];
                            }
                        }

                        return (int)Response;
                    }
                }
            }
        }
    }

    /*
     * MessageBox is for this session, go call CSR.
     */
    RtlInitUnicodeString(&Text, pText);
    RtlInitUnicodeString(&Caption, pCaption);
    Parameters[0] = (ULONG_PTR)&Text;
    Parameters[1] = (ULONG_PTR)&Caption;
    Parameters[2] = wType;

    /*
     * Compatibility: Pass the override bit to make sure this box always shows
     */
    Status = NtRaiseHardError(STATUS_SERVICE_NOTIFICATION | HARDERROR_OVERRIDE_ERRORMODE,
                              ARRAY_SIZE(Parameters),
                              3,
                              Parameters,
                              OptionOk,
                              &Response);

    if (!NT_SUCCESS(Status)) {
        RIPNTERR0(Status, RIP_VERBOSE, "");
    }

    return aidReturn[Response];
}


/***************************************************************************\
* MessageBox (API)
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

int MessageBoxA(
    HWND hwndOwner,
    LPCSTR lpszText,
    LPCSTR lpszCaption,
    UINT wStyle)
{
    EMIGETRETURNADDRESS();
    return MessageBoxExA(hwndOwner, lpszText, lpszCaption, wStyle, 0);
}

int MessageBoxW(
    HWND hwndOwner,
    LPCWSTR lpszText,
    LPCWSTR lpszCaption,
    UINT wStyle)
{
    EMIGETRETURNADDRESS();
    return MessageBoxExW(hwndOwner, lpszText, lpszCaption, wStyle, 0);
}


/***************************************************************************\
* MessageBoxEx (API)
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

int MessageBoxExA(
    HWND hwndOwner,
    LPCSTR lpszText,
    LPCSTR lpszCaption,
    UINT wStyle,
    WORD wLanguageId)
{
    int retval;
    LPWSTR lpwszText = NULL;
    LPWSTR lpwszCaption = NULL;

    if (lpszText) {
        if (!MBToWCS(lpszText, -1, &lpwszText, -1, TRUE))
            return 0;
    }

    if (lpszCaption) {
        if (!MBToWCS(lpszCaption, -1, &lpwszCaption, -1, TRUE)) {
            UserLocalFree(lpwszText);
            return 0;
        }
    }

    EMIGETRETURNADDRESS();
    retval = MessageBoxExW(hwndOwner,
                           lpwszText,
                           lpwszCaption,
                           wStyle,
                           wLanguageId);

    UserLocalFree(lpwszText);
    if (lpwszCaption)
        UserLocalFree(lpwszCaption);

    return retval;
}

int MessageBoxExW(
    HWND hwndOwner,
    LPCWSTR lpszText,
    LPCWSTR lpszCaption,
    UINT wStyle,
    WORD wLanguageId)
{
    MSGBOXDATA  MsgBoxParams;


#if DBG
    /*
     * MB_USERICON is valid for MessageBoxIndirect only.
     * MessageBoxWorker validates the other style bits
     */
    if (wStyle & MB_USERICON) {
        RIPMSG0(RIP_WARNING, "MessageBoxExW: Invalid flag: MB_USERICON");
    }
#endif

    RtlZeroMemory(&MsgBoxParams, sizeof(MsgBoxParams));
    MsgBoxParams.cbSize           = sizeof(MSGBOXPARAMS);
    MsgBoxParams.hwndOwner        = hwndOwner;
    MsgBoxParams.hInstance        = NULL;
    MsgBoxParams.lpszText         = lpszText;
    MsgBoxParams.lpszCaption      = lpszCaption;
    MsgBoxParams.dwStyle          = wStyle;
    MsgBoxParams.wLanguageId      = wLanguageId;

    EMIGETRETURNADDRESS();
    return MessageBoxWorker(&MsgBoxParams);
}

/**************************************************************************\
* MessageBoxIndirect (API)
*
* 09-30-94 FritzS  Created
\**************************************************************************/

int MessageBoxIndirectA(
    CONST MSGBOXPARAMSA *lpmbp)
{
    int retval;
    MSGBOXDATA  MsgBoxParams;
    LPWSTR lpwszText = NULL;
    LPWSTR lpwszCaption = NULL;

    if (lpmbp->cbSize != sizeof(MSGBOXPARAMS)) {
        RIPMSG0(RIP_WARNING, "MessageBoxIndirect: Invalid cbSize");
    }

    RtlZeroMemory(&MsgBoxParams, sizeof(MsgBoxParams));
    RtlCopyMemory(&MsgBoxParams, lpmbp, sizeof(MSGBOXPARAMS));

    if (IS_PTR(MsgBoxParams.lpszText)) {
        if (!MBToWCS((LPSTR)MsgBoxParams.lpszText, -1, &lpwszText, -1, TRUE))
            return 0;
        MsgBoxParams.lpszText = lpwszText;
    }
    if (IS_PTR(MsgBoxParams.lpszCaption)) {
        if (!MBToWCS((LPSTR)MsgBoxParams.lpszCaption, -1, &lpwszCaption, -1, TRUE)) {
            UserLocalFree(lpwszText);
            return 0;
        }
        MsgBoxParams.lpszCaption = lpwszCaption;
    }

    EMIGETRETURNADDRESS();
    retval = MessageBoxWorker(&MsgBoxParams);

    if (lpwszText)
        UserLocalFree(lpwszText);
    if (lpwszCaption)
        UserLocalFree(lpwszCaption);

    return retval;
}

int MessageBoxIndirectW(
    CONST MSGBOXPARAMSW *lpmbp)
{
    MSGBOXDATA  MsgBoxParams;

    if (lpmbp->cbSize != sizeof(MSGBOXPARAMS)) {
        RIPMSG0(RIP_WARNING, "MessageBoxIndirect: Invalid cbSize");
    }

    RtlZeroMemory(&MsgBoxParams, sizeof(MsgBoxParams));
    RtlCopyMemory(&MsgBoxParams, lpmbp, sizeof(MSGBOXPARAMS));

    EMIGETRETURNADDRESS();
    return MessageBoxWorker(&MsgBoxParams);
}

/***************************************************************************\
* MessageBoxWorker (API)
*
* History:
* 03-10-93 JohnL      Created
\***************************************************************************/

int MessageBoxWorker(
    LPMSGBOXDATA pMsgBoxParams)
{
    DWORD  dwStyle = pMsgBoxParams->dwStyle;
    UINT   wBtnCnt;
    UINT   wDefButton;
    UINT   i;
    UINT   wBtnBeg;
    WCHAR  szErrorBuf[64];
    LPWSTR apstrButton[4];
    int    aidButton[4];
    BOOL   fCancel = FALSE;
    int    retValue;
    MBSTRING* pMBString;

#if DBG
    if (dwStyle & ~MB_VALID) {
        RIPMSG2(RIP_WARNING, "MessageBoxWorker: Invalid flags, %#lx & ~%#lx != 0",
              dwStyle, MB_VALID);
    }
#endif

#ifdef _JANUS_
    /*
     * Error message instrument start here
     * Check EMI enable
     */

    if (gfEMIEnable) {
        if (!ErrorMessageInst(pMsgBoxParams))
            RIPMSG0(RIP_WARNING, "MessageBoxWorker: Fail to instrument error msg");
    };

    /*
     * Default Message Return: on unattended systems the default button
     * can be returned automatically without putting up the message box
     */

    if (gfDMREnable) {
        /*
         * validate the style and default button as in the main code path
         */

        /*
         * Validate the "type" of message box requested.
         */
        if ((dwStyle & MB_TYPEMASK) > MB_LASTVALIDTYPE) {
            RIPERR0(ERROR_INVALID_MSGBOX_STYLE, RIP_VERBOSE, "");
            return 0;
        }

        wBtnCnt = mpTypeCcmd[dwStyle & MB_TYPEMASK] +
                                ((dwStyle & MB_HELP) ? 1 : 0);

        /*
         * Set the default button value
         */
        wDefButton = (dwStyle & (UINT)MB_DEFMASK) / (UINT)(MB_DEFMASK & (MB_DEFMASK >> 3));

        if (wDefButton >= wBtnCnt)   /* Check if valid */
            wDefButton = 0;          /* Set the first button if error */

        /*
         * return the default button
         */
        return aidButton[ wDefButton ];
    }
#endif // _JANUS_

    /*
     * If lpszCaption is NULL, then use "Error!" string as the caption
     * string.
     * LATER: IanJa localize according to wLanguageId
     */
    if (pMsgBoxParams->lpszCaption == NULL) {
        /*
         * Load the default error string if we haven't done it yet
         */
        if (*szERROR == 0) {
            LoadStringW(hmodUser, STR_ERROR, szERROR, ARRAY_SIZE(szERROR));
        }
        if (pMsgBoxParams->wLanguageId == 0) {
            pMsgBoxParams->lpszCaption = szERROR;
        } else {
            LoadStringOrError(hmodUser,
                                 STR_ERROR,
                                 szErrorBuf,
                                 sizeof(szErrorBuf)/sizeof(WCHAR),
                                 pMsgBoxParams->wLanguageId);

            /*
             *  If it didn't find the string, use the default language
             */
            if (*szErrorBuf) {
               pMsgBoxParams->lpszCaption = szErrorBuf;
            } else {
               pMsgBoxParams->lpszCaption = szERROR;

               RIPMSG1(RIP_WARNING, "MessageBoxWorker: STR_ERROR string resource for language %#lx not found",
                      pMsgBoxParams->wLanguageId);
            }
        }
    }

    /*
     * MB_SERVICE_NOTIFICATION had to be redefined because
     * Win95 defined MB_TOPMOST using the same value.
     * So for old apps, we map it to the new value
     */

    if((dwStyle & MB_TOPMOST) && (GetClientInfo()->dwExpWinVer < VER40)) {
        dwStyle &= ~MB_TOPMOST;
        dwStyle |= MB_SERVICE_NOTIFICATION;
        pMsgBoxParams->dwStyle = dwStyle;

        RIPMSG1(RIP_WARNING, "MessageBoxWorker: MB_SERVICE_NOTIFICATION flag mapped. New dwStyle:%#lx", dwStyle);
    }

    /*
     * For backward compatiblity, use MB_SERVICE_NOTIFICATION if
     * it's going to the default desktop.
     */
    if (dwStyle & (MB_DEFAULT_DESKTOP_ONLY | MB_SERVICE_NOTIFICATION)) {

        /*
         * Allow services to put up popups without getting
         * access to the current desktop.
         */
        if (pMsgBoxParams->hwndOwner != NULL) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
            return 0;
        }

        return ServiceMessageBox(pMsgBoxParams->lpszText,
                                 pMsgBoxParams->lpszCaption,
                                 dwStyle & ~MB_SERVICE_NOTIFICATION);
    }

    /*
     * Make sure we have a valid window handle.
     */
    if (pMsgBoxParams->hwndOwner && !IsWindow(pMsgBoxParams->hwndOwner)) {
        RIPERR0(ERROR_INVALID_WINDOW_HANDLE, RIP_VERBOSE, "");
        return 0;
    }

    /*
     * Validate the "type" of message box requested.
     */
    if ((dwStyle & MB_TYPEMASK) > MB_LASTVALIDTYPE) {
        RIPERR0(ERROR_INVALID_MSGBOX_STYLE, RIP_VERBOSE, "");
        return 0;
    }

    wBtnCnt = mpTypeCcmd[dwStyle & MB_TYPEMASK] +
                            ((dwStyle & MB_HELP) ? 1 : 0);

    /*
     * Set the default button value
     */
    wDefButton = (dwStyle & (UINT)MB_DEFMASK) / (UINT)(MB_DEFMASK & (MB_DEFMASK >> 3));

    if (wDefButton >= wBtnCnt)   /* Check if valid */
        wDefButton = 0;          /* Set the first button if error */

    /*
     * Calculate the strings to use in the message box
     */
    wBtnBeg = mpTypeIich[dwStyle & (UINT)MB_TYPEMASK];
    for (i=0; i<wBtnCnt; i++) {

        pMBString = &gpsi->MBStrings[SEBbuttons[wBtnBeg + i]];
        /*
         * Pick up the string for the button.
         */
        if (pMsgBoxParams->wLanguageId == 0) {
            apstrButton[i] = pMBString->szName;
        } else {
            WCHAR szButtonBuf[64];
            // LATER is it possible to have button text greater than 64 chars

           /*
            *  BUG: gpsi->wMaxBtnSize might be too short for the length of this string...
            */
            LoadStringOrError(hmodUser,
                    pMBString->uStr,
                    szButtonBuf,
                    sizeof(szButtonBuf)/sizeof(WCHAR),
                    pMsgBoxParams->wLanguageId);

            /*
             *  If it didn't find the string, use the default language.
             */
            if (*szButtonBuf) {
               apstrButton[i] = TextAlloc(szButtonBuf);
            } else {
               apstrButton[i] = TextAlloc(pMBString->szName);

               RIPMSG2(RIP_WARNING, "MessageBoxWorker: string resource %#lx for language %#lx not found",
                      pMBString->uStr,
                      pMsgBoxParams->wLanguageId);
            }
        }
        aidButton[i] = pMBString->uID;
        if (aidButton[i] == IDCANCEL) {
            fCancel = TRUE;
        }
    }

    /*
     * Hackery: There are some apps that use MessageBox as initial error
     * indicators, such as mplay32, and we want this messagebox to be
     * visible regardless of waht was specified in the StartupInfo->wShowWindow
     * field.  ccMail for instance starts all of its embedded objects hidden
     * but on win 3.1 the error message would show because they don't have
     * the startup info.
     */
    NtUserModifyUserStartupInfoFlags(STARTF_USESHOWWINDOW, 0);

    pMsgBoxParams->pidButton      = aidButton;
    pMsgBoxParams->ppszButtonText = apstrButton;
    pMsgBoxParams->DefButton      = wDefButton;
    pMsgBoxParams->cButtons       = wBtnCnt;
    pMsgBoxParams->CancelId      = ((dwStyle & MB_TYPEMASK) == 0) ? IDOK : (fCancel ? IDCANCEL : 0);
    retValue = SoftModalMessageBox(pMsgBoxParams);

    if (pMsgBoxParams->wLanguageId != 0) {
        for (i=0; i<wBtnCnt; i++)
           UserLocalFree(apstrButton[i]);
    }

    return retValue;
}

#define MAX_RES_STRING  256

/***************************************************************************\
*
*  SoftModalMessageBox()
*
\***************************************************************************/
int  SoftModalMessageBox(LPMSGBOXDATA lpmb) {
    LPBYTE              lpDlgTmp;
    int                 cyIcon, cxIcon;
    int                 cxButtons;
    int                 cxMBMax;
    int                 cxText, cyText, xText;
    int                 cxBox, cyBox;
    int                 cxFoo, cxCaption;
    int                 xMB, yMB;
    HDC                 hdc;
    DWORD               wIconOrdNum;
    DWORD               wCaptionLen;
    DWORD               wTextLen;
    WORD                OrdNum[2];  // Must be an array or WORDs
    RECT                rc;
    RECT                rcWork;
    HCURSOR             hcurOld;
    DWORD               dwStyleMsg, dwStyleText;
    DWORD               dwExStyleMsg = 0;
    DWORD               dwStyleDlg;
    HWND                hwndOwner;
    LPWSTR              lpsz;
    int                 iRetVal     = 0;
    HICON               hIcon;
    HGLOBAL             hTemplate   = NULL;
    HGLOBAL             hCaption    = NULL;
    HGLOBAL             hText       = NULL;
    HINSTANCE           hInstMsg    = lpmb->hInstance;
    SIZE                size;
    HFONT               hFontOld    = NULL;
    int                 cntMBox;
    PMONITOR            pMonitor;

    ConnectIfNecessary();

    dwStyleMsg = lpmb->dwStyle;

    //
    // This code is disabled since Mirroring will take care of this.
    //
#ifndef USE_MIRRORING
    if (dwStyleMsg & MB_RTLREADING) {
        dwExStyleMsg |= WS_EX_RTLREADING;
    }
#endif
    if (dwStyleMsg & MB_RIGHT) {
        dwExStyleMsg |= WS_EX_RIGHT;
    }

    if (!IS_PTR(lpmb->lpszCaption)) {

        // won't ever be NULL because MessageBox sticks "Error!" in in that case
        if (hInstMsg && (hCaption = LocalAlloc(LPTR, MAX_RES_STRING * sizeof(WCHAR)))) {
            lpsz = (LPWSTR) hCaption;
            LoadString(hInstMsg, PTR_TO_ID(lpmb->lpszCaption), lpsz, MAX_RES_STRING);
        } else
            lpsz = NULL;

        lpmb->lpszCaption = lpsz ? lpsz : szEmpty;
    }

    if (!IS_PTR(lpmb->lpszText)) {
        // NULL not allowed
        if (hInstMsg && (hText = LocalAlloc(LPTR, MAX_RES_STRING * sizeof(WCHAR)))) {
            lpsz = (LPWSTR) hText;
            LoadString(hInstMsg, PTR_TO_ID(lpmb->lpszText), lpsz, MAX_RES_STRING);
        } else
            lpsz = NULL;

        lpmb->lpszText = lpsz ? lpsz : szEmpty;
    }

#ifdef USE_MIRRORING
    //
    // Mirroring of MessageBox'es is only enabled if :-
    //
    // * MB_RTLREADING style has been specified in the MessageBox styles OR
    // * The first two code points of the MessageBox text are Right-To-Left
    //   marks (RLMs = U+200f).
    // The feature of enable RTL mirroring if two consecutive RLMs are found
    // in the MB text is to acheive a no-code-change for localization of
    // of MessageBoxes for BiDi Apps.  [samera]
    //
    if ((dwStyleMsg & MB_RTLREADING) ||
            (lpmb->lpszText != NULL && (lpmb->lpszText[0] == UNICODE_RLM) &&
            (lpmb->lpszText[1] == UNICODE_RLM))) {
        //
        // Set Mirroring so that MessageBox and its child controls
        // get mirrored. Otherwise, the message box and its child controls
        // are Left-To-Right.
        //
        dwExStyleMsg |= WS_EX_LAYOUTRTL;

        //
        // And turn off any conflicting flags.
        //
        dwExStyleMsg &= ~WS_EX_RIGHT;
        if (dwStyleMsg & MB_RTLREADING) {
            dwStyleMsg &= ~MB_RTLREADING;
            dwStyleMsg ^= MB_RIGHT;
        }
    }
#endif


    if ((dwStyleMsg & MB_ICONMASK) == MB_USERICON)
        hIcon = LoadIcon(hInstMsg, lpmb->lpszIcon);
    else
        hIcon = NULL;

    // For compatibility reasons, we still allow the message box to come up.
    hwndOwner = lpmb->hwndOwner;

    // For PowerBuilder4.0, we must make their messageboxes owned popups. Or, else
    // they get WM_ACTIVATEAPP and they install multiple keyboard hooks and get into
    // infinite loop later.
    // Bug #15896 -- WIN95B -- 2/17/95 -- SANKAR --
    if(!hwndOwner)
      {
        WCHAR pwszLibFileName[MAX_PATH];
        static WCHAR szPB040[] = L"PB040";  // Module name of PowerBuilder4.0
        WCHAR *pw1;

        //Is this a win3.1 or older app?
        if(GetClientInfo()->dwExpWinVer <= VER31)
          {
            if (GetModuleFileName(NULL, pwszLibFileName, sizeof(pwszLibFileName)/sizeof(WCHAR)) == 0) goto getthedc;
            pw1 = pwszLibFileName + wcslen(pwszLibFileName) - 1;
            while (pw1 > pwszLibFileName) {
                if (*pw1 == TEXT('.')) *pw1-- = 0;
                else if (*pw1 == TEXT(':')) {pw1++; break;}
                else if (*pw1 == TEXT('\\')) {pw1++; break;}
                else pw1--;
            }
            // Is this the PowerBuilder 4.0 module?
            if(!_wcsicmp(pw1, szPB040))
                hwndOwner = NtUserGetForegroundWindow(); // Make the MsgBox owned.
          }
      }
getthedc:
    // Check if we're out of cache DCs until robustness...
    if (!(hdc = NtUserGetDCEx(NULL, NULL, DCX_WINDOW | DCX_CACHE))) {

        /*
         * The above call might fail for TIF_RESTRICTED processes
         * so check for the DC from the owner window
         */
        if (!(hdc = NtUserGetDCEx(hwndOwner, NULL, DCX_WINDOW | DCX_CACHE)))
            goto SMB_Exit;
    }

    // Figure out the types and dimensions of buttons

    cxButtons = (lpmb->cButtons * gpsi->wMaxBtnSize) + ((lpmb->cButtons - 1) * XPixFromXDU(DU_BTNGAP, gpsi->cxMsgFontChar));

    // Ditto for the icon, if there is one.  If not, cxIcon & cyIcon are 0.

    if (wIconOrdNum = MB_GetIconOrdNum(dwStyleMsg)) {
        cxIcon = SYSMET(CXICON) + XPixFromXDU(DU_INNERMARGIN, gpsi->cxMsgFontChar);
        cyIcon = SYSMET(CYICON);
    } else
        cxIcon = cyIcon = 0;

    hFontOld = SelectObject(hdc, gpsi->hCaptionFont);

    // Find the max between the caption text and the buttons
    wCaptionLen = wcslen(lpmb->lpszCaption);
    GetTextExtentPoint(hdc, lpmb->lpszCaption, wCaptionLen, &size);
    cxCaption = size.cx + 2*SYSMET(CXSIZE);

    //
    // The max width of the message box is 5/8 of the work area for most
    // countries.  We will then try 6/8 and 7/8 if it won't fit.  Then
    // we will use whole screen.
    //
    pMonitor = GetDialogMonitor(hwndOwner, MONITOR_DEFAULTTOPRIMARY);
    CopyRect(&rcWork, &pMonitor->rcWork);
    cxMBMax = MultDiv(rcWork.right - rcWork.left, 5, 8);

    cxFoo = 2*XPixFromXDU(DU_OUTERMARGIN, gpsi->cxMsgFontChar);

    SelectObject(hdc, gpsi->hMsgFont);

    //
    // If the text doesn't fit in 5/8, try 7/8 of the screen
    //
ReSize:
    //
    // The message box is as big as needed to hold the caption/text/buttons,
    // but not bigger than the maximum width.
    //

    cxBox = cxMBMax - 2*SYSMET(CXFIXEDFRAME);

    // Ask DrawText for the right cx and cy
    rc.left     = 0;
    rc.top      = 0;
    rc.right    = cxBox - cxFoo - cxIcon;
    rc.bottom   = rcWork.bottom - rcWork.top;
    cyText = DrawTextExW(hdc, (LPWSTR)lpmb->lpszText, -1, &rc,
                DT_CALCRECT | DT_WORDBREAK | DT_EXPANDTABS |
                DT_NOPREFIX | DT_EXTERNALLEADING | DT_EDITCONTROL, NULL);
    //
    // Make sure we have enough width to hold the buttons, in addition to
    // the icon+text.  Always force the buttons.  If they don't fit, it's
    // because the working area is small.
    //
    //
    // The buttons are centered underneath the icon/text.
    //
    cxText = rc.right - rc.left + cxIcon + cxFoo;
    cxBox = min(cxBox, max(cxText, cxCaption));
    cxBox = max(cxBox, cxButtons + cxFoo);
    cxText = cxBox - cxFoo - cxIcon;

    //
    // Now we know the text width for sure.  Really calculate how high the
    // text will be.
    //
    rc.left     = 0;
    rc.top      = 0;
    rc.right    = cxText;
    rc.bottom   = rcWork.bottom - rcWork.top;
    cyText      = DrawTextExW(hdc, (LPWSTR)lpmb->lpszText, -1, &rc, DT_CALCRECT | DT_WORDBREAK
        | DT_EXPANDTABS | DT_NOPREFIX | DT_EXTERNALLEADING | DT_EDITCONTROL, NULL);

    // Find the window size.
    cxBox += 2*SYSMET(CXFIXEDFRAME);
    cyBox = 2*SYSMET(CYFIXEDFRAME) + SYSMET(CYCAPTION) + YPixFromYDU(2*DU_OUTERMARGIN +
        DU_INNERMARGIN + DU_BTNHEIGHT, gpsi->cyMsgFontChar);

    cyBox += max(cyIcon, cyText);

    //
    // If the message box doesn't fit on the working area, we'll try wider
    // sizes successively:  6/8 of work then 7/8 of screen.
    //
    if (cyBox > rcWork.bottom - rcWork.top)
    {
        int cxTemp;

        cxTemp = MultDiv(rcWork.right - rcWork.left, 6, 8);

        if (cxMBMax == MultDiv(rcWork.right - rcWork.left, 5, 8))
        {
            cxMBMax = cxTemp;
            goto ReSize;
        }
        else if (cxMBMax == cxTemp)
        {
            // then let's try with rcMonitor
            CopyRect(&rcWork, &pMonitor->rcMonitor);
            cxMBMax = MultDiv(rcWork.right - rcWork.left, 7, 8);
            goto ReSize;
        }
    }

    if (hFontOld)
        SelectFont(hdc, hFontOld);
    NtUserReleaseDC(NULL, hdc);

    // Find the window position
    cntMBox = GetClientInfo()->pDeskInfo->cntMBox;

    xMB = (rcWork.left + rcWork.right - cxBox) / 2 + (cntMBox * SYSMET(CXSIZE));
    xMB = max(xMB, rcWork.left);
    yMB = (rcWork.top + rcWork.bottom - cyBox) / 2 + (cntMBox * SYSMET(CYSIZE));
    yMB = max(yMB, rcWork.top);

    // Bottom, right justify if we're going off the screen--but leave a
    // little gap

    if (xMB + cxBox > rcWork.right)
        xMB = rcWork.right - SYSMET(CXEDGE) - cxBox;

    //
    // Pin to the working area.  If it won't fit, then pin to the screen
    // height.  Bottom justify it at least if too big even for that, so
    // that the buttons are visible.
    //
    if (yMB + cyBox > rcWork.bottom) {
        yMB = rcWork.bottom - SYSMET(CYEDGE) - cyBox;
        if (yMB < rcWork.top) {
            yMB = pMonitor->rcMonitor.bottom - SYSMET(CYEDGE) - cyBox;
        }
    }

    wTextLen = wcslen(lpmb->lpszText);

    // Find out the memory required for the Dlg template and try to alloc it
    hTemplate = LocalAlloc(LMEM_ZEROINIT, MB_FindDlgTemplateSize(lpmb));

    if (!hTemplate)
        goto SMB_Exit;

    lpDlgTmp = (LPBYTE) hTemplate;

    //
    // Setup the dialog style for the message box
    //
    dwStyleDlg = WS_POPUPWINDOW | WS_CAPTION | DS_ABSALIGN | DS_NOIDLEMSG |
                 DS_SETFONT | DS_3DLOOK;

    if ((dwStyleMsg & MB_MODEMASK) == MB_SYSTEMMODAL)
        dwStyleDlg |= DS_SYSMODAL | DS_SETFOREGROUND;
    else
        dwStyleDlg |= DS_MODALFRAME | WS_SYSMENU;

    if (dwStyleMsg & MB_SETFOREGROUND)
        dwStyleDlg |= DS_SETFOREGROUND;

    // Add the Header of the Dlg Template
    // BOGUS !!!  don't ADD bools
    lpDlgTmp = MB_UpdateDlgHdr((LPDLGTEMPLATE) lpDlgTmp, dwStyleDlg, dwExStyleMsg,
        (BYTE) (lpmb->cButtons + (wIconOrdNum != 0) + (lpmb->lpszText != NULL)),
        xMB, yMB, cxBox, cyBox, (LPWSTR)lpmb->lpszCaption, wCaptionLen);

    //
    // Center the buttons
    //

    cxFoo = (cxBox - 2*SYSMET(CXFIXEDFRAME) - cxButtons) / 2;

    lpDlgTmp = MB_AddPushButtons((LPDLGITEMTEMPLATE)lpDlgTmp, lpmb, cxFoo,
        cyBox - SYSMET(CYCAPTION) - (2 * SYSMET(CYFIXEDFRAME)) -
        YPixFromYDU(DU_OUTERMARGIN, gpsi->cyMsgFontChar), dwStyleMsg);

    // Add Icon, if any, to the Dlg template
    //
    // The icon is always top justified.  If the text is shorter than the
    // height of the icon, we center it.  Otherwise the text will start at
    // the top.
    //
    if (wIconOrdNum) {
        OrdNum[0] = 0xFFFF;  // To indicate that an Ordinal number follows
        OrdNum[1] = (WORD) wIconOrdNum;

        lpDlgTmp = MB_UpdateDlgItem((LPDLGITEMTEMPLATE)lpDlgTmp, IDUSERICON,        // Control Id
            SS_ICON | WS_GROUP | WS_CHILD | WS_VISIBLE, 0,
            XPixFromXDU(DU_OUTERMARGIN, gpsi->cxMsgFontChar),   // X co-ordinate
            YPixFromYDU(DU_OUTERMARGIN, gpsi->cyMsgFontChar),   // Y co-ordinate
            0,  0,          // For Icons, CX and CY are ignored, can be zero
            OrdNum,         // Ordinal number of Icon
            sizeof(OrdNum)/sizeof(WCHAR), // Length of OrdNum
            STATICCODE);
    }

    // Add the Text of the Message to the Dlg Template
    if (lpmb->lpszText) {
        //
        // Center the text if shorter than the icon.
        //
        if (cyText >= cyIcon)
            cxFoo = 0;
        else
            cxFoo = (cyIcon - cyText) / 2;

        dwStyleText = SS_NOPREFIX | WS_GROUP | WS_CHILD | WS_VISIBLE | SS_EDITCONTROL;
        if (dwStyleMsg & MB_RIGHT) {
            dwStyleText |= SS_RIGHT;
            xText = cxBox - (SYSMET(CXSIZE) + cxText);
        } else {
            dwStyleText |= SS_LEFT;
            xText = cxIcon + XPixFromXDU(DU_INNERMARGIN, gpsi->cxMsgFontChar);
        }

        MB_UpdateDlgItem((LPDLGITEMTEMPLATE)lpDlgTmp, -1, dwStyleText, dwExStyleMsg, xText,
            YPixFromYDU(DU_OUTERMARGIN, gpsi->cyMsgFontChar) + cxFoo,
            cxText, cyText,
            (LPWSTR)lpmb->lpszText, wTextLen, STATICCODE);
    }

    // The dialog template is ready

    //
    // Set the normal cursor
    //
    hcurOld = NtUserSetCursor(LoadCursor(NULL, IDC_ARROW));

    lpmb->lpszIcon = (LPWSTR) hIcon;  // BUGBUG - How to diff this from a resource?

    if (!(lpmb->dwStyle & MB_USERICON))
    {
        int wBeep = (LOWORD(lpmb->dwStyle & MB_ICONMASK)) >> MB_MASKSHIFT;
        if (wBeep < USER_SOUND_MAX) {
            NtUserCallOneParam(wBeep, SFI_PLAYEVENTSOUND);
        }
    }

    iRetVal = (int)InternalDialogBox(hmodUser, hTemplate, hwndOwner,
        MB_DlgProcW, (LPARAM) lpmb, FALSE);

    //
    // Fix up return value
    if (iRetVal == -1)
        iRetVal = 0;                /* Messagebox should also return error */

     //
     // If the messagebox contains only OK button, then its ID is changed as
     // IDCANCEL in MB_DlgProc; So, we must change it back to IDOK irrespective
     // of whether ESC is pressed or Carriage return is pressed;
     //
    if (((dwStyleMsg & MB_TYPEMASK) == MB_OK) && iRetVal)
        iRetVal = IDOK;


    //
    // Restore the previous cursor
    //
    if (hcurOld)
        NtUserSetCursor(hcurOld);

SMB_Exit:
    if (hTemplate)
        UserLocalFree(hTemplate);

    if (hCaption) {
        UserLocalFree(hCaption);
    }

    if (hText) {
        UserLocalFree(hText);
    }

    return(iRetVal);
}

/***************************************************************************\
* MB_CopyToClipboard
*
* Called in response to WM_COPY, it will save the title, message and button's
* texts to the clipboard in CF_UNICODETEXT format.
*
*   ---------------------------
*   Caption
*   ---------------------------
*   Text
*   ---------------------------
*   Button1   ...   ButtonN
*   ---------------------------
*
*
* History:
* 08-03-97 MCostea      Created
\***************************************************************************/

void MB_CopyToClipboard(
    HWND hwndDlg)
{
    LPCWSTR lpszRead;
    LPWSTR  lpszAll, lpszWrite;
    HANDLE  hData;
    static  WCHAR   szLine[] = L"---------------------------\r\n";
    UINT    cBufSize, i, cWrote;
    LPMSGBOXDATA lpmb;

    if (!(lpmb = (LPMSGBOXDATA)GetWindowLongPtr(hwndDlg, GWLP_USERDATA)))
        return;

    if (!OpenClipboard(hwndDlg))
        return;

    /*
     * Calculate the buffer size:
     *      - the message text can be all \n, that will become \r\n
     *      - there are a few extra \r\n (that's why 8)
     */
    cBufSize =  (lpmb->lpszCaption ? wcslen(lpmb->lpszCaption) : 0) +
                (lpmb->lpszText ? 2*wcslen(lpmb->lpszText) : 0) +
                4*sizeof(szLine) +
                lpmb->cButtons * gpsi->wMaxBtnSize +
                8;

    cBufSize *= sizeof(WCHAR);

    if (!(hData = UserGlobalAlloc(LHND, (LONG)(cBufSize))) ) {
        goto CloseClip;
    }

    USERGLOBALLOCK(hData, lpszAll);
    UserAssert(lpszAll);

    cWrote = wsprintf(lpszAll, L"%s%s\r\n%s",
                                szLine,
                                lpmb->lpszCaption ? lpmb->lpszCaption : L"",
                                szLine);

    lpszWrite = lpszAll + cWrote;
    lpszRead = lpmb->lpszText;
    /*
     * Change \n to \r\n in the text
     */
    for (i = 0; *lpszRead; i++) {

        if (*lpszRead == L'\n')
            *lpszWrite++ = L'\r';

        *lpszWrite++ = *lpszRead++;
    }

    cWrote = wsprintf(lpszWrite, L"\r\n%s", szLine);
    lpszWrite += cWrote;

    /*
     * Remove & from the button texts
     */
    for (i = 0; i<lpmb->cButtons; i++) {

        lpszRead = lpmb->ppszButtonText[i];
        while (*lpszRead) {
            if (*lpszRead != L'&') {
                *lpszWrite++ = *lpszRead;
            }
            lpszRead++;
        }
        *lpszWrite++ = L' ';
        *lpszWrite++ = L' ';
        *lpszWrite++ = L' ';
    }
    wsprintf(lpszWrite, L"\r\n%s\0", szLine);

    USERGLOBALUNLOCK(hData);

    NtUserEmptyClipboard();
    /*
     * If we just called EmptyClipboard in the context of a 16 bit
     * app then we also have to tell WOW to nix its 16 handle copy of
     * clipboard data.  WOW does its own clipboard caching because
     * some 16 bit apps use clipboard data even after the clipboard
     * has been emptied.  See the note in the server code.
     *
     * Note: this is another place (besides client\editec.c) where
     * EmptyClipboard is called* for a 16 bit app not going through WOW.
     * If we added others we might want to move this into EmptyClipboard
     * and have two versions.
     */
    if (GetClientInfo()->CI_flags & CI_16BIT) {
        pfnWowEmptyClipBoard();
    }

    SetClipboardData(CF_UNICODETEXT, hData);

CloseClip:
    NtUserCloseClipboard();

}

/***************************************************************************\
* MB_UpdateDlgHdr
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LPBYTE MB_UpdateDlgHdr(
    LPDLGTEMPLATE lpDlgTmp,
    long lStyle,
    long lExtendedStyle,
    BYTE bItemCount,
    int iX,
    int iY,
    int iCX,
    int iCY,
    LPWSTR lpszCaption,
    int cchCaptionLen)
{
    LPTSTR lpStr;
    RECT rc;

    /*
     * Adjust the rectangle dimensions.
     */
    rc.left     = iX + SYSMET(CXFIXEDFRAME);
    rc.top      = iY + SYSMET(CYFIXEDFRAME);
    rc.right    = iX + iCX - SYSMET(CXFIXEDFRAME);
    rc.bottom   = iY + iCY - SYSMET(CYFIXEDFRAME);


    /*
     * Adjust for the caption.
     */
    rc.top += SYSMET(CYCAPTION);

    lpDlgTmp->style = lStyle;
    lpDlgTmp->dwExtendedStyle = lExtendedStyle;
    lpDlgTmp->cdit = bItemCount;
    lpDlgTmp->x  = XDUFromXPix(rc.left, gpsi->cxMsgFontChar);
    lpDlgTmp->y  = YDUFromYPix(rc.top, gpsi->cyMsgFontChar);
    lpDlgTmp->cx = XDUFromXPix(rc.right - rc.left, gpsi->cxMsgFontChar);
    lpDlgTmp->cy = YDUFromYPix(rc.bottom - rc.top, gpsi->cyMsgFontChar);

    /*
     * Move pointer to variable length fields.  No menu resource for
     * message box, a zero window class (means dialog box class).
     */
    lpStr = (LPWSTR)(lpDlgTmp + 1);
    *lpStr++ = 0;                           // Menu
    lpStr = (LPWSTR)NextWordBoundary(lpStr);
    *lpStr++ = 0;                           // Class
    lpStr = (LPWSTR)NextWordBoundary(lpStr);

    /*
     * NOTE: iCaptionLen may be less than the length of the Caption string;
     * So, DO NOT USE lstrcpy();
     */
    RtlCopyMemory(lpStr, lpszCaption, cchCaptionLen*sizeof(WCHAR));
    lpStr += cchCaptionLen;
    *lpStr++ = TEXT('\0');                // Null terminate the caption str

    /*
     * Font height of 0x7FFF means use the message box font
     */
    *lpStr++ = 0x7FFF;

    return NextDWordBoundary(lpStr);
}

/***************************************************************************\
* MB_AddPushButtons
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LPBYTE MB_AddPushButtons(
    LPDLGITEMTEMPLATE  lpDlgTmp,
    LPMSGBOXDATA       lpmb,
    UINT               wLEdge,
    UINT               wBEdge,
    DWORD              dwStyleMsg)
{
    UINT   wYValue;
    UINT   i;
    UINT   wHeight;
    UINT   wCount = lpmb->cButtons;
#ifdef USE_MIRRORING
    UNREFERENCED_PARAMETER(dwStyleMsg);
#endif

    wHeight = YPixFromYDU(DU_BTNHEIGHT, gpsi->cyMsgFontChar);

    wYValue = wBEdge - wHeight;         // Y co-ordinate for push buttons

#ifndef USE_MIRRORING
    /*
     * Since USE_MIRRORING is enabled now, this code is disabled
     * since Mirroring will take care of this. [samera]
     */
    if ((dwStyleMsg & MB_RTLREADING) && (wCount > 1)) {
        wLEdge += ((wCount-1) *
                   (gpsi->wMaxBtnSize + XPixFromXDU(DU_BTNGAP, gpsi->cxMsgFontChar)));
    }
#endif

    for (i = 0; i < wCount; i++) {

        lpDlgTmp = (LPDLGITEMTEMPLATE)MB_UpdateDlgItem(
                lpDlgTmp,                       /* Ptr to template */
                lpmb->pidButton[i],             /* Control Id */
                WS_TABSTOP | WS_CHILD | WS_VISIBLE | (i == 0 ? WS_GROUP : 0) |
                ((UINT)i == lpmb->DefButton ? BS_DEFPUSHBUTTON : BS_PUSHBUTTON),
                0,
                wLEdge,                         /* X co-ordinate */
                wYValue,                        /* Y co-ordinate */
                gpsi->wMaxBtnSize,              /* CX */
                wHeight,                        /* CY */
                lpmb->ppszButtonText[i],        /* String for button */
                (UINT)wcslen(lpmb->ppszButtonText[i]),/* Length */
                BUTTONCODE);

        /*
         * Get the X co-ordinate for the next Push button
         */
#ifndef USE_MIRRORING
        /*
         * Since USE_MIRRORING is enabled now, this code is disabled
         * since Mirroring will take care of this. [samera]
         */
        if (dwStyleMsg & MB_RTLREADING)
            wLEdge -= gpsi->wMaxBtnSize + XPixFromXDU(DU_BTNGAP, gpsi->cxMsgFontChar);
        else
#endif
            wLEdge += gpsi->wMaxBtnSize + XPixFromXDU(DU_BTNGAP, gpsi->cxMsgFontChar);
    }

    return (LPBYTE)lpDlgTmp;
}

/***************************************************************************\
* MB_UpdateDlgItem
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

LPBYTE MB_UpdateDlgItem(
    LPDLGITEMTEMPLATE lpDlgItem,
    int iCtrlId,
    long lStyle,
    long lExtendedStyle,
    int iX,
    int iY,
    int iCX,
    int iCY,
    LPWSTR lpszText,
    UINT cchTextLen,
    int iControlClass)
{
    LPWSTR lpStr;
    BOOL fIsOrdNum;


    lpDlgItem->x        = XDUFromXPix(iX, gpsi->cxMsgFontChar);
    lpDlgItem->y        = YDUFromYPix(iY, gpsi->cyMsgFontChar);
    lpDlgItem->cx       = XDUFromXPix(iCX,gpsi->cxMsgFontChar);
    lpDlgItem->cy       = YDUFromYPix(iCY,gpsi->cyMsgFontChar);
    lpDlgItem->id       = (WORD)iCtrlId;
    lpDlgItem->style    = lStyle;
    lpDlgItem->dwExtendedStyle = lExtendedStyle;

    /*
     * We have to avoid the following nasty rounding off problem:
     * (e.g) If iCX=192 and cxSysFontChar=9, then cx becomes 85; When the
     * static text is drawn, from 85 dlg units we get 191 pixels; So, the text
     * is truncated;
     * So, to avoid this, check if this is a static text and if so,
     * add one more dialog unit to cx and cy;
     * --Fix for Bug #4481 --SANKAR-- 09-29-89--
     */

    /*
     * Also, make sure we only do this to static text items.  davidds
     */

    /*
     * Now static text uses SS_NOPREFIX = 0x80;
     * So, test the lStyle field only with 0x0F instead of 0xFF;
     * Fix for Bugs #5933 and 5935 --SANKAR-- 11-28-89
     */
    if (iControlClass == STATICCODE &&
         (((lStyle & 0x0F) == SS_LEFT) || ((lStyle & 0x0F) == SS_RIGHT))) {

        /*
         * This is static text
         */
        lpDlgItem->cx++;
        lpDlgItem->cy++;
    }

    /*
     * Move ptr to the variable fields
     */
    lpStr = (LPWSTR)(lpDlgItem + 1);

    /*
     * Store the Control Class value
     */
    *lpStr++ = 0xFFFF;
    *lpStr++ = (BYTE)iControlClass;
    lpStr = (LPWSTR)NextWordBoundary(lpStr);        // WORD-align lpszText

    /*
     * Check if the String contains Ordinal number or not
     */
    fIsOrdNum = ((*lpszText == 0xFFFF) && (cchTextLen == sizeof(DWORD)/sizeof(WCHAR)));

    /*
     * NOTE: cchTextLen may be less than the length of lpszText.  So,
     * DO NOT USE lstrcpy() for the copy.
     */
    RtlCopyMemory(lpStr, lpszText, cchTextLen*sizeof(WCHAR));
    lpStr = lpStr + cchTextLen;
    if (!fIsOrdNum) {
        *lpStr = TEXT('\0');    // NULL terminate the string
        lpStr = (LPWSTR)NextWordBoundary(lpStr + 1);
    }

    *lpStr++ = 0;           // sizeof control data (there is none)

    return NextDWordBoundary(lpStr);
}


/***************************************************************************\
* MB_FindDlgTemplateSize
*
* This routine computes the amount of memory that will be needed for the
* messagebox's dialog template structure.  The dialog template has several
* required and optional records.  The dialog manager expects each record to
* be DWORD aligned so any necessary padding is also accounted for.
*
* (header - required)
* DLGTEMPLATE (header) + 1 menu byte + 1 pad + 1 class byte + 1 pad
* szCaption + 0 term + DWORD alignment
*
* (static icon control - optional)
* DLGITEMTEMPLATE + 1 class byte + 1 pad + (0xFF00 + icon ordinal # [szText]) +
* UINT alignment + 1 control data length byte (0) + DWORD alignment
*
* (pushbutton controls - variable, but at least one required)
* DLGITEMTEMPLATE + 1 class byte + 1 pad + length of button text +
* UINT alignment + 1 control data length byte (0) + DWORD alignment
*
* (static text control - optional)
* DLGITEMTEMPLATE + 1 class byte + 1 pad + length of text +
* UINT alignment + 1 control data length byte (0) + DWORD alignment
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

UINT MB_FindDlgTemplateSize( LPMSGBOXDATA   lpmb )
{
    ULONG_PTR cbLen;
    UINT cbT;
    UINT i;
    UINT wCount;

    wCount = lpmb->cButtons;

    /*
     * Start with dialog header's size.
     */
    cbLen = (ULONG_PTR)NextWordBoundary(sizeof(DLGTEMPLATE) + sizeof(WCHAR));
    cbLen = (ULONG_PTR)NextWordBoundary(cbLen + sizeof(WCHAR));
    cbLen += wcslen(lpmb->lpszCaption) * sizeof(WCHAR) + sizeof(WCHAR);
    cbLen += sizeof(WORD);                   // Font height
    cbLen = (ULONG_PTR)NextDWordBoundary(cbLen);

    /*
     * Check if an Icon is present.
     */
    if (lpmb->dwStyle & MB_ICONMASK)
        cbLen += (ULONG_PTR)NextDWordBoundary(sizeof(DLGITEMTEMPLATE) + 7 * sizeof(WCHAR));

    /*
     * Find the number of buttons in the msg box.
     */
    for (i = 0; i < wCount; i++) {
        cbLen = (ULONG_PTR)NextWordBoundary(cbLen + sizeof(DLGITEMTEMPLATE) +
                (2 * sizeof(WCHAR)));
        cbT = (wcslen(lpmb->ppszButtonText[i]) + 1) * sizeof(WCHAR);
        cbLen = (ULONG_PTR)NextWordBoundary(cbLen + cbT);
        cbLen += sizeof(WCHAR);
        cbLen = (ULONG_PTR)NextDWordBoundary(cbLen);
    }

    /*
     * Add in the space required for the text message (if there is one).
     */
    if (lpmb->lpszText != NULL) {
        cbLen = (ULONG_PTR)NextWordBoundary(cbLen + sizeof(DLGITEMTEMPLATE) +
                (2 * sizeof(WCHAR)));
        cbT = (wcslen(lpmb->lpszText) + 1) * sizeof(WCHAR);
        cbLen = (ULONG_PTR)NextWordBoundary(cbLen + cbT);
        cbLen += sizeof(WCHAR);
        cbLen = (ULONG_PTR)NextDWordBoundary(cbLen);
    }

    return (UINT)cbLen;
}

/***************************************************************************\
* MB_GetIconOrdNum
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

UINT MB_GetIconOrdNum(
    UINT rgBits)
{
    switch (rgBits & MB_ICONMASK) {
    case MB_USERICON:
    case MB_ICONHAND:
        return PtrToUlong(IDI_HAND);

    case MB_ICONQUESTION:
        return PtrToUlong(IDI_QUESTION);

    case MB_ICONEXCLAMATION:
        return PtrToUlong(IDI_EXCLAMATION);

    case MB_ICONASTERISK:
        return PtrToUlong(IDI_ASTERISK);
    }

    return 0;
}

/***************************************************************************\
* MB_GetString
*
* History:
*  1-24-95 JerrySh      Created.
\***************************************************************************/

LPWSTR MB_GetString(
    UINT wBtn)
{
    if (wBtn < MAX_SEB_STYLES)
        return GETGPSIMBPSTR(wBtn);

    RIPMSG1(RIP_ERROR, "Invalid wBtn: %d", wBtn);

    return NULL;
}

/***************************************************************************\
* MB_DlgProc
*
* Returns: TRUE  - message processed
*          FALSE - message not processed
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

INT_PTR MB_DlgProcWorker(
    HWND hwndDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    HWND hwndT;
    int iCount;
    LPMSGBOXDATA lpmb;
    HWND hwndOwner;
    PVOID lpfnCallback;
    PWND pwnd;

    switch (wMsg) {
    case WM_CTLCOLORDLG:
    case WM_CTLCOLORSTATIC:
        if ((pwnd = ValidateHwnd(hwndDlg)) == NULL)
            return 0L;
        return DefWindowProcWorker(pwnd, WM_CTLCOLORMSGBOX,
                                   wParam, lParam, fAnsi);

    case WM_INITDIALOG:

        lpmb = (LPMSGBOXDATA)lParam;
        SetWindowLongPtr(hwndDlg, GWLP_USERDATA, (ULONG_PTR)lParam);

        NtUserCallHwnd(hwndDlg, SFI_SETMSGBOX);

        /*
         * Create the message box atoms, if we haven't done it yet
         */
        if (atomBwlProp == 0) {
            atomBwlProp = AddAtomW(WINDOWLIST_PROP_NAME);
            atomMsgBoxCallback = AddAtomW(MSGBOX_CALLBACK);

            if (atomBwlProp == 0 || atomMsgBoxCallback == 0) {
                RIPMSG0(RIP_WARNING, "MB_DlgProcWorker: AddAtomW failed.  Out of memory?");
            }
        }

        if (lpmb->dwStyle & MB_HELP) {
            NtUserSetWindowContextHelpId(hwndDlg, lpmb->dwContextHelpId);
            //See if there is an app supplied callback.
            if(lpmb->lpfnMsgBoxCallback)
                SetProp(hwndDlg, MAKEINTATOM(atomMsgBoxCallback),
                            lpmb->lpfnMsgBoxCallback);
        }

        if (lpmb->dwStyle & MB_TOPMOST)
            NtUserSetWindowPos(hwndDlg, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);

        if (lpmb->dwStyle & MB_USERICON) {
            SendDlgItemMessage(hwndDlg, IDUSERICON, STM_SETICON, (WPARAM)(lpmb->lpszIcon), 0);
            iCount = ALERT_SYSTEM_WARNING;
        } else {
            /*
             * Generate an alert notification
             */
            switch (lpmb->dwStyle & MB_ICONMASK) {
            case MB_ICONWARNING:
                iCount = ALERT_SYSTEM_WARNING;
                break;

            case MB_ICONQUESTION:
                iCount = ALERT_SYSTEM_QUERY;
                break;

            case MB_ICONERROR:
                iCount = ALERT_SYSTEM_ERROR;
                break;

            case MB_ICONINFORMATION:
            default:
                iCount = ALERT_SYSTEM_INFORMATIONAL;
                break;
            }
        }

        if (FWINABLE()) {
            NotifyWinEvent(EVENT_SYSTEM_ALERT, hwndDlg, OBJID_ALERT, iCount);
        }

#ifdef LATER
// darrinm - 06/17/91
// SYSMODAL dialogs are history for now.

        /*
         * Check if the Dialog box is a Sys Modal Dialog Box
         */
        if (GetWindowLong(hwndDlg, GWL_STYLE) & DS_SYSMODAL, FALSE)
            SetSysModalWindow(hwndDlg);
#endif

        if ((lpmb->hwndOwner == NULL) &&
                ((lpmb->dwStyle & MB_MODEMASK) == MB_TASKMODAL)) {
            StartTaskModalDialog(hwndDlg);
        }

        /*
         * Set focus on the default button
         */
        hwndT = GetWindow(hwndDlg, GW_CHILD);
        iCount = lpmb->DefButton;
        while (iCount--)
            hwndT = GetWindow(hwndT, GW_HWNDNEXT);

        NtUserSetFocus(hwndT);

        //
        // If this dialogbox does not contain a IDCANCEL button, then
        // remove the CLOSE command from the system menu.
        // Bug #4445, --SANKAR-- 09-13-89 --
        //
        if (lpmb->CancelId == 0)
        {
            HMENU hMenu;
            if (hMenu = NtUserGetSystemMenu(hwndDlg, FALSE))
                NtUserDeleteMenu(hMenu, SC_CLOSE, (UINT)MF_BYCOMMAND);
        }

        if ((lpmb->dwStyle & MB_TYPEMASK) == MB_OK)
        {
            //
            // Make the ID of OK button to be CANCEL, because we want
            // the ESC to terminate the dialogbox; GetDlgItem32() will
            // not fail, because this is MB_OK messagebox!
            //

            hwndDlg = GetDlgItem(hwndDlg, IDOK);

            if (hwndDlg != NULL) {
            //    hwndDlg->hMenu = (HMENU)IDCANCEL;
                SetWindowLongPtr(hwndDlg, GWLP_ID, IDCANCEL);
            } else {
                RIPMSG0(RIP_WARNING, "MB_DlgProcWorker - IDOK control not found");
            }
        }

        /*
         * We have changed the input focus
         */
        return FALSE;

    case WM_HELP:
        // When user hits an F1 key, it results in this message.
        // It is possible that this MsgBox has a callback instead of a
        // parent. So, we must behave as if the user hit the HELP button.

        goto  MB_GenerateHelp;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
           //
           // Check if a control exists with the given ID; This
           // check is needed because DlgManager returns IDCANCEL
           // blindly when ESC is pressed even if a button with
           // IDCANCEL is not present.
           // Bug #4445 --SANKAR--09-13-1989--
           //
           if (!GetDlgItem(hwndDlg, LOWORD(wParam)))
              return FALSE;


           // else FALL THRO....This is intentional.
        case IDABORT:
        case IDIGNORE:
        case IDNO:
        case IDRETRY:
        case IDYES:
        case IDTRYAGAIN:
        case IDCONTINUE:
           EndTaskModalDialog(hwndDlg);
           EndDialog(hwndDlg, LOWORD(wParam));
             break;
        case IDHELP:
MB_GenerateHelp:
                // Generate the WM_HELP message and send it to owner or callback
           hwndOwner = NULL;

           // Check if there is an app supplied callback for this MsgBox
           if(!(lpfnCallback = (PVOID)GetProp(hwndDlg,
                                  MAKEINTATOM(atomMsgBoxCallback)))) {
               // If not, see if we need to inform the parent.
               hwndOwner = GetWindow(hwndDlg, GW_OWNER);
#ifdef LATER
               // Chicagoism
               if (hwndOwner && hwndOwner == GetDesktopWindow())
                   hwndOwner = NULL;
#endif
           }

                // See if we need to generate the Help message or call back.
           if (hwndOwner || lpfnCallback) {
               SendHelpMessage(hwndOwner, HELPINFO_WINDOW, IDHELP,
                   hwndDlg, NtUserGetWindowContextHelpId(hwndDlg), lpfnCallback);
           }
           break;

        default:
            return(FALSE);
            break;
        }
        break;

    case WM_COPY:
        MB_CopyToClipboard(hwndDlg);
        break;

    default:
        return FALSE;
    }

    return TRUE;
}

INT_PTR WINAPI MB_DlgProcA(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MB_DlgProcWorker(hwnd, message, wParam, lParam, TRUE);
}

INT_PTR WINAPI MB_DlgProcW(
    HWND hwnd,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    return MB_DlgProcWorker(hwnd, message, wParam, lParam, FALSE);
}


/***************************************************************************\
* StartTaskModalDialog
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

void StartTaskModalDialog(
    HWND hwndDlg)
{
    int cHwnd;
    HWND *phwnd;
    HWND *phwndList, *phwndEnd;
    HWND hwnd;
    PWND pwnd;

    /*
     * Get the hwnd list.  It is returned in a block of memory
     * allocated with LocalAlloc.
     */
    if ((cHwnd = BuildHwndList(NULL, NULL, FALSE, GetCurrentThreadId(), &phwndList)) == 0) {
        return;
    }
    /*
     * If atomBwlProp couldn't be added in WM_INITDIALOG processing, SetProp will fail
     * and we need to free the hwndList as EndTaskModalDialog will not be able to do that
     * MCostea 226543
     */
    if (!SetProp(hwndDlg, MAKEINTATOM(atomBwlProp), (HANDLE)phwndList)) {
        UserLocalFree(phwndList);
        return;
    }

    phwndEnd = phwndList + cHwnd;
    for (phwnd = phwndList; phwnd < phwndEnd; phwnd++) {
        if ((hwnd = *phwnd) == NULL || (pwnd = RevalidateHwnd(hwnd)) == NULL)
            continue;

        /*
         * if the window belongs to the current task and is enabled, disable
         * it.  All other windows are NULL'd out, to prevent their being
         * enabled later
         */
        if (!TestWF(pwnd, WFDISABLED) && DIFFWOWHANDLE(hwnd, hwndDlg)) {
            NtUserEnableWindow(hwnd, FALSE);
        } else {
            *phwnd = NULL;
        }
    }
}


/***************************************************************************\
* EndTaskModalDialog
*
* History:
* 11-20-90 DarrinM      Ported from Win 3.0 sources.
\***************************************************************************/

void EndTaskModalDialog(
    HWND hwndDlg)
{
    HWND *phwnd;
    HWND *phwndList;
    HWND hwnd;

    phwndList = (HWND *)GetProp(hwndDlg, MAKEINTATOM(atomBwlProp));

    if (phwndList == NULL)
        return;

    RemoveProp(hwndDlg, MAKEINTATOM(atomBwlProp));

    for (phwnd = phwndList; *phwnd != (HWND)1; phwnd++) {
        if ((hwnd = *phwnd) != NULL) {
            NtUserEnableWindow(hwnd, TRUE);
        }
    }

    UserLocalFree(phwndList);
}

#ifdef _JANUS_
/***************************************************************************\
* ErrorMessageInst
*
* Instrument routine for recording error msg
*
* Returns: TRUE  - Instrument error msg Success
*          FALSE - Fail
*
* History:
*   8-5-98 Chienho      Created
\***************************************************************************/

BOOL ErrorMessageInst(
     LPMSGBOXDATA pMsgBoxParams)
{
    ERROR_ELEMENT ErrEle;
    WCHAR *pwcs;
    PVOID ImageBase;
    PIMAGE_NT_HEADERS NtHeaders;
    BOOL rc;

    /*
     * Check if the MessageBox style is within the logged severity level
     */
    switch (pMsgBoxParams->dwStyle & MB_ICONMASK) {
    case MB_ICONHAND:
        /*
         * when EMI is enabled, we at least log error messages
         */
        break;
    case MB_ICONEXCLAMATION:
        if (gdwEMIControl > EMI_SEVERITY_WARNING) {
            rc = TRUE;
            goto End;
        }
        break;
    case MB_ICONQUESTION:
        if (gdwEMIControl > EMI_SEVERITY_QUESTION) {
            rc = TRUE;
            goto End;
        }
        break;
    case MB_ICONASTERISK:
        if (gdwEMIControl > EMI_SEVERITY_INFORMATION) {
            rc = TRUE;
            goto End;
        }
        break;
    case MB_USERICON:
        if (gdwEMIControl > EMI_SEVERITY_USER) {
            rc = TRUE;
            goto End;
        }
        break;
    default:
        if (gdwEMIControl > EMI_SEVERITY_ALL) {
            rc = TRUE;
            goto End;
        }
        break;
    }

    if (gdwEMIThreadID != GETTHREADID()) {
        rc = FALSE;
        goto End;
    }
    RtlZeroMemory(&ErrEle, sizeof(ErrEle));

    /*
     * get last error first, check with FormatMessage???
     */
    ErrEle.dwErrorCode = GetLastError();

    /*
     * get return address
     */

    ErrEle.ReturnAddr = gpReturnAddr;

    /*
     * get the process image name
     */
    if (GetModuleFileName(NULL, ErrEle.ProcessName, MAX_PATH)) {
        pwcs = wcsrchr(ErrEle.ProcessName, TEXT('\\'));
        if (pwcs) {
            pwcs++;
            lstrcpy(ErrEle.ProcessName, pwcs);
        }
    } else {
        lstrcpy(ErrEle.ProcessName, szUnknown);
    }

    /*
     * get the window title
     */
    GetWindowTextW(pMsgBoxParams->hwndOwner, ErrEle.WindowTitle, TITLE_SIZE);
    if (!(*(ErrEle.WindowTitle))) {
        lstrcpy(ErrEle.WindowTitle, szUnknown);
    }

    /*
     * get messagebox data
     */
    ErrEle.lpszText = (LPWSTR)pMsgBoxParams->lpszText;
    ErrEle.lpszCaption = (LPWSTR)pMsgBoxParams->lpszCaption;
    ErrEle.dwStyle = pMsgBoxParams->dwStyle;

    /*
     * resolve the module name of caller
     */
    if (!RtlPcToFileHeader((PVOID)ErrEle.ReturnAddr, &ImageBase)) {
        RIPMSG0(RIP_WARNING, "ErrorMessageInst: Can't find Caller");
        ErrEle.BaseAddr = (PVOID)-1;
        ErrEle.dwImageSize = -1;
        lstrcpy(ErrEle.CallerModuleName, szUnknown);
    } else {
        ErrEle.BaseAddr = ImageBase;
        if (GetModuleFileName((HMODULE)ImageBase, ErrEle.CallerModuleName, MAX_PATH)) {
            pwcs = wcsrchr(ErrEle.CallerModuleName, TEXT('\\'));
            if (pwcs) {
                pwcs++;
                lstrcpy(ErrEle.CallerModuleName, pwcs);
            }
        } else {
            lstrcpy(ErrEle.CallerModuleName, szUnknown);
        }
        NtHeaders = RtlImageNtHeader(ImageBase);
        if (NtHeaders == NULL) {
            ErrEle.dwImageSize = -1;
        } else {
            ErrEle.dwImageSize = NtHeaders->OptionalHeader.SizeOfImage;
        }
    }
    /*
     * Register the event if we haven't done so already.
     * Since RegisterEventSource is supported by a service, we must not hold
     * any locks while making this call. Hence we might have several threads
     * registering the event simultaneously.
     */

    if (!gEventSource) {
        gEventSource = RegisterLogSource(L"Error Instrument");
        if (!gEventSource) {
            ErrEle.dwErrorCode = GetLastError();
            rc = FALSE;
        }
    }

    /*
     * report event
     */
    if(gEventSource) {
       rc = LogMessageBox(&ErrEle);
    }

    /*
     * allow to process another event log again
     */

    InterlockedExchangePointer(&gdwEMIThreadID, 0);

End:
    return rc;
}

/***************************************************************************\
* InitInstrument
*
* Returns: TRUE  - Initialization Success
*          FALSE - Initialization Fail
*
\***************************************************************************/
BOOL InitInstrument(
    LPDWORD lpEMIControl)
{
    NTSTATUS Status;
    HKEY hKeyEMI = NULL;
    UNICODE_STRING UnicodeStringEMIKey;
    UNICODE_STRING UnicodeStringEnable;
    UNICODE_STRING UnicodeStringStyle;
    OBJECT_ATTRIBUTES ObjA;
    DWORD EMIEnable = 0; //means disable
    DWORD EMISeverity;
    struct {
        KEY_VALUE_PARTIAL_INFORMATION;
        LARGE_INTEGER;
    } EMIValueInfo;
    DWORD dwDisposition;

    RtlInitUnicodeString(&UnicodeStringEMIKey, szEMIKey);
    InitializeObjectAttributes(&ObjA, &UnicodeStringEMIKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

    Status = NtOpenKey(&hKeyEMI, KEY_READ, &ObjA);
    if (!NT_SUCCESS(Status)) {
        /*
         * Key doesn't exist, assume disable
         */
        return FALSE;
    }

    /*
     * read the logging enable and setting
     */
    RtlInitUnicodeString(&UnicodeStringEnable, szEMIEnable);
    Status = NtQueryValueKey(hKeyEMI,
                     &UnicodeStringEnable,
                     KeyValuePartialInformation,
                     &EMIValueInfo,
                     sizeof(EMIValueInfo),
                     &dwDisposition);

    if (NT_SUCCESS(Status)) {

        RtlCopyMemory(&EMIEnable, &EMIValueInfo.Data, sizeof(EMIEnable));

        RtlInitUnicodeString(&UnicodeStringStyle, szEMISeverity);
        Status = NtQueryValueKey(hKeyEMI,
                         &UnicodeStringStyle,
                         KeyValuePartialInformation,
                         &EMIValueInfo,
                         sizeof(EMIValueInfo),
                         &dwDisposition);

        if (NT_SUCCESS(Status)) {
            RtlCopyMemory(&EMISeverity, &EMIValueInfo.Data, sizeof(EMISeverity));
            /*
             * Validate data
             */
            if (EMISeverity > EMI_SEVERITY_MAX_VALUE) {
                EMISeverity = EMI_SEVERITY_MAX_VALUE;
            }
        } else {
            /*
             * default severity for instrument
             */
            EMISeverity = EMI_SEVERITY_WARNING;
        }
        *lpEMIControl = EMISeverity;
    }

    /*
     * read default message reply enable
     */
    RtlInitUnicodeString(&UnicodeStringEnable, szDMREnable);
    Status = NtQueryValueKey(hKeyEMI,
                     &UnicodeStringEnable,
                     KeyValuePartialInformation,
                     &EMIValueInfo,
                     sizeof(EMIValueInfo),
                     &dwDisposition);

    if (NT_SUCCESS(Status)) {
        RtlCopyMemory(&gfDMREnable, &EMIValueInfo.Data, sizeof(gfDMREnable));
    }

    NtClose(hKeyEMI);

    if (EMIEnable) {

          /*
           * add eventlog file
           */
          if (NT_SUCCESS(CreateLogSource())) {
              return TRUE;
          }
    }
    return FALSE;
}

/***************************************************************************\
* CreateLogSource
*
* Create the event source for eventlog
* Return : NTSTATUS
*
\***************************************************************************/
NTSTATUS CreateLogSource()
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeStringEventKey;
    OBJECT_ATTRIBUTES ObjA;
    HKEY hKeyEvent = NULL;
    UNICODE_STRING UnicodeString;
    DWORD dwDisposition;


    RtlInitUnicodeString(&UnicodeStringEventKey, szEventKey);
    InitializeObjectAttributes(&ObjA, &UnicodeStringEventKey, OBJ_CASE_INSENSITIVE, NULL, NULL);

    if (NT_SUCCESS(Status = NtOpenKey(&hKeyEvent, KEY_READ, &ObjA))) {

        struct {
            KEY_VALUE_PARTIAL_INFORMATION KeyInfo;
            WCHAR awchMsgFileName[256];
        } MsgFile;

        RtlInitUnicodeString(&UnicodeString, szEventMsgFile);

        Status = NtQueryValueKey(hKeyEvent,
                                 &UnicodeString,
                                 KeyValuePartialInformation,
                                 &MsgFile,
                                 sizeof MsgFile,
                                 &dwDisposition);
        if (NT_SUCCESS(Status)) {
            Status = lstrcmpi((LPWSTR)MsgFile.KeyInfo.Data, L"%SystemRoot%\\System32\\user32.dll");
        }
        NtClose(hKeyEvent);
    }

    return Status;
}

/***************************************************************************\
* RegisterLogSource
*
* Get eventlog apis from advapi32.dll and register the event source
* Return : HANDLE of event source
*
\***************************************************************************/
HANDLE RegisterLogSource(
    LPWSTR lpszSourceName)
{
    /*
     * If we haven't already dynamically linked to advadpi32.dll, do it now.
     */
    if (gfnRegisterEventSource == NULL) {
        HINSTANCE hAdvApi;
        FARPROC fnRegisterEventSource;
        FARPROC fnDeregisterEventSource;
        FARPROC fnReportEvent;
        FARPROC fnGetTokenInformation;
        FARPROC fnOpenProcessToken;
        FARPROC fnOpenThreadToken;

        /*
         * Try to load the DLL and function pointers.
         */
        if ((hAdvApi = LoadLibrary(L"advapi32.dll")) == NULL) {
            return NULL;
        }
        fnRegisterEventSource = GetProcAddress(hAdvApi, "RegisterEventSourceW");
        fnDeregisterEventSource = GetProcAddress(hAdvApi, "DeregisterEventSource");
        fnReportEvent = GetProcAddress(hAdvApi, "ReportEventW");
        fnGetTokenInformation = GetProcAddress(hAdvApi, "GetTokenInformation");
        fnOpenProcessToken = GetProcAddress(hAdvApi, "OpenProcessToken");
        fnOpenThreadToken = GetProcAddress(hAdvApi, "OpenThreadToken");
        if (!fnRegisterEventSource || !fnDeregisterEventSource || !fnReportEvent ||
            !fnGetTokenInformation || !fnOpenProcessToken || !fnOpenThreadToken) {
            FreeLibrary(hAdvApi);
            return NULL;
        }

        /*
         * Update the global function pointers if they're not set already.
         */
        if (gfnRegisterEventSource == NULL) {
            gfnReportEvent = fnReportEvent;
            gfnDeregisterEventSource = fnDeregisterEventSource;
            gfnGetTokenInformation = fnGetTokenInformation;
            gfnOpenProcessToken = fnOpenProcessToken;
            gfnOpenThreadToken = fnOpenThreadToken;
            // This must be last since we test it above
            gfnRegisterEventSource = fnRegisterEventSource;
            ghAdvApi = hAdvApi;
            hAdvApi = NULL;
        }

        /*
         * If another thread beat us to it, free the library.
         */
        if (hAdvApi) {
            FreeLibrary(hAdvApi);
        }
    }

    /*
     * Let's be paranoid and verify we loaded everything correctly.
     */
    UserAssert(gfnRegisterEventSource != NULL);
    UserAssert(gfnDeregisterEventSource != NULL);
    UserAssert(gfnReportEvent != NULL);
    UserAssert(gfnGetTokenInformation != NULL);
    UserAssert(gfnOpenProcessToken != NULL);
    UserAssert(gfnOpenThreadToken != NULL);

    /*
     * Call the real function.
     */
    return (HANDLE)gfnRegisterEventSource(NULL, lpszSourceName);
}
/***************************************************************************\
* LogMessageBox
*
* Output error message record into eventlog
*
\***************************************************************************/
BOOL LogMessageBox(
    LPERROR_ELEMENT lpErrEle)
{

    LPWSTR      lps[8];
    DWORD       dwData[2];
    WCHAR       BaseAddress[19];
    WCHAR       ImageSize[19];
    WCHAR       ReturnAddress[19];
    PTOKEN_USER pTokenUser = NULL;
    PSID        pSid = NULL;
    BOOL        rc;

    lps[0] = lpErrEle->ProcessName;
    lps[1] = lpErrEle->WindowTitle;
    lps[2] = lpErrEle->lpszCaption;
    lps[3] = lpErrEle->lpszText;
    lps[4] = lpErrEle->CallerModuleName;
    wsprintf(BaseAddress, L"%-#16p", lpErrEle->BaseAddr);
    lps[5] = BaseAddress;
    wsprintf(ImageSize, L"%-#16lX", lpErrEle->dwImageSize);
    lps[6] = ImageSize;
    wsprintf(ReturnAddress, L"%-#16p", lpErrEle->ReturnAddr);
    lps[7] = ReturnAddress;

    dwData[0] = lpErrEle->dwStyle;
    dwData[1] = lpErrEle->dwErrorCode;

    if( GetUserSid(&pTokenUser) )
        pSid = pTokenUser->User.Sid;

    UserAssert(gEventSource != NULL);
    rc = (BOOL)gfnReportEvent(gEventSource, EVENTLOG_INFORMATION_TYPE, 0,
            STATUS_LOG_ERROR_MSG, pSid, sizeof(lps) / sizeof(*lps),
            sizeof(dwData), lps, dwData);

    if( pTokenUser ) {

        VirtualFree( pTokenUser, 0, MEM_RELEASE );
    }

    return rc;
}

/***************************************************************************\
* GetUserSid
*
*  Well, actually it gets a pointer to a newly allocated TOKEN_USER,
*  which contains a SID, somewhere.
*  Caller must remember to free it when it's been used.
*
* History:
*   10-16-98 Chienho      stole from spooler
*
\***************************************************************************/
BOOL GetUserSid(
     PTOKEN_USER *ppTokenUser)
{
    HANDLE      TokenHandle;
    PTOKEN_USER pTokenUser = NULL;
    DWORD       cbTokenUser = 0;
    DWORD       cbNeeded;
    BOOL        bRet = FALSE;

    if ( !GetTokenHandle( &TokenHandle) ) {
        return FALSE;
    }

    bRet = (BOOL)gfnGetTokenInformation( TokenHandle,
                                         TokenUser,
                                         pTokenUser,
                                         cbTokenUser,
                                         &cbNeeded);

    /* We've passed a NULL pointer and 0 for the amount of memory
     * allocated.  We expect to fail with bRet = FALSE and
     * GetLastError = ERROR_INSUFFICIENT_BUFFER. If we do not
     * have these conditions we will return FALSE
     */

    if ( !bRet && (GetLastError() == ERROR_INSUFFICIENT_BUFFER) ) {

        pTokenUser = VirtualAlloc(NULL, cbNeeded, MEM_COMMIT, PAGE_READWRITE);

        if ( pTokenUser == NULL ) {

            goto GetUserSidDone;
        }

        cbTokenUser = cbNeeded;

        bRet = (BOOL)gfnGetTokenInformation( TokenHandle,
                                             TokenUser,
                                             pTokenUser,
                                             cbTokenUser,
                                             &cbNeeded );

    } else {

        /*
         * Any other case -- return FALSE
         */
        bRet = FALSE;
    }

GetUserSidDone:
    if ( bRet == TRUE ) {

        *ppTokenUser  = pTokenUser;

    } else if ( pTokenUser ) {

        VirtualFree( pTokenUser, 0, MEM_RELEASE );
    }

    CloseHandle( TokenHandle );

    return bRet;
}

/***************************************************************************\
* GetTokenHandle
*
* Get handle of token for current thread
*
\***************************************************************************/
BOOL
GetTokenHandle(
    PHANDLE pTokenHandle
    )
{
    if (!gfnOpenThreadToken(GetCurrentThread(),
                            TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                            TRUE,
                            pTokenHandle)) {

        if (GetLastError() == ERROR_NO_TOKEN) {

            /* This means we are not impersonating anybody.
             * Instead, lets get the token out of the process.
             */

            if (!gfnOpenProcessToken(GetCurrentProcess(),
                                     TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                     pTokenHandle)) {

                return FALSE;
            }

        } else

            return FALSE;
    }

    return TRUE;
}

#endif //_JANUS_
