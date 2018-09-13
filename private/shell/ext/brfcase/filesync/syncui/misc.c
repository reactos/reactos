//---------------------------------------------------------------------------
//
// Copyright (c) Microsoft Corporation 1993-1994
//
// File: misc.c
//
//  This file contains miscellaneous dialog code
//
// History:
//  08-06-93 ScottH     Transferred from twin code
//
//---------------------------------------------------------------------------

#include "brfprv.h"     // common headers

#include "res.h"


typedef struct _MB_BUTTONS
    {
    UINT id;        // id
    UINT ids;       // string ID
    } MB_BUTTONS, * PMB_BUTTONS;

typedef struct _BTNSTYLE
    {
    UINT cButtons;
    MB_BUTTONS rgmbb[4];
    } BTNSTYLE;


//---------------------------------------------------------------------------
// Control manipulation stuff
//---------------------------------------------------------------------------


// Flags for SNAPCTL
#define SCF_ANCHOR      0x0001
#define SCF_VCENTER     0x0002
#define SCF_BOTTOM      0x0004
#define SCF_TOP         0x0008
#define SCF_SNAPLEFT    0x0010
#define SCF_SNAPRIGHT   0x0020

typedef struct tagSNAPCTL
    {
    UINT    idc;
    UINT    uFlags;
    } SNAPCTL, * PSNAPCTL;


