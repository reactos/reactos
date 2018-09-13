/****************************** Module Header ******************************\
* Module Name: access.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains the Access Pack functions.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

CONST ACCESSIBILITYPROC aAccessibilityProc[] = {
    HighContrastHotKey,
    FilterKeys,
    xxxStickyKeys,
    MouseKeys,
    ToggleKeys,
    UtilityManager
};

typedef struct tagMODBITINFO {
    int BitPosition;
    BYTE ScanCode;
    USHORT Vk;
} MODBITINFO, *PMODBITINFO;

CONST MODBITINFO aModBit[] =
{
    { 0x01, SCANCODE_LSHIFT, VK_LSHIFT },
    { 0x02, SCANCODE_RSHIFT, VK_RSHIFT | KBDEXT },
    { 0x04, SCANCODE_CTRL, VK_LCONTROL },
    { 0x08, SCANCODE_CTRL, VK_RCONTROL | KBDEXT },
    { 0x10, SCANCODE_ALT, VK_LMENU },
    { 0x20, SCANCODE_ALT, VK_RMENU | KBDEXT },
    { 0x40, SCANCODE_LWIN, VK_LWIN },
    { 0x80, SCANCODE_RWIN,    VK_RWIN | KBDEXT}
};

/*
 * The ausMouseVKey array provides a translation from the virtual key
 * value to an index.  The index is used to select the appropriate
 * routine to process the virtual key, as well as to select extra
 * information that is used by this routine during its processing.
 */
CONST USHORT ausMouseVKey[] = {
                       VK_CLEAR,
                       VK_PRIOR,
                       VK_NEXT,
                       VK_END,
                       VK_HOME,
                       VK_LEFT,
                       VK_UP,
                       VK_RIGHT,
                       VK_DOWN,
                       VK_INSERT,
                       VK_DELETE,
                       VK_MULTIPLY,
                       VK_ADD,
                       VK_SUBTRACT,
                       VK_DIVIDE | KBDEXT,
                       VK_NUMLOCK | KBDEXT
                      };

CONST int cMouseVKeys = sizeof(ausMouseVKey) / sizeof(ausMouseVKey[0]);

/*
 * aMouseKeyEvent is an array of function pointers.  The routine to call
 * is selected using the index created by scanning the ausMouseVKey array.
 */
CONST MOUSEPROC aMouseKeyEvent[] = {
    xxxMKButtonClick,      // Numpad 5 (Clear)
    xxxMKMouseMove,        // Numpad 9 (PgUp)
    xxxMKMouseMove,        // Numpad 3 (PgDn)
    xxxMKMouseMove,        // Numpad 1 (End)
    xxxMKMouseMove,        // Numpad 7 (Home)
    xxxMKMouseMove,        // Numpad 4 (Left)
    xxxMKMouseMove,        // Numpad 8 (Up)
    xxxMKMouseMove,        // Numpad 6 (Right)
    xxxMKMouseMove,        // Numpad 2 (Down)
    xxxMKButtonSetState,   // Numpad 0 (Ins)
    xxxMKButtonSetState,   // Numpad . (Del)
    MKButtonSelect,        // Numpad * (Multiply)
    xxxMKButtonDoubleClick,// Numpad + (Add)
    MKButtonSelect,        // Numpad - (Subtract)
    MKButtonSelect,        // Numpad / (Divide)
    xxxMKToggleMouseKeys   // Num Lock
};

/*
 * ausMouseKeyData contains useful data for the routines that process
 * the virtual mousekeys.  This array is indexed using the index created
 * by scanning the ausMouseVKey array.
 */
CONST USHORT ausMouseKeyData[] = {
    0,                     // Numpad 5: Click active button
    MK_UP | MK_RIGHT,      // Numpad 9: Up & Right
    MK_DOWN | MK_RIGHT,    // Numpad 3: Down & Right
    MK_DOWN | MK_LEFT,     // Numpad 1: Down & Left
    MK_UP | MK_LEFT,       // Numpad 7: Up & Left
    MK_LEFT,               // Numpad 4: Left
    MK_UP,                 // Numpad 8: Up
    MK_RIGHT,              // Numpad 6: Right
    MK_DOWN,               // Numpad 2: Down
    FALSE,                 // Numpad 0: Active button down
    TRUE,                  // Numpad .: Active button up
    MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT,   // Numpad *: Select both buttons
    0,                     // Numpad +: Double click active button
    MOUSE_BUTTON_RIGHT,    // Numpad -: Select right button
    MOUSE_BUTTON_LEFT,     // Numpad /: Select left button
    0
};

__inline void
PostAccessNotification(UINT accessKeyType)
{
    if (gspwndLogonNotify != NULL)
    {
        glinp.ptiLastWoken = GETPTI(gspwndLogonNotify);

        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                 LOGON_ACCESSNOTIFY, accessKeyType);
    }
}

void PostRitSound(PTERMINAL pTerm, UINT message) {
    PostEventMessage(
                    pTerm->ptiDesktop,
                    pTerm->ptiDesktop->pq,
                    QEVENT_RITSOUND,
                    NULL,
                    message, 0, 0);
    return;
}

void PostAccessibility( LPARAM lParam )
{
    PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;

    PostEventMessage(
            pTerm->ptiDesktop,
            pTerm->pqDesktop,
            QEVENT_RITACCESSIBILITY,
            NULL,
            0, HSHELL_ACCESSIBILITYSTATE, lParam);
}

/***************************************************************************\
* AccessProceduresStream
*
* This function controls the order in which the access functions are called.
* All key events pass through this routine.  If an access function returns
* FALSE then none of the other access functions in the stream are called.
* This routine is called initially from KeyboardApcProcedure(), but then
* can be called any number of times by the access functions as they process
* the current key event or add more key events.
*
* Return value:
*   TRUE    All access functions returned TRUE, the key event can be
*           processed.
*   FALSE   An access function returned FALSE, the key event should be
*           discarded.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
BOOL AccessProceduresStream(PKE pKeyEvent, ULONG ExtraInformation, int dwProcIndex)
{
    int index;

    CheckCritIn();
    for (index = dwProcIndex; index < ARRAY_SIZE(aAccessibilityProc); index++) {
        if (!aAccessibilityProc[index](pKeyEvent, ExtraInformation, index+1)) {
            return FALSE;
        }
    }

    return TRUE;
}


/***************************************************************************\
* FKActivationTimer
*
* If the hot key (right shift key) is held down this routine is called after
* 4, 8, 12 and 16 seconds.  This routine is only called at the 12 and 16
* second time points if we're in the process of enabling FilterKeys.  If at
* 8 seconds FilterKeys is disabled then this routine will not be called again
* until the hot key is released and then pressed.
*
* This routine is called with the critical section already locked.
*
* Return value:
*    0
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID FKActivationTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{
    UINT TimerDelta;
    PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;

    CheckCritIn();

    switch (gFilterKeysState) {

    case FKFIRSTWARNING:
        //
        // The audible feedback cannot be disabled for this warning.
        //
        /*PostEventMessage(
                    pTerm->ptiDesktop,
                    pTerm->ptiDesktop->pq,
                    QEVENT_RITSOUND,
                    NULL,
                    RITSOUND_DOBEEP, RITSOUND_HIGHBEEP, 3);*/
        TimerDelta = FKACTIVATIONDELTA;
        break;

    case FKTOGGLE:
        if (TEST_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON)) {
            //
            // Disable Filter Keys
            //
            CLEAR_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON);
            if (TEST_ACCESSFLAG(FilterKeys, FKF_HOTKEYSOUND)) {
                PostRitSound(
                    pTerm,
                    RITSOUND_DOWNSIREN);
            }
            PostAccessibility( ACCESS_FILTERKEYS );
            //
            // Stop all timers that are currently running.
            //
            if (gtmridFKResponse != 0) {
                KILLRITTIMER(NULL, gtmridFKResponse);
                gtmridFKResponse = 0;
            }
            if (gtmridFKAcceptanceDelay != 0) {
                KILLRITTIMER(NULL, gtmridFKAcceptanceDelay);
                gtmridFKAcceptanceDelay = 0;
            }

            //
            // Don't reset activation timer.  Emergency levels are only
            // activated after enabling Filter Keys.
            //
            return;
        } else {
            PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;

            if (TEST_ACCESSFLAG(FilterKeys, FKF_HOTKEYSOUND)) {
                PostRitSound(
                    pTerm,
                    RITSOUND_UPSIREN);
            }

            PostAccessNotification(ACCESS_FILTERKEYS);

        }
        TimerDelta = FKEMERGENCY1DELTA;
        break;

    case FKFIRSTLEVELEMERGENCY:
        //
        // First level emergency settings:
        //    Repeat Rate OFF
        //    SlowKeys OFF (Acceptance Delay of 0)
        //    BounceKeys Debounce Time of 1 second
        //
        if (TEST_ACCESSFLAG(FilterKeys, FKF_HOTKEYSOUND)) {
            PostEventMessage(
                    pTerm->ptiDesktop,
                    pTerm->ptiDesktop->pq,
                    QEVENT_RITSOUND,
                    NULL,
                    RITSOUND_DOBEEP, RITSOUND_UPSIREN, 2);
        }
        gFilterKeys.iRepeatMSec = 0;
        gFilterKeys.iWaitMSec = 0;
        gFilterKeys.iBounceMSec = 1000;
        TimerDelta = FKEMERGENCY2DELTA;
        break;

    case FKSECONDLEVELEMERGENCY:
        //
        // Second level emergency settings:
        //    Repeat Rate OFF
        //    SlowKeys Acceptance Delay of 2 seconds
        //    BounceKeys OFF (Debounce Time of 0)
        //
        gFilterKeys.iRepeatMSec = 0;
        gFilterKeys.iWaitMSec = 2000;
        gFilterKeys.iBounceMSec = 0;
        if (TEST_ACCESSFLAG(FilterKeys, FKF_HOTKEYSOUND)) {
            PostEventMessage(
                    pTerm->ptiDesktop,
                    pTerm->ptiDesktop->pq,
                    QEVENT_RITSOUND,
                    NULL,
                    RITSOUND_DOBEEP, RITSOUND_UPSIREN, 3);
        }
        return;
        break;

    default:
        return;
    }

    gFilterKeysState++;
    gtmridFKActivation = InternalSetTimer(
                                    NULL,
                                    nID,
                                    TimerDelta,
                                    FKActivationTimer,
                                    TMRF_RIT | TMRF_ONESHOT
                                    );
    return;

    DBG_UNREFERENCED_PARAMETER(pwnd);
    DBG_UNREFERENCED_PARAMETER(lParam);
    DBG_UNREFERENCED_PARAMETER(message);
}

