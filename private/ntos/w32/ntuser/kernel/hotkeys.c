/****************************** Module Header ******************************\
* Module Name: hotkeys.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the core functions of hotkey processing.
*
* History:
* 12-04-90 DavidPe      Created.
* 02-12-91 JimA         Added access checks
* 13-Feb-1991 mikeke    Added Revalidation code (None)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* InitSystemHotKeys
*
* Called from InitWindows(), this routine registers the default system
* hotkeys.
*
* History:
* 12-04-90 DavidPe      Created.
\***************************************************************************/
VOID SetDebugHotKeys()
{
    UINT VkDebug;

    VkDebug = FastGetProfileDwordW(NULL, PMAP_AEDEBUG, L"UserDebuggerHotkey", 0);
    if (VkDebug == 0) {
        if (ENHANCED_KEYBOARD(gKeyboardInfo.KeyboardIdentifier)) {
            VkDebug = VK_F12;
        } else {
            VkDebug = VK_SUBTRACT;
        }
    } else {
        UserAssert((0xFFFFFF00 & VkDebug) == 0);
    }

    _UnregisterHotKey(PWND_INPUTOWNER, IDHOT_DEBUG);
    _UnregisterHotKey(PWND_INPUTOWNER, IDHOT_DEBUGSERVER);

    _RegisterHotKey(PWND_INPUTOWNER, IDHOT_DEBUG, 0, VkDebug);
    _RegisterHotKey(PWND_INPUTOWNER, IDHOT_DEBUGSERVER, MOD_SHIFT, VkDebug);
}


/***************************************************************************\
* DestroyThreadsHotKeys
*
* History:
* 26-Feb-1991 mikeke    Created.
\***************************************************************************/

VOID DestroyThreadsHotKeys()
{
    PHOTKEY *pphk;
    PHOTKEY phk;
    PTHREADINFO ptiCurrent = PtiCurrent();
    pphk = &gphkFirst;
    while (*pphk) {
        if ((*pphk)->pti == ptiCurrent) {
            phk = *pphk;
            *pphk = (*pphk)->phkNext;

            /*
             * Unlock the object stored here.
             */
            if ((phk->spwnd != PWND_FOCUS) && (phk->spwnd != PWND_INPUTOWNER)) {
                Unlock(&phk->spwnd);
            }

            UserFreePool(phk);
        } else {
            pphk = &((*pphk)->phkNext);
        }
    }
}


/***************************************************************************\
* DestroyWindowsHotKeys
*
* Called from xxxFreeWindow.
* Hotkeys not unregistered properly by the app must be destroyed when the
* window is destroyed.  Keeps things clean (problems with bad apps #5553)
*
* History:
* 23-Sep-1992 IanJa     Created.
\***************************************************************************/

VOID DestroyWindowsHotKeys(
    PWND pwnd)
{
    PHOTKEY *pphk;
    PHOTKEY phk;
    pphk = &gphkFirst;
    while (*pphk) {
        if ((*pphk)->spwnd == pwnd) {
            phk = *pphk;
            *pphk = (*pphk)->phkNext;

            Unlock(&phk->spwnd);
            UserFreePool(phk);
        } else {
            pphk = &((*pphk)->phkNext);
        }
    }
}