/*----------------------------------------------------------
Purpose: Moves a control
Returns: HDWP
Cond:    --
*/
HDWP PRIVATE SlideControlPos(
    HDWP hdwp,
    HWND hDlg,
    UINT idc,
    int cx,
    int cy)
    {
    HWND hwndPos = GetDlgItem(hDlg, idc);
    RECT rcPos;

    GetWindowRect(hwndPos, &rcPos);
    MapWindowRect(HWND_DESKTOP, hDlg, &rcPos);
    return DeferWindowPos(hdwp, hwndPos, NULL,
                          rcPos.left + cx, rcPos.top + cy,
                          0, 0,
                          SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
    }


/*----------------------------------------------------------
Purpose: Aligns a list of controls, relative to an "anchor"
         control.

         Only one anchor control is supported; the first control
         designated as anchor in the list is selected.

Returns: --
Cond:    --
*/
void PRIVATE SnapControls(
    HWND hwnd,
    SNAPCTL const * psnap,
    UINT csnap)
    {
    HWND hwndAnchor;
    UINT i;
    SNAPCTL const * psnapStart = psnap;
    HDWP hdwp;
    RECT rcAnchor;
    int yCenter;

    ASSERT(psnap);

    // Find the anchor control
    for (i = 0; i < csnap; i++, psnap++)
        {
        if (IsFlagSet(psnap->uFlags, SCF_ANCHOR))
            {
            hwndAnchor = GetDlgItem(hwnd, psnap->idc);
            break;
            }
        }

    if (i == csnap)
        return;     // No anchor control!

    GetWindowRect(hwndAnchor, &rcAnchor);
    yCenter = rcAnchor.top + (rcAnchor.bottom - rcAnchor.top)/2;

    hdwp = BeginDeferWindowPos(csnap-1);

    if (hdwp)
        {
        RECT rc;
        UINT uFlags;
        HWND hwndPos;

        for (i = 0, psnap = psnapStart; i < csnap; i++, psnap++)
            {
            uFlags = psnap->uFlags;
            if (IsFlagSet(uFlags, SCF_ANCHOR))
                continue;       // skip anchor

            hwndPos = GetDlgItem(hwnd, psnap->idc);
            GetWindowRect(hwndPos, &rc);

            if (IsFlagSet(uFlags, SCF_VCENTER))
                {
                // Vertically match the center of this control with
                // the center of the anchor
                rc.top += yCenter - (rc.top + (rc.bottom - rc.top)/2);
                }
            else if (IsFlagSet(uFlags, SCF_TOP))
                {
                // Vertically match the top of this control with
                // the top of the anchor
                rc.top += rcAnchor.top - rc.top;
                }
            else if (IsFlagSet(uFlags, SCF_BOTTOM))
                {
                // Vertically match the bottom of this control with
                // the bottom of the anchor
                rc.top += rcAnchor.bottom - rc.bottom;
                }

            if (IsFlagSet(uFlags, SCF_SNAPLEFT))
                {
                // Snap the control so it is abut to the left side
                // of the anchor control
                rc.left += rcAnchor.left - rc.right;
                }
            else if (IsFlagSet(uFlags, SCF_SNAPRIGHT))
                {
                // Snap the control so it is abut to the right side
                // of the anchor control
                rc.left += rcAnchor.right - rc.left;
                }

            // Move control
            MapWindowRect(HWND_DESKTOP, hwnd, &rc);
            hdwp = DeferWindowPos(hdwp, hwndPos, NULL,
                                  rc.left, rc.top, 0, 0,
                                  SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOSIZE);
            }
        EndDeferWindowPos(hdwp);
        }
    }


//---------------------------------------------------------------------------
// Abort event stuff
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Creates an abort event.

Returns: TRUE on success
Cond:    --
*/
BOOL PUBLIC AbortEvt_Create(
    PABORTEVT * ppabortevt,
    UINT uFlags)
    {
    PABORTEVT this;

    ASSERT(ppabortevt);

    if (IsFlagSet(uFlags, AEF_SHARED))
        this = SharedAllocType(ABORTEVT);
    else
        this = GAllocType(ABORTEVT);

    if (this)
        {
        this->uFlags = uFlags;
        }

    *ppabortevt = this;

    return NULL != this;
    }


/*----------------------------------------------------------
Purpose: Destroys an abort event.

Returns: --
Cond:    --
*/
void PUBLIC AbortEvt_Free(
    PABORTEVT this)
    {
    if (this)
        {
        if (IsFlagSet(this->uFlags, AEF_SHARED))
            SharedFree(&this);
        else
            GFree(this);
        }
    }


/*----------------------------------------------------------
Purpose: Sets the abort event.

Returns: Returns the previous abort event.
Cond:    --
*/
BOOL PUBLIC AbortEvt_Set(
    PABORTEVT this,
    BOOL bAbort)
    {
    BOOL bRet;

    if (this)
        {
        bRet = IsFlagSet(this->uFlags, AEF_ABORT);

        if (bAbort)
            {
            TRACE_MSG(TF_GENERAL, TEXT("Setting abort event"));
            SetFlag(this->uFlags, AEF_ABORT);
            }
        else
            {
            TRACE_MSG(TF_GENERAL, TEXT("Clearing abort event"));
            ClearFlag(this->uFlags, AEF_ABORT);
            }
        }
    else
        bRet = FALSE;

    return bRet;
    }


/*----------------------------------------------------------
Purpose: Queries the abort event

Returns: the current abort event (TRUE or FALSE)
Cond:    --
*/
BOOL PUBLIC AbortEvt_Query(
    PABORTEVT this)
    {
    BOOL bRet;

    if (this)
        {
        bRet = IsFlagSet(this->uFlags, AEF_ABORT);

#ifdef DEBUG
        if (bRet)
            TRACE_MSG(TF_GENERAL, TEXT("Abort is set!"));
#endif
        }
    else
        bRet = FALSE;

    return bRet;
    }


//---------------------------------------------------------------------------
// Progress bar stuff
//---------------------------------------------------------------------------

#define MSECS_PER_SEC   1000

#define WM_QUERYABORT   (WM_APP + 1)


/*----------------------------------------------------------
Purpose: Progress dialog during reconciliations
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK UpdateProgressProc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    PUPDBAR this = (PUPDBAR)GetWindowLongPtr(hDlg, DWLP_USER);

    switch (wMsg)
        {
    case WM_INITDIALOG:
        SetWindowLongPtr(hDlg, DWLP_USER, lParam);
        this = (PUPDBAR)lParam;

        if (IsFlagSet(this->uFlags, UB_NOCANCEL))
            {
            ShowWindow(GetDlgItem(hDlg, IDCANCEL), SW_HIDE);
            EnableWindow(GetDlgItem(hDlg, IDCANCEL), FALSE);
            }
        break;

    case WM_COMMAND:
        switch (wParam)
            {
        case IDCANCEL:
            AbortEvt_Set(this->pabortevt, TRUE);
            break;
            }
        break;

    case WM_QUERYABORT:
        if (GetTickCount() >= this->dwTickShow &&
            0 != this->dwTickShow)
            {
            if (this->hcurSav)
                {
                SetCursor(this->hcurSav);
                this->hcurSav = NULL;
                }

            ShowWindow(hDlg, SW_SHOW);
            UpdateWindow(hDlg);
            this->dwTickShow = 0;
            }
        break;

    default:
        return FALSE;
        }

    return TRUE;
    }



/*----------------------------------------------------------
Purpose: Displays the update progress bar dialog

Returns: dialog handle to a modeless dialog
         NULL if dialog couldn't be created

Cond:    Call UpdBar_Kill when finished
*/
HWND PUBLIC UpdBar_Show(
    HWND hwndParent,
    UINT uFlags,        // UB_*
    UINT nSecs)         // Valid only if UB_TIMER set
    {
    HWND hdlg = NULL;
    PUPDBAR this;

    // Create and show the progress dialog
    //
    this = GAlloc(sizeof(*this));
    if (this)
        {
        // (It is okay if this fails--it just means we ignore the Cancel button)
        AbortEvt_Create(&this->pabortevt, AEF_DEFAULT);

        this->hwndParent = hwndParent;
        this->uFlags = uFlags;
        hdlg = CreateDialogParam(g_hinst, MAKEINTRESOURCE(IDD_PROGRESS),
            hwndParent, UpdateProgressProc, (LPARAM)(PUPDBAR)this);

        if (!hdlg)
            {
            GFree(this);
            }
        else
            {
            UpdBar_SetAvi(hdlg, uFlags);

            if (IsFlagClear(uFlags, UB_NOSHOW))
                EnableWindow(hwndParent, FALSE);

            if (IsFlagSet(uFlags, UB_TIMER))
                {
                this->dwTickShow = GetTickCount() + (nSecs * MSECS_PER_SEC);
                this->hcurSav = SetCursorRemoveWigglies(LoadCursor(NULL, IDC_WAIT));
                }
            else
                {
                this->dwTickShow = 0;
                this->hcurSav = NULL;

                if (IsFlagClear(uFlags, UB_NOSHOW))
                    {
                    ShowWindow(hdlg, SW_SHOW);
                    UpdateWindow(hdlg);
                    }
                }
            }
        }

    return hdlg;
    }


/*----------------------------------------------------------
Purpose: Destroy the update progress bar
Returns: --
Cond:    --
*/
void PUBLIC UpdBar_Kill(
    HWND hdlg)
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        PUPDBAR this = (PUPDBAR)GetWindowLongPtr(hdlg, DWLP_USER);

        ASSERT(this);
        if (this)
            {
            if (this->hcurSav)
                SetCursor(this->hcurSav);

            if (IsWindow(this->hwndParent))
                EnableWindow(this->hwndParent, TRUE);
            GFree(this);
            }
        DestroyWindow(hdlg);
        }
    }


