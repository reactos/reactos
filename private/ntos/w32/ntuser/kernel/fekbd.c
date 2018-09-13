/****************************** Module Header ******************************\
* Module Name: fekbd.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* OEM-specific tables and routines for FarEast keyboards.
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

/*
 * This macro will clear Virtual key code.
 */
#define NLS_CLEAR_VK(Vk)  \
    ((Vk) &= (KBDEXT|KBDMULTIVK|KBDSPECIAL|KBDNUMPAD|KBDBREAK))

/*
 * This macro will clear Virtual key code and 'make'/'break' bit.
 */
#define NLS_CLEAR_VK_AND_ATTR(Vk) \
    ((Vk) &= (KBDEXT|KBDMULTIVK|KBDSPECIAL|KBDNUMPAD))

/*
 * VK_DBE_xxx tables.
 */
BYTE NlsAlphaNumMode[] = {VK_DBE_ALPHANUMERIC,VK_DBE_HIRAGANA,VK_DBE_KATAKANA,0};
BYTE NlsSbcsDbcsMode[] = {VK_DBE_SBCSCHAR,VK_DBE_DBCSCHAR,0};
BYTE NlsRomanMode[] = {VK_DBE_NOROMAN,VK_DBE_ROMAN,0};
BYTE NlsCodeInputMode[] = {VK_DBE_NOCODEINPUT,VK_DBE_CODEINPUT,0};

/*
 * Modifiers for generate NLS Virtual Key.
 */
VK_TO_BIT aVkToBits_NLSKBD[] = {
    { VK_SHIFT,   KBDSHIFT},
    { VK_CONTROL, KBDCTRL},
    { VK_MENU,    KBDALT},
    { 0,          0}
};

MODIFIERS Modifiers_NLSKBD = {
    &aVkToBits_NLSKBD[0],
    7,
    {
        0,  // modifier keys (VK modification number 0)
        1,  // modifier keys (VK modification number 1)
        2,  // modifier keys (VK modification number 2)
        3,  // modifier keys (VK modification number 3)
        4,  // modifier keys (VK modification number 4)
        5,  // modifier keys (VK modification number 5)
        6,  // modifier keys (VK modification number 6)
        7,  // modifier keys (VK modification number 7)
    }
};

/*
 * For PC-9800 Series configuration.
 */
#define GEN_KANA_AWARE 0x1 // Switch generation for VK_END/VK_HELP based on Kana On/Off.
#define GEN_VK_END     0x2 // Generate VK_END, otherwise VK_HELP.
#define GEN_VK_HOME    0x4 // Generate VK_HOME, otherwise VK_CLEAR.

#define IS_KANA_AWARE()   (fNlsKbdConfiguration & GEN_KANA_AWARE)
#define IS_SEND_VK_END()  (fNlsKbdConfiguration & GEN_VK_END)
#define IS_SEND_VK_HOME() (fNlsKbdConfiguration & GEN_VK_HOME)

BYTE fNlsKbdConfiguration = GEN_KANA_AWARE | GEN_VK_END | GEN_VK_HOME;

/***************************************************************************\
* NlsTestKeyStateToggle()
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsTestKeyStateToggle(BYTE Vk)
{
    if (gpqForeground) {
        return (TestKeyStateToggle(gpqForeground,Vk));
    } else {
        return (TestAsyncKeyStateToggle(Vk));
    }
}

/***************************************************************************\
* NlsSetKeyStateToggle(BYTE Vk)
*
* History:
* 27-09-96 hideyukn       Created.
\***************************************************************************/

VOID NlsSetKeyStateToggle(BYTE Vk)
{
    if (gpqForeground)
        SetKeyStateToggle(gpqForeground,Vk);
    SetAsyncKeyStateToggle(Vk);
}

/***************************************************************************\
* NlsClearKeyStateToggle()
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

VOID NlsClearKeyStateToggle(BYTE Vk)
{
    if (gpqForeground)
        ClearKeyStateToggle(gpqForeground,Vk);
    ClearAsyncKeyStateToggle(Vk);
}

/***************************************************************************\
* NlsGetCurrentInputMode()
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BYTE NlsGetCurrentInputMode(BYTE *QueryMode)
{
    BYTE *VkTable = QueryMode;
    BYTE VkDefault;
    /*
     * Get VkDefault, we will return this, if no bit is toggled.
     */
    VkDefault = *VkTable;

    while (*VkTable) {
        if (NlsTestKeyStateToggle(*VkTable)) {
            return *VkTable;
        }
        VkTable++;
    }

    /* Something error */
    return VkDefault;
}

/***************************************************************************\
* NlsNullProc() - nothing to do
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsNullProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(pKe);
    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);

    /*
     * Actually we should not get here...
     */
    return TRUE;
}

