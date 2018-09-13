//+---------------------------------------------------------------------------
//
//  Microsoft Windows
// Copyright (c) 1993 - 1999 Microsoft Corporation. All rights reserved.*///
//  File:       debugui.cxx
//
//  Contents:   User interface for trace tags dialog
//
//----------------------------------------------------------------------------

#include <headers.hxx>
#pragma hdrstop

#ifndef X_STDIO_H_
#define X_STDIO_H_
#include <stdio.h>
#endif

#ifndef X_RESOURCE_H_
#define X_RESOURCE_H_
#include "resource.h"
#endif

// private typedefs
typedef int TMC;

// private function prototypes
void    DoTracePointsDialog(BOOL fWait);
VOID    EndButton(HWND hwndDlg, TMC tmc, BOOL fDirty);
WORD    TagFromSelection(HWND hwndDlg, TMC tmc);
INT_PTR CALLBACK DlgTraceEtc(HWND hwndDlg, UINT wm, WPARAM wparam, LPARAM lparam);

//  Debug UI Globals

//
//  Identifies the type of TAG with which the current modal dialog is
//  dealing.
//

//+-------------------------------------------------------------------------
//
//  Function:   TraceTagDlgThread
//
//  Synopsis:   Thread entry point for trace tag dialog.  Keeps caller
//              of DoTracePointsDialog from blocking.
//
//--------------------------------------------------------------------------

DWORD WIN16API
TraceTagDlgThread(void * pv)
{
    DWORD     dwRet;

    EnsureThreadState();

    dwRet = (DWORD)DialogBoxA(g_hinstMain, "TRCAST", NULL, (DLGPROC)DlgTraceEtc);
    if (dwRet == -1)
    {
        MessageBoxA(NULL, "Couldn't create trace tag dialog", "Error",
                   MB_OK | MB_ICONSTOP);
    }

    return dwRet;
}


//+---------------------------------------------------------------------------
//
//  Function:   DoTracePointsDialog
//
//  Synopsis:   Brings up and processes trace points dialog.  Any changes
//              made by the user are copied to the current debug state.
//
//  Arguments:  [fWait] -- If TRUE, this function will not return until the
//                         dialog has been closed.
//
//----------------------------------------------------------------------------

void
DoTracePointsDialog( BOOL fWait )
{
    THREAD_HANDLE          hThread = NULL;
#ifndef _MAC
    DWORD           idThread;
#endif

#ifdef WIN16
    fWait = TRUE;
#endif

    EnsureThreadState();

    if (fWait)
    {
        TraceTagDlgThread(NULL);
    }
    else
    {
#ifndef _MAC
        hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) TraceTagDlgThread, NULL, 0, &idThread);
#else
#pragma message("   DEBUGUI.cxx CreateThread")
Assert (0 && "  DEBUGUI.cxx CreateThread");
#endif
        if (hThread == NULL)
        {
            MessageBoxA(NULL,
                       "Couldn't create trace tag dialog thread",
                       "Error",
                       MB_OK | MB_ICONSTOP);
        }
#ifndef _MAC
        else
        {
            CloseThread(hThread);
        }
#endif
    }
}


/*
 *    FFillDebugListbox
 *
 *    Purpose:
 *        Initializes Windows debug listboxes by adding the correct strings
 *        to the listbox for the current dialog type.  This is only called
 *        once in the Windows interface when the dialog is initialized.
 *
 *    Parameters:
 *        hwndDlg    Handle to parent dialog box.
 *
 *    Returns:
 *        TRUE    if function is successful, FALSE otherwise.
 */
BOOL CALLBACK
FFillDebugListbox(HWND hwndDlg)
{
    TAG      tag;
    LRESULT  lresult;
    TGRC *   ptgrc;
    HWND     hwndListbox;
    CHAR     rgch[80];
    //HFONT    hFont;

    // Get the listbox handle
    hwndListbox = GetDlgItem(hwndDlg, tmcListbox);
    Assert((INT_PTR)hwndListbox);

    // Make sure it's clean
    SendMessageA(hwndListbox, CB_RESETCONTENT, 0, 0);

    //hFont = (HFONT) GetStockObject(SYSTEM_FIXED_FONT);
    //SendMessage(hwndListbox, WM_SETFONT, (WPARAM) hFont, FALSE);
    //DeleteObject(hFont);

    // Enter strings into the listbox-check all tags.
    for (tag = tagMin; tag < tagMac; tag++)
    {
        // If tag is of correct type, enter the string for it.
        if (mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID))
        {
            ptgrc = mptagtgrc + tag;

            #if 0   // old format
            _snprintf(rgch, sizeof(rgch), "%d : %s  %s",
                tag, ptgrc->szOwner, ptgrc->szDescrip);
            #endif

            _snprintf(rgch, sizeof(rgch), "%-17.17s  %s",
                ptgrc->szOwner, ptgrc->szDescrip);

            lresult = SendMessageA(hwndListbox, CB_ADDSTRING,
                                    0, (DWORD_PTR)(VOID *)rgch);

            if (lresult == CB_ERR || lresult == CB_ERRSPACE)
                return FALSE;

            lresult = SendMessageA(
                    hwndListbox, CB_SETITEMDATA, lresult, tag);

            if (lresult == CB_ERR || lresult == CB_ERRSPACE)
                return FALSE;

        }
    }

    return TRUE;
}


