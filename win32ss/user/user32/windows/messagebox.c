/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * PROJECT:         ReactOS user32.dll
 * FILE:            win32ss/user/user32/windows/messagebox.c
 * PURPOSE:         Message Boxes
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Thomas Weidenmueller (w3seek@users.sourceforge.net)
 *                  Hermes Belusca-Maito
 * UPDATE HISTORY:
 *      2003/07/28  Added some NT features
 *      2003/07/27  Code ported from wine
 *      09-05-2001  CSH  Created
 */

#include <user32.h>
#include <ndk/exfuncs.h>

#include <ntstrsafe.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* DEFINES *******************************************************************/

#define MSGBOX_IDICON   (1088)
#define MSGBOX_IDTEXT   (0xffff)

#define IDI_HANDW          MAKEINTRESOURCEW(32513)
#define IDI_QUESTIONW      MAKEINTRESOURCEW(32514)
#define IDI_EXCLAMATIONW   MAKEINTRESOURCEW(32515)
#define IDI_ASTERISKW      MAKEINTRESOURCEW(32516)
#define IDI_WINLOGOW       MAKEINTRESOURCEW(32517)


/* MessageBox metrics */

#define BTN_CX (75)
#define BTN_CY (23)

#define MSGBOXEX_SPACING        (16)
#define MSGBOXEX_BUTTONSPACING  (6)
#define MSGBOXEX_MARGIN         (12)
#define MSGBOXEX_MAXBTNSTR      (32)
#define MSGBOXEX_MAXBTNS        (4)

/* Rescale logical coordinates */
#define RESCALE_X(_x, _units)   (((_x) * 4 + (_units).cx - 1) / (_units).cx)
#define RESCALE_Y(_y, _units)   (((_y) * 8 + (_units).cy - 1) / (_units).cy)


/* MessageBox button helpers */

