/***************************************************************************\
*
*  DMMNEM.C -
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
*      Mnemonic Character Processing Routines
*
* ??-???-???? mikeke    Ported from Win 3.0 sources
* 12-Feb-1991 mikeke    Added Revalidation code
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/***************************************************************************\
* FindMnemChar
*
* Returns: 0x00 if no matching char,
*          0x01 if menmonic char is matching,
*          0x80 if first char is matching
*
* History:
*   11-18-90 JimA       Created.
\***************************************************************************/

int FindMnemChar(
    LPWSTR lpstr,
    WCHAR ch,
    BOOL fFirst,
    BOOL fPrefix)
{
    WCHAR chc;
    WCHAR chnext;
    WCHAR chFirst;

    while (*lpstr == TEXT(' '))
        lpstr++;

    ch = (WCHAR)(ULONG_PTR)CharLowerW((LPWSTR)ULongToPtr( (DWORD)(UTCHAR)ch ));
    chFirst = (WCHAR)(ULONG_PTR)CharLowerW((LPWSTR)ULongToPtr( (DWORD)(UTCHAR)(*lpstr) ));

#ifdef FE_SB // FindMnemChar()
    //
    // HIRO: LATER: CH_ENGLISHPREFIX is obsolete, so entire FE_SB can be removed ?
    //
    if (fPrefix) {
        SHORT wvch, xvkey;
        //
        // get OEM-dependent virtual key code
        //
        if ((wvch = VkKeyScanW(ch)) != -1)
            wvch &= 0x00FF;

        while (chc = *lpstr++) {
            //
            // This should think about KOREA & TAIWAN case. But probably OK.
            //
            if ((chc == CH_PREFIX) || (chc == CH_ENGLISHPREFIX)) {
                chnext = (WCHAR)(ULONG_PTR)CharLowerW((LPWSTR)ULongToPtr( (DWORD)(UTCHAR)*lpstr ));

                if (chnext == CH_PREFIX)
                    lpstr++;
                else if (chnext == ch)
                    return(0x01);
                else {
                    //
                    // Compare should be done with virtual key in Kanji menu mode
                    // in order to accept Digit shortcut key and save English
                    // windows applications!
                    //
                    xvkey = VkKeyScanW(chnext);
                    if (xvkey != 0xFFFF && ((xvkey & 0x00FF) == wvch))
                        return(0x01);
                    else
                        return(0x00);
               }
            }
        }
    }
#else
    if (fPrefix) {
        while (chc = *lpstr++) {
            if (((WCHAR)CharLower((LPWSTR)(DWORD)(UTCHAR)chc) == CH_PREFIX)) {
                chnext = (WCHAR)CharLowerW((LPWSTR)(DWORD)(UTCHAR)*lpstr);

                if (chnext == CH_PREFIX)
                    lpstr++;
                else if (chnext == ch)
                    return 0x01;
                else {
                    return 0x00;
                }
            }
        }
    }
#endif // FE_SB

    if (fFirst && (ch == chFirst))
        return 0x80;

    return 0x00;
}


/***************************************************************************\
* xxxFindNextMnem
*
* This function returns NULL if no control with the specified mnemonic
* can be found.
*
* History:
\***************************************************************************/

PWND xxxGNM_FindNextMnem(
    PWND pwndDlg,
    PWND pwnd,
    WCHAR ch)
{
    PWND pwndStart;
    PWND pwndT;
    WCHAR rgchText[256];
    int i = 0;
    TL tlpwndStart;
    TL tlpwnd;
    DWORD dwDlgCode;

    CheckLock(pwndDlg);
    CheckLock(pwnd);

    /*
     * Check if we are in a group box so we can find local mnemonics.
     */

    pwndStart = _GetChildControl(pwndDlg, pwnd);
    ThreadLock(pwndStart, &tlpwndStart);

    while (TRUE) {

        pwndT = _GetNextDlgGroupItem(pwndDlg, pwndStart, FALSE);

        ThreadUnlock(&tlpwndStart);

        i++;
        if (pwndT == NULL || pwndT == pwnd || i > 60) {

            /*
             * If we have returned to our starting window or if we have gone
             * through 60 iterations, let's exit.  There are no local mnemonics
             * that match.  We have to check for 60 iterations (or so) because
             * we run into problems with WS_GROUP not being properly defined in
             * rc files that we never reach this same starting window again....
             */
            break;
        }

        pwndStart = pwndT;

        /*
         * Only check for matching mnemonic if control doesn't want characters
         * and control isn't a static control with SS_NOPREFIX
         */
        ThreadLock(pwndStart, &tlpwndStart);

        dwDlgCode = (DWORD)SendMessage(HWq(pwndT), WM_GETDLGCODE, 0, 0L);
        if (!(dwDlgCode & DLGC_WANTCHARS) &&
                (!(dwDlgCode & DLGC_STATIC) || !(pwndT->style & SS_NOPREFIX))) {
            GetWindowText(HWq(pwndT), rgchText, sizeof(rgchText)/sizeof(WCHAR));
            if (FindMnemChar(rgchText, ch, FALSE, TRUE) != 0) {
                ThreadUnlock(&tlpwndStart);
                return pwndT;
            }
        }
    }

    pwnd = pwndStart = _GetChildControl(pwndDlg, pwnd);

    ThreadLock(pwnd, &tlpwnd);
    while (TRUE) {

        /*
         * Start with next so we see multiples of same mnemonic.
         */
        pwnd = _NextControl(pwndDlg, pwnd, TRUE);

        ThreadUnlock(&tlpwnd);
        ThreadLock(pwnd, &tlpwnd);



        /*
         * Only check for matching mnemonic if control doesn't want characters
         * and control isn't a static control with SS_NOPREFIX
         */
        dwDlgCode = (DWORD)SendMessage(HW(pwnd), WM_GETDLGCODE, 0, 0L);
        if (!(dwDlgCode & DLGC_WANTCHARS) &&
                (!(dwDlgCode & DLGC_STATIC) || !(pwnd->style & SS_NOPREFIX))) {
            GetWindowText(HW(pwnd), rgchText, sizeof(rgchText)/sizeof(WCHAR));
            if (FindMnemChar(rgchText, ch, FALSE, TRUE) != 0)
                break;
        }

        if (pwnd == pwndStart) {
            pwnd = NULL;
            break;
        }
    }
    ThreadUnlock(&tlpwnd);

    return pwnd;
}