/***************************************************************************\
* FKBounceKeyTimer
*
* If BounceKeys is active this routine is called after the debounce time
* has expired.  Until then, the last key released will not be accepted as
* input if it is pressed again.
*
* Return value:
*    0
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID FKBounceKeyTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{

    CheckCritIn();
    //
    // All we need to do is clear gBounceVk to allow this key as the
    // next keystroke.
    //
    gBounceVk = 0;
    return;

    DBG_UNREFERENCED_PARAMETER(pwnd);
    DBG_UNREFERENCED_PARAMETER(lParam);
    DBG_UNREFERENCED_PARAMETER(nID);
    DBG_UNREFERENCED_PARAMETER(message);

}

/***************************************************************************\
* xxxFKRepeatRateTimer
*
* If FilterKeys is active and a repeat rate is set, this routine controls
* the rate at which the last key pressed repeats.  The hardware keyboard
* typematic repeat is ignored in this case.
*
* This routine is called with the critical section already locked.
*
* Return value:
*    0
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID xxxFKRepeatRateTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{

    CheckCritIn();
    //
    // Repeat after me...
    //
    if (TEST_ACCESSFLAG(FilterKeys, FKF_CLICKON)) {
        PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
        PostRitSound(
                    pTerm,
                    RITSOUND_KEYCLICK);
    }

    UserAssert(gtmridFKAcceptanceDelay == 0);

    gtmridFKResponse = InternalSetTimer(
                                  NULL,
                                  nID,
                                  gFilterKeys.iRepeatMSec,
                                  xxxFKRepeatRateTimer,
                                  TMRF_RIT | TMRF_ONESHOT
                                  );
    if (AccessProceduresStream(gpFKKeyEvent, gFKExtraInformation, gFKNextProcIndex)) {
        xxxProcessKeyEvent(gpFKKeyEvent, gFKExtraInformation, FALSE);
    }
    return;


    DBG_UNREFERENCED_PARAMETER(pwnd);
    DBG_UNREFERENCED_PARAMETER(lParam);
    DBG_UNREFERENCED_PARAMETER(message);
}

/***************************************************************************\
* xxxFKAcceptanceDelayTimer
*
* If FilterKeys is active and an acceptance delay is set, this routine
* is called after the key has been held down for the acceptance delay
* period.
*
* This routine is called with the critical section already locked.
*
* Return value:
*    0
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID xxxFKAcceptanceDelayTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{

    CheckCritIn();
    //
    // The key has been held down long enough.  Send it on...
    //
    if (TEST_ACCESSFLAG(FilterKeys, FKF_CLICKON)) {
        PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
        PostRitSound(
                    pTerm,
                    RITSOUND_KEYCLICK);
    }

    if (AccessProceduresStream(gpFKKeyEvent, gFKExtraInformation, gFKNextProcIndex)) {
        xxxProcessKeyEvent(gpFKKeyEvent, gFKExtraInformation, FALSE);
    }
    if (!gFilterKeys.iRepeatMSec) {
        //
        // gptmrFKAcceptanceDelay needs to be released, but we can't do it while
        // in a RIT timer routine.  Set a global to indicate that the subsequent
        // break of this key should be passed on and the timer freed.
        //
        SET_ACCF(ACCF_FKMAKECODEPROCESSED);
        return;
    }
    UserAssert(gtmridFKResponse == 0);
    if (gFilterKeys.iDelayMSec) {
        gtmridFKResponse = InternalSetTimer(
                                      NULL,
                                      nID,
                                      gFilterKeys.iDelayMSec,
                                      xxxFKRepeatRateTimer,
                                      TMRF_RIT | TMRF_ONESHOT
                                      );
    } else {
        gtmridFKResponse = InternalSetTimer(
                                      NULL,
                                      nID,
                                      gFilterKeys.iRepeatMSec,
                                      xxxFKRepeatRateTimer,
                                      TMRF_RIT | TMRF_ONESHOT
                                      );
    }
    //
    // gptmrFKAcceptanceDelay timer structure was reused so set handle to NULL.
    //
    gtmridFKAcceptanceDelay = 0;

    return;

    DBG_UNREFERENCED_PARAMETER(lParam);
    DBG_UNREFERENCED_PARAMETER(message);
    DBG_UNREFERENCED_PARAMETER(pwnd);
}

/***************************************************************************\
* FilterKeys
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
BOOL FilterKeys(PKE pKeyEvent, ULONG ExtraInformation, int NextProcIndex)
{
    int fBreak;
    BYTE Vk;

    CheckCritIn();
    Vk = (BYTE)(pKeyEvent->usFlaggedVk & 0xff);
    fBreak = pKeyEvent->usFlaggedVk & KBDBREAK;

    //
    // Check for Filter Keys hot key (right shift key).
    //
    if (Vk == VK_RSHIFT) {
        if (fBreak) {
            if (gtmridFKActivation != 0) {
                KILLRITTIMER(NULL, gtmridFKActivation);
                gtmridFKActivation = 0;
            }
            gFilterKeysState = FKIDLE;
        } else if (ONLYRIGHTSHIFTDOWN(gPhysModifierState)) {
            //
            // Verify that activation via hotkey is allowed.
            //
            if (TEST_ACCESSFLAG(FilterKeys, FKF_HOTKEYACTIVE)) {
                if ((gtmridFKActivation == 0) && (gFilterKeysState != FKMOUSEMOVE)) {
                    gFilterKeysState = FKFIRSTWARNING;
                    gtmridFKActivation = InternalSetTimer(
                                                    NULL,
                                                    0,
                                                    FKFIRSTWARNINGTIME,
                                                    FKActivationTimer,
                                                    TMRF_RIT | TMRF_ONESHOT
                                                    );
                }
            }
        }
    }

    //
    // If another key is pressed while the hot key is down, kill
    // the timer.
    //
    if ((Vk != VK_RSHIFT) && (gtmridFKActivation != 0)) {
        gFilterKeysState = FKIDLE;
        KILLRITTIMER(NULL, gtmridFKActivation);
        gtmridFKActivation = 0;
    }
    //
    // If Filter Keys not enabled send the key event on.
    //
    if (!TEST_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON)) {
        return TRUE;
    }

    if (fBreak) {
        //
        // Kill the current timer and activate bounce key timer (if this is
        // a break of the last key down).
        //
        if (Vk == gLastVkDown) {
            KILLRITTIMER(NULL, gtmridFKResponse);
            gtmridFKResponse = 0;

            gLastVkDown = 0;
            if (gtmridFKAcceptanceDelay != 0) {
                KILLRITTIMER(NULL, gtmridFKAcceptanceDelay);
                gtmridFKAcceptanceDelay = 0;
                if (!TEST_ACCF(ACCF_FKMAKECODEPROCESSED)) {
                    //
                    // This key was released before accepted.  Don't pass on the
                    // break.
                    //
                    return FALSE;
                } else {
                    CLEAR_ACCF(ACCF_FKMAKECODEPROCESSED);
                }
            }

            if (gFilterKeys.iBounceMSec) {
                gBounceVk = Vk;
                gtmridFKResponse = InternalSetTimer(
                                              NULL,
                                              0,
                                              gFilterKeys.iBounceMSec,
                                              FKBounceKeyTimer,
                                              TMRF_RIT | TMRF_ONESHOT
                                              );
                if (TEST_ACCF(ACCF_IGNOREBREAKCODE)) {
                    return FALSE;
                }
            }
        }
    } else {
        //
        // Make key processing
        //
        // First check to see if this is a typematic repeat.  If so, we
        // can ignore this key event.  Our timer will handle any repeats.
        // LastVkDown is cleared during processing of the break.
        //
        if (Vk == gLastVkDown) {
            return FALSE;
        }
        //
        // Remember current Virtual Key down for typematic repeat check.
        //
        gLastVkDown = Vk;

        if (gBounceVk) {
            //
            // BounceKeys is active.  If this is a make of the last
            // key pressed we ignore it.  Only when the BounceKey
            // timer expires or another key is pressed will we accept
            // this key.
            //
            if (Vk == gBounceVk) {
                //
                // Ignore this make event and the subsequent break
                // code.  BounceKey timer will be reset on break.
                //
                SET_ACCF(ACCF_IGNOREBREAKCODE);
                return FALSE;
            } else {
                //
                // We have a make of a new key.  Kill the BounceKey
                // timer and clear gBounceVk.
                //
                UserAssert(gtmridFKResponse);
                if (gtmridFKResponse != 0) {
                    KILLRITTIMER(NULL, gtmridFKResponse);
                    gtmridFKResponse = 0;
                }
                gBounceVk = 0;
            }
        }
        CLEAR_ACCF(ACCF_IGNOREBREAKCODE);

        //
        // Give audible feedback that key was pressed.
        //
        if (TEST_ACCESSFLAG(FilterKeys, FKF_CLICKON)) {
            PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
            PostRitSound(
                    pTerm,
                    RITSOUND_KEYCLICK);
        }

        //
        // If gptmrFKAcceptanceDelay is non-NULL the previous key was
        // not held down long enough to be accepted.  Kill the current
        // timer.  A new timer will be started below for the key we're
        // processing now.
        //
        if (gtmridFKAcceptanceDelay != 0) {
            KILLRITTIMER(NULL, gtmridFKAcceptanceDelay);
            gtmridFKAcceptanceDelay = 0;
        }

        //
        // If gptmrFKResponse is non-NULL a repeat rate timer is active
        // on the previous key.  Kill the timer as we have a new make key.
        //
        if (gtmridFKResponse != 0) {
            KILLRITTIMER(NULL, gtmridFKResponse);
            gtmridFKResponse = 0;
        }

        //
        // Save the current key event for later use if we process an
        // acceptance delay or key repeat.
        //
        *gpFKKeyEvent = *pKeyEvent;
        gFKExtraInformation = ExtraInformation;
        gFKNextProcIndex = NextProcIndex;

        //
        // If there is an acceptance delay, set timer and ignore current
        // key event.  When timer expires, saved key event will be sent.
        //
        if (gFilterKeys.iWaitMSec) {
            gtmridFKAcceptanceDelay = InternalSetTimer(
                                          NULL,
                                          0,
                                          gFilterKeys.iWaitMSec,
                                          xxxFKAcceptanceDelayTimer,
                                          TMRF_RIT | TMRF_ONESHOT
                                          );
            CLEAR_ACCF(ACCF_FKMAKECODEPROCESSED);
            return FALSE;
        }
        //
        // No acceptance delay.  Before sending this key event on the
        // timer routine must be set to either the delay until repeat value
        // or the repeat rate value.  If repeat rate is 0 then ignore
        // delay until repeat.
        //
        if (!gFilterKeys.iRepeatMSec) {
            return TRUE;
        }

        UserAssert(gtmridFKResponse == 0);
        if (gFilterKeys.iDelayMSec) {
            gtmridFKResponse = InternalSetTimer(
                                          NULL,
                                          0,
                                          gFilterKeys.iDelayMSec,
                                          xxxFKRepeatRateTimer,
                                          TMRF_RIT | TMRF_ONESHOT
                                          );
        } else {
            gtmridFKResponse = InternalSetTimer(
                                          NULL,
                                          0,
                                          gFilterKeys.iRepeatMSec,
                                          xxxFKRepeatRateTimer,
                                          TMRF_RIT | TMRF_ONESHOT
                                          );
        }
    }

    return TRUE;
}

/***************************************************************************\
* StopFilterKeysTimers
*
* Called from SystemParametersInfo on SPI_SETFILTERKEYS if FKF_FILTERKEYSON
* is not set.  Timers must be stopped if user turns FilterKeys off.
*
* History:
*   18 Jul 94 GregoryW   Created.
\***************************************************************************/
VOID StopFilterKeysTimers(VOID)
{

    if (gtmridFKResponse != 0) {
        KILLRITTIMER(NULL, gtmridFKResponse);
        gtmridFKResponse = 0;
    }
    if (gtmridFKAcceptanceDelay) {
        KILLRITTIMER(NULL, gtmridFKAcceptanceDelay);
        gtmridFKAcceptanceDelay = 0;
    }
    gLastVkDown = 0;
    gBounceVk = 0;
}