#define DECLARE_MB_1(_btn0) \
    { 1, { ID##_btn0, 0, 0 }, { IDS_##_btn0, 0, 0 } }

#define DECLARE_MB_2(_btn0, _btn1) \
    { 2, { ID##_btn0, ID##_btn1, 0 }, { IDS_##_btn0, IDS_##_btn1, 0 } }

#define DECLARE_MB_3(_btn0, _btn1, _btn2) \
    { 3, { ID##_btn0, ID##_btn1, ID##_btn2 }, { IDS_##_btn0, IDS_##_btn1, IDS_##_btn2 } }

typedef struct _MSGBTNINFO
{
    DWORD btnCnt;
    INT   btnIdx[MSGBOXEX_MAXBTNS];
    UINT  btnIds[MSGBOXEX_MAXBTNS];
} MSGBTNINFO, *PMSGBTNINFO;

/* Default MessageBox buttons */
static const MSGBTNINFO MsgBtnInfo[] =
{
    /* MB_OK (0) */
    DECLARE_MB_1(OK),
    /* MB_OKCANCEL (1) */
    DECLARE_MB_2(OK, CANCEL),
    /* MB_ABORTRETRYIGNORE (2) */
    DECLARE_MB_3(ABORT, RETRY, IGNORE),
    /* MB_YESNOCANCEL (3) */
    DECLARE_MB_3(YES, NO, CANCEL),
    /* MB_YESNO (4) */
    DECLARE_MB_2(YES, NO),
    /* MB_RETRYCANCEL (5) */
    DECLARE_MB_2(RETRY, CANCEL),
    /* MB_CANCELTRYCONTINUE (6) */
    DECLARE_MB_3(CANCEL, TRYAGAIN, CONTINUE)
};


/* INTERNAL FUNCTIONS ********************************************************/

static UINT
LoadAllocStringW(
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID,
    OUT PWSTR* pString,
    IN PCWSTR pDefaultString OPTIONAL)
{
    UINT Length;
    PCWSTR pStr;

    /* Try to load the string from the resource */
    Length = LoadStringW(hInstance, uID, (LPWSTR)&pStr, 0);
    if (Length == 0)
    {
        /* If the resource string was not found, use the fallback default one */

        if (!pDefaultString)
        {
            /* None was specified, return NULL */
            *pString = NULL;
            return 0;
        }

        pStr = pDefaultString;
        Length = wcslen(pStr);
    }

    /* Allocate a new buffer, adding a NULL-terminator */
    *pString = RtlAllocateHeap(RtlGetProcessHeap(), 0, (Length + 1) * sizeof(WCHAR));
    if (!*pString)
        return 0;

    /* Copy the string, NULL-terminated */
    RtlStringCchCopyNW(*pString, Length + 1, pStr, Length);
    return Length;
}

static VOID MessageBoxTextToClipboard(HWND DialogWindow)
{
    HWND hwndText;
    PMSGBOXDATA mbd;
    int cchTotal, cchTitle, cchText, cchButton, i, n, cchBuffer;
    LPWSTR pszBuffer, pszBufferPos, pMessageBoxText, pszTitle, pszText, pszButton;
    WCHAR szButton[MSGBOXEX_MAXBTNSTR];
    HGLOBAL hGlobal;

    static const WCHAR szLine[] = L"---------------------------\r\n";

    mbd = (PMSGBOXDATA)GetPropW(DialogWindow, L"ROS_MSGBOX");
    hwndText = GetDlgItem(DialogWindow, MSGBOX_IDTEXT);
    cchTitle = GetWindowTextLengthW(DialogWindow) + 1;
    cchText = GetWindowTextLengthW(hwndText) + 1;

    if (!mbd)
        return;

    pMessageBoxText = (LPWSTR)RtlAllocateHeap(GetProcessHeap(), 0, (cchTitle + cchText) * sizeof(WCHAR));

    if (pMessageBoxText == NULL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
        return;
    }

    pszTitle = pMessageBoxText;
    pszText = pMessageBoxText + cchTitle;

    if (GetWindowTextW(DialogWindow, pszTitle, cchTitle) == 0 ||
        GetWindowTextW(hwndText, pszText, cchText) == 0)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
        return;
    }

    /*
     * Calculate the total buffer size.
     */
    cchTotal = 6 + cchTitle + cchText + (lstrlenW(szLine) * 4) + (mbd->dwButtons * MSGBOXEX_MAXBTNSTR + 3);

    hGlobal = GlobalAlloc(GHND, cchTotal * sizeof(WCHAR));

    pszBuffer = (LPWSTR)GlobalLock(hGlobal);

    if (pszBuffer == NULL)
    {
        RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
        GlobalFree(hGlobal);
        return;
    }

    /*
     * First format title and text.
     * ------------------
     * Title
     * ------------------
     * Text
     * ------------------
     */
    cchBuffer = wsprintfW(pszBuffer, L"%s%s\r\n%s%s\r\n%s", szLine, pszTitle, szLine, pszText, szLine);
    pszBufferPos = pszBuffer + cchBuffer;

    for (i = 0; i < mbd->dwButtons; i++)
    {
        GetDlgItemTextW(DialogWindow, mbd->pidButton[i], szButton, MSGBOXEX_MAXBTNSTR);

        cchButton = strlenW(szButton);
        pszButton = szButton;

        /* Skip '&' character. */
        if (szButton[0] == '&')
        {
            pszButton = pszButton + 1;
            cchButton = cchButton - 1;
        }

        for (n = 0; n < cchButton; n++)
            *(pszBufferPos++) = pszButton[n];

        /* Add spaces. */
        *(pszBufferPos++) = L' ';
        *(pszBufferPos++) = L' ';
        *(pszBufferPos++) = L' ';
    }

    wsprintfW(pszBufferPos, L"\r\n%s", szLine);

    GlobalUnlock(hGlobal);

    if (OpenClipboard(DialogWindow))
    {
        EmptyClipboard();
        SetClipboardData(CF_UNICODETEXT, hGlobal);
        CloseClipboard();
    }
    else
    {
        GlobalFree(hGlobal);
    }
    RtlFreeHeap(GetProcessHeap(), 0, pMessageBoxText);
}

static INT_PTR CALLBACK MessageBoxProc(
    HWND hwnd, UINT message,
    WPARAM wParam, LPARAM lParam)
{
    PMSGBOXDATA mbd;

    switch (message)
    {
    case WM_INITDIALOG:
    {
        int Alert;

        mbd = (PMSGBOXDATA)lParam;

        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)mbd);
        NtUserxSetMessageBox(hwnd);

        if (!GetPropW(hwnd, L"ROS_MSGBOX"))
        {
            SetPropW(hwnd, L"ROS_MSGBOX", (HANDLE)lParam);

            if (mbd->mbp.dwContextHelpId)
                SetWindowContextHelpId(hwnd, mbd->mbp.dwContextHelpId);

            if (mbd->mbp.lpszIcon)
            {
                SendDlgItemMessageW(hwnd, MSGBOX_IDICON, STM_SETICON, (WPARAM)(HICON)mbd->mbp.lpszIcon, 0);
                Alert = ALERT_SYSTEM_WARNING;
            }
            else /* Setup the rest of the alerts */
            {
                switch (mbd->mbp.dwStyle & MB_ICONMASK)
                {
                case MB_ICONWARNING:
                    Alert = ALERT_SYSTEM_WARNING;
                    break;
                case MB_ICONERROR:
                    Alert = ALERT_SYSTEM_ERROR;
                    break;
                case MB_ICONQUESTION:
                    Alert = ALERT_SYSTEM_QUERY;
                    break;
                default:
                    Alert = ALERT_SYSTEM_INFORMATIONAL;
                    /* Fall through */
                }
            }
            /* Send out the alert notifications */
            NotifyWinEvent(EVENT_SYSTEM_ALERT, hwnd, OBJID_ALERT, Alert);

            /* Disable the Close menu button if no Cancel button is specified */
            if (mbd->uCancelId == 0)
            {
                HMENU hSysMenu = GetSystemMenu(hwnd, FALSE);
                if (hSysMenu)
                    DeleteMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND);
            }

            /* Set the focus to the default button */
            if (mbd->dwButtons > 0)
            {
                ASSERT(mbd->uDefButton < mbd->dwButtons);
                SetFocus(GetDlgItem(hwnd, mbd->pidButton[mbd->uDefButton]));
            }

            /* Set up the window timer */
            if (mbd->dwTimeout && (mbd->dwTimeout != (UINT)-1))
                SetTimer(hwnd, 0, mbd->dwTimeout, NULL);
        }
        return FALSE;
    }

    case WM_COMMAND:
    {
        UINT i;
        INT_PTR iCtrlId = LOWORD(wParam);

        switch (iCtrlId)
        {
            /* Handle the default message-box buttons */
            case IDOK:
            case IDCANCEL:
                /*
                 * The dialog manager always sends IDCANCEL when the user
                 * presses ESCape. We check here whether the message box
                 * has a CANCEL button, or whether we should fall back to
                 * the OK button, by using the correct uCancelId.
                 */
                if (iCtrlId == IDCANCEL)
                {
                    mbd = (PMSGBOXDATA)GetPropW(hwnd, L"ROS_MSGBOX");
                    if (!mbd)
                        return FALSE; /* Ignore */

                    /* Check whether we can cancel the message box */
                    if (mbd->uCancelId == 0)
                        return TRUE; // FALSE; /* No, ignore */
                    /* Quit with the correct return value */
                    iCtrlId = mbd->uCancelId;
                }
                if (!GetDlgItem(hwnd, iCtrlId))
                    return FALSE; /* Ignore */

            /* Fall through */
            case IDABORT:
            case IDRETRY:
            case IDIGNORE:
            case IDYES:
            case IDNO:
            case IDTRYAGAIN:
            case IDCONTINUE:
                EndDialog(hwnd, iCtrlId);
                return TRUE;

            case IDCLOSE:
                return FALSE; /* Ignore */

            case IDHELP:
            {
                /* Send WM_HELP message to the message-box window */
                HELPINFO hi;
                hi.cbSize = sizeof(hi);
                hi.iContextType = HELPINFO_WINDOW;
                hi.iCtrlId = iCtrlId;
                hi.hItemHandle = (HANDLE)lParam;
                hi.dwContextId = 0;
                GetCursorPos(&hi.MousePos);
                SendMessageW(hwnd, WM_HELP, 0, (LPARAM)&hi);
                return TRUE;
            }

            default:
                break;
        }

        /* Check for any other user-defined buttons */
        mbd = (PMSGBOXDATA)GetPropW(hwnd, L"ROS_MSGBOX");
        if (!mbd)
            return FALSE;

        for (i = 0; i < mbd->dwButtons; ++i)
        {
            if (iCtrlId == mbd->pidButton[i])
            {
                EndDialog(hwnd, iCtrlId);
                return TRUE;
            }
        }

        return FALSE;
    }

    case WM_COPY:
        MessageBoxTextToClipboard(hwnd);
        return TRUE;

    case WM_HELP:
    {
        LPHELPINFO phi = (LPHELPINFO)lParam;
        mbd = (PMSGBOXDATA)GetPropW(hwnd, L"ROS_MSGBOX");
        if (!mbd)
            return FALSE;
        phi->dwContextId = GetWindowContextHelpId(hwnd);

        if (mbd->mbp.lpfnMsgBoxCallback)
        {
            mbd->mbp.lpfnMsgBoxCallback(phi);
        }
        else
        {
            HWND hwndOwner = GetWindow(hwnd, GW_OWNER);
            if (hwndOwner)
                SendMessageW(hwndOwner, WM_HELP, 0, lParam);
        }
        return TRUE;
    }

    case WM_CLOSE:
    {
        mbd = (PMSGBOXDATA)GetPropW(hwnd, L"ROS_MSGBOX");
        if (!mbd)
            return FALSE;

        /* Check whether we can cancel the message box */
        if (mbd->uCancelId == 0)
            return TRUE; /* No, ignore */
        /* Quit with the correct return value */
        EndDialog(hwnd, mbd->uCancelId);
        return TRUE;
    }

    case WM_TIMER:
        if (wParam == 0)
            EndDialog(hwnd, IDTIMEOUT);
        return FALSE;
    }

    return FALSE;
}

static int
MessageBoxTimeoutIndirectW(
    CONST MSGBOXPARAMSW *lpMsgBoxParams, UINT dwTimeout)
{
    int ret = 0;
    UINT i;
    LPWSTR defCaption = NULL;
    MSGBOXDATA mbd;
    MSGBTNINFO Buttons;
    LPCWSTR ButtonText[MSGBOXEX_MAXBTNS];

    // TODO: Check whether the caller is an NT 3.x app and if so, check
    // instead for the MB_SERVICE_NOTIFICATION_NT3X flag and adjust it.
    if (lpMsgBoxParams->dwStyle & MB_SERVICE_NOTIFICATION)
    {
        NTSTATUS Status;
        UNICODE_STRING CaptionU, TextU;
        ULONG Response = ResponseNotHandled; /* HARDERROR_RESPONSE */
        ULONG_PTR MsgBoxParams[4] =
        {
            (ULONG_PTR)&TextU,
            (ULONG_PTR)&CaptionU,
            /*
             * Retrieve the message box flags. Note that we filter out
             * MB_SERVICE_NOTIFICATION to not enter an infinite recursive
             * loop when we will call MessageBox() later on.
             */
            lpMsgBoxParams->dwStyle & ~MB_SERVICE_NOTIFICATION,
            dwTimeout
        };

        /* hwndOwner must be NULL */
        if (lpMsgBoxParams->hwndOwner != NULL)
        {
            ERR("MessageBoxTimeoutIndirectW(MB_SERVICE_NOTIFICATION): hwndOwner is not NULL!\n");
            return 0;
        }

        //
        // FIXME: TODO: Implement the special case for Terminal Services.
        //

        RtlInitUnicodeString(&CaptionU, lpMsgBoxParams->lpszCaption);
        RtlInitUnicodeString(&TextU, lpMsgBoxParams->lpszText);

        Status = NtRaiseHardError(STATUS_SERVICE_NOTIFICATION | HARDERROR_OVERRIDE_ERRORMODE,
                                  ARRAYSIZE(MsgBoxParams),
                                  (1 | 2),
                                  MsgBoxParams,
                                  OptionOk, /* NOTE: This parameter is ignored */
                                  &Response);
        if (!NT_SUCCESS(Status))
        {
            ERR("MessageBoxTimeoutIndirectW(MB_SERVICE_NOTIFICATION): NtRaiseHardError failed, Status = 0x%08lx\n", Status);
            return 0;
        }

        /* Map the returned response to the buttons */
        switch (Response)
        {
            /* Not handled */
            case ResponseReturnToCaller:
            case ResponseNotHandled:
                break;

            case ResponseAbort:
                return IDABORT;
            case ResponseCancel:
                return IDCANCEL;
            case ResponseIgnore:
                return IDIGNORE;
            case ResponseNo:
                return IDNO;
            case ResponseOk:
                return IDOK;
            case ResponseRetry:
                return IDRETRY;
            case ResponseYes:
                return IDYES;
            case ResponseTryAgain:
                return IDTRYAGAIN;
            case ResponseContinue:
                return IDCONTINUE;

            /* Not handled */
            default:
                break;
        }
        return 0;
    }

    ZeroMemory(&mbd, sizeof(mbd));
    memcpy(&mbd.mbp, lpMsgBoxParams, sizeof(mbd.mbp));
    mbd.wLanguageId = (WORD)lpMsgBoxParams->dwLanguageId;
    mbd.dwTimeout   = dwTimeout;

    if (!mbd.mbp.lpszCaption)
    {
        /* No caption, use the default one */
        LoadAllocStringW(User32Instance,
                         IDS_ERROR,
                         &defCaption,
                         L"Error");
        mbd.mbp.lpszCaption = (defCaption ? defCaption : L"Error");
    }

    /* Create the selected buttons; unknown types will fall back to MB_OK */
    i = (lpMsgBoxParams->dwStyle & MB_TYPEMASK);
    if (i >= ARRAYSIZE(MsgBtnInfo))
        i = MB_OK;

    /* Get the buttons IDs */
    Buttons = MsgBtnInfo[i];

    /* Add the Help button */
    if (lpMsgBoxParams->dwStyle & MB_HELP)
    {
        Buttons.btnIdx[Buttons.btnCnt] = IDHELP;
        Buttons.btnIds[Buttons.btnCnt] = IDS_HELP;
        Buttons.btnCnt++;
    }

    ASSERT(Buttons.btnCnt <= MSGBOXEX_MAXBTNS);

    /* Retrieve the pointers to the button labels and find the Cancel button */
    mbd.uCancelId = (i == MB_OK ? IDOK : 0);
    for (i = 0; i < Buttons.btnCnt; ++i)
    {
        // FIXME: Use the strings in the correct language.
        // MB_GetString gives the string in default system language.
        ButtonText[i] = MB_GetString(Buttons.btnIds[i] - IDS_OK); /* or: Buttons.btnIdx[i] - IDOK */
#if 0
        LoadAllocStringW(User32Instance,
                         Buttons.btnIds[i],
                         &ButtonText[i],
                         L"");
#endif
        if (Buttons.btnIdx[i] == IDCANCEL)
            mbd.uCancelId = IDCANCEL;
    }

    mbd.pidButton = Buttons.btnIdx;
    mbd.ppszButtonText = ButtonText;
    mbd.dwButtons = Buttons.btnCnt;

    mbd.uDefButton = ((lpMsgBoxParams->dwStyle & MB_DEFMASK) >> 8);
    /* Make the first button the default button if none other is */
    if (mbd.uDefButton >= mbd.dwButtons)
        mbd.uDefButton = 0;

    /* Call the helper function */
    ret = SoftModalMessageBox(&mbd);

#if 0
    for (i = 0; i < mbd.dwButtons; i++)
    {
        if (ButtonText[i] && *ButtonText[i])
            RtlFreeHeap(RtlGetProcessHeap(), 0, ButtonText[i]);
    }
#endif

    if (defCaption)
        RtlFreeHeap(RtlGetProcessHeap(), 0, defCaption);

    return ret;
}

int
WINAPI
SoftModalMessageBox(IN LPMSGBOXDATA lpMsgBoxData)
{
    int ret = 0;
    MSGBOXDATA mbd;
    LPMSGBOXPARAMSW lpMsgBoxParams = &mbd.mbp;
    DLGTEMPLATE *tpl;
    DLGITEMTEMPLATE *iico, *itxt, *ibtn;
    NONCLIENTMETRICSW nclm;
    LPVOID buf;
    BYTE *dest;
    LPWSTR caption, text;
    HFONT hFont, hOldFont;
    HICON hIcon;
    HWND hDCWnd;
    HDC hDC;
    SIZE units;
    int bufsize, caplen, textlen, i, btnleft, btntop;
    size_t ButtonLen;
    RECT btnrect, txtrect, rc;
    SIZE btnsize;
    POINT iconPos; SIZE iconSize;

    /* Capture the MsgBoxData */
    memcpy(&mbd, lpMsgBoxData, sizeof(mbd));

    /* Load the caption */
    caption = NULL;
    if (lpMsgBoxParams->lpszCaption && IS_INTRESOURCE(lpMsgBoxParams->lpszCaption))
    {
        /* User-defined resource string */
        caplen = LoadAllocStringW(lpMsgBoxParams->hInstance,
                                  PtrToUlong(lpMsgBoxParams->lpszCaption),
                                  &caption,
                                  NULL);
        lpMsgBoxParams->lpszCaption = caption;
    }
    else if (lpMsgBoxParams->lpszCaption)
    {
        /* UNICODE string pointer */
        caplen = wcslen(lpMsgBoxParams->lpszCaption);
    }
    if (!lpMsgBoxParams->lpszCaption)
    {
        /* No caption, use blank */
        lpMsgBoxParams->lpszCaption = L"";
        caplen = 0;
    }

    /* Load the text */
    text = NULL;
    if (lpMsgBoxParams->lpszText && IS_INTRESOURCE(lpMsgBoxParams->lpszText))
    {
        /* User-defined resource string */
        textlen = LoadAllocStringW(lpMsgBoxParams->hInstance,
                                   PtrToUlong(lpMsgBoxParams->lpszText),
                                   &text,
                                   NULL);
        lpMsgBoxParams->lpszText = text;
    }
    else if (lpMsgBoxParams->lpszText)
    {
        /* UNICODE string pointer */
        textlen = wcslen(lpMsgBoxParams->lpszText);
    }
    if (!lpMsgBoxParams->lpszText)
    {
        /* No text, use blank */
        lpMsgBoxParams->lpszText = L"";
        textlen = 0;
    }

    /* Load the icon */
    switch (lpMsgBoxParams->dwStyle & MB_ICONMASK)
    {
        case MB_ICONEXCLAMATION: // case MB_ICONWARNING:
            hIcon = LoadIconW(NULL, IDI_EXCLAMATIONW);
            MessageBeep(MB_ICONEXCLAMATION);
            break;
        case MB_ICONQUESTION:
            hIcon = LoadIconW(NULL, IDI_QUESTIONW);
            MessageBeep(MB_ICONQUESTION);
            break;
        case MB_ICONASTERISK: // case MB_ICONINFORMATION:
            hIcon = LoadIconW(NULL, IDI_ASTERISKW);
            MessageBeep(MB_ICONASTERISK);
            break;
        case MB_ICONHAND: // case MB_ICONSTOP: case MB_ICONERROR:
            hIcon = LoadIconW(NULL, IDI_HANDW);
            MessageBeep(MB_ICONHAND);
            break;
        case MB_USERICON:
            hIcon = LoadIconW(lpMsgBoxParams->hInstance, lpMsgBoxParams->lpszIcon);
            MessageBeep(MB_OK);
            break;
        default:
            /*
             * By default, Windows 95/98/NT does not associate an icon
             * to message boxes. So ReactOS should do the same.
             */
            hIcon = NULL;
            MessageBeep(MB_OK);
            break;
    }
    /* Reuse the internal pointer! */
    lpMsgBoxParams->lpszIcon = (LPCWSTR)hIcon;

    /* Basic space */
    bufsize = sizeof(DLGTEMPLATE) +
              2 * sizeof(WORD) +                /* menu and class */
              (caplen + 1) * sizeof(WCHAR) +    /* title */
              sizeof(WORD);                     /* font height */

    /* Space for the icon */
    if (hIcon)
    {
        bufsize = ALIGN_UP(bufsize, DWORD);
        bufsize += sizeof(DLGITEMTEMPLATE) +
                   4 * sizeof(WORD) +
                   sizeof(WCHAR);
    }

    /* Space for the text */
    bufsize = ALIGN_UP(bufsize, DWORD);
    bufsize += sizeof(DLGITEMTEMPLATE) +
               3 * sizeof(WORD) +
               (textlen + 1) * sizeof(WCHAR);

    /* Space for the buttons */
    for (i = 0; i < mbd.dwButtons; i++)
    {
        if (!mbd.ppszButtonText[i] || !*mbd.ppszButtonText[i])
        {
            /* No text, use blank */
            mbd.ppszButtonText[i] = L"";
            ButtonLen = 0;
        }
        else
        {
            /* UNICODE string pointer */
            ButtonLen = wcslen(mbd.ppszButtonText[i]);
        }

        bufsize = ALIGN_UP(bufsize, DWORD);
        bufsize += sizeof(DLGITEMTEMPLATE) +
                   3 * sizeof(WORD) +
                   (ButtonLen + 1) * sizeof(WCHAR);
    }

    /* Allocate the dialog template */
    buf = RtlAllocateHeap(RtlGetProcessHeap(), 0, bufsize);
    if (!buf)
        goto Quit;

    iico = itxt = NULL;


    nclm.cbSize = sizeof(nclm);
    SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, sizeof(nclm), &nclm, 0);
    hFont = CreateFontIndirectW(&nclm.lfMessageFont);
    if (!hFont)
    {
        ERR("Cannot retrieve nclm.lfMessageFont! (error %lu)\n", GetLastError());
        goto Quit;
    }

    hDCWnd = NULL;
    hDC = GetDCEx(hDCWnd, NULL, DCX_WINDOW | DCX_CACHE);
    if (!hDC)
    {
        /* Retry with the DC of the owner window */
        hDCWnd = lpMsgBoxParams->hwndOwner;
        hDC = GetDCEx(hDCWnd, NULL, DCX_WINDOW | DCX_CACHE);
    }
    if (!hDC)
    {
        ERR("GetDCEx() failed, bail out! (error %lu)\n", GetLastError());
        goto Quit;
    }
    hOldFont = SelectObject(hDC, hFont);

    units.cx = GdiGetCharDimensions(hDC, NULL, &units.cy);
    if (!units.cx)
    {
        DWORD defUnits;
        ERR("GdiGetCharDimensions() failed, falling back to default values (error %lu)\n", GetLastError());
        defUnits = GetDialogBaseUnits();
        units.cx = LOWORD(defUnits);
        units.cy = HIWORD(defUnits);
    }

    /* Calculate the caption rectangle */
    txtrect.right = MulDiv(GetSystemMetrics(SM_CXSCREEN), 4, 5);
    if (hIcon)
        txtrect.right -= GetSystemMetrics(SM_CXICON) + MSGBOXEX_SPACING;
    txtrect.top = txtrect.left = txtrect.bottom = 0;
    if (textlen != 0)
    {
        DrawTextW(hDC, lpMsgBoxParams->lpszText, textlen, &txtrect,
                  DT_LEFT | DT_NOPREFIX | DT_WORDBREAK | DT_EXPANDTABS | DT_EXTERNALLEADING | DT_EDITCONTROL | DT_CALCRECT);
    }
    else
    {
        txtrect.right = txtrect.left + 1;
        txtrect.bottom = txtrect.top + 1;
    }
    txtrect.right++;

    /* Calculate the maximum buttons size */
    btnsize.cx = BTN_CX;
    btnsize.cy = BTN_CY;
    btnrect.left = btnrect.top = 0;
    for (i = 0; i < mbd.dwButtons; i++)
    {
        // btnrect.right = btnrect.bottom = 0; // FIXME: Is it needed??
        DrawTextW(hDC, mbd.ppszButtonText[i], wcslen(mbd.ppszButtonText[i]),
                  &btnrect, DT_LEFT | DT_SINGLELINE | DT_CALCRECT);
        btnsize.cx = max(btnsize.cx, btnrect.right);
        btnsize.cy = max(btnsize.cy, btnrect.bottom);
    }

    if (hOldFont)
        SelectObject(hDC, hOldFont);

    ReleaseDC(hDCWnd, hDC);

    if (hFont)
        DeleteObject(hFont);


    /* Calculate position and size of controls */


    /* Calculate position and size of the icon */
    rc.left = rc.bottom = rc.right = 0;
    btntop = 0;
    if (hIcon)
    {
        rc.right = GetSystemMetrics(SM_CXICON);
        rc.bottom = GetSystemMetrics(SM_CYICON);
#ifdef MSGBOX_ICONVCENTER
        rc.top = MSGBOXEX_MARGIN + ((max(txtrect.bottom, rc.bottom) - rc.bottom) / 2);
        rc.top = max(MSGBOXEX_SPACING, rc.top);
#else
        rc.top = MSGBOXEX_MARGIN;
#endif
        btnleft = (mbd.dwButtons * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
        if (btnleft > txtrect.right + rc.right + MSGBOXEX_SPACING)
        {
#ifdef MSGBOX_TEXTHCENTER
            rc.left = MSGBOXEX_MARGIN + ((btnleft - txtrect.right - rc.right - MSGBOXEX_SPACING) / 2);
#else
            rc.left = MSGBOXEX_MARGIN;
#endif
            btnleft = MSGBOXEX_MARGIN;
        }
        else
        {
            rc.left = MSGBOXEX_MARGIN;
            btnleft = MSGBOXEX_MARGIN + ((txtrect.right + rc.right + MSGBOXEX_SPACING - btnleft) / 2);
        }

        iconPos.x = RESCALE_X(rc.left, units);
        iconPos.y = RESCALE_Y(rc.top, units);
        iconSize.cx = RESCALE_X(rc.right, units);
        iconSize.cy = RESCALE_Y(rc.bottom, units);

        btntop = rc.top + rc.bottom + MSGBOXEX_SPACING;
        rc.left += rc.right + MSGBOXEX_SPACING;
    }
    else
    {
        btnleft = (mbd.dwButtons * (btnsize.cx + MSGBOXEX_BUTTONSPACING)) - MSGBOXEX_BUTTONSPACING;
        if (btnleft > txtrect.right)
        {
#ifdef MSGBOX_TEXTHCENTER
            rc.left = MSGBOXEX_MARGIN + ((btnleft - txtrect.right) / 2);
#else
            rc.left = MSGBOXEX_MARGIN;
#endif
            btnleft = MSGBOXEX_MARGIN;
        }
        else
        {
            rc.left = MSGBOXEX_MARGIN;
            btnleft = MSGBOXEX_MARGIN + ((txtrect.right - btnleft) / 2);
        }
    }


    /* Initialize the dialog template */
    tpl = (DLGTEMPLATE *)buf;

    tpl->style = WS_CAPTION | WS_POPUP | WS_VISIBLE | WS_CLIPSIBLINGS | WS_SYSMENU |
                 DS_CENTER | DS_SETFONT | DS_MODALFRAME | DS_NOIDLEMSG;
    tpl->dwExtendedStyle = WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE | WS_EX_CONTROLPARENT;
    if (lpMsgBoxParams->dwStyle & MB_TOPMOST)
        tpl->dwExtendedStyle |= WS_EX_TOPMOST;
    if (lpMsgBoxParams->dwStyle & MB_RIGHT)
        tpl->dwExtendedStyle |= WS_EX_RIGHT;
    tpl->x = 100;
    tpl->y = 100;
    tpl->cdit = mbd.dwButtons + (hIcon ? 1 : 0) + 1; /* Buttons, icon and text */

    dest = (BYTE *)(tpl + 1);

    *(DWORD*)dest = 0;  /* no menu and use default window class */
    dest += 2 * sizeof(WORD);
    memcpy(dest, lpMsgBoxParams->lpszCaption, caplen * sizeof(WCHAR));
    dest += caplen * sizeof(WCHAR);
    *(WCHAR*)dest = L'\0';
    dest += sizeof(WCHAR);

    /*
     * A font point size (height) of 0x7FFF means that we use
     * the message box font (NONCLIENTMETRICSW.lfMessageFont).
     */
    *(WORD*)dest = 0x7FFF;
    dest += sizeof(WORD);

    /* Create the icon */
    if (hIcon)
    {
        dest = ALIGN_UP_POINTER(dest, DWORD);
        iico = (DLGITEMTEMPLATE *)dest;
        iico->style = WS_CHILD | WS_VISIBLE | SS_ICON;
        iico->dwExtendedStyle = 0;
        iico->id = MSGBOX_IDICON;

        iico->x = iconPos.x;
        iico->y = iconPos.y;
        iico->cx = iconSize.cx;
        iico->cy = iconSize.cy;

        dest += sizeof(DLGITEMTEMPLATE);
        *(WORD*)dest = 0xFFFF;
        dest += sizeof(WORD);
        *(WORD*)dest = 0x0082; /* static control */
        dest += sizeof(WORD);
        *(WORD*)dest = 0xFFFF;
        dest += sizeof(WORD);
        *(WCHAR*)dest = 0;
        dest += sizeof(WCHAR);
        *(WORD*)dest = 0;
        dest += sizeof(WORD);
    }

    /* Create static for text */
    dest = ALIGN_UP_POINTER(dest, DWORD);
    itxt = (DLGITEMTEMPLATE *)dest;
    itxt->style = WS_CHILD | WS_VISIBLE | SS_NOPREFIX;
    if (lpMsgBoxParams->dwStyle & MB_RIGHT)
        itxt->style |= SS_RIGHT;
    else
        itxt->style |= SS_LEFT;
    itxt->dwExtendedStyle = 0;
    itxt->id = MSGBOX_IDTEXT;
    dest += sizeof(DLGITEMTEMPLATE);
    *(WORD*)dest = 0xFFFF;
    dest += sizeof(WORD);
    *(WORD*)dest = 0x0082; /* static control */
    dest += sizeof(WORD);
    memcpy(dest, lpMsgBoxParams->lpszText, textlen * sizeof(WCHAR));
    dest += textlen * sizeof(WCHAR);
    *(WCHAR*)dest = 0;
    dest += sizeof(WCHAR);
    *(WORD*)dest = 0;
    dest += sizeof(WORD);


    /* Calculate position of the text */
    rc.top = MSGBOXEX_MARGIN + ((rc.bottom - txtrect.bottom) / 2);
    rc.top = max(rc.top, MSGBOXEX_MARGIN);


    /* Make the first button the default button if none other is */
    if (mbd.uDefButton >= mbd.dwButtons)
        mbd.uDefButton = 0;

    /* Create and calculate the position of the buttons */
    btntop = max(rc.top + txtrect.bottom + MSGBOXEX_SPACING, btntop);
    for (i = 0; i < mbd.dwButtons; i++)
    {
        ButtonLen = wcslen(mbd.ppszButtonText[i]);

        dest = ALIGN_UP_POINTER(dest, DWORD);
        ibtn = (DLGITEMTEMPLATE *)dest;

        ibtn->style = WS_CHILD | WS_VISIBLE | WS_TABSTOP;
        if (i == mbd.uDefButton)
            ibtn->style |= BS_DEFPUSHBUTTON;
        else
            ibtn->style |= BS_PUSHBUTTON;

        ibtn->dwExtendedStyle = 0;
        ibtn->id = mbd.pidButton[i];
        dest += sizeof(DLGITEMTEMPLATE);
        *(WORD*)dest = 0xFFFF;
        dest += sizeof(WORD);
        *(WORD*)dest = 0x0080; /* button control */
        dest += sizeof(WORD);
        memcpy(dest, mbd.ppszButtonText[i], ButtonLen * sizeof(WCHAR));
        dest += ButtonLen * sizeof(WCHAR);
        *(WORD*)dest = 0;
        dest += sizeof(WORD);
        *(WORD*)dest = 0;
        dest += sizeof(WORD);

        ibtn->x = RESCALE_X(btnleft, units);
        ibtn->y = RESCALE_Y(btntop, units);
        ibtn->cx = RESCALE_X(btnsize.cx, units);
        ibtn->cy = RESCALE_Y(btnsize.cy, units);
        btnleft += btnsize.cx + MSGBOXEX_BUTTONSPACING;
    }

    /* Calculate the size and position of the message-box window */
    btnleft = max(btnleft - MSGBOXEX_BUTTONSPACING, rc.left + txtrect.right);
    btnleft += MSGBOXEX_MARGIN;
    if (mbd.dwButtons > 0)
        btntop += btnsize.cy + MSGBOXEX_MARGIN;

    /* Set the size and position of the static message */
    itxt->x = RESCALE_X(rc.left, units);
    itxt->y = RESCALE_Y(rc.top, units);
    itxt->cx = RESCALE_X(btnleft - rc.left - MSGBOXEX_MARGIN, units);
    itxt->cy = RESCALE_Y(txtrect.bottom, units);

    /* Set the size of the window */
    tpl->cx = RESCALE_X(btnleft, units);
    tpl->cy = RESCALE_Y(btntop, units);

    /* Finally show the message-box */
    ERR("MessageBox: %s\n", wine_dbgstr_wn(lpMsgBoxParams->lpszText, textlen));
    ret = DialogBoxIndirectParamW(lpMsgBoxParams->hInstance, tpl,
                                  lpMsgBoxParams->hwndOwner,
                                  MessageBoxProc, (LPARAM)&mbd);

Quit:
    if (buf)
        RtlFreeHeap(RtlGetProcessHeap(), 0, buf);

    if (text)
        RtlFreeHeap(RtlGetProcessHeap(), 0, text);

    if (caption)
        RtlFreeHeap(RtlGetProcessHeap(), 0, caption);

    return ret;
}


/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
int
WINAPI
MessageBoxA(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType)
{
    return MessageBoxExA(hWnd, lpText, lpCaption, uType, LANG_NEUTRAL);
}

/*
 * @implemented
 */
int
WINAPI
MessageBoxW(
    IN HWND hWnd,
    IN LPCWSTR lpText,
    IN LPCWSTR lpCaption,
    IN UINT uType)
{
    return MessageBoxExW(hWnd, lpText, lpCaption, uType, LANG_NEUTRAL);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxExA(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId)
{
    MSGBOXPARAMSA msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxIndirectA(&msgbox);
}

/*
 * @implemented
 */
int
WINAPI
MessageBoxExW(
    IN HWND hWnd,
    IN LPCWSTR lpText,
    IN LPCWSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId)
{
    MSGBOXPARAMSW msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxTimeoutIndirectW(&msgbox, (UINT)-1);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxIndirectA(
    IN CONST MSGBOXPARAMSA* lpMsgBoxParams)
{
    MSGBOXPARAMSW msgboxW;
    UNICODE_STRING textW, captionW, iconW;
    int ret;

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszText))
    {
        RtlCreateUnicodeStringFromAsciiz(&textW, (PCSZ)lpMsgBoxParams->lpszText);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        textW.Buffer[textW.Length / sizeof(WCHAR)] = L'\0';
    }
    else
        textW.Buffer = (LPWSTR)lpMsgBoxParams->lpszText;

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszCaption))
    {
        RtlCreateUnicodeStringFromAsciiz(&captionW, (PCSZ)lpMsgBoxParams->lpszCaption);
        /*
         * UNICODE_STRING objects are always allocated with an extra byte so you
         * can null-term if you want
         */
        captionW.Buffer[captionW.Length / sizeof(WCHAR)] = L'\0';
    }
    else
        captionW.Buffer = (LPWSTR)lpMsgBoxParams->lpszCaption;

    if (lpMsgBoxParams->dwStyle & MB_USERICON)
    {
        if (!IS_INTRESOURCE(lpMsgBoxParams->lpszIcon))
        {
            RtlCreateUnicodeStringFromAsciiz(&iconW, (PCSZ)lpMsgBoxParams->lpszIcon);
            /*
             * UNICODE_STRING objects are always allocated with an extra byte so you
             * can null-term if you want
             */
            iconW.Buffer[iconW.Length / sizeof(WCHAR)] = L'\0';
        }
        else
            iconW.Buffer = (LPWSTR)lpMsgBoxParams->lpszIcon;
    }
    else
        iconW.Buffer = NULL;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = lpMsgBoxParams->hwndOwner;
    msgboxW.hInstance = lpMsgBoxParams->hInstance;
    msgboxW.lpszText = textW.Buffer;
    msgboxW.lpszCaption = captionW.Buffer;
    msgboxW.dwStyle = lpMsgBoxParams->dwStyle;
    msgboxW.lpszIcon = iconW.Buffer;
    msgboxW.dwContextHelpId = lpMsgBoxParams->dwContextHelpId;
    msgboxW.lpfnMsgBoxCallback = lpMsgBoxParams->lpfnMsgBoxCallback;
    msgboxW.dwLanguageId = lpMsgBoxParams->dwLanguageId;

    ret = MessageBoxTimeoutIndirectW(&msgboxW, (UINT)-1);

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszText))
        RtlFreeUnicodeString(&textW);

    if (!IS_INTRESOURCE(lpMsgBoxParams->lpszCaption))
        RtlFreeUnicodeString(&captionW);

    if ((lpMsgBoxParams->dwStyle & MB_USERICON) && !IS_INTRESOURCE(iconW.Buffer))
        RtlFreeUnicodeString(&iconW);

    return ret;
}

