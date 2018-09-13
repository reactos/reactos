/****************************** Module Header ******************************\
* Module Name: keyboard.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* History:
* 11-11-90 DavidPe      Created.
* 13-Feb-1991 mikeke    Added Revalidation code (None)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop


/***************************************************************************\
* _GetKeyState (API)
*
* This API returns the up/down and toggle state of the specified VK based
* on the input synchronized keystate in the current queue.  The toggle state
* is mainly for 'state' keys like Caps-Lock that are toggled each time you
* press them.
*
* History:
* 11-11-90 DavidPe      Created.
\***************************************************************************/

SHORT _GetKeyState(
    int vk)
{
    UINT wKeyState;
    PTHREADINFO pti;

    if ((UINT)vk >= CVKKEYSTATE) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"vk\" (%ld) to _GetKeyState",
                vk);

        return 0;
    }

    pti = PtiCurrentShared();

#ifdef LATER
//
// note - anything that accesses the pq structure is a bad idea since it
// can be changed between any two instructions.
//
#endif

    wKeyState = 0;

    /*
     * Set the toggle bit.
     */
    if (TestKeyStateToggle(pti->pq, vk))
        wKeyState = 0x0001;

    /*
     * Set the keyup/down bit.
     */
    if (TestKeyStateDown(pti->pq, vk)) {
        /*
         * Used to be wKeyState|= 0x8000.Fix for bug 28820; Ctrl-Enter
         * accelerator doesn't work on Nestscape Navigator Mail 2.0
         */
        wKeyState |= 0xff80;  // This is what 3.1 returned!!!!
    }

    return (SHORT)wKeyState;
}

/***************************************************************************\
* _GetAsyncKeyState (API)
*
* This function is similar to GetKeyState except it returns what could be
* considered the 'hardware' keystate or what state the key is in at the
* moment the function is called, rather than based on what key events the
* application has processed.  Also, rather than returning the toggle bit,
* it has a bit telling whether the key was pressed since the last call to
* GetAsyncKeyState().
*
* History:
* 11-11-90 DavidPe      Created.
\***************************************************************************/

SHORT _GetAsyncKeyState(
    int vk)
{
    SHORT sKeyState;

    if ((UINT)vk >= CVKKEYSTATE) {
        RIPERR1(ERROR_INVALID_PARAMETER,
                RIP_WARNING,
                "Invalid parameter \"vk\" (%ld) to _GetAsyncKeyState",
                vk);

        return 0;
    }

    /*
     * See if this key went down since the last time state for it was
     * read. Clear the flag if so.
     */
    sKeyState = 0;
    if (TestAsyncKeyStateRecentDown(vk)) {
        ClearAsyncKeyStateRecentDown(vk);
        sKeyState = 1;
    }

    /*
     * Set the keyup/down bit.
     */
    if (TestAsyncKeyStateDown(vk))
        sKeyState |= 0x8000;

    /*
     * Don't return the toggle bit since it's a new bit and might
     * cause compatibility problems.
     */
    return sKeyState;
}

/***************************************************************************\
* _SetKeyboardState (API)
*
* This function allows the app to set the current keystate.  This is mainly
* useful for setting the toggle bit, particularly for the keys associated
* with the LEDs on the physical keyboard.
*
* History:
* 11-11-90 DavidPe      Created.
* 16-May-1991 mikeke    Changed to return BOOL
\***************************************************************************/

