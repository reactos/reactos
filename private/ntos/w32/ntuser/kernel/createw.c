/****************************** Module Header ******************************\
* Module Name: createw.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* Contains xxxCreateWindow, xxxDestroyWindow and a few close friends.
*
* Note that during creation or deletion, the window is locked so that
*   it can't be deleted recursively
*
* History:
* 19-Oct-1990 DarrinM   Created.
* 11-Feb-1991 JimA      Added access checks.
* 19-Feb-1991 MikeKe    Added Revalidation code
* 20-Jan-1992 IanJa     ANSI/UNICODE neutralization
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

BOOL WantImeWindow(IN PWND pwndParent, IN PWND pwnd);

/***************************************************************************\
* xxxCreateWindowEx (API)
*
* History:
* 10-18-90 darrinm      Ported from Win 3.0 sources.
* 02-07-91 DavidPe      Added Win 3.1 WH_CBT support.
* 02-11-91 JimA         Added access checks.
* 04-11-92 ChandanC     Added initialization of WOW words
\***************************************************************************/

PWND xxxCreateWindowEx(
    DWORD         dwExStyle,
    PLARGE_STRING cczpstrClass,
    PLARGE_STRING cczpstrName,
    DWORD         style,
    int           x,
    int           y,
    int           cx,
    int           cy,
    PWND          pwndParent,
    PMENU         pMenu,
    HANDLE        hInstance,
    LPVOID        lpCreateParams,
    DWORD         dwExpWinVerAndFlags)
{
    /*
     * The buffers for Class and Name may be client memory, and access
     * to those buffers must be protected.
     */
    UINT           mask = 0;
    BOOL           fChild;
    BOOL           fDefPos = FALSE;
    BOOL           fStartup = FALSE;
    PCLS           pcls;
    PPCLS          ppcls;
    RECT           rc;
    int            dx, dy;
    SIZERECT       src;
    int            sw = SW_SHOW;
    PWND           pwnd;
    PWND           pwndZOrder, pwndHardError;
    CREATESTRUCTEX csex;
    PDESKTOP       pdesk;
    ATOM           atomT;
    PTHREADINFO    ptiCurrent;
    TL             tlpwnd;
    TL             tlpwndParent;
    TL             tlpwndParentT;
    BOOL           fLockParent = FALSE;
    WORD           wWFAnsiCreator = 0;
    DWORD          dw;
    DWORD          dwMinMax;
    PMONITOR       pMonitor;
    BOOL           fTiled;
#ifdef USE_MIRRORING
    DWORD dwLayout;
#endif

    CheckLock(pwndParent);
    UserAssert(IsWinEventNotifyDeferredOK());

    /*
     * For Edit Controls (including those in comboboxes), we must know whether
     * the App used an ANSI or a Unicode CreateWindow call.  This is passed in
     * with the private WS_EX_ANSICREATOR dwExStyle bit, but we MUST NOT leave
     * out this bit in the window's dwExStyle! Transfer to the internal window
     * flag WFANSICREATOR immediately.
     */
    if (dwExStyle & WS_EX_ANSICREATOR) {
        wWFAnsiCreator = WFANSICREATOR;
        dwExStyle &= ~WS_EX_ANSICREATOR;
    }

    ptiCurrent = PtiCurrent();
    /*
     * If this thread has already been in xxxDestroyThreadInfo, then this window
     *  is probably going to end up with a bogus pti.
     */
    UserAssert(!(ptiCurrent->TIF_flags & TIF_INCLEANUP));
    pdesk = ptiCurrent->rpdesk;

    /*
     * If a parent window is specified, make sure it's on the
     * same desktop.
     */
    if (pwndParent != NULL && pwndParent->head.rpdesk != pdesk) {
        RIPERR0(ERROR_INVALID_PARAMETER, RIP_VERBOSE, "");
        return NULL;
    }

    /*
     * Set a flag to indicate whther a it is a child window
     */
    fChild = ((HIWORD(style) & MaskWF(WFTYPEMASK)) == MaskWF(WFCHILD));

#ifdef USE_MIRRORING
    /*


    The WS_EX_LAYOUT_RTL flag is set if,

    1- WS_EX_LAYOUT_RTL set in dwExStyle parameter of the CreateWindow call.
    2- If the window is created from DialogBox class, then it can't inherit from its parent
       and it has to specify WS_EX_LAYOUTRTL explicitly to enable mirroring on it.
    3- If the window is an owned window then the window is left to right layout and the algorithm terminates.
        (An owned window is one created with an HWND passed in the hWndParent paremeter to CreateWindow(Ex),
        but without the WS_CHILD flag present in it's styles.
    4- If the window is a child window, and it's parent is right to left layout,
        and it's parent does not have the WS_EX_NOINHERIT_LAYOUT flag set in it's extended styles,
        then the window is right to left layout and the algorithm terminates.
    5- If the hWndParent parameter to Createwindow(Ex) was NULL, and the process calling CreateWindow(Ex) has called
        SetProcessDefaultLayout(LAYOUT_RTL), then the window is right to left layout and the algorithm terminates.
    6- In all other cases, the layout is left to right.
    */

    if (!(dwExStyle & WS_EX_LAYOUTRTL)) {
        if (pwndParent != NULL) {
            if (fChild && TestWF(pwndParent, WEFLAYOUTRTL) && !TestWF(pwndParent, WEFNOINHERITLAYOUT)) {
                dwExStyle |= WS_EX_LAYOUTRTL;
            }
        } else if (!(!IS_PTR(cczpstrClass) && (PTR_TO_ID(cczpstrClass) == PTR_TO_ID(DIALOGCLASS)))) {
            if ((_GetProcessDefaultLayout(&dwLayout)) && (dwLayout & LAYOUT_RTL)) {
                dwExStyle |= WS_EX_LAYOUTRTL;
            }
        }
    }
#endif
    /*
     * Ensure that we can create the window.  If there is no desktop
     * yet, assume that this will be the root desktop window and allow
     * the creation.
     */
    if (ptiCurrent->hdesk) {
        RETURN_IF_ACCESS_DENIED(
                ptiCurrent->amdesk, DESKTOP_CREATEWINDOW, NULL);
    }

    if (fChild) {

        /*
         * Don't allow child windows without a parent handle.
         */
        if (pwndParent == NULL) {
            RIPERR0(ERROR_TLW_WITH_WSCHILD, RIP_VERBOSE, "");
            return NULL;
        }

        if (!ValidateParentDepth(NULL, pwndParent)) {
            RIPERR0(ERROR_INVALID_PARAMETER, RIP_WARNING, "Exceeded nested children limit");
            return NULL;
        }
    }

    /*
     * Make sure we can get the window class.
     */
    if (IS_PTR(cczpstrClass)) {
        /*
         * UserFindAtom protects access of the string.
         */
        atomT = UserFindAtom(cczpstrClass->Buffer);
    } else
        atomT = PTR_TO_ID(cczpstrClass);

    if (atomT == 0) {
CantFindClassMessageAndFail:
#if DBG
        if (IS_PTR(cczpstrClass)) {
            try {
                RIPMSG1(RIP_WARNING,
                        "Couldn't find class string %ws",
                        cczpstrClass->Buffer);

            } except (W32ExceptionHandler(FALSE, RIP_WARNING)) {
            }
        } else {
            RIPMSG1(RIP_WARNING,
                    "Couldn't find class atom %lx",
                    cczpstrClass);
        }
#endif

        RIPERR0(ERROR_CANNOT_FIND_WND_CLASS, RIP_VERBOSE, "");
        return NULL;
    }

    /*
     * First scan the private classes.  If we don't find the class there
     * scan the public classes.  If we don't find it there, fail.
     */
    ppcls = GetClassPtr(atomT, ptiCurrent->ppi, hInstance);
    if (ppcls == NULL) {
        goto CantFindClassMessageAndFail;
    }

    pcls = *ppcls;

    if (NeedsWindowEdge(style, dwExStyle, Is400Compat(dwExpWinVerAndFlags))) {
        dwExStyle |= WS_EX_WINDOWEDGE;
    } else {
        dwExStyle &= ~WS_EX_WINDOWEDGE;
    }

    /*
     * Allocate memory for regular windows.
     */
    pwnd = HMAllocObject(
            ptiCurrent, pdesk, TYPE_WINDOW, sizeof(WND) + pcls->cbwndExtra);

    if (pwnd == NULL) {
        RIPERR0(ERROR_OUTOFMEMORY,
                RIP_WARNING,
                "Out of pool in xxxCreateWindowEx");

        return NULL;
    }

    /*
     * Stuff in the pq, class pointer, and window style.
     */
    pwnd->pcls = pcls;
    pwnd->style = style & ~WS_VISIBLE;
#ifdef REDIRECTION
    pwnd->ExStyle = dwExStyle & ~(WS_EX_LAYERED | WS_EX_REDIRECTED);
#else
    pwnd->ExStyle = dwExStyle & ~WS_EX_LAYERED;
#endif // REDIRECTION
    pwnd->cbwndExtra = pcls->cbwndExtra;


    /*
     * Increment the Window Reference Count in the Class structure
     * Because xxxFreeWindow() decrements the count, incrementing has
     * to be done now.  Incase of error, xxxFreeWindow() will decrement it.
     */
    if (!ReferenceClass(pcls, pwnd)) {
        HMFreeObject(pwnd);
        goto CantFindClassMessageAndFail;
    }

    /*
     * Button control doesn't need input context. Other windows
     * will associate with the default input context.
     */
    if (atomT == gpsi->atomSysClass[ICLS_BUTTON]) {
        pwnd->hImc = NULL_HIMC;
    } else {
        pwnd->hImc = (HIMC)PtoH(ptiCurrent->spDefaultImc);
    }

    /*
     * Update the window count.  Doing this now will ensure that if
     * the creation fails, xxxFreeWindow will keep the window count
     * correct.
     */
    ptiCurrent->cWindows++;

    /*
     * Get the class from the window because ReferenceClass may have
     * cloned the class.
     */
    pcls = pwnd->pcls;

    /*
     * This is a replacement for the &lpCreateParams stuff that used to
     * pass a pointer directly to the parameters on the stack.  This
     * step must be done AFTER referencing the class because we
     * may use the ANSI class name.
     */
    RtlZeroMemory(&csex, sizeof(csex));
    csex.cs.dwExStyle = dwExStyle;
    csex.cs.hInstance = hInstance;

    if (!IS_PTR(cczpstrClass)) {
        csex.cs.lpszClass = (LPWSTR)cczpstrClass;
    } else {
        if (wWFAnsiCreator) {
            csex.cs.lpszClass = (LPWSTR)pcls->lpszAnsiClassName;
            if (IS_PTR(csex.cs.lpszClass)) {
                RtlInitLargeAnsiString(
                        (PLARGE_ANSI_STRING)&csex.strClass,
                        (LPSTR)csex.cs.lpszClass,
                        (UINT)-1);
            }
        } else {
            csex.cs.lpszClass = cczpstrClass->Buffer;
            csex.strClass = *cczpstrClass;
        }
    }

    if (cczpstrName != NULL) {
        csex.cs.lpszName = cczpstrName->Buffer;
        csex.strName = *cczpstrName;
    }
    csex.cs.style = style;
    csex.cs.x = x;
    csex.cs.y = y;
    csex.cs.cx = cx;
    csex.cs.cy = cy;
    csex.cs.hwndParent = HW(pwndParent);

    /*
     * If pMenu is non-NULL and the window is not a child, pMenu must
     * be a menu.
     * Child windows get their UIState bits from their parent.  Top level ones
     * remain with the default cleared bits.
     *
     * The below test is equivalent to TestwndChild().
     */
    if (fChild) {
        csex.cs.hMenu = (HMENU)pMenu;

        pwnd->ExStyle |= pwndParent->ExStyle & (WS_EXP_FOCUSHIDDEN | WS_EXP_ACCELHIDDEN);
#if WS_EXP_ACCELHIDDEN  !=  0x40000000
#error Fix UISTATE bits copying if you moved the UISTATE bits from ExStyle
#endif

    } else {
        csex.cs.hMenu = PtoH(pMenu);
    }

    csex.cs.lpCreateParams = lpCreateParams;

    /*
     * ThreadLock: we are going to be doing multiple callbacks here.
     */
    ThreadLockAlwaysWithPti(ptiCurrent, pwnd, &tlpwnd);

    /*
     * set the parent to be the desktop window (if exists)
     * before any callback. This way we'll always have a
     * pointer on spwndParent
     */
    if (pwnd->head.rpdesk) {
        Lock(&(pwnd->spwndParent), PWNDMESSAGE(pwnd));
    }

    /*
     * Create the class small icon if there isn't one since we are in context
     * and we are creating a window from this class...
     */
    if (pcls->spicn && !pcls->spicnSm) {
        xxxCreateClassSmIcon(pcls);
    }

    /*
     * Store the instance handle and window proc address.  We do this earlier
     * than Windows because they have a bug were a message can be sent
     * but lpfnWndProc is not set (3986 CBT WM_CREATE not allowed.)
     */
    pwnd->hModule = hInstance;

    /*
     * Get rid of EditWndProc plain.
     */
    pwnd->lpfnWndProc = (WNDPROC_PWND)MapClientNeuterToClientPfn(pcls, 0, wWFAnsiCreator);

    /*
     * If this window class has a server-side window procedure, mark
     * it as such.  If the app subclasses it later with an app-side proc
     * then this mark will be removed.
     */
    if (pcls->CSF_flags & CSF_SERVERSIDEPROC) {
        SetWF(pwnd, WFSERVERSIDEPROC);
        UserAssert(!(pcls->CSF_flags & CSF_ANSIPROC));
    }

    /*
     * If this window was created with an ANSI CreateWindow*() call, mark
     * it as such so edit controls will be created correctly. (A combobox
     * will be able to pass the WFANSICREATOR bit on to its edit control)
     */
    SetWF(pwnd, wWFAnsiCreator);

    /*
     * If this window belongs to an ANSI class or it is a WFANSICREATOR
     * control, then mark it as an ANSI window
     */
    if ((pcls->CSF_flags & CSF_ANSIPROC) ||
            (wWFAnsiCreator &&
             ((atomT == gpsi->atomSysClass[ICLS_BUTTON]) ||
              (atomT == gpsi->atomSysClass[ICLS_COMBOBOX]) ||
              (atomT == gpsi->atomSysClass[ICLS_COMBOLISTBOX]) ||
              (atomT == gpsi->atomSysClass[ICLS_DIALOG]) ||
              (atomT == gpsi->atomSysClass[ICLS_EDIT]) ||
              (atomT == gpsi->atomSysClass[ICLS_LISTBOX]) ||
              (atomT == gpsi->atomSysClass[ICLS_MDICLIENT]) ||
              (atomT == gpsi->atomSysClass[ICLS_IME]) ||
              (atomT == gpsi->atomSysClass[ICLS_STATIC])))) {
        SetWF(pwnd, WFANSIPROC);
    }

    /*
     * If a 3.1-compatible application is creating the window, set this
     * bit to enable various backward-compatibility hacks.
     *
     * If it's not 3.1 compatible, see if we need to turn on the PixieHack
     * (see wmupdate.c for more info on this)
     */

    dw = GetAppCompatFlags(ptiCurrent);

    if (dw & GACF_RANDOM3XUI) {
        SetWF(pwnd, WFOLDUI);

        dwExStyle &= 0x0000003f;
        csex.cs.dwExStyle &= 0x0000003f;
    }

    pwnd->hMod16  = ((ptiCurrent->TIF_flags & TIF_16BIT) && !TestWF(pwnd, WFSERVERSIDEPROC))? xxxClientWOWGetProcModule(pwnd->lpfnWndProc):0;
    if (Is310Compat(dwExpWinVerAndFlags)) {
        SetWF(pwnd, WFWIN31COMPAT);
        if (Is400Compat(dwExpWinVerAndFlags)) {
            SetWF(pwnd, WFWIN40COMPAT);
            if (Is500Compat(dwExpWinVerAndFlags)) {
                SetWF(pwnd, WFWIN50COMPAT);
            }
        }
    } else if (dw & GACF_ALWAYSSENDNCPAINT) {
        SetWF(pwnd, WFALWAYSSENDNCPAINT);
    }

    /*
     * Inform the CBT hook that a window is being created.  Pass it the
     * CreateParams and the window handle that the new one will be inserted
     * after.  The CBT hook handler returns TRUE to prevent the window
     * from being created.  It can also modify the CREATESTRUCT info, which
     * will affect the size, parent, and position of the window.
     * Defaultly position non-child windows at the top of their list.
     */

    if (IsHooked(ptiCurrent, WHF_CBT)) {
        CBT_CREATEWND cbt;

        /*
         * Use the extended createstruct so the hook thunk can
         * handle the strings correctly.
         */
        cbt.lpcs = (LPCREATESTRUCT)&csex;
        cbt.hwndInsertAfter = HWND_TOP;

        if ((BOOL)xxxCallHook(HCBT_CREATEWND, (WPARAM)HWq(pwnd),
                (LPARAM)&cbt, WH_CBT)) {

            goto MemError;
        } else {
            /*
             * The CreateHook may have modified some parameters so write them
             * out (in Windows 3.1 we used to write directly to the variables
             * on the stack).
             */

            x = csex.cs.x;
            y = csex.cs.y;
            cx = csex.cs.cx;
            cy = csex.cs.cy;

            if (!IS_PTR(cbt.hwndInsertAfter))
                pwndZOrder = (PWND)cbt.hwndInsertAfter;
            else
                pwndZOrder = RevalidateHwnd(cbt.hwndInsertAfter);
        }
    } else {
        pwndZOrder = (PWND)HWND_TOP;
    }

    if (!(fTiled = TestwndTiled(pwnd))) {

        /*
         * CW_USEDEFAULT is only valid for tiled and overlapped windows.
         * Don't let it be used.
         */
        if (x == CW_USEDEFAULT || x == CW2_USEDEFAULT) {
            x = 0;
            y = 0;
        }

        if (cx == CW_USEDEFAULT || cx == CW2_USEDEFAULT) {
            cx = 0;
            cy = 0;
        }
    }

    /*
     * Make local copies of these parameters.
     */
    src.x = x;
    src.y  = y;
    src.cx = cx;
    src.cy = cy;

    /*
     *    Position Child Windows
     */
    if (fChild = (BOOL)TestwndChild(pwnd)) {

        /*
         * Child windows are offset from the parent's origin.
         */
        UserAssert(pwndParent);
        if (pwndParent != PWNDDESKTOP(pwnd)) {
            src.x += pwndParent->rcClient.left;
            src.y += pwndParent->rcClient.top;
        }

        /*
         * Defaultly position child windows at bottom of their list.
         */
        pwndZOrder = PWND_BOTTOM;
    }

    /*
     *    Position Tiled Windows
     */

    /*
     * Is this a Tiled/Overlapping window?
     */
    if (fTiled) {

        /*
         * Force the WS_CLIPSIBLINGS window style and add a caption and
         * a border.
         */
        SetWF(pwnd, WFCLIPSIBLINGS);
        mask = MaskWF(WFCAPTION) | MaskWF(WFBORDER);

        //
        // We add on a raised edge since IF the person had passed in WS_CAPTION,
        // and didn't specify any 3D borders, we would've added it on to the
        // style bits above.
        //

        if (TestWF(pwnd, WFWIN40COMPAT)) {
//        if (!TestWF(pwnd, WEFEDGEMASK))
            SetWF(pwnd, WEFWINDOWEDGE);
        }

        /*
         * Set bit that will force size message to be sent at SHOW time.
         */
        SetWF(pwnd, WFSENDSIZEMOVE);

        /*
         * Here is how the "tiled" window initial positioning works...
         * If the app is a 1.0x app, then we use our standard "stair step"
         * default positioning scheme.  Otherwise, we check the x & cx
         * parameters.  If either of these == CW_USEDEFAULT then use the
         * default position/size, otherwise use the position/size they
         * specified.  If not using default position, use SW_SHOW for the
         * xxxShowWindow() parameter, otherwise use the y parameter given.
         *
         * In 32-bit world, CW_USEDEFAULT is 0x80000000, but apps still
         * store word-oriented values either in dialog templates or
         * in their own structures.  So CreateWindow still recognizes the
         * 16 bit equivalent, which is 0x8000, CW2_USEDEFAULT.  The original
         * is changed because parameters to CreateWindow() are 32 bit
         * values, which can cause sign extention, or weird results if
         * 16 bit math assumptions are being made, etc.
         */

        /*
         * Default to passing the y parameter to xxxShowWindow().
         */
        if (x == CW_USEDEFAULT || x == CW2_USEDEFAULT) {

            /*
             * If the y value is not CW_USEDEFAULT, use it as a SW_* command.
             */
            if (src.y != CW_USEDEFAULT && src.y != CW2_USEDEFAULT) {
                sw = src.y;
            }
        }


        /*
         * Allow the shell to tell us what monitor to run this app on
         */
        pMonitor = NULL;
        if (    x == CW_USEDEFAULT ||
                x == CW2_USEDEFAULT ||
                cx == CW_USEDEFAULT ||
                cx == CW2_USEDEFAULT) {

            if (ptiCurrent->ppi->hMonitor) {
                pMonitor = ValidateHmonitor(ptiCurrent->ppi->hMonitor);
            } else if (pwndParent) {
                pMonitor = _MonitorFromWindow(pwndParent, MONITOR_DEFAULTTONEAREST);
            }
        }

        if (!pMonitor) {
            pMonitor = GetPrimaryMonitor();
        }

        SetTiledRect(pwnd, &rc, pMonitor);

        /*
         * Did the app ask for default positioning?
         */
        if (x == CW_USEDEFAULT || x == CW2_USEDEFAULT) {

            /*
             * Use default positioning.
             */
            if (ptiCurrent->ppi->usi.dwFlags & STARTF_USEPOSITION ) {
                fStartup = TRUE;
                x = src.x = ptiCurrent->ppi->usi.dwX;
                y = src.y = ptiCurrent->ppi->usi.dwY;
            } else {
                x = src.x = rc.left;
                y = src.y = rc.top;
            }
            fDefPos = TRUE;

        } else {

            /*
             * Use the apps specified positioning.  Undo the "stacking"
             * effect caused by SetTiledRect().
             */
            if (pMonitor->cWndStack) {
                pMonitor->cWndStack--;
            }
        }

        /*
         * Did the app ask for default sizing?
         */
        if (src.cx == CW_USEDEFAULT || src.cx == CW2_USEDEFAULT) {

            /*
             * Use default sizing.
             */
            if (ptiCurrent->ppi->usi.dwFlags & STARTF_USESIZE) {
                fStartup = TRUE;
                src.cx = ptiCurrent->ppi->usi.dwXSize;
                src.cy = ptiCurrent->ppi->usi.dwYSize;
            } else {
                src.cx = rc.right - x;
                src.cy = rc.bottom - y;
            }
            fDefPos = TRUE;

        } else if (fDefPos) {
            /*
             * The app wants default positioning but not default sizing.
             * Make sure that it's still entirely visible by moving the
             * window.
             */
            dx = (src.x + src.cx) - pMonitor->rcMonitor.right;
            dy = (src.y + src.cy) - pMonitor->rcMonitor.bottom;
            if (dx > 0) {
                x -= dx;
                src.x = x;
                if (src.x < pMonitor->rcMonitor.left) {
                    src.x = x = pMonitor->rcMonitor.left;
                }
            }

            if (dy > 0) {
                y -= dy;
                src.y = y;
                if (src.y < pMonitor->rcMonitor.top) {
                    src.y = y = pMonitor->rcMonitor.top;
                }
            }
        }
    }

    /*
     * If we have used any startup postitions, turn off the startup
     * info so we don't use it again.
     */
    if (fStartup) {
        ptiCurrent->ppi->usi.dwFlags &=
                ~(STARTF_USESIZE | STARTF_USEPOSITION);
    }

    /*
     *    Position Popup Windows
     */

    if (TestwndPopup(pwnd)) {
// LATER: Why is this test necessary? Can one create a popup desktop?
        if (pwnd != _GetDesktopWindow()) {

            /*
             * Force the clipsiblings/overlap style.
             */
            SetWF(pwnd, WFCLIPSIBLINGS);
        }
    }

    /*
     * Shove in those default style bits.
     */
    *(((WORD *)&pwnd->style) + 1) |= mask;

    /*
     *    Menu/SysMenu Stuff
     */

    /*
     * If there is no menu handle given and it's not a child window but
     * there is a class menu, use the class menu.
     */
    if (pMenu == NULL && !fChild && (pcls->lpszMenuName != NULL)) {
        UNICODE_STRING strMenuName;

        RtlInitUnicodeStringOrId(&strMenuName, pcls->lpszMenuName);
        pMenu = xxxClientLoadMenu(pcls->hModule, &strMenuName);
        csex.cs.hMenu = PtoH(pMenu);

        /*
         * This load fails if the caller does not have DESKTOP_CREATEMENU
         * permission but that's ok they will just get a window without a menu
         */
    }

    /*
     * Store the menu handle.
     */
    if (TestwndChild(pwnd)) {

        /*
         * It's an id in this case.
         */
        pwnd->spmenu = pMenu;
    } else {

        /*
         * It's a real handle in this case.
         */
        LockWndMenu(pwnd, &pwnd->spmenu, pMenu);
    }

// LATER does this work?
    /*
     * Delete the Close menu item if directed.
     */
    if (TestCF(pwnd, CFNOCLOSE)) {

        /*
         * Do this by position since the separator does not have an ID.
         */
// LATER mikeke why is xxxGetSystemMenu() returning NULL?
        pMenu = xxxGetSystemMenu(pwnd, FALSE);
        if (pMenu != NULL) {
            TL tlpMenu;

            ThreadLock(pMenu, &tlpMenu);
            xxxDeleteMenu(pMenu, 5, MF_BYPOSITION);
            xxxDeleteMenu(pMenu, 5, MF_BYPOSITION);
            ThreadUnlock(&tlpMenu);
        }
    }

    /*
     *    Parent/Owner Stuff
     */

    /*
     * If this isn't a child window, reset the Owner/Parent info.
     */
    if (!fChild) {
        Lock(&(pwnd->spwndLastActive), pwnd);
        if ((pwndParent != NULL) &&
                (pwndParent != pwndParent->head.rpdesk->spwndMessage) &&
                (pwndParent != pwndParent->head.rpdesk->pDeskInfo->spwnd)) {

            PWND pwndOwner = GetTopLevelWindow(pwndParent);

            if (!ValidateOwnerDepth(pwnd, pwndOwner)) {
                RIPERR1(ERROR_INVALID_PARAMETER,
                        RIP_WARNING,
                        "Exceeded nested owner limit for pwnd %#p",
                        pwnd);
                goto MemError;
            }

#if DBG
            if (pwnd->pcls->atomClassName == gpsi->atomSysClass[ICLS_IME]) {
                UserAssert(!TestCF(pwndOwner, CFIME));
            }
#endif
            Lock(&(pwnd->spwndOwner), pwndOwner);
            if (pwnd->spwndOwner && TestWF(pwnd->spwndOwner, WEFTOPMOST)) {

                /*
                 * If this window's owner is a topmost window, then it has to
                 * be one also since a window must be above its owner.
                 */
                SetWF(pwnd, WEFTOPMOST);
            }

            /*
             * If this is a owner window on another thread, share input
             * state so this window gets z-ordered correctly.
             */
            if (atomT != gpsi->atomSysClass[ICLS_IME] &&
                    pwnd->spwndOwner != NULL &&
                    GETPTI(pwnd->spwndOwner) != ptiCurrent) {
                /*
                 * No need to DeferWinEventNotify() here:  pwnd and pwndParent
                 * are locked and because we called ReferenceClass(pcls, pwnd),
                 * pcls is safe until xxxFreeWindow(pwnd). (IanJa)
                 */
                zzzAttachThreadInput(ptiCurrent, GETPTI(pwnd->spwndOwner), TRUE);
            }

        } else {
            pwnd->spwndOwner = NULL;
        }

#if DBG
        if (ptiCurrent->rpdesk != NULL) {
            UserAssert(!(ptiCurrent->rpdesk->dwDTFlags & (DF_DESTROYED | DF_DESKWNDDESTROYED | DF_DYING)));
        }
#endif
        if ((pwndParent == NULL) ||
               (pwndParent != pwndParent->head.rpdesk->spwndMessage)) {
            pwndParent = _GetDesktopWindow();

            ThreadLockWithPti(ptiCurrent, pwndParent, &tlpwndParent);
            fLockParent = TRUE;
        }
    }

    /*
     * Store backpointer to parent.
     */
    Lock(&(pwnd->spwndParent), pwndParent);

    /*
     *    Final Window Positioning
     */

    if (!TestWF(pwnd, WFWIN31COMPAT)) {
        /*
         * BACKWARD COMPATIBILITY HACK
         *
         * In 3.0, CS_PARENTDC overrides WS_CLIPCHILDREN and WS_CLIPSIBLINGS,
         * but only if the parent is not WS_CLIPCHILDREN.
         * This behavior is required by PowerPoint and Charisma, among others.
         */
        if ((pcls->style & CS_PARENTDC) &&
                !TestWF(pwndParent, WFCLIPCHILDREN)) {
#if DBG
            if (TestWF(pwnd, WFCLIPCHILDREN))
                RIPMSG0(RIP_WARNING, "WS_CLIPCHILDREN overridden by CS_PARENTDC");
            if (TestWF(pwnd, WFCLIPSIBLINGS))
                RIPMSG0(RIP_WARNING, "WS_CLIPSIBLINGS overridden by CS_PARENTDC");
#endif
            ClrWF(pwnd, (WFCLIPCHILDREN | WFCLIPSIBLINGS));
        }
    }

    /*
     * If this is a child window being created in a parent window
     * of a different thread, but not on the desktop, attach their
     * input streams together. [windows with WS_CHILD can be created
     * on the desktop, that's why we check both the style bits
     * and the parent window.]
     */
    if (TestwndChild(pwnd) && (pwndParent != PWNDDESKTOP(pwnd)) &&
            (ptiCurrent != GETPTI(pwndParent))) {
        /*
         * No need to DeferWinEventNotify() - there is an xxx call just below
         */
        zzzAttachThreadInput(ptiCurrent, GETPTI(pwndParent), TRUE);
    }

    /*
     * Make sure the window is between the minimum and maximum sizes.
     */

    /*
     * HACK ALERT!
     * This sends WM_GETMINMAXINFO to a (tiled or sizable) window before
     * it has been created (before it is sent WM_NCCREATE).
     * Maybe some app expects this, so we nustn't reorder the messages.
     */
    xxxAdjustSize(pwnd, &src.cx, &src.cy);

    /*
     * check for a window being created full screen
     *
     * Note the check for a non-NULL pdeskParent -- this is important for CreateWindowStation
     */
    if (    pwnd->head.rpdesk != NULL &&
            !TestWF(pwnd, WFCHILD) &&
            !TestWF(pwnd, WEFTOOLWINDOW)) {

        xxxCheckFullScreen(pwnd, &src);
    }

    if (src.cx < 0) {
        RIPMSG1(RIP_WARNING, "xxxCreateWindowEx: adjusted cx in pwnd %#p", pwnd);
        src.cx = 0;
    }

    if (src.cy < 0) {
        RIPMSG1(RIP_WARNING, "xxxCreateWindowEx: adjusted cy in pwnd %#p", pwnd);
        src.cy = 0;
    }

    /*
     * Calculate final window dimensions...
     */
    RECTFromSIZERECT(&pwnd->rcWindow, &src);

    if (dwExStyle & WS_EX_LAYERED) {
        if (!xxxSetLayeredWindow(pwnd, FALSE)) {
            goto MemError;
        }
    }

#ifdef REDIRECTION
    if (dwExStyle & WS_EX_REDIRECTED) {
        if (!SetRedirectedWindow(pwnd)) {
            goto MemError;
        }
    }
#endif // REDIRECTION

    if (TestCF2(pcls, CFOWNDC) ||
            (TestCF2(pcls, CFCLASSDC) && pcls->pdce == NULL)) {
        if (NULL == CreateCacheDC(pwnd, DCX_OWNDC, NULL)) {

            RIPMSG1(RIP_WARNING, "xxxCreateWindowEx: pwnd %#p failed to create cached DC",
                    pwnd);

            goto MemError;
        }
    }

    /*
     * Update the create struct now that we've modified some passed in
     * parameters.
     */
    csex.cs.x = x;
    csex.cs.y = y;
    csex.cs.cx = cx;
    csex.cs.cy = cy;

    /*
     * Send a NCCREATE message to the window.
     */
    if (!xxxSendMessage(pwnd, WM_NCCREATE, 0L, (LPARAM)&csex)) {

MemError:

#if DBG
        if (!IS_PTR(cczpstrClass)) {
            RIPMSG2(RIP_WARNING,
                    (pwndParent) ?
                            "xxxCreateWindowEx failed, Class=%#.4x, ID=%d" :
                            "xxxCreateWindowEx failed, Class=%#.4x",
                    PTR_TO_ID(cczpstrClass),
                    (LONG_PTR) pMenu);
        } else {
            RIPMSG2(RIP_WARNING,
                    (pwndParent) ?
                            "xxxCreateWindowEx failed, Class=\"%s\", ID=%d" :
                            "xxxCreateWindowEx failed, Class=\"%s\"",
                    pcls->lpszAnsiClassName,
                    (LONG_PTR) pMenu);
        }
#endif

        if (fLockParent)
            ThreadUnlock(&tlpwndParent);

        /*
         * Set the state as destroyed so any z-ordering events will be ignored.
         * We cannot NULL out the owner field until WM_NCDESTROY is send or
         * apps like Rumba fault  (they call GetParent after every message)
         */
        SetWF(pwnd, WFDESTROYED);

        /*
         * Unset the visible flag so we don't think in xxxDestroyWindow that
         * this window is visible
         */
        if (TestWF(pwnd, WFVISIBLE)) {
            SetVisible(pwnd, SV_UNSET);
        }

        /*
         * FreeWindow performs a ThreadUnlock.
         */
        xxxFreeWindow(pwnd, &tlpwnd);

        return NULL;
    }

    /*
     * WM_NCCREATE processing may have changed the window text.  Change
     * the CREATESTRUCT to point to the real window text.
     *
     * MSMoney needs this because it clears the window and we need to
     * reflect the new name back into the cs structure.
     * A better thing to do would be to have a pointer to the CREATESTRUCT
     * within the window itself so that DefWindowProc can change the
     * the window name in the CREATESTRUCT to point to the real name and
     * this funky check is no longer needed.
     *
     * DefSetText converts a pointer to NULL to a NULL title so
     * we don't want to over-write cs.lpszName if it was a pointer to
     * a NULL string and pName is NULL.  Approach Database for Windows creates
     * windows with a pointer to NULL and then accesses the pointer later
     * during WM_CREATE
     */
    if (TestWF(pwnd, WFTITLESET))
        if (!(csex.strName.Buffer != NULL && csex.strName.Length == 0 &&
                pwnd->strName.Buffer == NULL)) {
            csex.cs.lpszName = pwnd->strName.Buffer;
            RtlCopyMemory(&csex.strName, &pwnd->strName, sizeof(LARGE_STRING));
        }

    /*
     * The Window is now officially "created."  Change the relevant global
     * stuff.
     */


     /*
      * Create per thread default IME window.
      */
    if (IS_IME_ENABLED() && ptiCurrent->spwndDefaultIme == NULL) {
        /*
         * Avoid creating the default IME window to any of message only windows
         * or windows on no I/O desktop.
         */
        if (WantImeWindow(pwndParent, pwnd)) {
            //
            // Make sure we are not creating a window for Ole,
            // for it does not pump messages even though
            // they creates a window.
            //
            UserAssert(gaOleMainThreadWndClass != atomT);

            Lock(&(ptiCurrent->spwndDefaultIme),
                  xxxCreateDefaultImeWindow(pwnd, atomT, hInstance));


            /*
             * If keybaord layout is switched but Imm activation was skipped
             * while spwndDefaultIme was gone, do the activation now.
             */
#if _DBG
            if (ptiCurrent->spDefaultImc == NULL) {
                RIPMSG1(RIP_WARNING, "xxxCreateWindowEx: ptiCurrent(%08p)->spDefaultImc is NULL.", ptiCurrent);
            }
            ASSERT(ptiCurrent->pClientInfo);
#endif

            if (ptiCurrent->spwndDefaultIme && (ptiCurrent->pClientInfo->CI_flags & CI_INPUTCONTEXT_REINIT)) {

                TL tlpwndIme;

                TAGMSG1(DBGTAG_IMM, "xxxCreateDefaultImeWindow: ptiCurrent(%08p)->spDefaultImc->fNeedClientImcActivate is set.", ptiCurrent);
                /*
                 * Make this client side callback to force the input context
                 * is re-initialized appropriately.
                 * (keyboard layout has been changed since this thread was taking
                 * a nap while with no window but still was GUI thread)
                 * see raid #294964
                 */
                ThreadLock(ptiCurrent->spwndDefaultIme, &tlpwndIme);
                xxxSendMessage(ptiCurrent->spwndDefaultIme, WM_IME_SYSTEM, (WPARAM)IMS_ACTIVATETHREADLAYOUT, (LPARAM)ptiCurrent->spklActive->hkl);

                // Reset the flag.
                ptiCurrent->pClientInfo->CI_flags &= ~CI_INPUTCONTEXT_REINIT;

                ThreadUnlock(&tlpwndIme);
            }
        }
        else {
            RIPMSG0(RIP_VERBOSE, "xxxCreateWindowEx: default IME window is not created.");
        }
    }


    /*
     * Update the Parent/Child linked list.
     */
    if (pwndParent != NULL) {
        if (!fChild && (pwndParent != pwndParent->head.rpdesk->spwndMessage)) {

            /*
             * If this is a top-level window, and it's not part of the
             * topmost pile of windows, then we have to make sure it
             * doesn't go on top of any of the topmost windows.
             *
             * If he's trying to put the window on the top, or trying
             * to insert it after one of the topmost windows, insert
             * it after the last topmost window in the pile.
             */
            if (!TestWF(pwnd, WEFTOPMOST)) {
                if (pwndZOrder == PWND_TOP ||
                        TestWF(pwndZOrder, WEFTOPMOST)) {
                    pwndZOrder = CalcForegroundInsertAfter(pwnd);
                }
            } else {
                pwndHardError = GETTOPMOSTINSERTAFTER(pwnd);
                if (pwndHardError != NULL) {
                    pwndZOrder = pwndHardError;
                }
            }
        }

        LinkWindow(pwnd, pwndZOrder, pwndParent);
    }

    /*
     *    Message Sending
     */

    /*
     * Send a NCCALCSIZE message to the window and have it return the official
     * size of its client area.
     */

#ifdef USE_MIRRORING
    if (fChild && TestWF(pwndParent, WEFLAYOUTRTL)) {
        cx = pwnd->rcWindow.right - pwnd->rcWindow.left;
        pwnd->rcWindow.right = pwndParent->rcClient.right - (pwnd->rcWindow.left - pwndParent->rcClient.left);
        pwnd->rcWindow.left  = pwnd->rcWindow.right - cx;
    }
#endif

    CopyRect(&rc, &pwnd->rcWindow);
    xxxSendMessage(pwnd, WM_NCCALCSIZE, 0L, (LPARAM)&rc);
    pwnd->rcClient = rc;

    /*
     * Send a CREATE message to the window.
     */
    if (xxxSendMessage(pwnd, WM_CREATE, 0L, (LPARAM)&csex) == -1L) {
#if DBG
        if (!IS_PTR(cczpstrClass)) {
            RIPMSG1(RIP_WARNING,
                    "CreateWindow() send of WM_CREATE failed, Class = 0x%x",
                    PTR_TO_ID(cczpstrClass));
        } else {
            RIPMSG1(RIP_WARNING,
                    "CreateWindow() send of WM_CREATE failed, Class = \"%s\"",
                    pcls->lpszAnsiClassName);
        }
#endif
        if (fLockParent)
            ThreadUnlock(&tlpwndParent);

        if (ThreadUnlock(&tlpwnd))
            xxxDestroyWindow(pwnd);

        return NULL;
    }

    SetWF(pwnd, WFISINITIALIZED); /* Flag that the window is created.
                                     WoW uses this bit to determine that
                                     an fnid of 0 really means 0. */

    /*
     * Notify anyone who is listening that the window is created.  Do this
     * before we size/move/max/min/show it so that event observers can count
     * on getting notifications for those things also.
     *
     * But do this AFTER WM_CREATE is sent.  The window and its data will not
     * be fully initialized until then.  Since the purpose of an event is to
     * let watchers turn around and do querying, we want their queries to
     * succeed and not fault.
     */
    if (FWINABLE()) {
        xxxWindowEvent(EVENT_OBJECT_CREATE, pwnd, OBJID_WINDOW, INDEXID_OBJECT, 0);
    }

    /*
     * If this is a Tiled/Overlapped window, don't send size or move msgs yet.
     */
    if (!TestWF(pwnd, WFSENDSIZEMOVE)) {
        xxxSendSizeMessage(pwnd, SIZENORMAL);

        if (pwndParent != NULL && PWNDDESKTOP(pwnd) != pwndParent) {
            rc.left -= pwndParent->rcClient.left;
            rc.top -= pwndParent->rcClient.top;
        }

        xxxSendMessage(pwnd, WM_MOVE, 0L, MAKELONG(rc.left, rc.top));
    }

    /*
     *    Min/Max Stuff
     */

    /*
     * If app specified either min/max style, then we must call our minmax
     * code to get it all set up correctly so that when the show is done,
     * the window is displayed right.
     */
    dwMinMax = MINMAX_KEEPHIDDEN | TEST_PUDF(PUDF_ANIMATE);
    if (TestWF(pwnd, WFMINIMIZED)) {
        SetMinimize(pwnd, SMIN_CLEAR);
        xxxMinMaximize(pwnd, SW_SHOWMINNOACTIVE, dwMinMax);
    } else if (TestWF(pwnd, WFMAXIMIZED)) {
        ClrWF(pwnd, WFMAXIMIZED);
        xxxMinMaximize(pwnd, SW_SHOWMAXIMIZED, dwMinMax);
    }

    /*
     * Send notification if child
     */

    // LATER 15-Aug-1991 mikeke
    // pointer passed as a word here

    if (fChild && !TestWF(pwnd, WEFNOPARENTNOTIFY) &&
            (pwnd->spwndParent != NULL)) {
        ThreadLockAlwaysWithPti(ptiCurrent, pwnd->spwndParent, &tlpwndParentT);
        xxxSendMessage(pwnd->spwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_CREATE, PTR_TO_ID(pwnd->spmenu)), (LPARAM)HWq(pwnd));
        ThreadUnlock(&tlpwndParentT);
    }

    /*
     * Show the Window
     */
    if (style & WS_VISIBLE) {
        xxxShowWindow(pwnd, sw | TEST_PUDF(PUDF_ANIMATE));
    }

    /*
     * Try and set the application's hot key.  Use the Win95 logic of
     * looking for the first tiled and/or APPWINDOW to be created by
     * this process.
     */
    if (TestwndTiled(pwnd) || TestWF(pwnd, WEFAPPWINDOW)) {
        if (ptiCurrent->ppi->dwHotkey) {
            /*
             * Ignore hot keys for WowExe the first thread of a wow process.
             */
            if (!(ptiCurrent->TIF_flags & TIF_16BIT) || (ptiCurrent->ppi->cThreads > 1)) {
#ifdef LATER
                /*
                 * Win95 sets the hot key directly, we on the other hand send
                 * a WM_SETHOTKEY message to the app.  Which is right?
                 */
                DWP_SetHotKey(pwnd, ptiCurrent->ppi->dwHotkey);
#else
                xxxSendMessage(pwnd, WM_SETHOTKEY, ptiCurrent->ppi->dwHotkey, 0);
#endif
                ptiCurrent->ppi->dwHotkey = 0;
            }
        }
    }

    if (fLockParent)
        ThreadUnlock(&tlpwndParent);

    return ThreadUnlock(&tlpwnd);
}

