/*
 *  Microsoft Confidential
 *  Copyright (C) Microsoft Corporation 1991
 *  All Rights Reserved.
 *
 *
 *  PIFMSC.C
 *  User interface dialogs for GROUP_KBD, GROUP_MSE, and GROUP_ENV.
 *
 *  History:
 *  Created 04-Jan-1993 1:10pm by Jeff Parsons
 */

#include "shellprv.h"
#pragma hdrstop

#define VMD_DEVICE_ID           0x0000C


BINF abinfTsk[] = {
    {IDC_FGNDSCRNSAVER, BITNUM(TSK_NOSCREENSAVER)  | 0x80},
    {IDC_BGNDSUSPEND,   BITNUM(TSK_BACKGROUND)     | 0x80},
    {IDC_WARNTERMINATE, BITNUM(TSK_NOWARNTERMINATE)| 0x80},
};

BINF abinfKbd[] = {
    {IDC_ALTESC,        BITNUM(KBD_NOALTESC)    | 0x80},
    {IDC_ALTTAB,        BITNUM(KBD_NOALTTAB)    | 0x80},
    {IDC_CTRLESC,       BITNUM(KBD_NOCTRLESC)   | 0x80},
    {IDC_PRTSC,         BITNUM(KBD_NOPRTSC)     | 0x80},
    {IDC_ALTPRTSC,      BITNUM(KBD_NOALTPRTSC)  | 0x80},
    {IDC_ALTSPACE,      BITNUM(KBD_NOALTSPACE)  | 0x80},
    {IDC_ALTENTER,      BITNUM(KBD_NOALTENTER)  | 0x80},
    {IDC_FASTPASTE,     BITNUM(KBD_FASTPASTE)         },
};

BINF abinfMse[] = {
    {IDC_QUICKEDIT,     BITNUM(MSE_WINDOWENABLE)| 0x80},
    {IDC_EXCLMOUSE,     BITNUM(MSE_EXCLUSIVE)         },    // WARNING -- Assumed to be abinfMse[1] below
};

#ifdef ENVINIT_INSTRUCTIONS
BINF abinfEnvInit[] = {
    {IDC_INSTRUCTIONS,  BITNUM(ENVINIT_INSTRUCTIONS)  },
};
#endif

// Private function prototypes

void EnableMscDlg(HWND hDlg, PPROPLINK ppl);
BOOL GetSetMscProps(HWND hDlg, GETSETFN lpfn, PPROPLINK ppl, LPPROPTSK lptsk, LPPROPKBD lpkbd, LPPROPMSE lpmse, LPPROPENV lpenv, int idError);
void InitMscDlg(HWND hDlg, PPROPLINK ppl);
void ApplyMscDlg(HWND hDlg, PPROPLINK ppl);


// Context-sensitive help ids

const static DWORD rgdwHelp[] = {
    IDC_FGNDGRP,         IDH_COMM_GROUPBOX,
    IDC_FGNDSCRNSAVER,   IDH_DOS_TASKING_ALLOW_SCREENSAVER,
    IDC_BGNDGRP,         IDH_COMM_GROUPBOX,
    IDC_BGNDSUSPEND,     IDH_DOS_TASKING_SUSPEND,
    IDC_IDLEGRP,         IDH_COMM_GROUPBOX,
    IDC_IDLELOWLBL,      IDH_DOS_TASKING_IDLE_SLIDER,
    IDC_IDLEHIGHLBL,     IDH_DOS_TASKING_IDLE_SLIDER,
    IDC_IDLESENSE,       IDH_DOS_TASKING_IDLE_SLIDER,
    IDC_TERMGRP,         IDH_COMM_GROUPBOX,
    IDC_WARNTERMINATE,   IDH_DOS_WINDOWS_WARN,
    IDC_MISCMOUSEGRP,    IDH_COMM_GROUPBOX,
    IDC_QUICKEDIT,       IDH_DOS_WINDOWS_MOUSE_QUICKEDIT,
    IDC_EXCLMOUSE,       IDH_DOS_WINDOWS_MOUSE_EXCLUSIVE,
    IDC_ALTESC,          IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_MISCKBDGRP,      IDH_COMM_GROUPBOX,
    IDC_ALTTAB,          IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_CTRLESC,         IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_PRTSC,           IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_ALTPRTSC,        IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_ALTSPACE,        IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_ALTENTER,        IDH_DOS_KEYBOARD_SHORTCUTS,
    IDC_MISCOTHERGRP,    IDH_COMM_GROUPBOX,
    IDC_FASTPASTE,       IDH_DOS_KEYBOARD_FASTPASTE,
#ifdef ENVINIT_INSTRUCTIONS
    IDC_INSTRUCTIONS,    IDH_DOS_WINDOWS_EXITINSTR,
#endif
    IDC_TOOLBAR,         IDH_DOS_WINDOWS_TOOLBAR,
    IDC_WINRESTORE,      IDH_DOS_WINDOWS_RESTORE,
    IDC_REALMODEDISABLE, IDH_DOS_REALMODEPROPS,
    0, 0
};