/***************************************************************************\
* _RegisterHotKey (API)
*
* This API registers the hotkey specified.  If the specified key sequence
* has already been registered we return FALSE.  If the specified hwnd
* and id have already been registered, fsModifiers and vk are reset for
* the HOTKEY structure.
*
* History:
* 12-04-90 DavidPe      Created.
* 02-12-91 JimA         Added access check
\***************************************************************************/
BOOL _RegisterHotKey(
    PWND pwnd,
    int id,
    UINT fsModifiers,
    UINT vk)
{
    PHOTKEY         phk;
    BOOL            fKeysExist;
    PTHREADINFO     ptiCurrent;
    BOOL            bSAS = FALSE;
    WORD            wFlags;

    wFlags = fsModifiers & MOD_SAS;
    fsModifiers &= ~MOD_SAS;

    ptiCurrent = PtiCurrent();

    /*
     * Blow it off if the caller is not the windowstation init thread
     * and doesn't have the proper access rights
     */
    if (PsGetCurrentProcess() != gpepCSRSS) {
        if (grpWinStaList && !CheckWinstaWriteAttributesAccess()) {
            return FALSE;
        }
    }

    /*
     * If VK_PACKET is specified, just bail out, since VK_PACKET is
     * not a real keyboard input.
     */
    if (vk == VK_PACKET) {
        return FALSE;
    }

    /*
     * If this is the SAS check that winlogon is the one registering it.
     */
    if (wFlags & MOD_SAS) {
        if (PsGetCurrentProcess()->UniqueProcessId == gpidLogon) {
            bSAS = TRUE;
        }
    }

    /*
     * Can't register hotkey for a window of another queue.
     * Return FALSE in this case.
     */
    if ((pwnd != PWND_FOCUS) && (pwnd != PWND_INPUTOWNER)) {
        if (GETPTI(pwnd) != ptiCurrent) {
            RIPERR0(ERROR_WINDOW_OF_OTHER_THREAD, RIP_VERBOSE, "");
            return FALSE;
        }
    }

    phk = FindHotKey(ptiCurrent, pwnd, id, fsModifiers, vk, FALSE, &fKeysExist);

    /*
     * If the keys have already been registered, return FALSE.
     */
    if (fKeysExist) {
        RIPERR0(ERROR_HOTKEY_ALREADY_REGISTERED, RIP_WARNING, "Hotkey already exists");
        return FALSE;
    }

    if (phk == NULL) {
        /*
         * This hotkey doesn't exist yet.
         */
        phk = (PHOTKEY)UserAllocPool(sizeof(HOTKEY), TAG_HOTKEY);

        /*
         * If the allocation failed, bail out.
         */
        if (phk == NULL) {
            return FALSE;
        }

        phk->pti = ptiCurrent;

        if ((pwnd != PWND_FOCUS) && (pwnd != PWND_INPUTOWNER)) {
            phk->spwnd = NULL;
            Lock(&phk->spwnd, pwnd);
        } else {
            phk->spwnd = pwnd;
        }
        phk->fsModifiers = (WORD)fsModifiers;
        phk->wFlags = wFlags;

        phk->vk = vk;
        phk->id = id;

        /*
         * Link the new hotkey to the front of the list.
         */
        phk->phkNext = gphkFirst;
        gphkFirst = phk;

    } else {

        /*
         * Hotkey already exists, reset the keys.
         */
        phk->fsModifiers = (WORD)fsModifiers;
        phk->wFlags = wFlags;
        phk->vk = vk;
    }

    if (bSAS) {
        /*
         * store the SAS on the terminal.
         */
        gvkSAS = vk;
        gfsSASModifiers = fsModifiers;
    }

    return TRUE;
}


/***************************************************************************\
* _UnregisterHotKey (API)
*
* This API will 'unregister' the specified hwnd/id hotkey so that the
* WM_HOTKEY message will not be generated for it.
*
* History:
* 12-04-90 DavidPe      Created.
\***************************************************************************/

BOOL _UnregisterHotKey(
    PWND pwnd,
    int id)
{
    PHOTKEY phk;
    BOOL fKeysExist;
    PTHREADINFO  ptiCurrent = PtiCurrent();
    phk = FindHotKey(ptiCurrent, pwnd, id, 0, 0, TRUE, &fKeysExist);

    /*
     * No hotkey to unregister, return FALSE.
     */
    if (phk == NULL) {
        RIPERR0(ERROR_HOTKEY_NOT_REGISTERED, RIP_VERBOSE, "");
        return FALSE;
    }

    return TRUE;
}


/***************************************************************************\
* FindHotKey
*
* Both RegisterHotKey() and UnregisterHotKey() call this function to search
* for hotkeys that already exist.  If a HOTKEY is found that matches
* fsModifiers and vk, *pfKeysExist is set to TRUE.  If a HOTKEY is found that
* matches pwnd and id, a pointer to it is returned.
*
* If fUnregister is TRUE, we remove the HOTKEY from the list if we find
* one that matches pwnd and id and return (PHOTKEY)1.
*
* History:
* 12-04-90 DavidPe      Created.
\***************************************************************************/
PHOTKEY FindHotKey(
    PTHREADINFO ptiCurrent,
    PWND pwnd,
    int id,
    UINT fsModifiers,
    UINT vk,
    BOOL fUnregister,
    PBOOL pfKeysExist)
{
    PHOTKEY phk, phkRet, phkPrev;

    /*
     * Initialize out 'return' values.
     */
    *pfKeysExist = FALSE;
    phkRet = NULL;

    phk = gphkFirst;

    while (phk) {

        /*
         * If all this matches up then we've found it.
         */
        if ((phk->pti == ptiCurrent) && (phk->spwnd == pwnd) && (phk->id == id)) {
            if (fUnregister) {

                /*
                 * Unlink the HOTKEY from the list.
                 */
                if (phk == gphkFirst) {
                    gphkFirst = phk->phkNext;
                } else {
                    phkPrev->phkNext = phk->phkNext;
                }

                if ((pwnd != PWND_FOCUS) && (pwnd != PWND_INPUTOWNER)) {
                    Unlock(&phk->spwnd);
                }
                UserFreePool((PVOID)phk);

                return((PHOTKEY)1);
            }
            phkRet = phk;
        }

        /*
         * If the key is already registered, set the exists flag so
         * the app knows it can't use this hotkey sequence.
         */
        if ((phk->fsModifiers == (WORD)fsModifiers) && (phk->vk == vk)) {

            /*
             * In the case of PWND_FOCUS, we need to check that the queues
             * are the same since PWND_FOCUS is local to the queue it was
             * registered under.
             */
            if (phk->spwnd == PWND_FOCUS) {
                if (phk->pti == ptiCurrent) {
                    *pfKeysExist = TRUE;
                }
            } else {
                *pfKeysExist = TRUE;
            }
        }

        phkPrev = phk;
        phk = phk->phkNext;
    }

    return phkRet;
}