BOOL WantImeWindow(IN PWND pwndParent, IN PWND pwnd)
{
    PDESKTOP pdesk;

    UserAssert(pwnd);

    if (PtiCurrent()->TIF_flags & TIF_DISABLEIME) {
        return FALSE;
    }
    if (TestWF(pwnd, WFSERVERSIDEPROC)) {
        return FALSE;
    }

    pdesk = pwnd->head.rpdesk;
    if (pdesk == NULL || pdesk->rpwinstaParent == NULL) {
        return FALSE;
    }

    // Check whether pwnd's desktop has I/O.
    if (pdesk->rpwinstaParent->dwWSF_Flags & WSF_NOIO) {
        return FALSE;
    }

    // Check if the owner window is message-only window.
    if (pwndParent) {
        PWND pwndT = pwndParent;

        while (pwndT && pdesk == pwndT->head.rpdesk) {
            if (pwndT == pdesk->spwndMessage) {
                return FALSE;
            }
            pwndT = pwndT->spwndParent;
        }
    }

    return TRUE;
}


/***************************************************************************\
* SetTiledRect
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void SetTiledRect(
    PWND        pwnd,
    LPRECT      lprc,
    PMONITOR    pMonitor)
{
    POINT   pt;
    RECT    rcT;

    UserAssert(pMonitor->cWndStack >= 0);

    /*
     * Get available desktop area, minus minimized spacing area.
     */
    GetRealClientRect(PWNDDESKTOP(pwnd), &rcT, GRC_MINWNDS, pMonitor);

    /*
     * Normalized rectangle is 3/4 width, 3/4 height of desktop area.  We
     * offset it based on the value of giwndStack for cascading.
     */

    /*
     * We want the left edge of the new window to align with the
     * right edge of the old window's system menu.  And we want the
     * top edge of the new window to align with the bottom edge of the
     * selected caption area (caption height - cyBorder) of the old
     * window.
     */
    pt.x = pMonitor->cWndStack * (SYSMET(CXSIZEFRAME) + SYSMET(CXSIZE));
    pt.y = pMonitor->cWndStack * (SYSMET(CYSIZEFRAME) + SYSMET(CYSIZE));

    /*
     * If below upper top left 1/4 of free area, reset.
     */
    if (    (pt.x > ((rcT.right-rcT.left) / 4)) ||
            (pt.y > ((rcT.bottom-rcT.top) / 4)) ) {

        pMonitor->cWndStack = 0;
        pt.x = 0;
        pt.y = 0;
    }

    /*
     * Get starting position
     */
    pt.x += rcT.left;
    pt.y += rcT.top;

    lprc->left      = pt.x;
    lprc->top       = pt.y;
    lprc->right     = pt.x + MultDiv(rcT.right-rcT.left, 3, 4);
    lprc->bottom    = pt.y + MultDiv(rcT.bottom-rcT.top, 3, 4);

    /*
     * Increment the count of stacked windows.
     */
    pMonitor->cWndStack++;
}