/*
 *    FDlgTraceEtc
 *
 *    Purpose:
 *        Dialog procedure for Trace Points and Asserts dialogs.
 *        Keeps the state of the checkboxes identical to
 *        the state of the currently selected TAG in the listbox.
 *
 *    Parameters:
 *        hwndDlg    Handle to dialog window
 *        wm        SDM dialog message
 *        wparam
 *        lparam    Long parameter
 *
 *    Returns:
 *        TRUE if the function processed this message, FALSE if not.
 */
INT_PTR CALLBACK
DlgTraceEtc(HWND hwndDlg, UINT wm, WPARAM wparam, LPARAM lparam)
{
    DBGTHREADSTATE *   pts = DbgGetThreadState();
    TAG             tag;
    TGRC *          ptgrc;
    DWORD           wNew;
    BOOL            fEnable;        // enable all
    TGRC_FLAG       tf;
    BOOL            fTrace;
    HWND            hwndListBox;

    switch (wm)
    {
    default:
        return FALSE;
        break;

    case WM_INITDIALOG:
        pts->fDirtyDlg = FALSE;

        if (!FFillDebugListbox(hwndDlg))
        {
            MessageBoxA(hwndDlg,
                "Error initializing listbox. Cannot display dialog.",
                "Trace/Assert Dialog", MB_OK);
            EndButton(hwndDlg, 0, FALSE);
            break;
        }

        hwndListBox = GetDlgItem(hwndDlg, tmcListbox);
        Assert((INT_PTR)hwndListBox);
        SendMessageA(hwndListBox, CB_SETCURSEL, 0, 0);
        SendMessageA(
                hwndDlg,
                WM_COMMAND,
                GET_WM_COMMAND_MPS(tmcListbox, hwndListBox, CBN_SELCHANGE));


        SetForegroundWindow(hwndDlg);
        break;

    case WM_COMMAND:
        switch (GET_WM_COMMAND_ID(wparam, lparam))
        {
        case tmcOk:
        case tmcCancel:
            EndButton(hwndDlg, GET_WM_COMMAND_ID(wparam, lparam), pts->fDirtyDlg);
            break;

        case tmcEnableAll:
        case tmcDisableAll:
            pts->fDirtyDlg = TRUE;

            fEnable = FALSE;
            if (GET_WM_COMMAND_ID(wparam, lparam) == tmcEnableAll)
                fEnable = TRUE;

            for (tag = tagMin; tag < tagMac; tag++)
            {
                    mptagtgrc[tag].fEnabled = fEnable;
            }

            tag = TagFromSelection(hwndDlg, tmcListbox);

            CheckDlgButton(hwndDlg, tmcEnabled, fEnable);

            break;

        case tmcListbox:
            // Need to check for CBN_EDITCHANGE also, because this seems to cause
            // the selection in listbox to change - especially when it is made empty.
            if (GET_WM_COMMAND_CMD(wparam, lparam) == CBN_EDITCHANGE)
            {
                // Post the message because the selection change seems to be delayed
                PostMessageA(
                    hwndDlg,
                    WM_COMMAND,
                    GET_WM_COMMAND_MPS(tmcListbox,
                            GetDlgItem(hwndDlg, tmcListbox),
                            CBN_SELCHANGE));
                break;
            }

            if (GET_WM_COMMAND_CMD(wparam, lparam) != CBN_SELCHANGE
                && GET_WM_COMMAND_CMD(wparam, lparam) != CBN_DBLCLK)
                break;

            tag = TagFromSelection(hwndDlg, tmcListbox);

            if (tag == tagNull) // possible if the editbox is empty
            {
                // disable all checkboxes
                EnableWindow(GetDlgItem(hwndDlg, tmcEnabled),  FALSE);
                EnableWindow(GetDlgItem(hwndDlg, tmcDisk),  FALSE);
                EnableWindow(GetDlgItem(hwndDlg, tmcCom1),  FALSE);
                EnableWindow(GetDlgItem(hwndDlg, tmcBreak), FALSE);
                break;
            }

            // re-enable it always just in case we disabled it previously
            EnableWindow(GetDlgItem(hwndDlg, tmcEnabled),  TRUE);

            ptgrc = mptagtgrc + tag;

            if (GET_WM_COMMAND_CMD(wparam, lparam) == CBN_DBLCLK)
            {
                ptgrc->fEnabled = !ptgrc->fEnabled;
                pts->fDirtyDlg = TRUE;
            }

            CheckDlgButton(hwndDlg, tmcEnabled, ptgrc->fEnabled);
            CheckDlgButton(hwndDlg, tmcDisk, ptgrc->TestFlag(TGRC_FLAG_DISK));
            CheckDlgButton(hwndDlg, tmcCom1, ptgrc->TestFlag(TGRC_FLAG_COM1));
            CheckDlgButton(hwndDlg, tmcBreak, ptgrc->TestFlag(TGRC_FLAG_BREAK));
            fTrace = (ptgrc->tgty == tgtyTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcDisk),  fTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcCom1),  fTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcBreak), fTrace);
            break;

        case tmcEnabled:
        case tmcDisk:
        case tmcCom1:
        case tmcBreak:
            pts->fDirtyDlg = TRUE;

            tag = TagFromSelection(hwndDlg, tmcListbox);
            ptgrc = mptagtgrc + tag;

            wNew = IsDlgButtonChecked(hwndDlg, GET_WM_COMMAND_ID(wparam, lparam));