/***************************************************************************\
* xxxStickyKeys
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
BOOL xxxStickyKeys(PKE pKeyEvent, ULONG ExtraInformation, int NextProcIndex)
{
    int fBreak;
    BYTE NewLockBits, NewLatchBits;
    int BitPositions;
    BOOL bChange;
    PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;


    CheckCritIn();
    fBreak = pKeyEvent->usFlaggedVk & KBDBREAK;

    if (gCurrentModifierBit) {
        //
        // Process modifier key
        //

        //
        // One method of activating StickyKeys is to press either the
        // left shift key or the right shift key five times without
        // pressing any other keys.  We don't want the typematic shift
        // (make code) to enable/disable StickyKeys so we perform a
        // special test for them.
        //
        if (!fBreak) {
            if (gCurrentModifierBit & gPrevModifierState) {
                //
                // This is a typematic make of a modifier key.  Don't do
                // any further processing.  Just pass it along.
                //
                gPrevModifierState = gPhysModifierState;
                return TRUE;
            }
        }

        gPrevModifierState = gPhysModifierState;

        if (LEFTSHIFTKEY(pKeyEvent->usFlaggedVk) &&
            ((gPhysModifierState & ~gCurrentModifierBit) == 0)) {
            gStickyKeysLeftShiftCount++;
        } else {
            gStickyKeysLeftShiftCount = 0;
        }
        if (RIGHTSHIFTKEY(pKeyEvent->usFlaggedVk) &&
            ((gPhysModifierState & ~gCurrentModifierBit) == 0)) {
            gStickyKeysRightShiftCount++;
        } else {
            gStickyKeysRightShiftCount = 0;
        }

        //
        // Check to see if StickyKeys should be toggled on/off
        //
        if ((gStickyKeysLeftShiftCount == (TOGGLE_STICKYKEYS_COUNT * 2)) ||
            (gStickyKeysRightShiftCount == (TOGGLE_STICKYKEYS_COUNT * 2))) {
            if (TEST_ACCESSFLAG(StickyKeys, SKF_HOTKEYACTIVE)) {
                if (TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON)) {
                    xxxTurnOffStickyKeys();
                    if (TEST_ACCESSFLAG(StickyKeys, SKF_HOTKEYSOUND)) {
                        PostRitSound(
                            pTerm,
                            RITSOUND_DOWNSIREN);
                    }
                } else {
                    if (TEST_ACCESSFLAG(StickyKeys, SKF_HOTKEYSOUND)) {
                        PostRitSound(
                            pTerm,
                            RITSOUND_UPSIREN);
                    }
                    // To make the notification window get the focus
                    // The same is done other places where WM_LOGONNOTIFY message is
                    // sent    : a-anilk
                    PostAccessNotification(ACCESS_STICKYKEYS);

                }
            }
            gStickyKeysLeftShiftCount = 0;
            gStickyKeysRightShiftCount = 0;
            return TRUE;
        }

        //
        // If StickyKeys is enabled process the modifier key, otherwise
        // just pass on the modifier key.
        //
        if (TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON)) {
            if (fBreak) {
                //
                // If either locked or latched bit set for this key then
                // don't pass the break on.
                //
                if (UNION(gLatchBits, gLockBits) & gCurrentModifierBit) {
                    return FALSE;
                } else {
                    return TRUE;
                }
            } else{
                if (gPhysModifierState != gCurrentModifierBit) {
                    //
                    // More than one modifier key down at the same time.
                    // This condition may signal sticky keys to turn off.
                    // The routine xxxTwoKeysDown will return the new value
                    // of fStickyKeysOn.  If sticky keys is turned off
                    // (return value 0), the key event should be passed
                    // on without further processing here.
                    //
                    if (!xxxTwoKeysDown(NextProcIndex)) {
                        return TRUE;
                    }

                    //
                    // Modifier states were set to physical state by
                    // xxxTwoKeysDown.  The modifier keys currently in
                    // the down position will be latched by updating
                    // gLatchBits.  No more processing for this key
                    // event is needed.
                    //
                    bChange = gLockBits ||
                              (gLatchBits != gPhysModifierState);
                    gLatchBits = gPhysModifierState;
                    gLockBits = 0;
                    if (bChange) {
                        PostAccessibility( ACCESS_STICKYKEYS );
                    }

                    //
                    // Provide sound feedback, if enabled, before returning.
                    //
                    if (TEST_ACCESSFLAG(StickyKeys, SKF_AUDIBLEFEEDBACK)) {
                        PostRitSound(
                            pTerm,
                            RITSOUND_LOWBEEP);
                        PostRitSound(
                            pTerm,
                            RITSOUND_HIGHBEEP);
                    }
                    return FALSE;
                }
                //
                // Figure out which bits (Shift, Ctrl or Alt key bits) to
                // examine.  Also set up default values for NewLatchBits
                // and NewLockBits in case they're not set later.
                //
                // See the depiction of the bit pattern in KeyboardApcProcedure.
                //
                // Bit 0 -- L SHIFT
                // Bit 1 -- R SHIFT
                // Bit 2 -- L CTL
                // Bit 3 -- R CTL
                // Bit 4 -- L ALT
                // Bit 5 -- R RLT
                // Bit 6 -- L WIN
                // Bit 7 -- R WIN
                switch(pKeyEvent->usFlaggedVk) {
                case VK_LSHIFT:
                case VK_RSHIFT:
                    BitPositions = 0x3;
                    break;
                case VK_LCONTROL:
                case VK_RCONTROL:
                    BitPositions = 0xc;
                    break;
                case VK_LMENU:
                case VK_RMENU:
                    BitPositions = 0x30;
                    break;
                case VK_LWIN:
                case VK_RWIN:
                    BitPositions = 0xc0;
                    break;
                }
                NewLatchBits = gLatchBits;
                NewLockBits = gLockBits;

                //
                // If either left or right modifier is locked clear latched
                // and locked states and send appropriate break/make messages.
                //
                if (gLockBits & BitPositions) {
                    NewLockBits = gLockBits & ~BitPositions;
                    NewLatchBits = gLatchBits & ~BitPositions;
                    xxxUpdateModifierState(
                        NewLockBits | NewLatchBits | gCurrentModifierBit,
                        NextProcIndex
                        );
                } else {
                    //
                    // If specific lock bit (left or right) not
                    // previously set then toggle latch bits.
                    //
                    if (!(gLockBits & gCurrentModifierBit)) {
                        NewLatchBits = gLatchBits ^ gCurrentModifierBit;
                    }
                    //
                    // If locked mode (tri-state) enabled then if latch or lock
                    // bit previously set, toggle lock bit.
                    //
                    if (TEST_ACCESSFLAG(StickyKeys, SKF_TRISTATE)) {
                        if (UNION(gLockBits, gLatchBits) & gCurrentModifierBit) {
                            NewLockBits = gLockBits ^ gCurrentModifierBit;
                        }
                    }
                }

                //
                // Update globals
                //
                bChange = ((gLatchBits != NewLatchBits) ||
                           (gLockBits != NewLockBits));

                gLatchBits = NewLatchBits;
                gLockBits = NewLockBits;

                if (bChange) {
                    PostAccessibility( ACCESS_STICKYKEYS );
                }
                //
                // Now provide sound feedback if enabled.  For the transition
                // to LATCH mode issue a low beep then a high beep.  For the
                // transition to LOCKED mode issue a high beep.  For the
                // transition out of LOCKED mode (or LATCH mode if tri-state
                // not enabled) issue a low beep.
                //
                if (TEST_ACCESSFLAG(StickyKeys, SKF_AUDIBLEFEEDBACK)) {
                    if (!(gLockBits & gCurrentModifierBit)) {
                        PostRitSound(
                            pTerm,
                            RITSOUND_LOWBEEP);
                    }
                    if ((gLatchBits | gLockBits) & gCurrentModifierBit) {
                        PostRitSound(
                            pTerm,
                            RITSOUND_HIGHBEEP);
                    }
                }
                //
                // Pass key on if shift bit is set (e.g., if transitioning
                // from shift to lock mode don't pass on make).
                //
                if (gLatchBits & gCurrentModifierBit) {
                    return TRUE;
                } else {
                    return FALSE;
                }

            }
        }
    } else {
        //
        // Non-shift key processing here...
        //
        gStickyKeysLeftShiftCount = 0;
        gStickyKeysRightShiftCount = 0;
        if (!TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON)) {
            return TRUE;
        }

        //
        // If no modifier keys are down, or this is a break, pass the key event
        // on and clear any latch states.
        //
        if (!gPhysModifierState || fBreak) {
            if (AccessProceduresStream(pKeyEvent, ExtraInformation, NextProcIndex)) {
                xxxProcessKeyEvent(pKeyEvent, ExtraInformation, FALSE);
            }
            xxxUpdateModifierState(gLockBits, NextProcIndex);

            bChange = gLatchBits != 0;
            gLatchBits = 0;
            if (bChange) {

                PostAccessibility( ACCESS_STICKYKEYS );
            }
            return FALSE;
        } else {
            //
            // This is a make of a non-modifier key and there is a modifier key
            // down.  Update the states and pass the key event on.
            //
            xxxTwoKeysDown(NextProcIndex);
            return TRUE;
        }
    }

    return TRUE;
}

/***************************************************************************\
* xxxUpdateModifierState
*
* Starting from the current modifier keys state, send the necessary key
* events (make or break) to end up with the NewModifierState passed in.
*
* Return value:
*    None.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID xxxUpdateModifierState(int NewModifierState, int NextProcIndex)
{
    KE ke;
    int CurrentModState;
    int CurrentModBit, NewModBit;
    int i;

    CheckCritIn();

    CurrentModState = gLockBits | gLatchBits;

    for (i = 0; i < ARRAY_SIZE(aModBit); i++) {
        CurrentModBit = CurrentModState & aModBit[i].BitPosition;
        NewModBit = NewModifierState & aModBit[i].BitPosition;
        if (CurrentModBit != NewModBit) {
            ke.bScanCode = (BYTE)aModBit[i].ScanCode;
            ke.usFlaggedVk = aModBit[i].Vk;
            if (CurrentModBit) {          // if it's currently on, send break
                ke.usFlaggedVk |= KBDBREAK;
            }
            if (AccessProceduresStream(&ke, 0L, NextProcIndex)) {
                xxxProcessKeyEvent(&ke, 0L, FALSE);
            }
        }
    }
}

/***************************************************************************\
* xxxTurnOffStickyKeys
*
* The user either pressed the appropriate key sequence or used the
* access utility to turn StickyKeys off.  Update modifier states and
* reset globals.
*
* Return value:
*   None.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID xxxTurnOffStickyKeys(VOID)
{
    INT index;
    
    CheckCritIn();

    for (index = 0; index < ARRAY_SIZE(aAccessibilityProc); index++) {
        if (aAccessibilityProc[index] == xxxStickyKeys) {

            xxxUpdateModifierState(gPhysModifierState, index+1);
            gLockBits = gLatchBits = 0;
            CLEAR_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON);

            PostAccessibility( ACCESS_STICKYKEYS );
            break;
        }
    }
}

/***************************************************************************\
* xxxUnlatchStickyKeys
*
* This routine releases any sticky keys that are latched.  This routine
* is called during mouse up event processing.
*
* Return value:
*   None.
*
* History:
*   21 Jun 93 GregoryW   Created.
\***************************************************************************/
VOID xxxUnlatchStickyKeys(VOID)
{
    INT index;
    BOOL bChange;

    if (!gLatchBits) {
        return;
    }

    for (index = 0; index < ARRAY_SIZE(aAccessibilityProc); index++) {
        if (aAccessibilityProc[index] == xxxStickyKeys) {
            xxxUpdateModifierState(gLockBits, index+1);
            bChange = gLatchBits != 0;
            gLatchBits = 0;

            if (bChange) {

                PostAccessibility( ACCESS_STICKYKEYS );
            }
            break;
        }
    }
}


/***************************************************************************\
* xxxHardwareMouseKeyUp
*
* This routine is called during a mouse button up event.  If MouseKeys is
* on and the button up event corresponds to a mouse key that's locked down,
* the mouse key must be released.
*
* If StickyKeys is on, all latched keys are released.
*
* Return value:
*   None.
*
* History:
*   17 Jun 94 GregoryW   Created.
\***************************************************************************/

VOID xxxHardwareMouseKeyUp(DWORD dwButton)
{
    CheckCritIn();

    if (TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)) {
        gwMKButtonState &= ~dwButton;
    }

	// Not required to post a setting change
    //PostAccessibility( SPI_SETMOUSEKEYS );

    if (TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON)) {
        xxxUnlatchStickyKeys();
    }
}


/***************************************************************************\
* xxxTwoKeysDown
*
* Two keys are down simultaneously.  Check to see if StickyKeys should be
* turned off.  In all cases update the modifier key state to reflect the
* physical key state and clear latched and locked modes.
*
* Return value:
*    1 if StickyKeys is enabled.
*    0 if StickyKeys is disabled.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
BOOL xxxTwoKeysDown(int NextProcIndex)
{
    PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;

    if (TEST_ACCESSFLAG(StickyKeys, SKF_TWOKEYSOFF)) {
        CLEAR_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON);
        if (TEST_ACCESSFLAG(StickyKeys, SKF_HOTKEYSOUND)) {
            PostRitSound(
                    pTerm,
                    RITSOUND_DOWNSIREN);
        }
        gStickyKeysLeftShiftCount = 0;
        gStickyKeysRightShiftCount = 0;
    }
    xxxUpdateModifierState(gPhysModifierState, NextProcIndex);
    gLockBits = gLatchBits = 0;

    PostAccessibility( ACCESS_STICKYKEYS );

    return TEST_BOOL_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON);
}

/***************************************************************************\
* SetGlobalCursorLevel
*
* Set the cursor level of all threads running on the visible
* windowstation.
*
* History:
* 04-17-95 JimA         Created.
\***************************************************************************/