/***************************************************************************\
* xxxAdjustSize
*
* Make sure that *lpcx and *lpcy are within the legal limits.
*
* History:
* 10-19-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxAdjustSize(
    PWND pwnd,
    LPINT lpcx,
    LPINT lpcy)
{
    POINT       ptmin,
                ptmax;
    MINMAXINFO  mmi;

    CheckLock(pwnd);

    /*
     * If this window is sizeable or if this window is tiled, check size
     */
    if (TestwndTiled(pwnd) || TestWF(pwnd, WFSIZEBOX)) {

        /*
         * Get size info from pwnd
         */
        xxxInitSendValidateMinMaxInfo(pwnd, &mmi);

        if (TestWF(pwnd, WFMINIMIZED)) {
            ptmin = mmi.ptReserved;
            ptmax = mmi.ptMaxSize;
        } else {
            ptmin = mmi.ptMinTrackSize;
            ptmax = mmi.ptMaxTrackSize;
        }

        //
        // Make sure we're less than the max, and greater than the min
        //
        *lpcx = max(ptmin.x, min(*lpcx, ptmax.x));
        *lpcy = max(ptmin.y, min(*lpcy, ptmax.y));
    }
}

#if DBG
/***************************************************************************\
* VerifyWindowLink
*
* History:
* 10/28/96 GerardoB     Added
\***************************************************************************/
void VerifyWindowLink (PWND pwnd, PWND pwndParent, BOOL fLink)
{
    BOOL fFirstFound = FALSE;
    BOOL fInFound = FALSE;
    PWND pwndNext = pwndParent->spwndChild;
    PWND pwndFirst = pwndNext;

    while (pwndNext != NULL) {
        if (pwndFirst == pwndNext) {
            if (fFirstFound) {
                RIPMSG1(RIP_ERROR, "Loop in %#p spwndNext chain", pwnd);
                return;
            } else {
                fFirstFound = TRUE;
            }
        }

        if (pwndNext == pwnd) fInFound = TRUE;
        pwndNext = pwndNext->spwndNext;
    }

    if (fLink && !fInFound) {
        RIPMSG1(RIP_ERROR, "%#p not found in spwndNext chain", pwnd);
    }
}
#endif

