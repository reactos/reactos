/***************************** Module Header ******************************\
* Module Name: ntcftxt.h
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Kernel call forward stubs with text arguments
*
* Each function will be created with two flavors Ansi and Unicode
*
* 06-Jan-1992 IanJa      Moved from cf.h
* 18-Mar-1995 JimA       Ported from cftxt.h
\**************************************************************************/

#ifdef UNICODE
#define IS_ANSI FALSE
#ifndef _UNICODE
#define _UNICODE
#endif
#else
#define IS_ANSI TRUE
#undef _UNICODE
#endif
#include <tchar.h>
#include "ntsend.h"

HWND TEXT_FN(InternalFindWindowEx)(
    HWND    hwndParent,
    HWND    hwndChild,
    LPCTSTR pClassName,
    LPCTSTR pWindowName,
    DWORD   dwFlag)
{
    IN_STRING strClass;
    IN_STRING strWindow;

    /*
     * Make sure cleanup will work successfully
     */
    strClass.fAllocated = FALSE;
    strWindow.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPTSTRIDOPT(&strClass, pClassName);
        COPYLPTSTROPT(&strWindow, pWindowName);

        retval = (ULONG_PTR)NtUserFindWindowEx(
                hwndParent,
                hwndChild,
                strClass.pstr,
                strWindow.pstr,
                dwFlag);

    ERRORTRAP(0);
    CLEANUPLPTSTR(strClass);
    CLEANUPLPTSTR(strWindow);
    ENDCALL(HWND);

}

HWND FindWindowEx(
    HWND    hwndParent,
    HWND    hwndChild,
    LPCTSTR pClassName,
    LPCTSTR pWindowName)
{
    return TEXT_FN(InternalFindWindowEx)(hwndParent, hwndChild, pClassName, pWindowName, FW_BOTH);
}

HWND FindWindow(
    LPCTSTR pClassName,
    LPCTSTR pWindowName)
{
    return TEXT_FN(InternalFindWindowEx)(NULL, NULL, pClassName, pWindowName, FW_BOTH);
}

extern WNDPROC mpPfnAddress[];

BOOL GetClassInfo(
    HINSTANCE hmod OPTIONAL,
    LPCTSTR pszClassName,
    LPWNDCLASS pwc)
{
    IN_STRING strClassName;
    LPWSTR pszMenuName;
    WNDCLASSEXW WndClass;

    /*
     * Make sure cleanup will work successfully
     */
    strClassName.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPTSTRID(&strClassName, pszClassName);

        retval = (DWORD)NtUserGetClassInfo(
                hmod,
                strClassName.pstr,
                &WndClass,
                &pszMenuName,
                IS_ANSI);

        if (retval) {

            /*
             * Move the info from the WNDCLASSEX to the WNDCLASS structure
             * On 64-bit plaforms we'll have 32 bits of padding between style and
             * lpfnWndProc in WNDCLASS, so start the copy from the first 64-bit
             * aligned field and hand copy the rest.
             */
            RtlCopyMemory(&(pwc->lpfnWndProc), &(WndClass.lpfnWndProc), sizeof(WNDCLASS) - FIELD_OFFSET(WNDCLASS, lpfnWndProc));
            pwc->style = WndClass.style;

            /*
             * Update these pointers so they point to something real.
             * pszMenuName is actually just the pointer the app originally
             * passed to us.
             */
            pwc->lpszMenuName = (LPTSTR)pszMenuName;
            pwc->lpszClassName = pszClassName;
        }

    ERRORTRAP(0);
    CLEANUPLPTSTR(strClassName);
    ENDCALL(BOOL);
}

BOOL GetClassInfoEx(
    HINSTANCE hmod OPTIONAL,
    LPCTSTR pszClassName,
    LPWNDCLASSEX pwc)
{
    IN_STRING strClassName;
    LPWSTR pszMenuName;

    /*
     * Make sure cleanup will work successfully
     */
    strClassName.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPTSTRID(&strClassName, pszClassName);

        retval = (DWORD)NtUserGetClassInfo(
                hmod,
                strClassName.pstr,
                (LPWNDCLASSEXW)pwc,
                &pszMenuName,
                IS_ANSI);

        if (retval) {

            /*
             * Update these pointers so they point to something real.
             * pszMenuName is actually just the pointer the app originally
             * passed to us.
             */
            pwc->lpszMenuName = (LPTSTR)pszMenuName;
            pwc->lpszClassName = pszClassName;
        }

    ERRORTRAP(0);
    CLEANUPLPTSTR(strClassName);
    ENDCALL(BOOL);
}

int GetClipboardFormatName(
    UINT wFormat,
    LPTSTR pFormatName,
    int chMaxCount)
{
    LPWSTR lpszReserve;

    BEGINCALL()

#ifdef UNICODE
        lpszReserve = pFormatName;
#else
        lpszReserve = LocalAlloc(LMEM_FIXED, chMaxCount * sizeof(WCHAR));
        if (!lpszReserve) {
            return 0;
        }
#endif

        retval = (DWORD)NtUserGetClipboardFormatName(
                wFormat,
                lpszReserve,
                chMaxCount);

#ifndef UNICODE
        if (retval) {
            /*
             * Do not copy out more than the requested byte count 'chMaxCount'.
             * Set retval to reflect the number of ANSI bytes.
             */
            retval = WCSToMB(lpszReserve, (UINT)retval,
                    &pFormatName, chMaxCount-1, FALSE);
            pFormatName[retval] = '\0';
        }
        LocalFree(lpszReserve);
#endif

    ERRORTRAP(0);
    ENDCALL(int);
}

