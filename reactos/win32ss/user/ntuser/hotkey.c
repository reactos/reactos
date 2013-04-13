/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          HotKey support
 * FILE:             subsystems/win32/win32k/ntuser/hotkey.c
 * PROGRAMER:        Eric Kohl
 */

/*
 * FIXME: Hotkey notifications are triggered by keyboard input (physical or programatically)
 * and since only desktops on WinSta0 can receive input in seems very wrong to allow
 * windows/threads on destops not belonging to WinSta0 to set hotkeys (receive notifications).
 *     -- Gunnar
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserHotkey);

/* GLOBALS *******************************************************************/

/* Hardcoded hotkeys. See http://ivanlef0u.fr/repo/windoz/VI20051005.html */
/*                   thread hwnd  modifiers  vk      id  next */
HOT_KEY hkF12 =      {NULL, NULL, 0,         VK_F12, IDHK_F12,      NULL};
HOT_KEY hkShiftF12 = {NULL, NULL, MOD_SHIFT, VK_F12, IDHK_SHIFTF12, &hkF12};
HOT_KEY hkWinKey =   {NULL, NULL, MOD_WIN,   0,      IDHK_WINKEY,   &hkShiftF12};

PHOT_KEY gphkFirst = &hkWinKey;
BOOL bWinHotkeyActive = FALSE;

/* FUNCTIONS *****************************************************************/

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
    HWND hWnd = pWnd->head.h;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->hWnd == hWnd)
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
UnregisterThreadHotKeys(struct _ETHREAD *pThread)
{
    PHOT_KEY pHotKey = gphkFirst, phkNext, *pLink = &gphkFirst;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->pThread == pThread)
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

    if (wVk == VK_SHIFT || wVk == VK_CONTROL || wVk == VK_MENU ||
        wVk == VK_LWIN || wVk == VK_RWIN)
    {
        /* Those keys are specified by modifiers */
        wVk = 0;
    }

    /* Check if it is a hotkey */
    fModifiers = IntGetModifiers(gafAsyncKeyState);
    pHotKey = IsHotKey(fModifiers, wVk);
    if (pHotKey)
    {
        /* Process hotkey if it is key up event */
        if (!bIsDown)
        {
            TRACE("Hot key pressed (hWnd %p, id %d)\n", pHotKey->hWnd, pHotKey->id);

            /* WIN and F12 keys are hardcoded here. See: http://ivanlef0u.fr/repo/windoz/VI20051005.html */
            if (pHotKey == &hkWinKey)
            {
                if(bWinHotkeyActive == TRUE)
                {
                    UserPostMessage(InputWindowStation->ShellWindow, WM_SYSCOMMAND, SC_TASKLIST, 0);
                    bWinHotkeyActive = FALSE;
                }
            }
            else if (pHotKey == &hkF12 || pHotKey == &hkShiftF12)
            {
                //co_ActivateDebugger(); // FIXME
            }
            else if (pHotKey->id == IDHK_REACTOS && !pHotKey->pThread) // FIXME: Those hotkeys doesn't depend on RegisterHotKey
            {
                UserPostMessage(pHotKey->hWnd, WM_SYSCOMMAND, SC_HOTKEY, (LPARAM)pHotKey->hWnd);
            }
            else
            {
                /* If a hotkey with the WIN modifier was activated, do not treat the release of the WIN key as a hotkey*/
                if((pHotKey->fsModifiers & MOD_WIN) != 0)
                    bWinHotkeyActive = FALSE;

                MsqPostHotKeyMessage(pHotKey->pThread,
                                     pHotKey->hWnd,
                                     (WPARAM)pHotKey->id,
                                     MAKELPARAM((WORD)fModifiers, wVk));
            }
        }
        else
        {
            if (pHotKey == &hkWinKey)
            {
               /* The user pressed the win key */
                bWinHotkeyActive = TRUE;
            }
        }

        return TRUE; /* Don't send any message */
    }

    return FALSE;
}


/*
 * DefWndGetHotKey
 *
 * GetHotKey message support
 */
UINT FASTCALL
DefWndGetHotKey(HWND hWnd)
{
    PHOT_KEY pHotKey = gphkFirst;

    WARN("DefWndGetHotKey\n");

    while (pHotKey)
    {
        if (pHotKey->hWnd == hWnd && pHotKey->id == IDHK_REACTOS)
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
 * DefWndSetHotKey
 *
 * SetHotKey message support
 */
INT FASTCALL
DefWndSetHotKey(PWND pWnd, WPARAM wParam)
{
    UINT fsModifiers, vk;
    PHOT_KEY pHotKey, *pLink;
    HWND hWnd;
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
    hWnd = UserHMGetHandle(pWnd);

    if (wParam)
    {
        pHotKey = gphkFirst;
        while (pHotKey)
        {
            if (pHotKey->fsModifiers == fsModifiers &&
                pHotKey->vk == vk &&
                pHotKey->id == IDHK_REACTOS)
            {
                if (pHotKey->hWnd != hWnd)
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
        if (pHotKey->hWnd == hWnd &&
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

            pHotKey->hWnd = hWnd;
            pHotKey->id = IDHK_REACTOS; // Don't care, these hot keys are unrelated to the hot keys set by RegisterHotKey
            pHotKey->pNext = gphkFirst;
            gphkFirst = pHotKey;
        }

        /* A window can only have one hot key. If the window already has a
           hot key associated with it, the new hot key replaces the old one. */
        pHotKey->pThread = NULL;
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

/* SYSCALLS *****************************************************************/


BOOL APIENTRY
NtUserRegisterHotKey(HWND hWnd,
                     int id,
                     UINT fsModifiers,
                     UINT vk)
{
    PHOT_KEY pHotKey;
    PWND pWnd;
    PETHREAD pHotKeyThread;
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
        pHotKeyThread = PsGetCurrentThread();
    }
    else
    {
        pWnd = UserGetWindowObject(hWnd);
        if (!pWnd)
            goto cleanup;

        pHotKeyThread = pWnd->head.pti->pEThread;
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

    pHotKey->pThread = pHotKeyThread;
    pHotKey->hWnd = hWnd;
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

    TRACE("Enter NtUserUnregisterHotKey\n");
    UserEnterExclusive();

    /* Fail if given window is invalid */
    if (hWnd && !UserGetWindowObject(hWnd))
        goto cleanup;

    while (pHotKey)
    {
        /* Save next ptr for later use */
        phkNext = pHotKey->pNext;

        /* Should we delete this hotkey? */
        if (pHotKey->hWnd == hWnd && pHotKey->id == id)
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