/***************************************************************************\
* LinkWindow
*
* History:
\***************************************************************************/

void LinkWindow(
    PWND pwnd,
    PWND pwndInsert,
    PWND pwndParent)
{
    if (pwndParent->spwndChild == pwnd) {
        RIPMSG0(RIP_WARNING, "Attempting to link a window to itself");
        return;
    }
    UserAssert(pwnd != pwndInsert);
    UserAssert((pwnd->spwndParent == NULL) || (pwnd->spwndParent == pwndParent));

    if (pwndInsert == PWND_TOP) {

        /*
         * We are at the top of the list.
         */
LinkTop:
#if DBG
        /*
         * If the first child is topmost, so must be pwnd, but only for top-level windows.
         *
         * IME or IME related windows are the exceptions, because ImeSetTopmost() and its fellow
         * do most of the relinking by its own: when LinkWindow() is called, it's possible TOPMOST flags
         * are left in intermediate state. By the time the all window relinking finishes, TOPMOST
         * flags have been taken care of and they are just fine.
         */
        if (pwndParent == PWNDDESKTOP(pwndParent) &&
                pwndParent->spwndChild &&
                FSwpTopmost(pwndParent->spwndChild) &&
                pwndParent != PWNDMESSAGE(pwndParent) &&
                // Check if the target is IME related window
                !TestCF(pwnd, CFIME) && pwnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME]) {

            /*
             * There are few cases that cause the z-ordering code to leave the WFTOGGLETOPMOST bit set.
             * One is when SWP_NOOWNERZORDER is used when changing the topmost state of a window;
             *  in this case, ZOrderByOwner2 doesn't add ownees to the psmwp list, still SetTopMost
             *  sets the bit on all the ownees.
             * Another case is when SetWindowPos gets re-entered on the same window.
             * It's too late to attempt to fix this ancient behavior (2/24/99) so let's turn off
             *  the assert for now.
             */
            if (!FSwpTopmost(pwnd)) {
                RIPMSG1(RIP_WARNING, "LinkWindow pwnd:%p is not FSwpTopmost", pwnd);
            }
        }
#endif

        Lock(&pwnd->spwndNext, pwndParent->spwndChild);
        Lock(&(pwndParent->spwndChild), pwnd);
    } else {
        if (pwndInsert == PWND_BOTTOM) {

            /*
             * Find bottom-most window.
             */
            if (((pwndInsert = pwndParent->spwndChild) == NULL) ||
                TestWF(pwndInsert, WFBOTTOMMOST))
                goto LinkTop;

            /*
             * Since we know (ahem) that there's only one bottommost window,
             * we can't possibly insert after it.  Either we're inserting
             * the bottomost window, in which case it's not in the linked
             * list currently, or we're inserting some other window.
             */

            while (pwndInsert->spwndNext != NULL) {
                if (TestWF(pwndInsert->spwndNext, WFBOTTOMMOST)) {
#if DBG
                    UserAssert(pwnd != pwndInsert->spwndNext);
                    if (TestWF(pwnd, WFBOTTOMMOST))
                        UserAssert(FALSE);
#endif
                    break;
                }

                pwndInsert = pwndInsert->spwndNext;
            }
        }

        UserAssert(pwnd != pwndInsert);
        UserAssert(pwnd != pwndInsert->spwndNext);
        UserAssert(!TestWF(pwndInsert, WFDESTROYED));
        UserAssert(!TestWF(pwnd, WEFTOPMOST) || TestWF(pwndInsert, WEFTOPMOST) || TestWF(pwnd, WFTOGGLETOPMOST) || (pwndParent != PWNDDESKTOP(pwndInsert)));
        UserAssert(pwnd->spwndParent == pwndInsert->spwndParent);

        Lock(&pwnd->spwndNext, pwndInsert->spwndNext);
        Lock(&pwndInsert->spwndNext, pwnd);
    }

    if (TestWF(pwnd, WEFLAYERED))
        TrackLayeredZorder(pwnd);