/***************************************************************************\
* IsSAS
*
* Checks the physical state of keyboard modifiers that would effect SAS
*
*
\***************************************************************************/

BOOL IsSAS(
    BYTE  vk,
    UINT* pfsModifiers)
{
    UINT fsDown = 0;

    CheckCritIn();

    if (gvkSAS != vk) {
        return FALSE;
    }

    /*
     * Special case for SAS - examine real physical modifier-key state!
     *
     * An evil daemon process can fool convincingly pretend to be winlogon
     * by registering Alt+Del as a hotkey, and spinning another thread that
     * continually calls keybd_event() to send the Ctrl key up: when the
     * user types Ctrl+Alt+Del, only Alt+Del will be seen by the system,
     * the evil daemon will get woken by WM_HOTKEY and can pretend to be
     * winlogon.  So look at gfsSASModifiersDown in this case, to see what keys
     * were physically pressed.
     * NOTE: If hotkeys are ever made to work under journal playback, make
     * sure they don't affect the gfsSASModifiersDown!  - IanJa.
     */
    if (gfsSASModifiersDown == gfsSASModifiers) {
        *pfsModifiers = gfsSASModifiersDown;
        return TRUE;
    }

    return FALSE;
}


/***************************************************************************\
* xxxDoHotKeyStuff
*
* This function gets called for every key event from low-level input
* processing.  It keeps track of the current state of modifier keys
* and when fsModifiers and vk match up with one of the registered
* hotkeys, a WM_HOTKEY message is generated. DoHotKeyStuff() will
* tell the input system to eat both the make and break for the 'vk'
* event.  This prevents apps from getting input that wasn't really
* intended for them.  DoHotKeyStuff() returns TRUE if it wants to 'eat'
* the event, FALSE if the system can pass on the event like it normally
* would.
*
* A Note on Modifier-Only Hotkeys
* Some hotkeys involve VK_SHIFT, VK_CONTROL, VK_MENU and/or VK_WINDOWS only.
* These are called Modifier-Only hotkeys.
* In order to distinguish hotkeys such as Alt-Shift-S and and Alt-Shift alone,
* modifier-only hotkeys must operate on a break, not a make.
* In order to prevent Alt-Shift-S from activating the Alt-Shift hotkey when
* the keys are released, modifier-only hotkeys are only activated when a
* modifier keyup (break) was immediately preceded by a modifier keydown (break)
* This also lets Alt-Shift,Shift,Shift activate the Alt-Shift hotkey 3 times.
*
* History:
* 12-05-90 DavidPe      Created.
*  4-15-93 Sanfords  Added code to return TRUE for Ctrl-Alt-Del events.
\***************************************************************************/