/*----------------------------------------------------------
Purpose: Set the progress bar range.  Reset the position to 0
Returns: --
Cond:    --
*/
void PUBLIC UpdBar_SetRange(
    HWND hdlg,
    WORD wRangeMax)
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETPOS, 0, 0);
        SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETRANGE, 0, MAKELONG(0, wRangeMax));
        }
    }


/*----------------------------------------------------------
Purpose: Increment the position of progress bar
Returns: --
Cond:    --
*/
void PUBLIC UpdBar_DeltaPos(
    HWND hdlg,
    WORD wdelta)
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_DELTAPOS, wdelta, 0);
        }
    }


/*----------------------------------------------------------
Purpose: Set the position of progress bar
Returns: --
Cond:    --
*/
void PUBLIC UpdBar_SetPos(
    HWND hdlg,
    WORD wPos)
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        SendDlgItemMessage(hdlg, IDC_PROGRESS, PBM_SETPOS, wPos, 0);
        }
    }


/*----------------------------------------------------------
Purpose: Set the current name we're updating in the progress
         bar.
Returns: --
Cond:    --
*/
void PUBLIC UpdBar_SetName(
    HWND hdlg,
    LPCTSTR pszName)
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        HWND hwndName = GetDlgItem(hdlg, IDC_NAME);

        Static_SetText(hwndName, pszName);
        }
    }


/*----------------------------------------------------------
Purpose: Set the current name we're updating in the progress
         bar.
Returns: --
Cond:    --
*/
void PUBLIC UpdBar_SetDescription(
    HWND hdlg,
    LPCTSTR psz)
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        HWND hwndName = GetDlgItem(hdlg, IDC_TONAME);

        Static_SetText(hwndName, psz);
        }
    }


/*----------------------------------------------------------
Purpose: Get the window handle of the progress status text.
Returns: --
Cond:    --
*/
HWND PUBLIC UpdBar_GetStatusWindow(
    HWND hdlg)
    {
    HWND hwnd;

    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        hwnd = GetDlgItem(hdlg, IDC_TEXT);
    else
        hwnd = NULL;

    return hwnd;
    }