#if DBG
    VerifyWindowLink (pwnd, pwndParent, TRUE);
#endif

}


/***************************************************************************\
* xxxDestroyWindow (API)
*
* Destroy the specified window. The window passed in is not thread locked.
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 02-07-91 DavidPe      Added Win 3.1 WH_CBT support.
* 02-11-91 JimA         Added access checks.
\***************************************************************************/

BOOL xxxDestroyWindow(
    PWND pwnd)
{
    PMENUSTATE  pMenuState, pmnsEnd;
    PTHREADINFO pti = PtiCurrent();
    TL          tlpwnd;
    PWND pwndFocus;
    TL tlpwndFocus;
    TL tlpwndParent;
    BOOL fAlreadyDestroyed;
    DWORD dwDisableHooks;

    dwDisableHooks = 0;
    ThreadLockWithPti(pti, pwnd, &tlpwnd);

    /*
     * First, if this handle has been marked for destruction, that means it
     * is possible that the current thread is not its owner! (meaning we're
     * being called from a handle unlock call).  In this case, set the owner
     * to be the current thread so inter-thread send messages occur.
     */
    fAlreadyDestroyed = HMIsMarkDestroy(pwnd);
    if (fAlreadyDestroyed) {
        /*
         * UserAssert(dwInAtomicOperation > 0);
         * This Assert ensures that we are here only because of an unlock
         * on a previously destroyed window.  We BEGIN/ENDATOMICHCHECK in
         * HMDestroyUnlockedObject to ensure we don't leave the crit sect
         * unexpectedly, which gives us dwInAtomicCheck > 0.  We set
         * TIF_DISABLEHOOKS to prevent a callback in Unlock
         * However, it is currently possible destroy the same window handle
         * twice, since we don't (yet) fail to revalidate zombie handles:
         * GerardoB may change this, at which time we should probably restore
         * this Assert, and test #76902 (close winmsd.exe) again. (preventing
         * hooks in a second destroy of a zombie window should be OK) - IanJa
         */
        // UserAssert(dwInAtomicOperation > 0);

        if (HMPheFromObject(pwnd)->pOwner != pti) {
            UserAssert(PsGetCurrentThread()->Tcb.Win32Thread);
            HMChangeOwnerThread(pwnd, pti);
        }
        dwDisableHooks = pti->TIF_flags & TIF_DISABLEHOOKS;
        pti->TIF_flags |= TIF_DISABLEHOOKS;
    } else {
        /*
         * Ensure that we can destroy the window.  JIMA: no other process or thread
         * should be able to destroy any other process or thread's window.
         */
        if (pti != GETPTI(pwnd)) {
            RIPERR0(ERROR_ACCESS_DENIED,
                    RIP_WARNING,
                    "Access denied in xxxDestroyWindow");

            goto FalseReturn;
        }
    }

    /*
     * First ask the CBT hook if we can destroy this window.
     * If this object has already been destroyed OR this thread is currently
     * in cleanup mode, *do not* make any callbacks via hooks to the client
     * process.
     */
    if (!fAlreadyDestroyed && !(pti->TIF_flags & TIF_INCLEANUP) &&
            IsHooked(pti, WHF_CBT)) {
        if (xxxCallHook(HCBT_DESTROYWND, (WPARAM)HWq(pwnd), 0, WH_CBT)) {
            goto FalseReturn;
        }
    }

    /*
     * If the window we are destroying is in menu mode, end the menu
     */
    pMenuState = GetpMenuState(pwnd);
    if ((pMenuState != NULL)
            && (pwnd == pMenuState->pGlobalPopupMenu->spwndNotify)) {

        MNEndMenuStateNotify(pMenuState);
        /*
         * Signal all states to end. The window(s) will be unlocked when
         *  the menu exits; we cannot unlock it now because the menu
         *  code could fault.
         */
        pmnsEnd = pMenuState;
        do {
            UserAssert(pwnd == pMenuState->pGlobalPopupMenu->spwndNotify);
            pMenuState->fInsideMenuLoop = FALSE;
            pMenuState = pMenuState->pmnsPrev;
        } while (pMenuState != NULL) ;

        /*
         * All states have been signaled to exit, so once we callback
         *  we cannot count on pmnsEnd->pmnsPrev to be valid. Thus
         *  we simply end the current menu here and let the others go
         *  on their own. No state points to pwnd anymore so that
         *  should be OK.
         */
        if (!pmnsEnd->fModelessMenu) {
            xxxEndMenu(pmnsEnd);
        }
    }

    if (ghwndSwitch == HWq(pwnd))
        ghwndSwitch = NULL;

    if (!TestWF(pwnd, WFCHILD) && (pwnd->spwndOwner == NULL)) {

        if (TestWF(pwnd, WFHASPALETTE)) {
            xxxFlushPalette(pwnd);
        }
    }

    /*
     * Disassociate thread state if this is top level and owned by a different
     * thread. This is done to begin with so these windows z-order together.
     */
    if (pwnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME] &&
        !TestwndChild(pwnd) && pwnd->spwndOwner != NULL &&
            GETPTI(pwnd->spwndOwner) != GETPTI(pwnd)) {
        /*
         * No need to zzzDeferWinEventNotify() - there is an xxx call just below
         */
        zzzAttachThreadInput(GETPTI(pwnd), GETPTI(pwnd->spwndOwner), FALSE);
    }

    /*
     * If we are a child window without the WS_NOPARENTNOTIFY style, send
     * the appropriate notification message.
     *
     * NOTE: Although it would appear that we are illegally cramming a
     * a WORD (WM_DESTROY) and a DWORD (pwnd->spmenu) into a single LONG
     * (wParam) this isn't really the case because we first test if this
     * is a child window.  The pMenu field in a child window is really
     * the window's id and only the LOWORD is significant.
     */
    if (TestWF(pwnd, WFCHILD) && !TestWF(pwnd, WEFNOPARENTNOTIFY) &&
            pwnd->spwndParent != NULL) {

        ThreadLockAlwaysWithPti(pti, pwnd->spwndParent, &tlpwndParent);
        xxxSendMessage(pwnd->spwndParent, WM_PARENTNOTIFY,
                MAKELONG(WM_DESTROY, PTR_TO_ID(pwnd->spmenu)), (LPARAM)HWq(pwnd));
        ThreadUnlock(&tlpwndParent);
    }

    /*
     * Mark this window as beginning the destroy process.  This is necessary
     * to prevent window-management calls such as ShowWindow or SetWindowPos
     * from coming in and changing the visible-state of the window
     * once we hide it.  Otherwise, if the app attempts to make it
     * visible, then we can get our vis-rgns screwed up once we truely
     * destroy the window.
     *
     * Don't mark the mother desktop with this bit.  The xxxSetWindowPos()
     * will fail for this window, and thus possibly cause an assertion
     * in the xxxFreeWindow() call when we check for the visible-bit.
     */
    if (pwnd->spwndParent && (pwnd->spwndParent->head.rpdesk != NULL))
        SetWF(pwnd, WFINDESTROY);

    /*
     * Hide the window.
     */
    if (TestWF(pwnd, WFVISIBLE)) {
        if (TestWF(pwnd, WFCHILD)) {
            xxxShowWindow(pwnd, SW_HIDE | TEST_PUDF(PUDF_ANIMATE));
        } else {

            /*
             * Hide this window without activating anyone else.
             */
            xxxSetWindowPos(pwnd, NULL, 0, 0, 0, 0, SWP_HIDEWINDOW |
                    SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER |
                    (fAlreadyDestroyed ? SWP_DEFERDRAWING : 0));
        }

        /*
         * Under low memory conditions, the above attempt to hide could fail.
         */
        if (TestWF(pwnd, WFVISIBLE)) {
            RIPMSG0(RIP_WARNING, "xxxDestroyWindow: normal hide failed");
            SetVisible(pwnd, SV_UNSET);

            /*
             * Invalidate windows below so they redraw properly.
             */
            xxxRedrawWindow(NULL, &pwnd->rcWindow, NULL, RDW_INVALIDATE |
                    RDW_ERASE | RDW_ALLCHILDREN);

        }
    } else if (IsTrayWindow(pwnd)) {
        PostShellHookMessages(HSHELL_WINDOWDESTROYED,
                              (LPARAM)PtoHq( pwnd ));
    }

    /*
     * Destroy any owned windows.
     */
    if (!TestWF(pwnd, WFCHILD)) {
        xxxDW_DestroyOwnedWindows(pwnd);

        /*
         * And remove the window hot-key, if it has one
         */
        DWP_SetHotKey(pwnd, 0);
    }

    /*
     * If the window has already been destroyed, don't muck with
     * activation because we may already be in the middle of
     * an activation event.  Changing activation now may cause us
     * to leave our critical section while holding the display lock.
     * This will result in a deadlock if another thread gets the
     * critical section before we do and attempts to lock the
     * display.
     */
    if (!fAlreadyDestroyed) {
        PWND pwndActivate = NULL;
        TL tlpwndActivate;
        UINT cmdActivate;

        /*
         * If hiding the active window, activate someone else.
         * This call is strategically located after DestroyOwnedWindows() so we
         * don't end up activating our owner window.
         *
         * If the window is a popup, try to activate his creator not the top
         * window in the Z list.
         */
        if (pwnd == pti->pq->spwndActive) {
            if (TestWF(pwnd, WFPOPUP) && pwnd->spwndOwner) {
                pwndActivate = pwnd->spwndOwner;
                cmdActivate = AW_TRY;

            } else {
                pwndActivate = pwnd;
                cmdActivate = AW_SKIP;
            }
        } else if ((pti->pq->spwndActive == NULL) && (gpqForeground == pti->pq)) {
            pwndActivate = pwnd;
            cmdActivate = AW_SKIP;
        }

        if (pwndActivate) {
            ThreadLockAlwaysWithPti(pti, pwndActivate, &tlpwndActivate);

            if (!xxxActivateWindow(pwndActivate, cmdActivate) ||
                    ((cmdActivate == AW_SKIP) && (pwnd == pti->pq->spwndActive))) {
                if ((cmdActivate == AW_SKIP) || (pwnd == pti->pq->spwndActive)) {
                    Unlock(&pti->pq->spwndActive);
                    pwndFocus = Unlock(&pti->pq->spwndFocus);
                    if (IS_IME_ENABLED() && pwndFocus != NULL) {
                        ThreadLockAlwaysWithPti(pti, pwndFocus, &tlpwndFocus);
                        xxxFocusSetInputContext(pwndFocus, FALSE, FALSE);
                        ThreadUnlock(&tlpwndFocus);
                    }
                    if (FWINABLE() && (pti->pq == gpqForeground)) {
                        xxxWindowEvent(EVENT_OBJECT_FOCUS, NULL, OBJID_CLIENT,
                                INDEXID_CONTAINER, 0);
                        xxxWindowEvent(EVENT_SYSTEM_FOREGROUND, NULL, OBJID_WINDOW,
                                INDEXID_CONTAINER, WEF_USEPWNDTHREAD);
                    }
                    zzzInternalDestroyCaret();
                }
            }

            ThreadUnlock(&tlpwndActivate);
        }
    }

    /*
     * fix last active popup
     */
    {
        PWND pwndOwner = pwnd->spwndOwner;

        if (pwndOwner != NULL) {
            while (pwndOwner->spwndOwner != NULL) {
                pwndOwner = pwndOwner->spwndOwner;
            }

            if (pwnd == pwndOwner->spwndLastActive) {
                Lock(&(pwndOwner->spwndLastActive), pwnd->spwndOwner);
            }
        }
    }

    if (!fAlreadyDestroyed) {
    /*
     * Note we do this BEFORE telling the app the window is dying.  Note also
     * that we do NOT loop through the children generating DESTROY events.
     * DESTROY of a parent implies DESTROY of all children (see #71846 - IanJa)
     */
        if (FWINABLE() && !TestWF(pwnd, WFDESTROYED)) {
            xxxWindowEvent(EVENT_OBJECT_DESTROY, pwnd, OBJID_WINDOW, INDEXID_CONTAINER, 0);
        }

    /*
     * Send destroy messages before the WindowLockStart in case
     * he tries to destroy windows as a result.
     */
        xxxDW_SendDestroyMessages(pwnd);
    }

    /*
     * Check the owner of IME window again.
     * If thread is destroying, don't bother to check.
     */
    if (IS_IME_ENABLED() && !(pti->TIF_flags & TIF_INCLEANUP) &&
            pti->spwndDefaultIme != NULL &&
            !TestCF(pwnd, CFIME) &&
            pwnd->pcls->atomClassName != gpsi->atomSysClass[ICLS_IME]) {

        if (fAlreadyDestroyed) {
            RIPMSG2(RIP_VERBOSE, "xxxDestroyWindow: in final destruction of %#p, ime=%#p",
                    pwnd, pti->spwndDefaultIme);
        } else {
            if (!TestWF(pwnd, WFCHILD)) {
                if (ImeCanDestroyDefIME(pti->spwndDefaultIme, pwnd)) {
                    TAGMSG1(DBGTAG_IMM, "xxxDestroyWindow: destroying (1) the default IME window=%p", pti->spwndDefaultIme);
                    xxxDestroyWindow(pti->spwndDefaultIme);
                }
            }
            else if (pwnd->spwndParent != NULL) {
                if (ImeCanDestroyDefIMEforChild(pti->spwndDefaultIme, pwnd)) {
                    TAGMSG1(DBGTAG_IMM, "xxxDestroyWindow: destroying (2) the default IME window=%p", pti->spwndDefaultIme);
                    xxxDestroyWindow(pti->spwndDefaultIme);
                }
            }
        }
    }

    if ((pwnd->spwndParent != NULL) && !fAlreadyDestroyed) {

        /*
         * TestwndChild() on checks to WFCHILD bit.  Make sure this
         * window wasn't SetParent()'ed to the desktop as well.
         */
        if (TestwndChild(pwnd) && (pwnd->spwndParent != PWNDDESKTOP(pwnd)) &&
                (GETPTI(pwnd) != GETPTI(pwnd->spwndParent))) {
            /*
             * pwnd is threadlocked, so no need to DeferWinEventNotify()
             */
            CheckLock(pwnd);
            zzzAttachThreadInput(GETPTI(pwnd), GETPTI(pwnd->spwndParent), FALSE);
        }

        UnlinkWindow(pwnd, pwnd->spwndParent);
    }

    /*
     * This in intended to check for a case where we destroy the window,
     * but it's still listed as the active-window in the queue.  This
     * could cause problems in window-activation (see xxxActivateThisWindow)
     * where we attempt to activate another window and in the process, try
     * to deactivate this window (bad).
     */
