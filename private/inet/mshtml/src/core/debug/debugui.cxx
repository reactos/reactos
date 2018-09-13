//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1993.
//
//  File:       debugui.cxx
//
//  Contents:   User interface for trace tags dialog
//
//----------------------------------------------------------------------------

#include <headers.hxx>

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
VOID    EndButton(HWND hwndDlg, TMC tmc, BOOL fDirty);
WORD    TagFromSelection(HWND hwndDlg, TMC tmc);
BOOL    CALLBACK DlgTraceEtc(HWND hwndDlg, UINT wm, WPARAM wparam, LPARAM lparam);

//  Debug UI Globals
//
void    SetRGBValues(void);

extern DWORD       rgbWindowColor;
extern DWORD       rgbHiliteColor;
extern DWORD       rgbWindowText;
extern DWORD       rgbHiliteText;
extern DWORD       rgbGrayText;
extern DWORD       rgbDDWindow;
extern DWORD       rgbDDHilite;

HBITMAP g_hbmpCheck = NULL;
HDC     g_hdcCheck  = NULL;

#define TAG_STRBUF_SIZE  80

//
//  Identifies the type of TAG with which the current modal dialog is
//  dealing.
//

//+-------------------------------------------------------------------------
//
//  Function:   TraceTagDlgThread
//
//  Synopsis:   Thread entry point for trace tag dialog.  Keeps caller
//              of DbgExDoTracePointsDialog from blocking.
//
//--------------------------------------------------------------------------

DWORD WIN16API
TraceTagDlgThread(void * pv)
{
    int     r;

    EnsureThreadState();

    r = DialogBoxA(g_hinstMain, "TRCAST", NULL, (DLGPROC)DlgTraceEtc);
    if (r == -1)
    {
        MessageBoxA(NULL, "Couldn't create trace tag dialog", "Error",
                   MB_OK | MB_ICONSTOP);
    }

    return (DWORD) r;
}


//+---------------------------------------------------------------------------
//
//  Function:   DbgExDoTracePointsDialog
//
//  Synopsis:   Brings up and processes trace points dialog.  Any changes
//              made by the user are copied to the current debug state.
//
//  Arguments:  [fWait] -- If TRUE, this function will not return until the
//                         dialog has been closed.
//
//----------------------------------------------------------------------------