/*
 * @implemented
 */
int
WINAPI
MessageBoxIndirectW(
    IN CONST MSGBOXPARAMSW* lpMsgBoxParams)
{
    return MessageBoxTimeoutIndirectW(lpMsgBoxParams, (UINT)-1);
}


/*
 * @implemented
 */
int
WINAPI
MessageBoxTimeoutA(
    IN HWND hWnd,
    IN LPCSTR lpText,
    IN LPCSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout)
{
    MSGBOXPARAMSW msgboxW;
    UNICODE_STRING textW, captionW;
    int ret;

    if (!IS_INTRESOURCE(lpText))
        RtlCreateUnicodeStringFromAsciiz(&textW, (PCSZ)lpText);
    else
        textW.Buffer = (LPWSTR)lpText;

    if (!IS_INTRESOURCE(lpCaption))
        RtlCreateUnicodeStringFromAsciiz(&captionW, (PCSZ)lpCaption);
    else
        captionW.Buffer = (LPWSTR)lpCaption;

    msgboxW.cbSize = sizeof(msgboxW);
    msgboxW.hwndOwner = hWnd;
    msgboxW.hInstance = 0;
    msgboxW.lpszText = textW.Buffer;
    msgboxW.lpszCaption = captionW.Buffer;
    msgboxW.dwStyle = uType;
    msgboxW.lpszIcon = NULL;
    msgboxW.dwContextHelpId = 0;
    msgboxW.lpfnMsgBoxCallback = NULL;
    msgboxW.dwLanguageId = wLanguageId;

    ret = MessageBoxTimeoutIndirectW(&msgboxW, (UINT)dwTimeout);

    if (!IS_INTRESOURCE(textW.Buffer))
        RtlFreeUnicodeString(&textW);

    if (!IS_INTRESOURCE(captionW.Buffer))
        RtlFreeUnicodeString(&captionW);

    return ret;
}