#if DBG
    if (pwnd == pti->pq->spwndActive) {
        RIPMSG1(RIP_WARNING, "xxxDestroyWindow: pwnd == pti->pq->spwndActive (%#p)", pwnd);
    }
#endif

    /*
     * Set the state as destroyed so any z-ordering events will be ignored.
     * We cannot NULL out the owner field until WM_NCDESTROY is send or
     * apps like Rumba fault  (they call GetParent after every message)
     */
    SetWF(pwnd, WFDESTROYED);

    /*
     * FreeWindow performs a ThreadUnlock.
     */

    xxxFreeWindow(pwnd, &tlpwnd);

    if (fAlreadyDestroyed) {
        pti->TIF_flags = (pti->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;
    }
    return TRUE;

FalseReturn:
    if (fAlreadyDestroyed) {
        pti->TIF_flags = (pti->TIF_flags & ~TIF_DISABLEHOOKS) | dwDisableHooks;
    }
    ThreadUnlock(&tlpwnd);
    return FALSE;
}


/***************************************************************************\
* xxxDW_DestroyOwnedWindows
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
* 07-22-91 darrinm      Re-ported from Win 3.1 sources.
\***************************************************************************/

void xxxDW_DestroyOwnedWindows(
    PWND pwndParent)
{
    PWND pwnd, pwndDesktop;
    PDESKTOP pdeskParent;
    PWND pwndDefaultIme = GETPTI(pwndParent)->spwndDefaultIme;

    CheckLock(pwndParent);

    if ((pdeskParent = pwndParent->head.rpdesk) == NULL)
        return;
    pwndDesktop = pdeskParent->pDeskInfo->spwnd;

    /*
     * During shutdown, the desktop owner window will be
     * destroyed.  In this case, pwndDesktop will be NULL.
     */
    if (pwndDesktop == NULL)
        return;

    pwnd = pwndDesktop->spwndChild;

    while (pwnd != NULL) {
        if (pwnd->spwndOwner == pwndParent) {
            /*
             * We don't destroy the IME window here
             * unless the thread is doing cleanup.
             */
            if (IS_IME_ENABLED() && !(GETPTI(pwndParent)->TIF_flags & TIF_INCLEANUP) &&
                    pwnd == pwndDefaultIme) {
                Unlock(&pwnd->spwndOwner);
                pwnd = pwnd->spwndNext;
                continue;
            }

            /*
             * If the window doesn't get destroyed, set its owner to NULL.
             * A good example of this is trying to destroy a window created
             * by another thread or process, but there are other cases.
             */
            if (!xxxDestroyWindow(pwnd)) {
                Unlock(&pwnd->spwndOwner);
            }

            /*
             * Start the search over from the beginning since the app could
             * have caused other windows to be created or activation/z-order
             * changes.
             */
            pwnd = pwndDesktop->spwndChild;
        } else {
            pwnd = pwnd->spwndNext;
        }
    }
}


/***************************************************************************\
* xxxDW_SendDestroyMessages
*
* History:
* 10-20-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxDW_SendDestroyMessages(
    PWND pwnd)
{
    PWND pwndChild;
    PWND pwndNext;
    TL tlpwndNext;
    TL tlpwndChild;
    PWINDOWSTATION pwinsta;

    CheckLock(pwnd);

    /*
     * Be sure the window gets any resulting messages before being destroyed.
     */
    xxxCheckFocus(pwnd);

    pwinsta = _GetProcessWindowStation(NULL);
    if (pwinsta != NULL && pwnd == pwinsta->spwndClipOwner) {
        /*
         * Pass along the pwnd which is the reason we are dis'ing the clipboard.
         * We want to later make sure the owner is still this window after we make callbacks
         * and clear the owner
         */
        DisownClipboard(pwnd);
    }

    /*
     * Send the WM_DESTROY message.
     */
#if _DBG
    if (pwnd == PtiCurrent()->spwndDefaultIme) {
        TAGMSG2(DBGTAG_IMM, "xxxDW_SendDestroyMessages: sending WM_DESTROY message to def IME=%p, pti=%p", pwnd, PtiCurrent());
    }
#endif
    xxxSendMessage(pwnd, WM_DESTROY, 0L, 0L);

    /*
     * Now send destroy message to all children of pwnd.
     * Enumerate down (pwnd->spwndChild) and sideways (pwnd->spwndNext).
     * We do it this way because parents often assume that child windows still
     * exist during WM_DESTROY message processing.
     */
    pwndChild = pwnd->spwndChild;

    while (pwndChild != NULL) {

        pwndNext = pwndChild->spwndNext;

        ThreadLock(pwndNext, &tlpwndNext);

        ThreadLockAlways(pwndChild, &tlpwndChild);
        xxxDW_SendDestroyMessages(pwndChild);
        ThreadUnlock(&tlpwndChild);
        pwndChild = pwndNext;

        /*
         * The unlock may nuke the next window.  If so, get out.
         */
        if (!ThreadUnlock(&tlpwndNext))
            break;
    }

    xxxCheckFocus(pwnd);
}


/***************************************************************************\
* xxxFW_DestroyAllChildren
*
* History:
* 11-06-90 darrinm      Ported from Win 3.0 sources.
\***************************************************************************/

void xxxFW_DestroyAllChildren(
    PWND pwnd)
{
    PWND pwndChild;
    TL tlpwndChild;
    PTHREADINFO pti;
    PTHREADINFO ptiCurrent = PtiCurrent();

    CheckLock(pwnd);

    while (pwnd->spwndChild != NULL) {
        pwndChild = pwnd->spwndChild;

        /*
         * ThreadLock prior to the unlink in case pwndChild
         * is already marked as destroyed.
         */
        ThreadLockAlwaysWithPti(ptiCurrent, pwndChild, &tlpwndChild);

        /*
         * Propagate the VISIBLE flag. We need to do this so that
         * when a child window gets destroyed we don't try to hide it
         * if the WFVISIBLE flag is set.
         */
        if (TestWF(pwndChild, WFVISIBLE)) {
            SetVisible(pwndChild, SV_UNSET);
        }

        UnlinkWindow(pwndChild, pwnd);

        /*
         * Set the state as destroyed so any z-ordering events will be ignored.
         * We cannot NULL out the owner field until WM_NCDESTROY is send or
         * apps like Rumba fault  (they call GetParent after every message)
         */
        SetWF(pwndChild, WFDESTROYED);

        /*
         * If the window belongs to another thread, post
         * an event to let it know it should be destroyed.
         * Otherwise, free the window.
         */
        pti = GETPTI(pwndChild);
        if (pti != ptiCurrent) {
            PostEventMessage(pti, pti->pq, QEVENT_DESTROYWINDOW,
                             NULL, 0,
                             (WPARAM)HWq(pwndChild), 0);
            ThreadUnlock(&tlpwndChild);
        } else {
            /*
             * FreeWindow performs a ThreadUnlock.
             */
            xxxFreeWindow(pwndChild, &tlpwndChild);
        }
    }
}

/***************************************************************************\
* UnlockNotifyWindow
*
* Walk down a menu and unlock all notify windows.
*
* History:
* 18-May-1994 JimA      Created.
\***************************************************************************/

VOID UnlockNotifyWindow(
    PMENU pmenu)
{
    PITEM pItem;
    int   i;

    /*
     * Go down the item list and unlock submenus.
     */
    pItem = pmenu->rgItems;
    for (i = pmenu->cItems; i--; ++pItem) {

        if (pItem->spSubMenu != NULL)
            UnlockNotifyWindow(pItem->spSubMenu);
    }

    Unlock(&pmenu->spwndNotify);
}

/***************************************************************************\
* xxxFreeWindow
*
* History:
* 19-Oct-1990 DarrinM   Ported from Win 3.0 sources.
\***************************************************************************/