#ifdef WIN16
            // we don't have auto checkbox so we need to toggle it
            wNew = !wNew;
            CheckDlgButton(hwndDlg, GET_WM_COMMAND_ID(wparam, lparam), wNew);
#endif // ndef WIN16

            if (GET_WM_COMMAND_ID(wparam, lparam) == tmcEnabled)
            {
                ptgrc->fEnabled = wNew;
            }
            else
            {
                switch (GET_WM_COMMAND_ID(wparam, lparam))
                {
            case tmcDisk:
                    tf = TGRC_FLAG_DISK;
                break;

            case tmcCom1:
                    tf = TGRC_FLAG_COM1;
                    break;

                case tmcBreak:
                    tf = TGRC_FLAG_BREAK;
                    break;

                default:
                    Assert(0 && "Logic error in DlgTraceEtc");
                    tf = (TGRC_FLAG) 0;
                break;
                }

                ptgrc->SetFlagValue(tf, wNew);
            }
        }
        break;
    }

    return TRUE;
}


/*
 *    EndButton
 *
 *    Purpose:
 *        Does necessary processing when either OK or Cancel is pressed in
 *        any of the debug dialogs.  If OK is pressed, the debug state is
 *        saved if dirty.  If Cancel is hit, the debug state is restored if
 *        dirty.
 *
 *    In Windows, the EndDialog function must also be called.
 *
 *    Parameters:
 *        tmc        tmc of the button pressed, either tmcOk or tmcCancel.
 *        fDirty    indicates if the debug state has been modified.
 */
void
EndButton(HWND hwndDlg, TMC tmc, BOOL fDirty)
{
    if (fDirty)
    {
        if (tmc == tmcOk)
            SaveDefaultDebugState();
        else
            RestoreDefaultDebugState();
    }

    EndDialog(hwndDlg, tmc == tmcOk);

    return;
}


/*
 *    TagFromSelection
 *
 *    Purpose:
 *        Isolation function for dialog procedures to eliminate a bunch of
 *         ifdef's everytime the index of the selection in the current listbox
 *        is requried.
 *
 *     Parameters:
 *        tmc        ID value of the listbox.
 *
 *     Returns:
 *        ctag for the currently selected listbox item.
 */

WORD
TagFromSelection(HWND hwndDlg, TMC tmc)
{
    HWND    hwndListbox;
    LRESULT lresult;

    hwndListbox = GetDlgItem(hwndDlg, tmcListbox);
    Assert((INT_PTR)hwndListbox);

    lresult = SendMessageA(hwndListbox, CB_GETCURSEL, 0, 0);

    // the selection can be -ve if the editbox is empty !
    if (lresult == CB_ERR)
        return tagNull;

    Assert(lresult >= 0);
    lresult = SendMessageA(hwndListbox, CB_GETITEMDATA, lresult, 0);
    Assert(tagMin <= lresult && lresult < tagMac);
    return (WORD) lresult;
}