VOID SetGlobalCursorLevel(
    INT iCursorLevel)
{

/*
 * LATER
 * We have other code which assumes that the
 * iCursorLevel of a queue is the sum of the iCursorLevel values for the
 * threads attached to the queue.  But this code, if you set iCursorLevel to
 * -1 (to indicate no mouse) will set the queue iCursorLevel to -1, no matter
 * how many threads are attached to the queue.  This needs to be revisited.
 * See the function AttachToQueue.
 *  FritzS
 */


    PDESKTOP pdesk;
    PTHREADINFO pti;
    PLIST_ENTRY pHead, pEntry;

    TAGMSG1(DBGTAG_PNP, "SetGlobalCursorLevel %x", iCursorLevel);

    if (grpdeskRitInput) {
        for (pdesk = grpdeskRitInput->rpwinstaParent->rpdeskList;
                pdesk != NULL; pdesk = pdesk->rpdeskNext) {

            pHead = &pdesk->PtiList;
            for (pEntry = pHead->Flink; pEntry != pHead; pEntry = pEntry->Flink) {
                pti = CONTAINING_RECORD(pEntry, THREADINFO, PtiLink);

                pti->iCursorLevel = iCursorLevel;
                pti->pq->iCursorLevel = iCursorLevel;
            }
        }
    }

    /*
     * CSRSS doesn't seem to be on the list, so fix it up now.
     */
    for (pti = PpiFromProcess(gpepCSRSS)->ptiList;
            pti != NULL; pti = pti->ptiSibling) {
        if (pti->iCursorLevel != iCursorLevel) {
            TAGMSG3(DBGTAG_PNP, "pti %#p has cursorlevel %x, should be %x",
                    pti, pti->iCursorLevel, iCursorLevel);
        }
        if (pti->pq->iCursorLevel != iCursorLevel) {
            TAGMSG4(DBGTAG_PNP, "pti->pq %#p->%#p has cursorlevel %x, should be %x",
                    pti, pti->pq, pti->pq->iCursorLevel, iCursorLevel);
        }
        pti->iCursorLevel = iCursorLevel;
        pti->pq->iCursorLevel = iCursorLevel;
    }
}

/***************************************************************************\
* MKShowMouseCursor
*
* If no hardware mouse is installed and MouseKeys is enabled, we need
* to fix up the system metrics, the oem information and the queue
* information.  The mouse cursor then gets displayed.
*
* Return value:
*    None.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID MKShowMouseCursor()
{
    TAGMSG1(DBGTAG_PNP, "MKShowMouseCursor (gpDeviceInfoList == %#p)", gpDeviceInfoList);

    //
    // If TEST_GTERMF(GTERMF_MOUSE) is TRUE then we either have a hardware mouse
    // or we're already pretending a mouse is installed.  In either case,
    // there's nothing to do so just return.
    //
    if (TEST_GTERMF(GTERMF_MOUSE)) {
        TAGMSG0(DBGTAG_PNP, "MKShowMouseCursor just returns");
        return;
    }

    SET_GTERMF(GTERMF_MOUSE);
    SET_ACCF(ACCF_MKVIRTUALMOUSE);
    SYSMET(MOUSEPRESENT) = TRUE;
    SYSMET(CMOUSEBUTTONS) = 2;
    /*
     * HACK: CreateQueue() uses oemInfo.fMouse to determine if a mouse is
     * present and thus whether to set the iCursorLevel field in the
     * THREADINFO structure to 0 or -1.  Unfortunately some queues have
     * already been created at this point.  Since oemInfo.fMouse is
     * initialized to FALSE, we need to go back through any queues already
     * around and set their iCursorLevel field to the correct value when
     * mousekeys is enabled.
     */
    SetGlobalCursorLevel(0);
}

/***************************************************************************\
* MKHideMouseCursor
*
* If no hardware mouse is installed and MouseKeys is disabled, we need
* to fix up the system metrics, the oem information and the queue
* information.  The mouse cursor then disappears.
*
* Return value:
*    None.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID MKHideMouseCursor()
{
    TAGMSG1(DBGTAG_PNP, "MKHideMouseCursor (gpDeviceInfoList == %#p)", gpDeviceInfoList);

    //
    // If a hardware mouse is present we don't need to do anything.
    //
    if (!TEST_ACCF(ACCF_MKVIRTUALMOUSE)) {
        return;
    }

    CLEAR_ACCF(ACCF_MKVIRTUALMOUSE);
    CLEAR_GTERMF(GTERMF_MOUSE);
    SYSMET(MOUSEPRESENT) = FALSE;
    SYSMET(CMOUSEBUTTONS) = 0;

    SetGlobalCursorLevel(-1);
}

/***************************************************************************\
* xxxMKToggleMouseKeys
*
* This routine is called when the NumLock key is pressed and MouseKeys is
* active.  If the left shift key and the left alt key are down then MouseKeys
* is turned off.  If just the NumLock key is pressed then we toggle between
* MouseKeys active and the state of the number pad before MouseKeys was
* activated.
*
* Return value:
*    TRUE  - key should be passed on in the input stream.
*    FALSE - key should not be passed on.
*
* History:
\***************************************************************************/
BOOL xxxMKToggleMouseKeys(USHORT NotUsed)
{
    BOOL bRetVal = TRUE;
    BOOL bNewPassThrough;
    PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;


    //
    // If this is a typematic repeat of NumLock we just pass it on.
    //
    if (TEST_ACCF(ACCF_MKREPEATVK)) {
        return bRetVal;
    }
    //
    // This is a make of NumLock.  Check for disable sequence.
    //
    if ((gLockBits | gLatchBits | gPhysModifierState) == MOUSEKEYMODBITS) {
        if (TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYACTIVE)) {
            if (!gbMKMouseMode) {
               //
               // User wants to turn MouseKeys off.  If we're currently in
               // pass through mode then the NumLock key is in the same state
               // (on or off) as it was when the user invoked MouseKeys.  We
               // want to leave it in that state, so don't pass the NumLock
               // key on.
               //
               bRetVal = FALSE;
            }
            TurnOffMouseKeys();
        }
        return bRetVal;
    }
    /*
     * This is a NumLock with no modifiers.  Toggle current state and
     * provide audible feedback.
     *
     * Note -- this test is the reverse of other ones because it tests the
     * state of VK_NUMLOCK before the keypress flips the state of NUMLOCK.
     * So the code checks for what the state will be.
     */
    bNewPassThrough =
#ifdef FE_SB // MouseKeys()
        (TestAsyncKeyStateToggle(gNumLockVk) != 0) ^
#else  // FE_SB
        (TestAsyncKeyStateToggle(VK_NUMLOCK) != 0) ^
#endif // FE_SB
             (TEST_ACCESSFLAG(MouseKeys, MKF_REPLACENUMBERS) != 0);


    if (!bNewPassThrough) {
        gbMKMouseMode = TRUE;
        PostRitSound(
              pTerm,
              RITSOUND_HIGHBEEP);
    } else {
        WORD SaveCurrentActiveButton;
        //
        // User wants keys to be passed on.  Release all buttons currently
        // down.
        //
        gbMKMouseMode = FALSE;
        PostRitSound(
              pTerm,
              RITSOUND_LOWBEEP);
        SaveCurrentActiveButton = gwMKCurrentButton;
        gwMKCurrentButton = MOUSE_BUTTON_LEFT | MOUSE_BUTTON_RIGHT;
        xxxMKButtonSetState(TRUE);
        gwMKCurrentButton = SaveCurrentActiveButton;
    }

    PostAccessibility( ACCESS_MOUSEKEYS );

    return bRetVal;


    DBG_UNREFERENCED_PARAMETER(NotUsed);
}

/***************************************************************************\
* xxxMKButtonClick
*
* Click the active mouse button.
*
* Return value:
*    Always FALSE - key should not be passed on.
*
* History:
\***************************************************************************/
BOOL xxxMKButtonClick(USHORT NotUsed)
{
    //
    // The button click only happens on initial make of key.  If this is a
    // typematic repeat we just ignore it.
    //
    if (TEST_ACCF(ACCF_MKREPEATVK)) {
        return FALSE;
    }
    //
    // Ensure active button is UP before the click
    //
    xxxMKButtonSetState(TRUE);
    //
    // Now push the button DOWN
    //
    xxxMKButtonSetState(FALSE);
    //
    // Now release the button
    //
    xxxMKButtonSetState(TRUE);

    return FALSE;


    UNREFERENCED_PARAMETER(NotUsed);
}


/***************************************************************************\
* xxxMKMoveConstCursorTimer
*
* Timer routine that handles constant speed mouse movement.  This routine
* is called 20 times per second and uses information from
* gMouseCursor.bConstantTable[] to determine how many pixels to move the
* mouse cursor on each tick.
*
* Return value:
*    None.
*
* History:
\***************************************************************************/
VOID xxxMKMoveConstCursorTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{
    LONG  MovePixels;

    CheckCritIn();

    if (TEST_ACCESSFLAG(MouseKeys, MKF_MODIFIERS)) {
        if ((gLockBits | gLatchBits | gPhysModifierState) & LRSHIFT) {
            MovePixels = 1;
            goto MoveIt;
        }
        if ((gLockBits | gLatchBits | gPhysModifierState) & LRCONTROL) {
            MovePixels = gMouseCursor.bConstantTable[0] * MK_CONTROL_SPEED;
            goto MoveIt;
        }
    }

    giMouseMoveTable %= gMouseCursor.bConstantTableLen;

    MovePixels = gMouseCursor.bConstantTable[giMouseMoveTable++];

    if (MovePixels == 0) {
        return;
    }

MoveIt:
    //
    // We're inside the critical section - leave before calling MoveEvent.
    // Set gbMouseMoved to TRUE so RawInputThread wakes up the appropriate
    // user thread (if any) to receive this event.
    //
    LeaveCrit();

    xxxMoveEvent(MovePixels * gMKDeltaX, MovePixels * gMKDeltaY, 0, 0, 0, FALSE);
    QueueMouseEvent(0, 0, 0, gptCursorAsync, NtGetTickCount(), FALSE, TRUE);
    EnterCrit();
    return;


    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(lParam);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(message);
}

/***************************************************************************\
* xxxMKMoveAccelCursorTimer
*
* Timer routine that handles mouse acceleration.  It gets called 20 times
* per second and uses information from gMouseCursor.bAccelTable[] to determine
* how many pixels to move the mouse cursor on each tick.
*
* Return value:
*    None.
*
* History:
\***************************************************************************/
VOID xxxMKMoveAccelCursorTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{
    LONG  MovePixels;

    CheckCritIn();

    if (TEST_ACCESSFLAG(MouseKeys, MKF_MODIFIERS)) {
        if ((gLockBits | gLatchBits | gPhysModifierState) & LRSHIFT) {
            MovePixels = 1;
            goto MoveIt;
        }
        if ((gLockBits | gLatchBits | gPhysModifierState) & LRCONTROL) {
            MovePixels = gMouseCursor.bConstantTable[0] * MK_CONTROL_SPEED;
            goto MoveIt;
        }
    }

    if (giMouseMoveTable < gMouseCursor.bAccelTableLen) {
        MovePixels = gMouseCursor.bAccelTable[giMouseMoveTable++];
    } else {
        //
        // We've reached maximum cruising speed.  Switch to constant table.
        //
        MovePixels = gMouseCursor.bConstantTable[0];
        giMouseMoveTable = 1;
        gtmridMKMoveCursor = InternalSetTimer(
                                        NULL,
                                        gtmridMKMoveCursor,
                                        MOUSETIMERRATE,
                                        xxxMKMoveConstCursorTimer,
                                        TMRF_RIT
                                        );

    }
    if (MovePixels == 0) {
        return;
    }

MoveIt:
    //
    // We're inside the critical section - leave before calling xxxMoveEvent.
    // Set gbMouseMoved to TRUE so RawInputThread wakes up the appropriate
    // user thread (if any) to receive this event.
    //
    LeaveCrit();
    xxxMoveEvent(MovePixels * gMKDeltaX, MovePixels * gMKDeltaY, 0, 0, 0, FALSE);
    QueueMouseEvent(0, 0, 0, gptCursorAsync, NtGetTickCount(), FALSE, TRUE);

    EnterCrit();

    return;

    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(lParam);
}