VOID xxxFreeWindow(
    PWND pwnd,
    PTL  ptlpwndFree)
{
    PDCE           *ppdce;
    PDCE           pdce;
    UINT           uDCERelease;
    PMENU          pmenu;
    PQMSG          pqmsg;
    PPCLS          ppcls;
    WORD           fnid;
    TL             tlpdesk;
    PWINDOWSTATION pwinsta = _GetProcessWindowStation(NULL);
    PTHREADINFO    pti  = PtiCurrent();
    PPROCESSINFO   ppi;
    PMONITOR       pMonitor;
    TL             tlpMonitor;

    UNREFERENCED_PARAMETER(ptlpwndFree);

    CheckLock(pwnd);

    /*
     * If the pwnd is any of the global shell-related windows,
     * then we need to unlock them from the deskinfo.
     */
    if (pwnd->head.rpdesk != NULL) {
        if (pwnd == pwnd->head.rpdesk->pDeskInfo->spwndShell)
            Unlock(&pwnd->head.rpdesk->pDeskInfo->spwndShell);
        if (pwnd == pwnd->head.rpdesk->pDeskInfo->spwndBkGnd)
            Unlock(&pwnd->head.rpdesk->pDeskInfo->spwndBkGnd);
        if (pwnd == pwnd->head.rpdesk->pDeskInfo->spwndTaskman)
            Unlock(&pwnd->head.rpdesk->pDeskInfo->spwndTaskman);
        if (pwnd == pwnd->head.rpdesk->pDeskInfo->spwndProgman)
            Unlock(&pwnd->head.rpdesk->pDeskInfo->spwndProgman);
        if (TestWF(pwnd,WFSHELLHOOKWND)) {
            _DeregisterShellHookWindow(pwnd);
        }

        if (TestWF(pwnd, WFMSGBOX)) {
            pwnd->head.rpdesk->pDeskInfo->cntMBox--;
            ClrWF(pwnd, WFMSGBOX);
        }
    }

    /*
     * First, if this handle has been marked for destruction, that means it
     * is possible that the current thread is not its owner! (meaning we're
     * being called from a handle unlock call).  In this case, set the owner
     * to be the current thread so inter-thread send messages don't occur.
     */
    if (HMIsMarkDestroy(pwnd))
        HMChangeOwnerThread(pwnd, pti);

    /*
     * Blow away the children.
     *
     * DestroyAllChildren() will still destroy windows created by other
     * threads! This needs to be looked at more closely: the ultimate
     * "right" thing to do is not to destroy these windows but just
     * unlink them.
     */
    xxxFW_DestroyAllChildren(pwnd);
    xxxSendMessage(pwnd, WM_NCDESTROY, 0, 0L);

    pMonitor = _MonitorFromWindow(pwnd, MONITOR_DEFAULTTOPRIMARY);
    ThreadLockAlwaysWithPti(pti, pMonitor, &tlpMonitor);
    xxxRemoveFullScreen(pwnd, pMonitor);
    ThreadUnlock(&tlpMonitor);

    /*
     * If this is one of the built in controls which hasn't been cleaned
     * up yet, do it now. If it lives in the kernel, call the function
     * directly, otherwise call back to the client. Even if the control
     * is sub- or super-classed, use the window procs associated with
     * the function id.
     */
    fnid = GETFNID(pwnd);
    if ((fnid >= FNID_WNDPROCSTART) && !(pwnd->fnid & FNID_CLEANEDUP_BIT)) {

       if (fnid <= FNID_WNDPROCEND) {

           FNID(fnid)(pwnd, WM_FINALDESTROY, 0, 0, 0);

       } else if (fnid <= FNID_CONTROLEND && !(pti->TIF_flags & TIF_INCLEANUP)) {

           CallClientWorkerProc(pwnd,
                                    WM_FINALDESTROY,
                                    0,
                                    0,
                                    (PROC)FNID_TO_CLIENT_PFNWORKER(fnid));
       }

       pwnd->fnid |= FNID_CLEANEDUP_BIT;
    }

    pwnd->fnid |= FNID_DELETED_BIT;

    /*
     * Check to clear the most recently active window in owned list.
     */
    if (pwnd->spwndOwner && (pwnd->spwndOwner->spwndLastActive == pwnd)) {
        Lock(&(pwnd->spwndOwner->spwndLastActive), pwnd->spwndOwner);
    }

    /*
     * The windowstation may be NULL if we are destroying a desktop
     * or windowstation.  If this is the case, this thread will not
     * be using the clipboard.
     */
    if (pwinsta != NULL) {

        if (pwnd == pwinsta->spwndClipOpen) {
            Unlock(&pwinsta->spwndClipOpen);
            pwinsta->ptiClipLock = NULL;
        }

        if (pwnd == pwinsta->spwndClipViewer) {
            Unlock(&pwinsta->spwndClipViewer);
        }
    }

    if (IS_IME_ENABLED() && pwnd == pti->spwndDefaultIme)
        Unlock(&pti->spwndDefaultIme);

    if (pwnd == pti->pq->spwndFocus)
        Unlock(&pti->pq->spwndFocus);

    if (pwnd == pti->pq->spwndActivePrev)
        Unlock(&pti->pq->spwndActivePrev);

    if (pwnd == gspwndActivate)
        Unlock(&gspwndActivate);

    if (pwnd->head.rpdesk != NULL) {

        if (pwnd == pwnd->head.rpdesk->spwndForeground)
            Unlock(&pwnd->head.rpdesk->spwndForeground);

        if (pwnd == pwnd->head.rpdesk->spwndTray)
            Unlock(&pwnd->head.rpdesk->spwndTray);

        if (pwnd == pwnd->head.rpdesk->spwndTrack) {
            /*
             * Remove tooltip, if any
             */
            if (GETPDESK(pwnd)->dwDTFlags & DF_TOOLTIPSHOWING) {
                PWND pwndTooltip = GETPDESK(pwnd)->spwndTooltip;
                TL tlpwndTooltip;

                ThreadLockAlways(pwndTooltip, &tlpwndTooltip);
                xxxResetTooltip((PTOOLTIPWND)pwndTooltip);
                ThreadUnlock(&tlpwndTooltip);
            }

            Unlock(&pwnd->head.rpdesk->spwndTrack);
            pwnd->head.rpdesk->dwDTFlags &= ~DF_MOUSEMOVETRK;
        }
    }

    if (pwnd == pti->pq->spwndCapture)
        xxxReleaseCapture();

    /*
     * This window won't be needing any more input.
     */
    if (pwnd == gspwndMouseOwner)
        Unlock(&gspwndMouseOwner);

    /*
     * It also won't have any mouse cursors over it.
     */
    if (pwnd == gspwndCursor)
        Unlock(&gspwndCursor);

    DestroyWindowsTimers(pwnd);
    DestroyWindowsHotKeys(pwnd);

    /*
     * Make sure this window has no pending sent messages.
     */
    ClearSendMessages(pwnd);

    /*
     * Remove the associated GDI sprite.
     */
    if (TestWF(pwnd, WEFLAYERED)) {
        UnsetLayeredWindow(pwnd);
    }

#ifdef REDIRECTION
    if (TestWF(pwnd, WEFREDIRECTED)) {
        UnsetRedirectedWindow(pwnd);
    }
#endif // REDIRECTION

    /*
     * Blow away any update region lying around.
     */
    if (NEEDSPAINT(pwnd)) {

        DecPaintCount(pwnd);

        DeleteMaybeSpecialRgn(pwnd->hrgnUpdate);
        pwnd->hrgnUpdate = NULL;
        ClrWF(pwnd, WFINTERNALPAINT);
    }

    /*
     * Decrememt queue's syncpaint count if necessary.
     */
    if (NEEDSSYNCPAINT(pwnd)) {
        ClrWF(pwnd, WFSENDNCPAINT);
        ClrWF(pwnd, WFSENDERASEBKGND);
    }

    /*
     * Clear both flags to ensure that the window is removed
     * from the hung redraw list.
     */
    ClearHungFlag(pwnd, WFREDRAWIFHUNG);
    ClearHungFlag(pwnd, WFREDRAWFRAMEIFHUNG);

    /*
     * If there is a WM_QUIT message in this app's message queue, call
     * PostQuitMessage() (this happens if the app posts itself a quit message.
     * WinEdit2.0 posts a quit to a window while receiving the WM_DESTROY
     * for that window - it works because we need to do a PostQuitMessage()
     * automatically for this thread.
     */
    if (pti->mlPost.pqmsgRead != NULL) {

        /*
         * try to get rid of WM_DDE_ACK too.
         */
        if ((pqmsg = FindQMsg(pti,
                              &(pti->mlPost),
                              pwnd,
                              WM_QUIT,
                              WM_QUIT, TRUE)) != NULL) {

            _PostQuitMessage((int)pqmsg->msg.wParam);
        }
    }

    if (!TestwndChild(pwnd) && pwnd->spmenu != NULL) {
        pmenu = (PMENU)pwnd->spmenu;
        if (UnlockWndMenu(pwnd, &pwnd->spmenu))
            _DestroyMenu(pmenu);
    }

    if (pwnd->spmenuSys != NULL) {
        pmenu = (PMENU)pwnd->spmenuSys;
        if (pmenu != pwnd->head.rpdesk->spmenuDialogSys) {
            if (UnlockWndMenu(pwnd, &pwnd->spmenuSys)) {
                _DestroyMenu(pmenu);
            }
        } else {
            UnlockWndMenu(pwnd, &pwnd->spmenuSys);
        }
    }

    /*
     * If it was using either of the desktop system menus, unlock it
     */
    if (pwnd->head.rpdesk != NULL) {
        if (pwnd->head.rpdesk->spmenuSys != NULL &&
                pwnd == pwnd->head.rpdesk->spmenuSys->spwndNotify) {

            UnlockNotifyWindow(pwnd->head.rpdesk->spmenuSys);
        } else if (pwnd->head.rpdesk->spmenuDialogSys != NULL &&
                pwnd == pwnd->head.rpdesk->spmenuDialogSys->spwndNotify) {

            UnlockNotifyWindow(pwnd->head.rpdesk->spmenuDialogSys);
        }

    }


    /*
     * Tell Gdi that the window is going away.
     */
    if (gcountPWO != 0) {
        PVOID pwo = InternalRemoveProp(pwnd, PROP_WNDOBJ, TRUE);
        if (pwo != NULL) {
            GreLockDisplay(gpDispInfo->hDev);
            GreDeleteWnd(pwo);
            gcountPWO--;
            GreUnlockDisplay(gpDispInfo->hDev);
        }
    }

#ifdef HUNGAPP_GHOSTING

    /*
     * RemoveGhost handles the case when pwnd is the hung window that has a
     * corresponding ghost window and the case when pwnd is the ghost itself.
     */
    RemoveGhost(pwnd);

#endif // HUNGAPP_GHOSTING

    /*
     * Scan the DC cache to find any DC's for this window.  If any are there,
     * then invalidate them.  We don't need to worry about calling SpbCheckDC
     * because the window has been hidden by this time.
     */
    for (ppdce = &gpDispInfo->pdceFirst; *ppdce != NULL; ) {

        pdce = *ppdce;
        if (pdce->DCX_flags & DCX_INVALID) {
            goto NextEntry;
        }

        if ((pdce->pwndOrg == pwnd) || (pdce->pwndClip == pwnd)) {

            if (!(pdce->DCX_flags & DCX_CACHE)) {

                if (TestCF(pwnd, CFCLASSDC)) {

                    GreLockDisplay(gpDispInfo->hDev);

                    if (pdce->DCX_flags & (DCX_EXCLUDERGN | DCX_INTERSECTRGN))
                        DeleteHrgnClip(pdce);

                    MarkDCEInvalid(pdce);
                    pdce->pwndOrg  = NULL;
                    pdce->pwndClip = NULL;
                    pdce->hrgnClip = NULL;

                    /*
                     * Remove the vis rgn since it is still owned - if we did
                     * not, gdi would not be able to clean up properly if the
                     * app that owns this vis rgn exist while the vis rgn is
                     * still selected.
                     */
                    GreSelectVisRgn(pdce->hdc, NULL, SVR_DELETEOLD);
                    GreUnlockDisplay(gpDispInfo->hDev);

                } else if (TestCF(pwnd, CFOWNDC)) {
                    DestroyCacheDC(ppdce, pdce->hdc);
                } else {
                    UserAssert(FALSE);
                }

            } else {

                /*
                 * If the DC is checked out, release it before
                 * we invalidate.  Note, that if this process is exiting
                 * and it has a dc checked out, gdi is going to destroy that
                 * dc.  We need to similarly remove that dc from the dc cache.
                 * This is not done here, but in the exiting code.
                 *
                 * The return for ReleaseDC() could fail, which would
                 * indicate a delayed-free (DCE_NUKE).
                 */
                uDCERelease = DCE_RELEASED;

                if (pdce->DCX_flags & DCX_INUSE) {
                    uDCERelease = ReleaseCacheDC(pdce->hdc, FALSE);
                } else if (!GreSetDCOwner(pdce->hdc, OBJECT_OWNER_NONE)) {
                    uDCERelease = DCE_NORELEASE;
                }

                if (uDCERelease != DCE_FREED) {

                    if (uDCERelease == DCE_NORELEASE) {

                        /*
                         * We either could not release this dc or could not set
                         * its owner. In either case it means some other thread
                         * is actively using it. Since it is not too useful if
                         * the window it is calculated for is gone, mark it as
                         * INUSE (so we don't give it out again) and as
                         * DESTROYTHIS (so we just get rid of it since it is
                         * easier to do this than to release it back into the
                         * cache). The W32PF_OWNERDCCLEANUP bit means "look for
                         * DESTROYTHIS flags and destroy that dc", and the bit
                         * gets looked at in various strategic execution paths.
                         */
                        pdce->DCX_flags = DCX_DESTROYTHIS | DCX_INUSE | DCX_CACHE;
                        pti->ppi->W32PF_Flags |= W32PF_OWNDCCLEANUP;

                    } else {

                        /*
                         * We either released the DC or changed its owner
                         * successfully.  Mark the entry as invalid so it can
                         * be given out again.
                         */
                        MarkDCEInvalid(pdce);
                        pdce->hrgnClip = NULL;
                    }

                    /*
                     * We shouldn't reference this window anymore. Setting
                     * these to NULL here will make sure that even if we were
                     * not able to release the DC here, we won't return this
                     * window from one of the DC matching functions.
                     */
                    pdce->pwndOrg  = NULL;
                    pdce->pwndClip = NULL;

                    /*
                     * Remove the visrgn since it is still owned - if we did
                     * not, gdi would not be able to clean up properly if the
                     * app that owns this visrgn exist while the visrgn is
                     * still selected.
                     */
                    GreLockDisplay(gpDispInfo->hDev);
                    GreSelectVisRgn(pdce->hdc, NULL, SVR_DELETEOLD);
                    GreUnlockDisplay(gpDispInfo->hDev);
                }
            }
        }

        /*
         * Step to the next DC.  If the DC was deleted, there
         * is no need to calculate address of the next entry.
         */
        if (pdce == *ppdce)
NextEntry:
            ppdce = &pdce->pdceNext;
    }

    /*
     * Clean up the spb that may still exist - like child window spb's.
     */
    if (pwnd == gspwndLockUpdate) {
        FreeSpb(FindSpb(pwnd));
        Unlock(&gspwndLockUpdate);
        gptiLockUpdate = NULL;
    }

    if (TestWF(pwnd, WFHASSPB)) {
        FreeSpb(FindSpb(pwnd));
    }

    /*
     * Blow away the window clipping region. If the window is maximized, don't
     * blow away the monitor region. If the window is the desktop, don't blow
     * away the screen region.
     */
    if (    pwnd->hrgnClip != NULL &&
            !TestWF(pwnd, WFMAXFAKEREGIONAL) &&
            GETFNID(pwnd) != FNID_DESKTOP) {

        GreDeleteObject(pwnd->hrgnClip);
        pwnd->hrgnClip = NULL;
    }

    /*
     * Clean up any memory allocated for scroll bars...
     */
    if (pwnd->pSBInfo) {
        DesktopFree(pwnd->head.rpdesk, (HANDLE)(pwnd->pSBInfo));
        pwnd->pSBInfo = NULL;
    }

    /*
     * Free any callback handles associated with this window.
     * This is done outside of DeleteProperties because of the special
     * nature of callback handles as opposed to normal memory handles
     * allocated for a thread.
     */

    /*
     * Blow away the title
     */
    if (pwnd->strName.Buffer != NULL) {
        DesktopFree(pwnd->head.rpdesk, pwnd->strName.Buffer);
        pwnd->strName.Buffer = NULL;
        pwnd->strName.Length = 0;
    }

    /*
     * Blow away any properties connected to the window.
     */
    if (pwnd->ppropList != NULL) {
        TL tlpDdeConv;
        PDDECONV pDdeConv;
        PDDEIMP pddei;

        /*
         * Get rid of any icon properties.
         */
        DestroyWindowSmIcon(pwnd);
        InternalRemoveProp(pwnd, MAKEINTATOM(gpsi->atomIconProp), PROPF_INTERNAL);

        pDdeConv = (PDDECONV)_GetProp(pwnd, PROP_DDETRACK, PROPF_INTERNAL);
        if (pDdeConv != NULL) {
            ThreadLockAlwaysWithPti(pti, pDdeConv, &tlpDdeConv);
            xxxDDETrackWindowDying(pwnd, pDdeConv);
            ThreadUnlock(&tlpDdeConv);
        }
        pddei = (PDDEIMP)InternalRemoveProp(pwnd, PROP_DDEIMP, PROPF_INTERNAL);
        if (pddei != NULL) {
            pddei->cRefInit = 0;
            if (pddei->cRefConv == 0) {
                /*
                 * If this is not 0 it is referenced by one or more DdeConv
                 * structures so DON'T free it yet!
                 */
                UserFreePool(pddei);
            }
        }
    }

    /*
     * Unlock everything that the window references.
     * After we have sent the WM_DESTROY and WM_NCDESTROY message we
     * can unlock & NULL the owner field so no other windows get z-ordered
     * relative to this window.  Rhumba faults if we NULL it before the
     * destroy.  (It calls GetParent after every message).
     *
     * We special-case the spwndParent window.  In this case, if the
     * window being destroyed is a desktop window, unlock the parent.
     * Otherwise, we lock in the desktop-window as the parent so that
     * if we aren't freed in this function, we will ensure that we
     * won't fault when doing things like clipping-calculations.  We'll
     * unlock this once we know we're truly going to free this window.
     */
    if (pwnd->head.rpdesk != NULL &&
            pwnd != pwnd->head.rpdesk->pDeskInfo->spwnd)
        Lock(&pwnd->spwndParent, pwnd->head.rpdesk->pDeskInfo->spwnd);
    else
        Unlock(&pwnd->spwndParent);

    Unlock(&pwnd->spwndChild);
    Unlock(&pwnd->spwndOwner);
    Unlock(&pwnd->spwndLastActive);

    /*
     * Decrement the Window Reference Count in the Class structure.
     */
    DereferenceClass(pwnd);

    /*
     * Mark the object for destruction before this final unlock. This way
     * the WM_FINALDESTROY will get sent if this is the last thread lock.
     * We're currently destroying this window, so don't allow unlock recursion
     * at this point (this is what HANDLEF_INDESTROY will do for us).
     */
    HMMarkObjectDestroy(pwnd);
    HMPheFromObject(pwnd)->bFlags |= HANDLEF_INDESTROY;

    /*
     * Unlock the window... This shouldn't return FALSE because HANDLEF_DESTROY
     * is set, but just in case...  if it isn't around anymore, return because
     * pwnd is invalid.
     */
    if (!ThreadUnlock(ptlpwndFree))
        return;

    /*
     * Try to free the object.  The object won't free if it is locked - but
     * it will be marked for destruction.  If the window is locked, change
     * it's wndproc to xxxDefWindowProc().
     *
     * HMMarkObjectDestroy() will clear the HANDLEF_INDESTROY flag if the
     * object isn't about to go away (so it can be destroyed again!)
     */
    pwnd->pcls = NULL;
    if (HMMarkObjectDestroy(pwnd)) {

        /*
         * Delete the window's property list. Wait until now in case some
         * thread keeps a property pointer around across a callback.
         */
        if (pwnd->ppropList != NULL) {
            DeleteProperties(pwnd);
        }

#if DBG
        /*
         * If we find the window is visible at the time we free it, then
         * somehow the app was made visible on a callback (we hide it
         * during xxxDestroyWindow().  This screws up our vis-window
         * count for the thread, so we need to assert it.
         */
        if (TestWF(pwnd, WFINDESTROY) && TestWF(pwnd, WFVISIBLE))
            RIPMSG1(RIP_ERROR, "xxxFreeWindow: Window should not be visible (pwnd == %#p)", pwnd);
#endif

        pti->cWindows--;

        /*
         * Since we're freeing the memory for this window, we need
         * to unlock the parent (which is the desktop for zombie windows).
         */
        Unlock(&pwnd->spwndParent);

        ThreadLockDesktop(pti, pwnd->head.rpdesk, &tlpdesk, LDLT_FN_FREEWINDOW);
        HMFreeObject(pwnd);
        ThreadUnlockDesktop(pti, &tlpdesk, LDUT_FN_FREEWINDOW);
        return;
    }

    /*
     * Turn this into an object that the app won't see again - turn
     * it into an icon title window - the window is still totally
     * valid and useable by any structures that has this window locked.
     */
    pwnd->lpfnWndProc = xxxDefWindowProc;
    if (pwnd->head.rpdesk)
        ppi = pwnd->head.rpdesk->rpwinstaParent->pTerm->ptiDesktop->ppi;
    else
        ppi = PpiCurrent();
    ppcls = GetClassPtr(gpsi->atomSysClass[ICLS_ICONTITLE], ppi, hModuleWin);

    UserAssert(ppcls);
    pwnd->pcls = *ppcls;

    /*
     * Since pwnd is marked as destroyed, there should be no client-side
     * code which can validate it.  So we do not need to search for a clone
     * class of the right desktop -- just use the base class and bump the
     * WndReferenceCount.  This also helps if we are in a low-memory situation
     * and cannot alloc another clone.
     */

    pwnd->pcls->cWndReferenceCount++;

    SetWF(pwnd, WFSERVERSIDEPROC);

    /*
     * Clear the palette bit so that WM_PALETTECHANGED will not be sent
     * again when the window is finally destroyed.
     */
    ClrWF(pwnd, WFHASPALETTE);

    /*
     * Clear its child bits so no code assumes that if the child bit
     * is set, it has a parent. Change spmenu to NULL - it is only
     * non-zero if this was child.
     */
    ClrWF(pwnd, WFTYPEMASK);
    SetWF(pwnd, WFTILED);
    pwnd->spmenu = NULL;
}