/***************************************************************************\
* xxxGotoNextMnem
*
* History:
\***************************************************************************/

PWND xxxGotoNextMnem(
    PWND pwndDlg,
    PWND pwnd,
    WCHAR ch)
{
    UINT code;
    PWND pwndFirstFound = NULL;
    int count = 0;
    TL tlpwnd;
    PWND pwndT;
    HWND hwnd;

    CheckLock(pwndDlg);
    CheckLock(pwnd);

    ThreadLock(pwnd, &tlpwnd);

    /*
     * Loop for a long time but not long enough so we hang...
     */
    while (count < 256 * 2) {

        /*
         * If the dialog box doesn't has the mnemonic specified, return NULL.
         */
        if ((pwnd = xxxGNM_FindNextMnem(pwndDlg, pwnd, ch)) == NULL) {
            ThreadUnlock(&tlpwnd);
            return NULL;
        }
        hwnd = HWq(pwnd);

        ThreadUnlock(&tlpwnd);
        ThreadLock(pwnd, &tlpwnd);

        code = (UINT)SendMessage(hwnd, WM_GETDLGCODE, 0, 0L);

        /*
         * If a non-disabled static item, then jump ahead to nearest tabstop.
         */
        if (code & DLGC_STATIC && !TestWF(pwnd, WFDISABLED)) {
            pwndT = _GetNextDlgTabItem(pwndDlg, pwnd, FALSE);

            /*
             * If there is no other tab item, keep looking
             */
            if (pwndT == NULL)
                continue;
            pwnd = pwndT;
            hwnd = HWq(pwnd);

            ThreadUnlock(&tlpwnd);
            ThreadLock(pwnd, &tlpwnd);

            /*
             * I suppose we should do a getdlgcode here, but who is going to
             * label a button with a static control?  The setup guys, that's
             * who...  Also, generally useful for ownerdraw buttons which are
             * labeled with a static text item.
             */
            code = (UINT)SendMessage(hwnd, WM_GETDLGCODE, 0, 0L);
        }

        if (!TestWF(pwnd, WFDISABLED)) {

            /*
             * Is it a Pushbutton?
             */
            if (!(code & DLGC_BUTTON)) {

                /*
                 * No, simply give it the focus.
                 */
                DlgSetFocus(hwnd);
            } else {

                /*
                 * Yes, click it, but don't give it the focus.
                 */
                if ((code & DLGC_DEFPUSHBUTTON) || (code & DLGC_UNDEFPUSHBUTTON)) {

                    /*
                     * Flash the button.
                     */
                    SendMessage(hwnd, BM_SETSTATE, TRUE, 0L);

                    /*
                     * Delay
                     */
#ifdef LATER
// JimA - 2/19/92
// There oughta be a better way of doing this...
                    for (i = 0; i < 10000; i++)
                        ;
#else
                    Sleep(1);
#endif

                    /*
                     * Un-Flash it.
                     */
                    SendMessage(hwnd, BM_SETSTATE, FALSE, 0L);

                    /*
                     * Send the WM_COMMAND message.
                     */
                    pwndT = REBASEPWND(pwnd, spwndParent);
                    SendMessage(HW(pwndT), WM_COMMAND,
                            MAKELONG(PTR_TO_ID(pwnd->spmenu), BN_CLICKED), (LPARAM)hwnd);
                    ThreadUnlock(&tlpwnd);
                    return (PWND)1;
                } else {

                    /*
                     * Because BM_CLICK processing will result in BN_CLICK msg,
                     * xxxSetFocus must be prevented from sending the same msg;
                     * Otherwise, it will notify parent twice!
                     * Fix for Bug #3024 -- SANKAR -- 09-22-89 --
                     */
                    BOOL fIsNTButton;
                    PBUTN pbutn;

                    fIsNTButton = (pwnd->fnid == FNID_BUTTON);
                    if (fIsNTButton) {
                      pbutn = ((PBUTNWND)pwnd)->pbutn;
                      BUTTONSTATE(pbutn) |= BST_DONTCLICK;
                    } else {
                     RIPMSG0(RIP_WARNING, "xxxGotoNextMnem: fnid != FNID_BUTTON");
                    }

                    DlgSetFocus(hwnd);

                    if (fIsNTButton) {
                      BUTTONSTATE(pbutn) &= ~BST_DONTCLICK;
                    }

                    /*
                     * Send click message if button has a UNIQUE mnemonic
                     */
                    if (xxxGNM_FindNextMnem(pwndDlg, pwnd, ch) == pwnd) {
                        SendMessage(hwnd, BM_CLICK, TRUE, 0L);
                    }
                }
            }

            ThreadUnlock(&tlpwnd);
            return pwnd;
        } else {

            /*
             * Stop if we've looped back to the first item we checked
             */
            if (pwnd == pwndFirstFound) {
                ThreadUnlock(&tlpwnd);
                return NULL;
            }

            if (pwndFirstFound == NULL)
                pwndFirstFound = pwnd;
        }

        count++;

    }  /* Loop for a long time */

    ThreadUnlock(&tlpwnd);
    return NULL;
}