BOOL _SetKeyboardState(
    CONST BYTE *pb)
{
    int i;
    PQ pq;
    PTHREADINFO  ptiCurrent = PtiCurrent();

    pq = ptiCurrent->pq;

    /*
     * Copy in the new state table.
     */
    for (i = 0; i < 256; i++, pb++) {
        if (*pb & 0x80) {
            SetKeyStateDown(pq, i);
        } else {
            ClearKeyStateDown(pq, i);
        }

        if (*pb & 0x01) {
            SetKeyStateToggle(pq, i);
        } else {
            ClearKeyStateToggle(pq, i);
        }
    }

    /*
     * Update the key cache index.
     */
    gpsi->dwKeyCache++;

#ifdef LATER
// scottlu 6-9-91
// I don't think we ought to do this unless someone really complains. This
// could have bad side affects, especially considering that terminal
// apps will want to do this, and terminal apps could easily not respond
// to input for awhile, causing this state to change unexpectedly while
// a user is using some other application. - scottlu.

/* DavidPe 02/05/92
 *  How about if we only do it when the calling app is foreground?
 */

    /*
     * Propagate the toggle bits for the keylight keys to the
     * async keystate table and update the keylights.
     *
     * THIS could be evil in a de-synced environment, but to do this
     * in a totally "synchronous" way is hard.
     */
    if (pb[VK_CAPITAL] & 0x01) {
        SetAsyncKeyStateToggle(VK_CAPITAL);
    } else {
        ClearAsyncKeyStateToggle(VK_CAPITAL);
    }

    if (pb[VK_NUMLOCK] & 0x01) {
        SetAsyncKeyStateToggle(VK_NUMLOCK);
    } else {
        ClearAsyncKeyStateToggle(VK_NUMLOCK);
    }

    if (pb[VK_SCROLL] & 0x01) {
        SetAsyncKeyStateToggle(VK_SCROLL);
    } else {
        ClearAsyncKeyStateToggle(VK_SCROLL);
    }

    UpdateKeyLights(TRUE);
#endif

    return TRUE;
}

/***************************************************************************\
* RegisterPerUserKeyboardIndicators
*
* Saves the current keyboard indicators in the user's profile.
*
* ASSUMPTIONS:
*
* 10-14-92 IanJa        Created.
\***************************************************************************/

static CONST WCHAR wszInitialKeyboardIndicators[] = L"InitialKeyboardIndicators";

VOID
RegisterPerUserKeyboardIndicators(PUNICODE_STRING pProfileUserName)
{
    WCHAR wszInitKbdInd[2] = L"0";

    /*
     * Initial Keyboard state (Num-Lock only)
     */

    /*
     * For HYDRA we do not want to save this as set, since it confuses
     * dial in laptop computers. So we force it off in the profile on
     * logoff.
     */
    if (!ISTS()) {
        wszInitKbdInd[0] += TestAsyncKeyStateToggle(VK_NUMLOCK) ? 2 : 0;
    }

    FastWriteProfileStringW(pProfileUserName,
                            PMAP_KEYBOARD,
                            wszInitialKeyboardIndicators,
                            wszInitKbdInd);
}

/***************************************************************************\
* UpdatePerUserKeyboardIndicators
*
* Sets the initial keyboard indicators according to the user's profile.
*
* ASSUMPTIONS:
*
* 10-14-92 IanJa        Created.
\***************************************************************************/
VOID
UpdatePerUserKeyboardIndicators(PUNICODE_STRING pProfileUserName)
{
    DWORD dw;
    PQ pq;
    PTHREADINFO  ptiCurrent = PtiCurrent();
    pq = ptiCurrent->pq;

    /*
     * For terminal server, the client is responsible for synchronizing the
     * keyboard state.
     */
    if (gbRemoteSession) {
        return;
    }

    /*
     * Initial Keyboard state (Num-Lock only)
     */
    dw = FastGetProfileIntW(pProfileUserName,
                            PMAP_KEYBOARD,
                            wszInitialKeyboardIndicators,
                            2);

    dw &= 0x80000002;


    /*
     * The special value 0x80000000 in the registry indicates that the BIOS
     * settings are to be used as the initial LED state. (This is undocumented)
     */
    if (dw == 0x80000000) {
        dw = gklpBootTime.LedFlags;
    }
    if (dw & 0x02) {
        SetKeyStateToggle(pq, VK_NUMLOCK);
        SetAsyncKeyStateToggle(VK_NUMLOCK);
        SetRawKeyToggle(VK_NUMLOCK);
    } else {
        ClearKeyStateToggle(pq, VK_NUMLOCK);
        ClearAsyncKeyStateToggle(VK_NUMLOCK);
        ClearRawKeyToggle(VK_NUMLOCK);
    }

    /*
     * Initialize KANA Toggle status
     */
    gfKanaToggle = FALSE;
    ClearKeyStateToggle(pq, VK_KANA);
    ClearAsyncKeyStateToggle(VK_KANA);
    ClearRawKeyToggle(VK_KANA);

    UpdateKeyLights(FALSE);
}