/***************************************************************************\
* NlsSendBaseVk() - nothing to do
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsSendBaseVk(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(pKe);
    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);

    /*
     * We don't need to modify Original data.
     */
    return TRUE;
}

/***************************************************************************\
* NlsSendParamVk() - Replace original message with parameter
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsSendParamVk(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(dwExtraInfo);

    /*
     * Clear Virtual code.
     */
    NLS_CLEAR_VK(pKe->usFlaggedVk);
    /*
     * Set parameter as new VK key.
     */
    pKe->usFlaggedVk |= (BYTE)dwParam;
    return TRUE;
}

/***************************************************************************\
* NlsLapseProc() - Lapse handle (Locale dependent)
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsLapseProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(pKe);
    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);

    /*
     * Just throw away this event.
     */
    return FALSE;
}

/***************************************************************************\
* AlphanumericModeProc() - handle special case Alphanumeric key
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsAlphanumericModeProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        /*
         * We are in 'make' sequence.
         */
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        if (!NlsTestKeyStateToggle(VK_DBE_ALPHANUMERIC)) {
            /*
             * Query current mode.
             */
            BYTE CurrentMode = NlsGetCurrentInputMode(NlsAlphaNumMode);
            /*
             * Off toggle for previous key mode.
             */
            NlsClearKeyStateToggle(CurrentMode);
            /*
             * We are not in 'AlphaNumeric' mode, before enter 'AlphaNumeric'
             * mode, we should send 'break' for previous key mode.
             */
            xxxKeyEvent((USHORT)(pKe->usFlaggedVk | CurrentMode | KBDBREAK),
                      pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
        }
        /*
         * Switch to 'AlphaNumeric' mode.
         */
        pKe->usFlaggedVk |= VK_DBE_ALPHANUMERIC;

        /*
         * Call i/o control.
         */
        if ((!gdwIMEOpenStatus) && NlsTestKeyStateToggle(VK_KANA)) {
            NlsKbdSendIMEProc(TRUE, IME_CMODE_KATAKANA);
        }
    } else {
        return NlsLapseProc(pKe,dwExtraInfo,dwParam);
    }
    return TRUE;
}

/***************************************************************************\
* KatakanaModeProc() - handle special case Katakana key
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsKatakanaModeProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        /*
         * We are in 'make' sequence.
         */
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        if (!NlsTestKeyStateToggle(VK_DBE_KATAKANA)) {
            /*
             * Query current mode.
             */
            BYTE CurrentMode = NlsGetCurrentInputMode(NlsAlphaNumMode);
            /*
             * Off toggle for previous key mode.
             */
            NlsClearKeyStateToggle(CurrentMode);
            /*
             * We are not in 'Katakana' mode, yet. Before enter 'Katakana'
             * mode, we should make 'break key' for previous mode.
             */
            xxxKeyEvent((USHORT)(pKe->usFlaggedVk | CurrentMode | KBDBREAK),
                       pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
        }
        /*
         * Switch to 'Katakana' mode.
         */
        pKe->usFlaggedVk |= VK_DBE_KATAKANA;

        /*
         * Call i/o control.
         */
        if ((!gdwIMEOpenStatus) && (!(NlsTestKeyStateToggle(VK_KANA)))) {
            NlsKbdSendIMEProc(FALSE, IME_CMODE_ALPHANUMERIC);
        }
    } else {
        return(NlsLapseProc(pKe,dwExtraInfo,dwParam));
    }
    return TRUE;
}

/***************************************************************************\
* HiraganaModeProc() - handle special case Hiragana key (Locale dependent)
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsHiraganaModeProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        /*
         * We are in 'make' sequence.
         */
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        if (!NlsTestKeyStateToggle(VK_DBE_HIRAGANA)) {
            /*
             * Query current mode.
             */
            BYTE CurrentMode = NlsGetCurrentInputMode(NlsAlphaNumMode);
            /*
             * Off toggle for previous key mode.
             */
            NlsClearKeyStateToggle(CurrentMode);
            /*
             * We are not in 'Hiragana' mode, yet. Before enter 'Hiragana'
             * mode, we should make 'break key' for previous key.
             */
            xxxKeyEvent((USHORT)(pKe->usFlaggedVk | CurrentMode | KBDBREAK),
                      pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
        }
        /*
         * Switch to 'Hiragana' mode.
         */
        pKe->usFlaggedVk |= VK_DBE_HIRAGANA;

        /*
         * Call i/o control.
         */
        if ((!gdwIMEOpenStatus) && (!(NlsTestKeyStateToggle(VK_KANA)))) {
            NlsKbdSendIMEProc(FALSE, IME_CMODE_ALPHANUMERIC);
        }
    } else {
        return(NlsLapseProc(pKe,dwExtraInfo,dwParam));
    }
    return TRUE;
}