/*----------------------------------------------------------
Purpose: Returns a pointer to the abort event owned by this
         progress window.

Returns: pointer to abort event or NULL

Cond:    --
*/
PABORTEVT PUBLIC UpdBar_GetAbortEvt(
    HWND hdlg)
    {
    PABORTEVT pabortevt = NULL;

    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        PUPDBAR this;

        this = (PUPDBAR)GetWindowLongPtr(hdlg, DWLP_USER);
        if (this)
            {
            pabortevt = this->pabortevt;
            }
        }

    return pabortevt;
    }


/*----------------------------------------------------------
Purpose: Sets the animate control to play the avi file designated
         by the UB_ flags

Returns: --
Cond:    --
*/
void PUBLIC UpdBar_SetAvi(
    HWND hdlg,
    UINT uFlags)    // UB_*
    {
    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        UINT ida;
        UINT ids;
        HWND hwndAvi = GetDlgItem(hdlg, IDC_ANIMATE);
        TCHAR sz[MAXBUFLEN];
        RECT rc;


        if (IsFlagClear(uFlags, UB_NOSHOW))
            {
            SetWindowRedraw(hdlg, FALSE);

            // Is the window visible yet?
            if (IsFlagSet(GetWindowLong(hdlg, GWL_STYLE), WS_VISIBLE))
                {
                // Yes; select just the upper area of the progress bar to
                // repaint
                int cy;

                GetWindowRect(GetDlgItem(hdlg, IDC_NAME), &rc);
                MapWindowPoints(HWND_DESKTOP, hdlg, (LPPOINT)&rc, 1);
                cy = rc.top;
                GetClientRect(hdlg, &rc);
                rc.bottom = cy;
                }
            else
                {
                // No
                GetWindowRect(hdlg, &rc);
                MapWindowPoints(HWND_DESKTOP, hdlg, (LPPOINT)&rc, 2);
                }
            }

        if (IsFlagSet(uFlags, UB_NOPROGRESS))
            {
            ShowWindow(GetDlgItem(hdlg, IDC_PROGRESS), SW_HIDE);
            }
        else
            {
            ShowWindow(GetDlgItem(hdlg, IDC_PROGRESS), SW_SHOW);
            }

        // Special text when checking?
        if (IsFlagSet(uFlags, UB_CHECKAVI))
            {
            // Yes
            SetDlgItemText(hdlg, IDC_TONAME, SzFromIDS(IDS_MSG_CHECKING, sz, ARRAYSIZE(sz)));
            }
        else
            {
            // No
            SetDlgItemText(hdlg, IDC_TONAME, TEXT(""));
            }

        // Run AVI?
        if (uFlags & (UB_CHECKAVI | UB_UPDATEAVI))
            {
            // Yes
#pragma data_seg(DATASEG_READONLY)
            static const SNAPCTL rgsnap[] = {
                { IDC_ICON1, SCF_BOTTOM | SCF_SNAPLEFT },
                { IDC_ANIMATE, SCF_ANCHOR },
                { IDC_ICON2, SCF_BOTTOM | SCF_SNAPRIGHT },
                };
#pragma data_seg()

            if (IsFlagSet(uFlags, UB_CHECKAVI))
                {
                ida = IDA_CHECK;
                ids = IDS_CAP_CHECKING;
                }
            else if (IsFlagSet(uFlags, UB_UPDATEAVI))
                {
                ida = IDA_UPDATE;
                ids = IDS_CAP_UPDATING;
                }
            else
                ASSERT(0);

            SetWindowText(hdlg, SzFromIDS(ids, sz, ARRAYSIZE(sz)));
            Animate_Open(hwndAvi, MAKEINTRESOURCE(ida));

            // Snap the icons on either side to the animation
            // control
            SnapControls(hdlg, rgsnap, ARRAYSIZE(rgsnap));

            Animate_Play(hwndAvi, 0, -1, -1);
            }

        // Don't bother setting the redraw if we're never going to show
        // the progress bar
        if (IsFlagClear(uFlags, UB_NOSHOW))
            {
            SetWindowRedraw(hdlg, TRUE);
            InvalidateRect(hdlg, &rc, TRUE);
            UpdateWindow(hdlg);
            }
        }
    }