/*
 * @implemented
 */
int
WINAPI
MessageBoxTimeoutW(
    IN HWND hWnd,
    IN LPCWSTR lpText,
    IN LPCWSTR lpCaption,
    IN UINT uType,
    IN WORD wLanguageId,
    IN DWORD dwTimeout)
{
    MSGBOXPARAMSW msgbox;

    msgbox.cbSize = sizeof(msgbox);
    msgbox.hwndOwner = hWnd;
    msgbox.hInstance = 0;
    msgbox.lpszText = lpText;
    msgbox.lpszCaption = lpCaption;
    msgbox.dwStyle = uType;
    msgbox.lpszIcon = NULL;
    msgbox.dwContextHelpId = 0;
    msgbox.lpfnMsgBoxCallback = NULL;
    msgbox.dwLanguageId = wLanguageId;

    return MessageBoxTimeoutIndirectW(&msgbox, (UINT)dwTimeout);
}


/*
 * @implemented
 */
BOOL
WINAPI
MessageBeep(
    IN UINT uType)
{
    return NtUserxMessageBeep(uType);
}


/*
 * @implemented
 *
 * See: https://learn.microsoft.com/en-us/windows/win32/dlgbox/mb-getstring
 * and: http://undoc.airesoft.co.uk/user32.dll/MB_GetString.php
 * for more information.
 */