/***************************************************************************\
* UnlinkWindow
*
* History:
* 19-Oct-1990 DarrinM   Ported from Win 3.0 sources.
\***************************************************************************/

VOID UnlinkWindow(
    PWND pwndUnlink,
    PWND pwndParent)
{
    PWND pwnd;

    pwnd = pwndParent->spwndChild;

    if (pwnd == pwndUnlink) {
        Lock(&(pwndParent->spwndChild), pwndUnlink->spwndNext);
        Unlock(&pwndUnlink->spwndNext);

#if DBG
        VerifyWindowLink (pwnd, pwndParent, FALSE);
#endif

        return;
    }

    while (pwnd != NULL) {
        UserAssert(pwnd->spwndParent == pwndParent);
        if (pwnd->spwndNext == pwndUnlink) {
            Lock(&(pwnd->spwndNext), pwndUnlink->spwndNext);
            Unlock(&pwndUnlink->spwndNext);

#if DBG
            VerifyWindowLink (pwnd, pwndParent, FALSE);
#endif
            return;
        }

        pwnd = pwnd->spwndNext;
    }

    /*
     * We should never get here unless the window isn't in the list!
     */
    RIPMSG1(RIP_WARNING,
          "Unlinking previously unlinked window %#p\n",
          pwndUnlink);

#if DBG
    VerifyWindowLink (pwnd, pwndParent, FALSE);
#endif

    return;
}

/***************************************************************************\
* DestroyCacheDCEntries
*
* Destroys all cache dc entries currently in use by this thread.
*
* 24-Feb-1992 ScottLu   Created.
\***************************************************************************/

VOID DestroyCacheDCEntries(
    PTHREADINFO pti)
{
    PDCE *ppdce;
    PDCE pdce;
    /*
     * Before any window destruction occurs, we need to destroy any dcs
     * in use in the dc cache.  When a dc is checked out, it is marked owned,
     * which makes gdi's process cleanup code delete it when a process
     * goes away.  We need to similarly destroy the cache entry of any dcs
     * in use by the exiting process.
     */
    for (ppdce = &gpDispInfo->pdceFirst; *ppdce != NULL; ) {

        /*
         * If the dc owned by this thread, remove it from the cache.  Because
         * DestroyCacheEntry destroys gdi objects, it is important that
         * USER be called first in process destruction ordering.
         *
         * Only destroy this dc if it is a cache dc, because if it is either
         * an owndc or a classdc, it will be destroyed for us when we destroy
         * the window (for owndcs) or destroy the class (for classdcs).
         */
        pdce = *ppdce;
        if (pti == pdce->ptiOwner) {

            if (pdce->DCX_flags & DCX_CACHE)
                DestroyCacheDC(ppdce, pdce->hdc);
        }

        /*
         * Step to the next DC.  If the DC was deleted, there
         * is no need to calculate address of the next entry.
         */
        if (pdce == *ppdce)
            ppdce = &pdce->pdceNext;
    }
}

/***************************************************************************\
* PatchThreadWindows
*
* This patches a thread's windows so that their window procs point to
* server only windowprocs. This is used for cleanup so that app aren't
* called back while the system is cleaning up after them.
*
* 24-Feb-1992 ScottLu   Created.
\***************************************************************************/

VOID PatchThreadWindows(
    PTHREADINFO pti)
{
    PHE  pheT;
    PHE  pheMax;
    PWND pwnd;

    /*
     * First do any preparation work: windows need to be "patched" so that
     * their window procs point to server only windowprocs, for example.
     */
    pheMax = &gSharedInfo.aheList[giheLast];
    for (pheT = gSharedInfo.aheList; pheT <= pheMax; pheT++) {

        /*
         * Make sure this object is a window, it hasn't been marked for
         * destruction, and that it is owned by this thread.
         */
        if (pheT->bType != TYPE_WINDOW)
            continue;

        if (pheT->bFlags & HANDLEF_DESTROY)
            continue;

        if ((PTHREADINFO)pheT->pOwner != pti)
            continue;

        /*
         * don't patch the shared menu window
         */
        if (pti->rpdesk && (PHEAD)pti->rpdesk->spwndMenu == pheT->phead) {

            ((PTHROBJHEAD)pheT->phead)->pti = pti->rpdesk->pDeskInfo->spwnd->head.pti;
            pheT->pOwner = pti->rpdesk->pDeskInfo->spwnd->head.pti;

            continue;
        }

        /*
         * Don't patch the window based on the class it was created from -
         * because apps can sometimes sub-class a class - make a random class,
         * then call ButtonWndProc with windows of that class by using
         * the CallWindowProc() api.  So patch the wndproc based on what
         * wndproc this window has been calling.
         */
        pwnd = (PWND)pheT->phead;

        if ((pwnd->fnid >= (WORD)FNID_WNDPROCSTART) &&
            (pwnd->fnid <= (WORD)FNID_WNDPROCEND)) {

            pwnd->lpfnWndProc = STOCID(pwnd->fnid);

            if (pwnd->lpfnWndProc == NULL)
                pwnd->lpfnWndProc = xxxDefWindowProc;

        } else {

            pwnd->lpfnWndProc = xxxDefWindowProc;
        }

        /*
         * This is a server side window now...
         */
        SetWF(pwnd, WFSERVERSIDEPROC);
        ClrWF(pwnd, WFANSIPROC);
    }
}