/*----------------------------------------------------------
Purpose: Yield, and check if user aborted
Returns: TRUE to abort
         FALSE to continue
Cond:    --
*/
BOOL PUBLIC UpdBar_QueryAbort(
    HWND hdlg)
    {
    BOOL bAbort = FALSE;

    ASSERT(IsWindow(hdlg));

    if (IsWindow(hdlg))
        {
        MSG msg;
        PUPDBAR this;

        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
            {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
            }

        /*
         * Don't use SendMessage() here to ask hdlg if reconciliation has been
         * aborted.  hdlg has typically been created in a different thread.
         * hdlg's creator thread may already be blocked in the sync engine.  We
         * must avoid inter-thread SendMessage() to avoid a deadlock on the
         * sync engine's briefcase critical section.  The sync engine is not
         * reentrant.
         */

        PostMessage(hdlg, WM_QUERYABORT, 0, 0);

        this = (PUPDBAR)GetWindowLongPtr(hdlg, DWLP_USER);

        if (this)
            {
            bAbort = AbortEvt_Query(this->pabortevt);
            }
        }

    return bAbort;
    }


//---------------------------------------------------------------------------
// Confirm Replace dialog
//---------------------------------------------------------------------------

// This is the private data structure for the dialog
typedef struct
    {
    UINT uFlags;        // CRF_*
    TCHAR szDesc[MAXBUFLEN+MAXPATHLEN];
    TCHAR szInfoExisting[MAXMEDLEN];
    TCHAR szInfoOther[MAXMEDLEN];
    HICON hicon;
    } CONFIRMREPLACE;


/*----------------------------------------------------------
Purpose: Confirm replace dialog
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK ConfirmReplace_Proc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    switch (wMsg)
        {
    case WM_INITDIALOG:
        {
        CONFIRMREPLACE * pcr = (CONFIRMREPLACE *)lParam;
        UINT i;
        UINT cButtons;
        MB_BUTTONS const * pmbb;
#pragma data_seg(DATASEG_READONLY)
        static UINT const rgidc[4] = { IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4 };
        static BTNSTYLE const btnstyleSingle =
                // (List buttons backwards)
                { 2, { { IDNO,  IDS_NO },
                       { IDYES, IDS_YES },
                     } };

        static BTNSTYLE const btnstyleMulti =
                // (List buttons backwards)
                { 4, { { IDCANCEL,      IDS_CANCEL },
                       { IDNO,          IDS_NO },
                       { IDC_YESTOALL,  IDS_YESTOALL },
                       { IDYES,         IDS_YES },
                     } };
#pragma data_seg()

        Static_SetText(GetDlgItem(hDlg, IDC_DESC), pcr->szDesc);

        if (IsFlagClear(pcr->uFlags, CRF_FOLDER))
            {
            Static_SetText(GetDlgItem(hDlg, IDC_EXISTING), pcr->szInfoExisting);
            Static_SetText(GetDlgItem(hDlg, IDC_OTHER), pcr->szInfoOther);

            Static_SetIcon(GetDlgItem(hDlg, IDC_ICON_EXISTING), pcr->hicon);
            Static_SetIcon(GetDlgItem(hDlg, IDC_ICON_OTHER), pcr->hicon);
            }

        // Set the IDs and strings of used buttons
        if (IsFlagSet(pcr->uFlags, CRF_MULTI))
            {
            cButtons = btnstyleMulti.cButtons;
            pmbb = btnstyleMulti.rgmbb;
            }
        else
            {
            cButtons = btnstyleSingle.cButtons;
            pmbb = btnstyleSingle.rgmbb;
            }

        for (i = 0; i < cButtons; i++)
            {
            TCHAR sz[MAXMEDLEN];
            HWND hwnd = GetDlgItem(hDlg, rgidc[i]);

            LoadString(g_hinst, pmbb[i].ids, sz, ARRAYSIZE(sz));
            SetWindowLongPtr(hwnd, GWLP_ID, pmbb[i].id);
            SetWindowText(hwnd, sz);
            }
        // Disable unused buttons
        for (; i < ARRAYSIZE(rgidc); i++)
            {
            HWND hwnd = GetDlgItem(hDlg, rgidc[i]);

            EnableWindow(hwnd, FALSE);
            ShowWindow(hwnd, SW_HIDE);
            }
        }
        break;

    case WM_COMMAND:
        switch (wParam)
            {
        case IDCANCEL:
        case IDYES:
        case IDC_YESTOALL:
        case IDNO:
            EndDialog(hDlg, wParam);
            break;
            }
        break;

    default:
        return FALSE;
        }

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Brings up the replace confirmation dialog.

Returns: IDYES, IDC_YESTOALL, IDNO or IDCANCEL
Cond:    --
*/
int PUBLIC ConfirmReplace_DoModal(
    HWND hwndOwner,
    LPCTSTR pszPathExisting,
    LPCTSTR pszPathOther,
    UINT uFlags)                // CRF_*
    {
    INT_PTR idRet;
    CONFIRMREPLACE * pcr;

    pcr = GAlloc(sizeof(*pcr));
    if (pcr)
        {
        LPTSTR pszMsg;
        DWORD dwAttrs = GetFileAttributes(pszPathExisting);

        pcr->uFlags = uFlags;

        // Is this replacing a folder?
        if (IsFlagSet(dwAttrs, FILE_ATTRIBUTE_DIRECTORY))
            {
            // Yes
            if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(IDS_MSG_ConfirmFolderReplace),
                PathFindFileName(pszPathOther)))
                {
                lstrcpy(pcr->szDesc, pszMsg);
                GFree(pszMsg);
                }
            else
                *pcr->szDesc = 0;

            SetFlag(pcr->uFlags, CRF_FOLDER);

            idRet = DoModal(hwndOwner, ConfirmReplace_Proc, IDD_REPLACE_FOLDER, (LPARAM)pcr);
            }
        else
            {
            // No
            UINT ids;
            FileInfo * pfi;

            if (SUCCEEDED(FICreate(pszPathExisting, &pfi, FIF_ICON)))
                {
                pcr->hicon = pfi->hicon;

                FIGetInfoString(pfi, pcr->szInfoExisting, ARRAYSIZE(pcr->szInfoExisting));

                pfi->hicon = NULL;      // (keep icon around)
                FIFree(pfi);
                }

            if (SUCCEEDED(FICreate(pszPathOther, &pfi, FIF_DEFAULT)))
                {
                FIGetInfoString(pfi, pcr->szInfoOther, ARRAYSIZE(pcr->szInfoOther));
                FIFree(pfi);
                }

            if (IsFlagSet(dwAttrs, FILE_ATTRIBUTE_READONLY))
                {
                ids = IDS_MSG_ConfirmFileReplace_RO;
                }
            else if (IsFlagSet(dwAttrs, FILE_ATTRIBUTE_SYSTEM))
                {
                ids = IDS_MSG_ConfirmFileReplace_Sys;
                }
            else
                {
                ids = IDS_MSG_ConfirmFileReplace;
                }

            if (ConstructMessage(&pszMsg, g_hinst, MAKEINTRESOURCE(ids),
                PathFindFileName(pszPathOther)))
                {
                lstrcpy(pcr->szDesc, pszMsg);
                GFree(pszMsg);
                }
            else
                *pcr->szDesc = 0;

            ClearFlag(pcr->uFlags, CRF_FOLDER);

            idRet = DoModal(hwndOwner, ConfirmReplace_Proc, IDD_REPLACE_FILE, (LPARAM)pcr);

            if (pcr->hicon)
                DestroyIcon(pcr->hicon);
            }
        GFree(pcr);
        }
    else
        {
        idRet = -1;     // Out of memory
        }
    return (int)idRet;
    }