BOOL_PTR CALLBACK DlgMscProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    PPROPLINK ppl;
    FunctionName(DlgMscProc);

    ppl = (PPROPLINK)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (uMsg) {
    case WM_INITDIALOG:
        lParam = ((LPPROPSHEETPAGE)lParam)->lParam;
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        ppl = (PPROPLINK)lParam;
        InitMscDlg(hDlg, ppl);
        break;

    HELP_CASES(rgdwHelp)                // Handle help messages

    case WM_HSCROLL:                    // assumed to be notifications
                                        // from our one and only trackbar
        SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
        break;

    case WM_COMMAND:
        if (LOWORD(lParam) == 0)
            break;                      // message not from a control

        switch (LOWORD(wParam)) {

        case IDC_FGNDSCRNSAVER:
        case IDC_BGNDSUSPEND:
        case IDC_QUICKEDIT:
        case IDC_EXCLMOUSE:
        case IDC_WARNTERMINATE:
        case IDC_FASTPASTE:
        case IDC_INSTRUCTIONS:
        case IDC_ALTTAB:
        case IDC_CTRLESC:
        case IDC_ALTPRTSC:
        case IDC_ALTESC:
        case IDC_PRTSC:
        case IDC_ALTENTER:
        case IDC_ALTSPACE:
            if (HIWORD(wParam) == BN_CLICKED)
                SendMessage(GetParent(hDlg), PSM_CHANGED, (WPARAM)hDlg, 0L);
            break;
        }
        break;

    case WM_NOTIFY:
        switch (((NMHDR *)lParam)->code) {
        case PSN_SETACTIVE:
            EnableMscDlg(hDlg, ppl);
            break;

        case PSN_KILLACTIVE:
            // This gives the current page a chance to validate itself
            // SetWindowLong(hDlg, DWL_MSGRESULT, 0);
            break;

        case PSN_APPLY:
            // This happens on OK....
            ApplyMscDlg(hDlg, ppl);
            break;

        case PSN_RESET:
            // This happens on Cancel....
            break;
        }
        break;

    case WM_WININICHANGE:
    case WM_SYSCOLORCHANGE:
    case WM_DISPLAYCHANGE:
        RelayMessageToChildren(hDlg, uMsg, wParam, lParam);
        break;

    default:
        return FALSE;                   // return 0 when not processing
    }
    return TRUE;
}


void EnableMscDlg(HWND hDlg, PPROPLINK ppl)
{
#ifndef WINNT
    WORD wVmdVer;

    if (AdjustRealModeControls(ppl, hDlg)) {

        wVmdVer = GetVxDVersion(VMD_DEVICE_ID);
        /*
         * If the mouse VxD is not 4.0 or better, the Exclusive Mouse feature
         * is not supported.
         *
         * If the mouse VxD is not 3.1 or better, the QuickEdit Mode feature
         * cannot be disabled.
         *
         * So the logic is:
         *
         *  If <= 3.0
         *      Disable both controls, force Exclusive false, QuickEdit true.
         *  Else if < 4.0
         *      Disable Exclusive only, force false.
         *  Else
         *      Remove "unsupported property" warning.
         */
        if (wVmdVer <= 0x0300) {
            CheckDlgButton(hDlg, IDC_QUICKEDIT, TRUE);
            CheckDlgButton(hDlg, IDC_EXCLMOUSE, FALSE);
            DisableDlgItems(hDlg, &abinfMse[0], ARRAYSIZE(abinfMse));
        } else if (wVmdVer < 0x400) {
            CheckDlgButton(hDlg, IDC_EXCLMOUSE, FALSE);
            DisableDlgItems(hDlg, &abinfMse[1], ARRAYSIZE(abinfMse)-1);
        }
    }
#else
#endif
}