/***************************************************************************\
* SbcsDbcsToggleProc() - handle special case SBCS/DBCS key
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsSbcsDbcsToggleProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        /*
         * We are in 'make' sequence.
         */
        /*
         * Query current 'Sbcs'/'Dbcs' mode.
         */
        BYTE CurrentMode = NlsGetCurrentInputMode(NlsSbcsDbcsMode);
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);
        /*
         * Off toggle for previous key mode.
         */
        NlsClearKeyStateToggle(CurrentMode);

        switch (CurrentMode) {
        case VK_DBE_SBCSCHAR:
            /*
             * We are in 'SbcsChar' mode, let us send 'break key' for that.
             */
            xxxKeyEvent((USHORT)(pKe->usFlaggedVk|VK_DBE_SBCSCHAR|KBDBREAK),
                      pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
            /*
             * Then, switch to 'DbcsChar' mode.
             */
            pKe->usFlaggedVk |= VK_DBE_DBCSCHAR;
            break;
        case VK_DBE_DBCSCHAR:
            /*
             * We are in 'DbcsChar' mode, let us send 'break key' for that.
             */
            xxxKeyEvent((USHORT)(pKe->usFlaggedVk|VK_DBE_DBCSCHAR|KBDBREAK),
                      pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
            /*
             * Then, switch to 'SbcsChar' mode.
             */
            pKe->usFlaggedVk |= VK_DBE_SBCSCHAR;
            break;
        }
    } else {
        return(NlsLapseProc(pKe,dwExtraInfo,dwParam));
    }
    return TRUE;
}

/***************************************************************************\
* RomanToggleProc() - handle special case Roman key (Locale dependent)
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsRomanToggleProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        /*
         * We are in 'make' sequence.
         */
        /*
         * Query current 'Roman'/'NoRoman' mode.
         */
        BYTE CurrentMode = NlsGetCurrentInputMode(NlsRomanMode);
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);
        /*
         * Off toggle for previous key mode.
         */
        NlsClearKeyStateToggle(CurrentMode);

        switch (CurrentMode) {
            case VK_DBE_NOROMAN:
                /*
                 * We are in 'NoRoman' mode, let us send 'break key' for that.
                 */
                xxxKeyEvent((USHORT)(pKe->usFlaggedVk|VK_DBE_NOROMAN|KBDBREAK),
                          pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
                /*
                 * Then, switch to 'Roman' mode.
                 */
                pKe->usFlaggedVk |= VK_DBE_ROMAN;
                break;
            case VK_DBE_ROMAN:
                /*
                 * We are in 'Roman' mode, let us send 'break key' for that.
                 */
                xxxKeyEvent((USHORT)(pKe->usFlaggedVk|VK_DBE_ROMAN|KBDBREAK),
                          pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
                /*
                 * Then, switch to 'NoRoman' mode.
                 */
                pKe->usFlaggedVk |= VK_DBE_NOROMAN;
                break;
        }
    } else {
        return(NlsLapseProc(pKe,dwExtraInfo,dwParam));
    }
    return TRUE;
}

/***************************************************************************\
* CodeInputToggleProc() - handle special case Code Input key
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsCodeInputToggleProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        /*
         * We are in 'make' sequence.
         */
        /*
         * Query current 'CodeInput'/'NoCodeInput' mode.
         */
        BYTE CurrentMode = NlsGetCurrentInputMode(NlsCodeInputMode);
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);
        /*
         * Off toggle for previous key mode.
         */
        NlsClearKeyStateToggle(CurrentMode);

        switch (CurrentMode) {
            case VK_DBE_NOCODEINPUT:
                /*
                 * We are in 'NoCodeInput' mode, let us send 'break key' for that.
                 */
                xxxKeyEvent((USHORT)(pKe->usFlaggedVk|VK_DBE_NOCODEINPUT|KBDBREAK),
                          pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
                /*
                 * Then, switch to 'CodeInput' mode.
                 */
                pKe->usFlaggedVk |= VK_DBE_CODEINPUT;
                break;
            case VK_DBE_CODEINPUT:
                /*
                 * We are in 'CodeInput' mode, let us send 'break key' for that.
                 */
                xxxKeyEvent((USHORT)(pKe->usFlaggedVk|VK_DBE_CODEINPUT|KBDBREAK),
                          pKe->bScanCode, pKe->dwTime, dwExtraInfo, FALSE);
                /*
                 * Then, switch to 'NoCodeInput' mode.
                 */
                pKe->usFlaggedVk |= VK_DBE_NOCODEINPUT;
                break;
        }
    } else {
        return(NlsLapseProc(pKe,dwExtraInfo,dwParam));
    }
    return TRUE;
}