/***************************************************************************\
* xxxMKMouseMove
*
* Send a mouse move event.  A timer routine is set to handle the mouse
* cursor acceleration.  The timer will be set on the first make of a
* mouse move key if FilterKeys repeat rate is OFF.  Otherwise, the timer
* is set on the first repeat (typematic make) of the mouse move key.
* Once the timer is set the timer routine handles all mouse movement
* until the key is released or a new key is pressed.
*
* Return value:
*    Always FALSE - key should not be passed on.
*
* History:
\***************************************************************************/
BOOL xxxMKMouseMove(USHORT Data)
{


    /*
     * Let the mouse acceleration timer routine handle repeats.
     */
    if (TEST_ACCF(ACCF_MKREPEATVK) && (gtmridMKMoveCursor != 0)) {
        return FALSE;
    }


    gMKDeltaX = (LONG)((CHAR)LOBYTE(Data));   // Force sign extension
    gMKDeltaY = (LONG)((CHAR)HIBYTE(Data));   // Force sign extension

    LeaveCrit();

    if ((TEST_ACCESSFLAG(MouseKeys, MKF_MODIFIERS) && ((gLockBits | gLatchBits | gPhysModifierState) & LRCONTROL))) {
        xxxMoveEvent(gMKDeltaX * MK_CONTROL_SPEED * gMouseCursor.bConstantTable[0], gMKDeltaY * MK_CONTROL_SPEED * gMouseCursor.bConstantTable[0], 0, 0, 0, FALSE);
    } else {
        xxxMoveEvent(gMKDeltaX, gMKDeltaY, 0, 0, 0, FALSE);
    }

    QueueMouseEvent(0, 0, 0, gptCursorAsync, NtGetTickCount(), FALSE, TRUE);

    EnterCrit();

    /*
     * If the repeat rate is zero we'll start the mouse acceleration
     * immediately.  Otherwise we wait until after the first repeat
     * of the mouse movement key.
     */
    if (!gFilterKeys.iRepeatMSec || TEST_ACCF(ACCF_MKREPEATVK)) {
        giMouseMoveTable = 0;
        gtmridMKMoveCursor = InternalSetTimer(
                                        NULL,
                                        gtmridMKMoveCursor,
                                        MOUSETIMERRATE,
                                        (gMouseCursor.bAccelTableLen) ?
                                            xxxMKMoveAccelCursorTimer :
                                            xxxMKMoveConstCursorTimer,
                                        TMRF_RIT
                                        );
    }
    return FALSE;
}

/***************************************************************************\
* xxxMKButtonSetState
*
* Set the active mouse button(s) to the state specified by fButtonUp
* (if fButtonUp is TRUE then the button is released, o.w. the button
*  is pressed).
*
* Return value:
*    Always FALSE - key should not be passed on.
*
* History:
\***************************************************************************/
BOOL xxxMKButtonSetState(USHORT fButtonUp)
{
    WORD NewButtonState;

    CheckCritIn();
    if (fButtonUp) {
        NewButtonState = gwMKButtonState & ~gwMKCurrentButton;
    } else {
        NewButtonState = gwMKButtonState | gwMKCurrentButton;
    }

    if ((NewButtonState & MOUSE_BUTTON_LEFT) != (gwMKButtonState & MOUSE_BUTTON_LEFT)) {
        xxxButtonEvent(MOUSE_BUTTON_LEFT,
                       gptCursorAsync,
                       fButtonUp,
                       NtGetTickCount(),
                       0L,
                       FALSE,
                       FALSE);
    }
    if ((NewButtonState & MOUSE_BUTTON_RIGHT) != (gwMKButtonState & MOUSE_BUTTON_RIGHT)) {
        xxxButtonEvent(MOUSE_BUTTON_RIGHT,
                       gptCursorAsync,
                       fButtonUp,
                       NtGetTickCount(),
                       0L,
                       FALSE,
                       FALSE);
    }
    gwMKButtonState = NewButtonState;

    PostAccessibility( ACCESS_MOUSEKEYS );

    return FALSE;
}

/***************************************************************************\
* MKButtonSelect
*
* Mark ThisButton as the active mouse button.  It's possible to select both
* the left and right mouse buttons as active simultaneously.
*
* Return value:
*    Always FALSE - key should not be passed on.
*
* History:
\***************************************************************************/
BOOL MKButtonSelect(WORD ThisButton)
{
    gwMKCurrentButton = ThisButton;

    PostAccessibility( ACCESS_MOUSEKEYS );
    return FALSE;
}

/***************************************************************************\
* xxxMKButtonDoubleClick
*
* Double click the active mouse button.
*
* Return value:
*    Always FALSE - key should not be passed on.
*
* History:
\***************************************************************************/
BOOL xxxMKButtonDoubleClick(USHORT NotUsed)
{
    xxxMKButtonClick(0);
    xxxMKButtonClick(0);
    return FALSE;

    UNREFERENCED_PARAMETER(NotUsed);
}

BOOL HighContrastHotKey(PKE pKeyEvent, ULONG ExtraInformation, int NotUsed) {
    int CurrentModState;
    int fBreak;
    BYTE Vk;


    CheckCritIn();

    Vk = (BYTE)(pKeyEvent->usFlaggedVk & 0xff);
    fBreak = pKeyEvent->usFlaggedVk & KBDBREAK;
    CurrentModState = gLockBits | gLatchBits | gPhysModifierState;

    if (!TEST_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON)) {
        if (TEST_ACCESSFLAG(HighContrast, HCF_HOTKEYACTIVE) && Vk == VK_SNAPSHOT && !fBreak && CurrentModState == MOUSEKEYMODBITS) {

            if (TEST_ACCESSFLAG(HighContrast, MKF_HOTKEYSOUND)) {
                PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
                PostRitSound(
                    pTerm,
                    RITSOUND_UPSIREN);
            }

            PostAccessNotification(ACCESS_HIGHCONTRAST);

            return FALSE;
        }
    } else {
        if (TEST_ACCESSFLAG(HighContrast, HCF_HOTKEYACTIVE) && Vk == VK_SNAPSHOT && !fBreak && CurrentModState == MOUSEKEYMODBITS) {

            CLEAR_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON);

                        if (TEST_ACCESSFLAG(HighContrast, MKF_HOTKEYSOUND)) {
                PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
                PostRitSound(
                    pTerm,
                    RITSOUND_DOWNSIREN);
            }

            if (gspwndLogonNotify != NULL) {

            _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                         LOGON_ACCESSNOTIFY, ACCESS_HIGHCONTRASTOFF);
            }
        }
    }
    return TRUE;  // send key event to next accessibility routine.
    UNREFERENCED_PARAMETER(NotUsed);
    UNREFERENCED_PARAMETER(ExtraInformation);
}


