/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          HotKey support
 * FILE:             win32ss/user/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 */

/*
 * FIXME: Hotkey notifications are triggered by keyboard input (physical or programmatically)
 * and since only desktops on WinSta0 can receive input in seems very wrong to allow
 * windows/threads on destops not belonging to WinSta0 to set hotkeys (receive notifications).
 *     -- Gunnar
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserHotkey);

/* GLOBALS *******************************************************************/

/*
 * Hardcoded hotkeys. See http://ivanlef0u.fr/repo/windoz/VI20051005.html (DEAD_LINK)
 * or https://web.archive.org/web/20170826161432/http://repo.meh.or.id/Windows/VI20051005.html .
 *
 * NOTE: The (Shift-)F12 keys are used only for the "UserDebuggerHotKey" setting
 * which enables setting a key shortcut which, when pressed, establishes a
 * breakpoint in the code being debugged:
 * see https://learn.microsoft.com/en-us/previous-versions/windows/it-pro/windows-server-2003/cc786263(v=ws.10)
 * and https://flylib.com/books/en/4.441.1.33/1/ for more details.
 * By default the key is VK-F12 on a 101-key keyboard, and is VK_SUBTRACT
 * (hyphen / substract sign) on a 82-key keyboard.
 */
/*                       pti   pwnd  modifiers  vk      id  next */
// HOT_KEY hkF12 =      {NULL, 1,    0,         VK_F12, IDHK_F12,      NULL};
// HOT_KEY hkShiftF12 = {NULL, 1,    MOD_SHIFT, VK_F12, IDHK_SHIFTF12, &hkF12};
// HOT_KEY hkWinKey =   {NULL, 1,    MOD_WIN,   0,      IDHK_WINKEY,   &hkShiftF12};

PHOT_KEY gphkFirst = NULL;
UINT gfsModOnlyCandidate;

/* FUNCTIONS *****************************************************************/

VOID FASTCALL
StartDebugHotKeys(VOID)
{
    UINT vk = VK_F12;
    UserUnregisterHotKey(PWND_BOTTOM, IDHK_F12);
    UserUnregisterHotKey(PWND_BOTTOM, IDHK_SHIFTF12);
    if (!ENHANCED_KEYBOARD(gKeyboardInfo.KeyboardIdentifier))
    {
        vk = VK_SUBTRACT;
    }
    UserRegisterHotKey(PWND_BOTTOM, IDHK_SHIFTF12, MOD_SHIFT, vk);
    UserRegisterHotKey(PWND_BOTTOM, IDHK_F12, 0, vk);
    TRACE("Start up the debugger hotkeys!! If you see this you enabled debugprints. Congrats!\n");
}

/*
 * IntGetModifiers
 *
 * Returns a value that indicates if the key is a modifier key, and
 * which one.
 */
static
UINT FASTCALL
IntGetModifiers(PBYTE pKeyState)
{
    UINT fModifiers = 0;

    if (IS_KEY_DOWN(pKeyState, VK_SHIFT))
        fModifiers |= MOD_SHIFT;

    if (IS_KEY_DOWN(pKeyState, VK_CONTROL))
        fModifiers |= MOD_CONTROL;

    if (IS_KEY_DOWN(pKeyState, VK_MENU))
        fModifiers |= MOD_ALT;

    if (IS_KEY_DOWN(pKeyState, VK_LWIN) || IS_KEY_DOWN(pKeyState, VK_RWIN))
        fModifiers |= MOD_WIN;

    return fModifiers;
}

/*
 * UnregisterWindowHotKeys
 *
 * Removes hotkeys registered by specified window on its cleanup
 */
VOID FASTCALL
UnregisterWindowHotKeys(PWND pWnd)
{
    PHOT_KEY pHotKey = gphkFirst, phkNext, *pLink = &gphkFirst;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->pWnd == pWnd)
        {
            /* Update next ptr for previous hotkey and free memory */
            *pLink = phkNext;
            ExFreePoolWithTag(pHotKey, USERTAG_HOTKEY);
        }
        else /* This hotkey will stay, use its next ptr */
            pLink = &pHotKey->pNext;

        /* Move to the next entry */
        pHotKey = phkNext;
    }
}

/*
 * UnregisterThreadHotKeys
 *
 * Removes hotkeys registered by specified thread on its cleanup
 */
VOID FASTCALL
UnregisterThreadHotKeys(PTHREADINFO pti)
{
    PHOT_KEY pHotKey = gphkFirst, phkNext, *pLink = &gphkFirst;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->pti == pti)
        {
            /* Update next ptr for previous hotkey and free memory */
            *pLink = phkNext;
            ExFreePoolWithTag(pHotKey, USERTAG_HOTKEY);
        }
        else /* This hotkey will stay, use its next ptr */
            pLink = &pHotKey->pNext;

        /* Move to the next entry */
        pHotKey = phkNext;
    }
}