BOOL xxxDoHotKeyStuff(
    UINT vk,
    BOOL fBreak,
    DWORD fsReserveKeys)
{
    static UINT fsModifiers = 0;
    static UINT fsModOnlyCandidate = 0;
    UINT fsModOnlyHotkey;
    UINT fs;
    PHOTKEY phk;
    BOOL fCancel;
    BOOL fEatDebugKeyBreak = FALSE;
    PWND pwnd;
    BOOL bSAS;

    CheckCritIn();
    UserAssert(IsWinEventNotifyDeferredOK());

    if (gfInNumpadHexInput & NUMPAD_HEXMODE_LL) {
        RIPMSG0(RIP_VERBOSE, "xxxDoHotKeyStuff: Since we're in gfInNumpadHexInput, just bail out.");
        return FALSE;
    }

    /*
     * Update fsModifiers.
     */
    fs = 0;
    fsModOnlyHotkey = 0;

    switch (vk) {
    case VK_SHIFT:
        fs = MOD_SHIFT;
        break;

    case VK_CONTROL:
        fs = MOD_CONTROL;
        break;

    case VK_MENU:
        fs = MOD_ALT;
        break;

    case VK_LWIN:
    case VK_RWIN:
        fs = MOD_WIN;
        break;

    default:
        /*
         * A non-modifier key rules out Modifier-Only hotkeys
         */
        fsModOnlyCandidate = 0;
        break;
    }

    if (fBreak) {
        fsModifiers &= ~fs;
        /*
         * If a modifier key is coming up, the current modifier only hotkey
         * candidate must be tested to see if it is a hotkey.  Store this
         * in fsModOnlyHotkey, and prevent the next key release from
         * being a candidate by clearing fsModOnlyCandidate.
         */
        if (fs != 0) {
            fsModOnlyHotkey = fsModOnlyCandidate;
            fsModOnlyCandidate = 0;
        }
    } else {
        fsModifiers |= fs;
        /*
         * If a modifier key is going down, we have a modifier-only hotkey
         * candidate.  Save current modifier state until the following break.
         */
        if (fs != 0) {
            fsModOnlyCandidate = fsModifiers;
        }
    }

    /*
     * We look at the physical state for the modifiers because they can not be
     * manipulated and this prevents someone from writing a trojan winlogon look alike.
     * (See comment in AreModifiersIndicatingSAS)
     */
    bSAS = IsSAS((BYTE)vk, &fsModifiers);

    /*
     * If the key is not a hotkey then we're done but first check if the
     * key is an Alt-Escape if so we need to cancel journalling.
     *
     * NOTE: Support for Alt+Esc to cancel journalling dropped in NT 4.0
     */
    if (fsModOnlyHotkey && fBreak) {
        /*
         * A hotkey involving only VK_SHIFT, VK_CONTROL, VK_MENU or VK_WINDOWS
         * must only operate on a key release.
         */
        if ((phk = IsHotKey(fsModOnlyHotkey, VK_NONE)) == NULL) {
            return FALSE;
        }
    } else if ((phk = IsHotKey(fsModifiers, vk)) == NULL) {
        return FALSE;
    }

    /*
     * If we tripped a SAS hotkey, but it's not really the SAS, don't do it.
     */
    if ((phk->wFlags & MOD_SAS) && !bSAS) {
        return FALSE;

    }
    if (phk->id == IDHOT_WINDOWS) {
        pwnd = GETDESKINFO(PtiCurrent())->spwndShell;
        if (pwnd != NULL) {
            fsModOnlyCandidate = 0; /* Make it return TRUE */
            goto PostTaskListSysCmd;
        }
    }

    if ((phk->id == IDHOT_DEBUG) || (phk->id == IDHOT_DEBUGSERVER)) {

        if (!fBreak) {
            /*
             * The DEBUG key has been pressed.  Break the appropriate
             * thread into the debugger.  Won't need phk after this callback
             * because we return immediately.
             */
            fEatDebugKeyBreak = xxxActivateDebugger(phk->fsModifiers);
        }

        /*
         * This'll eat the debug key down and break if we broke into
         * the debugger on the server only on the down.
         */
        return fEatDebugKeyBreak;
    }

    /*
     * don't allow hotkeys(except for ones owned by the logon process)
     * if the window station is locked.
     */

    if (((grpdeskRitInput->rpwinstaParent->dwWSF_Flags & WSF_SWITCHLOCK) != 0) &&
            (phk->pti->pEThread->Cid.UniqueProcess != gpidLogon)) {
        RIPMSG0(RIP_WARNING, "Ignoring hotkey because Workstation locked");
        return FALSE;
    }

    if ((fsModOnlyHotkey == 0) && fBreak) {
        /*
         * Do Modifier-Only hotkeys on break events, else return here.
         */
        return FALSE;
    }

    /*
     * Unhook hooks if a control-escape, alt-escape, or control-alt-del
     * comes through, so the user can cancel if the system seems hung.
     *
     * Note the hook may be locked so even if the unhook succeeds it
     * won't remove the hook from the global asphkStart array.  So
     * we have to walk the list manually.  This code works because
     * we are in the critical section and we know other hooks won't
     * be deleted.
     *
     * Once we've unhooked, post a WM_CANCELJOURNAL message to the app
     * that set the hook so it knows we did this.
     *
     * NOTE: Support for Alt+Esc to cancel journalling dropped in NT 4.0
     */
    fCancel = FALSE;
    if (vk == VK_ESCAPE && (fsModifiers == MOD_CONTROL)) {
        fCancel = TRUE;
    }

    if (bSAS) {
        fCancel = TRUE;
    }

    if (fCancel)
        zzzCancelJournalling(); // BUG BUG phk might go away IANJA

    /*
     * See if the key is reserved by a console window.  If it is,
     * return FALSE so the key will be passed to the console.
     */
    if (fsReserveKeys != 0) {
        switch (vk) {
        case VK_TAB:
            if ((fsReserveKeys & CONSOLE_ALTTAB) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        case VK_ESCAPE:
            if ((fsReserveKeys & CONSOLE_ALTESC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            if ((fsReserveKeys & CONSOLE_CTRLESC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_CONTROL)) {
                return FALSE;
            }
            break;
        case VK_RETURN:
            if ((fsReserveKeys & CONSOLE_ALTENTER) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        case VK_SNAPSHOT:
            if ((fsReserveKeys & CONSOLE_PRTSC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == 0)) {
                return FALSE;
            }
            if ((fsReserveKeys & CONSOLE_ALTPRTSC) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        case VK_SPACE:
            if ((fsReserveKeys & CONSOLE_ALTSPACE) &&
                    ((fsModifiers & (MOD_CONTROL | MOD_ALT)) == MOD_ALT)) {
                return FALSE;
            }
            break;
        }
    }

    /*
     * If this is the task-list hotkey, go ahead and set foreground
     * status to the task-list queue right now.  This prevents problems
     * where the user hits ctrl-esc and types-ahead before the task-list
     * processes the hotkey and brings up the task-list window.
     */
    if ((fsModifiers == MOD_CONTROL) && (vk == VK_ESCAPE) && !fBreak) {
        PWND pwndSwitch;
        TL tlpwndSwitch;

        if (ghwndSwitch != NULL) {
            pwndSwitch = PW(ghwndSwitch);
            ThreadLock(pwndSwitch, &tlpwndSwitch);
            xxxSetForegroundWindow2(pwndSwitch, NULL, 0);  // BUG BUG phk might go away IANJA
            ThreadUnlock(&tlpwndSwitch);
        }
    }

    /*
     * Get the hot key contents.
     */
    if (phk->spwnd == NULL) {
        _PostThreadMessage(
                phk->pti, WM_HOTKEY, phk->id,
                MAKELONG(fsModifiers, vk));
        /*
         * Since this hotkey is for this guy, he owns the last input.
         */
        glinp.ptiLastWoken = phk->pti;

    } else {
        if (phk->spwnd == PWND_INPUTOWNER) {
            if (gpqForeground != NULL) {
                pwnd = gpqForeground->spwndFocus;
            } else {
                return FALSE;
            }
        } else {
            pwnd = phk->spwnd;
        }

        if (pwnd) {
            if (pwnd == pwnd->head.rpdesk->pDeskInfo->spwndShell && phk->id == SC_TASKLIST) {
PostTaskListSysCmd:
                _PostMessage(pwnd, WM_SYSCOMMAND, SC_TASKLIST, 0);
            } else {
                _PostMessage(pwnd, WM_HOTKEY, phk->id, MAKELONG(fsModifiers, vk));
            }

            /*
             * Since this hotkey is for this guy, he owns the last input.
             */
            glinp.ptiLastWoken = GETPTI(pwnd);
        }
    }

    /*
     * If this is a Modifier-Only hotkey, let the modifier break through
     * by returning FALSE, otherwise we will have modifier keys stuck down.
     */
    return (fsModOnlyHotkey == 0);

}


/***************************************************************************\
* IsHotKey
*
*
* History:
* 03-10-91 DavidPe      Created.
\***************************************************************************/

PHOTKEY IsHotKey(
    UINT fsModifiers,
    UINT vk)
{
    PHOTKEY phk;

    CheckCritIn();

    phk = gphkFirst;

    while (phk != NULL) {

        /*
         * Do the modifiers and vk for this hotkey match the current state?
         */
        if ((phk->fsModifiers == fsModifiers) && (phk->vk == vk)) {
            return phk;
        }

        phk = phk->phkNext;
    }

    return NULL;
}