LPCWSTR
WINAPI
MB_GetString(
    IN UINT wBtn)
{
    static BOOL bCached = FALSE;
    static MBSTRING MBStrings[MAX_MB_STRINGS]; // FIXME: Use gpsi->MBStrings when this is loaded by Win32k!

    //
    // FIXME - TODO: The gpsi->MBStrings[] array should be loaded by win32k!
    //
    ASSERT(IDCONTINUE <= MAX_MB_STRINGS);
    if (!bCached)
    {
        UINT i;
        for (i = 0; i < MAX_MB_STRINGS; ++i)
        {
            /*gpsi->*/MBStrings[i].uID  = IDOK + i;
            /*gpsi->*/MBStrings[i].uStr = IDS_OK + i; // See user32/include/resource.h
            LoadStringW(User32Instance,
                        /*gpsi->*/MBStrings[i].uStr,
                        /*gpsi->*/MBStrings[i].szName,
                        ARRAYSIZE(/*gpsi->*/MBStrings[i].szName));
        }
        bCached = TRUE;
    }

    /*
     * The allowable IDs are between "IDOK - 1" (0) and "IDCONTINUE - 1" (10) inclusive.
     * See psdk/winuser.h and user32/include/resource.h .
     */
    if (wBtn > IDCONTINUE - 1)
        return NULL;

    return /*gpsi->*/MBStrings[wBtn].szName;
}

/* EOF */