/*
 * IsHotKey
 *
 * Checks if given key and modificators have corresponding hotkey
 */
static PHOT_KEY FASTCALL
IsHotKey(UINT fsModifiers, WORD wVk)
{
    PHOT_KEY pHotKey = gphkFirst;

    while (pHotKey)
    {
        if (pHotKey->fsModifiers == fsModifiers &&
            pHotKey->vk == wVk)
        {
            /* We have found it */
            return pHotKey;
        }

        /* Move to the next entry */
        pHotKey = pHotKey->pNext;
    }

    return NULL;
}

/*
 * co_UserProcessHotKeys
 *
 * Sends WM_HOTKEY message if given keys are hotkey
 */
BOOL NTAPI
co_UserProcessHotKeys(WORD wVk, BOOL bIsDown)
{
    UINT fModifiers;
    PHOT_KEY pHotKey;
    PWND pWnd;
    BOOL DoNotPostMsg = FALSE;
    BOOL IsModifier = FALSE;

    if (wVk == VK_SHIFT || wVk == VK_CONTROL || wVk == VK_MENU ||
        wVk == VK_LWIN || wVk == VK_RWIN)
    {
        /* Remember that this was a modifier */
        IsModifier = TRUE;
    }

    fModifiers = IntGetModifiers(gafAsyncKeyState);

    if (bIsDown)
    {
        if (IsModifier)
        {
            /* Modifier key down -- no hotkey trigger, but remember this */
            gfsModOnlyCandidate = fModifiers;
            return FALSE;
        }
        else
        {
            /* Regular key down -- check for hotkey, and reset mod candidates */
            pHotKey = IsHotKey(fModifiers, wVk);
            gfsModOnlyCandidate = 0;
        }
    }
    else
    {
        if (IsModifier)
        {
            /* Modifier key up -- modifier-only keys are triggered here */
            pHotKey = IsHotKey(gfsModOnlyCandidate, 0);
            gfsModOnlyCandidate = 0;
        }
        else
        {
            /* Regular key up -- no hotkey, but reset mod-only candidates */
            gfsModOnlyCandidate = 0;
            return FALSE;
        }
    }

    if (pHotKey)
    {
        TRACE("Hot key pressed (pWnd %p, id %d)\n", pHotKey->pWnd, pHotKey->id);

        /* FIXME: See comment about "UserDebuggerHotKey" on top of this file. */
        if (pHotKey->id == IDHK_SHIFTF12 || pHotKey->id == IDHK_F12)
        {
            if (bIsDown)
            {
                ERR("Hot key pressed for Debug Activation! ShiftF12 = %d or F12 = %d\n",pHotKey->id == IDHK_SHIFTF12 , pHotKey->id == IDHK_F12);
                //DoNotPostMsg = co_ActivateDebugger(); // FIXME
            }
            return DoNotPostMsg;
        }

        /* WIN and F12 keys are not hardcoded here. See comments on top of this file. */
        if (pHotKey->id == IDHK_WINKEY)
        {
            ASSERT(!bIsDown);
            pWnd = ValidateHwndNoErr(InputWindowStation->ShellWindow);
            if (pWnd)
            {
               TRACE("System Hot key Id %d Key %u\n", pHotKey->id, wVk );
               UserPostMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_TASKLIST, 0);
               co_IntShellHookNotify(HSHELL_TASKMAN, 0, 0);
               return FALSE;
            }
        }
        
        if (pHotKey->id == IDHK_SNAP_LEFT ||
            pHotKey->id == IDHK_SNAP_RIGHT ||
            pHotKey->id == IDHK_SNAP_UP ||
            pHotKey->id == IDHK_SNAP_DOWN)
        {
            HWND topWnd = UserGetForegroundWindow();
            if (topWnd)
            {
                UserPostMessage(topWnd, WM_KEYDOWN, wVk, 0);
            }
            return TRUE;
        }

        if (!pHotKey->pWnd)
        {
            TRACE("UPTM Hot key Id %d Key %u\n", pHotKey->id, wVk );
            UserPostThreadMessage(pHotKey->pti, WM_HOTKEY, pHotKey->id, MAKELONG(fModifiers, wVk));
            //ptiLastInput = pHotKey->pti;
            return TRUE; /* Don't send any message */
        }
        else
        {
            pWnd = pHotKey->pWnd;
            if (pWnd == PWND_BOTTOM)
            {
                if (gpqForeground == NULL)
                    return FALSE;

                pWnd = gpqForeground->spwndFocus;
            }

            if (pWnd)
            {
                //  pWnd->head.rpdesk->pDeskInfo->spwndShell needs testing.
                if (pWnd == ValidateHwndNoErr(InputWindowStation->ShellWindow) && pHotKey->id == SC_TASKLIST)
                {
                    UserPostMessage(UserHMGetHandle(pWnd), WM_SYSCOMMAND, SC_TASKLIST, 0);
                    co_IntShellHookNotify(HSHELL_TASKMAN, 0, 0);
                }
                else
                {
                    TRACE("UPM Hot key Id %d Key %u\n", pHotKey->id, wVk );
                    UserPostMessage(UserHMGetHandle(pWnd), WM_HOTKEY, pHotKey->id, MAKELONG(fModifiers, wVk));
                }
                //ptiLastInput = pWnd->head.pti;
                return TRUE; /* Don't send any message */
            }
        }
    }
    return FALSE;
}