/***************************************************************************\
* KanaToggleProc() - handle special case Kana key (Locale dependent)
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL NlsKanaModeToggleProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    /*
     * Check this is 'make' or 'break'.
     */
    BOOL bMake = !(pKe->usFlaggedVk & KBDBREAK);
    /*
     * Check we are in 'kana' mode or not.
     */
    BOOL bKana = NlsTestKeyStateToggle(VK_KANA);
    /*
     * Clear virtual code and key attributes.
     */
    NLS_CLEAR_VK_AND_ATTR(pKe->usFlaggedVk);

    if (bMake) {
        /*
         * We are in 'make' sequence.
         */
        if (bKana) {
            /*
             * Make 'break' for VK_KANA.
             */
            pKe->usFlaggedVk |= (VK_KANA|KBDBREAK);
        } else {
            /*
             * Not yet in 'kana' mode, Let generate 'make' for VK_KANA...
             */
            pKe->usFlaggedVk |= VK_KANA;
        }
        return TRUE;
    } else {
        /*
         * We will keep 'down' & 'toggled' in 'kana' mode,
         * then don't need to generate 'break' for VK_KANA.
         * when next time generate 'make' for this, we will generate
         * 'break' for this.
         */
        return(NlsLapseProc(pKe,dwExtraInfo,dwParam));
    }
}

/**********************************************************************\
* NlsHelpOrEndProc()
*
* History:
* 26-09-96 hideyukn       Ported from NEC code.
\**********************************************************************/

BOOL NlsHelpOrEndProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);

    if (!(pKe->usFlaggedVk & KBDNUMPAD)) {
        /*
         * Clear Virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        if (!IS_KANA_AWARE()) {
            /*
             * We don't care 'kana' status. just check VK_END or VK_HELP.
             */
            if (IS_SEND_VK_END()) {
                pKe->usFlaggedVk |= VK_END;
            } else {
                pKe->usFlaggedVk |= VK_HELP;
            }
        } else {
            /*
             * We care 'kana' status.
             */
            if (IS_SEND_VK_END()) {
                if (NlsTestKeyStateToggle(VK_KANA)) {
                    pKe->usFlaggedVk |= VK_HELP;
                } else {
                    pKe->usFlaggedVk |= VK_END;
                }
            } else {
                if (NlsTestKeyStateToggle(VK_KANA)) {
                    pKe->usFlaggedVk |= VK_END;
                } else {
                    pKe->usFlaggedVk |= VK_HELP;
                }
            }
        }
    }
    return TRUE;
}

/**********************************************************************\
* NlsHelpOrEndProc()
*
* History:
* 26-09-96 hideyukn       Ported from NEC code.
\**********************************************************************/

BOOL NlsHomeOrClearProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);

    if (!(pKe->usFlaggedVk & KBDNUMPAD)) {
        /*
         * Clear virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        if (IS_SEND_VK_HOME()) {
            pKe->usFlaggedVk |= VK_HOME;
        } else {
            pKe->usFlaggedVk |= VK_CLEAR;
        }
    }
    return TRUE;
}

/**********************************************************************\
* NlsNumpadModeProc()
*
* History:
* 26-09-96 hideyukn       Ported from NEC code.
\**********************************************************************/

BOOL NlsNumpadModeProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    /*
     * Get current Virtual key.
     */
    BYTE Vk = LOBYTE(pKe->usFlaggedVk);

    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);

    if(!NlsTestKeyStateToggle(VK_NUMLOCK)) {
        /*
         * Clear virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        switch (Vk) {
        case VK_NUMPAD0:
             pKe->usFlaggedVk |= VK_INSERT;
             break;
        case VK_NUMPAD1:
             pKe->usFlaggedVk |= VK_END;
             break;
        case VK_NUMPAD2:
             pKe->usFlaggedVk |= VK_DOWN;
             break;
        case VK_NUMPAD3:
             pKe->usFlaggedVk |= VK_NEXT;
             break;
        case VK_NUMPAD4:
             pKe->usFlaggedVk |= VK_LEFT;
             break;
        case VK_NUMPAD5:
             pKe->usFlaggedVk |= VK_CLEAR;
             break;
        case VK_NUMPAD6:
             pKe->usFlaggedVk |= VK_RIGHT;
             break;
        case VK_NUMPAD7:
             pKe->usFlaggedVk |= VK_HOME;
             break;
        case VK_NUMPAD8:
             pKe->usFlaggedVk |= VK_UP;
             break;
        case VK_NUMPAD9:
             pKe->usFlaggedVk |= VK_PRIOR;
             break;
        case VK_DECIMAL:
             pKe->usFlaggedVk |= VK_DELETE;
             break;
        }

    } else if (TestRawKeyDown(VK_SHIFT)) {
        /*
         * Clear virtual code.
         */
        NLS_CLEAR_VK(pKe->usFlaggedVk);

        switch (Vk) {
        case VK_NUMPAD0:
             pKe->usFlaggedVk |= VK_INSERT;
             break;
        case VK_NUMPAD1:
             pKe->usFlaggedVk |= VK_END;
             break;
        case VK_NUMPAD2:
             pKe->usFlaggedVk |= VK_DOWN;
             break;
        case VK_NUMPAD3:
             pKe->usFlaggedVk |= VK_NEXT;
             break;
        case VK_NUMPAD4:
             pKe->usFlaggedVk |= VK_LEFT;
             break;
        case VK_NUMPAD5:
             pKe->usFlaggedVk |= VK_CLEAR;
             break;
        case VK_NUMPAD6:
             pKe->usFlaggedVk |= VK_RIGHT;
             break;
        case VK_NUMPAD7:
             pKe->usFlaggedVk |= VK_HOME;
             break;
        case VK_NUMPAD8:
             pKe->usFlaggedVk |= VK_UP;
             break;
        case VK_NUMPAD9:
             pKe->usFlaggedVk |= VK_PRIOR;
             break;
        case VK_DECIMAL:
             pKe->usFlaggedVk |= VK_DELETE;
             break;
        }
    } else {
        /*
         * Otherwise, just pass through...
         */
    }
    return TRUE;
}