/***************************************************************************\
* MouseKeys
*
* This is the strategy routine that gets called as part of the input stream
* processing.  MouseKeys enabling/disabling is handled here.  All MouseKeys
* helper routines are called from this routine.
*
* Return value:
*    TRUE  - key event should be passed on to the next access routine.
*    FALSE - key event was processed and should not be passed on.
*
* History:
\***************************************************************************/
BOOL MouseKeys(PKE pKeyEvent, ULONG ExtraInformation, int NotUsed)
{
    int CurrentModState;
    int fBreak;
    BYTE Vk;
    USHORT FlaggedVk;
    int i;

    CheckCritIn();
    Vk = (BYTE)(pKeyEvent->usFlaggedVk & 0xff);
    fBreak = pKeyEvent->usFlaggedVk & KBDBREAK;
    CurrentModState = gLockBits | gLatchBits | gPhysModifierState;

    if (!TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)) {
        //
        // MouseKeys currently disabled.  Check for enabling sequence:
        //   left Shift + left Alt + Num Lock.
        //
#ifdef FE_SB // MouseKeys()
        if (TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYACTIVE) && Vk == gNumLockVk && !fBreak && CurrentModState == MOUSEKEYMODBITS) {
#else  // FE_SB
        if (TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYACTIVE) && Vk == VK_NUMLOCK && !fBreak && CurrentModState == MOUSEKEYMODBITS) {
#endif // FE_SB
            gMKPreviousVk = Vk;
            if (TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYSOUND)) {
                PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
                PostRitSound(
                    pTerm,
                    RITSOUND_UPSIREN);
            }
            PostAccessNotification(ACCESS_MOUSEKEYS);

            return FALSE;
        }
    } else {
        //
        // Is this a MouseKey key?
        //
        //
        FlaggedVk = Vk | (pKeyEvent->usFlaggedVk & KBDEXT);
        for (i = 0; i < cMouseVKeys; i++) {
#ifdef FE_SB // MouseKeys()
            if (FlaggedVk == gpusMouseVKey[i]) {
#else  // FE_SB
            if (FlaggedVk == ausMouseVKey[i]) {
#endif // FE_SB
                break;
            }
        }

        if (i == cMouseVKeys) {
            return TRUE;          // not a mousekey
        }
        //
        // Check to see if we should pass on key events until Num Lock is
        // entered.
        //

        if (!gbMKMouseMode) {
#ifdef FE_SB // MouseKeys()
            if (Vk != gNumLockVk) {
#else  // FE_SB
            if (Vk != VK_NUMLOCK) {
#endif // FE_SB
                return TRUE;
            }
        }

        //
        // Check for Ctrl-Alt-Numpad Del.  Pass key event on if sequence
        // detected.
        //
        if (Vk == VK_DELETE && CurrentModState & LRALT && CurrentModState & LRCONTROL) {
            return TRUE;
        }
        if (fBreak) {
            //
            // If this is a break of the key that we're accelerating then
            // kill the timer.
            //
            if (gMKPreviousVk == Vk) {
                if (gtmridMKMoveCursor != 0) {
                    KILLRITTIMER(NULL, gtmridMKMoveCursor);
                    gtmridMKMoveCursor = 0;
                }
                CLEAR_ACCF(ACCF_MKREPEATVK);
                gMKPreviousVk = 0;
            }
            //
            // Pass break of Numlock along.  Other mousekeys stop here.
            //
#ifdef FE_SB // MouseKeys()
            if (Vk == gNumLockVk) {
#else  // FE_SB
            if (Vk == VK_NUMLOCK) {
#endif // FE_SB
                return TRUE;
            } else {
                return FALSE;
            }
        } else {
            SET_OR_CLEAR_ACCF(ACCF_MKREPEATVK,
                              (gMKPreviousVk == Vk));
            //
            // If this is not a typematic repeat, kill the mouse acceleration
            // timer.
            //
            if ((!TEST_ACCF(ACCF_MKREPEATVK)) && (gtmridMKMoveCursor)) {
                KILLRITTIMER(NULL, gtmridMKMoveCursor);
                gtmridMKMoveCursor = 0;
            }
            gMKPreviousVk = Vk;
        }
        return aMouseKeyEvent[i](ausMouseKeyData[i]);
    }
    return TRUE;

    UNREFERENCED_PARAMETER(NotUsed);
    UNREFERENCED_PARAMETER(ExtraInformation);

}

/***************************************************************************\
* TurnOffMouseKeys
*
* Return value:
*    None.
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID TurnOffMouseKeys(VOID)
{
    CLEAR_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON);
//    gMKPassThrough = 0;
    CLEAR_ACCF(ACCF_MKREPEATVK);
    MKHideMouseCursor();
    if (TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYSOUND)) {
        PostRitSound(
            grpdeskRitInput->rpwinstaParent->pTerm,
            RITSOUND_DOWNSIREN);
    }
    PostAccessibility( ACCESS_MOUSEKEYS );
}


/***************************************************************************\
* CalculateMouseTable
*
* Set mouse table based on time to max speed and max speed.  This routine
* is called during user logon (after the registry entries for the access
* features are read).
*
* Return value:
*    None.
*
* History:
*    Taken from access utility.
*
****************************************************************************/
VOID CalculateMouseTable(VOID)
{
    long    Total_Distance;         /* in 1000th of pixel */

    long    Accel_Per_Tick;         /* in 1000th of pixel/tick */
    long    Current_Speed;          /* in 1000th of pixel/tick */
    long    Max_Speed;              /* in 1000th of pixel/tick */
    long    Real_Total_Distance;    /* in pixels */
    long    Real_Delta_Distance;    /* in pixels */
    int     i;
    int     Num_Constant_Table,Num_Accel_Table;


    Max_Speed = gMouseKeys.iMaxSpeed;
    Max_Speed *= 1000 / MOUSETICKS;

    Accel_Per_Tick = Max_Speed * 1000 / (gMouseKeys.iTimeToMaxSpeed * MOUSETICKS);
    Current_Speed = 0;
    Total_Distance = 0;
    Real_Total_Distance = 0;
    Num_Constant_Table = 0;
    Num_Accel_Table = 0;

    for(i=0; i<= 255; i++) {
        Current_Speed = Current_Speed + Accel_Per_Tick;
        if (Current_Speed > Max_Speed) {
            Current_Speed = Max_Speed;
        }
        Total_Distance += Current_Speed;

        //
        // Calculate how many pixels to move on this tick
        //
        Real_Delta_Distance = ((Total_Distance - (Real_Total_Distance * 1000)) + 500) / 1000 ;
        //
        // Calculate total distance moved up to this point
        //
        Real_Total_Distance = Real_Total_Distance + Real_Delta_Distance;

        if ((Current_Speed < Max_Speed) && (Num_Accel_Table < 128)) {
            gMouseCursor.bAccelTable[Num_Accel_Table++] = (BYTE)Real_Delta_Distance;
        }

        if ((Current_Speed == Max_Speed) && (Num_Constant_Table < 128)) {
            gMouseCursor.bConstantTable[Num_Constant_Table++] = (BYTE)Real_Delta_Distance;
        }

    }
    gMouseCursor.bAccelTableLen = (BYTE)Num_Accel_Table;
    gMouseCursor.bConstantTableLen = (BYTE)Num_Constant_Table;
}


/***************************************************************************\
* xxxToggleKeysTimer
*
* Enable ToggleKeys if it is currently disabled.  Disable ToggleKeys if it
* is currently enabled.
*
* This routine is called only when the NumLock key is held down for 5 seconds.
*
* Return value:
*    0
*
* History:
*   11 Feb 93 GregoryW   Created.
\***************************************************************************/
VOID xxxToggleKeysTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{
    KE ToggleKeyEvent;
    PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;

    CheckCritIn();
    //
    // Toggle ToggleKeys and provide audible feedback if appropriate.
    //
    if (TEST_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON)) {
        CLEAR_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON);
        if (TEST_ACCESSFLAG(ToggleKeys, TKF_HOTKEYSOUND)) {
            PostRitSound(
                    pTerm,
                    RITSOUND_DOWNSIREN);
        }
    } else {
        if (TEST_ACCESSFLAG(ToggleKeys, TKF_HOTKEYSOUND)) {
            PostRitSound(
                    pTerm,
                    RITSOUND_UPSIREN);
        }

        PostAccessNotification(ACCESS_TOGGLEKEYS);
    }
    //
    // Send a fake break/make combination so state of numlock key remains
    // the same as it was before user pressed it to activate/deactivate
    // ToggleKeys.
    //
    ToggleKeyEvent.bScanCode = gTKScanCode;
#ifdef FE_SB // ToggleKeysTimer()
    ToggleKeyEvent.usFlaggedVk = gNumLockVk | KBDBREAK;
#else
    ToggleKeyEvent.usFlaggedVk = VK_NUMLOCK | KBDBREAK;
#endif // FE_SB
    if (AccessProceduresStream(&ToggleKeyEvent, gTKExtraInformation, gTKNextProcIndex)) {
        xxxProcessKeyEvent(&ToggleKeyEvent, gTKExtraInformation, FALSE);
    }
#ifdef FE_SB // ToggleKeysTimer()
    ToggleKeyEvent.usFlaggedVk = gNumLockVk;
#else
    ToggleKeyEvent.usFlaggedVk = VK_NUMLOCK;
#endif // FE_SB
    if (AccessProceduresStream(&ToggleKeyEvent, gTKExtraInformation, gTKNextProcIndex)) {
        xxxProcessKeyEvent(&ToggleKeyEvent, gTKExtraInformation, FALSE);
    }
    return;

    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(lParam);
}


/***************************************************************************\
* ToggleKeys
*
* This is the strategy routine that gets called as part of the input stream
* processing.  Keys of interest are Num Lock, Scroll Lock and Caps Lock.
*
* Return value:
*    TRUE - key event should be passed on to the next access routine.
*    FALSE - key event was processed and should not be passed on.
*
* History:
\***************************************************************************/
BOOL ToggleKeys(PKE pKeyEvent, ULONG ExtraInformation, int NextProcIndex)
{
    int fBreak;
    BYTE Vk;

    CheckCritIn();
    Vk = (BYTE)pKeyEvent->usFlaggedVk;
    fBreak = pKeyEvent->usFlaggedVk & KBDBREAK;

    //
    // Check for Numlock key.  On the first make set the ToggleKeys timer.
    // The timer is killed on the break of the Numlock key.
    //
    switch (Vk) {
    case VK_NUMLOCK:
#ifdef FE_SB // ToggleKeys()
NumLockProc:
#endif // FE_SB
        /*
         * Don't handle NUMLOCK toggles if the user is doing MouseKey
         * toggling.
         */
        if ((gLockBits | gLatchBits | gPhysModifierState) == MOUSEKEYMODBITS &&
                TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYACTIVE)) {
            break;
        }
        if (fBreak)
        {
            //
            // Only reset gptmrToggleKeys on the break of NumLock. This
            // prevents cycling the toggle keys state by continually
            // holding down the NumLock key.
            //
            KILLRITTIMER(NULL, gtmridToggleKeys);
            gtmridToggleKeys = 0;
            gTKExtraInformation = 0;
            gTKScanCode = 0;
        }
        else
        {
            if (gtmridToggleKeys == 0 &&
                TEST_ACCESSFLAG(ToggleKeys, TKF_HOTKEYACTIVE))
            {

                //
                // Remember key information to be used by timer routine.
                //
                gTKExtraInformation = ExtraInformation;
                gTKScanCode = pKeyEvent->bScanCode;
                gTKNextProcIndex = NextProcIndex;
                gtmridToggleKeys = InternalSetTimer(
                                              NULL,
                                              0,
                                              TOGGLEKEYTOGGLETIME,
                                              xxxToggleKeysTimer,
                                              TMRF_RIT | TMRF_ONESHOT
                                              );
            }
        }
        //
        // If MouseKeys is on, audible feedback has already occurred for this
        // keystroke.  Skip the rest of the processing.
        //
        if (TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)) {
            break;
        }
        // fall through

    case VK_SCROLL:
    case VK_CAPITAL:
#ifdef FE_SB // ToggleKeys()
CapitalProc:
#endif // FE_SB
        if (TEST_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON) && !fBreak) {
            if (!TestAsyncKeyStateDown(Vk)) {
                PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
                if (!TestAsyncKeyStateToggle(Vk)) {
                    PostRitSound(
                        pTerm,
                        RITSOUND_HIGHBEEP);
                } else {
                    PostRitSound(
                        pTerm,
                        RITSOUND_LOWBEEP);
                }
            }
        }
        break;

    default:
#ifdef FE_SB // ToggleKeys()
        if (Vk == gNumLockVk) goto NumLockProc;
        if (Vk == gOemScrollVk) goto CapitalProc;
#endif // FE_SB
        if (gtmridToggleKeys != 0) {
            KILLRITTIMER(NULL, gtmridToggleKeys);
        }
    }

    return TRUE;
}


/***************************************************************************\
* AccessTimeOutTimer
*
* This routine is called if no keyboard activity takes place for the
* user configured amount of time.  All access related functions are
* disabled.
*
* This routine is called with the critical section already locked.
*
* Return value:
*    0
*
* History:
\***************************************************************************/
VOID xxxAccessTimeOutTimer(PWND pwnd, UINT message, UINT_PTR nID, LPARAM lParam)
{
    CheckCritIn();
    /*
     * The timeout timer will remain on (if so configured) as long as
     * TEST_ACCF(ACCF_ACCESSENABLED) is TRUE.  This means we might get timeouts when
     * only hot keys are enabled, but no features are actually on.  Don't
     * provide any audible feedback in this case.
     */
    if (    TEST_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON)   ||
            TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON)   ||
            TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)     ||
            TEST_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON)   ||
            TEST_ACCESSFLAG(SoundSentry, SSF_SOUNDSENTRYON) ||
            TEST_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON) ||
            TEST_ACCF(ACCF_SHOWSOUNDSON)) {

        PTERMINAL pTerm = grpdeskRitInput->rpwinstaParent->pTerm;
        CLEAR_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON);
        xxxTurnOffStickyKeys();
        CLEAR_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON);
        CLEAR_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON);
        CLEAR_ACCESSFLAG(SoundSentry, SSF_SOUNDSENTRYON);
        CLEAR_ACCF(ACCF_SHOWSOUNDSON);
        CLEAR_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON);

        if (gspwndLogonNotify != NULL) {

        _PostMessage(gspwndLogonNotify, WM_LOGONNOTIFY,
                     LOGON_ACCESSNOTIFY, ACCESS_HIGHCONTRASTOFF);
        }

        if (TEST_ACCESSFLAG(AccessTimeOut, ATF_ONOFFFEEDBACK)) {
            PostRitSound(
                    pTerm,
                    RITSOUND_DOWNSIREN);
        }
        PostAccessibility( ACCESS_MOUSEKEYS );

        PostAccessibility( ACCESS_FILTERKEYS );

        PostAccessibility( ACCESS_STICKYKEYS );

    }
    SetAccessEnabledFlag();
    return;


    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(nID);
    UNREFERENCED_PARAMETER(lParam);
}

/***************************************************************************\
* AccessTimeOutReset
*
* This routine resets the timeout timer.
*
* Return value:
*    0
*
* History:
\***************************************************************************/
VOID AccessTimeOutReset()
{

    if (gtmridAccessTimeOut != 0) {
        KILLRITTIMER(NULL, gtmridAccessTimeOut);
    }
    if (TEST_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON)) {
        gtmridAccessTimeOut = InternalSetTimer(
                                         NULL,
                                         0,
                                         (UINT)gAccessTimeOut.iTimeOutMSec,
                                         xxxAccessTimeOutTimer,
                                         TMRF_RIT | TMRF_ONESHOT
                                         );
    }
}