/*
 * DefWndGetHotKey --- GetHotKey message support
 *
 * Win: DWP_GetHotKey
 */
UINT FASTCALL
DefWndGetHotKey(PWND pWnd)
{
    PHOT_KEY pHotKey = gphkFirst;

    WARN("DefWndGetHotKey\n");

    while (pHotKey)
    {
        if (pHotKey->pWnd == pWnd && pHotKey->id == IDHK_REACTOS)
        {
            /* We have found it */
            return MAKELONG(pHotKey->vk, pHotKey->fsModifiers);
        }

        /* Move to the next entry */
        pHotKey = pHotKey->pNext;
    }

    return 0;
}

/*
 * DefWndSetHotKey --- SetHotKey message support
 *
 * Win: DWP_SetHotKey
 */
INT FASTCALL
DefWndSetHotKey(PWND pWnd, WPARAM wParam)
{
    UINT fsModifiers, vk;
    PHOT_KEY pHotKey, *pLink;
    INT iRet = 1;

    WARN("DefWndSetHotKey wParam 0x%x\n", wParam);

    // A hot key cannot be associated with a child window.
    if (pWnd->style & WS_CHILD)
        return 0;

    // VK_ESCAPE, VK_SPACE, and VK_TAB are invalid hot keys.
    if (LOWORD(wParam) == VK_ESCAPE ||
        LOWORD(wParam) == VK_SPACE ||
        LOWORD(wParam) == VK_TAB)
    {
        return -1;
    }

    vk = LOWORD(wParam);
    fsModifiers = HIWORD(wParam);

    if (wParam)
    {
        pHotKey = gphkFirst;
        while (pHotKey)
        {
            if (pHotKey->fsModifiers == fsModifiers &&
                pHotKey->vk == vk &&
                pHotKey->id == IDHK_REACTOS)
            {
                if (pHotKey->pWnd != pWnd)
                    iRet = 2; // Another window already has the same hot key.
                break;
            }

            /* Move to the next entry */
            pHotKey = pHotKey->pNext;
        }
    }

    pHotKey = gphkFirst;
    pLink = &gphkFirst;
    while (pHotKey)
    {
        if (pHotKey->pWnd == pWnd &&
            pHotKey->id == IDHK_REACTOS)
        {
            /* This window has already hotkey registered */
            break;
        }

        /* Move to the next entry */
        pLink = &pHotKey->pNext;
        pHotKey = pHotKey->pNext;
    }

    if (wParam)
    {
        if (!pHotKey)
        {
            /* Create new hotkey */
            pHotKey = ExAllocatePoolWithTag(PagedPool, sizeof(HOT_KEY), USERTAG_HOTKEY);
            if (pHotKey == NULL)
                return 0;

            pHotKey->pWnd = pWnd;
            pHotKey->id = IDHK_REACTOS; // Don't care, these hot keys are unrelated to the hot keys set by RegisterHotKey
            pHotKey->pNext = gphkFirst;
            gphkFirst = pHotKey;
        }

        /* A window can only have one hot key. If the window already has a
           hot key associated with it, the new hot key replaces the old one. */
        pHotKey->pti = NULL;
        pHotKey->fsModifiers = fsModifiers;
        pHotKey->vk = vk;
    }
    else if (pHotKey)
    {
        /* Remove hotkey */
        *pLink = pHotKey->pNext;
        ExFreePoolWithTag(pHotKey, USERTAG_HOTKEY);
    }

    return iRet;
}