//---------------------------------------------------------------------------
// Introduction dialog
//---------------------------------------------------------------------------


/*----------------------------------------------------------
Purpose: Intro dialog
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK Intro_Proc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    NMHDR  *lpnm;

    switch (wMsg)
        {
    case WM_INITDIALOG:
        break;

    case WM_NOTIFY:
        lpnm = (NMHDR  *)lParam;
        switch(lpnm->code)
            {
        case PSN_SETACTIVE: {
            // Only allow the Finish button.  The user cannot go back and
            // change the settings.
            HWND hwndCancel = GetDlgItem(GetParent(hDlg), IDCANCEL);

            PropSheet_SetWizButtons(GetParent(hDlg), PSWIZB_FINISH);

            // Hide cancel button
            EnableWindow(hwndCancel, FALSE);
            ShowWindow(hwndCancel, SW_HIDE);
            }
            break;

        case PSN_KILLACTIVE:
        case PSN_HELP:
        case PSN_WIZBACK:
        case PSN_WIZNEXT:
            break;

        default:
            return FALSE;
            }
        break;

    default:
        return FALSE;
        }

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Invoke the introduction wizard.

Returns: ID of button that terminated dialog
Cond:    --
*/
int PUBLIC Intro_DoModal(
    HWND hwndParent)
    {
    PROPSHEETPAGE psp = {
        sizeof(psp),
        PSP_DEFAULT | PSP_HIDEHEADER,
        g_hinst,
        MAKEINTRESOURCE(IDD_INTRO_WIZARD),
        NULL,           // hicon
        NULL,           // caption
        Intro_Proc,
        0,              // lParam
        NULL,           // pfnCallback
        NULL            // pointer to ref count
        };
    PROPSHEETHEADER psh = {
        sizeof(psh),
        PSH_WIZARD_LITE | PSH_WIZARD | PSH_PROPSHEETPAGE,     // (use ppsp field)
        hwndParent,
        g_hinst,
        0,              // hicon
        0,              // caption
        1,              // number of pages
        0,              // start page
        &psp
        };

    return (int)PropertySheet(&psh);
    }