/***************************************************************************\
* xxxUpdatePerUserAccessPackSettings
*
* Sets the initial access pack features according to the user's profile.
*
* 02-14-93 GregoryW        Created.
\***************************************************************************/
void
xxxUpdatePerUserAccessPackSettings(PUNICODE_STRING pProfileUserName)
{
    LUID luidCaller;
    NTSTATUS status;
    BOOL fSystem = FALSE;
    BOOL fRegFilterKeysOn;
    BOOL fRegStickyKeysOn;
    BOOL fRegMouseKeysOn;
    BOOL fRegToggleKeysOn;
    BOOL fRegTimeOutOn;
    BOOL fRegKeyboardPref;
    BOOL fRegScreenReader;
    BOOL fRegHighContrastOn;
    DWORD dwDefFlags;
    UNICODE_STRING usHighContrastScheme;
    WCHAR wcHighContrastScheme[MAX_SCHEME_NAME_SIZE];

    status = GetProcessLuid(NULL, &luidCaller);
    //
    // If we're called in the system context no one is logged on.
    // We want to read the current .DEFAULT settings for the access
    // features.  Later when we're called in the user context (e.g.,
    // someone has successfully logged on) we check to see if the
    // current access state is the same as the default setting.  If
    // not, the user has enabled/disabled one or more access features
    // from the keyboard.  These changes will be propagated across
    // the logon into the user's intial state (overriding the settings
    // in the user's profile).
    //
    if (RtlEqualLuid(&luidCaller, &luidSystem)) {
        fSystem = TRUE;
    }



    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_KEYBOARDRESPONSE,
                     TEXT("Flags"),
                     0
                     );
    fRegFilterKeysOn = (dwDefFlags & FKF_FILTERKEYSON) != 0;

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_STICKYKEYS,
                     TEXT("Flags"),
                     0
                     );
    fRegStickyKeysOn = (dwDefFlags & SKF_STICKYKEYSON) != 0;

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_MOUSEKEYS,
                     TEXT("Flags"),
                     0
                     );
    fRegMouseKeysOn = (dwDefFlags & MKF_MOUSEKEYSON) != 0;

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_TOGGLEKEYS,
                     TEXT("Flags"),
                     0
                     );
    fRegToggleKeysOn = (dwDefFlags & TKF_TOGGLEKEYSON) != 0;

    fRegKeyboardPref = !!FastGetProfileIntW(pProfileUserName,
                     PMAP_KEYBOARDPREF,
                     TEXT("On"),
                     0
                     );

    fRegScreenReader = !!FastGetProfileIntW(pProfileUserName,
                     PMAP_SCREENREADER,
                     TEXT("On"),
                     0
                     );

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_TIMEOUT,
                     TEXT("Flags"),
                     0
                     );
    fRegTimeOutOn = (dwDefFlags & ATF_TIMEOUTON) != 0;

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_HIGHCONTRAST,
                     TEXT("Flags"),
                     0
                     );
    fRegHighContrastOn = (dwDefFlags & HCF_HIGHCONTRASTON) != 0;

    if (fSystem) {
        //
        // We're in system mode (e.g., no one is logged in).  Remember
        // the .DEFAULT state for comparison during the next user logon
        // and set the current state to the .DEFAULT state.
        //
        if (fRegFilterKeysOn) {
            SET_ACCF(ACCF_DEFAULTFILTERKEYSON);
            SET_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTFILTERKEYSON);
            CLEAR_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON);
        }

        //
        // If StickyKeys is currently on and we're about to turn it
        // off we need to make sure the latch keys and lock keys are
        // released.
        //
        if (TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON) && (fRegFilterKeysOn == 0)) {
                xxxTurnOffStickyKeys();
        }

        if (fRegStickyKeysOn) {
            SET_ACCF(ACCF_DEFAULTSTICKYKEYSON);
            SET_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTSTICKYKEYSON);
            CLEAR_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON);
        }

        if (fRegMouseKeysOn) {
            SET_ACCF(ACCF_DEFAULTMOUSEKEYSON);
            SET_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTMOUSEKEYSON);
            CLEAR_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON);
        }

        if (fRegToggleKeysOn) {
            SET_ACCF(ACCF_DEFAULTTOGGLEKEYSON);
            SET_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTTOGGLEKEYSON);
            CLEAR_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON);
        }

        if (fRegTimeOutOn) {
            SET_ACCF(ACCF_DEFAULTTIMEOUTON);
            SET_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTTIMEOUTON);
            CLEAR_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON);
        }

        if (fRegKeyboardPref) {
            SET_ACCF(ACCF_DEFAULTKEYBOARDPREF);
            SET_ACCF(ACCF_KEYBOARDPREF);
            gpsi->bKeyboardPref = TRUE;
        } else {
            CLEAR_ACCF(ACCF_DEFAULTKEYBOARDPREF);
            CLEAR_ACCF(ACCF_KEYBOARDPREF);
            gpsi->bKeyboardPref = FALSE;
        }

        if (fRegScreenReader) {
            SET_ACCF(ACCF_DEFAULTSCREENREADER);
            SET_ACCF(ACCF_SCREENREADER);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTSCREENREADER);
            CLEAR_ACCF(ACCF_SCREENREADER);
        }

        if (fRegHighContrastOn) {
            SET_ACCF(ACCF_DEFAULTHIGHCONTRASTON);
            SET_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON);
        } else {
            CLEAR_ACCF(ACCF_DEFAULTHIGHCONTRASTON);
            CLEAR_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON);
        }
    } else {
        //
        // A user has successfully logged on.  If the current state is
        // different from the default state stored earlier then we know
        // the user has modified the state via the keyboard (at the logon
        // dialog).  This state will override whatever on/off state the
        // user has set in their profile.  If the current state is the
        // same as the default state then the on/off setting from the
        // user profile is used.
        //

        if (    TEST_BOOL_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTFILTERKEYSON)) {
            //
            // Current state and default state are the same.  Use the
            // user's profile setting.
            //

            SET_OR_CLEAR_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON, fRegFilterKeysOn);
        }

        if (    TEST_BOOL_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTSTICKYKEYSON)) {
            //
            // If StickyKeys is currently on and we're about to turn it
            // off we need to make sure the latch keys and lock keys are
            // released.
            //
            if (    TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON) &&
                    (fRegStickyKeysOn == 0)) {

                xxxTurnOffStickyKeys();
            }

            SET_OR_CLEAR_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON, fRegStickyKeysOn);
        }

        if (    TEST_BOOL_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTMOUSEKEYSON)) {
            //
            // Current state and default state are the same.  Use the user's
            // profile setting.
            //
            SET_OR_CLEAR_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON, fRegMouseKeysOn);
        }

        if (    TEST_BOOL_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTTOGGLEKEYSON)) {
            //
            // Current state and default state are the same.  Use the user's
            // profile setting.
            //
            SET_OR_CLEAR_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON, fRegToggleKeysOn);
        }

        if (    TEST_BOOL_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTTIMEOUTON)) {
            //
            // Current state and default state are the same.  Use the user's
            // profile setting.
            //
            SET_OR_CLEAR_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON, fRegTimeOutOn);
        }

        if (    TEST_BOOL_ACCF(ACCF_KEYBOARDPREF) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTKEYBOARDPREF)) {
            //
            // Current state and default state are the same.  Use the user's
            // profile setting.
            //
            SET_OR_CLEAR_ACCF(ACCF_KEYBOARDPREF, fRegKeyboardPref);
        }

        if (    TEST_BOOL_ACCF(ACCF_SCREENREADER) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTSCREENREADER)) {
            //
            // Current state and default state are the same.  Use the user's
            // profile setting.
            //
            SET_OR_CLEAR_ACCF(ACCF_SCREENREADER, fRegScreenReader);
        }

        if (    TEST_BOOL_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON) ==
                TEST_BOOL_ACCF(ACCF_DEFAULTHIGHCONTRASTON)) {
            //
            // Current state and default state are the same.  Use the user's
            // profile setting.
            //
            SET_OR_CLEAR_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON, fRegHighContrastOn);
        }
    }

    //
    // Get the default FilterKeys state.
    //
    // -------- flag --------------- value --------- default ------
    // #define FKF_FILTERKEYSON    0x00000001           0
    // #define FKF_AVAILABLE       0x00000002           2
    // #define FKF_HOTKEYACTIVE    0x00000004           0
    // #define FKF_CONFIRMHOTKEY   0x00000008           0
    // #define FKF_HOTKEYSOUND     0x00000010          10
    // #define FKF_INDICATOR       0x00000020           0
    // #define FKF_CLICKON         0x00000040          40
    // ----------------------------------------- total = 0x52 = 82
    //

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
                     PMAP_KEYBOARDRESPONSE,
                     TEXT("Flags"),
                     82
                     );

    SET_OR_CLEAR_FLAG(
            dwDefFlags,
            FKF_FILTERKEYSON,
            TEST_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON));

    gFilterKeys.dwFlags = dwDefFlags;
    gFilterKeys.iWaitMSec = FastGetProfileIntW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            TEXT("DelayBeforeAcceptance"),
            1000
            );

    gFilterKeys.iRepeatMSec = FastGetProfileIntW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            TEXT("AutoRepeatRate"),
            500
            );

    gFilterKeys.iDelayMSec = FastGetProfileIntW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            TEXT("AutoRepeatDelay"),
            1000
            );

    gFilterKeys.iBounceMSec = FastGetProfileIntW(pProfileUserName,
            PMAP_KEYBOARDRESPONSE,
            TEXT("BounceTime"),
            0
            );

    //
    // Fill in the SoundSentry state.  This release of the
    // accessibility features only supports iWindowsEffect.
    //
    // -------- flag --------------- value --------- default ------
    // #define SSF_SOUNDSENTRYON   0x00000001           0
    // #define SSF_AVAILABLE       0x00000002           1
    // #define SSF_INDICATOR       0x00000004           0
    // ----------------------------------------- total = 0x2 = 2
    //
    gSoundSentry.dwFlags = FastGetProfileIntW(pProfileUserName,
            PMAP_SOUNDSENTRY,
            TEXT("Flags"),
            2
            );

    gSoundSentry.iFSTextEffect = FastGetProfileIntW(pProfileUserName,
            PMAP_SOUNDSENTRY,
            TEXT("FSTextEffect"),
            0
            );

    gSoundSentry.iWindowsEffect = FastGetProfileIntW(pProfileUserName,
            PMAP_SOUNDSENTRY,
            TEXT("WindowsEffect"),
            0
            );

    /*
     * Set ShowSounds flag.
     */
    SET_OR_CLEAR_ACCF(ACCF_SHOWSOUNDSON, FastGetProfileIntW(pProfileUserName,
            PMAP_SHOWSOUNDS,
            TEXT("On"),
            0
            ));

    //
    // Get the default StickyKeys state.
    //
    // -------- flag --------------- value --------- default ------
    // #define SKF_STICKYKEYSON    0x00000001          0
    // #define SKF_AVAILABLE       0x00000002          2
    // #define SKF_HOTKEYACTIVE    0x00000004          0
    // #define SKF_CONFIRMHOTKEY   0x00000008          0
    // #define SKF_HOTKEYSOUND     0x00000010         10
    // #define SKF_INDICATOR       0x00000020          0
    // #define SKF_AUDIBLEFEEDBACK 0x00000040         40
    // #define SKF_TRISTATE        0x00000080         80
    // #define SKF_TWOKEYSOFF      0x00000100        100
    // ----------------------------------------- total = 0x1d2 = 466
    //
    dwDefFlags = FastGetProfileIntW(pProfileUserName,
            PMAP_STICKYKEYS,
            TEXT("Flags"),
            466
            );

    SET_OR_CLEAR_FLAG(
            dwDefFlags,
            SKF_STICKYKEYSON,
            TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON));

    gStickyKeys.dwFlags = dwDefFlags;

    //
    // Get the default MouseKeys state.
    //
    // -------- flag --------------- value --------- default ------
    // #define MKF_MOUSEKEYSON     0x00000001           0
    // #define MKF_AVAILABLE       0x00000002           2
    // #define MKF_HOTKEYACTIVE    0x00000004           0
    // #define MKF_CONFIRMHOTKEY   0x00000008           0
    // #define MKF_HOTKEYSOUND     0x00000010          10
    // #define MKF_INDICATOR       0x00000020           0
    // #define MKF_MODIFIERS       0x00000040           0
    // #define MKF_REPLACENUMBERS  0x00000080           0
    // ----------------------------------------- total = 0x12 = 18
    //
    dwDefFlags = FastGetProfileIntW(pProfileUserName,
            PMAP_MOUSEKEYS,
            TEXT("Flags"),
            18
            );

    SET_OR_CLEAR_FLAG(
            dwDefFlags,
            MKF_MOUSEKEYSON,
            TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON));

    gMouseKeys.dwFlags = dwDefFlags;
    gMouseKeys.iMaxSpeed = FastGetProfileIntW(pProfileUserName,
            PMAP_MOUSEKEYS,
            TEXT("MaximumSpeed"),
            40
            );

    gMouseKeys.iTimeToMaxSpeed = FastGetProfileIntW(pProfileUserName,
                                     PMAP_MOUSEKEYS,
                                     TEXT("TimeToMaximumSpeed"),
                                     3000
                                     );
    CalculateMouseTable();

    gbMKMouseMode =