int GetKeyNameText(
    LONG lParam,
    LPTSTR pString,
    int cchSize)
{
    LPWSTR lpszReserve;

    BEGINCALL()

#ifdef UNICODE
        lpszReserve = pString;
#else
        lpszReserve = LocalAlloc(LMEM_FIXED, cchSize * sizeof(WCHAR));
        if (!lpszReserve) {
            return 0;
        }
#endif

        retval = (DWORD)NtUserGetKeyNameText(
                lParam,
                lpszReserve,
                cchSize);

#ifndef UNICODE
        if (retval) {
            /*
             * Do not copy out more than the requested byte count 'nSize'.
             * Set retval to reflect the number of ANSI bytes.
             */
            retval = WCSToMB(lpszReserve, (UINT)retval,
                    &pString, cchSize-1, FALSE);
        }
        LocalFree(lpszReserve);
        ((LPSTR)pString)[retval] = '\0';
#endif

    ERRORTRAP(0);
    ENDCALL(int);
}

BOOL APIENTRY GetMessage(
    LPMSG pmsg,
    HWND hwnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax)
{
    BEGINCALL()

        /*
         * Prevent apps from setting hi 16 bits so we can use them internally.
         */
        if ((wMsgFilterMin | wMsgFilterMax) & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }

#ifndef UNICODE
        /*
         * If we have pushed message for DBCS messaging, we should pass this one
         * to Apps at first...
         */
        GET_DBCS_MESSAGE_IF_EXIST(GetMessage, pmsg, wMsgFilterMin, wMsgFilterMax, TRUE);
#endif

        retval = (DWORD)NtUserGetMessage(
                pmsg,
                hwnd,
                wMsgFilterMin,
                wMsgFilterMax);

#ifndef UNICODE
        // May have a bit more work to do if this MSG is for an ANSI app

        // !!! LATER if the unichar translates into multiple ANSI chars
        // !!! then what??? Divide into two messages??  WM_SYSDEADCHAR??

        if (RtlWCSMessageWParamCharToMB(pmsg->message, &(pmsg->wParam))) {
            WPARAM dwAnsi = pmsg->wParam;
            /*
             * Build DBCS-ware wParam. (for EM_SETPASSWORDCHAR...)
             */
            BUILD_DBCS_MESSAGE_TO_CLIENTA_FROM_SERVER(pmsg, dwAnsi, TRUE, TRUE);
        } else {
            retval = 0;
        }
ExitGetMessage:
#else
        /*
         * Only LOWORD of WPARAM is valid for WM_CHAR (Unicode)....
         * (Mask off DBCS messaging information.)
         */
        BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_SERVER(pmsg->message,pmsg->wParam);
#endif // UNICODE

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL GetKeyboardLayoutName(
    LPTSTR pwszKL)
{
#ifdef UNICODE
    UNICODE_STRING str;
    PUNICODE_STRING pstr = &str;
#else
    PUNICODE_STRING pstr = &NtCurrentTeb()->StaticUnicodeString;
#endif

    BEGINCALL()

#ifdef UNICODE
        str.MaximumLength = KL_NAMELENGTH * sizeof(WCHAR);
        str.Buffer = pwszKL;
#endif

        retval = (DWORD)NtUserGetKeyboardLayoutName(pstr);

#ifndef UNICODE
        if (retval) {
            /*
             * Non-zero retval means some text to copy out.  Do not copy out
             * more than the requested byte count 'chMaxCount'.
             */
            WCSToMB(pstr->Buffer, -1, &pwszKL, KL_NAMELENGTH, FALSE);
        }
#endif

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

UINT MapVirtualKey(
    UINT wCode,
    UINT wMapType)
{
    BEGINCALL()

        retval = (DWORD)NtUserMapVirtualKeyEx(
                wCode,
                wMapType,
                0,
                FALSE);

#ifndef UNICODE
        if ((wMapType == 2) && (retval != 0)) {
            WCHAR wch = LOWORD(retval);
            retval &= ~0xFFFF;
            RtlUnicodeToMultiByteN((LPSTR)&(retval), sizeof(CHAR),
                    NULL, &wch, sizeof(WCHAR));
        }
#endif

    ERRORTRAP(0);
    ENDCALL(UINT);
}

/**************************************************************************\
* MapVirtualKeyEx
*
* 21-Feb-1995 GregoryW    Created
\**************************************************************************/

#ifndef UNICODE
static HKL  hMVKCachedHKL = 0;
static UINT uMVKCachedCP  = 0;
#endif
UINT MapVirtualKeyEx(
    UINT wCode,
    UINT wMapType,
    HKL hkl)
{
    BEGINCALL()

        retval = (DWORD)NtUserMapVirtualKeyEx(
                wCode,
                wMapType,
                (ULONG_PTR)hkl,
                TRUE);

#ifndef UNICODE
        if ((wMapType == 2) && (retval != 0)) {
            WCHAR wch = LOWORD(retval);

            if (hkl != hMVKCachedHKL) {
                DWORD dwCodePage;
                if (!GetLocaleInfoW(
                         HandleToUlong(hkl) & 0xffff,
                         LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                         (LPWSTR)&dwCodePage,
                         sizeof(dwCodePage) / sizeof(WCHAR)
                         )) {
                    MSGERROR();
                }
                uMVKCachedCP = dwCodePage;
                hMVKCachedHKL = hkl;
            }
            /*
             * Clear low word which contains Unicode character returned from server.
             * This preserves the high word which is used to indicate dead key status.
             */
            retval = retval & 0xffff0000;
            if (!WideCharToMultiByte(
                     uMVKCachedCP,
                     0,
                     &wch,
                     1,
                     (LPSTR)&(retval),
                     1,
                     NULL,
                     NULL)) {
                MSGERROR();
            }
        }
#endif

    ERRORTRAP(0);
    ENDCALL(UINT);
}

BOOL APIENTRY PostMessage(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    BEGINCALL()

        switch (wMsg) {
        case WM_DROPFILES:
            if (GetWindowProcess(hwnd) != GETPROCESSID()) {
                /*
                 * We first send a WM_COPYGLOBALDATA message to get the data into the proper
                 * context.
                 */
                HGLOBAL hg;

                hg = (HGLOBAL)SendMessage(hwnd, WM_COPYGLOBALDATA,
                        GlobalSize((HGLOBAL)wParam), wParam);
                if (!hg) {
                    MSGERROR();
                }
                wParam = (WPARAM)hg;
            }
            break;

        case LB_DIR:
        case CB_DIR:
            /*
             * Make sure this bit is set so the client side string gets
             * successfully copied over.
             */
            wParam |= DDL_POSTMSGS;
            break;
        }

#ifndef UNICODE
        /*
         * Setup DBCS Messaging for WM_CHAR...
         */
        BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(wMsg,wParam,TRUE);

        RtlMBMessageWParamCharToWCS(wMsg, &wParam);
#endif
        retval = (DWORD)NtUserPostMessage(
                hwnd,
                wMsg,
                wParam,
                lParam);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL APIENTRY PostThreadMessage(
    DWORD idThread,
    UINT msg,
    WPARAM wParam,
    LPARAM lParam)
{
    BEGINCALL()

#ifndef UNICODE
#ifdef FE_SB // PostThreadMessage()
        /*
         * The server always expects the characters to be unicode so
         * if this was generated from an ANSI routine convert it to Unicode
         */
        BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(msg,wParam,TRUE);
#endif // FE_SB

        RtlMBMessageWParamCharToWCS(msg, &wParam);
#endif

        retval = (DWORD)NtUserPostThreadMessage(
                idThread,
                msg,
                wParam,
                lParam);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}


/**************************************************************************\
* StringDuplicate
*
* 03-25-96 GerardoB         Added Header.
\**************************************************************************/
#define StringDuplicate TEXT_FN(StringDuplicate)
LPTSTR StringDuplicate(LPCTSTR ptszDup) {
    LPTSTR ptsz;
    ULONG cb;

    cb = (_tcslen(ptszDup) + 1) * sizeof(TCHAR);
    ptsz = (LPTSTR)LocalAlloc(NONZEROLPTR, cb);
    if (ptsz != NULL) {
        RtlCopyMemory(ptsz, ptszDup, cb);
    }
    return ptsz;
}
/**************************************************************************\
* InitClsMenuName
*
* 03-22-96 GerardoB         Created.
\**************************************************************************/
#define InitClsMenuName TEXT_FN(InitClsMenuName)
BOOL InitClsMenuName (PCLSMENUNAME pcmn, LPCTSTR lpszMenuName, PIN_STRING pstrMenuName)
{
    /*
     * We check the high-word because this may be a resource-ID.
     */
    if (IS_PTR(lpszMenuName)) {
#ifdef UNICODE
        if ((pcmn->pwszClientUnicodeMenuName = StringDuplicate(lpszMenuName)) == NULL) {
            return FALSE;
        }

        if (!WCSToMB(lpszMenuName, -1, &(pcmn->pszClientAnsiMenuName), -1, TRUE)) {
            pcmn->pszClientAnsiMenuName = NULL;
        }
#else
        if ((pcmn->pszClientAnsiMenuName = StringDuplicate(lpszMenuName)) == NULL) {
            return FALSE;
        }

        if (!MBToWCS(lpszMenuName, -1, &(pcmn->pwszClientUnicodeMenuName), -1, TRUE)) {
            pcmn->pwszClientUnicodeMenuName = NULL;
        }
#endif // UNICODE
    } else {
        /* Copy the ID */
        pcmn->pszClientAnsiMenuName = (LPSTR)lpszMenuName;
        pcmn->pwszClientUnicodeMenuName = (LPWSTR)lpszMenuName;
    }

    COPYLPTSTRID(pstrMenuName, lpszMenuName);
    pcmn->pusMenuName = pstrMenuName->pstr;

    return TRUE;

    goto errorexit; /* Keep the compiler happy */
errorexit: /* Used by COPYLPTSTRID */
   return FALSE;
}

/**************************************************************************\
* SetClassLong
*
* 03-22-96 GerardoB      Moved from client\cltxt.h & client\ntstubs.c
\**************************************************************************/
ULONG_PTR APIENTRY SetClassLongPtr(HWND hwnd, int nIndex, LONG_PTR dwNewLong)
{
    CLSMENUNAME cmn;
    IN_STRING strMenuName;

    switch (nIndex) {
        case GCLP_MENUNAME:
            if (!InitClsMenuName(&cmn, (LPCTSTR) dwNewLong, &strMenuName)) {
                RIPERR0(ERROR_INVALID_HANDLE, RIP_WARNING, "SetClassLong: InitClsMenuName failed");
                return 0;
            }
            dwNewLong = (ULONG_PTR) &cmn;
            break;

        case GCLP_HBRBACKGROUND:
            if ((DWORD)dwNewLong > COLOR_ENDCOLORS) {
                /*
                 * Let gdi validate the brush.  If it's invalid, then
                 * gdi will log a warning.  No need to rip twice so we'll
                 * just set the last-error.
                 */
                if (GdiValidateHandle((HBRUSH)dwNewLong) == FALSE) {
                    RIPERR0(ERROR_INVALID_HANDLE, RIP_VERBOSE, "");
                    return 0;
                }
            }
            break;
    }

    BEGINCALL()

    retval = (ULONG_PTR)NtUserSetClassLongPtr(hwnd, nIndex, dwNewLong, IS_ANSI);

    ERRORTRAP(0);

    /* Clean up */
    switch (nIndex) {
        case GCLP_MENUNAME:
            CLEANUPLPTSTR(strMenuName); /* Initialized by InitClsMenuName */
            /*
             * We free either the old strings (returned by the kernel),
             *  or the new ones if the kernel call failed
             */
            if (IS_PTR(cmn.pszClientAnsiMenuName)) {
                LocalFree(cmn.pszClientAnsiMenuName);
            }
            if (IS_PTR(cmn.pwszClientUnicodeMenuName)) {
                LocalFree(cmn.pwszClientUnicodeMenuName);
            }

            break;
    }

    ENDCALL(ULONG_PTR);

}

#ifdef _WIN64
DWORD  APIENTRY SetClassLong(HWND hwnd, int nIndex, LONG dwNewLong)
{
    BEGINCALL()

    retval = (DWORD)NtUserSetClassLong(hwnd, nIndex, dwNewLong, IS_ANSI);

    ERRORTRAP(0);
    ENDCALL(DWORD);
}
#endif
/**************************************************************************\
* RegisterClassExWOW
*
* 03-22-96 GerardoB      Added Header
\**************************************************************************/
ATOM TEXT_FN(RegisterClassExWOW)(
    WNDCLASSEX *lpWndClass,
    LPDWORD pdwWOWstuff,
    WORD fnid)
{
    WNDCLASSEX WndClass;
    IN_STRING strClassName;
    IN_STRING strMenuName;
    DWORD dwFlags, dwExpWinVer;
    CLSMENUNAME cmn;

    strClassName.fAllocated = 0;
    strMenuName.fAllocated  = 0;

    /*
     * Skip validation for our classes
     */
    if (fnid !=0) {
        /*
         * This is a hack to bypass validation for DDE classes
         * specifically, allow them to use hmodUser.
         */
         if (fnid == FNID_DDE_BIT) {
             fnid = 0;
         }
         dwExpWinVer = VER40;
    } else {
        if (lpWndClass->cbSize != sizeof(WNDCLASSEX)) {
            RIPMSG0(RIP_WARNING, "RegisterClass: Invalid cbSize");
        }

        if (lpWndClass->cbClsExtra < 0 ||
                lpWndClass->cbWndExtra < 0) {
            RIPMSG0(RIP_WARNING, "RegisterClass: invalid cb*Extra");
            goto BadParameter;
        }

        /*
         * Validate hInstance
         * Don't allow 4.0 apps to use hmodUser
         */
         if ((lpWndClass->hInstance == hmodUser)
                && (GetClientInfo()->dwExpWinVer >= VER40)) {
             RIPMSG0(RIP_WARNING, "RegisterClass: Cannot use USER's hInstance");
             goto BadParameter;
         } else if (lpWndClass->hInstance == NULL) {
            /*
             * For 32 bit apps we need to fix up the hInstance because Win 95 does
             * this in their thunk MapHInstLS
             */

            lpWndClass->hInstance = GetModuleHandle(NULL);
            RIPMSG1(RIP_VERBOSE, "RegisterClass: fixing up NULL hmodule to %#p",
                    lpWndClass->hInstance);
        }

        dwExpWinVer = GETEXPWINVER(lpWndClass->hInstance);


        /*
         * Check for valid style bits and strip if appropriate
         */
        if (lpWndClass->style & ~CS_VALID40) {

            if (dwExpWinVer > VER31) {
                RIPMSG0(RIP_WARNING, "RegisterClass: Invalid class style");
                goto BadParameter;
            }

            /*
             * Old application - strip bogus bits and pass through
             */
            RIPMSG0(RIP_WARNING, "RegisterClass: Invalid class style, stripping bad styles");
            lpWndClass->style &= CS_VALID40;
        }

        /*
         * Validate hbrBackground
         */
        if (lpWndClass->hbrBackground > (HBRUSH)COLOR_MAX
                && !GdiValidateHandle(lpWndClass->hbrBackground)) {

            RIPMSG1(RIP_WARNING, "RegisterClass: Invalid class brush:%#p", lpWndClass->hbrBackground);
            if (dwExpWinVer > VER30) {
                goto BadParameter;
            }

            lpWndClass->hbrBackground = NULL;
        }

    } /* if (fnid !=0) */


    if (!InitClsMenuName(&cmn, lpWndClass->lpszMenuName, &strMenuName)) {
        return FALSE;
    }

    BEGINCALL()

        WndClass = *lpWndClass;

#ifdef UNICODE
        dwFlags = 0;
#else
        dwFlags = CSF_ANSIPROC;
#endif // UNICODE

        if (dwExpWinVer > VER31) {
            dwFlags |= CSF_WIN40COMPAT;
        }

        COPYLPTSTRID(&strClassName, (LPTSTR)lpWndClass->lpszClassName);

        retval = NtUserRegisterClassExWOW(
                &WndClass,
                strClassName.pstr,
                &cmn,
                fnid,
                dwFlags,
                pdwWOWstuff);

        /*
         * Return the atom associated with this class or if earlier
         * than Win 3.1 convert it to a strict BOOL (some apps check)
         */
        if (GETEXPWINVER(lpWndClass->hInstance) < VER31)
            retval = !!retval;

    ERRORTRAP(0);
    CLEANUPLPTSTR(strMenuName);     /* Initialized by InitClsMenuName */
    CLEANUPLPTSTR(strClassName);

    if (!retval) {
        if (IS_PTR(cmn.pszClientAnsiMenuName)) {
            LocalFree(cmn.pszClientAnsiMenuName);
        }
        if (IS_PTR(cmn.pwszClientUnicodeMenuName)) {
            LocalFree(cmn.pwszClientUnicodeMenuName);
        }
    }
    ENDCALL(BOOL);

BadParameter:
    RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "RegisterClass: Invalid Parameter");
    return FALSE;

}

UINT RegisterWindowMessage(
    LPCTSTR pString)
{
    IN_STRING str;

    /*
     * Make sure cleanup will work successfully
     */
    str.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPTSTR(&str, (LPTSTR)pString);

        retval = (DWORD)NtUserRegisterWindowMessage(
                str.pstr);

    ERRORTRAP(0);
    CLEANUPLPTSTR(str);
    ENDCALL(UINT);
}

HANDLE RemoveProp(
    HWND hwnd,
    LPCTSTR pString)
{
    ATOM atomProp;
    DWORD dwProp;

    BEGINCALL()

        if (IS_PTR(pString)) {
            atomProp = GlobalFindAtom(pString);
            if (atomProp == 0)
                MSGERROR();
            dwProp = MAKELONG(atomProp, TRUE);
        } else
            dwProp = MAKELONG(PTR_TO_ID(pString), FALSE);

        retval = (ULONG_PTR)NtUserRemoveProp(
                hwnd,
                dwProp);

        if (retval != 0 && IS_PTR(pString))
            GlobalDeleteAtom(atomProp);

    ERRORTRAP(0);
    ENDCALL(HANDLE);
}

BOOL APIENTRY SendMessageCallback(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam,
    SENDASYNCPROC lpResultCallBack,
    ULONG_PTR dwData)
{
    BEGINCALL()

#ifndef UNICODE
#ifdef FE_SB // SendMessageCallBack()
        /*
         * Setup DBCS Messaging for WM_CHAR...
         */
        BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(wMsg,wParam,TRUE);
#endif // FE_SB

        RtlMBMessageWParamCharToWCS(wMsg, &wParam);
#endif

        retval = (DWORD)NtUserSendMessageCallback(
                hwnd,
                wMsg,
                wParam,
                lParam,
                lpResultCallBack,
                dwData);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL APIENTRY SendNotifyMessage(
    HWND hwnd,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
{
    LARGE_STRING str;
    LPWSTR  lpUniName = NULL;


    BEGINCALL()

#ifndef UNICODE
#ifdef FE_SB // SendNotifyMessage()
        /*
         * Setup DBCS Messaging for WM_CHAR...
         */
        BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(wMsg,wParam,TRUE);
#endif // FE_SB

        RtlMBMessageWParamCharToWCS(wMsg, &wParam);
#endif

        /*
         * Allow system notification messages containing
         * strings to go through.
         */
        if ((wMsg == WM_WININICHANGE || wMsg == WM_DEVMODECHANGE) && lParam) {
#ifdef UNICODE
            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&str,
                    (LPWSTR)lParam, (UINT)-1);
#else
            if (!MBToWCS((LPSTR)lParam, -1, &lpUniName, -1, TRUE))
                return FALSE;

            RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&str,
                        lpUniName, (UINT)-1);
#endif
            lParam = (LPARAM)(&str);
        }

        retval = (DWORD)NtUserSendNotifyMessage(
                hwnd,
                wMsg,
                wParam,
                lParam);

    if (lpUniName)
        UserLocalFree(lpUniName);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL SetProp(
    HWND hwnd,
    LPCTSTR pString,
    HANDLE hData)
{
    ATOM atomProp;
    DWORD dwProp;

    BEGINCALL()

        if (IS_PTR(pString)) {
            atomProp = GlobalAddAtom(pString);
            if (atomProp == 0)
                MSGERROR();
            dwProp = MAKELONG(atomProp, TRUE);
        } else
            dwProp = MAKELONG(PTR_TO_ID(pString), FALSE);

        retval = (DWORD)NtUserSetProp(
                hwnd,
                dwProp,
                hData);

        /*
         * If it failed, get rid of the atom
         */
        if (retval == FALSE && IS_PTR(pString))
            GlobalDeleteAtom(atomProp);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL UnregisterClass(
    LPCTSTR pszClassName,
    HINSTANCE hModule)
{
    IN_STRING strClassName;
    CLSMENUNAME cmn;

    /*
     * Make sure cleanup will work successfully
     */
    strClassName.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPTSTRID(&strClassName, pszClassName);

        retval = (DWORD)NtUserUnregisterClass(
                strClassName.pstr,
                hModule,
                &cmn);


        /*
         * Check explicity for TRUE so we don't get a !FALSE when
         * converttogui fails and the NtUser returns a status code intead of bool.
         */
        if (retval == TRUE) {
            /*
             * Free the menu strings if they are not resource IDs
             */
            if (IS_PTR(cmn.pszClientAnsiMenuName)) {
                LocalFree(cmn.pszClientAnsiMenuName);
            }
            if (IS_PTR(cmn.pwszClientUnicodeMenuName)) {
                LocalFree(cmn.pwszClientUnicodeMenuName);
            }
        }

    ERRORTRAP(0);
    CLEANUPLPTSTR(strClassName);
    ENDCALL(BOOL);
}

SHORT VkKeyScan(
    TCHAR cChar)
{
    WCHAR wChar;

    BEGINCALL()

#ifdef UNICODE
        wChar = cChar;
#else
#ifdef FE_SB // VkKeyScan()
        /*
         * Return 0xFFFFFFFF for DBCS LeadByte character.
         */
        if (IsDBCSLeadByte(cChar)) {
            MSGERROR();
        }
#endif // FE_SB

        RtlMultiByteToUnicodeN((LPWSTR)&(wChar), sizeof(WCHAR), NULL, &cChar, sizeof(CHAR));
#endif // UNICODE

        retval = (DWORD)NtUserVkKeyScanEx(
                wChar,
                0,
                FALSE);

    ERRORTRAP(-1);
    ENDCALL(SHORT);
}

#ifndef UNICODE
static HKL  hVKSCachedHKL = 0;
static UINT uVKSCachedCP  = 0;
#endif
SHORT VkKeyScanEx(
    TCHAR cChar,
    HKL hkl)
{
    WCHAR wChar;
    BEGINCALL()

#ifdef UNICODE
        wChar = cChar;
#else
        if (hkl != hVKSCachedHKL) {
            DWORD dwCodePage;
            if (!GetLocaleInfoW(
                     HandleToUlong(hkl) & 0xffff,
                     LOCALE_IDEFAULTANSICODEPAGE | LOCALE_RETURN_NUMBER,
                     (LPWSTR)&dwCodePage,
                     sizeof(dwCodePage) / sizeof(WCHAR)
                     )) {
                MSGERROR();
            }
            uVKSCachedCP = dwCodePage;
            hVKSCachedHKL = hkl;
        }

#ifdef FE_SB // VkKeyScanEx()
        /*
         * Return 0xFFFFFFFF for DBCS LeadByte character.
         */
        if (IsDBCSLeadByteEx(uVKSCachedCP,cChar)) {
            MSGERROR();
        }
#endif // FE_SB

        if (!MultiByteToWideChar(
                 uVKSCachedCP,
                 0,
                 &cChar,
                 sizeof(CHAR),
                 &wChar,
                 sizeof(WCHAR))) {
            MSGERROR();
        }
#endif // UNICODE

        retval = (DWORD)NtUserVkKeyScanEx(
                wChar,
                (ULONG_PTR)hkl,
                TRUE);

    ERRORTRAP(-1);
    ENDCALL(SHORT);
}

BOOL
EnumDisplayDevices(
    LPCTSTR lpszDevice,
    DWORD iDevNum,
    PDISPLAY_DEVICE lpDisplayDevice,
    DWORD dwFlags)
{
    UNICODE_STRING  UnicodeString;
    PUNICODE_STRING pUnicodeString = NULL;
    NTSTATUS Status;
    DISPLAY_DEVICEW tmpDisplayDevice;

    //
    // Clear out things to make sure the caller passes in appropriate
    // parameters
    //

    ZeroMemory(((PUCHAR)lpDisplayDevice) + sizeof(DWORD),
               lpDisplayDevice->cb - sizeof(DWORD));

    tmpDisplayDevice.cb = sizeof(DISPLAY_DEVICEW);

    if (lpszDevice) {

#ifdef UNICODE

        RtlInitUnicodeString(&UnicodeString, lpszDevice);

#else

        ANSI_STRING     AnsiString;

        UnicodeString = NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString, (LPSTR)lpszDevice);

        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                     &AnsiString,
                                                     FALSE))) {
            return FALSE;
        }

#endif

        pUnicodeString = &UnicodeString;
    }

    Status = NtUserEnumDisplayDevices(
                pUnicodeString,
                iDevNum,
                &tmpDisplayDevice,
                dwFlags);

    if (NT_SUCCESS(Status))
    {
#ifndef UNICODE
        LPSTR psz;

        if (lpDisplayDevice->cb >= FIELD_OFFSET(DISPLAY_DEVICE, DeviceString)) {
            psz = (LPSTR)&(lpDisplayDevice->DeviceName[0]);
            WCSToMB(&(tmpDisplayDevice.DeviceName[0]), -1, &psz, 32, FALSE);
        }

        if (lpDisplayDevice->cb >= FIELD_OFFSET(DISPLAY_DEVICE, StateFlags)) {
            psz = (LPSTR)&(lpDisplayDevice->DeviceString[0]);
            WCSToMB(&(tmpDisplayDevice.DeviceString[0]), -1, &psz, 128, FALSE);
        }

        if (lpDisplayDevice->cb >= FIELD_OFFSET(DISPLAY_DEVICE, DeviceID)) {
            lpDisplayDevice->StateFlags = tmpDisplayDevice.StateFlags;
        }

        if (lpDisplayDevice->cb >= FIELD_OFFSET(DISPLAY_DEVICE, DeviceKey)) {
            psz = (LPSTR)&(lpDisplayDevice->DeviceID[0]);
            WCSToMB(&(tmpDisplayDevice.DeviceID[0]), -1, &psz, 128, FALSE);
        }
        if (lpDisplayDevice->cb >= sizeof(DISPLAY_DEVICE)) {
            psz = (LPSTR)&(lpDisplayDevice->DeviceKey[0]);
            WCSToMB(&(tmpDisplayDevice.DeviceKey[0]), -1, &psz, 128, FALSE);
        }
#else

        RtlMoveMemory(lpDisplayDevice,
                      &tmpDisplayDevice,
                      lpDisplayDevice->cb);

#endif

        return TRUE;
    }

    return FALSE;
}