/**********************************************************************\
* NlsKanaEventProc() - Fujitsu FMV oyayubi shift keyboard use only.
*
* History:
* 10-10-96 v-kazuta       Created.
\**********************************************************************/
BOOL NlsKanaEventProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(dwExtraInfo);
    /*
     * Clear Virtual code.
     */
    NLS_CLEAR_VK(pKe->usFlaggedVk);

    /*
     * Set parameter as new VK key.
     */
    pKe->usFlaggedVk |= (BYTE)dwParam;

    /*
     * Send notification to kernel mode keyboard driver.
     */
    if (!(pKe->usFlaggedVk & KBDBREAK)) {
        if (NlsTestKeyStateToggle(VK_KANA)) {
            /*
             * Call i/o control.
             */
            NlsKbdSendIMEProc(FALSE, IME_CMODE_ALPHANUMERIC);
        } else {
            /*
             * Call i/o control.
             */
            NlsKbdSendIMEProc(TRUE, IME_CMODE_KATAKANA);
        }
    }
    return TRUE;
}

/**********************************************************************\
* NlsConvOrNonConvProc() - Fujitsu FMV oyayubi shift keyboard only.
*
* History:
* 10-10-96 v-kazuta       Created.
\**********************************************************************/
BOOL NlsConvOrNonConvProc(PKE pKe, ULONG_PTR dwExtraInfo, ULONG dwParam)
{
    UNREFERENCED_PARAMETER(pKe);
    UNREFERENCED_PARAMETER(dwExtraInfo);
    UNREFERENCED_PARAMETER(dwParam);
    /*
     *
     */
    if ((!gdwIMEOpenStatus) && (!(NlsTestKeyStateToggle(VK_KANA)))) {
        NlsKbdSendIMEProc(FALSE, IME_CMODE_ALPHANUMERIC);
    }
    /*
     * We don't need to modify Original data.
     */
    return TRUE;
}

/**********************************************************************\
* Index to function body dispatcher table
*
* History:
* 16-07-96 hideyukn       Created.
\**********************************************************************/

NLSKEPROC aNLSKEProc[] = {
    NlsNullProc,             // KBDNLS_NULL (Invalid function)
    NlsLapseProc,            // KBDNLS_NOEVENT (Drop keyevent)
    NlsSendBaseVk,           // KBDNLS_SEND_BASE_VK (Send Base VK_xxx)
    NlsSendParamVk,          // KBDNLS_SEND_PARAM_VK (Send Parameter VK_xxx)
    NlsKanaModeToggleProc,   // KBDNLS_KANAMODE (VK_KANA (Special case))
    NlsAlphanumericModeProc, // KBDNLS_ALPHANUM (VK_DBE_ALPHANUMERIC)
    NlsHiraganaModeProc,     // KBDNLS_HIRAGANA (VK_DBE_HIRAGANA)
    NlsKatakanaModeProc,     // KBDNLS_KATAKANA (VK_DBE_KATAKANA)
    NlsSbcsDbcsToggleProc,   // KBDNLS_SBCSDBCS (VK_DBE_SBCSCHAR/VK_DBE_DBCSCHAR)
    NlsRomanToggleProc,      // KBDNLS_ROMAN (VK_DBE_ROMAN/VK_DBE_NOROMAN)
    NlsCodeInputToggleProc,  // KBDNLS_CODEINPUT (VK_DBE_CODEINPUT/VK_DBE_NOCODEINPUT)
    NlsHelpOrEndProc,        // KBDNLS_HELP_OR_END (VK_HELP or VK_END)     [NEC PC-9800 Only]
    NlsHomeOrClearProc,      // KBDNLS_HOME_OR_CLEAR (VK_HOME or VK_CLEAR) [NEC PC-9800 Only]
    NlsNumpadModeProc,       // KBDNLS_NUMPAD (VK_xxx for Numpad)          [NEC PC-9800 Only]
    NlsKanaEventProc,        // KBDNLS_KANAEVENT (VK_KANA) [Fujitsu FMV oyayubi Only]
    NlsConvOrNonConvProc,    // KBDNLS_CONV_OR_NONCONV (VK_CONVERT and VK_NONCONVERT) [Fujitsu FMV oyayubi Only]
};