BOOL GetSetMscProps(HWND hDlg, GETSETFN lpfn, PPROPLINK ppl, LPPROPTSK lptsk, LPPROPKBD lpkbd, LPPROPMSE lpmse, LPPROPENV lpenv, int idError)
{
    if (!(*lpfn)(ppl, MAKELP(0,GROUP_TSK),
                        lptsk, sizeof(*lptsk), GETPROPS_NONE) ||
        !(*lpfn)(ppl, MAKELP(0,GROUP_KBD),
                        lpkbd, sizeof(*lpkbd), GETPROPS_NONE) ||
        !(*lpfn)(ppl, MAKELP(0,GROUP_MSE),
                        lpmse, sizeof(*lpmse), GETPROPS_NONE) ||
        !(*lpfn)(ppl, MAKELP(0,GROUP_ENV),
                        lpenv, sizeof(*lpenv), GETPROPS_NONE)) {
        Warning(hDlg, (WORD)idError, (WORD)(MB_ICONEXCLAMATION | MB_OK));
        return FALSE;
    }
    return TRUE;
}


void InitMscDlg(HWND hDlg, PPROPLINK ppl)
{
    PROPTSK tsk;
    PROPKBD kbd;
    PROPMSE mse;
    PROPENV env;
    FunctionName(InitMscDlg);

    if (!GetSetMscProps(hDlg, PifMgr_GetProperties, ppl, &tsk, &kbd, &mse, &env, IDS_QUERY_ERROR))
        return;

    SetDlgItemPct(hDlg, IDC_IDLESENSE, tsk.wIdleSensitivity);
    SetDlgBits(hDlg, &abinfTsk[0], ARRAYSIZE(abinfTsk), tsk.flTsk);
    SetDlgBits(hDlg, &abinfKbd[0], ARRAYSIZE(abinfKbd), kbd.flKbd);
    SetDlgBits(hDlg, &abinfMse[0], ARRAYSIZE(abinfMse), mse.flMse);
#ifdef ENVINIT_INSTRUCTIONS
    SetDlgBits(hDlg, &abinfEnvInit[0], ARRAYSIZE(abinfEnvInit), env.flEnvInit);
#endif
}


void ApplyMscDlg(HWND hDlg, PPROPLINK ppl)
{
    PROPTSK tsk;
    PROPKBD kbd;
    PROPMSE mse;
    PROPENV env;
    FunctionName(ApplyMscDlg);

    if (!GetSetMscProps(hDlg, PifMgr_GetProperties, ppl, &tsk, &kbd, &mse, &env, IDS_UPDATE_ERROR))
        return;

    GetDlgBits(hDlg, &abinfTsk[0], ARRAYSIZE(abinfTsk), &tsk.flTsk);
    tsk.wIdleSensitivity = (WORD) GetDlgItemPct(hDlg, IDC_IDLESENSE);
    GetDlgBits(hDlg, &abinfKbd[0], ARRAYSIZE(abinfKbd), &kbd.flKbd);
    GetDlgBits(hDlg, &abinfMse[0], ARRAYSIZE(abinfMse), &mse.flMse);
#ifdef ENVINIT_INSTRUCTIONS
    GetDlgBits(hDlg, &abinfEnvInit[0], ARRAYSIZE(abinfEnvInit), &env.flEnvInit);
#endif

    if (GetSetMscProps(hDlg, PifMgr_SetProperties, ppl, &tsk, &kbd, &mse, &env, IDS_UPDATE_ERROR)) {
        if (ppl->hwndNotify) {
            ppl->flProp |= PROP_NOTIFY;
            PostMessage(ppl->hwndNotify, ppl->uMsgNotify, sizeof(mse), (LPARAM)MAKELP(0,GROUP_MSE));
        }
    }
}


BOOL IsBufferDifferent( LPVOID lpBuff1, LPVOID lpBuff2, UINT cb )
{
    BYTE bRet = 0;
    LPBYTE lpByte1 = (LPBYTE)lpBuff1;
    LPBYTE lpByte2 = (LPBYTE)lpBuff2;

    ASSERT(cb>0);

    while ((cb!=0) && (bRet==0))
    {
        bRet = *lpByte1 - *lpByte2;

        cb--;
        lpByte1++;
        lpByte2++;

    }

    return (DWORD)bRet;
}