BOOL FASTCALL
UserRegisterHotKey(PWND pWnd,
                   int id,
                   UINT fsModifiers,
                   UINT vk)
{
    PHOT_KEY pHotKey;
    PTHREADINFO pHotKeyThread;

    /* Find hotkey thread */
    if (pWnd == NULL || pWnd == PWND_BOTTOM)
    {
        pHotKeyThread = PsGetCurrentThreadWin32Thread();
    }
    else
    {
        pHotKeyThread = pWnd->head.pti;
    }

    /* Check for existing hotkey */
    if (IsHotKey(fsModifiers, vk))
    {
        EngSetLastError(ERROR_HOTKEY_ALREADY_REGISTERED);
        WARN("Hotkey already exists\n");
        return FALSE;
    }

    /* Create new hotkey */
    pHotKey = ExAllocatePoolWithTag(PagedPool, sizeof(HOT_KEY), USERTAG_HOTKEY);
    if (pHotKey == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    pHotKey->pti = pHotKeyThread;
    pHotKey->pWnd = pWnd;
    pHotKey->fsModifiers = fsModifiers;
    pHotKey->vk = vk;
    pHotKey->id = id;

    /* Insert hotkey to the global list */
    pHotKey->pNext = gphkFirst;
    gphkFirst = pHotKey;

    return TRUE;
}

BOOL FASTCALL
UserUnregisterHotKey(PWND pWnd, int id)
{
    PHOT_KEY pHotKey = gphkFirst, phkNext, *pLink = &gphkFirst;
    BOOL bRet = FALSE;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->pWnd == pWnd && pHotKey->id == id)
        {
            /* Update next ptr for previous hotkey and free memory */
            *pLink = phkNext;
            ExFreePoolWithTag(pHotKey, USERTAG_HOTKEY);

            bRet = TRUE;
        }
        else /* This hotkey will stay, use its next ptr */
            pLink = &pHotKey->pNext;

        /* Move to the next entry */
        pHotKey = phkNext;
    }
    return bRet;
}


/* SYSCALLS *****************************************************************/


BOOL APIENTRY
NtUserRegisterHotKey(HWND hWnd,
                     int id,
                     UINT fsModifiers,
                     UINT vk)
{
    PHOT_KEY pHotKey;
    PWND pWnd = NULL;
    PTHREADINFO pHotKeyThread;
    BOOL bRet = FALSE;

    TRACE("Enter NtUserRegisterHotKey\n");

    if (fsModifiers & ~(MOD_ALT|MOD_CONTROL|MOD_SHIFT|MOD_WIN)) // FIXME: Does Win2k3 support MOD_NOREPEAT?
    {
        WARN("Invalid modifiers: %x\n", fsModifiers);
        EngSetLastError(ERROR_INVALID_FLAGS);
        return 0;
    }

    UserEnterExclusive();

    /* Find hotkey thread */
    if (hWnd == NULL)
    {
        pHotKeyThread = gptiCurrent;
    }
    else
    {
        pWnd = UserGetWindowObject(hWnd);
        if (!pWnd)
            goto cleanup;

        pHotKeyThread = pWnd->head.pti;

        /* Fix wine msg "Window on another thread" test_hotkey */
        if (pWnd->head.pti != gptiCurrent)
        {
           EngSetLastError(ERROR_WINDOW_OF_OTHER_THREAD);
           WARN("Must be from the same Thread.\n");
           goto cleanup;
        }
    }

    /* Check for existing hotkey */
    if (IsHotKey(fsModifiers, vk))
    {
        EngSetLastError(ERROR_HOTKEY_ALREADY_REGISTERED);
        WARN("Hotkey already exists\n");
        goto cleanup;
    }

    /* Create new hotkey */
    pHotKey = ExAllocatePoolWithTag(PagedPool, sizeof(HOT_KEY), USERTAG_HOTKEY);
    if (pHotKey == NULL)
    {
        EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }

    pHotKey->pti = pHotKeyThread;
    pHotKey->pWnd = pWnd;
    pHotKey->fsModifiers = fsModifiers;
    pHotKey->vk = vk;
    pHotKey->id = id;

    /* Insert hotkey to the global list */
    pHotKey->pNext = gphkFirst;
    gphkFirst = pHotKey;

    bRet = TRUE;

cleanup:
    TRACE("Leave NtUserRegisterHotKey, ret=%i\n", bRet);
    UserLeave();
    return bRet;
}


BOOL APIENTRY
NtUserUnregisterHotKey(HWND hWnd, int id)
{
    PHOT_KEY pHotKey = gphkFirst, phkNext, *pLink = &gphkFirst;
    BOOL bRet = FALSE;
    PWND pWnd = NULL;

    TRACE("Enter NtUserUnregisterHotKey\n");
    UserEnterExclusive();

    /* Fail if given window is invalid */
    if (hWnd && !(pWnd = UserGetWindowObject(hWnd)))
        goto cleanup;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->pWnd == pWnd && pHotKey->id == id)
        {
            /* Update next ptr for previous hotkey and free memory */
            *pLink = phkNext;
            ExFreePoolWithTag(pHotKey, USERTAG_HOTKEY);

            bRet = TRUE;
        }
        else /* This hotkey will stay, use its next ptr */
            pLink = &pHotKey->pNext;

        /* Move to the next entry */
        pHotKey = phkNext;
    }

cleanup:
    TRACE("Leave NtUserUnregisterHotKey, ret=%i\n", bRet);
    UserLeave();
    return bRet;
}

/* EOF */