void WINAPI
DbgExDoTracePointsDialog( BOOL fWait )
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
    TRACETAG tag;
    LRESULT  lresult;
    TGRC *   ptgrc;
    HWND     hwndListbox;

    // Get the listbox handle
    hwndListbox = GetDlgItem(hwndDlg, tmcListbox);
    Assert(hwndListbox);

    // Make sure it's clean
    SendMessageA(hwndListbox, CB_RESETCONTENT, 0, 0);

    // Enter tags into the listbox
    for (tag = tagMin; tag < tagMac; tag++)
    {
        // If tag is of correct type, enter the string for it.
        if (mptagtgrc[tag].TestFlag(TGRC_FLAG_VALID))
        {
            ptgrc = mptagtgrc + tag;

            lresult = SendMessageA(hwndListbox, CB_ADDSTRING, 0, tag);

            if (lresult == CB_ERR || lresult == CB_ERRSPACE)
                return FALSE;
        }
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   TextFromTag
//
//  Synopsis:   Returns the string that should be displayed in the dialog
//              for a given tag.
//
//----------------------------------------------------------------------------

void
TextFromTag(TGRC *ptgrc, char *psz, int cch)
{
    _snprintf(psz, cch, "%-17.17s  %s", ptgrc->szOwner, ptgrc->szDescrip);
}

//+---------------------------------------------------------------------------
//
//  Function:   DrawTraceItem
//
//  Synopsis:   Draws a list item in the combobox.
//
//----------------------------------------------------------------------------

VOID
DrawTraceItem(LPDRAWITEMSTRUCT pdis)
{
    TGRC *    ptgrc;
    COLORREF  crText = 0, crBack = 0;
    char      achBuf[TAG_STRBUF_SIZE];
    int       size;
    BOOL      fSelected = (pdis->itemState & ODS_SELECTED) != 0;

    if((int)pdis->itemID < 0)
        return;

    if((ODA_DRAWENTIRE | ODA_SELECT) & pdis->itemAction)
    {
        if (fSelected)
        {
            // Select the appropriate text colors
            crText = SetTextColor(pdis->hDC, rgbHiliteText);
            crBack = SetBkColor(pdis->hDC, rgbHiliteColor);
        }

        ptgrc = mptagtgrc + pdis->itemData;

        TextFromTag(ptgrc, achBuf, sizeof(achBuf));

        size = pdis->rcItem.bottom - pdis->rcItem.top - 1;

        ExtTextOutA(pdis->hDC,
                    pdis->rcItem.left + size,
                    pdis->rcItem.top+1,
                    ETO_OPAQUE|ETO_CLIPPED,
                    &pdis->rcItem,
                    achBuf,
                    lstrlenA(achBuf),
                    NULL);

        if (ptgrc->fEnabled)
        {
            BitBlt(pdis->hDC,
                   pdis->rcItem.left,
                   pdis->rcItem.top+1,
                   size,
                   size,
                   g_hdcCheck,
                   0,
                   0,
                   SRCCOPY);
        }

        // Restore original colors if we changed them above.
        if(fSelected)
        {
            SetTextColor(pdis->hDC, crText);
            SetBkColor(pdis->hDC,   crBack);
        }
    }

    if((ODA_FOCUS & pdis->itemAction) || (ODS_FOCUS & pdis->itemState))
        DrawFocusRect(pdis->hDC, &pdis->rcItem);
}

//+---------------------------------------------------------------------------
//
//  Function:   CompareTraceItem
//
//  Synopsis:   Compare two items in the listbox for sorting.
//
//----------------------------------------------------------------------------

BOOL
CompareTraceItem(LPCOMPAREITEMSTRUCT pcis)
{
    int    retVal = 0;
    TGRC * ptgrc1 = NULL;
    TGRC * ptgrc2 = NULL;
    char * psz1, *psz2;
    char   achBuf[TAG_STRBUF_SIZE];
    int    cch = -1;

    if (pcis->itemID1 != -1)
    {
        Assert((TRACETAG)pcis->itemData1 > tagNull && (TRACETAG)pcis->itemData1 <= tagMac);

        ptgrc1 = mptagtgrc + pcis->itemData1;
        psz1   = ptgrc1->szOwner;
    }
    else
    {
        //
        // This is used when the user types into the textbox.
        //
        GetWindowTextA(pcis->hwndItem, achBuf, sizeof(achBuf));

        psz1 = achBuf;

        cch = lstrlenA(psz1);
    }

    if ((TRACETAG)pcis->itemData2 > tagNull && (TRACETAG)pcis->itemData2 <= tagMac)
    {
        ptgrc2 = mptagtgrc + pcis->itemData2;
        psz2   = ptgrc2->szOwner;
    }
    else
    {
        psz2 = ""; // Shouldn't get here - just don't crash if we do.
    }

    if (   ptgrc1
        && ptgrc2
        && ptgrc1->TestFlag(TGRC_FLAG_SORTFIRST) !=
           ptgrc2->TestFlag(TGRC_FLAG_SORTFIRST))
    {
        retVal = ptgrc1->TestFlag(TGRC_FLAG_SORTFIRST) ? -1 : 1;
    }
    else
    {
        retVal = CompareStringA(LOCALE_USER_DEFAULT,
                                NORM_IGNORECASE,
                                psz1, cch,
                                psz2, cch) - 2;

        if (retVal == 0 && ptgrc1 && ptgrc2)
        {
            retVal = CompareStringA(LOCALE_USER_DEFAULT,
                                    NORM_IGNORECASE,
                                    ptgrc1->szDescrip, -1,
                                    ptgrc2->szDescrip, -1) - 2;

        }
    }

    return (BOOL)retVal;
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
BOOL CALLBACK
DlgTraceEtc(HWND hwndDlg, UINT wm, WPARAM wparam, LPARAM lparam)
{
    DBGTHREADSTATE *   pts = DbgGetThreadState();
    TRACETAG        tag;
    TGRC *          ptgrc;
    DWORD           wNew;
    BOOL            fEnable;        // enable all
    TGRC_FLAG       tf;
    BOOL            fTrace;
    HWND            hwndListBox;
    BOOL            fInvalidate = FALSE;

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
        Assert(hwndListBox);
        SetRGBValues();
        SendMessageA(hwndListBox, CB_SETCURSEL, 0, 0);
        SendMessageA(
                hwndDlg,
                WM_COMMAND,
                GET_WM_COMMAND_MPS(tmcListbox, hwndListBox, CBN_SELCHANGE));


        SetForegroundWindow(hwndDlg);

        if (!g_hdcCheck)
        {
            HDC hdc = GetDC(NULL);
            g_hdcCheck = CreateCompatibleDC(hdc);
            ReleaseDC(NULL, hdc);
        }

        if (!g_hbmpCheck)
        {
            Assert(g_hdcCheck);

            g_hbmpCheck = (HBITMAP)LoadImageA(NULL,
                                              (LPCSTR)OBM_CHECK,
                                              IMAGE_BITMAP,
                                              0, 0,
                                              LR_LOADTRANSPARENT);

            SelectObject(g_hdcCheck, g_hbmpCheck);
        }

        break;

    case WM_SYSCOLORCHANGE:
        SetRGBValues();
        InvalidateRect(hwndDlg, NULL, FALSE);
        break;

    case WM_DRAWITEM:
        if (GET_WM_COMMAND_ID(wparam, lparam) == tmcListbox)
        {
            DrawTraceItem((DRAWITEMSTRUCT*)lparam);
        }
        return TRUE;

    case WM_COMPAREITEM:
        if (GET_WM_COMMAND_ID(wparam, lparam) == tmcListbox)
        {
            return CompareTraceItem((COMPAREITEMSTRUCT*)lparam);
        }
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

            fInvalidate = TRUE;

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
                EnableWindow(GetDlgItem(hwndDlg, tmcBreak), FALSE);
                break;
            }

            // re-enable it always just in case we disabled it previously
            EnableWindow(GetDlgItem(hwndDlg, tmcEnabled),  TRUE);

            ptgrc = mptagtgrc + tag;

            {
                char achBuf[TAG_STRBUF_SIZE];

                TextFromTag(ptgrc, achBuf, sizeof(achBuf));

                SetWindowTextA(GetDlgItem(hwndDlg, tmcListbox), achBuf);
            }

            if (GET_WM_COMMAND_CMD(wparam, lparam) == CBN_DBLCLK)
            {
                ptgrc->fEnabled = !ptgrc->fEnabled;
                pts->fDirtyDlg = TRUE;
                fInvalidate = TRUE;
            }

            CheckDlgButton(hwndDlg, tmcEnabled, ptgrc->fEnabled);
            CheckDlgButton(hwndDlg, tmcDisk, ptgrc->TestFlag(TGRC_FLAG_DISK));
            CheckDlgButton(hwndDlg, tmcBreak, ptgrc->TestFlag(TGRC_FLAG_BREAK));
            fTrace = (ptgrc->tgty == tgtyTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcDisk),  fTrace);
            EnableWindow(GetDlgItem(hwndDlg, tmcBreak), fTrace);
            break;

        case tmcEnabled:
        case tmcDisk:
        case tmcBreak:
            pts->fDirtyDlg = TRUE;
            fInvalidate = TRUE;

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

        if (fInvalidate)
        {
            InvalidateRect(GetDlgItem(hwndDlg, tmcListbox), NULL, FALSE);
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
            DbgExRestoreDefaultDebugState();
    }

    DeleteDC(g_hdcCheck);
    DeleteObject(g_hbmpCheck);

    g_hdcCheck = NULL;
    g_hbmpCheck = NULL;

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
    Assert(hwndListbox);

    lresult = SendMessageA(hwndListbox, CB_GETCURSEL, 0, 0);

    // the selection can be -ve if the editbox is empty !
    if (lresult == CB_ERR)
        return tagNull;

    Assert(lresult >= 0);
    lresult = SendMessageA(hwndListbox, CB_GETITEMDATA, lresult, 0);
    Assert(tagMin <= lresult && lresult < tagMac);
    return (WORD) lresult;
}