/***************************************************************************\
* UpdateAsyncKeyState
*
* Based on a VK and a make/break flag, this function will update the async
* keystate table.
*
* History:
* 06-09-91 ScottLu      Added keystate synchronization across threads.
* 11-12-90 DavidPe      Created.
\***************************************************************************/

void UpdateAsyncKeyState(
    PQ pqOwner,
    UINT wVK,
    BOOL fBreak)
{
    PQ pqT;
    PLIST_ENTRY pHead, pEntry;
    PTHREADINFO pti;

    CheckCritIn();

    /*
     * First check to see if the queue this key is going to has a pending
     * key state event. If it does, post it because we need to copy the
     * async key state into this event as it is before we modify
     * this key's state, or else we'll generate a key state event with
     * the wrong key state in it.
     */
    if (pqOwner != NULL && pqOwner->QF_flags & QF_UPDATEKEYSTATE) {
        PostUpdateKeyStateEvent(pqOwner);
    }

    if (!fBreak) {
        /*
         * This key has gone down - update the "recent down" bit in the
         * async key state table.
         */
        SetAsyncKeyStateRecentDown(wVK);

        /*
         * This is a key make. If the key was not already down, update the
         * toggle bit.
         */
        if (!TestAsyncKeyStateDown(wVK)) {
            if (TestAsyncKeyStateToggle(wVK)) {
                ClearAsyncKeyStateToggle(wVK);
            } else {
                SetAsyncKeyStateToggle(wVK);
            }
        }

        /*
         * This is a make, so turn on the key down bit.
         */
        SetAsyncKeyStateDown(wVK);

    } else {
        /*
         * This is a break, so turn off the key down bit.
         */
        ClearAsyncKeyStateDown(wVK);
    }

    /*
     * If this is one of the keys we cache, update the async key cache index.
     */
    if (wVK < CVKASYNCKEYCACHE) {
        gpsi->dwAsyncKeyCache++;
    }

    /*
     * A key has changed state. Update all queues not receiving this input so
     * they know that this key has changed state. This lets us know which keys to
     * update in the thread specific key state table to keep it in sync
     * with the user.  Walking down the thread list may mean that an
     * individual queue may by updated more than once, but it is cheaper
     * than maintaining a list of queues on the desktop.
     */
    UserAssert(grpdeskRitInput != NULL);

    pHead = &grpdeskRitInput->PtiList;
    for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
        pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

        /*
         * Don't update the queue this message is going to - it'll be
         * in sync because it is receiving this message.
         */
        pqT = pti->pq;
        if (pqT == pqOwner)
            continue;

        /*
         * Set the "recent down" bit. In this case this doesn't really mean
         * "recent down", it means "recent change" (since the last time
         * we synced this queue), either up or down. This tells us which
         * keys went down since the last time this thread synced with key
         * state. Set the "update key state" flag so we know that later
         * we need to sync with these keys.
         */
        SetKeyRecentDownBit(pqT->afKeyRecentDown, wVK);
        pqT->QF_flags |= QF_UPDATEKEYSTATE;
    }

    /*
     * Update the key cache index.
     */
    gpsi->dwKeyCache++;
}