BOOL GenerateNlsVkKey(PVK_F pVkToF, WORD nMod, PKE pKe, ULONG_PTR dwExtraInfo)
{
    BYTE  iFuncIndex;
    DWORD dwParam;

    iFuncIndex = pVkToF->NLSFEProc[nMod].NLSFEProcIndex;
    dwParam = pVkToF->NLSFEProc[nMod].NLSFEProcParam;

    return((aNLSKEProc[iFuncIndex])(pKe, dwExtraInfo, dwParam));
}

BOOL GenerateNlsVkAltKey(PVK_F pVkToF, WORD nMod, PKE pKe, ULONG_PTR dwExtraInfo)
{
    BYTE  iFuncIndex;
    DWORD dwParam;

    iFuncIndex = pVkToF->NLSFEProcAlt[nMod].NLSFEProcIndex;
    dwParam = pVkToF->NLSFEProcAlt[nMod].NLSFEProcParam;

    return((aNLSKEProc[iFuncIndex])(pKe,dwExtraInfo,dwParam));
}

/***************************************************************************\
* KbdNlsFuncTypeDummy() - KBDNLS_FUNC_TYPE_NULL
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL KbdNlsFuncTypeDummy(PVK_F pVkToF, PKE pKe, ULONG_PTR dwExtraInfo)
{
    UNREFERENCED_PARAMETER(pVkToF);
    UNREFERENCED_PARAMETER(pKe);
    UNREFERENCED_PARAMETER(dwExtraInfo);

    /*
     * We don't need to modify Original data.
     */
    return TRUE;
}

/***************************************************************************\
* KbdNlsFuncTypeNormal - KBDNLS_FUNC_TYPE_NORMAL
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL KbdNlsFuncTypeNormal(PVK_F pVkToF, PKE pKe, ULONG_PTR dwExtraInfo)
{
    WORD nMod;

    if (pKe == NULL) {
        /*
         * Clear state and deactivate this key processor
         */
        return FALSE;
    }

    nMod = GetModificationNumber(&Modifiers_NLSKBD,
                                 GetModifierBits(&Modifiers_NLSKBD,
                                                 gafRawKeyState));

    if (nMod != SHFT_INVALID) {
        return(GenerateNlsVkKey(pVkToF, nMod, pKe, dwExtraInfo));
    }
    return FALSE;
}

/***************************************************************************\
* KbdNlsFuncTypeAlt - KBDNLS_FUNC_TYPE_ALT
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

BOOL KbdNlsFuncTypeAlt(PVK_F pVkToF, PKE pKe, ULONG_PTR dwExtraInfo)
{
    WORD nMod;
    BOOL fRet = FALSE;

    if (pKe == NULL) {
        /*
         * Clear state and deactivate this key processor
         */
        return FALSE;
    }

    nMod = GetModificationNumber(&Modifiers_NLSKBD,
                                 GetModifierBits(&Modifiers_NLSKBD,
                                                 gafRawKeyState));

    if (nMod != SHFT_INVALID) {
        if (!(pKe->usFlaggedVk & KBDBREAK)) {
            if (pVkToF->NLSFEProcCurrent == KBDNLS_INDEX_ALT) {
                fRet = GenerateNlsVkAltKey(pVkToF, nMod, pKe, dwExtraInfo);
            } else {
                fRet = GenerateNlsVkKey(pVkToF, nMod, pKe, dwExtraInfo);
            }
            if (pVkToF->NLSFEProcSwitch & (1 << nMod)) {
                TAGMSG0(DBGTAG_IMM, "USERKM:FEKBD Switching Alt table\n");
                /*
                 * Switch to "alt".
                 */
                pVkToF->NLSFEProcCurrent = KBDNLS_INDEX_ALT;
            }
        } else {
            if (pVkToF->NLSFEProcCurrent == KBDNLS_INDEX_ALT) {
                fRet = GenerateNlsVkAltKey(pVkToF, nMod, pKe, dwExtraInfo);
                /*
                 * Back to "normal"
                 */
                pVkToF->NLSFEProcCurrent = KBDNLS_INDEX_NORMAL;
            } else {
                fRet = GenerateNlsVkKey(pVkToF, nMod, pKe, dwExtraInfo);
            }
        }
    }
    return fRet;
}

