/**************************************************************************\
* Module Name: ntstubs.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* client side API stubs
*
* History:
* 03-19-95 JimA             Created.
\**************************************************************************/

#include "precomp.h"
#pragma hdrstop

#define CLIENTSIDE 1

#include <dbt.h>

#include "ntsend.h"
#include "cfgmgr32.h"
#include "csrhlpr.h"

extern BOOL GetRemoteKeyboardLayout(PWCHAR);

WINUSERAPI
BOOL
WINAPI
SetSysColors(
    int cElements,
    CONST INT * lpaElements,
    CONST COLORREF * lpaRgbValues)
{

    return NtUserSetSysColors(cElements,
                              lpaElements,
                              lpaRgbValues,
                              SSCF_NOTIFY | SSCF_FORCESOLIDCOLOR | SSCF_SETMAGICCOLORS);
}


HWND WOWFindWindow(
    LPCSTR pClassName,
    LPCSTR pWindowName)
{
    return InternalFindWindowExA(NULL, NULL, pClassName, pWindowName, FW_16BIT);
}


BOOL UpdatePerUserSystemParameters(
    HANDLE  hToken,
    BOOL    bUserLoggedOn)
{
    WCHAR pwszKLID[KL_NAMELENGTH];

    BEGINCALL()

        /*
         * Initialize IME hotkeys before loading keyboard
         * layouts.
         */
        CliImmInitializeHotKeys(ISHK_INITIALIZE, NULL);
        /*
         * Load initial keyboard layout.
         */
        if (!GetRemoteKeyboardLayout(pwszKLID)) {
            GetActiveKeyboardName(pwszKLID);
        }

        LoadKeyboardLayoutWorker(NULL, pwszKLID, KLF_ACTIVATE | KLF_RESET | KLF_SUBSTITUTE_OK, TRUE);

        /*
         * Now load the remaining preload keyboard layouts.
         */
        LoadPreloadKeyboardLayouts();
        retval = (DWORD)NtUserUpdatePerUserSystemParameters(hToken, bUserLoggedOn);

        /*
         * Cause the wallpaper to be changed.
         */
        SystemParametersInfo(SPI_SETDESKWALLPAPER, 0, 0, 0);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

DWORD Event(
    PEVENT_PACKET pep)
{
    BEGINCALL()

        CheckDDECritOut;

        retval = (DWORD)NtUserEvent(
                pep);

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

LONG GetClassWOWWords(
    HINSTANCE hInstance,
    LPCTSTR pString)
{
    IN_STRING strClassName;
    PCLS pcls;

    /*
     * Make sure cleanup will work successfully
     */
    strClassName.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPSTRW(&strClassName, pString);

        pcls = NtUserGetWOWClass(hInstance, strClassName.pstr);

        if (pcls == NULL) {
            MSGERRORCODE(ERROR_CLASS_DOES_NOT_EXIST);
        }

        pcls = (PCLS)((PBYTE)pcls - GetClientInfo()->ulClientDelta);
        retval = _GetClassData(pcls, NULL, GCLP_WOWWORDS, TRUE);

    ERRORTRAP(0);
    CLEANUPLPSTRW(strClassName);
    ENDCALL(LONG);
}

/***************************************************************************\
* InitTask
*
* Initialize a WOW task.  This is the first call a WOW thread makes to user.
* NtUserInitTask returns NTSTATUS because if the thread fails to convert
* to a GUI thread, STATUS_INVALID_SYSTEM_SERVICE is returned.
*
* 11-03-95 JimA         Modified to use NTSTATUS.
\***************************************************************************/

BOOL InitTask(
    UINT wVersion,
    DWORD dwAppCompatFlags,
    LPCSTR pszModName,
    LPCSTR pszBaseFileName,
    DWORD hTaskWow,
    DWORD dwHotkey,
    DWORD idTask,
    DWORD dwX,
    DWORD dwY,
    DWORD dwXSize,
    DWORD dwYSize)
{
    IN_STRING strModName;
    IN_STRING strBaseFileName;
    NTSTATUS Status;

    /*
     * Make sure cleanup will work successfully
     */
    strModName.fAllocated = FALSE;
    strBaseFileName.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPSTRW(&strModName, pszModName);
        COPYLPSTRW(&strBaseFileName, pszBaseFileName);

        Status = NtUserInitTask(
                wVersion,
                dwAppCompatFlags,
                strModName.pstr,
                strBaseFileName.pstr,
                hTaskWow,
                dwHotkey,
                idTask,
                dwX,
                dwY,
                dwXSize,
                dwYSize);
        retval = (Status == STATUS_SUCCESS);

        CLEANUPLPSTRW(strModName);
        CLEANUPLPSTRW(strBaseFileName);

    ERRORTRAP(FALSE);
    ENDCALL(BOOL);
}

HANDLE ConvertMemHandle(
    HANDLE hData,
    UINT cbNULL)
{
    UINT cbData;
    LPBYTE lpData;

    BEGINCALL()

        if (GlobalFlags(hData) == GMEM_INVALID_HANDLE) {
            RIPMSG0(RIP_WARNING, "ConvertMemHandle hMem is not valid\n");
            MSGERROR();
            }

        if (!(cbData = (UINT)GlobalSize(hData)))
            MSGERROR();

        USERGLOBALLOCK(hData, lpData);
        if (lpData == NULL) {
            MSGERROR();
        }

        /*
         * Make sure text formats are NULL terminated.
         */
        switch (cbNULL) {
        case 2:
            lpData[cbData - 2] = 0;
            // FALL THROUGH
        case 1:
            lpData[cbData - 1] = 0;
        }

        retval = (ULONG_PTR)NtUserConvertMemHandle(lpData, cbData);

        USERGLOBALUNLOCK(hData);

    ERRORTRAP(NULL);
    ENDCALL(HANDLE);
}

HANDLE CreateLocalMemHandle(
    HANDLE hMem)
{
    UINT cbData;
    NTSTATUS Status;

    BEGINCALL()

        Status = NtUserCreateLocalMemHandle(hMem, NULL, 0, &cbData);
        if (Status != STATUS_BUFFER_TOO_SMALL) {
            RIPMSG0(RIP_WARNING, "__CreateLocalMemHandle server returned failure\n");
            MSGERROR();
        }

        if (!(retval = (ULONG_PTR)GlobalAlloc(GMEM_FIXED, cbData)))
            MSGERROR();

        Status = NtUserCreateLocalMemHandle(hMem, (LPBYTE)retval, cbData, NULL);
        if (!NT_SUCCESS(Status)) {
            RIPMSG0(RIP_WARNING, "__CreateLocalMemHandle server returned failure\n");
            UserGlobalFree((HANDLE)retval);
            MSGERROR();
        }

    ERRORTRAP(0);
    ENDCALL(HANDLE);
}

HHOOK _SetWindowsHookEx(
    HANDLE hmod,
    LPTSTR pszLib,
    DWORD idThread,
    int nFilterType,
    PROC pfnFilterProc,
    DWORD dwFlags)
{
    IN_STRING strLib;

    /*
     * Make sure cleanup will work successfully
     */
    strLib.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPWSTROPT(&strLib, pszLib);

        retval = (ULONG_PTR)NtUserSetWindowsHookEx(
                hmod,
                strLib.pstr,
                idThread,
                nFilterType,
                pfnFilterProc,
                dwFlags);

    ERRORTRAP(0);
    CLEANUPLPWSTR(strLib);
    ENDCALL(HHOOK);
}

/***************************************************************************\
* SetWinEventHook
*
* History:
* 1996-09-23 IanJa Created
\***************************************************************************/
WINUSERAPI
HWINEVENTHOOK
WINAPI
SetWinEventHook(
    DWORD        eventMin,
    DWORD        eventMax,
    HMODULE      hmodWinEventProc,   // Must pass this if global!
    WINEVENTPROC lpfnWinEventProc,
    DWORD        idProcess,          // Can be zero; all processes
    DWORD        idThread,           // Can be zero; all threads
    DWORD        dwFlags)
{
    UNICODE_STRING str;
    PUNICODE_STRING pstr;
    WCHAR awchLib[MAX_PATH];

    BEGINCALL()

        if ((dwFlags & WINEVENT_INCONTEXT) && (hmodWinEventProc != NULL)) {
            /*
             * If we're passing an hmod, we need to grab the file name of the
             * module while we're still on the client since module handles
             * are NOT global.
             */
            USHORT cb;
            cb = (USHORT)(sizeof(WCHAR) * GetModuleFileNameW(hmodWinEventProc, awchLib, sizeof(awchLib)/sizeof(WCHAR)));
            if (cb == 0) {
                /*
                 * hmod is bogus - return NULL.
                 */
                return NULL;
            }
            str.Buffer = awchLib;
            str.Length = cb - sizeof(UNICODE_NULL);
            str.MaximumLength = cb;
            pstr = &str;
        } else {
            pstr = NULL;
        }

        retval = (ULONG_PTR)NtUserSetWinEventHook(
                eventMin,
                eventMax,
                hmodWinEventProc,
                pstr,
                lpfnWinEventProc,
                idProcess,
                idThread,
                dwFlags);

    ERRORTRAP(0);
    ENDCALL(HWINEVENTHOOK);
};


WINUSERAPI
VOID
WINAPI
NotifyWinEvent(
    DWORD dwEvent,
    HWND  hwnd,
    LONG  idObject,
    LONG  idChild)
{
    BEGINCALLVOID()

    if (FWINABLE()) {
        NtUserNotifyWinEvent(dwEvent, hwnd, idObject, idChild);
    }

    ERRORTRAPVOID();
    ENDCALLVOID();
}


/***************************************************************************\
* ThunkedMenuItemInfo
*
* History:
*  07-22-96 GerardoB - Added header and Fixed up for 5.0
\***************************************************************************/
BOOL ThunkedMenuItemInfo(
    HMENU hMenu,
    UINT nPosition,
    BOOL fByPosition,
    BOOL fInsert,
    LPMENUITEMINFOW lpmii,
    BOOL fAnsi)
{
    MENUITEMINFOW mii;
    IN_STRING strItem;

    /*
     * Make sure cleanup will work successfully
     */
    strItem.fAllocated = FALSE;

    BEGINCALL()

        /*
         *  Make a local copy so we can make changes
         */
        mii = *(LPMENUITEMINFO)(lpmii);

        strItem.pstr = NULL;
        if (mii.fMask & MIIM_BITMAP) {
            if (((HBITMAP)LOWORD(HandleToUlong(mii.hbmpItem)) < HBMMENU_MAX) && IS_PTR(mii.hbmpItem)) {
                /*
                 *  Looks like the user was trying to insert one of the
                 *  HBMMENU_* bitmaps, but stuffed some data in the HIWORD.
                 *  We know the HIWORD data is invalid because the LOWORD
                  *  handle is below the GDI minimum.
                 */
                RIPMSG1(RIP_WARNING, "Invalid HIWORD data (0x%04X) for HBMMENU_* bitmap.", HIWORD(HandleToUlong(mii.hbmpItem)));
                mii.hbmpItem = (HBITMAP)LOWORD(HandleToUlong(mii.hbmpItem));
            } else if (!IS_PTR(mii.hbmpItem) && (mii.hbmpItem >= HBMMENU_MAX)) {
            /*
             * The app is passing a 16-bit GDI handle.  GDI handles this on the
             * client-side, but not on the kernel side.  So convert it to 32-bits.
             * This fixes bug 201493 in Macromedia Director.
             */
                HBITMAP hbmNew = GdiFixUpHandle(mii.hbmpItem);
                if (hbmNew) {
                    RIPMSG2(RIP_WARNING, "Menu bitmap change, fix 16-bit bitmap handle %lx to %lx\n", mii.hbmpItem, hbmNew);
                    mii.hbmpItem = hbmNew;
                }
            }
        }

        if (mii.fMask & MIIM_STRING){
            if (fAnsi) {
                FIRSTCOPYLPSTROPTW(&strItem, mii.dwTypeData);
            } else {
                FIRSTCOPYLPWSTROPT(&strItem, mii.dwTypeData);
            }
        }

        retval = (DWORD)NtUserThunkedMenuItemInfo(
                hMenu,
                nPosition,
                fByPosition,
                fInsert,
                &mii,
                strItem.pstr);

    ERRORTRAP(FALSE);
    CLEANUPLPSTRW(strItem);
    ENDCALL(BOOL);
}

BOOL DrawCaption(
    HWND hwnd,
    HDC hdc,
    CONST RECT *lprc,
    UINT flags)
{
    HDC hdcr;
    BEGINCALL()

        if (IsMetaFile(hdc))
            return FALSE;

        hdcr = GdiConvertAndCheckDC(hdc);
        if (hdcr == (HDC)0)
            return FALSE;

        retval = (DWORD)NtUserDrawCaption(hwnd, hdcr, lprc, flags);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

SHORT GetAsyncKeyState(
    int vKey)
{
    BEGINCALLCONNECT()

        /*
         * Asynchronous key state reports the PHYSICAL mouse button,
         * regardless of whether the buttons have been swapped or not.
         */
        if (((vKey == VK_RBUTTON) || (vKey == VK_LBUTTON)) && SYSMET(SWAPBUTTON)) {
            vKey ^= (VK_RBUTTON ^ VK_LBUTTON);
        }

        /*
         * If this is one of the common keys, see if we can pull it out
         * of the cache.
         */
        if ((UINT)vKey < CVKASYNCKEYCACHE) {
            PCLIENTINFO pci = GetClientInfo();
            if ((pci->dwAsyncKeyCache == gpsi->dwAsyncKeyCache) &&
                !TestKeyRecentDownBit(pci->afAsyncKeyStateRecentDown, vKey)) {

                if (TestKeyDownBit(pci->afAsyncKeyState, vKey))
                    retval = 0x8000;
                else
                    retval = 0;

                return (SHORT)retval;
            }
        }

        retval = (DWORD)NtUserGetAsyncKeyState(
                vKey);

    ERRORTRAP(0);
    ENDCALL(SHORT);
}

SHORT GetKeyState(
    int vKey)
{
    BEGINCALLCONNECT()

        /*
         * If this is one of the common keys, see if we can pull it out
         * of the cache.
         */
        if ((UINT)vKey < CVKKEYCACHE) {
            PCLIENTINFO pci = GetClientInfo();
            if (pci->dwKeyCache == gpsi->dwKeyCache) {
                retval = 0;
                if (TestKeyToggleBit(pci->afKeyState, vKey))
                    retval |= 0x0001;
                if (TestKeyDownBit(pci->afKeyState, vKey)) {
                  /*
                   * Used to be retval |= 0x8000.Fix for bug 28820; Ctrl-Enter
                   * accelerator doesn't work on Nestscape Navigator Mail 2.0
                   */
                    retval |= 0xff80;  // This is what 3.1 returned!!!!
                }

                return (SHORT)retval;
            }
        }

        retval = (DWORD)NtUserGetKeyState(
                vKey);

    ERRORTRAP(0);
    ENDCALL(SHORT);
}

BOOL OpenClipboard(
    HWND hwnd)
{
    BOOL fEmptyClient;

    BEGINCALL()

        retval = (DWORD)NtUserOpenClipboard(hwnd, &fEmptyClient);

        if (fEmptyClient)
            ClientEmptyClipboard();

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL _PeekMessage(
    LPMSG pmsg,
    HWND hwnd,
    UINT wMsgFilterMin,
    UINT wMsgFilterMax,
    UINT wRemoveMsg,
    BOOL bAnsi)
{
    BEGINCALL()

        if (bAnsi) {
            //
            // If we have pushed message for DBCS messaging, we should pass this one
            // to Apps at first...
            //
            GET_DBCS_MESSAGE_IF_EXIST(
                PeekMessage,pmsg,wMsgFilterMin,wMsgFilterMax,((wRemoveMsg & PM_REMOVE) ? TRUE:FALSE));
        }

        retval = (DWORD)NtUserPeekMessage(
                pmsg,
                hwnd,
                wMsgFilterMin,
                wMsgFilterMax,
                wRemoveMsg);

        if (retval) {
            // May have a bit more work to do if this MSG is for an ANSI app

            if (bAnsi) {
                if (RtlWCSMessageWParamCharToMB(pmsg->message, &(pmsg->wParam))) {
                    WPARAM dwAnsi = pmsg->wParam;
                    //
                    // Build DBCS-ware wParam. (for EM_SETPASSWORDCHAR...)
                    //
                    BUILD_DBCS_MESSAGE_TO_CLIENTA_FROM_SERVER(
                        pmsg,dwAnsi,TRUE,((wRemoveMsg & PM_REMOVE) ? TRUE:FALSE));
                } else {
                    retval = 0;
                }
            } else {
               //
               // Only LOWORD of WPARAM is valid for WM_CHAR....
               // (Mask off DBCS messaging information.)
               //
               BUILD_DBCS_MESSAGE_TO_CLIENTW_FROM_SERVER(pmsg->message,pmsg->wParam);
            }
        }

ExitPeekMessage:

    ERRORTRAP(0);
    ENDCALL(BOOL);
}


LONG_PTR _SetWindowLongPtr(
    HWND hwnd,
    int nIndex,
    LONG_PTR dwNewLong,
    BOOL bAnsi)
{
    PWND pwnd;
    LONG_PTR dwOldLong;
    DWORD dwCPDType = 0;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    if (TestWF(pwnd, WFDIALOGWINDOW)) {
        switch (nIndex) {
        case DWLP_DLGPROC:    // See similar case GWL_WNDPROC

            /*
             * Hide the window proc from other processes
             */
            if (!TestWindowProcess(pwnd)) {
                RIPERR1(ERROR_ACCESS_DENIED,
                        RIP_WARNING,
                        "Access denied to hwnd (%#lx) in _SetWindowLong",
                        hwnd);

                return 0;
            }

            /*
             * Get the old window proc address
             */
            dwOldLong = (LONG_PTR)PDLG(pwnd)->lpfnDlg;

            /*
             * We always store the actual address in the wndproc; We only
             * give the CallProc handles to the application
             */
            UserAssert(!ISCPDTAG(dwOldLong));

            /*
             * May need to return a CallProc handle if there is an
             * Ansi/Unicode tranistion
             */

            if (bAnsi != ((PDLG(pwnd)->flags & DLGF_ANSI) ? TRUE : FALSE)) {
                dwCPDType |= bAnsi ? CPD_ANSI_TO_UNICODE : CPD_UNICODE_TO_ANSI;
            }

            /*
             * If we detected a transition create a CallProc handle for
             * this type of transition and this wndproc (dwOldLong)
             */
            if (dwCPDType) {
                ULONG_PTR cpd;

                cpd = GetCPD(pwnd, dwCPDType | CPD_DIALOG, dwOldLong);

                if (cpd) {
                    dwOldLong = cpd;
                } else {
                    RIPMSG0(RIP_WARNING, "SetWindowLong (DWL_DLGPROC) unable to alloc CPD returning handle\n");
                }
            }

            /*
             * Convert a possible CallProc Handle into a real address.
             * The app may have kept the CallProc Handle from some
             * previous mixed GetClassinfo or SetWindowLong.
             *
             * WARNING bAnsi is modified here to represent real type of
             * proc rather than if SetWindowLongA or W was called
             */
            if (ISCPDTAG(dwNewLong)) {
                PCALLPROCDATA pCPD;
                if (pCPD = HMValidateHandleNoRip((HANDLE)dwNewLong, TYPE_CALLPROC)) {
                    dwNewLong = KERNEL_ULONG_PTR_TO_ULONG_PTR(pCPD->pfnClientPrevious);
                    bAnsi = pCPD->wType & CPD_UNICODE_TO_ANSI;
                }
            }

            /*
             * If an app 'unsubclasses' a server-side window proc we need to
             * restore everything so SendMessage and friends know that it's
             * a server-side proc again.  Need to check against client side
             * stub addresses.
             */
            PDLG(pwnd)->lpfnDlg = (DLGPROC)dwNewLong;
            if (bAnsi) {
                PDLG(pwnd)->flags |= DLGF_ANSI;
            } else {
                PDLG(pwnd)->flags &= ~DLGF_ANSI;
            }

            return dwOldLong;

        case DWLP_USER:
#ifdef BUILD_WOW6432
            // kernel has special handling of DWLP_USER
            nIndex = sizeof(KERNEL_LRESULT) + sizeof(KERNEL_PVOID);
#endif
        case DWLP_MSGRESULT:
            break;

        default:
            if (nIndex >= 0 && nIndex < DLGWINDOWEXTRA) {
                RIPERR0(ERROR_PRIVATE_DIALOG_INDEX, RIP_VERBOSE, "");
                return 0;
            }
        }
    }

    BEGINCALL()

    /*
     * If this is a listbox window and the listbox structure has
     * already been initialized, don't allow the app to override the
     * owner draw styles. We need to do this since Windows only
     * used the styles in creating the structure, but we also use
     * them to determine if strings need to be thunked.
     *
     */

    if (nIndex == GWL_STYLE &&
        GETFNID(pwnd) == FNID_LISTBOX &&
        ((PLBWND)pwnd)->pLBIV != NULL &&
        (!TestWindowProcess(pwnd) || ((PLBWND)pwnd)->pLBIV->fInitialized)) {

#if DBG
        LONG_PTR dwDebugLong = dwNewLong;
#endif

        dwNewLong &= ~(LBS_OWNERDRAWFIXED |
                       LBS_OWNERDRAWVARIABLE |
                       LBS_HASSTRINGS);

        dwNewLong |= pwnd->style & (LBS_OWNERDRAWFIXED |
                                    LBS_OWNERDRAWVARIABLE |
                                    LBS_HASSTRINGS);

#if DBG
        if (dwDebugLong != dwNewLong) {
           RIPMSG0(RIP_WARNING, "SetWindowLong can't change LBS_OWNERDRAW* or LBS_HASSTRINGS.");
        }
#endif
    }


        retval = (ULONG_PTR)NtUserSetWindowLongPtr(
                hwnd,
                nIndex,
                dwNewLong,
                bAnsi);

    ERRORTRAP(0);
    ENDCALL(LONG_PTR);
}

#ifdef _WIN64
LONG _SetWindowLong(
    HWND hwnd,
    int nIndex,
    LONG dwNewLong,
    BOOL bAnsi)
{
    PWND pwnd;

    pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    if (TestWF(pwnd, WFDIALOGWINDOW)) {
        switch (nIndex) {
        case DWLP_DLGPROC:    // See similar case GWLP_WNDPROC
            RIPERR1(ERROR_INVALID_INDEX, RIP_WARNING, "SetWindowLong: invalid index %d", nIndex);
            return 0;

        case DWLP_MSGRESULT:
        case DWLP_USER:
            break;

        default:
            if (nIndex >= 0 && nIndex < DLGWINDOWEXTRA) {
                RIPERR0(ERROR_PRIVATE_DIALOG_INDEX, RIP_VERBOSE, "");
                return 0;
            }
        }
    }

    BEGINCALL()

    /*
     * If this is a listbox window and the listbox structure has
     * already been initialized, don't allow the app to override the
     * owner draw styles. We need to do this since Windows only
     * used the styles in creating the structure, but we also use
     * them to determine if strings need to be thunked.
     *
     */

    if (nIndex == GWL_STYLE &&
        GETFNID(pwnd) == FNID_LISTBOX &&
        ((PLBWND)pwnd)->pLBIV != NULL &&
        (!TestWindowProcess(pwnd) || ((PLBWND)pwnd)->pLBIV->fInitialized)) {

#if DBG
        LONG dwDebugLong = dwNewLong;
#endif

        dwNewLong &= ~(LBS_OWNERDRAWFIXED |
                       LBS_OWNERDRAWVARIABLE |
                       LBS_HASSTRINGS);

        dwNewLong |= pwnd->style & (LBS_OWNERDRAWFIXED |
                                    LBS_OWNERDRAWVARIABLE |
                                    LBS_HASSTRINGS);

#if DBG
        if (dwDebugLong != dwNewLong) {
           RIPMSG0(RIP_WARNING, "SetWindowLong can't change LBS_OWNERDRAW* or LBS_HASSTRINGS.");
        }
#endif
    }


        retval = (DWORD)NtUserSetWindowLong(
                hwnd,
                nIndex,
                dwNewLong,
                bAnsi);

    ERRORTRAP(0);
    ENDCALL(LONG);
}
#endif

BOOL TranslateMessageEx(
    CONST MSG *pmsg,
    UINT flags)
{
    BEGINCALL()

        /*
         * Don't bother going over to the kernel if this isn't
         * key message.
         */
        switch (pmsg->message) {
        case WM_KEYDOWN:
        case WM_KEYUP:
        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
            break;
        default:
            if (pmsg->message & RESERVED_MSG_BITS) {
                RIPERR1(ERROR_INVALID_PARAMETER,
                        RIP_WARNING,
                        "Invalid parameter \"pmsg->message\" (%ld) to TranslateMessageEx",
                        pmsg->message);
            }
            MSGERROR();
        }

        retval = (DWORD)NtUserTranslateMessage(
                pmsg,
                flags);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL TranslateMessage(
    CONST MSG *pmsg)
{
    //
    // IME special key handling
    //
    if ( LOWORD(pmsg->wParam) == VK_PROCESSKEY ) {
        BOOL fResult;
        //
        // This vkey should be processed by IME
        //
        fResult = fpImmTranslateMessage( pmsg->hwnd,
                                       pmsg->message,
                                       pmsg->wParam,
                                       pmsg->lParam );
        if ( fResult )
            return fResult;
    }
    return(TranslateMessageEx(pmsg, 0));
}

BOOL SetWindowRgn(
    HWND hwnd,
    HRGN hrgn,
    BOOL bRedraw)
{
    BEGINCALL()

        retval = (DWORD)NtUserSetWindowRgn(
                hwnd,
                hrgn,
                bRedraw);

        if (retval) {
            DeleteObject(hrgn);
        }

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL InternalGetWindowText(
    HWND hwnd,
    LPWSTR pString,
    int cchMaxCount)
{
    BEGINCALL()

        retval = (DWORD)NtUserInternalGetWindowText(
                hwnd,
                pString,
                cchMaxCount);

        if (!retval) {
            *pString = (WCHAR)0;
        }

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

int ToUnicode(
    UINT wVirtKey,
    UINT wScanCode,
    CONST BYTE *pKeyState,
    LPWSTR pwszBuff,
    int cchBuff,
    UINT wFlags)
{
    BEGINCALL()

        retval = (DWORD)NtUserToUnicodeEx(
                wVirtKey,
                wScanCode,
                pKeyState,
                pwszBuff,
                cchBuff,
                wFlags,
                (HKL)NULL);

        if (!retval) {
            *pwszBuff = L'\0';
        }

    ERRORTRAP(0);
    ENDCALL(int);
}

int ToUnicodeEx(
    UINT wVirtKey,
    UINT wScanCode,
    CONST BYTE *pKeyState,
    LPWSTR pwszBuff,
    int cchBuff,
    UINT wFlags,
    HKL hkl)
{
    BEGINCALL()

    retval = (DWORD)NtUserToUnicodeEx(
            wVirtKey,
            wScanCode,
            pKeyState,
            pwszBuff,
            cchBuff,
            wFlags,
            hkl);

    if (!retval) {
        *pwszBuff = L'\0';
    }

    ERRORTRAP(0);
    ENDCALL(int);
}

#if DBG
VOID DbgWin32HeapFail(
    DWORD dwFlags,
    BOOL  bFail)
{
    if ((dwFlags | WHF_VALID) != WHF_VALID) {
        RIPMSG1(RIP_WARNING, "Invalid flags for DbgWin32HeapFail %x", dwFlags);
        return;
    }

    if (dwFlags & WHF_CSRSS) {
        // Tell csr about it
        CsrWin32HeapFail(dwFlags, bFail);
    }

    NtUserDbgWin32HeapFail(dwFlags, bFail);
}

DWORD DbgWin32HeapStat(
    PDBGHEAPSTAT    phs,
    DWORD   dwLen,
    DWORD   dwFlags)
{
    if ((dwFlags | WHF_VALID) != WHF_VALID) {
        RIPMSG1(RIP_WARNING, "Invalid flags for DbgWin32HeapFail %x", dwFlags);
        return 0;
    }

    if (dwFlags & WHF_CSRSS) {
        return CsrWin32HeapStat(phs, dwLen);
    } else if (dwFlags & WHF_DESKTOP) {
        return NtUserDbgWin32HeapStat(phs, dwLen);
    }
    return 0;
}

#endif // DBG

BOOL SetWindowStationUser(
    HWINSTA hwinsta,
    PLUID   pluidUser,
    PSID    psidUser,
    DWORD   cbsidUser)
{
    LUID luidNone = { 0, 0 };


    BEGINCALL()

        retval = (DWORD)NtUserSetWindowStationUser(hwinsta,
                                                   pluidUser,
                                                   psidUser,
                                                   cbsidUser);

        /*
         * Load global atoms if the logon succeeded
         */
        if (retval) {

            if (!RtlEqualLuid(pluidUser,&luidNone)) {
                /*
                 * Reset console and load Nls data.
                 */
                Logon(TRUE);
            } else {
                /*
                 * Flush NLS cache.
                 */
                Logon(FALSE);
            }

            retval = TRUE;
        }
    ERRORTRAP(0);
    ENDCALL(BOOL);
}

BOOL SetSystemCursor(
    HCURSOR hcur,
    DWORD   id)
{
    BEGINCALL()

        if (hcur == NULL) {
            hcur = (HANDLE)LoadIcoCur(NULL,
                                      MAKEINTRESOURCE(id),
                                      RT_CURSOR,
                                      0,
                                      0,
                                      LR_DEFAULTSIZE);

            if (hcur == NULL)
                MSGERROR();
        }

        retval = (DWORD)NtUserSetSystemCursor(hcur, id);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

HCURSOR FindExistingCursorIcon(
    LPWSTR      pszModName,
    LPCWSTR     pszResName,
    PCURSORFIND pcfSearch)
{
    IN_STRING strModName;
    IN_STRING strResName;

    /*
     * Make sure cleanup will work successfully
     */
    strModName.fAllocated = FALSE;
    strResName.fAllocated = FALSE;

    BEGINCALL()

        if (pszModName == NULL)
            pszModName = szUSER32;

        COPYLPWSTR(&strModName, pszModName);
        COPYLPWSTRID(&strResName, pszResName);

        retval = (ULONG_PTR)NtUserFindExistingCursorIcon(strModName.pstr,
                                                     strResName.pstr,
                                                     pcfSearch);

    ERRORTRAP(0);

    CLEANUPLPWSTR(strModName);
    CLEANUPLPWSTR(strResName);

    ENDCALL(HCURSOR);
}



BOOL _SetCursorIconData(
    HCURSOR     hCursor,
    PCURSORDATA pcur)
{
    IN_STRING  strModName;
    IN_STRING  strResName;

    /*
     * Make sure cleanup will work successfully
     */
    strModName.fAllocated = FALSE;
    strResName.fAllocated = FALSE;

    BEGINCALL()

        COPYLPWSTROPT(&strModName, pcur->lpModName);
        COPYLPWSTRIDOPT(&strResName, pcur->lpName);

        retval = (DWORD)NtUserSetCursorIconData(hCursor,
                                                strModName.pstr,
                                                strResName.pstr,
                                                pcur);

    ERRORTRAP(0);

    CLEANUPLPWSTR(strModName);
    CLEANUPLPWSTR(strResName);

    ENDCALL(BOOL);
}



BOOL _DefSetText(
    HWND hwnd,
    LPCWSTR lpszText,
    BOOL bAnsi)
{
    LARGE_STRING str;

    BEGINCALL()

        if (lpszText) {
            if (bAnsi)
                RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&str,
                        (LPSTR)lpszText, (UINT)-1);
            else
                RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&str,
                        lpszText, (UINT)-1);
        }

        retval = (DWORD)NtUserDefSetText(
                hwnd,
                lpszText ? &str : NULL);

    ERRORTRAP(0);
    ENDCALL(BOOL);
}

HWND _CreateWindowEx(
    DWORD dwExStyle,
    LPCTSTR pClassName,
    LPCTSTR pWindowName,
    DWORD dwStyle,
    int x,
    int y,
    int nWidth,
    int nHeight,
    HWND hwndParent,
    HMENU hmenu,
    HANDLE hModule,
    LPVOID pParam,
    DWORD dwFlags)
{
    LARGE_IN_STRING strClassName;
    LARGE_STRING strWindowName;
    PLARGE_STRING pstrClassName;
    PLARGE_STRING pstrWindowName;
    DWORD dwExpWinVerAndFlags;

    /*
     * Make sure cleanup will work successfully
     */
    strClassName.fAllocated = FALSE;

    /*
     * To be compatible with Chicago, we test the validity of
     * the ExStyle bits and fail if any invalid bits are found.
     * And for backward compatibilty with NT apps, we only fail for
     * new apps (post NT 3.1).
     */

// BOGUS

    if (dwExStyle & 0x00000800L) {
        dwExStyle |= WS_EX_TOOLWINDOW;
        dwExStyle &= 0xfffff7ffL;
    }

    dwExpWinVerAndFlags = (DWORD)(WORD)GETEXPWINVER(hModule);
    if ((dwExStyle & ~WS_EX_VALID50) && (dwExpWinVerAndFlags >= VER40) ) {
        RIPMSG0(RIP_ERROR, "Invalid 5.0 ExStyle\n");
        return NULL;
    }
    {

    BOOL fMDIchild = FALSE;
    MDICREATESTRUCT mdics;
    HMENU hSysMenu;

    BEGINCALL()

        if ((fMDIchild = (BOOL)(dwExStyle & WS_EX_MDICHILD))) {
            SHORTCREATE sc;
            PWND pwndParent;

            pwndParent = ValidateHwnd(hwndParent);

            if ((pwndParent == NULL) || (GETFNID(pwndParent) != FNID_MDICLIENT)) {
                RIPMSG0(RIP_ERROR, "Invalid parent for MDI child window\n");
                MSGERROR();
            }

            mdics.lParam  = (LPARAM)pParam;
            pParam = &mdics;
            mdics.x = sc.x = x;
            mdics.y = sc.y = y;
            mdics.cx = sc.cx = nWidth;
            mdics.cy = sc.cy = nHeight;
            mdics.style = sc.style = dwStyle;
            mdics.hOwner = hModule;
            mdics.szClass = pClassName;
            mdics.szTitle = pWindowName;

            if (!CreateMDIChild(&sc, &mdics, dwExpWinVerAndFlags, &hSysMenu, pwndParent))
                MSGERROR();

            x = sc.x;
            y = sc.y;
            nWidth = sc.cx;
            nHeight = sc.cy;
            dwStyle = sc.style;
            hmenu = sc.hMenu;
        }

        /*
         * Set up class and window name.  If the window name is an
         * ordinal, make it look like a string so the callback thunk
         * will be able to ensure it is in the correct format.
         */
        pstrWindowName = NULL;
        if (dwFlags & CW_FLAGS_ANSI) {
            dwExStyle = dwExStyle | WS_EX_ANSICREATOR;
            if (IS_PTR(pClassName)) {
                RtlCaptureLargeAnsiString(&strClassName,
                        (PCHAR)pClassName, TRUE);
                pstrClassName = (PLARGE_STRING)strClassName.pstr;
            } else
                pstrClassName = (PLARGE_STRING)pClassName;

            if (pWindowName != NULL) {
                if (*(PBYTE)pWindowName == 0xff) {
                    strWindowName.bAnsi = TRUE;
                    strWindowName.Buffer = (PVOID)pWindowName;
                    strWindowName.Length = 3;
                    strWindowName.MaximumLength = 3;
                } else
                    RtlInitLargeAnsiString((PLARGE_ANSI_STRING)&strWindowName,
                            (LPSTR)pWindowName, (UINT)-1);
                pstrWindowName = &strWindowName;
            }
        } else {
            if (IS_PTR(pClassName)) {
                RtlInitLargeUnicodeString(
                        (PLARGE_UNICODE_STRING)&strClassName.strCapture,
                        pClassName, (UINT)-1);
                pstrClassName = (PLARGE_STRING)&strClassName.strCapture;
            } else
                pstrClassName = (PLARGE_STRING)pClassName;

            if (pWindowName != NULL) {
                if (pWindowName != NULL &&
                     *(PWORD)pWindowName == 0xffff) {
                    strWindowName.bAnsi = FALSE;
                    strWindowName.Buffer = (PVOID)pWindowName;
                    strWindowName.Length = 4;
                    strWindowName.MaximumLength = 4;
                } else
                    RtlInitLargeUnicodeString((PLARGE_UNICODE_STRING)&strWindowName,
                            pWindowName, (UINT)-1);
                pstrWindowName = &strWindowName;
            }
        }
        if (dwFlags & CW_FLAGS_DIFFHMOD) {
            dwExpWinVerAndFlags |= CW_FLAGS_DIFFHMOD;
        }

        retval = (ULONG_PTR)NtUserCreateWindowEx(
                dwExStyle,
                pstrClassName,
                pstrWindowName,
                dwStyle,
                x,
                y,
                nWidth,
                nHeight,
                hwndParent,
                hmenu,
                hModule,
                pParam,
                dwExpWinVerAndFlags);

    // If this is an MDI child, we need to do some more to complete the
    // process of creating an MDI child.
    if (retval && fMDIchild) {
        MDICompleteChildCreation((HWND)retval, hSysMenu, ((dwStyle & WS_VISIBLE) != 0L), (BOOL)((dwStyle & WS_DISABLED)!= 0L));
    }


    ERRORTRAP(0);
    CLEANUPLPSTRW(strClassName);
    ENDCALL(HWND);
    }
}

HKL _LoadKeyboardLayoutEx(
    HANDLE hFile,
    UINT offTable,
    HKL hkl,
    LPCTSTR pwszKL,
    UINT KbdInputLocale,
    UINT Flags)
{
    IN_STRING strKL;

    /*
     * Make sure cleanup will work successfully
     */
    strKL.fAllocated = FALSE;

    BEGINCALL()

        FIRSTCOPYLPWSTR(&strKL, pwszKL);

        retval = (ULONG_PTR)NtUserLoadKeyboardLayoutEx(
                hFile,
                offTable,
                hkl,
                strKL.pstr,
                KbdInputLocale,
                Flags);

    ERRORTRAP(0);
    CLEANUPLPWSTR(strKL);
    ENDCALL(HKL);
}

VOID mouse_event(
    DWORD dwFlags,
    DWORD dx,
    DWORD dy,
    DWORD dwData,
    ULONG_PTR dwExtraInfo)
{
    INPUT ms;

    BEGINCALLVOID()

        ms.type           = INPUT_MOUSE;
        ms.mi.dwFlags     = dwFlags;
        ms.mi.dx          = dx;
        ms.mi.dy          = dy;
        ms.mi.mouseData   = dwData;
        ms.mi.time        = 0;
        ms.mi.dwExtraInfo = dwExtraInfo;

        NtUserSendInput(1, &ms, sizeof(INPUT));

    ENDCALLVOID()
}

VOID keybd_event(
    BYTE  bVk,
    BYTE  bScan,
    DWORD dwFlags,
    ULONG_PTR dwExtraInfo)
{
    INPUT kbd;

    BEGINCALLVOID()

        kbd.type           = INPUT_KEYBOARD;
        kbd.ki.dwFlags     = dwFlags;
        kbd.ki.wVk         = bVk;
        kbd.ki.wScan       = bScan;
        kbd.ki.time        = 0;
        kbd.ki.dwExtraInfo = dwExtraInfo;

        NtUserSendInput(1, &kbd, sizeof(INPUT));

    ENDCALLVOID()
}

/*
 * Message thunks
 */
MESSAGECALL(fnINWPARAMDBCSCHAR)
{
    BEGINCALL()

        /*
         * The server always expects the characters to be unicode so
         * if this was generated from an ANSI routine convert it to Unicode
         */
        if (bAnsi) {

            /*
             * Setup for DBCS Messaging..
             */
            BUILD_DBCS_MESSAGE_TO_SERVER_FROM_CLIENTA(msg,wParam,TRUE);

            /*
             * Convert DBCS/SBCS to Unicode...
             */
            RtlMBMessageWParamCharToWCS(msg, &wParam);
        }

        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnCOPYGLOBALDATA)
{
    PBYTE pData;
    BEGINCALL()

        if (wParam == 0) {
            MSGERROR();
        }

        USERGLOBALLOCK((HGLOBAL)lParam, pData);
        if (pData == NULL) {
            MSGERROR();
        }
        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                (LPARAM)pData,
                xParam,
                xpfnProc,
                bAnsi);
        USERGLOBALUNLOCK((HGLOBAL)lParam);
        UserGlobalFree((HGLOBAL)lParam);
    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnINPAINTCLIPBRD)
{
    LPPAINTSTRUCT lpps;

    BEGINCALL()

        USERGLOBALLOCK((HGLOBAL)lParam, lpps);
        if (lpps) {
            retval = (DWORD)NtUserMessageCall(
                    hwnd,
                    msg,
                    wParam,
                    (LPARAM)lpps,
                    xParam,
                    xpfnProc,
                    bAnsi);
            USERGLOBALUNLOCK((HGLOBAL)lParam);
        } else {
            RIPMSG1(RIP_WARNING, "MESSAGECALL(fnINPAINTCLIPBRD): USERGLOBALLOCK failed on %p!", lParam);
        }

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnINSIZECLIPBRD)
{
    LPRECT lprc;
    BEGINCALL()

        USERGLOBALLOCK((HGLOBAL)lParam, lprc);
        if (lprc) {
            retval = (DWORD)NtUserMessageCall(
                    hwnd,
                    msg,
                    wParam,
                    (LPARAM)lprc,
                    xParam,
                    xpfnProc,
                bAnsi);
            USERGLOBALUNLOCK((HGLOBAL)lParam);
        } else {
            RIPMSG1(RIP_WARNING, "MESSAGECALL(fnINSIZECLIPBRD): USERGLOBALLOCK failed on %p!", lParam);
        }

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnINDEVICECHANGE)
{
    struct _DEV_BROADCAST_HEADER *pHdr;
    PDEV_BROADCAST_PORT_W pPortW = NULL;
    PDEV_BROADCAST_PORT_A pPortA;
    PDEV_BROADCAST_DEVICEINTERFACE_W pInterfaceW = NULL;
    PDEV_BROADCAST_DEVICEINTERFACE_A pInterfaceA;
    PDEV_BROADCAST_HANDLE pHandleW = NULL;
    PDEV_BROADCAST_HANDLE pHandleA;

    LPWSTR lpStr;
    int iStr, iSize;

    BEGINCALL()

        if (!(wParam &0x8000) || !lParam || !bAnsi)
            goto shipit;

        pHdr = (struct _DEV_BROADCAST_HEADER *)lParam;
        switch (pHdr->dbcd_devicetype) {
        case DBT_DEVTYP_PORT:
            pPortA = (PDEV_BROADCAST_PORT_A)lParam;
            iStr = strlen(pPortA->dbcp_name);
            iSize = FIELD_OFFSET(DEV_BROADCAST_PORT_W, dbcp_name) + sizeof(WCHAR)*(iStr+1);
            pPortW = UserLocalAlloc(0, iSize);
            if (pPortW == NULL)
                return 0;
            RtlCopyMemory(pPortW, pPortA, sizeof(DEV_BROADCAST_PORT_A));
            lpStr = pPortW->dbcp_name;
            if (iStr) {
                MBToWCS(pPortA->dbcp_name, -1, &lpStr, iStr, FALSE);
                lpStr[iStr] = 0;
            } else {
                lpStr[0] = 0;
            }
            pPortW->dbcp_size = iSize;
            lParam = (LPARAM)pPortW;
            bAnsi = FALSE;
            break;

        case DBT_DEVTYP_DEVICEINTERFACE:
            pInterfaceA = (PDEV_BROADCAST_DEVICEINTERFACE_A)lParam;
            iStr = strlen(pInterfaceA->dbcc_name);
            iSize = FIELD_OFFSET(DEV_BROADCAST_DEVICEINTERFACE_W, dbcc_name) + sizeof(WCHAR)*(iStr+1);
            pInterfaceW = UserLocalAlloc(0, iSize);
            if (pInterfaceW == NULL)
                return 0;
            RtlCopyMemory(pInterfaceW, pInterfaceA, sizeof(DEV_BROADCAST_DEVICEINTERFACE_A));
            lpStr = pInterfaceW->dbcc_name;
            if (iStr) {
                MBToWCS(pInterfaceA->dbcc_name, -1, &lpStr, iStr, FALSE);
                lpStr[iStr] = 0;
            } else {
                lpStr[0] = 0;
            }
            pInterfaceW->dbcc_size = iSize;
            lParam = (LPARAM)pInterfaceW;
            bAnsi = FALSE;
            break;

        case DBT_DEVTYP_HANDLE:
            pHandleA = (PDEV_BROADCAST_HANDLE)lParam;
            bAnsi = FALSE;
            if ((wParam != DBT_CUSTOMEVENT) || (pHandleA->dbch_nameoffset < 0)) break;
            iStr = strlen(pHandleA->dbch_data+pHandleA->dbch_nameoffset);
        /*
         * Calculate size of new structure with UNICODE string instead of Ansi string
         */

            iSize = FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data)+ pHandleA->dbch_nameoffset + sizeof(WCHAR)*(iStr+1);
            /*
             * Just in case there were an odd number of bytes in the non-text data
             */
            if (iSize & 1) iSize++;
            pHandleW = UserLocalAlloc(0, iSize);
            if (pHandleW == NULL)
                return 0;
            RtlCopyMemory(pHandleW, pHandleA, FIELD_OFFSET(DEV_BROADCAST_HANDLE, dbch_data)+ pHandleA->dbch_nameoffset);

            /*
             * Make sure this is even for the UNICODE string.
             */

            if (pHandleW->dbch_nameoffset & 1) pHandleW->dbch_nameoffset++;

            lpStr = (LPWSTR)(pHandleW->dbch_data+pHandleW->dbch_nameoffset);
            if (iStr) {
                MBToWCS(pHandleA->dbch_data+pHandleA->dbch_nameoffset, -1,
                        &lpStr, iStr, FALSE);
            }
                lpStr[iStr] = 0;
            pHandleW->dbch_size = iSize;
            lParam = (LPARAM)pHandleW;

            break;
        }

shipit:
        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);

    if (pPortW) UserLocalFree(pPortW);
    if (pInterfaceW) UserLocalFree(pInterfaceW);
    if (pHandleW) UserLocalFree(pHandleW);

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnIMECONTROL)
{
    PVOID pvData = NULL;
    LPARAM lData = lParam;

    BEGINCALL()

        /*
         * The server always expects the characters to be unicode so
         * if this was generated from an ANSI routine convert it to Unicode
         */
        if (bAnsi) {
            switch (wParam) {
                case IMC_GETCOMPOSITIONFONT:
                case IMC_GETSOFTKBDFONT:
                case IMC_SETCOMPOSITIONFONT:
                    pvData = UserLocalAlloc(0, sizeof(LOGFONTW));
                    if (pvData == NULL)
                        MSGERROR();

                    if (wParam == IMC_SETCOMPOSITIONFONT) {
                        // Later, we do A/W conversion based on thread hkl/CP.
                        CopyLogFontAtoW((PLOGFONTW)pvData, (PLOGFONTA)lParam);
                    }

                    lData = (LPARAM)pvData;
                    break;

                case IMC_SETSOFTKBDDATA:
                    {
                        PSOFTKBDDATA pSoftKbdData;
                        PWORD pCodeA;
                        PWSTR pCodeW;
                        CHAR  ch[3];
                        DWORD cbSize;
                        UINT  uCount, i;

                        uCount = ((PSOFTKBDDATA)lParam)->uCount;

                        cbSize = FIELD_OFFSET(SOFTKBDDATA, wCode[0])
                               + uCount * sizeof(WORD) * 256;

                        pvData = UserLocalAlloc(0, cbSize);
                        if (pvData == NULL)
                            MSGERROR();

                        pSoftKbdData = (PSOFTKBDDATA)pvData;

                        pSoftKbdData->uCount = uCount;

                        ch[2] = (CHAR)'\0';

                        pCodeA = &((PSOFTKBDDATA)lParam)->wCode[0][0];
                        pCodeW = &pSoftKbdData->wCode[0][0];

                        i = uCount * 256;

                        while (i--) {
                            if (HIBYTE(*pCodeA)) {
                                ch[0] = (CHAR)HIBYTE(*pCodeA);
                                ch[1] = (CHAR)LOBYTE(*pCodeA);
                            } else {
                                ch[0] = (CHAR)LOBYTE(*pCodeA);
                                ch[1] = (CHAR)'\0';
                            }
                            MBToWCSEx(THREAD_CODEPAGE(), (LPSTR)&ch, -1, &pCodeW, 1, FALSE);
                            pCodeA++; pCodeW++;
                        }

                        lData = (LPARAM)pvData;
                    }
                    break;

                default:
                    break;
            }
        }

        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lData,
                xParam,
                xpfnProc,
                bAnsi);

        if (bAnsi) {
            switch (wParam) {
                case IMC_GETCOMPOSITIONFONT:
                case IMC_GETSOFTKBDFONT:
                    CopyLogFontWtoA((PLOGFONTA)lParam, (PLOGFONTW)pvData);
                    break;

                default:
                    break;
            }
        }

        if (pvData != NULL)
            UserLocalFree(pvData);

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

#ifdef LATER
DWORD CalcCharacterPositionAtoW(
    DWORD dwCharPosA,
    LPSTR lpszCharStr,
    DWORD dwCodePage)
{
    DWORD dwCharPosW = 0;

    while (dwCharPosA != 0) {
        if (IsDBCSLeadByteEx(dwCodePage, *lpszCharStr)) {
            if (dwCharPosA >= 2) {
                dwCharPosA -= 2;
            }
            else {
                dwCharPosA--;
            }
            lpszCharStr += 2;
        }
        else {
            dwCharPosA--;
            lpszCharStr++;
        }
        dwCharPosW++;
    }

    return dwCharPosW;
}

int UnicodeToMultiByteSize(DWORD dwCodePage, LPCWSTR pwstr)
{
    char dummy[2], *lpszDummy = dummy;
    return WCSToMBEx((WORD)dwCodePage, pwstr, 1, &lpszDummy, sizeof(WCHAR), FALSE);
}

DWORD CalcCharacterPositionWtoA(
    DWORD dwCharPosW,
    LPWSTR lpwszCharStr,
    DWORD  dwCodePage)
{
    DWORD dwCharPosA = 0;
    ULONG MultiByteSize;

    while (dwCharPosW != 0) {
        MultiByteSize = UnicodeToMultiByteSize(dwCodePage, lpwszCharStr);
        if (MultiByteSize == 2) {
            dwCharPosA += 2;
        }
        else {
            dwCharPosA++;
        }
        dwCharPosW--;
        lpwszCharStr++;
    }

    return dwCharPosA;
}

DWORD WINAPI ImmGetReconvertTotalSize(DWORD dwSize, REQ_CALLER eCaller, BOOL bAnsiTarget)
{
    if (dwSize < sizeof(RECONVERTSTRING)) {
        return 0;
    }
    if (bAnsiTarget) {
        dwSize -= sizeof(RECONVERTSTRING);
        if (eCaller == FROM_IME) {
            dwSize /= 2;
        } else {
            dwSize *= 2;
        }
        dwSize += sizeof(RECONVERTSTRING);
    }
    return dwSize;
}

DWORD WINAPI ImmReconversionWorker(
        LPRECONVERTSTRING lpRecTo,
        LPRECONVERTSTRING lpRecFrom,
        BOOL bToAnsi,
        DWORD dwCodePage)
{
    INT i;
    DWORD dwSize = 0;

    UserAssert(lpRecTo);
    UserAssert(lpRecFrom);

    if (lpRecFrom->dwVersion != 0 || lpRecTo->dwVersion != 0) {
        RIPMSG0(RIP_WARNING, "ImmReconversionWorker: dwVersion in lpRecTo or lpRecFrom is incorrect.");
        return 0;
    }
    // Note:
    // In any IME related structures, use the following principal.
    // 1) xxxStrOffset is an actual offset, i.e. byte count.
    // 2) xxxStrLen is a number of characters, i.e. TCHAR count.
    //
    // CalcCharacterPositionXtoY() takes TCHAR count so that we
    // need to adjust xxxStrOffset if it's being converted. But you
    // should be careful, because the actual position of the string
    // is always at something like (LPBYTE)lpStruc + lpStruc->dwStrOffset.
    //
    if (bToAnsi) {
        // Convert W to A
        lpRecTo->dwStrOffset = sizeof *lpRecTo;
        i = WideCharToMultiByte(dwCodePage,
                                (DWORD)0,
                                (LPWSTR)((LPSTR)lpRecFrom + lpRecFrom->dwStrOffset), // src
                                (INT)lpRecFrom->dwStrLen,
                                (LPSTR)lpRecTo + lpRecTo->dwStrOffset,  // dest
                                (INT)lpRecFrom->dwStrLen * DBCS_CHARSIZE,
                                (LPSTR)NULL,
                                (LPBOOL)NULL);
        lpRecTo->dwCompStrOffset =
            CalcCharacterPositionWtoA(lpRecFrom->dwCompStrOffset / sizeof(WCHAR),
                                      (LPWSTR)((LPBYTE)lpRecFrom + lpRecFrom->dwStrOffset),
                                      dwCodePage)
                            * sizeof(CHAR);

        lpRecTo->dwCompStrLen =
            (CalcCharacterPositionWtoA(lpRecFrom->dwCompStrOffset / sizeof(WCHAR) +
                                      lpRecFrom->dwCompStrLen,
                                      (LPWSTR)((LPBYTE)lpRecFrom + lpRecFrom->dwStrOffset),
                                      dwCodePage)
                            * sizeof(CHAR))
            - lpRecTo->dwCompStrOffset;

        lpRecTo->dwTargetStrOffset =
            CalcCharacterPositionWtoA(lpRecFrom->dwTargetStrOffset / sizeof(WCHAR),
                                      (LPWSTR)((LPBYTE)lpRecFrom +
                                                lpRecFrom->dwStrOffset),
                                      dwCodePage)
                            * sizeof(CHAR);

        lpRecTo->dwTargetStrLen =
            (CalcCharacterPositionWtoA(lpRecFrom->dwTargetStrOffset / sizeof(WCHAR) +
                                      lpRecFrom->dwTargetStrLen,
                                      (LPWSTR)((LPBYTE)lpRecFrom + lpRecFrom->dwStrOffset),
                                       dwCodePage)
                            * sizeof(CHAR))
            - lpRecTo->dwTargetStrOffset;

        ((LPSTR)lpRecTo)[lpRecTo->dwStrOffset + i] = '\0';
        lpRecTo->dwStrLen = i * sizeof(CHAR);

        dwSize = sizeof(RECONVERTSTRING) + ((i + 1) * sizeof(CHAR));

    } else {

        // AtoW
        lpRecTo->dwStrOffset = sizeof *lpRecTo;
        i = MultiByteToWideChar(dwCodePage,
                                (DWORD)MB_PRECOMPOSED,
                                (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,  // src
                                (INT)lpRecFrom->dwStrLen,
                                (LPWSTR)((LPSTR)lpRecTo + lpRecTo->dwStrOffset), // dest
                                (INT)lpRecFrom->dwStrLen);

        lpRecTo->dwCompStrOffset =
            CalcCharacterPositionAtoW(lpRecFrom->dwCompStrOffset,
                                      (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                      dwCodePage) * sizeof(WCHAR);

        lpRecTo->dwCompStrLen =
            ((CalcCharacterPositionAtoW(lpRecFrom->dwCompStrOffset +
                                       lpRecFrom->dwCompStrLen,
                                       (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                        dwCodePage)  * sizeof(WCHAR))
            - lpRecTo->dwCompStrOffset) / sizeof(WCHAR);

        lpRecTo->dwTargetStrOffset =
            CalcCharacterPositionAtoW(lpRecFrom->dwTargetStrOffset,
                                      (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                      dwCodePage) * sizeof(WCHAR);

        lpRecTo->dwTargetStrLen =
            ((CalcCharacterPositionAtoW(lpRecFrom->dwTargetStrOffset +
                                       lpRecFrom->dwTargetStrLen,
                                       (LPSTR)lpRecFrom + lpRecFrom->dwStrOffset,
                                       dwCodePage)  * sizeof(WCHAR))
            - lpRecTo->dwTargetStrOffset) / sizeof(WCHAR);

        lpRecTo->dwStrLen = i;  // Length is TCHAR count.
        if (lpRecTo->dwSize >= (DWORD)(lpRecTo->dwStrOffset + (i + 1)* sizeof(WCHAR))) {
            LPWSTR lpW = (LPWSTR)((LPSTR)lpRecTo + lpRecTo->dwStrOffset);
            lpW[i] = L'\0';
        }
        dwSize = sizeof(RECONVERTSTRING) + ((i + 1) * sizeof(WCHAR));
    }
    return dwSize;
}

#define GETCOMPOSITIONSTRING(hImc, index, buf, buflen) \
            (bAnsi ? fpImmGetCompositionStringA : fpImmGetCompositionStringW)((hImc), (index), (buf), (buflen))

MESSAGECALL(fnIMEREQUEST)
{
    PVOID pvData = NULL;
    LPARAM lData = lParam;

    BEGINCALL()

        if (!IS_IME_ENABLED()) {
            // If IME is not enabled, save time.
            MSGERROR();
        }

        /*
         * The server always expects the characters to be unicode so
         * if this was generated from an ANSI routine convert it to Unicode
         */
        if (wParam == IMR_QUERYCHARPOSITION) {
            //
            // Store the UNICODE character count in PrivateIMECHARPOSITION.
            //
            // No need to save the original dwCharPos, since dwCharPositionA/W are not
            // overwritten in the kernel.
            //
            if (bAnsi) {
                ((LPIMECHARPOSITION)lParam)->dwCharPos = ((LPPrivateIMECHARPOSITION)lParam)->dwCharPositionW;
            }
        }
        else if (bAnsi) {
            switch (wParam) {
            case IMR_COMPOSITIONFONT:
                pvData = UserLocalAlloc(0, sizeof(LOGFONTW));
                if (pvData == NULL)
                    MSGERROR();
                lData = (LPARAM)pvData;
                break;

            case IMR_CONFIRMRECONVERTSTRING:
            case IMR_RECONVERTSTRING:
            case IMR_DOCUMENTFEED:
                if ((LPVOID)lParam != NULL) {
                    // IME wants not only the buffer size but the real reconversion information
                    DWORD dwSize = ImmGetReconvertTotalSize(((LPRECONVERTSTRING)lParam)->dwSize, FROM_IME, FALSE);
                    LPRECONVERTSTRING lpReconv;

                    pvData = UserLocalAlloc(0, dwSize + sizeof(WCHAR));
                    if (pvData == NULL) {
                        RIPMSG0(RIP_WARNING, "fnIMEREQUEST: failed to allocate a buffer for reconversion.");
                        MSGERROR();
                    }
                    lpReconv = (LPRECONVERTSTRING)pvData;
                    // setup the information in the allocated structure
                    lpReconv->dwVersion = 0;
                    lpReconv->dwSize = dwSize;

                    //
                    // if it's confirmation message, we need to translate the contents
                    //
                    if (wParam == IMR_CONFIRMRECONVERTSTRING) {
                        ImmReconversionWorker(lpReconv, (LPRECONVERTSTRING)lParam, FALSE, CP_ACP);
                    }
                }
                break;

            default:
                break;
            }
        }

        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lData,
                xParam,
                xpfnProc,
                bAnsi);

        if (bAnsi) {
            switch (wParam) {
            case IMR_COMPOSITIONFONT:
                if (retval) {
                    CopyLogFontWtoA((PLOGFONTA)lParam, (PLOGFONTW)pvData);
                }
                break;

            case IMR_QUERYCHARPOSITION:
                ((LPIMECHARPOSITION)lParam)->dwCharPos = ((LPPrivateIMECHARPOSITION)lParam)->dwCharPositionA;
                break;

            case IMR_RECONVERTSTRING:
            case IMR_DOCUMENTFEED:
                //
                // Note: by definition, we don't need back-conversion for IMR_CONFIRMRECONVERTSTRING
                //
                if (retval) {
                    // IME wants the buffer size
                    retval = ImmGetReconvertTotalSize((DWORD)retval, FROM_APP, FALSE);
                    if (retval < sizeof(RECONVERTSTRING)) {
                        RIPMSG2(RIP_WARNING, "WM_IME_REQUEST(%x): return value from application %d is invalid.", wParam, retval);
                        retval = 0;
                    } else if (lParam) {
                        // We need to perform the A/W conversion of the contents
                        if (!ImmReconversionWorker((LPRECONVERTSTRING)lParam, (LPRECONVERTSTRING)pvData, TRUE, CP_ACP)) {
                            MSGERROR();
                        }
                    }
                }
                break;
            }
        }


    ERRORTRAP(0);

    if (pvData != NULL)
        UserLocalFree(pvData);

    ENDCALL(DWORD);
}
#endif

MESSAGECALL(fnEMGETSEL)
{
    PWND pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    BEGINCALL()

        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);

        //
        // temp for our beta...
        //
        // !!! THIS CODE SHOULD BE IN KERNEL MODE !!!
        //
        // to reduce user <-> kernel mode transition...
        //
        if (bAnsi != ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE)) {
            ULONG  cchTextLength;
            LONG   lOriginalLengthW;
            LONG   lOriginalLengthL;
            LONG   wParamLocal;
            LONG   lParamLocal;

            if (wParam) {
                lOriginalLengthW = *(LONG *)wParam;
            } else {
                lOriginalLengthW = (LONG)(LOWORD(retval));
            }

            if (lParam) {
                lOriginalLengthL = *(LONG *)lParam;
            } else {
                lOriginalLengthL = (LONG)(HIWORD(retval));
            }

            cchTextLength = (DWORD)NtUserMessageCall(
                           hwnd,
                           WM_GETTEXTLENGTH,
                           (WPARAM)0,
                           (LPARAM)0,
                           xParam,
                           xpfnProc,
                           bAnsi);

            if (cchTextLength) {
                PVOID pvString;
                ULONG cbTextLength;

                cchTextLength++;
                if (!bAnsi) {
                    cbTextLength = cchTextLength * sizeof(WCHAR);
                } else {
                    cbTextLength = cchTextLength;
                }

                pvString = UserLocalAlloc(0,cbTextLength);

                if (pvString) {

                    retval = (DWORD)NtUserMessageCall(
                            hwnd,
                            WM_GETTEXT,
                            cchTextLength,
                            (LPARAM)pvString,
                            xParam,
                            xpfnProc,
                            bAnsi);

                    if (retval) {
                        if (bAnsi) {
                            /*
                             * ansiString/unicodeLenght -> ansiLength
                             */
                            CalcAnsiStringLengthA(pvString, lOriginalLengthW, &wParamLocal)
                            CalcAnsiStringLengthA(pvString, lOriginalLengthL, &lParamLocal);
                        } else {
                            /*
                             * unicodeString/ansiLenght -> unicodeLength
                             */
                            CalcUnicodeStringLengthW(pvString, lOriginalLengthW, &wParamLocal);
                            CalcUnicodeStringLengthW(pvString, lOriginalLengthL, &lParamLocal);
                        }

                        retval = (DWORD)(((lParamLocal) << 16) | ((wParamLocal) & 0x0000FFFF));

                        if (wParam) {
                            *(LONG *)wParam = wParamLocal;
                        }

                        if (lParam) {
                            *(LONG *)lParam = lParamLocal;
                        }

                    } else {
                        UserLocalFree(pvString);
                        MSGERROR();
                    }

                    UserLocalFree(pvString);

                } else
                    MSGERROR();
            } else
                MSGERROR();
        }

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnEMSETSEL)
{
    PWND pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    BEGINCALL()

        //
        // temp for our beta...
        //
        // !!! THIS CODE SHOULD BE IN KERNEL MODE !!!
        //
        // to reduce user <-> kernel mode transition...
        //
        if (bAnsi != ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE)) {
            if (((LONG)wParam <= 0) && ((LONG)lParam <=0)) {
                //
                // if (wParam == 0 or wParam == -1)
                //               and
                //    (lParam == 0 or lParam == -1)
                //
                // In this case, we don't need to convert the value...
                //
            } else {
                ULONG  cchTextLength;
                LONG   lOriginalLengthW = (LONG)wParam;
                LONG   lOriginalLengthL = (LONG)lParam;

                cchTextLength = (DWORD)NtUserMessageCall(
                               hwnd,
                               WM_GETTEXTLENGTH,
                               (WPARAM)0,
                               (LPARAM)0,
                               xParam,
                               xpfnProc,
                               bAnsi);

                if (cchTextLength) {
                    PVOID pvString;
                    ULONG cbTextLength;

                    cchTextLength++;
                    if (!bAnsi) {
                        cbTextLength = cchTextLength * sizeof(WCHAR);
                    } else {
                        cbTextLength = cchTextLength;
                    }

                    pvString = UserLocalAlloc(0,cbTextLength);

                    if (pvString) {

                        retval = (DWORD)NtUserMessageCall(
                                hwnd,
                                WM_GETTEXT,
                                cchTextLength,
                                (LPARAM)pvString,
                                xParam,
                                xpfnProc,
                                bAnsi);

                        if (retval) {
                            if ((LONG)retval < lOriginalLengthW) {
                                lOriginalLengthW = (LONG)retval;
                            }
                            if ((LONG)retval < lOriginalLengthL) {
                                lOriginalLengthL = (LONG)retval;
                            }
                            if (bAnsi) {
                                if (lOriginalLengthW > 0) {
                                    CalcUnicodeStringLengthA(pvString, lOriginalLengthW, &wParam);
                                }
                                if(lOriginalLengthL > 0) {
                                    CalcUnicodeStringLengthA(pvString, lOriginalLengthL, &lParam);
                                }
                            } else {
                                if (lOriginalLengthW > 0) {
                                    CalcAnsiStringLengthW(pvString, lOriginalLengthW, &wParam);
                                }
                                if(lOriginalLengthL > 0) {
                                    CalcAnsiStringLengthW(pvString, lOriginalLengthL, &lParam);
                                }
                            }
                        } else {
                            UserLocalFree(pvString);
                            MSGERROR();
                        }

                        UserLocalFree(pvString);

                    } else
                        MSGERROR();
                } else
                    MSGERROR();
            }
        }

        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

MESSAGECALL(fnCBGETEDITSEL)
{
    PWND pwnd = ValidateHwnd(hwnd);

    if (pwnd == NULL)
        return 0;

    BEGINCALL()

        retval = (DWORD)NtUserMessageCall(
                hwnd,
                msg,
                wParam,
                lParam,
                xParam,
                xpfnProc,
                bAnsi);

        //
        // temp for our beta...
        //
        // !!! THIS CODE SHOULD BE IN KERNEL MODE !!!
        //
        // to reduce user <-> kernel mode transition...
        //
        if (bAnsi != ((TestWF(pwnd, WFANSIPROC)) ? TRUE : FALSE)) {
            ULONG  cchTextLength;
            LONG   lOriginalLengthW = *(LONG *)wParam;
            LONG   lOriginalLengthL = *(LONG *)lParam;
            LONG   wParamLocal;
            LONG   lParamLocal;

            if (wParam) {
                lOriginalLengthW = *(LONG *)wParam;
            } else {
                lOriginalLengthW = (LONG)(LOWORD(retval));
            }

            if (lParam) {
                lOriginalLengthL = *(LONG *)lParam;
            } else {
                lOriginalLengthL = (LONG)(HIWORD(retval));
            }

            cchTextLength = (DWORD)NtUserMessageCall(
                           hwnd,
                           WM_GETTEXTLENGTH,
                           (WPARAM)0,
                           (LPARAM)0,
                           xParam,
                           xpfnProc,
                           bAnsi);

            if (cchTextLength) {
                PVOID pvString;
                ULONG cbTextLength;

                cchTextLength++;
                if (!bAnsi) {
                    cbTextLength = cchTextLength * sizeof(WCHAR);
                } else {
                    cbTextLength = cchTextLength;
                }

                pvString = UserLocalAlloc(0,cbTextLength);

                if (pvString) {

                    retval = (DWORD)NtUserMessageCall(
                            hwnd,
                            WM_GETTEXT,
                            cchTextLength,
                            (LPARAM)pvString,
                            xParam,
                            xpfnProc,
                            bAnsi);

                    if (retval) {
                        if (bAnsi) {
                            /*
                             * ansiString/unicodeLenght -> ansiLength
                             */
                            CalcAnsiStringLengthA(pvString, lOriginalLengthW, &wParamLocal);
                            CalcAnsiStringLengthA(pvString, lOriginalLengthL, &lParamLocal);
                        } else {
                            /*
                             * unicodeString/ansiLenght -> unicodeLength
                             */
                            CalcUnicodeStringLengthW(pvString, lOriginalLengthW, &wParamLocal);
                            CalcUnicodeStringLengthW(pvString, lOriginalLengthL, &lParamLocal);
                        }

                        retval = (DWORD)(((lParamLocal) << 16) | ((wParamLocal) & 0x0000FFFF));

                        if (wParam) {
                            *(LONG *)wParam = wParamLocal;
                        }

                        if (lParam) {
                            *(LONG *)lParam = lParamLocal;
                        }

                    } else {
                        UserLocalFree(pvString);
                        MSGERROR();
                    }

                    UserLocalFree(pvString);

                } else
                    MSGERROR();
            } else
                MSGERROR();
        }

    ERRORTRAP(0);
    ENDCALL(DWORD);
}

LONG BroadcastSystemMessageWorker(
    DWORD dwFlags,
    LPDWORD lpdwRecipients,
    UINT message,
    WPARAM wParam,
    LPARAM lParam,
    BOOL fAnsi)
{
    DWORD  dwRecipients;

    /*
     * Prevent apps from setting hi 16 bits so we can use them internally.
     */
    if (message & RESERVED_MSG_BITS) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid message (%x) for BroadcastSystemMessage\n", message);
        return(0);
    }

    if (dwFlags & ~BSF_VALID) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid dwFlags (%x) for BroadcastSystemMessage\n", dwFlags);
        return(0);
    }

    //
    // Check if the message number is in the private message range.
    // If so, do not send it to Win4.0 windows.
    // (This is required because apps like SimCity broadcast a message
    // that has the value 0x500 and that confuses MsgSrvr's
    // MSGSRVR_NOTIFY handler.
    //
    if ((message >= WM_USER) && (message < 0xC000))
    {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid message (%x) for BroadcastSystemMessage\n", message);
        return(0L);
    }

    if (dwFlags & BSF_FORCEIFHUNG)
        dwFlags |= BSF_NOHANG;

    //
    // If BSF_QUERY or message has a pointer, it can not be posted.
    //
    if (dwFlags & BSF_QUERY)
    {
#if DBG
        if (dwFlags & BSF_ASYNC)
        {
            RIPMSG0(RIP_ERROR, "BroadcastSystemMessage: Can't post queries\n");
        }
#endif

        dwFlags &= ~BSF_ASYNC;          // Strip the BSF_ASYNC flags.
    }

    if (dwFlags & BSF_ASYNC) {
        if (TESTSYNCONLYMESSAGE(message, wParam)) {
            RIPERR0(ERROR_MESSAGE_SYNC_ONLY, RIP_WARNING, "BroadcastSystemMessage: Can't post messages with pointers\n");
            dwFlags &= ~BSF_ASYNC;          // Strip the BSF_ASYNC flags.
        }
    }


    // Let us find out who the intended recipients are.
    if (lpdwRecipients != NULL)
        dwRecipients = *lpdwRecipients;
    else
        dwRecipients = BSM_ALLCOMPONENTS;

    // if they want all components, add the corresponding bits
    if ((dwRecipients & BSM_COMPONENTS) == BSM_ALLCOMPONENTS)
        dwRecipients |= (BSM_VXDS | BSM_NETDRIVER | BSM_INSTALLABLEDRIVERS |
                             BSM_APPLICATIONS);


    if (dwRecipients & ~BSM_VALID) {
        RIPERR1(ERROR_INVALID_PARAMETER, RIP_WARNING, "invalid dwRecipients (%x) for BroadcastSystemMessage\n", dwRecipients);
        return(0);
    }

    //
    // Check if this is a WM_USERCHANGED message; If so, we want to reload
    // the per-user settings before anyone else sees this message.
    //
    // LATER -- FritzS
//    if (uiMessage == WM_USERCHANGED)
//        ReloadPerUserSettings();


    // Does this need to be sent to all apps?
    if (dwRecipients & BSM_APPLICATIONS)
    {
        BROADCASTSYSTEMMSGPARAMS bsmParams;

        bsmParams.dwFlags = dwFlags;
        bsmParams.dwRecipients = dwRecipients;

        return (LONG)CsSendMessage(GetDesktopWindow(), message, wParam, lParam,
            (ULONG_PTR)&bsmParams, FNID_SENDMESSAGEBSM, fAnsi);
    }

    return -1;
}

HDEVNOTIFY
RegisterDeviceNotificationWorker(
    IN HANDLE hRecipient,
    IN LPVOID NotificationFilter,
    IN DWORD Flags,
    IN BOOL IsAnsi
    )
{
    HINSTANCE   hLib = NULL;
    FARPROC     fpRegisterNotification = NULL;
    PVOID       Context = NULL;
    HDEVNOTIFY  notifyHandle = NULL;
    CONFIGRET   Status = CR_SUCCESS;

    extern
    CONFIGRET
    CMP_RegisterNotification(IN  HANDLE   hRecipient,
                             IN  LPBYTE   NotificationFilter,
                             IN  DWORD    Flags,
                             OUT PVOID   *Context
                             );

    UNREFERENCED_PARAMETER(IsAnsi);

    try {
        //
        // load the config manager client dll and retrieve entry pts
        //
        hLib = LoadLibrary(TEXT("SETUPAPI.DLL"));
        if (hLib == NULL) {
            goto Clean0;  // use last error set by LoadLibrary
        }

        fpRegisterNotification = GetProcAddress(hLib, "CMP_RegisterNotification");
        if (fpRegisterNotification == NULL) {
            goto Clean0;    // use last error set by GetProcAddress
        }

        Status = (CONFIGRET)(fpRegisterNotification)(hRecipient,
                                        NotificationFilter,
                                        Flags,
                                        &Context);

        Clean0:
            ;

    } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
    }

    if (hLib != NULL) {
        FreeLibrary(hLib);
    }

    if (Status != CR_SUCCESS) {

        //
        //Something went wrong, map the CR errors to a
        //W32 style error code
        //
        switch (Status) {
            case CR_INVALID_POINTER:
                SetLastError (ERROR_INVALID_PARAMETER);
                break;
            case CR_INVALID_DATA:
                SetLastError (ERROR_INVALID_DATA);
                break;
            case CR_OUT_OF_MEMORY:
                SetLastError (ERROR_NOT_ENOUGH_MEMORY);
                break;
            case CR_FAILURE:
            default:
                SetLastError (ERROR_SERVICE_SPECIFIC_ERROR);
                break;

        }
    }

    if ((Context != NULL) && ((ULONG_PTR)Context != -1)) {
        notifyHandle = (HDEVNOTIFY)Context;
    }

    return notifyHandle;
}


BOOL
UnregisterDeviceNotification(
    IN HDEVNOTIFY Handle
    )
{
    BOOL        status = TRUE;
    HINSTANCE   hLib = NULL;
    FARPROC     fpUnregisterNotification = NULL;
    CONFIGRET   crStatus = CR_SUCCESS;

    extern
    CONFIGRET
    CMP_UnregisterNotification(IN ULONG Context);

    try {
        //
        // load the config manager client dll and retrieve entry pts
        //
        hLib = LoadLibrary(TEXT("SETUPAPI.DLL"));
        if (hLib == NULL) {
            status = FALSE;
            goto Clean0;  // use last error set by LoadLibrary
        }

        fpUnregisterNotification = GetProcAddress(hLib, "CMP_UnregisterNotification");
        if (fpUnregisterNotification == NULL) {
            status = FALSE;
            goto Clean0;    // use last error set by GetProcAddress
        }

        crStatus = (CONFIGRET)(fpUnregisterNotification)((ULONG_PTR)Handle);

        if (crStatus != CR_SUCCESS) {
            status = FALSE;
            //
            //Something went wrong, map the CR errors to a
            //W32 style error code
            //
            switch (crStatus) {
                case CR_INVALID_POINTER:
                    SetLastError (ERROR_INVALID_PARAMETER);
                    break;
                case CR_INVALID_DATA:
                    SetLastError (ERROR_INVALID_DATA);
                    break;
                case CR_FAILURE:
                default:
                    SetLastError (ERROR_SERVICE_SPECIFIC_ERROR);
                    break;
            }
        }

        Clean0:
            ;

    } except(W32ExceptionHandler(FALSE, RIP_WARNING)) {
        status = FALSE;
    }

    if (hLib != NULL) {
        FreeLibrary(hLib);
    }

    return status;
}