//---------------------------------------------------------------------------
// MsgBox dialog
//---------------------------------------------------------------------------

typedef struct _MSGBOX
    {
    LPCTSTR pszText;
    LPCTSTR pszCaption;
    HICON  hicon;
    UINT   uStyle;
    } MSGBOX, * PMSGBOX;


/*----------------------------------------------------------
Purpose: Determines whether to resize the dialog and reposition
         the buttons to fit the text.

         The dialog is not resized any smaller than its initial
         size.

         The dialog is only resized vertically.

Returns: --
Cond:    --
*/
void PRIVATE MsgBox_Resize(
    HWND hDlg,
    LPCTSTR pszText,
    UINT cchText)
    {
    HDC hdc;
    HWND hwndText = GetDlgItem(hDlg, IDC_TEXT);

    hdc = GetDC(hwndText);
    if (hdc)
        {
        HFONT hfont = GetStockObject(DEFAULT_GUI_FONT);
        HFONT hfontSav = SelectFont(hdc, hfont);
        RECT rc;
        RECT rcOrg;

        // Determine new dimensions
        GetClientRect(hwndText, &rcOrg);
        rc = rcOrg;
        DrawTextEx(hdc, (LPTSTR)pszText, cchText, &rc, DT_CALCRECT | DT_WORDBREAK | DT_LEFT, NULL);

        SelectFont(hdc, hfontSav);
        ReleaseDC(hwndText, hdc);

        // Is the required size bigger?
        if (rc.bottom > rcOrg.bottom)
            {
            // Yes; resize the windows
            int cy = rc.bottom - rcOrg.bottom;
            int cyFudge = GetSystemMetrics(SM_CYCAPTION) + 2*GetSystemMetrics(SM_CYFIXEDFRAME);
            int cxFudge = 2*GetSystemMetrics(SM_CXFIXEDFRAME);
            HDWP hdwp = BeginDeferWindowPos(4);

            if (hdwp)
                {
                // Move Buttons
                hdwp = SlideControlPos(hdwp, hDlg, IDC_BUTTON1, 0, cy);
                hdwp = SlideControlPos(hdwp, hDlg, IDC_BUTTON2, 0, cy);
                hdwp = SlideControlPos(hdwp, hDlg, IDC_BUTTON3, 0, cy);

                // Resize Static Text
                hdwp = DeferWindowPos(hdwp, hwndText, GetDlgItem(hDlg, IDC_BUTTON3),
                                      0, 0,
                                      rc.right-rc.left, rc.bottom-rc.top,
                                      SWP_NOACTIVATE | SWP_NOMOVE);

                EndDeferWindowPos(hdwp);
                }

            // Resize Dialog
            GetClientRect(hDlg, &rc);
            SetWindowPos(hDlg, NULL, 0, 0,
                         rc.right-rc.left + cxFudge, rc.bottom-rc.top + cy + cyFudge,
                         SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOMOVE);
            }
        }
    }