/***************************************************************************\
* KENLSProcs()
*
* History:
* 16-07-96 hideyukn       Created.
\***************************************************************************/

NLSVKFPROC aNLSVKFProc[] = {
    KbdNlsFuncTypeDummy,  // KBDNLS_INDEX_NULL     0
    KbdNlsFuncTypeNormal, // KBDNLS_INDEX_NORMAL   1
    KbdNlsFuncTypeAlt     // KBDNLS_INDEX_ALT      2
};

/*
 * Returning FALSE means the Key Event has been deleted by a special-case
 * KeyEvent processor.
 * Returning TRUE means the Key Event should be passed on (although it may
 * have been altered.
 */
BOOL xxxKENLSProcs(PKE pKe, ULONG_PTR dwExtraInfo)
{

    CheckCritIn();

    if (gpKbdNlsTbl != NULL) {
        PVK_F pVkToF = gpKbdNlsTbl->pVkToF;
        UINT  iNumVk = gpKbdNlsTbl->NumOfVkToF;

        while(iNumVk) {
            if (pVkToF[iNumVk-1].Vk == LOBYTE(pKe->usFlaggedVk)) {
                return((aNLSVKFProc[pVkToF[iNumVk-1].NLSFEProcType])
                                    (&(pVkToF[iNumVk-1]),pKe,dwExtraInfo));
            }
            iNumVk--;
        }
    }
    /*
     * Other special Key Event processors
     */
    return TRUE;
}

/***************************************************************************\
* NlsKbdSendIMENotification()
*
* History:
* 10-09-96 hideyukn       Created.
\***************************************************************************/

VOID NlsKbdSendIMENotification(DWORD dwImeOpen, DWORD dwImeConversion)
{
    PKBDNLSTABLES       pKbdNlsTable = gpKbdNlsTbl;

    if (pKbdNlsTable == NULL) {
        /*
         * 'Active' layout driver does not have NLSKBD table.
         */
        return;
    }

    /*
     * Let us send notification to kernel mode keyboard driver, if nessesary.
     */
    if ((pKbdNlsTable->LayoutInformation) & NLSKBD_INFO_SEND_IME_NOTIFICATION) {
        PDEVICEINFO pDeviceInfo;

        /*
         * Fill up the KEYBOARD_IME_STATUS structure.
         */
        gKbdImeStatus.UnitId      = 0;
        gKbdImeStatus.ImeOpen     = dwImeOpen;
        gKbdImeStatus.ImeConvMode = dwImeConversion;

        EnterDeviceInfoListCrit();
        BEGINATOMICDEVICEINFOLISTCHECK();
        for (pDeviceInfo = gpDeviceInfoList; pDeviceInfo; pDeviceInfo = pDeviceInfo->pNext) {
            if ((pDeviceInfo->type == DEVICE_TYPE_KEYBOARD) && (pDeviceInfo->handle)) {
                RequestDeviceChange(pDeviceInfo, GDIAF_IME_STATUS, TRUE);
            }
        }
        ENDATOMICDEVICEINFOLISTCHECK();
        LeaveDeviceInfoListCrit();
    }
    return;
}

VOID NlsKbdSendIMEProc(DWORD dwImeOpen, DWORD dwImeConversion)
{
    if (gpqForeground != NULL && gpqForeground->ptiKeyboard != NULL &&
        (!(GetAppImeCompatFlags(gpqForeground->ptiKeyboard) & IMECOMPAT_HYDRACLIENT))) {
        NlsKbdSendIMENotification(dwImeOpen, dwImeConversion);
    }
}

/*
 * Compatibility for Windows NT 3.xx and Windows 3.x for NEC PC-9800 Series
 */
#define NLSKBD_CONFIG_PATH \
        L"WOW\\keyboard"

/***************************************************************************\
* NlsKbdInitializePerSystem()
*
* History:
* 26-09-96 hideyukn       Created.
\***************************************************************************/