BOOL EnumDisplaySettings(
    LPCTSTR   lpszDeviceName,
    DWORD     iModeNum,
    LPDEVMODE lpDevMode)
{

    //
    // Work-around Win95 problem which does not require the caller
    // to initialize these two fields.
    //

    lpDevMode->dmDriverExtra = 0;
    lpDevMode->dmSize = FIELD_OFFSET(DEVMODE, dmICMMethod);

    return EnumDisplaySettingsEx(lpszDeviceName, iModeNum, lpDevMode, 0);
}

BOOL EnumDisplaySettingsEx(
    LPCTSTR   lpszDeviceName,
    DWORD     iModeNum,
    LPDEVMODE lpDevMode,
    DWORD     dwFlags)
{
    UNICODE_STRING  UnicodeString;
    PUNICODE_STRING pUnicodeString = NULL;
    LPDEVMODEW      lpDevModeReserve;
    BOOL            retval = FALSE;
    WORD            size = lpDevMode->dmSize;

    if (lpszDeviceName) {

#ifdef UNICODE

        RtlInitUnicodeString(&UnicodeString, lpszDeviceName);

#else

        ANSI_STRING     AnsiString;

        UnicodeString = NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString, (LPSTR)lpszDeviceName);

        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                     &AnsiString,
                                                     FALSE))) {
            return FALSE;
        }