/*----------------------------------------------------------
Purpose: MsgBox dialog
Returns: varies
Cond:    --
*/
INT_PTR CALLBACK MsgBox_Proc(
    HWND hDlg,
    UINT wMsg,
    WPARAM wParam,
    LPARAM lParam)
    {
    switch (wMsg)
        {
    case WM_INITDIALOG:
        {
        PMSGBOX pmsgbox = (PMSGBOX)lParam;
        UINT uStyle = pmsgbox->uStyle;
        UINT i;
        UINT imb = uStyle & MB_TYPEMASK;
        UINT cButtons;
        MB_BUTTONS const * pmbb;
#pragma data_seg(DATASEG_READONLY)
        static UINT const rgidc[3] = { IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3 };
        static BTNSTYLE const rgmbstyle[] = {
                // (List buttons backwards)
                // MB_OK
                { 1, { { IDOK,      IDS_OK },
                     } },
                // MB_OKCANCEL
                { 2, { { IDCANCEL,  IDS_CANCEL },
                       { IDOK,      IDS_OK },
                     } },
                // MB_ABORTRETRYIGNORE (not supported)
                { 1, { { IDOK,      IDS_OK },
                     } },
                // MB_YESNOCANCEL
                { 3, { { IDCANCEL,  IDS_CANCEL },
                       { IDNO,      IDS_NO },
                       { IDYES,     IDS_YES },
                     } },
                // MB_YESNO
                { 2, { { IDNO,      IDS_NO },
                       { IDYES,     IDS_YES },
                     } },
                // MB_RETRYCANCEL
                { 2, { { IDCANCEL,  IDS_CANCEL },
                       { IDRETRY,   IDS_RETRY },
                     } },
                };
#pragma data_seg()

        // Set the text
        if (pmsgbox->pszText)
            {
            Static_SetText(GetDlgItem(hDlg, IDC_TEXT), pmsgbox->pszText);

            // Resize and reposition the buttons if necessary
            MsgBox_Resize(hDlg, pmsgbox->pszText, lstrlen(pmsgbox->pszText));
            }
        if (pmsgbox->pszCaption)
            SetWindowText(hDlg, pmsgbox->pszCaption);

        // Use a custom icon?
        if (NULL == pmsgbox->hicon)
            {
            // No; use a system icon
            LPCTSTR pszIcon;

            if (IsFlagSet(uStyle, MB_ICONEXCLAMATION))
                pszIcon = IDI_EXCLAMATION;
            else if (IsFlagSet(uStyle, MB_ICONHAND))
                pszIcon = IDI_HAND;
            else if (IsFlagSet(uStyle, MB_ICONQUESTION))
                pszIcon = IDI_QUESTION;
            else
                pszIcon = IDI_ASTERISK;

            pmsgbox->hicon = LoadIcon(NULL, pszIcon);
            }
        Static_SetIcon(GetDlgItem(hDlg, IDC_MSGICON), pmsgbox->hicon);

        // Set the IDs and strings of used buttons
        cButtons = rgmbstyle[imb].cButtons;
        pmbb = rgmbstyle[imb].rgmbb;
        for (i = 0; i < cButtons; i++)
            {
            TCHAR sz[MAXMEDLEN];
            HWND hwnd = GetDlgItem(hDlg, rgidc[i]);

            LoadString(g_hinst, pmbb[i].ids, sz, ARRAYSIZE(sz));
            SetWindowLongPtr(hwnd, GWLP_ID, pmbb[i].id);
            SetWindowText(hwnd, sz);
            }
        // Disable unused buttons
        for (; i < ARRAYSIZE(rgidc); i++)
            {
            HWND hwnd = GetDlgItem(hDlg, rgidc[i]);

            EnableWindow(hwnd, FALSE);
            ShowWindow(hwnd, SW_HIDE);
            }
        }
        break;

    case WM_COMMAND:
        switch (wParam)
            {
        case IDOK:
        case IDCANCEL:
        case IDYES:
        case IDNO:
        case IDRETRY:
            EndDialog(hDlg, wParam);
            break;
            }
        break;

    default:
        return FALSE;
        }

    return TRUE;
    }


/*----------------------------------------------------------
Purpose: Invoke the introduction dialog.

Returns: ID of button that terminated dialog
Cond:    --
*/
int PUBLIC MsgBox(
    HWND hwndParent,
    LPCTSTR pszText,
    LPCTSTR pszCaption,
    HICON hicon,            // May be NULL
    UINT uStyle, ...)
    {
    INT_PTR iRet = -1;
    int ids;
    TCHAR szCaption[MAXPATHLEN];
    LPTSTR pszRet;
    va_list ArgList;

    va_start(ArgList, uStyle);

    pszRet = _ConstructMessageString(g_hinst, pszText, &ArgList);

    va_end(ArgList);

    if (pszRet)
        {
        // Is pszCaption a resource ID?
        if (0 == HIWORD(pszCaption))
            {
            // Yes; load it
            ids = LOWORD(pszCaption);
            SzFromIDS(ids, szCaption, ARRAYSIZE(szCaption));
            pszCaption = szCaption;
            }

        // Invoke dialog
        if (pszCaption)
            {
            MSGBOX msgbox = { pszRet, pszCaption, hicon, uStyle };
            iRet = DoModal(hwndParent, MsgBox_Proc, IDC_MSGBOX, (LPARAM)&msgbox);
            }
        LocalFree(pszRet);
        }

    return (int)iRet;
    }