VOID NlsKbdInitializePerSystem(VOID)
{
    RTL_QUERY_REGISTRY_TABLE QueryTable[4];

    UNICODE_STRING EndString, HelpString;
    UNICODE_STRING YesString, NoString;
    UNICODE_STRING HomeString, ClearString;

    UNICODE_STRING HelpKeyString;
    UNICODE_STRING KanaHelpString;
    UNICODE_STRING ClearKeyString;

    NTSTATUS status;

    //
    // Set default VK_DBE_xxx status.
    //
    //
    // AlphaNumeric input mode.
    //
    NlsSetKeyStateToggle(VK_DBE_ALPHANUMERIC);
    //
    // Single byte character input mode.
    //
    NlsSetKeyStateToggle(VK_DBE_SBCSCHAR);
    //
    // No roman input mode.
    //
    NlsSetKeyStateToggle(VK_DBE_NOROMAN);
    //
    // No code input mode.
    //
    NlsSetKeyStateToggle(VK_DBE_NOCODEINPUT);

    //
    // From Here, below code is for compatibility for Windows NT 3.xx
    // for NEC PC-9800 verion.
    //

    //
    // Initialize default strings.
    //
    RtlInitUnicodeString(&EndString, L"end");
    RtlInitUnicodeString(&HelpString,L"help");

    RtlInitUnicodeString(&YesString,L"yes");
    RtlInitUnicodeString(&NoString, L"no");

    RtlInitUnicodeString(&HomeString, L"home");
    RtlInitUnicodeString(&ClearString,L"clear");

    //
    // Initialize recieve buffer.
    //
    RtlInitUnicodeString(&HelpKeyString,NULL);
    RtlInitUnicodeString(&KanaHelpString,NULL);
    RtlInitUnicodeString(&ClearKeyString,NULL);

    //
    // Initalize query tables.
    //
    // ValueName : "helpkey"
    // ValueData : if "end" VK_END, otherwise VK_HELP
    //
    QueryTable[0].QueryRoutine = NULL;
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[0].Name = (PWSTR) L"helpkey",
    QueryTable[0].EntryContext = (PVOID) &HelpKeyString;
    QueryTable[0].DefaultType = REG_SZ;
    QueryTable[0].DefaultData = &EndString;
    QueryTable[0].DefaultLength = 0;

    //
    // ValueName : "KanaHelpKey"
    // ValueData : if "yes" if kana on switch VK_HELP and VK_END
    //
    QueryTable[1].QueryRoutine = NULL;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[1].Name = (PWSTR) L"KanaHelpKey",
    QueryTable[1].EntryContext = (PVOID) &KanaHelpString;
    QueryTable[1].DefaultType = REG_SZ;
    QueryTable[1].DefaultData = &YesString;
    QueryTable[1].DefaultLength = 0;

    //
    // ValueName : "clrkey"
    // ValueData : if "home" VK_HOME, otherwise VK_CLEAR
    //
    QueryTable[2].QueryRoutine = NULL;
    QueryTable[2].Flags = RTL_QUERY_REGISTRY_DIRECT;
    QueryTable[2].Name = (PWSTR) L"clrkey",
    QueryTable[2].EntryContext = (PVOID) &ClearKeyString;
    QueryTable[2].DefaultType = REG_SZ;
    QueryTable[2].DefaultData = &HomeString;
    QueryTable[2].DefaultLength = 0;

    QueryTable[3].QueryRoutine = NULL;
    QueryTable[3].Flags = 0;
    QueryTable[3].Name = NULL;

    status = RtlQueryRegistryValues(
                 RTL_REGISTRY_WINDOWS_NT,
                 NLSKBD_CONFIG_PATH,
                 QueryTable, NULL, NULL);

    if (!NT_SUCCESS(status)) {
        RIPMSG1(RIP_WARNING, "FEKBD:RtlQueryRegistryValues fails (%x)\n", status);
        return;
    }

    if (RtlEqualUnicodeString(&HelpKeyString,&HelpString,TRUE)) {
        /*
         * Generate VK_HELP, when NLSKBD_HELP_OR_END is called.
         */
        fNlsKbdConfiguration &= ~GEN_VK_END;
    }

    if (RtlEqualUnicodeString(&KanaHelpString,&NoString,TRUE)) {
        /*
         * In case of "yes":
         * If 'kana' is on, when NLSKBD_HELP_OR_END is called, switch VK_END and VK_HELP.
         * Else, in case of "no":
         * Doesn't generate by 'kana' toggle state.
         */
        fNlsKbdConfiguration &= ~GEN_KANA_AWARE;
    }

    if (RtlEqualUnicodeString(&ClearKeyString,&ClearString,TRUE)) {
        /*
         * Generate VK_CLEAR, when KBDNLS_HOME_OR_CLEAR is called.
         */
        fNlsKbdConfiguration &= ~GEN_VK_HOME;
    }

    return;
}