#endif

        pUnicodeString = &UnicodeString;
    }

    /*
     * Currently -2 is reserved (undocumented function of the NT api.
     * remove the check is win95 implements this.
     * -> -1 returns the content of the registry at the time of the call
     *
     *
     * if (iModeNum == (DWORD) -2)
     * {
     *     return FALSE;
     * }
     *
     *
     * -1 should return the current DEVMODE for the device.
     * This is handled in the kernel part of the function, so we pass it on.
     *
     *
     *
     * We will always request a full DEVMODE from the kernel.
     * So allocate the space needed
     *
     */
    lpDevModeReserve = UserLocalAlloc(HEAP_ZERO_MEMORY,
                                      sizeof(DEVMODEW) + lpDevMode->dmDriverExtra);

    if (lpDevModeReserve) {

        lpDevModeReserve->dmSize = sizeof(DEVMODEW);
        lpDevModeReserve->dmDriverExtra = lpDevMode->dmDriverExtra;

        /*
         * Get the information
         */
        retval = (NT_SUCCESS(NtUserEnumDisplaySettings(pUnicodeString,
                                                       iModeNum,
                                                       lpDevModeReserve,
                                                       dwFlags)));
        if (retval) {

#ifndef UNICODE
            LPSTR psz;
#endif

            /*
             * return only the amount of information requested.
             * For ANSI, this requires a conversion.
             */

            /*
             * First, copy the driver extra information
             */

            if (lpDevMode->dmDriverExtra &&
                lpDevModeReserve->dmDriverExtra) {

                RtlMoveMemory(((PUCHAR)lpDevMode) + size,
                              lpDevModeReserve + 1,
                              min(lpDevMode->dmDriverExtra,
                                  lpDevModeReserve->dmDriverExtra));
            }

#ifndef UNICODE
            psz = (LPSTR)&(lpDevMode->dmDeviceName[0]);

            retval = WCSToMB(lpDevModeReserve->dmDeviceName,
                             -1,
                             &psz,
                             32,
                             FALSE);

            RtlMoveMemory(&lpDevMode->dmSpecVersion,
                          &lpDevModeReserve->dmSpecVersion,
                          min(size, FIELD_OFFSET(DEVMODE,dmFormName)) -
                              FIELD_OFFSET(DEVMODE,dmSpecVersion));

            lpDevMode->dmSize = size;

            if (size >= FIELD_OFFSET(DEVMODE,dmFormName)) {

                psz = (LPSTR)&(lpDevMode->dmFormName[0]);

                retval = WCSToMB(lpDevModeReserve->dmFormName, -1, &psz, 32, FALSE);
            }

            if (size > FIELD_OFFSET(DEVMODE,dmBitsPerPel)) {

                RtlMoveMemory(&lpDevMode->dmBitsPerPel,
                              &lpDevModeReserve->dmBitsPerPel,
                              lpDevMode->dmSize +
                                  lpDevMode->dmDriverExtra -
                                  FIELD_OFFSET(DEVMODE,dmBitsPerPel));
            }

#else
            RtlMoveMemory(lpDevMode, lpDevModeReserve, size);

            lpDevMode->dmSize = size;

#endif

            if (size != lpDevMode->dmSize) {
                RIPMSG0(RIP_WARNING, "EnumDisplaySettings : Error in dmSize");
            }

            /*
             * Don't return invalid field flags to the application
             * Add any other new ones here.
             *
             * We assume apps at least have up to dmDisplayFrenquency for
             * now ...
             */

            if (size < FIELD_OFFSET(DEVMODE,dmPanningWidth))
                lpDevMode->dmFields &= ~DM_PANNINGWIDTH;

            if (size < FIELD_OFFSET(DEVMODE,dmPanningHeight))
                lpDevMode->dmFields &= ~DM_PANNINGHEIGHT;
        }

        LocalFree(lpDevModeReserve);
    }

    return retval;
}