#ifdef FE_SB
            (TestAsyncKeyStateToggle(gNumLockVk) != 0) ^
#else  // FE_SB
            (TestAsyncKeyStateToggle(VK_NUMLOCK) != 0) ^
#endif // FE_SB
            (TEST_ACCESSFLAG(MouseKeys, MKF_REPLACENUMBERS) != 0);

    //
    // If the system does not have a hardware mouse:
    //    If MouseKeys is enabled show the mouse cursor,
    //    o.w. hide the mouse cursor.
    //
    if (TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)) {
        MKShowMouseCursor();
    } else {
        MKHideMouseCursor();
    }

    //
    // Get the default ToggleKeys state.
    //
    // -------- flag --------------- value --------- default ------
    // #define TKF_TOGGLEKEYSON    0x00000001           0
    // #define TKF_AVAILABLE       0x00000002           2
    // #define TKF_HOTKEYACTIVE    0x00000004           0
    // #define TKF_CONFIRMHOTKEY   0x00000008           0
    // #define TKF_HOTKEYSOUND     0x00000010          10
    // #define TKF_INDICATOR       0x00000020           0
    // ----------------------------------------- total = 0x12 = 18
    //
    dwDefFlags = FastGetProfileIntW(pProfileUserName,
            PMAP_TOGGLEKEYS,
            TEXT("Flags"),
            18
            );

    SET_OR_CLEAR_FLAG(
            dwDefFlags,
            TKF_TOGGLEKEYSON,
            TEST_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON));

    gToggleKeys.dwFlags = dwDefFlags;

    //
    // Get the default Timeout state.
    //
    // -------- flag --------------- value --------- default ------
    // #define ATF_TIMEOUTON       0x00000001           0
    // #define ATF_ONOFFFEEDBACK   0x00000002           2
    // ----------------------------------------- total = 0x2 = 2
    //
    dwDefFlags = FastGetProfileIntW(pProfileUserName,
            PMAP_TIMEOUT,
            TEXT("Flags"),
            2
            );

    SET_OR_CLEAR_FLAG(
            dwDefFlags,
            ATF_TIMEOUTON,
            TEST_ACCESSFLAG(AccessTimeOut, ATF_TIMEOUTON));

    gAccessTimeOut.dwFlags = dwDefFlags;

#ifdef FE_SB //
    if (gpKbdNlsTbl) {
        //
        // Is there any alternative MouseVKey table in KBDNLSTABLE ?
        //
        if ((gpKbdNlsTbl->NumOfMouseVKey == cMouseVKeys) &&
            (gpKbdNlsTbl->pusMouseVKey   != NULL)) {
            //
            // Overwite the pointer.
            //
            gpusMouseVKey = gpKbdNlsTbl->pusMouseVKey;
        }

        //
        // Is there any remapping flag for VK_NUMLOCK/VK_SCROLL ?
        //
        if (gpKbdNlsTbl->LayoutInformation & NLSKBD_INFO_ACCESSIBILITY_KEYMAP) {
            //
            // Overwrite default.
            //
            gNumLockVk = VK_HOME;
            gOemScrollVk = VK_KANA;
        }
    }
#endif // FE_SB

    gAccessTimeOut.iTimeOutMSec = (DWORD)FastGetProfileIntW(pProfileUserName,
             PMAP_TIMEOUT,
             TEXT("TimeToWait"),
             300000
             );  // default is 5 minutes

   /*
    * Get High Contrast state
    */

    dwDefFlags = FastGetProfileIntW(pProfileUserName,
             PMAP_HIGHCONTRAST,
             TEXT("Flags"),
             HCF_AVAILABLE | HCF_HOTKEYSOUND | HCF_HOTKEYAVAILABLE
             );

    SET_OR_CLEAR_FLAG(
            dwDefFlags,
            HCF_HIGHCONTRASTON,
            TEST_ACCESSFLAG(HighContrast, HCF_HIGHCONTRASTON));

    gHighContrast.dwFlags = dwDefFlags;

    /*
     * Get scheme -- set up buffer
     */

    usHighContrastScheme.Buffer = wcHighContrastScheme;
    if (FastGetProfileStringW(pProfileUserName,
            PMAP_HIGHCONTRAST,
            TEXT("High Contrast Scheme"),
            NULL,
            wcHighContrastScheme,
            MAX_SCHEME_NAME_SIZE
            )) {

        /*
         * copy data
         */

        wcscpy(gHighContrastDefaultScheme, usHighContrastScheme.Buffer);
    }


    AccessTimeOutReset();
    SetAccessEnabledFlag();
}


/***************************************************************************\
* SetAccessEnabledFlag
*
* Sets the global flag ACCF_ACCESSENABLED to non-zero if any accessibility
* function is on or hot key activation is enabled.  When TEST_ACCF(ACCF_ACCESSENABLED)
* is zero keyboard input is processed directly.  When TEST_ACCF(ACCF_ACCESSENABLED) is
* non-zero keyboard input is filtered through AccessProceduresStream().
* See KeyboardApcProcedure in ntinput.c.
*
* History:
* 01-19-94 GregoryW         Created.
\***************************************************************************/
VOID SetAccessEnabledFlag(VOID)
{

    SET_OR_CLEAR_ACCF(ACCF_ACCESSENABLED,
                      TEST_ACCESSFLAG(FilterKeys, FKF_FILTERKEYSON)  ||
                      TEST_ACCESSFLAG(FilterKeys, FKF_HOTKEYACTIVE)  ||
                      TEST_ACCESSFLAG(StickyKeys, SKF_STICKYKEYSON)  ||
                      TEST_ACCESSFLAG(StickyKeys, SKF_HOTKEYACTIVE)  ||
                      TEST_ACCESSFLAG(HighContrast, HCF_HOTKEYACTIVE)  ||
                      TEST_ACCESSFLAG(MouseKeys, MKF_MOUSEKEYSON)    ||
                      TEST_ACCESSFLAG(MouseKeys, MKF_HOTKEYACTIVE)   ||
                      TEST_ACCESSFLAG(ToggleKeys, TKF_TOGGLEKEYSON)  ||
                      TEST_ACCESSFLAG(ToggleKeys, TKF_HOTKEYACTIVE)  ||
                      TEST_ACCESSFLAG(SoundSentry, SSF_SOUNDSENTRYON)||
                      TEST_ACCF(ACCF_SHOWSOUNDSON));
}

VOID SoundSentryTimer(PWND pwnd, UINT message, UINT_PTR idTimer, LPARAM lParam)
{
    TL tlpwndT;
    PWND pwndSoundSentry;

    if (pwndSoundSentry = RevalidateHwnd(ghwndSoundSentry)) {
        ThreadLock(pwndSoundSentry, &tlpwndT);
        xxxFlashWindow(pwndSoundSentry,
                       (TEST_BOOL_ACCF(ACCF_FIRSTTICK) ? FLASHW_ALL : FLASHW_STOP),
                       0);
        ThreadUnlock(&tlpwndT);
    }

    if (TEST_ACCF(ACCF_FIRSTTICK)) {
        gtmridSoundSentry = InternalSetTimer(
                                       NULL,
                                       idTimer,
                                       5,
                                       SoundSentryTimer,
                                       TMRF_RIT | TMRF_ONESHOT
                                       );
        CLEAR_ACCF(ACCF_FIRSTTICK);
    } else {
        ghwndSoundSentry = NULL;
        gtmridSoundSentry = 0;
        SET_ACCF(ACCF_FIRSTTICK);
    }

    return;

    UNREFERENCED_PARAMETER(pwnd);
    UNREFERENCED_PARAMETER(message);
    UNREFERENCED_PARAMETER(lParam);
}

/***************************************************************************\
* _UserSoundSentryWorker
*
* This is the worker routine that provides the visual feedback requested
* by the user.
*
* History:
* 08-02-93 GregoryW         Created.
\***************************************************************************/
BOOL
_UserSoundSentryWorker(VOID)
{
    PWND pwndActive;
    TL tlpwndT;

    CheckCritIn();
    //
    // Check to see if SoundSentry is on.
    //
    if (!TEST_ACCESSFLAG(SoundSentry, SSF_SOUNDSENTRYON)) {
        return TRUE;
    }

    if ((gpqForeground != NULL) && (gpqForeground->spwndActive != NULL)) {
        pwndActive = gpqForeground->spwndActive;
    } else {
        return TRUE;
    }

    switch (gSoundSentry.iWindowsEffect) {

    case SSWF_NONE:
        break;

    case SSWF_TITLE:
        //
        // Flash the active caption bar.
        //
        if (gtmridSoundSentry) {
            break;
        }
        ThreadLock(pwndActive, &tlpwndT);
        xxxFlashWindow(pwndActive, FLASHW_ALL, 0);
        ThreadUnlock(&tlpwndT);

        ghwndSoundSentry = HWq(pwndActive);
        gtmridSoundSentry = InternalSetTimer(
                                       NULL,
                                       0,
                                       100,
                                       SoundSentryTimer,
                                       TMRF_RIT | TMRF_ONESHOT
                                       );
        break;

    case SSWF_WINDOW:
    {
        //
        // Flash the active window.
        //
        HDC hdc;
        RECT rc;

        hdc = _GetWindowDC(pwndActive);
        _GetWindowRect(pwndActive, &rc);
        //
        // _GetWindowRect returns screen coordinates.  First adjust them
        // to window (display) coordinates and then map them to logical
        // coordinates before calling InvertRect.
        //
        OffsetRect(&rc, -rc.left, -rc.top);
        GreDPtoLP(hdc, (LPPOINT)&rc, 2);
        InvertRect(hdc, &rc);
        InvertRect(hdc, &rc);
        _ReleaseDC(hdc);
        break;
    }

    case SSWF_DISPLAY:
    {
        //
        // Flash the entire display.
        //
        HDC hdc;
        RECT rc;

        hdc = _GetDCEx(PWNDDESKTOP(pwndActive), NULL, DCX_WINDOW | DCX_CACHE);
        rc.left = rc.top = 0;
        rc.right = SYSMET(CXVIRTUALSCREEN);
        rc.bottom = SYSMET(CYVIRTUALSCREEN);
        InvertRect(hdc, &rc);
        InvertRect(hdc, &rc);
        _ReleaseDC(hdc);
        break;
    }
    }

    return TRUE;
}

/***************************************************************************\
* UtilityManager
*
* This is the strategy routine that gets called as part of the input stream
* processing.  Utility Manager launching happens here.
*
* Return value:
*    TRUE  - key event should be passed on to the next access routine.
*    FALSE - key event was processed and should not be passed on.
*
* History: 10-28-98 a-anilk created
\***************************************************************************/
BOOL UtilityManager(PKE pKeyEvent, ULONG ExtraInformation, int NotUsed)
{
    int CurrentModState;
    int fBreak;
    BYTE Vk;


    CheckCritIn();

    Vk = (BYTE)(pKeyEvent->usFlaggedVk & 0xff);
    fBreak = pKeyEvent->usFlaggedVk & KBDBREAK;
    CurrentModState = gLockBits | gLatchBits | gPhysModifierState;

    // the hot key to launch the utility manager is WinKey + U
    if ((Vk == VK_U && !fBreak && (CurrentModState & LRWIN)) )
    {
        PostAccessNotification(ACCESS_UTILITYMANAGER);

        return FALSE;
    }
    return TRUE;  // send key event to next accessibility routine.

    UNREFERENCED_PARAMETER(NotUsed);
    UNREFERENCED_PARAMETER(ExtraInformation);
}