LONG ChangeDisplaySettings(
    LPDEVMODE lpDevMode,
    DWORD     dwFlags)
{

    /*
     * Compatibility
     */
    if (lpDevMode)
        lpDevMode->dmDriverExtra = 0;

    return ChangeDisplaySettingsEx(NULL, lpDevMode, NULL, dwFlags, NULL);
}

LONG ChangeDisplaySettingsEx(
    LPCTSTR   lpszDeviceName,
    LPDEVMODE lpDevMode,
    HWND      hwnd,
    DWORD     dwFlags,
    LPVOID    lParam)
{
#ifndef UNICODE
    ANSI_STRING     AnsiString;
#endif

    UNICODE_STRING  UnicodeString;
    PUNICODE_STRING pUnicodeString = NULL;
    LONG            status = DISP_CHANGE_FAILED;
    LPDEVMODEW      lpDevModeW;

    if (lpszDeviceName) {

#ifdef UNICODE

        RtlInitUnicodeString(&UnicodeString, lpszDeviceName);

#else

        UnicodeString = NtCurrentTeb()->StaticUnicodeString;
        RtlInitAnsiString(&AnsiString, (LPSTR)lpszDeviceName);

        if (!NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
                                                     &AnsiString,
                                                     FALSE))) {
            return FALSE;
        }

#endif

        pUnicodeString = &UnicodeString;
    }

#ifdef UNICODE

    lpDevModeW = lpDevMode;

#else

    lpDevModeW = NULL;

    if (lpDevMode) {

        lpDevModeW = GdiConvertToDevmodeW(lpDevMode);

        if (lpDevModeW == NULL) {

            return FALSE;
        }
    }

#endif

    status = NtUserChangeDisplaySettings(pUnicodeString,
                                         lpDevModeW,
                                         hwnd,
                                         dwFlags,
                                         lParam);

#ifndef UNICODE
    if (lpDevMode) {
        LocalFree(lpDevModeW);
    }
#endif

    return status;
}


BOOL CallMsgFilter(
    LPMSG pmsg,
    int   nCode)
{
    PCLIENTINFO pci;
    MSG         msg;

    BEGINCALLCONNECT()

        /*
         * If we're not hooked, don't bother going to the server
         */
        pci = GetClientInfo();
        if (!IsHooked(pci, (WH_MSGFILTER | WH_SYSMSGFILTER))) {
            return FALSE;
        }

        /*
         * Don't allow apps to use the hiword of the message parameter.
         */
        if (pmsg->message & RESERVED_MSG_BITS) {
            MSGERRORCODE(ERROR_INVALID_PARAMETER);
        }
        msg = *pmsg;

#ifndef UNICODE
        switch (pmsg->message) {
#ifdef FE_SB // CallMsgFilter()
        case WM_CHAR:
        case EM_SETPASSWORDCHAR:
#ifndef LATER
             /*
              * we should not return "TRUE" everytime for DBCS leadbyte character...
              * but should convert DBCS character to Unicode correctly.. How I can do ??
              * then ,finally, we just take what we did in NT 3.51, it means do nothing..
              */
#else
             /*
              * Build DBCS-aware message.
              */
             BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(pmsg->message,pmsg->wParam,TRUE);
             /*
              * Fall through.....
              */
#endif // LATER
#else
        case WM_CHAR:
        case EM_SETPASSWORDCHAR:
#endif // FE_SB
        case WM_CHARTOITEM:
        case WM_DEADCHAR:
        case WM_SYSCHAR:
        case WM_SYSDEADCHAR:
        case WM_MENUCHAR:
#ifdef FE_IME // CallMsgFilter()
        case WM_IME_CHAR:
        case WM_IME_COMPOSITION:
#endif // FE_IME

            RtlMBMessageWParamCharToWCS( msg.message, &(msg.wParam));
            break;
        }
#endif //!UNICODE

        retval = (DWORD)NtUserCallMsgFilter(
                &msg,
                nCode);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL DrawCaptionTemp(
    HWND hwnd,
    HDC hdc,
    LPCRECT lprc,
    HFONT hFont,
    HICON hicon,
    LPCTSTR lpText,
    UINT flags)
{
    HDC hdcr;
    IN_STRING strText;

    /*
     * Make sure cleanup will work successfully
     */
    strText.fAllocated = FALSE;

    BEGINCALL()

        if (IsMetaFile(hdc)) return FALSE;

        hdcr = GdiConvertAndCheckDC(hdc);
        if (hdcr == (HDC)0)
            return FALSE;

        FIRSTCOPYLPTSTRIDOPT(&strText, lpText);

        retval = (DWORD)NtUserDrawCaptionTemp(
                hwnd,
                hdc,
                lprc,
                hFont,
                hicon,
                strText.pstr,
                flags);

    ERRORTRAP(0);
    CLEANUPLPTSTR(strText);
    ENDCALL(BOOL);
}

WINUSERAPI UINT WINAPI
RealGetWindowClass(
    HWND hwnd,
    LPTSTR ptszClassName,
    UINT cchClassNameMax)
{
    UNICODE_STRING strClassName;
    int retval;

    strClassName.MaximumLength = (USHORT)(cchClassNameMax * sizeof(WCHAR));

#ifndef UNICODE
    strClassName.Buffer = LocalAlloc(LMEM_FIXED, strClassName.MaximumLength);
    if (!strClassName.Buffer) {
        return 0;
    }
#else
    strClassName.Buffer = ptszClassName;
#endif

    retval = NtUserGetClassName(hwnd, TRUE, &strClassName);

#ifndef UNICODE
    if (retval || (cchClassNameMax == 1)) {
        /*
         * Copy the result
         */
        retval = WCSToMB(strClassName.Buffer, retval,
                &ptszClassName, cchClassNameMax-1, FALSE);
        ptszClassName[retval] = '\0';
    }
    LocalFree(strClassName.Buffer);
#endif

  return retval;
}

WINUSERAPI BOOL WINAPI GetAltTabInfo(
    HWND hwnd,
    int iItem,
    PALTTABINFO pati,
    LPTSTR pszItemText,
    UINT cchItemText OPTIONAL)
{
    BEGINCALL()

    retval = (DWORD)NtUserGetAltTabInfo(hwnd,  iItem, pati,
            (LPWSTR)pszItemText, cchItemText, IS_ANSI);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}
